// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef CODEVIEW_H
#define CODEVIEW_H

#pragma pack(push, 1)

// https://github.com/microsoft/microsoft-pdb/blob/master/include/cvinfo.h

////////////////////////////////
//~ rjf: Format Shared Types

typedef U32 CV_TypeId;
typedef U32 CV_ItemId;
typedef U16 CV_ModIndex;
typedef U16 CV_SectionIndex;
typedef U16 CV_Reg;

read_only global CV_TypeId cv_type_id_variadic = 0xFFFFFFFF;

////////////////////////////////
//~ rjf: Generated Code

#include "generated/codeview.meta.h"

////////////////////////////////
//~ rjf: Registers

// X(NAME, CODE, (RDI_RegisterCode_X86) NAME, BYTE_POS, BYTE_SIZE)
#define CV_Reg_X86_XList(X) \
X(NONE,     0, nil, 0, 0)\
X(AL,       1, eax, 0, 1)\
X(CL,       2, ecx, 0, 1)\
X(DL,       3, edx, 0, 1)\
X(BL,       4, ebx, 0, 1)\
X(AH,       5, eax, 1, 1)\
X(CH,       6, ecx, 1, 1)\
X(DH,       7, edx, 1, 1)\
X(BH,       8, ebx, 1, 1)\
X(AX,       9, eax, 0, 2)\
X(CX,      10, ecx, 0, 2)\
X(DX,      11, edx, 0, 2)\
X(BX,      12, ebx, 0, 2)\
X(SP,      13, esp, 0, 2)\
X(BP,      14, ebp, 0, 2)\
X(SI,      15, esi, 0, 2)\
X(DI,      16, edi, 0, 2)\
X(EAX,     17, eax, 0, 4)\
X(ECX,     18, ecx, 0, 4)\
X(EDX,     19, edx, 0, 4)\
X(EBX,     20, ebx, 0, 4)\
X(ESP,     21, esp, 0, 4)\
X(EBP,     22, ebp, 0, 4)\
X(ESI,     23, esi, 0, 4)\
X(EDI,     24, edi, 0, 4)\
X(ES,      25, es,  0, 2)\
X(CS,      26, cs,  0, 2)\
X(SS,      27, ss,  0, 2)\
X(DS,      28, ds,  0, 2)\
X(FS,      29, fs,  0, 2)\
X(GS,      30, gs,  0, 2)\
X(IP,      31, eip,    0, 2)\
X(FLAGS,   32, eflags, 0, 2)\
X(EIP,     33, eip,    0, 4)\
X(EFLAGS,  34, eflags, 0, 4)\
X(MM0,    146, fpr0, 0, 8)\
X(MM1,    147, fpr1, 0, 8)\
X(MM2,    148, fpr2, 0, 8)\
X(MM3,    149, fpr3, 0, 8)\
X(MM4,    150, fpr4, 0, 8)\
X(MM5,    151, fpr5, 0, 8)\
X(MM6,    152, fpr6, 0, 8)\
X(MM7,    153, fpr7, 0, 8)\
X(XMM0,   154, ymm0,  0, 16)\
X(XMM1,   155, ymm1,  0, 16)\
X(XMM2,   156, ymm2,  0, 16)\
X(XMM3,   157, ymm3,  0, 16)\
X(XMM4,   158, ymm4,  0, 16)\
X(XMM5,   159, ymm5,  0, 16)\
X(XMM6,   160, ymm6,  0, 16)\
X(XMM7,   161, ymm7,  0, 16)\
X(XMM00,  162, ymm0,  0,  4)\
X(XMM01,  163, ymm0,  4,  4)\
X(XMM02,  164, ymm0,  8,  4)\
X(XMM03,  165, ymm0, 12,  4)\
X(XMM10,  166, ymm1,  0,  4)\
X(XMM11,  167, ymm1,  4,  4)\
X(XMM12,  168, ymm1,  8,  4)\
X(XMM13,  169, ymm1, 12,  4)\
X(XMM20,  170, ymm2,  0,  4)\
X(XMM21,  171, ymm2,  4,  4)\
X(XMM22,  172, ymm2,  8,  4)\
X(XMM23,  173, ymm2, 12,  4)\
X(XMM30,  174, ymm3,  0,  4)\
X(XMM31,  175, ymm3,  4,  4)\
X(XMM32,  176, ymm3,  8,  4)\
X(XMM33,  177, ymm3, 12,  4)\
X(XMM40,  178, ymm4,  0,  4)\
X(XMM41,  179, ymm4,  4,  4)\
X(XMM42,  180, ymm4,  8,  4)\
X(XMM43,  181, ymm4, 12,  4)\
X(XMM50,  182, ymm5,  0,  4)\
X(XMM51,  183, ymm5,  4,  4)\
X(XMM52,  184, ymm5,  8,  4)\
X(XMM53,  185, ymm5, 12,  4)\
X(XMM60,  186, ymm6,  0,  4)\
X(XMM61,  187, ymm6,  4,  4)\
X(XMM62,  188, ymm6,  8,  4)\
X(XMM63,  189, ymm6, 12,  4)\
X(XMM70,  190, ymm7,  0,  4)\
X(XMM71,  191, ymm7,  4,  4)\
X(XMM72,  192, ymm7,  8,  4)\
X(XMM73,  193, ymm7, 12,  4)\
X(XMM0L,  194, ymm0,  0,  8)\
X(XMM1L,  195, ymm1,  0,  8)\
X(XMM2L,  196, ymm2,  0,  8)\
X(XMM3L,  197, ymm3,  0,  8)\
X(XMM4L,  198, ymm4,  0,  8)\
X(XMM5L,  199, ymm5,  0,  8)\
X(XMM6L,  200, ymm6,  0,  8)\
X(XMM7L,  201, ymm7,  0,  8)\
X(XMM0H,  202, ymm0,  8,  8)\
X(XMM1H,  203, ymm1,  8,  8)\
X(XMM2H,  204, ymm2,  8,  8)\
X(XMM3H,  205, ymm3,  8,  8)\
X(XMM4H,  206, ymm4,  8,  8)\
X(XMM5H,  207, ymm5,  8,  8)\
X(XMM6H,  208, ymm6,  8,  8)\
X(XMM7H,  209, ymm7,  8,  8)\
X(YMM0,   252, ymm0,  0, 32)\
X(YMM1,   253, ymm1,  0, 32)\
X(YMM2,   254, ymm2,  0, 32)\
X(YMM3,   255, ymm3,  0, 32)\
X(YMM4,   256, ymm4,  0, 32)\
X(YMM5,   257, ymm5,  0, 32)\
X(YMM6,   258, ymm6,  0, 32)\
X(YMM7,   259, ymm7,  0, 32)\
X(YMM0H,  260, ymm0, 16, 16)\
X(YMM1H,  261, ymm1, 16, 16)\
X(YMM2H,  262, ymm2, 16, 16)\
X(YMM3H,  263, ymm3, 16, 16)\
X(YMM4H,  264, ymm4, 16, 16)\
X(YMM5H,  265, ymm5, 16, 16)\
X(YMM6H,  266, ymm6, 16, 16)\
X(YMM7H,  267, ymm7, 16, 16)\
X(YMM0I0, 268, ymm0,  0,  8)\
X(YMM0I1, 269, ymm0,  8,  8)\
X(YMM0I2, 270, ymm0, 16,  8)\
X(YMM0I3, 271, ymm0, 24,  8)\
X(YMM1I0, 272, ymm1,  0,  8)\
X(YMM1I1, 273, ymm1,  8,  8)\
X(YMM1I2, 274, ymm1, 16,  8)\
X(YMM1I3, 275, ymm1, 24,  8)\
X(YMM2I0, 276, ymm2,  0,  8)\
X(YMM2I1, 277, ymm2,  8,  8)\
X(YMM2I2, 278, ymm2, 16,  8)\
X(YMM2I3, 279, ymm2, 24,  8)\
X(YMM3I0, 280, ymm3,  0,  8)\
X(YMM3I1, 281, ymm3,  8,  8)\
X(YMM3I2, 282, ymm3, 16,  8)\
X(YMM3I3, 283, ymm3, 24,  8)\
X(YMM4I0, 284, ymm4,  0,  8)\
X(YMM4I1, 285, ymm4,  8,  8)\
X(YMM4I2, 286, ymm4, 16,  8)\
X(YMM4I3, 287, ymm4, 24,  8)\
X(YMM5I0, 288, ymm5,  0,  8)\
X(YMM5I1, 289, ymm5,  8,  8)\
X(YMM5I2, 290, ymm5, 16,  8)\
X(YMM5I3, 291, ymm5, 24,  8)\
X(YMM6I0, 292, ymm6,  0,  8)\
X(YMM6I1, 293, ymm6,  8,  8)\
X(YMM6I2, 294, ymm6, 16,  8)\
X(YMM6I3, 295, ymm6, 24,  8)\
X(YMM7I0, 296, ymm7,  0,  8)\
X(YMM7I1, 297, ymm7,  8,  8)\
X(YMM7I2, 298, ymm7, 16,  8)\
X(YMM7I3, 299, ymm7, 24,  8)\
X(YMM0F0, 300, ymm0,  0,  4)\
X(YMM0F1, 301, ymm0,  4,  4)\
X(YMM0F2, 302, ymm0,  8,  4)\
X(YMM0F3, 303, ymm0, 12,  4)\
X(YMM0F4, 304, ymm0, 16,  4)\
X(YMM0F5, 305, ymm0, 20,  4)\
X(YMM0F6, 306, ymm0, 24,  4)\
X(YMM0F7, 307, ymm0, 28,  4)\
X(YMM1F0, 308, ymm1,  0,  4)\
X(YMM1F1, 309, ymm1,  4,  4)\
X(YMM1F2, 310, ymm1,  8,  4)\
X(YMM1F3, 311, ymm1, 12,  4)\
X(YMM1F4, 312, ymm1, 16,  4)\
X(YMM1F5, 313, ymm1, 20,  4)\
X(YMM1F6, 314, ymm1, 24,  4)\
X(YMM1F7, 315, ymm1, 28,  4)\
X(YMM2F0, 316, ymm2,  0,  4)\
X(YMM2F1, 317, ymm2,  4,  4)\
X(YMM2F2, 318, ymm2,  8,  4)\
X(YMM2F3, 319, ymm2, 12,  4)\
X(YMM2F4, 320, ymm2, 16,  4)\
X(YMM2F5, 321, ymm2, 20,  4)\
X(YMM2F6, 322, ymm2, 24,  4)\
X(YMM2F7, 323, ymm2, 28,  4)\
X(YMM3F0, 324, ymm3,  0,  4)\
X(YMM3F1, 325, ymm3,  4,  4)\
X(YMM3F2, 326, ymm3,  8,  4)\
X(YMM3F3, 327, ymm3, 12,  4)\
X(YMM3F4, 328, ymm3, 16,  4)\
X(YMM3F5, 329, ymm3, 20,  4)\
X(YMM3F6, 330, ymm3, 24,  4)\
X(YMM3F7, 331, ymm3, 28,  4)\
X(YMM4F0, 332, ymm4,  0,  4)\
X(YMM4F1, 333, ymm4,  4,  4)\
X(YMM4F2, 334, ymm4,  8,  4)\
X(YMM4F3, 335, ymm4, 12,  4)\
X(YMM4F4, 336, ymm4, 16,  4)\
X(YMM4F5, 337, ymm4, 20,  4)\
X(YMM4F6, 338, ymm4, 24,  4)\
X(YMM4F7, 339, ymm4, 28,  4)\
X(YMM5F0, 340, ymm5,  0,  4)\
X(YMM5F1, 341, ymm5,  4,  4)\
X(YMM5F2, 342, ymm5,  8,  4)\
X(YMM5F3, 343, ymm5, 12,  4)\
X(YMM5F4, 344, ymm5, 16,  4)\
X(YMM5F5, 345, ymm5, 20,  4)\
X(YMM5F6, 346, ymm5, 24,  4)\
X(YMM5F7, 347, ymm5, 28,  4)\
X(YMM6F0, 348, ymm6,  0,  4)\
X(YMM6F1, 349, ymm6,  4,  4)\
X(YMM6F2, 350, ymm6,  8,  4)\
X(YMM6F3, 351, ymm6, 12,  4)\
X(YMM6F4, 352, ymm6, 16,  4)\
X(YMM6F5, 353, ymm6, 20,  4)\
X(YMM6F6, 354, ymm6, 24,  4)\
X(YMM6F7, 355, ymm6, 28,  4)\
X(YMM7F0, 356, ymm7,  0,  4)\
X(YMM7F1, 357, ymm7,  4,  4)\
X(YMM7F2, 358, ymm7,  8,  4)\
X(YMM7F3, 359, ymm7, 12,  4)\
X(YMM7F4, 360, ymm7, 16,  4)\
X(YMM7F5, 361, ymm7, 20,  4)\
X(YMM7F6, 362, ymm7, 24,  4)\
X(YMM7F7, 363, ymm7, 28,  4)\
X(YMM0D0, 364, ymm0,  0,  8)\
X(YMM0D1, 365, ymm0,  8,  8)\
X(YMM0D2, 366, ymm0, 16,  8)\
X(YMM0D3, 367, ymm0, 24,  8)\
X(YMM1D0, 368, ymm1,  0,  8)\
X(YMM1D1, 369, ymm1,  8,  8)\
X(YMM1D2, 370, ymm1, 16,  8)\
X(YMM1D3, 371, ymm1, 24,  8)\
X(YMM2D0, 372, ymm2,  0,  8)\
X(YMM2D1, 373, ymm2,  8,  8)\
X(YMM2D2, 374, ymm2, 16,  8)\
X(YMM2D3, 375, ymm2, 24,  8)\
X(YMM3D0, 376, ymm3,  0,  8)\
X(YMM3D1, 377, ymm3,  8,  8)\
X(YMM3D2, 378, ymm3, 16,  8)\
X(YMM3D3, 379, ymm3, 24,  8)\
X(YMM4D0, 380, ymm4,  0,  8)\
X(YMM4D1, 381, ymm4,  8,  8)\
X(YMM4D2, 382, ymm4, 16,  8)\
X(YMM4D3, 383, ymm4, 24,  8)\
X(YMM5D0, 384, ymm5,  0,  8)\
X(YMM5D1, 385, ymm5,  8,  8)\
X(YMM5D2, 386, ymm5, 16,  8)\
X(YMM5D3, 387, ymm5, 24,  8)\
X(YMM6D0, 388, ymm6,  0,  8)\
X(YMM6D1, 389, ymm6,  8,  8)\
X(YMM6D2, 390, ymm6, 16,  8)\
X(YMM6D3, 391, ymm6, 24,  8)\
X(YMM7D0, 392, ymm7,  0,  8)\
X(YMM7D1, 393, ymm7,  8,  8)\
X(YMM7D2, 394, ymm7, 16,  8)\
X(YMM7D3, 395, ymm7, 24,  8)

