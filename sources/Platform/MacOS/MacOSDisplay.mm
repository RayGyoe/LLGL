/*
 * MacOSDisplay.mm
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#include "MacOSDisplay.h"
#include "../../Core/CoreUtils.h"
#include <LLGL/Utils/ForRange.h>


namespace LLGL
{


static const std::uint32_t                          g_maxNumDisplays = 16;
static std::vector<std::unique_ptr<MacOSDisplay>>   g_displayList;
static std::vector<Display*>                        g_displayRefList;
static MacOSDisplay*                                g_primaryDisplay;


/*
 * Internal functions
 */

static std::uint32_t GetDisplayModeRefreshRate(CGDisplayModeRef displayMode, CGDirectDisplayID displayID)
{
    std::uint32_t refreshRate = static_cast<std::uint32_t>(CGDisplayModeGetRefreshRate(displayMode) + 0.5);

    /* Built-in displays return 0 */
    if (refreshRate == 0)
    {
        #ifdef LLGL_MACOS_ENABLE_COREVIDEO

        /* Use CoreVideo framework to query accurate display refresh rate */
        CVDisplayLinkRef displayLink = nullptr;
        CVDisplayLinkCreateWithCGDisplay(displayID, &displayLink);
        {
            CVTime time = CVDisplayLinkGetNominalOutputVideoRefreshPeriod(displayLink);
            if ((time.flags & kCVTimeIsIndefinite) == 0 && time.timeValue != 0)
                refreshRate = static_cast<std::uint32_t>(static_cast<double>(time.timeScale) / static_cast<double>(time.timeValue) + 0.5);
        }
        CVDisplayLinkRelease(displayLink);

        #else

        /* Without CoreVideo framework, we just assume 60 Hz */
        refreshRate = 60;

        #endif // /LLGL_MACOS_ENABLE_COREVIDEO
    }

    return refreshRate;
}

// Converts a CGDisplayMode to a descriptor structure
static void ConvertCGDisplayMode(DisplayModeDescriptor& dst, CGDisplayModeRef src, CGDirectDisplayID displayID)
{
    if (@available(macOS 10.8, *))
    {
        dst.resolution.width    = static_cast<std::uint32_t>(CGDisplayModeGetPixelWidth(src));
        dst.resolution.height   = static_cast<std::uint32_t>(CGDisplayModeGetPixelHeight(src));
    }
    else
    {
        dst.resolution.width    = static_cast<std::uint32_t>(CGDisplayModeGetWidth(src));
        dst.resolution.height   = static_cast<std::uint32_t>(CGDisplayModeGetHeight(src));
    }
    dst.refreshRate = GetDisplayModeRefreshRate(src, displayID);
}

// Returns true if the specified descriptor matches the display mode
static bool MatchDisplayMode(const DisplayModeDescriptor& displayModeDesc, CGDisplayModeRef modeRef)
{
    return
    (
        static_cast<std::size_t>(displayModeDesc.resolution.width ) == CGDisplayModeGetWidth (modeRef) &&
        static_cast<std::size_t>(displayModeDesc.resolution.height) == CGDisplayModeGetHeight(modeRef)
    );
}

static bool UpdateDisplayList()
{
    bool changed = false;

    /* Retrieve all active display IDs */
    CGDirectDisplayID displayIDArray[g_maxNumDisplays];
    std::uint32_t numDisplays = 0;

    if (CGGetActiveDisplayList(g_maxNumDisplays, displayIDArray, &numDisplays) == kCGErrorSuccess)
    {
        /* Check if number of displays has changed */
        if (numDisplays != g_displayList.size())
        {
            g_displayList.resize(numDisplays);
            changed = true;
        }

        /* Allocate new display for each ID */
        for (std::uint32_t i = 0; i < numDisplays; ++i)
        {
            if (g_displayList[i] == nullptr || g_displayList[i]->GetID() != displayIDArray[i])
            {
                g_displayList[i] = MakeUnique<MacOSDisplay>(displayIDArray[i]);
                if (g_displayList[i]->IsPrimary())
                    g_primaryDisplay = g_displayList[i].get();
                changed = true;
            }
        }
    }

    return changed;
}


/*
 * Display class
 */

std::size_t Display::Count()
{
    std::uint32_t numDisplays = 0;
    CGGetActiveDisplayList(g_maxNumDisplays, nullptr, &numDisplays);
    return numDisplays;
}

Display* const * Display::GetList()
{
    if (UpdateDisplayList() || g_displayRefList.empty())
    {
        /* Update reference list and append null terminator to array */
        g_displayRefList.clear();
        g_displayRefList.reserve(g_displayList.size() + 1);
        for (const auto& display : g_displayList)
            g_displayRefList.push_back(display.get());
        g_displayRefList.push_back(nullptr);
    }
    return g_displayRefList.data();
}

Display* Display::Get(std::size_t index)
{
    UpdateDisplayList();
    return (index < g_displayList.size() ? g_displayList[index].get() : nullptr);
}

Display* Display::GetPrimary()
{
    UpdateDisplayList();
    return g_primaryDisplay;
}

static bool g_cursorVisible = true;

bool Display::ShowCursor(bool show)
{
    if (g_cursorVisible != show)
    {
        if (show)
            [NSCursor unhide];
        else
            [NSCursor hide];
        g_cursorVisible = show;
    }
    return true;
}

bool Display::IsCursorShown()
{
    return g_cursorVisible;
}

// Singleton for the custom NSCursor.
class MacOSCustomNSCursor
{

    public:

        ~MacOSCustomNSCursor();

        static void SetHotSpot(const NSPoint& hotSpot);

    private:

