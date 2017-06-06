/* Copyright (c) 1998, 1999 Thai Open Source Software Center Ltd
   See the file COPYING for copying permission.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "expat.h"
#include "codepage.h"
#include "internal.h"  /* for UNUSED_P only */
#include "xmlfile.h"
#include "xmltchar.h"

#ifdef _MSC_VER
#include <crtdbg.h>
#endif

#if defined(__amigaos__) && defined(__USE_INLINE__)
#include <proto/expat.h>
#endif

/* This ensures proper sorting. */

#define NSSEP TCH('\001')

static void XMLCALL
characterData(void *userData, const XML_Char *s, int len)
{
  FILE *fp = (FILE *)userData;
  T_FN_START;

  for (; len > 0; --len, ++s) {
    switch (*s) {
    case TCH('&'):
      fputts(TSTR("&amp;"), fp);
      break;
    case TCH('<'):
      fputts(TSTR("&lt;"), fp);
      break;
    case TCH('>'):
      fputts(TSTR("&gt;"), fp);
      break;
#ifdef W3C14N
    case 13:
      fputts(TSTR("&#xD;"), fp);
      break;
#else
    case TCH('"'):
      fputts(TSTR("&quot;"), fp);
      break;
    case 9:
    case 10:
    case 13:
      ftprintf(fp, TSTR("&#%d;"), *s);
      break;
#endif
    default:
      puttc(*s, fp);
      break;
    }
  }
  T_FN_END;
}

static void
attributeValue(FILE *fp, const XML_Char *s)
{
  T_FN_START;
  puttc(TCH('='), fp);
  puttc(TCH('"'), fp);
  for (;;) {
    switch (*s) {
    case 0:
    case NSSEP:
      puttc(TCH('"'), fp);
      return;
    case TCH('&'):
      fputts(TSTR("&amp;"), fp);
      break;
    case TCH('<'):
      fputts(TSTR("&lt;"), fp);
      break;
    case TCH('"'):
      fputts(TSTR("&quot;"), fp);
      break;
#ifdef W3C14N
    case 9:
      fputts(TSTR("&#x9;"), fp);
      break;
    case 10:
      fputts(TSTR("&#xA;"), fp);
      break;
    case 13:
      fputts(TSTR("&#xD;"), fp);
      break;
#else
    case TCH('>'):
      fputts(TSTR("&gt;"), fp);
      break;
    case 9:
    case 10:
    case 13:
      ftprintf(fp, TSTR("&#%d;"), *s);
      break;
#endif
    default:
      puttc(*s, fp);
      break;
    }
    s++;
  }
  T_FN_END;
}

/* Lexicographically comparing UTF-8 encoded attribute values,
is equivalent to lexicographically comparing based on the character number. */

static int
attcmp(const void *att1, const void *att2)
{
  return tcscmp(*(const XML_Char **)att1, *(const XML_Char **)att2);
}

static void XMLCALL
startElement(void *userData, const XML_Char *name, const XML_Char **atts)
{
  int nAtts;
  const XML_Char **p;
  FILE *fp = (FILE *)userData;
  puttc(TCH('<'), fp);
  fputts(name, fp);

  p = atts;
  while (*p)
    ++p;
  nAtts = (int)((p - atts) >> 1);
  if (nAtts > 1)
    qsort((void *)atts, nAtts, sizeof(XML_Char *) * 2, attcmp);
  while (*atts) {
    puttc(TCH(' '), fp);
    fputts(*atts++, fp);
    attributeValue(fp, *atts);
    atts++;
  }
  puttc(TCH('>'), fp);
}

static void XMLCALL
endElement(void *userData, const XML_Char *name)
{
  FILE *fp = (FILE *)userData;
  puttc(TCH('<'), fp);
  puttc(TCH('/'), fp);
  fputts(name, fp);
  puttc(TCH('>'), fp);
}

