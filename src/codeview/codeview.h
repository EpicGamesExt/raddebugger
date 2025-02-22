// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef CODEVIEW_H
#define CODEVIEW_H

// https://github.com/microsoft/microsoft-pdb/blob/master/include/cvinfo.h

////////////////////////////////
//~ rjf: CodeView Format Shared Types

#define CV_MinComplexTypeIndex 0x1000

#define CV_TypeIndex_Max max_U32
typedef U32          CV_TypeIndex;
typedef CV_TypeIndex CV_TypeId;
typedef CV_TypeIndex CV_ItemId;

#define CV_ModIndex_Max     max_U16
#define CV_ModIndex_Invalid CV_ModIndex_Max
typedef U16 CV_ModIndex;

typedef U16 CV_SectionIndex;
typedef U16 CV_Reg;

read_only global CV_TypeId cv_type_id_variadic = 0xFFFFFFFF;

////////////////////////////////
//~ rjf: Generated Code

#include "generated/codeview.meta.h"

////////////////////////////////
//~ Aligns

#define CV_LeafAlign          4
#define CV_SymbolAlign        1
#define CV_C13SubSectionAlign 4
#define CV_FileCheckSumsAlign 4

////////////////////////////////
//~ rjf: Registers

// X(NAME, CODE, (RDI_RegCode_X86) NAME, BYTE_POS, BYTE_SIZE)
#define CV_Reg_X86_XList(X)      \
  X(NONE,     0, nil,    0,  0)  \
  X(AL,       1, eax,    0,  1)  \
  X(CL,       2, ecx,    0,  1)  \
  X(DL,       3, edx,    0,  1)  \
  X(BL,       4, ebx,    0,  1)  \
  X(AH,       5, eax,    1,  1)  \
  X(CH,       6, ecx,    1,  1)  \
  X(DH,       7, edx,    1,  1)  \
  X(BH,       8, ebx,    1,  1)  \
  X(AX,       9, eax,    0,  2)  \
  X(CX,      10, ecx,    0,  2)  \
  X(DX,      11, edx,    0,  2)  \
  X(BX,      12, ebx,    0,  2)  \
  X(SP,      13, esp,    0,  2)  \
  X(BP,      14, ebp,    0,  2)  \
  X(SI,      15, esi,    0,  2)  \
  X(DI,      16, edi,    0,  2)  \
  X(EAX,     17, eax,    0,  4)  \
  X(ECX,     18, ecx,    0,  4)  \
  X(EDX,     19, edx,    0,  4)  \
  X(EBX,     20, ebx,    0,  4)  \
  X(ESP,     21, esp,    0,  4)  \
  X(EBP,     22, ebp,    0,  4)  \
  X(ESI,     23, esi,    0,  4)  \
  X(EDI,     24, edi,    0,  4)  \
  X(ES,      25, es,     0,  2)  \
  X(CS,      26, cs,     0,  2)  \
  X(SS,      27, ss,     0,  2)  \
  X(DS,      28, ds,     0,  2)  \
  X(FS,      29, fs,     0,  2)  \
  X(GS,      30, gs,     0,  2)  \
  X(IP,      31, eip,    0,  2)  \
  X(FLAGS,   32, eflags, 0,  2)  \
  X(EIP,     33, eip,    0,  4)  \
  X(EFLAGS,  34, eflags, 0,  4)  \
  X(MM0,    146, fpr0,   0,  8)  \
  X(MM1,    147, fpr1,   0,  8)  \
  X(MM2,    148, fpr2,   0,  8)  \
  X(MM3,    149, fpr3,   0,  8)  \
  X(MM4,    150, fpr4,   0,  8)  \
  X(MM5,    151, fpr5,   0,  8)  \
  X(MM6,    152, fpr6,   0,  8)  \
  X(MM7,    153, fpr7,   0,  8)  \
  X(XMM0,   154, ymm0,   0,  16) \
  X(XMM1,   155, ymm1,   0,  16) \
  X(XMM2,   156, ymm2,   0,  16) \
  X(XMM3,   157, ymm3,   0,  16) \
  X(XMM4,   158, ymm4,   0,  16) \
  X(XMM5,   159, ymm5,   0,  16) \
  X(XMM6,   160, ymm6,   0,  16) \
  X(XMM7,   161, ymm7,   0,  16) \
  X(XMM00,  162, ymm0,   0,  4)  \
  X(XMM01,  163, ymm0,   4,  4)  \
  X(XMM02,  164, ymm0,   8,  4)  \
  X(XMM03,  165, ymm0,   12, 4)  \
  X(XMM10,  166, ymm1,   0,  4)  \
  X(XMM11,  167, ymm1,   4,  4)  \
  X(XMM12,  168, ymm1,   8,  4)  \
  X(XMM13,  169, ymm1,   12, 4)  \
  X(XMM20,  170, ymm2,   0,  4)  \
  X(XMM21,  171, ymm2,   4,  4)  \
  X(XMM22,  172, ymm2,   8,  4)  \
  X(XMM23,  173, ymm2,   12, 4)  \
  X(XMM30,  174, ymm3,   0,  4)  \
  X(XMM31,  175, ymm3,   4,  4)  \
  X(XMM32,  176, ymm3,   8,  4)  \
  X(XMM33,  177, ymm3,   12, 4)  \
  X(XMM40,  178, ymm4,   0,  4)  \
  X(XMM41,  179, ymm4,   4,  4)  \
  X(XMM42,  180, ymm4,   8,  4)  \
  X(XMM43,  181, ymm4,   12, 4)  \
  X(XMM50,  182, ymm5,   0,  4)  \
  X(XMM51,  183, ymm5,   4,  4)  \
  X(XMM52,  184, ymm5,   8,  4)  \
  X(XMM53,  185, ymm5,   12, 4)  \
  X(XMM60,  186, ymm6,   0,  4)  \
  X(XMM61,  187, ymm6,   4,  4)  \
  X(XMM62,  188, ymm6,   8,  4)  \
  X(XMM63,  189, ymm6,   12, 4)  \
  X(XMM70,  190, ymm7,   0,  4)  \
  X(XMM71,  191, ymm7,   4,  4)  \
  X(XMM72,  192, ymm7,   8,  4)  \
  X(XMM73,  193, ymm7,   12, 4)  \
  X(XMM0L,  194, ymm0,   0,  8)  \
  X(XMM1L,  195, ymm1,   0,  8)  \
  X(XMM2L,  196, ymm2,   0,  8)  \
  X(XMM3L,  197, ymm3,   0,  8)  \
  X(XMM4L,  198, ymm4,   0,  8)  \
  X(XMM5L,  199, ymm5,   0,  8)  \
  X(XMM6L,  200, ymm6,   0,  8)  \
  X(XMM7L,  201, ymm7,   0,  8)  \
  X(XMM0H,  202, ymm0,   8,  8)  \
  X(XMM1H,  203, ymm1,   8,  8)  \
  X(XMM2H,  204, ymm2,   8,  8)  \
  X(XMM3H,  205, ymm3,   8,  8)  \
  X(XMM4H,  206, ymm4,   8,  8)  \
  X(XMM5H,  207, ymm5,   8,  8)  \
  X(XMM6H,  208, ymm6,   8,  8)  \
  X(XMM7H,  209, ymm7,   8,  8)  \
  X(YMM0,   252, ymm0,   0,  32) \
  X(YMM1,   253, ymm1,   0,  32) \
  X(YMM2,   254, ymm2,   0,  32) \
  X(YMM3,   255, ymm3,   0,  32) \
  X(YMM4,   256, ymm4,   0,  32) \
  X(YMM5,   257, ymm5,   0,  32) \
  X(YMM6,   258, ymm6,   0,  32) \
  X(YMM7,   259, ymm7,   0,  32) \
  X(YMM0H,  260, ymm0,   16, 16) \
  X(YMM1H,  261, ymm1,   16, 16) \
  X(YMM2H,  262, ymm2,   16, 16) \
  X(YMM3H,  263, ymm3,   16, 16) \
  X(YMM4H,  264, ymm4,   16, 16) \
  X(YMM5H,  265, ymm5,   16, 16) \
  X(YMM6H,  266, ymm6,   16, 16) \
  X(YMM7H,  267, ymm7,   16, 16) \
  X(YMM0I0, 268, ymm0,   0,  8)  \
  X(YMM0I1, 269, ymm0,   8,  8)  \
  X(YMM0I2, 270, ymm0,   16, 8)  \
  X(YMM0I3, 271, ymm0,   24, 8)  \
  X(YMM1I0, 272, ymm1,   0,  8)  \
  X(YMM1I1, 273, ymm1,   8,  8)  \
  X(YMM1I2, 274, ymm1,   16, 8)  \
  X(YMM1I3, 275, ymm1,   24, 8)  \
  X(YMM2I0, 276, ymm2,   0,  8)  \
  X(YMM2I1, 277, ymm2,   8,  8)  \
  X(YMM2I2, 278, ymm2,   16, 8)  \
  X(YMM2I3, 279, ymm2,   24, 8)  \
  X(YMM3I0, 280, ymm3,   0,  8)  \
  X(YMM3I1, 281, ymm3,   8,  8)  \
  X(YMM3I2, 282, ymm3,   16, 8)  \
  X(YMM3I3, 283, ymm3,   24, 8)  \
  X(YMM4I0, 284, ymm4,   0,  8)  \
  X(YMM4I1, 285, ymm4,   8,  8)  \
  X(YMM4I2, 286, ymm4,   16, 8)  \
  X(YMM4I3, 287, ymm4,   24, 8)  \
  X(YMM5I0, 288, ymm5,   0,  8)  \
  X(YMM5I1, 289, ymm5,   8,  8)  \
  X(YMM5I2, 290, ymm5,   16, 8)  \
  X(YMM5I3, 291, ymm5,   24, 8)  \
  X(YMM6I0, 292, ymm6,   0,  8)  \
  X(YMM6I1, 293, ymm6,   8,  8)  \
  X(YMM6I2, 294, ymm6,   16, 8)  \
  X(YMM6I3, 295, ymm6,   24, 8)  \
  X(YMM7I0, 296, ymm7,   0,  8)  \
  X(YMM7I1, 297, ymm7,   8,  8)  \
  X(YMM7I2, 298, ymm7,   16, 8)  \
  X(YMM7I3, 299, ymm7,   24, 8)  \
  X(YMM0F0, 300, ymm0,   0,  4)  \
  X(YMM0F1, 301, ymm0,   4,  4)  \
  X(YMM0F2, 302, ymm0,   8,  4)  \
  X(YMM0F3, 303, ymm0,   12, 4)  \
  X(YMM0F4, 304, ymm0,   16, 4)  \
  X(YMM0F5, 305, ymm0,   20, 4)  \
  X(YMM0F6, 306, ymm0,   24, 4)  \
  X(YMM0F7, 307, ymm0,   28, 4)  \
  X(YMM1F0, 308, ymm1,   0,  4)  \
  X(YMM1F1, 309, ymm1,   4,  4)  \
  X(YMM1F2, 310, ymm1,   8,  4)  \
  X(YMM1F3, 311, ymm1,   12, 4)  \
  X(YMM1F4, 312, ymm1,   16, 4)  \
  X(YMM1F5, 313, ymm1,   20, 4)  \
  X(YMM1F6, 314, ymm1,   24, 4)  \
  X(YMM1F7, 315, ymm1,   28, 4)  \
  X(YMM2F0, 316, ymm2,   0,  4)  \
  X(YMM2F1, 317, ymm2,   4,  4)  \
  X(YMM2F2, 318, ymm2,   8,  4)  \
  X(YMM2F3, 319, ymm2,   12, 4)  \
  X(YMM2F4, 320, ymm2,   16, 4)  \
  X(YMM2F5, 321, ymm2,   20, 4)  \
  X(YMM2F6, 322, ymm2,   24, 4)  \
  X(YMM2F7, 323, ymm2,   28, 4)  \
  X(YMM3F0, 324, ymm3,   0,  4)  \
  X(YMM3F1, 325, ymm3,   4,  4)  \
  X(YMM3F2, 326, ymm3,   8,  4)  \
  X(YMM3F3, 327, ymm3,   12, 4)  \
  X(YMM3F4, 328, ymm3,   16, 4)  \
  X(YMM3F5, 329, ymm3,   20, 4)  \
  X(YMM3F6, 330, ymm3,   24, 4)  \
  X(YMM3F7, 331, ymm3,   28, 4)  \
  X(YMM4F0, 332, ymm4,   0,  4)  \
  X(YMM4F1, 333, ymm4,   4,  4)  \
  X(YMM4F2, 334, ymm4,   8,  4)  \
  X(YMM4F3, 335, ymm4,   12, 4)  \
  X(YMM4F4, 336, ymm4,   16, 4)  \
  X(YMM4F5, 337, ymm4,   20, 4)  \
  X(YMM4F6, 338, ymm4,   24, 4)  \
  X(YMM4F7, 339, ymm4,   28, 4)  \
  X(YMM5F0, 340, ymm5,   0,  4)  \
  X(YMM5F1, 341, ymm5,   4,  4)  \
  X(YMM5F2, 342, ymm5,   8,  4)  \
  X(YMM5F3, 343, ymm5,   12, 4)  \
  X(YMM5F4, 344, ymm5,   16, 4)  \
  X(YMM5F5, 345, ymm5,   20, 4)  \
  X(YMM5F6, 346, ymm5,   24, 4)  \
  X(YMM5F7, 347, ymm5,   28, 4)  \
  X(YMM6F0, 348, ymm6,   0,  4)  \
  X(YMM6F1, 349, ymm6,   4,  4)  \
  X(YMM6F2, 350, ymm6,   8,  4)  \
  X(YMM6F3, 351, ymm6,   12, 4)  \
  X(YMM6F4, 352, ymm6,   16, 4)  \
  X(YMM6F5, 353, ymm6,   20, 4)  \
  X(YMM6F6, 354, ymm6,   24, 4)  \
  X(YMM6F7, 355, ymm6,   28, 4)  \
  X(YMM7F0, 356, ymm7,   0,  4)  \
  X(YMM7F1, 357, ymm7,   4,  4)  \
  X(YMM7F2, 358, ymm7,   8,  4)  \
  X(YMM7F3, 359, ymm7,   12, 4)  \
  X(YMM7F4, 360, ymm7,   16, 4)  \
  X(YMM7F5, 361, ymm7,   20, 4)  \
  X(YMM7F6, 362, ymm7,   24, 4)  \
  X(YMM7F7, 363, ymm7,   28, 4)  \
  X(YMM0D0, 364, ymm0,   0,  8)  \
  X(YMM0D1, 365, ymm0,   8,  8)  \
  X(YMM0D2, 366, ymm0,   16, 8)  \
  X(YMM0D3, 367, ymm0,   24, 8)  \
  X(YMM1D0, 368, ymm1,   0,  8)  \
  X(YMM1D1, 369, ymm1,   8,  8)  \
  X(YMM1D2, 370, ymm1,   16, 8)  \
  X(YMM1D3, 371, ymm1,   24, 8)  \
  X(YMM2D0, 372, ymm2,   0,  8)  \
  X(YMM2D1, 373, ymm2,   8,  8)  \
  X(YMM2D2, 374, ymm2,   16, 8)  \
  X(YMM2D3, 375, ymm2,   24, 8)  \
  X(YMM3D0, 376, ymm3,   0,  8)  \
  X(YMM3D1, 377, ymm3,   8,  8)  \
  X(YMM3D2, 378, ymm3,   16, 8)  \
  X(YMM3D3, 379, ymm3,   24, 8)  \
  X(YMM4D0, 380, ymm4,   0,  8)  \
  X(YMM4D1, 381, ymm4,   8,  8)  \
  X(YMM4D2, 382, ymm4,   16, 8)  \
  X(YMM4D3, 383, ymm4,   24, 8)  \
  X(YMM5D0, 384, ymm5,   0,  8)  \
  X(YMM5D1, 385, ymm5,   8,  8)  \
  X(YMM5D2, 386, ymm5,   16, 8)  \
  X(YMM5D3, 387, ymm5,   24, 8)  \
  X(YMM6D0, 388, ymm6,   0,  8)  \
  X(YMM6D1, 389, ymm6,   8,  8)  \
  X(YMM6D2, 390, ymm6,   16, 8)  \
  X(YMM6D3, 391, ymm6,   24, 8)  \
  X(YMM7D0, 392, ymm7,   0,  8)  \
  X(YMM7D1, 393, ymm7,   8,  8)  \
  X(YMM7D2, 394, ymm7,   16, 8)  \
  X(YMM7D3, 395, ymm7,   24, 8)

