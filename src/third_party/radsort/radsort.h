// New radsort.

// To Use:
//   Create a less_than function and then call radsort.
//
//   So, for an array of unsigned ints:
//
//     RSFORCEINLINE int int_is_before( void * elementa, void * elementb )
//     {
//       return *(unsigned int*)elementa < *(unsigned int*)elementb;
//     }
//
//     radsort( buffer, count, int_is_before, unsigned int ); // type of each element is the last parameter
//
//     If you comparison function is very complicated, then you might try
//       dropping the RSFORCEINLINE.

#include <stddef.h> // for size_t

#ifdef _MSC_VER
#define RSFORCEINLINE __forceinline __declspec(safebuffers)
#define CompilerReset(ptr) __assume(ptr)
#else
#define RSFORCEINLINE __attribute__((always_inline))
#define CompilerReset(ptr)
#endif

#if defined(_x86_64) || defined( __x86_64__ ) || defined( _M_X64 ) || defined(__x86_64) || defined(_M_AMD64) || defined(__SSE__) || defined(__SSE2__) || defined(USE_SSE) 
#include <xmmintrin.h>
#define RS_PREFETCH( addr ) _mm_prefetch( (addr), 0 )
#endif

// nonsense to make adding pointers a more convenient
#define rsadd_ptr( ptr, ind ) (((char*)(ptr))+(ptrdiff_t)(ind))
#define rssub_ptr( ptr, ind ) (((char*)(ptr))-(ptrdiff_t)(ind))
#define rsadd_ptr_elements( ptr, ind ) rsadd_ptr( ptr, (ptrdiff_t)(ind)*(ptrdiff_t)element_size )
#define rsdiff_ptr_elements( ptra, ptrb ) ( (size_t)(((char*)(ptra))-((char*)(ptrb))) / (size_t)element_size )

// this is the maximum size of struct that we treat as a "simple" struct
typedef struct RS_MAX_SIMPLE_BUF { char b[32]; } RS_MAX_SIMPLE_BUF;  // todo, 64-bit


// ==============================================================================================================
//  swap and move utility functions
typedef struct bytes64 { char b[64]; } bytes64;  // copying with this turns into m512 moves (when arch is set)
typedef struct bytes32 { char b[32]; } bytes32;  // copying with this turns into m256 moves (when arch is set)
typedef struct bytes16 { char b[16]; } bytes16;  // copying with this turns into m128 moves
typedef struct bytes8  { char b[8];  } bytes8;

static RSFORCEINLINE void radsortswapper( void * a, void * b, size_t size )
{
  #define RSSWAPMEM(type) ( size >= sizeof(type) ) { type v = *(type const*)a; *(type*)a = *(type const*)b; *(type*)b = v; a=rsadd_ptr(a,sizeof(type)); b=rsadd_ptr(b,sizeof(type)); size -= sizeof(type); }
  
  while RSSWAPMEM(bytes64);
  if RSSWAPMEM(bytes32);
  if RSSWAPMEM(bytes16);
  if RSSWAPMEM(bytes8);
  if RSSWAPMEM(int);
  if RSSWAPMEM(short);
  if RSSWAPMEM(char);
  
  #undef RSSWAPMEM
}

// since size is always constant, this big function compiles down to 4 to 12 instructions (for normal structs 4-6)
static RSFORCEINLINE void radsortmover( void * a, void * b, size_t size )  
{
  #define RSMOVEMEM(type) ( size >= sizeof(type) ) { *(type*)a = *(type const*)b; a=rsadd_ptr(a,sizeof(type)); b=rsadd_ptr(b,sizeof(type)); size -= sizeof(type); }
  
  while RSMOVEMEM(bytes64);
  if RSMOVEMEM(bytes32);
  if RSMOVEMEM(bytes16);
  if RSMOVEMEM(bytes8);
  if RSMOVEMEM(int);
  if RSMOVEMEM(short);
  if RSMOVEMEM(char);
  
  #undef RSMOVEMEM
}