static int
nsattcmp(const void *p1, const void *p2)
{
  const XML_Char *att1 = *(const XML_Char **)p1;
  const XML_Char *att2 = *(const XML_Char **)p2;
  int sep1 = (tcsrchr(att1, NSSEP) != 0);
  int sep2 = (tcsrchr(att1, NSSEP) != 0);
  if (sep1 != sep2)
    return sep1 - sep2;
  return tcscmp(att1, att2);
}

static void XMLCALL
startElementNS(void *userData, const XML_Char *name, const XML_Char **atts)
{
  int nAtts;
  int nsi;
  const XML_Char **p;
  FILE *fp = (FILE *)userData;
  const XML_Char *sep;
  T_FN_START;

  puttc(TCH('<'), fp);

  sep = tcsrchr(name, NSSEP);
  if (sep) {
    fputts(TSTR("n1:"), fp);
    fputts(sep + 1, fp);
    fputts(TSTR(" xmlns:n1"), fp);
    attributeValue(fp, name);
    nsi = 2;
  }
  else {
    fputts(name, fp);
    nsi = 1;
  }

  p = atts;
  while (*p)
    ++p;
  nAtts = (int)((p - atts) >> 1);
  if (nAtts > 1)
    qsort((void *)atts, nAtts, sizeof(XML_Char *) * 2, nsattcmp);
  while (*atts) {
    name = *atts++;
    sep = tcsrchr(name, NSSEP);
    puttc(TCH(' '), fp);
    if (sep) {
      ftprintf(fp, TSTR("n%d:"), nsi);
      fputts(sep + 1, fp);
    }
    else
      fputts(name, fp);
    attributeValue(fp, *atts);
    if (sep) {
      ftprintf(fp, TSTR(" xmlns:n%d"), nsi++);
      attributeValue(fp, name);
    }
    atts++;
  }
  puttc(TCH('>'), fp);
  T_FN_END;
}

static void XMLCALL
endElementNS(void *userData, const XML_Char *name)
{
  FILE *fp = (FILE *)userData;
  const XML_Char *sep;
  T_FN_START;

  puttc(TCH('<'), fp);
  puttc(TCH('/'), fp);
  sep = tcsrchr(name, NSSEP);
  if (sep) {
    fputts(TSTR("n1:"), fp);
    fputts(sep + 1, fp);
  }
  else
    fputts(name, fp);
  puttc(TCH('>'), fp);
  T_FN_END;
}

#ifndef W3C14N

static void XMLCALL
processingInstruction(void *userData, const XML_Char *target,
                      const XML_Char *data)
{
  FILE *fp = (FILE *)userData;
  puttc(TCH('<'), fp);
  puttc(TCH('?'), fp);
  fputts(target, fp);
  puttc(TCH(' '), fp);
  fputts(data, fp);
  puttc(TCH('?'), fp);
  puttc(TCH('>'), fp);
}

#endif /* not W3C14N */

static void XMLCALL
defaultCharacterData(void *userData, const XML_Char *UNUSED_P(s), int UNUSED_P(len))
{
  XML_DefaultCurrent((XML_Parser) userData);
}

static void XMLCALL
defaultStartElement(void *userData, const XML_Char *UNUSED_P(name),
                    const XML_Char **UNUSED_P(atts))
{
  XML_DefaultCurrent((XML_Parser) userData);
}

static void XMLCALL
defaultEndElement(void *userData, const XML_Char *UNUSED_P(name))
{
  XML_DefaultCurrent((XML_Parser) userData);
}

static void XMLCALL
defaultProcessingInstruction(void *userData, const XML_Char *UNUSED_P(target),
                             const XML_Char *UNUSED_P(data))
{
  XML_DefaultCurrent((XML_Parser) userData);
}

static void XMLCALL
nopCharacterData(void *UNUSED_P(userData), const XML_Char *UNUSED_P(s), int UNUSED_P(len))
{
}

static void XMLCALL
nopStartElement(void *UNUSED_P(userData), const XML_Char *UNUSED_P(name), const XML_Char **UNUSED_P(atts))
{
}

static void XMLCALL
nopEndElement(void *UNUSED_P(userData), const XML_Char *UNUSED_P(name))
{
}

