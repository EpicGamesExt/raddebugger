// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RDI_DWARF_H
#define RDI_DWARF_H

// https://dwarfstd.org/doc/DWARF4.pdf
// https://dwarfstd.org/doc/DWARF5.pdf

// TODO(allen):
// [ ] function to parse info just for unit headers & root attributes
// [ ] put together unit info from all sections in one structure
// [ ] actually check version numbers in unit header parsers

#pragma pack(push,1)

////////////////////////////////
//~ Dwarf Format Code X Lists

// unit type  X(name, code)
#define DWARF_UnitTypeXList(X)\
X(null, 0x00)\
X(compile, 0x01)\
X(type, 0x02)\
X(partial, 0x03)\
X(skeleton, 0x04)\
X(split_compile, 0x05)\
X(split_type, 0x06)\
X(lo_user, 0x80)\
X(hi_user, 0xff)

typedef enum DWARF_UnitType{
#define X(N,C) DWARF_UnitType_##N = C,
  DWARF_UnitTypeXList(X)
#undef X
} DWARF_UnitType;


// tag  X(name, code)
#define DWARF_TagXList(X)\
X(null,             0x00)\
X(array_type,       0x01)\
X(class_type,       0x02)\
X(entry_point,      0x03)\
X(enumeration_type, 0x04)\
X(formal_parameter, 0x05)\
X(imported_declaration, 0x08)\
X(label,            0x0a)\
X(lexical_block,    0x0b)\
X(member,           0x0d)\
X(pointer_type,     0x0f)\
X(reference_type,   0x10)\
X(compile_unit,     0x11)\
X(string_type,      0x12)\
X(structure_type,   0x13)\
X(subroutine_type,  0x15)\
X(typedef,          0x16)\
X(union_type,       0x17)\
X(unspecified_parameters, 0x18)\
X(variant,          0x19)\
X(common_block,     0x1a)\
X(common_inclusion, 0x1b)\
X(inheritance,      0x1c)\
X(inlined_subroutine, 0x1d)\
X(module,           0x1e)\
X(ptr_to_member_type, 0x1f)\
X(set_type,         0x20)\
X(subrange_type,    0x21)\
X(with_stmt,        0x22)\
X(access_declaration, 0x23)\
X(base_type,        0x24)\
X(catch_block,      0x25)\
X(const_type,       0x26)\
X(constant,         0x27)\
X(enumerator,       0x28)\
X(file_type,        0x29)\
X(friend,           0x2a)\
X(namelist,         0x2b)\
X(namelist_item,    0x2c)\
X(packed_type,      0x2d)\
X(subprogram,       0x2e)\
X(template_type_parameter, 0x2f)\
X(template_value_parameter, 0x30)\
X(thrown_type,      0x31)\
X(try_block,        0x32)\
X(variant_part,     0x33)\
X(variable,         0x34)\
X(volatile_type,    0x35)\
X(dwarf_procedure,  0x36)\
X(restrict_type,    0x37)\
X(interface_type,   0x38)\
X(namespace,        0x39)\
X(imported_module,  0x3a)\
X(unspecified_type, 0x3b)\
X(partial_unit,     0x3c)\
X(imported_unit,    0x3d)\
X(condition,        0x3f)\
X(shared_type,      0x40)\
X(type_unit,        0x41)\
X(rvalue_reference_type, 0x42)\
X(template_alias,   0x43)\
X(coarray_type,     0x44)\
X(generic_subrange, 0x45)\
X(dynamic_type,     0x46)\
X(atomic_type,      0x47)\
X(call_site,        0x48)\
X(call_site_parameter, 0x49)\
X(skeleton_unit,    0x4a)\
X(immutable_type,   0x4b)\
X(lo_user,        0x4080)\
X(hi_user,        0xffff)

typedef enum DWARF_Tag{
#define X(N,C) DWARF_Tag_##N = C,
  DWARF_TagXList(X)
#undef X
} DWARF_Tag;


// attribute classes:  X(name,code)
#define DWARF_AttributeClassXList(X)\
X(address,        0)\
X(addrptr,        1)\
X(block,          2)\
X(constant,       3)\
X(exprloc,        4)\
X(flag,           5)\
X(lineptr,        6)\
X(loclist,        7)\
X(loclistsptr,    8)\
X(macptr,         9)\
X(reference,     10)\
X(rnglist,       11)\
X(rnglistsptr,   12)\
X(string,        13)\
X(stroffsetsptr, 14)

typedef U32 DWARF_AttributeClassFlags;
enum{
#define X(N,C) DWARF_AttributeClassFlag_##N = (1 << C),
  DWARF_AttributeClassXList(X)
#undef X
  
  DWARF_AttributeClassFlag_0           =  0,
  DWARF_AttributeClassFlag_specialcase = ~0,
  DWARF_AttributeClassFlag_sec_offset_classes  = 
  (DWARF_AttributeClassFlag_addrptr |
   DWARF_AttributeClassFlag_lineptr |
   DWARF_AttributeClassFlag_loclist |
   DWARF_AttributeClassFlag_loclistsptr |
   DWARF_AttributeClassFlag_macptr |
   DWARF_AttributeClassFlag_rnglist |
   DWARF_AttributeClassFlag_rnglistsptr |
   DWARF_AttributeClassFlag_stroffsetsptr |
   0),
  
};


