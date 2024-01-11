/*
   BLAKE2 reference source code package - reference C implementations

   Copyright 2016, JP Aumasson <jeanphilippe.aumasson@gmail.com>.
   Copyright 2016, Samuel Neves <sneves@dei.uc.pt>.

   You may use this under the terms of the CC0, the OpenSSL Licence, or
   the Apache Public License 2.0, at your option.  The terms of these
   licenses can be found at:

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

int blake2xs_init( blake2xs_state *S, const size_t outlen ) {
  return blake2xs_init_key(S, outlen, NULL, 0);
}

int blake2xs_init_key( blake2xs_state *S, const size_t outlen, const void *key, size_t keylen )
{
  if ( outlen == 0 || outlen > 0xFFFFUL ) {
    return -1;
  }

  if (NULL != key && keylen > BLAKE2S_KEYBYTES) {
    return -1;
  }

  if (NULL == key && keylen > 0) {
    return -1;
  }

  /* Initialize parameter block */
  S->P->digest_length = BLAKE2S_OUTBYTES;
  S->P->key_length    = keylen;
  S->P->fanout        = 1;
  S->P->depth         = 1;
  store32( &S->P->leaf_length, 0 );
  store32( &S->P->node_offset, 0 );
  store16( &S->P->xof_length, outlen );
  S->P->node_depth    = 0;
  S->P->inner_length  = 0;
  memset( S->P->salt,     0, sizeof( S->P->salt ) );
  memset( S->P->personal, 0, sizeof( S->P->personal ) );

  if( blake2s_init_param( S->S, S->P ) < 0 ) {
    return -1;
  }

  if (keylen > 0) {
    uint8_t block[BLAKE2S_BLOCKBYTES];
    memset(block, 0, BLAKE2S_BLOCKBYTES);
    memcpy(block, key, keylen);
    blake2s_update(S->S, block, BLAKE2S_BLOCKBYTES);
    secure_zero_memory(block, BLAKE2S_BLOCKBYTES);
  }
  return 0;
}

int blake2xs_update( blake2xs_state *S, const void *in, size_t inlen ) {
  return blake2s_update( S->S, in, inlen );
}

int blake2xs_final(blake2xs_state *S, void *out, size_t outlen) {

  blake2s_state C[1];
  blake2s_param P[1];
  uint16_t xof_length = load16(&S->P->xof_length);
  uint8_t root[BLAKE2S_BLOCKBYTES];
  size_t i;

  if (NULL == out) {
    return -1;
  }

  /* outlen must match the output size defined in xof_length, */
  /* unless it was -1, in which case anything goes except 0. */
  if(xof_length == 0xFFFFUL) {
    if(outlen == 0) {
      return -1;
    }
  } else {
    if(outlen != xof_length) {
      return -1;
    }
  }

  /* Finalize the root hash */
  if (blake2s_final(S->S, root, BLAKE2S_OUTBYTES) < 0) {
    return -1;
  }

  /* Set common block structure values */
  /* Copy values from parent instance, and only change the ones below */
  memcpy(P, S->P, sizeof(blake2s_param));
  P->key_length = 0;
  P->fanout = 0;
  P->depth = 0;
  store32(&P->leaf_length, BLAKE2S_OUTBYTES);
  P->inner_length = BLAKE2S_OUTBYTES;
  P->node_depth = 0;

  for (i = 0; outlen > 0; ++i) {
    const size_t block_size = (outlen < BLAKE2S_OUTBYTES) ? outlen : BLAKE2S_OUTBYTES;
    /* Initialize state */
    P->digest_length = block_size;
    store32(&P->node_offset, i);
    blake2s_init_param(C, P);
    /* Process key if needed */
    blake2s_update(C, root, BLAKE2S_OUTBYTES);
    if (blake2s_final(C, (uint8_t *)out + i * BLAKE2S_OUTBYTES, block_size) < 0) {
        return -1;
    }
    outlen -= block_size;
  }
  secure_zero_memory(root, sizeof(root));
  secure_zero_memory(P, sizeof(P));
  secure_zero_memory(C, sizeof(C));
  /* Put blake2xs in an invalid state? cf. blake2s_is_lastblock */
  return 0;
}

int blake2xs(void *out, size_t outlen, const void *in, size_t inlen, const void *key, size_t keylen)
{
  blake2xs_state S[1];

  /* Verify parameters */
  if (NULL == in && inlen > 0)
    return -1;

  if (NULL == out)
    return -1;

  if (NULL == key && keylen > 0)
    return -1;

  if (keylen > BLAKE2S_KEYBYTES)
    return -1;

  if (outlen == 0)
    return -1;

  /* Initialize the root block structure */
  if (blake2xs_init_key(S, outlen, key, keylen) < 0) {
    return -1;
  }

  /* Absorb the input message */
  blake2xs_update(S, in, inlen);

  /* Compute the root node of the tree and the final hash using the counter construction */
  return blake2xs_final(S, out, outlen);
}

#if defined(BLAKE2XS_SELFTEST)
#include <string.h>
#include "blake2-kat.h"
int main( void )
{
  uint8_t key[BLAKE2S_KEYBYTES];
  uint8_t buf[BLAKE2_KAT_LENGTH];
  size_t i, step, outlen;

  for( i = 0; i < BLAKE2S_KEYBYTES; ++i ) {
    key[i] = ( uint8_t )i;
  }

  for( i = 0; i < BLAKE2_KAT_LENGTH; ++i ) {
    buf[i] = ( uint8_t )i;
  }

  /* Testing length of outputs rather than inputs */
  /* (Test of input lengths mostly covered by blake2s tests) */

  /* Test simple API */
  for( outlen = 1; outlen <= BLAKE2_KAT_LENGTH; ++outlen )
  {
      uint8_t hash[BLAKE2_KAT_LENGTH] = {0};
      if( blake2xs( hash, outlen, buf, BLAKE2_KAT_LENGTH, key, BLAKE2S_KEYBYTES ) < 0 ) {
        goto fail;
      }

      if( 0 != memcmp( hash, blake2xs_keyed_kat[outlen-1], outlen ) )
      {
        goto fail;
      }
  }

  /* Test streaming API */
  for(step = 1; step < BLAKE2S_BLOCKBYTES; ++step) {
    for (outlen = 1; outlen <= BLAKE2_KAT_LENGTH; ++outlen) {
      uint8_t hash[BLAKE2_KAT_LENGTH];
      blake2xs_state S;
      uint8_t * p = buf;
      size_t mlen = BLAKE2_KAT_LENGTH;
      int err = 0;

      if( (err = blake2xs_init_key(&S, outlen, key, BLAKE2S_KEYBYTES)) < 0 ) {
        goto fail;
      }

      while (mlen >= step) {
        if ( (err = blake2xs_update(&S, p, step)) < 0 ) {
          goto fail;
        }
        mlen -= step;
        p += step;
      }
      if ( (err = blake2xs_update(&S, p, mlen)) < 0) {
        goto fail;
      }
      if ( (err = blake2xs_final(&S, hash, outlen)) < 0) {
        goto fail;
      }

      if (0 != memcmp(hash, blake2xs_keyed_kat[outlen-1], outlen)) {
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
