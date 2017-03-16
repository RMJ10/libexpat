/* Copyright (c) 1998, 1999, 2000 Thai Open Source Software Center Ltd
   See the file COPYING for copying permission.

   runtest.c : run the Expat test suite
*/

#ifdef HAVE_EXPAT_CONFIG_H
#include <expat_config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>  /* ptrdiff_t */
#include <ctype.h>
#ifndef __cplusplus
# include <stdbool.h>
#endif
#include <limits.h>

#include "expat.h"
#include "chardata.h"
#include "internal.h"  /* for UNUSED_P only */
#include "minicheck.h"
#include "ascii.h" /* for ASCII_xxx */

#if defined(__amigaos__) && defined(__USE_INLINE__)
#include <proto/expat.h>
#endif

#ifdef XML_LARGE_SIZE
#define XML_FMT_INT_MOD "ll"
#else
#define XML_FMT_INT_MOD "l"
#endif

static XML_Parser parser;


static void
basic_setup(void)
{
    parser = XML_ParserCreate(NULL);
    if (parser == NULL)
        fail("Parser not created.");
}

static void
basic_teardown(void)
{
    if (parser != NULL)
        XML_ParserFree(parser);
}

/* Generate a failure using the parser state to create an error message;
   this should be used when the parser reports an error we weren't
   expecting.
*/
static void
_xml_failure(XML_Parser parser, const char *file, int line)
{
    char buffer[1024];
    enum XML_Error err = XML_GetErrorCode(parser);
    sprintf(buffer,
            "    %d: %s (line %" XML_FMT_INT_MOD "u, offset %"\
                XML_FMT_INT_MOD "u)\n    reported from %s, line %d\n",
            err,
            XML_ErrorString(err),
            XML_GetCurrentLineNumber(parser),
            XML_GetCurrentColumnNumber(parser),
            file, line);
    _fail_unless(0, file, line, buffer);
}

static enum XML_Status
_XML_Parse_SINGLE_BYTES(XML_Parser parser, const char *s, int len, int isFinal)
{
    enum XML_Status res = XML_STATUS_ERROR;
    int offset = 0;

    if (len == 0) {
        return XML_Parse(parser, s, len, isFinal);
    }

    for (; offset < len; offset++) {
        const int innerIsFinal = (offset == len - 1) && isFinal;
        const char c = s[offset]; /* to help out-of-bounds detection */
        res = XML_Parse(parser, &c, sizeof(char), innerIsFinal);
        if (res != XML_STATUS_OK) {
            return res;
        }
    }
    return res;
}

#define xml_failure(parser) _xml_failure((parser), __FILE__, __LINE__)

static void
_expect_failure(const char *text, enum XML_Error errorCode, const char *errorMessage,
                const char *file, int lineno)
{
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_OK)
        /* Hackish use of _fail_unless() macro, but let's us report
           the right filename and line number. */
        _fail_unless(0, file, lineno, errorMessage);
    if (XML_GetErrorCode(parser) != errorCode)
        _xml_failure(parser, file, lineno);
}

#define expect_failure(text, errorCode, errorMessage) \
        _expect_failure((text), (errorCode), (errorMessage), \
                        __FILE__, __LINE__)

/* Dummy handlers for when we need to set a handler to tickle a bug,
   but it doesn't need to do anything.
*/
static unsigned long dummy_handler_flags = 0;

#define DUMMY_START_DOCTYPE_HANDLER_FLAG        (1UL << 0)
#define DUMMY_END_DOCTYPE_HANDLER_FLAG          (1UL << 1)
#define DUMMY_ENTITY_DECL_HANDLER_FLAG          (1UL << 2)
#define DUMMY_NOTATION_DECL_HANDLER_FLAG        (1UL << 3)
#define DUMMY_ELEMENT_DECL_HANDLER_FLAG         (1UL << 4)
#define DUMMY_ATTLIST_DECL_HANDLER_FLAG         (1UL << 5)
#define DUMMY_COMMENT_HANDLER_FLAG              (1UL << 6)
#define DUMMY_PI_HANDLER_FLAG                   (1UL << 7)
#define DUMMY_START_ELEMENT_HANDLER_FLAG        (1UL << 8)
#define DUMMY_START_CDATA_HANDLER_FLAG          (1UL << 9)
#define DUMMY_END_CDATA_HANDLER_FLAG            (1UL << 10)
#define DUMMY_UNPARSED_ENTITY_DECL_HANDLER_FLAG (1UL << 11)
#define DUMMY_START_NS_DECL_HANDLER_FLAG        (1UL << 12)
#define DUMMY_END_NS_DECL_HANDLER_FLAG          (1UL << 13)
#define DUMMY_START_DOCTYPE_DECL_HANDLER_FLAG   (1UL << 14)
#define DUMMY_END_DOCTYPE_DECL_HANDLER_FLAG     (1UL << 15)
#define DUMMY_SKIP_HANDLER_FLAG                 (1UL << 16)


static void XMLCALL
dummy_xdecl_handler(void *UNUSED_P(userData),
                    const XML_Char *UNUSED_P(version),
                    const XML_Char *UNUSED_P(encoding),
                    int UNUSED_P(standalone))
{}

static void XMLCALL
dummy_start_doctype_handler(void           *UNUSED_P(userData),
                            const XML_Char *UNUSED_P(doctypeName),
                            const XML_Char *UNUSED_P(sysid),
                            const XML_Char *UNUSED_P(pubid),
                            int            UNUSED_P(has_internal_subset))
{
    dummy_handler_flags |= DUMMY_START_DOCTYPE_HANDLER_FLAG;
}

static void XMLCALL
dummy_end_doctype_handler(void *UNUSED_P(userData))
{
    dummy_handler_flags |= DUMMY_END_DOCTYPE_HANDLER_FLAG;
}

static void XMLCALL
dummy_entity_decl_handler(void           *UNUSED_P(userData),
                          const XML_Char *UNUSED_P(entityName),
                          int            UNUSED_P(is_parameter_entity),
                          const XML_Char *UNUSED_P(value),
                          int            UNUSED_P(value_length),
                          const XML_Char *UNUSED_P(base),
                          const XML_Char *UNUSED_P(systemId),
                          const XML_Char *UNUSED_P(publicId),
                          const XML_Char *UNUSED_P(notationName))
{
    dummy_handler_flags |= DUMMY_ENTITY_DECL_HANDLER_FLAG;
}

static void XMLCALL
dummy_notation_decl_handler(void *UNUSED_P(userData),
                            const XML_Char *UNUSED_P(notationName),
                            const XML_Char *UNUSED_P(base),
                            const XML_Char *UNUSED_P(systemId),
                            const XML_Char *UNUSED_P(publicId))
{
    dummy_handler_flags |= DUMMY_NOTATION_DECL_HANDLER_FLAG;
}

static void XMLCALL
dummy_element_decl_handler(void *UNUSED_P(userData),
                           const XML_Char *UNUSED_P(name),
                           XML_Content *UNUSED_P(model))
{
    dummy_handler_flags |= DUMMY_ELEMENT_DECL_HANDLER_FLAG;
}

static void XMLCALL
dummy_attlist_decl_handler(void           *UNUSED_P(userData),
                           const XML_Char *UNUSED_P(elname),
                           const XML_Char *UNUSED_P(attname),
                           const XML_Char *UNUSED_P(att_type),
                           const XML_Char *UNUSED_P(dflt),
                           int            UNUSED_P(isrequired))
{
    dummy_handler_flags |= DUMMY_ATTLIST_DECL_HANDLER_FLAG;
}

static void XMLCALL
dummy_comment_handler(void *UNUSED_P(userData), const XML_Char *UNUSED_P(data))
{
    dummy_handler_flags |= DUMMY_COMMENT_HANDLER_FLAG;
}

static void XMLCALL
dummy_pi_handler(void *UNUSED_P(userData), const XML_Char *UNUSED_P(target), const XML_Char *UNUSED_P(data))
{
    dummy_handler_flags |= DUMMY_PI_HANDLER_FLAG;
}

static void XMLCALL
dummy_start_element(void *UNUSED_P(userData),
                    const XML_Char *UNUSED_P(name), const XML_Char **UNUSED_P(atts))
{
    dummy_handler_flags |= DUMMY_START_ELEMENT_HANDLER_FLAG;
}

static void XMLCALL
dummy_end_element(void *UNUSED_P(userData), const XML_Char *UNUSED_P(name))
{}

static void XMLCALL
dummy_start_cdata_handler(void *UNUSED_P(userData))
{
    dummy_handler_flags |= DUMMY_START_CDATA_HANDLER_FLAG;
}

static void XMLCALL
dummy_end_cdata_handler(void *UNUSED_P(userData))
{
    dummy_handler_flags |= DUMMY_END_CDATA_HANDLER_FLAG;
}

static void XMLCALL
dummy_cdata_handler(void *UNUSED_P(userData),
                    const XML_Char *UNUSED_P(s),
                    int UNUSED_P(len))
{}

static void XMLCALL
dummy_start_namespace_decl_handler(void *UNUSED_P(userData),
                                   const XML_Char *UNUSED_P(prefix),
                                   const XML_Char *UNUSED_P(uri))
{
    dummy_handler_flags |= DUMMY_START_NS_DECL_HANDLER_FLAG;
}

static void XMLCALL
dummy_end_namespace_decl_handler(void *UNUSED_P(userData),
                                 const XML_Char *UNUSED_P(prefix))
{
    dummy_handler_flags |= DUMMY_END_NS_DECL_HANDLER_FLAG;
}

/* This handler is obsolete, but while the code exists we should
 * ensure that dealing with the handler is covered by tests.
 */
static void XMLCALL
dummy_unparsed_entity_decl_handler(void *UNUSED_P(userData),
                                   const XML_Char *UNUSED_P(entityName),
                                   const XML_Char *UNUSED_P(base),
                                   const XML_Char *UNUSED_P(systemId),
                                   const XML_Char *UNUSED_P(publicId),
                                   const XML_Char *UNUSED_P(notationName))
{
    dummy_handler_flags |= DUMMY_UNPARSED_ENTITY_DECL_HANDLER_FLAG;
}

static void XMLCALL
dummy_default_handler(void *UNUSED_P(userData),
                      const XML_Char *UNUSED_P(s),
                      int UNUSED_P(len))
{}

static void XMLCALL
dummy_start_doctype_decl_handler(void *UNUSED_P(userData),
                                 const XML_Char *UNUSED_P(doctypeName),
                                 const XML_Char *UNUSED_P(sysid),
                                 const XML_Char *UNUSED_P(pubid),
                                 int UNUSED_P(has_internal_subset))
{
    dummy_handler_flags |= DUMMY_START_DOCTYPE_DECL_HANDLER_FLAG;
}

static void XMLCALL
dummy_end_doctype_decl_handler(void *UNUSED_P(userData))
{
    dummy_handler_flags |= DUMMY_END_DOCTYPE_DECL_HANDLER_FLAG;
}

static void XMLCALL
dummy_skip_handler(void *UNUSED_P(userData),
                   const XML_Char *UNUSED_P(entityName),
                   int UNUSED_P(is_parameter_entity))
{
    dummy_handler_flags |= DUMMY_SKIP_HANDLER_FLAG;
}

/*
 * Character & encoding tests.
 */

START_TEST(test_nul_byte)
{
    char text[] = "<doc>\0</doc>";

    /* test that a NUL byte (in US-ASCII data) is an error */
    if (_XML_Parse_SINGLE_BYTES(parser, text, sizeof(text) - 1, XML_TRUE) == XML_STATUS_OK)
        fail("Parser did not report error on NUL-byte.");
    if (XML_GetErrorCode(parser) != XML_ERROR_INVALID_TOKEN)
        xml_failure(parser);
}
END_TEST


START_TEST(test_u0000_char)
{
    /* test that a NUL byte (in US-ASCII data) is an error */
    expect_failure("<doc>&#0;</doc>",
                   XML_ERROR_BAD_CHAR_REF,
                   "Parser did not report error on NUL-byte.");
}
END_TEST

START_TEST(test_bom_utf8)
{
    /* This test is really just making sure we don't core on a UTF-8 BOM. */
    const char *text = "\357\273\277<e/>";

    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

START_TEST(test_bom_utf16_be)
{
    char text[] = "\376\377\0<\0e\0/\0>";

    if (_XML_Parse_SINGLE_BYTES(parser, text, sizeof(text)-1, XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

START_TEST(test_bom_utf16_le)
{
    char text[] = "\377\376<\0e\0/\0>\0";

    if (_XML_Parse_SINGLE_BYTES(parser, text, sizeof(text)-1, XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

static void XMLCALL
accumulate_characters(void *userData, const XML_Char *s, int len)
{
    CharData_AppendXMLChars((CharData *)userData, s, len);
}

static void XMLCALL
accumulate_attribute(void *userData, const XML_Char *UNUSED_P(name),
                     const XML_Char **atts)
{
    CharData *storage = (CharData *)userData;
    if (storage->count < 0 && atts != NULL && atts[0] != NULL) {
        /* "accumulate" the value of the first attribute we see */
        CharData_AppendXMLChars(storage, atts[1], -1);
    }
}


static void
_run_character_check(const XML_Char *text, const XML_Char *expected,
                     const char *file, int line)
{
    CharData storage;

    CharData_Init(&storage);
    XML_SetUserData(parser, &storage);
    XML_SetCharacterDataHandler(parser, accumulate_characters);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        _xml_failure(parser, file, line);
    CharData_CheckXMLChars(&storage, expected);
}

#define run_character_check(text, expected) \
        _run_character_check(text, expected, __FILE__, __LINE__)

static void
_run_attribute_check(const XML_Char *text, const XML_Char *expected,
                     const char *file, int line)
{
    CharData storage;

    CharData_Init(&storage);
    XML_SetUserData(parser, &storage);
    XML_SetStartElementHandler(parser, accumulate_attribute);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        _xml_failure(parser, file, line);
    CharData_CheckXMLChars(&storage, expected);
}

#define run_attribute_check(text, expected) \
        _run_attribute_check(text, expected, __FILE__, __LINE__)

typedef struct ExtTest {
    const char *parse_text;
    const char *encoding;
    CharData   *storage;
} ExtTest;

static void XMLCALL
ext_accumulate_characters(void *userData, const XML_Char *s, int len)
{
    ExtTest *test_data = (ExtTest *)userData;
    accumulate_characters(test_data->storage, s, len);
}

static void
_run_ext_character_check(const XML_Char *text,
                         ExtTest *test_data,
                         const XML_Char *expected,
                         const char *file, int line)
{
    CharData storage;

    CharData_Init(&storage);
    test_data->storage = &storage;
    XML_SetUserData(parser, test_data);
    XML_SetCharacterDataHandler(parser, ext_accumulate_characters);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        _xml_failure(parser, file, line);
    CharData_CheckXMLChars(&storage, expected);
}

#define run_ext_character_check(text, test_data, expected)               \
    _run_ext_character_check(text, test_data, expected, __FILE__, __LINE__)

/* Regression test for SF bug #491986. */
START_TEST(test_danish_latin1)
{
    const char *text =
        "<?xml version='1.0' encoding='iso-8859-1'?>\n"
        "<e>J\xF8rgen \xE6\xF8\xE5\xC6\xD8\xC5</e>";
    run_character_check(text,
             "J\xC3\xB8rgen \xC3\xA6\xC3\xB8\xC3\xA5\xC3\x86\xC3\x98\xC3\x85");
}
END_TEST


/* Regression test for SF bug #514281. */
START_TEST(test_french_charref_hexidecimal)
{
    const char *text =
        "<?xml version='1.0' encoding='iso-8859-1'?>\n"
        "<doc>&#xE9;&#xE8;&#xE0;&#xE7;&#xEA;&#xC8;</doc>";
    run_character_check(text,
                        "\xC3\xA9\xC3\xA8\xC3\xA0\xC3\xA7\xC3\xAA\xC3\x88");
}
END_TEST

START_TEST(test_french_charref_decimal)
{
    const char *text =
        "<?xml version='1.0' encoding='iso-8859-1'?>\n"
        "<doc>&#233;&#232;&#224;&#231;&#234;&#200;</doc>";
    run_character_check(text,
                        "\xC3\xA9\xC3\xA8\xC3\xA0\xC3\xA7\xC3\xAA\xC3\x88");
}
END_TEST

START_TEST(test_french_latin1)
{
    const char *text =
        "<?xml version='1.0' encoding='iso-8859-1'?>\n"
        "<doc>\xE9\xE8\xE0\xE7\xEa\xC8</doc>";
    run_character_check(text,
                        "\xC3\xA9\xC3\xA8\xC3\xA0\xC3\xA7\xC3\xAA\xC3\x88");
}
END_TEST

START_TEST(test_french_utf8)
{
    const char *text =
        "<?xml version='1.0' encoding='utf-8'?>\n"
        "<doc>\xC3\xA9</doc>";
    run_character_check(text, "\xC3\xA9");
}
END_TEST

/* Regression test for SF bug #600479.
   XXX There should be a test that exercises all legal XML Unicode
   characters as PCDATA and attribute value content, and XML Name
   characters as part of element and attribute names.
*/
START_TEST(test_utf8_false_rejection)
{
    const char *text = "<doc>\xEF\xBA\xBF</doc>";
    run_character_check(text, "\xEF\xBA\xBF");
}
END_TEST

/* Regression test for SF bug #477667.
   This test assures that any 8-bit character followed by a 7-bit
   character will not be mistakenly interpreted as a valid UTF-8
   sequence.
*/
START_TEST(test_illegal_utf8)
{
    char text[100];
    int i;

    for (i = 128; i <= 255; ++i) {
        sprintf(text, "<e>%ccd</e>", i);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_OK) {
            sprintf(text,
                    "expected token error for '%c' (ordinal %d) in UTF-8 text",
                    i, i);
            fail(text);
        }
        else if (XML_GetErrorCode(parser) != XML_ERROR_INVALID_TOKEN)
            xml_failure(parser);
        /* Reset the parser since we use the same parser repeatedly. */
        XML_ParserReset(parser, NULL);
    }
}
END_TEST


/* Examples, not masks: */
#define UTF8_LEAD_1  "\x7f"  /* 0b01111111 */
#define UTF8_LEAD_2  "\xdf"  /* 0b11011111 */
#define UTF8_LEAD_3  "\xef"  /* 0b11101111 */
#define UTF8_LEAD_4  "\xf7"  /* 0b11110111 */
#define UTF8_FOLLOW  "\xbf"  /* 0b10111111 */

START_TEST(test_utf8_auto_align)
{
    struct TestCase {
        ptrdiff_t expectedMovementInChars;
        const char * input;
    };

    struct TestCase cases[] = {
        {00, ""},

        {00, UTF8_LEAD_1},

        {-1, UTF8_LEAD_2},
        {00, UTF8_LEAD_2 UTF8_FOLLOW},

        {-1, UTF8_LEAD_3},
        {-2, UTF8_LEAD_3 UTF8_FOLLOW},
        {00, UTF8_LEAD_3 UTF8_FOLLOW UTF8_FOLLOW},

        {-1, UTF8_LEAD_4},
        {-2, UTF8_LEAD_4 UTF8_FOLLOW},
        {-3, UTF8_LEAD_4 UTF8_FOLLOW UTF8_FOLLOW},
        {00, UTF8_LEAD_4 UTF8_FOLLOW UTF8_FOLLOW UTF8_FOLLOW},
    };

    size_t i = 0;
    bool success = true;
    for (; i < sizeof(cases) / sizeof(*cases); i++) {
        const char * fromLim = cases[i].input + strlen(cases[i].input);
        const char * const fromLimInitially = fromLim;
        ptrdiff_t actualMovementInChars;

        align_limit_to_full_utf8_characters(cases[i].input, &fromLim);

        actualMovementInChars = (fromLim - fromLimInitially);
        if (actualMovementInChars != cases[i].expectedMovementInChars) {
            size_t j = 0;
            success = false;
            printf("[-] UTF-8 case %2lu: Expected movement by %2ld chars"
                    ", actually moved by %2ld chars: \"",
                    i + 1, cases[i].expectedMovementInChars, actualMovementInChars);
            for (; j < strlen(cases[i].input); j++) {
                printf("\\x%02x", (unsigned char)cases[i].input[j]);
            }
            printf("\"\n");
        }
    }

    if (! success) {
        fail("UTF-8 auto-alignment is not bullet-proof\n");
    }
}
END_TEST

START_TEST(test_utf16)
{
    /* <?xml version="1.0" encoding="UTF-16"?>
       <doc a='123'>some text</doc>
    */
    char text[] =
        "\000<\000?\000x\000m\000\154\000 \000v\000e\000r\000s\000i\000o"
        "\000n\000=\000'\0001\000.\000\060\000'\000 \000e\000n\000c\000o"
        "\000d\000i\000n\000g\000=\000'\000U\000T\000F\000-\0001\000\066"
        "\000'\000?\000>\000\n"
        "\000<\000d\000o\000c\000 \000a\000=\000'\0001\0002\0003\000'"
        "\000>\000s\000o\000m\000e\000 \000t\000e\000x\000t\000<\000/"
        "\000d\000o\000c\000>";
    if (_XML_Parse_SINGLE_BYTES(parser, text, sizeof(text)-1, XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

START_TEST(test_utf16_le_epilog_newline)
{
    unsigned int first_chunk_bytes = 17;
    char text[] = 
        "\xFF\xFE"                      /* BOM */
        "<\000e\000/\000>\000"          /* document element */
        "\r\000\n\000\r\000\n\000";     /* epilog */

    if (first_chunk_bytes >= sizeof(text) - 1)
        fail("bad value of first_chunk_bytes");
    if (  _XML_Parse_SINGLE_BYTES(parser, text, first_chunk_bytes, XML_FALSE)
          == XML_STATUS_ERROR)
        xml_failure(parser);
    else {
        enum XML_Status rc;
        rc = _XML_Parse_SINGLE_BYTES(parser, text + first_chunk_bytes,
                       sizeof(text) - first_chunk_bytes - 1, XML_TRUE);
        if (rc == XML_STATUS_ERROR)
            xml_failure(parser);
    }
}
END_TEST

/* Test that an outright lie in the encoding is faulted */
START_TEST(test_not_utf16)
{
    const char *text =
        "<?xml version='1.0' encoding='utf-16'?>"
        "<doc>Hi</doc>";

    /* Use a handler to provoke the appropriate code paths */
    XML_SetXmlDeclHandler(parser, dummy_xdecl_handler);
    expect_failure(text,
                   XML_ERROR_INCORRECT_ENCODING,
                   "UTF-16 declared in UTF-8 not faulted");
}
END_TEST

/* Test that an unknown encoding is rejected */
START_TEST(test_bad_encoding)
{
    const char *text = "<doc>Hi</doc>";

    if (!XML_SetEncoding(parser, "unknown-encoding"))
        fail("XML_SetEncoding failed");
    expect_failure(text,
                   XML_ERROR_UNKNOWN_ENCODING,
                   "Unknown encoding not faulted");
}
END_TEST

/* Regression test for SF bug #481609, #774028. */
START_TEST(test_latin1_umlauts)
{
    const char *text =
        "<?xml version='1.0' encoding='iso-8859-1'?>\n"
        "<e a='\xE4 \xF6 \xFC &#228; &#246; &#252; &#x00E4; &#x0F6; &#xFC; >'\n"
        "  >\xE4 \xF6 \xFC &#228; &#246; &#252; &#x00E4; &#x0F6; &#xFC; ></e>";
    const char *utf8 =
        "\xC3\xA4 \xC3\xB6 \xC3\xBC "
        "\xC3\xA4 \xC3\xB6 \xC3\xBC "
        "\xC3\xA4 \xC3\xB6 \xC3\xBC >";
    run_character_check(text, utf8);
    XML_ParserReset(parser, NULL);
    run_attribute_check(text, utf8);
    /* Repeat with a default handler */
    XML_ParserReset(parser, NULL);
    XML_SetDefaultHandler(parser, dummy_default_handler);
    run_character_check(text, utf8);
    XML_ParserReset(parser, NULL);
    XML_SetDefaultHandler(parser, dummy_default_handler);
    run_attribute_check(text, utf8);
}
END_TEST

/* Regression test #1 for SF bug #653180. */
START_TEST(test_line_number_after_parse)
{  
    const char *text =
        "<tag>\n"
        "\n"
        "\n</tag>";
    XML_Size lineno;

    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_FALSE) == XML_STATUS_ERROR)
        xml_failure(parser);
    lineno = XML_GetCurrentLineNumber(parser);
    if (lineno != 4) {
        char buffer[100];
        sprintf(buffer, 
            "expected 4 lines, saw %" XML_FMT_INT_MOD "u", lineno);
        fail(buffer);
    }
}
END_TEST

/* Regression test #2 for SF bug #653180. */
START_TEST(test_column_number_after_parse)
{
    const char *text = "<tag></tag>";
    XML_Size colno;

    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_FALSE) == XML_STATUS_ERROR)
        xml_failure(parser);
    colno = XML_GetCurrentColumnNumber(parser);
    if (colno != 11) {
        char buffer[100];
        sprintf(buffer, 
            "expected 11 columns, saw %" XML_FMT_INT_MOD "u", colno);
        fail(buffer);
    }
}
END_TEST

static void XMLCALL
start_element_event_handler2(void *userData, const XML_Char *name,
			     const XML_Char **UNUSED_P(attr))
{
    CharData *storage = (CharData *) userData;
    char buffer[100];

    sprintf(buffer,
        "<%s> at col:%" XML_FMT_INT_MOD "u line:%"\
            XML_FMT_INT_MOD "u\n", name,
	    XML_GetCurrentColumnNumber(parser),
	    XML_GetCurrentLineNumber(parser));
    CharData_AppendString(storage, buffer);
}

static void XMLCALL
end_element_event_handler2(void *userData, const XML_Char *name)
{
    CharData *storage = (CharData *) userData;
    char buffer[100];

    sprintf(buffer,
        "</%s> at col:%" XML_FMT_INT_MOD "u line:%"\
            XML_FMT_INT_MOD "u\n", name,
	    XML_GetCurrentColumnNumber(parser),
	    XML_GetCurrentLineNumber(parser));
    CharData_AppendString(storage, buffer);
}

/* Regression test #3 for SF bug #653180. */
START_TEST(test_line_and_column_numbers_inside_handlers)
{
    const char *text =
        "<a>\n"        /* Unix end-of-line */
        "  <b>\r\n"    /* Windows end-of-line */
        "    <c/>\r"   /* Mac OS end-of-line */
        "  </b>\n"
        "  <d>\n"
        "    <f/>\n"
        "  </d>\n"
        "</a>";
    const char *expected =
        "<a> at col:0 line:1\n"
        "<b> at col:2 line:2\n"
        "<c> at col:4 line:3\n"
        "</c> at col:8 line:3\n"
        "</b> at col:2 line:4\n"
        "<d> at col:2 line:5\n"
        "<f> at col:4 line:6\n"
        "</f> at col:8 line:6\n"
        "</d> at col:2 line:7\n"
        "</a> at col:0 line:8\n";
    CharData storage;

    CharData_Init(&storage);
    XML_SetUserData(parser, &storage);
    XML_SetStartElementHandler(parser, start_element_event_handler2);
    XML_SetEndElementHandler(parser, end_element_event_handler2);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);

    CharData_CheckString(&storage, expected); 
}
END_TEST

/* Regression test #4 for SF bug #653180. */
START_TEST(test_line_number_after_error)
{
    const char *text =
        "<a>\n"
        "  <b>\n"
        "  </a>";  /* missing </b> */
    XML_Size lineno;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_FALSE) != XML_STATUS_ERROR)
        fail("Expected a parse error");

    lineno = XML_GetCurrentLineNumber(parser);
    if (lineno != 3) {
        char buffer[100];
        sprintf(buffer, "expected 3 lines, saw %" XML_FMT_INT_MOD "u", lineno);
        fail(buffer);
    }
}
END_TEST
    
/* Regression test #5 for SF bug #653180. */
START_TEST(test_column_number_after_error)
{
    const char *text =
        "<a>\n"
        "  <b>\n"
        "  </a>";  /* missing </b> */
    XML_Size colno;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_FALSE) != XML_STATUS_ERROR)
        fail("Expected a parse error");

    colno = XML_GetCurrentColumnNumber(parser);
    if (colno != 4) { 
        char buffer[100];
        sprintf(buffer, 
            "expected 4 columns, saw %" XML_FMT_INT_MOD "u", colno);
        fail(buffer);
    }
}
END_TEST

