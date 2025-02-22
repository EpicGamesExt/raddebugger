#ifndef _INSTRUCTION_H_
#define _INSTRUCTION_H_

struct instruction {
	U32 opcode;
	U64 PC;
};

struct instruction *instruction_new(U32, U64);
void instruction_free(struct instruction *);

#endif
