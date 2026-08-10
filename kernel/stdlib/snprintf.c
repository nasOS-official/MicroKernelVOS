// Copyright (C) 2019 Miroslaw Toton, mirtoto@gmail.com

/**
 * Unix snprintf() implementation.
 * @version 2.3
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * Revision History:
 * 
 * @version 2.3
 * @author Miroslaw Toton (mirtoto), mirtoto@gmail.com
 *  - support NULL as output buffer to calculate size of output string
 *  - fix 0 precision for 0 value integers
 * 
 * @version 2.2
 * @author Miroslaw Toton (mirtoto), mirtoto@gmail.com
 *  - fix precision for integers
 *  - remove global buffers and unnecessary copying & lookin through srings
 *  - cleanup code and add Doxygen style comments
 * 
 * @version 2.1
 * @author Miroslaw Toton (mirtoto), mirtoto@gmail.com
 *  - fix problem with very big and very low "long long" values
 *  - change exponent width from 3 to 2
 *  - fix zero value for floating
 *  - support for "p" (%p)
 *
 * @version 2.0
 * @author Miroslaw Toton (mirtoto), mirtoto@gmail.com
 *  - move all defines & macros from header to codefile
 *  - support for "long long" (%llu, %lld, %llo, %llx)
 *  - fix for interpreting precision in some situations
 *  - fix unsigned (%u) for negative input
 *  - fix h & hh length of input number specifier
 *  - fix Clang linter warnings
 *
 * @version 1.1
 * @author Alain Magloire, alainm@rcsm.ee.mcgill.ca
 *  - added changes from Miles Bader
 *  - corrected a bug with %f
 *  - added support for %#g
 *  - added more comments :-)
 *
 * @version 1.0
 * @author Alain Magloire, alainm@rcsm.ee.mcgill.ca
 *  - supporting must ANSI syntaxic_sugars (see below)
 *
 * @version 0.0
 * @author Alain Magloire, alainm@rcsm.ee.mcgill.ca
 *  - suppot %s %c %d
 *
 * For the floating point format the challenge was finding a way to
 * manipulate the Real numbers without having to resort to mathematical
 * function (it would require to link with -lm) and not going down
 * to the bit pattern (not portable).
 *
 * So a number, a real is:
 *
 *    real = integral + fraction
 *
 *    integral = ... + a(2)*10^2 + a(1)*10^1 + a(0)*10^0
 *    fraction = b(1)*10^-1 + b(2)*10^-2 + ...
 *
 *    where:
 *      0 <= a(i) => 9
 *      0 <= b(i) => 9
 *
 *   from then it was simple math
 *
 * THANKS (for the patches and ideas):
 *  - Miles Bader
 *  - Cyrille Rustom
 *  - Jacek Slabocewiz
 *  - Mike Parker (mouse)
 */

#include <ctype.h>
#include <string.h>

#include <snprintf.h>


#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextra-semi-stmt"
#endif


/** 
 * This struct holds temporary data during processing @p format 
 * of vsnprintf()/snprintf() functions.
 */
struct DATA {
  size_t counter;             /**< counter of length of string in DATA::ps */
  size_t ps_size;             /**< size of DATA::ps - 1 */
  char *ps;                   /**< pointer to output string */
  const char *pf;             /**< pointer to input format string */

/** Value of DATA::width - undefined width of field. */
#define WIDTH_UNSET          -1

  int width;                  /**< width of field */

/** Value of DATA::precision - undefined precision of field. */
#define PRECISION_UNSET      -1

  int precision;

/** Value of DATA::align - undefined align of field. */
#define ALIGN_UNSET           0
/** Value of DATA::align - align right of field. */
#define ALIGN_RIGHT           1
/** Value of DATA::align - align left of field. */
#define ALIGN_LEFT            2

  unsigned int align:2;     /**< align of field */
  unsigned int is_square:1; /**< is field with hash flag? */
  unsigned int is_space:1;  /**< is field with space flag? */
  unsigned int is_dot:1;    /**< is field with dot flag? */
  unsigned int is_star_w:1; /**< is field with width defined? */
  unsigned int is_star_p:1; /**< is field with precision defined? */

/** Value of DATA::a_long - "int" type of input argument. */
#define INT_LEN_DEFAULT       0
/** Value of DATA::a_long - "long" type of input argument. */
#define INT_LEN_LONG          1
/** Value of DATA::a_long - "long long" of input type argument. */
#define INT_LEN_LONG_LONG     2
/** Value of DATA::a_long - "short" type of input argument. */
#define INT_LEN_SHORT         3
/** Value of DATA::a_long - "char" type of input argument. */
#define INT_LEN_CHAR          4

