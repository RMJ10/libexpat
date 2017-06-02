/* Copyright (c) 2017 The Expat Maintainers
 * Copying is permitted under the MIT license.  See the file COPYING
 * for details.
 *
 * unicode.c: support for UTF-16 unicode handling without wchar_t
 */

#if defined (XML_UNICODE) && !defined(XML_UNICODE_WCHAR_T)

#include <stdlib.h>
#include <string.h>
#include "expat.h"
#include "minicheck.h"
#include "unicode.h"

const XML_Char *
tstring_create_utf16(TString **phead, const char *s)
{
    /* Over-estimate; all UTF-16 surrogate pairs encode U+10000 or
     * more, which take four bytes to represent in UTF-8.  The
     * following is guaranteed to be enough.
     */
    int len = strlen(s) + 1;
    TString *new = malloc(sizeof(TString));
    XML_Char *d;

    if (new == NULL)
        fail("Unable to create string constant");

    if ((d = malloc(len * sizeof(XML_Char))) == NULL) {
        free(new);
        fail("Unable to create string constant");
    }
    new->str = d;


    /* Covert UTF-8 to UTF-16 */
    while (*s) {
        unsigned long codepoint;

        if ((s[0] & 0xf8) == 0xf0) {
            /* The first byte of a 4-byte sequence.  Verify the next
             * three bytes are all continuations.
             */
            if ((s[1] & 0xc0) != 0x80 ||
                (s[2] & 0xc0) != 0x80 ||
                (s[3] & 0xc0) != 0x80) {
                fail("Invalid UTF-8 in string literal");
            }
            codepoint = ((s[0] & 0x07) << 18) |
                ((s[1] & 0x3f) << 12) |
                ((s[2] & 0x3f) << 6) |
                (s[3] & 0x3f);
            s += 4;
        }
        else if ((s[0] & 0xf0) == 0xe0) {
            /* The first byte of a 3-byte sequence.  Verify the next
             * two bytes are all continuations.
             */
            if ((s[1] & 0xc0) != 0x80 ||
                (s[2] & 0xc0) != 0x80) {
                fail("Invalid UTF-8 in string literal");
            }
            codepoint = ((s[0] & 0x0f) << 12) |
                ((s[1] & 0x3f) << 6) |
                (s[2] & 0x3f);
            s += 3;
        }
        else if ((s[0] & 0xe0) == 0xc0) {
            /* The first byte of a two byte sequence.  Verify the
             * second byte is a continuation.
             */
            if ((s[1] & 0xc0) != 0x80) {
                fail("Invalid UTF-8 in string literal");
            }
            codepoint = ((s[0] & 0x1f) << 6) | (s[1] & 0x3f);
            s += 2;
        }
        else if ((s[0] & 0x80) == 0) {
            codepoint = *s++;
        }
        else {
            fail("Invalid UTF-8 in string literal");
        }

        /* Now convert the codepoint to UTF-16 */
        if (codepoint >= 0x10000) {
            /* Requires a surrogate pair */
            codepoint -= 0x10000;
            *d++ = ((codepoint >> 10) & 0x03ff) | 0xd800;
            *d++ = (codepoint & 0x03ff) | 0xdc00;
        }
        else if (codepoint < 0xd800 || codepoint > 0xdfff) {
            *d++ = codepoint;
        }
        else {
            fail("Invalid codepoint in string literal");
        }
    }
    *d = 0;
    new->next = *phead;
    *phead = new;
    return (const XML_Char *)new->str;
}

static unsigned long
utf16_to_codepoint(const XML_Char **ps)
{
    const XML_Char *s = *ps;

    if ((*s & 0xfc00) == 0xd800) {
        /* A leading surrogate.  Check it is followed by a trailing
         * surrogate
         */
        if ((s[1] & 0xfc00) != 0xdc00) {
            fail("Invalid UTF-16 in string");
        }
        *ps = s+2;
        return (((s[0] & 0x03ff) << 10) | (s[1] & 0x03ff)) + 0x10000;
    }
    if ((*s & 0xfc00) == 0xdc00) {
        fail("Invalid UTF-16 in string");
    }
    *ps = s+1;
    return *s;
}


const char *
tstring_create_utf8(TString **phead, const XML_Char *s)
{
    int len = 0;
    TString *new = malloc(sizeof(TString));
    char *d;
    const XML_Char *p = s;
    unsigned long codepoint;

    if (new == NULL)
        fail("Unable to create string constant");

    /* First determine the length of the UTF-8 string to come */
    while (*p) {
        codepoint = utf16_to_codepoint(&p);
        if (codepoint >= 0x10000)
            len += 4;
        else if (codepoint >= 0x0800)
            len += 3;
        else if (codepoint >= 0x0080)
            len += 2;
        else
            len++;
    }

    if ((d = malloc(len + 1)) == NULL) {
        free(new);
        fail("Unable to create string constant");
    }
    new->str = d;

    /* Now do the conversion */
    p = s;
    while (*s) {
        codepoint = utf16_to_codepoint(&s);
        if (codepoint >= 0x10000) {
            d[0] = 0xf0 | ((codepoint >> 18) & 0x07);
            d[1] = 0x80 | ((codepoint >> 12) & 0x3f);
            d[2] = 0x80 | ((codepoint >> 6) & 0x3f);
            d[3] = 0x80 | (codepoint & 0x3f);
            d += 4;
        }
        else if (codepoint >= 0x0800) {
            d[0] = 0xe0 | ((codepoint >> 12) & 0x0f);
            d[1] = 0x80 | ((codepoint >> 6) & 0x3f);
            d[2] = 0x80 | (codepoint & 0x3f);
            d += 3;
        }
        else if (codepoint >= 0x0080) {
            d[0] = 0xc0 | ((codepoint >> 6) & 0x1f);
            d[1] = 0x80 | (codepoint & 0x3f);
            d += 2;
        }
        else {
            *d++ = codepoint & 0x7f;
        }
    }
    *d = 0;
    new->next = *phead;
    *phead = new;
    return (const char *)new->str;
}

void
tstring_dispose(TString *head)
{
    while (head != NULL) {
        TString *next = head->next;
        free(head->str);
        free(head);
        head = next;
    }
}

int
tstring_cmp(const XML_Char *s1, const XML_Char *s2)
{
    while (*s1 && *s2) {
        if (*s1 < *s2)
            return -1;
        if (*s1++ > *s2++)
            return 1;
    }
    if (*s1 < *s2)
        return -1;
    if (*s1 > *s2)
        return 1;
    return 0;
}

#endif /* defined (XML_UNICODE) && !defined(XML_UNICODE_WCHAR_T) */
