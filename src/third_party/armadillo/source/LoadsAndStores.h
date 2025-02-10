#ifndef _LOADSANDSTORES_H_
#define _LOADSANDSTORES_H_

#define NO_ALLOCATE 0
#define POST_INDEXED 1
#define OFFSET 2
#define PRE_INDEXED 3

#define UNSIGNED_IMMEDIATE -1

#define UNSCALED_IMMEDIATE 0
#define IMMEDIATE_POST_INDEXED 1
#define UNPRIVILEGED 2
#define IMMEDIATE_PRE_INDEXED 3

S32 LoadsAndStoresDisassemble(struct instruction *, struct ad_insn *);

#endif