typedef U16 CV_Regx86;
typedef enum CV_Regx86Enum
{
#define X(CVN,C,RDN,BP,BZ) CV_Regx86_##CVN = C,
  CV_Reg_X86_XList(X)
#undef X
}
CV_Regx86Enum;

// X(NAME, CODE, (RDI_RegisterCode_X64) NAME, BYTE_POS, BYTE_SIZE)
#define CV_Reg_X64_XList(X) \
X(NONE,      0, nil, 0, 0)\
X(AL,        1, rax, 0, 1)\
X(CL,        2, rcx, 0, 1)\
X(DL,        3, rdx, 0, 1)\
X(BL,        4, rbx, 0, 1)\
X(AH,        5, rax, 1, 1)\
X(CH,        6, rcx, 1, 1)\
X(DH,        7, rdx, 1, 1)\
X(BH,        8, rbx, 1, 1)\
X(AX,        9, rax, 0, 2)\
X(CX,       10, rcx, 0, 2)\
X(DX,       11, rdx, 0, 2)\
X(BX,       12, rbx, 0, 2)\
X(SP,       13, rsp, 0, 2)\
X(BP,       14, rbp, 0, 2)\
X(SI,       15, rsi, 0, 2)\
X(DI,       16, rdi, 0, 2)\
X(EAX,      17, rax, 0, 4)\
X(ECX,      18, rcx, 0, 4)\
X(EDX,      19, rdx, 0, 4)\
X(EBX,      20, rbx, 0, 4)\
X(ESP,      21, rsp, 0, 4)\
X(EBP,      22, rbp, 0, 4)\
X(ESI,      23, rsi, 0, 4)\
X(EDI,      24, rdi, 0, 4)\
X(ES,       25, es,  0, 2)\
X(CS,       26, cs,  0, 2)\
X(SS,       27, ss,  0, 2)\
X(DS,       28, ds,  0, 2)\
X(FS,       29, fs,  0, 2)\
X(GS,       30, gs,  0, 2)\
X(FLAGS,    32, rflags, 0, 2)\
X(RIP,      33, rip,    0, 8)\
X(EFLAGS,   34, rflags, 0, 4)\
/* TODO: possibly missing control registers in x64 definitions? */ \
X(CR0,      80, nil, 0, 0)\
X(CR1,      81, nil, 0, 0)\
X(CR2,      82, nil, 0, 0)\
X(CR3,      83, nil, 0, 0)\
X(CR4,      84, nil, 0, 0)\
X(CR8,      88, nil, 0, 0)\
X(DR0,      90, dr0, 0, 4)\
X(DR1,      91, dr1, 0, 4)\
X(DR2,      92, dr2, 0, 4)\
X(DR3,      93, dr3, 0, 4)\
X(DR4,      94, dr4, 0, 4)\
X(DR5,      95, dr5, 0, 4)\
X(DR6,      96, dr6, 0, 4)\
X(DR7,      97, dr7, 0, 4)\
/* TODO: possibly missing debug registers 8-15 in x64 definitions? */ \
X(DR8,      98, nil, 0, 0)\
X(DR9,      99, nil, 0, 0)\
X(DR10,    100, nil, 0, 0)\
X(DR11,    101, nil, 0, 0)\
X(DR12,    102, nil, 0, 0)\
X(DR13,    103, nil, 0, 0)\
X(DR14,    104, nil, 0, 0)\
X(DR15,    105, nil, 0, 0)\
/* TODO: possibly missing ~whatever these are~ in x64 definitions? */ \
X(GDTR,    110, nil, 0, 0)\
X(GDTL,    111, nil, 0, 0)\
X(IDTR,    112, nil, 0, 0)\
X(IDTL,    113, nil, 0, 0)\
X(LDTR,    114, nil, 0, 0)\
X(TR,      115, nil, 0, 0)\
X(ST0,     128, st0, 0, 10)\
X(ST1,     129, st1, 0, 10)\
X(ST2,     130, st2, 0, 10)\
X(ST3,     131, st3, 0, 10)\
X(ST4,     132, st4, 0, 10)\
X(ST5,     133, st5, 0, 10)\
X(ST6,     134, st6, 0, 10)\
X(ST7,     135, st7, 0, 10)\
/* TODO: possibly missing these, or not sure how they map to our x64 definitions? */ \
X(CTRL,    136, nil, 0, 0)\
X(STAT,    137, nil, 0, 0)\
X(TAG,     138, nil, 0, 0)\
X(FPIP,    139, nil, 0, 0)\
X(FPCS,    140, nil, 0, 0)\
X(FPDO,    141, nil, 0, 0)\
X(FPDS,    142, nil, 0, 0)\
X(ISEM,    143, nil, 0, 0)\
X(FPEIP,   144, nil, 0, 0)\
X(FPEDO,   145, nil, 0, 0)\
X(MM0,     146, fpr0, 0, 8)\
X(MM1,     147, fpr1, 0, 8)\
X(MM2,     148, fpr2, 0, 8)\
X(MM3,     149, fpr3, 0, 8)\
X(MM4,     150, fpr4, 0, 8)\
X(MM5,     151, fpr5, 0, 8)\
X(MM6,     152, fpr6, 0, 8)\
X(MM7,     153, fpr7, 0, 8)\
X(XMM0,    154, ymm0,  0, 16)\
X(XMM1,    155, ymm1,  0, 16)\
X(XMM2,    156, ymm2,  0, 16)\
X(XMM3,    157, ymm3,  0, 16)\
X(XMM4,    158, ymm4,  0, 16)\
X(XMM5,    159, ymm5,  0, 16)\
X(XMM6,    160, ymm6,  0, 16)\
X(XMM7,    161, ymm7,  0, 16)\
X(XMM0_0,  162, ymm0,  0,  4)\
X(XMM0_1,  163, ymm0,  4,  4)\
X(XMM0_2,  164, ymm0,  8,  4)\
X(XMM0_3,  165, ymm0, 12,  4)\
X(XMM1_0,  166, ymm1,  0,  4)\
X(XMM1_1,  167, ymm1,  4,  4)\
X(XMM1_2,  168, ymm1,  8,  4)\
X(XMM1_3,  169, ymm1, 12,  4)\
X(XMM2_0,  170, ymm2,  0,  4)\
X(XMM2_1,  171, ymm2,  4,  4)\
X(XMM2_2,  172, ymm2,  8,  4)\
X(XMM2_3,  173, ymm2, 12,  4)\
X(XMM3_0,  174, ymm3,  0,  4)\
X(XMM3_1,  175, ymm3,  4,  4)\
X(XMM3_2,  176, ymm3,  8,  4)\
X(XMM3_3,  177, ymm3, 12,  4)\
X(XMM4_0,  178, ymm4,  0,  4)\
X(XMM4_1,  179, ymm4,  4,  4)\
X(XMM4_2,  180, ymm4,  8,  4)\
X(XMM4_3,  181, ymm4, 12,  4)\
X(XMM5_0,  182, ymm5,  0,  4)\
X(XMM5_1,  183, ymm5,  4,  4)\
X(XMM5_2,  184, ymm5,  8,  4)\
X(XMM5_3,  185, ymm5, 12,  4)\
X(XMM6_0,  186, ymm6,  0,  4)\
X(XMM6_1,  187, ymm6,  4,  4)\
X(XMM6_2,  188, ymm6,  8,  4)\
X(XMM6_3,  189, ymm6, 12,  4)\
X(XMM7_0,  190, ymm7,  0,  4)\
X(XMM7_1,  191, ymm7,  4,  4)\
X(XMM7_2,  192, ymm7,  8,  4)\
X(XMM7_3,  193, ymm7, 12,  4)\
X(XMM0L,   194, ymm0,  0,  8)\
X(XMM1L,   195, ymm1,  0,  8)\
X(XMM2L,   196, ymm2,  0,  8)\
X(XMM3L,   197, ymm3,  0,  8)\
X(XMM4L,   198, ymm4,  0,  8)\
X(XMM5L,   199, ymm5,  0,  8)\
X(XMM6L,   200, ymm6,  0,  8)\
X(XMM7L,   201, ymm7,  0,  8)\
X(XMM0H,   202, ymm0,  8,  8)\
X(XMM1H,   203, ymm1,  8,  8)\
X(XMM2H,   204, ymm2,  8,  8)\
X(XMM3H,   205, ymm3,  8,  8)\
X(XMM4H,   206, ymm4,  8,  8)\
X(XMM5H,   207, ymm5,  8,  8)\
X(XMM6H,   208, ymm6,  8,  8)\
X(XMM7H,   209, ymm7,  8,  8)\
X(MXCSR,   211, mxcsr, 0,  4)\
X(EMM0L,   220, ymm0,  0,  8)\
X(EMM1L,   221, ymm1,  0,  8)\
X(EMM2L,   222, ymm2,  0,  8)\
X(EMM3L,   223, ymm3,  0,  8)\
X(EMM4L,   224, ymm4,  0,  8)\
X(EMM5L,   225, ymm5,  0,  8)\
X(EMM6L,   226, ymm6,  0,  8)\
X(EMM7L,   227, ymm7,  0,  8)\
X(EMM0H,   228, ymm0,  8,  8)\
X(EMM1H,   229, ymm1,  8,  8)\
X(EMM2H,   230, ymm2,  8,  8)\
X(EMM3H,   231, ymm3,  8,  8)\
X(EMM4H,   232, ymm4,  8,  8)\
X(EMM5H,   233, ymm5,  8,  8)\
X(EMM6H,   234, ymm6,  8,  8)\
X(EMM7H,   235, ymm7,  8,  8)\
X(MM00,    236, fpr0,  0,  4)\
X(MM01,    237, fpr0,  4,  4)\
X(MM10,    238, fpr1,  0,  4)\
X(MM11,    239, fpr1,  4,  4)\
X(MM20,    240, fpr2,  0,  4)\
X(MM21,    241, fpr2,  4,  4)\
X(MM30,    242, fpr3,  0,  4)\
X(MM31,    243, fpr3,  4,  4)\
X(MM40,    244, fpr4,  0,  4)\
X(MM41,    245, fpr4,  4,  4)\
X(MM50,    246, fpr5,  0,  4)\
X(MM51,    247, fpr5,  4,  4)\
X(MM60,    248, fpr6,  0,  4)\
X(MM61,    249, fpr6,  4,  4)\
X(MM70,    250, fpr7,  0,  4)\
X(MM71,    251, fpr7,  4,  4)\
X(XMM8,    252, ymm8,   0, 16)\
X(XMM9,    253, ymm9,   0, 16)\
X(XMM10,   254, ymm10,  0, 16)\
X(XMM11,   255, ymm11,  0, 16)\
X(XMM12,   256, ymm12,  0, 16)\
X(XMM13,   257, ymm13,  0, 16)\
X(XMM14,   258, ymm14,  0, 16)\
X(XMM15,   259, ymm15,  0, 16)\
X(XMM8_0,  260, ymm8,   0, 16)\
X(XMM8_1,  261, ymm8,   4, 16)\
X(XMM8_2,  262, ymm8,   8, 16)\
X(XMM8_3,  263, ymm8,  12, 16)\
X(XMM9_0,  264, ymm9,   0,  4)\
X(XMM9_1,  265, ymm9,   4,  4)\
X(XMM9_2,  266, ymm9,   8,  4)\
X(XMM9_3,  267, ymm9,  12,  4)\
X(XMM10_0, 268, ymm10,  0,  4)\
X(XMM10_1, 269, ymm10,  4,  4)\
X(XMM10_2, 270, ymm10,  8,  4)\
X(XMM10_3, 271, ymm10, 12,  4)\
X(XMM11_0, 272, ymm11,  0,  4)\
X(XMM11_1, 273, ymm11,  4,  4)\
X(XMM11_2, 274, ymm11,  8,  4)\
X(XMM11_3, 275, ymm11, 12,  4)\
X(XMM12_0, 276, ymm12,  0,  4)\
X(XMM12_1, 277, ymm12,  4,  4)\
X(XMM12_2, 278, ymm12,  8,  4)\
X(XMM12_3, 279, ymm12, 12,  4)\
X(XMM13_0, 280, ymm13,  0,  4)\
X(XMM13_1, 281, ymm13,  4,  4)\
X(XMM13_2, 282, ymm13,  8,  4)\
X(XMM13_3, 283, ymm13, 12,  4)\
X(XMM14_0, 284, ymm14,  0,  4)\
X(XMM14_1, 285, ymm14,  4,  4)\
X(XMM14_2, 286, ymm14,  8,  4)\
X(XMM14_3, 287, ymm14, 12,  4)\
X(XMM15_0, 288, ymm15,  0,  4)\
X(XMM15_1, 289, ymm15,  4,  4)\
X(XMM15_2, 290, ymm15,  8,  4)\
X(XMM15_3, 291, ymm15, 12,  4)\
X(XMM8L,   292, ymm8,   0,  8)\
X(XMM9L,   293, ymm9,   0,  8)\
X(XMM10L,  294, ymm10,  0,  8)\
X(XMM11L,  295, ymm11,  0,  8)\
X(XMM12L,  296, ymm12,  0,  8)\
X(XMM13L,  297, ymm13,  0,  8)\
X(XMM14L,  298, ymm14,  0,  8)\
X(XMM15L,  299, ymm15,  0,  8)\
X(XMM8H,   300, ymm8,   8,  8)\
X(XMM9H,   301, ymm9,   8,  8)\
X(XMM10H,  302, ymm10,  8,  8)\
X(XMM11H,  303, ymm11,  8,  8)\
X(XMM12H,  304, ymm12,  8,  8)\
X(XMM13H,  305, ymm13,  8,  8)\
X(XMM14H,  306, ymm14,  8,  8)\
X(XMM15H,  307, ymm15,  8,  8)\
X(EMM8L,   308, ymm8,   0,  8)\
X(EMM9L,   309, ymm9,   0,  8)\
X(EMM10L,  310, ymm10,  0,  8)\
X(EMM11L,  311, ymm11,  0,  8)\
X(EMM12L,  312, ymm12,  0,  8)\
X(EMM13L,  313, ymm13,  0,  8)\
X(EMM14L,  314, ymm14,  0,  8)\
X(EMM15L,  315, ymm15,  0,  8)\
X(EMM8H,   316, ymm8,   8,  8)\
X(EMM9H,   317, ymm9,   8,  8)\
X(EMM10H,  318, ymm10,  8,  8)\
X(EMM11H,  319, ymm11,  8,  8)\
X(EMM12H,  320, ymm12,  8,  8)\
X(EMM13H,  321, ymm13,  8,  8)\
X(EMM14H,  322, ymm14,  8,  8)\
X(EMM15H,  323, ymm15,  8,  8)\
X(SIL,     324, rsi, 0, 1)\
X(DIL,     325, rdi, 0, 1)\
X(BPL,     326, rbp, 0, 1)\
X(SPL,     327, rsp, 0, 1)\
X(RAX,     328, rax, 0, 8)\
X(RBX,     329, rbx, 0, 8)\
X(RCX,     330, rcx, 0, 8)\
X(RDX,     331, rdx, 0, 8)\
X(RSI,     332, rsi, 0, 8)\
X(RDI,     333, rdi, 0, 8)\
X(RBP,     334, rbp, 0, 8)\
X(RSP,     335, rsp, 0, 8)\
X(R8,      336, r8,  0, 8)\
X(R9,      337, r9,  0, 8)\
X(R10,     338, r10, 0, 8)\
X(R11,     339, r11, 0, 8)\
X(R12,     340, r12, 0, 8)\
X(R13,     341, r13, 0, 8)\
X(R14,     342, r14, 0, 8)\
X(R15,     343, r15, 0, 8)\
X(R8B,     344, r8,  0, 1)\
X(R9B,     345, r9,  0, 1)\
X(R10B,    346, r10, 0, 1)\
X(R11B,    347, r11, 0, 1)\
X(R12B,    348, r12, 0, 1)\
X(R13B,    349, r13, 0, 1)\
X(R14B,    350, r14, 0, 1)\
X(R15B,    351, r15, 0, 1)\
X(R8W,     352, r8,  0, 2)\
X(R9W,     353, r9,  0, 2)\
X(R10W,    354, r10, 0, 2)\
X(R11W,    355, r11, 0, 2)\
X(R12W,    356, r12, 0, 2)\
X(R13W,    357, r13, 0, 2)\
X(R14W,    358, r14, 0, 2)\
X(R15W,    359, r15, 0, 2)\
X(R8D,     360, r8,  0, 4)\
X(R9D,     361, r9,  0, 4)\
X(R10D,    362, r10, 0, 4)\
X(R11D,    363, r11, 0, 4)\
X(R12D,    364, r12, 0, 4)\
X(R13D,    365, r13, 0, 4)\
X(R14D,    366, r14, 0, 4)\
X(R15D,    367, r15, 0, 4)\
X(YMM0,    368, ymm0,   0, 32)\
X(YMM1,    369, ymm1,   0, 32)\
X(YMM2,    370, ymm2,   0, 32)\
X(YMM3,    371, ymm3,   0, 32)\
X(YMM4,    372, ymm4,   0, 32)\
X(YMM5,    373, ymm5,   0, 32)\
X(YMM6,    374, ymm6,   0, 32)\
X(YMM7,    375, ymm7,   0, 32)\
X(YMM8,    376, ymm8,   0, 32)\
X(YMM9,    377, ymm9,   0, 32)\
X(YMM10,   378, ymm10,  0, 32)\
X(YMM11,   379, ymm11,  0, 32)\
X(YMM12,   380, ymm12,  0, 32)\
X(YMM13,   381, ymm13,  0, 32)\
X(YMM14,   382, ymm14,  0, 32)\
X(YMM15,   383, ymm15,  0, 32)\
X(YMM0H,   384, ymm0,  16, 32)\
X(YMM1H,   385, ymm1,  16, 32)\
X(YMM2H,   386, ymm2,  16, 32)\
X(YMM3H,   387, ymm3,  16, 32)\
X(YMM4H,   388, ymm4,  16, 32)\
X(YMM5H,   389, ymm5,  16, 32)\
X(YMM6H,   390, ymm6,  16, 32)\
X(YMM7H,   391, ymm7,  16, 32)\
X(YMM8H,   392, ymm8,  16, 32)\
X(YMM9H,   393, ymm9,  16, 32)\
X(YMM10H,  394, ymm10, 16, 32)\
X(YMM11H,  395, ymm11, 16, 32)\
X(YMM12H,  396, ymm12, 16, 32)\
X(YMM13H,  397, ymm13, 16, 32)\
X(YMM14H,  398, ymm14, 16, 32)\
X(YMM15H,  399, ymm15, 16, 32)\
X(XMM0IL,  400, ymm0,   0,  8)\
X(XMM1IL,  401, ymm1,   0,  8)\
X(XMM2IL,  402, ymm2,   0,  8)\
X(XMM3IL,  403, ymm3,   0,  8)\
X(XMM4IL,  404, ymm4,   0,  8)\
X(XMM5IL,  405, ymm5,   0,  8)\
X(XMM6IL,  406, ymm6,   0,  8)\
X(XMM7IL,  407, ymm7,   0,  8)\
X(XMM8IL,  408, ymm8,   0,  8)\
X(XMM9IL,  409, ymm9,   0,  8)\
X(XMM10IL, 410, ymm10,  0,  8)\
X(XMM11IL, 411, ymm11,  0,  8)\
X(XMM12IL, 412, ymm12,  0,  8)\
X(XMM13IL, 413, ymm13,  0,  8)\
X(XMM14IL, 414, ymm14,  0,  8)\
X(XMM15IL, 415, ymm15,  0,  8)\
X(XMM0IH,  416, ymm0,   8,  8)\
X(XMM1IH,  417, ymm1,   8,  8)\
X(XMM2IH,  418, ymm2,   8,  8)\
X(XMM3IH,  419, ymm3,   8,  8)\
X(XMM4IH,  420, ymm4,   8,  8)\
X(XMM5IH,  421, ymm5,   8,  8)\
X(XMM6IH,  422, ymm6,   8,  8)\
X(XMM7IH,  423, ymm7,   8,  8)\
X(XMM8IH,  424, ymm8,   8,  8)\
X(XMM9IH,  425, ymm9,   8,  8)\
X(XMM10IH, 426, ymm10,  8,  8)\
X(XMM11IH, 427, ymm11,  8,  8)\
X(XMM12IH, 428, ymm12,  8,  8)\
X(XMM13IH, 429, ymm13,  8,  8)\
X(XMM14IH, 430, ymm14,  8,  8)\
X(XMM15IH, 431, ymm15,  8,  8)\
X(YMM0I0,  432, ymm0,   0,  8)\
X(YMM0I1,  433, ymm0,   8,  8)\
X(YMM0I2,  434, ymm0,  16,  8)\
X(YMM0I3,  435, ymm0,  24,  8)\
X(YMM1I0,  436, ymm1,   0,  8)\
X(YMM1I1,  437, ymm1,   8,  8)\
X(YMM1I2,  438, ymm1,  16,  8)\
X(YMM1I3,  439, ymm1,  24,  8)\
X(YMM2I0,  440, ymm2,   0,  8)\
X(YMM2I1,  441, ymm2,   8,  8)\
X(YMM2I2,  442, ymm2,  16,  8)\
X(YMM2I3,  443, ymm2,  24,  8)\
X(YMM3I0,  444, ymm3,   0,  8)\
X(YMM3I1,  445, ymm3,   8,  8)\
X(YMM3I2,  446, ymm3,  16,  8)\
X(YMM3I3,  447, ymm3,  24,  8)\
X(YMM4I0,  448, ymm4,   0,  8)\
X(YMM4I1,  449, ymm4,   8,  8)\
X(YMM4I2,  450, ymm4,  16,  8)\
X(YMM4I3,  451, ymm4,  24,  8)\
X(YMM5I0,  452, ymm5,   0,  8)\
X(YMM5I1,  453, ymm5,   8,  8)\
X(YMM5I2,  454, ymm5,  16,  8)\
X(YMM5I3,  455, ymm5,  24,  8)\
X(YMM6I0,  456, ymm6,   0,  8)\
X(YMM6I1,  457, ymm6,   8,  8)\
X(YMM6I2,  458, ymm6,  16,  8)\
X(YMM6I3,  459, ymm6,  24,  8)\
X(YMM7I0,  460, ymm7,   0,  8)\
X(YMM7I1,  461, ymm7,   8,  8)\
X(YMM7I2,  462, ymm7,  16,  8)\
X(YMM7I3,  463, ymm7,  24,  8)\
X(YMM8I0,  464, ymm8,   0,  8)\
X(YMM8I1,  465, ymm8,   8,  8)\
X(YMM8I2,  466, ymm8,  16,  8)\
X(YMM8I3,  467, ymm8,  24,  8)\
X(YMM9I0,  468, ymm9,   0,  8)\
X(YMM9I1,  469, ymm9,   8,  8)\
X(YMM9I2,  470, ymm9,  16,  8)\
X(YMM9I3,  471, ymm9,  24,  8)\
X(YMM10I0, 472, ymm10,  0,  8)\
X(YMM10I1, 473, ymm10,  8,  8)\
X(YMM10I2, 474, ymm10, 16,  8)\
X(YMM10I3, 475, ymm10, 24,  8)\
X(YMM11I0, 476, ymm11,  0,  8)\
X(YMM11I1, 477, ymm11,  8,  8)\
X(YMM11I2, 478, ymm11, 16,  8)\
X(YMM11I3, 479, ymm11, 24,  8)\
X(YMM12I0, 480, ymm12,  0,  8)\
X(YMM12I1, 481, ymm12,  8,  8)\
X(YMM12I2, 482, ymm12, 16,  8)\
X(YMM12I3, 483, ymm12, 24,  8)\
X(YMM13I0, 484, ymm13,  0,  8)\
X(YMM13I1, 485, ymm13,  8,  8)\
X(YMM13I2, 486, ymm13, 16,  8)\
X(YMM13I3, 487, ymm13, 24,  8)\
X(YMM14I0, 488, ymm14,  0,  8)\
X(YMM14I1, 489, ymm14,  8,  8)\
X(YMM14I2, 490, ymm14, 16,  8)\
X(YMM14I3, 491, ymm14, 24,  8)\
X(YMM15I0, 492, ymm15,  0,  8)\
X(YMM15I1, 493, ymm15,  8,  8)\
X(YMM15I2, 494, ymm15, 16,  8)\
X(YMM15I3, 495, ymm15, 24,  8)\
X(YMM0F0,  496, ymm0,   0,  4)\
X(YMM0F1,  497, ymm0,   4,  4)\
X(YMM0F2,  498, ymm0,   8,  4)\
X(YMM0F3,  499, ymm0,  12,  4)\
X(YMM0F4,  500, ymm0,  16,  4)\
X(YMM0F5,  501, ymm0,  20,  4)\
X(YMM0F6,  502, ymm0,  24,  4)\
X(YMM0F7,  503, ymm0,  28,  4)\
X(YMM1F0,  504, ymm1,   0,  4)\
X(YMM1F1,  505, ymm1,   4,  4)\
X(YMM1F2,  506, ymm1,   8,  4)\
X(YMM1F3,  507, ymm1,  12,  4)\
X(YMM1F4,  508, ymm1,  16,  4)\
X(YMM1F5,  509, ymm1,  20,  4)\
X(YMM1F6,  510, ymm1,  24,  4)\
X(YMM1F7,  511, ymm1,  28,  4)\
X(YMM2F0,  512, ymm2,   0,  4)\
X(YMM2F1,  513, ymm2,   4,  4)\
X(YMM2F2,  514, ymm2,   8,  4)\
X(YMM2F3,  515, ymm2,  12,  4)\
X(YMM2F4,  516, ymm2,  16,  4)\
X(YMM2F5,  517, ymm2,  20,  4)\
X(YMM2F6,  518, ymm2,  24,  4)\
X(YMM2F7,  519, ymm2,  28,  4)\
X(YMM3F0,  520, ymm3,   0,  4)\
X(YMM3F1,  521, ymm3,   4,  4)\
X(YMM3F2,  522, ymm3,   8,  4)\
X(YMM3F3,  523, ymm3,  12,  4)\
X(YMM3F4,  524, ymm3,  16,  4)\
X(YMM3F5,  525, ymm3,  20,  4)\
X(YMM3F6,  526, ymm3,  24,  4)\
X(YMM3F7,  527, ymm3,  28,  4)\
X(YMM4F0,  528, ymm4,   0,  4)\
X(YMM4F1,  529, ymm4,   4,  4)\
X(YMM4F2,  530, ymm4,   8,  4)\
X(YMM4F3,  531, ymm4,  12,  4)\
X(YMM4F4,  532, ymm4,  16,  4)\
X(YMM4F5,  533, ymm4,  20,  4)\
X(YMM4F6,  534, ymm4,  24,  4)\
X(YMM4F7,  535, ymm4,  28,  4)\
X(YMM5F0,  536, ymm5,   0,  4)\
X(YMM5F1,  537, ymm5,   4,  4)\
X(YMM5F2,  538, ymm5,   8,  4)\
X(YMM5F3,  539, ymm5,  12,  4)\
X(YMM5F4,  540, ymm5,  16,  4)\
X(YMM5F5,  541, ymm5,  20,  4)\
X(YMM5F6,  542, ymm5,  24,  4)\
X(YMM5F7,  543, ymm5,  28,  4)\
X(YMM6F0,  544, ymm6,   0,  4)\
X(YMM6F1,  545, ymm6,   4,  4)\
X(YMM6F2,  546, ymm6,   8,  4)\
X(YMM6F3,  547, ymm6,  12,  4)\
X(YMM6F4,  548, ymm6,  16,  4)\
X(YMM6F5,  549, ymm6,  20,  4)\
X(YMM6F6,  550, ymm6,  24,  4)\
X(YMM6F7,  551, ymm6,  28,  4)\
X(YMM7F0,  552, ymm7,   0,  4)\
X(YMM7F1,  553, ymm7,   4,  4)\
X(YMM7F2,  554, ymm7,   8,  4)\
X(YMM7F3,  555, ymm7,  12,  4)\
X(YMM7F4,  556, ymm7,  16,  4)\
X(YMM7F5,  557, ymm7,  20,  4)\
X(YMM7F6,  558, ymm7,  24,  4)\
X(YMM7F7,  559, ymm7,  28,  4)\
X(YMM8F0,  560, ymm8,   0,  4)\
X(YMM8F1,  561, ymm8,   4,  4)\
X(YMM8F2,  562, ymm8,   8,  4)\
X(YMM8F3,  563, ymm8,  12,  4)\
X(YMM8F4,  564, ymm8,  16,  4)\
X(YMM8F5,  565, ymm8,  20,  4)\
X(YMM8F6,  566, ymm8,  24,  4)\
X(YMM8F7,  567, ymm8,  28,  4)\
X(YMM9F0,  568, ymm9,   0,  4)\
X(YMM9F1,  569, ymm9,   4,  4)\
X(YMM9F2,  570, ymm9,   8,  4)\
X(YMM9F3,  571, ymm9,  12,  4)\
X(YMM9F4,  572, ymm9,  16,  4)\
X(YMM9F5,  573, ymm9,  20,  4)\
X(YMM9F6,  574, ymm9,  24,  4)\
X(YMM9F7,  575, ymm9,  28,  4)\
X(YMM10F0, 576, ymm10,  0,  4)\
X(YMM10F1, 577, ymm10,  4,  4)\
X(YMM10F2, 578, ymm10,  8,  4)\
X(YMM10F3, 579, ymm10, 12,  4)\
X(YMM10F4, 580, ymm10, 16,  4)\
X(YMM10F5, 581, ymm10, 20,  4)\
X(YMM10F6, 582, ymm10, 24,  4)\
X(YMM10F7, 583, ymm10, 28,  4)\
X(YMM11F0, 584, ymm11,  0,  4)\
X(YMM11F1, 585, ymm11,  4,  4)\
X(YMM11F2, 586, ymm11,  8,  4)\
X(YMM11F3, 587, ymm11, 12,  4)\
X(YMM11F4, 588, ymm11, 16,  4)\
X(YMM11F5, 589, ymm11, 20,  4)\
X(YMM11F6, 590, ymm11, 24,  4)\
X(YMM11F7, 591, ymm11, 28,  4)\
X(YMM12F0, 592, ymm12,  0,  4)\
X(YMM12F1, 593, ymm12,  4,  4)\
X(YMM12F2, 594, ymm12,  8,  4)\
X(YMM12F3, 595, ymm12, 12,  4)\
X(YMM12F4, 596, ymm12, 16,  4)\
X(YMM12F5, 597, ymm12, 20,  4)\
X(YMM12F6, 598, ymm12, 24,  4)\
X(YMM12F7, 599, ymm12, 28,  4)\
X(YMM13F0, 600, ymm13,  0,  4)\
X(YMM13F1, 601, ymm13,  4,  4)\
X(YMM13F2, 602, ymm13,  8,  4)\
X(YMM13F3, 603, ymm13, 12,  4)\
X(YMM13F4, 604, ymm13, 16,  4)\
X(YMM13F5, 605, ymm13, 20,  4)\
X(YMM13F6, 606, ymm13, 24,  4)\
X(YMM13F7, 607, ymm13, 28,  4)\
X(YMM14F0, 608, ymm14,  0,  4)\
X(YMM14F1, 609, ymm14,  4,  4)\
X(YMM14F2, 610, ymm14,  8,  4)\
X(YMM14F3, 611, ymm14, 12,  4)\
X(YMM14F4, 612, ymm14, 16,  4)\
X(YMM14F5, 613, ymm14, 20,  4)\
X(YMM14F6, 614, ymm14, 24,  4)\
X(YMM14F7, 615, ymm14, 28,  4)\
X(YMM15F0, 616, ymm15,  0,  4)\
X(YMM15F1, 617, ymm15,  4,  4)\
X(YMM15F2, 618, ymm15,  8,  4)\
X(YMM15F3, 619, ymm15, 12,  4)\
X(YMM15F4, 620, ymm15, 16,  4)\
X(YMM15F5, 621, ymm15, 20,  4)\
X(YMM15F6, 622, ymm15, 24,  4)\
X(YMM15F7, 623, ymm15, 28,  4)\
X(YMM0D0,  624, ymm0,   0,  8)\
X(YMM0D1,  625, ymm0,   8,  8)\
X(YMM0D2,  626, ymm0,  16,  8)\
X(YMM0D3,  627, ymm0,  24,  8)\
X(YMM1D0,  628, ymm1,   0,  8)\
X(YMM1D1,  629, ymm1,   8,  8)\
X(YMM1D2,  630, ymm1,  16,  8)\
X(YMM1D3,  631, ymm1,  24,  8)\
X(YMM2D0,  632, ymm2,   0,  8)\
X(YMM2D1,  633, ymm2,   8,  8)\
X(YMM2D2,  634, ymm2,  16,  8)\
X(YMM2D3,  635, ymm2,  24,  8)\
X(YMM3D0,  636, ymm3,   0,  8)\
X(YMM3D1,  637, ymm3,   8,  8)\
X(YMM3D2,  638, ymm3,  16,  8)\
X(YMM3D3,  639, ymm3,  24,  8)\
X(YMM4D0,  640, ymm4,   0,  8)\
X(YMM4D1,  641, ymm4,   8,  8)\
X(YMM4D2,  642, ymm4,  16,  8)\
X(YMM4D3,  643, ymm4,  24,  8)\
X(YMM5D0,  644, ymm5,   0,  8)\
X(YMM5D1,  645, ymm5,   8,  8)\
X(YMM5D2,  646, ymm5,  16,  8)\
X(YMM5D3,  647, ymm5,  24,  8)\
X(YMM6D0,  648, ymm6,   0,  8)\
X(YMM6D1,  649, ymm6,   8,  8)\
X(YMM6D2,  650, ymm6,  16,  8)\
X(YMM6D3,  651, ymm6,  24,  8)\
X(YMM7D0,  652, ymm7,   0,  8)\
X(YMM7D1,  653, ymm7,   8,  8)\
X(YMM7D2,  654, ymm7,  16,  8)\
X(YMM7D3,  655, ymm7,  24,  8)\
X(YMM8D0,  656, ymm8,   0,  8)\
X(YMM8D1,  657, ymm8,   8,  8)\
X(YMM8D2,  658, ymm8,  16,  8)\
X(YMM8D3,  659, ymm8,  24,  8)\
X(YMM9D0,  660, ymm9,   0,  8)\
X(YMM9D1,  661, ymm9,   8,  8)\
X(YMM9D2,  662, ymm9,  16,  8)\
X(YMM9D3,  663, ymm9,  24,  8)\
X(YMM10D0, 664, ymm10,  0,  8)\
X(YMM10D1, 665, ymm10,  8,  8)\
X(YMM10D2, 666, ymm10, 16,  8)\
X(YMM10D3, 667, ymm10, 24,  8)\
X(YMM11D0, 668, ymm11,  0,  8)\
X(YMM11D1, 669, ymm11,  8,  8)\
X(YMM11D2, 670, ymm11, 16,  8)\
X(YMM11D3, 671, ymm11, 24,  8)\
X(YMM12D0, 672, ymm12,  0,  8)\
X(YMM12D1, 673, ymm12,  8,  8)\
X(YMM12D2, 674, ymm12, 16,  8)\
X(YMM12D3, 675, ymm12, 24,  8)\
X(YMM13D0, 676, ymm13,  0,  8)\
X(YMM13D1, 677, ymm13,  8,  8)\
X(YMM13D2, 678, ymm13, 16,  8)\
X(YMM13D3, 679, ymm13, 24,  8)\
X(YMM14D0, 680, ymm14,  0,  8)\
X(YMM14D1, 681, ymm14,  8,  8)\
X(YMM14D2, 682, ymm14, 16,  8)\
X(YMM14D3, 683, ymm14, 24,  8)\
X(YMM15D0, 684, ymm15,  0,  8)\
X(YMM15D1, 685, ymm15,  8,  8)\
X(YMM15D2, 686, ymm15, 16,  8)\
X(YMM15D3, 687, ymm15, 24,  8)