// attribute name:  X(name, code, classflag1, classflag2, classflag3, classflag4)
#define DWARF_AttributeNameXList(X)\
X(null,                    0x00, 0,             0,         0,         0)\
X(sibling,                 0x01, reference,     0,         0,         0)\
X(location,                0x02, exprloc,       loclist,   0,         0)\
X(name,                    0x03, string,        0,         0,         0)\
X(ordering,                0x09, constant,      0,         0,         0)\
X(byte_size,               0x0b, constant,      exprloc,   reference, 0)\
X(bit_size,                0x0d, constant,      exprloc,   reference, 0)\
X(stmt_list,               0x10, lineptr,       0,         0,         0)\
X(low_pc,                  0x11, address,       0,         0,         0)\
X(high_pc,                 0x12, address,       constant,  0,         0)\
X(language,                0x13, constant,      0,         0,         0)\
X(discr,                   0x15, reference,     0,         0,         0)\
X(discr_value,             0x16, constant,      0,         0,         0)\
X(visibility,              0x17, constant,      0,         0,         0)\
X(import,                  0x18, reference,     0,         0,         0)\
X(string_length,           0x19, exprloc,       loclist,   reference, 0)\
X(common_reference,        0x1a, reference,     0,         0,         0)\
X(comp_dir,                0x1b, string,        0,         0,         0)\
X(const_value,             0x1c, block,         constant,  string,    0)\
X(containing_type,         0x1d, reference,     0,         0,         0)\
X(default_value,           0x1e, constant,      reference, flag,      0)\
X(inline,                  0x20, constant,      0,         0,         0)\
X(is_optional,             0x21, flag,          0,         0,         0)\
X(lower_bound,             0x22, constant,      exprloc,   reference, 0)\
X(producer,                0x25, string,        0,         0,         0)\
X(prototyped,              0x27, flag,          0,         0,         0)\
X(return_addr,             0x2a, exprloc,       loclist,   0,         0)\
X(start_scope,             0x2c, constant,      rnglist,   0,         0)\
X(bit_stride,              0x2e, constant,      exprloc,   reference, 0)\
X(upper_bound,             0x2f, constant,      exprloc,   reference, 0)\
X(abstract_origin,         0x31, reference,     0,         0,         0)\
X(accessibility,           0x32, constant,      0,         0,         0)\
X(address_class,           0x33, constant,      0,         0,         0)\
X(artificial,              0x34, flag,          0,         0,         0)\
X(base_types,              0x35, reference,     0,         0,         0)\
X(calling_convention,      0x36, constant,      0,         0,         0)\
X(count,                   0x37, constant,      exprloc,   reference, 0)\
X(data_member_location,    0x38, constant,      exprloc,   loclist,   0)\
X(decl_column,             0x39, constant,      0,         0,         0)\
X(decl_file,               0x3a, constant,      0,         0,         0)\
X(decl_line,               0x3b, constant,      0,         0,         0)\
X(declaration,             0x3c, flag,          0,         0,         0)\
X(discr_list,              0x3d, block,         0,         0,         0)\
X(encoding,                0x3e, constant,      0,         0,         0)\
X(external,                0x3f, flag,          0,         0,         0)\
X(frame_base,              0x40, exprloc,       loclist,   0,         0)\
X(friend,                  0x41, reference,     0,         0,         0)\
X(identifier_case,         0x42, constant,      0,         0,         0)\
X(namelist_item,           0x44, reference,     0,         0,         0)\
X(priority,                0x45, reference,     0,         0,         0)\
X(segment,                 0x46, exprloc,       loclist,   0,         0)\
X(specification,           0x47, reference,     0,         0,         0)\
X(static_link,             0x48, exprloc,       loclist,   0,         0)\
X(type,                    0x49, reference,     0,         0,         0)\
X(use_location,            0x4a, exprloc,       loclist,   0,         0)\
X(variable_parameter,      0x4b, flag,          0,         0,         0)\
X(virtuality,              0x4c, constant,      0,         0,         0)\
X(vtable_elem_location,    0x4d, exprloc,       loclist,   0,         0)\
X(allocated,               0x4e, constant,      exprloc,   reference, 0)\
X(associated,              0x4f, constant,      exprloc,   reference, 0)\
X(data_location,           0x50, exprloc,       0,         0,         0)\
X(byte_stride,             0x51, constant,      exprloc,   reference, 0)\
X(entry_pc,                0x52, address,       constant,  0,         0)\
X(use_UTF8,                0x53, flag,          0,         0,         0)\
X(extension,               0x54, reference,     0,         0,         0)\
X(ranges,                  0x55, rnglist,       0,         0,         0)\
X(trampoline,              0x56, address,       flag,      reference, string)\
X(call_column,             0x57, constant,      0,         0,         0)\
X(call_file,               0x58, constant,      0,         0,         0)\
X(call_line,               0x59, constant,      0,         0,         0)\
X(description,             0x5a, string,        0,         0,         0)\
X(binary_scale,            0x5b, constant,      0,         0,         0)\
X(decimal_scale,           0x5c, constant,      0,         0,         0)\
X(small,                   0x5d, reference,     0,         0,         0)\
X(decimal_sign,            0x5e, constant,      0,         0,         0)\
X(digit_count,             0x5f, constant,      0,         0,         0)\
X(picture_string,          0x60, string,        0,         0,         0)\
X(mutable,                 0x61, flag,          0,         0,         0)\
X(threads_scaled,          0x62, flag,          0,         0,         0)\
X(explicit,                0x63, flag,          0,         0,         0)\
X(object_pointer,          0x64, reference,     0,         0,         0)\
X(endianity,               0x65, constant,      0,         0,         0)\
X(elemental,               0x66, flag,          0,         0,         0)\
X(pure,                    0x67, flag,          0,         0,         0)\
X(recursive,               0x68, flag,          0,         0,         0)\
X(signature,               0x69, reference,     0,         0,         0)\
X(main_subprogram,         0x6a, flag,          0,         0,         0)\
X(data_bit_offset,         0x6b, constant,      0,         0,         0)\
X(const_expr,              0x6c, flag,          0,         0,         0)\
X(enum_class,              0x6d, flag,          0,         0,         0)\
X(linkage_name,            0x6e, string,        0,         0,         0)\
X(string_length_bit_size,  0x6f, constant,      0,         0,         0)\
X(string_length_byte_size, 0x70, constant,      0,         0,         0)\
X(rank,                    0x71, constant,      exprloc,   0,         0)\
X(str_offsets_base,        0x72, stroffsetsptr, 0,         0,         0)\
X(addr_base,               0x73, addrptr,       0,         0,         0)\
X(rnglists_base,           0x74, rnglistsptr,   0,         0,         0)\
X(dwo_name,                0x76, string,        0,         0,         0)\
X(reference,               0x77, flag,          0,         0,         0)\
X(rvalue_reference,        0x78, flag,          0,         0,         0)\
X(macros,                  0x79, macptr,        0,         0,         0)\
X(call_all_calls,          0x7a, flag,          0,         0,         0)\
X(call_all_source_calls,   0x7b, flag,          0,         0,         0)\
X(call_all_tail_calls,     0x7c, flag,          0,         0,         0)\
X(call_return_pc,          0x7d, address,       0,         0,         0)\
X(call_value,              0x7e, exprloc,       0,         0,         0)\
X(call_origin,             0x7f, exprloc,       0,         0,         0)\
X(call_parameter,          0x80, reference,     0,         0,         0)\
X(call_pc,                 0x81, address,       0,         0,         0)\
X(call_tail_call,          0x82, flag,          0,         0,         0)\
X(call_target,             0x83, exprloc,       0,         0,         0)\
X(call_target_clobbered,   0x84, exprloc,       0,         0,         0)\
X(call_data_location,      0x85, exprloc,       0,         0,         0)\
X(call_data_value,         0x86, exprloc,       0,         0,         0)\
X(noreturn,                0x87, flag,          0,         0,         0)\
X(alignment,               0x88, constant,      0,         0,         0)\
X(export_symbols,          0x89, flag,          0,         0,         0)\
X(deleted,                 0x8a, flag,          0,         0,         0)\
X(defaulted,               0x8b, constant,      0,         0,         0)\
X(loclists_base,           0x8c, loclistsptr,   0,         0,         0)\
X(lo_user,               0x2000, 0, 0, 0, 0)\
X(hi_user,               0x3fff, 0, 0, 0, 0)

typedef enum DWARF_AttributeName{
#define X(N,C,f1,f2,f3,f4) DWARF_AttributeName_##N = C,
  DWARF_AttributeNameXList(X)
#undef X
} DWARF_AttributeName;


