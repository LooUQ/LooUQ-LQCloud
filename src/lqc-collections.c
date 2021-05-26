/******************************************************************************
 *  \file lqc-collections.c
 *  \author Greg Terrell
 *  \license MIT License
 *
 *  Copyright (c) 2020, 2021 LooUQ Incorporated.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED
 * "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ******************************************************************************
 * LooUQ LQCloud Client Collections Utilities
 *****************************************************************************/

#define _DEBUG 2                        // set to non-zero value for PRINTF debugging output, 
// debugging output options             // LTEm1c will satisfy PRINTF references with empty definition if not already resolved
#if defined(_DEBUG) && _DEBUG > 0
    asm(".global _printf_float");       // forces build to link in float support for printf
    #if _DEBUG == 1
    #define SERIAL_DBG 1                // enable serial port output using devl host platform serial, 1=wait for port
    #elif _DEBUG == 2
    #include <jlinkRtt.h>               // output debug PRINTF macros to J-Link RTT channel
    #endif
#else
#define PRINTF(c_, f_, ...) ;
#endif


#include "lqc-collections.h"

#pragma region Local Static Function Declarations
static uint16_t findJsonBlockLength(const char *blockStart, const char *jsonEnd, char blockOpen, char blockClose);
#pragma endregion


/**
 *  \brief Parses a HTTP style query string (key\value pairs) and creates a dictionary overlay for the keys and values. 
 * 
 *  \param dictSrc [in] - Char pointer to the c-string containing key value pairs to identify. NOTE: the source is mutated in the process, keys\values are NULL term'd.
 *  \param qsSize [in] - Lenght of the incoming query string. 
 * 
 *  \return Struct with map (pointers) to the keys and values (within the source array)
*/
keyValueDict_t lqc_createDictFromQueryString(char *dictSrc, size_t qsSize)
{
    keyValueDict_t result = {0};
    //keyValueDict_t result = {0, 0, {0}, {0}};

    if (qsSize == 0)
        return result;

    result.length = qsSize;
    
    char *next = dictSrc;
    char *delimAt;
    char *endAt = dictSrc + result.length;

    for (size_t i = 0; i < KEYVALUE_DICT_SZ; i++)                        // 1st pass; get names + values
    {
        delimAt = memchr(dictSrc, '&', endAt - dictSrc);
        delimAt = (delimAt == NULL) ? endAt : delimAt;

        result.keys[i] = dictSrc;
        *delimAt = '\0';
        dictSrc = delimAt + 1;
        result.count = i;
        if (delimAt == endAt)
            break;
    }
    result.count++;
    
    for (size_t i = 0; i < result.count; i++)                               // 2nd pass; split names/values
    {
        delimAt = memchr(result.keys[i], '=', endAt - result.keys[i]);
        if (delimAt == NULL)
        {
            result.count = i;
            break;
        }
        *delimAt = '\0';
        result.values[i] = delimAt + 1;
    }
    return result;
}


/**
 *  \brief Scans the qryProps struct for the a prop and returns the value from the underlying char array.
 * 
 *  \param [in] propsName - Char pointer to the c-string containing message properties passed in topic 
 * 
 *  \return Struct with pointer arrays to the properties (name/value)
*/
void lqc_getDictValue(const char *key, keyValueDict_t dict, char *value, uint8_t valSz)
{
    if (dict.keys[0] == NULL || dict.keys[0][0] == '\0')        // underlying char array is invalid
        return;

    for (size_t i = 0; i < dict.count; i++)
    {
        if (strcmp(dict.keys[i], key) == 0)
        {
            strncpy(value, dict.values[i], valSz-1);
            break;
        }
    }
}


/**
 *  \brief Scans the source string for fromChr and replaces with toChr. Usefull for substitution of special char in query strings.
 * 
 *  \param [in\out] srcStr - Char pointer to the c-string containing required substitutions 
 *  \param [in] fromChr - Char value to replace in src 
 *  \param [in] toChr - Char value to put in place of fromChr 
 * 
 *  \return Number of substitutions made
*/
uint16_t lqc_strReplace(char *srcStr, char fromChr, char toChr)
{
    char *opPtr = srcStr;
    uint16_t replacements = 0;

    if (fromChr == 0 || toChr == 0 || fromChr == toChr)
        return 0;

    while(*opPtr != 0)
    {
        if (*opPtr == fromChr)
        {
            *opPtr = toChr;
            replacements++;
        }
        opPtr++;
    }
    return replacements;
}