typedef U16 CV_Regx64;
typedef enum CV_Regx64Enum
{
#define X(CVN,C,RDN,BP,BZ) CV_Regx64_##CVN = C,
  CV_Reg_X64_XList(X)
#undef X
}
CV_Regx64Enum;


#define CV_SignatureXList(X) \
X(C6,  0)\
X(C7,  1)\
X(C11, 2)\
X(C13, 4)\
X(RESERVED, 5)

typedef U16 CV_Signature;
typedef enum CV_SignatureEnum
{
#define X(N,c) CV_Signature_##N = c,
  CV_SignatureXList(X)
#undef X
}
CV_SignatureEnum;


#define CV_LanguageXList(X) \
X(C,       0x00)\
X(CXX,     0x01)\
X(FORTRAN, 0x02)\
X(MASM,    0x03)\
X(PASCAL,  0x04)\
X(BASIC,   0x05)\
X(COBOL,   0x06)\
X(LINK,    0x07)\
X(CVTRES,  0x08)\
X(CVTPGD,  0x09)\
X(CSHARP,  0x0A)\
X(VB,      0x0B)\
X(ILASM,   0x0C)\
X(JAVA,    0x0D)\
X(JSCRIPT, 0x0E)\
X(MSIL,    0x0F)\
X(HLSL,    0x10)