typedef U16 CV_Regx86;
typedef enum CV_Regx86Enum
{
#define X(CVN,C,RDN,BP,BZ) CV_Regx86_##CVN = C,
  CV_Reg_X86_XList(X)
#undef X
}
CV_Regx86Enum;

// X(NAME, CODE, (RDI_RegisterCode_X64) NAME, BYTE_POS, BYTE_SIZE)
#define CV_Reg_X64_XList(X)       \
  X(NONE,      0, nil,    0,  0)  \
  X(AL,        1, rax,    0,  1)  \
  X(CL,        2, rcx,    0,  1)  \
  X(DL,        3, rdx,    0,  1)  \
  X(BL,        4, rbx,    0,  1)  \
  X(AH,        5, rax,    1,  1)  \
  X(CH,        6, rcx,    1,  1)  \
  X(DH,        7, rdx,    1,  1)  \
  X(BH,        8, rbx,    1,  1)  \
  X(AX,        9, rax,    0,  2)  \
  X(CX,       10, rcx,    0,  2)  \
  X(DX,       11, rdx,    0,  2)  \
  X(BX,       12, rbx,    0,  2)  \
  X(SP,       13, rsp,    0,  2)  \
  X(BP,       14, rbp,    0,  2)  \
  X(SI,       15, rsi,    0,  2)  \
  X(DI,       16, rdi,    0,  2)  \
  X(EAX,      17, rax,    0,  4)  \
  X(ECX,      18, rcx,    0,  4)  \
  X(EDX,      19, rdx,    0,  4)  \
  X(EBX,      20, rbx,    0,  4)  \
  X(ESP,      21, rsp,    0,  4)  \
  X(EBP,      22, rbp,    0,  4)  \
  X(ESI,      23, rsi,    0,  4)  \
  X(EDI,      24, rdi,    0,  4)  \
  X(ES,       25, es,     0,  2)  \
  X(CS,       26, cs,     0,  2)  \
  X(SS,       27, ss,     0,  2)  \
  X(DS,       28, ds,     0,  2)  \
  X(FS,       29, fs,     0,  2)  \
  X(GS,       30, gs,     0,  2)  \
  X(FLAGS,    32, rflags, 0,  2)  \
  X(RIP,      33, rip,    0,  8)  \
  X(EFLAGS,   34, rflags, 0,  4)  \
  /* TODO: possibly missing control registers in x64 definitions? */ \
  X(CR0,      80, nil,    0,  0)  \
  X(CR1,      81, nil,    0,  0)  \
  X(CR2,      82, nil,    0,  0)  \
  X(CR3,      83, nil,    0,  0)  \
  X(CR4,      84, nil,    0,  0)  \
  X(CR8,      88, nil,    0,  0)  \
  X(DR0,      90, dr0,    0,  4)  \
  X(DR1,      91, dr1,    0,  4)  \
  X(DR2,      92, dr2,    0,  4)  \
  X(DR3,      93, dr3,    0,  4)  \
  X(DR4,      94, dr4,    0,  4)  \
  X(DR5,      95, dr5,    0,  4)  \
  X(DR6,      96, dr6,    0,  4)  \
  X(DR7,      97, dr7,    0,  4)  \
  /* TODO: possibly missing debug registers 8-15 in x64 definitions? */ \
  X(DR8,      98, nil,    0,  0)  \
  X(DR9,      99, nil,    0,  0)  \
  X(DR10,    100, nil,    0,  0)  \
  X(DR11,    101, nil,    0,  0)  \
  X(DR12,    102, nil,    0,  0)  \
  X(DR13,    103, nil,    0,  0)  \
  X(DR14,    104, nil,    0,  0)  \
  X(DR15,    105, nil,    0,  0)  \
  /* TODO: possibly missing ~whatever these are~ in x64 definitions? */ \
  X(GDTR,    110, nil,    0,  0)  \
  X(GDTL,    111, nil,    0,  0)  \
  X(IDTR,    112, nil,    0,  0)  \
  X(IDTL,    113, nil,    0,  0)  \
  X(LDTR,    114, nil,    0,  0)  \
  X(TR,      115, nil,    0,  0)  \
  X(ST0,     128, st0,    0,  10) \
  X(ST1,     129, st1,    0,  10) \
  X(ST2,     130, st2,    0,  10) \
  X(ST3,     131, st3,    0,  10) \
  X(ST4,     132, st4,    0,  10) \
  X(ST5,     133, st5,    0,  10) \
  X(ST6,     134, st6,    0,  10) \
  X(ST7,     135, st7,    0,  10) \
  /* TODO: possibly missing these, or not sure how they map to our x64 definitions? */ \
  X(CTRL,    136, nil,    0,  0)  \
  X(STAT,    137, nil,    0,  0)  \
  X(TAG,     138, nil,    0,  0)  \
  X(FPIP,    139, nil,    0,  0)  \
  X(FPCS,    140, nil,    0,  0)  \
  X(FPDO,    141, nil,    0,  0)  \
  X(FPDS,    142, nil,    0,  0)  \
  X(ISEM,    143, nil,    0,  0)  \
  X(FPEIP,   144, nil,    0,  0)  \
  X(FPEDO,   145, nil,    0,  0)  \
  X(MM0,     146, fpr0,   0,  8)  \
  X(MM1,     147, fpr1,   0,  8)  \
  X(MM2,     148, fpr2,   0,  8)  \
  X(MM3,     149, fpr3,   0,  8)  \
  X(MM4,     150, fpr4,   0,  8)  \
  X(MM5,     151, fpr5,   0,  8)  \
  X(MM6,     152, fpr6,   0,  8)  \
  X(MM7,     153, fpr7,   0,  8)  \
  X(XMM0,    154, zmm0,   0,  16) \
  X(XMM1,    155, zmm1,   0,  16) \
  X(XMM2,    156, zmm2,   0,  16) \
  X(XMM3,    157, zmm3,   0,  16) \
  X(XMM4,    158, zmm4,   0,  16) \
  X(XMM5,    159, zmm5,   0,  16) \
  X(XMM6,    160, zmm6,   0,  16) \
  X(XMM7,    161, zmm7,   0,  16) \
  X(XMM0_0,  162, zmm0,   0,  4)  \
  X(XMM0_1,  163, zmm0,   4,  4)  \
  X(XMM0_2,  164, zmm0,   8,  4)  \
  X(XMM0_3,  165, zmm0,   12, 4)  \
  X(XMM1_0,  166, zmm1,   0,  4)  \
  X(XMM1_1,  167, zmm1,   4,  4)  \
  X(XMM1_2,  168, zmm1,   8,  4)  \
  X(XMM1_3,  169, zmm1,   12, 4)  \
  X(XMM2_0,  170, zmm2,   0,  4)  \
  X(XMM2_1,  171, zmm2,   4,  4)  \
  X(XMM2_2,  172, zmm2,   8,  4)  \
  X(XMM2_3,  173, zmm2,   12, 4)  \
  X(XMM3_0,  174, zmm3,   0,  4)  \
  X(XMM3_1,  175, zmm3,   4,  4)  \
  X(XMM3_2,  176, zmm3,   8,  4)  \
  X(XMM3_3,  177, zmm3,   12, 4)  \
  X(XMM4_0,  178, zmm4,   0,  4)  \
  X(XMM4_1,  179, zmm4,   4,  4)  \
  X(XMM4_2,  180, zmm4,   8,  4)  \
  X(XMM4_3,  181, zmm4,   12, 4)  \
  X(XMM5_0,  182, zmm5,   0,  4)  \
  X(XMM5_1,  183, zmm5,   4,  4)  \
  X(XMM5_2,  184, zmm5,   8,  4)  \
  X(XMM5_3,  185, zmm5,   12, 4)  \
  X(XMM6_0,  186, zmm6,   0,  4)  \
  X(XMM6_1,  187, zmm6,   4,  4)  \
  X(XMM6_2,  188, zmm6,   8,  4)  \
  X(XMM6_3,  189, zmm6,   12, 4)  \
  X(XMM7_0,  190, zmm7,   0,  4)  \
  X(XMM7_1,  191, zmm7,   4,  4)  \
  X(XMM7_2,  192, zmm7,   8,  4)  \
  X(XMM7_3,  193, zmm7,   12, 4)  \
  X(XMM0L,   194, zmm0,   0,  8)  \
  X(XMM1L,   195, zmm1,   0,  8)  \
  X(XMM2L,   196, zmm2,   0,  8)  \
  X(XMM3L,   197, zmm3,   0,  8)  \
  X(XMM4L,   198, zmm4,   0,  8)  \
  X(XMM5L,   199, zmm5,   0,  8)  \
  X(XMM6L,   200, zmm6,   0,  8)  \
  X(XMM7L,   201, zmm7,   0,  8)  \
  X(XMM0H,   202, zmm0,   8,  8)  \
  X(XMM1H,   203, zmm1,   8,  8)  \
  X(XMM2H,   204, zmm2,   8,  8)  \
  X(XMM3H,   205, zmm3,   8,  8)  \
  X(XMM4H,   206, zmm4,   8,  8)  \
  X(XMM5H,   207, zmm5,   8,  8)  \
  X(XMM6H,   208, zmm6,   8,  8)  \
  X(XMM7H,   209, zmm7,   8,  8)  \
  X(MXCSR,   211, mxcsr,  0,  4)  \
  X(EMM0L,   220, zmm0,   0,  8)  \
  X(EMM1L,   221, zmm1,   0,  8)  \
  X(EMM2L,   222, zmm2,   0,  8)  \
  X(EMM3L,   223, zmm3,   0,  8)  \
  X(EMM4L,   224, zmm4,   0,  8)  \
  X(EMM5L,   225, zmm5,   0,  8)  \
  X(EMM6L,   226, zmm6,   0,  8)  \
  X(EMM7L,   227, zmm7,   0,  8)  \
  X(EMM0H,   228, zmm0,   8,  8)  \
  X(EMM1H,   229, zmm1,   8,  8)  \
  X(EMM2H,   230, zmm2,   8,  8)  \
  X(EMM3H,   231, zmm3,   8,  8)  \
  X(EMM4H,   232, zmm4,   8,  8)  \
  X(EMM5H,   233, zmm5,   8,  8)  \
  X(EMM6H,   234, zmm6,   8,  8)  \
  X(EMM7H,   235, zmm7,   8,  8)  \
  X(MM00,    236, fpr0,   0,  4)  \
  X(MM01,    237, fpr0,   4,  4)  \
  X(MM10,    238, fpr1,   0,  4)  \
  X(MM11,    239, fpr1,   4,  4)  \
  X(MM20,    240, fpr2,   0,  4)  \
  X(MM21,    241, fpr2,   4,  4)  \
  X(MM30,    242, fpr3,   0,  4)  \
  X(MM31,    243, fpr3,   4,  4)  \
  X(MM40,    244, fpr4,   0,  4)  \
  X(MM41,    245, fpr4,   4,  4)  \
  X(MM50,    246, fpr5,   0,  4)  \
  X(MM51,    247, fpr5,   4,  4)  \
  X(MM60,    248, fpr6,   0,  4)  \
  X(MM61,    249, fpr6,   4,  4)  \
  X(MM70,    250, fpr7,   0,  4)  \
  X(MM71,    251, fpr7,   4,  4)  \
  X(XMM8,    252, zmm8,   0,  16) \
  X(XMM9,    253, zmm9,   0,  16) \
  X(XMM10,   254, zmm10,  0,  16) \
  X(XMM11,   255, zmm11,  0,  16) \
  X(XMM12,   256, zmm12,  0,  16) \
  X(XMM13,   257, zmm13,  0,  16) \
  X(XMM14,   258, zmm14,  0,  16) \
  X(XMM15,   259, zmm15,  0,  16) \
  X(XMM8_0,  260, zmm8,   0,  16) \
  X(XMM8_1,  261, zmm8,   4,  16) \
  X(XMM8_2,  262, zmm8,   8,  16) \
  X(XMM8_3,  263, zmm8,   12, 16) \
  X(XMM9_0,  264, zmm9,   0,  4)  \
  X(XMM9_1,  265, zmm9,   4,  4)  \
  X(XMM9_2,  266, zmm9,   8,  4)  \
  X(XMM9_3,  267, zmm9,   12, 4)  \
  X(XMM10_0, 268, zmm10,  0,  4)  \
  X(XMM10_1, 269, zmm10,  4,  4)  \
  X(XMM10_2, 270, zmm10,  8,  4)  \
  X(XMM10_3, 271, zmm10,  12, 4)  \
  X(XMM11_0, 272, zmm11,  0,  4)  \
  X(XMM11_1, 273, zmm11,  4,  4)  \
  X(XMM11_2, 274, zmm11,  8,  4)  \
  X(XMM11_3, 275, zmm11,  12, 4)  \
  X(XMM12_0, 276, zmm12,  0,  4)  \
  X(XMM12_1, 277, zmm12,  4,  4)  \
  X(XMM12_2, 278, zmm12,  8,  4)  \
  X(XMM12_3, 279, zmm12,  12, 4)  \
  X(XMM13_0, 280, zmm13,  0,  4)  \
  X(XMM13_1, 281, zmm13,  4,  4)  \
  X(XMM13_2, 282, zmm13,  8,  4)  \
  X(XMM13_3, 283, zmm13,  12, 4)  \
  X(XMM14_0, 284, zmm14,  0,  4)  \
  X(XMM14_1, 285, zmm14,  4,  4)  \
  X(XMM14_2, 286, zmm14,  8,  4)  \
  X(XMM14_3, 287, zmm14,  12, 4)  \
  X(XMM15_0, 288, zmm15,  0,  4)  \
  X(XMM15_1, 289, zmm15,  4,  4)  \
  X(XMM15_2, 290, zmm15,  8,  4)  \
  X(XMM15_3, 291, zmm15,  12, 4)  \
  X(XMM8L,   292, zmm8,   0,  8)  \
  X(XMM9L,   293, zmm9,   0,  8)  \
  X(XMM10L,  294, zmm10,  0,  8)  \
  X(XMM11L,  295, zmm11,  0,  8)  \
  X(XMM12L,  296, zmm12,  0,  8)  \
  X(XMM13L,  297, zmm13,  0,  8)  \
  X(XMM14L,  298, zmm14,  0,  8)  \
  X(XMM15L,  299, zmm15,  0,  8)  \
  X(XMM8H,   300, zmm8,   8,  8)  \
  X(XMM9H,   301, zmm9,   8,  8)  \
  X(XMM10H,  302, zmm10,  8,  8)  \
  X(XMM11H,  303, zmm11,  8,  8)  \
  X(XMM12H,  304, zmm12,  8,  8)  \
  X(XMM13H,  305, zmm13,  8,  8)  \
  X(XMM14H,  306, zmm14,  8,  8)  \
  X(XMM15H,  307, zmm15,  8,  8)  \
  X(EMM8L,   308, zmm8,   0,  8)  \
  X(EMM9L,   309, zmm9,   0,  8)  \
  X(EMM10L,  310, zmm10,  0,  8)  \
  X(EMM11L,  311, zmm11,  0,  8)  \
  X(EMM12L,  312, zmm12,  0,  8)  \
  X(EMM13L,  313, zmm13,  0,  8)  \
  X(EMM14L,  314, zmm14,  0,  8)  \
  X(EMM15L,  315, zmm15,  0,  8)  \
  X(EMM8H,   316, zmm8,   8,  8)  \
  X(EMM9H,   317, zmm9,   8,  8)  \
  X(EMM10H,  318, zmm10,  8,  8)  \
  X(EMM11H,  319, zmm11,  8,  8)  \
  X(EMM12H,  320, zmm12,  8,  8)  \
  X(EMM13H,  321, zmm13,  8,  8)  \
  X(EMM14H,  322, zmm14,  8,  8)  \
  X(EMM15H,  323, zmm15,  8,  8)  \
  X(SIL,     324, rsi,    0,  1)  \
  X(DIL,     325, rdi,    0,  1)  \
  X(BPL,     326, rbp,    0,  1)  \
  X(SPL,     327, rsp,    0,  1)  \
  X(RAX,     328, rax,    0,  8)  \
  X(RBX,     329, rbx,    0,  8)  \
  X(RCX,     330, rcx,    0,  8)  \
  X(RDX,     331, rdx,    0,  8)  \
  X(RSI,     332, rsi,    0,  8)  \
  X(RDI,     333, rdi,    0,  8)  \
  X(RBP,     334, rbp,    0,  8)  \
  X(RSP,     335, rsp,    0,  8)  \
  X(R8,      336, r8,     0,  8)  \
  X(R9,      337, r9,     0,  8)  \
  X(R10,     338, r10,    0,  8)  \
  X(R11,     339, r11,    0,  8)  \
  X(R12,     340, r12,    0,  8)  \
  X(R13,     341, r13,    0,  8)  \
  X(R14,     342, r14,    0,  8)  \
  X(R15,     343, r15,    0,  8)  \
  X(R8B,     344, r8,     0,  1)  \
  X(R9B,     345, r9,     0,  1)  \
  X(R10B,    346, r10,    0,  1)  \
  X(R11B,    347, r11,    0,  1)  \
  X(R12B,    348, r12,    0,  1)  \
  X(R13B,    349, r13,    0,  1)  \
  X(R14B,    350, r14,    0,  1)  \
  X(R15B,    351, r15,    0,  1)  \
  X(R8W,     352, r8,     0,  2)  \
  X(R9W,     353, r9,     0,  2)  \
  X(R10W,    354, r10,    0,  2)  \
  X(R11W,    355, r11,    0,  2)  \
  X(R12W,    356, r12,    0,  2)  \
  X(R13W,    357, r13,    0,  2)  \
  X(R14W,    358, r14,    0,  2)  \
  X(R15W,    359, r15,    0,  2)  \
  X(R8D,     360, r8,     0,  4)  \
  X(R9D,     361, r9,     0,  4)  \
  X(R10D,    362, r10,    0,  4)  \
  X(R11D,    363, r11,    0,  4)  \
  X(R12D,    364, r12,    0,  4)  \
  X(R13D,    365, r13,    0,  4)  \
  X(R14D,    366, r14,    0,  4)  \
  X(R15D,    367, r15,    0,  4)  \
  X(YMM0,    368, zmm0,   0,  32) \
  X(YMM1,    369, zmm1,   0,  32) \
  X(YMM2,    370, zmm2,   0,  32) \
  X(YMM3,    371, zmm3,   0,  32) \
  X(YMM4,    372, zmm4,   0,  32) \
  X(YMM5,    373, zmm5,   0,  32) \
  X(YMM6,    374, zmm6,   0,  32) \
  X(YMM7,    375, zmm7,   0,  32) \
  X(YMM8,    376, zmm8,   0,  32) \
  X(YMM9,    377, zmm9,   0,  32) \
  X(YMM10,   378, zmm10,  0,  32) \
  X(YMM11,   379, zmm11,  0,  32) \
  X(YMM12,   380, zmm12,  0,  32) \
  X(YMM13,   381, zmm13,  0,  32) \
  X(YMM14,   382, zmm14,  0,  32) \
  X(YMM15,   383, zmm15,  0,  32) \
  X(YMM0H,   384, zmm0,   16, 32) \
  X(YMM1H,   385, zmm1,   16, 32) \
  X(YMM2H,   386, zmm2,   16, 32) \
  X(YMM3H,   387, zmm3,   16, 32) \
  X(YMM4H,   388, zmm4,   16, 32) \
  X(YMM5H,   389, zmm5,   16, 32) \
  X(YMM6H,   390, zmm6,   16, 32) \
  X(YMM7H,   391, zmm7,   16, 32) \
  X(YMM8H,   392, zmm8,   16, 32) \
  X(YMM9H,   393, zmm9,   16, 32) \
  X(YMM10H,  394, zmm10,  16, 32) \
  X(YMM11H,  395, zmm11,  16, 32) \
  X(YMM12H,  396, zmm12,  16, 32) \
  X(YMM13H,  397, zmm13,  16, 32) \
  X(YMM14H,  398, zmm14,  16, 32) \
  X(YMM15H,  399, zmm15,  16, 32) \
  X(XMM0IL,  400, zmm0,   0,  8)  \
  X(XMM1IL,  401, zmm1,   0,  8)  \
  X(XMM2IL,  402, zmm2,   0,  8)  \
  X(XMM3IL,  403, zmm3,   0,  8)  \
  X(XMM4IL,  404, zmm4,   0,  8)  \
  X(XMM5IL,  405, zmm5,   0,  8)  \
  X(XMM6IL,  406, zmm6,   0,  8)  \
  X(XMM7IL,  407, zmm7,   0,  8)  \
  X(XMM8IL,  408, zmm8,   0,  8)  \
  X(XMM9IL,  409, zmm9,   0,  8)  \
  X(XMM10IL, 410, zmm10,  0,  8)  \
  X(XMM11IL, 411, zmm11,  0,  8)  \
  X(XMM12IL, 412, zmm12,  0,  8)  \
  X(XMM13IL, 413, zmm13,  0,  8)  \
  X(XMM14IL, 414, zmm14,  0,  8)  \
  X(XMM15IL, 415, zmm15,  0,  8)  \
  X(XMM0IH,  416, zmm0,   8,  8)  \
  X(XMM1IH,  417, zmm1,   8,  8)  \
  X(XMM2IH,  418, zmm2,   8,  8)  \
  X(XMM3IH,  419, zmm3,   8,  8)  \
  X(XMM4IH,  420, zmm4,   8,  8)  \
  X(XMM5IH,  421, zmm5,   8,  8)  \
  X(XMM6IH,  422, zmm6,   8,  8)  \
  X(XMM7IH,  423, zmm7,   8,  8)  \
  X(XMM8IH,  424, zmm8,   8,  8)  \
  X(XMM9IH,  425, zmm9,   8,  8)  \
  X(XMM10IH, 426, zmm10,  8,  8)  \
  X(XMM11IH, 427, zmm11,  8,  8)  \
  X(XMM12IH, 428, zmm12,  8,  8)  \
  X(XMM13IH, 429, zmm13,  8,  8)  \
  X(XMM14IH, 430, zmm14,  8,  8)  \
  X(XMM15IH, 431, zmm15,  8,  8)  \
  X(YMM0I0,  432, zmm0,   0,  8)  \
  X(YMM0I1,  433, zmm0,   8,  8)  \
  X(YMM0I2,  434, zmm0,   16, 8)  \
  X(YMM0I3,  435, zmm0,   24, 8)  \
  X(YMM1I0,  436, zmm1,   0,  8)  \
  X(YMM1I1,  437, zmm1,   8,  8)  \
  X(YMM1I2,  438, zmm1,   16, 8)  \
  X(YMM1I3,  439, zmm1,   24, 8)  \
  X(YMM2I0,  440, zmm2,   0,  8)  \
  X(YMM2I1,  441, zmm2,   8,  8)  \
  X(YMM2I2,  442, zmm2,   16, 8)  \
  X(YMM2I3,  443, zmm2,   24, 8)  \
  X(YMM3I0,  444, zmm3,   0,  8)  \
  X(YMM3I1,  445, zmm3,   8,  8)  \
  X(YMM3I2,  446, zmm3,   16, 8)  \
  X(YMM3I3,  447, zmm3,   24, 8)  \
  X(YMM4I0,  448, zmm4,   0,  8)  \
  X(YMM4I1,  449, zmm4,   8,  8)  \
  X(YMM4I2,  450, zmm4,   16, 8)  \
  X(YMM4I3,  451, zmm4,   24, 8)  \
  X(YMM5I0,  452, zmm5,   0,  8)  \
  X(YMM5I1,  453, zmm5,   8,  8)  \
  X(YMM5I2,  454, zmm5,   16, 8)  \
  X(YMM5I3,  455, zmm5,   24, 8)  \
  X(YMM6I0,  456, zmm6,   0,  8)  \
  X(YMM6I1,  457, zmm6,   8,  8)  \
  X(YMM6I2,  458, zmm6,   16, 8)  \
  X(YMM6I3,  459, zmm6,   24, 8)  \
  X(YMM7I0,  460, zmm7,   0,  8)  \
  X(YMM7I1,  461, zmm7,   8,  8)  \
  X(YMM7I2,  462, zmm7,   16, 8)  \
  X(YMM7I3,  463, zmm7,   24, 8)  \
  X(YMM8I0,  464, zmm8,   0,  8)  \
  X(YMM8I1,  465, zmm8,   8,  8)  \
  X(YMM8I2,  466, zmm8,   16, 8)  \
  X(YMM8I3,  467, zmm8,   24, 8)  \
  X(YMM9I0,  468, zmm9,   0,  8)  \
  X(YMM9I1,  469, zmm9,   8,  8)  \
  X(YMM9I2,  470, zmm9,   16, 8)  \
  X(YMM9I3,  471, zmm9,   24, 8)  \
  X(YMM10I0, 472, zmm10,  0,  8)  \
  X(YMM10I1, 473, zmm10,  8,  8)  \
  X(YMM10I2, 474, zmm10,  16, 8)  \
  X(YMM10I3, 475, zmm10,  24, 8)  \
  X(YMM11I0, 476, zmm11,  0,  8)  \
  X(YMM11I1, 477, zmm11,  8,  8)  \
  X(YMM11I2, 478, zmm11,  16, 8)  \
  X(YMM11I3, 479, zmm11,  24, 8)  \
  X(YMM12I0, 480, zmm12,  0,  8)  \
  X(YMM12I1, 481, zmm12,  8,  8)  \
  X(YMM12I2, 482, zmm12,  16, 8)  \
  X(YMM12I3, 483, zmm12,  24, 8)  \
  X(YMM13I0, 484, zmm13,  0,  8)  \
  X(YMM13I1, 485, zmm13,  8,  8)  \
  X(YMM13I2, 486, zmm13,  16, 8)  \
  X(YMM13I3, 487, zmm13,  24, 8)  \
  X(YMM14I0, 488, zmm14,  0,  8)  \
  X(YMM14I1, 489, zmm14,  8,  8)  \
  X(YMM14I2, 490, zmm14,  16, 8)  \
  X(YMM14I3, 491, zmm14,  24, 8)  \
  X(YMM15I0, 492, zmm15,  0,  8)  \
  X(YMM15I1, 493, zmm15,  8,  8)  \
  X(YMM15I2, 494, zmm15,  16, 8)  \
  X(YMM15I3, 495, zmm15,  24, 8)  \
  X(YMM0F0,  496, zmm0,   0,  4)  \
  X(YMM0F1,  497, zmm0,   4,  4)  \
  X(YMM0F2,  498, zmm0,   8,  4)  \
  X(YMM0F3,  499, zmm0,   12, 4)  \
  X(YMM0F4,  500, zmm0,   16, 4)  \
  X(YMM0F5,  501, zmm0,   20, 4)  \
  X(YMM0F6,  502, zmm0,   24, 4)  \
  X(YMM0F7,  503, zmm0,   28, 4)  \
  X(YMM1F0,  504, zmm1,   0,  4)  \
  X(YMM1F1,  505, zmm1,   4,  4)  \
  X(YMM1F2,  506, zmm1,   8,  4)  \
  X(YMM1F3,  507, zmm1,   12, 4)  \
  X(YMM1F4,  508, zmm1,   16, 4)  \
  X(YMM1F5,  509, zmm1,   20, 4)  \
  X(YMM1F6,  510, zmm1,   24, 4)  \
  X(YMM1F7,  511, zmm1,   28, 4)  \
  X(YMM2F0,  512, zmm2,   0,  4)  \
  X(YMM2F1,  513, zmm2,   4,  4)  \
  X(YMM2F2,  514, zmm2,   8,  4)  \
  X(YMM2F3,  515, zmm2,   12, 4)  \
  X(YMM2F4,  516, zmm2,   16, 4)  \
  X(YMM2F5,  517, zmm2,   20, 4)  \
  X(YMM2F6,  518, zmm2,   24, 4)  \
  X(YMM2F7,  519, zmm2,   28, 4)  \
  X(YMM3F0,  520, zmm3,   0,  4)  \
  X(YMM3F1,  521, zmm3,   4,  4)  \
  X(YMM3F2,  522, zmm3,   8,  4)  \
  X(YMM3F3,  523, zmm3,   12, 4)  \
  X(YMM3F4,  524, zmm3,   16, 4)  \
  X(YMM3F5,  525, zmm3,   20, 4)  \
  X(YMM3F6,  526, zmm3,   24, 4)  \
  X(YMM3F7,  527, zmm3,   28, 4)  \
  X(YMM4F0,  528, zmm4,   0,  4)  \
  X(YMM4F1,  529, zmm4,   4,  4)  \
  X(YMM4F2,  530, zmm4,   8,  4)  \
  X(YMM4F3,  531, zmm4,   12, 4)  \
  X(YMM4F4,  532, zmm4,   16, 4)  \
  X(YMM4F5,  533, zmm4,   20, 4)  \
  X(YMM4F6,  534, zmm4,   24, 4)  \
  X(YMM4F7,  535, zmm4,   28, 4)  \
  X(YMM5F0,  536, zmm5,   0,  4)  \
  X(YMM5F1,  537, zmm5,   4,  4)  \
  X(YMM5F2,  538, zmm5,   8,  4)  \
  X(YMM5F3,  539, zmm5,   12, 4)  \
  X(YMM5F4,  540, zmm5,   16, 4)  \
  X(YMM5F5,  541, zmm5,   20, 4)  \
  X(YMM5F6,  542, zmm5,   24, 4)  \
  X(YMM5F7,  543, zmm5,   28, 4)  \
  X(YMM6F0,  544, zmm6,   0,  4)  \
  X(YMM6F1,  545, zmm6,   4,  4)  \
  X(YMM6F2,  546, zmm6,   8,  4)  \
  X(YMM6F3,  547, zmm6,   12, 4)  \
  X(YMM6F4,  548, zmm6,   16, 4)  \
  X(YMM6F5,  549, zmm6,   20, 4)  \
  X(YMM6F6,  550, zmm6,   24, 4)  \
  X(YMM6F7,  551, zmm6,   28, 4)  \
  X(YMM7F0,  552, zmm7,   0,  4)  \
  X(YMM7F1,  553, zmm7,   4,  4)  \
  X(YMM7F2,  554, zmm7,   8,  4)  \
  X(YMM7F3,  555, zmm7,   12, 4)  \
  X(YMM7F4,  556, zmm7,   16, 4)  \
  X(YMM7F5,  557, zmm7,   20, 4)  \
  X(YMM7F6,  558, zmm7,   24, 4)  \
  X(YMM7F7,  559, zmm7,   28, 4)  \
  X(YMM8F0,  560, zmm8,   0,  4)  \
  X(YMM8F1,  561, zmm8,   4,  4)  \
  X(YMM8F2,  562, zmm8,   8,  4)  \
  X(YMM8F3,  563, zmm8,   12, 4)  \
  X(YMM8F4,  564, zmm8,   16, 4)  \
  X(YMM8F5,  565, zmm8,   20, 4)  \
  X(YMM8F6,  566, zmm8,   24, 4)  \
  X(YMM8F7,  567, zmm8,   28, 4)  \
  X(YMM9F0,  568, zmm9,   0,  4)  \
  X(YMM9F1,  569, zmm9,   4,  4)  \
  X(YMM9F2,  570, zmm9,   8,  4)  \
  X(YMM9F3,  571, zmm9,   12, 4)  \
  X(YMM9F4,  572, zmm9,   16, 4)  \
  X(YMM9F5,  573, zmm9,   20, 4)  \
  X(YMM9F6,  574, zmm9,   24, 4)  \
  X(YMM9F7,  575, zmm9,   28, 4)  \
  X(YMM10F0, 576, zmm10,  0,  4)  \
  X(YMM10F1, 577, zmm10,  4,  4)  \
  X(YMM10F2, 578, zmm10,  8,  4)  \
  X(YMM10F3, 579, zmm10,  12, 4)  \
  X(YMM10F4, 580, zmm10,  16, 4)  \
  X(YMM10F5, 581, zmm10,  20, 4)  \
  X(YMM10F6, 582, zmm10,  24, 4)  \
  X(YMM10F7, 583, zmm10,  28, 4)  \
  X(YMM11F0, 584, zmm11,  0,  4)  \
  X(YMM11F1, 585, zmm11,  4,  4)  \
  X(YMM11F2, 586, zmm11,  8,  4)  \
  X(YMM11F3, 587, zmm11,  12, 4)  \
  X(YMM11F4, 588, zmm11,  16, 4)  \
  X(YMM11F5, 589, zmm11,  20, 4)  \
  X(YMM11F6, 590, zmm11,  24, 4)  \
  X(YMM11F7, 591, zmm11,  28, 4)  \
  X(YMM12F0, 592, zmm12,  0,  4)  \
  X(YMM12F1, 593, zmm12,  4,  4)  \
  X(YMM12F2, 594, zmm12,  8,  4)  \
  X(YMM12F3, 595, zmm12,  12, 4)  \
  X(YMM12F4, 596, zmm12,  16, 4)  \
  X(YMM12F5, 597, zmm12,  20, 4)  \
  X(YMM12F6, 598, zmm12,  24, 4)  \
  X(YMM12F7, 599, zmm12,  28, 4)  \
  X(YMM13F0, 600, zmm13,  0,  4)  \
  X(YMM13F1, 601, zmm13,  4,  4)  \
  X(YMM13F2, 602, zmm13,  8,  4)  \
  X(YMM13F3, 603, zmm13,  12, 4)  \
  X(YMM13F4, 604, zmm13,  16, 4)  \
  X(YMM13F5, 605, zmm13,  20, 4)  \
  X(YMM13F6, 606, zmm13,  24, 4)  \
  X(YMM13F7, 607, zmm13,  28, 4)  \
  X(YMM14F0, 608, zmm14,  0,  4)  \
  X(YMM14F1, 609, zmm14,  4,  4)  \
  X(YMM14F2, 610, zmm14,  8,  4)  \
  X(YMM14F3, 611, zmm14,  12, 4)  \
  X(YMM14F4, 612, zmm14,  16, 4)  \
  X(YMM14F5, 613, zmm14,  20, 4)  \
  X(YMM14F6, 614, zmm14,  24, 4)  \
  X(YMM14F7, 615, zmm14,  28, 4)  \
  X(YMM15F0, 616, zmm15,  0,  4)  \
  X(YMM15F1, 617, zmm15,  4,  4)  \
  X(YMM15F2, 618, zmm15,  8,  4)  \
  X(YMM15F3, 619, zmm15,  12, 4)  \
  X(YMM15F4, 620, zmm15,  16, 4)  \
  X(YMM15F5, 621, zmm15,  20, 4)  \
  X(YMM15F6, 622, zmm15,  24, 4)  \
  X(YMM15F7, 623, zmm15,  28, 4)  \
  X(YMM0D0,  624, zmm0,   0,  8)  \
  X(YMM0D1,  625, zmm0,   8,  8)  \
  X(YMM0D2,  626, zmm0,   16, 8)  \
  X(YMM0D3,  627, zmm0,   24, 8)  \
  X(YMM1D0,  628, zmm1,   0,  8)  \
  X(YMM1D1,  629, zmm1,   8,  8)  \
  X(YMM1D2,  630, zmm1,   16, 8)  \
  X(YMM1D3,  631, zmm1,   24, 8)  \
  X(YMM2D0,  632, zmm2,   0,  8)  \
  X(YMM2D1,  633, zmm2,   8,  8)  \
  X(YMM2D2,  634, zmm2,   16, 8)  \
  X(YMM2D3,  635, zmm2,   24, 8)  \
  X(YMM3D0,  636, zmm3,   0,  8)  \
  X(YMM3D1,  637, zmm3,   8,  8)  \
  X(YMM3D2,  638, zmm3,   16, 8)  \
  X(YMM3D3,  639, zmm3,   24, 8)  \
  X(YMM4D0,  640, zmm4,   0,  8)  \
  X(YMM4D1,  641, zmm4,   8,  8)  \
  X(YMM4D2,  642, zmm4,   16, 8)  \
  X(YMM4D3,  643, zmm4,   24, 8)  \
  X(YMM5D0,  644, zmm5,   0,  8)  \
  X(YMM5D1,  645, zmm5,   8,  8)  \
  X(YMM5D2,  646, zmm5,   16, 8)  \
  X(YMM5D3,  647, zmm5,   24, 8)  \
  X(YMM6D0,  648, zmm6,   0,  8)  \
  X(YMM6D1,  649, zmm6,   8,  8)  \
  X(YMM6D2,  650, zmm6,   16, 8)  \
  X(YMM6D3,  651, zmm6,   24, 8)  \
  X(YMM7D0,  652, zmm7,   0,  8)  \
  X(YMM7D1,  653, zmm7,   8,  8)  \
  X(YMM7D2,  654, zmm7,   16, 8)  \
  X(YMM7D3,  655, zmm7,   24, 8)  \
  X(YMM8D0,  656, zmm8,   0,  8)  \
  X(YMM8D1,  657, zmm8,   8,  8)  \
  X(YMM8D2,  658, zmm8,   16, 8)  \
  X(YMM8D3,  659, zmm8,   24, 8)  \
  X(YMM9D0,  660, zmm9,   0,  8)  \
  X(YMM9D1,  661, zmm9,   8,  8)  \
  X(YMM9D2,  662, zmm9,   16, 8)  \
  X(YMM9D3,  663, zmm9,   24, 8)  \
  X(YMM10D0, 664, zmm10,  0,  8)  \
  X(YMM10D1, 665, zmm10,  8,  8)  \
  X(YMM10D2, 666, zmm10,  16, 8)  \
  X(YMM10D3, 667, zmm10,  24, 8)  \
  X(YMM11D0, 668, zmm11,  0,  8)  \
  X(YMM11D1, 669, zmm11,  8,  8)  \
  X(YMM11D2, 670, zmm11,  16, 8)  \
  X(YMM11D3, 671, zmm11,  24, 8)  \
  X(YMM12D0, 672, zmm12,  0,  8)  \
  X(YMM12D1, 673, zmm12,  8,  8)  \
  X(YMM12D2, 674, zmm12,  16, 8)  \
  X(YMM12D3, 675, zmm12,  24, 8)  \
  X(YMM13D0, 676, zmm13,  0,  8)  \
  X(YMM13D1, 677, zmm13,  8,  8)  \
  X(YMM13D2, 678, zmm13,  16, 8)  \
  X(YMM13D3, 679, zmm13,  24, 8)  \
  X(YMM14D0, 680, zmm14,  0,  8)  \
  X(YMM14D1, 681, zmm14,  8,  8)  \
  X(YMM14D2, 682, zmm14,  16, 8)  \
  X(YMM14D3, 683, zmm14,  24, 8)  \
  X(YMM15D0, 684, zmm15,  0,  8)  \
  X(YMM15D1, 685, zmm15,  8,  8)  \
  X(YMM15D2, 686, zmm15,  16, 8)  \
  X(YMM15D3, 687, zmm15,  24, 8)  \
  X(XMM16,   694, zmm16,  0,  16) \
  X(XMM17,   695, zmm17,  0,  16) \
  X(XMM18,   696, zmm18,  0,  16) \
  X(XMM19,   697, zmm19,  0,  16) \
  X(XMM20,   698, zmm20,  0,  16) \
  X(XMM21,   699, zmm21,  0,  16) \
  X(XMM22,   700, zmm22,  0,  16) \
  X(XMM23,   701, zmm23,  0,  16) \
  X(XMM24,   702, zmm24,  0,  16) \
  X(XMM25,   703, zmm25,  0,  16) \
  X(XMM26,   704, zmm26,  0,  16) \
  X(XMM27,   705, zmm27,  0,  16) \
  X(XMM28,   706, zmm28,  0,  16) \
  X(XMM29,   707, zmm29,  0,  16) \
  X(XMM30,   708, zmm30,  0,  16) \
  X(XMM31,   709, zmm31,  0,  16) \
  X(YMM16,   710, zmm16,  0,  32) \
  X(YMM17,   711, zmm17,  0,  32) \
  X(YMM18,   712, zmm18,  0,  32) \
  X(YMM19,   713, zmm19,  0,  32) \
  X(YMM20,   714, zmm20,  0,  32) \
  X(YMM21,   715, zmm21,  0,  32) \
  X(YMM22,   716, zmm22,  0,  32) \
  X(YMM23,   717, zmm23,  0,  32) \
  X(YMM24,   718, zmm24,  0,  32) \
  X(YMM25,   719, zmm25,  0,  32) \
  X(YMM26,   720, zmm26,  0,  32) \
  X(YMM27,   721, zmm27,  0,  32) \
  X(YMM28,   722, zmm28,  0,  32) \
  X(YMM29,   723, zmm29,  0,  32) \
  X(YMM30,   724, zmm30,  0,  32) \
  X(YMM31,   725, zmm31,  0,  32) \
  X(ZMM0,    726, zmm0,   0,  64) \
  X(ZMM1,    727, zmm1,   0,  64) \
  X(ZMM2,    728, zmm2,   0,  64) \
  X(ZMM3,    729, zmm3,   0,  64) \
  X(ZMM4,    730, zmm4,   0,  64) \
  X(ZMM5,    731, zmm5,   0,  64) \
  X(ZMM6,    732, zmm6,   0,  64) \
  X(ZMM7,    733, zmm7,   0,  64) \
  X(ZMM8,    734, zmm8,   0,  64) \
  X(ZMM9,    735, zmm9,   0,  64) \
  X(ZMM10,   736, zmm10,  0,  64) \
  X(ZMM11,   737, zmm11,  0,  64) \
  X(ZMM12,   738, zmm12,  0,  64) \
  X(ZMM13,   739, zmm13,  0,  64) \
  X(ZMM14,   740, zmm14,  0,  64) \
  X(ZMM15,   741, zmm15,  0,  64) \
  X(ZMM16,   742, zmm16,  0,  64) \
  X(ZMM17,   743, zmm17,  0,  64) \
  X(ZMM18,   744, zmm18,  0,  64) \
  X(ZMM19,   745, zmm19,  0,  64) \
  X(ZMM20,   746, zmm20,  0,  64) \
  X(ZMM21,   747, zmm21,  0,  64) \
  X(ZMM22,   748, zmm22,  0,  64) \
  X(ZMM23,   749, zmm23,  0,  64) \
  X(ZMM24,   750, zmm24,  0,  64) \
  X(ZMM25,   751, zmm25,  0,  64) \
  X(ZMM26,   752, zmm26,  0,  64) \
  X(ZMM27,   753, zmm27,  0,  64) \
  X(ZMM28,   754, zmm28,  0,  64) \
  X(ZMM29,   755, zmm29,  0,  64) \
  X(ZMM30,   756, zmm30,  0,  64) \
  X(ZMM31,   757, zmm31,  0,  64) \
  X(K0,      758, k0,     0,  8)  \
  X(K1,      759, k1,     0,  8)  \
  X(K2,      760, k2,     0,  8)  \
  X(K3,      761, k3,     0,  8)  \
  X(K4,      762, k4,     0,  8)  \
  X(K5,      763, k5,     0,  8)  \
  X(K6,      764, k6,     0,  8)  \
  X(K7,      765, k7,     0,  8)  \
  X(ZMM0H,   766, zmm0,   32, 32) \
  X(ZMM1H,   767, zmm1,   32, 32) \
  X(ZMM2H,   768, zmm2,   32, 32) \
  X(ZMM3H,   769, zmm3,   32, 32) \
  X(ZMM4H,   770, zmm4,   32, 32) \
  X(ZMM5H,   771, zmm5,   32, 32) \
  X(ZMM6H,   772, zmm6,   32, 32) \
  X(ZMM7H,   773, zmm7,   32, 32) \
  X(ZMM8H,   774, zmm8,   32, 32) \
  X(ZMM9H,   775, zmm9,   32, 32) \
  X(ZMM10H,  776, zmm10,  32, 32) \
  X(ZMM11H,  777, zmm11,  32, 32) \
  X(ZMM12H,  778, zmm12,  32, 32) \
  X(ZMM13H,  779, zmm13,  32, 32) \
  X(ZMM14H,  780, zmm14,  32, 32) \
  X(ZMM15H,  781, zmm15,  32, 32)