  unsigned int a_long:3;    /**< type of input */

  unsigned int rfu:6;       /**< RFU */

  char pad;                 /**< padding character */

  char slop[5];             /**< RFU */
};



/** Put a @p c character to output buffer if there is enough space. */
#define PUT_CHAR(c, p)                                  \
  if ((p)->counter < (p)->ps_size) {                    \
    if ((p)->ps != NULL) {                              \
      *(p)->ps++ = (c);                                 \
    }                                                   \
    (p)->counter++;                                     \
  }

/** Put optionally '+' character to to output buffer if there is enough space. */
#define PUT_PLUS(d, p)                                  \
  if ((d) > 0 && (p)->align == ALIGN_RIGHT) {           \
    PUT_CHAR('+', p);                                   \
  }

/** Put optionally ' ' character to to output buffer if there is enough space. */
#define PUT_SPACE(d, p)                                 \
  if ((p)->is_space && (d) > 0) {                       \
    PUT_CHAR(' ', p);                                   \
  }

/** Padding right optionally. */
#define PAD_RIGHT(p)                                    \
  if ((p)->width > 0 && (p)->align != ALIGN_LEFT) {     \
    for (; (p)->width > 0; (p)->width--) {              \
      PUT_CHAR((p)->pad, p);                            \
    }                                                   \
  }

/** Padding left optionally. */
#define PAD_LEFT(p)                                     \
  if ((p)->width > 0 && (p)->align == ALIGN_LEFT) {     \
    for (; (p)->width > 0; (p)->width--) {              \
      PUT_CHAR((p)->pad, p);                            \
    }                                                   \
  }

/** Get width and precision arguments if available. */
#define WIDTH_AND_PRECISION_ARGS(p)                     \
  if ((p)->is_star_w) {                                 \
    (p)->width = va_arg(args, int);                     \
  }                                                     \
  if ((p)->is_star_p) {                                 \
    (p)->precision = va_arg(args, int);                 \
  }

/** Get integer argument of given type and convert it to long long. */
#define INTEGER_ARG(p, type, ll)                        \
  WIDTH_AND_PRECISION_ARGS(p);                          \
  if ((p)->a_long == INT_LEN_LONG_LONG) {               \
    ll = (long long)va_arg(args, type long long);       \
  } else if ((p)->a_long == INT_LEN_LONG) {             \
    ll = (long long)va_arg(args, type long);            \
  } else {                                              \
    type int a = va_arg(args, type int);                \
    if ((p)->a_long == INT_LEN_SHORT) {                 \
      ll = (type short)a;                               \
    } else if ((p)->a_long == INT_LEN_CHAR) {           \
      ll = (type char)a;                                \
    } else {                                            \
      ll = a;                                           \
    }                                                   \
  }

/** Get double argument. */
#define DOUBLE_ARG(p, d)                                \
  WIDTH_AND_PRECISION_ARGS(p);                          \
  if ((p)->precision == PRECISION_UNSET) {              \
    (p)->precision = 6;                                 \
  }                                                     \
  d = va_arg(args, double);

/**
 * Convert maximum @p n characters of @p a string to integer.
 * 
 * Function stop conversion and return result in any of the following cases:
 *  - encounter end of string ('\0') character,
 *  - encounter non digit character,
 *  - reach @p n characters.
 * 
 * @return Integer (as type 'int') representation of @p a.
 *
static int antoi(const char *a, size_t n) {
  size_t i = 0;
  int res = 0;

  for (; a[i] != '\0' && i < n && isdigit(a[i]); i++) {
    res = res * 10 + (a[i] - '0');
  }

  return res;
}
 */

/**
 * Convert @p a string to @p res integer.
 * 
 * Function stop conversion and return result in any of the following cases:
 *  - encounter end of string ('\0') character,
 *  - encounter non digit character.
 * 
 * @param a Not NULL input string.
 * @param res Not NULL pointer to output integer.
 * 
 * @return Number of parsed character form @p a.
 */
static int strtoi(const char *a, int *res) {
  int i = 0;

  *res = 0;

  for (; a[i] != '\0' && isdigit(a[i]); i++) {
    *res = *res * 10 + (a[i] - '0');
  }

  return i;
}

/**
 * Convert @p number to string representation of given @p base.
 *
 * @param number Input number to conversion.
 * @param is_signed Interpret @p number as 'unsigned' (0) / 'signed' (1).
 * @param precision Input @p number precision.
 * @param base Output base (8, 10, 16).
 * @param output Buffer for output string.
 * @param output_size Size of @p optput buffer (at least 3 characters).
 */
