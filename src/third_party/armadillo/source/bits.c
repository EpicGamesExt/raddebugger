#include "bits.h"

U32 bits(U32 number, U32 start, U32 end){
    U32 amount = (end - start) + 1;
    U32 mask = ((1 << amount) - 1) << start;

    return (number & mask) >> start;
}

U32 sign_extend(U32 number, int numbits){
    if(number & (1 << (numbits - 1)))
        return number | ~((1 << numbits) - 1);

    return number;
}
