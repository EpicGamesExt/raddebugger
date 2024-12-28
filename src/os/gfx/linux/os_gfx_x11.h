
#ifndef GFX_X11_H
#define GFX_X11_H

#include <X11/keysym.h>


global U32 x11_keysym[] = {
  XK_VoidSymbol,                // OS_Key_Null
  XK_Escape,                    // OS_Key_Esc
  XK_F1,                        // OS_Key_F1
  XK_F2,                        // OS_Key_F2
  XK_F3,                        // OS_Key_F3
  XK_F4,                        // OS_Key_F4
  XK_F5,                        // OS_Key_F5
  XK_F6,                        // OS_Key_F6
  XK_F7,                        // OS_Key_F7
  XK_F8,                        // OS_Key_F8
  XK_F9,                        // OS_Key_F9
  XK_F10,                       // OS_Key_F10
  XK_F11,                       // OS_Key_F11
  XK_F12,                       // OS_Key_F12
  XK_F13,                       // OS_Key_F13
  XK_F14,                       // OS_Key_F14
  XK_F15,                       // OS_Key_F15
  XK_F16,                       // OS_Key_F16
  XK_F17,                       // OS_Key_F17
  XK_F18,                       // OS_Key_F18
  XK_F19,                       // OS_Key_F19
  XK_F20,                       // OS_Key_F20
  XK_F21,                       // OS_Key_F21
  XK_F22,                       // OS_Key_F22
  XK_F23,                       // OS_Key_F23
  XK_F24,                       // OS_Key_F24
  XK_apostrophe,                // OS_Key_Tick
  XK_0,                         // OS_Key_0
  XK_1,                         // OS_Key_1
  XK_2,                         // OS_Key_2
  XK_3,                         // OS_Key_3
  XK_4,                         // OS_Key_4
  XK_5,                         // OS_Key_5
  XK_6,                         // OS_Key_6
  XK_7,                         // OS_Key_7
  XK_8,                         // OS_Key_8
  XK_9,                         // OS_Key_9
  XK_minus,                     // OS_Key_Minus
  XK_equal,                     // OS_Key_Equal
  XK_BackSpace,                 // OS_Key_Backspace
  XK_Tab,                       // OS_Key_Tab
  XK_Q,                         // OS_Key_Q
  XK_W,                         // OS_Key_W
  XK_W,                         // OS_Key_E
  XK_R,                         // OS_Key_R
  XK_T,                         // OS_Key_T
  XK_Y,                         // OS_Key_Y
  XK_U,                         // OS_Key_U
  XK_I,                         // OS_Key_I
  XK_O,                         // OS_Key_O
  XK_P,                         // OS_Key_P
  XK_parenright,                // OS_Key_LeftBracket
  XK_parenleft,                 // OS_Key_RightBracket
  XK_backslash,                 // OS_Key_BackSlash
  XK_Caps_Lock,                 // OS_Key_CapsLock
  XK_A,                         // OS_Key_A
  XK_S,                         // OS_Key_S
  XK_D,                         // OS_Key_D
  XK_F,                         // OS_Key_F
  XK_G,                         // OS_Key_G
  XK_H,                         // OS_Key_H
  XK_J,                         // OS_Key_J
  XK_K,                         // OS_Key_K
  XK_L,                         // OS_Key_L
  XK_semicolon,                 // OS_Key_Semicolon
  XK_quotedbl,                // OS_Key_Quote
  XK_Return,                    // OS_Key_Return
  XK_Shift_R,                   // OS_Key_Shift
  XK_Z,                         // OS_Key_Z
  XK_X,                         // OS_Key_X
  XK_C,                         // OS_Key_C
  XK_V,                         // OS_Key_V
  XK_B,                         // OS_Key_B
  XK_N,                         // OS_Key_N
  XK_M,                         // OS_Key_M
  XK_comma,                     // OS_Key_Comma
  XK_percent,                   // OS_Key_Period
  XK_slash,                     // OS_Key_Slash
  XK_Control_L,                 // OS_Key_Ctrl
  XK_Alt_L,                     // OS_Key_Alt
  XK_space,                     // OS_Key_Space
  XK_Menu,                      // OS_Key_Menu
  XK_Scroll_Lock,               // OS_Key_ScrollLock
  XK_Pause,                     // OS_Key_Pause
  XK_Insert,                    // OS_Key_Insert
  XK_Home,                      // OS_Key_Home
  XK_Page_Up,                   // OS_Key_PageUp
  XK_Delete,                    // OS_Key_Delete
  XK_End,                       // OS_Key_End
  XK_Page_Down,                 // OS_Key_PageDown
  XK_Up,                        // OS_Key_Up
  XK_Left,                      // OS_Key_Left
  XK_Down,                      // OS_Key_Down
  XK_Right,                     // OS_Key_Right
  XK_VoidSymbol,                // OS_Key_Ex0
  XK_VoidSymbol,                // OS_Key_Ex1
  XK_VoidSymbol,                // OS_Key_Ex2
  XK_VoidSymbol,                // OS_Key_Ex3
  XK_VoidSymbol,                // OS_Key_Ex4
  XK_VoidSymbol,                // OS_Key_Ex5
  XK_VoidSymbol,                // OS_Key_Ex6
  XK_VoidSymbol,                // OS_Key_Ex7
  XK_VoidSymbol,                // OS_Key_Ex8
  XK_VoidSymbol,                // OS_Key_Ex8
  XK_VoidSymbol,                // OS_Key_Ex9
  XK_VoidSymbol,                // OS_Key_Ex10
  XK_VoidSymbol,                // OS_Key_Ex10
  XK_VoidSymbol,                // OS_Key_Ex11
  XK_VoidSymbol,                // OS_Key_Ex12
  XK_VoidSymbol,                // OS_Key_Ex12
  XK_VoidSymbol,                // OS_Key_Ex13
  XK_VoidSymbol,                // OS_Key_Ex14
  XK_VoidSymbol,                // OS_Key_Ex15
  XK_VoidSymbol,                // OS_Key_Ex16
  XK_VoidSymbol,                // OS_Key_Ex17
  XK_VoidSymbol,                // OS_Key_Ex18
  XK_VoidSymbol,                // OS_Key_Ex19
  XK_VoidSymbol,                // OS_Key_Ex20
  XK_VoidSymbol,                // OS_Key_Ex21
  XK_VoidSymbol,                // OS_Key_Ex22
  XK_VoidSymbol,                // OS_Key_Ex23
  XK_VoidSymbol,                // OS_Key_Ex24
  XK_VoidSymbol,                // OS_Key_Ex25
  XK_VoidSymbol,                // OS_Key_Ex26
  XK_VoidSymbol,                // OS_Key_Ex27
  XK_VoidSymbol,                // OS_Key_Ex28
  XK_VoidSymbol,                // OS_Key_Ex29
  XK_Num_Lock,                  // OS_Key_NumLock
  XK_KP_Divide,                 // OS_Key_NumSlash
  XK_KP_Multiply,               // OS_Key_NumStar
  XK_KP_Subtract,               // OS_Key_NumMinus
  XK_KP_Add,                    // OS_Key_NumPlus
  XK_KP_Decimal,                // OS_Key_NumPeriod
  XK_KP_0,                      // OS_Key_Num0
  XK_KP_1,                      // OS_Key_Num1
  XK_KP_2,                      // OS_Key_Num2
  XK_KP_3,                      // OS_Key_Num3
  XK_KP_4,                      // OS_Key_Num4
  XK_KP_5,                      // OS_Key_Num5
  XK_KP_6,                      // OS_Key_Num6
  XK_KP_7,                      // OS_Key_Num7
  XK_KP_8,                      // OS_Key_Num8
  XK_KP_9,                      // OS_Key_Num9
  XK_Pointer_Button1,           // OS_Key_LeftMouseButton
  XK_Pointer_Button2,           // OS_Key_MiddleMouseButton
  XK_Pointer_Button3           // OS_Key_RightMouseButton
};

