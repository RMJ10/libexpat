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
#include "minicheck.h"
#include "xmlchar.h"
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
    int len;
    ElementDataItem *element = &storage->elements[storage->count];

    assert(storage->count < MAX_ELEMENTS);

    if (name == NULL)
        element->name[0] = 0;
    else {
        len = xmlstrlen(name) + 1;
        assert(len < MAX_ELEMENT_NAME_LEN);
        memcpy(element->name, name, len * sizeof(XML_Char));
    }
    element->line = line;
    element->column = column;
    element->is_end = is_end;
    storage->count++;
}


int ElementData_CheckData(ElementData *storage,
                          const ElementResults *expected)
{
    int i;
    char buffer[512];

    if (storage->count != expected->count) {
        sprintf(buffer, "wrong number of element entries: got %d, expected %d",
                storage->count, expected->count);
        fail(buffer);
    }
    for (i = 0; i < storage->count; i++) {
        ElementDataItem *element = &storage->elements[i];
        const ElementResultItem *result = &expected->elements[i];
        if (XML_CHAR_strcmp(element->name, result->name) != 0) {
            sprintf(buffer, "wrong element: got %s, expected %s\n",
                    element->name, result->name);
            fail(buffer);
        }
        if (element->is_end != result->is_end) {
            sprintf(buffer, "got %s element, expected %s element\n",
                    element->is_end ? "end" : "start",
                    result->is_end ? "end" : "start");
            fail(buffer);
        }
        if (element->line != result->line ||
            element->column != result->column) {
            sprintf(buffer, "wrong line/column: got %d/%d, expected %d/%d\n",
                    element->line, element->column,
                    result->line, result->column);
            fail(buffer);
        }
    }
    return 1;
}
