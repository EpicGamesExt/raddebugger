/*
   BLAKE2 reference source code package - optimized C implementations

   Copyright 2012, Samuel Neves <sneves@dei.uc.pt>.  You may use this under the
   terms of the CC0, the OpenSSL Licence, or the Apache Public License 2.0, at
   your option.  The terms of these licenses can be found at:

   - CC0 1.0 Universal : http://creativecommons.org/publicdomain/zero/1.0
   - OpenSSL license   : https://www.openssl.org/source/license.html
   - Apache 2.0        : http://www.apache.org/licenses/LICENSE-2.0

   More information about the BLAKE2 hash function can be found at
   https://blake2.net.
*/

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "blake2.h"
#include "blake2-impl.h"

#include "blake2-config.h"


#include <emmintrin.h>
#if defined(HAVE_SSSE3)
#include <tmmintrin.h>
#endif
#if defined(HAVE_SSE41)
#include <smmintrin.h>
#endif
#if defined(HAVE_AVX)
#include <immintrin.h>
#endif
#if defined(HAVE_XOP)
#include <x86intrin.h>
#endif

#include "blake2s-round.h"

static const uint32_t blake2s_IV[8] =
{
  0x6A09E667UL, 0xBB67AE85UL, 0x3C6EF372UL, 0xA54FF53AUL,
  0x510E527FUL, 0x9B05688CUL, 0x1F83D9ABUL, 0x5BE0CD19UL
};

/* Some helper functions */
static void blake2s_set_lastnode( blake2s_state *S )
{
  S->f[1] = (uint32_t)-1;
}

static int blake2s_is_lastblock( const blake2s_state *S )
{
  return S->f[0] != 0;
}

static void blake2s_set_lastblock( blake2s_state *S )
{
  if( S->last_node ) blake2s_set_lastnode( S );

  S->f[0] = (uint32_t)-1;
}

static void blake2s_increment_counter( blake2s_state *S, const uint32_t inc )
{
  uint64_t t = ( ( uint64_t )S->t[1] << 32 ) | S->t[0];
  t += inc;
  S->t[0] = ( uint32_t )( t >>  0 );
  S->t[1] = ( uint32_t )( t >> 32 );
}

/* init2 xors IV with input parameter block */
int blake2s_init_param( blake2s_state *S, const blake2s_param *P )
{
  size_t i;
  /*blake2s_init0( S ); */
  const uint8_t * v = ( const uint8_t * )( blake2s_IV );
  const uint8_t * p = ( const uint8_t * )( P );
  uint8_t * h = ( uint8_t * )( S->h );
  /* IV XOR ParamBlock */
  memset( S, 0, sizeof( blake2s_state ) );

  for( i = 0; i < BLAKE2S_OUTBYTES; ++i ) h[i] = v[i] ^ p[i];

  S->outlen = P->digest_length;
  return 0;
}


/* Some sort of default parameter block initialization, for sequential blake2s */
int blake2s_init( blake2s_state *S, size_t outlen )
{
  blake2s_param P[1];

  /* Move interval verification here? */
  if ( ( !outlen ) || ( outlen > BLAKE2S_OUTBYTES ) ) return -1;

  P->digest_length = (uint8_t)outlen;
  P->key_length    = 0;
  P->fanout        = 1;
  P->depth         = 1;
  store32( &P->leaf_length, 0 );
  store32( &P->node_offset, 0 );
  store16( &P->xof_length, 0 );
  P->node_depth    = 0;
  P->inner_length  = 0;
  /* memset(P->reserved, 0, sizeof(P->reserved) ); */
  memset( P->salt,     0, sizeof( P->salt ) );
  memset( P->personal, 0, sizeof( P->personal ) );

  return blake2s_init_param( S, P );
}


int blake2s_init_key( blake2s_state *S, size_t outlen, const void *key, size_t keylen )
{
  blake2s_param P[1];

  /* Move interval verification here? */
  if ( ( !outlen ) || ( outlen > BLAKE2S_OUTBYTES ) ) return -1;

  if ( ( !key ) || ( !keylen ) || keylen > BLAKE2S_KEYBYTES ) return -1;

  P->digest_length = (uint8_t)outlen;
  P->key_length    = (uint8_t)keylen;
  P->fanout        = 1;
  P->depth         = 1;
  store32( &P->leaf_length, 0 );
  store32( &P->node_offset, 0 );
  store16( &P->xof_length, 0 );
  P->node_depth    = 0;
  P->inner_length  = 0;
  /* memset(P->reserved, 0, sizeof(P->reserved) ); */
  memset( P->salt,     0, sizeof( P->salt ) );
  memset( P->personal, 0, sizeof( P->personal ) );

  if( blake2s_init_param( S, P ) < 0 )
    return -1;

  {
    uint8_t block[BLAKE2S_BLOCKBYTES];
    memset( block, 0, BLAKE2S_BLOCKBYTES );
    memcpy( block, key, keylen );
    blake2s_update( S, block, BLAKE2S_BLOCKBYTES );
    secure_zero_memory( block, BLAKE2S_BLOCKBYTES ); /* Burn the key from stack */
  }
  return 0;
}


