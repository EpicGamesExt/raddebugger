// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

/*
** Make sure we have an inlined function
*/

#if defined(_MSC_VER)
# define FORCE_INLINE __forceinline
#elif defined(__clang__) || defined(__GNUC__)
# define FORCE_INLINE  __attribute__((always_inline))
#else
# error need force inline for this compiler
#endif

////////////////////////////////
// NOTE(allen): Inline Stepping

unsigned int fixed_frac_bits = 5;
static unsigned int bias = 7;

static FORCE_INLINE unsigned int
fixed_mul(unsigned int a, unsigned int b){
  unsigned int c = (((a - bias)*(b - bias)) >> fixed_frac_bits) + bias;
  return(c);
}

static FORCE_INLINE unsigned int
multi_file_inlinesite(unsigned int x){
	// force compiler to generate annotations for code that's inside another file
#include "inline_body.cpp"
	return x >> fixed_frac_bits;
}

static unsigned int test_value = 0;

unsigned int
inline_stepping_tests(void){
  bias = 15;
  
  // NOTE(nick): Interesting that CL does not generate inline site symbols in order of apperance here unlike clang.
  
  // CL:
  //  BinaryAnnotations:    CodeLengthAndCodeOffset d 0
  //  BinaryAnnotation Length: 4 bytes (1 bytes padding)
  //
  // Clang:
  //  BinaryAnnotations:    LineOffset 1  CodeLength d
  //  BinaryAnnotation Length: 4 bytes (0 bytes padding)
  unsigned int x = fixed_mul(5001, 7121);
  
  // CL:
  //  BinaryAnnotations:    CodeOffsetAndLineOffset d  File 0  CodeOffsetAndLineOffset 22  LineOffset 1e
  //                        CodeLengthAndCodeOffset 2 3
  //  BinaryAnnotation Length: 12 bytes (1 bytes padding)
  //
  // Clang:
  //  BinaryAnnotations:  File 18  LineOffset ffffffe6  CodeOffset d  CodeOffsetAndLineOffset 22
  //                      File 0  LineOffset 1e  CodeOffset 3  CodeLength 2
  //  BinaryAnnotation Length: 16 bytes (0 bytes padding)
  unsigned int z = multi_file_inlinesite(x);
  return(z);
}

