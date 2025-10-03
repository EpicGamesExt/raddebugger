// This is a collection of code originally sourced from LibTomCrypt, located at
// https://github.com/libtom/libtomcrypt, released under the following license:
//
// ---
//
// The LibTom license
//
// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.
//
// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// For more information, please refer to <http://unlicense.org/>
//
// ---
//
// The code has been narrowed down and slightly modified, to include only the
// things that the RAD Debugger project needs, and to work with the project's
// build structure cleanly.

#ifndef TOMCRYPT_HASH_H
#define TOMCRYPT_HASH_H

////////////////////////////////
//~ rjf: Common Helpers

#define CRYPT_OK 1

#define LOAD32H(x, y)                            \
do { x = ((U32)((y)[0] & 255)<<24) | \
((U32)((y)[1] & 255)<<16) | \
((U32)((y)[2] & 255)<<8)  | \
((U32)((y)[3] & 255)); } while(0)

#define STORE32H(x, y)                                                                     \
do { (y)[0] = (unsigned char)(((x)>>24)&255); (y)[1] = (unsigned char)(((x)>>16)&255);   \
(y)[2] = (unsigned char)(((x)>>8)&255); (y)[3] = (unsigned char)((x)&255); } while(0)

#define STORE64H(x, y)                                                                     \
do { (y)[0] = (unsigned char)(((x)>>56)&255); (y)[1] = (unsigned char)(((x)>>48)&255);     \
(y)[2] = (unsigned char)(((x)>>40)&255); (y)[3] = (unsigned char)(((x)>>32)&255);     \
(y)[4] = (unsigned char)(((x)>>24)&255); (y)[5] = (unsigned char)(((x)>>16)&255);     \
(y)[6] = (unsigned char)(((x)>>8)&255); (y)[7] = (unsigned char)((x)&255); } while(0)

#define LTC_TMPVAR__(n, l) n ## l
#define LTC_TMPVAR_(n, l) LTC_TMPVAR__(n, l)
#define LTC_TMPVAR(n) LTC_TMPVAR_(LTC_ ## n ## _, __LINE__)

#define ROL(x, y) ( (((U32)(x)<<(U32)((y)&31)) | (((U32)(x)&0xFFFFFFFFUL)>>(U32)((32-((y)&31))&31))) & 0xFFFFFFFFUL)
#define ROR(x, y) ( ((((U32)(x)&0xFFFFFFFFUL)>>(U32)((y)&31)) | ((U32)(x)<<(U32)((32-((y)&31))&31))) & 0xFFFFFFFFUL)
#define ROLc(x, y) ( (((U32)(x)<<(U32)((y)&31)) | (((U32)(x)&0xFFFFFFFFUL)>>(U32)((32-((y)&31))&31))) & 0xFFFFFFFFUL)
#define RORc(x, y) ( ((((U32)(x)&0xFFFFFFFFUL)>>(U32)((y)&31)) | ((U32)(x)<<(U32)((32-((y)&31))&31))) & 0xFFFFFFFFUL)

#define MIN(x, y) ( ((x)<(y))?(x):(y) )

////////////////////////////////
//~ rjf: SHA256

typedef struct SHA256State SHA256State;
struct SHA256State
{
  U64 length;
  U32 state[8], curlen;
  U8 buf[64];
};

/* Various logical functions */
#define Ch(x,y,z)       (z ^ (x & (y ^ z)))
#define Maj(x,y,z)      (((x | y) & z) | (x & y))
#define S(x, n)         RORc((x),(n))
#define R(x, n)         (((x)&0xFFFFFFFFUL)>>(n))
#define Sigma0(x)       (S(x, 2) ^ S(x, 13) ^ S(x, 22))
#define Sigma1(x)       (S(x, 6) ^ S(x, 11) ^ S(x, 25))
#define Gamma0(x)       (S(x, 7) ^ S(x, 18) ^ R(x, 3))
#define Gamma1(x)       (S(x, 17) ^ S(x, 19) ^ R(x, 10))