static void blake2s_compress( blake2s_state *S, const uint8_t block[BLAKE2S_BLOCKBYTES] )
{
  __m128i row1, row2, row3, row4;
  __m128i buf1, buf2, buf3, buf4;
#if defined(HAVE_SSE41)
  __m128i t0, t1;
#if !defined(HAVE_XOP)
  __m128i t2;
#endif
#endif
  __m128i ff0, ff1;
#if defined(HAVE_SSSE3) && !defined(HAVE_XOP)
  const __m128i r8 = _mm_set_epi8( 12, 15, 14, 13, 8, 11, 10, 9, 4, 7, 6, 5, 0, 3, 2, 1 );
  const __m128i r16 = _mm_set_epi8( 13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2 );
#endif
#if defined(HAVE_SSE41)
  const __m128i m0 = LOADU( block +  00 );
  const __m128i m1 = LOADU( block +  16 );
  const __m128i m2 = LOADU( block +  32 );
  const __m128i m3 = LOADU( block +  48 );
#else
  const uint32_t  m0 = load32(block +  0 * sizeof(uint32_t));
  const uint32_t  m1 = load32(block +  1 * sizeof(uint32_t));
  const uint32_t  m2 = load32(block +  2 * sizeof(uint32_t));
  const uint32_t  m3 = load32(block +  3 * sizeof(uint32_t));
  const uint32_t  m4 = load32(block +  4 * sizeof(uint32_t));
  const uint32_t  m5 = load32(block +  5 * sizeof(uint32_t));
  const uint32_t  m6 = load32(block +  6 * sizeof(uint32_t));
  const uint32_t  m7 = load32(block +  7 * sizeof(uint32_t));
  const uint32_t  m8 = load32(block +  8 * sizeof(uint32_t));
  const uint32_t  m9 = load32(block +  9 * sizeof(uint32_t));
  const uint32_t m10 = load32(block + 10 * sizeof(uint32_t));
  const uint32_t m11 = load32(block + 11 * sizeof(uint32_t));
  const uint32_t m12 = load32(block + 12 * sizeof(uint32_t));
  const uint32_t m13 = load32(block + 13 * sizeof(uint32_t));
  const uint32_t m14 = load32(block + 14 * sizeof(uint32_t));
  const uint32_t m15 = load32(block + 15 * sizeof(uint32_t));
#endif
  row1 = ff0 = LOADU( &S->h[0] );
  row2 = ff1 = LOADU( &S->h[4] );
  row3 = _mm_loadu_si128( (__m128i const *)&blake2s_IV[0] );
  row4 = _mm_xor_si128( _mm_loadu_si128( (__m128i const *)&blake2s_IV[4] ), LOADU( &S->t[0] ) );
  ROUND( 0 );
  ROUND( 1 );
  ROUND( 2 );
  ROUND( 3 );
  ROUND( 4 );
  ROUND( 5 );
  ROUND( 6 );
  ROUND( 7 );
  ROUND( 8 );
  ROUND( 9 );
  STOREU( &S->h[0], _mm_xor_si128( ff0, _mm_xor_si128( row1, row3 ) ) );
  STOREU( &S->h[4], _mm_xor_si128( ff1, _mm_xor_si128( row2, row4 ) ) );
}

int blake2s_update( blake2s_state *S, const void *pin, size_t inlen )
{
  const unsigned char * in = (const unsigned char *)pin;
  if( inlen > 0 )
  {
    size_t left = S->buflen;
    size_t fill = BLAKE2S_BLOCKBYTES - left;
    if( inlen > fill )
    {
      S->buflen = 0;
      memcpy( S->buf + left, in, fill ); /* Fill buffer */
      blake2s_increment_counter( S, BLAKE2S_BLOCKBYTES );
      blake2s_compress( S, S->buf ); /* Compress */
      in += fill; inlen -= fill;
      while(inlen > BLAKE2S_BLOCKBYTES) {
        blake2s_increment_counter(S, BLAKE2S_BLOCKBYTES);
        blake2s_compress( S, in );
        in += BLAKE2S_BLOCKBYTES;
        inlen -= BLAKE2S_BLOCKBYTES;
      }
    }
    memcpy( S->buf + S->buflen, in, inlen );
    S->buflen += inlen;
  }
  return 0;
}