typedef U16 CV_Regx64;
typedef enum CV_Regx64Enum
{
#define X(CVN,C,RDN,BP,BZ) CV_Regx64_##CVN = C,
  CV_Reg_X64_XList(X)
#undef X
}
CV_Regx64Enum;

// X(NAME, CODE, (RDI_RegisterCode_ARM64) NAME, BYTE_POS, BYTE_SIZE)
#define CV_Reg_ARM64_XList(X) \
X(NONE,  0,   nil,  0, 0)\
X(W0,    10,  x0,   0, 4)\
X(W1,    11,  x1,   0, 4)\
X(W2,    12,  x2,   0, 4)\
X(W3,    13,  x3,   0, 4)\
X(W4,    14,  x4,   0, 4)\
X(W5,    15,  x5,   0, 4)\
X(W6,    16,  x6,   0, 4)\
X(W7,    17,  x7,   0, 4)\
X(W8,    18,  x8,   0, 4)\
X(W9,    19,  x9,   0, 4)\
X(W10,   20,  x10,  0, 4)\
X(W11,   21,  x11,  0, 4)\
X(W12,   22,  x12,  0, 4)\
X(W13,   23,  x13,  0, 4)\
X(W14,   24,  x14,  0, 4)\
X(W15,   25,  x15,  0, 4)\
X(W16,   26,  x16,  0, 4)\
X(W17,   27,  x17,  0, 4)\
X(W18,   28,  x18,  0, 4)\
X(W19,   29,  x19,  0, 4)\
X(W20,   30,  x20,  0, 4)\
X(W21,   31,  x21,  0, 4)\
X(W22,   32,  x22,  0, 4)\
X(W23,   33,  x23,  0, 4)\
X(W24,   34,  x24,  0, 4)\
X(W25,   35,  x25,  0, 4)\
X(W26,   36,  x26,  0, 4)\
X(W27,   37,  x27,  0, 4)\
X(W28,   38,  x28,  0, 4)\
X(W29,   39,  x29,  0, 4)\
X(W30,   40,  x30,  0, 4)\
X(WZR,   41,  x31,  0, 4)\
X(X0,    50,  x0,   0, 8)\
X(X1,    51,  x1,   0, 8)\
X(X2,    52,  x2,   0, 8)\
X(X3,    53,  x3,   0, 8)\
X(X4,    54,  x4,   0, 8)\
X(X5,    55,  x5,   0, 8)\
X(X6,    56,  x6,   0, 8)\
X(X7,    57,  x7,   0, 8)\
X(X8,    58,  x8,   0, 8)\
X(X9,    59,  x9,   0, 8)\
X(X10,   60,  x10,  0, 8)\
X(X11,   61,  x11,  0, 8)\
X(X12,   62,  x12,  0, 8)\
X(X13,   63,  x13,  0, 8)\
X(X14,   64,  x14,  0, 8)\
X(X15,   65,  x15,  0, 8)\
X(IP0,   66,  x16,  0, 8)\
X(IP1,   67,  x17,  0, 8)\
X(X18,   68,  x18,  0, 8)\
X(X19,   69,  x19,  0, 8)\
X(X20,   70,  x20,  0, 8)\
X(X21,   71,  x21,  0, 8)\
X(X22,   72,  x22,  0, 8)\
X(X23,   73,  x23,  0, 8)\
X(X24,   74,  x24,  0, 8)\
X(X25,   75,  x25,  0, 8)\
X(X26,   76,  x26,  0, 8)\
X(X27,   77,  x27,  0, 8)\
X(X28,   78,  x28,  0, 8)\
X(FP,    79,  x29,  0, 8)\
X(LR,    80,  x30,  0, 8)\
X(SP,    81,  x31,  0, 8)\
X(ZR,    82,  x31,  0, 8)\
X(NZCV,  90,  context_flags,  0, 4)\
X(S0,    100, v0,   0, 4)\
X(S1,    101, v1,   0, 4)\
X(S2,    102, v2,   0, 4)\
X(S3,    103, v3,   0, 4)\
X(S4,    104, v4,   0, 4)\
X(S5,    105, v5,   0, 4)\
X(S6,    106, v6,   0, 4)\
X(S7,    107, v7,   0, 4)\
X(S8,    108, v8,   0, 4)\
X(S9,    109, v9,   0, 4)\
X(S10,   110, v10,  0, 4)\
X(S11,   111, v11,  0, 4)\
X(S12,   112, v12,  0, 4)\
X(S13,   113, v13,  0, 4)\
X(S14,   114, v14,  0, 4)\
X(S15,   115, v15,  0, 4)\
X(S16,   116, v16,  0, 4)\
X(S17,   117, v17,  0, 4)\
X(S18,   118, v18,  0, 4)\
X(S19,   119, v19,  0, 4)\
X(S20,   120, v20,  0, 4)\
X(S21,   121, v21,  0, 4)\
X(S22,   122, v22,  0, 4)\
X(S23,   123, v23,  0, 4)\
X(S24,   124, v24,  0, 4)\
X(S25,   125, v25,  0, 4)\
X(S26,   126, v26,  0, 4)\
X(S27,   127, v27,  0, 4)\
X(S28,   128, v28,  0, 4)\
X(S29,   129, v29,  0, 4)\
X(S30,   130, v30,  0, 4)\
X(S31,   131, v31,  0, 4)\
X(D0,    140, v0,   0, 8)\
X(D1,    141, v1,   0, 8)\
X(D2,    142, v2,   0, 8)\
X(D3,    143, v3,   0, 8)\
X(D4,    144, v4,   0, 8)\
X(D5,    145, v5,   0, 8)\
X(D6,    146, v6,   0, 8)\
X(D7,    147, v7,   0, 8)\
X(D8,    148, v8,   0, 8)\
X(D9,    149, v9,   0, 8)\
X(D10,   150, v10,  0, 8)\
X(D11,   151, v11,  0, 8)\
X(D12,   152, v12,  0, 8)\
X(D13,   153, v13,  0, 8)\
X(D14,   154, v14,  0, 8)\
X(D15,   155, v15,  0, 8)\
X(D16,   156, v16,  0, 8)\
X(D17,   157, v17,  0, 8)\
X(D18,   158, v18,  0, 8)\
X(D19,   159, v19,  0, 8)\
X(D20,   160, v20,  0, 8)\
X(D21,   161, v21,  0, 8)\
X(D22,   162, v22,  0, 8)\
X(D23,   163, v23,  0, 8)\
X(D24,   164, v24,  0, 8)\
X(D25,   165, v25,  0, 8)\
X(D26,   166, v26,  0, 8)\
X(D27,   167, v27,  0, 8)\
X(D28,   168, v28,  0, 8)\
X(D29,   169, v29,  0, 8)\
X(D30,   170, v30,  0, 8)\
X(D31,   171, v31,  0, 8)\
X(Q0,    180, v0,   0, 16)\
X(Q1,    181, v1,   0, 16)\
X(Q2,    182, v2,   0, 16)\
X(Q3,    183, v3,   0, 16)\
X(Q4,    184, v4,   0, 16)\
X(Q5,    185, v5,   0, 16)\
X(Q6,    186, v6,   0, 16)\
X(Q7,    187, v7,   0, 16)\
X(Q8,    188, v8,   0, 16)\
X(Q9,    189, v9,   0, 16)\
X(Q10,   190, v10,  0, 16)\
X(Q11,   191, v11,  0, 16)\
X(Q12,   192, v12,  0, 16)\
X(Q13,   193, v13,  0, 16)\
X(Q14,   194, v14,  0, 16)\
X(Q15,   195, v15,  0, 16)\
X(Q16,   196, v16,  0, 16)\
X(Q17,   197, v17,  0, 16)\
X(Q18,   198, v18,  0, 16)\
X(Q19,   199, v19,  0, 16)\
X(Q20,   200, v20,  0, 16)\
X(Q21,   201, v21,  0, 16)\
X(Q22,   202, v22,  0, 16)\
X(Q23,   203, v23,  0, 16)\
X(Q24,   204, v24,  0, 16)\
X(Q25,   205, v25,  0, 16)\
X(Q26,   206, v26,  0, 16)\
X(Q27,   207, v27,  0, 16)\
X(Q28,   208, v28,  0, 16)\
X(Q29,   209, v29,  0, 16)\
X(Q30,   210, v30,  0, 16)\
X(Q31,   211, v31,  0, 16)\
X(FPSR,  220, fpsr, 0, 4)