static void XMLCALL
nopProcessingInstruction(void *UNUSED_P(userData), const XML_Char *UNUSED_P(target),
                         const XML_Char *UNUSED_P(data))
{
}

static void XMLCALL
markup(void *userData, const XML_Char *s, int len)
{
  FILE *fp = (FILE *)XML_GetUserData((XML_Parser) userData);
  for (; len > 0; --len, ++s)
    puttc(*s, fp);
}

static void
metaLocation(XML_Parser parser)
{
  const XML_Char *uri = XML_GetBase(parser);
  T_FN_START;

  if (uri)
    ftprintf((FILE *)XML_GetUserData(parser), TSTR(" uri=\"%s\""), uri);
  ftprintf((FILE *)XML_GetUserData(parser),
           TSTR(" byte=\"%" XML_FMT_INT_MOD "d\" nbytes=\"%d\" \
			 line=\"%" XML_FMT_INT_MOD "u\" col=\"%" XML_FMT_INT_MOD "u\""),
           XML_GetCurrentByteIndex(parser),
           XML_GetCurrentByteCount(parser),
           XML_GetCurrentLineNumber(parser),
           XML_GetCurrentColumnNumber(parser));
  T_FN_END;
}

static void
metaStartDocument(void *userData)
{
  T_FN_START;
  fputts(TSTR("<document>\n"), (FILE *)XML_GetUserData((XML_Parser) userData));
  T_FN_END;
}

static void
metaEndDocument(void *userData)
{
  T_FN_START;
  fputts(TSTR("</document>\n"), (FILE *)XML_GetUserData((XML_Parser) userData));
  T_FN_END;
}

static void XMLCALL
metaStartElement(void *userData, const XML_Char *name,
                 const XML_Char **atts)
{
  XML_Parser parser = (XML_Parser) userData;
  FILE *fp = (FILE *)XML_GetUserData(parser);
  const XML_Char **specifiedAttsEnd
    = atts + XML_GetSpecifiedAttributeCount(parser);
  const XML_Char **idAttPtr;
  int idAttIndex = XML_GetIdAttributeIndex(parser);
  T_FN_START;

  if (idAttIndex < 0)
    idAttPtr = 0;
  else
    idAttPtr = atts + idAttIndex;
    
  ftprintf(fp, TSTR("<starttag name=\"%s\""), name);
  metaLocation(parser);
  if (*atts) {
    fputts(TSTR(">\n"), fp);
    do {
      ftprintf(fp, TSTR("<attribute name=\"%s\" value=\""), atts[0]);
      characterData(fp, atts[1], (int)tcslen(atts[1]));
      if (atts >= specifiedAttsEnd)
        fputts(TSTR("\" defaulted=\"yes\"/>\n"), fp);
      else if (atts == idAttPtr)
        fputts(TSTR("\" id=\"yes\"/>\n"), fp);
      else
        fputts(TSTR("\"/>\n"), fp);
    } while (*(atts += 2));
    fputts(TSTR("</starttag>\n"), fp);
  }
  else
    fputts(TSTR("/>\n"), fp);
  T_FN_END;
}

static void XMLCALL
metaEndElement(void *userData, const XML_Char *name)
{
  XML_Parser parser = (XML_Parser) userData;
  FILE *fp = (FILE *)XML_GetUserData(parser);
  T_FN_START;

  ftprintf(fp, TSTR("<endtag name=\"%s\""), name);
  metaLocation(parser);
  fputts(TSTR("/>\n"), fp);
  T_FN_END;
}

static void XMLCALL
metaProcessingInstruction(void *userData, const XML_Char *target,
                          const XML_Char *data)
{
  XML_Parser parser = (XML_Parser) userData;
  FILE *fp = (FILE *)XML_GetUserData(parser);
  T_FN_START;

  ftprintf(fp, TSTR("<pi target=\"%s\" data=\""), target);
  characterData(fp, data, (int)tcslen(data));
  puttc(TCH('"'), fp);
  metaLocation(parser);
  fputts(TSTR("/>\n"), fp);
  T_FN_END;
}

