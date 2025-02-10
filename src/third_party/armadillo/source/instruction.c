#include <stdlib.h>

#include "instruction.h"

struct instruction *instruction_new(U32 opcode, U64 PC){
    struct instruction *i = malloc(sizeof(struct instruction));

    i->opcode = opcode;
    i->PC = PC;

    return i;
}

void instruction_free(struct instruction *i){
    free(i);
}
