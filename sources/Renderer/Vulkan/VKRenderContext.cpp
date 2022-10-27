/*
 * VKRenderContext.cpp
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015-2019 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#include "VKRenderContext.h"
#include "VKCore.h"
#include "VKTypes.h"
#include "Memory/VKDeviceMemoryManager.h"
#include <LLGL/Platform/NativeHandle.h>
#include "../../Core/Helper.h"
#include "../TextureUtils.h"
#include <limits.h>
#include <set>


namespace LLGL
{


/* ----- Common ----- */

const std::uint32_t VKRenderContext::g_maxNumColorBuffers;

static const std::vector<const char*> g_deviceExtensions
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static VKPtr<VkImageView> NullVkImageView(const VKPtr<VkDevice>& device)
{
    return VKPtr<VkImageView>{ device, vkDestroyImageView };
}

static VKPtr<VkFramebuffer> NullVkFramebuffer(const VKPtr<VkDevice>& device)
{
    return VKPtr<VkFramebuffer>{ device, vkDestroyFramebuffer };
}

VKRenderContext::VKRenderContext(
    const VKPtr<VkInstance>&        instance,
    VkPhysicalDevice                physicalDevice,
    const VKPtr<VkDevice>&          device,
    VKDeviceMemoryManager&          deviceMemoryMngr,
    const SwapChainDescriptor&      desc,
    const std::shared_ptr<Surface>& surface)
:
    RenderContext            { desc                            },
    instance_                { instance                        },
    physicalDevice_          { physicalDevice                  },
    device_                  { device                          },
    deviceMemoryMngr_        { deviceMemoryMngr                },
    surface_                 { instance, vkDestroySurfaceKHR   },
    swapChain_               { device, vkDestroySwapchainKHR   },
    swapChainRenderPass_     { device                          },
    swapChainSamples_        { GetClampedSamples(desc.samples) },
    swapChainImageViews_     { NullVkImageView(device_),
                               NullVkImageView(device_),
                               NullVkImageView(device_)        },
    swapChainFramebuffers_   { NullVkFramebuffer(device_),
                               NullVkFramebuffer(device_),
                               NullVkFramebuffer(device_)      },
    secondaryRenderPass_     { device                          },
    depthStencilBuffer_      { device                          },
    colorBuffers_            { device, device, device          },
    imageAvailableSemaphore_ { device, vkDestroySemaphore      },
    renderFinishedSemaphore_ { device, vkDestroySemaphore      }
{
    SetOrCreateSurface(surface, desc.resolution, desc.fullscreen, nullptr);

    CreatePresentSemaphores();
    CreateGpuSurface();

    /* Pick image count for swap-chain and depth-stencil format */
    numSwapChainBuffers_ = PickSwapChainSize(desc.swapBuffers);
    depthStencilFormat_ = PickDepthStencilFormat(desc.depthBits, desc.stencilBits);

    /* Create Vulkan swap-chain and render pass */
    CreateSwapChainRenderPass();
    CreateSecondaryRenderPass();

    /* Create Vulkan swap-chain, depth-stencil buffer, and multisampling color buffers */
    CreateResolutionDependentResources(desc.resolution);
}

void VKRenderContext::Present()
{
    /* Initialize semaphores */
    VkSemaphore waitSemaphorse[] = { imageAvailableSemaphore_ };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphores[] = { renderFinishedSemaphore_ };

    /* Submit signal semaphore to graphics queue */
    VkSubmitInfo submitInfo;
    {
        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext                = nullptr;
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = waitSemaphorse;
        submitInfo.pWaitDstStageMask    = waitStages;
        submitInfo.commandBufferCount   = 0;
        submitInfo.pCommandBuffers      = nullptr;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = signalSemaphores;
    }
    auto result = vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
    VKThrowIfFailed(result, "failed to submit semaphore to Vulkan graphics queue");

    /* Present result on screen */
    VkSwapchainKHR swapChains[] = { swapChain_ };

    VkPresentInfoKHR presentInfo;
    {
        presentInfo.sType               = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext               = nullptr;
        presentInfo.waitSemaphoreCount  = 1;
        presentInfo.pWaitSemaphores     = signalSemaphores;
        presentInfo.swapchainCount      = 1;
        presentInfo.pSwapchains         = swapChains;
        presentInfo.pImageIndices       = &presentImageIndex_;
        presentInfo.pResults            = nullptr;
    }
    result = vkQueuePresentKHR(presentQueue_, &presentInfo);
    VKThrowIfFailed(result, "failed to present Vulkan graphics queue");

    /* Get image index for next presentation */
    AcquireNextPresentImage();
}

