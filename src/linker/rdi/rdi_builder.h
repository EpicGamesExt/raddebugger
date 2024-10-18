// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

StaticAssert(sizeof(RDI_Header) == AlignPow2(sizeof(RDI_Header), 8), g_rdi_header_align_check);

////////////////////////////////
// TODO: move to rdi_format.h

#define RDI_IsCompleteUserDefinedTypeKind(x)   ((x) == RDI_TypeKind_Class || (x) == RDI_TypeKind_Struct || (x) == RDI_TypeKind_Union || (x) == RDI_TypeKind_Enum)
#define RDI_IsIncompleteUsedDefinedTypeKind(x) (RDI_TypeKind_FirstIncomplete <= (x) && (x) <= RDI_TypeKind_LastIncomplete)
#define RDI_IsUserDefinedType(x)               (RDI_IsCompleteUserDefinedTypeKind(x) || RDI_IsIncompleteUsedDefinedTypeKind(x))
#define RDI_IsBuiltinType(x)                   (RDI_TypeKind_FirstBuiltIn <= (x) && (x) <= RDI_TypeKind_LastBuiltIn)
#define RDI_IsPtrType(x)                       ((x) == RDI_TypeKind_Ptr || (x) == RDI_TypeKind_LRef || (x) == RDI_TypeKind_RRef)

typedef enum
{
  RDI_Checksum_Null,
  RDI_Checksum_MD5,
  RDI_Checksum_SHA1,
  RDI_Checksum_SHA256,
  RDI_Checksum_TimeStamp
} RDI_ChecksumKind;

////////////////////////////////

typedef enum
{
  RDIB_DataModel_Null,
  RDIB_DataModel_ILP32,
  RDIB_DataModel_LLP64,
  RDIB_DataModel_LP64,
  RDIB_DataModel_ILP64,
  RDIB_DataModel_SILP64
} RDIB_DataModel;

////////////////////////////////

typedef void * RDIB_TypeRef;

typedef struct RDIB_EvalBytecodeOp
{
  struct RDIB_EvalBytecodeOp *next;
  RDI_EvalOp                  op;
  U64                         p_size;
  U64                         p;
} RDIB_EvalBytecodeOp;

typedef struct RDIB_EvalBytecode
{
  U64                  count;
  U64                  size;
  RDIB_EvalBytecodeOp *first;
  RDIB_EvalBytecodeOp *last;
} RDIB_EvalBytecode;

typedef struct RDIB_Location
{
  RDI_LocationKind kind;
  Rng1U64List      ranges;
  union {
    struct {
      RDI_RegCode reg_code;
      U64         offset;
    };
    RDIB_EvalBytecode bytecode;
  };

  // used by RDI builder
  U64 data_offset;
} RDIB_Location;

typedef struct RDIB_LocationNode
{
  struct RDIB_LocationNode *next;
  RDIB_Location             v;
} RDIB_LocationNode;

typedef struct RDIB_LocationList
{
  U64                count;
  RDIB_LocationNode *first;
  RDIB_LocationNode *last;
} RDIB_LocationList;


typedef struct RDIB_Variable
{
  RDI_LinkFlags              link_flags;
  String8                    name;
  String8                    link_name;
  RDI_LocalKind              kind;
  struct RDIB_Type          *type;
  struct RDIB_Type          *container_type;
  struct RDIB_Procedure     *container_proc;
  RDIB_LocationList          locations;
  struct RDIB_VariableChunk *chunk;
  struct RDIB_Variable      *next;
} RDIB_Variable;

typedef struct RDIB_VariableNode
{
  struct RDIB_VariableNode  *next;
  RDIB_Variable              v;
} RDIB_VariableNode;

typedef struct RDIB_VariableList
{
  U64                count;
  RDIB_VariableNode *first;
  RDIB_VariableNode *last;
} RDIB_VariableList;

////////////////////////////////

typedef struct
{
  RDI_Arch arch;
  U64      exe_hash;
  U64      voff_max;
  String8  exe_name;
  String8  producer_string;
} RDIB_TopLevelInfo;

typedef struct
{
  String8                name;
  RDI_BinarySectionFlags flags;
  U64                    voff_first;
  U64                    voff_opl;
  U64                    foff_first;
  U64                    foff_opl;
} RDIB_BinarySection;

typedef struct RDIB_LineTableFragment
{
  struct RDIB_SourceFile        *src_file;
  PairU32                        ln_voff;
  U64                           *voffs;
  U32                           *line_nums;
  U16                           *col_nums;
  U64                            line_count;
  U64                            col_count;
  struct RDIB_LineTableFragment *next_src_file;
  struct RDIB_LineTableFragment *next_line_table;
  struct RDIB_LineTableFragmentChunk *chunk;
} RDIB_LineTableFragment;

typedef struct RDIB_LineTableFragmentChunk
{
  struct RDIB_LineTableFragmentChunk *next;
  U64 base;
  U64 count;
  U64 cap;
  RDIB_LineTableFragment *v;
} RDIB_LineTableFragmentChunk;

typedef struct RDIB_LineTableFragmentChunkList
{
  U64 count;
  RDIB_LineTableFragmentChunk *first;
  RDIB_LineTableFragmentChunk *last;
} RDIB_LineTableFragmentChunkList;

typedef struct RDIB_LineTable
{
  struct RDIB_LineTableChunk *chunk;
  U64                         count;
  RDIB_LineTableFragment     *first;
  RDIB_LineTableFragment     *last;
  U32                         output_array_idx;
} RDIB_LineTable;

typedef struct RDIB_LineTableChunk
{
  struct RDIB_LineTableChunk *next;
  U64                         base;
  U64                         count;
  U64                         cap;
  RDIB_LineTable             *v;
} RDIB_LineTableChunk;

typedef struct RDIB_LineTableChunkList
{
  U64                  count;
  RDIB_LineTableChunk *first;
  RDIB_LineTableChunk *last;
} RDIB_LineTableChunkList;

typedef struct RDIB_SourceFile
{
  String8                      file_path;
  String8                      normal_full_path;
  RDI_ChecksumKind             checksum_kind;
  String8                      checksum;
  RDIB_LineTableFragment      *line_table_frags;

  U64 src_line_map_idx;
  U64 line_table_idx;

  struct RDIB_SourceFileChunk *chunk;
} RDIB_SourceFile;