// these macros generate tiny move/swap routines that don't go through the generic function above  (mostly for debug build performance)
#define RS_SIMPLE_SIZES _X(1) _X(2) _X(4) _X(8) _X(12) _X(16)
#define rsmoverfunc( num )   static RSFORCEINLINE void radsortmover##num  ( void * dest, void * src, size_t element_size ) { typedef struct rs { char x[num]; } rs; *(rs*)dest = *(rs*)src; }
#define rsswapperfunc( num ) static RSFORCEINLINE void radsortswapper##num( void * a,    void * b,   size_t element_size ) { typedef struct rs { char x[num]; } rs; rs temp; temp = *(rs*)a; *(rs*)a = *(rs*)b; *(rs*)b = temp; }

#define _X rsmoverfunc
RS_SIMPLE_SIZES
#undef _X
#define _X rsswapperfunc
RS_SIMPLE_SIZES
#undef _X

#undef RS_SIMPLE_SIZES
#undef rsmoverfunc
#undef rsswapperfunc


// ==============================================================================================================

typedef int is_before_func( void * elementa, void * elementb );
typedef void swap_func( void * elementa, void * elementb, size_t element_size );
typedef void move_func( void * dest, void * src, size_t size );
typedef void rs_small_sort_func( void * left, size_t n, size_t element_size, is_before_func * is_before, move_func * mover, void * tmp );

#define radsortswapsize( size ) ( ( size == 1 ) ? radsortswapper1 : ( ( size == 2 ) ? radsortswapper2 : ( ( size == 4 ) ? radsortswapper4 : ( ( size == 8 ) ? radsortswapper8 : ( ( size == 12 ) ? radsortswapper12 : ( ( size == 16 ) ? radsortswapper16 : radsortswapper ) ) ) ) ) )
#define radsortmovesize( size ) ( ( size == 1 ) ? radsortmover1   : ( ( size == 2 ) ? radsortmover2   : ( ( size == 4 ) ? radsortmover4   : ( ( size == 8 ) ? radsortmover8   : ( ( size == 12 ) ? radsortmover12   : ( ( size == 16 ) ? radsortmover16   : radsortmover   ) ) ) ) ) )

// todo - maybe no bubble at all?
#define RS_SMALL_FLIP_TO_INSERTION_GT_SIZE sizeof( size_t )
typedef struct RS_MAX_BUBBLE_BUF { char b[RS_SMALL_FLIP_TO_INSERTION_GT_SIZE]; } RS_MAX_BUBBLE_BUF; 

#define radsort( start, len, is_before_func ) \
  do { \
    char __rs_tmp[ sizeof( (start)[0] ) ]; \
    radsortinternal( start, len, sizeof( (start)[0] ), \
                     is_before_func, \
                     radsortswapsize( sizeof( (start)[0] ) ), \
                     radsortmovesize( sizeof( (start)[0] ) ), \
                     ( sizeof( (start)[0] ) > RS_SMALL_FLIP_TO_INSERTION_GT_SIZE ) ? radinsertionsort : radbubble2sort, \
                     ( sizeof( (start)[0] ) > RS_SMALL_FLIP_TO_INSERTION_GT_SIZE ) ? RSS_FLIP_TO_SMALL_SORT_INSERTION : RSS_FLIP_TO_SMALL_SORT_BUBBLE2, \
                     &__rs_tmp \
                   ); \
  } while (0)
#define radheapsort( start, len, is_before_func )  do { radheapsortinteral( start, len, sizeof( ((start)[0]) ), is_before_func, radsortswapsize( sizeof( ((start)[0]) ) ) ); } while (0)


//===================================================================================================
// small heap sort - this sort is around 200 bytes compiled - can use directly when size is important