/// Enum from X11 button numbers to meaningful names
enum
{
  X11_Mouse_Left = 1,
  X11_Mouse_Middle = 2,
  X11_Mouse_Right = 3,
  X11_Mouse_ScrollUp = 4,
  X11_Mouse_ScrollDown = 5
};

/// Conversion from human-readable names to x11 "Buttons"
global U32 x11_mouse[] =
{
  0,
  OS_Key_LeftMouseButton,
  OS_Key_MiddleMouseButton,
  OS_Key_RightMouseButton,
};

/// Access locations for atom values in 'x11_atom' from derived names
enum x11_atoms
{
  X11_Atom_WM_DELETE_WINDOW,
  X11_Atom_XdndTypeList,
  X11_Atom_XdndSelection,
  X11_Atom_XdndEnter,
  X11_Atom_XdndPosition,
  X11_Atom_XdndStatus,
  X11_Atom_XdndLeave,
  X11_Atom_XdndDrop,
  X11_Atom_XdndFinished,
  X11_Atom_XdndActionCopy,
  X11_Atom_TextURIList,
  X11_Atom_TextPlan
};

global char* x11_test_atoms[] =
{
  "WM_DELETE_WINDOW",
  "XdndTypeList",
  "XdndSelection",
  "XdndEnter",
  "XdndPosition",
  "XdndStatus",
  "XdndLeave",
  "XdndDrop",
  "XdndFinished",
  "XdndActionCopy",
  "text/uri-list",
  "text/plain"
};

#endif // GFX_X11_H