std::uint32_t VKRenderContext::GetSamples() const
{
    return swapChainSamples_;
}

Format VKRenderContext::GetColorFormat() const
{
    return VKTypes::Unmap(swapChainFormat_.format);
}

Format VKRenderContext::GetDepthStencilFormat() const
{
    return VKTypes::Unmap(depthStencilFormat_);
}

const RenderPass* VKRenderContext::GetRenderPass() const
{
    return (&swapChainRenderPass_);
}

bool VKRenderContext::SetVsyncInterval(std::uint32_t vsyncInterval)
{
    /* Recreate swap-chain with new vsnyc settings */
    if (vsyncInterval_ != vsyncInterval)
    {
        CreateSwapChain(GetResolution(), vsyncInterval);
        CreateSwapChainFramebuffers();
        vsyncInterval_ = vsyncInterval;
    }
    return true;
}

/* --- Extended functions --- */

bool VKRenderContext::HasDepthStencilBuffer() const
{
    return (depthStencilFormat_ != VK_FORMAT_UNDEFINED);
}

bool VKRenderContext::HasMultiSampling() const
{
    return (swapChainSamples_ > 1);
}


/*
 * ======= Private: =======
 */

bool VKRenderContext::ResizeBuffersPrimary(const Extent2D& resolution)
{
    /* Check if new resolution would actually change the swap-chain extent */
    if (swapChainExtent_.width  != resolution.width ||
        swapChainExtent_.height != resolution.height)
    {
        /* Wait until graphics queue is idle before resources are destroyed and recreated */
        vkQueueWaitIdle(graphicsQueue_);

        /* Recreate presenting semaphores and Vulkan surface */
        CreatePresentSemaphores();
        CreateGpuSurface();

        /* Recreate color and depth-stencil buffers */
        ReleaseRenderBuffers();
        CreateResolutionDependentResources(resolution);
    }
    return true;
}

void VKRenderContext::CreateGpuSemaphore(VKPtr<VkSemaphore>& semaphore)
{
    /* Create semaphore (no flags) */
    VkSemaphoreCreateInfo createInfo;
    {
        createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
    }
    auto result = vkCreateSemaphore(device_, &createInfo, nullptr, semaphore.ReleaseAndGetAddressOf());
    VKThrowIfFailed(result, "failed to create Vulkan semaphore");
}

void VKRenderContext::CreatePresentSemaphores()
{
    /* Create presentation semaphorse */
    CreateGpuSemaphore(imageAvailableSemaphore_);
    CreateGpuSemaphore(renderFinishedSemaphore_);
}

void VKRenderContext::CreateGpuSurface()
{
    /* All previous swap-chains must be destroyed before VkSurfaceKHR can be destroyed */
    swapChain_.Release();

    /* Get hantive handle from context surface */
    NativeHandle nativeHandle = {};
    GetSurface().GetNativeHandle(&nativeHandle, sizeof(nativeHandle));

    #if defined LLGL_OS_WIN32

    /* Setup Win32 surface descriptor */
    VkWin32SurfaceCreateInfoKHR createInfo;
    {
        createInfo.sType        = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.pNext        = nullptr;
        createInfo.flags        = 0;
        createInfo.hinstance    = GetModuleHandle(NULL);
        createInfo.hwnd         = nativeHandle.window;
    }
    auto result = vkCreateWin32SurfaceKHR(instance_, &createInfo, nullptr, surface_.ReleaseAndGetAddressOf());
    VKThrowIfFailed(result, "failed to create Win32 surface for Vulkan render context");

    #elif defined LLGL_OS_LINUX

    VkXlibSurfaceCreateInfoKHR createInfo;
    {
        createInfo.sType    = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
        createInfo.pNext    = nullptr;
        createInfo.flags    = 0;
        createInfo.dpy      = nativeHandle.display;
        createInfo.window   = nativeHandle.window;
    }
    auto result = vkCreateXlibSurfaceKHR(instance_, &createInfo, nullptr, surface_.ReleaseAndGetAddressOf());
    VKThrowIfFailed(result, "failed to create Xlib surface for Vulkan render context");

    #endif

    /* Query surface support details and pick surface format */
    surfaceSupportDetails_  = VKQuerySurfaceSupport(physicalDevice_, surface_);
    swapChainFormat_        = PickSwapSurfaceFormat(surfaceSupportDetails_.formats);
}