typedef U16 CV_Language;
typedef enum CV_LanguageEnum
{
#define X(N,c) CV_Language_##N = c,
  CV_LanguageXList(X)
#undef X
}
CV_LanguageEnum;

////////////////////////////////
//~ rjf: "Sym" and "Leaf" Header Type

typedef struct CV_RecHeader CV_RecHeader;
struct CV_RecHeader
{
  U16 size;
  U16 kind;
};

////////////////////////////////
//~ rjf: "Sym" Types
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
  U32 off;
  U16 seg;
  CV_LocalFlags flags;
};

//- (SymKind: COMPILE)

typedef U32 CV_CompileFlags;
#define CV_CompileFlags_ExtractLanguage(f)    (((f)    )&0xFF)
#define CV_CompileFlags_ExtractFloatPrec(f)   (((f)>> 8)&0x03)
#define CV_CompileFlags_ExtractFloatPkg(f)    (((f)>>10)&0x03)
#define CV_CompileFlags_ExtractAmbientData(f) (((f)>>12)&0x07)
#define CV_CompileFlags_ExtractAmbientCode(f) (((f)>>15)&0x07)
#define CV_CompileFlags_ExtractMode(f)        (((f)>>18)&0x01)

typedef struct CV_SymCompile CV_SymCompile;
struct CV_SymCompile
{
  U8 machine;
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
  CV_GenericStyle_REG,  //  "return data is in register"
  CV_GenericStyle_ICAN, //  "indirect caller allocated near"
  CV_GenericStyle_ICAF, //  "indirect caller allocated far"
  CV_GenericStyle_IRAN, //  "indirect returnee allocated near"
  CV_GenericStyle_IRAF, //  "indirect returnee allocated far"
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
  COFF_Guid id;
  CV_TypeId itype;
  //  padding align(4)
};