typedef U16 CV_Regarm64;
typedef enum CV_Regarm64Enum
{
#define X(CVN,C,RDN,BP,BZ) CV_Regarm64_##CVN = C,
  CV_Reg_ARM64_XList(X)
#undef X
}
CV_Regarm64Enum;

#define CV_SignatureXList(X) \
  X(C6,       0)             \
  X(C7,       1)             \
  X(C11,      2)             \
  X(C13,      4)             \
  X(RESERVED, 5)

typedef U32 CV_Signature;
typedef enum CV_SignatureEnum
{
#define X(N,c) CV_Signature_##N = c,
  CV_SignatureXList(X)
#undef X
}
CV_SignatureEnum;


#define CV_LanguageXList(X) \
  X(C,       0x00)          \
  X(CXX,     0x01)          \
  X(FORTRAN, 0x02)          \
  X(MASM,    0x03)          \
  X(PASCAL,  0x04)          \
  X(BASIC,   0x05)          \
  X(COBOL,   0x06)          \
  X(LINK,    0x07)          \
  X(CVTRES,  0x08)          \
  X(CVTPGD,  0x09)          \
  X(CSHARP,  0x0A)          \
  X(VB,      0x0B)          \
  X(ILASM,   0x0C)          \
  X(JAVA,    0x0D)          \
  X(JSCRIPT, 0x0E)          \
  X(MSIL,    0x0F)          \
  X(HLSL,    0x10)

