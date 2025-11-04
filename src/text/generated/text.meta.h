// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef TEXT_META_H
#define TEXT_META_H

typedef enum TXT_LangKind
{
TXT_LangKind_Null,
TXT_LangKind_C,
TXT_LangKind_CPlusPlus,
TXT_LangKind_Odin,
TXT_LangKind_Jai,
TXT_LangKind_Zig,
TXT_LangKind_Rust,
TXT_LangKind_DisasmX64Intel,
TXT_LangKind_COUNT,
} TXT_LangKind;

C_LINKAGE_BEGIN
extern String8 txt_lang_kind_ext_table[8];
extern String8Array txt_keywords_from_lang_kind_table[8];
extern String8Array txt_multichar_symbols_from_lang_kind_table[8];
extern TXT_TokenizerRuleArray txt_tokenizer_rules_from_lang_kind_table[8];
extern String8 txt_keywords__null[1];
extern String8 txt_multichar_symbols__null[1];
extern TXT_TokenizerRule txt_tokenizer_rules__null[1];
extern String8 txt_keywords__c[32];
extern String8 txt_multichar_symbols__c[20];
extern TXT_TokenizerRule txt_tokenizer_rules__c[7];
extern String8 txt_keywords__cpp[97];
extern String8 txt_keywords__odin[40];
extern String8 txt_keywords__jai[39];
extern String8 txt_keywords__zig[49];
extern String8 txt_keywords__rust[43];

C_LINKAGE_END

#endif // TEXT_META_H