RSFORCEINLINE void radheapsortinteral( void * start, size_t len, size_t element_size, is_before_func * is_before, swap_func * swapper )
{
  void * left;
  void * right;
  size_t length;

  left = start;
  right = rsadd_ptr_elements( start, len - 1 );
  length = len;

  if ( length > 1 )
  {
    // unusual small in-place heap sort
    void * i; void * ind; void * v; void * n;
    size_t s, k;

    s = length >> 1;
    i = rsadd_ptr_elements( left, s );

    for(;;)
    {
      --s;
      i = rsadd_ptr_elements( i, -1 );
      ind = i;
      k = ( s << 1 ) + 1;

      for(;;)
      {
        v = rsadd_ptr_elements( left, k );
        n = rsadd_ptr_elements( v, 1 );
        
        if ( ( ( n <= right ) ) && ( is_before( v, n ) ) )
        {
          ++k;
          v = n;
        }

        if ( is_before( ind, v ) )   
        {
          swapper( ind, v, element_size );
          ind = v;
          k = ( k << 1 ) + 1;

          if ( k < length )
            continue;
        }

        // if s is non-zero, we are still building the heap!
        if ( s ) 
          break;

        swapper( left, right, element_size );
        right = rsadd_ptr_elements( right, -1 );
        ind = left;
        k = 1;
        --length;
        
        if ( length <= 1 )
          return;
      }
    }
  } 
}

//===================================================================================================
//  median routines

#define rsswapsmaller( X, Y ) { RS_MAX_SIMPLE_BUF tmp; int cond; cond = is_before( &Y, &X); mover( &tmp, &X, element_size ); if ( cond ) mover( &X, &Y, element_size ); if ( cond ) mover( &Y, &tmp, element_size ); } 

static RSFORCEINLINE void radsortgetmedian5( void * output, void * left, void * right, size_t length, size_t element_size, is_before_func * is_before, swap_func * swapper, move_func * mover )
{
  RS_MAX_SIMPLE_BUF mb0,mb1,mb2,mb3,mb4; 

  mover( &mb0, left, element_size ); 
  mover( &mb1, rsadd_ptr_elements( left, length >> 2 ), element_size ); 
  mover( &mb2, rsadd_ptr_elements( left, length >> 1 ), element_size ); 
  mover( &mb3, rsadd_ptr_elements( left, length - (length >> 2) ), element_size ); 
  mover( &mb4, right, element_size ); 

  // Basically, for simple compares, and for simple in-register types, this funcion 
  //   must turn info 7 compares and then 5-7 movs, and 12 cmovs.  Any 
  //   compiler *should* do this - if this doesn't happen, then the compiler is 
  //   hosing you. You can put int 3s at the start and end of this function to check.

  rsswapsmaller( mb0, mb1 ); 
  rsswapsmaller( mb2, mb3 ); 
  rsswapsmaller( mb0, mb2 ); 
  rsswapsmaller( mb1, mb3 ); 
  rsswapsmaller( mb1, mb4 ); 
  rsswapsmaller( mb1, mb2 ); 
  
  mover( output, &mb2, element_size ); 
  if ( is_before( &mb4, &mb2 ) ) mover( output, &mb4, element_size );
}