typedef U16 CV_Language;
typedef enum CV_LanguageEnum
{
#define X(N,c) CV_Language_##N = c,
  CV_LanguageXList(X)
#undef X
}
CV_LanguageEnum;

#pragma pack(push, 1)

////////////////////////////////
//~ rjf: CodeView Format "Sym" and "Leaf" Header Type

#define CV_LeafSize_Max max_U16
typedef U16 CV_LeafSize;

#define CV_SymSize_Max max_U16
typedef U16 CV_SymSize;

typedef struct CV_RecHeader CV_RecHeader;
struct CV_RecHeader
{
  U16 size;
  U16 kind;
};

////////////////////////////////
//~ rjf: CodeView Format "Sym" Types
//
// (per-compilation-unit info, variables, procedures, etc.)
//

typedef U8 CV_ProcFlags;
enum
{
  CV_ProcFlag_NoFPO       = (1 << 0),
  CV_ProcFlag_IntReturn   = (1 << 1),
  CV_ProcFlag_FarReturn   = (1 << 2),
  CV_ProcFlag_NeverReturn = (1 << 3),
  CV_ProcFlag_NotReached  = (1 << 4),
  CV_ProcFlag_CustomCall  = (1 << 5),
  CV_ProcFlag_NoInline    = (1 << 6),
  CV_ProcFlag_OptDbgInfo  = (1 << 7),
};

typedef U16 CV_LocalFlags;
enum
{
  CV_LocalFlag_Param           = (1 << 0),
  CV_LocalFlag_AddrTaken       = (1 << 1),
  CV_LocalFlag_Compgen         = (1 << 2),
  CV_LocalFlag_Aggregate       = (1 << 3),
  CV_LocalFlag_PartOfAggregate = (1 << 4),
  CV_LocalFlag_Aliased         = (1 << 5),
  CV_LocalFlag_Alias           = (1 << 6),
  CV_LocalFlag_Retval          = (1 << 7),
  CV_LocalFlag_OptOut          = (1 << 8),
  CV_LocalFlag_Global          = (1 << 9),
  CV_LocalFlag_Static          = (1 << 10),
};

typedef struct CV_LocalVarAttr CV_LocalVarAttr;
struct CV_LocalVarAttr
{
  U32           off;
  U16           seg;
  CV_LocalFlags flags;
};

//- (SymKind: COMPILE)

typedef U32 CV_CompileFlags;
#define CV_CompileFlags_Extract_Language(f)    (((f)    )&0xFF)
#define CV_CompileFlags_Extract_FloatPrec(f)   (((f)>> 8)&0x03)
#define CV_CompileFlags_Extract_FloatPkg(f)    (((f)>>10)&0x03)
#define CV_CompileFlags_Extract_AmbientData(f) (((f)>>12)&0x07)
#define CV_CompileFlags_Extract_AmbientCode(f) (((f)>>15)&0x07)
#define CV_CompileFlags_Extract_Mode(f)        (((f)>>18)&0x01)

typedef struct CV_SymCompile CV_SymCompile;
struct CV_SymCompile
{
  U8              machine;
  CV_CompileFlags flags;
  // U8[] ver_str (null terminated)
};

//- (SymKind: SSEARCH)

typedef struct CV_SymStartSearch CV_SymStartSearch;
struct CV_SymStartSearch
{
  U32 start_symbol;
  U16 segment;
};

//- (SymKind: END) (empty)

//- (SymKind: RETURN)

typedef U8 CV_GenericStyle;
typedef enum CV_GenericStyleEnum
{
  CV_GenericStyle_VOID,
  CV_GenericStyle_REG,    //  "return data is in register"
  CV_GenericStyle_ICAN,   //  "indirect caller allocated near"
  CV_GenericStyle_ICAF,   //  "indirect caller allocated far"
  CV_GenericStyle_IRAN,   //  "indirect returnee allocated near"
  CV_GenericStyle_IRAF,   //  "indirect returnee allocated far"
  CV_GenericStyle_UNUSED,
}
CV_GenericStyleEnum;

typedef U16 CV_GenericFlags;
enum
{
  CV_GenericFlags_CSTYLE  = (1 << 0),
  CV_GenericFlags_RSCLEAN = (1 << 1), //  "returnee stack cleanup"
};

typedef struct CV_SymReturn CV_SymReturn;
struct CV_SymReturn
{
  CV_GenericFlags flags;
  CV_GenericStyle style;
};

//- (SymKind: SLINK32)

typedef struct CV_SymSLink32 CV_SymSLink32;
struct CV_SymSLink32
{
  U32 frame_size;
  U32 offset;
  U16 reg;
};

//- (SymKind: OEM)

typedef struct CV_SymOEM CV_SymOEM;
struct CV_SymOEM
{
  Guid      id;
  CV_TypeId itype;
  //  padding align(4)
};

//- (SymKind: VFTABLE32)

typedef struct CV_SymVPath32 CV_SymVPath32;
struct CV_SymVPath32
{
  CV_TypeId root;
  CV_TypeId path;
  U32       off;
  U16       seg;
};

//- (SymKind: FRAMEPROC)

typedef U8 CV_EncodedFramePtrReg;
typedef enum CV_EncodedFramePtrRegEnum
{
  CV_EncodedFramePtrReg_None,
  CV_EncodedFramePtrReg_StackPtr,
  CV_EncodedFramePtrReg_FramePtr,
  CV_EncodedFramePtrReg_BasePtr,
}
CV_EncodedFramePtrRegEnum;

typedef U32 CV_FrameprocFlags;
enum
{
  CV_FrameprocFlag_UsesAlloca        = (1 << 0),
  CV_FrameprocFlag_UsesSetJmp        = (1 << 1),
  CV_FrameprocFlag_UsesLongJmp       = (1 << 2),
  CV_FrameprocFlag_UsesInlAsm        = (1 << 3),
  CV_FrameprocFlag_UsesEH            = (1 << 4),
  CV_FrameprocFlag_Inline            = (1 << 5),
  CV_FrameprocFlag_HasSEH            = (1 << 6),
  CV_FrameprocFlag_Naked             = (1 << 7),
  CV_FrameprocFlag_HasSecurityChecks = (1 << 8),
  CV_FrameprocFlag_AsyncEH           = (1 << 9),
  CV_FrameprocFlag_GSNoStackOrdering = (1 << 10),
  CV_FrameprocFlag_WasInlined        = (1 << 11),
  CV_FrameprocFlag_GSCheck           = (1 << 12),
  CV_FrameprocFlag_SafeBuffers       = (1 << 13),
  // LocalBasePointer: 14,15
  // ParamBasePointer: 16,17
  CV_FrameprocFlag_PogoOn            = (1 << 18),
  CV_FrameprocFlag_PogoCountsValid   = (1 << 19),
  CV_FrameprocFlag_OptSpeed          = (1 << 20),
  CV_FrameprocFlag_HasCFG            = (1 << 21),
  CV_FrameprocFlag_HasCFW            = (1 << 22),
};
#define CV_FrameprocFlags_Extract_LocalBasePointer(f) (((f) >> 14)&3)
#define CV_FrameprocFlags_Extract_ParamBasePointer(f) (((f) >> 16)&3)

typedef struct CV_SymFrameproc CV_SymFrameproc;
struct CV_SymFrameproc
{
  U32               frame_size;
  U32               pad_size;
  U32               pad_off;
  U32               save_reg_size;
  U32               eh_off;
  CV_SectionIndex   eh_sec;
  CV_FrameprocFlags flags;
};

//- (SymKind: ANNOTATION)

typedef struct CV_SymAnnotation CV_SymAnnotation;
struct CV_SymAnnotation
{
  U32 off;
  U16 seg;
  U16 count;
  // U8[] annotation (null terminated)
};

//- (SymKind: OBJNAME)

typedef struct CV_SymObjName CV_SymObjName;
struct CV_SymObjName
{
  U32 sig;
  // U8[] name (null terminated)
};

//- (SymKind: THUNK32)

typedef U8 CV_ThunkOrdinal;
typedef enum CV_ThunkOrdinalEnum
{
  CV_ThunkOrdinal_NoType,
  CV_ThunkOrdinal_Adjustor,
  CV_ThunkOrdinal_VCall,
  CV_ThunkOrdinal_PCode,
  CV_ThunkOrdinal_Load,
  CV_ThunkOrdinal_TrampIncremental,
  CV_ThunkOrdinal_TrampBranchIsland,
}
CV_ThunkOrdinalEnum;

typedef struct CV_SymThunk32 CV_SymThunk32;
struct CV_SymThunk32
{
  U32             parent;
  U32             end;
  U32             next;
  U32             off;
  U16             sec;
  U16             len;
  CV_ThunkOrdinal ord;
  // U8[] name (null terminated)
  // U8[] variant (null terminated)
};

//- (SymKind: BLOCK32)

typedef struct CV_SymBlock32 CV_SymBlock32;
struct CV_SymBlock32
{
  U32 parent;
  U32 end;
  U32 len;
  U32 off;
  U16 sec;
  // U8[] name (null terminated)
};

//- (SymKind: LABEL32)

typedef struct CV_SymLabel32 CV_SymLabel32;
struct CV_SymLabel32
{
  U32          off;
  U16          sec;
  CV_ProcFlags flags;
  // U8[] name (null terminated)
};

//- (SymKind: REGISTER)

typedef struct CV_SymRegister CV_SymRegister;
struct CV_SymRegister
{
  CV_TypeId itype;
  U16       reg;
  // U8[] name (null terminated)
};

//- (SymKind: CONSTANT)

typedef struct CV_SymConstant CV_SymConstant;
struct CV_SymConstant
{
  CV_TypeId itype;
  // CV_Numeric num
  // U8[] name (null terminated)
};

//- (SymKind: UDT)

typedef struct CV_SymUDT CV_SymUDT;
struct CV_SymUDT
{
  CV_TypeId itype;
  // U8[] name (null terminated)
};

//- (SymKind: MANYREG)

typedef struct CV_SymManyreg CV_SymManyreg;
struct CV_SymManyreg
{
  CV_TypeId itype;
  U8        count;
  // U8[count] regs;
};

//- (SymKind: BPREL32)

typedef struct CV_SymBPRel32 CV_SymBPRel32;
struct CV_SymBPRel32
{
  U32       off;
  CV_TypeId itype;
  // U8[] name (null terminated)
};

//- (SymKind: LDATA32, GDATA32)

typedef struct CV_SymData32 CV_SymData32;
struct CV_SymData32
{
  CV_TypeId       itype;
  U32             off;
  CV_SectionIndex sec;
  // U8[] name (null terminated)
};

//- (SymKind: PUB32)

typedef U32 CV_Pub32Flags;
enum
{
  CV_Pub32Flag_Code        = (1 << 0),
  CV_Pub32Flag_Function    = (1 << 1),
  CV_Pub32Flag_ManagedCode = (1 << 2),
  CV_Pub32Flag_MSIL        = (1 << 3),
};

