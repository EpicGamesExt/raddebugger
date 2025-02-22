#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static S32 _concat_internal(char **dst, const char *src, va_list args){
    if(!src || !dst)
        return 0;

    U64 srclen = strlen(src), dstlen = 0;

    if(*dst)
        dstlen = strlen(*dst);

    /* Back up args before it gets used. Client calls va_end
     * on the parameter themselves when calling vconcat.
     */
    va_list args1;
    va_copy(args1, args);

    U64 total = srclen + dstlen + vsnprintf(NULL, 0, src, args) + 1;

    char *dst1 = malloc(total);

    if(!(*dst))
        *dst1 = '\0';
    else{
        strncpy(dst1, *dst, dstlen + 1);
        free(*dst);
        *dst = NULL;
    }

    S32 w = vsnprintf(dst1 + dstlen, total, src, args1);

    va_end(args1);

    *dst = (char*)realloc(dst1, strlen(dst1) + 1);

    return w;
}

S32 concat(char **dst, const char *src, ...){
    va_list args;
    va_start(args, src);

    S32 w = _concat_internal(dst, src, args);

    va_end(args);

    return w;
}

S32 vconcat(char **dst, const char *src, va_list args){
    return _concat_internal(dst, src, args);
}
