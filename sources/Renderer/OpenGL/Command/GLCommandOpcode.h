/*
 * GLCommandOpcode.h
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015-2019 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef LLGL_GL_COMMAND_OPCODE_H
#define LLGL_GL_COMMAND_OPCODE_H


#include <cstdint>


namespace LLGL
{


enum GLOpcode : std::uint8_t
{
    GLOpcodeBufferSubData = 1,
    GLOpcodeCopyBufferSubData,
    GLOpcodeClearBufferData,
    GLOpcodeClearBufferSubData,
    GLOpcodeCopyImageSubData,
    GLOpcodeCopyImageToBuffer,
    GLOpcodeCopyImageFromBuffer,
    GLOpcodeGenerateMipmap,
    GLOpcodeGenerateMipmapSubresource,
    GLOpcodeExecute,
    GLOpcodeViewport,
    GLOpcodeViewportArray,
    GLOpcodeScissor,
    GLOpcodeScissorArray,
    GLOpcodeClearColor,
    GLOpcodeClearDepth,
    GLOpcodeClearStencil,
    GLOpcodeClear,
    GLOpcodeClearAttachmentsWithRenderPass,
    GLOpcodeClearBuffers,
    GLOpcodeBindVertexArray,
    GLOpcodeBindGL2XVertexArray,
    GLOpcodeBindElementArrayBufferToVAO,
    GLOpcodeBindBufferBase,
    GLOpcodeBindBuffersBase,
    GLOpcodeBeginTransformFeedback,
    GLOpcodeBeginTransformFeedbackNV,
    GLOpcodeEndTransformFeedback,
    GLOpcodeEndTransformFeedbackNV,
    GLOpcodeBindResourceHeap,
    GLOpcodeBindRenderTarget,
    GLOpcodeBindPipelineState,
    GLOpcodeSetBlendColor,
    GLOpcodeSetStencilRef,
    GLOpcodeSetUniforms,
    GLOpcodeBeginQuery,
    GLOpcodeEndQuery,
    GLOpcodeBeginConditionalRender,
    GLOpcodeEndConditionalRender,
    GLOpcodeDrawArrays,
    GLOpcodeDrawArraysInstanced,
    GLOpcodeDrawArraysInstancedBaseInstance,
    GLOpcodeDrawArraysIndirect,
    GLOpcodeDrawElements,
    GLOpcodeDrawElementsBaseVertex,
    GLOpcodeDrawElementsInstanced,
    GLOpcodeDrawElementsInstancedBaseVertex,
    GLOpcodeDrawElementsInstancedBaseVertexBaseInstance,
    GLOpcodeDrawElementsIndirect,
    GLOpcodeMultiDrawArraysIndirect,
    GLOpcodeMultiDrawElementsIndirect,
    GLOpcodeDispatchCompute,
    GLOpcodeDispatchComputeIndirect,
    GLOpcodeBindTexture,
    GLOpcodeBindImageTexture,
    GLOpcodeBindSampler,
    GLOpcodeBindGL2XSampler,
    GLOpcodeUnbindResources,
    GLOpcodePushDebugGroup,
    GLOpcodePopDebugGroup,
};


} // /namespace LLGL


#endif



// ================================================================================