typedef struct CV_SymPub32 CV_SymPub32;
struct CV_SymPub32
{
  CV_Pub32Flags   flags;
  U32             off;
  CV_SectionIndex sec;
  // U8[] name (null terminated)
};

//- (SymKind: LPROC32, GPROC32)

typedef struct CV_SymProc32 CV_SymProc32;
struct CV_SymProc32
{
  U32          parent;
  U32          end;
  U32          next;
  U32          len;
  U32          dbg_start;
  U32          dbg_end;
  CV_TypeId    itype;
  U32          off;
  U16          sec;
  CV_ProcFlags flags;
  // U8[] name (null terminated)
};

//- (SymKind: REGREL32)

typedef struct CV_SymRegrel32 CV_SymRegrel32;
struct CV_SymRegrel32
{
  U32       reg_off;
  CV_TypeId itype;
  CV_Reg    reg;
  // U8[] name (null terminated)
};

//- (SymKind: LTHREAD32, GTHREAD32)

typedef struct CV_SymThread32 CV_SymThread32;
struct CV_SymThread32
{
  CV_TypeId itype;
  U32       tls_off;
  U16       tls_seg;
  // U8[] name (null terminated)
};

//- (SymKind: COMPILE2)

typedef U32 CV_Compile2Flags;
#define CV_Compile2Flags_Extract_Language(f)        (((f)    )&0xFF)
#define CV_Compile2Flags_Extract_EditAndContinue(f) (((f)>> 8)&0x01)
#define CV_Compile2Flags_Extract_NoDbgInfo(f)       (((f)>> 9)&0x01)
#define CV_Compile2Flags_Extract_LTCG(f)            (((f)>>10)&0x01)
#define CV_Compile2Flags_Extract_NoDataAlign(f)     (((f)>>11)&0x01)
#define CV_Compile2Flags_Extract_ManagedPresent(f)  (((f)>>12)&0x01)
#define CV_Compile2Flags_Extract_SecurityChecks(f)  (((f)>>13)&0x01)
#define CV_Compile2Flags_Extract_HotPatch(f)        (((f)>>14)&0x01)
#define CV_Compile2Flags_Extract_CVTCIL(f)          (((f)>>15)&0x01)
#define CV_Compile2Flags_Extract_MSILModule(f)      (((f)>>16)&0x01)

typedef struct CV_SymCompile2 CV_SymCompile2;
struct CV_SymCompile2
{
  CV_Compile2Flags flags;
  CV_Arch          machine;
  U16              ver_fe_major;
  U16              ver_fe_minor;
  U16              ver_fe_build;
  U16              ver_major;
  U16              ver_minor;
  U16              ver_build;
  // U8[] ver_str (null terminated)
};

//- (SymKind: MANYREG2)

typedef struct CV_SymManyreg2 CV_SymManyreg2;
struct CV_SymManyreg2
{
  CV_TypeId itype;
  U16       count;
  // U16[count] regs;
};

//- (SymKind: LOCALSLOT)

typedef struct CV_SymSlot CV_SymSlot;
struct CV_SymSlot
{
  U32       slot_index;
  CV_TypeId itype;
  // U8[] name (null terminated)
};

//- (SymKind: MANFRAMEREL, ATTR_FRAMEREL)

typedef struct CV_SymAttrFrameRel CV_SymAttrFrameRel;
struct CV_SymAttrFrameRel
{
  U32             off;
  CV_TypeId       itype;
  CV_LocalVarAttr attr;
  // U8[] name (null terminated)
};

//- (SymKind: MANREGISTER, ATTR_REGISTER)

typedef struct CV_SymAttrReg CV_SymAttrReg;
struct CV_SymAttrReg
{
  CV_TypeId       itype;
  CV_LocalVarAttr attr;
  U16             reg;
  // U8[] name (null terminated)
};

//- (SymKind: MANMANYREG, ATTR_MANYREG)


typedef struct CV_SymAttrManyReg CV_SymAttrManyReg;
struct CV_SymAttrManyReg
{
  CV_TypeId       itype;
  CV_LocalVarAttr attr;
  U8              count;
  // U8[count] regs
  // U8[] name (null terminated)
};

//- (SymKind: MANREGREL, ATTR_REGREL)

typedef struct CV_SymAttrRegRel CV_SymAttrRegRel;
struct CV_SymAttrRegRel
{
  U32             off;
  CV_TypeId       itype;
  U16             reg;
  CV_LocalVarAttr attr;
  // U8[] name (null terminated)
};

//- (SymKind: UNAMESPACE)

typedef struct CV_SymUNamespace CV_SymUNamespace;
struct CV_SymUNamespace
{
  // *** "dummy" is the first character of name - it should not be skipped!
  // *** It is placed here so the C compiler will accept this struct.
  // *** The actual fixed size part of this record has a size of zero.
  
  U8 dummy;
  
  // U8[] name (null terminated)
};

//- (SymKind: PROCREF, DATAREF, LPROCREF)

typedef struct CV_SymRef2 CV_SymRef2;
struct CV_SymRef2
{
  U32         suc_name;
  U32         sym_off;
  CV_ModIndex imod;
  // U8[] name (null terminated)
};

//- (SymKind: TRAMPOLINE)

typedef U16 CV_TrampolineKind;
typedef enum CV_TrampolineKindEnum
{
  CV_TrampolineKind_Incremental,
  CV_TrampolineKind_BranchIsland,
}
CV_TrampolineKindEnum;

typedef struct CV_SymTrampoline CV_SymTrampoline;
struct CV_SymTrampoline
{
  CV_TrampolineKind kind;
  U16               thunk_size;
  U32               thunk_sec_off;
  U32               target_sec_off;
  CV_SectionIndex   thunk_sec;
  CV_SectionIndex   target_sec;
};

//- (SymKind: SEPCODE)

typedef U32 CV_SepcodeFlags;
enum
{
  CV_SepcodeFlag_IsLexicalScope  = (1 << 0),
  CV_SepcodeFlag_ReturnsToParent = (1 << 1),
};

typedef struct CV_SymSepcode CV_SymSepcode;
struct CV_SymSepcode
{
  U32             parent;
  U32             end;
  U32             len;
  CV_SepcodeFlags flags;
  U32             sec_off;
  U32             sec_parent_off;
  U16             sec;
  U16             sec_parent;
};

//- (SymKind: SECTION)

typedef struct CV_SymSection CV_SymSection;
struct CV_SymSection
{
  U16 sec_index;
  U8  align;
  U8  pad;
  U32 rva;
  U32 size;
  U32 characteristics;
  // U8[] name (null terminated)
};

//- (SymKind: COFFGROUP)

typedef struct CV_SymCoffGroup CV_SymCoffGroup;
struct CV_SymCoffGroup
{
  U32 size;
  U32 characteristics;
  U32 off;
  U16 sec;
  // U8[] name (null terminated)
};

//- (SymKind: EXPORT)

typedef U16 CV_ExportFlags;
enum
{
  CV_ExportFlag_Constant  = (1 << 0),
  CV_ExportFlag_Data      = (1 << 1),
  CV_ExportFlag_Private   = (1 << 2),
  CV_ExportFlag_NoName    = (1 << 3),
  CV_ExportFlag_Ordinal   = (1 << 4),
  CV_ExportFlag_Forwarder = (1 << 5),
};

typedef struct CV_SymExport CV_SymExport;
struct CV_SymExport
{
  U16            ordinal;
  CV_ExportFlags flags;
  // U8[] name (null terminated)
};

//- (SymKind: CALLSITEINFO)

typedef struct CV_SymCallSiteInfo CV_SymCallSiteInfo;
struct CV_SymCallSiteInfo
{
  U32       off;
  U16       sec;
  U16       pad;
  CV_TypeId itype;
};

//- (SymKind: FRAMECOOKIE)

typedef U8 CV_FrameCookieKind;
typedef enum CV_FrameCookieKindEnum
{
  CV_FrameCookieKind_Copy,
  CV_FrameCookieKind_XorSP,
  CV_FrameCookieKind_XorBP,
  CV_FrameCookieKind_XorR13,
}
CV_FrameCookieKindEnum;

typedef struct CV_SymFrameCookie CV_SymFrameCookie;
struct CV_SymFrameCookie
{
  U32                off;
  CV_Reg             reg;
  CV_FrameCookieKind kind;
  U8                 flags;
};

//- (SymKind: DISCARDED)

typedef U8 CV_DiscardedKind;
typedef enum CV_DiscardedKindEnum
{
  CV_DiscardedKind_Unknown,
  CV_DiscardedKind_NotSelected,
  CV_DiscardedKind_NotReferenced,
}
CV_DiscardedKindEnum;

typedef struct CV_SymDiscarded CV_SymDiscarded;
struct CV_SymDiscarded
{
  CV_DiscardedKind kind;
  U32              file_id;
  U32              file_ln;
  // U8[] data (rest of data)
};

//- (SymKind: COMPILE3)
typedef U32 CV_Compile3Flags;
enum
{
  CV_Compile3Flag_EC             = (1 << 8),
  CV_Compile3Flag_NoDbgInfo      = (1 << 9),
  CV_Compile3Flag_LTCG           = (1 << 10),
  CV_Compile3Flag_NoDataAlign    = (1 << 11),
  CV_Compile3Flag_ManagedPresent = (1 << 12),
  CV_Compile3Flag_SecurityChecks = (1 << 13),
  CV_Compile3Flag_HotPatch       = (1 << 14),
  CV_Compile3Flag_CVTCIL         = (1 << 15),
  CV_Compile3Flag_MSILModule     = (1 << 16),
  CV_Compile3Flag_SDL            = (1 << 17),
  CV_Compile3Flag_PGO            = (1 << 18),
  CV_Compile3Flag_EXP            = (1 << 19),

  CV_Compile3Flag_Language_Shift = 0, CV_Compile3Flag_Language_Mask = 0xff,
};

typedef U32 CV_Compile3Flags;
#define CV_Compile3Flags_Extract_Language(f)        (((f)    )&0xFF)
#define CV_Compile3Flags_Extract_EditAndContinue(f) (((f)>> 9)&0x01)
#define CV_Compile3Flags_Extract_NoDbgInfo(f)       (((f)>>10)&0x01)
#define CV_Compile3Flags_Extract_LTCG(f)            (((f)>>11)&0x01)
#define CV_Compile3Flags_Extract_NoDataAlign(f)     (((f)>>12)&0x01)
#define CV_Compile3Flags_Extract_ManagedPresent(f)  (((f)>>13)&0x01)
#define CV_Compile3Flags_Extract_SecurityChecks(f)  (((f)>>14)&0x01)
#define CV_Compile3Flags_Extract_HotPatch(f)        (((f)>>15)&0x01)
#define CV_Compile3Flags_Extract_CVTCIL(f)          (((f)>>16)&0x01)
#define CV_Compile3Flags_Extract_MSILModule(f)      (((f)>>17)&0x01)
#define CV_Compile3Flags_Extract_SDL(f)             (((f)>>18)&0x01)
#define CV_Compile3Flags_Extract_PGO(f)             (((f)>>19)&0x01)
#define CV_Compile3Flags_Extract_EXP(f)             (((f)>>20)&0x01)

typedef struct CV_SymCompile3 CV_SymCompile3;
struct CV_SymCompile3
{
  CV_Compile3Flags flags;
  CV_Arch          machine;
  U16              ver_fe_major;
  U16              ver_fe_minor;
  U16              ver_fe_build;
  U16              ver_feqfe;
  U16              ver_major;
  U16              ver_minor;
  U16              ver_build;
  U16              ver_qfe;
  // U8[] ver_str (null terminated)
};

//- (SymKind: ENVBLOCK)

typedef struct CV_SymEnvBlock CV_SymEnvBlock;
struct CV_SymEnvBlock
{
  U8 flags;
  // U8[][] rgsz (sequence null terminated strings)
};

//- (SymKind: LOCAL)

typedef struct CV_SymLocal CV_SymLocal;
struct CV_SymLocal
{
  CV_TypeId     itype;
  CV_LocalFlags flags;
  // U8[] name (null terminated)
};

//- DEFRANGE

typedef struct CV_LvarAddrRange CV_LvarAddrRange;
struct CV_LvarAddrRange
{
  U32 off;
  U16 sec;
  U16 len;
};

typedef struct CV_LvarAddrGap CV_LvarAddrGap;
struct CV_LvarAddrGap
{
  U16 off;
  U16 len;
};

typedef U16 CV_RangeAttribs;
enum
{
  CV_RangeAttrib_Maybe = (1 << 0),
};

//- (SymKind: DEFRANGE)

typedef struct CV_SymDefrange CV_SymDefrange;
struct CV_SymDefrange
{
  U32              program;
  CV_LvarAddrRange range;
  // variable-width: CV_LvarAddrGap gaps;
};

//- (SymKind: DEFRANGE_SUBFIELD)

typedef struct CV_SymDefrangeSubfield CV_SymDefrangeSubfield;
struct CV_SymDefrangeSubfield
{
  U32              program;
  U32              off_in_parent;
  CV_LvarAddrRange range;
  // CV_LvarAddrGap[] gaps (rest of data)
};

//- (SymKind: DEFRANGE_REGISTER)

typedef struct CV_SymDefrangeRegister CV_SymDefrangeRegister;
struct CV_SymDefrangeRegister
{
  CV_Reg           reg;
  CV_RangeAttribs  attribs;
  CV_LvarAddrRange range;
  // CV_LvarAddrGap[] gaps (rest of data)
};

//- (SymKind: DEFRANGE_FRAMEPOINTER_REL)

typedef struct CV_SymDefrangeFramepointerRel CV_SymDefrangeFramepointerRel;
struct CV_SymDefrangeFramepointerRel
{
  S32              off;
  CV_LvarAddrRange range;
  // CV_LvarAddrGap[] gaps (rest of data)
};

//- (SymKind: DEFRANGE_SUBFIELD_REGISTER)

#define CV_DefrangeSubfieldRegister_Extract_ParentOffset(x) ((x) & 0x1FFF)

typedef struct CV_SymDefrangeSubfieldRegister CV_SymDefrangeSubfieldRegister;
struct CV_SymDefrangeSubfieldRegister
{
  CV_Reg           reg;
  CV_RangeAttribs  attribs;
  U32              field_offset;
  CV_LvarAddrRange range;
  // CV_LvarAddrGap[] gaps (rest of data)
};

//- (SymKind: DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE)

typedef struct CV_SymDefrangeFramepointerRelFullScope CV_SymDefrangeFramepointerRelFullScope;
struct CV_SymDefrangeFramepointerRelFullScope
{
  S32 off;
};

//- (SymKind: DEFRANGE_REGISTER_REL)

typedef U16 CV_DefrangeRegisterRelFlags;
enum
{
  CV_DefrangeRegisterRelFlag_SpilledOutUDTMember = (1 << 0),
};
#define CV_DefrangeRegisterRelFlag_Extract_OffsetParent(f) (((f)>>4)&0xFFF)

typedef struct CV_SymDefrangeRegisterRel CV_SymDefrangeRegisterRel;
struct CV_SymDefrangeRegisterRel
{
  CV_Reg                      reg;
  CV_DefrangeRegisterRelFlags flags;
  S32                         reg_off;
  CV_LvarAddrRange            range;
  // CV_LvarAddGap[] gaps (rest of data)
};

//- (SymKind: BUILDINFO)

typedef struct CV_SymBuildInfo CV_SymBuildInfo;
struct CV_SymBuildInfo
{
  CV_ItemId id;
};

//- (SymKind: INLINESITE)

typedef U32 CV_InlineBinaryAnnotation;
typedef enum CV_InlineBinaryAnnotationEnum
{
  CV_InlineBinaryAnnotation_Null,
  CV_InlineBinaryAnnotation_CodeOffset,
  CV_InlineBinaryAnnotation_ChangeCodeOffsetBase,
  CV_InlineBinaryAnnotation_ChangeCodeOffset,
  CV_InlineBinaryAnnotation_ChangeCodeLength,
  CV_InlineBinaryAnnotation_ChangeFile,
  CV_InlineBinaryAnnotation_ChangeLineOffset,
  CV_InlineBinaryAnnotation_ChangeLineEndDelta,
  CV_InlineBinaryAnnotation_ChangeRangeKind,
  CV_InlineBinaryAnnotation_ChangeColumnStart,
  CV_InlineBinaryAnnotation_ChangeColumnEndDelta,
  CV_InlineBinaryAnnotation_ChangeCodeOffsetAndLineOffset,
  CV_InlineBinaryAnnotation_ChangeCodeLengthAndCodeOffset,
  CV_InlineBinaryAnnotation_ChangeColumnEnd
}
CV_InlineBinaryAnnotationEnum;