// attribute forms:  X(name, code, classflag)
#define DWARF_AttributeFormXList(X)\
X(null,           0x00, 0)\
X(addr,           0x01, address)\
X(block2,         0x03, block)\
X(block4,         0x04, block)\
X(data2,          0x05, constant)\
X(data4,          0x06, constant)\
X(data8,          0x07, constant)\
X(string,         0x08, string)\
X(block,          0x09, block)\
X(block1,         0x0a, block)\
X(data1,          0x0b, constant)\
X(flag,           0x0c, flag)\
X(sdata,          0x0d, constant)\
X(strp,           0x0e, string)\
X(udata,          0x0f, constant)\
X(ref_addr,       0x10, reference)\
X(ref1,           0x11, reference)\
X(ref2,           0x12, reference)\
X(ref4,           0x13, reference)\
X(ref8,           0x14, reference)\
X(ref_udata,      0x15, reference)\
X(indirect,       0x16, specialcase)\
X(sec_offset,     0x17, sec_offset_classes)\
X(exprloc,        0x18, exprloc)\
X(flag_present,   0x19, flag)\
X(strx,           0x1a, string)\
X(addrx,          0x1b, address)\
X(ref_sup4,       0x1c, reference)\
X(strp_sup,       0x1d, string)\
X(data16,         0x1e, constant)\
X(line_strp,      0x1f, string)\
X(ref_sig8,       0x20, reference)\
X(implicit_const, 0x21, specialcase)\
X(loclistx,       0x22, loclist)\
X(rnglistx,       0x23, rnglist)\
X(ref_sup8,       0x24, reference)\
X(strx1,          0x25, string)\
X(strx2,          0x26, string)\
X(strx3,          0x27, string)\
X(strx4,          0x28, string)\
X(addrx1,         0x29, address)\
X(addrx2,         0x2a, address)\
X(addrx3,         0x2b, address)\
X(addrx4,         0x2c, address)

typedef enum DWARF_AttributeForm{
#define X(N,C,f) DWARF_AttributeForm_##N = C,
  DWARF_AttributeFormXList(X)
#undef X
} DWARF_AttributeForm;


// ops:  X(name, code, opnum)
#define DWARF_OpXList(X)\
X(addr,                0x03, 1)\
X(deref,               0x06, 0)\
X(const1u,             0x08, 1)\
X(const1s,             0x09, 1)\
X(const2u,             0x0a, 1)\
X(const2s,             0x0b, 1)\
X(const4u,             0x0c, 1)\
X(const4s,             0x0d, 1)\
X(const8u,             0x0e, 1)\
X(const8s,             0x0f, 1)\
X(constu,              0x10, 1)\
X(consts,              0x11, 1)\
X(dup,                 0x12, 0)\
X(drop,                0x13, 0)\
X(over,                0x14, 0)\
X(pick,                0x15, 1)\
X(swap,                0x16, 0)\
X(rot,                 0x17, 0)\
X(xderef,              0x18, 0)\
X(abs,                 0x19, 0)\
X(and,                 0x1a, 0)\
X(div,                 0x1b, 0)\
X(minus,               0x1c, 0)\
X(mod,                 0x1d, 0)\
X(mul,                 0x1e, 0)\
X(neg,                 0x1f, 0)\
X(not,                 0x20, 0)\
X(or,                  0x21, 0)\
X(plus,                0x22, 0)\
X(plus_uconst,         0x23, 1)\
X(shl,                 0x24, 0)\
X(shr,                 0x25, 0)\
X(shra,                0x26, 0)\
X(xor,                 0x27, 0)\
X(bra,                 0x28, 1)\
X(eq,                  0x29, 0)\
X(ge,                  0x2a, 0)\
X(gt,                  0x2b, 0)\
X(le,                  0x2c, 0)\
X(lt,                  0x2d, 0)\
X(ne,                  0x2e, 0)\
X(skip,                0x2f, 1)\
X(lit0,                0x30, 0)\
X(lit1,                0x31, 0)\
X(lit31,               0x4f, 0)\
X(reg0,                0x50, 0)\
X(reg1,                0x51, 0)\
X(reg31,               0x6f, 0)\
X(breg0,               0x70, 1)\
X(breg1,               0x71, 1)\
X(breg31,              0x8f, 1)\
X(regx,                0x90, 1)\
X(fbreg,               0x91, 1)\
X(bregx,               0x92, 2)\
X(piece,               0x93, 1)\
X(deref_size,          0x94, 1)\
X(xderef_size,         0x95, 1)\
X(nop,                 0x96, 0)\
X(push_object_address, 0x97, 0)\
X(call2,               0x98, 1)\
X(call4,               0x99, 1)\
X(call_ref,            0x9a, 1)\
X(form_tls_address,    0x9b, 0)\
X(call_frame_cfa,      0x9c, 0)\
X(bit_piece,           0x9d, 2)\
X(implicit_value,      0x9e, 2)\
X(stack_value,         0x9f, 0)\
X(implicit_pointer,    0xa0, 2)\
X(addrx,               0xa1, 1)\
X(constx,              0xa2, 1)\
X(entry_value,         0xa3, 2)\
X(const_type,          0xa4, 3)\
X(regval_type,         0xa5, 2)\
X(deref_type,          0xa6, 2)\
X(xderef_type,         0xa7, 2)\
X(convert,             0xa8, 1)\
X(reinterpret,         0xa9, 1)\
X(lo_user,             0xe0, 0)\
X(hi_user,             0xff, 0)

typedef enum DWARF_Op{
#define X(N,C,k) DWARF_Op_##N = C,
  DWARF_OpXList(X)
#undef X
} DWARF_Op;


// location list entry:  X(name, code)
#define DWARF_LocationListEntryXList(X)\
X(end_of_list,      0x00)\
X(base_addressx,    0x01)\
X(startx_endx,      0x02)\
X(startx_length,    0x03)\
X(offset_pair,      0x04)\
X(default_location, 0x05)\
X(base_address,     0x06)\
X(start_end,        0x07)\
X(start_length,     0x08)

typedef enum DWARF_LocationListEntry{
#define X(N,C) DWARF_LocationListEntry_##N = C,
  DWARF_LocationListEntryXList(X)
#undef X
} DWARF_LocationListEntry;


// base type:  X(name, code)
#define DWARF_BaseTypeXList(X)\
X(address,         0x01)\
X(boolean,         0x02)\
X(complex_float,   0x03)\
X(float,           0x04)\
X(signed,          0x05)\
X(signed_char,     0x06)\
X(unsigned,        0x07)\
X(unsigned_char,   0x08)\
X(imaginary_float, 0x09)\
X(packed_decimal,  0x0a)\
X(numeric_string,  0x0b)\
X(edited,          0x0c)\
X(signed_fixed,    0x0d)\
X(unsigned_fixed,  0x0e)\
X(decimal_float,   0x0f)\
X(UTF,             0x10)\
X(UCS,             0x11)\
X(ASCII,           0x12)\
X(lo_user,         0x80)\
X(hi_user,         0xff)

typedef enum DWARF_BaseType{
#define X(N,C) DWARF_BaseType_##N = C,
  DWARF_BaseTypeXList(X)
#undef X
} DWARF_BaseType;


// decimal sign:  X(name, code)
#define DWARF_DecimalSignXList(X)\
X(unsigned,           0x01)\
X(leading_overpunch,  0x02)\
X(trailing_overpunch, 0x03)\
X(leading_separate,   0x04)\
X(trailing_separate,  0x05)

typedef enum DWARF_DecimalSign{
#define X(N,C) DWARF_DecimalSign_##N = C,
  DWARF_DecimalSignXList(X)
#undef X
} DWARF_DecimalSign;


// endianity:  X(name, code)
#define DWARF_EndianityXList(X)\
X(default, 0x00)\
X(big,     0x01)\
X(little,  0x02)\
X(lo_user, 0x40)\
X(hi_user, 0xff)

typedef enum DWARF_Endianity{
#define X(N,C) DWARF_Endianity_##N = C,
  DWARF_EndianityXList(X)
#undef X
} DWARF_Endianity;