typedef struct RDIB_SourceFileChunk
{
  struct RDIB_SourceFileChunk *next;
  U64                          base;
  U64                          count;
  U64                          cap;
  RDIB_SourceFile             *v;
} RDIB_SourceFileChunk;

typedef struct RDIB_SourceFileChunkList
{
  U64                   count;
  RDIB_SourceFileChunk *first;
  RDIB_SourceFileChunk *last;
} RDIB_SourceFileChunkList;

typedef struct RDIB_Procedure
{
  RDI_LinkFlags          link_flags;
  String8                name;
  String8                link_name;
  struct RDIB_Type      *type;
  struct RDIB_Type      *container_type;
  struct RDIB_Procedure *container_proc;

  struct RDIB_Scope     *scope;
  struct RDIB_ScopeNode *scope_first;
  struct RDIB_ScopeNode *scope_last;
  
  U64                         scope_count;
  struct RDIB_ProcedureChunk *chunk;
} RDIB_Procedure;

typedef struct RDIB_Scope
{
  struct RDIB_Scope      *parent;
  struct RDIB_Scope      *first_child;
  struct RDIB_Scope      *last_child;
  struct RDIB_Scope      *next_sibling;
  struct RDIB_Procedure  *container_proc;
  Rng1U64List             ranges;
  RDIB_Variable          *local_first;
  RDIB_Variable          *local_last;
  U64                     local_count;
  struct RDIB_InlineSite *inline_site;

  // used by RDI builder
  U64 local_first_idx;

  struct RDIB_ScopeChunk *chunk;
} RDIB_Scope;

typedef struct RDIB_InlineSite
{
  String8                      name;
  struct RDIB_Type            *type;
  struct RDIB_Type            *owner;
  struct RDIB_InlineSiteChunk *chunk;
  union {
    struct RDIB_LineTable *line_table;
    struct {
      void *ud0;
      U64   ud1;
      U64   ud2;
    } convert_ref;
  };
} RDIB_InlineSite;

typedef RDI_MemberKind RDI_MemberKindExt;
enum
{
  RDI_MemberKind_COUNT = RDI_MemberKind_NestedType,
  RDI_MemberKindExt_MemberListPointer // NOTE: must always be last in the list!
};

typedef struct RDIB_UDTMember
{
  RDI_MemberKindExt kind;
  union {
    struct {
      String8      name;
      U64          offset;
      RDIB_TypeRef type_ref;
    } data_field;
    struct {
      String8      name;
      RDIB_TypeRef type_ref;
    } static_data;
    RDIB_TypeRef member_list_pointer;
    struct {
      RDI_MemberKind kind;
      String8         name;
      RDIB_TypeRef    type_ref;
      U64             vftable_offset;
    } method;
    struct {
      String8      name;
      RDIB_TypeRef type_ref;
    } nested_type;
    struct {
      RDIB_TypeRef type_ref;
      U64          offset;
    } base_class;
    struct {
      RDIB_TypeRef type_ref;
      U64          vbptr_off;
      U64          vtable_off;
    } virtual_base_class;
    struct {
      String8 name;
      U64     value;
    } enumerate;
  };
  struct RDIB_UDTMember      *next;
  struct RDIB_UDTMemberChunk *chunk;
} RDIB_UDTMember;

typedef struct RDIB_UDTMemberList
{
  U64             count;
  RDIB_UDTMember *first;
  RDIB_UDTMember *last;
} RDIB_UDTMemberList;

typedef struct RDIB_UDT
{
  struct RDIB_Type *self_type;
  struct RDIB_Type *members;
  RDIB_SourceFile  *decl_src_file;
  U32               decl_line_num;
  U32               decl_col_num;
} RDIB_UDT;

enum
{
  RDI_TypeKindExt_Lo = RDI_TypeKind_Count,
  RDI_TypeKindExt_VirtualTable,
  RDI_TypeKindExt_StaticMethod,
  RDI_TypeKindExt_Members,
  RDI_TypeKindExt_Params,
  RDI_TypeKindExt_Count,
};
typedef RDI_TypeKind RDI_TypeKindExt;

typedef struct RDIB_Type
{
  RDI_TypeKindExt kind;
  U64 final_idx;
  U64 itype;
  union {
    struct {
      String8 name;
      U64     size;
    } builtin;
    struct {
      RDI_TypeModifierFlags flags;
      RDIB_TypeRef          type_ref;
    } modifier;
    struct {
      RDIB_TypeRef type_ref;
      U64          size;
    } ptr;
    struct {
      RDIB_TypeRef return_type;
      RDIB_TypeRef params_type;

      U64 param_idx_run_bucket_idx;
    } func;
    struct {
      RDIB_TypeRef class_type;
      RDIB_TypeRef this_type;
      RDIB_TypeRef return_type;
      RDIB_TypeRef params_type;

      U64 param_idx_run_bucket_idx;
    } method;
    struct {
      RDIB_TypeRef class_type;
      RDIB_TypeRef return_type;
      RDIB_TypeRef params_type;

      U64 param_idx_run_bucket_idx;
    } static_method;
    struct {
      U64          off;
      U64          count;
      RDIB_TypeRef value_type;
    } bitfield;
    struct {
      RDIB_TypeRef entry_type;
      U64          size;
    } array;
    struct {
      String8      name;
      String8      link_name;
      RDIB_TypeRef members;
      // assigned in UDT build step
      U64 udt_idx;
      union {
        struct {
          U64          size;
          RDIB_TypeRef derived;
          RDIB_TypeRef vtshape;
        } struct_type;
        struct {
          U64          size;
        } union_type;
        struct {
          String8      name;
          RDIB_TypeRef base_type;
        } enum_type;
      };
    } udt;
    struct {
      U64           count;
      RDIB_TypeRef *types;
    } params;
    struct {
      RDIB_UDTMemberList list;

      // assigned in member build step
      B32 is_head;
      U64 first_member_idx;
    } members, enum_members;
  };
  struct RDIB_TypeChunk *chunk;
} RDIB_Type;

typedef struct RDIB_Unit
{
  RDI_Arch        arch;
  String8         unit_name;
  String8         compiler_name;
  String8         source_file;
  String8         object_file;
  String8         archive_file;
  String8         build_path;
  RDI_Language    language;
  RDIB_LineTable *line_table;
  U64             virt_range_count;
  Rng1U64        *virt_ranges;

  struct RDIB_UnitChunk *chunk;
} RDIB_Unit;

