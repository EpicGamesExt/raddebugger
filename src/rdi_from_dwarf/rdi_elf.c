// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ ELF Parser Functions

static ELF_Parsed*
elf_parsed_from_data(Arena *arena, String8 elf_data){
  //- test magic number
  B32 has_good_magic_number = 0;
  if (elf_data.size >= sizeof(ELF_NIDENT) &&
      MemoryMatch(elf_data.str, elf_magic, sizeof(elf_magic))){
    has_good_magic_number = 1;
  }
  
  //- determine elf class
  U8 elf_class = ELF_Class_NONE;
  if (has_good_magic_number){
    elf_class = elf_data.str[ELF_Identification_CLASS];
  }
  
  //- extract header information
  B32 decoded_header = 0;
  
  U8 e_data_encoding = ELF_DataEncoding_NONE;
  U16 e_machine = ELF_Machine_NONE;
  
  U64 e_entry = 0;
  
  U64 e_shoff = 0;
  U16 e_shentsize = 0;
  U16 e_shnum = 0;
  
  U64 e_phoff = 0;
  U16 e_phentsize = 0;
  U16 e_phnum = 0;
  
  U16 e_shstrndx = 0;
  
  switch (elf_class){
    case ELF_Class_NONE: /* not good */ break;
    
    case ELF_Class_32:
    {
      if (elf_data.size >= sizeof(ELF_Ehdr32)){
        ELF_Ehdr32 *hdr = (ELF_Ehdr32*)elf_data.str;
        
        decoded_header = 1;
        e_data_encoding = hdr->e_ident[ELF_Identification_DATA];
        e_machine = hdr->e_machine;
        e_entry = hdr->e_entry;
        e_phoff = hdr->e_phoff;
        e_shoff = hdr->e_shoff;
        e_phentsize = hdr->e_phentsize;
        e_phnum = hdr->e_phnum;
        e_shentsize = hdr->e_shentsize;
        e_shnum = hdr->e_shnum;
        e_shstrndx = hdr->e_shstrndx;
      }
    }break;
    
    case ELF_Class_64:
    {
      if (elf_data.size >= sizeof(ELF_Ehdr64)){
        ELF_Ehdr64 *hdr = (ELF_Ehdr64*)elf_data.str;
        
        decoded_header = 1;
        e_data_encoding = hdr->e_ident[ELF_Identification_DATA];
        e_machine = hdr->e_machine;
        e_entry = hdr->e_entry;
        e_phoff = hdr->e_phoff;
        e_shoff = hdr->e_shoff;
        e_phentsize = hdr->e_phentsize;
        e_phnum = hdr->e_phnum;
        e_shentsize = hdr->e_shentsize;
        e_shnum = hdr->e_shnum;
        e_shstrndx = hdr->e_shstrndx;
      }
    }break;
  }
  
  //- validate & translate header values
  B32 header_is_good = 0;
  Architecture arch = Architecture_Null;
  if (decoded_header){
    header_is_good = 1;
    
    // only supporting little-endian versions right now
    if (header_is_good){
      if (e_data_encoding != ELF_DataEncoding_2LSB){
        header_is_good = 0;
      }
    }
    
    // make sure this is a supported machine type
    if (header_is_good){
      switch (e_machine){
        default: header_is_good = 0;
        case ELF_Machine_386:    arch = Architecture_x86; break;
        case ELF_Machine_X86_64: arch = Architecture_x64; break;
      }
    }
    
    // make sure section & segment sizes are correct
    if (header_is_good){
      switch (elf_class){
        case ELF_Class_32:
        {
          if (e_shentsize != sizeof(ELF_Shdr32) ||
              e_phentsize != sizeof(ELF_Phdr32)){
            header_is_good = 0;
          }
        }break;
        case ELF_Class_64:
        {
          if (e_shentsize != sizeof(ELF_Shdr64) ||
              e_phentsize != sizeof(ELF_Phdr64)){
            header_is_good = 0;
          }
        }break;
      }
    }
  }
  
  //- extract extra information from the special first section
  U64 section_count_raw = e_shnum;
  U32 section_header_string_table_index = e_shstrndx;
  if (header_is_good){
    if (e_shoff <= elf_data.size && e_shentsize <= elf_data.size &&
        e_shoff + e_shentsize <= elf_data.size){
      U64 size = 0;
      U32 link = 0;
      switch (elf_class){
        case ELF_Class_32:
        {
          ELF_Shdr32 *shdr = (ELF_Shdr32*)(elf_data.str + e_shoff);
          size = shdr->sh_size;
          link = shdr->sh_link;
        }break;
        case ELF_Class_64:
        {
          ELF_Shdr64 *shdr = (ELF_Shdr64*)(elf_data.str + e_shoff);
          size = shdr->sh_size;
          link = shdr->sh_link;
        }break;
      }
      
      // extended section count
      if (size != 0){
        section_count_raw = size;
      }
      
      // extended section header string table index
      if (link != 0){
        section_header_string_table_index = link;
      }
    }
  }
  
  //- clamp section & program arrays to size
  U64 section_foff = 0;
  U64 section_size = 0;
  U64 section_count = 0;
  
  U64 segment_foff = 0;
  U64 segment_size = 0;
  U64 segment_count = 0;
  
  if (header_is_good){
    if (e_shentsize > 0){
      U64 section_opl_raw = e_shoff + e_shentsize*section_count_raw;
      U64 section_opl = ClampTop(section_opl_raw, elf_data.size);
      if (section_opl > e_shoff){
        section_foff = e_shoff;
        section_size = e_shentsize;
        section_count = (section_opl - e_shoff)/e_shentsize;
      }
    }
    
    if (e_phentsize > 0){
      U64 segment_opl_raw = e_phoff + e_phentsize*e_phnum;
      U64 segment_opl = ClampTop(segment_opl_raw, elf_data.size);
      if (segment_opl > e_phoff){
        segment_foff = e_phoff;
        segment_size = e_phentsize;
        segment_count = (segment_opl - e_phoff)/e_phentsize;
      }
    }
  }
  
  //- determine the vbase for this file
  U64 vbase = 0;
  if (header_is_good){
    // find the first LOAD segment
    U64 load_segment_off = 0;
    {
      U64 segment_cursor = segment_foff;
      U64 segment_opl = segment_foff + segment_size*segment_count;
      for (;segment_cursor < segment_opl; segment_cursor += segment_size){
        U32 p_type = *(U32*)(elf_data.str + segment_cursor);
        if (p_type == ELF_SegmentType_LOAD){
          load_segment_off = segment_cursor;
          break;
        }
      }
    }
    
    // use the segment's p_vaddr to determine vbase
    if (load_segment_off != 0){
      switch (elf_class){
        case ELF_Class_32:
        {
          ELF_Phdr32 *phdr = (ELF_Phdr32*)(elf_data.str + load_segment_off);
          vbase = phdr->p_vaddr;
        }break;
        case ELF_Class_64:
        {
          ELF_Phdr64 *phdr = (ELF_Phdr64*)(elf_data.str + load_segment_off);
          vbase = phdr->p_vaddr;
        }break;
      }
    }
  }
  
  //- locate the section header string table
  U64 section_name_table_foff = 0;
  U64 section_name_table_opl = 0;
  if (header_is_good){
    if (section_header_string_table_index < section_count){
      U64 sec_foff = section_foff + section_header_string_table_index*section_size;
      switch (elf_class){
        case ELF_Class_32:
        {
          ELF_Shdr32 *shdr = (ELF_Shdr32*)(elf_data.str + sec_foff);
          section_name_table_foff = shdr->sh_offset;
          section_name_table_opl  = shdr->sh_offset + shdr->sh_size;
        }break;
        case ELF_Class_64:
        {
          ELF_Shdr64 *shdr = (ELF_Shdr64*)(elf_data.str + sec_foff);
          section_name_table_foff = shdr->sh_offset;
          section_name_table_opl  = shdr->sh_offset + shdr->sh_size;
        }break;
      }
    }
  }
  
  //- format sections data
  ELF_Shdr64 *sections = 0;
  if (header_is_good && section_count > 0){
    switch (elf_class){
      case ELF_Class_32:
      {
        sections = push_array(arena, ELF_Shdr64, section_count);
        {
          ELF_Shdr32 *shdr32 = (ELF_Shdr32*)(elf_data.str + section_foff);
          ELF_Shdr64 *shdr64 = sections;
          for (U64 i = 0; i < section_count; i += 1, shdr32 += 1, shdr64 += 1){
            shdr64->sh_name = shdr32->sh_name;
            shdr64->sh_type = shdr32->sh_type;
            shdr64->sh_flags = shdr32->sh_flags;
            shdr64->sh_addr = shdr32->sh_addr;
            shdr64->sh_offset = shdr32->sh_offset;
            shdr64->sh_size = shdr32->sh_size;
            shdr64->sh_link = shdr32->sh_link;
            shdr64->sh_info = shdr32->sh_info;
            shdr64->sh_addralign = shdr32->sh_addralign;
            shdr64->sh_entsize = shdr32->sh_entsize;
          }
        }
      }break;
      case ELF_Class_64:
      {
        sections = (ELF_Shdr64*)(elf_data.str + section_foff);
      }break;
    }
  }
  
  //- extract section names
  String8 *section_names = 0;
  if (sections != 0 && section_count > 0){
    U8 *string_table_opl = elf_data.str + section_name_table_opl;
    
    section_names = push_array(arena, String8, section_count);
    String8 *sec_name = section_names;
    ELF_Shdr64 *sec = sections;
    for (U64 i = 0;
         i < section_count;
         i += 1, sec += 1, sec_name += 1){
      U64 name_foff = section_name_table_foff + sec->sh_name;
      if (section_name_table_foff <= name_foff && name_foff < section_name_table_opl){
        U8 *base = elf_data.str + name_foff;
        U8 *opl = base;
        for (;opl < string_table_opl && *opl != 0; opl += 1);
        sec_name->str = base;
        sec_name->size = (U64)(opl - base);
      }
    }
  }
  
  //- format segments data
  ELF_Phdr64 *segments = 0;
  if (header_is_good && segment_count > 0){
    switch (elf_class){
      case ELF_Class_32:
      {
        segments = push_array(arena, ELF_Phdr64, segment_count);
        {
          ELF_Phdr32 *phdr32 = (ELF_Phdr32*)(elf_data.str + segment_foff);
          ELF_Phdr64 *phdr64 = segments;
          for (U64 i = 0; i < segment_count; i += 1, phdr32 += 1, phdr64 += 1){
            phdr64->p_type = phdr32->p_type;
            phdr64->p_flags = phdr32->p_flags;
            phdr64->p_offset = phdr32->p_offset;
            phdr64->p_vaddr = phdr32->p_vaddr;
            phdr64->p_paddr = phdr32->p_paddr;
            phdr64->p_filesz = phdr32->p_filesz;
            phdr64->p_memsz = phdr32->p_memsz;
            phdr64->p_align = phdr32->p_align;
          }
        }
      }break;
      case ELF_Class_64:
      {
        segments = (ELF_Phdr64*)(elf_data.str + segment_foff);
      }break;
    }
  }
  
  //- find special sections
  U64 strtab_idx = 0;
  U64 symtab_idx = 0;
  U64 dynsym_idx = 0;
  if (section_names != 0){
    for (U64 i = 0; i < section_count; i += 1){
      String8 name = section_names[i];
      if (str8_match(name, str8_lit(".strtab"), 0)){
        strtab_idx = i;
      }
      else if (str8_match(name, str8_lit(".symtab"), 0)){
        symtab_idx = i;
      }
      else if (str8_match(name, str8_lit(".dynsym"), 0)){
        dynsym_idx = i;
      }
    }
  }
  
  
  //- fill result
  ELF_Parsed *result = 0;
  if (header_is_good){
    result = push_array(arena, ELF_Parsed, 1);
    result->data = elf_data;
    result->elf_class = elf_class;
    result->arch = arch;
    result->sections = sections;
    result->section_names = section_names;
    result->section_foff = section_foff;
    result->section_count = section_count;
    result->segments = segments;
    result->segment_foff = segment_foff;
    result->segment_count = segment_count;
    result->vbase = vbase;
    result->entry_vaddr = e_entry;
    result->section_name_table_foff = section_name_table_foff;
    result->section_name_table_opl  = section_name_table_opl;
    result->strtab_idx = strtab_idx;
    result->symtab_idx = symtab_idx;
    result->dynsym_idx = dynsym_idx;
  }
  
  return(result);
}