//- (SymKind: VFTABLE32)

typedef struct CV_SymVPath32 CV_SymVPath32;
struct CV_SymVPath32
{
  CV_TypeId root;
  CV_TypeId path;
  U32 off;
  U16 seg;
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
  CV_FrameprocFlag_UsesAlloca  = (1 << 0),
  CV_FrameprocFlag_UsesSetJmp  = (1 << 1),
  CV_FrameprocFlag_UsesLongJmp = (1 << 2),
  CV_FrameprocFlag_UsesInlAsm  = (1 << 3),
  CV_FrameprocFlag_UsesEH      = (1 << 4),
  CV_FrameprocFlag_Inline      = (1 << 5),
  CV_FrameprocFlag_HasSEH      = (1 << 6),
  CV_FrameprocFlag_Naked       = (1 << 7),
  CV_FrameprocFlag_HasSecurityChecks = (1 << 8),
  CV_FrameprocFlag_AsyncEH     = (1 << 9),
  CV_FrameprocFlag_GSNoStackOrdering = (1 << 10),
  CV_FrameprocFlag_WasInlined  = (1 << 11),
  CV_FrameprocFlag_GSCheck     = (1 << 12),
  CV_FrameprocFlag_SafeBuffers = (1 << 13),
  // LocalBasePointer: 14,15
  // ParamBasePointer: 16,17
  CV_FrameprocFlag_PogoOn      = (1 << 18),
  CV_FrameprocFlag_PogoCountsValid = (1 << 19),
  CV_FrameprocFlag_OptSpeed    = (1 << 20),
  CV_FrameprocFlag_HasCFG      = (1 << 21),
  CV_FrameprocFlag_HasCFW      = (1 << 22),
};
#define CV_FrameprocFlags_ExtractLocalBasePointer(f) (((f) >> 14)&3)
#define CV_FrameprocFlags_ExtractParamBasePointer(f) (((f) >> 16)&3)

typedef struct CV_SymFrameproc CV_SymFrameproc;
struct CV_SymFrameproc
{
  U32 frame_size;
  U32 pad_size;
  U32 pad_off;
  U32 save_reg_size;
  U32 eh_off;
  CV_SectionIndex eh_sec;
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

typedef struct CV_SymObjname CV_SymObjname;
struct CV_SymObjname
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
  U32 parent;
  U32 end;
  U32 next;
  U32 off;
  U16 sec;
  U16 len;
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
  U32 off;
  U16 sec;
  CV_ProcFlags flags;
  // U8[] name (null terminated)
};

//- (SymKind: REGISTER)

typedef struct CV_SymRegister CV_SymRegister;
struct CV_SymRegister
{
  CV_TypeId itype;
  U16 reg;
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
  U8 count;
  // U8[count] regs;
};

//- (SymKind: BPREL32)

typedef struct CV_SymBPRel32 CV_SymBPRel32;
struct CV_SymBPRel32
{
  U32 off;
  CV_TypeId itype;
  // U8[] name (null terminated)
};

//- (SymKind: LDATA32, GDATA32)

typedef struct CV_SymData32 CV_SymData32;
struct CV_SymData32
{
  CV_TypeId itype;
  U32 off;
  CV_SectionIndex sec;
  // U8[] name (null terminated)
};

//- (SymKind: PUB32)

typedef U32 CV_PubFlags;
enum
{
  CV_PubFlag_Code        = (1 << 0),
  CV_PubFlag_Function    = (1 << 1),
  CV_PubFlag_ManagedCode = (1 << 2),
  CV_PubFlag_MSIL        = (1 << 3),
};

typedef struct CV_SymPub32 CV_SymPub32;
struct CV_SymPub32
{
  CV_PubFlags flags;
  U32 off;
  CV_SectionIndex sec;
  // U8[] name (null terminated)
};

//- (SymKind: LPROC32, GPROC32)

typedef struct CV_SymProc32 CV_SymProc32;
struct CV_SymProc32
{
  U32 parent;
  U32 end;
  U32 next;
  U32 len;
  U32 dbg_start;
  U32 dbg_end;
  CV_TypeId itype;
  U32 off;
  U16 sec;
  CV_ProcFlags flags;
  // U8[] name (null terminated)
};

//- (SymKind: REGREL32)

typedef struct CV_SymRegrel32 CV_SymRegrel32;
struct CV_SymRegrel32
{
  U32 reg_off;
  CV_TypeId itype;
  CV_Reg reg;
  // U8[] name (null terminated)
};

//- (SymKind: LTHREAD32, GTHREAD32)

typedef struct CV_SymThread32 CV_SymThread32;
struct CV_SymThread32
{
  CV_TypeId itype;
  U32 tls_off;
  U16 tls_seg;
  // U8[] name (null terminated)
};

//- (SymKind: COMPILE2)