void VKRenderContext::CreateRenderPass(VKRenderPass& renderPass, bool isSecondary)
{
    RenderPassDescriptor renderPassDesc;
    {
        /* Pass number of samples to render pass descriptor */
        renderPassDesc.samples = swapChainSamples_;

        /* Determine load and store operations for primary and secondary render passes */
        auto loadOp     = (isSecondary ? AttachmentLoadOp::Load : AttachmentLoadOp::Undefined);
        auto storeOp    = AttachmentStoreOp::Store;

        /* Specify single color attachment */
        renderPassDesc.colorAttachments =
        {
            AttachmentFormatDescriptor{ GetColorFormat(), loadOp, storeOp }
        };

        /* Specify depth-stencil attachment */
        auto depthStencilFormat = GetDepthStencilFormat();

        if (IsDepthFormat(depthStencilFormat))
            renderPassDesc.depthAttachment = AttachmentFormatDescriptor{ depthStencilFormat, loadOp, storeOp };
        if (IsStencilFormat(depthStencilFormat))
            renderPassDesc.stencilAttachment = AttachmentFormatDescriptor{ depthStencilFormat, loadOp, storeOp };
    }
    renderPass.CreateVkRenderPass(device_, renderPassDesc);
}

void VKRenderContext::CreateSecondaryRenderPass()
{
    CreateRenderPass(secondaryRenderPass_, true);
}

void VKRenderContext::CreateSwapChainRenderPass()
{
    CreateRenderPass(swapChainRenderPass_, false);
}

void VKRenderContext::CreateSwapChain(const Extent2D& resolution, std::uint32_t vsyncInterval)
{
    /* Pick swap-chain extent by resolution */
    swapChainExtent_ = PickSwapExtent(surfaceSupportDetails_.caps, resolution);

    /* Get device queues for graphics and presentation */
    VkSurfaceKHR surface = surface_.Get();
    auto queueFamilyIndices = VKFindQueueFamilies(physicalDevice_, VK_QUEUE_GRAPHICS_BIT, &surface);

    vkGetDeviceQueue(device_, queueFamilyIndices.graphicsFamily, 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, queueFamilyIndices.presentFamily, 0, &presentQueue_);

    /* Pick swap-chain presentation mode (with v-sync parameters) */
    auto presentMode = PickSwapPresentMode(surfaceSupportDetails_.presentModes, vsyncInterval);

    /* Create swap-chain */
    VkSwapchainCreateInfoKHR createInfo;
    {
        createInfo.sType                        = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.pNext                        = nullptr;
        createInfo.flags                        = 0;
        createInfo.surface                      = surface_;
        createInfo.minImageCount                = numSwapChainBuffers_;
        createInfo.imageFormat                  = swapChainFormat_.format;
        createInfo.imageColorSpace              = swapChainFormat_.colorSpace;
        createInfo.imageExtent                  = swapChainExtent_;
        createInfo.imageArrayLayers             = 1;
        createInfo.imageUsage                   = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily)
        {
            createInfo.imageSharingMode         = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount    = 2;
            createInfo.pQueueFamilyIndices      = queueFamilyIndices.indices;
        }
        else
        {
            createInfo.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount    = 0;
            createInfo.pQueueFamilyIndices      = nullptr;
        }

        createInfo.preTransform                 = surfaceSupportDetails_.caps.currentTransform;
        createInfo.compositeAlpha               = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode                  = presentMode;
        createInfo.clipped                      = VK_TRUE;
        createInfo.oldSwapchain                 = VK_NULL_HANDLE;
    }
    auto result = vkCreateSwapchainKHR(device_, &createInfo, nullptr, swapChain_.ReleaseAndGetAddressOf());
    VKThrowIfFailed(result, "failed to create Vulkan swap-chain");

    /* Query swap-chain images */
    result = vkGetSwapchainImagesKHR(device_, swapChain_, &numSwapChainBuffers_, nullptr);
    VKThrowIfFailed(result, "failed to query number of Vulkan swap-chain images");

    result = vkGetSwapchainImagesKHR(device_, swapChain_, &numSwapChainBuffers_, swapChainImages_);
    VKThrowIfFailed(result, "failed to query Vulkan swap-chain images");

    /* Create swap-chain image views */
    CreateSwapChainImageViews();

    /* Acquire first image for presentation */
    AcquireNextPresentImage();
}

