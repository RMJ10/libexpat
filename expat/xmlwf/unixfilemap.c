/* Copyright (c) 1998, 1999 Thai Open Source Software Center Ltd
   See the file COPYING for copying permission.
*/

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#ifndef MAP_FILE
#define MAP_FILE 0
#endif

#include "expat.h"
#include "filemap.h"
#include "xmltchar.h"

int
filemap(const XML_Char *name,
        void (*processor)(const void *, size_t, const XML_Char *, void *arg),
        void *arg)
{
  int fd;
  size_t nbytes;
  struct stat sb;
  void *p;
  T_FN_START;

  fd = topen(name, O_RDONLY);
  if (fd < 0) {
    tperror(name);
    T_FN_END;
    return 0;
  }
  if (fstat(fd, &sb) < 0) {
    tperror(name);
    close(fd);
    T_FN_END;
    return 0;
  }
  if (!S_ISREG(sb.st_mode)) {
    close(fd);
    ftprintf(stderr, TSTR("%s: not a regular file\n"), name);
    T_FN_END;
    return 0;
  }
  if (sb.st_size > XML_MAX_CHUNK_LEN) {
    close(fd);
    T_FN_END;
    return 2;  /* Cannot be passed to XML_Parse in one go */
  }

  nbytes = sb.st_size;
  /* mmap fails for zero length files */
  if (nbytes == 0) {
    static const char c = '\0';
    processor(&c, 0, name, arg);
    close(fd);
    T_FN_END;
    return 1;
  }
  p = (void *)mmap((void *)0, (size_t)nbytes, PROT_READ,
                   MAP_FILE|MAP_PRIVATE, fd, (off_t)0);
  if (p == (void *)-1) {
    tperror(name);
    close(fd);
    T_FN_END;
    return 0;
  }
  processor(p, nbytes, name, arg);
  munmap((void *)p, nbytes);
  close(fd);
  T_FN_END;
  return 1;
}