/* compress 512-bits */
static int s_sha256_compress(SHA256State *state, const unsigned char *buf)
{
  U32 S[8], W[64], t0, t1;
  int i;
  
  /* copy state into S */
  for (i = 0; i < 8; i++) {
    S[i] = state->state[i];
  }
  
  /* copy the state into 512-bits into W[0..15] */
  for (i = 0; i < 16; i++) {
    LOAD32H(W[i], buf + (4*i));
  }
  
  /* fill W[16..63] */
  for (i = 16; i < 64; i++) {
    W[i] = Gamma1(W[i - 2]) + W[i - 7] + Gamma0(W[i - 15]) + W[i - 16];
  }
  
  /* Compress */
#define RND(a,b,c,d,e,f,g,h,i,ki)                \
t0 = h + Sigma1(e) + Ch(e, f, g) + ki + W[i];   \
t1 = Sigma0(a) + Maj(a, b, c);                  \
d += t0;                                        \
h  = t0 + t1;
  RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],0,0x428a2f98);
  RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],1,0x71374491);
  RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],2,0xb5c0fbcf);
  RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],3,0xe9b5dba5);
  RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],4,0x3956c25b);
  RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],5,0x59f111f1);
  RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],6,0x923f82a4);
  RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],7,0xab1c5ed5);
  RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],8,0xd807aa98);
  RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],9,0x12835b01);
  RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],10,0x243185be);
  RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],11,0x550c7dc3);
  RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],12,0x72be5d74);
  RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],13,0x80deb1fe);
  RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],14,0x9bdc06a7);
  RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],15,0xc19bf174);
  RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],16,0xe49b69c1);
  RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],17,0xefbe4786);
  RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],18,0x0fc19dc6);
  RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],19,0x240ca1cc);
  RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],20,0x2de92c6f);
  RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],21,0x4a7484aa);
  RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],22,0x5cb0a9dc);
  RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],23,0x76f988da);
  RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],24,0x983e5152);
  RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],25,0xa831c66d);
  RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],26,0xb00327c8);
  RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],27,0xbf597fc7);
  RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],28,0xc6e00bf3);
  RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],29,0xd5a79147);
  RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],30,0x06ca6351);
  RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],31,0x14292967);
  RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],32,0x27b70a85);
  RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],33,0x2e1b2138);
  RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],34,0x4d2c6dfc);
  RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],35,0x53380d13);
  RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],36,0x650a7354);
  RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],37,0x766a0abb);
  RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],38,0x81c2c92e);
  RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],39,0x92722c85);
  RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],40,0xa2bfe8a1);
  RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],41,0xa81a664b);
  RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],42,0xc24b8b70);
  RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],43,0xc76c51a3);
  RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],44,0xd192e819);
  RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],45,0xd6990624);
  RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],46,0xf40e3585);
  RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],47,0x106aa070);
  RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],48,0x19a4c116);
  RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],49,0x1e376c08);
  RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],50,0x2748774c);
  RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],51,0x34b0bcb5);
  RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],52,0x391c0cb3);
  RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],53,0x4ed8aa4a);
  RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],54,0x5b9cca4f);
  RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],55,0x682e6ff3);
  RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],56,0x748f82ee);
  RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],57,0x78a5636f);
  RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],58,0x84c87814);
  RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],59,0x8cc70208);
  RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],60,0x90befffa);
  RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],61,0xa4506ceb);
  RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],62,0xbef9a3f7);
  RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],63,0xc67178f2);
#undef RND
  
  /* feedback */
  for (i = 0; i < 8; i++) {
    state->state[i] = state->state[i] + S[i];
  }
  return CRYPT_OK;
}

/**
   Initialize the hash state
   @param md   The hash state you wish to initialize
   @return CRYPT_OK if successful
*/
int sha256_init(SHA256State *state)
{
  state->curlen = 0;
  state->length = 0;
  state->state[0] = 0x6A09E667UL;
  state->state[1] = 0xBB67AE85UL;
  state->state[2] = 0x3C6EF372UL;
  state->state[3] = 0xA54FF53AUL;
  state->state[4] = 0x510E527FUL;
  state->state[5] = 0x9B05688CUL;
  state->state[6] = 0x1F83D9ABUL;
  state->state[7] = 0x5BE0CD19UL;
  return CRYPT_OK;
}

/**
   Process a block of memory though the hash
   @param md     The hash state
   @param in     The data to hash
   @param inlen  The length of the data (octets)
   @return CRYPT_OK if successful
*/

int sha256_process(SHA256State *state, const unsigned char *in, unsigned long inlen)
{
  unsigned long n;
  int           err;
  int block_size = 64;
  if(state->curlen > sizeof(state->buf))
  {
    return 0; // CRYPT_INVALID_ARG
  }
  if(((state->length + inlen * 8) < state->length) || ((inlen * 8) < inlen))
  {
    return 0; // CRYPT_HASH_OVERFLOW
  }
  while(inlen > 0)
  {
    if(state->curlen == 0 && inlen >= block_size)
    {
      if ((err = s_sha256_compress(state, in)) != CRYPT_OK)
      {
        return err;
      }
      state->length += block_size * 8;
      in             += block_size;
      inlen          -= block_size;
    } else {
      n = MIN(inlen, (block_size - state->curlen));
      MemoryCopy(state->buf + state->curlen, in, (size_t)n);
      state->curlen += n;
      in             += n;
      inlen          -= n;
      if(state->curlen == block_size)
      {
        if((err = s_sha256_compress(state, state->buf)) != CRYPT_OK)
        {
          return err;
        }
        state->length += 8*block_size;
        state->curlen = 0;
      }
    }
  }
  return CRYPT_OK;
}