static void XMLCALL
metaComment(void *userData, const XML_Char *data)
{
  XML_Parser parser = (XML_Parser) userData;
  FILE *fp = (FILE *)XML_GetUserData(parser);
  T_FN_START;

  fputts(TSTR("<comment data=\""), fp);
  characterData(fp, data, (int)tcslen(data));
  puttc(TCH('"'), fp);
  metaLocation(parser);
  fputts(TSTR("/>\n"), fp);
  T_FN_END;
}

static void XMLCALL
metaStartCdataSection(void *userData)
{
  XML_Parser parser = (XML_Parser) userData;
  FILE *fp = (FILE *)XML_GetUserData(parser);
  T_FN_START;

  fputts(TSTR("<startcdata"), fp);
  metaLocation(parser);
  fputts(TSTR("/>\n"), fp);
  T_FN_END;
}

static void XMLCALL
metaEndCdataSection(void *userData)
{
  XML_Parser parser = (XML_Parser) userData;
  FILE *fp = (FILE *)XML_GetUserData(parser);
  T_FN_START;

  fputts(TSTR("<endcdata"), fp);
  metaLocation(parser);
  fputts(TSTR("/>\n"), fp);
  T_FN_END;
}

static void XMLCALL
metaCharacterData(void *userData, const XML_Char *s, int len)
{
  XML_Parser parser = (XML_Parser) userData;
  FILE *fp = (FILE *)XML_GetUserData(parser);
  T_FN_START;

  fputts(TSTR("<chars str=\""), fp);
  characterData(fp, s, len);
  puttc(TCH('"'), fp);
  metaLocation(parser);
  fputts(TSTR("/>\n"), fp);
  T_FN_END;
}

static void XMLCALL
metaStartDoctypeDecl(void *userData,
                     const XML_Char *doctypeName,
                     const XML_Char *UNUSED_P(sysid),
                     const XML_Char *UNUSED_P(pubid),
                     int UNUSED_P(has_internal_subset))
{
  XML_Parser parser = (XML_Parser) userData;
  FILE *fp = (FILE *)XML_GetUserData(parser);
  T_FN_START;

  ftprintf(fp, TSTR("<startdoctype name=\"%s\""), doctypeName);
  metaLocation(parser);
  fputts(TSTR("/>\n"), fp);
  T_FN_END;
}

static void XMLCALL
metaEndDoctypeDecl(void *userData)
{
  XML_Parser parser = (XML_Parser) userData;
  FILE *fp = (FILE *)XML_GetUserData(parser);
  T_FN_START;

  fputts(TSTR("<enddoctype"), fp);
  metaLocation(parser);
  fputts(TSTR("/>\n"), fp);
  T_FN_END;
}

static void XMLCALL
metaNotationDecl(void *userData,
                 const XML_Char *notationName,
                 const XML_Char *UNUSED_P(base),
                 const XML_Char *systemId,
                 const XML_Char *publicId)
{
  XML_Parser parser = (XML_Parser) userData;
  FILE *fp = (FILE *)XML_GetUserData(parser);
  T_FN_START;

  ftprintf(fp, TSTR("<notation name=\"%s\""), notationName);
  if (publicId)
    ftprintf(fp, TSTR(" public=\"%s\""), publicId);
  if (systemId) {
    fputts(TSTR(" system=\""), fp);
    characterData(fp, systemId, (int)tcslen(systemId));
    puttc(TCH('"'), fp);
  }
  metaLocation(parser);
  fputts(TSTR("/>\n"), fp);
  T_FN_END;
}


