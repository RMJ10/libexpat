/* Copyright (c) 2017 The Expat Maintainers
* Copying is permitted under the MIT license.  See the file COPYING
* for details.
*
* xmlchar.h: support various XML_Char internal data formats
*/

#ifndef XML_XMLCHAR_H
#define XML_XMLCHAR_H 1

#ifdef XML_UNICODE_WCHAR_T
#include <wchar.h>
#define XML_FMT_CHAR "lc"
#define XML_FMT_STR "ls"
#define XML_CHAR_strlen(s) wcslen(s)
#define XML_CHAR_strcmp(s, t) wcscmp((s), (t))
#define XML_CHAR_strncmp(s, t, n) wcsncmp((s), (t), (n))
#define XML_CHAR_CONST(s) _XML_CHAR_CONST(s)
#define _XML_CHAR_CONST(s) L ## s
#else
#ifdef XML_UNICODE
#error "No support for UTF-16 without wchar_t in tests"
#else
#define XML_FMT_CHAR "c"
#define XML_FMT_STR "s"
#define XML_CHAR_strlen(s) strlen(s)
#define XML_CHAR_strcmp(s, t) strcmp((s), (t))
#define XML_CHAR_strncmp(s, t, n) strncmp((s), (t), (n))
#define XML_CHAR_CONST(s) s
#endif
#endif



#endif /* XML_XMLCHAR_H */