/**
   Terminate the hash to get the digest
   @param md  The hash state
   @param out [out] The destination of the hash (32 bytes)
   @return CRYPT_OK if successful
*/
int sha256_done(SHA256State *state, unsigned char *out)
{
  int i;
  
  if (state->curlen >= sizeof(state->buf)) {
    return 0; // CRYPT_INVALID_ARG
  }
  
  
  /* increase the length of the message */
  state->length += state->curlen * 8;
  
  /* append the '1' bit */
  state->buf[state->curlen++] = (unsigned char)0x80;
  
  /* if the length is currently above 56 bytes we append zeros
   * then compress.  Then we can fall back to padding zeros and length
   * encoding like normal.
   */
  if (state->curlen > 56) {
    while (state->curlen < 64) {
      state->buf[state->curlen++] = (unsigned char)0;
    }
    s_sha256_compress(state, state->buf);
    state->curlen = 0;
  }
  
  /* pad upto 56 bytes of zeroes */
  while (state->curlen < 56) {
    state->buf[state->curlen++] = (unsigned char)0;
  }
  
  /* store length */
  STORE64H(state->length, state->buf+56);
  s_sha256_compress(state, state->buf);
  
  /* copy output */
  for (i = 0; i < 8; i++) {
    STORE32H(state->state[i], out+(4*i));
  }
  return CRYPT_OK;
}

#undef Ch
#undef Maj
#undef S
#undef R
#undef Sigma0
#undef Sigma1
#undef Gamma0
#undef Gamma1

////////////////////////////////
//~ rjf: SHA1

typedef struct SHA1State SHA1State;
struct SHA1State
{
  U64 length;
  U32 state[5], curlen;
  unsigned char buf[64];
};

#define F0(x,y,z)  (z ^ (x & (y ^ z)))
#define F1(x,y,z)  (x ^ y ^ z)
#define F2(x,y,z)  ((x & y) | (z & (x | y)))
#define F3(x,y,z)  (x ^ y ^ z)

static int s_sha1_compress(SHA1State *state, const unsigned char *buf)
{
  U32 a,b,c,d,e,W[80],i;
  
  /* copy the state into 512-bits into W[0..15] */
  for (i = 0; i < 16; i++) {
    LOAD32H(W[i], buf + (4*i));
  }
  
  /* copy state */
  a = state->state[0];
  b = state->state[1];
  c = state->state[2];
  d = state->state[3];
  e = state->state[4];
  
  /* expand it */
  for (i = 16; i < 80; i++) {
    W[i] = ROL(W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16], 1);
  }
  
  /* compress */
  /* round one */
#define FF0(a,b,c,d,e,i) e = (ROLc(a, 5) + F0(b,c,d) + e + W[i] + 0x5a827999UL); b = ROLc(b, 30);
#define FF1(a,b,c,d,e,i) e = (ROLc(a, 5) + F1(b,c,d) + e + W[i] + 0x6ed9eba1UL); b = ROLc(b, 30);
#define FF2(a,b,c,d,e,i) e = (ROLc(a, 5) + F2(b,c,d) + e + W[i] + 0x8f1bbcdcUL); b = ROLc(b, 30);
#define FF3(a,b,c,d,e,i) e = (ROLc(a, 5) + F3(b,c,d) + e + W[i] + 0xca62c1d6UL); b = ROLc(b, 30);
  
#ifdef LTC_SMALL_CODE
  
  for (i = 0; i < 20; ) {
    FF0(a,b,c,d,e,i++); t = e; e = d; d = c; c = b; b = a; a = t;
  }
  
  for (; i < 40; ) {
    FF1(a,b,c,d,e,i++); t = e; e = d; d = c; c = b; b = a; a = t;
  }
  
  for (; i < 60; ) {
    FF2(a,b,c,d,e,i++); t = e; e = d; d = c; c = b; b = a; a = t;
  }
  
  for (; i < 80; ) {
    FF3(a,b,c,d,e,i++); t = e; e = d; d = c; c = b; b = a; a = t;
  }
  