        MacOSCustomNSCursor() = default;

        void MakeNewNSCursor(NSImage* image, NSPoint hotSpot);

    private:

        NSCursor* cursor_ = nullptr;

};

MacOSCustomNSCursor::~MacOSCustomNSCursor()
{
    if (cursor_ != nullptr)
        [cursor_ release];
}

void MacOSCustomNSCursor::SetHotSpot(const NSPoint &hotSpot)
{
    static MacOSCustomNSCursor instance;
    if (NSCursor* oldCursor = [NSCursor currentSystemCursor])
        instance.MakeNewNSCursor([oldCursor image], hotSpot);
}

void MacOSCustomNSCursor::MakeNewNSCursor(NSImage* image, NSPoint hotSpot)
{
    if (cursor_ != nullptr)
        [cursor_ release];
    cursor_ = [[NSCursor alloc] initWithImage:image hotSpot:hotSpot];
    [cursor_ set];
}

bool Display::SetCursorPosition(const Offset2D& position)
{
    /*
    NSCursor API doesn't allow to change the cursor position,
    so we create a new cursor at the requested location and make it the new cursor.
    */
    NSPoint newHotSpot = NSMakePoint(
        static_cast<CGFloat>(position.x),
        static_cast<CGFloat>(position.y)
    );
    MacOSCustomNSCursor::SetHotSpot(newHotSpot);
    return true;
}

Offset2D Display::GetCursorPosition()
{
    /*
    Return 'hot spot' of current system cursor as primary cursor position.
    This will return a value whether the cursor is hidden or visible.
    */
    if (NSCursor* cursor = [NSCursor currentSystemCursor])
    {
        NSPoint hotSpot = [cursor hotSpot];
        return Offset2D
        {
            static_cast<std::int32_t>(hotSpot.x),
            static_cast<std::int32_t>(hotSpot.y)
        };
    }
    return { 0, 0 };
}


/*
 * MacOSDisplay class
 */

MacOSDisplay::MacOSDisplay(CGDirectDisplayID displayID) :
    displayID_             { displayID                            },
    defaultDisplayModeRef_ { CGDisplayCopyDisplayMode(displayID_) }
{
}

MacOSDisplay::~MacOSDisplay()
{
    CGDisplayModeRelease(defaultDisplayModeRef_);
}

bool MacOSDisplay::IsPrimary() const
{
    return (CGDisplayIsMain(displayID_) != 0);
}

UTF8String MacOSDisplay::GetDeviceName() const
{
    //TODO
    return UTF8String{};
}

Offset2D MacOSDisplay::GetOffset() const
{
    CGRect rc = CGDisplayBounds(displayID_);
    return Offset2D
    {
        static_cast<std::int32_t>(rc.origin.x),
        static_cast<std::int32_t>(rc.origin.y)
    };
}

float MacOSDisplay::GetScale() const
{
    if (@available(macOS 10.8, *))
    {
        CGDisplayModeRef modeRef = CGDisplayCopyDisplayMode(displayID_);
        const float pixelWidth = static_cast<float>(CGDisplayModeGetPixelWidth(modeRef));
        const float pointWidth = static_cast<float>(CGDisplayModeGetWidth(modeRef));
        const float scale = pixelWidth / pointWidth;
        CGDisplayModeRelease(modeRef);
        return scale;
    }
    return 1.0f;
}

bool MacOSDisplay::ResetDisplayMode()
{
    return (CGDisplaySetDisplayMode(displayID_, defaultDisplayModeRef_, nullptr) == kCGErrorSuccess);
}

bool MacOSDisplay::SetDisplayMode(const DisplayModeDescriptor& displayModeDesc)
{
    bool result = false;

    CFArrayRef modeArrayRef = CGDisplayCopyAllDisplayModes(displayID_, nullptr);

    for (CFIndex i = 0, n = CFArrayGetCount(modeArrayRef); i < n; ++i)
    {
        /* Check if current display mode matches the input descriptor */
        CGDisplayModeRef modeRef = (CGDisplayModeRef)CFArrayGetValueAtIndex(modeArrayRef, i);

        if (MatchDisplayMode(displayModeDesc, modeRef))
        {
            result = (CGDisplaySetDisplayMode(displayID_, modeRef, nullptr) == kCGErrorSuccess);
            break;
        }
    }

    CFRelease(modeArrayRef);

    return result;
}

DisplayModeDescriptor MacOSDisplay::GetDisplayMode() const
{
    DisplayModeDescriptor displayModeDesc;
    {
        CGDisplayModeRef modeRef = CGDisplayCopyDisplayMode(displayID_);
        ConvertCGDisplayMode(displayModeDesc, modeRef, displayID_);
        CGDisplayModeRelease(modeRef);
    }
    return displayModeDesc;
}

std::vector<DisplayModeDescriptor> MacOSDisplay::GetSupportedDisplayModes() const
{
    std::vector<DisplayModeDescriptor> displayModeDescs;

    CFArrayRef modeArrayRef = CGDisplayCopyAllDisplayModes(displayID_, nullptr);

    for_range(i, CFArrayGetCount(modeArrayRef))
    {
        DisplayModeDescriptor modeDesc;
        CGDisplayModeRef modeRef = (CGDisplayModeRef)CFArrayGetValueAtIndex(modeArrayRef, i);
        ConvertCGDisplayMode(modeDesc, modeRef, displayID_);
        displayModeDescs.push_back(modeDesc);
    }

    CFRelease(modeArrayRef);

    /* Sort final display mode list and remove duplciate entries */
    FinalizeDisplayModes(displayModeDescs);

    return displayModeDescs;
}


} // /namespace LLGL



// ================================================================================