static void XMLCALL
metaEntityDecl(void *userData,
               const XML_Char *entityName,
               int  UNUSED_P(is_param),
               const XML_Char *value,
               int  value_length,
               const XML_Char *UNUSED_P(base),
               const XML_Char *systemId,
               const XML_Char *publicId,
               const XML_Char *notationName)
{
  XML_Parser parser = (XML_Parser) userData;
  FILE *fp = (FILE *)XML_GetUserData(parser);
  T_FN_START;

  if (value) {
    ftprintf(fp, TSTR("<entity name=\"%s\""), entityName);
    metaLocation(parser);
    puttc(TCH('>'), fp);
    characterData(fp, value, value_length);
    fputts(TSTR("</entity/>\n"), fp);
  }
  else if (notationName) {
    ftprintf(fp, TSTR("<entity name=\"%s\""), entityName);
    if (publicId)
      ftprintf(fp, TSTR(" public=\"%s\""), publicId);
    fputts(TSTR(" system=\""), fp);
    characterData(fp, systemId, (int)tcslen(systemId));
    puttc(TCH('"'), fp);
    ftprintf(fp, TSTR(" notation=\"%s\""), notationName);
    metaLocation(parser);
    fputts(TSTR("/>\n"), fp);
  }
  else {
    ftprintf(fp, TSTR("<entity name=\"%s\""), entityName);
    if (publicId)
      ftprintf(fp, TSTR(" public=\"%s\""), publicId);
    fputts(TSTR(" system=\""), fp);
    characterData(fp, systemId, (int)tcslen(systemId));
    puttc(TCH('"'), fp);
    metaLocation(parser);
    fputts(TSTR("/>\n"), fp);
  }
  T_FN_END;
}

static void XMLCALL
metaStartNamespaceDecl(void *userData,
                       const XML_Char *prefix,
                       const XML_Char *uri)
{
  XML_Parser parser = (XML_Parser) userData;
  FILE *fp = (FILE *)XML_GetUserData(parser);
  T_FN_START;

  fputts(TSTR("<startns"), fp);
  if (prefix)
    ftprintf(fp, TSTR(" prefix=\"%s\""), prefix);
  if (uri) {
    fputts(TSTR(" ns=\""), fp);
    characterData(fp, uri, (int)tcslen(uri));
    fputts(TSTR("\"/>\n"), fp);
  }
  else
    fputts(TSTR("/>\n"), fp);
  T_FN_END;
}

static void XMLCALL
metaEndNamespaceDecl(void *userData, const XML_Char *prefix)
{
  XML_Parser parser = (XML_Parser) userData;
  FILE *fp = (FILE *)XML_GetUserData(parser);
  T_FN_START;

  if (!prefix)
    fputts(TSTR("<endns/>\n"), fp);
  else
    ftprintf(fp, TSTR("<endns prefix=\"%s\"/>\n"), prefix);
  T_FN_END;
}

static int XMLCALL
unknownEncodingConvert(void *data, const char *p)
{
  return codepageConvert(*(int *)data, p);
}

static int XMLCALL
unknownEncoding(void *UNUSED_P(userData), const XML_Char *name, XML_Encoding *info)
{
  int cp;
#if defined(XML_UNICODE) && !defined(XML_UNICODE_WCHAR_T)
  /* There is no good way of doing this is all variation of XML_Char */
  static const XML_Char prefixL[] = { TCH('w'), TCH('i'), TCH('n'),
                                      TCH('d'), TCH('o'), TCH('w'),
                                      TCH('s'), TCH('-'), 0 };
  static const XML_Char prefixU[] = { TCH('W'), TCH('I'), TCH('N'),
                                      TCH('D'), TCH('O'), TCH('W'),
                                      TCH('S'), TCH('-'), 0 };
#else
  static const XML_Char prefixL[] = TSTR("windows-");
  static const XML_Char prefixU[] = TSTR("WINDOWS-");
#endif
  int i;

  for (i = 0; prefixU[i]; i++)
    if (name[i] != prefixU[i] && name[i] != prefixL[i])
      return 0;
  
  cp = 0;
  for (; name[i]; i++) {
#if defined(XML_UNICODE) && !defined(XML_UNICODE_WCHAR_T)
    /* There is still no good portable way of merging these */
    static const XML_Char digits[] = { TCH('0'), TCH('1'), TCH('2'),
                                       TCH('3'), TCH('4'), TCH('5'),
                                       TCH('6'), TCH('7'), TCH('8'),
                                       TCH('9'), 0 };
#else
    static const XML_Char digits[] = TSTR("0123456789");
#endif
    const XML_Char *s = tcschr(digits, name[i]);
    if (!s)
      return 0;
    cp *= 10;
    cp += (int)(s - digits);
    if (cp >= 0x10000)
      return 0;
  }
  if (!codepageMap(cp, info->map))
    return 0;
  info->convert = unknownEncodingConvert;
  /* We could just cast the code page integer to a void *,
  and avoid the use of release. */
  info->release = free;
  info->data = malloc(sizeof(int));
  if (!info->data)
    return 0;
  *(int *)info->data = cp;
  return 1;
}

