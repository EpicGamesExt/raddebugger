// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_STRINGS_H
#define BASE_STRINGS_H

////////////////////////////////
//~ rjf: Third Party Includes

#define STB_SPRINTF_DECORATE(name) raddbg_##name
#include "third_party/stb/stb_sprintf.h"

////////////////////////////////
//~ rjf: String Types

typedef struct String8 String8;
struct String8
{
  U8 *str;
  U64 size;
};

typedef struct String16 String16;
struct String16
{
  U16 *str;
  U64 size;
};

typedef struct String32 String32;
struct String32
{
  U32 *str;
  U64 size;
};

////////////////////////////////
//~ rjf: String List & Array Types

typedef struct String8Node String8Node;
struct String8Node
{
  String8Node *next;
  String8 string;
};

typedef struct String8MetaNode String8MetaNode;
struct String8MetaNode
{
  String8MetaNode *next;
  String8Node *node;
};

typedef struct String8List String8List;
struct String8List
{
  String8Node *first;
  String8Node *last;
  U64 node_count;
  U64 total_size;
};

typedef struct String8Array String8Array;
struct String8Array
{
  String8 *v;
  U64 count;
};

////////////////////////////////
//~ rjf: String Matching, Splitting, & Joining Types

typedef U32 StringMatchFlags;
enum
{
  StringMatchFlag_CaseInsensitive  = (1 << 0),
  StringMatchFlag_RightSideSloppy  = (1 << 1),
  StringMatchFlag_SlashInsensitive = (1 << 2),
};

typedef U32 StringSplitFlags;
enum
{
  StringSplitFlag_KeepEmpties = (1 << 0),
};

typedef enum PathStyle
{
  PathStyle_Null,
  PathStyle_Relative,
  PathStyle_WindowsAbsolute,
  PathStyle_UnixAbsolute,
  
#if OS_WINDOWS
  PathStyle_SystemAbsolute = PathStyle_WindowsAbsolute
#elif OS_LINUX
  PathStyle_SystemAbsolute = PathStyle_UnixAbsolute
#else
# error "absolute path style is undefined for this OS"
#endif
}
PathStyle;

typedef struct StringJoin StringJoin;
struct StringJoin
{
  String8 pre;
  String8 sep;
  String8 post;
};

////////////////////////////////
//~ rjf: String Pair Types

typedef struct String8TxtPtPair String8TxtPtPair;
struct String8TxtPtPair
{
  String8 string;
  TxtPt pt;
};

////////////////////////////////
//~ rjf: UTF Decoding Types

typedef struct UnicodeDecode UnicodeDecode;
struct UnicodeDecode
{
  U32 inc;
  U32 codepoint;
};

////////////////////////////////
//~ rjf: String Fuzzy Matching Types

typedef struct FuzzyMatchRangeNode FuzzyMatchRangeNode;
struct FuzzyMatchRangeNode
{
  FuzzyMatchRangeNode *next;
  Rng1U64 range;
};

typedef struct FuzzyMatchRangeList FuzzyMatchRangeList;
struct FuzzyMatchRangeList
{
  FuzzyMatchRangeNode *first;
  FuzzyMatchRangeNode *last;
  U64 count;
  U64 needle_part_count;
  U64 total_dim;
};

typedef struct ScoredFuzzyMatchRangeList ScoredFuzzyMatchRangeList;
struct ScoredFuzzyMatchRangeList
{
  FuzzyMatchRangeList list;
  S32 score;
};

////////////////////////////////
//~ rjf: Character Classification & Conversion Functions

internal B32 char_is_space(U8 c);
internal B32 char_is_upper(U8 c);
internal B32 char_is_lower(U8 c);
internal B32 char_is_alpha(U8 c);
internal B32 char_is_slash(U8 c);
internal B32 char_is_digit(U8 c, U32 base);
internal U8 char_to_lower(U8 c);
internal U8 char_to_upper(U8 c);
internal U8 char_to_correct_slash(U8 c);

////////////////////////////////
//~ rjf: C-String Measurement

internal U64 cstring8_length(U8 *c);
internal U64 cstring16_length(U16 *c);
internal U64 cstring32_length(U32 *c);

////////////////////////////////
//~ rjf: String Constructors