typedef U32 CV_Compile2Flags;
#define CV_Compile2Flags_ExtractLanguage(f)        (((f)    )&0xFF)
#define CV_Compile2Flags_ExtractEditAndContinue(f) (((f)>> 8)&0x01)
#define CV_Compile2Flags_ExtractNoDbgInfo(f)       (((f)>> 9)&0x01)
#define CV_Compile2Flags_ExtractLTCG(f)            (((f)>>10)&0x01)
#define CV_Compile2Flags_ExtractNoDataAlign(f)     (((f)>>11)&0x01)
#define CV_Compile2Flags_ExtractManagedPresent(f)  (((f)>>12)&0x01)
#define CV_Compile2Flags_ExtractSecurityChecks(f)  (((f)>>13)&0x01)
#define CV_Compile2Flags_ExtractHotPatch(f)        (((f)>>14)&0x01)
#define CV_Compile2Flags_ExtractCVTCIL(f)          (((f)>>15)&0x01)
#define CV_Compile2Flags_ExtractMSILModule(f)      (((f)>>16)&0x01)

typedef struct CV_SymCompile2 CV_SymCompile2;
struct CV_SymCompile2
{
  CV_Compile2Flags flags;
  CV_Arch machine;
  U16 ver_fe_major;
  U16 ver_fe_minor;
  U16 ver_fe_build;
  U16 ver_major;
  U16 ver_minor;
  U16 ver_build;
  // U8[] ver_str (null terminated)
};

//- (SymKind: MANYREG2)

typedef struct CV_SymManyreg2 CV_SymManyreg2;
struct CV_SymManyreg2
{
  CV_TypeId itype;
  U16 count;
  // U16[count] regs;
};

//- (SymKind: LOCALSLOT)

typedef struct CV_SymSlot CV_SymSlot;
struct CV_SymSlot
{
  U32 slot_index;
  CV_TypeId itype;
  // U8[] name (null terminated)
};

//- (SymKind: MANFRAMEREL, ATTR_FRAMEREL)

typedef struct CV_SymAttrFrameRel CV_SymAttrFrameRel;
struct CV_SymAttrFrameRel
{
  U32 off;
  CV_TypeId itype;
  CV_LocalVarAttr attr;
  // U8[] name (null terminated)
};

//- (SymKind: MANREGISTER, ATTR_REGISTER)

typedef struct CV_SymAttrReg CV_SymAttrReg;
struct CV_SymAttrReg
{
  CV_TypeId itype;
  CV_LocalVarAttr attr;
  U16 reg;
  // U8[] name (null terminated)
};

//- (SymKind: MANMANYREG, ATTR_MANYREG)


typedef struct CV_SymAttrManyReg CV_SymAttrManyReg;
struct CV_SymAttrManyReg
{
  CV_TypeId itype;
  CV_LocalVarAttr attr;
  U8 count;
  // U8[count] regs
  // U8[] name (null terminated)
};

//- (SymKind: MANREGREL, ATTR_REGREL)

typedef struct CV_SymAttrRegRel CV_SymAttrRegRel;
struct CV_SymAttrRegRel
{
  U32 off;
  CV_TypeId itype;
  U16 reg;
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
  U32 suc_name;
  U32 sym_off;
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
  U16 thunk_size;
  U32 thunk_sec_off;
  U32 target_sec_off;
  CV_SectionIndex thunk_sec;
  CV_SectionIndex target_sec;
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
  U32 parent;
  U32 end;
  U32 len;
  CV_SepcodeFlags flags;
  U32 sec_off;
  U32 sec_parent_off;
  U16 sec;
  U16 sec_parent;
};

//- (SymKind: SECTION)

typedef struct CV_SymSection CV_SymSection;
struct CV_SymSection
{
  U16 sec_index;
  U8 align;
  U8 pad;
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
  U16 ordinal;
  CV_ExportFlags flags;
  // U8[] name (null terminated)
};

//- (SymKind: CALLSITEINFO)

typedef struct CV_SymCallSiteInfo CV_SymCallSiteInfo;
struct CV_SymCallSiteInfo
{
  U32 off;
  U16 sec;
  U16 pad;
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
  U32 off;
  CV_Reg reg;
  CV_FrameCookieKind kind;
  U8 flags;
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
  U32 file_id;
  U32 file_ln;
  // U8[] data (rest of data)
};

//- (SymKind: COMPILE3)

typedef U32 CV_Compile3Flags;
#define CV_Compile3Flags_ExtractLanguage(f)        (((f)    )&0xFF)
#define CV_Compile3Flags_ExtractEditAndContinue(f) (((f)>> 9)&0x01)
#define CV_Compile3Flags_ExtractNoDbgInfo(f)       (((f)>>10)&0x01)
#define CV_Compile3Flags_ExtractLTCG(f)            (((f)>>11)&0x01)
#define CV_Compile3Flags_ExtractNoDataAlign(f)     (((f)>>12)&0x01)
#define CV_Compile3Flags_ExtractManagedPresent(f)  (((f)>>13)&0x01)
#define CV_Compile3Flags_ExtractSecurityChecks(f)  (((f)>>14)&0x01)
#define CV_Compile3Flags_ExtractHotPatch(f)        (((f)>>15)&0x01)
#define CV_Compile3Flags_ExtractCVTCIL(f)          (((f)>>16)&0x01)
#define CV_Compile3Flags_ExtractMSILModule(f)      (((f)>>17)&0x01)
#define CV_Compile3Flags_ExtractSDL(f)             (((f)>>18)&0x01)
#define CV_Compile3Flags_ExtractPGO(f)             (((f)>>19)&0x01)
#define CV_Compile3Flags_ExtractEXP(f)             (((f)>>20)&0x01)