static RSFORCEINLINE void radsortgetmedian9( void * output, void * left, void * right, size_t length, size_t element_size, is_before_func * is_before, swap_func * swapper, move_func * mover )
{
  RS_MAX_SIMPLE_BUF mb0,mb1,mb2,mb3,mb4,mb5,mb6,mb7,mb8; // todo, temp mem!

  #ifdef RS_PREFETCH
  RS_PREFETCH( left ); 
  RS_PREFETCH( right ); 
  RS_PREFETCH( rsadd_ptr_elements( left, length >> 3 ) ); 
  RS_PREFETCH( rsadd_ptr_elements( left, length >> 2 ) ); 
  RS_PREFETCH( rsadd_ptr_elements( left, (length >> 1) - (length >> 3) ) ); 
  RS_PREFETCH( rsadd_ptr_elements( left, length >> 1 ) ); 
  RS_PREFETCH( rsadd_ptr_elements( left, (length >> 1) + (0 >> 3) ) ); 
  RS_PREFETCH( rsadd_ptr_elements( left, length - (length >> 2) ) ); 
  RS_PREFETCH( rsadd_ptr_elements( left, length - (length >> 3) ) ); 
  #endif

  mover( &mb0, left, element_size ); 
  mover( &mb1, rsadd_ptr_elements( left, length >> 3 ), element_size ); 
  mover( &mb2, rsadd_ptr_elements( left, length >> 2 ), element_size ); 
  mover( &mb3, rsadd_ptr_elements( left, (length >> 1) - (length >> 3) ), element_size ); 
  mover( &mb4, rsadd_ptr_elements( left, length >> 1 ), element_size ); 
  mover( &mb5, rsadd_ptr_elements( left, (length >> 1) + (length >> 3) ), element_size ); 
  mover( &mb6, rsadd_ptr_elements( left, length - (length >> 2) ), element_size ); 
  mover( &mb7, rsadd_ptr_elements( left, length - (length >> 3) ), element_size ); 
  mover( &mb8, right, element_size ); 

  // Basically, for simple compares, and for simple in-register types, this funcion 
  //   should turn info 19 compares and then 15-19 movs, and 36 cmovs. However,  
  //   most compilers can only so-so job at this, and you'll end up with 3-4 jumps.
  //   We just need cmov intrinsics.
  
  rsswapsmaller( mb0, mb7 ); 
  rsswapsmaller( mb1, mb2 ); 
  rsswapsmaller( mb3, mb5 ); 
  rsswapsmaller( mb4, mb8 ); 
  rsswapsmaller( mb0, mb2 ); 
  rsswapsmaller( mb1, mb5 ); 
  rsswapsmaller( mb3, mb8 ); 
  rsswapsmaller( mb4, mb7 ); 
  rsswapsmaller( mb0, mb3 ); 
  rsswapsmaller( mb1, mb4 ); 
  rsswapsmaller( mb2, mb8 ); 
  rsswapsmaller( mb5, mb7 ); 
  rsswapsmaller( mb3, mb4 ); 
  rsswapsmaller( mb5, mb6 ); 
  rsswapsmaller( mb2, mb5 ); 
  rsswapsmaller( mb4, mb6 ); 
  rsswapsmaller( mb2, mb3 ); 
  rsswapsmaller( mb4, mb5 ); 
 
  mover( output, &mb3, element_size ); 
  if ( is_before( &mb4, &mb3 ) ) mover( output, &mb4, element_size );
}

#define RSS_USE_MEDIAN_9   1024

static RSFORCEINLINE void radsortgetmedian( void * output, void * left, void * right, size_t length, size_t element_size, is_before_func * is_before, swap_func * swapper, move_func * mover )
{
  // get the median into copy
  if ( length >= RSS_USE_MEDIAN_9 )
    radsortgetmedian9( output, left, right, length, element_size, is_before, swapper, mover );
  else
    radsortgetmedian5( output, left, right, length, element_size, is_before, swapper, mover );
}



//===================================================================================================
//  bubble 2 routines - for partitions <= 16 count

// from Gerben Stavenga - bubble sort moving two values through at once
//   for ints, this compiles down to 38 instructions
#define RSS_FLIP_TO_SMALL_SORT_BUBBLE2  16
static RSFORCEINLINE void radbubble2sort( void * left, size_t n, size_t element_size, is_before_func * is_before, move_func * mover, void * tmp ) 
{
  void * i;  // todo - test with bigger blocks
  void * s = rsadd_ptr_elements( left, 2 );
  RS_MAX_BUBBLE_BUF x, y, z;

  #define rsbubbleswap( X, Y ) { int cond; cond = is_before( &Y, &X); mover( tmp, &X, element_size ); if ( cond ) mover( &X, &Y, element_size ); if ( cond ) mover( &Y, tmp, element_size ); } 

  for ( i = rsadd_ptr_elements( left, (int)n - 1 ) ; i > left ; i = rsadd_ptr_elements( i, -2 ) )
  {
    void * j, * jm2;

    // load x & y
    mover( &x, left, element_size );
    mover( &y, rsadd_ptr_elements( left, 1 ), element_size );
    
    // swap x & y, so that x is smaller than y    
    rsbubbleswap( x, y );
    
    // for ints, this loop needs to be 4 cmps, 6 cmovs, and 5 movs
    //  anything else will kill performance

    jm2 = left;
    for ( j = s ; j <= i ; j = rsadd_ptr_elements( j, 1 ) )
    {
      // make z smaller than x and y, and the dump it to the left
      mover( &z, j, element_size );
      rsbubbleswap( z, x );
      rsbubbleswap( z, y );
      rsbubbleswap( x, y );
      mover( jm2, &z, element_size );
      jm2 = rsadd_ptr_elements( jm2, 1 );
    }

    mover( rsadd_ptr_elements( i, -1 ), &x, element_size );
    mover( i, &y, element_size );
  }
}

