#ifdef XML_UNICODE
#ifndef XML_UNICODE_WCHAR_T
#include "u16str.h"
/* This is a bit horrible.
 * Because C90 has no direct support for 16-bit characters, we have to
 * manually translate 8-bit string constants into 16-bit buffers.  We
 * can't use static buffers because there's the risk of more than one
 * string being wanted at once.  Therefore we allocate them
 * dynamically, and add fields to link them in a list so we can tear
 * down at the end of a function and not leak memory like a sieve.
 *
 * T_FN_START must come at the start of the function, to define the
 * head pointer for the linked list of allocated strings.
 */
#define T_FN_START TString *_tstring_list_head = NULL
/* T_FN_END invokes the function that frees the list */
#define T_FN_END tstring_dispose(_tstring_list_head)
/* TSTR invokes the function that creates the new u16 buffer */
#define TSTR(x) tstring_create(&_tstring_list_head, (x))
/* Be sure a default of signed char does not cause sign extension here */
#define TCH(x) ((unsigned short)(unsigned char)(x))
#define ftprintf tstring_fprintf
#define tfopen tstring_fopen
#define fputts tstring_fputs
#define puttc tstring_putc
#define tcscmp tstring_cmp
#define tcscpy tstring_cpy
#define tcscat tstring_cat
#define tcschr tstring_chr
#define tcsrchr tstring_rchr
#define tcslen tstring_len
#define tperror tstring_perror
#define topen tstring_open
#define tmain main
#else /* XML_UNICODE_WCHAR_T */
#define T_FN_START
#define T_FN_END
#define TSTR(x) L ## x
#define TCH(x) L ## x
#define ftprintf fwprintf
#define tfopen _wfopen
#define fputts fputws
#define puttc putwc
#define tcscmp wcscmp
#define tcscpy wcscpy
#define tcscat wcscat
#define tcschr wcschr
#define tcsrchr wcsrchr
#define tcslen wcslen
#define tperror _wperror
#define topen _wopen
#define tmain wmain
#define tremove _wremove
#endif /* XML_UNICODE_WCHAR_T */
#else /* not XML_UNICODE */
#define T_FN_START
#define T_FN_END
#define TSTR(x) x
#define TCH(x) x
#define ftprintf fprintf
#define tfopen fopen
#define fputts fputs
#define puttc putc
#define tcscmp strcmp
#define tcscpy strcpy
#define tcscat strcat
#define tcschr strchr
#define tcsrchr strrchr
#define tcslen strlen
#define tperror perror
#define topen open
#define tmain main
#define tremove remove
#endif /* not XML_UNICODE */