static void inttoa(long long number, int is_signed, int precision, int base,
    char *output, size_t output_size) {    
  size_t i = 0, j;

  output_size--; /* for '\0' character */

  if (number != 0) {
    unsigned long long n;

    if (is_signed && number < 0) {
      n = (unsigned long long)-number;
      output_size--; /* for '-' character */
    } else {
      n = (unsigned long long)number;
    }

    while (n != 0 && i < output_size) {
      int r = (int)(n % (unsigned long long)(base));
      output[i++] = (char)r + (r < 10 ? '0' : 'a' - 10);
      n /= (unsigned long long)(base);
    }

    if (precision > 0) { /* precision defined ? */
      for (; i < (size_t)precision && i < output_size; i++) {
        output[i] = '0';
      }
    }

    /* put the sign ? */
    if (is_signed && number < 0) {
      output[i++] = '-';
    }

    output[i] = '\0';
    
    /* reverse every thing */
    for (i--, j = 0; j < i; j++, i--) {
      char tmp = output[i];
      output[i] = output[j];
      output[j] = tmp;
    }
  } else {
    precision = precision < 0 ? 1 : precision;
    for (i = 0; i < (size_t)precision && i < output_size; i++) {
      output[i] = '0';
    }
    output[i] = '\0';
  }
}


/** Maximum size of the buffer for the integral part. */
#define MAX_INTEGRAL_SIZE (99 + 1)
/** Maximum size of the buffer for the fraction part. */
#define MAX_FRACTION_SIZE (29 + 1)
/** Precision. */
#define PRECISION (1.e-6)


/** Format @p ll number as ASCII decimal string according to @p p flags. */
static void decimal(struct DATA *p, long long ll) {
  char number[MAX_INTEGRAL_SIZE], *pnumber = number;
  inttoa(ll, *p->pf == 'i' || *p->pf == 'd', p->precision, 10,
    number, sizeof(number));

  p->width -= strlen(number);
  PAD_RIGHT(p);

  PUT_PLUS(ll, p);
  PUT_SPACE(ll, p);

  for (; *pnumber != '\0'; pnumber++) {
    PUT_CHAR(*pnumber, p);
  }

  PAD_LEFT(p);
}

/** Format @p ll number as ASCII octal string according to @p p flags. */
static void octal(struct DATA *p, long long ll) {
  char number[MAX_INTEGRAL_SIZE], *pnumber = number;
  inttoa(ll, 0, p->precision, 8, number, sizeof(number));

  p->width -= strlen(number);
  PAD_RIGHT(p);

  if (p->is_square && *number != '\0') { /* prefix '0' for octal */
    PUT_CHAR('0', p);
  }

  for (; *pnumber != '\0'; pnumber++) {
    PUT_CHAR(*pnumber, p);
  }

  PAD_LEFT(p);
}

/** Format @p ll number as ASCII hexadecimal string according to @p p flags. */
static void hex(struct DATA *p, long long ll) {
  char number[MAX_INTEGRAL_SIZE], *pnumber = number;
  inttoa(ll, 0, p->precision, 16, number, sizeof(number));

  p->width -= strlen(number);
  PAD_RIGHT(p);

  if (p->is_square && *number != '\0') { /* prefix '0x' for hex */
    PUT_CHAR('0', p);
    PUT_CHAR(*p->pf == 'p' ? 'x' : *p->pf, p);
  }

  for (; *pnumber != '\0'; pnumber++) {
    PUT_CHAR((*p->pf == 'X' ? (char)toupper(*pnumber) : *pnumber), p);
  }

  PAD_LEFT(p);
}

/** Format @p str string according to @p p flags. */
static void strings(struct DATA *p, char *s) {
  int len = (int)strlen(s);
  if (p->precision != PRECISION_UNSET && len > p->precision) { /* the smallest number */
    len = p->precision;
  }

  p->width -= len;

  PAD_RIGHT(p);

  for (; len-- > 0; s++) {
    PUT_CHAR(*s, p);
  }

  PAD_LEFT(p);
}