typedef struct RDIB_DataSection
{
  RDI_SectionKind tag;
  String8List     data;
} RDIB_DataSection;

typedef struct RDIB_DataSectionNode
{
  struct RDIB_DataSectionNode *next;
  RDIB_DataSection             v;
} RDIB_DataSectionNode;

typedef struct RDIB_DataSectionList
{
  U64                   count;
  RDIB_DataSectionNode *first;
  RDIB_DataSectionNode *last;
} RDIB_DataSectionList;


typedef struct RDIB_UnitChunk
{
  U64                    base;
  U64                    count;
  U64                    cap;
  RDIB_Unit             *v;
  struct RDIB_UnitChunk *next;
} RDIB_UnitChunk;

typedef struct RDIB_UnitChunkList
{
  U64             count;
  RDIB_UnitChunk *first;
  RDIB_UnitChunk *last;
} RDIB_UnitChunkList;

typedef struct RDIB_VariableChunk
{
  struct RDIB_VariableChunk *next;
  U64                        base;
  U64                        count;
  U64                        cap;
  RDIB_Variable             *v;
} RDIB_VariableChunk;

typedef struct RDIB_VariableChunkList
{
  U64                 count;
  RDIB_VariableChunk *first;
  RDIB_VariableChunk *last;
} RDIB_VariableChunkList;

typedef struct RDIB_ProcedureChunk
{
  struct RDIB_ProcedureChunk *next;
  U64                         base;
  U64                         count;
  U64                         cap;
  RDIB_Procedure             *v;
} RDIB_ProcedureChunk;

typedef struct RDIB_ProcedureChunkList
{
  U64                  count;
  RDIB_ProcedureChunk *first;
  RDIB_ProcedureChunk *last;
} RDIB_ProcedureChunkList;

typedef struct RDIB_ScopeChunk
{
  struct RDIB_ScopeChunk *next;
  U64                     base;
  U64                     count;
  U64                     cap;
  RDIB_Scope             *v;
} RDIB_ScopeChunk;

typedef struct RDIB_ScopeChunkList
{
  U64              count;
  RDIB_ScopeChunk *first;
  RDIB_ScopeChunk *last;
} RDIB_ScopeChunkList;

typedef struct RDIB_ScopeNode
{
  struct RDIB_ScopeNode *next;
  RDIB_Scope            *v;
} RDIB_ScopeNode;

typedef struct RDIB_ScopeList
{
  U64         count;
  RDIB_Scope *first;
  RDIB_Scope *last;
} RDIB_ScopeList;

typedef struct RDIB_InlineSiteChunk
{
  struct RDIB_InlineSiteChunk *next;
  U64                          base;
  U64                          count;
  U64                          cap;
  RDIB_InlineSite             *v;
} RDIB_InlineSiteChunk;

typedef struct RDIB_InlineSiteChunkList
{
  U64                   count;
  RDIB_InlineSiteChunk *first;
  RDIB_InlineSiteChunk *last;
} RDIB_InlineSiteChunkList;

typedef struct RDIB_TypeChunk
{
  struct RDIB_TypeChunk *next;
  U64                    base;
  U64                    count;
  U64                    cap;
  RDIB_Type             *v;
} RDIB_TypeChunk;

typedef struct
{
  U64             count;
  RDIB_TypeChunk *first;
  RDIB_TypeChunk *last;
} RDIB_TypeChunkList;

typedef struct RDIB_UDTMemberChunk
{
  struct RDIB_UDTMemberChunk *next;
  U64                         base;
  U64                         count;
  U64                         cap;
  RDIB_UDTMember             *v;
} RDIB_UDTMemberChunk;

typedef struct RDIB_UDTMemberChunkList
{
  U64                  count;
  RDIB_UDTMemberChunk *first;
  RDIB_UDTMemberChunk *last;
} RDIB_UDTMemberChunkList;

////////////////////////////////
// UDT Forward Ref Map

typedef struct
{
  struct RDIB_Type *type;
  U64 idx;
} RDIB_UDTFwdrefBucket;

////////////////////////////////
// String Map

typedef struct RDIB_StringMapBucket
{
  String8 string;

  union {
    // to get deterministic output we assign each bucket a unique index
    union {
      struct {
        U32 lo;
        U32 hi;
      };
      U64 v;
    } sorter;

    // after buckets are sorted we replace 'sorter' with indices into output array
    U64 idx;
  };

  union {
    // depending on the usage context sotres: pointers to variables, procedures, and etc.
    VoidNode *raw_values;

    // during index-run-map build step 'raw_values' are replaced with index-run bucket index
    struct {
      U32 count;
      // if we have single index - store it in the bucket
      union {
        U64 idx_run_bucket_idx;
        U32 match_idx;
      };
    };
  };
} RDIB_StringMapBucket;

typedef struct RDIB_StringMap
{
  U64                    cap;
  RDIB_StringMapBucket **buckets;
} RDIB_StringMap;

#define RDIB_STRING_MAP_UPDATE_FUNC(name) void name(VoidNode **head, VoidNode *node)
typedef RDIB_STRING_MAP_UPDATE_FUNC(RDIB_StringMapUpdateFunc);

typedef struct
{
  RDIB_StringMap        *string_map;
  Rng1U64               *ranges;
  U64                   *counts;
  U64                   *offsets;
  RDIB_StringMapBucket **result;
} RDIB_GetExtantBucketsStringMapTask;

typedef struct
{
  U32                   *string_table;
  U64                    string_data_size;
  U8                    *string_data;
  RDIB_StringMapBucket **buckets;
  Rng1U64               *ranges;
} RDIB_CopyStringDataTask;

typedef struct
{
  U64                    chunk_idx_opl;
  Rng1U64               *ranges;
  RDIB_StringMapBucket **src;
  RDIB_StringMapBucket **dst;
  U32                   *chunk_histo;
  U32                   *chunk_offsets;
} RDIB_StringMapRadixSort;

////////////////////////////////
// Index Run Map

typedef struct RDIB_IndexRunBucket
{
  union {
    struct {
      U32 lo;
      U32 hi;
    };
    U64 v;
  } sorter;
  U32Array indices;
  U64      index_in_output_array;
} RDIB_IndexRunBucket;