typedef struct CV_SymCompile3 CV_SymCompile3;
struct CV_SymCompile3
{
  CV_Compile3Flags flags;
  CV_Arch machine;
  U16 ver_fe_major;
  U16 ver_fe_minor;
  U16 ver_fe_build;
  U16 ver_feqfe;
  U16 ver_major;
  U16 ver_minor;
  U16 ver_build;
  U16 ver_qfe;
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
  CV_TypeId itype;
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

//- (SymKind: DEFRANGE_SUBFIELD)

typedef struct CV_SymDefrangeSubfield CV_SymDefrangeSubfield;
struct CV_SymDefrangeSubfield
{
  U32 program;
  U32 off_in_parent;
  CV_LvarAddrRange range;
  // CV_LvarAddrGap[] gaps (rest of data)
};

//- (SymKind: DEFRANGE_REGISTER)

typedef struct CV_SymDefrangeRegister CV_SymDefrangeRegister;
struct CV_SymDefrangeRegister
{
  CV_Reg reg;
  CV_RangeAttribs attribs;
  CV_LvarAddrRange range;
  // CV_LvarAddrGap[] gaps (rest of data)
};

//- (SymKind: DEFRANGE_FRAMEPOINTER_REL)

typedef struct CV_SymDefrangeFramepointerRel CV_SymDefrangeFramepointerRel;
struct CV_SymDefrangeFramepointerRel
{
  S32 off;
  CV_LvarAddrRange range;
  // CV_LvarAddrGap[] gaps (rest of data)
};

//- (SymKind: DEFRANGE_SUBFIELD_REGISTER)

typedef struct CV_SymDefrangeSubfieldRegister CV_SymDefrangeSubfieldRegister;
struct CV_SymDefrangeSubfieldRegister
{
  CV_Reg reg;
  CV_RangeAttribs attribs;
  U32 field_offset;
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
#define CV_DefrangeRegisterRelFlag_ExtractOffsetParent(f) (((f)>>4)&0xFFF)

typedef struct CV_SymDefrangeRegisterRel CV_SymDefrangeRegisterRel;
struct CV_SymDefrangeRegisterRel
{
  CV_Reg reg;
  CV_DefrangeRegisterRelFlags flags;
  S32 reg_off;
  CV_LvarAddrRange range;
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
} CV_InlineBinaryAnnotationEnum;

typedef U32 CV_InlineRangeKind;
typedef enum CV_InlnineRangeKindEnum
{
  CV_InlineRangeKind_Expr,
  CV_InlineRangeKind_Stmt
} CV_InlnineRangeKindEnum;

typedef struct CV_SymInlineSite CV_SymInlineSite;
struct CV_SymInlineSite
{
  U32 parent;
  U32 end;
  CV_ItemId inlinee;
  // U8 annotations[] (rest of data)
};

//- (SymKind: INLINESITE2)

typedef struct CV_SymInlineSite2 CV_SymInlineSite2;
struct CV_SymInlineSite2
{
  U32 parent_off;
  U32 end_off;
  CV_ItemId inlinee;
  U32 invocations;
  // U8 annotations[] (rest of data)
};

//- (SymKind: INLINESITE_END) (empty)

//- (SymKind: FILESTATIC)

typedef struct CV_SymFileStatic CV_SymFileStatic;
struct CV_SymFileStatic
{
  CV_TypeId itype;
  U32 mod_offset;
  CV_LocalFlags flags;
  // U8[] name (null terminated)
};

//- (SymKind: ARMSWITCHTABLE)

typedef U16 CV_ArmSwitchKind;
typedef enum
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
} CV_ArmSwitchKindEnum;

typedef struct CV_SymArmSwitchTable CV_SymArmSwitchTable;
struct CV_SymArmSwitchTable
{
  U32 off_base;
  U16 sec_base;
  CV_ArmSwitchKind kind;
  U32 off_branch;
  U32 off_table;
  U16 sec_branch;
  U16 sec_table;
  U32 entry_count;
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
  U32 off;
  U16 sec;
  U16 call_inst_len;
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
  U32 word0;
  U32 word1;
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
  U32 data;
  CV_ModIndex imod;
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
  CV_TypeId itype;
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

#define CV_LeafIDKindXList(X) \
X(FUNC_ID, 0x1601)\
X(MFUNC_ID, 0x1602)\
X(BUILDINFO, 0x1603)\
X(SUBSTR_LIST, 0x1604)\
X(STRING_ID, 0x1605)\
X(UDT_SRC_LINE, 0x1606)\
X(UDT_MOD_SRC_LINE, 0x1607)

typedef U16 CV_LeafIDKind;
typedef enum CV_LeafIDKindEnum
{
#define X(N,c) CV_LeafIDKind_##N = c,
  CV_LeafIDKindXList(X)
#undef X
}
CV_LeafIDKindEnum;

#define CV_TypeId_Variadic 0

#define CV_BasicPointerKindXList(X) \
X(VALUE,      0x0)\
X(16BIT,      0x1)\
X(FAR_16BIT,  0x2)\
X(HUGE_16BIT, 0x3)\
X(32BIT,      0x4)\
X(16_32BIT,   0x5)\
X(64BIT,      0x6)

typedef U8 CV_BasicPointerKind;
typedef enum
{
#define X(N,c) CV_BasicPointerKind_##N = c,
  CV_BasicPointerKindXList(X)
#undef X
} CV_BasicPointerKindEnum;

#define CV_BasicTypeFromTypeId(x) ((x)&0xFF)
#define CV_BasicPointerKindFromTypeId(x) (((x)>>8)&0xFF)

typedef U8 CV_HFAKind;
typedef enum CV_HFAKindEnum
{
  CV_HFAKind_None,
  CV_HFAKind_Float,
  CV_HFAKind_Double,
  CV_HFAKind_Other
}
CV_HFAKindEnum;

typedef U8 CV_MoComUDTKind;
typedef enum CV_MoComUDTKindEnum
{
  CV_MoComUDTKind_None,
  CV_MoComUDTKind_Ref,
  CV_MoComUDTKind_Value,
  CV_MoComUDTKind_Interface
}
CV_MoComUDTKindEnum;

typedef U16 CV_TypeProps;
enum
{
  CV_TypeProp_Packed                     = (1<<0),
  CV_TypeProp_HasConstructorsDestructors = (1<<1),
  CV_TypeProp_OverloadedOperators        = (1<<2),
  CV_TypeProp_IsNested                   = (1<<3),
  CV_TypeProp_ContainsNested             = (1<<4),
  CV_TypeProp_OverloadedAssignment       = (1<<5),
  CV_TypeProp_OverloadedCasting          = (1<<6),
  CV_TypeProp_FwdRef                     = (1<<7),
  CV_TypeProp_Scoped                     = (1<<8),
  CV_TypeProp_HasUniqueName              = (1<<9),
  CV_TypeProp_Sealed                     = (1<<10),
  // HFA: 11,12
  CV_TypeProp_Intrinsic                  = (1<<13),
  // MOCOM: 14,15
};
#define CV_TypeProps_ExtractHFA(f)   (((f)>>11)&0x3)
#define CV_TypeProps_ExtractMOCOM(f) (((f)>>14)&0x3)

typedef U8 CV_PointerKind;
typedef enum CV_PointerKindEnum
{
  CV_PointerKind_Near,  // 16 bit
  CV_PointerKind_Far,   // 16:16 bit
  CV_PointerKind_Huge,  // 16:16 bit
  CV_PointerKind_BaseSeg,
  CV_PointerKind_BaseVal,
  CV_PointerKind_BaseSegVal,
  CV_PointerKind_BaseAddr,
  CV_PointerKind_BaseSegAddr,
  CV_PointerKind_BaseType,
  CV_PointerKind_BaseSelf,
  CV_PointerKind_Near32, // 32 bit
  CV_PointerKind_Far32,  // 16:32 bit
  CV_PointerKind_64,     // 64 bit
}
CV_PointerKindEnum;

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
#define CV_FieldAttribs_ExtractAccess(f) ((f)&0x3)
#define CV_FieldAttribs_ExtractMethodProp(f) (((f)>>2)&0x7)

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
  U32 signature;
  // U8[] name (null terminated)
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
  COFF_Guid sig70;
  U32 age;
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
  CV_ModifierFlag_Const = (1 << 0),
  CV_ModifierFlag_Volatile = (1 << 1),
  CV_ModifierFlag_Unaligned = (1 << 2),
};

typedef struct CV_LeafModifier CV_LeafModifier;
struct CV_LeafModifier
{
  CV_TypeId itype;
  CV_ModifierFlags flags;
};

//- (LeafKind: POINTER)

typedef U32 CV_PointerAttribs;
enum
{
  // Kind: [0:4]
  // Mode: [5:7]
  CV_PointerAttrib_IsFlat   = (1 << 8),
  CV_PointerAttrib_Volatile = (1 << 9),
  CV_PointerAttrib_Const    = (1 << 10),
  CV_PointerAttrib_Unaligned = (1 << 11),
  CV_PointerAttrib_Restricted = (1 << 12),
  // Size: [13,18]
  CV_PointerAttrib_MOCOM = (1 << 19),
  CV_PointerAttrib_LRef  = (1 << 21),
  CV_PointerAttrib_RRef  = (1 << 22)
};

#define CV_PointerAttribs_ExtractKind(a) ((a)&0x1F)
#define CV_PointerAttribs_ExtractMode(a) (((a)>>5)&0x7)
#define CV_PointerAttribs_ExtractSize(a) (((a)>>13)&0x3F)

typedef struct CV_LeafPointer CV_LeafPointer;
struct CV_LeafPointer
{
  CV_TypeId itype;
  CV_PointerAttribs attribs;
};

//- (LeafKind: PROCEDURE)

typedef struct CV_LeafProcedure CV_LeafProcedure;
struct CV_LeafProcedure
{
  CV_TypeId ret_itype;
  CV_CallKind call_kind;
  CV_FunctionAttribs attribs;
  U16 arg_count;
  CV_TypeId arg_itype;
};

//- (LeafKind: MFUNCTION)

typedef struct CV_LeafMFunction CV_LeafMFunction;
struct CV_LeafMFunction
{
  CV_TypeId ret_itype;
  CV_TypeId class_itype;
  CV_TypeId this_itype;
  CV_CallKind call_kind;
  CV_FunctionAttribs attribs;
  U16 arg_count;
  CV_TypeId arg_itype;
  S32 this_adjust;
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
  U8 len;
  U8 pos;
};

//- (LeafKind: METHODLIST)

//   ("jagged" array of these vvvvvvvv)
typedef struct CV_LeafMethodListMember CV_LeafMethodListMember;
struct CV_LeafMethodListMember
{
  CV_FieldAttribs attribs;
  U16 pad;
  CV_TypeId itype;
  // U32 vbaseoff (when Intro or PureIntro)
};

//- (LeafKind: INDEX)

typedef struct CV_LeafIndex CV_LeafIndex;
struct CV_LeafIndex
{
  U16 pad;
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
  U16 count;
  CV_TypeProps props;
  CV_TypeId field_itype;
  CV_TypeId derived_itype;
  CV_TypeId vshape_itype;
  // CV_Numeric size
  // U8[] name (null terminated)
  // U8[] unique_name (null terminated)
};

//- (LeafKind: UNION)

typedef struct CV_LeafUnion CV_LeafUnion;
struct CV_LeafUnion
{
  U16 count;
  CV_TypeProps props;
  CV_TypeId field_itype;
  // CV_Numeric size
  // U8[] name (null terminated)
  // U8[] unique_name (null terminated)
};

//- (LeafKind: ENUM)

typedef struct CV_LeafEnum CV_LeafEnum;
struct CV_LeafEnum
{
  U16 count;
  CV_TypeProps props;
  CV_TypeId base_itype;
  CV_TypeId field_itype;
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
  CV_TypeId itype;
  // CV_Numeric offset
  // U8[] name (null terminated)
};

//- (LeafKind: STMEMBER)

typedef struct CV_LeafStMember CV_LeafStMember;
struct CV_LeafStMember
{
  CV_FieldAttribs attribs;
  CV_TypeId itype;
  // U8[] name (null terminated)
};

//- (LeafKind: METHOD)

typedef struct CV_LeafMethod CV_LeafMethod;
struct CV_LeafMethod
{
  U16 count;
  CV_TypeId list_itype;
  // U8[] name (null terminated)
};

//- (LeafKind: ONEMETHOD)

typedef struct CV_LeafOneMethod CV_LeafOneMethod;
struct CV_LeafOneMethod
{
  CV_FieldAttribs attribs;
  CV_TypeId itype;
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
  U16 pad;
  CV_TypeId itype;
  // U8[] name (null terminated)
};

//- (LeafKind: NESTTYPEEX)

typedef struct CV_LeafNestTypeEx CV_LeafNestTypeEx;
struct CV_LeafNestTypeEx
{
  CV_FieldAttribs attribs;
  CV_TypeId itype;
  // U8[] name (null terminated)
};

//- (LeafKind: BCLASS)

typedef struct CV_LeafBClass CV_LeafBClass;
struct CV_LeafBClass
{
  CV_FieldAttribs attribs;
  CV_TypeId itype;
  // CV_Numeric offset
};

//- (LeafKind: VBCLASS, IVBCLASS)

typedef struct CV_LeafVBClass CV_LeafVBClass;
struct CV_LeafVBClass
{
  CV_FieldAttribs attribs;
  CV_TypeId itype;
  CV_TypeId vbptr_itype;
  // CV_Numeric vbptr_off
  // CV_Numeric vtable_off
};

//- (LeafKind: VFUNCTAB)

typedef struct CV_LeafVFuncTab CV_LeafVFuncTab;
struct CV_LeafVFuncTab
{
  U16 pad;
  CV_TypeId itype;
};

//- (LeafKind: VFUNCOFF)

typedef struct CV_LeafVFuncOff CV_LeafVFuncOff;
struct CV_LeafVFuncOff
{
  U16 pad;
  CV_TypeId itype;
  U32 off;
};

//- (LeafKind: VFTABLE)

typedef struct CV_LeafVFTable CV_LeafVFTable;
struct CV_LeafVFTable
{
  CV_TypeId owner_itype;
  CV_TypeId base_table_itype;
  U32 offset_in_object_layout;
  U32 names_len;
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
  U16 unknown1;
  CV_TypeId field_itype;
  CV_TypeId derived_itype;
  CV_TypeId vshape_itype;
  U16 unknown2;
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
  U32 line;
};

//- (LeafIDKind: UDT_MOD_SRC_LINE)

typedef struct CV_LeafUDTModSrcLine CV_LeafUDTModSrcLine;
struct CV_LeafUDTModSrcLine
{
  CV_TypeId udt_itype;
  CV_ItemId src_string_id;
  U32 line;
  CV_ModIndex imod;
};

////////////////////////////////
//~ C13 Line Info Types

#define CV_C13_SubSectionKind_IgnoreFlag 0x80000000

#define CV_C13_SubSectionKindXList(X)\
X(Symbols,             0xF1)\
X(Lines,               0xF2)\
X(StringTable,         0xF3)\
X(FileChksms,          0xF4)\
X(FrameData,           0xF5)\
X(InlineeLines,        0xF6)\
X(CrossScopeImports,   0xF7)\
X(CrossScopeExports,   0xF8)\
X(IlLines,             0xF9)\
X(FuncMDTokenMap,      0xFA)\
X(TypeMDTokenMap,      0xFB)\
X(MergedAssemblyInput, 0xFC)\
X(CoffSymbolRVA,       0xFD)\
X(XfgHashType,         0xFF)\
X(XfgHashVirtual,      0x100)

typedef U32 CV_C13_SubSectionKind;
typedef enum
{
#define X(N,c) CV_C13_SubSectionKind_##N = c,
  CV_C13_SubSectionKindXList(X)
#undef X
  CV_C13_SubSectionKind_FIRST = CV_C13_SubSectionKind_Symbols
} CV_C13_SubSectionKindEnum;

typedef U32 CV_C13_SubSectionIdxKind;
typedef enum
{
  CV_C13_SubSectionIdxKind_NULL,
#define X(N,c) CV_C13_SubSectionIdxKind_##N,
  CV_C13_SubSectionKindXList(X)
#undef X
  CV_C13_SubSectionIdxKind_COUNT
} CV_C13SubSectionIdxKind;

typedef struct CV_C13_SubSectionHeader CV_C13_SubSectionHeader;
struct CV_C13_SubSectionHeader
{
  CV_C13_SubSectionKind kind;
  U32 size;
};

//- FileChksms sub-section

typedef U8 CV_C13_ChecksumKind;
typedef enum
{
  CV_C13_ChecksumKind_Null,
  CV_C13_ChecksumKind_MD5,
  CV_C13_ChecksumKind_SHA1,
  CV_C13_ChecksumKind_SHA256,
} CV_C13_ChecksumKindEnum;

typedef struct CV_C13_Checksum CV_C13_Checksum;
struct CV_C13_Checksum
{
  U32 name_off;
  U8 len;
  CV_C13_ChecksumKind kind;
};

//- Lines sub-section

typedef U16 CV_C13_SubSecLinesFlags;
enum
{
  CV_C13_SubSecLinesFlag_HasColumns = (1 << 0)
};

typedef struct CV_C13_SubSecLinesHeader CV_C13_SubSecLinesHeader;
struct CV_C13_SubSecLinesHeader
{
  U32 sec_off;
  CV_SectionIndex sec;
  CV_C13_SubSecLinesFlags flags;
  U32 len;
};

typedef struct CV_C13_File CV_C13_File;
struct CV_C13_File
{
  U32 file_off;
  U32 num_lines;
  U32 block_size;
  // CV_C13_Line[num_lines] lines;
  // CV_C13_Column[num_lines] columns; (if HasColumns)
};

typedef U32 CV_C13_LineFlags;
#define CV_C13_LineFlags_ExtractLineNumber(f) ((f)&0xFFFFFF)
#define CV_C13_LineFlags_ExtractDeltaToEnd(f) (((f)>>24)&0x7F)
#define CV_C13_LineFlags_ExtractStatement(f)  (((f)>>31)&0x1)

typedef struct CV_C13_Line CV_C13_Line;
struct CV_C13_Line
{
  U32 off;
  CV_C13_LineFlags flags; 
};

typedef struct CV_C13_Column CV_C13_Column;
struct CV_C13_Column
{
  U16 start;
  U16 end;
};

//- FrameData sub-section

typedef U32 CV_C13_FrameDataFlags;
enum
{
  CV_C13_FrameDataFlag_HasStructuredExceptionHandling = (1 << 0),
  CV_C13_FrameDataFlag_HasExceptionHandling           = (1 << 1),
  CV_C13_FrameDataFlag_HasIsFuncStart                 = (1 << 2),
};

typedef struct CV_C13_FrameData CV_C13_FrameData;
struct CV_C13_FrameData
{
  U32 start_voff;
  U32 code_size;
  U32 local_size;
  U32 params_size;
  U32 max_stack_size;
  U32 frame_func;
  U16 prolog_size;
  U16 saved_reg_size;
  CV_C13_FrameDataFlags flags;
};

//- InlineLines sub-section 

typedef U32 CV_C13_InlineeLinesSig;
enum
{
  CV_C13_InlineeLinesSig_NORMAL,
  CV_C13_InlineeLinesSig_EXTRA_FILES,
};

typedef struct CV_C13_InlineeSourceLineHeader CV_C13_InlineeSourceLineHeader;
struct CV_C13_InlineeSourceLineHeader
{
  CV_ItemId inlinee;          // LF_FUNC_ID or LF_MFUNC_ID
  U32 file_off;               // offset into FileChksms sub-section
  U32 first_source_ln;        // base source line number for binary annotations
  // if sig set to CV_C13_InlineeLinesSig_EXTRA_FILES
  //  U32 extra_file_count;
  //  U32 files[];
};

#pragma pack(pop)

////////////////////////////////
//~ Common Parser Types

// CV_Numeric layout
// x: U16
// buf: U8[]
// case (x < 0x8000):  kind=U16 val=x
// case (x >= 0x8000): kind=x   val=buf

typedef struct CV_NumericParsed CV_NumericParsed;
struct CV_NumericParsed
{
  CV_NumericKind kind;
  U8 *val;
  U64 encoded_size;
};

typedef struct CV_RecRange CV_RecRange;
struct CV_RecRange
{
  U32 off;
  CV_RecHeader hdr;
};

#define CV_REC_RANGE_CHUNK_SIZE 511

typedef struct CV_RecRangeChunk CV_RecRangeChunk;
struct CV_RecRangeChunk
{
  struct CV_RecRangeChunk *next;
  CV_RecRange ranges[CV_REC_RANGE_CHUNK_SIZE];
};

typedef struct CV_RecRangeStream CV_RecRangeStream;
struct CV_RecRangeStream
{
  CV_RecRangeChunk *first_chunk;
  CV_RecRangeChunk *last_chunk;
  U64 total_count;
};

typedef struct CV_RecRangeArray CV_RecRangeArray;
struct CV_RecRangeArray
{
  CV_RecRange *ranges;
  U64 count;
};

////////////////////////////////
//~ Sym Parser Types

typedef struct CV_SymParsed CV_SymParsed;
struct CV_SymParsed
{
  // source information
  String8 data;
  U64 sym_align;
  
