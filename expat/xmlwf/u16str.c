/* Copyright (c) 2017 The Expat Maintainers
   See the file COPYING for copying permission.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "expat.h"
#include "u16str.h"
#include "internal.h" /* for UNUSED_P */

#define BAD_CODEPOINT ((unsigned int)(-1))

#if defined(XML_UNICODE) && !defined (XML_UNICODE_WCHAR_T)
static int surrogate_pair_started = 0;
static XML_Char surrogate_high;

static unsigned int
utf16_to_codepoint(const XML_Char hi, const XML_Char lo)
{
    if (hi < 0xD800)
        return hi;
    if (hi < 0xDC00) {
        /* The first of a surrogate pair.  Check the second is OK */
        if (lo < 0xDC00 || lo > 0xDFFF)
            return BAD_CODEPOINT;
        return 0x10000 + ((hi - 0xD800) << 10) + (lo - 0xDC00);
    }
    if (hi < 0xE000)
        return BAD_CODEPOINT;
    return hi;
}

static int
utf16_to_utf8(const XML_Char *s, char *buf)
{
    unsigned int codepoint;

    codepoint = utf16_to_codepoint(s[0], s[1]);
    if (codepoint == BAD_CODEPOINT) {
        /* We will substitute U+FFFD REPLACEMENT CHARACTER, which is
         * three bytes in UTF-8.
         */
        buf[0] = '\xEF';
        buf[1] = '\xBF';
        buf[2] = '\xBD';
        buf[3] = '\0';
        return 1;
    }
    if (codepoint >= 0x10000) {
        /* A surrogate pair, will be four bytes in UTF-8 */
        buf[0] = ((codepoint >> 18) & 0x07) | 0xF0;
        buf[1] = ((codepoint >> 12) & 0x3F) | 0x80;
        buf[2] = ((codepoint >> 6) & 0x3F) | 0x80;
        buf[3] = (codepoint & 0x3F) | 0x80;
        buf[4] = '\0';
        return 2;
    }
    if (codepoint >= 0x0800) {
        buf[0] = ((codepoint >> 12) & 0x0F) | 0xE0;
        buf[1] = ((codepoint >> 6) & 0x3F) | 0x80;
        buf[2] = (codepoint & 0x3F) | 0x80;
        buf[3] = '\0';
    }
    else if (codepoint >= 0x80) {
        buf[0] = ((codepoint >> 6) & 0x1F) | 0xC0;
        buf[1] = (codepoint & 0x3F) | 0x80;
        buf[2] = '\0';
    }
    else {
        buf[0] = codepoint;
        buf[1] = '\0';
    }
    return 1;
}

static int
utf16_len_as_utf8(const XML_Char *s)
{
    int len = 0;
    int incr;
    char buffer[5];

    while (*s) {
        incr = utf16_to_utf8(s, buffer);
        len += strlen(buffer);
        s += incr;
    }
    return len;
}

static char *
utf16_dup_as_utf8(const XML_Char *s)
{
    int incr;
    char buffer[5];
    char *result = malloc(utf16_len_as_utf8(s) + 1);
    char *d = result;

    if (result == NULL)
        return NULL;
    while (*s) {
        incr = utf16_to_utf8(s, buffer);
        strcpy(d, buffer);
        d += strlen(buffer);
        s += incr;
    }
    *d = '\0';
    return result;
}
#endif /* XML_UNICODE && !XML_UNICODE_WCHAR_T */


#if defined(XML_UNICODE) && !defined (XML_UNICODE_WCHAR_T)
const XML_Char *
tstring_create(TString **phead, const char *s)
{
    int i;
    int len = strlen(s) + 1;
    TString *new = (TString *)malloc(sizeof(TString) +
                                     len * sizeof(XML_Char));

    if (new == NULL) {
        fprintf(stderr, "Unable to create string constant\n");
        exit(1);
    }

    for (i = 0; i < len; i++)
        new->str[i] = (XML_Char)s[i];
    new->next = *phead;
    *phead = new;
    return new->str;
}
#else
const XML_Char *
tstring_create(TString **UNUSED_P(phead), const char *UNUSED_P(s))
{
    return NULL;
}
#endif

#if defined(XML_UNICODE) && !defined (XML_UNICODE_WCHAR_T)
void
tstring_dispose(TString *head)
{
    while (head != NULL) {
        TString *next = head->next;
        free(head);
        head = next;
    }
}
#else
void
tstring_dispose(TString *UNUSED_P(head))
{
}
#endif

