// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef ICO_H
#define ICO_H

////////////////////////////////
//~ rjf: ICO File Format Types

#pragma pack(push, 1)

typedef struct ICO_Header ICO_Header;
struct ICO_Header
{
  U16 reserved_padding; // must be 0
  U16 image_type; // if 1 -> ICO, if 2 -> CUR
  U16 num_images;
};

typedef struct ICO_Entry ICO_Entry;
struct ICO_Entry
{
  U8 image_width_px;
  U8 image_height_px;
  U8 num_colors;
  U8 reserved_padding; // should be 0
  union
  {
    U16 ico_color_planes; // in ICO
    U16 cur_hotspot_x_px; // in CUR
  };
  union
  {
    U16 ico_bits_per_pixel; // in ICO
    U16 cur_hotspot_y_px;   // in CUR
  };
  U32 image_data_size;
  U32 image_data_off;
};

#pragma pack(pop)

#endif // ICO_H
