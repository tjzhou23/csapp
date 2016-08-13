/*
 * CS:APP Data Lab
 *
 * <Please put your name and userid here>
 * Longqi Cai
 * longqic
 *
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:

  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code
  must conform to the following style:

  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>

  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.


  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size. (e.g. 2 << 32 can get unexpected result)

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function.
     The max operator count is checked by dlc. Note that '=' is not
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 *
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce
 *      the correct answers.
 */


#endif


/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {
    return (~(~x&~y))&(~(x&y));  
} 


/*  
 * bitAnd - x&y using only ~ and |  
 *   Example: bitAnd(6, 5) = 4 
 *   Legal ops: ~ | 
 *   Max ops: 8 
 *   Rating: 1 
 */  
int bitAnd(int x, int y) {  
    return ~((~x)|(~y));  
}  


/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
  return 1<<31;
}



/*
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
  // build mask: 0xAAAAAAAA
  int mask = (0xAA << 8) + 0xAA;
  mask = (mask << 16) + mask;
  
  return !((x & mask) ^ mask);
}


/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
  return ~x+1;
}



/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 2
 */
int isTmax(int x) {
  int indicator = x + x + 2;
  return !(indicator | !~x);
}


/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x) {
  int x_comp = ~x;
  int d1 = 0x30 + x_comp;
  int d2 = 0x3a + x_comp;
  return !(~(d1 >> 31) | (d2 >> 31));
}

/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */

/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
  int d = y + (~x + 1);
  int sign_d = d >> 31;
  int sign_x = x >> 31;
  int sign_y = y >> 31;
  int is_sign_diff = sign_x ^ sign_y;
  return !((is_sign_diff | sign_d) & (~sign_x | sign_y));
}


/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
  int x_neg = ~x + 1;
  return (x | x_neg) >> 31 & 0x1 ^ 0x1;
}


/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
    int temp=x^(x>>31);//get positive of x;
    int isZero=!temp;
    //notZeroMask is 0xffffffff
    int notZeroMask=(!(!temp)<<31)>>31;
    int bit_16,bit_8,bit_4,bit_2,bit_1;
    bit_16=!(!(temp>>16))<<4;
    //see if the high 16bits have value,if have,then we need at least 16 bits
    //if the highest 16 bits have value,then rightshift 16 to see the exact place of  
    //if not means they are all zero,right shift nothing and we should only consider the low 16 bits
    temp=temp>>bit_16;
    bit_8=!(!(temp>>8))<<3;
    temp=temp>>bit_8;
    bit_4=!(!(temp>>4))<<2;
    temp=temp>>bit_4;
    bit_2=!(!(temp>>2))<<1;
    temp=temp>>bit_2;
    bit_1=!(!(temp>>1));
    temp=bit_16+bit_8+bit_4+bit_2+bit_1+2;//at least we need one bit for 1 to tmax,
    //and we need another bit for sign
    return isZero|(temp&notZeroMask);
}


/* 
 * float_twice - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_twice(unsigned uf) {
  /*
   * Case 1: uf and uf*2 are both normalized.
   *   i.e. exp != 00...0
   *            != 11...1
   *            != 11...0
   *   In this case, return uf + (1 << 23)
   *
   * Case 2: exp == 11...1
   *   return uf
   *
   * Case 3: exp == 1...10
   *   return inf
   *
   * Case 4: exp = 00...0
   *   frac_r = frac << 1
   *   4.1: if frac has a leading 1,
   *     exp_r = 0...01
   *   4.2: otherwise,
   *     exp_r = exp
   *   * Trick: to save op, just left-shift frac,
   *            the overflow 1 can add to exp.
   */
  // sign with offset
  unsigned sign_wo = (uf >> 31) << 31;
  // uf getting rid of sign
  unsigned uf_u = uf - sign_wo;
  unsigned exp = uf_u >> 23;

  // Case 2
  // (1 << 8) - 1 = 0xff;
  unsigned exp_sp = 0xff;  // exp for special values
  unsigned d_exp = exp_sp - exp;
  if (!d_exp) {
    return uf;
  }

  // Case 3
  if (!(d_exp - 1)) {
    return sign_wo | (exp_sp << 23);
  }

  // Case 1
  if (exp) {
    // 1 << 23 = 0x00800000
    return uf + 0x00800000;
  }

  // Case 4
  return sign_wo | uf << 1;
}


/* 
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_i2f(int x) {

  const int tmin = 0x80000000;
  int sign_wo = (x >> 31) << 31;  // sign with offset
  int x_abs = sign_wo ? -x : x;
  int exp;  // biased exp, appears in float
  int counter;
  int frac;
  int frac_wo;  // frac with offset

  if (x == tmin) {
    exp = 0x9e;
    frac = 0;
  } else if (!x) {
    return 0;
  } else {
    int frac_wol;  // frac with offset and leading 1
    frac_wo = x_abs;
    counter = 0;
    while (!(frac_wo & tmin)) {
      frac_wo = frac_wo << 1;
      counter++;
    }
    frac_wol = frac_wo;  // reserve the highest bit
    frac_wo -= tmin;  // subtract the highest bit
    // To save ops, write 0x7f + 31 as 0x9e
    exp = 0x9e - counter;

    if (~frac_wol <= 0x7f) {
      exp++;
      frac = 0;
    } else {  // round to even
      int remainder = frac_wo & 0xff;
      frac = frac_wo >> 8;
      if (remainder > 0x80 || (remainder == 0x80 && (frac & 0x1))) {
        frac += 1;
      }
    }
  }
  return sign_wo | (exp << 23) | frac;
}



/* 
 * float_f2i - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int float_f2i(unsigned uf) {
	  /*
	   * Case 1: Too small, i.e. exp - 127 = e < 0
	   *   return 0
	   *
	   * Case 2: Out of range, i.e. exp - 128 = e > 30
	   *   return 0x80000000u
	   *
	   * Case 3: Normalized.
	   *   Just shift. Be careful of rounding.
	   */
	  const int OUT_OF_RANGE = 0x80000000u;
	  int sign_ew; 

	  int exp;
	  int frac;
	  int num_trunc;  // number of bits to be truncated
	  int r;

	  sign_ew = uf >> 31;
	  exp = (uf >> 23) & 0xff;

	  frac = uf & 0x007fffff;

	  if (exp < 0x7f) {
	    return 0;
	  }

	  if (exp > 157) {
	    return OUT_OF_RANGE;
	  }

	  r = 0x40000000 + (frac << 7);
	  num_trunc = 157 - exp;
	  r = r >> num_trunc;
	  
	  return sign_ew ? ~r + 1 : r;
}





