/* Copyright (c) 2017 The Expat Maintainers
 * Copying is permitted under the MIT license.  See the file COPYING
 * for details.
 *
 * elementdata.c: support for element, row and column analysis
 */


#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "expat.h"
#include "unicode.h"
#include "minicheck.h"
#include "elementdata.h"

static int
xmlstrlen(const XML_Char *s)
{
    int len = 0;
    assert(s != NULL);
    while (s[len] != 0)
        ++len;
    return len;
}


void
ElementData_Init(ElementData *storage)
{
    assert(storage != NULL);
    storage->count = 0;
}


void
ElementData_AddData(ElementData *storage, const XML_Char *name,
                    int line, int column, int is_end)
{
    int len = xmlstrlen(name) + 1;
    ElementDataItem *element = &storage->elements[storage->count];

    assert(storage->count < MAX_ELEMENTS);
    assert(len < MAX_ELEMENT_NAME_LEN);

    memcpy(element->name, name, len * sizeof(XML_Char));
    element->line = line;
    element->column = column;
    element->is_end = is_end;
    storage->count++;
}


int ElementData_CheckData(ElementData *storage, ElementResults *expected)
{
    TSTR_FN_START;
    int i;
    char buffer[512];

    if (storage->count != expected->count) {
        sprintf(buffer, "wrong number of element entries: got %d, expected %d",
                storage->count, expected->count);
        TSTR_FN_END;
        fail(buffer);
    }
    for (i = 0; i < storage->count; i++) {
        ElementDataItem *element = &storage->elements[i];
        ElementResultItem *result = &expected->elements[i];
        if (TSTR_CMP(element->name, TSTR(result->name)) != 0) {
            sprintf(buffer, "wrong element: got %s, expected %s\n",
                    TSTR2CHAR(element->name), result->name);
            TSTR_FN_END;
            fail(buffer);
        }
        if (element->is_end != result->is_end) {
            sprintf(buffer, "got %s element, expected %s element\n",
                    element->is_end ? "end" : "start",
                    result->is_end ? "end" : "start");
            TSTR_FN_END;
            fail(buffer);
        }
        if (element->line != result->line ||
            element->column != result->column) {
            sprintf(buffer, "wrong line/column: got %d/%d, expected %d/%d\n",
                    element->line, element->column,
                    result->line, result->column);
            TSTR_FN_END;
            fail(buffer);
        }
    }
    TSTR_FN_END;
    return 1;
}
