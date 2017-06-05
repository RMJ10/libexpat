/* Copyright (c) 2017 The Expat Maintainers
   See the file COPYING for copying permission.
*/

#ifndef XMLWF_U16STR_H_INCLUDED
#define XMLWF_U16STR_H_INCLUDED

#if defined(XML_UNICODE) && !defined (XML_UNICODE_WCHAR_T)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tstring_s {
    struct tstring_s *next;
    /* The C90 standard doesn't allow flexible arrays (type var[];), so
     * we use the old dodge of declaring a single byte but allocating far
     * more.  This will probably not play well with bounds checkers.
     */
    XML_Char str[1];
} TString;

extern const XML_Char *tstring_create(TString **phead, const char *s);
extern void tstring_dispose(TString *head);
extern void tstring_fprintf(FILE *fp, const XML_Char *format, ...);
extern void tstring_fputs(const XML_Char *s, FILE *fp);
extern void tstring_putc(XML_Char ch, FILE *fp);
extern int tstring_cmp(const XML_Char *s1, const XML_Char *s2);
extern void tstring_cpy(XML_Char *d, const XML_Char *s);
extern void tstring_cat(XML_Char *d, const XML_Char *s);
extern XML_Char *tstring_chr(const XML_Char *s, int c);
extern XML_Char *tstring_rchr(const XML_Char *s, int c);
extern int tstring_len(const XML_Char *s);
extern FILE *tstring_fopen(const XML_Char *filename, XML_Char *mode);
extern int tstring_open(const XML_Char *filename, int flags);
extern void tstring_perror(const XML_Char *s);

#ifdef __cplusplus
}
#endif

#endif /* XML_UNICODE && !XML_UNICODE_WCHAR_T */
#endif /* XMLWF_U16STR_H_INCLUDED */