static ELF_SectionArray
elf_section_array_from_elf(ELF_Parsed *elf){
  ELF_SectionArray result = {0};
  if (elf != 0){
    result.sections = elf->sections;
    result.count = elf->section_count;
  }
  return(result);
}

static String8Array
elf_section_name_array_from_elf(ELF_Parsed *elf){
  String8Array result = {0};
  if (elf != 0){
    result.v = elf->section_names;
    result.count = elf->section_count;
  }
  return(result);
}

static ELF_SegmentArray
elf_segment_array_from_elf(ELF_Parsed *elf){
  ELF_SegmentArray result = {0};
  if (elf != 0){
    result.segments = elf->segments;
    result.count = elf->segment_count;
  }
  return(result);
}

static String8
elf_section_name_from_name_offset(ELF_Parsed *elf, U64 offset){
  String8 result = {0};
  if (elf != 0){
    if (offset > 0){
      U64 foff = elf->section_name_table_foff + offset;
      if (elf->section_name_table_foff <= foff && foff < elf->section_name_table_opl){
        U8 *base = elf->data.str + foff;
        U8 *section_opl = elf->data.str + elf->section_name_table_opl;
        U8 *opl = base;
        for (;opl < section_opl && *opl != 0; opl += 1);
        result.str = base;
        result.size = opl - base;
      }
    }
  }
  return(result);
}