// access:  X(name, code)
#define DWARF_AccessXList(X)\
X(public,    0x01)\
X(protected, 0x02)\
X(private,   0x03)

typedef enum DWARF_Access{
#define X(N,C) DWARF_Access_##N = C,
  DWARF_AccessXList(X)
#undef X
} DWARF_Access;


// visibility:  X(name, code)
#define DWARF_VisibilityXList(X)\
X(local,     0x01)\
X(exported,  0x02)\
X(qualified, 0x03)

typedef enum DWARF_Visibility{
#define X(N,C) DWARF_Visibility_##N = C,
  DWARF_VisibilityXList(X)
#undef X
} DWARF_Visibility;


// virtuality:  X(name, code)
#define DWARF_VirtualityXList(X)\
X(none,         0x00)\
X(virtual,      0x01)\
X(pure_virtual, 0x02)

typedef enum DWARF_Virtuality{
#define X(N,C) DWARF_Virtuality_##N = C,
  DWARF_VirtualityXList(X)
#undef X
} DWARF_Virtuality;


// language:  X(name, code, deflowerbound)
#define DWARF_LanguageXList(X)\
X(C89,            0x0001, 0)\
X(C,              0x0002, 0)\
X(Ada83,          0x0003, 1)\
X(C_plus_plus,    0x0004, 0)\
X(Cobol74,        0x0005, 1)\
X(Cobol85,        0x0006, 1)\
X(Fortran77,      0x0007, 1)\
X(Fortran90,      0x0008, 1)\
X(Pascal83,       0x0009, 1)\
X(Modula2,        0x000a, 1)\
X(Java,           0x000b, 0)\
X(C99,            0x000c, 0)\
X(Ada95,          0x000d, 1)\
X(Fortran95,      0x000e, 1)\
X(PLI,            0x000f, 1)\
X(ObjC,           0x0010, 0)\
X(ObjC_plus_plus, 0x0011, 0)\
X(UPC,            0x0012, 0)\
X(D,              0x0013, 0)\
X(Python,         0x0014, 0)\
X(OpenCL,         0x0015, 0)\
X(Go,             0x0016, 0)\
X(Modula3,        0x0017, 1)\
X(Haskell,        0x0018, 0)\
X(C_plus_plus_03, 0x0019, 0)\
X(C_plus_plus_11, 0x001a, 0)\
X(OCaml,          0x001b, 0)\
X(Rust,           0x001c, 0)\
X(C11,            0x001d, 0)\
X(Swift,          0x001e, 0)\
X(Julia,          0x001f, 1)\
X(Dylan,          0x0020, 0)\
X(C_plus_plus_14, 0x0021, 0)\
X(Fortran03,      0x0022, 1)\
X(Fortran08,      0x0023, 1)\
X(RenderScript,   0x0024, 0)\
X(BLISS,          0x0025, 0)\
X(lo_user,        0x8000, 0)\
X(hi_user,        0xffff, 0)

typedef enum DWARF_Language{
#define X(N,C,k) DWARF_Language_##N = C,
  DWARF_LanguageXList(X)
#undef X
} DWARF_Language;


// identifier case:  X(name, code)
#define DWARF_IdentifierCaseXList(X)\
X(case_sensitive,   0x00)\
X(up_case,          0x01)\
X(down_case,        0x02)\
X(case_insensitive, 0x03)

typedef enum DWARF_IdentifierCase{
#define X(N,C) DWARF_IdentifierCase_##N = C,
  DWARF_IdentifierCaseXList(X)
#undef X
} DWARF_IdentifierCase;


// calling convention:  X(name, code)
#define DWARF_CallingConventionXList(X)\
X(normal,            0x01)\
X(program,           0x02)\
X(nocall,            0x03)\
X(pass_by_reference, 0x04)\
X(pass_by_value,     0x05)\
X(lo_user,           0x40)\
X(hi_user,           0xff)

typedef enum DWARF_CallingConvention{
#define X(N,C) DWARF_CallingConvention_##N = C,
  DWARF_CallingConventionXList(X)
#undef X
} DWARF_CallingConvention;


// inline:  X(name, code)
#define DWARF_InlineXList(X)\
X(not_inlined, 0x00)\
X(inlined, 0x01)\
X(declared_not_inlined, 0x02)\
X(declared_inlined, 0x03)

typedef enum DWARF_Inline{
#define X(N,C) DWARF_Inline_##N = C,
  DWARF_InlineXList(X)
#undef X
} DWARF_Inline;


// array ordering:  X(name, code)
#define DWARF_ArrayOrderingXList(X)\
X(row_major, 0x00)\
X(col_major, 0x01)

typedef enum DWARF_ArrayOrdering{
#define X(N,C) DWARF_ArrayOrdering_##N = C,
  DWARF_ArrayOrderingXList(X)
#undef X
} DWARF_ArrayOrdering;


// discriminant:  X(name, code)
#define DWARF_DiscriminantXList(X)\
X(label, 0x00)\
X(range, 0x01)

typedef enum DWARF_Discriminant{
#define X(N,C) DWARF_Discriminant_##N = C,
  DWARF_DiscriminantXList(X)
#undef X
} DWARF_Discriminant;


// name index:  X(name, code)
#define DWARF_NameIndexXList(X)\
X(compile_unit, 1)\
X(type_unit,    2)\
X(die_offset,   3)\
X(parent,       4)\
X(type_hash,    5)\
X(lo_user, 0x2000)\
X(hi_user, 0x3fff)

typedef enum DWARF_NameIndex{
#define X(N,C) DWARF_NameIndex_##N = C,
  DWARF_NameIndexXList(X)
#undef X
} DWARF_NameIndex;


// defaulted:  X(name, code)
#define DWARF_DefaultedXList(X)\
X(no,           0x00)\
X(in_class,     0x01)\
X(out_of_class, 0x02)

typedef enum DWARF_Defaulted{
#define X(N,C) DWARF_Defaulted_##N = C,
  DWARF_DefaultedXList(X)
#undef X
} DWARF_Defaulted;

// call frame instruction:  X(N, hi2bits, matchlow, low6bits, operand1, operand2)
//  "CFA"
#define DWARF_CallFrameInsnXList(X)\
X(advance_loc,       0x1, 0,    0, NULL,  NULL)\
X(offset,            0x2, 0,    0, ULEB,  NULL)\
X(restore,           0x3, 0,    0, NULL,  NULL)\
X(nop,               0x0, 1,    0, NULL,  NULL)\
X(set_loc,           0x0, 1, 0x01, ADDRESS, NULL)\
X(advance_loc1,      0x0, 1, 0x02, 1BYTE, NULL)\
X(advance_loc2,      0x0, 1, 0x03, 2BYTE, NULL)\
X(advance_loc4,      0x0, 1, 0x04, 4BYTE, NULL)\
X(offset_extended,   0x0, 1, 0x05, ULEB,  ULEB)\
X(restore_extended,  0x0, 1, 0x06, ULEB,  NULL)\
X(undefined,         0x0, 1, 0x07, ULEB,  NULL)\
X(same_value,        0x0, 1, 0x08, ULEB,  NULL)\
X(register,          0x0, 1, 0x09, ULEB,  ULEB)\
X(remember_state,    0x0, 1, 0x0a, NULL,  NULL)\
X(restore_state,     0x0, 1, 0x0b, NULL,  NULL)\
X(def_cfa,           0x0, 1, 0x0c, ULEB,  ULEB)\
X(def_cfa_register,  0x0, 1, 0x0d, ULEB,  NULL)\
X(def_cfa_offset,    0x0, 1, 0x0e, ULEB,  NULL)\
X(def_cfa_expression,0x0, 1, 0x0f, BLOCK, NULL)\
X(expression,        0x0, 1, 0x10, ULEB,  BLOCK)\
X(offset_extended_sf,0x0, 1, 0x11, ULEB,  SLEB)\
X(def_cfa_sf,        0x0, 1, 0x12, ULEB,  SLEB)\
X(def_cfa_offset_sf, 0x0, 1, 0x13, SLEB,  NULL)\
X(val_offset,        0x0, 1, 0x14, ULEB,  ULEB)\
X(val_offset_sf,     0x0, 1, 0x15, ULEB,  SLEB)\
X(val_expression,    0x0, 1, 0x16, ULEB,  BLOCK)\
X(lo_user,           0x0, 1, 0x1c, NULL,  NULL)\
X(hi_user,           0x0, 1, 0x3f, NULL,  NULL)