#define RSS_FLIP_TO_SMALL_SORT_INSERTION  28
static RSFORCEINLINE void radinsertionsort(void * start, size_t len, size_t element_size, is_before_func * is_before, move_func * mover, void * tmp ) 
{
  void * cur;
  void * prev;

  cur = rsadd_ptr_elements( start, 1 );
  --len;
  prev = start;
  do
  {
    void * comp = cur;
    if ( is_before( comp, prev ) )
    {
      mover( tmp, comp, element_size );
      do
      {
        mover( comp, prev, element_size );
        comp = rsadd_ptr_elements( comp, -1 );
        if ( comp == start )
          break;
        prev = rsadd_ptr_elements( prev, -1 );
      } while ( is_before( tmp, prev ) );
      mover( comp, tmp, element_size );
    }
    prev = cur;
    cur = rsadd_ptr_elements( cur, 1 );
  } while( --len );
}

/*
todo
static void * rs_start;
static is_before_func * rs_ib;
static size_t rs_es;

static RSFORCEINLINE int rss_byte_is_before_func( void * elementa, void * elementb )
{
  unsigned char a = *(unsigned char*)elementa;
  unsigned char b = *(unsigned char*)elementb;
  size_t element_size = rs_es;
  return rs_ib( rsadd_ptr_elements( rs_start, a ), rsadd_ptr_elements( rs_start, b ) );
}


// do bubble sort of offsets, and THEN do all the swaps - faster on biy structures
static RSFORCEINLINE void radsortbubble2offsets( void * left, size_t n, size_t element_size, is_before_func * is_before, swap_func * swapper, move_func * mover ) 
{
  static unsigned char init[16] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
  unsigned char offsets[16];
  unsigned char swap[16];

  radsortmover16( offsets, init, 16 );
  radsortmover16( swap, init, 16 );
  rs_start = left;
rs_ib = is_before;
rs_es = element_size;

  // sort the byte offsets
  radsortbubble2( offsets, n, 1, rss_byte_is_before_func, radsortmover1 );

  // now reorder the data
  {
    unsigned char i;
    void * ip = left;

    for( i = 0 ; i < (unsigned char)n ; i++ )
    {
      unsigned char j = swap[ offsets[ i ] ];
      if ( i != j )
      {
        swapper( ip, rsadd_ptr_elements( left, j ), element_size );
        swap[ j ] = swap[ i ];
      }
      ip = rsadd_ptr_elements( ip, 1 );
    }
  }
}
*/
//===================================================================================================

#undef rsswapsmaller

#define RSS_MAX_RECURSE        128 