typedef struct RDIB_IndexRunMap
{
  U64                   cap;
  RDIB_IndexRunBucket **buckets;
} RDIB_IndexRunMap;

////////////////////////////////

typedef struct
{
  U64 voff;
  U32 size;
  U32 idx;
} RDIB_VMapRange;

////////////////////////////////

#define RDIB_PATH_TREE_NIL_STRING str8_lit("<nil>")

typedef struct RDIB_PathTreeNode
{
  struct RDIB_PathTreeNode *parent;
  struct RDIB_PathTreeNode *next_order;
  struct RDIB_PathTreeNode *next_sibling;
  struct RDIB_PathTreeNode *first_child;
  struct RDIB_PathTreeNode *last_child;
  U64                       node_idx;
  String8                   sub_path;
  RDIB_SourceFile          *src_file;
} RDIB_PathTreeNode;

typedef struct RDIB_PathTreeNodeList
{
  U64                count;
  RDIB_PathTreeNode *first;
  RDIB_PathTreeNode *last;
} RDIB_PathTreeNodeList;

typedef struct RDIB_PathTree
{
  RDIB_PathTreeNode     *root;
  U64                    node_count;
  U64                    next_list_idx;
  U64                    list_count;
  RDIB_PathTreeNodeList *node_lists;
} RDIB_PathTree;

////////////////////////////////

typedef struct RDIB_Input
{
  U64 unit_chunk_cap;
  U64 src_file_chunk_cap;
  U64 symbol_chunk_cap;
  U64 line_table_cap;
  U64 inline_site_cap;
  U64 type_cap;
  U64 udt_cap;

  RDIB_TopLevelInfo         top_level_info;
  U64                       sect_count;
  RDIB_BinarySection       *sections;
  RDIB_UnitChunkList        units;
  RDIB_SourceFileChunkList  src_files;
  RDIB_LineTableChunkList   line_tables;
  RDIB_ScopeChunkList       scopes;
  RDIB_VariableChunkList    locals;
  RDIB_VariableChunkList    gvars;
  RDIB_VariableChunkList    extern_gvars;
  RDIB_VariableChunkList    static_gvars;
  RDIB_VariableChunkList    tvars;
  RDIB_VariableChunkList    extern_tvars;
  RDIB_VariableChunkList    static_tvars;
  RDIB_ProcedureChunkList   procs;
  RDIB_ProcedureChunkList   extern_procs;
  RDIB_ProcedureChunkList   static_procs;
  RDIB_InlineSiteChunkList  inline_sites;
  RDIB_TypeChunkList        types;
  RDIB_TypeChunkList        struct_list;
  RDIB_TypeChunkList        union_list;
  RDIB_TypeChunkList        enum_list;
  RDIB_TypeChunkList        param_types;
  RDIB_TypeChunkList        member_types;
  RDIB_TypeChunkList        enum_types;
  RDIB_UDTMemberChunkList   udt_members;
  RDIB_UDTMemberChunkList   enum_members;

  RDIB_SourceFile        *null_src_file;
  RDIB_LineTable         *null_line_table;
  RDIB_LineTableFragment *null_frag;
  RDIB_Type              *null_type;
  RDIB_Scope             *null_scope;
  RDIB_Unit              *null_unit;
  RDIB_Procedure         *null_proc;
  RDIB_Variable          *null_local;
  RDIB_Variable          *null_gvar;
  RDIB_Variable          *null_tvar;
  RDIB_UDTMember         *null_udt_member;
  RDIB_UDTMember         *null_enum_member;
  RDIB_InlineSite        *null_inline_site;

  RDIB_Type *variadic_type;

  //RDIB_TypeChunkList       struct_list;
  //RDIB_TypeChunkList       union_list;
  //RDIB_TypeChunkList       enum_list;
  //RDIB_TypeChunkList       param_types;
} RDIB_Input;

////////////////////////////////
// Parallel For Tasks

typedef struct
{
  U64 *udt_counts;
} RDIB_TypeStats;

typedef struct
{
  RDIB_TypeChunk **chunks;
  RDIB_TypeStats  *type_stats;
} RDIB_TypeStatsTask;

typedef struct
{
  Rng1U64         *ranges;
  U64             *counts;
  U64             *offsets;
  RDIB_TypeChunk **type_chunks;
  RDIB_StringMap  *string_map;
  union {
    RDI_Member     *udt_members_rdi;
    RDI_EnumMember *enum_members_rdi;
  };
} RDIB_MembersTask;

typedef struct
{
  RDIB_TypeChunk    **type_chunks;
  RDIB_TypeStats      type_stats;
  U64                *udt_base_idx;
  RDI_UDT            *udts;
} RDIB_UserDefinesTask;

typedef struct
{
  U64                 addr_size;
  RDIB_StringMap     *string_map;
  RDIB_IndexRunMap   *idx_run_map;
  RDIB_TypeChunk    **type_chunks;
  RDIB_TypeStats      type_stats;
  RDI_TypeNode       *type_nodes;
} RDIB_TypeNodesTask;

typedef struct
{
  RDIB_StringMap            *string_map;
  Rng1U64                   *ranges;
  RDIB_StringMapUpdateFunc  *string_map_update_func;
  RDIB_StringMapBucket     **free_buckets;
  U64                       *insert_counts;
  U64                       *element_indices;
  union
  {
    RDIB_UnitChunk        **units;
    RDIB_BinarySection     *sects;
    RDIB_SourceFileChunk  **src_file_chunks;
    RDIB_VariableChunk    **vars;
    RDIB_ProcedureChunk   **procs;
    RDIB_InlineSiteChunk  **inline_sites;
    RDIB_UDTMemberChunk   **udt_members;
    RDIB_UDTMemberChunk   **enum_members;
    RDIB_TypeChunk        **types;
    RDIB_PathTreeNodeList  *path_node_lists;
  };
} RDIB_CollectStringsTask;

typedef struct
{
  RDIB_StringMap  *string_map;
  Rng1U64         *ranges;
  RDIB_TypeChunk **chunks;
  String8List     *data_lists;
} RDIB_BuildTypeDataTask;

