/* Copyright (c) 2017 The Expat Maintainers
   See the file COPYING for copying permission.
*/

#ifndef XMLWF_WSTRING_H_INCLUDED
#define XMLWF_WSTRING_H_INCLUDED

#if defined(XML_UNICODE) && defined(XML_UNICODE_WCHAR_T) && !define(_WIN32)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct xstring_s {
    struct xstring_s *next;
    const wchar_t str[];
} XString;

extern wchar_t *xstring_char_to_wchar(XString **phead, const char *s);
extern void xstring_dispose(XString *head);

#ifdef __cplusplus
}
#endif

#endif /* XMLWF_WSTRING_H_INCLUDED */