#define str8_lit(S)  str8((U8*)(S), sizeof(S) - 1)
#define str8_lit_comp(S) {(U8*)(S), sizeof(S) - 1,}
#define str8_varg(S) (int)((S).size), ((S).str)

#define str8_array(S,C) str8((U8*)(S), sizeof(*(S))*(C))
#define str8_array_fixed(S) str8((U8*)(S), sizeof(S))
#define str8_struct(S) str8((U8*)(S), sizeof(*(S)))

internal String8  str8(U8 *str, U64 size);
internal String8  str8_range(U8 *first, U8 *one_past_last);
internal String8  str8_zero(void);
internal String16 str16(U16 *str, U64 size);
internal String16 str16_range(U16 *first, U16 *one_past_last);
internal String16 str16_zero(void);
internal String32 str32(U32 *str, U64 size);
internal String32 str32_range(U32 *first, U32 *one_past_last);
internal String32 str32_zero(void);
internal String8  str8_cstring(char *c);
internal String16 str16_cstring(U16 *c);
internal String32 str32_cstring(U32 *c);
internal String8  str8_cstring_capped(void *cstr, void *cap);
internal String16 str16_cstring_capped(void *cstr, void *cap);
internal String8  str8_cstring_capped_reverse(void *raw_start, void *raw_cap);

////////////////////////////////
//~ rjf: String Stylization

internal String8 upper_from_str8(Arena *arena, String8 string);
internal String8 lower_from_str8(Arena *arena, String8 string);
internal String8 backslashed_from_str8(Arena *arena, String8 string);

////////////////////////////////
//~ rjf: String Matching

#define str8_match_lit(a_lit, b, flags)   str8_match(str8_lit(a_lit), (b), (flags))
#define str8_match_cstr(a_cstr, b, flags) str8_match(str8_cstring(a_cstr), (b), (flags))
internal B32 str8_match(String8 a, String8 b, StringMatchFlags flags);
internal U64 str8_find_needle(String8 string, U64 start_pos, String8 needle, StringMatchFlags flags);
internal U64 str8_find_needle_reverse(String8 string, U64 start_pos, String8 needle, StringMatchFlags flags);
internal B32 str8_ends_with(String8 string, String8 end, StringMatchFlags flags);
#define str8_ends_with_lit(string, end_lit, flags) str8_ends_with((string), str8_lit(end_lit), (flags))

////////////////////////////////
//~ rjf: String Slicing

internal String8 str8_substr(String8 str, Rng1U64 range);
internal String8 str8_prefix(String8 str, U64 size);
internal String8 str8_skip(String8 str, U64 amt);
internal String8 str8_postfix(String8 str, U64 size);
internal String8 str8_chop(String8 str, U64 amt);
internal String8 str8_skip_chop_whitespace(String8 string);

////////////////////////////////
//~ rjf: String Formatting & Copying

internal String8 push_str8_cat(Arena *arena, String8 s1, String8 s2);
internal String8 push_str8_copy(Arena *arena, String8 s);
internal String8 push_str8fv(Arena *arena, char *fmt, va_list args);
internal String8 push_str8f(Arena *arena, char *fmt, ...);

////////////////////////////////
//~ rjf: String <=> Integer Conversions

//- rjf: string -> integer
internal S64 sign_from_str8(String8 string, String8 *string_tail);
internal B32 str8_is_integer(String8 string, U32 radix);

internal U64 u64_from_str8(String8 string, U32 radix);
internal S64 s64_from_str8(String8 string, U32 radix);
internal U32 u32_from_str8(String8 string, U32 radix);
internal S32 s32_from_str8(String8 string, U32 radix);
internal B32 try_u64_from_str8_c_rules(String8 string, U64 *x);
internal B32 try_s64_from_str8_c_rules(String8 string, S64 *x);

//- rjf: integer -> string
internal String8 str8_from_memory_size(Arena *arena, U64 size);
internal String8 str8_from_count(Arena *arena, U64 count);
internal String8 str8_from_bits_u32(Arena *arena, U32 x);
internal String8 str8_from_bits_u64(Arena *arena, U64 x);
internal String8 str8_from_u64(Arena *arena, U64 u64, U32 radix, U8 min_digits, U8 digit_group_separator);
internal String8 str8_from_s64(Arena *arena, S64 s64, U32 radix, U8 min_digits, U8 digit_group_separator);

