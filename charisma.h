/*
 *  Charisma - Unicode® character encoder and decoder library.
 *  Copyright (c) 2025 Railgun Labs, LLC
 *
 *  This software is dual-licensed: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3 as
 *  published by the Free Software Foundation. For the terms of this
 *  license, see <https://www.gnu.org/licenses/>.
 *
 *  Alternatively, you can license this software under a proprietary
 *  license, as set out in <https://railgunlabs.com/charisma/license/>.
 */

#ifndef CHARISMA_H
#define CHARISMA_H

#if defined(_WIN32)
    #if defined(DLL_EXPORT)
        #define CHARISMA_API __declspec(dllexport)
    #elif defined(CHARISMA_STATIC)
        #define CHARISMA_API
    #else
        #define CHARISMA_API __declspec(dllimport)
    #endif
#else
    #define CHARISMA_API
#endif

#include <stdint.h>

typedef uint32_t uchar;

CHARISMA_API int32_t utf8_decode(const uint8_t *text, int32_t length, int32_t *index, uchar *c);
CHARISMA_API int32_t utf16_decode(const uint16_t *text, int32_t length, int32_t *index, uchar *c);
CHARISMA_API int32_t utf16be_decode(const uint16_t *text, int32_t length, int32_t *index, uchar *c);
CHARISMA_API int32_t utf16le_decode(const uint16_t *text, int32_t length, int32_t *index, uchar *c);
CHARISMA_API int32_t utf32_decode(const uint32_t *text, int32_t length, int32_t *index, uchar *c);
CHARISMA_API int32_t utf32be_decode(const uint32_t *text, int32_t length, int32_t *index, uchar *c);
CHARISMA_API int32_t utf32le_decode(const uint32_t *text, int32_t length, int32_t *index, uchar *c);

CHARISMA_API int32_t utf8_encode(uchar c, uint8_t *buf);
CHARISMA_API int32_t utf16_encode(uchar c, uint16_t *buf);
CHARISMA_API int32_t utf16be_encode(uchar c, uint16_t *buf);
CHARISMA_API int32_t utf16le_encode(uchar c, uint16_t *buf);
CHARISMA_API int32_t utf32_encode(uchar c, uint32_t *buf);
CHARISMA_API int32_t utf32be_encode(uchar c, uint32_t *buf);
CHARISMA_API int32_t utf32le_encode(uchar c, uint32_t *buf);

#endif