RSFORCEINLINE void radsortinternal( void * start, size_t len, size_t element_size, is_before_func * is_before, swap_func * swapper, move_func * mover, rs_small_sort_func * small_sort, size_t small_sort_thres, void * tmp )
{
  void * left;
  size_t length;

  if ( len <= 1 )
    return;

  #if _DEBUG
    if ( element_size > sizeof( RS_MAX_SIMPLE_BUF ) )
      __debugbreak();
  #endif

  // stack for no recursion
  typedef struct stks
  {
    void * left;
    size_t len;
  } stks;

  stks stk[ RSS_MAX_RECURSE ];
  stks * stk_ptr = stk + RSS_MAX_RECURSE;

  // we use the stk_ptr to tell when to flip to heap.
  //   when we hit the end of the stack, we heap it, so
  //    back the start of the stack to log1.5 of len
  length = len;
  do {
    --stk_ptr;
    if ( stk_ptr == stk ) { stk_ptr = stk+1; break; }
    length = ( length >> 1 ) + ( length >> 2 );
  } while ( length );
  stk_ptr[ -1 ].len = 0;

  left = start;
  length = len;

  do
  {
    for(;;)
    {
      // if tiny, hand with insertion
      if ( length <= small_sort_thres )
      {
        CompilerReset(left); // we reset the compiler before each major sort
        small_sort( left, length, element_size, is_before, mover, tmp );
        break;
      }
      else
      {
        // if we have hit end of our recursion stack, flip to using a heap (this prevents N^2 behavior)
        if ( stk_ptr >= stk + RSS_MAX_RECURSE )
        {
          CompilerReset(left); // we reset the compiler before each major sort
          //printf("heap: %d\n",(int)length);
          radheapsortinteral( left, length, element_size, is_before, swapper );
          break;
        }
        else
        {
          // partition
          void * rightequalpiv;
          size_t leftlen;
          void * scan, * piv, * rend, * right;

          CompilerReset(left); // we reset the compiler before each major sort

          right = rsadd_ptr_elements( left, length - 1 );

          // check for and correct inverted blocks 
          scan = left;
          rend = right;
          while ( is_before( rend, scan ) )
          {
            swapper( rend, scan, element_size );
            scan = rsadd_ptr_elements( scan, 1 );
            rend = rsadd_ptr_elements( rend, -1 );
            if ( scan >= rend ) break;
          }

          // scan to see if the block is in order (or all the same)
          scan = left;
          do
          {
            void * next = rsadd_ptr( scan, element_size );
            if ( is_before( next, scan ) )
              goto doqsort;
            scan = next;
          } while ( scan < right );
          // if we get out of the loop cleanly, this block is already sorted, so just fall out and do next block
          break;

         doqsort:

          // get the median into copy
          radsortgetmedian( tmp, left, right, length, element_size, is_before, swapper, mover );

          // if scan != left, then we have a few in order, so we can skip them all if the final is under the copy
          if ( !is_before( scan, tmp ) )
            scan = left;
          // this loop should be 3 instructions
          // skip values below the pivot at the start of the segment 
          while( is_before( scan, tmp ) ) // the pivot will stop this loop
            scan = rsadd_ptr( scan, element_size );

          // skip values above and equal to the pivot at the end of the segment
          rend = right;
          if ( left == start )
          {
            // we have to use this loop to check that we don't read off the front of 
            //   the array this loop should be 5 instructions
            while( rend > scan )   
            {
              if ( !is_before( tmp, rend ) )
                break;
              rend = rsadd_ptr_elements( rend, -1 );
            }
          }
          else
          {
            // if we're not at the very start of the entire buffer, then we
            //  can use this loop, which is only 3 instructions
            while( is_before( tmp, rend ) ) // the pivot will stop this loop
              rend = rsadd_ptr_elements( rend, -1 );
          }

          // finally, do actual partitioning nanosort style - 65-70% of the 
          //   total time will be in this loop, for ints, this should be
          //   4 movs, 2 cmps, 1 cmov, 2 add, 1 jmp - 10 instructions
          // compilers getting this wrong is a 50-100% slowdown! You can
          //   check the output by putting int 3s around this loop.
          CompilerReset(scan);
          piv = scan;
          while( scan <= rend )
          {
            size_t adv = is_before( scan, tmp );
            swapper( piv, scan, element_size );
            if ( adv ) piv = rsadd_ptr( piv, element_size ); // needs to be a cmov
            scan = rsadd_ptr( scan, element_size );
          }

          // now move the right side to skip over all of the equal values...
          // this loop should be 5 instructions
          rightequalpiv = piv;
          while ( rightequalpiv < right )
          {
            if ( is_before( tmp, rightequalpiv ) )
              break;
            rightequalpiv = rsadd_ptr_elements( rightequalpiv, 1 );
          }

          // ok, now get the size of each half and prepare to descend
          leftlen = rsdiff_ptr_elements( piv, left );
          length -= rsdiff_ptr_elements( rightequalpiv, left ); 

          // put the smaller segment on the stack
          if ( length < leftlen )
          {
            // put small right on stack
            stk_ptr->left = rightequalpiv;
            stk_ptr->len = length;
            stk_ptr += ( length > 1 );
            length = leftlen;
          } 
          else
          {
            // put small left on stack
            stk_ptr->left = left;
            stk_ptr->len = leftlen;
            stk_ptr += ( leftlen > 1 );
            left = rightequalpiv;
            if ( length <= 1 ) break;
          }
        }
      }
    }
    --stk_ptr;
    left = stk_ptr->left;
    length = stk_ptr->len;
  } while ( length );
}

#undef rsadd_ptr
#undef rsadd_ptr_elements
#undef rsdiff_ptr_elements