/* This is a very limited reimplementation of fprintf */
#if defined(XML_UNICODE) && !defined (XML_UNICODE_WCHAR_T)
void
tstring_fprintf(FILE *fp, const XML_Char *format, ...)
{
    char buffer[5];
    va_list args;
    va_start(args, format);

    while (*format) {
        if (*format == (XML_Char)'%') {
            int is_long = 0;
#ifdef XML_LARGE_SIZE
            int is_very_long = 0;
#endif

            if (*++format == (XML_Char)'l') {
                is_long = 1;
                format++;
#ifdef XML_LARGE_SIZE
#if !defined(XML_USE_MSC_EXTENSIONS) || _MSC_VER >= 1400
                if (*format == (XML_Char)'l') {
                    is_very_long = 1;
                    format++;
                }
#endif
#endif
            }
#ifdef XML_LARGE_SIZE
#if defined(XML_USE_MSC_EXTENSIONS) && _MSC_VER >= 1400
            if (format[0] == (XML_Char)'I' &&
                format[1] == (XML_Char)'6' &&
                format[2] == (XML_Char)'4') {
                is_very_long = 1;
                format += 3;
            }
#endif
#endif
            switch (*format) {
                case (XML_Char)'%':
                    fputc('%', fp);
                    break;

                case (XML_Char)'d':
#ifdef XML_LARGE_SIZE
                    if (is_very_long) {
                        long long d = va_arg(args, long long);
                        fprintf(fp, "%" XML_FMT_INT_MOD "d", d);
                    } else
#endif /* XML_LARGE_SIZE */
                    if (is_long) {
                        long d = va_arg(args, long);
                        fprintf(fp, "%ld", d);
                    }
                    else {
                        int d = va_arg(args, int);
                        fprintf(fp, "%d", d);
                    }
                    break;

                case (XML_Char)'u':
#ifdef XML_LARGE_SIZE
                    if (is_very_long) {
                        unsigned long long d = va_arg(args, unsigned long long);
                        fprintf(fp, "%" XML_FMT_INT_MOD "d", d);
                    } else
#endif /* XML_LARGE_SIZE */
                    if (is_long) {
                        unsigned long u = va_arg(args, unsigned long);
                        fprintf(fp, "%lu", u);
                    }
                    else {
                        unsigned u = va_arg(args, unsigned);
                        fprintf(fp, "%u", u);
                    }
                    break;

                case (XML_Char)'s': {
                    XML_Char *s = va_arg(args, XML_Char *);
                    int incr;

                    while (*s) {
                        incr = utf16_to_utf8(s, buffer);
                        fputs(buffer, fp);
                        s += incr;
                    }
                    break;
                }

                default:
                    utf16_to_utf8(format, buffer);
                    fprintf(fp, "Unknown format character '%s'\n", buffer);
            }
            format++;
        }
        else {
            int incr = utf16_to_utf8(format, buffer);
            fputs(buffer, fp);
            format += incr;
        }
    }

    va_end(args);
}
#else /* defined(XML_UNICODE) && !defined (XML_UNICODE_WCHAR_T) */
void
tstring_fprintf(FILE *UNUSED_P(fp), const XML_Char *UNUSED_P(format), ...)
{
}
#endif /* defined(XML_UNICODE) && !defined (XML_UNICODE_WCHAR_T) */

#if defined(XML_UNICODE) && !defined (XML_UNICODE_WCHAR_T)
void
tstring_fputs(const XML_Char *s, FILE *fp)
{
    char buffer[5];
    int incr;

    while (*s) {
        incr = utf16_to_utf8(s, buffer);
        fputs(buffer, fp);
        s += incr;
    }
}
#else
void
tstring_fputs(const XML_Char *UNUSED_P(s), FILE *UNUSED_P(fp))
{
}
#endif

#if defined(XML_UNICODE) && !defined (XML_UNICODE_WCHAR_T)
void
tstring_putc(XML_Char ch, FILE *fp)
{
    XML_Char input[2];
    char buffer[5];

    if (surrogate_pair_started) {
        surrogate_pair_started = 0;
        if (ch >= 0xdc00 && ch <= 0xdfff) {
            /* Second of a surrogate pair */
            input[0] = surrogate_high;
            input[1] = ch;
            utf16_to_utf8(input, buffer);
            fputs(buffer, fp);
            return;
        }
        /* Else this is not a surrogate pair, and the previous
         * 16-bit character is in error somehow.
         */
        input[0] = surrogate_high;
        input[1] = 0;
        utf16_to_utf8(input, buffer);
        fputs(buffer, fp);
        /* DO NOT return here, we must process the new character */
    }

    if (ch >= 0xd800 && ch <= 0xdbff) {
        /* This is (probably) the start of a surrogate pair */
        surrogate_pair_started = 1;
        surrogate_high = ch;
        return;
    }

    input[0] = ch;
    input[1] = 0;
    utf16_to_utf8(input, buffer);
    if (buffer[0] == '\0')
        putc('\0', fp);
    else
        fputs(buffer, fp);
}
#else
void
tstring_putc(XML_Char UNUSED_P(ch), FILE *UNUSED_P(fp))
{
}
#endif

