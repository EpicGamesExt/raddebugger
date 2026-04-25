// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef OS_GFX_MAC_APPKIT_H
#define OS_GFX_MAC_APPKIT_H

// AppKit enum defininitions

enum {
  NSWindowStyleMaskBorderless             = 0,
  NSWindowStyleMaskTitled                 = 1 << 0,
  NSWindowStyleMaskClosable               = 1 << 1,
  NSWindowStyleMaskMiniaturizable         = 1 << 2,
  NSWindowStyleMaskResizable              = 1 << 3,
  NSWindowStyleMaskUnifiedTitleAndToolbar = 1 << 12,
  NSWindowStyleMaskFullScreen             = 1 << 14,
  NSWindowStyleMaskFullSizeContentView    = 1 << 15,
  NSWindowStyleMaskUtilityWindow          = 1 << 4,
  NSWindowStyleMaskDocModalWindow         = 1 << 6,
  NSWindowStyleMaskNonactivatingPanel     = 1 << 7,
  NSWindowStyleMaskHUDWindow              = 1 << 13
};

enum {
  NSEventTypeLeftMouseDown                = 1,
  NSEventTypeLeftMouseUp                  = 2,
  NSEventTypeRightMouseDown               = 3,
  NSEventTypeRightMouseUp                 = 4,
  NSEventTypeMouseMoved                   = 5,
  NSEventTypeLeftMouseDragged             = 6,
  NSEventTypeRightMouseDragged            = 7,
  NSEventTypeMouseEntered                 = 8,
  NSEventTypeMouseExited                  = 9,
  NSEventTypeKeyDown                      = 10,
  NSEventTypeKeyUp                        = 11,
  NSEventTypeFlagsChanged                 = 12,
  NSEventTypeAppKitDefined                = 13,
  NSEventTypeSystemDefined                = 14,
  NSEventTypeApplicationDefined           = 15,
  NSEventTypePeriodic                     = 16,
  NSEventTypeCursorUpdate                 = 17,
  NSEventTypeScrollWheel                  = 22,
  NSEventTypeTabletPoint                  = 23,
  NSEventTypeTabletProximity              = 24,
  NSEventTypeOtherMouseDown               = 25,
  NSEventTypeOtherMouseUp                 = 26,
  NSEventTypeOtherMouseDragged            = 27,
  // Introduced in macOS on 10.5.2 and later
  NSEventTypeGesture                      = 29,
  NSEventTypeMagnify                      = 30,
  NSEventTypeSwipe                        = 31,
  NSEventTypeRotate                       = 18,
  NSEventTypeBeginGesture                 = 19,
  NSEventTypeEndGesture                   = 20,
  
  NSEventTypeSmartMagnify                 = 32,
  NSEventTypeQuickLook                    = 33,
  
  NSEventTypePressure                     = 34,
  NSEventTypeDirectTouch                  = 37,
  // Introduced in macOS Tahoe
  NSEventTypeChangeMode                   = 38,
};

typedef NSUInteger NSEventModifierFlags;
enum {
    NSEventModifierFlagCapsLock           = 1 << 16, // Set if Caps Lock key is pressed.
    NSEventModifierFlagShift              = 1 << 17, // Set if Shift key is pressed.
    NSEventModifierFlagControl            = 1 << 18, // Set if Control key is pressed.
    NSEventModifierFlagOption             = 1 << 19, // Set if Option or Alternate key is pressed.
    NSEventModifierFlagCommand            = 1 << 20, // Set if Command key is pressed.
    NSEventModifierFlagNumericPad         = 1 << 21, // Set if any key in the numeric keypad is pressed.
    NSEventModifierFlagHelp               = 1 << 22, // Set if the Help key is pressed.
    NSEventModifierFlagFunction           = 1 << 23, // Set if any function key is pressed.
};

typedef NSUInteger NSWindowButton;
enum {
  NSWindowCloseButton,
  NSWindowMiniaturizeButton,
  NSWindowZoomButton,
};

// AppKit external object defininitions

extern id const NSDefaultRunLoopMode;

extern id const NSImageNameCaution;
extern id const NSImageNameInfo;

extern id NSPasteboardNameGeneral;
extern id NSPasteboardNameFont;
extern id NSPasteboardNameRuler;
extern id NSPasteboardNameFind;
extern id NSPasteboardNameDrag;

extern id const NSPasteboardTypeString;
extern id const NSPasteboardTypePDF;
extern id const NSPasteboardTypeTIFF;
extern id const NSPasteboardTypePNG;
extern id const NSPasteboardTypeRTF;
extern id const NSPasteboardTypeRTFD;
extern id const NSPasteboardTypeHTML;
extern id const NSPasteboardTypeTabularText;
extern id const NSPasteboardTypeFont;
extern id const NSPasteboardTypeRuler;
extern id const NSPasteboardTypeColor;
extern id const NSPasteboardTypeSound;
extern id const NSPasteboardTypeMultipleTextSelection;
extern id const NSPasteboardTypeTextFinderOptions;
extern id const NSPasteboardTypeURL;
extern id const NSPasteboardTypeFileURL;

extern id const NSPasteboardURLReadingFileURLsOnlyKey;

#endif