typedef U32 CV_InlineRangeKind;
typedef enum CV_InlineRangeKindEnum
{
  CV_InlineRangeKind_Expr,
  CV_InlineRangeKind_Stmt
}
CV_InlineRangeKindEnum;

typedef struct CV_SymInlineSite CV_SymInlineSite;
struct CV_SymInlineSite
{
  U32       parent;
  U32       end;
  CV_ItemId inlinee;
  // U8 annotations[] (rest of data)
};

//- (SymKind: INLINESITE2)

typedef struct CV_SymInlineSite2 CV_SymInlineSite2;
struct CV_SymInlineSite2
{
  U32       parent_off;
  U32       end_off;
  CV_ItemId inlinee;
  U32       invocations;
  // U8 annotations[] (rest of data)
};

//- (SymKind: INLINESITE_END) (empty)

//- (SymKind: FILESTATIC)

typedef struct CV_SymFileStatic CV_SymFileStatic;
struct CV_SymFileStatic
{
  CV_TypeId     itype;
  U32           mod_offset;
  CV_LocalFlags flags;
  // U8[] name (null terminated)
};

//- (SymKind: ARMSWITCHTABLE)

typedef U16 CV_ArmSwitchKind;
typedef enum CV_ArmSwitchKindEnum
{
  CV_ArmSwitchKind_INT1,
  CV_ArmSwitchKind_UINT1,
  CV_ArmSwitchKind_INT2,
  CV_ArmSwitchKind_UINT2,
  CV_ArmSwitchKind_INT4,
  CV_ArmSwitchKind_UINT5,
  CV_ArmSwitchKind_POINTER,
  CV_ArmSwitchKind_UINT1SHL1,
  CV_ArmSwitchKind_UINT2SHL1,
  CV_ArmSwitchKind_INT1SSHL1,
  CV_ArmSwitchKind_INT2SSHL1,
}
CV_ArmSwitchKindEnum;

typedef struct CV_SymArmSwitchTable CV_SymArmSwitchTable;
struct CV_SymArmSwitchTable
{
  U32              off_base;
  U16              sec_base;
  CV_ArmSwitchKind kind;
  U32              off_branch;
  U32              off_table;
  U16              sec_branch;
  U16              sec_table;
  U32              entry_count;
};

//- (SymKind: CALLEES, CALLERS)

typedef struct CV_SymFunctionList CV_SymFunctionList;
struct CV_SymFunctionList
{
  U32 count;
  // CV_TypeId[count] funcs
  // U32[clamp(count, rest_of_data/4)] invocations
};

//- (SymKind: POGODATA)

typedef struct CV_SymPogoInfo CV_SymPogoInfo;
struct CV_SymPogoInfo
{
  U32 invocations;
  U64 dynamic_inst_count;
  U32 static_inst_count;
  U32 post_inline_static_inst_count;
};

//- (SymKind: HEAPALLOCSITE)

typedef struct CV_SymHeapAllocSite CV_SymHeapAllocSite;
struct CV_SymHeapAllocSite
{
  U32       off;
  U16       sec;
  U16       call_inst_len;
  CV_TypeId itype;
};

//- (SymKind: MOD_TYPEREF)

typedef U32 CV_ModTypeRefFlags;
enum
{
  CV_ModTypeRefFlag_None     = (1 << 0),
  CV_ModTypeRefFlag_RefTMPCT = (1 << 1),
  CV_ModTypeRefFlag_OwnTMPCT = (1 << 2),
  CV_ModTypeRefFlag_OwnTMR   = (1 << 3),
  CV_ModTypeRefFlag_OwnTM    = (1 << 4),
  CV_ModTypeRefFlag_RefTM    = (1 << 5),
};

typedef struct CV_SymModTypeRef CV_SymModTypeRef;
struct CV_SymModTypeRef
{
  CV_ModTypeRefFlags flags;
  // contain stream number or module index depending on flags     (undocumented)
  U32                word0;
  U32                word1;
};

//- (SymKind: REF_MINIPDB)

typedef U16 CV_RefMiniPdbFlags;
enum
{
  CV_RefMiniPdbFlag_Local = (1 << 0),
  CV_RefMiniPdbFlag_Data  = (1 << 1),
  CV_RefMiniPdbFlag_UDT   = (1 << 2),
  CV_RefMiniPdbFlag_Label = (1 << 3),
  CV_RefMiniPdbFlag_Const = (1 << 4),
};

typedef struct CV_SymRefMiniPdb CV_SymRefMiniPdb;
struct CV_SymRefMiniPdb
{
  U32                data;
  CV_ModIndex        imod;
  CV_RefMiniPdbFlags flags;
  // U8[] name (null terminated)
};

//- (SymKind: FASTLINK)

typedef U16 CV_FastLinkFlags;
enum
{
  CV_FastLinkFlag_IsGlobalData = (1 << 0),
  CV_FastLinkFlag_IsData       = (1 << 1),
  CV_FastLinkFlag_IsUDT        = (1 << 2),
  // 3 ~ unknown/unused
  CV_FastLinkFlag_IsConst      = (1 << 4),
  // 5 ~ unknown/unused
  CV_FastLinkFlag_IsNamespace  = (1 << 6),
};

typedef struct CV_SymFastLink CV_SymFastLink;
struct CV_SymFastLink
{
  CV_TypeId        itype;
  CV_FastLinkFlags flags;
  // U8[] name (null terminated)
};

//- (SymKind: INLINEES)

typedef struct CV_SymInlinees CV_SymInlinees;
struct CV_SymInlinees
{
  U32 count;
  // U32[count] desc;
};

////////////////////////////////
//~ rjf: CodeView Format "Leaf" Types
//
//   (type info)
//

#define CV_TypeId_Variadic 0

#define CV_BasicPointerKindXList(X) \
  X(VALUE,      0x0)                \
  X(16BIT,      0x1)                \
  X(FAR_16BIT,  0x2)                \
  X(HUGE_16BIT, 0x3)                \
  X(32BIT,      0x4)                \
  X(16_32BIT,   0x5)                \
  X(64BIT,      0x6)

typedef U8 CV_BasicPointerKind;
typedef enum
{
#define X(N,c) CV_BasicPointerKind_##N = c,
  CV_BasicPointerKindXList(X)
#undef X
} CV_BasicPointerKindEnum;

#define CV_BasicTypeFromTypeId(x)        ((x)&0xFF)
#define CV_BasicPointerKindFromTypeId(x) (((x)>>8)&0xFF)

typedef U8 CV_HFAKind;
typedef enum CV_HFAKindEnum
{
  CV_HFAKind_None,
  CV_HFAKind_Float,
  CV_HFAKind_Double,
  CV_HFAKind_Other
} CV_HFAKindEnum;

typedef U8 CV_MoComUDTKind;
typedef enum CV_MoComUDTKindEnum
{
  CV_MoComUDTKind_None,
  CV_MoComUDTKind_Ref,
  CV_MoComUDTKind_Value,
  CV_MoComUDTKind_Interface
} CV_MoComUDTKindEnum;

typedef U16 CV_TypeProps;
enum
{
  CV_TypeProp_Packed                     = (1 << 0),
  CV_TypeProp_HasConstructorsDestructors = (1 << 1),
  CV_TypeProp_OverloadedOperators        = (1 << 2),
  CV_TypeProp_IsNested                   = (1 << 3),
  CV_TypeProp_ContainsNested             = (1 << 4),
  CV_TypeProp_OverloadedAssignment       = (1 << 5),
  CV_TypeProp_OverloadedCasting          = (1 << 6),
  CV_TypeProp_FwdRef                     = (1 << 7),
  CV_TypeProp_Scoped                     = (1 << 8),
  CV_TypeProp_HasUniqueName              = (1 << 9),
  CV_TypeProp_Sealed                     = (1 << 10),
  // HFA: 11,12
  CV_TypeProp_Intrinsic                  = (1 << 13),
  // MOCOM: 14,15
};
#define CV_TypeProps_Extract_HFA(f)   (((f)>>11)&0x3)
#define CV_TypeProps_Extract_MOCOM(f) (((f)>>14)&0x3)

typedef U8 CV_PointerKind;
typedef enum CV_PointerKindEnum
{
  CV_PointerKind_Near,      // 16 bit
  CV_PointerKind_Far,       // 16:16 bit
  CV_PointerKind_Huge,      // 16:16 bit
  CV_PointerKind_BaseSeg,
  CV_PointerKind_BaseVal,
  CV_PointerKind_BaseSegVal,
  CV_PointerKind_BaseAddr,
  CV_PointerKind_BaseSegAddr,
  CV_PointerKind_BaseType,
  CV_PointerKind_BaseSelf,
  CV_PointerKind_Near32,    // 32 bit
  CV_PointerKind_Far32,     // 16:32 bit
  CV_PointerKind_64,        // 64 bit
} CV_PointerKindEnum;

typedef U8 CV_PointerMode;
typedef enum CV_PointerModeEnum
{
  CV_PointerMode_Ptr,
  CV_PointerMode_LRef,
  CV_PointerMode_PtrMem,
  CV_PointerMode_PtrMethod,
  CV_PointerMode_RRef,
}
CV_PointerModeEnum;

typedef U16 CV_MemberPointerKind;
typedef enum CV_MemberPointerKindEnum
{
  CV_MemberPointerKind_Undef,
  CV_MemberPointerKind_DataSingle,
  CV_MemberPointerKind_DataMultiple,
  CV_MemberPointerKind_DataVirtual,
  CV_MemberPointerKind_DataGeneral,
  CV_MemberPointerKind_FuncSingle,
  CV_MemberPointerKind_FuncMultiple,
  CV_MemberPointerKind_FuncVirtual,
  CV_MemberPointerKind_FuncGeneral,
}
CV_MemberPointerKindEnum;

typedef U32 CV_VirtualTableShape;
typedef enum CV_VirtualTableShapeEnum
{
  CV_VirtualTableShape_Near,    // 16 bit ptr
  CV_VirtualTableShape_Far,     // 16:16 bit ptr
  CV_VirtualTableShape_Thin,    // ???
  CV_VirtualTableShape_Outer,   // address point displacment to outermost class entry[-1]
  CV_VirtualTableShape_Meta,    // far pointer to metaclass descriptor entry[-2]
  CV_VirtualTableShape_Near32,  // 32 bit ptr
  CV_VirtualTableShape_Far32,   // ???
}
CV_VirtualTableShapeEnum;

typedef U8 CV_MethodProp;
enum
{
  CV_MethodProp_Vanilla,
  CV_MethodProp_Virtual,
  CV_MethodProp_Static,
  CV_MethodProp_Friend,
  CV_MethodProp_Intro,
  CV_MethodProp_PureVirtual,
  CV_MethodProp_PureIntro,
};

typedef U8 CV_MemberAccess;
typedef enum CV_MemberAccessEnum
{
  CV_MemberAccess_Null,
  CV_MemberAccess_Private,
  CV_MemberAccess_Protected,
  CV_MemberAccess_Public
}
CV_MemberAccessEnum;

typedef U16 CV_FieldAttribs;
enum
{
  // Access: 0,1
  // MethodProp: [2:4]
  CV_FieldAttrib_Pseudo          = (1<<5),
  CV_FieldAttrib_NoInherit       = (1<<6),
  CV_FieldAttrib_NoConstruct     = (1<<7),
  CV_FieldAttrib_CompilerGenated = (1<<8),
  CV_FieldAttrib_Sealed          = (1<<9),
};
#define CV_FieldAttribs_Extract_Access(f)     ((f)&0x3)
#define CV_FieldAttribs_Extract_MethodProp(f) (((f)>>2)&0x7)

typedef U16 CV_LabelKind;
typedef enum CV_LabelKindEnum
{
  CV_LabelKind_Near = 0,
  CV_LabelKind_Far  = 4,
}
CV_LabelKindEnum;

typedef U8 CV_FunctionAttribs;
enum
{
  CV_FunctionAttrib_CxxReturnUDT     = (1<<0),
  CV_FunctionAttrib_Constructor      = (1<<1),
  CV_FunctionAttrib_ConstructorVBase = (1<<2),
};

typedef U8 CV_CallKind;
typedef enum CV_CallKindEnum
{
  CV_CallKind_NearC,
  CV_CallKind_FarC,
  CV_CallKind_NearPascal,
  CV_CallKind_FarPascal,
  CV_CallKind_NearFast,
  CV_CallKind_FarFast,
  CV_CallKind_UNUSED,
  CV_CallKind_NearStd,
  CV_CallKind_FarStd,
  CV_CallKind_NearSys,
  CV_CallKind_FarSys,
  CV_CallKind_This,
  CV_CallKind_Mips,
  CV_CallKind_Generic,
  CV_CallKind_Alpha,
  CV_CallKind_PPC,
  CV_CallKind_HitachiSuperH,
  CV_CallKind_Arm,
  CV_CallKind_AM33,
  CV_CallKind_TriCore,
  CV_CallKind_HitachiSuperH5,
  CV_CallKind_M32R,
  CV_CallKind_Clr,
  CV_CallKind_Inline,
  CV_CallKind_NearVector,
}
CV_CallKindEnum;

//- (LeafKind: PRECOMP)

typedef struct CV_LeafPreComp CV_LeafPreComp;
struct CV_LeafPreComp
{
  U32 start_index;
  U32 count;
  U32 sig;
  // U8[] name (null terminated)
};

//- (LeafKind; END_PRECOMP)

typedef struct CV_LeafEndPreComp CV_LeafEndPreComp;
struct CV_LeafEndPreComp
{
  U32 sig;
};

//- (LeafKind: TYPESERVER)

typedef struct CV_LeafTypeServer CV_LeafTypeServer;
struct CV_LeafTypeServer
{
  U32 sig;
  U32 age;
  // U8[] name (null terminated)
};

//- (LeafKind: TYPESERVER2)

typedef struct CV_LeafTypeServer2 CV_LeafTypeServer2;
struct CV_LeafTypeServer2
{
  Guid sig70;
  U32  age;
  // U8[] name (null terminated)
};

//- (LeafKind: SKIP)

typedef struct CV_LeafSkip CV_LeafSkip;
struct CV_LeafSkip
{
  CV_TypeId itype;
};

//- (LeafKind: VTSHAPE)

typedef struct CV_LeafVTShape CV_LeafVTShape;
struct CV_LeafVTShape
{
  U16 count;
  // U4[count] shapes (CV_VirtualTableShape)
};

//- (LeafKind: LABEL)

typedef struct CV_LeafLabel CV_LeafLabel;
struct CV_LeafLabel
{
  CV_LabelKind kind;
};

//- (LeafKind: MODIFIER)

typedef U16 CV_ModifierFlags;
enum
{
  CV_ModifierFlag_Const     = (1 << 0),
  CV_ModifierFlag_Volatile  = (1 << 1),
  CV_ModifierFlag_Unaligned = (1 << 2),
};

typedef struct CV_LeafModifier CV_LeafModifier;
struct CV_LeafModifier
{
  CV_TypeId        itype;
  CV_ModifierFlags flags;
};

//- (LeafKind: POINTER)

typedef U32 CV_PointerAttribs;
enum
{
  // Kind: [0:4]
  // Mode: [5:7]
  CV_PointerAttrib_IsFlat     = (1 << 8),
  CV_PointerAttrib_Volatile   = (1 << 9),
  CV_PointerAttrib_Const      = (1 << 10),
  CV_PointerAttrib_Unaligned  = (1 << 11),
  CV_PointerAttrib_Restricted = (1 << 12),
  // Size: [13,18]
  CV_PointerAttrib_MOCOM      = (1 << 19),
  CV_PointerAttrib_LRef       = (1 << 21),
  CV_PointerAttrib_RRef       = (1 << 22)
};