  // sym index derived from source
  CV_RecRangeArray sym_ranges;
};


////////////////////////////////
//~ Leaf Parser Types

typedef struct CV_LeafParsed CV_LeafParsed;
struct CV_LeafParsed
{
  // source information
  String8 data;
  CV_TypeId itype_first;
  CV_TypeId itype_opl;
  
  // leaf index derived from source
  CV_RecRangeArray leaf_ranges;
};

////////////////////////////////
//~ C13 Info Parser Types

//- $$LINES

typedef struct CV_C13LineArray CV_C13LineArray;
struct CV_C13LineArray
{
  U32 file_off;
  U64 line_count;
  U64 col_count;
  U64 *voffs;     // [line_count + 1]
  U32 *line_nums; // [line_count]
  U16 *col_nums;  // [line_count * 2]
};

typedef struct CV_C13File CV_C13File;
struct CV_C13File
{
  U32 file_off;
  CV_C13LineArray lines;
};

typedef struct CV_C13LinesParsed CV_C13LinesParsed;
struct CV_C13LinesParsed
{
  U64 sec_idx;
  U64 sec_off_lo;
  U64 sec_off_hi;
  U64 file_off;
  U64 line_count;
  U64 col_count;
  U64 line_array_off;
  U64 col_array_off;
};

typedef struct CV_C13LinesParsedNode CV_C13LinesParsedNode;
struct CV_C13LinesParsedNode
{
  CV_C13LinesParsedNode *next;
  CV_C13LinesParsed v;
};

typedef struct CV_C13LinesParsedList CV_C13LinesParsedList;
struct CV_C13LinesParsedList
{
  CV_C13LinesParsedNode *first;
  CV_C13LinesParsedNode *last;
  U64 count;
};
//- $$INLINEE_LINES

typedef struct CV_C13InlineeLinesParsed CV_C13InlineeLinesParsed;
struct CV_C13InlineeLinesParsed
{
  CV_ItemId inlinee;
  U32 file_off;
  U32 first_source_ln;
  U32 extra_file_count;
  U32 *extra_files;
};

typedef struct CV_C13InlineeLinesParsedNode CV_C13InlineeLinesParsedNode;
struct CV_C13InlineeLinesParsedNode
{
  struct CV_C13InlineeLinesParsedNode *next;
  CV_C13InlineeLinesParsed v;
};

typedef struct CV_C13InlineeLinesParsedList CV_C13InlineeLinesParsedList;
struct CV_C13InlineeLinesParsedList
{
  CV_C13InlineeLinesParsedNode *first;
  CV_C13InlineeLinesParsedNode *last;
  U64 count;
};

typedef struct CV_C13SubSectionNode CV_C13SubSectionNode;
struct CV_C13SubSectionNode
{
  struct CV_C13SubSectionNode *next;
  CV_C13_SubSectionKind kind;
  Rng1U64 range;
};

typedef struct CV_C13SubSectionList CV_C13SubSectionList;
struct CV_C13SubSectionList
{
  CV_C13SubSectionNode *first;
  CV_C13SubSectionNode *last;
  U64 count;
};

typedef struct CV_C13Parsed CV_C13Parsed;
struct CV_C13Parsed
{
  CV_C13SubSectionList v[CV_C13_SubSectionIdxKind_COUNT];
};

////////////////////////////////
// Accels

typedef struct CV_C13Line CV_C13Line;
struct CV_C13Line
{
  U64 voff;
  U32 file_off;
  U32 line_num;
  U16 col_num;
};

typedef struct CV_C13LinesAccel CV_C13LinesAccel;
struct CV_C13LinesAccel
{
  U64 map_count;
  CV_C13Line *map;
};


typedef struct CV_C13InlineeLinesAccel CV_C13InlineeLinesAccel;
struct CV_C13InlineeLinesAccel
{
  U64 bucket_count;
  U64 bucket_max;
  CV_C13InlineeLinesParsed **buckets;
};

typedef struct CV_InlineBinaryAnnotsParsed CV_InlineBinaryAnnotsParsed;
struct CV_InlineBinaryAnnotsParsed
{
  U64 lines_count;
  CV_C13LineArray *lines;
  U64 code_range_count;
  Rng1U64 *code_ranges;
};

////////////////////////////////
//~ Compound Types

typedef struct CV_TypeIdArray CV_TypeIdArray;
struct CV_TypeIdArray
{
  CV_TypeId *itypes;
  U64 count;
};


////////////////////////////////
//~ Common Functions

internal CV_NumericParsed cv_numeric_from_data_range(U8 *first, U8 *opl);

internal B32              cv_numeric_fits_in_u64(CV_NumericParsed *num);
internal B32              cv_numeric_fits_in_s64(CV_NumericParsed *num);
internal B32              cv_numeric_fits_in_f64(CV_NumericParsed *num);

internal U64              cv_u64_from_numeric(CV_NumericParsed *num);
internal S64              cv_s64_from_numeric(CV_NumericParsed *num);
internal F64              cv_f64_from_numeric(CV_NumericParsed *num);

////////////////////////////////
//~ Sym/Leaf Parser Functions

//- the first pass parser
internal CV_RecRangeStream* cv_rec_range_stream_from_data(Arena *arena, String8 data, U64 align);

//- sym
internal CV_SymParsed cv_sym_from_data(Arena *arena, String8 sym_data, U64 sym_align);

//- leaf
internal CV_LeafParsed* cv_leaf_from_data(Arena *arena, String8 leaf_data, CV_TypeId first);

//- range streams
internal CV_RecRangeChunk* cv_rec_range_stream_push_chunk(Arena *arena,
                                                          CV_RecRangeStream *stream);
// TODO(allen): check why this isn't a pointer return - 
// leave a note if there's a good reason, otherwise switch to pointer return
internal CV_RecRangeArray  cv_rec_range_array_from_stream(Arena *arena, CV_RecRangeStream *stream);

////////////////////////////////
//~ C13 $$ Parsers

internal CV_C13_SubSectionIdxKind cv_c13_sub_section_idx_from_kind(CV_C13_SubSectionKind kind);

internal CV_C13SubSectionList         cv_c13_sub_section_list_from_data(Arena *arena, String8 data, U64 align);
internal String8                      cv_c13_file_chksms_from_sub_sections(String8 c13_data, CV_C13Parsed *ss);
internal CV_C13LinesParsedList        cv_c13_lines_from_sub_sections(Arena *arena, String8 c13_data, Rng1U64 ss_range);
internal CV_C13InlineeLinesParsedList cv_c13_inlinee_lines_from_sub_sections(Arena *arena, String8 c13_data, CV_C13SubSectionList ss_list);
internal CV_C13Parsed                 cv_c13_parsed_from_list(CV_C13SubSectionList *list);

internal CV_C13LineArray cv_c13_line_array_from_data(Arena *arena, String8 c13_data, U64 sec_base, CV_C13LinesParsed parsed_lines);

////////////////////////////////
// $$LINES Accel

internal void               cv_make_c13_files(Arena *arena, String8 c13_data, CV_C13SubSectionList lines, U64 *file_count_out, CV_C13File **files_out);
internal CV_C13LinesAccel * cv_c13_make_lines_accel(Arena *arena, U64 lines_count, CV_C13LineArray *lines);
internal CV_C13Line *       cv_c13_line_from_voff(CV_C13LinesAccel *accel, U64 voff, U64 *out_line_count);

////////////////////////////////
// $$INLINEE_LINES Accel

internal U64                        cv_c13_inlinee_lines_accel_hash(void *buffer, U64 size);
internal B32                        cv_c13_inlinee_lines_accel_push(CV_C13InlineeLinesAccel *accel, CV_C13InlineeLinesParsed *parsed);
internal CV_C13InlineeLinesParsed * cv_c13_inlinee_lines_accel_find(CV_C13InlineeLinesAccel *accel, CV_ItemId inlinee);
internal CV_C13InlineeLinesAccel *  cv_c13_make_inlinee_lines_accel(Arena *arena, CV_C13InlineeLinesParsedList sub_sects);

////////////////////////////////
// Inline Binary Annots Parser

internal S32 cv_inline_annot_convert_to_signed_operand(U32 value);
internal U64 cv_decode_inline_annot_u32(String8 data, U64 offset, U32 *out_value);
internal U64 cv_decode_inline_annot_s32(String8 data, U64 offset, S32 *out_value);

internal CV_InlineBinaryAnnotsParsed cv_c13_parse_inline_binary_annots(Arena *arena, U64 parent_voff, CV_C13InlineeLinesParsed *inlinee_parsed, String8 binary_annots);

////////////////////////////////
// Enum -> String

internal String8 cv_string_from_inline_range_kind(CV_InlineRangeKind kind);

#endif // CODEVIEW_H