////////////////////////////////
//~ rjf: String <=> Float Conversions

internal F64 f64_from_str8(String8 string);

////////////////////////////////
//~ rjf: String List Construction Functions

internal String8Node* str8_list_push_node(String8List *list, String8Node *node);
internal String8Node* str8_list_push_node_set_string(String8List *list, String8Node *node, String8 string);
internal String8Node* str8_list_push_node_front(String8List *list, String8Node *node);
internal String8Node* str8_list_push_node_front_set_string(String8List *list, String8Node *node, String8 string);
internal String8Node* str8_list_push(Arena *arena, String8List *list, String8 string);
internal String8Node* str8_list_push_front(Arena *arena, String8List *list, String8 string);
internal void         str8_list_concat_in_place(String8List *list, String8List *to_push);
internal String8Node* str8_list_push_aligner(Arena *arena, String8List *list, U64 min, U64 align);
internal String8Node* str8_list_pushf(Arena *arena, String8List *list, char *fmt, ...);
internal String8Node* str8_list_push_frontf(Arena *arena, String8List *list, char *fmt, ...);
internal String8List  str8_list_copy(Arena *arena, String8List *list);
#define str8_list_first(list) ((list)->first ? (list)->first->string : str8_zero())

////////////////////////////////
//~ rjf: String Splitting & Joining

internal String8List  str8_split(Arena *arena, String8 string, U8 *split_chars, U64 split_char_count, StringSplitFlags flags);
internal String8List  str8_split_by_string_chars(Arena *arena, String8 string, String8 split_chars, StringSplitFlags flags);
internal String8List  str8_list_split_by_string_chars(Arena *arena, String8List list, String8 split_chars, StringSplitFlags flags);
internal String8      str8_list_join(Arena *arena, String8List *list, StringJoin *optional_params);
internal void         str8_list_from_flags(Arena *arena, String8List *list, U32 flags, String8 *flag_string_table, U32 flag_string_count);

////////////////////////////////
//~ rjf; String Arrays

internal String8Array str8_array_from_list(Arena *arena, String8List *list);
internal String8Array str8_array_reserve(Arena *arena, U64 count);

////////////////////////////////
//~ rjf: String Path Helpers

internal String8 str8_chop_last_slash(String8 string);
internal String8 str8_skip_last_slash(String8 string);
internal String8 str8_chop_last_dot(String8 string);
internal String8 str8_skip_last_dot(String8 string);

internal PathStyle   path_style_from_str8(String8 string);
internal String8List str8_split_path(Arena *arena, String8 string);
internal void        str8_path_list_resolve_dots_in_place(String8List *path, PathStyle style);
internal String8     str8_path_list_join_by_style(Arena *arena, String8List *path, PathStyle style);

internal String8TxtPtPair str8_txt_pt_pair_from_string(String8 string);

////////////////////////////////
//~ rjf: UTF-8 & UTF-16 Decoding/Encoding

internal UnicodeDecode utf8_decode(U8 *str, U64 max);
internal UnicodeDecode utf16_decode(U16 *str, U64 max);
internal U32 utf8_encode(U8 *str, U32 codepoint);
internal U32 utf16_encode(U16 *str, U32 codepoint);
internal U32 utf8_from_utf32_single(U8 *buffer, U32 character);

////////////////////////////////
//~ rjf: Unicode String Conversions

internal String8 str8_from_16(Arena *arena, String16 in);
internal String16 str16_from_8(Arena *arena, String8 in);
internal String8 str8_from_32(Arena *arena, String32 in);
internal String32 str32_from_8(Arena *arena, String8 in);

////////////////////////////////
//~ String -> Enum Conversions

internal OperatingSystem operating_system_from_string(String8 string);

////////////////////////////////
//~ rjf: Basic Types & Space Enum -> String Conversions

internal String8 string_from_dimension(Dimension dimension);
internal String8 string_from_side(Side side);
internal String8 string_from_operating_system(OperatingSystem os);
internal String8 string_from_arch(Arch arch);

////////////////////////////////
//~ rjf: Time Types -> String

internal String8 string_from_week_day(WeekDay week_day);
internal String8 string_from_month(Month month);
internal String8 push_date_time_string(Arena *arena, DateTime *date_time);
internal String8 push_file_name_date_time_string(Arena *arena, DateTime *date_time);
internal String8 string_from_elapsed_time(Arena *arena, DateTime dt);