// line number encoding codes
//  (DWARF4.pdf + 7.21) (DWARF5.pdf + 7.22)

//  X(name, code) (V4 & V5)
#define DWARF_LineStdOpXList(X) \
X(copy,               0x01)\
X(advance_pc,         0x02)\
X(advance_line,       0x03)\
X(set_file,           0x04)\
X(set_column,         0x05)\
X(negate_stmt,        0x06)\
X(set_basic_block,    0x07)\
X(const_add_pc,       0x08)\
X(fixed_advance_pc,   0x09)\
X(set_prologue_end,   0x0a)\
X(set_epilogue_begin, 0x0b)\
X(set_isa,            0x0c)

typedef enum DWARF_LineStdOp{
#define X(N,C) DWARF_LineStdOp_##N = C,
  DWARF_LineStdOpXList(X)
#undef X
} DWARF_LineStdOp;

//  X(name, code) (V4 & V5)
#define DWARF_LineExtOpXList(X) \
X(end_sequence,      0x01)\
X(set_address,       0x02)\
X(define_file,       0x03)\
X(set_discriminator, 0x04)\
X(lo_user,           0x80)\
X(hi_user,           0xff)

typedef enum DWARF_LineExtOp{
#define X(N,C) DWARF_LineExtOp_##N = C,
  DWARF_LineExtOpXList(X)
#undef X
} DWARF_LineExtOp;

//  X(name, code) (V5)
#define DWARF_LineEntryFormatXList(X) \
X(path,            0x1)\
X(directory_index, 0x2)\
X(timestamp,       0x3)\
X(size,            0x4)\
X(MD5,             0x5)\
X(lo_user,      0x2000)\
X(hi_user,      0x3fff)

typedef enum DWARF_LineEntryFormat{
#define X(N,C) DWARF_LineEntryFormat_##N = C,
  DWARF_LineEntryFormatXList(X)
#undef X
} DWARF_LineEntryFormat;

////////////////////////////////
//~ Dwarf Parser Codes and Data Tables

#define DWARF_SECTION_NAME_VARIANT_COUNT 3

//  X(section_code_name, versionflags, section_name0, section_name1, section_name2)
#define DWARF_SectionNameXList(X,V4,V5)\
X(Null,       0,     "", "", "")\
X(Loc,        V4,    ".debug_loc",         ".debug_loc.dwo",         "__debug_loc")\
X(Str,        V4|V5, ".debug_str",         ".debug_str.dwo",         "__debug_str")\
X(LineStr,    V5,    ".debug_line_str",    ".debug_line_str.dwo",    "__debug_line_str")\
X(CmpUnitIdx, V5,    ".debug_cu_index",    ".debug_cu_index.dwo",    "__debug_cu_index")\
X(TypeIdx,    V5,    ".debug_tu_index",    ".debug_tu_index.dwo",    "__debug_tu_index")\
X(Supplement, V5,    ".debug_sup",         ".debug_sup.dwo",         "__debug_sup")\
X(Info,       V4|V5, ".debug_info",        ".debug_info.dwo",        "__debug_info")\
X(Abbrev,     V4|V5, ".debug_abbrev",      ".debug_abbrev.dwo",      "__debug_abbrev")\
X(PubNames,   V4,    ".debug_pubnames",    ".debug_pubnames.dwo",    "__debug_pubnames")\
X(PubTypes,   V4,    ".debug_pubtypes",    ".debug_pubtypes.dwo",    "__debug_pubtypes")\
X(Names,      V5,    ".debug_names",       ".debug_names.dwo",       "__debug_names")\
X(Aranges,    V4|V5, ".debug_aranges",     ".debug_aranges.dwo",     "__debug_aranges")\
X(Line,       V4|V5, ".debug_line",        ".debug_line.dwo",        "__debug_line")\
X(MacInfo,    V4,    ".debug_macinfo",     ".debug_macinfo.dwo",     "__debug_macinfo")\
X(Macro,      V5,    ".debug_macro",       ".debug_macro.dwo",       "__debug_macro")\
X(Frame,      V4|V5, ".debug_frame",       ".debug_frame.dwo",       "__debug_frame")\
X(Ranges,     V4,    ".debug_ranges",      ".debug_ranges.dwo",      "__debug_ranges")\
X(StrOffsets, V5,    ".debug_str_offsets", ".debug_str_offsets.dwo", "__debug_str_offsets")\
X(Addr,       V5,    ".debug_addr",        ".debug_addr.dwo",        "__debug_addr")\
X(RngLists,   V5,    ".debug_rnglists",    ".debug_rnglists.dwo",    "__debug_rnglists")\
X(LocLists,   V5,    ".debug_loclists",    ".debug_loclists.dwo",    "__debug_loclists")


typedef enum DWARF_SectionCode{
#define X(c,vf,n0,n1,n2) DWARF_SectionCode_##c,
  DWARF_SectionNameXList(X,0,0)
#undef X
  DWARF_SectionCode_COUNT
} DWARF_SectionCode;

typedef struct DWARF_SectionNameRow{
  String8 name[DWARF_SECTION_NAME_VARIANT_COUNT];
} DWARF_SectionNameRow;

read_only global DWARF_SectionNameRow dwarf_section_name_table[] = {
#define X(c,vf,n0,n1,n2) \
{ { str8_lit_comp(n0), str8_lit_comp(n1), str8_lit_comp(n2) } },
  DWARF_SectionNameXList(X,0,0)
#undef X
};


#pragma pack(pop)


////////////////////////////////
//~ Dwarf Parser Types

typedef struct DWARF_Parsed{
  ELF_Parsed *elf;
  U32 debug_section_idx[DWARF_SectionCode_COUNT];
  String8 debug_section_name[DWARF_SectionCode_COUNT];
  String8 debug_data[DWARF_SectionCode_COUNT];
} DWARF_Parsed;


// form decoding

typedef struct DWARF_FormDecodeRules{
  union{
    // form decode fields
    struct{
      U8 size;
      B8 uleb128;
      B8 sleb128;
      B8 in_abbrev;
      B8 auto_1;
      B8 block;
      B8 null_terminated;
    };
    
    // for alignment and padding to 8
    U64 x;
  };
} DWARF_FormDecodeRules;