#define CV_PointerAttribs_Extract_Kind(a) ((a)&0x1F)
#define CV_PointerAttribs_Extract_Mode(a) (((a)>>5)&0x7)
#define CV_PointerAttribs_Extract_Size(a) (((a)>>13)&0x3F)

typedef struct CV_LeafPointer CV_LeafPointer;
struct CV_LeafPointer
{
  CV_TypeId         itype;
  CV_PointerAttribs attribs;
};

//- (LeafKind: PROCEDURE)

typedef struct CV_LeafProcedure CV_LeafProcedure;
struct CV_LeafProcedure
{
  CV_TypeId          ret_itype;
  CV_CallKind        call_kind;
  CV_FunctionAttribs attribs;
  U16                arg_count;
  CV_TypeId          arg_itype;
};

//- (LeafKind: MFUNCTION)

typedef struct CV_LeafMFunction CV_LeafMFunction;
struct CV_LeafMFunction
{
  CV_TypeId          ret_itype;
  CV_TypeId          class_itype;
  CV_TypeId          this_itype;
  CV_CallKind        call_kind;
  CV_FunctionAttribs attribs;
  U16                arg_count;
  CV_TypeId          arg_itype;
  S32                this_adjust;
};

//- (LeafKind: ARGLIST)

typedef struct CV_LeafArgList CV_LeafArgList;
struct CV_LeafArgList
{
  U32 count;
  // CV_TypeId[count] itypes;
};

//- (LeafKind: BITFIELD)

typedef struct CV_LeafBitField CV_LeafBitField;
struct CV_LeafBitField
{
  CV_TypeId itype;
  U8        len;
  U8        pos;
};

//- (LeafKind: METHODLIST)

//   ("jagged" array of these vvvvvvvv)
typedef struct CV_LeafMethodListMember CV_LeafMethodListMember;
struct CV_LeafMethodListMember
{
  CV_FieldAttribs attribs;
  U16             pad;
  CV_TypeId       itype;
  // U32 vbaseoff (when Intro or PureIntro)
};

//- (LeafKind: INDEX)

typedef struct CV_LeafIndex CV_LeafIndex;
struct CV_LeafIndex
{
  U16       pad;
  CV_TypeId itype;
};

//- (LeafKind: ARRAY)

typedef struct CV_LeafArray CV_LeafArray;
struct CV_LeafArray
{
  CV_TypeId entry_itype;
  CV_TypeId index_itype;
  // CV_Numeric count
};

//- (LeafKind: CLASS, STRUCTURE, INTERFACE)

typedef struct CV_LeafStruct CV_LeafStruct;
struct CV_LeafStruct
{
  U16          count;
  CV_TypeProps props;
  CV_TypeId    field_itype;
  CV_TypeId    derived_itype;
  CV_TypeId    vshape_itype;
  // CV_Numeric size
  // U8[] name (null terminated)
  // U8[] unique_name (null terminated)
};

//- (LeafKind: UNION)

typedef struct CV_LeafUnion CV_LeafUnion;
struct CV_LeafUnion
{
  U16          count;
  CV_TypeProps props;
  CV_TypeId    field_itype;
  // CV_Numeric size
  // U8[] name (null terminated)
  // U8[] unique_name (null terminated)
};

//- (LeafKind: ENUM)

typedef struct CV_LeafEnum CV_LeafEnum;
struct CV_LeafEnum
{
  U16          count;
  CV_TypeProps props;
  CV_TypeId    base_itype;
  CV_TypeId    field_itype;
  // U8[] name (null terminated)
  // U8[] unique_name (null terminated)
};

//- (LeafKind: ALIAS)

typedef struct CV_LeafAlias CV_LeafAlias;
struct CV_LeafAlias
{
  CV_TypeId itype;
  // U8[] name (null terminated)
};

//- (LeafKind: MEMBER)

typedef struct CV_LeafMember CV_LeafMember;
struct CV_LeafMember
{
  CV_FieldAttribs attribs;
  CV_TypeId       itype;
  // CV_Numeric offset
  // U8[] name (null terminated)
};

//- (LeafKind: STMEMBER)

typedef struct CV_LeafStMember CV_LeafStMember;
struct CV_LeafStMember
{
  CV_FieldAttribs attribs;
  CV_TypeId       itype;
  // U8[] name (null terminated)
};

//- (LeafKind: METHOD)

typedef struct CV_LeafMethod CV_LeafMethod;
struct CV_LeafMethod
{
  U16       count;
  CV_TypeId list_itype;
  // U8[] name (null terminated)
};

//- (LeafKind: ONEMETHOD)

typedef struct CV_LeafOneMethod CV_LeafOneMethod;
struct CV_LeafOneMethod
{
  CV_FieldAttribs attribs;
  CV_TypeId       itype;
  // U32 vbaseoff (when Intro or PureIntro)
  // U8[] name (null terminated)
};

//- (LeafKind: ENUMERATE)

typedef struct CV_LeafEnumerate CV_LeafEnumerate;
struct CV_LeafEnumerate
{
  CV_FieldAttribs attribs;
  // CV_Numeric val
  // U8[] name (null terminated)
};

//- (LeafKind: NESTTYPE)

typedef struct CV_LeafNestType CV_LeafNestType;
struct CV_LeafNestType
{
  U16       pad;
  CV_TypeId itype;
  // U8[] name (null terminated)
};

//- (LeafKind: NESTTYPEEX)

typedef struct CV_LeafNestTypeEx CV_LeafNestTypeEx;
struct CV_LeafNestTypeEx
{
  CV_FieldAttribs attribs;
  CV_TypeId       itype;
  // U8[] name (null terminated)
};

//- (LeafKind: BCLASS)

typedef struct CV_LeafBClass CV_LeafBClass;
struct CV_LeafBClass
{
  CV_FieldAttribs attribs;
  CV_TypeId       itype;
  // CV_Numeric offset
};

//- (LeafKind: VBCLASS, IVBCLASS)

typedef struct CV_LeafVBClass CV_LeafVBClass;
struct CV_LeafVBClass
{
  CV_FieldAttribs attribs;
  CV_TypeId       itype;
  CV_TypeId       vbptr_itype;
  // CV_Numeric vbptr_off
  // CV_Numeric vtable_off
};

//- (LeafKind: VFUNCTAB)

typedef struct CV_LeafVFuncTab CV_LeafVFuncTab;
struct CV_LeafVFuncTab
{
  U16       pad;
  CV_TypeId itype;
};

//- (LeafKind: VFUNCOFF)

typedef struct CV_LeafVFuncOff CV_LeafVFuncOff;
struct CV_LeafVFuncOff
{
  U16       pad;
  CV_TypeId itype;
  U32       off;
};

//- (LeafKind: VFTABLE)

typedef struct CV_LeafVFTable CV_LeafVFTable;
struct CV_LeafVFTable
{
  CV_TypeId owner_itype;
  CV_TypeId base_table_itype;
  U32       offset_in_object_layout;
  U32       names_len;
  // U8[] names (multiple null terminated strings)
};

//- (LeafKind: VFTPATH)

typedef struct CV_LeafVFPath CV_LeafVFPath;
struct CV_LeafVFPath
{
  U32 count;
  // CV_TypeId[count] base;
};

//- (LeafKind: CLASS2, STRUCT2)

typedef struct CV_LeafStruct2 CV_LeafStruct2;
struct CV_LeafStruct2
{
  // NOTE: still reverse engineering this - if you find docs please help!
  CV_TypeProps props;
  U16          unknown1;
  CV_TypeId    field_itype;
  CV_TypeId    derived_itype;
  CV_TypeId    vshape_itype;
  U16          unknown2;
  // CV_Numeric size
  // U8[] name (null terminated)
  // U8[] unique_name (null terminated)
};

//- (LeafIDKind: FUNC_ID)

typedef struct CV_LeafFuncId CV_LeafFuncId;
struct CV_LeafFuncId
{
  CV_ItemId scope_string_id;
  CV_TypeId itype;
  // U8[] name (null terminated)
};

//- (LeafIDKind: MFUNC_ID)

typedef struct CV_LeafMFuncId CV_LeafMFuncId;
struct CV_LeafMFuncId
{
  CV_TypeId owner_itype;
  CV_TypeId itype;
  // U8[] name (null terminated)
};

//- (LeafIDKind: STRING_ID)

typedef struct CV_LeafStringId CV_LeafStringId;
struct CV_LeafStringId
{
  CV_ItemId substr_list_id;
  // U8[] string (null terminated)
};

//- (LeafIDKind: BUILDINFO)

typedef enum CV_BuildInfoIndexEnum
{
  CV_BuildInfoIndex_BuildDirectory     = 0,
  CV_BuildInfoIndex_CompilerExecutable = 1,
  CV_BuildInfoIndex_TargetSourceFile   = 2,
  CV_BuildInfoIndex_CombinedPdb        = 3,
  CV_BuildInfoIndex_CompileArguments   = 4,
}
CV_BuildInfoIndexEnum;

typedef struct CV_LeafBuildInfo CV_LeafBuildInfo;
struct CV_LeafBuildInfo
{
  U16 count;
  // CV_ItemId[count] items
};

//- (LeafIDKind: SUBSTR_LIST)

typedef struct CV_LeafSubstrList CV_LeafSubstrList;
struct CV_LeafSubstrList
{
  U32 count;
  // CV_ItemId[count] items
};

//- (LeafIDKind: UDT_SRC_LINE)

typedef struct CV_LeafUDTSrcLine CV_LeafUDTSrcLine;
struct CV_LeafUDTSrcLine
{
  CV_TypeId udt_itype;
  CV_ItemId src_string_id;
  U32       line;
};

//- (LeafIDKind: UDT_MOD_SRC_LINE)

typedef struct CV_LeafUDTModSrcLine CV_LeafUDTModSrcLine;
struct CV_LeafUDTModSrcLine
{
  CV_TypeId   udt_itype;
  CV_ItemId   src_string_id;
  U32         line;
  CV_ModIndex imod;
};

////////////////////////////////
//~ CodeView Format C13 Line Info Types

#define CV_C13SubSectionKind_IgnoreFlag 0x80000000

#define CV_C13SubSectionKindXList(X) \
  X(Symbols,             0xF1)       \
  X(Lines,               0xF2)       \
  X(StringTable,         0xF3)       \
  X(FileChksms,          0xF4)       \
  X(FrameData,           0xF5)       \
  X(InlineeLines,        0xF6)       \
  X(CrossScopeImports,   0xF7)       \
  X(CrossScopeExports,   0xF8)       \
  X(IlLines,             0xF9)       \
  X(FuncMDTokenMap,      0xFA)       \
  X(TypeMDTokenMap,      0xFB)       \
  X(MergedAssemblyInput, 0xFC)       \
  X(CoffSymbolRVA,       0xFD)       \
  X(XfgHashType,         0xFF)       \
  X(XfgHashVirtual,      0x100)

typedef U32 CV_C13SubSectionKind;
typedef enum CV_C13SubSectionKindEnum
{
#define X(N,c) CV_C13SubSectionKind_##N = c,
  CV_C13SubSectionKindXList(X)
#undef X
}
CV_C13SubSectionKindEnum;

typedef struct CV_C13SubSectionHeader CV_C13SubSectionHeader;
struct CV_C13SubSectionHeader
{
  CV_C13SubSectionKind kind;
  U32                  size;
};

//- FileChksms sub-section

typedef U8 CV_C13ChecksumKind;
typedef enum CV_C13ChecksumKindEnum
{
  CV_C13ChecksumKind_Null,
  CV_C13ChecksumKind_MD5,
  CV_C13ChecksumKind_SHA1,
  CV_C13ChecksumKind_SHA256,
}
CV_C13ChecksumKindEnum;

typedef struct CV_C13Checksum CV_C13Checksum;
struct CV_C13Checksum
{
  U32                name_off;
  U8                 len;
  CV_C13ChecksumKind kind;
};

//- Lines sub-section

typedef U16 CV_C13SubSecLinesFlags;
enum
{
  CV_C13SubSecLinesFlag_HasColumns = (1 << 0)
};

typedef struct CV_C13SubSecLinesHeader CV_C13SubSecLinesHeader;
struct CV_C13SubSecLinesHeader
{
  U32                    sec_off;
  CV_SectionIndex        sec;
  CV_C13SubSecLinesFlags flags;
  U32                    len;
};

typedef struct CV_C13File CV_C13File;
struct CV_C13File
{
  U32 file_off;
  U32 num_lines;
  U32 block_size;
  // CV_C13Line[num_lines] lines;
  // CV_C13Column[num_lines] columns; (if HasColumns)
};

typedef U32 CV_C13LineFlags;
#define CV_C13LineFlags_Extract_LineNumber(f) ((f)&0xFFFFFF)
#define CV_C13LineFlags_Extract_DeltaToEnd(f) (((f)>>24)&0x7F)
#define CV_C13LineFlags_Extract_Statement(f)  (((f)>>31)&0x1)

typedef struct CV_C13Line CV_C13Line;
struct CV_C13Line
{
  U32             off;
  CV_C13LineFlags flags;
};

typedef struct CV_C13Column CV_C13Column;
struct CV_C13Column
{
  U16 start;
  U16 end;
};

//- FrameData sub-section

typedef U32 CV_C13FrameDataFlags;
enum
{
  CV_C13FrameDataFlag_HasStructuredExceptionHandling = (1 << 0),
  CV_C13FrameDataFlag_HasExceptionHandling           = (1 << 1),
  CV_C13FrameDataFlag_HasIsFuncStart                 = (1 << 2),
};

typedef struct CV_C13FrameData CV_C13FrameData;
struct CV_C13FrameData
{
  U32                  start_voff;
  U32                  code_size;
  U32                  local_size;
  U32                  params_size;
  U32                  max_stack_size;
  U32                  frame_func;
  U16                  prolog_size;
  U16                  saved_reg_size;
  CV_C13FrameDataFlags flags;
};

//- InlineLines sub-section 

typedef U32 CV_C13InlineeLinesSig;
enum
{
  CV_C13InlineeLinesSig_NORMAL,
  CV_C13InlineeLinesSig_EXTRA_FILES,
};

typedef struct CV_C13InlineeSourceLineHeader CV_C13InlineeSourceLineHeader;
struct CV_C13InlineeSourceLineHeader
{
  CV_ItemId inlinee;          // LF_FUNC_ID or LF_MFUNC_ID
  U32       file_off;         // offset into FileChksms sub-section
  U32       first_source_ln;  // base source line number for binary annotations
  // if sig set to CV_C13InlineeLinesSig_EXTRA_FILES
  //  U32 extra_file_count;
  //  U32 files[];
};

#pragma pack(pop)

////////////////////////////////
//~ Type Index Helper

enum CV_TypeIndexSource
{
  CV_TypeIndexSource_NULL,
  CV_TypeIndexSource_TPI,
  CV_TypeIndexSource_IPI,
  CV_TypeIndexSource_COUNT
};
typedef enum CV_TypeIndexSource CV_TypeIndexSource;

typedef struct CV_TypeIndexInfo CV_TypeIndexInfo;
struct CV_TypeIndexInfo
{
  struct CV_TypeIndexInfo *next;
  U64                      offset;
  CV_TypeIndexSource       source;
};

typedef struct CV_TypeIndexInfoList CV_TypeIndexInfoList;
struct CV_TypeIndexInfoList
{
  U64               count;
  CV_TypeIndexInfo *first;
  CV_TypeIndexInfo *last;
};

typedef struct CV_TypeIndexArray CV_TypeIndexArray;
struct CV_TypeIndexArray
{
  U32           count;
  CV_TypeIndex *v;
};

////////////////////////////////

internal CV_Arch               cv_arch_from_coff_machine(COFF_MachineType machine);
internal U64                   cv_size_from_reg_x86(CV_Reg reg);
internal U64                   cv_size_from_reg_x64(CV_Reg reg);
internal U64                   cv_size_from_reg(CV_Arch arch, CV_Reg reg);
internal B32                   cv_is_reg_sp(CV_Arch arch, CV_Reg reg);
internal CV_EncodedFramePtrReg cv_pick_fp_encoding(CV_SymFrameproc *frameproc, B32 is_local_param);
internal CV_Reg                cv_decode_fp_reg(CV_Arch arch, CV_EncodedFramePtrReg encoded_reg);
internal U32                   cv_map_encoded_base_pointer(CV_Arch arch, U32 encoded_frame_reg);

#endif // CODEVIEW_H
 