int blake2s_final( blake2s_state *S, void *out, size_t outlen )
{
  uint8_t buffer[BLAKE2S_OUTBYTES] = {0};
  size_t i;

  if( out == NULL || outlen < S->outlen )
    return -1;

  if( blake2s_is_lastblock( S ) )
    return -1;

  blake2s_increment_counter( S, (uint32_t)S->buflen );
  blake2s_set_lastblock( S );
  memset( S->buf + S->buflen, 0, BLAKE2S_BLOCKBYTES - S->buflen ); /* Padding */
  blake2s_compress( S, S->buf );

  for( i = 0; i < 8; ++i ) /* Output full hash to temp buffer */
    store32( buffer + sizeof( S->h[i] ) * i, S->h[i] );

  memcpy( out, buffer, S->outlen );
  secure_zero_memory( buffer, sizeof(buffer) );
  return 0;
}

/* inlen, at least, should be uint64_t. Others can be size_t. */
int blake2s( void *out, size_t outlen, const void *in, size_t inlen, const void *key, size_t keylen )
{
  blake2s_state S[1];

  /* Verify parameters */
  if ( NULL == in && inlen > 0 ) return -1;

  if ( NULL == out ) return -1;

  if ( NULL == key && keylen > 0) return -1;

  if( !outlen || outlen > BLAKE2S_OUTBYTES ) return -1;

  if( keylen > BLAKE2S_KEYBYTES ) return -1;

  if( keylen > 0 )
  {
    if( blake2s_init_key( S, outlen, key, keylen ) < 0 ) return -1;
  }
  else
  {
    if( blake2s_init( S, outlen ) < 0 ) return -1;
  }

  blake2s_update( S, ( const uint8_t * )in, inlen );
  blake2s_final( S, out, outlen );
  return 0;
}

#if defined(SUPERCOP)
int crypto_hash( unsigned char *out, unsigned char *in, unsigned long long inlen )
{
  return blake2s( out, BLAKE2S_OUTBYTES, in, inlen, NULL, 0 );
}
#endif

#if defined(BLAKE2S_SELFTEST)
#include <string.h>
#include "blake2-kat.h"
int main( void )
{
  uint8_t key[BLAKE2S_KEYBYTES];
  uint8_t buf[BLAKE2_KAT_LENGTH];
  size_t i, step;

  for( i = 0; i < BLAKE2S_KEYBYTES; ++i )
    key[i] = ( uint8_t )i;

  for( i = 0; i < BLAKE2_KAT_LENGTH; ++i )
    buf[i] = ( uint8_t )i;

  /* Test simple API */
  for( i = 0; i < BLAKE2_KAT_LENGTH; ++i )
  {
    uint8_t hash[BLAKE2S_OUTBYTES];
    blake2s( hash, BLAKE2S_OUTBYTES, buf, i, key, BLAKE2S_KEYBYTES );

    if( 0 != memcmp( hash, blake2s_keyed_kat[i], BLAKE2S_OUTBYTES ) )
    {
      goto fail;
    }
  }

  /* Test streaming API */
  for(step = 1; step < BLAKE2S_BLOCKBYTES; ++step) {
    for (i = 0; i < BLAKE2_KAT_LENGTH; ++i) {
      uint8_t hash[BLAKE2S_OUTBYTES];
      blake2s_state S;
      uint8_t * p = buf;
      size_t mlen = i;
      int err = 0;

      if( (err = blake2s_init_key(&S, BLAKE2S_OUTBYTES, key, BLAKE2S_KEYBYTES)) < 0 ) {
        goto fail;
      }

      while (mlen >= step) {
        if ( (err = blake2s_update(&S, p, step)) < 0 ) {
          goto fail;
        }
        mlen -= step;
        p += step;
      }
      if ( (err = blake2s_update(&S, p, mlen)) < 0) {
        goto fail;
      }
      if ( (err = blake2s_final(&S, hash, BLAKE2S_OUTBYTES)) < 0) {
        goto fail;
      }

      if (0 != memcmp(hash, blake2s_keyed_kat[i], BLAKE2S_OUTBYTES)) {
        goto fail;
      }
    }
  }

  puts( "ok" );
  return 0;
fail:
  puts("error");
  return -1;
}
#endif