static String8
elf_section_name_from_idx(ELF_Parsed *elf, U32 idx){
  String8 result = {0};
  if (elf != 0){
    if (idx < elf->section_count){
      result = elf->section_names[idx];
    }
  }
  return(result);
}

static U32
elf_section_idx_from_name(ELF_Parsed *elf, String8 name){
  U32 result = 0;
  if (elf != 0){
    String8 *sec_name = elf->section_names;
    U64 count = elf->section_count;
    for (U64 i = 0; i < count; i += 1, sec_name += 1){
      if (str8_match(*sec_name, name, 0)){
        result = i;
        break;
      }
    }
  }
  return(result);
}

static String8
elf_section_data_from_idx(ELF_Parsed *elf, U32 idx){
  String8 result = {0};
  if (elf != 0){
    if (idx < elf->section_count){
      ELF_Shdr64 *shdr = elf->sections + idx;
      U64 off_raw = shdr->sh_offset;
      U64 size = shdr->sh_size;
      if (shdr->sh_flags & ELF_SectionType_NOBITS){
        size = 0;
      }
      U64 opl_raw = off_raw + size;
      U64 opl = ClampTop(opl_raw, elf->data.size);
      U64 off = ClampTop(off_raw, opl);
      result.str = elf->data.str + off;
      result.size = opl - off;
    }
  }
  return(result);
}