/* Regression test for SF bug #478332. */
START_TEST(test_really_long_lines)
{
    /* This parses an input line longer than INIT_DATA_BUF_SIZE
       characters long (defined to be 1024 in xmlparse.c).  We take a
       really cheesy approach to building the input buffer, because
       this avoids writing bugs in buffer-filling code.
    */
    const char *text =
        "<e>"
        /* 64 chars */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        /* until we have at least 1024 characters on the line: */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "</e>";
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

/* Test cdata processing across a buffer boundary */
START_TEST(test_really_long_encoded_lines)
{
    /* As above, except that we want to provoke an output buffer
     * overflow with a non-trivial encoding.  For this we need to pass
     * the whole cdata in one go, not byte-by-byte.
     */
    void *buffer;
    const char *text =
        "<?xml version='1.0' encoding='iso-8859-1'?>"
        "<e>"
        /* 64 chars */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        /* until we have at least 1024 characters on the line: */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "</e>";
    int parse_len = strlen(text);

    /* Need a cdata handler to provoke the code path we want to test */
    XML_SetCharacterDataHandler(parser, dummy_cdata_handler);
    buffer = XML_GetBuffer(parser, parse_len);
    if (buffer == NULL)
        fail("Could not allocate parse buffer");
    memcpy(buffer, text, parse_len);
    if (XML_ParseBuffer(parser, parse_len, XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST


/*
 * Element event tests.
 */

static void XMLCALL
end_element_event_handler(void *userData, const XML_Char *name)
{
    CharData *storage = (CharData *) userData;
    CharData_AppendString(storage, "/");
    CharData_AppendXMLChars(storage, name, -1);
}

START_TEST(test_end_element_events)
{
    const char *text = "<a><b><c/></b><d><f/></d></a>";
    const char *expected = "/c/b/f/d/a";
    CharData storage;

    CharData_Init(&storage);
    XML_SetUserData(parser, &storage);
    XML_SetEndElementHandler(parser, end_element_event_handler);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    CharData_CheckString(&storage, expected);
}
END_TEST


/*
 * Attribute tests.
 */

/* Helpers used by the following test; this checks any "attr" and "refs"
   attributes to make sure whitespace has been normalized.

   Return true if whitespace has been normalized in a string, using
   the rules for attribute value normalization.  The 'is_cdata' flag
   is needed since CDATA attributes don't need to have multiple
   whitespace characters collapsed to a single space, while other
   attribute data types do.  (Section 3.3.3 of the recommendation.)
*/
static int
is_whitespace_normalized(const XML_Char *s, int is_cdata)
{
    int blanks = 0;
    int at_start = 1;
    while (*s) {
        if (*s == ' ')
            ++blanks;
        else if (*s == '\t' || *s == '\n' || *s == '\r')
            return 0;
        else {
            if (at_start) {
                at_start = 0;
                if (blanks && !is_cdata)
                    /* illegal leading blanks */
                    return 0;
            }
            else if (blanks > 1 && !is_cdata)
                return 0;
            blanks = 0;
        }
        ++s;
    }
    if (blanks && !is_cdata)
        return 0;
    return 1;
}

/* Check the attribute whitespace checker: */
static void
testhelper_is_whitespace_normalized(void)
{
    assert(is_whitespace_normalized("abc", 0));
    assert(is_whitespace_normalized("abc", 1));
    assert(is_whitespace_normalized("abc def ghi", 0));
    assert(is_whitespace_normalized("abc def ghi", 1));
    assert(!is_whitespace_normalized(" abc def ghi", 0));
    assert(is_whitespace_normalized(" abc def ghi", 1));
    assert(!is_whitespace_normalized("abc  def ghi", 0));
    assert(is_whitespace_normalized("abc  def ghi", 1));
    assert(!is_whitespace_normalized("abc def ghi ", 0));
    assert(is_whitespace_normalized("abc def ghi ", 1));
    assert(!is_whitespace_normalized(" ", 0));
    assert(is_whitespace_normalized(" ", 1));
    assert(!is_whitespace_normalized("\t", 0));
    assert(!is_whitespace_normalized("\t", 1));
    assert(!is_whitespace_normalized("\n", 0));
    assert(!is_whitespace_normalized("\n", 1));
    assert(!is_whitespace_normalized("\r", 0));
    assert(!is_whitespace_normalized("\r", 1));
    assert(!is_whitespace_normalized("abc\t def", 1));
}

static void XMLCALL
check_attr_contains_normalized_whitespace(void *UNUSED_P(userData),
                                          const XML_Char *UNUSED_P(name),
                                          const XML_Char **atts)
{
    int i;
    for (i = 0; atts[i] != NULL; i += 2) {
        const XML_Char *attrname = atts[i];
        const XML_Char *value = atts[i + 1];
        if (strcmp("attr", attrname) == 0
            || strcmp("ents", attrname) == 0
            || strcmp("refs", attrname) == 0) {
            if (!is_whitespace_normalized(value, 0)) {
                char buffer[256];
                sprintf(buffer, "attribute value not normalized: %s='%s'",
                        attrname, value);
                fail(buffer);
            }
        }
    }
}

START_TEST(test_attr_whitespace_normalization)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "  <!ATTLIST doc\n"
        "            attr NMTOKENS #REQUIRED\n"
        "            ents ENTITIES #REQUIRED\n"
        "            refs IDREFS   #REQUIRED>\n"
        "]>\n"
        "<doc attr='    a  b c\t\td\te\t' refs=' id-1   \t  id-2\t\t'  \n"
        "     ents=' ent-1   \t\r\n"
        "            ent-2  ' >\n"
        "  <e id='id-1'/>\n"
        "  <e id='id-2'/>\n"
        "</doc>";

    XML_SetStartElementHandler(parser,
                               check_attr_contains_normalized_whitespace);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST


/*
 * XML declaration tests.
 */

START_TEST(test_xmldecl_misplaced)
{
    expect_failure("\n"
                   "<?xml version='1.0'?>\n"
                   "<a/>",
                   XML_ERROR_MISPLACED_XML_PI,
                   "failed to report misplaced XML declaration");
}
END_TEST

/* Regression test for SF bug #584832. */
static int XMLCALL
UnknownEncodingHandler(void *UNUSED_P(data),const XML_Char *encoding,XML_Encoding *info)
{
    if (strcmp(encoding,"unsupported-encoding") == 0) {
        int i;
        for (i = 0; i < 256; ++i)
            info->map[i] = i;
        info->data = NULL;
        info->convert = NULL;
        info->release = NULL;
        return XML_STATUS_OK;
    }
    return XML_STATUS_ERROR;
}

START_TEST(test_unknown_encoding_internal_entity)
{
    const char *text =
        "<?xml version='1.0' encoding='unsupported-encoding'?>\n"
        "<!DOCTYPE test [<!ENTITY foo 'bar'>]>\n"
        "<test a='&foo;'/>";

    XML_SetUnknownEncodingHandler(parser, UnknownEncodingHandler, NULL);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

/* Test unrecognised encoding handler */
static void dummy_release(void *UNUSED_P(data))
{
}

/* Regression test for SF bug #620106. */
static int XMLCALL
UnrecognisedEncodingHandler(void *UNUSED_P(data),
                            const XML_Char *UNUSED_P(encoding),
                            XML_Encoding *info)
{
    info->data = NULL;
    info->convert = NULL;
    info->release = dummy_release;
    return XML_STATUS_ERROR;
}

START_TEST(test_unrecognised_encoding_internal_entity)
{
    const char *text =
        "<?xml version='1.0' encoding='unsupported-encoding'?>\n"
        "<!DOCTYPE test [<!ENTITY foo 'bar'>]>\n"
        "<test a='&foo;'/>";

    XML_SetUnknownEncodingHandler(parser,
                                  UnrecognisedEncodingHandler,
                                  NULL);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) != XML_STATUS_ERROR)
        fail("Unrecognised encoding not rejected");
}
END_TEST

/* Regression test for SF bug #620106. */
static int XMLCALL
external_entity_loader(XML_Parser parser,
                       const XML_Char *context,
                       const XML_Char *UNUSED_P(base),
                       const XML_Char *UNUSED_P(systemId),
                       const XML_Char *UNUSED_P(publicId))
{
    ExtTest *test_data = (ExtTest *)XML_GetUserData(parser);
    XML_Parser extparser;

    extparser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (extparser == NULL)
        fail("Could not create external entity parser.");
    if (test_data->encoding != NULL) {
        if (!XML_SetEncoding(extparser, test_data->encoding))
            fail("XML_SetEncoding() ignored for external entity");
    }
    if ( _XML_Parse_SINGLE_BYTES(extparser,
                                 test_data->parse_text,
                                 strlen(test_data->parse_text),
                                 XML_TRUE)
          == XML_STATUS_ERROR) {
        xml_failure(extparser);
    }
    return XML_STATUS_OK;
}

START_TEST(test_ext_entity_set_encoding)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
        "]>\n"
        "<doc>&en;</doc>";
    ExtTest test_data = {
        /* This text says it's an unsupported encoding, but it's really
           UTF-8, which we tell Expat using XML_SetEncoding().
        */
        "<?xml encoding='iso-8859-3'?>\xC3\xA9",
        "utf-8",
        NULL
    };

    XML_SetExternalEntityRefHandler(parser, external_entity_loader);
    run_ext_character_check(text, &test_data, "\xC3\xA9");
}
END_TEST

/* Test external entities with no handler */
START_TEST(test_ext_entity_no_handler)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
        "]>\n"
        "<doc>&en;</doc>";

    XML_SetDefaultHandler(parser, dummy_default_handler);
    run_character_check(text, "");
}
END_TEST

/* Test UTF-8 BOM is accepted */
START_TEST(test_ext_entity_set_bom)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
        "]>\n"
        "<doc>&en;</doc>";
    ExtTest test_data = {
        "\xEF\xBB\xBF" /* BOM */
        "<?xml encoding='iso-8859-3'?>"
        "\xC3\xA9",
        "utf-8",
        NULL
    };

    XML_SetExternalEntityRefHandler(parser, external_entity_loader);
    run_ext_character_check(text, &test_data, "\xC3\xA9");
}
END_TEST


/* Test that bad encodings are faulted */
typedef struct ext_faults
{
    const char *parse_text;
    const char *fail_text;
    const char *encoding;
    enum XML_Error error;
} ExtFaults;

static int XMLCALL
external_entity_faulter(XML_Parser parser,
                        const XML_Char *context,
                        const XML_Char *UNUSED_P(base),
                        const XML_Char *UNUSED_P(systemId),
                        const XML_Char *UNUSED_P(publicId))
{
    XML_Parser ext_parser;
    ExtFaults *fault = (ExtFaults *)XML_GetUserData(parser);

    ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (ext_parser == NULL)
        fail("Could not create external entity parser");
    if (fault->encoding != NULL) {
        if (!XML_SetEncoding(ext_parser, fault->encoding))
            fail("XML_SetEncoding failed");
    }
    if (_XML_Parse_SINGLE_BYTES(ext_parser,
                                fault->parse_text,
                                strlen(fault->parse_text),
                                XML_TRUE) != XML_STATUS_ERROR)
        fail(fault->fail_text);
    if (XML_GetErrorCode(ext_parser) != fault->error)
        xml_failure(ext_parser);
    return XML_STATUS_ERROR;
}

START_TEST(test_ext_entity_bad_encoding)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
        "]>\n"
        "<doc>&en;</doc>";
    ExtFaults fault = {
        "<?xml encoding='iso-8859-3'?>u",
        "Unsupported encoding not faulted",
        "unknown",
        XML_ERROR_UNKNOWN_ENCODING
    };

    XML_SetExternalEntityRefHandler(parser, external_entity_faulter);
    XML_SetUserData(parser, &fault);
    expect_failure(text,
                   XML_ERROR_EXTERNAL_ENTITY_HANDLING,
                   "Bad encoding should not have been accepted");
}
END_TEST

/* Try handing an invalid encoding to an external entity parser */
START_TEST(test_ext_entity_bad_encoding_2)
{
    const char *text =
        "<?xml version='1.0' encoding='us-ascii'?>\n"
        "<!DOCTYPE doc SYSTEM 'foo'>\n"
        "<doc>&entity;</doc>";
    ExtFaults fault = {
        "<!ELEMENT doc (#PCDATA)*>",
        "Unknown encoding not faulted",
        "unknown-encoding",
        XML_ERROR_UNKNOWN_ENCODING
    };

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(parser, external_entity_faulter);
    XML_SetUserData(parser, &fault);
    expect_failure(text, XML_ERROR_EXTERNAL_ENTITY_HANDLING,
                   "Bad encoding not faulted in external entity handler");
}
END_TEST

/* Test that no error is reported for unknown entities if we don't
   read an external subset.  This was fixed in Expat 1.95.5.
*/
START_TEST(test_wfc_undeclared_entity_unread_external_subset) {
    const char *text =
        "<!DOCTYPE doc SYSTEM 'foo'>\n"
        "<doc>&entity;</doc>";

    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

/* Test that an error is reported for unknown entities if we don't
   have an external subset.
*/
START_TEST(test_wfc_undeclared_entity_no_external_subset) {
    expect_failure("<doc>&entity;</doc>",
                   XML_ERROR_UNDEFINED_ENTITY,
                   "Parser did not report undefined entity w/out a DTD.");
}
END_TEST

/* Test that an error is reported for unknown entities if we don't
   read an external subset, but have been declared standalone.
*/
START_TEST(test_wfc_undeclared_entity_standalone) {
    const char *text =
        "<?xml version='1.0' encoding='us-ascii' standalone='yes'?>\n"
        "<!DOCTYPE doc SYSTEM 'foo'>\n"
        "<doc>&entity;</doc>";

    expect_failure(text,
                   XML_ERROR_UNDEFINED_ENTITY,
                   "Parser did not report undefined entity (standalone).");
}
END_TEST

/* Test that an error is reported for unknown entities if we have read
   an external subset, and standalone is true.
*/
START_TEST(test_wfc_undeclared_entity_with_external_subset_standalone) {
    const char *text =
        "<?xml version='1.0' encoding='us-ascii' standalone='yes'?>\n"
        "<!DOCTYPE doc SYSTEM 'foo'>\n"
        "<doc>&entity;</doc>";
    ExtTest test_data = {
        "<!ELEMENT doc (#PCDATA)*>",
        NULL,
        NULL
    };

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetUserData(parser, &test_data);
    XML_SetExternalEntityRefHandler(parser, external_entity_loader);
    expect_failure(text,
                   XML_ERROR_UNDEFINED_ENTITY,
                   "Parser did not report undefined entity (external DTD).");
}
END_TEST

/* Test that external entity handling is not done if the parsing flag
 * is set to UNLESS_STANDALONE
 */
START_TEST(test_entity_with_external_subset_unless_standalone) {
    const char *text =
        "<?xml version='1.0' encoding='us-ascii' standalone='yes'?>\n"
        "<!DOCTYPE doc SYSTEM 'foo'>\n"
        "<doc>&entity;</doc>";
    ExtTest test_data = { "<!ENTITY entity 'bar'>", NULL, NULL };

    XML_SetParamEntityParsing(parser,
                              XML_PARAM_ENTITY_PARSING_UNLESS_STANDALONE);
    XML_SetUserData(parser, &test_data);
    XML_SetExternalEntityRefHandler(parser, external_entity_loader);
    expect_failure(text,
                   XML_ERROR_UNDEFINED_ENTITY,
                   "Parser did not report undefined entity");
}
END_TEST

/* Test that no error is reported for unknown entities if we have read
   an external subset, and standalone is false.
*/
START_TEST(test_wfc_undeclared_entity_with_external_subset) {
    const char *text =
        "<?xml version='1.0' encoding='us-ascii'?>\n"
        "<!DOCTYPE doc SYSTEM 'foo'>\n"
        "<doc>&entity;</doc>";
    ExtTest test_data = {
        "<!ELEMENT doc (#PCDATA)*>",
        NULL,
        NULL
    };

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(parser, external_entity_loader);
    run_ext_character_check(text, &test_data, "");
}
END_TEST

/* Test that an error is reported if our NotStandalone handler fails */
static int XMLCALL
reject_not_standalone_handler(void *UNUSED_P(userData))
{
    return XML_STATUS_ERROR;
}

START_TEST(test_not_standalone_handler_reject)
{
    const char *text =
        "<?xml version='1.0' encoding='us-ascii'?>\n"
        "<!DOCTYPE doc SYSTEM 'foo'>\n"
        "<doc>&entity;</doc>";
    ExtTest test_data = {
        "<!ELEMENT doc (#PCDATA)*>",
        NULL,
        NULL
    };

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetUserData(parser, &test_data);
    XML_SetExternalEntityRefHandler(parser, external_entity_loader);
    XML_SetNotStandaloneHandler(parser, reject_not_standalone_handler);
    expect_failure(text, XML_ERROR_NOT_STANDALONE,
                   "NotStandalone handler failed to reject");

    /* Try again but without external entity handling */
    XML_ParserReset(parser, NULL);
    XML_SetNotStandaloneHandler(parser, reject_not_standalone_handler);
    expect_failure(text, XML_ERROR_NOT_STANDALONE,
                   "NotStandalone handler failed to reject");
}
END_TEST

/* Test that no error is reported if our NotStandalone handler succeeds */
static int XMLCALL
accept_not_standalone_handler(void *UNUSED_P(userData))
{
    return XML_STATUS_OK;
}

START_TEST(test_not_standalone_handler_accept)
{
    const char *text =
        "<?xml version='1.0' encoding='us-ascii'?>\n"
        "<!DOCTYPE doc SYSTEM 'foo'>\n"
        "<doc>&entity;</doc>";
    ExtTest test_data = {
        "<!ELEMENT doc (#PCDATA)*>",
        NULL,
        NULL
    };

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(parser, external_entity_loader);
    XML_SetNotStandaloneHandler(parser, accept_not_standalone_handler);
    run_ext_character_check(text, &test_data, "");

    /* Repeat wtihout the external entity handler */
    XML_ParserReset(parser, NULL);
    XML_SetNotStandaloneHandler(parser, accept_not_standalone_handler);
    run_character_check(text, "");
}
END_TEST

START_TEST(test_wfc_no_recursive_entity_refs)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "  <!ENTITY entity '&#38;entity;'>\n"
        "]>\n"
        "<doc>&entity;</doc>";

    expect_failure(text,
                   XML_ERROR_RECURSIVE_ENTITY_REF,
                   "Parser did not report recursive entity reference.");
}
END_TEST

/* Test incomplete external entities are faulted */
START_TEST(test_ext_entity_invalid_parse)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
        "]>\n"
        "<doc>&en;</doc>";
    ExtFaults faults[] = {
        {
            "<",
            "Incomplete element declaration not faulted",
            NULL,
            XML_ERROR_UNCLOSED_TOKEN
        },
        {
            "<\xe2\x82", /* First two bytes of a three-byte char */
            "Incomplete character not faulted",
            NULL,
            XML_ERROR_PARTIAL_CHAR
        },
        {
            "<tag>\xe2\x82",
            "Incomplete character in CDATA not faulted",
            NULL,
            XML_ERROR_PARTIAL_CHAR
        },
        { NULL, NULL, NULL, XML_ERROR_NONE }
    };
    ExtFaults *fault;

    for (fault = &faults[0]; fault->parse_text != NULL; fault++) {
        XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
        XML_SetExternalEntityRefHandler(parser, external_entity_faulter);
        XML_SetUserData(parser, fault);
        expect_failure(text,
                       XML_ERROR_EXTERNAL_ENTITY_HANDLING,
                       "Parser did not report external entity error");
        XML_ParserReset(parser, NULL);
    }
}
END_TEST


/* Regression test for SF bug #483514. */
START_TEST(test_dtd_default_handling)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "<!ENTITY e SYSTEM 'http://example.org/e'>\n"
        "<!NOTATION n SYSTEM 'http://example.org/n'>\n"
        "<!ELEMENT doc EMPTY>\n"
        "<!ATTLIST doc a CDATA #IMPLIED>\n"
        "<?pi in dtd?>\n"
        "<!--comment in dtd-->\n"
        "]><doc/>";

    XML_SetDefaultHandler(parser, accumulate_characters);
    XML_SetStartDoctypeDeclHandler(parser, dummy_start_doctype_handler);
    XML_SetEndDoctypeDeclHandler(parser, dummy_end_doctype_handler);
    XML_SetEntityDeclHandler(parser, dummy_entity_decl_handler);
    XML_SetNotationDeclHandler(parser, dummy_notation_decl_handler);
    XML_SetElementDeclHandler(parser, dummy_element_decl_handler);
    XML_SetAttlistDeclHandler(parser, dummy_attlist_decl_handler);
    XML_SetProcessingInstructionHandler(parser, dummy_pi_handler);
    XML_SetCommentHandler(parser, dummy_comment_handler);
    XML_SetStartCdataSectionHandler(parser, dummy_start_cdata_handler);
    XML_SetEndCdataSectionHandler(parser, dummy_end_cdata_handler);
    run_character_check(text, "\n\n\n\n\n\n\n<doc/>");
}
END_TEST

/* Test handling of attribute declarations */
typedef struct AttTest {
    const XML_Char *definition;
    const XML_Char *element_name;
    const XML_Char *attr_name;
    const XML_Char *attr_type;
    const XML_Char *default_value;
    int is_required;
} AttTest;

static void XMLCALL
verify_attlist_decl_handler(void *userData,
                            const XML_Char *element_name,
                            const XML_Char *attr_name,
                            const XML_Char *attr_type,
                            const XML_Char *default_value,
                            int is_required)
{
    AttTest *at = (AttTest *)userData;

    if (strcmp(element_name, at->element_name))
        fail("Unexpected element name in attribute declaration");
    if (strcmp(attr_name, at->attr_name))
        fail("Unexpected attribute name in attribute declaration");
    if (strcmp(attr_type, at->attr_type))
        fail("Unexpected attribute type in attribute declaration");
    if ((default_value == NULL && at->default_value != NULL) ||
        (default_value != NULL && at->default_value == NULL) ||
        (default_value != NULL && strcmp(default_value, at->default_value)))
        fail("Unexpected default value in attribute declaration");
    if (is_required != at->is_required)
        fail("Requirement mismatch in attribute declaration");
}

START_TEST(test_dtd_attr_handling)
{
    const char *prolog =
        "<!DOCTYPE doc [\n"
        "<!ELEMENT doc EMPTY>\n";
    AttTest attr_data[] = {
        {
            "<!ATTLIST doc a ( one | two | three ) #REQUIRED>\n"
            "]>"
            "<doc a='two'/>",
            "doc",
            "a",
            "(one|two|three)", /* Extraneous spaces will be removed */
            NULL,
            XML_TRUE
        },
        {
            "<!NOTATION foo SYSTEM 'http://example.org/foo'>\n"
            "<!ATTLIST doc a NOTATION (foo) #IMPLIED>\n"
            "]>"
            "<doc/>",
            "doc",
            "a",
            "NOTATION(foo)",
            NULL,
            XML_FALSE
        },
        {
            "<!ATTLIST doc a NOTATION (foo) 'bar'>\n"
            "]>"
            "<doc/>",
            "doc",
            "a",
            "NOTATION(foo)",
            "bar",
            XML_FALSE
        },
        { NULL, NULL, NULL, NULL, NULL, XML_FALSE }
    };
    AttTest *test;

    for (test = attr_data; test->definition != NULL; test++) {
        XML_SetAttlistDeclHandler(parser, verify_attlist_decl_handler);
        XML_SetUserData(parser, test);
        if (_XML_Parse_SINGLE_BYTES(parser, prolog, strlen(prolog),
                                    XML_FALSE) == XML_STATUS_ERROR)
            xml_failure(parser);
        if (_XML_Parse_SINGLE_BYTES(parser,
                                    test->definition,
                                    strlen(test->definition),
                                    XML_TRUE) == XML_STATUS_ERROR)
            xml_failure(parser);
        XML_ParserReset(parser, NULL);
    }
}
END_TEST

/* See related SF bug #673791.
   When namespace processing is enabled, setting the namespace URI for
   a prefix is not allowed; this test ensures that it *is* allowed
   when namespace processing is not enabled.
   (See Namespaces in XML, section 2.)
*/
START_TEST(test_empty_ns_without_namespaces)
{
    const char *text =
        "<doc xmlns:prefix='http://example.org/'>\n"
        "  <e xmlns:prefix=''/>\n"
        "</doc>";

    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

/* Regression test for SF bug #824420.
   Checks that an xmlns:prefix attribute set in an attribute's default
   value isn't misinterpreted.
*/
START_TEST(test_ns_in_attribute_default_without_namespaces)
{
    const char *text =
        "<!DOCTYPE e:element [\n"
        "  <!ATTLIST e:element\n"
        "    xmlns:e CDATA 'http://example.org/'>\n"
        "      ]>\n"
        "<e:element/>";

    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

static const char *long_character_data_text =
    "<?xml version='1.0' encoding='iso-8859-1'?><s>"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "</s>";

static XML_Bool resumable = XML_FALSE;

static void
clearing_aborting_character_handler(void *UNUSED_P(userData),
                                    const XML_Char *UNUSED_P(s), int UNUSED_P(len))
{
    XML_StopParser(parser, resumable);
    XML_SetCharacterDataHandler(parser, NULL);
}

/* Regression test for SF bug #1515266: missing check of stopped
   parser in doContext() 'for' loop. */
START_TEST(test_stop_parser_between_char_data_calls)
{
    /* The sample data must be big enough that there are two calls to
       the character data handler from within the inner "for" loop of
       the XML_TOK_DATA_CHARS case in doContent(), and the character
       handler must stop the parser and clear the character data
       handler.
    */
    const char *text = long_character_data_text;

    XML_SetCharacterDataHandler(parser, clearing_aborting_character_handler);
    resumable = XML_FALSE;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) != XML_STATUS_ERROR)
        xml_failure(parser);
    if (XML_GetErrorCode(parser) != XML_ERROR_ABORTED)
        xml_failure(parser);
}
END_TEST

/* Regression test for SF bug #1515266: missing check of stopped
   parser in doContext() 'for' loop. */
START_TEST(test_suspend_parser_between_char_data_calls)
{
    /* The sample data must be big enough that there are two calls to
       the character data handler from within the inner "for" loop of
       the XML_TOK_DATA_CHARS case in doContent(), and the character
       handler must stop the parser and clear the character data
       handler.
    */
    const char *text = long_character_data_text;

    XML_SetCharacterDataHandler(parser, clearing_aborting_character_handler);
    resumable = XML_TRUE;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) != XML_STATUS_SUSPENDED)
        xml_failure(parser);
    if (XML_GetErrorCode(parser) != XML_ERROR_NONE)
        xml_failure(parser);
    /* Try parsing directly */
    if (XML_Parse(parser, text, strlen(text), XML_TRUE) != XML_STATUS_ERROR)
        fail("Attempt to continue parse while suspended not faulted");
    if (XML_GetErrorCode(parser) != XML_ERROR_SUSPENDED)
        fail("Suspended parse not faulted with correct error");
}
END_TEST


static XML_Bool abortable = XML_FALSE;

static void
parser_stop_character_handler(void *UNUSED_P(userData),
                              const XML_Char *UNUSED_P(s),
                              int UNUSED_P(len))
{
    XML_StopParser(parser, resumable);
    XML_SetCharacterDataHandler(parser, NULL);
    if (!resumable) {
        /* Check that aborting an aborted parser is faulted */
        if (XML_StopParser(parser, XML_FALSE) != XML_STATUS_ERROR)
            fail("Aborting aborted parser not faulted");
        if (XML_GetErrorCode(parser) != XML_ERROR_FINISHED)
            xml_failure(parser);
    } else if (abortable) {
        /* Check that aborting a suspended parser works */
        if (XML_StopParser(parser, XML_FALSE) == XML_STATUS_ERROR)
            xml_failure(parser);
    } else {
        /* Check that suspending a suspended parser works */
        if (XML_StopParser(parser, XML_TRUE) != XML_STATUS_ERROR)
            fail("Suspending suspended parser not faulted");
        if (XML_GetErrorCode(parser) != XML_ERROR_SUSPENDED)
            xml_failure(parser);
    }
}

/* Test repeated calls to XML_StopParser are handled correctly */
START_TEST(test_repeated_stop_parser_between_char_data_calls)
{
    const char *text = long_character_data_text;

    XML_SetCharacterDataHandler(parser, parser_stop_character_handler);
    resumable = XML_FALSE;
    abortable = XML_FALSE;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) != XML_STATUS_ERROR)
        fail("Failed to double-stop parser");

    XML_ParserReset(parser, NULL);
    XML_SetCharacterDataHandler(parser, parser_stop_character_handler);
    resumable = XML_TRUE;
    abortable = XML_FALSE;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) != XML_STATUS_SUSPENDED)
        fail("Failed to double-suspend parser");

    XML_ParserReset(parser, NULL);
    XML_SetCharacterDataHandler(parser, parser_stop_character_handler);
    resumable = XML_TRUE;
    abortable = XML_TRUE;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) != XML_STATUS_ERROR)
        fail("Failed to suspend-abort parser");
}
END_TEST


START_TEST(test_good_cdata_ascii)
{
    const char *text = "<a><![CDATA[<greeting>Hello, world!</greeting>]]></a>";
    const char *expected = "<greeting>Hello, world!</greeting>";

    CharData storage;
    CharData_Init(&storage);
    XML_SetUserData(parser, &storage);
    XML_SetCharacterDataHandler(parser, accumulate_characters);
    /* Add start and end handlers for coverage */
    XML_SetStartCdataSectionHandler(parser, dummy_start_cdata_handler);
    XML_SetEndCdataSectionHandler(parser, dummy_end_cdata_handler);

    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    CharData_CheckXMLChars(&storage, expected);

    /* Try again, this time with a default handler */
    XML_ParserReset(parser, NULL);
    CharData_Init(&storage);
    XML_SetUserData(parser, &storage);
    XML_SetCharacterDataHandler(parser, accumulate_characters);
    XML_SetDefaultHandler(parser, dummy_default_handler);

    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    CharData_CheckXMLChars(&storage, expected);
}
END_TEST