////////////////////////////////
//~ Globally Unique Ids

internal String8 string_from_guid(Arena *arena, Guid guid);
internal B32     try_guid_from_string(String8 string, Guid *guid_out);
internal Guid    guid_from_string(String8 string);

////////////////////////////////
//~ rjf: Basic Text Indentation

internal String8 indented_from_string(Arena *arena, String8 string);

////////////////////////////////
//~ rjf: Text Escaping

internal String8 escaped_from_raw_str8(Arena *arena, String8 string);
internal String8 raw_from_escaped_str8(Arena *arena, String8 string);

////////////////////////////////
//~ rjf: Text Wrapping

internal String8List wrapped_lines_from_string(Arena *arena, String8 string, U64 first_line_max_width, U64 max_width, U64 wrap_indent);

////////////////////////////////
//~ rjf: String <-> Color

internal String8 hex_string_from_rgba_4f32(Arena *arena, Vec4F32 rgba);
internal Vec4F32 rgba_from_hex_string_4f32(String8 hex_string);

////////////////////////////////
//~ rjf: String Fuzzy Matching

internal FuzzyMatchRangeList fuzzy_match_find(Arena *arena, String8 needle, String8 haystack);
internal FuzzyMatchRangeList fuzzy_match_range_list_copy(Arena *arena, FuzzyMatchRangeList *src);
internal ScoredFuzzyMatchRangeList scored_fuzzy_match_find(Arena *arena, String8 needles, String8 haystack);

////////////////////////////////
//~ NOTE(allen): Serialization Helpers

internal void    str8_serial_begin(Arena *arena, String8List *srl);
internal String8 str8_serial_end(Arena *arena, String8List *srl);
internal void    str8_serial_write_to_dst(String8List *srl, void *out);
internal U64     str8_serial_push_align(Arena *arena, String8List *srl, U64 align);
internal void *  str8_serial_push_size(Arena *arena, String8List *srl, U64 size);
internal void *  str8_serial_push_data(Arena *arena, String8List *srl, void *data, U64 size);
internal void    str8_serial_push_data_list(Arena *arena, String8List *srl, String8Node *first);
internal void    str8_serial_push_u64(Arena *arena, String8List *srl, U64 x);
internal void    str8_serial_push_u32(Arena *arena, String8List *srl, U32 x);
internal void    str8_serial_push_u16(Arena *arena, String8List *srl, U16 x);
internal void    str8_serial_push_u8(Arena *arena, String8List *srl, U8 x);
internal void    str8_serial_push_cstr(Arena *arena, String8List *srl, String8 str);
internal void    str8_serial_push_string(Arena *arena, String8List *srl, String8 str);
#define str8_serial_push_array(arena, srl, ptr, count) str8_serial_push_data(arena, srl, ptr, sizeof(*(ptr)) * (count))
#define str8_serial_push_struct(arena, srl, ptr) str8_serial_push_array(arena, srl, ptr, 1)

////////////////////////////////
//~ rjf: Deserialization Helpers

internal U64    str8_deserial_read(String8 string, U64 off, void *read_dst, U64 read_size, U64 granularity);
internal U64    str8_deserial_find_first_match(String8 string, U64 off, U16 scan_val);
internal void * str8_deserial_get_raw_ptr(String8 string, U64 off, U64 size);
internal U64    str8_deserial_read_cstr(String8 string, U64 off, String8 *cstr_out);
internal U64    str8_deserial_read_windows_utf16_string16(String8 string, U64 off, String16 *str_out);
internal U64    str8_deserial_read_block(String8 string, U64 off, U64 size, String8 *block_out);
internal U64    str8_deserial_read_uleb128(String8 string, U64 off, U64 *value_out);
internal U64    str8_deserial_read_sleb128(String8 string, U64 off, S64 *value_out);
#define str8_deserial_read_array(string, off, ptr, count) str8_deserial_read((string), (off), (ptr), sizeof(*(ptr))*(count), sizeof(*(ptr)))
#define str8_deserial_read_struct(string, off, ptr)       str8_deserial_read_array(string, off, ptr, 1)

#endif // BASE_STRINGS_H
