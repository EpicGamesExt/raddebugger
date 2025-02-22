#ifndef _ARMADILLO_H_
#define _ARMADILLO_H_

#include "adefs.h"

S32 ArmadilloDisassemble(U32 opcode, U64 PC, struct ad_insn **out);
S32 ArmadilloDone(struct ad_insn **insn);

#endif
