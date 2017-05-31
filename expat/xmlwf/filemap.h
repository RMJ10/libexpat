/* Copyright (c) 1998, 1999 Thai Open Source Software Center Ltd
   See the file COPYING for copying permission.
*/

#include <limits.h>  /* INT_MAX */
#include <stddef.h>


/* The following limit (for XML_Parse's int len) derives from
 * this loop in xmparse.c:
 *
 *    do {
 *      bufferSize = (int) (2U * (unsigned) bufferSize);
 *    } while (bufferSize < neededSize && bufferSize > 0);
 */
#define XML_MAX_CHUNK_LEN  (INT_MAX / 2 + 1)


int filemap(const XML_Char *name,
            void (*processor)(const void *, size_t,
                              const XML_Char *, void *arg),
            void *arg);
