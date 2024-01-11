// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

@table(name, name_lower, type)
UI_StackTable:
{
  //- rjf: parents
  { Parent                             parent                       `UI_Box *`      }
  
  //- rjf: layout params
  { ChildLayoutAxis                    child_layout_axis             Axis2          }
  
  //- rjf: size/position
  { FixedX                             fixed_x                       F32            }
  { FixedY                             fixed_y                       F32            }
  { FixedWidth                         fixed_width                   F32            }
  { FixedHeight                        fixed_height                  F32            }
  { PrefWidth                          pref_width                    UI_Size        }
  { PrefHeight                         pref_height                   UI_Size        }
  
  //- rjf: flags
  { Flags                              flags                         UI_BoxFlags    }
  
  //- rjf: interaction
  { FastpathCodepoint                  fastpath_codepoint            U32            }
  
  //- rjf: colors
  { BackgroundColor                    background_color              Vec4F32        }
  { TextColor                          text_color                    Vec4F32        }
  { BorderColor                        border_color                  Vec4F32        }
  { OverlayColor                       overlay_color                 Vec4F32        }
  { TextSelectColor                    text_select_color             Vec4F32        }
  { TextCursorColor                    text_cursor_color             Vec4F32        }
  
  //- rjf: hover cursor
  { HoverCursor                        hover_cursor                  OS_Cursor      }
  
  //- rjf: font
  { Font                               font                          F_Tag          }
  { FontSize                           font_size                     F32            }
  
  //- rjf: corner radii
  { CornerRadius00                     corner_radius_00              F32            }
  { CornerRadius01                     corner_radius_01              F32            }
  { CornerRadius10                     corner_radius_10              F32            }
  { CornerRadius11                     corner_radius_11              F32            }
  
  //- rjf: blur size
  { BlurSize                           blur_size                     F32            }
  
  //- rjf: text parameters
  { TextPadding                        text_padding                  F32            }
  { TextAlignment                      text_alignment                UI_TextAlign   }
}

@table_gen
{
  `#define UI_StackDecls struct{\\`;
  @expand(UI_StackTable a)
    `struct { $(a.type) active; $(a.type) v[64]; U64 count; B32 auto_pop; } $(a.name_lower);\\`;
  `}`;
}

@table_gen
{
  `#define UI_ZeroAllStacks(ui_state) do{\\`;
  @expand(UI_StackTable a)
    `MemoryZeroStruct(&ui_state->$(a.name_lower));\\`;
  `} while(0)`;
}

@table_gen
{
  `#define UI_AutoPopAllStacks(ui_state) do{\\`;
  @expand(UI_StackTable a)
    `if(ui_state->$(a.name_lower).auto_pop) {ui_state->$(a.name_lower).auto_pop = 0; ui_pop_$(a.name_lower)();}\\`;
  `} while(0)`;
}

@table_gen
{
  @expand(UI_StackTable a)
    `internal $(a.type) $(=>35) ui_push_$(a.name_lower)($(a.type) v);`;
  @expand(UI_StackTable a)
    `internal $(a.type) $(=>35) ui_pop_$(a.name_lower)(void);`;
  @expand(UI_StackTable a)
    `internal $(a.type) $(=>35) ui_top_$(a.name_lower)(void);`;
  @expand(UI_StackTable a)
    `internal $(a.type) $(=>35) ui_bottom_$(a.name_lower)(void);`;
  @expand(UI_StackTable a)
    `internal $(a.type) $(=>35) ui_set_next_$(a.name_lower)($(a.type) v);`;
}

@table_gen
{
  `#if 0`;
  @expand(UI_StackTable a)
    `#define UI_$(a.name)(v) $(=>35) DeferLoop(ui_push_$(a.name_lower)(v), ui_pop_$(a.name_lower)())`;
  `#endif`;
}

@table_gen @c_file
{
  @expand(UI_StackTable a)
    `internal $(a.type) ui_push_$(a.name_lower)($(a.type) v) {return UI_StackPush($(a.name_lower), v);}`;
  @expand(UI_StackTable a)
    `internal $(a.type) ui_pop_$(a.name_lower)(void) {$(a.type) popped; return UI_StackPop($(a.name_lower), popped);}`;
  @expand(UI_StackTable a)
    `internal $(a.type) ui_top_$(a.name_lower)(void) {return UI_StackTop($(a.name_lower));}`;
  @expand(UI_StackTable a)
    `internal $(a.type) ui_bottom_$(a.name_lower)(void) {return UI_StackBottom($(a.name_lower));}`;
  @expand(UI_StackTable a)
    `internal $(a.type) ui_set_next_$(a.name_lower)($(a.type) v) {return UI_StackSetNext($(a.name_lower), v);}`;
}
