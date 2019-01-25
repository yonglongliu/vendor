/*
 *Copyright 2014 NXP Semiconductors
 *
 *Licensed under the Apache License, Version 2.0 (the "License");
 *you may not use this file except in compliance with the License.
 *You may obtain a copy of the License at
 *            
 *http://www.apache.org/licenses/LICENSE-2.0
 *             
 *Unless required by applicable law or agreed to in writing, software
 *distributed under the License is distributed on an "AS IS" BASIS,
 *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *See the License for the specific language governing permissions and
 *limitations under the License.
 */

/************************************************************************
 *  Module:       tbase_func_TbTraceVPrintf.inc
 *  Description:
 *     implementation of tbase function                
 ************************************************************************/

/*
 NOTE: This file is to be included in *exactly one* C file of the conrete project.
 This way you will pull in the implementation of the function below.
*/


#if TB_TRACE_OUTPUT_AVAILABLE

#include <stdarg.h>


#define TBFLG_ZEROPAD 0x01    /* pad with zero */
#define TBFLG_SIGN    0x02    /* unsigned/signed long */
#define TBFLG_PLUS    0x04    /* show plus */
#define TBFLG_SPACE   0x08    /* space if plus */
#define TBFLG_LEFT    0x10    /* left justified */
#define TBFLG_SMALL		0x20		/* use lowercase in hex (must be 32 == 0x20) */
#define TBFLG_SPECIAL	0x40		/* prefix hex with "0x", octal with "0" */ 



static
int 
TbStringLen(
  const char* s, 
  int count
  )
{
  const char* sc;
  for (sc = s; count-- && *sc != '\0'; ++sc) ;
  return ( (int)(sc - s) );
}

static
int 
TbSkipAtoi(
  const char** s
  )
{
  int i = 0;
  while (TB_IS_DIGIT_CHAR(**s)) {
    i = i*10 + *((*s)++) - '0';
  }

  return i;
}

/* we are called with base 8, 10 or 16, only, thus don't need "G..."  */
static const char TbDigits[] = "0123456789ABCDEF"; /* "GHIJKLMNOPQRSTUVWXYZ"; */

static
void 
TbNumber(
  unsigned long num,		// 32 bit or 64 bit
  int size,
  int precision,
  char base, 
  unsigned char type
  ) 
{

  char tmp[25];
  char sign;
  char locase;
  int need_pfx = ((type & TBFLG_SPECIAL) && base != 10);
  int i;
  
   if ((base != 8) && (base != 10) && (base != 16)) return; 

	/* locase = 0 or 0x20. ORing digits or letters with 'locase'
	 * produces same digits or (maybe lowercased) letters */
	locase = (type & TBFLG_SMALL);

  if (type & TBFLG_LEFT) {
    type &= ~TBFLG_ZEROPAD;
  }
  
  sign = 0;
  if (type & TBFLG_SIGN) {
    if ((signed long)num < 0) {
      sign = '-';
      num = -(signed long)num;
      size--;
    } else if (type & TBFLG_PLUS) {
      sign = '+';
      size--;
    } else if (type & TBFLG_SPACE) {
      sign = ' ';
      size--;
    }
  }
  if (need_pfx) {
		size--;
    if (base == 16) {
      size--;
    }
  }
  /* generate full string in tmp[], in reverse order */
  i = 0;
  if (num == 0) {
    tmp[i++]='0';
  } else {
		if (base != 10) { /* 8 or 16 */
			int mask = base - 1;
			int shift = 3;
			if (base == 16) {
				shift = 4;
			}
			do {
				tmp[i++] = (TbDigits[((unsigned char)num) & mask] | locase);
				num >>= shift;
			} while (num);
		} else {
			/* base 10 */
			while (num != 0) {
				int res;
				res = num % (unsigned) base;
				num = num / (unsigned) base;
				tmp[i++] = (TbDigits[res] | locase);
			}
    }
  }
  /* printing 100 using %2d gives "100", not "00" */
  if (i > precision) {
    precision = i;
  }
  /* leading space padding */
  size -= precision;
  if (!(type&(TBFLG_ZEROPAD+TBFLG_LEFT))) {
    while(size-->0) {
			TbTraceOutputChar(' ');
		}
  }
  /* sign */
  if (sign) {
		TbTraceOutputChar(sign); 
	}
	/* "0x" / "0" prefix */
  if ((type & TBFLG_SPECIAL) && (base!=10)) {
		TbTraceOutputChar('0'); 
    if (base==16) {
			TbTraceOutputChar('X' | locase); 
    }
	}
	/* zero or space padding */
  if (!(type & TBFLG_LEFT)) {
		char c = (char)((type & TBFLG_ZEROPAD) ? '0' : ' ');
    while (size-- > 0) {
			TbTraceOutputChar(c);
    }
  }
  /* hmm even more zero padding? */
  while (i < precision--) {
		TbTraceOutputChar('0');
  }
  /* actual digits of result */
  while (i-- > 0) {
		TbTraceOutputChar(tmp[i]);
  }
  /* trailing space padding */
  while (size-- > 0) {
		TbTraceOutputChar(' ');
	}
	
} /* TbNumber */