static ELF_SymArray
elf_sym_array_from_data(Arena *arena, ELF_Class elf_class, String8 data){
  // converge to sym64 layout
  ELF_Sym64 *symbols = 0;
  U64 count = 0;
  switch (elf_class){
    case ELF_Class_32:
    {
      count = data.size/sizeof(ELF_Sym32);
      symbols = push_array(arena, ELF_Sym64, count);
      {
        ELF_Sym32 *sym32 = (ELF_Sym32*)(data.str);
        ELF_Sym64 *sym64 = symbols;
        for (U64 i = 0; i < count; i += 1, sym32 += 1, sym64 += 1){
          sym64->st_name  = sym32->st_name;
          sym64->st_value = sym32->st_value;
          sym64->st_size  = sym32->st_size;
          sym64->st_info  = sym32->st_info;
          sym64->st_other = sym32->st_other;
          sym64->st_shndx = sym32->st_shndx;
        }
      }
    }break;
    
    case ELF_Class_64:
    {
      count = data.size/sizeof(ELF_Sym64);
      symbols = (ELF_Sym64*)(data.str);
    }break;
  }
  
  // fill result
  ELF_SymArray result = {0};
  result.symbols = symbols;
  result.count = count;
  return(result);
}

// string functions

static String8
elf_string_from_section_type(ELF_SectionType section_type){
  String8 result = str8_lit("INVALID_SECTION_TYPE");
  switch (section_type){
#define X(N,C) case C: result = str8_lit(#N); break;
    ELF_SectionTypeXList(X)
#undef X
  }
  return(result);
}

static String8
elf_string_from_symbol_binding(ELF_SymbolBinding binding){
  String8 result = str8_lit("INVALID_SYMBOL_BINDING");
  switch (binding){
#define X(N,C) case C: result = str8_lit(#N); break;
    ELF_SymbolBindingXList(X)
#undef X
  }
  return(result);
}

static String8
elf_string_from_symbol_type(ELF_SymbolType type){
  String8 result = str8_lit("INVALID_SYMBOL_TYPE");
  switch (type){
#define X(N,C) case C: result = str8_lit(#N); break;
    ELF_SymbolTypeXList(X)
#undef X
  }
  return(result);
}

static String8
elf_string_from_symbol_visibility(ELF_SymbolVisibility visibility){
  String8 result = str8_lit("INVALID_SYMBOL_VISIBILITY");
  switch (visibility){
#define X(N,C) case C: result = str8_lit(#N); break;
    ELF_SymbolVisibilityXList(X)
#undef X
  }
  return(result);
}