typedef struct
{
  RDIB_StringMap *string_map;
  Rng1U64        *ranges;
  union {
    struct {
      RDIB_VariableChunk **gvars_rdib;
      String8List         *gvars_out;
    };
    struct {
      RDIB_VariableChunk **tvars_rdib;
      String8List         *tvars_out;
    };
    struct {
      RDIB_ProcedureChunk **procs_rdib;
      String8List          *procs_out;
    };
    struct {
      RDIB_ScopeChunk  **scopes_rdib;
      U64               *scope_voff_counts;
      U64               *loc_data_sizes;
      U64               *local_counts;
      U64               *loc_block_counts;
      U64               *scope_voff_offsets;
      U64               *local_offsets;
      U64               *loc_block_offsets;
      U64               *loc_data_offsets;
      U64               *scope_voffs_rdi;
      RDI_Scope         *scopes_rdi;
      RDI_Local         *locals_rdi;
      RDI_LocationBlock *loc_blocks_rdi;
      U8                *loc_data_rdi;
    };
  };
} RDIB_BuildSymbolSectionTask;

typedef union
{
  struct {
    U64            *counts;
    U64            *offsets;
    Rng1U64        *ranges;
    RDIB_VMapRange *vmap;
    union {
      RDIB_UnitChunk     **unit_chunks;
      RDIB_VariableChunk **gvar_chunks;
      RDIB_ScopeChunk    **scope_chunks;
    };
  };

  struct {
    U64             vmap_counts[3];
    RDIB_VMapRange *vmaps[3];
    String8List     raw_vmaps[3];
  };
} RDIB_VMapBuilderTask;

typedef struct
{
  U64                   sorter_idx;
  RDI_NameMapKind       name_map_kind;
  RDIB_IndexRunMap     *idx_run_map;
  RDIB_IndexRunBucket **free_buckets;
  Rng1U64              *ranges;
  union {
    RDIB_TypeChunk       **type_chunks;
    RDIB_StringMapBucket **name_map_buckets;
  };
} RDIB_BuildIndexRunsTask;

typedef struct
{
  RDIB_IndexRunBucket **buckets;
  Rng1U64             *ranges;
  U32                 *output_array;
} RDIB_IdxRunCopyTask;

typedef struct
{
  RDIB_IndexRunMap     *idx_run_map;
  Rng1U64              *ranges;
  U64                  *counts;
  U64                  *offsets;
  RDIB_IndexRunBucket **result;
} RDIB_GetExtantBucketsIndexRunMapTask;

typedef struct
{
  U64                   chunk_idx_opl;
  Rng1U64              *ranges;
  RDIB_IndexRunBucket **src;
  RDIB_IndexRunBucket **dst;
  U32                  *chunk_histo;
  U32                  *chunk_offsets;
} RDIB_IndexRunMapRadixSort;

typedef struct
{
  RDIB_StringMap         *string_map;
  RDIB_IndexRunMap       *idx_run_map;
  U64                    *in_bucket_counts;
  RDIB_StringMapBucket ***in_buckets;

  RDI_NameMapBucket **out_buckets;
  RDI_NameMapNode   **out_nodes;
  U64                *out_bucket_counts;
  U64                *out_node_counts;
} RDIB_NameMapBuilderTask;

typedef struct
{
  RDIB_PathTree    *path_tree;
  RDIB_StringMap   *string_map;
  RDI_FilePathNode *nodes_dst;
} RDIB_BuildFilePathNodesTask;

typedef struct
{
  RDIB_SourceFile **src_file_arr;
  U32             *out_line_counts;
  U32             *out_voff_counts;
  U32             **out_line_nums;
  U32             **out_line_ranges;
  U64             **out_voffs;
} RDIB_SrcLineMapsTask;

typedef struct
{
  RDIB_LineTableChunk **chunks;
  Rng1U64              *ranges;

  U64       *out_line_table_counts;
  U64      **out_line_table_voffs;
  RDI_Line **out_line_table_lines;
} RDIB_BuildLineTablesTask;

typedef struct
{
  Rng1U64               *ranges;
  RDIB_StringMap        *string_map;
  RDIB_PathTree         *path_tree;
  RDIB_SourceFileChunk **src_file_chunks;
  RDI_SourceFile        *src_files_dst;
} RDIB_FillSourceFilesTask;

////////////////////////////////
// Data Model Helpers

internal RDIB_DataModel rdib_infer_data_model(OperatingSystem os, RDI_Arch arch);

internal RDI_TypeKind rdib_short_type_from_data_model             (RDIB_DataModel data_model);
internal RDI_TypeKind rdib_unsigned_short_type_from_data_model    (RDIB_DataModel data_model);
internal RDI_TypeKind rdib_int_type_from_data_model               (RDIB_DataModel data_model);
internal RDI_TypeKind rdib_unsigned_int_type_from_data_model      (RDIB_DataModel data_model);
internal RDI_TypeKind rdib_long_type_from_data_model              (RDIB_DataModel data_model);
internal RDI_TypeKind rdib_unsigned_long_type_from_data_model     (RDIB_DataModel data_model);
internal RDI_TypeKind rdib_long_long_type_from_data_model         (RDIB_DataModel data_model);
internal RDI_TypeKind rdib_unsigned_long_long_type_from_data_model(RDIB_DataModel data_model);
internal RDI_TypeKind rdib_pointer_size_t_type_from_data_model    (RDIB_DataModel data_model);

////////////////////////////////

internal void rdib_udt_member_list_push_node      (RDIB_UDTMemberList *list, RDIB_UDTMember *node);
internal void rdib_udt_member_list_concat_in_place(RDIB_UDTMemberList *list, RDIB_UDTMemberList *to_concat);

internal RDIB_LineTableFragment * rdib_line_table_push(Arena *arena, RDIB_LineTable *list);

////////////////////////////////
// Chunk Lists

// push
internal RDIB_Unit          * rdib_unit_chunk_list_push       (Arena *arena, RDIB_UnitChunkList       *list, U64 cap);
internal RDIB_Scope         * rdib_scope_chunk_list_push      (Arena *arena, RDIB_ScopeChunkList      *list, U64 cap);
internal RDIB_Procedure     * rdib_procedure_chunk_list_push  (Arena *arena, RDIB_ProcedureChunkList  *list, U64 cap);
internal RDIB_Variable      * rdib_variable_chunk_list_push   (Arena *arena, RDIB_VariableChunkList   *list, U64 cap);
internal RDIB_LineTable     * rdib_line_table_chunk_list_push (Arena *arena, RDIB_LineTableChunkList  *list, U64 cap);
internal RDIB_Type          * rdib_type_chunk_list_push       (Arena *arena, RDIB_TypeChunkList       *list, U64 cap);
internal RDIB_UDTMember     * rdib_udt_member_chunk_list_push (Arena *arena, RDIB_UDTMemberChunkList  *list, U64 cap);
internal RDIB_SourceFile    * rdib_source_file_chunk_list_push(Arena *arena, RDIB_SourceFileChunkList *list, U64 cap);
internal RDIB_InlineSite    * rdib_inline_site_chunk_list_push(Arena *arena, RDIB_InlineSiteChunkList *list, U64 cap);