START_TEST(test_good_cdata_utf16)
{
    /* Test data is:
     *   <?xml version='1.0' encoding='utf-16'?>
     *   <a><![CDATA[hello]]></a>
     */
    const char text[] =
            "\0<\0?\0x\0m\0l\0"
                " \0v\0e\0r\0s\0i\0o\0n\0=\0'\0\x31\0.\0\x30\0'\0"
                " \0e\0n\0c\0o\0d\0i\0n\0g\0=\0'\0u\0t\0f\0-\0""1\0""6\0'"
                "\0?\0>\0\n"
            "\0<\0a\0>\0<\0!\0[\0C\0D\0A\0T\0A\0[\0h\0e\0l\0l\0o\0]\0]\0>\0<\0/\0a\0>";
    const char *expected = "hello";

    CharData storage;
    CharData_Init(&storage);
    XML_SetUserData(parser, &storage);
    XML_SetCharacterDataHandler(parser, accumulate_characters);

    if (_XML_Parse_SINGLE_BYTES(parser, text, sizeof(text) - 1, XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    CharData_CheckXMLChars(&storage, expected);
}
END_TEST

/* Test UTF16 conversion of a long cdata string */

/* 16 characters: handy macro to reduce visual clutter */
#define A_TO_P_IN_UTF16 "\0A\0B\0C\0D\0E\0F\0G\0H\0I\0J\0K\0L\0M\0N\0O\0P"

START_TEST(test_long_cdata_utf16)
{
    /* Test data is:
     * <?xlm version='1.0' encoding='utf-16'?>
     * <a><![CDATA[
     * ABCDEFGHIJKLMNOP
     * ]]></a>
     */
    const char text[] =
        "\0<\0?\0x\0m\0l\0 "
        "\0v\0e\0r\0s\0i\0o\0n\0=\0'\0\x31\0.\0\x30\0'\0 "
        "\0e\0n\0c\0o\0d\0i\0n\0g\0=\0'\0u\0t\0f\0-\0\x31\0\x36\0'\0?\0>"
        "\0<\0a\0>\0<\0!\0[\0C\0D\0A\0T\0A\0["
        /* 64 characters per line */
        A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16
        A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16
        A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16
        A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16
        A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16
        A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16
        A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16
        A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16
        A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16
        A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16
        A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16
        A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16
        A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16
        A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16
        A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16
        A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16  A_TO_P_IN_UTF16
        A_TO_P_IN_UTF16
        "\0]\0]\0>\0<\0/\0a\0>";
    const char *expected =
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOP";
    CharData storage;
    void *buffer;

    CharData_Init(&storage);
    XML_SetUserData(parser, &storage);
    XML_SetCharacterDataHandler(parser, accumulate_characters);
    buffer = XML_GetBuffer(parser, sizeof(text) - 1);
    if (buffer == NULL)
        fail("Could not allocate parse buffer");
    memcpy(buffer, text, sizeof(text) - 1);
    if (XML_ParseBuffer(parser,
                        sizeof(text) - 1,
                        XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    CharData_CheckXMLChars(&storage, expected);
}
END_TEST

/* Test handling of multiple unit UTF-16 characters */
START_TEST(test_multichar_cdata_utf16)
{
    /* Test data is:
     *   <?xml version='1.0' encoding='utf-16'?>
     *   <a><![CDATA[{MINIM}{CROTCHET}]]></a>
     *
     * where {MINIM} is U+1d15e (a minim or half-note)
     *   UTF-16: 0xd834 0xdd5e
     *   UTF-8:  0xf0 0x9d 0x85 0x9e
     * and {CROTCHET} is U+1d15f (a crotchet or quarter-note)
     *   UTF-16: 0xd834 0xdd5e
     *   UTF-8:  0xf0 0x9d 0x85 0x9e
     */
    const char text[] =
        "\0<\0?\0x\0m\0l\0"
        " \0v\0e\0r\0s\0i\0o\0n\0=\0'\0\x31\0.\0\x30\0'\0"
        " \0e\0n\0c\0o\0d\0i\0n\0g\0=\0'\0u\0t\0f\0-\0""1\0""6\0'"
        "\0?\0>\0\n"
        "\0<\0a\0>\0<\0!\0[\0C\0D\0A\0T\0A\0["
        "\xd8\x34\xdd\x5e\xd8\x34\xdd\x5f"
        "\0]\0]\0>\0<\0/\0a\0>";
    const char *expected = "\xf0\x9d\x85\x9e\xf0\x9d\x85\x9f";
    CharData storage;

    CharData_Init(&storage);
    XML_SetUserData(parser, &storage);
    XML_SetCharacterDataHandler(parser, accumulate_characters);

    if (_XML_Parse_SINGLE_BYTES(parser, text, sizeof(text) - 1, XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    CharData_CheckXMLChars(&storage, expected);
}
END_TEST


START_TEST(test_bad_cdata)
{
    struct CaseData {
        const char *text;
        enum XML_Error expectedError;
    };

    struct CaseData cases[] = {
        {"<a><", XML_ERROR_UNCLOSED_TOKEN},
        {"<a><!", XML_ERROR_UNCLOSED_TOKEN},
        {"<a><![", XML_ERROR_UNCLOSED_TOKEN},
        {"<a><![C", XML_ERROR_UNCLOSED_TOKEN},
        {"<a><![CD", XML_ERROR_UNCLOSED_TOKEN},
        {"<a><![CDA", XML_ERROR_UNCLOSED_TOKEN},
        {"<a><![CDAT", XML_ERROR_UNCLOSED_TOKEN},
        {"<a><![CDATA", XML_ERROR_UNCLOSED_TOKEN},

        {"<a><![CDATA[", XML_ERROR_UNCLOSED_CDATA_SECTION},
        {"<a><![CDATA[]", XML_ERROR_UNCLOSED_CDATA_SECTION},
        {"<a><![CDATA[]]", XML_ERROR_UNCLOSED_CDATA_SECTION},

        {"<a><!<a/>", XML_ERROR_INVALID_TOKEN},
        {"<a><![<a/>", XML_ERROR_UNCLOSED_TOKEN}, /* ?! */
        {"<a><![C<a/>", XML_ERROR_UNCLOSED_TOKEN}, /* ?! */
        {"<a><![CD<a/>", XML_ERROR_INVALID_TOKEN},
        {"<a><![CDA<a/>", XML_ERROR_INVALID_TOKEN},
        {"<a><![CDAT<a/>", XML_ERROR_INVALID_TOKEN},
        {"<a><![CDATA<a/>", XML_ERROR_INVALID_TOKEN},

        {"<a><![CDATA[<a/>", XML_ERROR_UNCLOSED_CDATA_SECTION},
        {"<a><![CDATA[]<a/>", XML_ERROR_UNCLOSED_CDATA_SECTION},
        {"<a><![CDATA[]]<a/>", XML_ERROR_UNCLOSED_CDATA_SECTION}
    };

    size_t i = 0;
    for (; i < sizeof(cases) / sizeof(struct CaseData); i++) {
        const enum XML_Status actualStatus = _XML_Parse_SINGLE_BYTES(
                parser, cases[i].text, strlen(cases[i].text), XML_TRUE);
        const enum XML_Error actualError = XML_GetErrorCode(parser);

        assert(actualStatus == XML_STATUS_ERROR);

        if (actualError != cases[i].expectedError) {
            char message[100];
            sprintf(message, "Expected error %d but got error %d for case %u: \"%s\"\n",
                    cases[i].expectedError, actualError, (unsigned int)i + 1, cases[i].text);
            fail(message);
        }

        XML_ParserReset(parser, NULL);
    }
}
END_TEST

/* Test failures in UTF-16 CDATA */
START_TEST(test_bad_cdata_utf16)
{
    struct CaseData {
        size_t text_bytes;
        const char *text;
        enum XML_Error expected_error;
    };

    const char prolog[] =
        "\0<\0?\0x\0m\0l\0"
        " \0v\0e\0r\0s\0i\0o\0n\0=\0'\0\x31\0.\0\x30\0'\0"
        " \0e\0n\0c\0o\0d\0i\0n\0g\0=\0'\0u\0t\0f\0-\0""1\0""6\0'"
        "\0?\0>\0\n"
        "\0<\0a\0>";
    struct CaseData cases[] = {
        {1, "\0", XML_ERROR_UNCLOSED_TOKEN},
        {2, "\0<", XML_ERROR_UNCLOSED_TOKEN},
        {3, "\0<\0", XML_ERROR_UNCLOSED_TOKEN},
        {4, "\0<\0!", XML_ERROR_UNCLOSED_TOKEN},
        {5, "\0<\0!\0", XML_ERROR_UNCLOSED_TOKEN},
        {6, "\0<\0!\0[", XML_ERROR_UNCLOSED_TOKEN},
        {7, "\0<\0!\0[\0", XML_ERROR_UNCLOSED_TOKEN},
        {8, "\0<\0!\0[\0C", XML_ERROR_UNCLOSED_TOKEN},
        {9, "\0<\0!\0[\0C\0", XML_ERROR_UNCLOSED_TOKEN},
        {10, "\0<\0!\0[\0C\0D", XML_ERROR_UNCLOSED_TOKEN},
        {11, "\0<\0!\0[\0C\0D\0", XML_ERROR_UNCLOSED_TOKEN},
        {12, "\0<\0!\0[\0C\0D\0A", XML_ERROR_UNCLOSED_TOKEN},
        {13, "\0<\0!\0[\0C\0D\0A\0", XML_ERROR_UNCLOSED_TOKEN},
        {14, "\0<\0!\0[\0C\0D\0A\0T", XML_ERROR_UNCLOSED_TOKEN},
        {15, "\0<\0!\0[\0C\0D\0A\0T\0", XML_ERROR_UNCLOSED_TOKEN},
        {16, "\0<\0!\0[\0C\0D\0A\0T\0A", XML_ERROR_UNCLOSED_TOKEN},
        {17, "\0<\0!\0[\0C\0D\0A\0T\0A\0", XML_ERROR_UNCLOSED_TOKEN},
        {18, "\0<\0!\0[\0C\0D\0A\0T\0A\0[",
         XML_ERROR_UNCLOSED_CDATA_SECTION},
        {19, "\0<\0!\0[\0C\0D\0A\0T\0A\0[\0",
         XML_ERROR_UNCLOSED_CDATA_SECTION},
        {20, "\0<\0!\0[\0C\0D\0A\0T\0A\0[\0Z",
         XML_ERROR_UNCLOSED_CDATA_SECTION},
        /* Now add a four-byte UTF-16 character */
        {21, "\0<\0!\0[\0C\0D\0A\0T\0A\0[\0Z\xd8",
         XML_ERROR_UNCLOSED_CDATA_SECTION},
        {22, "\0<\0!\0[\0C\0D\0A\0T\0A\0[\0Z\xd8\x34",
         XML_ERROR_PARTIAL_CHAR},
        {23, "\0<\0!\0[\0C\0D\0A\0T\0A\0[\0Z\xd8\x34\xdd",
         XML_ERROR_PARTIAL_CHAR},
        {24, "\0<\0!\0[\0C\0D\0A\0T\0A\0[\0Z\xd8\x34\xdd\x5e",
         XML_ERROR_UNCLOSED_CDATA_SECTION}
    };
    size_t i;

    for (i = 0; i < sizeof(cases)/sizeof(struct CaseData); i++) {
        enum XML_Status actual_status;
        enum XML_Error actual_error;

        if (_XML_Parse_SINGLE_BYTES(parser, prolog, sizeof(prolog)-1,
                                    XML_FALSE) == XML_STATUS_ERROR)
            xml_failure(parser);
        actual_status = _XML_Parse_SINGLE_BYTES(parser,
                                                cases[i].text,
                                                cases[i].text_bytes,
                                                XML_TRUE);
        assert(actual_status == XML_STATUS_ERROR);
        actual_error = XML_GetErrorCode(parser);
        if (actual_error != cases[i].expected_error) {
            char message[1024];

            sprintf(message,
                    "Expected error %d (%s), got %d (%s) for case %lu\n",
                    cases[i].expected_error,
                    XML_ErrorString(cases[i].expected_error),
                    actual_error,
                    XML_ErrorString(actual_error),
                    i+1);
            fail(message);
        }
        XML_ParserReset(parser, NULL);
    }
}
END_TEST

static const char *long_cdata_text =
    "<s><![CDATA["
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "]]></s>";

/* Test stopping the parser in cdata handler */
START_TEST(test_stop_parser_between_cdata_calls)
{
    const char *text = long_cdata_text;

    XML_SetCharacterDataHandler(parser,
                                clearing_aborting_character_handler);
    resumable = XML_FALSE;
    expect_failure(text, XML_ERROR_ABORTED,
                   "Parse not aborted in CDATA handler");
}
END_TEST

/* Test suspending the parser in cdata handler */
START_TEST(test_suspend_parser_between_cdata_calls)
{
    const char *text = long_cdata_text;
    enum XML_Status result;

    XML_SetCharacterDataHandler(parser,
                                clearing_aborting_character_handler);
    resumable = XML_TRUE;
    result = _XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE);
    if (result != XML_STATUS_SUSPENDED) {
        if (result == XML_STATUS_ERROR)
            xml_failure(parser);
        fail("Parse not suspended in CDATA handler");
    }
    if (XML_GetErrorCode(parser) != XML_ERROR_NONE)
        xml_failure(parser);
}
END_TEST

/* Test memory allocation functions */
START_TEST(test_memory_allocation)
{
    char *buffer = (char *)XML_MemMalloc(parser, 256);
    char *p;

    if (buffer == NULL) {
        fail("Allocation failed");
    } else {
        /* Try writing to memory; some OSes try to cheat! */
        buffer[0] = 'T';
        buffer[1] = 'E';
        buffer[2] = 'S';
        buffer[3] = 'T';
        buffer[4] = '\0';
        if (strcmp(buffer, "TEST") != 0) {
            fail("Memory not writable");
        } else {
            p = (char *)XML_MemRealloc(parser, buffer, 512);
            if (p == NULL) {
                fail("Reallocation failed");
            } else {
                /* Write again, just to be sure */
                buffer = p;
                buffer[0] = 'V';
                if (strcmp(buffer, "VEST") != 0) {
                    fail("Reallocated memory not writable");
                }
            }
        }
        XML_MemFree(parser, buffer);
    }
}
END_TEST

static void XMLCALL
record_default_handler(void *userData,
                       const XML_Char *UNUSED_P(s),
                       int UNUSED_P(len))
{
    CharData_AppendString((CharData *)userData, "D");
}

static void XMLCALL
record_cdata_handler(void *userData,
                     const XML_Char *UNUSED_P(s),
                     int UNUSED_P(len))
{
    CharData_AppendString((CharData *)userData, "C");
    XML_DefaultCurrent(parser);
}

static void XMLCALL
record_cdata_nodefault_handler(void *userData,
                     const XML_Char *UNUSED_P(s),
                     int UNUSED_P(len))
{
    CharData_AppendString((CharData *)userData, "c");
}

static void XMLCALL
record_skip_handler(void *userData,
                    const XML_Char *UNUSED_P(entityName),
                    int is_parameter_entity)
{
    CharData_AppendString((CharData *)userData,
                          is_parameter_entity ? "E" : "e");
}

/* Test XML_DefaultCurrent() passes handling on correctly */
START_TEST(test_default_current)
{
    const char *text = "<doc>hell]</doc>";
    const char *entity_text =
        "<!DOCTYPE doc [\n"
        "<!ENTITY entity '&#37;'>\n"
        "]>\n"
        "<doc>&entity;</doc>";
    CharData storage;

    XML_SetDefaultHandler(parser, record_default_handler);
    XML_SetCharacterDataHandler(parser, record_cdata_handler);
    CharData_Init(&storage);
    XML_SetUserData(parser, &storage);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    CharData_CheckString(&storage, "DCDCDCDCDCDD");

    /* Again, without the defaulting */
    XML_ParserReset(parser, NULL);
    XML_SetDefaultHandler(parser, record_default_handler);
    XML_SetCharacterDataHandler(parser, record_cdata_nodefault_handler);
    CharData_Init(&storage);
    XML_SetUserData(parser, &storage);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    CharData_CheckString(&storage, "DcccccD");

    /* Now with an internal entity to complicate matters */
    XML_ParserReset(parser, NULL);
    XML_SetDefaultHandler(parser, record_default_handler);
    XML_SetCharacterDataHandler(parser, record_cdata_handler);
    CharData_Init(&storage);
    XML_SetUserData(parser, &storage);
    if (_XML_Parse_SINGLE_BYTES(parser, entity_text, strlen(entity_text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    /* The default handler suppresses the entity */
    CharData_CheckString(&storage, "DDDDDDDDDDDDDDDDDDD");

    /* Again, with a skip handler */
    XML_ParserReset(parser, NULL);
    XML_SetDefaultHandler(parser, record_default_handler);
    XML_SetCharacterDataHandler(parser, record_cdata_handler);
    XML_SetSkippedEntityHandler(parser, record_skip_handler);
    CharData_Init(&storage);
    XML_SetUserData(parser, &storage);
    if (_XML_Parse_SINGLE_BYTES(parser, entity_text, strlen(entity_text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    /* The default handler suppresses the entity */
    CharData_CheckString(&storage, "DDDDDDDDDDDDDDDDDeD");

    /* This time, allow the entity through */
    XML_ParserReset(parser, NULL);
    XML_SetDefaultHandlerExpand(parser, record_default_handler);
    XML_SetCharacterDataHandler(parser, record_cdata_handler);
    CharData_Init(&storage);
    XML_SetUserData(parser, &storage);
    if (_XML_Parse_SINGLE_BYTES(parser, entity_text, strlen(entity_text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    CharData_CheckString(&storage, "DDDDDDDDDDDDDDDDDCDD");

    /* Finally, without passing the cdata to the default handler */
    XML_ParserReset(parser, NULL);
    XML_SetDefaultHandlerExpand(parser, record_default_handler);
    XML_SetCharacterDataHandler(parser, record_cdata_nodefault_handler);
    CharData_Init(&storage);
    XML_SetUserData(parser, &storage);
    if (_XML_Parse_SINGLE_BYTES(parser, entity_text, strlen(entity_text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    CharData_CheckString(&storage, "DDDDDDDDDDDDDDDDDcD");
}
END_TEST

/* Test DTD element parsing code paths */
START_TEST(test_dtd_elements)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "<!ELEMENT doc (chapter)>\n"
        "<!ELEMENT chapter (#PCDATA)>\n"
        "]>\n"
        "<doc><chapter>Wombats are go</chapter></doc>";

    XML_SetElementDeclHandler(parser, dummy_element_decl_handler);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

/* Test foreign DTD handling */
START_TEST(test_set_foreign_dtd)
{
    const char *text1 =
        "<?xml version='1.0' encoding='us-ascii'?>\n";
    const char *text2 =
        "<doc>&entity;</doc>";
    ExtTest test_data = {
        "<!ELEMENT doc (#PCDATA)*>",
        NULL,
        NULL
    };

    /* Check hash salt is passed through too */
    XML_SetHashSalt(parser, 0x12345678);
    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetUserData(parser, &test_data);
    XML_SetExternalEntityRefHandler(parser, external_entity_loader);
    /* Add a default handler to exercise more code paths */
    XML_SetDefaultHandler(parser, dummy_default_handler);
    if (XML_UseForeignDTD(parser, XML_TRUE) != XML_ERROR_NONE)
        fail("Could not set foreign DTD");
    if (_XML_Parse_SINGLE_BYTES(parser, text1, strlen(text1),
                                XML_FALSE) == XML_STATUS_ERROR)
        xml_failure(parser);

    /* Ensure that trying to set the DTD after parsing has started
     * is faulted, even if it's the same setting.
     */
    if (XML_UseForeignDTD(parser, XML_TRUE) !=
        XML_ERROR_CANT_CHANGE_FEATURE_ONCE_PARSING)
        fail("Failed to reject late foreign DTD setting");
    /* Ditto for the hash salt */
    if (XML_SetHashSalt(parser, 0x23456789))
        fail("Failed to reject late hash salt change");

    /* Now finish the parse */
    if (_XML_Parse_SINGLE_BYTES(parser, text2, strlen(text2),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

/* Test foreign DTD handling with a failing NotStandalone handler */
START_TEST(test_foreign_dtd_not_standalone)
{
    const char *text =
        "<?xml version='1.0' encoding='us-ascii'?>\n"
        "<doc>&entity;</doc>";
    ExtTest test_data = {
        "<!ELEMENT doc (#PCDATA)*>",
        NULL,
        NULL
    };

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetUserData(parser, &test_data);
    XML_SetExternalEntityRefHandler(parser, external_entity_loader);
    XML_SetNotStandaloneHandler(parser, reject_not_standalone_handler);
    if (XML_UseForeignDTD(parser, XML_TRUE) != XML_ERROR_NONE)
        fail("Could not set foreign DTD");
    expect_failure(text, XML_ERROR_NOT_STANDALONE,
                   "NotStandalonehandler failed to reject");
}
END_TEST

/* Test invalid character in a foreign DTD is faulted */
START_TEST(test_invalid_foreign_dtd)
{
    const char *text =
        "<?xml version='1.0' encoding='us-ascii'?>\n"
        "<doc>&entity;</doc>";
    ExtFaults test_data = {
        "$",
        "Dollar not faulted",
        NULL,
        XML_ERROR_INVALID_TOKEN
    };

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetUserData(parser, &test_data);
    XML_SetExternalEntityRefHandler(parser, external_entity_faulter);
    XML_UseForeignDTD(parser, XML_TRUE);
    expect_failure(text,
                   XML_ERROR_EXTERNAL_ENTITY_HANDLING,
                   "Bad DTD should not have been accepted");
}
END_TEST

/* Test foreign DTD use with a doctype */
START_TEST(test_foreign_dtd_with_doctype)
{
    const char *text1 =
        "<?xml version='1.0' encoding='us-ascii'?>\n"
        "<!DOCTYPE doc [<!ENTITY entity 'hello world'>]>\n";
    const char *text2 =
        "<doc>&entity;</doc>";
    ExtTest test_data = {
        "<!ELEMENT doc (#PCDATA)*>",
        NULL,
        NULL
    };

    /* Check hash salt is passed through too */
    XML_SetHashSalt(parser, 0x12345678);
    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetUserData(parser, &test_data);
    XML_SetExternalEntityRefHandler(parser, external_entity_loader);
    /* Add a default handler to exercise more code paths */
    XML_SetDefaultHandler(parser, dummy_default_handler);
    if (XML_UseForeignDTD(parser, XML_TRUE) != XML_ERROR_NONE)
        fail("Could not set foreign DTD");
    if (_XML_Parse_SINGLE_BYTES(parser, text1, strlen(text1),
                                XML_FALSE) == XML_STATUS_ERROR)
        xml_failure(parser);

    /* Ensure that trying to set the DTD after parsing has started
     * is faulted, even if it's the same setting.
     */
    if (XML_UseForeignDTD(parser, XML_TRUE) !=
        XML_ERROR_CANT_CHANGE_FEATURE_ONCE_PARSING)
        fail("Failed to reject late foreign DTD setting");
    /* Ditto for the hash salt */
    if (XML_SetHashSalt(parser, 0x23456789))
        fail("Failed to reject late hash salt change");

    /* Now finish the parse */
    if (_XML_Parse_SINGLE_BYTES(parser, text2, strlen(text2),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

/* Test XML_UseForeignDTD with no external subset present */
static int XMLCALL
external_entity_null_loader(XML_Parser UNUSED_P(parser),
                            const XML_Char *UNUSED_P(context),
                            const XML_Char *UNUSED_P(base),
                            const XML_Char *UNUSED_P(systemId),
                            const XML_Char *UNUSED_P(publicId))
{
    return XML_STATUS_OK;
}

START_TEST(test_foreign_dtd_without_external_subset)
{
    const char *text =
        "<!DOCTYPE doc [<!ENTITY foo 'bar'>]>\n"
        "<doc>&foo;</doc>";

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetUserData(parser, NULL);
    XML_SetExternalEntityRefHandler(parser, external_entity_null_loader);
    XML_UseForeignDTD(parser, XML_TRUE);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

START_TEST(test_empty_foreign_dtd)
{
    const char *text =
        "<?xml version='1.0' encoding='us-ascii'?>\n"
        "<doc>&entity;</doc>";

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(parser, external_entity_null_loader);
    XML_UseForeignDTD(parser, XML_TRUE);
    expect_failure(text, XML_ERROR_UNDEFINED_ENTITY,
                   "Undefined entity not faulted");
}
END_TEST

/* Test XML Base is set and unset appropriately */
START_TEST(test_set_base)
{
    const XML_Char *old_base;
    const XML_Char *new_base = "/local/file/name.xml";

    old_base = XML_GetBase(parser);
    if (XML_SetBase(parser, new_base) != XML_STATUS_OK)
        fail("Unable to set base");
    if (strcmp(XML_GetBase(parser), new_base) != 0)
        fail("Base setting not correct");
    if (XML_SetBase(parser, NULL) != XML_STATUS_OK)
        fail("Unable to NULL base");
    if (XML_GetBase(parser) != NULL)
        fail("Base setting not nulled");
    XML_SetBase(parser, old_base);
}
END_TEST

/* Test attribute counts, indexing, etc */
typedef struct attrInfo {
    const char *name;
    const char *value;
} AttrInfo;

typedef struct elementInfo {
    const char *name;
    int attr_count;
    const char *id_name;
    AttrInfo *attributes;
} ElementInfo;

static void XMLCALL
counting_start_element_handler(void *userData,
                               const XML_Char *name,
                               const XML_Char **atts)
{
    ElementInfo *info = (ElementInfo *)userData;
    AttrInfo *attr;
    int count, id, i;

    while (info->name != NULL) {
        if (!strcmp(name, info->name))
            break;
        info++;
    }
    if (info->name == NULL)
        fail("Element not recognised");
    /* The attribute count is twice what you might expect.  It is a
     * count of items in atts, an array which contains alternating
     * attribute names and attribute values.  For the naive user this
     * is possibly a little unexpected, but it is what the
     * documentation in expat.h tells us to expect.
     */
    count = XML_GetSpecifiedAttributeCount(parser);
    if (info->attr_count * 2 != count) {
        fail("Not got expected attribute count");
        return;
    }
    id = XML_GetIdAttributeIndex(parser);
    if (id == -1 && info->id_name != NULL) {
        fail("ID not present");
        return;
    }
    if (id != -1 && strcmp(atts[id], info->id_name)) {
        fail("ID does not have the correct name");
        return;
    }
    for (i = 0; i < info->attr_count; i++) {
        attr = info->attributes;
        while (attr->name != NULL) {
            if (!strcmp(atts[0], attr->name))
                break;
            attr++;
        }
        if (attr->name == NULL) {
            fail("Attribute not recognised");
            return;
        }
        if (strcmp(atts[1], attr->value)) {
            fail("Attribute has wrong value");
            return;
        }
        /* Remember, two entries in atts per attribute (see above) */
        atts += 2;
    }
}

START_TEST(test_attributes)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "<!ELEMENT doc (tag)>\n"
        "<!ATTLIST doc id ID #REQUIRED>\n"
        "]>"
        "<doc a='1' id='one' b='2'>"
        "<tag c='3'/>"
        "</doc>";
    AttrInfo doc_info[] = {
        { "a",  "1" },
        { "b",  "2" },
        { "id", "one" },
        { NULL, NULL }
    };
    AttrInfo tag_info[] = {
        { "c",  "3" },
        { NULL, NULL }
    };
    ElementInfo info[] = {
        { "doc", 3, "id", NULL },
        { "tag", 1, NULL, NULL },
        { NULL, 0, NULL, NULL }
    };

    /* Silence some warnings: doc_info and tag_info are not computable
     * at load time, making the variable initialisation harder.
     */
    info[0].attributes = doc_info;
    info[1].attributes = tag_info;

    XML_SetStartElementHandler(parser, counting_start_element_handler);
    XML_SetUserData(parser, info);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

/* Test reset works correctly in the middle of processing an internal
 * entity.  Exercises some obscure code in XML_ParserReset().
 */
START_TEST(test_reset_in_entity)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "<!ENTITY wombat 'wom'>\n"
        "<!ENTITY entity 'hi &wom; there'>\n"
        "]>\n"
        "<doc>&entity;</doc>";
    XML_ParsingStatus status;

    resumable = XML_TRUE;
    XML_SetCharacterDataHandler(parser, clearing_aborting_character_handler);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_FALSE) == XML_STATUS_ERROR)
        xml_failure(parser);
    XML_GetParsingStatus(parser, &status);
    if (status.parsing != XML_SUSPENDED)
        fail("Parsing status not SUSPENDED");
    XML_ParserReset(parser, NULL);
    XML_GetParsingStatus(parser, &status);
    if (status.parsing != XML_INITIALIZED)
        fail("Parsing status doesn't reset to INITIALIZED");
}
END_TEST

/* Test that resume correctly passes through parse errors */
START_TEST(test_resume_invalid_parse)
{
    const char *text = "<doc>Hello</doc"; /* Missing closing wedge */

    resumable = XML_TRUE;
    XML_SetCharacterDataHandler(parser,
                                clearing_aborting_character_handler);
    if (XML_Parse(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    if (XML_ResumeParser(parser) == XML_STATUS_OK)
        fail("Resumed invalid parse not faulted");
    if (XML_GetErrorCode(parser) != XML_ERROR_UNCLOSED_TOKEN)
        fail("Invalid parse not correctly faulted");
}
END_TEST

/* Test that re-suspended parses are correctly passed through */
START_TEST(test_resume_resuspended)
{
    const char *text = "<doc>Hello<meep/>world</doc>";

    resumable = XML_TRUE;
    XML_SetCharacterDataHandler(parser,
                                clearing_aborting_character_handler);
    if (XML_Parse(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    resumable = XML_TRUE;
    XML_SetCharacterDataHandler(parser,
                                clearing_aborting_character_handler);
    if (XML_ResumeParser(parser) != XML_STATUS_SUSPENDED)
        fail("Resumption not suspended");
    /* This one should succeed and finish up */
    if (XML_ResumeParser(parser) != XML_STATUS_OK)
        xml_failure(parser);
}
END_TEST

/* Test that CDATA shows up correctly through a default handler */
START_TEST(test_cdata_default)
{
    const char *text = "<doc><![CDATA[Hello\nworld]]></doc>";
    CharData storage;

    CharData_Init(&storage);
    XML_SetUserData(parser, &storage);
    XML_SetDefaultHandler(parser, accumulate_characters);

    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    CharData_CheckXMLChars(&storage, text);
}
END_TEST

/* Test resetting a subordinate parser does exactly nothing */

static int XMLCALL
external_entity_resetter(XML_Parser parser,
                         const XML_Char *context,
                         const XML_Char *UNUSED_P(base),
                         const XML_Char *UNUSED_P(systemId),
                         const XML_Char *UNUSED_P(publicId))
{
    const char *text = "<!ELEMENT doc (#PCDATA)*>";
    XML_Parser ext_parser;
    XML_ParsingStatus status;

    ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (ext_parser == NULL)
        fail("Could not create external entity parser");
    XML_GetParsingStatus(ext_parser, &status);
    if (status.parsing != XML_INITIALIZED) {
        fail("Parsing status is not INITIALIZED");
        return XML_STATUS_ERROR;
    }
    if (_XML_Parse_SINGLE_BYTES(ext_parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR) {
        xml_failure(parser);
        return XML_STATUS_ERROR;
    }
    XML_GetParsingStatus(ext_parser, &status);
    if (status.parsing != XML_FINISHED) {
        fail("Parsing status is not FINISHED");
        return XML_STATUS_ERROR;
    }
    /* Check we can't parse here */
    if (XML_Parse(ext_parser, text, strlen(text),
                  XML_TRUE) != XML_STATUS_ERROR)
        fail("Parsing when finished not faulted");
    if (XML_GetErrorCode(ext_parser) != XML_ERROR_FINISHED)
        fail("Parsing when finished faulted with wrong code");
    XML_ParserReset(ext_parser, NULL);
    XML_GetParsingStatus(ext_parser, &status);
    if (status.parsing != XML_FINISHED) {
        fail("Parsing status not still FINISHED");
        return XML_STATUS_ERROR;
    }
    return XML_STATUS_OK;
}

START_TEST(test_subordinate_reset)
{
    const char *text =
        "<?xml version='1.0' encoding='us-ascii'?>\n"
        "<!DOCTYPE doc SYSTEM 'foo'>\n"
        "<doc>&entity;</doc>";

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(parser, external_entity_resetter);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST


/* Test suspending a subordinate parser */

static void XMLCALL
entity_suspending_decl_handler(void *userData,
                               const XML_Char *UNUSED_P(name),
                               XML_Content *model)
{
    XML_Parser ext_parser = (XML_Parser)userData;

    if (XML_StopParser(ext_parser, XML_TRUE) != XML_STATUS_ERROR)
        fail("Attempting to suspend a subordinate parser not faulted");
    if (XML_GetErrorCode(ext_parser) != XML_ERROR_SUSPEND_PE)
        fail("Suspending subordinate parser get wrong code");
    XML_SetElementDeclHandler(ext_parser, NULL);
    XML_FreeContentModel(parser, model);
}

static int XMLCALL
external_entity_suspender(XML_Parser parser,
                          const XML_Char *context,
                          const XML_Char *UNUSED_P(base),
                          const XML_Char *UNUSED_P(systemId),
                          const XML_Char *UNUSED_P(publicId))
{
    const char *text = "<!ELEMENT doc (#PCDATA)*>";
    XML_Parser ext_parser;

    ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (ext_parser == NULL)
        fail("Could not create external entity parser");
    XML_SetElementDeclHandler(ext_parser, entity_suspending_decl_handler);
    XML_SetUserData(ext_parser, ext_parser);
    if (_XML_Parse_SINGLE_BYTES(ext_parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR) {
        xml_failure(ext_parser);
        return XML_STATUS_ERROR;
    }
    return XML_STATUS_OK;
}

START_TEST(test_subordinate_suspend)
{
    const char *text =
        "<?xml version='1.0' encoding='us-ascii'?>\n"
        "<!DOCTYPE doc SYSTEM 'foo'>\n"
        "<doc>&entity;</doc>";

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(parser, external_entity_suspender);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

/* Test suspending a subordinate parser from an XML declaration */
/* Increases code coverage of the tests */
static void XMLCALL
entity_suspending_xdecl_handler(void *userData,
                                const XML_Char *UNUSED_P(version),
                                const XML_Char *UNUSED_P(encoding),
                                int UNUSED_P(standalone))
{
    XML_Parser ext_parser = (XML_Parser)userData;

    XML_StopParser(ext_parser, resumable);
    XML_SetXmlDeclHandler(ext_parser, NULL);
}

static int XMLCALL
external_entity_suspend_xmldecl(XML_Parser parser,
                                const XML_Char *context,
                                const XML_Char *UNUSED_P(base),
                                const XML_Char *UNUSED_P(systemId),
                                const XML_Char *UNUSED_P(publicId))
{
    const char *text = "<?xml version='1.0' encoding='us-ascii'?>";
    XML_Parser ext_parser;
    XML_ParsingStatus status;
    enum XML_Status rc;

    ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (ext_parser == NULL)
        fail("Could not create external entity parser");
    XML_SetXmlDeclHandler(ext_parser, entity_suspending_xdecl_handler);
    XML_SetUserData(ext_parser, ext_parser);
    rc = _XML_Parse_SINGLE_BYTES(ext_parser, text, strlen(text), XML_TRUE);
    XML_GetParsingStatus(ext_parser, &status);
    if (resumable) {
        if (rc == XML_STATUS_ERROR)
            xml_failure(ext_parser);
        if (status.parsing != XML_SUSPENDED)
            fail("Ext Parsing status not SUSPENDED");
    } else {
        if (rc != XML_STATUS_ERROR)
            fail("Ext parsing not aborted");
        if (XML_GetErrorCode(ext_parser) != XML_ERROR_ABORTED)
            xml_failure(ext_parser);
        if (status.parsing != XML_FINISHED)
            fail("Ext Parsing status not FINISHED");
    }
    return XML_STATUS_OK;
}

START_TEST(test_subordinate_xdecl_suspend)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "  <!ENTITY entity SYSTEM 'http://example.org/dummy.ent'>\n"
        "]>\n"
        "<doc>&entity;</doc>";

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(parser,
                                    external_entity_suspend_xmldecl);
    resumable = XML_TRUE;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

START_TEST(test_subordinate_xdecl_abort)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "  <!ENTITY entity SYSTEM 'http://example.org/dummy.ent'>\n"
        "]>\n"
        "<doc>&entity;</doc>";

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(parser,
                                    external_entity_suspend_xmldecl);
    resumable = XML_FALSE;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

/* Test external entity fault handling with suspension */
static int XMLCALL
external_entity_suspending_faulter(XML_Parser parser,
                                   const XML_Char *context,
                                   const XML_Char *UNUSED_P(base),
                                   const XML_Char *UNUSED_P(systemId),
                                   const XML_Char *UNUSED_P(publicId))
{
    XML_Parser ext_parser;
    ExtFaults *fault = (ExtFaults *)XML_GetUserData(parser);
    void *buffer;
    int parse_len = strlen(fault->parse_text);

    ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (ext_parser == NULL)
        fail("Could not create external entity parser");
    XML_SetXmlDeclHandler(ext_parser, entity_suspending_xdecl_handler);
    XML_SetUserData(ext_parser, ext_parser);
    resumable = XML_TRUE;
    buffer = XML_GetBuffer(ext_parser, parse_len);
    if (buffer == NULL)
        fail("Could not allocate parse buffer");
    memcpy(buffer, fault->parse_text, parse_len);
    if (XML_ParseBuffer(ext_parser, parse_len,
                        XML_FALSE) != XML_STATUS_SUSPENDED)
        fail("XML declaration did not suspend");
    if (XML_ResumeParser(ext_parser) != XML_STATUS_OK)
        xml_failure(ext_parser);
    if (XML_ParseBuffer(ext_parser, 0, XML_TRUE) != XML_STATUS_ERROR)
        fail(fault->fail_text);
    if (XML_GetErrorCode(ext_parser) != fault->error)
        xml_failure(ext_parser);
    return XML_STATUS_ERROR;
}

START_TEST(test_ext_entity_invalid_suspended_parse)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
        "]>\n"
        "<doc>&en;</doc>";
    ExtFaults faults[] = {
        {
            "<?xml version='1.0' encoding='us-ascii'?><",
            "Incomplete element declaration not faulted",
            NULL,
            XML_ERROR_UNCLOSED_TOKEN
        },
        {
            /* First two bytes of a three-byte char */
            "<?xml version='1.0' encoding='utf-8'?>\xe2\x82",
            "Incomplete character not faulted",
            NULL,
            XML_ERROR_PARTIAL_CHAR
        },
        { NULL, NULL, NULL, XML_ERROR_NONE }
    };
    ExtFaults *fault;

    for (fault = &faults[0]; fault->parse_text != NULL; fault++) {
        XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
        XML_SetExternalEntityRefHandler(parser,
                                        external_entity_suspending_faulter);
        XML_SetUserData(parser, fault);
        expect_failure(text,
                       XML_ERROR_EXTERNAL_ENTITY_HANDLING,
                       "Parser did not report external entity error");
        XML_ParserReset(parser, NULL);
    }
}
END_TEST



/* Test setting an explicit encoding */
START_TEST(test_explicit_encoding)
{
    const char *text1 = "<doc>Hello ";
    const char *text2 = " World</doc>";

    /* Just check that we can set the encoding to NULL before starting */
    if (XML_SetEncoding(parser, NULL) != XML_STATUS_OK)
        fail("Failed to initialise encoding to NULL");
    /* Say we are UTF-8 */
    if (XML_SetEncoding(parser, "utf-8") != XML_STATUS_OK)
        fail("Failed to set explicit encoding");
    if (_XML_Parse_SINGLE_BYTES(parser, text1, strlen(text1),
                                XML_FALSE) == XML_STATUS_ERROR)
        xml_failure(parser);
    /* Try to switch encodings mid-parse */
    if (XML_SetEncoding(parser, "us-ascii") != XML_STATUS_ERROR)
        fail("Allowed encoding change");
    if (_XML_Parse_SINGLE_BYTES(parser, text2, strlen(text2),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    /* Try now the parse is over */
    if (XML_SetEncoding(parser, NULL) != XML_STATUS_OK)
        fail("Failed to unset encoding");
}
END_TEST


/* Test handling of trailing CR (rather than newline) */
static void XMLCALL
cr_cdata_handler(void *userData, const XML_Char *s, int len)
{
    int *pfound = (int *)userData;

    /* Internal processing turns the CR into a newline for the
     * character data handler, but not for the default handler
     */
    if (len == 1 && (*s == '\n' || *s == '\r'))
        *pfound = 1;
}

START_TEST(test_trailing_cr)
{
    const char *text = "<doc>\r";
    int found_cr;

    /* Try with a character handler, for code coverage */
    XML_SetCharacterDataHandler(parser, cr_cdata_handler);
    XML_SetUserData(parser, &found_cr);
    found_cr = 0;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_OK)
        fail("Failed to fault unclosed doc");
    if (found_cr == 0)
        fail("Did not catch the carriage return");
    XML_ParserReset(parser, NULL);

    /* Now with a default handler instead */
    XML_SetDefaultHandler(parser, cr_cdata_handler);
    XML_SetUserData(parser, &found_cr);
    found_cr = 0;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_OK)
        fail("Failed to fault unclosed doc");
    if (found_cr == 0)
        fail("Did not catch default carriage return");
}
END_TEST

/* Test trailing CR in an external entity parse */
static int XMLCALL
external_entity_cr_catcher(XML_Parser parser,
                           const XML_Char *context,
                           const XML_Char *UNUSED_P(base),
                           const XML_Char *UNUSED_P(systemId),
                           const XML_Char *UNUSED_P(publicId))
{
    const char *text = "\r";
    XML_Parser ext_parser;

    ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (ext_parser == NULL)
        fail("Could not create external entity parser");
    XML_SetCharacterDataHandler(ext_parser, cr_cdata_handler);
    if (_XML_Parse_SINGLE_BYTES(ext_parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(ext_parser);
    XML_ParserFree(ext_parser);
    return XML_STATUS_OK;
}

static int XMLCALL
external_entity_bad_cr_catcher(XML_Parser parser,
                               const XML_Char *context,
                               const XML_Char *UNUSED_P(base),
                               const XML_Char *UNUSED_P(systemId),
                               const XML_Char *UNUSED_P(publicId))
{
    const char *text = "<tag>\r";
    XML_Parser ext_parser;

    ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (ext_parser == NULL)
        fail("Could not create external entity parser");
    XML_SetCharacterDataHandler(ext_parser, cr_cdata_handler);
    if (_XML_Parse_SINGLE_BYTES(ext_parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_OK)
        fail("Async entity error not caught");
    if (XML_GetErrorCode(ext_parser) != XML_ERROR_ASYNC_ENTITY)
        xml_failure(ext_parser);
    XML_ParserFree(ext_parser);
    return XML_STATUS_OK;
}

START_TEST(test_ext_entity_trailing_cr)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
        "]>\n"
        "<doc>&en;</doc>";
    int found_cr;

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(parser, external_entity_cr_catcher);
    XML_SetUserData(parser, &found_cr);
    found_cr = 0;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) != XML_STATUS_OK)
        xml_failure(parser);
    if (found_cr == 0)
        fail("No carriage return found");
    XML_ParserReset(parser, NULL);

    /* Try again with a different trailing CR */
    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(parser, external_entity_bad_cr_catcher);
    XML_SetUserData(parser, &found_cr);
    found_cr = 0;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) != XML_STATUS_OK)
        xml_failure(parser);
    if (found_cr == 0)
        fail("No carriage return found");
}
END_TEST

/* Test handling of trailing square bracket */
static void XMLCALL
rsqb_handler(void *userData, const XML_Char *s, int len)
{
    int *pfound = (int *)userData;

    if (len == 1 && *s == ']')
        *pfound = 1;
}

START_TEST(test_trailing_rsqb)
{
    const char *text8 = "<doc>]";
    const char text16[] = "\xFF\xFE<\000d\000o\000c\000>\000]\000";
    int found_rsqb;
    int text8_len = strlen(text8);

    XML_SetCharacterDataHandler(parser, rsqb_handler);
    XML_SetUserData(parser, &found_rsqb);
    found_rsqb = 0;
    if (_XML_Parse_SINGLE_BYTES(parser, text8, text8_len,
                                XML_TRUE) == XML_STATUS_OK)
        fail("Failed to fault unclosed doc");
    if (found_rsqb == 0)
        fail("Did not catch the right square bracket");

    /* Try again with a different encoding */
    XML_ParserReset(parser, NULL);
    XML_SetCharacterDataHandler(parser, rsqb_handler);
    XML_SetUserData(parser, &found_rsqb);
    found_rsqb = 0;
    if (_XML_Parse_SINGLE_BYTES(parser, text16, sizeof(text16)-1,
                                XML_TRUE) == XML_STATUS_OK)
        fail("Failed to fault unclosed doc");
    if (found_rsqb == 0)
        fail("Did not catch the right square bracket");

    /* And finally with a default handler */
    XML_ParserReset(parser, NULL);
    XML_SetDefaultHandler(parser, rsqb_handler);
    XML_SetUserData(parser, &found_rsqb);
    found_rsqb = 0;
    if (_XML_Parse_SINGLE_BYTES(parser, text16, sizeof(text16)-1,
                                XML_TRUE) == XML_STATUS_OK)
        fail("Failed to fault unclosed doc");
    if (found_rsqb == 0)
        fail("Did not catch the right square bracket");
}
END_TEST

/* Test trailing right square bracket in an external entity parse */
static int XMLCALL
external_entity_rsqb_catcher(XML_Parser parser,
                             const XML_Char *context,
                             const XML_Char *UNUSED_P(base),
                             const XML_Char *UNUSED_P(systemId),
                             const XML_Char *UNUSED_P(publicId))
{
    const char *text = "<tag>]";
    XML_Parser ext_parser;

    ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (ext_parser == NULL)
        fail("Could not create external entity parser");
    XML_SetCharacterDataHandler(ext_parser, rsqb_handler);
    if (_XML_Parse_SINGLE_BYTES(ext_parser, text, strlen(text),
                                XML_TRUE) != XML_STATUS_ERROR)
        fail("Async entity error not caught");
    if (XML_GetErrorCode(ext_parser) != XML_ERROR_ASYNC_ENTITY)
        xml_failure(ext_parser);
    XML_ParserFree(ext_parser);
    return XML_STATUS_OK;
}

START_TEST(test_ext_entity_trailing_rsqb)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
        "]>\n"
        "<doc>&en;</doc>";
    int found_rsqb;

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(parser, external_entity_rsqb_catcher);
    XML_SetUserData(parser, &found_rsqb);
    found_rsqb = 0;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) != XML_STATUS_OK)
        xml_failure(parser);
    if (found_rsqb == 0)
        fail("No right square bracket found");
}
END_TEST

/* Test CDATA handling in an external entity */
static int XMLCALL
external_entity_good_cdata_ascii(XML_Parser parser,
                                 const XML_Char *context,
                                 const XML_Char *UNUSED_P(base),
                                 const XML_Char *UNUSED_P(systemId),
                                 const XML_Char *UNUSED_P(publicId))
{
    const char *text =
        "<a><![CDATA[<greeting>Hello, world!</greeting>]]></a>";
    const char *expected = "<greeting>Hello, world!</greeting>";
    CharData storage;
    XML_Parser ext_parser;

    CharData_Init(&storage);
    ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (ext_parser == NULL)
        fail("Could not create external entity parser");
    XML_SetUserData(ext_parser, &storage);
    XML_SetCharacterDataHandler(ext_parser, accumulate_characters);

    if (_XML_Parse_SINGLE_BYTES(ext_parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(ext_parser);
    CharData_CheckXMLChars(&storage, expected);

    return XML_STATUS_OK;
}

START_TEST(test_ext_entity_good_cdata)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
        "]>\n"
        "<doc>&en;</doc>";

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(parser,
                                    external_entity_good_cdata_ascii);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) != XML_STATUS_OK)
        xml_failure(parser);
}
END_TEST

/* Test user parameter settings */
/* Variable holding the expected handler userData */
static void *handler_data = NULL;
/* Count of the number of times the comment handler has been invoked */
static int comment_count = 0;
/* Count of the number of skipped entities */
static int skip_count = 0;
/* Count of the number of times the XML declaration handler is invoked */
static int xdecl_count = 0;

static void XMLCALL
xml_decl_handler(void *userData,
                 const XML_Char *UNUSED_P(version),
                 const XML_Char *UNUSED_P(encoding),
                 int standalone)
{
    if (userData != handler_data)
        fail("User data (xml decl) not correctly set");
    if (standalone != -1)
        fail("Standalone not flagged as not present in XML decl");
    xdecl_count++;
}

static void XMLCALL
param_check_skip_handler(void *userData,
                         const XML_Char *UNUSED_P(entityName),
                         int UNUSED_P(is_parameter_entity))
{
    if (userData != handler_data)
        fail("User data (skip) not correctly set");
    skip_count++;
}

static void XMLCALL
data_check_comment_handler(void *userData, const XML_Char *UNUSED_P(data))
{
    /* Check that the userData passed through is what we expect */
    if (userData != handler_data)
        fail("User data (parser) not correctly set");
    /* Check that the user data in the parser is appropriate */
    if (XML_GetUserData(userData) != (void *)1)
        fail("User data in parser not correctly set");
    comment_count++;
}

static int XMLCALL
external_entity_param_checker(XML_Parser parser,
                              const XML_Char *context,
                              const XML_Char *UNUSED_P(base),
                              const XML_Char *UNUSED_P(systemId),
                              const XML_Char *UNUSED_P(publicId))
{
    const char *text =
        "<!-- Subordinate parser -->\n"
        "<!ELEMENT doc (#PCDATA)*>";
    XML_Parser ext_parser;

    ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (ext_parser == NULL)
        fail("Could not create external entity parser");
    handler_data = ext_parser;
    if (_XML_Parse_SINGLE_BYTES(ext_parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR) {
        xml_failure(parser);
        return XML_STATUS_ERROR;
    }
    handler_data = parser;
    return XML_STATUS_OK;
}

START_TEST(test_user_parameters)
{
    const char *text =
        "<?xml version='1.0' encoding='us-ascii'?>\n"
        "<!-- Primary parse -->\n"
        "<!DOCTYPE doc SYSTEM 'foo'>\n"
        "<doc>&entity;";
    const char *epilog =
        "<!-- Back to primary parser -->\n"
        "</doc>";

    comment_count = 0;
    skip_count = 0;
    xdecl_count = 0;
    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetXmlDeclHandler(parser, xml_decl_handler);
    XML_SetExternalEntityRefHandler(parser, external_entity_param_checker);
    XML_SetCommentHandler(parser, data_check_comment_handler);
    XML_SetSkippedEntityHandler(parser, param_check_skip_handler);
    XML_UseParserAsHandlerArg(parser);
    XML_SetUserData(parser, (void *)1);
    handler_data = parser;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_FALSE) == XML_STATUS_ERROR)
        xml_failure(parser);
    if (comment_count != 2)
        fail("Comment handler not invoked enough times");
    /* Ensure we can't change policy mid-parse */
    if (XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_NEVER))
        fail("Changed param entity parsing policy while parsing");
    if (_XML_Parse_SINGLE_BYTES(parser, epilog, strlen(epilog),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    if (comment_count != 3)
        fail("Comment handler not invoked enough times");
    if (skip_count != 1)
        fail("Skip handler not invoked enough times");
    if (xdecl_count != 1)
        fail("XML declaration handler not invoked");
}
END_TEST

/* Test that an explicit external entity handler argument replaces
 * the parser as the first argument.
 *
 * We do not call the first parameter to the external entity handler
 * 'parser' for once, since the first time the handler is called it
 * will actually be a text string.  We need to be able to access the
 * global 'parser' variable to create our external entity parser from,
 * since there are code paths we need to ensure get executed.
 */
static int XMLCALL
external_entity_ref_param_checker(XML_Parser parameter,
                                  const XML_Char *context,
                                  const XML_Char *UNUSED_P(base),
                                  const XML_Char *UNUSED_P(systemId),
                                  const XML_Char *UNUSED_P(publicId))
{
    const char *text = "<!ELEMENT doc (#PCDATA)*>";
    XML_Parser ext_parser;

    if ((void *)parameter != handler_data)
        fail("External entity ref handler parameter not correct");

    /* Here we use the global 'parser' variable */
    ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (ext_parser == NULL)
        fail("Could not create external entity parser");
    if (_XML_Parse_SINGLE_BYTES(ext_parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(ext_parser);

    return XML_STATUS_OK;
}

START_TEST(test_ext_entity_ref_parameter)
{
    const char *text =
        "<?xml version='1.0' encoding='us-ascii'?>\n"
        "<!DOCTYPE doc SYSTEM 'foo'>\n"
        "<doc>&entity;</doc>";

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(parser,
                                    external_entity_ref_param_checker);
    /* Set a handler arg that is not NULL and not parser (which is
     * what NULL would cause to be passed.
     */
    XML_SetExternalEntityRefHandlerArg(parser, (void *)text);
    handler_data = (void *)text;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);

    /* Now try again with unset args */
    XML_ParserReset(parser, NULL);
    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(parser,
                                    external_entity_ref_param_checker);
    XML_SetExternalEntityRefHandlerArg(parser, NULL);
    handler_data = (void *)parser;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

/* Test the parsing of an empty string */
START_TEST(test_empty_parse)
{
    const char *text = "<doc></doc>";
    const char *partial = "<doc>";

    if (XML_Parse(parser, NULL, 0, XML_FALSE) == XML_STATUS_ERROR)
        fail("Parsing empty string faulted");
    if (XML_Parse(parser, NULL, 0, XML_TRUE) != XML_STATUS_ERROR)
        fail("Parsing final empty string not faulted");
    if (XML_GetErrorCode(parser) != XML_ERROR_NO_ELEMENTS)
        fail("Parsing final empty string faulted for wrong reason");

    /* Now try with valid text before the empty end */
    XML_ParserReset(parser, NULL);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_FALSE) == XML_STATUS_ERROR)
        xml_failure(parser);
    if (XML_Parse(parser, NULL, 0, XML_TRUE) == XML_STATUS_ERROR)
        fail("Parsing final empty string faulted");

    /* Now try with invalid text before the empty end */
    XML_ParserReset(parser, NULL);
    if (_XML_Parse_SINGLE_BYTES(parser, partial, strlen(partial),
                                XML_FALSE) == XML_STATUS_ERROR)
        xml_failure(parser);
    if (XML_Parse(parser, NULL, 0, XML_TRUE) != XML_STATUS_ERROR)
        fail("Parsing final incomplete empty string not faulted");
}
END_TEST

/* Test odd corners of the XML_GetBuffer interface */
static enum XML_Status
get_feature(enum XML_FeatureEnum feature_id, long *presult)
{
    const XML_Feature *feature = XML_GetFeatureList();

    if (feature == NULL)
        return XML_STATUS_ERROR;
    for (; feature->feature != XML_FEATURE_END; feature++) {
        if (feature->feature == feature_id) {
            *presult = feature->value;
            return XML_STATUS_OK;
        }
    }
    return XML_STATUS_ERROR;
}

/* Having an element name longer than 1024 characters exercises some
 * of the pool allocation code in the parser that otherwise does not
 * get executed.  The count at the end of the line is the number of
 * characters (bytes) in the element name by that point.x
 */
static const char *get_buffer_test_text =
        "<documentwitharidiculouslylongelementnametotease" /* 0x02f */
        "aparticularcorneroftheallocationinXML_GetBuffers" /* 0x05f */
        "othatwecanimprovethecoverageyetagain012345678901" /* 0x08f */
        "123456789abcdef0123456789abcdef0123456789abcdef0" /* 0x0bf */
        "123456789abcdef0123456789abcdef0123456789abcdef0" /* 0x0ef */
        "123456789abcdef0123456789abcdef0123456789abcdef0" /* 0x11f */
        "123456789abcdef0123456789abcdef0123456789abcdef0" /* 0x1f4 */
        "123456789abcdef0123456789abcdef0123456789abcdef0" /* 0x17f */
        "123456789abcdef0123456789abcdef0123456789abcdef0" /* 0x1af */
        "123456789abcdef0123456789abcdef0123456789abcdef0" /* 0x1df */
        "123456789abcdef0123456789abcdef0123456789abcdef0" /* 0x20f */
        "123456789abcdef0123456789abcdef0123456789abcdef0" /* 0x23f */
        "123456789abcdef0123456789abcdef0123456789abcdef0" /* 0x26f */
        "123456789abcdef0123456789abcdef0123456789abcdef0" /* 0x29f */
        "123456789abcdef0123456789abcdef0123456789abcdef0" /* 0x2cf */
        "123456789abcdef0123456789abcdef0123456789abcdef0" /* 0x2ff */
        "123456789abcdef0123456789abcdef0123456789abcdef0" /* 0x32f */
        "123456789abcdef0123456789abcdef0123456789abcdef0" /* 0x35f */
        "123456789abcdef0123456789abcdef0123456789abcdef0" /* 0x38f */
        "123456789abcdef0123456789abcdef0123456789abcdef0" /* 0x3bf */
        "123456789abcdef0123456789abcdef0123456789abcdef0" /* 0x3ef */
        "123456789abcdef0123456789abcdef0123456789>\n<ef0"; /* 0x418 */

/* Test odd corners of the XML_GetBuffer interface */
START_TEST(test_get_buffer_1)
{
    const char *text = get_buffer_test_text;
    void *buffer;
    long context_bytes;

    /* Attempt to allocate a negative length buffer */
    if (XML_GetBuffer(parser, -12) != NULL)
        fail("Negative length buffer not failed");

    /* Now get a small buffer and extend it past valid length */
    buffer = XML_GetBuffer(parser, 1536);
    if (buffer == NULL)
        fail("1.5K buffer failed");
    memcpy(buffer, text, strlen(text));
    if (XML_ParseBuffer(parser, strlen(text), XML_FALSE) == XML_STATUS_ERROR)
        xml_failure(parser);
    if (XML_GetBuffer(parser, INT_MAX) != NULL)
        fail("INT_MAX buffer not failed");

    /* Now try extending it a more reasonable but still too large
     * amount.  The allocator in XML_GetBuffer() doubles the buffer
     * size until it exceeds the requested amount or INT_MAX.  If it
     * exceeds INT_MAX, it rejects the request, so we want a request
     * between INT_MAX and INT_MAX/2.  A gap of 1K seems comfortable,
     * with an extra byte just to ensure that the request is off any
     * boundary.  The request will be inflated internally by
     * XML_CONTEXT_BYTES (if defined), so we subtract that from our
     * request.
     */
    if (get_feature(XML_FEATURE_CONTEXT_BYTES,
                    &context_bytes) != XML_STATUS_OK)
        context_bytes = 0;
    if (XML_GetBuffer(parser, INT_MAX - (context_bytes + 1025)) != NULL)
        fail("INT_MAX- buffer not failed");

    /* Now try extending it a carefully crafted amount */
    if (XML_GetBuffer(parser, 1000) == NULL)
        fail("1000 buffer failed");
}
END_TEST

/* Test more corners of the XML_GetBuffer interface */
START_TEST(test_get_buffer_2)
{
    const char *text = get_buffer_test_text;
    void *buffer;

    /* Now get a decent buffer */
    buffer = XML_GetBuffer(parser, 1536);
    if (buffer == NULL)
        fail("1.5K buffer failed");
    memcpy(buffer, text, strlen(text));
    if (XML_ParseBuffer(parser, strlen(text), XML_FALSE) == XML_STATUS_ERROR)
        xml_failure(parser);

    /* Extend it, to catch a different code path */
    if (XML_GetBuffer(parser, 1024) == NULL)
        fail("1024 buffer failed");
}
END_TEST

/* Test position information macros */
START_TEST(test_byte_info_at_end)
{
    const char *text = "<doc></doc>";

    if (XML_GetCurrentByteIndex(parser) != -1 ||
        XML_GetCurrentByteCount(parser) != 0)
        fail("Byte index/count incorrect at start of parse");
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    /* At end, the count will be zero and the index the end of string */
    if (XML_GetCurrentByteCount(parser) != 0)
        fail("Terminal byte count incorrect");
    if (XML_GetCurrentByteIndex(parser) != (XML_Index)strlen(text))
        fail("Terminal byte index incorrect");
}
END_TEST

/* Test position information from errors */
#define PRE_ERROR_STR  "<doc></"
#define POST_ERROR_STR "wombat></doc>"
START_TEST(test_byte_info_at_error)
{
    const char *text = PRE_ERROR_STR POST_ERROR_STR;

    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_OK)
        fail("Syntax error not faulted");
    if (XML_GetCurrentByteCount(parser) != 0)
        fail("Error byte count incorrect");
    if (XML_GetCurrentByteIndex(parser) != strlen(PRE_ERROR_STR))
        fail("Error byte index incorrect");
}
END_TEST
#undef PRE_ERROR_STR
#undef POST_ERROR_STR

/* Test position information in handler */
typedef struct ByteTestData {
    int start_element_len;
    int cdata_len;
    int total_string_len;
} ByteTestData;

static void
byte_character_handler(void *userData,
                       const XML_Char *s,
                       int len)
{
    int offset, size;
    const char *buffer;
    ByteTestData *data = (ByteTestData *)userData;

    buffer = XML_GetInputContext(parser, &offset, &size);
    if (buffer == NULL)
        fail("Failed to get context buffer");
    if (offset != data->start_element_len)
        fail("Context offset in unexpected position");
    if (len != data->cdata_len)
        fail("CDATA length reported incorrectly");
    if (size != data->total_string_len)
        fail("Context size is not full buffer");
    if (XML_GetCurrentByteIndex(parser) != offset)
        fail("Character byte index incorrect");
    if (XML_GetCurrentByteCount(parser) != len)
        fail("Character byte count incorrect");
    if (s != buffer + offset)
        fail("Buffer position incorrect");
}

#define START_ELEMENT "<e>"
#define CDATA_TEXT    "Hello"
#define END_ELEMENT   "</e>"
START_TEST(test_byte_info_at_cdata)
{
    const char *text = START_ELEMENT CDATA_TEXT END_ELEMENT;
    int offset, size;
    ByteTestData data;

    /* Check initial context is empty */
    if (XML_GetInputContext(parser, &offset, &size) != NULL)
        fail("Unexpected context at start of parse");

    data.start_element_len = strlen(START_ELEMENT);
    data.cdata_len = strlen(CDATA_TEXT);
    data.total_string_len = strlen(text);
    XML_SetCharacterDataHandler(parser, byte_character_handler);
    XML_SetUserData(parser, &data);
    if (XML_Parse(parser, text, strlen(text), XML_TRUE) != XML_STATUS_OK)
        xml_failure(parser);
}
END_TEST
#undef START_ELEMENT
#undef CDATA_TEXT
#undef END_ELEMENT

/* Test predefined entities are correctly recognised */
START_TEST(test_predefined_entities)
{
    const char *text = "<doc>&lt;&gt;&amp;&quot;&apos;</doc>";
    const char *result = "<>&\"'";
    CharData storage;

    XML_SetDefaultHandler(parser, accumulate_characters);
    /* run_character_check uses XML_SetCharacterDataHandler(), which
     * unfortunately heads off a code path that we need to exercise.
     */
    CharData_Init(&storage);
    XML_SetUserData(parser, &storage);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    /* The default handler doesn't translate the entities */
    CharData_CheckXMLChars(&storage, text);

    /* Now try again and check the translation */
    XML_ParserReset(parser, NULL);
    run_character_check(text, result);
}
END_TEST

/* Test conditional inclusion (IGNORE) */
static int XMLCALL
external_entity_load_ignore(XML_Parser parser,
                            const XML_Char *context,
                            const XML_Char *UNUSED_P(base),
                            const XML_Char *UNUSED_P(systemId),
                            const XML_Char *UNUSED_P(publicId))
{
    const char *text = "<![IGNORE[<!ELEMENT e (#PCDATA)*>]]>";
    XML_Parser ext_parser;

    ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (ext_parser == NULL)
        fail("Could not create external entity parser");
    if (_XML_Parse_SINGLE_BYTES(ext_parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    return XML_STATUS_OK;
}

START_TEST(test_ignore_section)
{
    const char *text =
        "<!DOCTYPE doc SYSTEM 'foo'>\n"
        "<doc><e>&entity;</e></doc>";
    const char *expected =
        "<![IGNORE[<!ELEMENT e (#PCDATA)*>]]>\n&entity;";
    CharData storage;

    CharData_Init(&storage);
    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetUserData(parser, &storage);
    XML_SetExternalEntityRefHandler(parser, external_entity_load_ignore);
    XML_SetDefaultHandler(parser, accumulate_characters);
    XML_SetStartDoctypeDeclHandler(parser, dummy_start_doctype_handler);
    XML_SetEndDoctypeDeclHandler(parser, dummy_end_doctype_handler);
    XML_SetElementDeclHandler(parser, dummy_element_decl_handler);
    XML_SetStartElementHandler(parser, dummy_start_element);
    XML_SetEndElementHandler(parser, dummy_end_element);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    CharData_CheckXMLChars(&storage, expected);
}
END_TEST

/* Test mis-formatted conditional exclusion */
START_TEST(test_bad_ignore_section)
{
    const char *text =
        "<!DOCTYPE doc SYSTEM 'foo'>\n"
        "<doc><e>&entity;</e></doc>";
    ExtFaults faults[] = {
        {
            "<![IGNORE[<!ELEM",
            "Broken-off declaration not faulted",
            NULL,
            XML_ERROR_SYNTAX
        },
        {
            "<![IGNORE[\x01]]>",
            "Invalid XML character not faulted",
            NULL,
            XML_ERROR_INVALID_TOKEN
        },
        {
            /* FIrst two bytes of a three-byte char */
            "<![IGNORE[\xe2\x82",
            "Partial XML character not faulted",
            NULL,
            XML_ERROR_PARTIAL_CHAR
        },
        { NULL, NULL, NULL, XML_ERROR_NONE }
    };
    ExtFaults *fault;

    for (fault = &faults[0]; fault->parse_text != NULL; fault++) {
        XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
        XML_SetExternalEntityRefHandler(parser, external_entity_faulter);
        XML_SetUserData(parser, fault);
        expect_failure(text, XML_ERROR_EXTERNAL_ENTITY_HANDLING,
                       "Incomplete IGNORE section not failed");
        XML_ParserReset(parser, NULL);
    }
}
END_TEST

/* Test recursive parsing */
static int XMLCALL
external_entity_valuer(XML_Parser parser,
                       const XML_Char *context,
                       const XML_Char *UNUSED_P(base),
                       const XML_Char *systemId,
                       const XML_Char *UNUSED_P(publicId))
{
    const char *text1 =
        "<!ELEMENT doc EMPTY>\n"
        "<!ENTITY % e1 SYSTEM '004-2.ent'>\n"
        "<!ENTITY % e2 '%e1;'>\n"
        "%e1;\n";
    XML_Parser ext_parser;

    if (systemId == NULL)
        return XML_STATUS_OK;
    ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (ext_parser == NULL)
        fail("Could not create external entity parser");
    if (!strcmp(systemId, "004-1.ent")) {
        if (_XML_Parse_SINGLE_BYTES(ext_parser, text1, strlen(text1),
                                    XML_TRUE) == XML_STATUS_ERROR)
            xml_failure(ext_parser);
    }
    else if (!strcmp(systemId, "004-2.ent")) {
        ExtFaults *fault = (ExtFaults *)XML_GetUserData(parser);
        enum XML_Status status;
        enum XML_Error error;

        status = _XML_Parse_SINGLE_BYTES(ext_parser,
                                         fault->parse_text,
                                         strlen(fault->parse_text),
                                         XML_TRUE);
        if (fault->error == XML_ERROR_NONE) {
            if (status == XML_STATUS_ERROR)
                xml_failure(ext_parser);
        } else {
            if (status != XML_STATUS_ERROR)
                fail(fault->fail_text);
            error = XML_GetErrorCode(ext_parser);
            if (error != fault->error &&
                (fault->error != XML_ERROR_XML_DECL ||
                 error != XML_ERROR_TEXT_DECL))
                xml_failure(ext_parser);
        }
    }
    return XML_STATUS_OK;
}

START_TEST(test_external_entity_values)
{
    const char *text =
        "<!DOCTYPE doc SYSTEM '004-1.ent'>\n"
        "<doc></doc>\n";
    ExtFaults data_004_2[] = {
        {
            "<!ATTLIST doc a1 CDATA 'value'>",
            NULL,
            NULL,
            XML_ERROR_NONE
        },
        {
            "<!ATTLIST $doc a1 CDATA 'value'>",
            "Invalid token not faulted",
            NULL,
            XML_ERROR_INVALID_TOKEN
        },
        {
            "'wombat",
            "Unterminated string not faulted",
            NULL,
            XML_ERROR_UNCLOSED_TOKEN
        },
        {
            "\xe2\x82",
            "Partial UTF-8 character not faulted",
            NULL,
            XML_ERROR_PARTIAL_CHAR
        },
        {
            "<?xml version='1.0' encoding='utf-8'?>\n",
            NULL,
            NULL,
            XML_ERROR_NONE
        },
        {
            "<?xml?>",
            "Malformed XML declaration not faulted",
            NULL,
            XML_ERROR_XML_DECL
        },
        {
            /* UTF-8 BOM */
            "\xEF\xBB\xBF<!ATTLIST doc a1 CDATA 'value'>",
            NULL,
            NULL,
            XML_ERROR_NONE
        },
        {
            "<?xml version='1.0' encoding='utf-8'?>\n$",
            "Invalid token after text declaration not faulted",
            NULL,
            XML_ERROR_INVALID_TOKEN
        },
        {
            "<?xml version='1.0' encoding='utf-8'?>\n'wombat",
            "Unterminated string after text decl not faulted",
            NULL,
            XML_ERROR_UNCLOSED_TOKEN
        },
        {
            "<?xml version='1.0' encoding='utf-8'?>\n\xe2\x82",
            "Partial UTF-8 character after text decl not faulted",
            NULL,
            XML_ERROR_PARTIAL_CHAR
        },
        { NULL, NULL, NULL, XML_ERROR_NONE }
    };
    int i;

    for (i = 0; data_004_2[i].parse_text != NULL; i++) {
        XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
        XML_SetExternalEntityRefHandler(parser, external_entity_valuer);
        XML_SetUserData(parser, &data_004_2[i]);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) == XML_STATUS_ERROR)
            xml_failure(parser);
        XML_ParserReset(parser, NULL);
    }
}
END_TEST

/* Test the recursive parse interacts with a not standalone handler */
static int XMLCALL
external_entity_not_standalone(XML_Parser parser,
                               const XML_Char *context,
                               const XML_Char *UNUSED_P(base),
                               const XML_Char *systemId,
                               const XML_Char *UNUSED_P(publicId))
{
    const char *text1 =
        "<!ELEMENT doc EMPTY>\n"
        "<!ENTITY % e1 SYSTEM 'bar'>\n"
        "%e1;\n";
    const char *text2 = "<!ATTLIST doc a1 CDATA 'value'>";
    XML_Parser ext_parser;

    if (systemId == NULL)
        return XML_STATUS_OK;
    ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (ext_parser == NULL)
        fail("Could not create external entity parser");
    if (!strcmp(systemId, "foo")) {
        XML_SetNotStandaloneHandler(ext_parser,
                                    reject_not_standalone_handler);
        if (_XML_Parse_SINGLE_BYTES(ext_parser, text1, strlen(text1),
                                    XML_TRUE) != XML_STATUS_ERROR)
            fail("Expected not standalone rejection");
        if (XML_GetErrorCode(ext_parser) != XML_ERROR_NOT_STANDALONE)
            xml_failure(ext_parser);
        XML_SetNotStandaloneHandler(ext_parser, NULL);
        return XML_STATUS_ERROR;
    }
    else if (!strcmp(systemId, "bar")) {
        if (_XML_Parse_SINGLE_BYTES(ext_parser, text2, strlen(text2),
                                    XML_TRUE) == XML_STATUS_ERROR)
            xml_failure(ext_parser);
    }
    return XML_STATUS_OK;
}

START_TEST(test_ext_entity_not_standalone)
{
    const char *text =
        "<!DOCTYPE doc SYSTEM 'foo'>\n"
        "<doc></doc>";

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(parser, external_entity_not_standalone);
    expect_failure(text, XML_ERROR_EXTERNAL_ENTITY_HANDLING,
                   "Standalone rejection not caught");
}
END_TEST

static int XMLCALL
external_entity_value_aborter(XML_Parser parser,
                              const XML_Char *context,
                              const XML_Char *UNUSED_P(base),
                              const XML_Char *systemId,
                              const XML_Char *UNUSED_P(publicId))
{
    const char *text1 =
        "<!ELEMENT doc EMPTY>\n"
        "<!ENTITY % e1 SYSTEM '004-2.ent'>\n"
        "<!ENTITY % e2 '%e1;'>\n"
        "%e1;\n";
    const char *text2 =
        "<?xml version='1.0' encoding='utf-8'?>";
    XML_Parser ext_parser;

    if (systemId == NULL)
        return XML_STATUS_OK;
    ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (ext_parser == NULL)
        fail("Could not create external entity parser");
    if (!strcmp(systemId, "004-1.ent")) {
        if (_XML_Parse_SINGLE_BYTES(ext_parser, text1, strlen(text1),
                                    XML_TRUE) == XML_STATUS_ERROR)
            xml_failure(ext_parser);
    }
    if (!strcmp(systemId, "004-2.ent")) {
        XML_SetXmlDeclHandler(ext_parser, entity_suspending_xdecl_handler);
        XML_SetUserData(ext_parser, ext_parser);
        if (_XML_Parse_SINGLE_BYTES(ext_parser, text2, strlen(text2),
                                    XML_TRUE) != XML_STATUS_ERROR)
            fail("Aborted parse not faulted");
        if (XML_GetErrorCode(ext_parser) != XML_ERROR_ABORTED)
            xml_failure(ext_parser);
    }
    return XML_STATUS_OK;
}

START_TEST(test_ext_entity_value_abort)
{
    const char *text =
        "<!DOCTYPE doc SYSTEM '004-1.ent'>\n"
        "<doc></doc>\n";

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(parser,
                                    external_entity_value_aborter);
    resumable = XML_FALSE;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

START_TEST(test_bad_public_doctype)
{
    const char *text =
        "<?xml version='1.0' encoding='utf-8'?>\n"
        "<!DOCTYPE doc PUBLIC '{BadName}' 'test'>\n"
        "<doc></doc>";

    /* Setting a handler provokes a particular code path */
    XML_SetDoctypeDeclHandler(parser,
                              dummy_start_doctype_handler,
                              dummy_end_doctype_handler);
    expect_failure(text, XML_ERROR_PUBLICID, "Bad Public ID not failed");
}
END_TEST

/* Test based on ibm/valid/P32/ibm32v04.xml */
START_TEST(test_attribute_enum_value)
{
    const char *text =
        "<?xml version='1.0' standalone='no'?>\n"
        "<!DOCTYPE animal SYSTEM 'test.dtd'>\n"
        "<animal>This is a \n    <a/>  \n\nyellow tiger</animal>";
    ExtTest dtd_data = {
        "<!ELEMENT animal (#PCDATA|a)*>\n"
        "<!ELEMENT a EMPTY>\n"
        "<!ATTLIST animal xml:space (default|preserve) 'preserve'>",
        NULL,
        NULL
    };

    XML_SetExternalEntityRefHandler(parser, external_entity_loader);
    XML_SetUserData(parser, &dtd_data);
    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    /* An attribute list handler provokes a different code path */
    XML_SetAttlistDeclHandler(parser, dummy_attlist_decl_handler);
    run_ext_character_check(text, &dtd_data,
                            "This is a \n      \n\nyellow tiger");
}
END_TEST

/* Slightly bizarrely, the library seems to silently ignore entity
 * definitions for predefined entities, even when they are wrong.  The
 * language of the XML 1.0 spec is somewhat unhelpful as to what ought
 * to happen, so this is currently treated as acceptable.
 */
START_TEST(test_predefined_entity_redefinition)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "<!ENTITY apos 'foo'>\n"
        "]>\n"
        "<doc>&apos;</doc>";
    run_character_check(text, "'");
}
END_TEST

/* Test that the parser stops processing the DTD after an unresolved
 * parameter entity is encountered.
 */
START_TEST(test_dtd_stop_processing)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "%foo;\n"
        "<!ENTITY bar 'bas'>\n"
        "]><doc/>";

    XML_SetEntityDeclHandler(parser, dummy_entity_decl_handler);
    dummy_handler_flags = 0;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    if (dummy_handler_flags != 0)
        fail("DTD processing still going after undefined PE");
}
END_TEST

/* Test public notations with no system ID */
START_TEST(test_public_notation_no_sysid)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "<!NOTATION note PUBLIC 'foo'>\n"
        "<!ELEMENT doc EMPTY>\n"
        "]>\n<doc/>";

    dummy_handler_flags = 0;
    XML_SetNotationDeclHandler(parser, dummy_notation_decl_handler);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    if (dummy_handler_flags != DUMMY_NOTATION_DECL_HANDLER_FLAG)
        fail("Notation declaration handler not called");
}
END_TEST

static void XMLCALL
record_element_start_handler(void *userData,
                             const XML_Char *name,
                             const XML_Char **UNUSED_P(atts))
{
    CharData_AppendString((CharData *)userData, name);
}

START_TEST(test_nested_groups)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "<!ELEMENT doc "
        /* Sixteen elements per line */
        "(e,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,"
        "(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?"
        "))))))))))))))))))))))))))))))))>\n"
        "<!ELEMENT e EMPTY>"
        "]>\n"
        "<doc><e/></doc>";
    CharData storage;

    CharData_Init(&storage);
    XML_SetElementDeclHandler(parser, dummy_element_decl_handler);
    XML_SetStartElementHandler(parser, record_element_start_handler);
    XML_SetUserData(parser, &storage);
    dummy_handler_flags = 0;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    CharData_CheckString(&storage, "doce");
    if (dummy_handler_flags != DUMMY_ELEMENT_DECL_HANDLER_FLAG)
        fail("Element handler not fired");
}
END_TEST

START_TEST(test_group_choice)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "<!ELEMENT doc (a|b|c)+>\n"
        "<!ELEMENT a EMPTY>\n"
        "<!ELEMENT b (#PCDATA)>\n"
        "<!ELEMENT c ANY>\n"
        "]>\n"
        "<doc>\n"
        "<a/>\n"
        "<b attr='foo'>This is a foo</b>\n"
        "<c></c>\n"
        "</doc>\n";

    XML_SetElementDeclHandler(parser, dummy_element_decl_handler);
    dummy_handler_flags = 0;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    if (dummy_handler_flags != DUMMY_ELEMENT_DECL_HANDLER_FLAG)
        fail("Element handler flag not raised");
}
END_TEST

static int XMLCALL
external_entity_public(XML_Parser parser,
                       const XML_Char *context,
                       const XML_Char *UNUSED_P(base),
                       const XML_Char *systemId,
                       const XML_Char *publicId)
{
    const char *text1 = (const char *)XML_GetUserData(parser);
    const char *text2 = "<!ATTLIST doc a CDATA 'value'>";
    const char *text = NULL;
    XML_Parser ext_parser;

    ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (ext_parser == NULL)
        return XML_STATUS_ERROR;
    if (systemId != NULL && !strcmp(systemId, "http://example.org/")) {
        text = text1;
    }
    else if (publicId != NULL && !strcmp(publicId, "foo")) {
        text = text2;
    }
    else
        fail("Unexpected parameters to external entity parser");
    return _XML_Parse_SINGLE_BYTES(ext_parser, text, strlen(text),
                                   XML_TRUE);
}

START_TEST(test_standalone_parameter_entity)
{
    const char *text =
        "<?xml version='1.0' standalone='yes'?>\n"
        "<!DOCTYPE doc SYSTEM 'http://example.org/' [\n"
        "<!ENTITY % entity '<!ELEMENT doc (#PCDATA)>'>\n"
        "%entity;\n"
        "]>\n"
        "<doc></doc>";
    char dtd_data[] =
        "<!ENTITY % e1 'foo'>\n";

    XML_SetUserData(parser, dtd_data);
    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(parser, external_entity_public);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

/* Test skipping of parameter entity in an external DTD */
/* Derived from ibm/invalid/P69/ibm69i01.xml */
START_TEST(test_skipped_parameter_entity)
{
    const char *text =
        "<?xml version='1.0'?>\n"
        "<!DOCTYPE root SYSTEM 'http://example.org/dtd.ent' [\n"
        "<!ELEMENT root (#PCDATA|a)* >\n"
        "]>\n"
        "<root></root>";
    ExtTest dtd_data = { "%pe2;", NULL, NULL };

    XML_SetExternalEntityRefHandler(parser, external_entity_loader);
    XML_SetUserData(parser, &dtd_data);
    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetSkippedEntityHandler(parser, dummy_skip_handler);
    dummy_handler_flags = 0;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    if (dummy_handler_flags != DUMMY_SKIP_HANDLER_FLAG)
        fail("Skip handler not executed");
}
END_TEST

/* Test recursive parameter entity definition rejected in external DTD */
START_TEST(test_recursive_external_parameter_entity)
{
    const char *text =
        "<?xml version='1.0'?>\n"
        "<!DOCTYPE root SYSTEM 'http://example.org/dtd.ent' [\n"
        "<!ELEMENT root (#PCDATA|a)* >\n"
        "]>\n"
        "<root></root>";
    ExtFaults dtd_data = {
        "<!ENTITY % pe2 '&#37;pe2;'>\n%pe2;",
        "Recursive external parameter entity not faulted",
        NULL,
        XML_ERROR_RECURSIVE_ENTITY_REF
    };

    XML_SetExternalEntityRefHandler(parser, external_entity_faulter);
    XML_SetUserData(parser, &dtd_data);
    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    expect_failure(text,
                   XML_ERROR_EXTERNAL_ENTITY_HANDLING,
                   "Recursive external parameter not spotted");
}
END_TEST

/* Test undefined parameter entity in external entity handler */
static int XMLCALL
external_entity_devaluer(XML_Parser parser,
                         const XML_Char *context,
                         const XML_Char *UNUSED_P(base),
                         const XML_Char *systemId,
                         const XML_Char *UNUSED_P(publicId))
{
    const char *text =
        "<!ELEMENT doc EMPTY>\n"
        "<!ENTITY % e1 SYSTEM 'bar'>\n"
        "%e1;\n";
    XML_Parser ext_parser;
    int clear_handler = (intptr_t)XML_GetUserData(parser);

    if (systemId == NULL || !strcmp(systemId, "bar"))
        return XML_STATUS_OK;
    if (strcmp(systemId, "foo"))
        fail("Unexpected system ID");
    ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (ext_parser == NULL)
        fail("Could note create external entity parser");
    if (clear_handler)
        XML_SetExternalEntityRefHandler(ext_parser, NULL);
    if (_XML_Parse_SINGLE_BYTES(ext_parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(ext_parser);
    return XML_STATUS_OK;
}

START_TEST(test_undefined_ext_entity_in_external_dtd)
{
    const char *text =
        "<!DOCTYPE doc SYSTEM 'foo'>\n"
        "<doc></doc>\n";

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(parser, external_entity_devaluer);
    XML_SetUserData(parser, (void *)(intptr_t)XML_FALSE);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);

    /* Now repeat without the external entity ref handler invoking
     * another copy of itself.
     */
    XML_ParserReset(parser, NULL);
    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(parser, external_entity_devaluer);
    XML_SetUserData(parser, (void *)(intptr_t)XML_TRUE);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST


static void XMLCALL
aborting_xdecl_handler(void           *UNUSED_P(userData),
                       const XML_Char *UNUSED_P(version),
                       const XML_Char *UNUSED_P(encoding),
                       int             UNUSED_P(standalone))
{
    XML_StopParser(parser, resumable);
    XML_SetXmlDeclHandler(parser, NULL);
}

/* Test suspending the parse on receiving an XML declaration works */
START_TEST(test_suspend_xdecl)
{
    const char *text = long_character_data_text;

    XML_SetXmlDeclHandler(parser, aborting_xdecl_handler);
    resumable = XML_TRUE;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) != XML_STATUS_SUSPENDED)
        xml_failure(parser);
    if (XML_GetErrorCode(parser) != XML_ERROR_NONE)
        xml_failure(parser);
    /* Attempt to start a new parse while suspended */
    if (XML_Parse(parser, text, strlen(text), XML_TRUE) != XML_STATUS_ERROR)
        fail("Attempt to parse while suspended not faulted");
    if (XML_GetErrorCode(parser) != XML_ERROR_SUSPENDED)
        fail("Suspended parse not faulted with correct error");
}
END_TEST

/* Test aborting the parse in an epilog works */
static void XMLCALL
selective_aborting_default_handler(void *userData,
                                   const XML_Char *s,
                                   int len)
{
    const char *match = (const char *)userData;

    if (match == NULL ||
        (strlen(match) == (unsigned)len &&
         !strncmp(match, s, len))) {
        XML_StopParser(parser, resumable);
        XML_SetDefaultHandler(parser, NULL);
    }
}

START_TEST(test_abort_epilog)
{
    const char *text = "<doc></doc>\n\r\n";
    char match[] = "\r";

    XML_SetDefaultHandler(parser, selective_aborting_default_handler);
    XML_SetUserData(parser, match);
    resumable = XML_FALSE;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) != XML_STATUS_ERROR)
        fail("Abort not triggered");
    if (XML_GetErrorCode(parser) != XML_ERROR_ABORTED)
        xml_failure(parser);
}
END_TEST


/*
 * Namespaces tests.
 */

static void
namespace_setup(void)
{
    parser = XML_ParserCreateNS(NULL, ' ');
    if (parser == NULL)
        fail("Parser not created.");
}

static void
namespace_teardown(void)
{
    basic_teardown();
}

/* Check that an element name and attribute name match the expected values.
   The expected values are passed as an array reference of string pointers
   provided as the userData argument; the first is the expected
   element name, and the second is the expected attribute name.
*/
static int triplet_start_flag = XML_FALSE;
static int triplet_end_flag = XML_FALSE;

static void XMLCALL
triplet_start_checker(void *userData, const XML_Char *name,
                      const XML_Char **atts)
{
    char **elemstr = (char **)userData;
    char buffer[1024];
    if (strcmp(elemstr[0], name) != 0) {
        sprintf(buffer, "unexpected start string: '%s'", name);
        fail(buffer);
    }
    if (strcmp(elemstr[1], atts[0]) != 0) {
        sprintf(buffer, "unexpected attribute string: '%s'", atts[0]);
        fail(buffer);
    }
    triplet_start_flag = XML_TRUE;
}

/* Check that the element name passed to the end-element handler matches
   the expected value.  The expected value is passed as the first element
   in an array of strings passed as the userData argument.
*/
static void XMLCALL
triplet_end_checker(void *userData, const XML_Char *name)
{
    char **elemstr = (char **)userData;
    if (strcmp(elemstr[0], name) != 0) {
        char buffer[1024];
        sprintf(buffer, "unexpected end string: '%s'", name);
        fail(buffer);
    }
    triplet_end_flag = XML_TRUE;
}

START_TEST(test_return_ns_triplet)
{
    const char *text =
        "<foo:e xmlns:foo='http://example.org/' bar:a='12'\n"
        "       xmlns:bar='http://example.org/'>";
    const char *epilog = "</foo:e>";
    const char *elemstr[] = {
        "http://example.org/ e foo",
        "http://example.org/ a bar"
    };
    XML_SetReturnNSTriplet(parser, XML_TRUE);
    XML_SetUserData(parser, elemstr);
    XML_SetElementHandler(parser, triplet_start_checker,
                          triplet_end_checker);
    triplet_start_flag = XML_FALSE;
    triplet_end_flag = XML_FALSE;
    XML_SetNamespaceDeclHandler(parser,
                                dummy_start_namespace_decl_handler,
                                dummy_end_namespace_decl_handler);
    dummy_handler_flags = 0;
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_FALSE) == XML_STATUS_ERROR)
        xml_failure(parser);
    if (!triplet_start_flag)
        fail("triplet_start_checker not invoked");
    /* Check that unsetting "return triplets" fails while still parsing */
    XML_SetReturnNSTriplet(parser, XML_FALSE);
    if (_XML_Parse_SINGLE_BYTES(parser, epilog, strlen(epilog),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    if (!triplet_end_flag)
        fail("triplet_end_checker not invoked");
    if (dummy_handler_flags != (DUMMY_START_NS_DECL_HANDLER_FLAG |
                                DUMMY_END_NS_DECL_HANDLER_FLAG))
        fail("Namespace handlers not called");
}
END_TEST

static void XMLCALL
overwrite_start_checker(void *userData, const XML_Char *name,
                        const XML_Char **atts)
{
    CharData *storage = (CharData *) userData;
    CharData_AppendString(storage, "start ");
    CharData_AppendXMLChars(storage, name, -1);
    while (*atts != NULL) {
        CharData_AppendString(storage, "\nattribute ");
        CharData_AppendXMLChars(storage, *atts, -1);
        atts += 2;
    }
    CharData_AppendString(storage, "\n");
}

static void XMLCALL
overwrite_end_checker(void *userData, const XML_Char *name)
{
    CharData *storage = (CharData *) userData;
    CharData_AppendString(storage, "end ");
    CharData_AppendXMLChars(storage, name, -1);
    CharData_AppendString(storage, "\n");
}

static void
run_ns_tagname_overwrite_test(const char *text, const char *result)
{
    CharData storage;
    CharData_Init(&storage);
    XML_SetUserData(parser, &storage);
    XML_SetElementHandler(parser,
                          overwrite_start_checker, overwrite_end_checker);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
    CharData_CheckString(&storage, result);
}

/* Regression test for SF bug #566334. */
START_TEST(test_ns_tagname_overwrite)
{
    const char *text =
        "<n:e xmlns:n='http://example.org/'>\n"
        "  <n:f n:attr='foo'/>\n"
        "  <n:g n:attr2='bar'/>\n"
        "</n:e>";
    const char *result =
        "start http://example.org/ e\n"
        "start http://example.org/ f\n"
        "attribute http://example.org/ attr\n"
        "end http://example.org/ f\n"
        "start http://example.org/ g\n"
        "attribute http://example.org/ attr2\n"
        "end http://example.org/ g\n"
        "end http://example.org/ e\n";
    run_ns_tagname_overwrite_test(text, result);
}
END_TEST

/* Regression test for SF bug #566334. */
START_TEST(test_ns_tagname_overwrite_triplet)
{
    const char *text =
        "<n:e xmlns:n='http://example.org/'>\n"
        "  <n:f n:attr='foo'/>\n"
        "  <n:g n:attr2='bar'/>\n"
        "</n:e>";
    const char *result =
        "start http://example.org/ e n\n"
        "start http://example.org/ f n\n"
        "attribute http://example.org/ attr n\n"
        "end http://example.org/ f n\n"
        "start http://example.org/ g n\n"
        "attribute http://example.org/ attr2 n\n"
        "end http://example.org/ g n\n"
        "end http://example.org/ e n\n";
    XML_SetReturnNSTriplet(parser, XML_TRUE);
    run_ns_tagname_overwrite_test(text, result);
}
END_TEST


/* Regression test for SF bug #620343. */
static void XMLCALL
start_element_fail(void *UNUSED_P(userData),
                   const XML_Char *UNUSED_P(name), const XML_Char **UNUSED_P(atts))
{
    /* We should never get here. */
    fail("should never reach start_element_fail()");
}

static void XMLCALL
start_ns_clearing_start_element(void *userData,
                                const XML_Char *UNUSED_P(prefix),
                                const XML_Char *UNUSED_P(uri))
{
    XML_SetStartElementHandler((XML_Parser) userData, NULL);
}

START_TEST(test_start_ns_clears_start_element)
{
    /* This needs to use separate start/end tags; using the empty tag
       syntax doesn't cause the problematic path through Expat to be
       taken.
    */
    const char *text = "<e xmlns='http://example.org/'></e>";

    XML_SetStartElementHandler(parser, start_element_fail);
    XML_SetStartNamespaceDeclHandler(parser, start_ns_clearing_start_element);
    XML_SetEndNamespaceDeclHandler(parser, dummy_end_namespace_decl_handler);
    XML_UseParserAsHandlerArg(parser);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

/* Regression test for SF bug #616863. */
static int XMLCALL
external_entity_handler(XML_Parser parser,
                        const XML_Char *context,
                        const XML_Char *UNUSED_P(base),
                        const XML_Char *UNUSED_P(systemId),
                        const XML_Char *UNUSED_P(publicId))
{
    intptr_t callno = 1 + (intptr_t)XML_GetUserData(parser);
    const char *text;
    XML_Parser p2;

    if (callno == 1)
        text = ("<!ELEMENT doc (e+)>\n"
                "<!ATTLIST doc xmlns CDATA #IMPLIED>\n"
                "<!ELEMENT e EMPTY>\n");
    else
        text = ("<?xml version='1.0' encoding='us-ascii'?>"
                "<e/>");

    XML_SetUserData(parser, (void *) callno);
    p2 = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (_XML_Parse_SINGLE_BYTES(p2, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR) {
        xml_failure(p2);
        return XML_STATUS_ERROR;
    }
    XML_ParserFree(p2);
    return XML_STATUS_OK;
}

START_TEST(test_default_ns_from_ext_subset_and_ext_ge)
{
    const char *text =
        "<?xml version='1.0'?>\n"
        "<!DOCTYPE doc SYSTEM 'http://example.org/doc.dtd' [\n"
        "  <!ENTITY en SYSTEM 'http://example.org/entity.ent'>\n"
        "]>\n"
        "<doc xmlns='http://example.org/ns1'>\n"
        "&en;\n"
        "</doc>";

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(parser, external_entity_handler);
    /* We actually need to set this handler to tickle this bug. */
    XML_SetStartElementHandler(parser, dummy_start_element);
    XML_SetUserData(parser, NULL);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

/* Regression test #1 for SF bug #673791. */
START_TEST(test_ns_prefix_with_empty_uri_1)
{
    const char *text =
        "<doc xmlns:prefix='http://example.org/'>\n"
        "  <e xmlns:prefix=''/>\n"
        "</doc>";

    expect_failure(text,
                   XML_ERROR_UNDECLARING_PREFIX,
                   "Did not report re-setting namespace"
                   " URI with prefix to ''.");
}
END_TEST

/* Regression test #2 for SF bug #673791. */
START_TEST(test_ns_prefix_with_empty_uri_2)
{
    const char *text =
        "<?xml version='1.0'?>\n"
        "<docelem xmlns:pre=''/>";

    expect_failure(text,
                   XML_ERROR_UNDECLARING_PREFIX,
                   "Did not report setting namespace URI with prefix to ''.");
}
END_TEST

/* Regression test #3 for SF bug #673791. */
START_TEST(test_ns_prefix_with_empty_uri_3)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "  <!ELEMENT doc EMPTY>\n"
        "  <!ATTLIST doc\n"
        "    xmlns:prefix CDATA ''>\n"
        "]>\n"
        "<doc/>";

    expect_failure(text,
                   XML_ERROR_UNDECLARING_PREFIX,
                   "Didn't report attr default setting NS w/ prefix to ''.");
}
END_TEST

/* Regression test #4 for SF bug #673791. */
START_TEST(test_ns_prefix_with_empty_uri_4)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "  <!ELEMENT prefix:doc EMPTY>\n"
        "  <!ATTLIST prefix:doc\n"
        "    xmlns:prefix CDATA 'http://example.org/'>\n"
        "]>\n"
        "<prefix:doc/>";
    /* Packaged info expected by the end element handler;
       the weird structuring lets us re-use the triplet_end_checker()
       function also used for another test. */
    const char *elemstr[] = {
        "http://example.org/ doc prefix"
    };
    XML_SetReturnNSTriplet(parser, XML_TRUE);
    XML_SetUserData(parser, elemstr);
    XML_SetEndElementHandler(parser, triplet_end_checker);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

/* Test with non-xmlns prefix */
START_TEST(test_ns_unbound_prefix)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "  <!ELEMENT prefix:doc EMPTY>\n"
        "  <!ATTLIST prefix:doc\n"
        "    notxmlns:prefix CDATA 'http://example.org/'>\n"
        "]>\n"
        "<prefix:doc/>";

    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) != XML_STATUS_ERROR)
        fail("Unbound prefix incorrectly passed");
    if (XML_GetErrorCode(parser) != XML_ERROR_UNBOUND_PREFIX)
        xml_failure(parser);
}
END_TEST

START_TEST(test_ns_default_with_empty_uri)
{
    const char *text =
        "<doc xmlns='http://example.org/'>\n"
        "  <e xmlns=''/>\n"
        "</doc>";
    /* Add some handlers to exercise extra code paths */
    XML_SetStartNamespaceDeclHandler(parser,
                                     dummy_start_namespace_decl_handler);
    XML_SetEndNamespaceDeclHandler(parser,
                                   dummy_end_namespace_decl_handler);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

/* Regression test for SF bug #692964: two prefixes for one namespace. */
START_TEST(test_ns_duplicate_attrs_diff_prefixes)
{
    const char *text =
        "<doc xmlns:a='http://example.org/a'\n"
        "     xmlns:b='http://example.org/a'\n"
        "     a:a='v' b:a='v' />";
    expect_failure(text,
                   XML_ERROR_DUPLICATE_ATTRIBUTE,
                   "did not report multiple attributes with same URI+name");
}
END_TEST

START_TEST(test_ns_duplicate_hashes)
{
    /* The hash of an attribute is calculated as the hash of its URI
     * concatenated with a space followed by its name (after the
     * colon).  We wish to generate attributes with the same hash
     * value modulo the attribute table size so that we can check that
     * the attribute hash table works correctly.  The attribute hash
     * table size will be the smallest power of two greater than the
     * number of attributes, but at least eight.  There is
     * unfortunately no programmatic way of getting the hash or the
     * table size at user level, but the test code coverage percentage
     * will drop if the hashes cease to point to the same row.
     *
     * The cunning plan is to have few enough attributes to have a
     * reliable table size of 8, and have the single letter attribute
     * names be 8 characters apart, producing a hash which will be the
     * same modulo 8.
     */
    const char *text =
        "<doc xmlns:a='http://example.org/a'\n"
        "     a:a='v' a:i='w' />";
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

/* Regression test for SF bug #695401: unbound prefix. */
START_TEST(test_ns_unbound_prefix_on_attribute)
{
    const char *text = "<doc a:attr=''/>";
    expect_failure(text,
                   XML_ERROR_UNBOUND_PREFIX,
                   "did not report unbound prefix on attribute");
}
END_TEST

/* Regression test for SF bug #695401: unbound prefix. */
START_TEST(test_ns_unbound_prefix_on_element)
{
    const char *text = "<a:doc/>";
    expect_failure(text,
                   XML_ERROR_UNBOUND_PREFIX,
                   "did not report unbound prefix on element");
}
END_TEST

/* Test that the parsing status is correctly reset by XML_ParserReset().
 * We usE test_return_ns_triplet() for our example parse to improve
 * coverage of tidying up code executed.
 */
START_TEST(test_ns_parser_reset)
{
    XML_ParsingStatus status;

    XML_GetParsingStatus(parser, &status);
    if (status.parsing != XML_INITIALIZED)
        fail("parsing status doesn't start INITIALIZED");
    test_return_ns_triplet();
    XML_GetParsingStatus(parser, &status);
    if (status.parsing != XML_FINISHED)
        fail("parsing status doesn't end FINISHED");
    XML_ParserReset(parser, NULL);
    XML_GetParsingStatus(parser, &status);
    if (status.parsing != XML_INITIALIZED)
        fail("parsing status doesn't reset to INITIALIZED");
}
END_TEST

/* Test that long element names with namespaces are handled correctly */
START_TEST(test_ns_long_element)
{
    const char *text =
        "<foo:thisisalongenoughelementnametotriggerareallocation\n"
        " xmlns:foo='http://example.org/' bar:a='12'\n"
        " xmlns:bar='http://example.org/'>"
        "</foo:thisisalongenoughelementnametotriggerareallocation>";
    const char *elemstr[] = {
        "http://example.org/"
        " thisisalongenoughelementnametotriggerareallocation foo",
        "http://example.org/ a bar"
    };

    XML_SetReturnNSTriplet(parser, XML_TRUE);
    XML_SetUserData(parser, elemstr);
    XML_SetElementHandler(parser,
                          triplet_start_checker,
                          triplet_end_checker);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

/* Test mixed population of prefixed and unprefixed attributes */
START_TEST(test_ns_mixed_prefix_atts)
{
    const char *text =
        "<e a='12' bar:b='13'\n"
        " xmlns:bar='http://example.org/'>"
        "</e>";

    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

/* Test having a long namespaced element name inside a short one.
 * This exercises some internal buffer reallocation that is shared
 * across elements with the same namespace URI.
 */
START_TEST(test_ns_extend_uri_buffer)
{
    const char *text =
        "<foo:e xmlns:foo='http://example.org/'>"
        " <foo:thisisalongenoughnametotriggerallocationaction"
        "   foo:a='12' />"
        "</foo:e>";
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

/* Test that xmlns is correctly rejected as an attribute in the xmlns
 * namespace, but not in other namespaces
 */
START_TEST(test_ns_reserved_attributes)
{
    const char *text1 =
        "<foo:e xmlns:foo='http://example.org/' xmlns:xmlns='12' />";
    const char *text2 =
        "<foo:e xmlns:foo='http://example.org/' foo:xmlns='12' />";
    expect_failure(text1, XML_ERROR_RESERVED_PREFIX_XMLNS,
                   "xmlns not rejected as an attribute");
    XML_ParserReset(parser, NULL);
    if (_XML_Parse_SINGLE_BYTES(parser, text2, strlen(text2),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST

/* Test more reserved attributes */
START_TEST(test_ns_reserved_attributes_2)
{
    const char *text1 =
        "<foo:e xmlns:foo='http://example.org/'"
        "  xmlns:xml='http://example.org/' />";
    const char *text2 =
        "<foo:e xmlns:foo='http://www.w3.org/XML/1998/namespace' />";
    const char *text3 =
        "<foo:e xmlns:foo='http://www.w3.org/2000/xmlns/' />";

    expect_failure(text1, XML_ERROR_RESERVED_PREFIX_XML,
                   "xml not rejected as an attribute");
    XML_ParserReset(parser, NULL);
    expect_failure(text2, XML_ERROR_RESERVED_NAMESPACE_URI,
                   "Use of w3.org URL not faulted");
    XML_ParserReset(parser, NULL);
    expect_failure(text3, XML_ERROR_RESERVED_NAMESPACE_URI,
                   "Use of w3.org xmlns URL not faulted");
}
END_TEST

/* Control variable; the number of times duff_allocator() will successfully allocate */
#define ALLOC_ALWAYS_SUCCEED (-1)
#define REALLOC_ALWAYS_SUCCEED (-1)

static int allocation_count = ALLOC_ALWAYS_SUCCEED;
static int reallocation_count = REALLOC_ALWAYS_SUCCEED;

/* Crocked allocator for allocation failure tests */
static void *duff_allocator(size_t size)
{
    if (allocation_count == 0)
        return NULL;
    if (allocation_count != ALLOC_ALWAYS_SUCCEED)
        allocation_count--;
    return malloc(size);
}

/* Crocked reallocator for allocation failure tests */
static void *duff_reallocator(void *ptr, size_t size)
{
    if (reallocation_count == 0)
        return NULL;
    if (reallocation_count != REALLOC_ALWAYS_SUCCEED)
        reallocation_count--;
    return realloc(ptr, size);
}

/* Test that a failure to allocate the parser structure fails gracefully */
START_TEST(test_misc_alloc_create_parser)
{
    XML_Memory_Handling_Suite memsuite = { duff_allocator, realloc, free };
    unsigned int i;

    /* Something this simple shouldn't need more than 10 allocations */
    for (i = 0; i < 10; i++)
    {
        allocation_count = i;
        parser = XML_ParserCreate_MM(NULL, &memsuite, NULL);
        if (parser != NULL)
            break;
    }
    if (i == 0)
        fail("Parser unexpectedly ignored failing allocator");
    else if (i == 10)
        fail("Parser not created with allocation count 10");
}
END_TEST

/* Test memory allocation failures for a parser with an encoding */
START_TEST(test_misc_alloc_create_parser_with_encoding)
{
    XML_Memory_Handling_Suite memsuite = { duff_allocator, realloc, free };
    unsigned int i;

    /* Try several levels of allocation */
    for (i = 0; i < 10; i++) {
        allocation_count = i;
        parser = XML_ParserCreate_MM("us-ascii", &memsuite, NULL);
        if (parser != NULL)
            break;
    }
    if (i == 0)
        fail("Parser ignored failing allocator");
    else if (i == 10)
        fail("Parser not created with allocation count 10");
}
END_TEST

/* Test that freeing a NULL parser doesn't cause an explosion.
 * (Not actually tested anywhere else)
 */
START_TEST(test_misc_null_parser)
{
    /* NULL the variable, otherwise the teardown code will attempt
     * to free the previous parser again, and things will go downhill
     * rapidly.
     */
    parser = NULL;
    XML_ParserFree(NULL);
}
END_TEST

/* Test that XML_ErrorString rejects out-of-range codes */
START_TEST(test_misc_error_string)
{
    parser = NULL; /* Ensure no double-freeing occurs */
    if (XML_ErrorString((enum XML_Error)-1) != NULL)
        fail("Negative error code not rejected");
    if (XML_ErrorString((enum XML_Error)100) != NULL)
        fail("Large error code not rejected");
}
END_TEST

/* Test the version information is consistent */

/* Since we are working in XML_LChars (potentially 16-bits), we
 * can't use the standard C library functions for character
 * manipulation and have to roll our own.
 */
static int
parse_version(const XML_LChar *version_text,
              XML_Expat_Version *version_struct)
{
    while (*version_text != 0x00) {
        if (*version_text >= ASCII_0 && *version_text <= ASCII_9)
            break;
        version_text++;
    }
    if (*version_text == 0x00)
        return XML_FALSE;

    /* version_struct->major = strtoul(version_text, 10, &version_text) */
    version_struct->major = 0;
    while (*version_text >= ASCII_0 && *version_text <= ASCII_9) {
        version_struct->major =
            10 * version_struct->major + (*version_text++ - ASCII_0);
    }
    if (*version_text++ != ASCII_PERIOD)
        return XML_FALSE;

    /* Now for the minor version number */
    version_struct->minor = 0;
    while (*version_text >= ASCII_0 && *version_text <= ASCII_9) {
        version_struct->minor =
            10 * version_struct->minor + (*version_text++ - ASCII_0);
    }
    if (*version_text++ != ASCII_PERIOD)
        return XML_FALSE;

    /* Finally the micro version number */
    version_struct->micro = 0;
    while (*version_text >= ASCII_0 && *version_text <= ASCII_9) {
        version_struct->micro =
            10 * version_struct->micro + (*version_text++ - ASCII_0);
    }
    if (*version_text != 0x00)
        return XML_FALSE;
    return XML_TRUE;
}

static int
versions_equal(const XML_Expat_Version *first,
               const XML_Expat_Version *second)
{
    return (first->major == second->major &&
            first->minor == second->minor &&
            first->micro == second->micro);
}

START_TEST(test_misc_version)
{
    XML_Expat_Version read_version = XML_ExpatVersionInfo();
    XML_Expat_Version parsed_version;
    const XML_LChar *version_text = XML_ExpatVersion();

    parser = NULL; /* Ensure no double-freeing occurs */
    if (version_text == NULL)
        fail("Could not obtain version text");
    if (!parse_version(version_text, &parsed_version))
        fail("Unable to parse version text");
    if (!versions_equal(&read_version, &parsed_version))
        fail("Version mismatch");
}
END_TEST

/* Test feature information */
START_TEST(test_misc_features)
{
    const XML_Feature *features = XML_GetFeatureList();

    /* Prevent problems with double-freeing parsers */
    parser = NULL;
    if (features == NULL)
        fail("Failed to get feature information");
    /* Loop through the features checking what we can */
    while (features->feature != XML_FEATURE_END) {
        switch(features->feature) {
            case XML_FEATURE_SIZEOF_XML_CHAR:
                if (features->value != sizeof(XML_Char))
                    fail("Incorrect size of XML_Char");
                break;
            case XML_FEATURE_SIZEOF_XML_LCHAR:
                if (features->value != sizeof(XML_LChar))
                    fail("Incorrect size of XML_LChar");
                break;
            default:
                break;
        }
        features++;
    }
}
END_TEST


static void
alloc_setup(void)
{
    XML_Memory_Handling_Suite memsuite = {
        duff_allocator,
        duff_reallocator,
        free
    };

    /* Ensure the parser creation will go through */
    allocation_count = ALLOC_ALWAYS_SUCCEED;
    reallocation_count = REALLOC_ALWAYS_SUCCEED;
    parser = XML_ParserCreate_MM(NULL, &memsuite, NULL);
    if (parser == NULL)
        fail("Parser not created");
}

static void
alloc_teardown(void)
{
    basic_teardown();
}


/* Test the effects of allocation failures on xml declaration processing */
START_TEST(test_alloc_parse_xdecl)
{
    const char *text =
        "<?xml version='1.0' encoding='utf-8'?>\n"
        "<doc>Hello, world</doc>";
    int i;
    int repeat = 0;
#define MAX_ALLOC_COUNT 10

    for (i = 0; i < MAX_ALLOC_COUNT; i++) {
        /* Repeat some (most) counts to defeat cached allocations */
        if (i == 2 && repeat != 1) {
            i--;
            repeat++;
        }
        else if (i == 3) {
            i -= 2;
            repeat++;
        }
        allocation_count = i;
        XML_SetXmlDeclHandler(parser, dummy_xdecl_handler);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parse succeeded despite failing allocator");
    if (i == MAX_ALLOC_COUNT)
        fail("Parse failed with max allocations");
}
#undef MAX_ALLOC_COUNT
END_TEST

/* As above, but with an encoding big enough to cause storing the
 * version information to expand the string pool being used.
 */
static int XMLCALL
long_encoding_handler(void *UNUSED_P(userData),
                      const XML_Char *UNUSED_P(encoding),
                      XML_Encoding *info)
{
    int i;

    for (i = 0; i < 256; i++)
        info->map[i] = i;
    info->data = NULL;
    info->convert = NULL;
    info->release = NULL;
    return XML_STATUS_OK;
}

START_TEST(test_alloc_parse_xdecl_2)
{
    const char *text =
        "<?xml version='1.0' encoding='"
        /* Each line is 64 characters */
        "ThisIsAStupidlyLongEncodingNameIntendedToTriggerPoolGrowth123456"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMN"
        "'?>"
        "<doc>Hello, world</doc>";
    int i;
    int repeat = 0;
#define MAX_ALLOC_COUNT 10

    for (i = 0; i < MAX_ALLOC_COUNT; i++) {
        /* Repeat counts to defeat cached allocations */
        if (i == 4 && repeat == 3) {
            i -= 2;
            repeat++;
        }
        else if ((i == 2 && repeat < 3) ||
                 (i == 3 && repeat > 3)) {
            i--;
            repeat++;
        }
        allocation_count = i;
        XML_SetXmlDeclHandler(parser, dummy_xdecl_handler);
        XML_SetUnknownEncodingHandler(parser, long_encoding_handler, NULL);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parse succeeded despite failing allocator");
    if (i == MAX_ALLOC_COUNT)
        fail("Parse failed with max allocations");
}
#undef MAX_ALLOC_COUNT
END_TEST

/* Test the effects of allocation failures on a straightforward parse */
START_TEST(test_alloc_parse_pi)
{
    const char *text =
        "<?xml version='1.0' encoding='utf-8'?>\n"
        "<?pi unknown?>\n"
        "<doc>"
        "Hello, world"
        "</doc>";
    int i;
    int repeat = 0;
#define MAX_ALLOC_COUNT 10

    for (i = 0; i < MAX_ALLOC_COUNT; i++) {
        allocation_count = i;
        /* Repeat some counts because of cached memory */
        if (i == 2 && repeat == 2) {
            i -= 2;
            repeat++;
        } else if ((i == 1 && repeat < 2) ||
                   (i == 1 && repeat > 2 && repeat < 5)) {
            i--;
            repeat++;
        }
        XML_SetProcessingInstructionHandler(parser, dummy_pi_handler);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parse succeeded despite failing allocator");
    if (i == MAX_ALLOC_COUNT)
        fail("Parse failed with max allocations");
#undef MAX_ALLOC_COUNT
}
END_TEST

START_TEST(test_alloc_parse_pi_2)
{
    const char *text =
        "<?xml version='1.0' encoding='utf-8'?>\n"
        "<doc>"
        "Hello, world"
        "<?pi unknown?>\n"
        "</doc>";
    int i;
    int repeat = 0;
#define MAX_ALLOC_COUNT 10

    for (i = 0; i < MAX_ALLOC_COUNT; i++) {
        allocation_count = i;
        /* Repeat some counts because of cached memory */
        if (i == 2 && repeat == 1) {
            i -= 2;
            repeat++;
        } else if ((i == 1 && repeat < 1) ||
                   (i == 1 && repeat > 1 && repeat < 4)) {
            i--;
            repeat++;
        }
        XML_SetProcessingInstructionHandler(parser, dummy_pi_handler);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parse succeeded despite failing allocator");
    if (i == MAX_ALLOC_COUNT)
        fail("Parse failed with max allocations");
#undef MAX_ALLOC_COUNT
}
END_TEST

START_TEST(test_alloc_parse_comment)
{
    const char *text =
        "<?xml version='1.0' encoding='utf-8'?>\n"
        "<!-- Test parsing this comment -->"
        "<doc>Hi</doc>";
    int i;
    int repeat = 0;
#define MAX_ALLOC_COUNT 10

    for (i = 0; i < MAX_ALLOC_COUNT; i++) {
        /* Repeat some counts because of cached memory */
        if (i == 2 && repeat == 2) {
            i -= 2;
            repeat++;
        } else if ((i == 1 && repeat < 2) ||
                   (i == 1 && repeat > 2 && repeat < 5)) {
            i--;
            repeat++;
        }
        allocation_count = i;
        XML_SetCommentHandler(parser, dummy_comment_handler);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parse succeeded despite failing allocator");
    if (i == MAX_ALLOC_COUNT)
        fail("Parse failed with max allocations");
#undef MAX_ALLOC_COUNT
}
END_TEST

START_TEST(test_alloc_parse_comment_2)
{
    const char *text =
        "<?xml version='1.0' encoding='utf-8'?>\n"
        "<doc>"
        "Hello, world"
        "<!-- Parse this comment too -->"
        "</doc>";
    int i;
    int repeat = 0;
#define MAX_ALLOC_COUNT 10

    for (i = 0; i < MAX_ALLOC_COUNT; i++) {
        allocation_count = i;
        /* Repeat some counts because of cached memory */
        if (i == 2 && repeat == 1) {
            i -= 2;
            repeat++;
        } else if ((i == 1 && repeat < 1) ||
                   (i == 1 && repeat > 1 && repeat < 4)) {
            i--;
            repeat++;
        }
        XML_SetCommentHandler(parser, dummy_comment_handler);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parse succeeded despite failing allocator");
    if (i == MAX_ALLOC_COUNT)
        fail("Parse failed with max allocations");
#undef MAX_ALLOC_COUNT
}
END_TEST

static int XMLCALL
external_entity_duff_loader(XML_Parser parser,
                            const XML_Char *context,
                            const XML_Char *UNUSED_P(base),
                            const XML_Char *UNUSED_P(systemId),
                            const XML_Char *UNUSED_P(publicId))
{
    XML_Parser new_parser;
    unsigned int i;

    /* Try a few different allocation levels */
    for (i = 0; i < 10; i++)
    {
        allocation_count = i;
        new_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
        if (new_parser != NULL)
        {
            XML_ParserFree(new_parser);
            break;
        }
    }
    if (i == 0)
        fail("External parser creation ignored failing allocator");
    else if (i == 10)
        fail("Extern parser not created with allocation count 10");

    /* Make sure other random allocation doesn't now fail */
    allocation_count = ALLOC_ALWAYS_SUCCEED;

    /* Make sure the failure code path is executed too */
    return XML_STATUS_ERROR;
}

/* Test that external parser creation running out of memory is
 * correctly reported.  Based on the external entity test cases.
 */
START_TEST(test_alloc_create_external_parser)
{
    const char *text =
        "<?xml version='1.0' encoding='us-ascii'?>\n"
        "<!DOCTYPE doc SYSTEM 'foo'>\n"
        "<doc>&entity;</doc>";
    char foo_text[] =
        "<!ELEMENT doc (#PCDATA)*>";

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetUserData(parser, foo_text);
    XML_SetExternalEntityRefHandler(parser,
                                    external_entity_duff_loader);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) != XML_STATUS_ERROR) {
        fail("External parser allocator returned success incorrectly");
    }
}
END_TEST

/* More external parser memory allocation testing */
START_TEST(test_alloc_run_external_parser)
{
    const char *text =
        "<?xml version='1.0' encoding='us-ascii'?>\n"
        "<!DOCTYPE doc SYSTEM 'foo'>\n"
        "<doc>&entity;</doc>";
    char foo_text[] =
        "<!ELEMENT doc (#PCDATA)*>";
    unsigned int i;

    for (i = 0; i < 10; i++) {
        XML_SetParamEntityParsing(parser,
                                  XML_PARAM_ENTITY_PARSING_ALWAYS);
        XML_SetUserData(parser, foo_text);
        XML_SetExternalEntityRefHandler(parser,
                                        external_entity_null_loader);
        allocation_count = i;
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) != XML_STATUS_ERROR)
            break;
        /* Re-use the parser */
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parsing ignored failing allocator");
    else if (i == 10)
        fail("Parsing failed with allocation count 10");
}
END_TEST


static int XMLCALL
external_entity_dbl_handler(XML_Parser parser,
                            const XML_Char *context,
                            const XML_Char *UNUSED_P(base),
                            const XML_Char *UNUSED_P(systemId),
                            const XML_Char *UNUSED_P(publicId))
{
    intptr_t callno = (intptr_t)XML_GetUserData(parser);
    const char *text;
    XML_Parser new_parser;
    int i;

    if (callno == 0) {
        /* First time through, check how many calls to malloc occur */
        text = ("<!ELEMENT doc (e+)>\n"
                "<!ATTLIST doc xmlns CDATA #IMPLIED>\n"
                "<!ELEMENT e EMPTY>\n");
        allocation_count = 10000;
        new_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
        if (new_parser == NULL) {
            fail("Unable to allocate first external parser");
            return XML_STATUS_ERROR;
        }
        /* Stash the number of calls in the user data */
        XML_SetUserData(parser, (void *)(intptr_t)(10000 - allocation_count));
    } else {
        text = ("<?xml version='1.0' encoding='us-ascii'?>"
                "<e/>");
        /* Try at varying levels to exercise more code paths */
        for (i = 0; i < 20; i++) {
            allocation_count = callno + i;
            new_parser = XML_ExternalEntityParserCreate(parser,
                                                        context,
                                                        NULL);
            if (new_parser != NULL)
                break;
        }
        if (i == 0) {
            fail("Second external parser unexpectedly created");
            XML_ParserFree(new_parser);
            return XML_STATUS_ERROR;
        }
        else if (i == 20) {
            fail("Second external parser not created");
            return XML_STATUS_ERROR;
        }
    }

    allocation_count = ALLOC_ALWAYS_SUCCEED;
    if (_XML_Parse_SINGLE_BYTES(new_parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR) {
        xml_failure(new_parser);
        return XML_STATUS_ERROR;
    }
    XML_ParserFree(new_parser);
    return XML_STATUS_OK;
}

/* Test that running out of memory in dtdCopy is correctly reported.
 * Based on test_default_ns_from_ext_subset_and_ext_ge()
 */
START_TEST(test_alloc_dtd_copy_default_atts)
{
    const char *text =
        "<?xml version='1.0'?>\n"
        "<!DOCTYPE doc SYSTEM 'http://example.org/doc.dtd' [\n"
        "  <!ENTITY en SYSTEM 'http://example.org/entity.ent'>\n"
        "]>\n"
        "<doc xmlns='http://example.org/ns1'>\n"
        "&en;\n"
        "</doc>";

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(parser,
                                    external_entity_dbl_handler);
    XML_SetUserData(parser, NULL);
    if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);
}
END_TEST


static int XMLCALL
external_entity_dbl_handler_2(XML_Parser parser,
                              const XML_Char *context,
                              const XML_Char *UNUSED_P(base),
                              const XML_Char *UNUSED_P(systemId),
                              const XML_Char *UNUSED_P(publicId))
{
    intptr_t callno = (intptr_t)XML_GetUserData(parser);
    const char *text;
    XML_Parser new_parser;
    enum XML_Status rv;

    if (callno == 0) {
        /* Try different allocation levels for whole exercise */
        text = ("<!ELEMENT doc (e+)>\n"
                "<!ATTLIST doc xmlns CDATA #IMPLIED>\n"
                "<!ELEMENT e EMPTY>\n");
        XML_SetUserData(parser, (void *)(intptr_t)1);
        new_parser = XML_ExternalEntityParserCreate(parser,
                                                    context,
                                                    NULL);
        if (new_parser == NULL)
            return XML_STATUS_ERROR;
        rv = _XML_Parse_SINGLE_BYTES(new_parser, text, strlen(text),
                                     XML_TRUE);
    } else {
        /* Just run through once */
        text = ("<?xml version='1.0' encoding='us-ascii'?>"
                "<e/>");
        new_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
        if (new_parser == NULL)
            return XML_STATUS_ERROR;
        rv =_XML_Parse_SINGLE_BYTES(new_parser, text, strlen(text),
                                    XML_TRUE);
    }
    XML_ParserFree(new_parser);
    if (rv == XML_STATUS_ERROR)
        return XML_STATUS_ERROR;
    return XML_STATUS_OK;
}

/* Test more external entity allocation failure paths */
START_TEST(test_alloc_external_entity)
{
    const char *text =
        "<?xml version='1.0'?>\n"
        "<!DOCTYPE doc SYSTEM 'http://example.org/doc.dtd' [\n"
        "  <!ENTITY en SYSTEM 'http://example.org/entity.ent'>\n"
        "]>\n"
        "<doc xmlns='http://example.org/ns1'>\n"
        "&en;\n"
        "</doc>";
    int i;
    int repeat = 0;
#define ALLOC_TEST_MAX_REPEATS 50

    for (i = 0; i < ALLOC_TEST_MAX_REPEATS; i++) {
        /* Some allocation counts need to be repeated to follow all
         * the failure paths, given some allocations are cached.
         */
        if (i == 11 && repeat == 7) {
            i -= 2;
            repeat++;
        } else if ((i == 1 && repeat < 2) ||
                   (i == 2 && repeat < 4) ||
                   (repeat < 7 && i == repeat+2) ||
                   (i == 10 && repeat == 8)) {
            i--;
            repeat++;
        }
        allocation_count = -1;
        XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
        XML_SetExternalEntityRefHandler(parser,
                                        external_entity_dbl_handler_2);
        XML_SetUserData(parser, NULL);
        allocation_count = i;
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) == XML_STATUS_OK)
            break;
        XML_ParserReset(parser, NULL);
    }
    allocation_count = -1;
    if (i == 0)
        fail("External entity parsed despite duff allocator");
    if (i == ALLOC_TEST_MAX_REPEATS)
        fail("External entity not parsed at max allocation count");
#undef ALLOC_TEST_MAX_REPEATS
}
END_TEST

/* Test more allocation failure paths */
static int XMLCALL
external_entity_alloc_set_encoding(XML_Parser parser,
                                   const XML_Char *context,
                                   const XML_Char *UNUSED_P(base),
                                   const XML_Char *UNUSED_P(systemId),
                                   const XML_Char *UNUSED_P(publicId))
{
    /* As for external_entity_loader() */
    const char *text =
        "<?xml encoding='iso-8859-3'?>"
        "\xC3\xA9";
    XML_Parser ext_parser;
    enum XML_Status status;

    ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (ext_parser == NULL)
        return XML_STATUS_ERROR;
    if (!XML_SetEncoding(ext_parser, "utf-8")) {
        XML_ParserFree(ext_parser);
        return XML_STATUS_ERROR;
    }
    status = _XML_Parse_SINGLE_BYTES(ext_parser, text, strlen(text),
                                     XML_TRUE);
    XML_ParserFree(ext_parser);
    if (status == XML_STATUS_ERROR)
        return XML_STATUS_ERROR;
    return XML_STATUS_OK;
}

START_TEST(test_alloc_ext_entity_set_encoding)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
        "]>\n"
        "<doc>&en;</doc>";
    int i;
    int repeat = 0;
#define MAX_ALLOCATION_COUNT 20

    for (i = 0; i < MAX_ALLOCATION_COUNT; i++) {
        /* Repeat some counts to get round caching */
        if ((i == 2 && repeat < 3) ||
            (i == 3 && repeat < 6) ||
            (i == 4 && repeat == 6)) {
            i--;
            repeat++;
        }
        XML_SetExternalEntityRefHandler(parser,
                                        external_entity_alloc_set_encoding);
        allocation_count = i;
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) == XML_STATUS_OK)
            break;
        allocation_count = -1;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Encoding check succeeded despite failing allocator");
    if (i == MAX_ALLOCATION_COUNT)
        fail("Encoding failed at max allocation count");
#undef MAX_ALLOCATION_COUNT
}
END_TEST

static int XMLCALL
unknown_released_encoding_handler(void *UNUSED_P(data),
                                  const XML_Char *encoding,
                                  XML_Encoding *info)
{
    if (!strcmp(encoding, "unsupported-encoding")) {
        int i;

        for (i = 0; i < 256; i++)
            info->map[i] = i;
        info->data = NULL;
        info->convert = NULL;
        info->release = dummy_release;
        return XML_STATUS_OK;
    }
    return XML_STATUS_ERROR;
}

/* Test the effects of allocation failure in internal entities.
 * Based on test_unknown_encoding_internal_entity
 */
START_TEST(test_alloc_internal_entity)
{
    const char *text =
        "<?xml version='1.0' encoding='unsupported-encoding'?>\n"
        "<!DOCTYPE test [<!ENTITY foo 'bar'>]>\n"
        "<test a='&foo;'/>";
    unsigned int i;
    int repeated = 0;

    for (i = 0; i < 10; i++) {
        /* Again, repeat some counts to account for caching */
        if (repeated < 2 && i == 2) {
            i--;
            repeated++;
        }
        XML_SetUnknownEncodingHandler(parser,
                                      unknown_released_encoding_handler,
                                      NULL);
        allocation_count = i;
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text), XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Internal entity worked despite failing allocations");
    else if (i == 10)
        fail("Internal entity failed at allocation count 10");
}
END_TEST


/* Test the robustness against allocation failure of element handling
 * Based on test_dtd_default_handling().
 */
START_TEST(test_alloc_dtd_default_handling)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "<!ENTITY e SYSTEM 'http://example.org/e'>\n"
        "<!NOTATION n SYSTEM 'http://example.org/n'>\n"
        "<!ENTITY e1 SYSTEM 'http://example.org/e' NDATA n>\n"
        "<!ELEMENT doc (#PCDATA)>\n"
        "<!ATTLIST doc a CDATA #IMPLIED>\n"
        "<?pi in dtd?>\n"
        "<!--comment in dtd-->\n"
        "]>\n"
        "<doc><![CDATA[text in doc]]></doc>";
    const char *expected = "\n\n\n\n\n\n\n\n\n<doc>text in doc</doc>";
    CharData storage;
    int i;
#define MAX_ALLOC_COUNT 15
    int repeat = 0;

    for (i = 0; i < MAX_ALLOC_COUNT; i++) {
        /* Repeat some counts to catch cached allocations */
        if ((repeat < 4 && i == 2) ||
            (repeat == 4 && i == 4) ||
            (repeat == 5 && i == 5) ||
            (repeat == 6 && i == 8)) {
            i--;
            repeat++;
        }
        allocation_count = i;
        dummy_handler_flags = 0;
        XML_SetDefaultHandler(parser, accumulate_characters);
        XML_SetDoctypeDeclHandler(parser,
                                  dummy_start_doctype_handler,
                                  dummy_end_doctype_handler);
        XML_SetEntityDeclHandler(parser, dummy_entity_decl_handler);
        XML_SetNotationDeclHandler(parser, dummy_notation_decl_handler);
        XML_SetElementDeclHandler(parser, dummy_element_decl_handler);
        XML_SetAttlistDeclHandler(parser, dummy_attlist_decl_handler);
        XML_SetProcessingInstructionHandler(parser, dummy_pi_handler);
        XML_SetCommentHandler(parser, dummy_comment_handler);
        XML_SetCdataSectionHandler(parser,
                                   dummy_start_cdata_handler,
                                   dummy_end_cdata_handler);
        XML_SetUnparsedEntityDeclHandler(
            parser,
            dummy_unparsed_entity_decl_handler);
        CharData_Init(&storage);
        XML_SetUserData(parser, &storage);
        XML_SetCharacterDataHandler(parser, accumulate_characters);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Default DTD parsed despite allocation failures");
    if (i == MAX_ALLOC_COUNT)
        fail("Default DTD not parsed with maximum alloc count");
    CharData_CheckXMLChars(&storage, expected);
    if (dummy_handler_flags != (DUMMY_START_DOCTYPE_HANDLER_FLAG |
                                DUMMY_END_DOCTYPE_HANDLER_FLAG |
                                DUMMY_ENTITY_DECL_HANDLER_FLAG |
                                DUMMY_NOTATION_DECL_HANDLER_FLAG |
                                DUMMY_ELEMENT_DECL_HANDLER_FLAG |
                                DUMMY_ATTLIST_DECL_HANDLER_FLAG |
                                DUMMY_COMMENT_HANDLER_FLAG |
                                DUMMY_PI_HANDLER_FLAG |
                                DUMMY_START_CDATA_HANDLER_FLAG |
                                DUMMY_END_CDATA_HANDLER_FLAG |
                                DUMMY_UNPARSED_ENTITY_DECL_HANDLER_FLAG))
        fail("Not all handlers were called");
}
#undef MAX_ALLOC_COUNT
END_TEST

/* Test robustness of XML_SetEncoding() with a failing allocator */
START_TEST(test_alloc_explicit_encoding)
{
    int i;

    for (i = 0; i < 5; i++) {
        allocation_count = i;
        if (XML_SetEncoding(parser, "us-ascii") == XML_STATUS_OK)
            break;
    }
    if (i == 0)
        fail("Encoding set despite failing allocator");
    else if (i == 5)
        fail("Encoding not set at allocation count 5");
}
END_TEST

/* Test robustness of XML_SetBase against a failing allocator */
START_TEST(test_alloc_set_base)
{
    const XML_Char *new_base = "/local/file/name.xml";
    int i;

    for (i = 0; i < 5; i++) {
        allocation_count = i;
        if (XML_SetBase(parser, new_base) == XML_STATUS_OK)
            break;
    }
    if (i == 0)
        fail("Base set despite failing allocator");
    else if (i == 5)
        fail("Base not set with allocation count 5");
}
END_TEST

/* Test buffer extension in the face of a duff reallocator */
START_TEST(test_alloc_realloc_buffer)
{
    const char *text = get_buffer_test_text;
    void *buffer;
    int i;
    int repeat = 0;

    /* Get a smallish buffer */
    for (i = 0; i < 10; i++) {
        /* Repeat some indexes for cached allocations */
        if (repeat < 6 && i == 2) {
            i--;
            repeat++;
        }
        reallocation_count = i;
        buffer = XML_GetBuffer(parser, 1536);
        if (buffer == NULL)
            fail("1.5K buffer reallocation failed");
        memcpy(buffer, text, strlen(text));
        if (XML_ParseBuffer(parser, strlen(text),
                            XML_FALSE) == XML_STATUS_OK)
            break;
        XML_ParserReset(parser, NULL);
    }
    reallocation_count = -1;
    if (i == 0)
        fail("Parse succeeded with no reallocation");
    else if (i == 10)
        fail("Parse failed with reallocation count 10");
}
END_TEST

/* Same test for external entity parsers */
static int XMLCALL
external_entity_reallocator(XML_Parser parser,
                            const XML_Char *context,
                            const XML_Char *UNUSED_P(base),
                            const XML_Char *UNUSED_P(systemId),
                            const XML_Char *UNUSED_P(publicId))
{
    const char *text = get_buffer_test_text;
    XML_Parser ext_parser;
    void *buffer;
    enum XML_Status status;

    ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (ext_parser == NULL)
        fail("Could not create external entity parser");

    reallocation_count = (intptr_t)XML_GetUserData(parser);
    buffer = XML_GetBuffer(ext_parser, 1536);
    if (buffer == NULL)
        fail("Buffer allocation failed");
    memcpy(buffer, text, strlen(text));
    status = XML_ParseBuffer(ext_parser, strlen(text), XML_FALSE);
    reallocation_count = -1;
    XML_ParserFree(ext_parser);
    return (status == XML_STATUS_OK) ? XML_STATUS_OK : XML_STATUS_ERROR;
}

START_TEST(test_alloc_ext_entity_realloc_buffer)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
        "]>\n"
        "<doc>&en;</doc>";
    int i;

    for (i = 0; i < 10; i++) {
        XML_SetExternalEntityRefHandler(parser,
                                        external_entity_reallocator);
        XML_SetUserData(parser, (void *)(intptr_t)i);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) == XML_STATUS_OK)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Succeeded with no reallocations");
    if (i == 10)
        fail("Failed with 10 reallocations");
}
END_TEST

/* Test elements with many attributes are handled correctly */
START_TEST(test_alloc_realloc_many_attributes)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "<!ATTLIST doc za CDATA 'default'>\n"
        "<!ATTLIST doc zb CDATA 'def2'>\n"
        "<!ATTLIST doc zc CDATA 'def3'>\n"
        "]>\n"
        "<doc a='1'"
        "     b='2'"
        "     c='3'"
        "     d='4'"
        "     e='5'"
        "     f='6'"
        "     g='7'"
        "     h='8'"
        "     i='9'"
        "     j='10'"
        "     k='11'"
        "     l='12'"
        "     m='13'"
        "     n='14'"
        "     p='15'"
        "     q='16'"
        "     r='17'"
        "     s='18'>"
        "</doc>";
    int i;
#define MAX_REALLOC_COUNT 10

    for (i = 0; i < MAX_REALLOC_COUNT; i++) {
        reallocation_count = i;
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parse succeeded despite no reallocations");
    if (i == MAX_REALLOC_COUNT)
        fail("Parse failed at max reallocations");
#undef MAX_REALLOC_COUNT
}
END_TEST

/* Test handling of a public entity with failing allocator */
START_TEST(test_alloc_public_entity_value)
{
    const char *text =
        "<!DOCTYPE doc SYSTEM 'http://example.org/'>\n"
        "<doc></doc>\n";
    char dtd_text[] =
        "<!ELEMENT doc EMPTY>\n"
        "<!ENTITY % e1 PUBLIC 'foo' 'bar.ent'>\n"
        "<!ENTITY % "
        /* Each line is 64 characters */
        "ThisIsAStupidlyLongParameterNameIntendedToTriggerPoolGrowth12345"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        " '%e1;'>\n"
        "%e1;\n";
    int i;
#define MAX_ALLOC_COUNT 25
    int repeat = 0;

    for (i = 0; i < MAX_ALLOC_COUNT; i++) {
        /* Repeat certain counts to defeat cached allocations */
        if ((i == 2 && repeat < 2) ||
            (i == 3 && repeat == 2) ||
            (i == 8 && repeat == 3) ||
            (i == 9 && repeat == 4) ||
            (i == 18 && repeat == 5) ||
            (i == 19 && repeat == 6)) {
            i--;
            repeat++;
        }
        allocation_count = i;
        dummy_handler_flags = 0;
        XML_SetUserData(parser, dtd_text);
        XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
        XML_SetExternalEntityRefHandler(parser, external_entity_public);
        /* Provoke a particular code path */
        XML_SetEntityDeclHandler(parser, dummy_entity_decl_handler);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parsing worked despite failing allocation");
    if (i == MAX_ALLOC_COUNT)
        fail("Parsing failed at max allocation count");
    if (dummy_handler_flags != DUMMY_ENTITY_DECL_HANDLER_FLAG)
        fail("Entity declaration handler not called");
}
#undef MAX_ALLOC_COUNT
END_TEST

START_TEST(test_alloc_realloc_subst_public_entity_value)
{
    const char *text =
        "<!DOCTYPE doc SYSTEM 'http://example.org/'>\n"
        "<doc></doc>\n";
    char dtd_text[] =
        "<!ELEMENT doc EMPTY>\n"
        "<!ENTITY % "
        /* Each line is 64 characters */
        "ThisIsAStupidlyLongParameterNameIntendedToTriggerPoolGrowth12345"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        " PUBLIC 'foo' 'bar.ent'>\n"
        "%ThisIsAStupidlyLongParameterNameIntendedToTriggerPoolGrowth12345"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP;";
    int i;
#define MAX_REALLOC_COUNT 10

    for (i = 0; i < MAX_REALLOC_COUNT; i++) {
        reallocation_count = i;
        XML_SetUserData(parser, dtd_text);
        XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
        XML_SetExternalEntityRefHandler(parser, external_entity_public);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parsing worked despite failing reallocation");
    if (i == MAX_REALLOC_COUNT)
        fail("Parsing failed at max reallocation count");
}
#undef MAX_REALLOC_COUNT
END_TEST

START_TEST(test_alloc_parse_public_doctype)
{
    const char *text =
        "<?xml version='1.0' encoding='utf-8'?>\n"
        "<!DOCTYPE doc PUBLIC '"
        /* 64 characters per line */
        "http://example.com/a/long/enough/name/to/trigger/pool/growth/zz/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "' 'test'>\n"
        "<doc></doc>";
    int i;
#define MAX_ALLOC_COUNT 10
    int repeat = 0;

    for (i = 0; i < MAX_ALLOC_COUNT; i++) {
        /* Repeat certain counts to defeat cached allocations */
        if (i == 4 && repeat == 5) {
            i -= 2;
            repeat++;
        }
        else if ((i == 2 && repeat < 3) ||
            (i == 3 && repeat < 5)) {
            i--;
            repeat++;
        }
        allocation_count = i;
        dummy_handler_flags = 0;
        XML_SetDoctypeDeclHandler(parser,
                                  dummy_start_doctype_decl_handler,
                                  dummy_end_doctype_decl_handler);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parse succeeded despite failing allocator");
    if (i == MAX_ALLOC_COUNT)
        fail("Parse failed at maximum allocation count");
    if (dummy_handler_flags != (DUMMY_START_DOCTYPE_DECL_HANDLER_FLAG |
                                DUMMY_END_DOCTYPE_DECL_HANDLER_FLAG))
        fail("Doctype handler functions not called");
}
#undef MAX_ALLOC_COUNT
END_TEST

START_TEST(test_alloc_parse_public_doctype_long_name)
{
    const char *text =
        "<?xml version='1.0' encoding='utf-8'?>\n"
        "<!DOCTYPE doc PUBLIC 'http://example.com/foo' '"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "'>\n"
        "<doc></doc>";
    int i;
#define MAX_ALLOC_COUNT 10
    int repeat = 0;

    for (i = 0; i < MAX_ALLOC_COUNT; i++) {
        /* Repeat some counts to defeat cached allocations */
        if (i == 4 && repeat == 6) {
            i -= 2;
            repeat++;
        }
        else if ((i == 2 && repeat < 3) ||
                 (i == 3 && repeat < 6)) {
            i--;
            repeat++;
        }
        allocation_count = i;
        XML_SetDoctypeDeclHandler(parser,
                                  dummy_start_doctype_decl_handler,
                                  dummy_end_doctype_decl_handler);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parse succeeded despite failing allocator");
    if (i == MAX_ALLOC_COUNT)
        fail("Parse failed at maximum allocation count");
}
#undef MAX_ALLOC_COUNT
END_TEST

static int XMLCALL
external_entity_alloc(XML_Parser parser,
                      const XML_Char *context,
                      const XML_Char *UNUSED_P(base),
                      const XML_Char *UNUSED_P(systemId),
                      const XML_Char *UNUSED_P(publicId))
{
    const char *text = (const char *)XML_GetUserData(parser);
    XML_Parser ext_parser;

    ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (ext_parser == NULL)
        return XML_STATUS_ERROR;
    return _XML_Parse_SINGLE_BYTES(ext_parser, text, strlen(text), XML_TRUE);
}

/* Test foreign DTD handling */
START_TEST(test_alloc_set_foreign_dtd)
{
    const char *text1 =
        "<?xml version='1.0' encoding='us-ascii'?>\n"
        "<doc>&entity;</doc>";
    char text2[] = "<!ELEMENT doc (#PCDATA)*>";
    int i;
#define MAX_ALLOC_COUNT 10
    int repeat = 0;

    for (i = 0; i < MAX_ALLOC_COUNT; i++) {
        /* Repeat some counts to deal with cached allocations */
        if (i == 9 && repeat == 2) {
            i -= 2;
            repeat++;
        }
        else if (i == 2 && repeat < 2) {
            i--;
            repeat++;
        }
        allocation_count = i;
        XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
        XML_SetUserData(parser, &text2);
        XML_SetExternalEntityRefHandler(parser, external_entity_alloc);
        if (XML_UseForeignDTD(parser, XML_TRUE) != XML_ERROR_NONE)
            fail("Could not set foreign DTD");
        if (_XML_Parse_SINGLE_BYTES(parser, text1, strlen(text1),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parse succeeded despite failing allocator");
    if (i == MAX_ALLOC_COUNT)
        fail("Parse failed at maximum allocation count");
}
#undef MAX_ALLOC_COUNT
END_TEST

/* Test based on ibm/valid/P32/ibm32v04.xml */
START_TEST(test_alloc_attribute_enum_value)
{
    const char *text =
        "<?xml version='1.0' standalone='no'?>\n"
        "<!DOCTYPE animal SYSTEM 'test.dtd'>\n"
        "<animal>This is a \n    <a/>  \n\nyellow tiger</animal>";
    char dtd_text[] =
        "<!ELEMENT animal (#PCDATA|a)*>\n"
        "<!ELEMENT a EMPTY>\n"
        "<!ATTLIST animal xml:space (default|preserve) 'preserve'>";
    int i;
#define MAX_ALLOC_COUNT 20
    int repeat = 0;

    for (i = 0; i < MAX_ALLOC_COUNT; i++) {
        /* Repeat some counts to allow for cached allocations */
        if (i == 13 && repeat == 5) {
            i -= 2;
            repeat++;
        }
        else if ((i == 2 && repeat < 2) ||
                 (i == 3 && repeat == 2) ||
                 (i == 8 && repeat == 3) ||
                 (i == 9 && repeat == 4)) {
            i--;
            repeat++;
        }
        allocation_count = i;
        XML_SetExternalEntityRefHandler(parser, external_entity_alloc);
        XML_SetUserData(parser, dtd_text);
        XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
        /* An attribute list handler provokes a different code path */
        XML_SetAttlistDeclHandler(parser, dummy_attlist_decl_handler);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parse succeeded despite failing allocator");
    if (i == MAX_ALLOC_COUNT)
        fail("Parse failed at maximum allocation count");
}
#undef MAX_ALLOC_COUNT
END_TEST

/* Test attribute enums sufficient to overflow the string pool */
START_TEST(test_alloc_realloc_attribute_enum_value)
{
    const char *text =
        "<?xml version='1.0' standalone='no'?>\n"
        "<!DOCTYPE animal SYSTEM 'test.dtd'>\n"
        "<animal>This is a yellow tiger</animal>";
    /* We wish to define a collection of attribute enums that will
     * cause the string pool storing them to have to expand.  This
     * means more than 1024 bytes, including the parentheses and
     * separator bars.
     */
    char dtd_text[] =
        "<!ELEMENT animal (#PCDATA)*>\n"
        "<!ATTLIST animal thing "
        "(default"
        /* Each line is 64 characters */
        "|ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|BBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|CBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|DBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|EBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|FBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|GBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|HBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|IBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|JBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|KBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|LBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|MBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|NBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|OBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|PBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO)"
        " 'default'>";
    int i;
#define MAX_REALLOC_COUNT 10

    for (i = 0; i < MAX_REALLOC_COUNT; i++) {
        reallocation_count = i;
        XML_SetExternalEntityRefHandler(parser, external_entity_alloc);
        XML_SetUserData(parser, dtd_text);
        XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
        /* An attribute list handler provokes a different code path */
        XML_SetAttlistDeclHandler(parser, dummy_attlist_decl_handler);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parse succeeded despite failing reallocator");
    if (i == MAX_REALLOC_COUNT)
        fail("Parse failed at maximum reallocation count");
}
#undef MAX_REALLOC_COUNT
END_TEST

/* Test attribute enums in a #IMPLIED attribute forcing pool growth */
START_TEST(test_alloc_realloc_implied_attribute)
{
    /* Forcing this particular code path is a balancing act.  The
     * addition of the closing parenthesis and terminal NUL must be
     * what pushes the string of enums over the 1024-byte limit,
     * otherwise a different code path will pick up the realloc.
     */
    const char *text =
        "<!DOCTYPE doc [\n"
        "<!ELEMENT doc EMPTY>\n"
        "<!ATTLIST doc a "
        /* Each line is 64 characters */
        "(ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|BBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|CBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|DBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|EBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|FBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|GBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|HBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|IBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|JBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|KBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|LBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|MBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|NBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|OBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|PBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMN)"
        " #IMPLIED>\n"
        "]><doc/>";
    int i;
#define MAX_REALLOC_COUNT 10
    for (i = 0; i < MAX_REALLOC_COUNT; i++) {
        reallocation_count = i;
        XML_SetAttlistDeclHandler(parser, dummy_attlist_decl_handler);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parse succeeded despite failing reallocator");
    if (i == MAX_REALLOC_COUNT)
        fail("Parse failed at maximum reallocation count");
}
#undef MAX_REALLOC_COUNT
END_TEST

/* Test attribute enums in a defaulted attribute forcing pool growth */
START_TEST(test_alloc_realloc_default_attribute)
{
    /* Forcing this particular code path is a balancing act.  The
     * addition of the closing parenthesis and terminal NUL must be
     * what pushes the string of enums over the 1024-byte limit,
     * otherwise a different code path will pick up the realloc.
     */
    const char *text =
        "<!DOCTYPE doc [\n"
        "<!ELEMENT doc EMPTY>\n"
        "<!ATTLIST doc a "
        /* Each line is 64 characters */
        "(ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|BBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|CBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|DBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|EBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|FBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|GBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|HBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|IBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|JBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|KBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|LBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|MBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|NBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|OBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|PBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMN)"
        " 'ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO'"
        ">\n]><doc/>";
    int i;
#define MAX_REALLOC_COUNT 10

    for (i = 0; i < MAX_REALLOC_COUNT; i++) {
        reallocation_count = i;
        XML_SetAttlistDeclHandler(parser, dummy_attlist_decl_handler);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parse succeeded despite failing reallocator");
    if (i == MAX_REALLOC_COUNT)
        fail("Parse failed at maximum reallocation count");
}
#undef MAX_REALLOC_COUNT
END_TEST

/* Test long notation name with dodgy allocator */
START_TEST(test_alloc_notation)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "<!NOTATION "
        /* Each line is 64 characters */
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        " SYSTEM 'http://example.org/n'>\n"
        "<!ENTITY e SYSTEM 'http://example.org/e' NDATA "
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        ">\n"
        "<!ELEMENT doc EMPTY>\n"
        "]>\n<doc/>";
    int i;
#define MAX_ALLOC_COUNT 10
    int repeat = 0;

    for (i = 0; i < MAX_ALLOC_COUNT; i++) {
        /* Repeat some counts to allow for cached allocations */
        if ((i == 2 && repeat < 4) || (i == 3)) {
            i--;
            repeat++;
        }
        allocation_count = i;
        dummy_handler_flags = 0;
        XML_SetNotationDeclHandler(parser, dummy_notation_decl_handler);
        XML_SetEntityDeclHandler(parser, dummy_entity_decl_handler);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parse succeeded despite allocation failures");
    if (i == MAX_ALLOC_COUNT)
        fail("Parse failed at maximum allocation count");
    if (dummy_handler_flags != (DUMMY_ENTITY_DECL_HANDLER_FLAG |
                                DUMMY_NOTATION_DECL_HANDLER_FLAG))
        fail("Entity declaration handler not called");
}
#undef MAX_ALLOC_COUNT
END_TEST

/* Test public notation with dodgy allocator */
START_TEST(test_alloc_public_notation)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "<!NOTATION note PUBLIC '"
        /* 64 characters per line */
        "http://example.com/a/long/enough/name/to/trigger/pool/growth/zz/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "' 'foo'>\n"
        "<!ENTITY e SYSTEM 'http://example.com/e' NDATA note>\n"
        "<!ELEMENT doc EMPTY>\n"
        "]>\n<doc/>";
    int i;
#define MAX_ALLOC_COUNT 10
    int repeat = 0;

    for (i = 0; i < MAX_ALLOC_COUNT; i++) {
        /* Repeat some counts to allow for cached allocations */
        if ((i == 2 && repeat < 5) ||
            (i == 3 && repeat == 5)) {
            i--;
            repeat++;
        }
        allocation_count = i;
        dummy_handler_flags = 0;
        XML_SetNotationDeclHandler(parser, dummy_notation_decl_handler);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parse succeeded despite allocation failures");
    if (i == MAX_ALLOC_COUNT)
        fail("Parse failed at maximum allocation count");
    if (dummy_handler_flags != DUMMY_NOTATION_DECL_HANDLER_FLAG)
        fail("Notation handler not called");
}
#undef MAX_ALLOC_COUNT
END_TEST

/* Test public notation with dodgy allocator */
START_TEST(test_alloc_system_notation)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "<!NOTATION note SYSTEM '"
        /* 64 characters per line */
        "http://example.com/a/long/enough/name/to/trigger/pool/growth/zz/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "'>\n"
        "<!ENTITY e SYSTEM 'http://example.com/e' NDATA note>\n"
        "<!ELEMENT doc EMPTY>\n"
        "]>\n<doc/>";
    int i;
#define MAX_ALLOC_COUNT 10
    int repeat = 0;

    for (i = 0; i < MAX_ALLOC_COUNT; i++) {
        /* Repeat some counts to allow for cached allocations */
        if ((i == 2 && repeat < 5) ||
            (i == 3 && repeat == 5)) {
            i--;
            repeat++;
        }
        allocation_count = i;
        dummy_handler_flags = 0;
        XML_SetNotationDeclHandler(parser, dummy_notation_decl_handler);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parse succeeded despite allocation failures");
    if (i == MAX_ALLOC_COUNT)
        fail("Parse failed at maximum allocation count");
    if (dummy_handler_flags != DUMMY_NOTATION_DECL_HANDLER_FLAG)
        fail("Notation handler not called");
}
#undef MAX_ALLOC_COUNT
END_TEST

START_TEST(test_alloc_nested_groups)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "<!ELEMENT doc "
        /* Sixteen elements per line */
        "(e,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,"
        "(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?"
        "))))))))))))))))))))))))))))))))>\n"
        "<!ELEMENT e EMPTY>"
        "]>\n"
        "<doc><e/></doc>";
    CharData storage;
    int i;
#define MAX_ALLOC_COUNT 10
    int repeat = 0;

    for (i = 0; i < MAX_ALLOC_COUNT; i++) {
        /* Repeat some counts to allow for cached allocations */
        if ((i == 2 && repeat < 3) || (i == 3 && repeat == 3)) {
            i--;
            repeat++;
        }
        allocation_count = i;
        CharData_Init(&storage);
        XML_SetElementDeclHandler(parser, dummy_element_decl_handler);
        XML_SetStartElementHandler(parser, record_element_start_handler);
        XML_SetUserData(parser, &storage);
        dummy_handler_flags = 0;
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }

    if (i == 0)
        fail("Parse succeeded despite failing reallocator");
    if (i == MAX_ALLOC_COUNT)
        fail("Parse failed at maximum reallocation count");
    CharData_CheckString(&storage, "doce");
    if (dummy_handler_flags != DUMMY_ELEMENT_DECL_HANDLER_FLAG)
        fail("Element handler not fired");
}
#undef MAX_ALLOC_COUNT
END_TEST

START_TEST(test_alloc_realloc_nested_groups)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "<!ELEMENT doc "
        /* Sixteen elements per line */
        "(e,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,"
        "(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?"
        "))))))))))))))))))))))))))))))))>\n"
        "<!ELEMENT e EMPTY>"
        "]>\n"
        "<doc><e/></doc>";
    CharData storage;
    int i;
#define MAX_REALLOC_COUNT 10

    for (i = 0; i < MAX_REALLOC_COUNT; i++) {
        reallocation_count = i;
        CharData_Init(&storage);
        XML_SetElementDeclHandler(parser, dummy_element_decl_handler);
        XML_SetStartElementHandler(parser, record_element_start_handler);
        XML_SetUserData(parser, &storage);
        dummy_handler_flags = 0;
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }

    if (i == 0)
        fail("Parse succeeded despite failing reallocator");
    if (i == MAX_REALLOC_COUNT)
        fail("Parse failed at maximum reallocation count");
    CharData_CheckString(&storage, "doce");
    if (dummy_handler_flags != DUMMY_ELEMENT_DECL_HANDLER_FLAG)
        fail("Element handler not fired");
}
#undef MAX_REALLOC_COUNT
END_TEST

START_TEST(test_alloc_realloc_group_choice)
{
    const char *text =
        "<!DOCTYPE doc [\n"
        "<!ELEMENT doc ("
        "a1|a2|a3|a4|a5|a6|a7|a8|"
        "b1|b2|b3|b4|b5|b6|b7|b8|"
        "c1|c2|c3|c4|c5|c6|c7|c8|"
        "d1|d2|d3|d4|d5|d6|d7|d8|"
        "e1"
        ")+>\n"
        "]>\n"
        "<doc>\n"
        "<a1/>\n"
        "<b2 attr='foo'>This is a foo</b2>\n"
        "<c3></c3>\n"
        "</doc>\n";
    int i;
#define MAX_REALLOC_COUNT 10

    for (i = 0; i < MAX_REALLOC_COUNT; i++) {
        reallocation_count = i;
        XML_SetElementDeclHandler(parser, dummy_element_decl_handler);
        dummy_handler_flags = 0;
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parse succeeded despite failing reallocator");
    if (i == MAX_REALLOC_COUNT)
        fail("Parse failed at maximum reallocation count");
    if (dummy_handler_flags != DUMMY_ELEMENT_DECL_HANDLER_FLAG)
        fail("Element handler flag not raised");
}
#undef MAX_REALLOC_COUNT
END_TEST

START_TEST(test_alloc_pi_in_epilog)
{
    const char *text =
        "<doc></doc>\n"
        "<?pi in epilog?>";
    int i;
#define MAX_ALLOC_COUNT 10
    int repeat = 0;

    for (i = 0; i < MAX_ALLOC_COUNT; i++) {
        /* Repeat certain counts to allow for cached allocations */
        if (i == 3 && repeat == 1) {
            i -= 2;
            repeat++;
        }
        else if (i == 2 && repeat < 4 && repeat != 1) {
            i--;
            repeat++;
        }
        allocation_count = i;
        XML_SetProcessingInstructionHandler(parser, dummy_pi_handler);
        dummy_handler_flags = 0;
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parse completed despite failing allocator");
    if (i == MAX_ALLOC_COUNT)
        fail("Parse failed at maximum allocation count");
    if (dummy_handler_flags != DUMMY_PI_HANDLER_FLAG)
        fail("Processing instruction handler not invoked");
}
#undef MAX_ALLOC_COUNT
END_TEST

static void
nsalloc_setup(void)
{
    XML_Memory_Handling_Suite memsuite = {
        duff_allocator,
        duff_reallocator,
        free
    };
    XML_Char ns_sep[2] = { ' ', '\0' };

    /* Ensure the parser creation will go through */
    allocation_count = ALLOC_ALWAYS_SUCCEED;
    reallocation_count = REALLOC_ALWAYS_SUCCEED;
    parser = XML_ParserCreate_MM(NULL, &memsuite, ns_sep);
    if (parser == NULL)
        fail("Parser not created");
}

static void
nsalloc_teardown(void)
{
    basic_teardown();
}


/* Test the effects of allocation failure in simple namespace parsing.
 * Based on test_ns_default_with_empty_uri()
 */
START_TEST(test_nsalloc_xmlns)
{
    const char *text =
        "<doc xmlns='http://example.org/'>\n"
        "  <e xmlns=''/>\n"
        "</doc>";
    unsigned int i;
    int repeated = 0;

    for (i = 0; i < 10; i++) {
        /* Repeat some tests with the same allocation count to
         * catch cached allocations not freed by XML_ParserReset()
         */
        if ((i == 4 && repeated == 3) ||
            (i == 7 && repeated == 8)) {
            i -= 2;
            repeated++;
        }
        else if ((i == 2 && repeated < 2) ||
                 (i == 3 && repeated < 3) ||
                 (i == 3 && repeated > 3 && repeated < 7) ||
                 (i == 5 && repeated < 8)) {
            i--;
            repeated++;
        }
        allocation_count = i;
        /* Exercise more code paths with a default handler */
        XML_SetDefaultHandler(parser, dummy_default_handler);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parsing worked despite failing allocations");
    else if (i == 10)
        fail("Parsing failed even at allocation count 10");
}
END_TEST

/* Test XML_ParseBuffer interface with namespace and a dicky allocator */
START_TEST(test_nsalloc_parse_buffer)
{
    const char *text = "<doc>Hello</doc>";
    void *buffer;

    /* Try a parse before the start of the world */
    /* (Exercises new code path) */
    allocation_count = 0;
    if (XML_ParseBuffer(parser, 0, XML_FALSE) != XML_STATUS_ERROR)
        fail("Pre-init XML_ParseBuffer not faulted");
    if (XML_GetErrorCode(parser) != XML_ERROR_NO_MEMORY)
        fail("Pre-init XML_ParseBuffer faulted for wrong reason");

    /* Now with actual memory allocation */
    allocation_count = ALLOC_ALWAYS_SUCCEED;
    if (XML_ParseBuffer(parser, 0, XML_FALSE) != XML_STATUS_OK)
        xml_failure(parser);

    /* Check that resuming an unsuspended parser is faulted */
    if (XML_ResumeParser(parser) != XML_STATUS_ERROR)
        fail("Resuming unsuspended parser not faulted");
    if (XML_GetErrorCode(parser) != XML_ERROR_NOT_SUSPENDED)
        xml_failure(parser);

    /* Get the parser into suspended state */
    XML_SetCharacterDataHandler(parser, clearing_aborting_character_handler);
    resumable = XML_TRUE;
    buffer = XML_GetBuffer(parser, strlen(text));
    if (buffer == NULL)
        fail("Could not acquire parse buffer");
    memcpy(buffer, text, strlen(text));
    if (XML_ParseBuffer(parser, strlen(text),
                        XML_TRUE) != XML_STATUS_SUSPENDED)
        xml_failure(parser);
    if (XML_GetErrorCode(parser) != XML_ERROR_NONE)
        xml_failure(parser);
    if (XML_ParseBuffer(parser, strlen(text), XML_TRUE) != XML_STATUS_ERROR)
        fail("Suspended XML_ParseBuffer not faulted");
    if (XML_GetErrorCode(parser) != XML_ERROR_SUSPENDED)
        xml_failure(parser);
    if (XML_GetBuffer(parser, strlen(text)) != NULL)
        fail("Suspended XML_GetBuffer not faulted");

    /* Get it going again and complete the world */
    XML_SetCharacterDataHandler(parser, NULL);
    if (XML_ResumeParser(parser) != XML_STATUS_OK)
        xml_failure(parser);
    if (XML_ParseBuffer(parser, strlen(text), XML_TRUE) != XML_STATUS_ERROR)
        fail("Post-finishing XML_ParseBuffer not faulted");
    if (XML_GetErrorCode(parser) != XML_ERROR_FINISHED)
        xml_failure(parser);
    if (XML_GetBuffer(parser, strlen(text)) != NULL)
        fail("Post-finishing XML_GetBuffer not faulted");
}
END_TEST

/* Check handling of long prefix names (pool growth) */
START_TEST(test_nsalloc_long_prefix)
{
    const char *text =
        "<"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        ":foo xmlns:"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "='http://example.org/'>"
        "</"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        ":foo>";
    int i;
#define MAX_ALLOC_COUNT 10
    int repeated = 0;

    for (i = 0; i < MAX_ALLOC_COUNT; i++) {
        /* Repeat some tests with the same allocation count to
         * catch cached allocations not freed by XML_ParserReset()
         */
        if ((i == 4 && repeated == 3) ||
            (i == 4 && repeated == 6)) {
            i -= 2;
            repeated++;
        }
        else if ((i == 2 && repeated < 2) ||
                 (i == 3 && repeated < 3) ||
                 (i == 3 && repeated > 3 && repeated < 6) ||
                 (i == 3 && repeated > 6 && repeated < 9)) {
            i--;
            repeated++;
        }
        allocation_count = i;
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parsing worked despite failing allocations");
    else if (i == MAX_ALLOC_COUNT)
        fail("Parsing failed even at max allocation count");
}
#undef MAX_ALLOC_COUNT
END_TEST

/* Check handling of long uri names (pool growth) */
START_TEST(test_nsalloc_long_uri)
{
    const char *text =
        "<foo:e xmlns:foo='http://example.org/"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "' bar:a='12'\n"
        "xmlns:bar='http://example.org/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "'>"
        "</foo:e>";
    int i;
#define MAX_ALLOC_COUNT 15
    int repeated = 0;

    for (i = 0; i < MAX_ALLOC_COUNT; i++) {
        /* Repeat some tests with the same allocation count to
         * catch cached allocations not freed by XML_ParserReset()
         */
        if ((i == 3 && repeated == 1) ||
            (i == 6 && repeated == 4)) {
            i -= 2;
            repeated++;
        }
        else if ((i == 2 && (repeated == 0 || repeated == 2)) ||
                 (i == 4 && repeated == 3) ||
                 (i == 8 && repeated == 5)) {
            i--;
            repeated++;
        }
        allocation_count = i;
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parsing worked despite failing allocations");
    else if (i == MAX_ALLOC_COUNT)
        fail("Parsing failed even at max allocation count");
}
#undef MAX_ALLOC_COUNT
END_TEST

/* Test handling of long attribute names with prefixes */
START_TEST(test_nsalloc_long_attr)
{
    const char *text =
        "<foo:e xmlns:foo='http://example.org/' bar:"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "='12'\n"
        "xmlns:bar='http://example.org/'>"
        "</foo:e>";
    int i;
#define MAX_ALLOC_COUNT 20
    int repeated = 0;

    for (i = 0; i < MAX_ALLOC_COUNT; i++) {
        /* Repeat some tests with the same allocation count to
         * catch cached allocation not freed by XML_ParserReset()
         */
        if ((i == 4 && repeated < 6) ||
            (i == 7 && repeated == 8) ||
            (i == 10 && repeated == 10)) {
            i -= 2;
            repeated++;
        }
        else if ((i == 2 && repeated < 2) ||
                 (i == 3 &&
                  (repeated == 2 || repeated == 4 || repeated == 6)) ||
                 (i == 5 && repeated == 7) ||
                 (i == 6 && repeated == 9)) {
            i--;
            repeated++;
        }
        allocation_count = i;
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parsing worked despite failing allocations");
    else if (i == MAX_ALLOC_COUNT)
        fail("Parsing failed even at max allocation count");
}
#undef MAX_ALLOC_COUNT
END_TEST

/* Test handling of an attribute name with a long namespace prefix */
START_TEST(test_nsalloc_long_attr_prefix)
{
    const char *text =
        "<foo:e xmlns:foo='http://example.org/' "
        /* 64 characters per line */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        ":a='12'\n"
        "xmlns:"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "='http://example.org/'>"
        "</foo:e>";
    const char *elemstr[] = {
        "http://example.org/ e foo",
        "http://example.org/ a "
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
    };
    int i;
#define MAX_ALLOC_COUNT 15
    int repeated = 0;

    for (i = 0; i < MAX_ALLOC_COUNT; i++) {
        /* Repeat some counts to flush out cached allocations */
        if ((i == 4 && (repeated == 3 || repeated == 6)) ||
            (i == 7 && repeated == 9) ||
            (i == 10 && repeated == 12)) {
            i -= 2;
            repeated++;
        }
        else if ((i == 2 && repeated < 2) ||
                 (i == 3 &&
                  (repeated == 2 ||
                   repeated == 4 ||
                   repeated == 5 ||
                   repeated == 7)) ||
                 (i == 5 && repeated == 8) ||
                 (i == 6 && repeated == 10) ||
                 (i == 8 && repeated == 11)) {
            i--;
            repeated++;
        }
        allocation_count = i;
        XML_SetReturnNSTriplet(parser, XML_TRUE);
        XML_SetUserData(parser, elemstr);
        XML_SetElementHandler(parser,
                              triplet_start_checker,
                              triplet_end_checker);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parsing worked despite failing allocations");
    else if (i == MAX_ALLOC_COUNT)
        fail("Parsing failed even at max allocation count");
}
#undef MAX_ALLOC_COUNT
END_TEST

/* Test attribute handling in the face of a dodgy reallocator */
START_TEST(test_nsalloc_realloc_attributes)
{
    const char *text =
        "<foo:e xmlns:foo='http://example.org/' bar:a='12'\n"
        "       xmlns:bar='http://example.org/'>"
        "</foo:e>";
    int i;
#define MAX_REALLOC_COUNT 10

    for (i = 0; i < MAX_REALLOC_COUNT; i++) {
        reallocation_count = i;
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parsing worked despite failing reallocations");
    else if (i == MAX_REALLOC_COUNT)
        fail("Parsing failed at max reallocation count");
}
#undef MAX_REALLOC_COUNT
END_TEST

/* Test long element names with namespaces under a failing allocator */
START_TEST(test_nsalloc_long_element)
{
    const char *text =
        "<foo:thisisalongenoughelementnametotriggerareallocation\n"
        " xmlns:foo='http://example.org/' bar:a='12'\n"
        " xmlns:bar='http://example.org/'>"
        "</foo:thisisalongenoughelementnametotriggerareallocation>";
    const char *elemstr[] = {
        "http://example.org/"
        " thisisalongenoughelementnametotriggerareallocation foo",
        "http://example.org/ a bar"
    };
    int i;
#define MAX_ALLOC_COUNT 15
    int repeated = 0;

    for (i = 0; i < MAX_ALLOC_COUNT; i++) {
        /* Repeat some allocation counts because some allocations
         * get cached across XML_ParserReset() called.
         */
        if ((i == 4 && (repeated == 3 || repeated == 5)) ||
            (i == 7 && repeated == 8) ||
            (i == 10 && repeated == 9)) {
            i -= 2;
            repeated++;
        }
        else if ((i == 2 && repeated < 2) ||
                 (i == 3 &&
                  (repeated == 2 || repeated == 4 || repeated == 6)) ||
                 (i == 5 && repeated == 7)) {
            i--;
            repeated++;
        }
        allocation_count = i;
        XML_SetReturnNSTriplet(parser, XML_TRUE);
        XML_SetUserData(parser, elemstr);
        XML_SetElementHandler(parser,
                              triplet_start_checker,
                              triplet_end_checker);
        if (_XML_Parse_SINGLE_BYTES(parser, text, strlen(text),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
        XML_ParserReset(parser, NULL);
    }
    if (i == 0)
        fail("Parsing worked despite failing reallocations");
    else if (i == MAX_ALLOC_COUNT)
        fail("Parsing failed at max reallocation count");
}
#undef MAX_ALLOC_COUNT
END_TEST

/* Test the effects of reallocation failure when reassigning a
 * binding.
 *
 * XML_ParserReset does not free the BINDING structures used by a
 * parser, but instead adds them to an internal free list to be reused
 * as necessary.  Likewise the URI buffers allocated for the binding
 * aren't freed, but kept attached to their existing binding.  If the
 * new binding has a longer URI, it will need reallocation.  This test
 * provokes that reallocation, and tests the control path if it fails.
 */
START_TEST(test_nsalloc_realloc_binding_uri)
{
    const char *first =
        "<doc xmlns='http://example.org/'>\n"
        "  <e xmlns='' />\n"
        "</doc>";
    const char *second =
        "<doc xmlns='http://example.org/long/enough/URI/to/reallocate/'>\n"
        "  <e xmlns='' />\n"
        "</doc>";
    unsigned i;
#define MAX_REALLOC_COUNT 10

    /* First, do a full parse that will leave bindings around */
    if (_XML_Parse_SINGLE_BYTES(parser, first, strlen(first),
                                XML_TRUE) == XML_STATUS_ERROR)
        xml_failure(parser);

    /* Now repeat with a longer URI and a duff reallocator */
    for (i = 0; i < MAX_REALLOC_COUNT; i++) {
        XML_ParserReset(parser, NULL);
        reallocation_count = i;
        if (_XML_Parse_SINGLE_BYTES(parser, second, strlen(second),
                                    XML_TRUE) != XML_STATUS_ERROR)
            break;
    }
    if (i == 0)
        fail("Parsing worked despite failing reallocation");
    else if (i == MAX_REALLOC_COUNT)
        fail("Parsing failed at max reallocation count");
}
#undef MAX_REALLOC_COUNT
END_TEST


static Suite *
make_suite(void)
{
    Suite *s = suite_create("basic");
    TCase *tc_basic = tcase_create("basic tests");
    TCase *tc_namespace = tcase_create("XML namespaces");
    TCase *tc_misc = tcase_create("miscellaneous tests");
    TCase *tc_alloc = tcase_create("allocation tests");
    TCase *tc_nsalloc = tcase_create("namespace allocattion tests");

    suite_add_tcase(s, tc_basic);
    tcase_add_checked_fixture(tc_basic, basic_setup, basic_teardown);
    tcase_add_test(tc_basic, test_nul_byte);
    tcase_add_test(tc_basic, test_u0000_char);
    tcase_add_test(tc_basic, test_bom_utf8);
    tcase_add_test(tc_basic, test_bom_utf16_be);
    tcase_add_test(tc_basic, test_bom_utf16_le);
    tcase_add_test(tc_basic, test_illegal_utf8);
    tcase_add_test(tc_basic, test_utf8_auto_align);
    tcase_add_test(tc_basic, test_utf16);
    tcase_add_test(tc_basic, test_utf16_le_epilog_newline);
    tcase_add_test(tc_basic, test_not_utf16);
    tcase_add_test(tc_basic, test_bad_encoding);
    tcase_add_test(tc_basic, test_latin1_umlauts);
    /* Regression test for SF bug #491986. */
    tcase_add_test(tc_basic, test_danish_latin1);
    /* Regression test for SF bug #514281. */
    tcase_add_test(tc_basic, test_french_charref_hexidecimal);
    tcase_add_test(tc_basic, test_french_charref_decimal);
    tcase_add_test(tc_basic, test_french_latin1);
    tcase_add_test(tc_basic, test_french_utf8);
    tcase_add_test(tc_basic, test_utf8_false_rejection);
    tcase_add_test(tc_basic, test_line_number_after_parse);
    tcase_add_test(tc_basic, test_column_number_after_parse);
    tcase_add_test(tc_basic, test_line_and_column_numbers_inside_handlers);
    tcase_add_test(tc_basic, test_line_number_after_error);
    tcase_add_test(tc_basic, test_column_number_after_error);
    tcase_add_test(tc_basic, test_really_long_lines);
    tcase_add_test(tc_basic, test_really_long_encoded_lines);
    tcase_add_test(tc_basic, test_end_element_events);
    tcase_add_test(tc_basic, test_attr_whitespace_normalization);
    tcase_add_test(tc_basic, test_xmldecl_misplaced);
    tcase_add_test(tc_basic, test_unknown_encoding_internal_entity);
    tcase_add_test(tc_basic, test_unrecognised_encoding_internal_entity);
    tcase_add_test(tc_basic,
                   test_wfc_undeclared_entity_unread_external_subset);
    tcase_add_test(tc_basic, test_wfc_undeclared_entity_no_external_subset);
    tcase_add_test(tc_basic, test_wfc_undeclared_entity_standalone);
    tcase_add_test(tc_basic, test_wfc_undeclared_entity_with_external_subset);
    tcase_add_test(tc_basic, test_not_standalone_handler_reject);
    tcase_add_test(tc_basic, test_not_standalone_handler_accept);
    tcase_add_test(tc_basic,
                   test_wfc_undeclared_entity_with_external_subset_standalone);
    tcase_add_test(tc_basic,
                   test_entity_with_external_subset_unless_standalone);
    tcase_add_test(tc_basic, test_wfc_no_recursive_entity_refs);
    tcase_add_test(tc_basic, test_ext_entity_set_encoding);
    tcase_add_test(tc_basic, test_ext_entity_no_handler);
    tcase_add_test(tc_basic, test_ext_entity_set_bom);
    tcase_add_test(tc_basic, test_ext_entity_bad_encoding);
    tcase_add_test(tc_basic, test_ext_entity_bad_encoding_2);
    tcase_add_test(tc_basic, test_ext_entity_invalid_parse);
    tcase_add_test(tc_basic, test_ext_entity_invalid_suspended_parse);
    tcase_add_test(tc_basic, test_dtd_default_handling);
    tcase_add_test(tc_basic, test_dtd_attr_handling);
    tcase_add_test(tc_basic, test_empty_ns_without_namespaces);
    tcase_add_test(tc_basic, test_ns_in_attribute_default_without_namespaces);
    tcase_add_test(tc_basic, test_stop_parser_between_char_data_calls);
    tcase_add_test(tc_basic, test_suspend_parser_between_char_data_calls);
    tcase_add_test(tc_basic, test_repeated_stop_parser_between_char_data_calls);
    tcase_add_test(tc_basic, test_good_cdata_ascii);
    tcase_add_test(tc_basic, test_good_cdata_utf16);
    tcase_add_test(tc_basic, test_long_cdata_utf16);
    tcase_add_test(tc_basic, test_multichar_cdata_utf16);
    tcase_add_test(tc_basic, test_bad_cdata);
    tcase_add_test(tc_basic, test_bad_cdata_utf16);
    tcase_add_test(tc_basic, test_stop_parser_between_cdata_calls);
    tcase_add_test(tc_basic, test_suspend_parser_between_cdata_calls);
    tcase_add_test(tc_basic, test_memory_allocation);
    tcase_add_test(tc_basic, test_default_current);
    tcase_add_test(tc_basic, test_dtd_elements);
    tcase_add_test(tc_basic, test_set_foreign_dtd);
    tcase_add_test(tc_basic, test_foreign_dtd_not_standalone);
    tcase_add_test(tc_basic, test_invalid_foreign_dtd);
    tcase_add_test(tc_basic, test_foreign_dtd_with_doctype);
    tcase_add_test(tc_basic, test_foreign_dtd_without_external_subset);
    tcase_add_test(tc_basic, test_empty_foreign_dtd);
    tcase_add_test(tc_basic, test_set_base);
    tcase_add_test(tc_basic, test_attributes);
    tcase_add_test(tc_basic, test_reset_in_entity);
    tcase_add_test(tc_basic, test_resume_invalid_parse);
    tcase_add_test(tc_basic, test_resume_resuspended);
    tcase_add_test(tc_basic, test_cdata_default);
    tcase_add_test(tc_basic, test_subordinate_reset);
    tcase_add_test(tc_basic, test_subordinate_suspend);
    tcase_add_test(tc_basic, test_subordinate_xdecl_suspend);
    tcase_add_test(tc_basic, test_subordinate_xdecl_abort);
    tcase_add_test(tc_basic, test_explicit_encoding);
    tcase_add_test(tc_basic, test_trailing_cr);
    tcase_add_test(tc_basic, test_ext_entity_trailing_cr);
    tcase_add_test(tc_basic, test_trailing_rsqb);
    tcase_add_test(tc_basic, test_ext_entity_trailing_rsqb);
    tcase_add_test(tc_basic, test_ext_entity_good_cdata);
    tcase_add_test(tc_basic, test_user_parameters);
    tcase_add_test(tc_basic, test_ext_entity_ref_parameter);
    tcase_add_test(tc_basic, test_empty_parse);
    tcase_add_test(tc_basic, test_get_buffer_1);
    tcase_add_test(tc_basic, test_get_buffer_2);
    tcase_add_test(tc_basic, test_byte_info_at_end);
    tcase_add_test(tc_basic, test_byte_info_at_error);
    tcase_add_test(tc_basic, test_byte_info_at_cdata);
    tcase_add_test(tc_basic, test_predefined_entities);
    tcase_add_test(tc_basic, test_ignore_section);
    tcase_add_test(tc_basic, test_bad_ignore_section);
    tcase_add_test(tc_basic, test_external_entity_values);
    tcase_add_test(tc_basic, test_ext_entity_not_standalone);
    tcase_add_test(tc_basic, test_ext_entity_value_abort);
    tcase_add_test(tc_basic, test_bad_public_doctype);
    tcase_add_test(tc_basic, test_attribute_enum_value);
    tcase_add_test(tc_basic, test_predefined_entity_redefinition);
    tcase_add_test(tc_basic, test_dtd_stop_processing);
    tcase_add_test(tc_basic, test_public_notation_no_sysid);
    tcase_add_test(tc_basic, test_nested_groups);
    tcase_add_test(tc_basic, test_group_choice);
    tcase_add_test(tc_basic, test_standalone_parameter_entity);
    tcase_add_test(tc_basic, test_skipped_parameter_entity);
    tcase_add_test(tc_basic, test_recursive_external_parameter_entity);
    tcase_add_test(tc_basic, test_undefined_ext_entity_in_external_dtd);
    tcase_add_test(tc_basic, test_suspend_xdecl);
    tcase_add_test(tc_basic, test_abort_epilog);

    suite_add_tcase(s, tc_namespace);
    tcase_add_checked_fixture(tc_namespace,
                              namespace_setup, namespace_teardown);
    tcase_add_test(tc_namespace, test_return_ns_triplet);
    tcase_add_test(tc_namespace, test_ns_tagname_overwrite);
    tcase_add_test(tc_namespace, test_ns_tagname_overwrite_triplet);
    tcase_add_test(tc_namespace, test_start_ns_clears_start_element);
    tcase_add_test(tc_namespace, test_default_ns_from_ext_subset_and_ext_ge);
    tcase_add_test(tc_namespace, test_ns_prefix_with_empty_uri_1);
    tcase_add_test(tc_namespace, test_ns_prefix_with_empty_uri_2);
    tcase_add_test(tc_namespace, test_ns_prefix_with_empty_uri_3);
    tcase_add_test(tc_namespace, test_ns_prefix_with_empty_uri_4);
    tcase_add_test(tc_namespace, test_ns_unbound_prefix);
    tcase_add_test(tc_namespace, test_ns_default_with_empty_uri);
    tcase_add_test(tc_namespace, test_ns_duplicate_attrs_diff_prefixes);
    tcase_add_test(tc_namespace, test_ns_duplicate_hashes);
    tcase_add_test(tc_namespace, test_ns_unbound_prefix_on_attribute);
    tcase_add_test(tc_namespace, test_ns_unbound_prefix_on_element);
    tcase_add_test(tc_namespace, test_ns_parser_reset);
    tcase_add_test(tc_namespace, test_ns_long_element);
    tcase_add_test(tc_namespace, test_ns_mixed_prefix_atts);
    tcase_add_test(tc_namespace, test_ns_extend_uri_buffer);
    tcase_add_test(tc_namespace, test_ns_reserved_attributes);
    tcase_add_test(tc_namespace, test_ns_reserved_attributes_2);

    suite_add_tcase(s, tc_misc);
    tcase_add_checked_fixture(tc_misc, NULL, basic_teardown);
    tcase_add_test(tc_misc, test_misc_alloc_create_parser);
    tcase_add_test(tc_misc, test_misc_alloc_create_parser_with_encoding);
    tcase_add_test(tc_misc, test_misc_null_parser);
    tcase_add_test(tc_misc, test_misc_error_string);
    tcase_add_test(tc_misc, test_misc_version);
    tcase_add_test(tc_misc, test_misc_features);

    suite_add_tcase(s, tc_alloc);
    tcase_add_checked_fixture(tc_alloc, alloc_setup, alloc_teardown);
    tcase_add_test(tc_alloc, test_alloc_parse_xdecl);
    tcase_add_test(tc_alloc, test_alloc_parse_xdecl_2);
    tcase_add_test(tc_alloc, test_alloc_parse_pi);
    tcase_add_test(tc_alloc, test_alloc_parse_pi_2);
    tcase_add_test(tc_alloc, test_alloc_parse_comment);
    tcase_add_test(tc_alloc, test_alloc_parse_comment_2);
    tcase_add_test(tc_alloc, test_alloc_create_external_parser);
    tcase_add_test(tc_alloc, test_alloc_run_external_parser);
    tcase_add_test(tc_alloc, test_alloc_dtd_copy_default_atts);
    tcase_add_test(tc_alloc, test_alloc_external_entity);
    tcase_add_test(tc_alloc, test_alloc_ext_entity_set_encoding);
    tcase_add_test(tc_alloc, test_alloc_internal_entity);
    tcase_add_test(tc_alloc, test_alloc_dtd_default_handling);
    tcase_add_test(tc_alloc, test_alloc_explicit_encoding);
    tcase_add_test(tc_alloc, test_alloc_set_base);
    tcase_add_test(tc_alloc, test_alloc_realloc_buffer);
    tcase_add_test(tc_alloc, test_alloc_ext_entity_realloc_buffer);
    tcase_add_test(tc_alloc, test_alloc_realloc_many_attributes);
    tcase_add_test(tc_alloc, test_alloc_public_entity_value);
    tcase_add_test(tc_alloc, test_alloc_realloc_subst_public_entity_value);
    tcase_add_test(tc_alloc, test_alloc_parse_public_doctype);
    tcase_add_test(tc_alloc, test_alloc_parse_public_doctype_long_name);
    tcase_add_test(tc_alloc, test_alloc_set_foreign_dtd);
    tcase_add_test(tc_alloc, test_alloc_attribute_enum_value);
    tcase_add_test(tc_alloc, test_alloc_realloc_attribute_enum_value);
    tcase_add_test(tc_alloc, test_alloc_realloc_implied_attribute);
    tcase_add_test(tc_alloc, test_alloc_realloc_default_attribute);
    tcase_add_test(tc_alloc, test_alloc_notation);
    tcase_add_test(tc_alloc, test_alloc_public_notation);
    tcase_add_test(tc_alloc, test_alloc_system_notation);
    tcase_add_test(tc_alloc, test_alloc_nested_groups);
    tcase_add_test(tc_alloc, test_alloc_realloc_nested_groups);
    tcase_add_test(tc_alloc, test_alloc_realloc_group_choice);
    tcase_add_test(tc_alloc, test_alloc_pi_in_epilog);

    suite_add_tcase(s, tc_nsalloc);
    tcase_add_checked_fixture(tc_nsalloc, nsalloc_setup, nsalloc_teardown);
    tcase_add_test(tc_nsalloc, test_nsalloc_xmlns);
    tcase_add_test(tc_nsalloc, test_nsalloc_parse_buffer);
    tcase_add_test(tc_nsalloc, test_nsalloc_long_prefix);
    tcase_add_test(tc_nsalloc, test_nsalloc_long_uri);
    tcase_add_test(tc_nsalloc, test_nsalloc_long_attr);
    tcase_add_test(tc_nsalloc, test_nsalloc_long_attr_prefix);
    tcase_add_test(tc_nsalloc, test_nsalloc_realloc_attributes);
    tcase_add_test(tc_nsalloc, test_nsalloc_long_element);
    tcase_add_test(tc_nsalloc, test_nsalloc_realloc_binding_uri);

    return s;
}


int
main(int argc, char *argv[])
{
    int i, nf;
    int verbosity = CK_NORMAL;
    Suite *s = make_suite();
    SRunner *sr = srunner_create(s);

    /* run the tests for internal helper functions */
    testhelper_is_whitespace_normalized();

    for (i = 1; i < argc; ++i) {
        char *opt = argv[i];
        if (strcmp(opt, "-v") == 0 || strcmp(opt, "--verbose") == 0)
            verbosity = CK_VERBOSE;
        else if (strcmp(opt, "-q") == 0 || strcmp(opt, "--quiet") == 0)
            verbosity = CK_SILENT;
        else {
            fprintf(stderr, "runtests: unknown option '%s'\n", opt);
            return 2;
        }
    }
    if (verbosity != CK_SILENT)
        printf("Expat version: %s\n", XML_ExpatVersion());
    srunner_run_all(sr, verbosity);
    nf = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