static int XMLCALL
notStandalone(void *UNUSED_P(userData))
{
  return 0;
}


#ifndef XML_UNICODE
/* This is the simplest case.  We have no unicode, or we are running
 * in UTF-8 (whichever makes you feel better).  We can simply use the
 * regular main() and normal character and string literals.
 */
#define X_FN_START
#define X_FN_END
#define xmain main
#define XChar char
#define XCH(x) x
#define XSTR(x) x
#define X2T(x) x
#define xfprintf fprintf
#define xcsrchr strrchr
#define xcslen strlen
#define xcscpy strcpy
#define xcscat strcat
#define xfopen fopen
#define xperror perror
#define xremove remove

#else /* XML_UNICODE */
#ifndef XML_UNICODE_WCHAR_T
/* We are using unicode, but with 16-bit characters which totally lack
 * library support in C90.  The basic environment is 8-bit (we don't
 * get into 16-bit fun until later), so we again use the basic types
 * and the regular main()
 */
#define X_FN_START
#define X_FN_END
#define xmain main
#define XChar char
#define XCH(x) x
#define XSTR(x) x
#define X2T(x) TSTR(x)
#define xfprintf fprintf
#define xcsrchr strrchr
#define xcslen strlen
#define xcscpy strcpy
#define xcscat strcat
#define xfopen fopen
#define xperror perror
#define xremove remove

#else /* XML_UNICODE && XML_UNICODE_WCHAR_T */
#ifdef _WIN32
/* Here's the fun case.  On Windows with full wchar_t-flavoured
 * Unicode support, you are expected to use wmain() and play with an
 * environment set up with wide characters.  Once upon a time these
 * were 16-bit, and everything was easy.  Now they aren't, sometimes,
 * and it's all very confusing.
 */
#define X_FN_START
#define X_FN_END
#define xmain wmain
#define XChar wchar_t
#define XCH(x) L ## x
#define XSTR(x) L ## x
#define X2T(x) x
#define xfprintf fwprintf
#define xcsrchr wcsrchr
#define xcslen wcslen
#define xcscpy wcscpy
#define xcscat wcscat
#define xfopen _wfopen
#define xperror _wperror
#define xremove _wremove

#else /* XML_UNICODE && XML_UNICODE_WCHAR_T && !_WIN32 */
/* We have main(), and an 8-bit environment.  All the fancy stuff
 * occurs later on.  Unfortunately we sometimes need to convert.
 */
#define X_FN_START XString *_xstring_list_head = NULL;
#define X_FN_END xstring_dispose(_xstring_list_head);
#define xmain main
#define XChar char
#define XCH(x) x
#define XSTR(x) x
#define X2T(x) xstring_char_to_wchar(&_xstring_list_head, x)
#define xfprintf fprintf
#define xcsrchr strrchr
#define xcslen strlen
#define xcscpy strcpy
#define xcscat strcat
#define xfopen fopen
#define xperror perror
#define xremove remove

#endif /* !_WIN32 */
#endif /* XML_UNICODE_WCHAR_T */
#endif /* XML_UNICODE */