void VKRenderContext::CreateSwapChainImageViews()
{
    /* Initialize image-view descriptor */
    VkImageViewCreateInfo createInfo;
    {
        createInfo.sType                            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.pNext                            = nullptr;
        createInfo.flags                            = 0;
        createInfo.image                            = VK_NULL_HANDLE;
        createInfo.viewType                         = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format                           = swapChainFormat_.format;
        createInfo.components.r                     = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g                     = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b                     = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a                     = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel    = 0;
        createInfo.subresourceRange.levelCount      = 1;
        createInfo.subresourceRange.baseArrayLayer  = 0;
        createInfo.subresourceRange.layerCount      = 1;
    }

    /* Create all image views for the swap-chain */
    for (std::uint32_t i = 0; i < numSwapChainBuffers_; ++i)
    {
        /* Update image handle in Vulkan descriptor */
        createInfo.image = swapChainImages_[i];

        /* Create image view for framebuffer */
        VKPtr<VkImageView> imageView{ device_, vkDestroyImageView };
        {
            auto result = vkCreateImageView(device_, &createInfo, nullptr, imageView.ReleaseAndGetAddressOf());
            VKThrowIfFailed(result, "failed to create Vulkan swap-chain image view");
        }
        swapChainImageViews_[i] = std::move(imageView);
    }
}

void VKRenderContext::CreateSwapChainFramebuffers()
{
    /* Initialize image view attachments */
    VkImageView attachments[3] = {};

    std::uint32_t numAttachments    = 0;
    std::uint32_t attachmentDSV     = 0;
    std::uint32_t attachmentColor   = 0;
    std::uint32_t attachmentColorMS = 0;

    attachmentColor = numAttachments++;

    if (HasDepthStencilBuffer())
    {
        attachmentDSV = numAttachments++;
        attachments[attachmentDSV] = depthStencilBuffer_.GetVkImageView();
    }

    if (HasMultiSampling())
        attachmentColorMS = numAttachments++;

    /* Initialize framebuffer descriptor */
    VkFramebufferCreateInfo createInfo;
    {
        createInfo.sType            = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.pNext            = nullptr;
        createInfo.flags            = 0;
        createInfo.renderPass       = swapChainRenderPass_.GetVkRenderPass();
        createInfo.attachmentCount  = numAttachments;
        createInfo.pAttachments     = attachments;
        createInfo.width            = swapChainExtent_.width;
        createInfo.height           = swapChainExtent_.height;
        createInfo.layers           = 1;
    }

    /* Create all framebuffers for the swap-chain */
    for (std::uint32_t i = 0; i < numSwapChainBuffers_; ++i)
    {
        /* Update image view in Vulkan descriptor */
        attachments[attachmentColor] = swapChainImageViews_[i];

        if (HasMultiSampling())
            attachments[attachmentColorMS] = colorBuffers_[i].GetVkImageView();

        /* Create framebuffer */
        VKPtr<VkFramebuffer> framebuffer{ device_, vkDestroyFramebuffer };
        {
            auto result = vkCreateFramebuffer(device_, &createInfo, nullptr, framebuffer.ReleaseAndGetAddressOf());
            VKThrowIfFailed(result, "failed to create Vulkan swap-chain framebuffer");
        }
        swapChainFramebuffers_[i] = std::move(framebuffer);
    }
}