/**
 *  \brief Scans a JSON formatted C-String (char array) for a property, once found a descriptive struct is populated with info to allow for property value consumption.
 * 
 *  \param [in] jsonSrc - Char array containing the JSON document.
 *  \param [in] propName - The name of the property you are searching for.
 * 
 *  \return Struct with a pointer to property value, a property type (enum) and the len of property value.
*/
lqcJsonPropValue_t lqc_getJsonPropValue(const char *jsonSrc, const char *propName)
{
    lqcJsonPropValue_t results = {0, 0, lqcJsonPropType_notFound};
    char *jsonEnd = (char*)jsonSrc + strlen(jsonSrc);
    char *next;

    char propSearch[40] = {0};
    uint8_t nameSz = strlen(propName);

    memcpy(propSearch + 1, propName, nameSz);
    propSearch[0] = '"';
    propSearch[nameSz + 1] = '"';

    char *nameAt = strstr(jsonSrc, propSearch);
    if (nameAt)
    {
        next = nameAt + nameSz;
        next = (char*)memchr(next, ':', jsonEnd - next);
        if (next)
        {
            next++;
            while (*next == '\040' || *next == '\011' )   // skip space or tab
            {
                next++;
                if (next >= jsonEnd)
                    return results;
            }
            switch (*next)
            {
            case '{':
                results.type = lqcJsonPropType_object;
                results.value = next;
                results.len = findJsonBlockLength(next, jsonEnd, '{', '}');
                return results;
            case '[':
                results.type = lqcJsonPropType_array;
                results.value = next;
                results.len = findJsonBlockLength(next, jsonEnd, '[', ']');
                return results;
            case '"':
                results.type = lqcJsonPropType_text;
                results.value = (char*)++next;
                results.len = (char*)memchr(results.value, '\042', jsonEnd - next) - results.value;    // dblQuote = \042
                return results;
            case 't':
                results.type = lqcJsonPropType_bool;
                results.value = (char*)next;
                results.len = 4;
                return results;
            case 'f':
                results.type = lqcJsonPropType_bool;
                results.value = (char*)next;
                results.len = 5;
                return results;
            case 'n':
                results.type = lqcJsonPropType_null;
                results.value = (char*)next;
                results.len = 4;
                return results;
            default:
                results.type = lqcJsonPropType_int;
                results.value = (char*)next;
                while (*next != ',' && *next != '}' && next < jsonEnd)   // scan forward until beyond current property
                {
                    next++;
                    if (*next == '.') { results.type = lqcJsonPropType_float; }
                }
                results.len = next - results.value;
                return results;
            }
        }
    }
    return results;
}


#pragma region Static Local Functions

/**
 *  \brief STATIC Scope: Local function to determine the length of a JSON object or array. Used by lqc_getJsonPropValue().
 * 
 *  \param [in] blockStart - Pointer to the starting point for the scan.
 *  \param [in] jsonEnd - End of the original JSON formatted char array.
 *  \param [in] blockOpen - Character marking the start of the block being sized, used to identify nested blocks.
 *  \param [in] blockClose - Character marking the end of the block being sized (including nested).
 * 
 *  \return The size of the block (object\array) including the opening and closing marking chars.
*/
static uint16_t findJsonBlockLength(const char *blockStart, const char *jsonEnd, char blockOpen, char blockClose)
{
    uint8_t openPairs = 1;
    char * next = (char*)blockStart;

    while (openPairs > 0 && next < jsonEnd)   // scan forward until . or beyond current property
    {
        next++;
        if (*next == blockOpen) openPairs++;
        if (*next == blockClose) openPairs--;
   }
   return (next - blockStart + 1);
}

#pragma endregion