internal RDIB_Scope      * rdib_scope_chunk_list_push_zero      (Arena *arena, RDIB_ScopeChunkList *list,      U64 cap);
internal RDIB_Procedure  * rdib_procedure_chunk_list_push_zero  (Arena *arena, RDIB_ProcedureChunkList *list,  U64 cap);
internal RDIB_Variable   * rdib_variable_chunk_list_push_zero   (Arena *arena, RDIB_VariableChunkList *list,   U64 cap);
internal RDIB_LineTable  * rdib_line_table_chunk_list_push_zero (Arena *arena, RDIB_LineTableChunkList *list,  U64 cap);
internal RDIB_Type       * rdib_type_chunk_list_push_zero       (Arena *arena, RDIB_TypeChunkList *list,       U64 cap);
internal RDIB_UDTMember  * rdib_udt_member_chunk_list_push_zero (Arena *arena, RDIB_UDTMemberChunkList *list,  U64 cap);
internal RDIB_SourceFile * rdib_source_file_chunk_list_push_zero(Arena *arena, RDIB_SourceFileChunkList *list, U64 cap);
internal RDIB_InlineSite * rdib_inline_site_chunk_list_push_zero(Arena *arena, RDIB_InlineSiteChunkList *list, U64 cap);

// push many
internal RDIB_UnitChunk * rdib_unit_chunk_list_reserve_ex(Arena *arena, RDIB_UnitChunkList *list, U64 chunk_count, U64 item_count);

internal void rdib_unit_chunk_list_reserve       (Arena *arena, RDIB_UnitChunkList       *list, U64 cap);
internal void rdib_type_chunk_list_reserve       (Arena *arena, RDIB_TypeChunkList       *list, U64 cap);
internal void rdib_source_file_chunk_list_reserve(Arena *arena, RDIB_SourceFileChunkList *list, U64 cap);

// concat in place
internal void rdib_scope_chunk_list_concat_in_place      (RDIB_ScopeChunkList      *list, RDIB_ScopeChunkList      *to_concat);
internal void rdib_udt_member_chunk_list_concat_in_place (RDIB_UDTMemberChunkList  *list, RDIB_UDTMemberChunkList  *to_concat);
internal void rdib_procedure_chunk_list_concat_in_place  (RDIB_ProcedureChunkList  *list, RDIB_ProcedureChunkList  *to_concat);
internal void rdib_variable_chunk_list_concat_in_place   (RDIB_VariableChunkList   *list, RDIB_VariableChunkList   *to_concat);
internal void rdib_inline_site_chunk_list_concat_in_place(RDIB_InlineSiteChunkList *list, RDIB_InlineSiteChunkList *to_concat);
internal void rdib_inline_site_chunk_list_concat_in_place(RDIB_InlineSiteChunkList *list, RDIB_InlineSiteChunkList *to_concat);
internal void rdib_type_chunk_list_concat_in_place       (RDIB_TypeChunkList       *list, RDIB_TypeChunkList       *to_concat);
internal void rdib_source_file_chunk_list_concat_in_place(RDIB_SourceFileChunkList *list, RDIB_SourceFileChunkList *to_concat);

// concat in place many
internal void rdib_type_chunk_list_concat_in_place_many      (RDIB_TypeChunkList      *list, RDIB_TypeChunkList      *to_concat, U64 count);
internal void rdib_udt_member_chunk_list_concat_in_place_many(RDIB_UDTMemberChunkList *list, RDIB_UDTMemberChunkList *to_concat, U64 count);

// array from chunk list
internal RDIB_UnitChunk       ** rdib_array_from_unit_chunk_list       (Arena *arena, RDIB_UnitChunkList list);
internal RDIB_ScopeChunk      ** rdib_array_from_scope_chunk_list      (Arena *arena, RDIB_ScopeChunkList      list);
internal RDIB_ProcedureChunk  ** rdib_array_from_procedure_chunk_list  (Arena *arena, RDIB_ProcedureChunkList  list);
internal RDIB_VariableChunk   ** rdib_array_from_variable_chunk_list   (Arena *arena, RDIB_VariableChunkList   list);
internal RDIB_LineTableChunk  ** rdib_array_from_line_table_chunk_list (Arena *arena, RDIB_LineTableChunkList  list);
internal RDIB_InlineSiteChunk ** rdib_array_from_inline_site_chunk_list(Arena *arena, RDIB_InlineSiteChunkList list);
internal RDIB_UDTMemberChunk  ** rdib_array_from_udt_member_chunk_list (Arena *arena, RDIB_UDTMemberChunkList  list);
internal RDIB_TypeChunk       ** rdib_array_from_type_chunk_list       (Arena *arena, RDIB_TypeChunkList       list);
internal RDIB_SourceFileChunk ** rdib_array_from_source_file_chunk_list(Arena *arena, RDIB_SourceFileChunkList list);

// total count from chunk list
internal U64 rdib_unit_chunk_list_total_count       (RDIB_UnitChunkList list);
internal U64 rdib_scope_chunk_list_total_count      (RDIB_ScopeChunkList      list);
internal U64 rdib_variable_chunk_list_total_count   (RDIB_VariableChunkList   list);
internal U64 rdib_line_table_chunk_list_total_count (RDIB_LineTableChunkList  list);
internal U64 rdib_procedure_chunk_list_total_count  (RDIB_ProcedureChunkList  list);
internal U64 rdib_inline_site_chunk_list_total_count(RDIB_InlineSiteChunkList list);
internal U64 rdib_udt_member_chunk_list_total_count (RDIB_UDTMemberChunkList  list);
internal U64 rdib_type_chunk_list_total_count       (RDIB_TypeChunkList       list);
internal U64 rdib_source_file_chunk_list_total_count(RDIB_SourceFileChunkList list);