void VKRenderContext::CreateDepthStencilBuffer(const Extent2D& resolution)
{
    const auto sampleCountBits = VKTypes::ToVkSampleCountBits(swapChainSamples_);
    depthStencilBuffer_.Create(deviceMemoryMngr_, resolution, depthStencilFormat_, sampleCountBits);
}

void VKRenderContext::CreateColorBuffers(const Extent2D& resolution)
{
    /* Create VkImage objects for each swap-chain buffer */
    const auto sampleCountBits = VKTypes::ToVkSampleCountBits(swapChainSamples_);
    for (std::uint32_t i = 0; i < numSwapChainBuffers_; ++i)
        colorBuffers_[i].Create(deviceMemoryMngr_, resolution, swapChainFormat_.format, sampleCountBits);
}

void VKRenderContext::ReleaseRenderBuffers()
{
    depthStencilBuffer_.Release();
    if (HasMultiSampling())
    {
        for (std::uint32_t i = 0; i < numSwapChainBuffers_; ++i)
            colorBuffers_[i].Release();
    }
}

void VKRenderContext::CreateResolutionDependentResources(const Extent2D& resolution)
{
    CreateSwapChain(resolution, vsyncInterval_);

    if (HasMultiSampling())
        CreateColorBuffers(resolution);

    if (depthStencilFormat_ != VK_FORMAT_UNDEFINED)
        CreateDepthStencilBuffer(resolution);

    CreateSwapChainFramebuffers();
}

VkSurfaceFormatKHR VKRenderContext::PickSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& surfaceFormats) const
{
    if (surfaceFormats.empty())
        throw std::runtime_error("no Vulkan surface formats available");

    if (surfaceFormats.size() == 1 && surfaceFormats.front().format == VK_FORMAT_UNDEFINED)
        return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

    for (const auto& format : surfaceFormats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }

    return surfaceFormats.front();
}

VkPresentModeKHR VKRenderContext::PickSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes, std::uint32_t vsyncInterval) const
{
    if (vsyncInterval == 0)
    {
        /* Check if MAILBOX or IMMEDIATE presentation mode is available, to avoid vertical synchronization */
        for (const auto& mode : presentModes)
        {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR || mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                return mode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VKRenderContext::PickSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCaps, const Extent2D& resolution) const
{
    return VkExtent2D
    {
        std::max(surfaceCaps.minImageExtent.width,  std::min(surfaceCaps.maxImageExtent.width,  resolution.width )),
        std::max(surfaceCaps.minImageExtent.height, std::min(surfaceCaps.maxImageExtent.height, resolution.height))
    };
}

static std::vector<VkFormat> GetDepthStencilFormatPreference(int depthBits, int stencilBits)
{
    if (stencilBits == 0)
    {
        if (depthBits == 32)
            return { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM };
    }
    else
    {
        if (depthBits == 32)
            return { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT };
    }
    return { VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM };
}

VkFormat VKRenderContext::PickDepthStencilFormat(int depthBits, int stencilBits) const
{
    const auto formats = GetDepthStencilFormatPreference(depthBits, stencilBits);
    return VKFindSupportedImageFormat(
        physicalDevice_,
        formats.data(),
        formats.size(),
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

std::uint32_t VKRenderContext::PickSwapChainSize(std::uint32_t swapBuffers) const
{
    return std::max(surfaceSupportDetails_.caps.minImageCount, std::min(swapBuffers, surfaceSupportDetails_.caps.maxImageCount));
}

void VKRenderContext::AcquireNextPresentImage()
{
    /* Get next image for presentation */
    vkAcquireNextImageKHR(
        device_,
        swapChain_,
        UINT64_MAX,
        imageAvailableSemaphore_,
        VK_NULL_HANDLE,
        &presentImageIndex_
    );
}


} // /namespace LLGL



// ================================================================================