/** Initialize and parse the conversion specifiers. */
static void conv_flags(struct DATA *p) {
  p->width = WIDTH_UNSET;
  p->precision = PRECISION_UNSET;
  p->is_star_w = p->is_star_p = 0;
  p->is_square = p->is_space = 0;
  p->a_long = INT_LEN_DEFAULT;
  p->align = ALIGN_UNSET;
  p->pad = ' ';
  p->is_dot = 0;

  for (; p != NULL && p->pf != NULL; p->pf++) {
    switch (*p->pf) {
      case ' ':
        p->is_space = 1;
        break;

      case '#':
        p->is_square = 1;
        break;

      case '*':
        if (p->width == WIDTH_UNSET) {
          p->width = 1;
          p->is_star_w = 1;
        } else {
          p->precision = 1;
          p->is_star_p = 1;
        }
        break;

      case '+':
        p->align = ALIGN_RIGHT;
        break;

      case '-':
        p->align = ALIGN_LEFT;
        break;

      case '.':
        if (p->width == WIDTH_UNSET) {
          p->width = 0;
        }
        p->is_dot = 1;
        break;

      case '0':
        p->pad = '0';
        if (p->is_dot) {
          p->precision = 0;
        }
        break;

      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9': /* get all the digits */
        p->pf += strtoi(p->pf,
          p->width == WIDTH_UNSET ? &p->width : &p->precision) - 1;
        break;

      case '%':
        return;

      default:
        p->pf--; /* went to far go back */
        return;
    }
  }
}

int vsnprintf(char *string, size_t length, const char *format, va_list args) {
  struct DATA data;

  /* calculate only size of output string */
  if (string == NULL) {
    length = __SIZE_MAX__;
  /* sanity check, the string must be > 1 */
  } else if (length < 1) {
    return -1;
  }

  data.ps_size = length - 1; /* leave room for '\0' */
  data.ps = string;
  data.pf = format;
  data.counter = 0;

  for (; *data.pf != '\0' && (data.counter < data.ps_size); data.pf++) {
    if (*data.pf == '%') { /* we got a magic % cookie */
      int is_continue = 1;
      conv_flags(&data); /* initialise format flags */
      while (*data.pf != '\0' && is_continue) {
        switch (*(++data.pf)) {
          case '\0': /* a NULL here ? ? bail out */
            PUT_CHAR('%', &data);
            if (data.ps != NULL) {
              *data.ps = '\0';
            }
            return (int)data.counter;

 
          case 'u': { /* unsigned decimal integer */
            long long ll;
            INTEGER_ARG(&data, unsigned, ll);
            decimal(&data, ll);
            is_continue = 0;
            break;
          }

          case 'i':
          case 'd': { /* signed decimal integer */
            long long ll;
            INTEGER_ARG(&data, signed, ll);
            decimal(&data, ll);
            is_continue = 0;
            break;
          }

          case 'o': { /* octal (always unsigned) */
            long long ll;
            INTEGER_ARG(&data, unsigned, ll);
            octal(&data, ll);
            is_continue = 0;
            break;
          }

          case 'x':
          case 'X': { /* hexadecimal (always unsigned) */
            long long ll;
            INTEGER_ARG(&data, unsigned, ll);
            hex(&data, ll);
            is_continue = 0;
            break;
          }

          case 'c': { /* single character */
            int i = va_arg(args, int);
            PUT_CHAR((char)i, &data);
            is_continue = 0;
            break;
          }

          case 's': /* string of characters */
            WIDTH_AND_PRECISION_ARGS(&data);
            strings(&data, va_arg(args, char *));
            is_continue = 0;
            break;

          case 'p': { /* pointer */
            void *v = va_arg(args, void *);
            data.is_square = 1;
            if (v == NULL) {
              strings(&data, "(nil)");
            } else {
              hex(&data, (long long)v);
            }
            is_continue = 0;
            break;
          }

          case 'n': /* what's the count ? */
            *(va_arg(args, int *)) = (int)data.counter;
            is_continue = 0;
            break;

          case 'l': /* long or long long */
            if (data.a_long == INT_LEN_LONG) {
              data.a_long = INT_LEN_LONG_LONG;
            } else {
              data.a_long = INT_LEN_LONG;
            }
            break;

          case 'h': /* short or char */
            if (data.a_long == INT_LEN_SHORT) {
              data.a_long = INT_LEN_CHAR;
            } else {
              data.a_long = INT_LEN_SHORT;
            }
            break;

          case '%': /* nothing just % */
            PUT_CHAR('%', &data);
            is_continue = 0;
            break;

          case '#':
          case ' ':
          case '+':
          case '*':
          case '-':
          case '.':
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            conv_flags(&data);
            break;

          default:
            /* is this an error ? maybe bail out */
            PUT_CHAR('%', &data);
            is_continue = 0;
            break;
        } /* end switch */
      } /* end of while */
    } else { /* not % */
      PUT_CHAR(*data.pf, &data); /* add the char the string */
    }
  }

  if (data.ps != NULL) {
    *data.ps = '\0'; /* the end ye ! */
  }

  return (int)data.counter;
}

int snprintf(char *string, size_t length, const char *format, ...) {
  int rval;
  va_list args;

  va_start(args, format);
  rval = vsnprintf(string, length, format, args);
  va_end(args);

  return rval;
}


#ifdef __clang__
#pragma clang diagnostic pop
#endif