typedef struct DWARF_FormDecoded{
  U64 val;
  U8 *dataptr;
  B32 error;
} DWARF_FormDecoded;


// index section: .debug_cu_index .debug_tu_index
//  (DWARF5.pdf + 7.3.5)

// ** not implemented yet **

typedef struct DWARF_IndexParsed{
  U32 dummy;
} DWARF_IndexParsed;


// supplementary section: .debug_sup
//  (DWARF5.pdf + 7.3.6)

// ** not implemented yet **

typedef struct DWARF_SupParsed{
  U32 dummy;
} DWARF_SupParsed;


// info section: .debug_info
//  (DWARF4.pdb + 7.5) (DWARF5.pdf + 7.5)

typedef struct DWARF_InfoAttribVal{
  U64 val;
  U8 *dataptr;
} DWARF_InfoAttribVal;

typedef struct DWARF_InfoEntry{
  struct DWARF_InfoEntry *next_sibling;
  struct DWARF_InfoEntry *first_child;
  struct DWARF_InfoEntry *last_child;
  U64 child_count;
  struct DWARF_InfoEntry *parent;
  
  U64 info_offset;
  struct DWARF_AbbrevDecl *abbrev_decl;
  DWARF_InfoAttribVal *attrib_vals;
} DWARF_InfoEntry;

#if 0
typedef struct DWARF_InfoUnit{
  struct DWARF_InfoUnit *next;
  
  // header
  U32 version;
  U32 offset_size;
  U32 address_size;
  
  // root attributes
  DWARF_Language language;
  U64 line_info_offset;
  U64 vbase;
  U64 str_offsets_base;
  U64 addr_base;
  U64 rnglists_base;
  U64 loclists_base;
  
  // info entries
  DWARF_InfoEntry *entry_root;
  U64 entry_count;
} DWARF_InfoUnit;
#endif

#if 0
typedef struct DWARF_InfoParams{
  U64 unit_idx_min;
  U64 unit_idx_max;
} DWARF_InfoParams;
#endif

typedef struct DWARF_InfoUnit{
  struct DWARF_InfoUnit *next;
  
  U64 hdr_off;
  U64 base_off;
  U64 opl_off;
  
  U8  offset_size;
  U8  version;
  U8  unit_type; // (DWARF_UnitType)
  U8  address_size;
  U64 abbrev_off;
  
  union{
    // unit_type: skeleton, split_compile
    U64 dwo_id;
    // unit_type: type, split_type
    struct{
      U64 type_signature;
      U64 type_offset;
    };
  };
} DWARF_InfoUnit;

typedef struct DWARF_InfoParsed{
  DWARF_InfoUnit *unit_first;
  DWARF_InfoUnit *unit_last;
  U64 unit_count;
} DWARF_InfoParsed;


// abbreviations section: .debug_abbrev
//  (DWARF4.pdf + 7.5.3) (DWARF5.pdf + 7.5.3)

typedef struct DWARF_AbbrevAttribSpec{
  DWARF_AttributeName name;
  DWARF_AttributeForm form;
} DWARF_AbbrevAttribSpec;

typedef struct DWARF_AbbrevDecl{
  struct DWARF_AbbrevDecl *next;
  U32 abbrev_code;
  DWARF_Tag tag;
  B8 has_children;
  U8 __filler__;
  U16 attrib_count;
  DWARF_AbbrevAttribSpec *attrib_specs;
  S64 *implicit_const;
} DWARF_AbbrevDecl;

typedef struct DWARF_AbbrevUnit{
  struct DWARF_AbbrevUnit *next;
  U64 offset;
  DWARF_AbbrevDecl *first;
  DWARF_AbbrevDecl *last;
  U64 count;
} DWARF_AbbrevUnit;

#if 0
typedef struct DWARF_AbbrevParams{
  U64 unit_idx_min;
  U64 unit_idx_max;
} DWARF_AbbrevParams;
#endif

typedef struct DWARF_AbbrevParsed{
  DWARF_AbbrevUnit *unit_first;
  DWARF_AbbrevUnit *unit_last;
  U64 unit_count;
  B32 decoding_error;
} DWARF_AbbrevParsed;


// name lookup tables (V4): .debug_pubnames .debug_pubtypes
//  (DWARF4.pdf + 7.19)

typedef struct DWARF_PubNamesUnit{
  struct DWARF_PubNamesUnit *next;
  
  U64 hdr_off;
  U64 base_off;
  U64 opl_off;
  
  U8  offset_size;
  U8  version;
  U64 info_off;
  U64 info_length;
} DWARF_PubNamesUnit;

typedef struct DWARF_PubNamesParsed{
  DWARF_PubNamesUnit *unit_first;
  DWARF_PubNamesUnit *unit_last;
  U64 unit_count;
} DWARF_PubNamesParsed;


// name lookup tables (V5): .debug_names
//  (DWARF5.pdf + 6.1.1.4.1 & 7.19)

typedef struct DWARF_NamesUnit{
  struct DWARF_NamesUnit *next;
  
  U64 hdr_off;
  U64 base_off;
  U64 opl_off;
  
  U8 version;
  U32 comp_unit_count;
  U32 local_type_unit_count;
  U32 foreign_type_unit_count;
  U32 bucket_count;
  U32 name_count;
  U32 abbrev_table_size;
  String8 augmentation_string;
  
} DWARF_NamesUnit;

typedef struct DWARF_NamesParsed{
  DWARF_NamesUnit *unit_first;
  DWARF_NamesUnit *unit_last;
  U64 unit_count;
} DWARF_NamesParsed;


// address range table: .debug_aranges
//  (DWARF4.pdf + 7.20) (DWARF5.pdf + 7.21)

typedef struct DWARF_ArangesUnit{
  struct DWARF_ArangesUnit *next;
  
  U64 hdr_off;
  U64 base_off;
  U64 opl_off;
  
  U8 version;
  U8 address_size;
  U8 segment_selector_size;
  U8 offset_size;
  U64 info_off;
} DWARF_ArangesUnit;

typedef struct DWARF_ArangesParsed{
  DWARF_ArangesUnit *unit_first;
  DWARF_ArangesUnit *unit_last;
  U64 unit_count;
} DWARF_ArangesParsed;


// line number information: .debug_line
//  (DWARF4.pdf + 6.2.4 & 7.21) (DWARF5.pdf + 6.2.4 & 7.22)

typedef struct DWARF_V4LineFileNamesEntry{
  struct DWARF_V4LineFileNamesEntry *next;
  String8 file_name;
  U64 include_directory_idx;
  U64 last_modified_time;
  U64 file_size;
} DWARF_V4LineFileNamesEntry;

typedef struct DWARF_V4LineFileNamesList{
  DWARF_V4LineFileNamesEntry *first;
  DWARF_V4LineFileNamesEntry *last;
  U64 count;
} DWARF_V4LineFileNamesList;

typedef struct DWARF_V5LinePathEntryFormat{
  U32 content_type; /* DWARF_LineEntryFormat */
  U32 form;         /* DWARF_AttributeForm */
} DWARF_V5LinePathEntryFormat;

typedef struct DWARF_V5Directory{
  String8 path_str;
  U64 path_off;
  U64 path_sec_form;
  U64 directory_index;
  U64 timestamp;
  U64 size;
  U8 md5_checksum[16];
} DWARF_V5Directory;