#else
  
  for (i = 0; i < 20; ) {
    FF0(a,b,c,d,e,i++);
    FF0(e,a,b,c,d,i++);
    FF0(d,e,a,b,c,i++);
    FF0(c,d,e,a,b,i++);
    FF0(b,c,d,e,a,i++);
  }
  
  /* round two */
  for (; i < 40; )  {
    FF1(a,b,c,d,e,i++);
    FF1(e,a,b,c,d,i++);
    FF1(d,e,a,b,c,i++);
    FF1(c,d,e,a,b,i++);
    FF1(b,c,d,e,a,i++);
  }
  
  /* round three */
  for (; i < 60; )  {
    FF2(a,b,c,d,e,i++);
    FF2(e,a,b,c,d,i++);
    FF2(d,e,a,b,c,i++);
    FF2(c,d,e,a,b,i++);
    FF2(b,c,d,e,a,i++);
  }
  
  /* round four */
  for (; i < 80; )  {
    FF3(a,b,c,d,e,i++);
    FF3(e,a,b,c,d,i++);
    FF3(d,e,a,b,c,i++);
    FF3(c,d,e,a,b,i++);
    FF3(b,c,d,e,a,i++);
  }
#endif
  
#undef FF0
#undef FF1
#undef FF2
#undef FF3
  
  /* store */
  state->state[0] = state->state[0] + a;
  state->state[1] = state->state[1] + b;
  state->state[2] = state->state[2] + c;
  state->state[3] = state->state[3] + d;
  state->state[4] = state->state[4] + e;
  
  return CRYPT_OK;
}

/**
   Initialize the hash state
   @param md   The hash state you wish to initialize
   @return CRYPT_OK if successful
*/
int sha1_init(SHA1State *state)
{
  state->state[0] = 0x67452301UL;
  state->state[1] = 0xefcdab89UL;
  state->state[2] = 0x98badcfeUL;
  state->state[3] = 0x10325476UL;
  state->state[4] = 0xc3d2e1f0UL;
  state->curlen = 0;
  state->length = 0;
  return CRYPT_OK;
}

/**
   Process a block of memory though the hash
   @param md     The hash state
   @param in     The data to hash
   @param inlen  The length of the data (octets)
   @return CRYPT_OK if successful
*/
// HASH_PROCESS(sha1_process, s_sha1_compress, sha1, 64)
int sha1_process(SHA1State *state, const unsigned char *in, unsigned long inlen)
{
  unsigned long n;
  int           err;
  int block_size = 64;
  if(state->curlen > sizeof(state->buf))
  {
    return 0; // CRYPT_INVALID_ARG
  }
  if(((state->length + inlen * 8) < state->length) || ((inlen * 8) < inlen))
  {
    return 0; // CRYPT_HASH_OVERFLOW
  }
  while(inlen > 0)
  {
    if(state->curlen == 0 && inlen >= block_size)
    {
      if ((err = s_sha1_compress(state, in)) != CRYPT_OK)
      {
        return err;
      }
      state->length += block_size * 8;
      in             += block_size;
      inlen          -= block_size;
    } else {
      n = MIN(inlen, (block_size - state->curlen));
      MemoryCopy(state->buf + state->curlen, in, (size_t)n);
      state->curlen += n;
      in             += n;
      inlen          -= n;
      if(state->curlen == block_size)
      {
        if((err = s_sha1_compress(state, state->buf)) != CRYPT_OK)
        {
          return err;
        }
        state->length += 8*block_size;
        state->curlen = 0;
      }
    }
  }
  return CRYPT_OK;
}


/**
   Terminate the hash to get the digest
   @param md  The hash state
   @param out [out] The destination of the hash (20 bytes)
   @return CRYPT_OK if successful
*/
int sha1_done(SHA1State *state, unsigned char *out)
{
  int i;
  
  if (state->curlen >= sizeof(state->buf)) {
    return 0; // CRYPT_INVALID_ARG;
  }
  
  /* increase the length of the message */
  state->length += state->curlen * 8;
  
  /* append the '1' bit */
  state->buf[state->curlen++] = (unsigned char)0x80;
  
  /* if the length is currently above 56 bytes we append zeros
   * then compress.  Then we can fall back to padding zeros and length
   * encoding like normal.
   */
  if (state->curlen > 56) {
    while (state->curlen < 64) {
      state->buf[state->curlen++] = (unsigned char)0;
    }
    s_sha1_compress(state, state->buf);
    state->curlen = 0;
  }
  
  /* pad upto 56 bytes of zeroes */
  while (state->curlen < 56) {
    state->buf[state->curlen++] = (unsigned char)0;
  }
  
  /* store length */
  STORE64H(state->length, state->buf+56);
  s_sha1_compress(state, state->buf);
  
  /* copy output */
  for (i = 0; i < 5; i++) {
    STORE32H(state->state[i], out+(4*i));
  }
  return CRYPT_OK;
}

#undef F0
#undef F1
#undef F2
#undef F3
#undef FF0
#undef FF1
#undef FF2
#undef FF3

#endif // TOMCRYPT_HASH_H