// pointer -> array index
internal U32 rdib_idx_from_unit       (RDIB_Unit *n);
internal U32 rdib_idx_from_scope      (RDIB_Scope      *n);
internal U32 rdib_idx_from_variable   (RDIB_Variable   *n);
internal U32 rdib_idx_from_procedure  (RDIB_Procedure  *n);
internal U32 rdib_idx_from_source_file(RDIB_SourceFile *n);
internal U32 rdib_idx_from_line_table (RDIB_LineTable *n);
internal U32 rdib_idx_from_type       (RDIB_Type       *n);
internal U32 rdib_idx_from_udt_type   (RDIB_Type       *n);
internal U32 rdib_idx_from_inline_site(RDIB_InlineSite *n);

////////////////////////////////

//- Source File

internal B32 rdib_source_file_match(RDIB_SourceFile *a, RDIB_SourceFile *b, OperatingSystem os);

//- Eval Ops

internal RDIB_EvalBytecodeOp * rdib_bytecode_push_op    (Arena *arena, RDIB_EvalBytecode *bytecode, RDI_EvalOp op, RDI_U64 p);
internal void                  rdib_bytecode_push_ucsont(Arena *arena, RDIB_EvalBytecode *bytecode, RDI_U64 uconst);
internal void                  rdib_bytecode_push_sconst(Arena *arena, RDIB_EvalBytecode *bytecode, RDI_S64 sconst);

//- Location

internal RDIB_Location rdib_make_location_addr_byte_stream      (Rng1U64List ranges, RDIB_EvalBytecode bytecode);
internal RDIB_Location rdib_make_location_addr_bytecode_stream  (Rng1U64List ranges, RDIB_EvalBytecode bytecode);
internal RDIB_Location rdib_make_location_val_bytecode_stream   (Rng1U64List ranges, RDIB_EvalBytecode bytecode);
internal RDIB_Location rdib_make_location_addr_reg_plus_u16     (Rng1U64List ranges, RDI_RegCode reg_code, RDI_U16 offset);

internal RDIB_Location rdib_make_location_addr_addr_reg_plus_u16(Rng1U64List ranges, RDI_RegCode reg_code, RDI_U16 offset);
internal RDIB_Location rdib_make_location_addr_reg_plus_u16     (Rng1U64List ranges, RDI_RegCode reg_code, RDI_U16 offset);
internal RDIB_Location rdib_make_location_val_reg               (Rng1U64List ranges, RDI_RegCode reg_code);

internal RDIB_LocationNode * rdib_push_location_addr_reg_off(Arena *arena, RDIB_LocationList *list, RDI_Arch arch, RDI_RegCode reg_code, U32 reg_byte_size, U32 reg_byte_pos, S64 offset, B32 is_reference, Rng1U64List ranges);

//- UDT Fwdrefs

internal U64                    rdib_udt_fwdref_map_hash(String8 string);
internal RDIB_UDTFwdrefBucket * rdib_udt_fwdref_map_insert_or_update(RDIB_UDTFwdrefBucket **buckets, U64 cap, U64 hash, RDIB_UDTFwdrefBucket *new_bucket);
internal RDIB_UDTFwdrefBucket * rdib_udt_fwdrefmap_map_lookup(RDIB_UDTFwdrefBucket **buckets, U64 cap, U64 hash, String8 name);

//- Types

internal RDIB_TypeRef rdib_make_type_ref(Arena *arena, RDIB_Type *type);
internal void         rdib_deref_type_refs(TP_Context *tp, RDIB_TypeChunkList *list);

internal RDIB_TypeStats rdib_sum_type_stats                 (RDIB_TypeStats *stats, U64 count);
internal U64            rdib_udt_count_from_type_stats      (RDIB_TypeStats *stats);
internal U64            rdib_type_node_count_from_type_stats(RDIB_TypeStats *stats);

internal U64 rdib_size_from_type    (RDIB_Type *type);
internal U64 rdib_count_members_deep(RDIB_Type *type);

//- Path Tree

internal RDIB_PathTree * rdib_path_tree_init    (Arena *arena, U64 list_count);
internal void            rdib_path_tree_insert  (Arena *arena, RDIB_PathTree *tree, String8 path, RDIB_SourceFile *src_file);
internal U32             rdib_idx_from_path_tree(RDIB_PathTree *tree, String8 path);


//- String Map

internal U64                     rdib_string_map_hash               (String8 string);
internal RDIB_StringMap *        rdib_init_string_map               (Arena *arena, U64 cap);
internal U32                     rdib_idx_from_string_map           (RDIB_StringMap *string_map, String8 string);
internal RDIB_StringMapBucket *  rdib_string_map_insert_or_update   (RDIB_StringMapBucket **buckets, U64 cap, U64 hash, RDIB_StringMapBucket *new_bucket, RDIB_StringMapUpdateFunc *update_func);
internal void                    rdib_string_map_assign_indices     (RDIB_StringMapBucket **buckets, U64 bucket_count);
internal RDIB_StringMapBucket ** rdib_extant_buckets_from_string_map(TP_Context *tp, Arena *arena, RDIB_StringMap *string_map, U64 *bucket_count_out);
internal void                    rdib_string_map_sort_buckets       (TP_Context *tp, RDIB_StringMapBucket **buckets, U64 bucket_count, U64 max_sorter);

//- String Map Specialized Inserters

internal void rdib_string_map_insert_item             (Arena *arena, RDIB_CollectStringsTask *task, U64 task_id, String8 string, void *value);
internal void rdib_string_map_insert_string_table_item(Arena *arena, RDIB_CollectStringsTask *task, U64 task_id, String8 string);
internal void rdib_string_map_insert_name_map_item    (Arena *arena, RDIB_CollectStringsTask *task, U64 task_id, String8 string, VoidNode *node);

//- Index Run Map