static void
showVersion(XChar *prog)
{
  XChar *s = prog;
  XChar ch;
  const XML_Feature *features = XML_GetFeatureList();
  T_FN_START;

  while ((ch = *s) != 0) {
      if (ch == XCH('/')
#if defined(_WIN32)
          || ch == XCH('\\')
#endif
        )
      prog = s + 1;
    ++s;
  }
  xfprintf(stdout, XSTR("%s "), prog);
  ftprintf(stdout, TSTR("using %s\n"), XML_ExpatVersion());
  if (features != NULL && features[0].feature != XML_FEATURE_END) {
    int i = 1;
    ftprintf(stdout, TSTR("%s"), features[0].name);
    if (features[0].value)
      ftprintf(stdout, TSTR("=%ld"), features[0].value);
    while (features[i].feature != XML_FEATURE_END) {
      ftprintf(stdout, TSTR(", %s"), features[i].name);
      if (features[i].value)
        ftprintf(stdout, TSTR("=%ld"), features[i].value);
      ++i;
    }
    ftprintf(stdout, TSTR("\n"));
  }
  T_FN_END;
}

static void
usage(const XChar *prog, int rc)
{
  xfprintf(stderr,
           XSTR("usage: %s [-s] [-n] [-p] [-x] [-e encoding] [-w] [-d output-dir] [-c] [-m] [-r] [-t] [file ...]\n"), prog);
  exit(rc);
}