#if (defined(TBASE_ARCH_64BIT) || defined(TB_TRACE_SUPPORTS_64BIT))
static
void 
TbNumber64(
  unsigned long long num,	// always 64 bit
  int size,
  int precision,
  char base, 
  unsigned char type
  ) 
{
  char tmp[50];
  char sign;
  char locase;
  int need_pfx = ((type & TBFLG_SPECIAL) && base != 10);
  int i;
  
  if ((base != 8) && (base != 10) && (base != 16)) return; 

	/* locase = 0 or 0x20. ORing digits or letters with 'locase'
	 * produces same digits or (maybe lowercased) letters */
	locase = (type & TBFLG_SMALL);

  if (type & TBFLG_LEFT) {
    type &= ~TBFLG_ZEROPAD;
  }
  
  sign = 0;
  if (type & TBFLG_SIGN) {
    if ((signed long long)num < 0) {
      sign = '-';
      num = -(signed long long)num;
      size--;
    } else if (type & TBFLG_PLUS) {
      sign = '+';
      size--;
    } else if (type & TBFLG_SPACE) {
      sign = ' ';
      size--;
    }
  }
  if (need_pfx) {
		size--;
    if (base == 16) {
      size--;
    }
  }
  /* generate full string in tmp[], in reverse order */
  i = 0;
  if (num == 0) {
    tmp[i++]='0';
  } else {
		if (base != 10) { /* 8 or 16 */
			int mask = base - 1;
			int shift = 3;
			if (base == 16) {
				shift = 4;
			}
			do {
				tmp[i++] = (TbDigits[((unsigned char)num) & mask] | locase);
				num >>= shift;
			} while (num);
		} else {
			/* base 10 */
			while (num != 0) {
				long long res;
				res = num % (unsigned) base;
				num = num / (unsigned) base;
				tmp[i++] = (TbDigits[res] | locase);
			}
    }
  }
  /* printing 100 using %2d gives "100", not "00" */
  if (i > precision) {
    precision = i;
  }
  /* leading space padding */
  size -= precision;
  if (!(type&(TBFLG_ZEROPAD+TBFLG_LEFT))) {
    while(size-->0) {
			TbTraceOutputChar(' ');
		}
  }
  /* sign */
  if (sign) {
		TbTraceOutputChar(sign); 
	}
	/* "0x" / "0" prefix */
  if ((type & TBFLG_SPECIAL) && (base!=10)) {
		TbTraceOutputChar('0'); 
    if (base==16) {
			TbTraceOutputChar('X' | locase); 
    }
	}
	/* zero or space padding */
  if (!(type & TBFLG_LEFT)) {
		char c = (char)((type & TBFLG_ZEROPAD) ? '0' : ' ');
    while (size-- > 0) {
			TbTraceOutputChar(c);
    }
  }
  /* hmm even more zero padding? */
  while (i < precision--) {
		TbTraceOutputChar('0');
  }
  /* actual digits of result */
  while (i-- > 0) {
		TbTraceOutputChar(tmp[i]);
  }
  /* trailing space padding */
  while (size-- > 0) {
		TbTraceOutputChar(' ');
	}
} /* TbNumber64 */

#endif  //#if (defined(TBASE_ARCH_64BIT) || defined(TB_TRACE_SUPPORTS_64BIT))


