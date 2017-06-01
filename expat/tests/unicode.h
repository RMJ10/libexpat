/* Copyright (c) 2017 The Expat Maintainers
 * Copying is permitted under the MIT license.  See the file COPYING
 * for details.
 *
 * unicode.h
 *
 * Macros and functions to let the test suite operate correctly
 * whatever unicode settings may be defined for the library.
 */

#ifndef XML_UNICODE_H
#define XML_UNICODE_H 1

#ifdef __cplusplus
extern "C" {
#endif

#ifdef XML_UNICODE
#ifdef XML_UNICODE_WCHAR_T

#error "wchar_t is not currently supported"

#else /* !XML_UNICODE_WCHAR_T */

/* 16-bit strings have no direct support under C90.  We have to cobble
 * together something horrible here.
 *
 * Rather than have a #if for every single XML_Char string literal, we
 * allocate a suitable chunk of memory, convert the string into it and
 * return that instead.  We could use static buffers instead of the
 * allocating fresh memory, but in places a number of strings are
 * required more or less simultaneously, and keeping track of that is
 * just inviting bugs.
 *
 * To avoid littering the test suite with small amounts of leaked
 * memory, we allocate enough space before the start of the string for
 * a pointer, and keep all the allocated strings for each function on
 * a linked list that we can free at the end of the function.  We do
 * this with macros so that the other builds don't have to carry the
 * overhead too.
 */

typedef struct tstring_s {
    struct tstring_s *next;
    void *str;
} TString;

/* Dangerous macro, but all the usual ways of making it safe won't work */
#define TSTR_FN_START TString *_tstring_list_head = NULL
#define TSTR_FN_END tstring_dispose(_tstring_list_head)

#define TCH(x) ((unsigned short)(unsigned char)(x))
#define TSTR(x) tstring_create_utf16(&_tstring_list_head, (x))
#define TSTR2CHAR(x) tstring_create_utf8(&_tstring_list_head, (x))

extern const XML_Char *tstring_create_utf16(TString **phead, const char *s);
extern const char *tstring_create_utf8(TString **phead, const XML_Char *s);
extern void tstring_dispose(TString *head);

#endif /* !XML_UNICODE_WCHAR_T */
#else /* !XML_UNICODE */

#define TSTR_FN_START
#define TSTR_FN_END

#define TCH(x) x
#define TSTR(x) x
#define TSTR2CHAR(x) x

#endif /* !XML_UNICODE */

#ifdef __cplusplus
}
#endif

#endif /* XML_UNICODE_H */