#if defined(XML_UNICODE) && !defined (XML_UNICODE_WCHAR_T)
int
tstring_cmp(const XML_Char *s1, const XML_Char *s2)
{
    while (*s1 != 0 && *s2 != 0) {
        if (*s1 < *s2)
            return -1;
        else if (*s1 > *s2)
            return 1;
        s1++;
        s2++;
    }
    if (*s1 != 0)
        return 1;
    if (*s2 != 0)
        return -1;
    return 0;
}
#else
int
tstring_cmp(const XML_Char *UNUSED_P(s1), const XML_Char *UNUSED_P(s2))
{
    return -1;
}
#endif

#if defined(XML_UNICODE) && !defined (XML_UNICODE_WCHAR_T)
void
tstring_cpy(XML_Char *d, const XML_Char *s)
{
    while (*s != 0)
        *d++ = *s++;
    *d = 0;
}
#else
void
tstring_cpy(XML_Char *UNUSED_P(d), const XML_Char *UNUSED_P(s))
{
}
#endif

#if defined(XML_UNICODE) && !defined (XML_UNICODE_WCHAR_T)
void
tstring_cat(XML_Char *d, const XML_Char *s)
{
    while (*d != 0)
        d++;
    while (*s != 0)
        *d++ = *s++;
    *d = 0;
}
#else
void
tstring_cat(XML_Char *UNUSED_P(d), const XML_Char *UNUSED_P(s))
{
}
#endif

#if defined(XML_UNICODE) && !defined (XML_UNICODE_WCHAR_T)
XML_Char *
tstring_chr(const XML_Char *s, int c)
{
    while (*s != 0)
        if (*s++ == c)
            return (XML_Char *)s-1;
    return NULL;
}
#else
XML_Char *
tstring_chr(const XML_Char *UNUSED_P(s), int UNUSED_P(c))
{
    return NULL;
}
#endif

#if defined(XML_UNICODE) && !defined (XML_UNICODE_WCHAR_T)
XML_Char *
tstring_rchr(const XML_Char *s, int c)
{
    const XML_Char *result = NULL;

    while (*s != 0)
        if (*s++ == c)
            result = s-1;
    return (XML_Char *)result;
}
#else
XML_Char *
tstring_rchr(const XML_Char *UNUSED_P(s), int UNUSED_P(c))
{
    return NULL;
}
#endif

#if defined(XML_UNICODE) && !defined (XML_UNICODE_WCHAR_T)
int
tstring_len(const XML_Char *s)
{
    int len = 0;

    while (*s++ != 0) {
        len++;
    }
    return len;
}
#else
int
tstring_len(const XML_Char *UNUSED_P(s))
{
    return 0;
}
#endif

#if defined(XML_UNICODE) && !defined (XML_UNICODE_WCHAR_T)
FILE *
tstring_fopen(const XML_Char *filename, XML_Char *mode)
{
    char *utf8_filename = utf16_dup_as_utf8(filename);
    char *utf8_mode = utf16_dup_as_utf8(mode);
    FILE *fp;

    if (utf8_filename == NULL || utf8_mode == NULL) {
        /* Freeing NULL pointers is safe */
        free(utf8_filename);
        free(utf8_mode);
        return NULL;
    }
    fp = fopen(utf8_filename, utf8_mode);
    free(utf8_filename);
    free(utf8_mode);
    return fp;
}
#else
FILE *
tstring_fopen(const XML_Char *UNUSED_P(filename), XML_Char *UNUSED_P(mode))
{
    return NULL;
}
#endif

#if defined(XML_UNICODE) && !defined (XML_UNICODE_WCHAR_T)
int
tstring_open(const XML_Char *filename, int flags)
{
    char *utf8_filename = utf16_dup_as_utf8(filename);
    int fd;

    if (utf8_filename == NULL)
        return -1;
    fd = open(utf8_filename, flags);
    free(utf8_filename);
    return fd;
}
#else
int
tstring_open(const XML_Char *UNUSED_P(filename), int UNUSED_P(flags))
{
    return -1;
}
#endif

#if defined(XML_UNICODE) && !defined (XML_UNICODE_WCHAR_T)
void
tstring_perror(const XML_Char *s)
{
    char *utf8 = utf16_dup_as_utf8(s);

    if (utf8 == NULL) {
        perror("Out of memory converting UTF-16");
    } else {
        perror(utf8);
        free(utf8);
    }
}
#else
void
tstring_perror(const XML_Char *UNUSED_P(s))
{
}
#endif