internal U64                    rdib_index_run_hash                   (U32 count, U32 *idxs);
internal RDIB_IndexRunMap *     rdib_init_index_run_map               (Arena *arena, U64 cap);
internal RDIB_IndexRunBucket *  rdib_index_run_map_insert_or_update   (Arena *arena, RDIB_IndexRunBucket **buckets, U64 cap, U64 hash, RDIB_IndexRunBucket *new_bucket, U64 *bucket_idx_out);
internal U32                    rdib_idx_run_from_bucket_idx          (RDIB_IndexRunMap *map, U64 bucket_idx);
internal void                   rdib_index_run_map_assign_indices     (RDIB_IndexRunBucket **buckets, U64 bucket_count);
internal RDIB_IndexRunBucket ** rdib_extant_buckets_from_index_run_map(TP_Context *tp, Arena *arena, RDIB_IndexRunMap *idx_run_map, U64 *bucket_count_out);
internal void                   rdib_index_run_map_sort_buckets       (TP_Context *tp, RDIB_IndexRunBucket **buckets, U64 bucket_count, U64 chunk_idx_opl);

//- Index Map Spesialized Query

internal U32 rdib_idx_from_params(RDIB_IndexRunMap *map, RDIB_Type *params);

//- Data Sections

internal void                   rdib_data_section_list_push_node      (RDIB_DataSectionList *list, RDIB_DataSectionNode *node);
internal RDIB_DataSectionNode * rdib_data_section_list_push           (Arena *arena, RDIB_DataSectionList *list, RDIB_DataSection v);
internal void                   rdib_data_section_list_concat_in_place(RDIB_DataSectionList *list, RDIB_DataSectionList *to_concat);

internal void rdib_data_sections_from_top_level_info       (Arena *arena, RDIB_DataSectionList *sect_list, RDIB_StringMap *string_map, RDIB_TopLevelInfo *src);
internal void rdib_data_sections_from_binary_sections      (Arena *arena, RDIB_DataSectionList *sect_list, RDIB_StringMap *string_map, RDIB_BinarySection *binary_sects, U64 binary_sects_count);
internal void rdib_data_sections_from_string_map           (TP_Context *tp, Arena *arena, RDIB_DataSectionList *sect_list, RDIB_StringMapBucket **buckets, U64 bucket_count);
internal void rdib_data_sections_from_index_runs           (TP_Context *tp, Arena *arena, RDIB_DataSectionList *sect_list, RDIB_IndexRunBucket **buckets, U64 bucket_count);
internal void rdib_data_sections_from_file_path_nodes      (TP_Context *tp, Arena *arena, RDIB_DataSectionList *sect_list,  RDIB_PathTree *tree);
internal void rdib_data_sections_from_source_files         (TP_Context *tp, TP_Arena *arena, RDIB_DataSectionList *sect_list, RDIB_StringMap *string_map, RDIB_PathTree *path_tree, U64 total_src_file_count, U64 src_file_chunk_count, RDIB_SourceFileChunk **src_file_chunks);
internal void rdib_data_sections_from_units                (Arena *arena, RDIB_DataSectionList *sect_list, RDIB_StringMap *string_map, RDIB_PathTree *path_tree, U64 total_unit_count, U64 unit_chunk_count, RDIB_UnitChunk **unit_chunks);
internal void rdib_data_sections_from_string_map           (TP_Context *tp, Arena *arena, RDIB_DataSectionList *sect_list, RDIB_StringMapBucket **buckets, U64 bucket_count);
internal void rdib_data_sections_from_types(TP_Context            *tp,
                                            Arena                 *arena,
                                            RDIB_DataSectionList  *sect_list,
                                            RDI_Arch               arch,
                                            RDIB_StringMap        *string_map,
                                            RDIB_IndexRunMap      *idx_run_map,
                                            U64                    udt_member_chunk_count,
                                            RDIB_TypeChunk       **udt_member_type_chunks,
                                            U64                    enum_member_chunk_count,
                                            RDIB_TypeChunk       **enum_member_type_chunks,
                                            U64                    total_type_node_count,
                                            U64                    type_chunk_count,
                                            RDIB_TypeChunk       **type_chunks,
                                            RDIB_TypeStats         type_stats);
internal void rdib_data_sections_from_global_variables     (TP_Context *tp, TP_Arena *arena, RDIB_DataSectionList *sect_list, RDIB_StringMap *string_map, U64 total_count, U64 chunk_count, RDIB_VariableChunk **chunks);
internal void rdib_data_sections_from_thread_variables     (TP_Context *tp, TP_Arena *arena, RDIB_DataSectionList *sect_list, RDIB_StringMap *string_map, U64 total_count, U64 chunk_count, RDIB_VariableChunk **chunks);
internal void rdib_data_sections_from_procedures           (TP_Context *tp, TP_Arena *arena, RDIB_DataSectionList *sect_list, RDIB_StringMap *string_map, U64 total_count, U64 chunk_count, RDIB_ProcedureChunk **chunks);
internal void rdib_data_sections_from_scopes               (TP_Context *tp, TP_Arena *arena, RDIB_DataSectionList *sect_list, RDIB_StringMap *string_map, U64 total_count, U64 chunk_count, RDIB_ScopeChunk **chunks);
internal void rdib_data_sections_from_name_maps            (TP_Context *tp, TP_Arena *arena, RDIB_DataSectionList *sect_list, RDIB_StringMap *string_map, RDIB_IndexRunMap *idx_run_map, RDIB_StringMapBucket **src_name_maps[RDI_NameMapKind_COUNT], U64 src_name_map_counts[RDI_NameMapKind_COUNT]);
internal void rdib_data_sections_from_source_line_maps 	   (TP_Context *tp, TP_Arena *arena, RDIB_DataSectionList *sect_list, U64 total_src_file_count, U64 src_file_chunk_count, RDIB_SourceFileChunk **src_fille_chunks);
internal void rdib_data_sections_from_unit_gvar_scope_vmaps(TP_Context *tp, TP_Arena *arena, RDIB_DataSectionList *sect_list, U64 unit_chunk_count, RDIB_UnitChunk **unit_chunks, U64 gvar_chunk_count, RDIB_VariableChunk **gvar_chunks, U64 scope_chunk_count, RDIB_ScopeChunk **scope_chunks);
internal void rdib_data_sections_from_inline_sites         (TP_Context *tp, Arena *arena, RDIB_DataSectionList *sect_list, RDIB_StringMap *string_map, U64 total_inline_site_count, U64 inline_site_chunk_count, RDIB_InlineSiteChunk **inline_site_chunks);
internal void rdib_data_sections_from_checksums            (TP_Context *tp, Arena *arena, RDIB_DataSectionList *sect_list);

internal RDIB_Input  rdib_init_input(Arena *arena);
internal String8List rdib_finish(TP_Context *tp, TP_Arena *arena, RDIB_Input *input);