int
xmain(int argc, XChar **argv)
{
  int i, j;
  const XChar *outputDir = NULL;
  const XML_Char *encoding = NULL;
  unsigned processFlags = XML_MAP_FILE;
  int windowsCodePages = 0;
  int outputType = 0;
  int useNamespaces = 0;
  int requireStandalone = 0;
  enum XML_ParamEntityParsing paramEntityParsing = 
    XML_PARAM_ENTITY_PARSING_NEVER;
  int useStdin = 0;
  T_FN_START;

#ifdef _MSC_VER
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);
#endif

  i = 1;
  j = 0;
  while (i < argc) {
    if (j == 0) {
      if (argv[i][0] != XCH('-'))
        break;
      if (argv[i][1] == XCH('-') && argv[i][2] == XCH('\0')) {
        i++;
        break;
      }
      j++;
    }
    switch (argv[i][j]) {
    case XCH('r'):
      processFlags &= ~XML_MAP_FILE;
      j++;
      break;
    case XCH('s'):
      requireStandalone = 1;
      j++;
      break;
    case XCH('n'):
      useNamespaces = 1;
      j++;
      break;
    case XCH('p'):
      paramEntityParsing = XML_PARAM_ENTITY_PARSING_ALWAYS;
      /* fall through */
    case XCH('x'):
      processFlags |= XML_EXTERNAL_ENTITIES;
      j++;
      break;
    case XCH('w'):
      windowsCodePages = 1;
      j++;
      break;
    case XCH('m'):
      outputType = 'm';
      j++;
      break;
    case XCH('c'):
      outputType = 'c';
      useNamespaces = 0;
      j++;
      break;
    case XCH('t'):
      outputType = 't';
      j++;
      break;
    case XCH('d'):
      if (argv[i][j + 1] == XCH('\0')) {
        if (++i == argc)
          usage(argv[0], 2);
        outputDir = argv[i];
      }
      else
        outputDir = argv[i] + j + 1;
      i++;
      j = 0;
      break;
    case XCH('e'):
      if (argv[i][j + 1] == XCH('\0')) {
        if (++i == argc)
          usage(argv[0], 2);
        encoding = X2T(argv[i]);
      }
      else
        encoding = X2T(argv[i] + j + 1);
      i++;
      j = 0;
      break;
    case XCH('h'):
      usage(argv[0], 0);
      return 0;
    case XCH('v'):
      showVersion(argv[0]);
      return 0;
    case XCH('\0'):
      if (j > 1) {
        i++;
        j = 0;
        break;
      }
      /* fall through */
    default:
      usage(argv[0], 2);
    }
  }
  if (i == argc) {
    useStdin = 1;
    processFlags &= ~XML_MAP_FILE;
    i--;
  }
  for (; i < argc; i++) {
    FILE *fp = 0;
    XChar *outName = 0;
    int result;
    XML_Parser parser;
    if (useNamespaces)
      parser = XML_ParserCreateNS(encoding, NSSEP);
    else
      parser = XML_ParserCreate(encoding);

    if (! parser) {
      perror("Could not instantiate parser");
      exit(1);
    }

    if (requireStandalone)
      XML_SetNotStandaloneHandler(parser, notStandalone);
    XML_SetParamEntityParsing(parser, paramEntityParsing);
    if (outputType == 't') {
      /* This is for doing timings; this gives a more realistic estimate of
         the parsing time. */
      outputDir = 0;
      XML_SetElementHandler(parser, nopStartElement, nopEndElement);
      XML_SetCharacterDataHandler(parser, nopCharacterData);
      XML_SetProcessingInstructionHandler(parser, nopProcessingInstruction);
    }
    else if (outputDir) {
      const XChar * delim = XSTR("/");
      const XChar *file = useStdin ? XSTR("STDIN") : argv[i];
      if (!useStdin) {
        /* Jump after last (back)slash */
        const XChar * lastDelim = xcsrchr(file, delim[0]);
        if (lastDelim)
          file = lastDelim + 1;
#if defined(_WIN32)
        else {
          const XML_Char * winDelim = XSTR("\\");
          lastDelim = xcsrchr(file, winDelim[0]);
          if (lastDelim) {
            file = lastDelim + 1;
            delim = winDelim;
          }
        }
#endif /* _WIN32 */
      }
      outName = (XChar *)malloc((xcslen(outputDir) + xcslen(file) + 2)
                       * sizeof(XChar));
      xcscpy(outName, outputDir);
      xcscat(outName, delim);
      xcscat(outName, file);
      fp = xfopen(outName, XSTR("wb"));
      if (!fp) {
        xperror(outName);
        exit(1);
      }
      setvbuf(fp, NULL, _IOFBF, 16384);
#if defined(XML_UNICODE_WCHAR_T) && defined(_WIN32)
      /* Write a BOM (is this still correct?) */
      puttc(0xFEFF, fp);
#endif
      XML_SetUserData(parser, fp);
      switch (outputType) {
      case 'm':
        XML_UseParserAsHandlerArg(parser);
        XML_SetElementHandler(parser, metaStartElement, metaEndElement);
        XML_SetProcessingInstructionHandler(parser, metaProcessingInstruction);
        XML_SetCommentHandler(parser, metaComment);
        XML_SetCdataSectionHandler(parser, metaStartCdataSection,
                                   metaEndCdataSection);
        XML_SetCharacterDataHandler(parser, metaCharacterData);
        XML_SetDoctypeDeclHandler(parser, metaStartDoctypeDecl,
                                  metaEndDoctypeDecl);
        XML_SetEntityDeclHandler(parser, metaEntityDecl);
        XML_SetNotationDeclHandler(parser, metaNotationDecl);
        XML_SetNamespaceDeclHandler(parser, metaStartNamespaceDecl,
                                    metaEndNamespaceDecl);
        metaStartDocument(parser);
        break;
      case 'c':
        XML_UseParserAsHandlerArg(parser);
        XML_SetDefaultHandler(parser, markup);
        XML_SetElementHandler(parser, defaultStartElement, defaultEndElement);
        XML_SetCharacterDataHandler(parser, defaultCharacterData);
        XML_SetProcessingInstructionHandler(parser,
                                            defaultProcessingInstruction);
        break;
      default:
        if (useNamespaces)
          XML_SetElementHandler(parser, startElementNS, endElementNS);
        else
          XML_SetElementHandler(parser, startElement, endElement);
        XML_SetCharacterDataHandler(parser, characterData);
#ifndef W3C14N
        XML_SetProcessingInstructionHandler(parser, processingInstruction);
#endif /* not W3C14N */
        break;
      }
    }
    if (windowsCodePages)
      XML_SetUnknownEncodingHandler(parser, unknownEncoding, 0);
    result = XML_ProcessFile(parser, useStdin ? NULL : X2T(argv[i]), processFlags);
    if (outputDir) {
      if (outputType == 'm')
        metaEndDocument(parser);
      fclose(fp);
      if (!result) {
        xremove(outName);
        exit(2);
      }
      free(outName);
    }
    XML_ParserFree(parser);
  }
  T_FN_END;
  return 0;
}