typedef struct DWARF_LineUnit{
  struct DWARF_LineUnit *next;
  
  U64 hdr_off;
  U64 base_off;
  U64 opl_off;
  
  U8 version;
  
} DWARF_LineUnit;

typedef struct DWARF_LineParsed{
  DWARF_LineUnit *unit_first;
  DWARF_LineUnit *unit_last;
  U64 unit_count;
} DWARF_LineParsed;


// macro information (V4): .debug_macinfo
//  (DWARF4.pdf + 7.22)

// ** not implemented yet **

typedef struct DWARF_MacInfoParsed{
  U32 dummy;
} DWARF_MacInfoParsed;


// macro information (V5): .debug_macro
//  (DWARF5.pdf + 7.23)

// ** not implemented yet **

typedef struct DWARF_MacroParsed{
  U32 dummy;
} DWARF_MacroParsed;


// call frame information: .debug_frame
//  (DWARF4.pdf + 7.23) (DWARF5.pdf + 7.24)

// ** not implemented yet **

typedef struct DWARF_FrameParsed{
  U32 dummy;
} DWARF_FrameParsed;


// range lists (V4): .debug_ranges
//  (DWARF4.pdf + 7.24)

// ** not implemented yet **

typedef struct DWARF_RangesParsed{
  U32 dummy;
} DWARF_RangesParsed;


// string offsets table: .debug_str_offsets
//  (DWARF5.pdf + 7.26)

// ** not implemented yet **

typedef struct DWARF_StrOffsetsParsed{
  U32 dummy;
} DWARF_StrOffsetsParsed;


// address table: .debug_addr
//  (DWARF5.pdf + 7.27)

typedef struct DWARF_AddrUnit{
  struct DWARF_AddrUnit *next;
  
  U64 hdr_off;
  U64 base_off;
  U64 opl_off;
  
  U8 offset_size;
  U8 dwarf_version;
  U8 address_size;
  U8 segment_selector_size;
} DWARF_AddrUnit;

typedef struct DWARF_AddrParsed{
  DWARF_AddrUnit *unit_first;
  DWARF_AddrUnit *unit_last;
  U64 unit_count;
} DWARF_AddrParsed;


// range lists (V5): .debug_rnglists
//  (DWARF5.pdf + 7.28 & 7.25)

// ** not implemented yet **

typedef struct DWARF_RngListsParsed{
  U32 dummy;
} DWARF_RngListsParsed;


// location lists: .debug_loclists
//  (DWARF5.pdf + 7.29)

// ** not implemented yet **

typedef struct DWARF_LocListsParsed{
  U32 dummy;
} DWARF_LocListsParsed;


////////////////////////////////
//~ Dwarf Decode Helpers

#define DWARF_LEB128_ADV(p,o,s) do{ for(;; (p)+=1){\
if ((p) == (o))         { (s)=0;  break; }   \
if (((*(p))&0x80) == 0) { (p)+=1; break; }   \
} }while(0)

#define DWARF_LEB128_ADV_NOCAP(p) for((p)+=1; ((*(p-1))&0x80) != 0; (p)+=1)

static U64 dwarf_leb128_decode_U64(U8 *ptr, U8 *opl);
static S64 dwarf_leb128_decode_S64(U8 *ptr, U8 *opl);
static U32 dwarf_leb128_decode_U32(U8 *ptr, U8 *opl);

#define dwarf_leb128_decode(T,ptr,opl) dwarf_leb128_decode_##T(ptr,opl)

#define DWARF_LEB128_DECODE_ADV(T,x,p,o) do{ \
U8 *first__ = (p); B32 success__;          \
DWARF_LEB128_ADV(p,o,success__);           \
if (success__)                             \
(x) = dwarf_leb128_decode(T,first__, (p)); \
}while(0)


////////////////////////////////
//~ allen: ELF/DW Unwind Types
//
// TODO(rjf): OLD TYPES FROM UNWINDER CODE. bucketing this here, and deferring dwarf-based
// unwinding info to future DWARF/linux work.
//
#if 0

// * applies to (any X: unwind(ELF/DW, X))

// EH: Exception Frames
typedef U8 UNW_DW_EhPtrEnc;
enum{
  UNW_DW_EhPtrEnc_TYPE_MASK = 0x0F,
  UNW_DW_EhPtrEnc_PTR     = 0x00, // Pointer sized unsigned value
  UNW_DW_EhPtrEnc_ULEB128 = 0x01, // Unsigned LE base-128 value
  UNW_DW_EhPtrEnc_UDATA2  = 0x02, // Unsigned 16-bit value
  UNW_DW_EhPtrEnc_UDATA4  = 0x03, // Unsigned 32-bit value
  UNW_DW_EhPtrEnc_UDATA8  = 0x04, // Unsigned 64-bit value
  UNW_DW_EhPtrEnc_SIGNED  = 0x08, // Signed pointer
  UNW_DW_EhPtrEnc_SLEB128 = 0x09, // Signed LE base-128 value
  UNW_DW_EhPtrEnc_SDATA2  = 0x0A, // Signed 16-bit value
  UNW_DW_EhPtrEnc_SDATA4  = 0x0B, // Signed 32-bit value
  UNW_DW_EhPtrEnc_SDATA8  = 0x0C, // Signed 64-bit value
};
enum{
  UNW_DW_EhPtrEnc_MODIF_MASK = 0x70,
  UNW_DW_EhPtrEnc_PCREL   = 0x10, // Value is relative to the current program counter.
  UNW_DW_EhPtrEnc_TEXTREL = 0x20, // Value is relative to the .text section.
  UNW_DW_EhPtrEnc_DATAREL = 0x30, // Value is relative to the .got or .eh_frame_hdr section.
  UNW_DW_EhPtrEnc_FUNCREL = 0x40, // Value is relative to the function.
  UNW_DW_EhPtrEnc_ALIGNED = 0x50, // Value is aligned to an address unit sized boundary.
};
enum{
  UNW_DW_EhPtrEnc_INDIRECT = 0x80, // This flag indicates that value is stored in virtual memory.
  UNW_DW_EhPtrEnc_OMIT     = 0xFF,
};

typedef struct UNW_DW_EhPtrCtx{
  U64 raw_base_vaddr; // address where pointer is being read
  U64 text_vaddr;     // base address of section with instructions (used for encoding pointer on SH and IA64)
  U64 data_vaddr;     // base address of data section (used for encoding pointer on x86-64)
  U64 func_vaddr;     // base address of function where IP is located
} UNW_DW_EhPtrCtx;

// CIE: Common Information Entry
typedef struct UNW_DW_CIEUnpacked{
  U8 version;
  UNW_DW_EhPtrEnc lsda_encoding;
  UNW_DW_EhPtrEnc addr_encoding;
  
  B8 has_augmentation_size;
  U64 augmentation_size;
  String8 augmentation;
  
  U64 code_align_factor;
  S64 data_align_factor;
  U64 ret_addr_reg;
  
  U64 handler_ip;
  
  U64 cfi_range_min;
  U64 cfi_range_max;
} UNW_DW_CIEUnpacked;

typedef struct UNW_DW_CIEUnpackedNode{
  struct UNW_DW_CIEUnpackedNode *next;
  UNW_DW_CIEUnpacked cie;
  U64 offset;
} UNW_DW_CIEUnpackedNode;