static
void 
TbTraceVPrintf(
  const char *fmt, 
  va_list args
  )
{

  /* note: long changes size on GNU compiler */
  unsigned long num;
  int len;
  int i;
  char base;
  const char *s;
  int field_width;  /* width of output field */
  int precision;    /* min. # of digits for integers;
                       max number of chars for string */
  char qualifier;   /* 'h', 'l', or 'L' for integer fields */
	char qualifier2;
  unsigned char flags;    /* flags to number() */




  for ( ; *fmt ; ++fmt) {
    if (*fmt != '%') {
      TbTraceOutputChar(*fmt); 
      continue;
    }
      
    /* process flags */
    flags = 0;
    repeat:
      ++fmt;    /* this also skips first '%' */
      switch (*fmt) {
        case '-': flags |= TBFLG_LEFT; goto repeat;
        case '+': flags |= TBFLG_PLUS; goto repeat;
        case ' ': flags |= TBFLG_SPACE; goto repeat;
        case '#': flags |= TBFLG_SPECIAL; goto repeat;
        case '0': flags |= TBFLG_ZEROPAD; goto repeat;
        default : break;
        }
    
    /* get field width */
    field_width = -1;
    if (TB_IS_DIGIT_CHAR(*fmt)) {
      field_width = TbSkipAtoi(&fmt);
    } else if (*fmt == '*') {
      ++fmt;
      /* it's the next argument */
      field_width = va_arg(args, int);
      if (field_width < 0) {
        field_width = -field_width;
        flags |= TBFLG_LEFT;
      }
    }

    /* get the precision */
    precision = -1;
    if (*fmt == '.') {
      ++fmt;  
      if (TB_IS_DIGIT_CHAR(*fmt)) {
        precision = TbSkipAtoi(&fmt);
      } else if (*fmt == '*') {
        ++fmt;
        /* it's the next argument */
        precision = va_arg(args, int);
      }
      if (precision < 0) {
        precision = 0;
      }
    }

    /* get the conversion qualifier */
    qualifier = 0;
    qualifier2 = 0;
    if (*fmt == 'z' || *fmt == 'L') {
      qualifier = *fmt;
      ++fmt;
    } else {
			if (*fmt == 'l') {
				qualifier = *fmt;
				++fmt;
				if (*fmt == 'l') {
					qualifier2 = *fmt;
					++fmt;
				}
			} else {
				if (*fmt == 'h') {
					qualifier = *fmt;
					++fmt;
					if (*fmt == 'h') {
						qualifier2 = *fmt;
						++fmt;
					}
				}
			}
		}

    /* default base */
    base = 10;

    switch (*fmt) {

    case 'c':
      if (!(flags & TBFLG_LEFT)) {
        while (--field_width > 0) {
					TbTraceOutputChar(' ');
        }
      }
      TbTraceOutputChar( (char)va_arg(args, int) ); 
      while (--field_width > 0) {
				TbTraceOutputChar(' ');
      }
      continue;

    case 's':
      /* va_arg(args, const char*): const needed for some compiler if a far pointer is the argument*/
      s = va_arg(args, const char*);
      if (!s) {
        s = "<NULL>";
      }

      len = TbStringLen(s, precision);

      if (!(flags & TBFLG_LEFT)) {
        while (len < field_width--) {
					TbTraceOutputChar(' ');
        }
      }
      for (i = 0; i < len; ++i) {
				TbTraceOutputChar(*s++);
      }
      while (len < field_width--) {
				TbTraceOutputChar(' ');
      }
      continue;

    case 'p':
      if (field_width == -1) {
        field_width = 2*sizeof(void*);
        flags |= TBFLG_ZEROPAD;
      }
      /* va_arg(args, const void *): const needed for some compiler if a far pointer is the argument */
#if (defined(TBASE_ARCH_64BIT) || defined(TB_TRACE_SUPPORTS_64BIT))
			TbNumber64((unsigned long long) va_arg(args, const void*), field_width, precision, 16, flags); 
#else
			TbNumber((unsigned long) va_arg(args, const void*), field_width, precision, 16, flags); 
#endif
      continue;

    /* integer number formats - set up the flags and "break" */
    case 'o':
      base = 8;
      break;

    case 'x':
      flags |= TBFLG_SMALL;
    case 'X':
      base = 16;
      break;

    case 'd':
    case 'i':
      flags |= TBFLG_SIGN;
    case 'u':
      break;

    default:
      if (*fmt != '%') {
				TbTraceOutputChar('%'); 
      }
      if (*fmt) {
				TbTraceOutputChar(*fmt);
      } else {
        --fmt;
      }
      continue;
    }
    if (qualifier == 'l') {
#if (defined(TBASE_ARCH_64BIT) || defined(TB_TRACE_SUPPORTS_64BIT))
			if (qualifier2 == 'l') {
				{
					long long num64;
					if (flags & TBFLG_SIGN) {
						num64 = va_arg(args, long long);
					} else {
						num64 = va_arg(args, unsigned long long);
					}
					TbNumber64(num64, field_width, precision, base, flags);
					continue;
				}
			} 
			else 
#endif
			{
				if (flags & TBFLG_SIGN) {
					num = va_arg(args, long);
				} else {
					num = va_arg(args, unsigned long);
				}
      }
    } else if (qualifier == 'h') {
			if (qualifier2 == 'h') {
				if (flags & TBFLG_SIGN) {
					num = va_arg(args, int);
				} else {
					num = va_arg(args, unsigned int);
				}
			} else {
				if (flags & TBFLG_SIGN) {
					num = va_arg(args, int);
				} else {
					num = va_arg(args, unsigned int);
				}
      }
    } else if (qualifier == 'z') {
			num = (unsigned long)va_arg(args, size_t); /* ### not working on 64bit Microsoft compiler */
    } else if (flags & TBFLG_SIGN) {
      num = va_arg(args, int);
    } else {
      num = va_arg(args, unsigned int);
    }
    TbNumber(num, field_width, precision, base, flags);
  }
} /* TbTraceVPrintf */

#endif  /* TB_TRACE_OUTPUT_AVAILABLE */

/*************************** EOF **************************************/
