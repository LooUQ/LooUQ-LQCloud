//	Copyright (c) 2020 LooUQ Incorporated.
//	Licensed under The MIT License. See LICENSE in the root directory.

#define _DEBUG
#include <jlinkRtt.h>

#include "lqc_collections.h"


// typedef enum lqcJsonPropType_tag
// {
//     lqcJsonPropType_notFound = 0,
//     lqcJsonPropType_object = 1,
//     lqcJsonPropType_array = 2,
//     lqcJsonPropType_text = 3,
//     lqcJsonPropType_bool = 4,
//     lqcJsonPropType_int = 5,
//     lqcJsonPropType_float = 6,
//     lqcJsonPropType_null = 9
// } lqcJsonPropType_t;


// typedef struct lqcJsonPropValue_tag
// {
//     char *value;
//     uint16_t len;
//     lqcJsonPropType_t type;
// } lqcJsonPropValue_t;


/* lqc_getJsonPropValue() TEST
------------------------------------------------------------------------------------------------ */

// the setup function runs once when you press reset or power the board
void setup() {
    char json[] = "{\"deviceName\": \"myDevice\", \"capabilities\": {\"action\": \"set-lamp\", \"params\": \"lamp-state=on\"},  \"blk2\": {\"state\": true, \"level\": 5.6} }";
    lqcJsonPropValue_t deviceNameProp = lqc_getJsonPropValue(json, "deviceName");
    lqcJsonPropValue_t capabilitiesProp = lqc_getJsonPropValue(json, "capabilities");
    lqcJsonPropValue_t paramsProp = lqc_getJsonPropValue(capabilitiesProp.value, "params");
    lqcJsonPropValue_t boolProp = lqc_getJsonPropValue(json, "state");
    lqcJsonPropValue_t numericProp = lqc_getJsonPropValue(json, "level");
    lqcJsonPropValue_t notFoundProp = lqc_getJsonPropValue(json, "notmyDevice");

    /* lqcJsonPropValue_t return values are a meta-wrapper around the original JSON content. Value points to the start of the property. For text types this is the
     * first char after the ", for objects\arrays this is the { or [. You will need to use strncpy(), atol(), atof() or test bools for == 't', == 'f'.
    */
    char printV[20] = {0};                          // initialize destination to NULL chars 1st, strncpy doesn't add trailing '\0'
    strncpy(printV, paramsProp.value, 13);
    PRINTF(0, "Parameter=%s\r", printV);
}

// the loop function runs over and over again forever
void loop() {
    while(1){}
}



// lqcJsonPropValue_t lqc_getJsonPropValue(const char *jsonSrc, const char *propName)
// {
//     lqcJsonPropValue_t results = {0, 0, lqcJsonPropType_notFound};
//     char *jsonEnd = (char*)jsonSrc + strlen(jsonSrc);
//     char *next;

//     char propSearch[40] = {0};
//     uint8_t nameSz = strlen(propName);

//     memcpy(propSearch + 1, propName, nameSz);
//     propSearch[0] = '"';
//     propSearch[nameSz + 1] = '"';

//     char *nameAt = strstr(jsonSrc, propSearch);
//     if (nameAt)
//     {
//         next = nameAt + nameSz;
//         next = (char*)memchr(next, ':', jsonEnd - next);
//         if (next)
//         {
//             next++;
//             while (*next == '\040' || *next == '\011' )   // skip space or tab
//             {
//                 next++;
//                 if (next >= jsonEnd)
//                     return results;
//             }
//             switch (*next)
//             {
//             case '{':
//                 results.type = lqcJsonPropType_object;
//                 results.value = next;
//                 results.len = findJsonBlockLength(next, jsonEnd, '{', '}');
//                 return results;
//             case '[':
//                 results.type = lqcJsonPropType_array;
//                 results.value = next;
//                 results.len = findJsonBlockLength(next, jsonEnd, '[', ']');
//                 return results;
//             case '"':
//                 results.type = lqcJsonPropType_text;
//                 results.value = (char*)++next;
//                 results.len = (char*)memchr(results.value, '\042', jsonEnd - next) - results.value;    // dblQuote = \042
//                 return results;
//             case 't':
//                 results.type = lqcJsonPropType_bool;
//                 results.value = (char*)next;
//                 results.len = 4;
//                 return results;
//             case 'f':
//                 results.type = lqcJsonPropType_bool;
//                 results.value = (char*)next;
//                 results.len = 5;
//                 return results;
//             case 'n':
//                 results.type = lqcJsonPropType_null;
//                 results.value = (char*)next;
//                 results.len = 4;
//                 return results;
//             default:
//                 results.type = lqcJsonPropType_int;
//                 results.value = (char*)next;
//                 while (*next != ',' && *next != '}' && next < jsonEnd)   // scan forward until beyond current property
//                 {
//                     next++;
//                     if (*next == '.') { results.type = lqcJsonPropType_float; }
//                 }
//                 results.len = next - results.value;
//                 return results;
//             }
//         }
//     }
//     return results;
// }


// static uint16_t findJsonBlockLength(const char *blockStart, const char *jsonEnd, char blockOpen, char blockClose)
// {
//     uint8_t openPairs = 1;
//     char * next = (char*)blockStart;

//     while (openPairs > 0 && next < jsonEnd)   // scan forward until . or beyond current property
//     {
//         next++;
//         if (*next == blockOpen) openPairs++;
//         if (*next == blockClose) openPairs--;
//    }
//    return (next - blockStart + 1);
// }