// FDE: Frame Description Entry
typedef struct UNW_DW_FDEUnpacked{
  U64 ip_voff_min;
  U64 ip_voff_max;
  U64 lsda_ip;
  
  U64 cfi_range_min;
  U64 cfi_range_max;
} UNW_DW_FDEUnpacked;

// CFI: Call Frame Information
typedef struct UNW_DW_CFIRecords{
  B32 valid;
  UNW_DW_CIEUnpacked cie;
  UNW_DW_FDEUnpacked fde;
} UNW_DW_CFIRecords;

typedef enum UNW_DW_CFICFARule{
  UNW_DW_CFICFARule_REGOFF,
  UNW_DW_CFICFARule_EXPR,
} UNW_DW_CFICFARule;

typedef struct UNW_DW_CFICFACell{
  UNW_DW_CFICFARule rule;
  union{
    struct{
      U64 reg_idx;
      S64 offset;
    };
    U64 expr_min;
    U64 expr_max;
  };
} UNW_DW_CFICFACell;

typedef enum UNW_DW_CFIRegisterRule{
  UNW_DW_CFIRegisterRule_SAME_VALUE,
  UNW_DW_CFIRegisterRule_UNDEFINED,
  UNW_DW_CFIRegisterRule_OFFSET,
  UNW_DW_CFIRegisterRule_VAL_OFFSET,
  UNW_DW_CFIRegisterRule_REGISTER,
  UNW_DW_CFIRegisterRule_EXPRESSION,
  UNW_DW_CFIRegisterRule_VAL_EXPRESSION,
} UNW_DW_CFIRegisterRule;

typedef struct UNW_DW_CFICell{
  UNW_DW_CFIRegisterRule rule;
  union{
    S64 n;
    struct{
      U64 expr_min;
      U64 expr_max;
    };
  };
} UNW_DW_CFICell;

typedef struct UNW_DW_CFIRow{
  struct UNW_DW_CFIRow *next;
  UNW_DW_CFICell *cells;
  UNW_DW_CFICFACell cfa_cell;
} UNW_DW_CFIRow;

typedef struct UNW_DW_CFIMachine{
  U64 cells_per_row;
  UNW_DW_CIEUnpacked *cie;
  UNW_DW_EhPtrCtx *ptr_ctx;
  UNW_DW_CFIRow *initial_row;
  U64 fde_ip;
} UNW_DW_CFIMachine;

typedef U8 UNW_DW_CFADecode;
enum{
  UNW_DW_CFADecode_NOP     = 0x0,
  // 1,2,4,8 reserved for literal byte sizes
  UNW_DW_CFADecode_ADDRESS = 0x9,
  UNW_DW_CFADecode_ULEB128 = 0xA,
  UNW_DW_CFADecode_SLEB128 = 0xB,
};

typedef U16 UNW_DW_CFAControlBits;
enum{
  UNW_DW_CFAControlBits_DEC1_MASK = 0x00F,
  UNW_DW_CFAControlBits_DEC2_MASK = 0x0F0,
  UNW_DW_CFAControlBits_IS_REG_0  = 0x100,
  UNW_DW_CFAControlBits_IS_REG_1  = 0x200,
  UNW_DW_CFAControlBits_IS_REG_2  = 0x400,
  UNW_DW_CFAControlBits_NEW_ROW   = 0x800,
};
#endif

////////////////////////////////
//~ Dwarf Parser Functions

static DWARF_Parsed*         dwarf_parsed_from_elf(Arena *arena, ELF_Parsed *elf);

static DWARF_IndexParsed*    dwarf_index_from_data(Arena *arena, String8 data);
static DWARF_SupParsed*      dwarf_sup_from_data(Arena *arena, String8 data);
static DWARF_InfoParsed*     dwarf_info_from_data(Arena *arena, String8 data);
static DWARF_PubNamesParsed* dwarf_pubnames_from_data(Arena *arena, String8 data);
static DWARF_NamesParsed*    dwarf_names_from_data(Arena *arena, String8 data);
static DWARF_ArangesParsed*  dwarf_aranges_from_data(Arena *arena, String8 data);
static DWARF_LineParsed*     dwarf_line_from_data(Arena *arena, String8 data);
static DWARF_MacInfoParsed*  dwarf_mac_info_from_data(Arena *arena, String8 data);
static DWARF_MacroParsed*    dwarf_macro_from_data(Arena *arena, String8 data);
static DWARF_FrameParsed*    dwarf_frame_from_data(Arena *arena, String8 data);
static DWARF_RangesParsed*   dwarf_ranges_from_data(Arena *arena, String8 data);
static DWARF_StrOffsetsParsed* dwarf_str_offsets_from_data(Arena *arena, String8 data);
static DWARF_AddrParsed*     dwarf_addr_from_data(Arena *arena, String8 data);
static DWARF_RngListsParsed* dwarf_rng_lists_from_data(Arena *arena, String8 data);
static DWARF_LocListsParsed* dwarf_loc_lists_from_data(Arena *arena, String8 data);


// parse helpers

// (DWARF4.pdf + 7.2.2) (DWARF5.pdf + 7.2.2)
static void dwarf__initial_length(String8 data,
                                  U8 **ptr_inout, U8 **unit_opl_out, B32 *is_64bit_out);

static void
dwarf__line_v5_directories(U64 address_size, U64 offset_size,
                           DWARF_V5LinePathEntryFormat *format, U64 format_count,
                           DWARF_V5Directory *directories_out, U64 dir_count,
                           U8 **ptr_io, U8 *opl);

// debug sections

static String8 dwarf_name_from_debug_section(DWARF_Parsed *dwarf, DWARF_SectionCode sec_code);

// abbrev functions

static DWARF_AbbrevUnit* dwarf_abbrev_unit_from_offset(DWARF_AbbrevParsed *abbrev, U64 off);
static DWARF_AbbrevDecl* dwarf_abbrev_decl_from_code(DWARF_AbbrevUnit *unit, U32 code);

// attribute decoding functions

static DWARF_AttributeClassFlags dwarf_attribute_class_from_form(DWARF_AttributeForm form);
static DWARF_AttributeClassFlags dwarf_attribute_class_from_name(DWARF_AttributeName name);

// form decoding functions

static DWARF_FormDecodeRules
dwarf_form_decode_rule(DWARF_AttributeForm form, U64 address_size, U64 offset_size);

static DWARF_FormDecoded
dwarf_form_decode(DWARF_FormDecodeRules *rules, U8 **ptr_io, U8 *opl,
                  DWARF_AbbrevDecl *abbrev_decl, U32 attrib_i);

// string functions

static String8 dwarf_string_from_unit_type(DWARF_UnitType type);
static String8 dwarf_string_from_tag(DWARF_Tag tag);
static String8 dwarf_string_from_attribute_name(DWARF_AttributeName name);
static String8 dwarf_string_from_attribute_form(DWARF_AttributeForm form);
static String8 dwarf_string_from_line_std_op(DWARF_LineStdOp op);
static String8 dwarf_string_from_line_ext_op(DWARF_LineExtOp op);
static String8 dwarf_string_from_line_entry_format(DWARF_LineEntryFormat format);
static String8 dwarf_string_from_section_code(DWARF_SectionCode sec_code);

#endif //RDI_DWARF_H

