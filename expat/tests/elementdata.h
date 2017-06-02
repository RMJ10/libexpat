/* Copyright (c) 2017 The Expat Maintainers
 * Copying is permitted under the MIT license.  See the file COPYING
 * for details.
 *
 * elementdata.c: support for element, row and column analysis
 *
 * Utilities to help assess element positioning information
 */

#ifndef XML_ELEMENTDATA_H
#define XML_ELEMENTDATA_H 1

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ELEMENTS 16
#define MAX_ELEMENT_NAME_LEN 32

typedef struct {
    XML_Char name[MAX_ELEMENT_NAME_LEN];
    int line;
    int column;
    int is_end;
} ElementDataItem;

typedef struct {
    int count;
    ElementDataItem elements[MAX_ELEMENTS];
} ElementData;

typedef struct {
    char name[MAX_ELEMENT_NAME_LEN];
    int line;
    int column;
    int is_end;
} ElementResultItem;

typedef struct {
    int count;
    ElementResultItem elements[MAX_ELEMENTS];
} ElementResults;

extern void ElementData_Init(ElementData *storage);
extern void ElementData_AddData(ElementData *storage, const XML_Char *name,
                                int line, int column, int is_end);
extern int ElementData_CheckData(ElementData *storage,
                                 ElementResults *expected);

#ifdef __cplusplus
}
#endif

#endif /* XML_ELEMENTDATA_H */
