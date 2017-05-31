/* Copyright (c) 2017 The Expat Maintainers
   See the file COPYING for copying permission.
*/

#if defined(XML_UNICODE) && defined(XML_UNICODE_WCHAR_T) && !defined(_WIN32)

#include <stdio.h>
#include <stdlib.h>
#include "wstring.h"

wchar_t *
xstring_char_to_wchar(XString **phead, const char *s)
{
    int len = mbstowcs(NULL, s, 0) + 1; /* How many characters? */
    XString *new;

    if (len == 0) { /* I.e. an error returned -1 */
        fprintf(stderr, "Error converting string to wchar\n");
        exit(1);
    }
    new = (XString *)malloc(sizeof(XString) + len * sizeof(wchar_t));
    if (new == NULL) {
        fprintf(stderr, "Unable to create wide string\n");
        exit(1);
    }
    mbstowcs(&new->str, s, len);
    new->next = *phead;
    *phead = new;
    return new->str;
}

void
xstring_dispose(XString *head)
{
    while (head != NULL) {
        XString *next = head->next;
        free(head);
        head = next;
    }
}

#endif
