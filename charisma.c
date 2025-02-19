/*
 *  Charisma - Unicode® character encoder and decoder library.
 *  Copyright (c) 2025 Railgun Labs, LLC
 *
 *  This software is dual-licensed: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License version 3
 *  as published by the Free Software Foundation. For the terms of this
 *  license, see <https://www.gnu.org/licenses/>.
 *
 *  Alternatively, you can license this software under a proprietary
 *  license, as set out in <https://railgunlabs.com/charisma/license/>.
 */

#include "charisma.h"
#include <assert.h>
#include <stdbool.h>

#if defined(HAVE_CONFIG_H)
    #include "config.h"
#elif !defined(HAVE_BIG_ENDIAN) && !defined(HAVE_LITTLE_ENDIAN)
    #error please define 'HAVE_BIG_ENDIAN' or 'HAVE_LITTLE_ENDIAN' if targeting a big or endian system
#endif

#if defined(HAVE_FREEBSD_BSWAP)
    #include <sys/endian.h>
#elif defined(HAVE_LINUX_BSWAP)
    #include <byteswap.h>
#elif defined(HAVE_MSVC_BYTESWAP)
    #include <stdlib.h>
#endif

#define UNICHAR_C(C) ((uchar)(C))

#define UNSET -2

typedef uint16_t(*ByteSwap16)(uint16_t val);
typedef uint32_t(*ByteSwap32)(uint32_t val);

static inline uint32_t swap32(uint32_t val)
{
#if defined(HAVE_FREEBSD_BSWAP)
    return bswap32(val);
#elif defined(HAVE_LINUX_BSWAP)
    return bswap_32(val);
#elif defined(HAVE_COMPILER_BSWAP)
    return __builtin_bswap32(val);
#elif defined(HAVE_MSVC_BYTESWAP)
    return _byteswap_ulong(val);
#else
    return ((val & 0x000000FFu) << 24u) |
           ((val & 0x0000FF00u) << 8u) |
           ((val & 0x00FF0000u) >> 8u) |
           ((val & 0xFF000000u) >> 24u);
#endif
}

static inline uint16_t swap16(uint16_t val)
{
#if defined(HAVE_FREEBSD_BSWAP)
    return bswap16(val);
#elif defined(HAVE_LINUX_BSWAP)
    return bswap_16(val);
#elif defined(HAVE_COMPILER_BSWAP)
    return __builtin_bswap16(val);
#elif defined(HAVE_MSVC_BYTESWAP)
    return _byteswap_ushort(val);
#else
    return (val << (uint16_t)8) | (val >> (uint16_t)8);
#endif
}

static inline uint32_t swap32_le(uint32_t val)
{
#if defined(HAVE_BIG_ENDIAN)
    return swap32(val);
#else
    return val;
#endif
}

static inline uint16_t swap16_le(uint16_t val)
{
#if defined(HAVE_BIG_ENDIAN)
    return swap16(val);
#else
    return val;
#endif
}

static inline uint16_t swap16_nop(uint16_t val)
{
    return val;
}

static inline uint32_t swap32_be(uint32_t val)
{
#if defined(HAVE_BIG_ENDIAN)
    return val;
#else
    return swap32(val);
#endif
}

static inline uint16_t swap16_be(uint16_t val)
{
#if defined(HAVE_BIG_ENDIAN)
    return val;
#else
    return swap16(val);
#endif
}

static inline uint32_t swap32_nop(uint32_t val)
{
    return val;
}

static bool is_low_surrogate(uchar c)
{
    bool is_low;
    if (c < UNICHAR_C(0xDC00))
    {
        is_low = false;
    }
    else if (c > UNICHAR_C(0xDFFF))
    {
        is_low = false;
    }
    else
    {
        is_low = true;
    }
    return is_low;
}

static bool is_high_surrogate(uchar c)
{
    bool is_high;
    if (c < UNICHAR_C(0xD800))
    {
        is_high = false;
    }
    else if (c > UNICHAR_C(0xDBFF))
    {
        is_high = false;
    }
    else
    {
        is_high = true;
    }
    return is_high;
}

static bool is_valid_scalar(uchar c)
{
    bool is_scalar;
    if (c > UNICHAR_C(0x10FFFF))
    {
        is_scalar = false;
    }
    else if (is_low_surrogate(c))
    {
        is_scalar = false;
    }
    else if (is_high_surrogate(c))
    {
        is_scalar = false;
    }
    else
    {
        is_scalar = true;
    }
    return is_scalar;
}

CHARISMA_API int32_t utf8_decode(const uint8_t *text, int32_t length, int32_t *index, uchar *c) // cppcheck-suppress misra-c2012-8.7 ; Public functions must have external linkage.
{
    int32_t ret = UNSET;

    // LCOV_EXCL_START
    assert(text);
    assert(index);
    assert(c);
    // LCOV_EXCL_STOP

    /*
     *  Lookup table for determining how many bytes are in a UTF-8 encoded sequence
     *  using only the first code unit. It is based on RFC 3629.
     *
     *  Using branches, written in pseudocode, the table looks like this:
     *
     *      if (c >= 0) and (c <= 127) return 1
     *      elif (c >= 194) and (c <= 223) return 2
     *      elif (c >= 224) and (c <= 239) return 3
     *      elif (c >= 240) and (c <= 244) return 4
     *      else return 0
     *
     *  This lookup table will return '0' for continuation bytes, overlong bytes,
     *  and bytes which do not appear in a valid UTF-8 sequence.
     */
    static const uint8_t bytes_needed_for_UTF8_sequence[] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        // Defines bit patterns for masking the leading byte of a UTF-8 sequence.
        0,
        (uint8_t)0xFF, // Single byte (i.e. fits in ASCII).
        0x1F, // Two byte sequence: 110xxxxx 10xxxxxx.
        0x0F, // Three byte sequence: 1110xxxx 10xxxxxx 10xxxxxx.
        0x07, // Four byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx.
    };

    // See "utf8.dot" for a visualization of the DFA.
    static const uint8_t next_UTF8_DFA[] = {
        0, 12, 24, 36, 60, 96, 84, 12, 12, 12, 48, 72,  // state 0
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, // state 1
        12, 0, 12, 12, 12, 12, 12, 0, 12, 0, 12, 12,    // state 2
        12, 24, 12, 12, 12, 12, 12, 24, 12, 24, 12, 12, // state 3
        12, 12, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, // state 4
        12, 24, 12, 12, 12, 12, 12, 12, 12, 24, 12, 12, // state 5
        12, 12, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, // state 6
        12, 36, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, // state 7
        12, 36, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, // state 8
    };

    // Character classes for the UTF-8 DFA.
    static const uint8_t byte_to_character_class[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        8, 8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        10, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 3, 3,
        11, 6, 6, 6, 5, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    };

    // The acceptance state for the UTF-8 DFA.
    static const uint8_t DFA_ACCEPTANCE_STATE = 0;

    // Load the text offset into a register for speedy access.
    register int32_t text_offset = *index;

    // Check for the end of the string.
    if ((length >= 0) && (text_offset >= length))
    {
        *c = UNICHAR_C('\0');
        ret = 0;
    }

    if (ret == UNSET)
    {
        // Offset to the requested code unit.
        const uint8_t *bytes = &text[text_offset];
        
        // Check if NUL (U+0000) was reached if this is a null-terminated string.
        if ((length < 0) && (bytes[0] == (uint8_t)0x0))
        {
            *c = UNICHAR_C('\0');
            ret = 0;
        }
        else
        {
            // Lookup expected UTF-8 sequence length based on the first byte.
            const int32_t seqlen = (int32_t)bytes_needed_for_UTF8_sequence[bytes[0]];
            if (seqlen == 0)
            {
                *c = UNICHAR_C(0xFFFD);
                *index += 1; // The first byte is illegal (advance beyond it).
                ret = -1;
            }
            // The first byte is valid, but that doesn't mean the rest are or
            // that the sequence is truncated; check for truncation now.
            else if (length < 0x0)
            {
                // Advance past the first byte in the malformed UTF-8 sequence.
                // We know this byte cannot be a zero byte otherwise it would
                // have been detected earlier.
                text_offset += 1;

                // Advance past the remaining bytes until a zero byte is found
                // or the end of the sequence was reached.
                for (int32_t i = 1; i < seqlen; i++)
                {
                    if (bytes[i] == (uint8_t)0x0)
                    {
                        *c = UNICHAR_C(0xFFFD);
                        ret = -1;
                        break;
                    }
                    text_offset += 1;
                }
                *index = text_offset;
            }
            else if ((text_offset + seqlen) > length)
            {
                *c = UNICHAR_C(0xFFFD);
                *index = length;
                ret = -1;
            }
            else
            {
                *index += seqlen;
            }

            if (ret == UNSET)
            {
                // Consume the first UTF-8 byte.
                uchar value = (uchar)bytes[0] & (uchar)bytes_needed_for_UTF8_sequence[256 + seqlen];

                // Transition to the first DFA state.
                uint8_t state = next_UTF8_DFA[byte_to_character_class[bytes[0]]];

                // Consume the remaining UTF-8 bytes.
                for (int32_t i = 1; i < seqlen; i++)
                {
                    // Mask off the next byte.
                    // It's of the form 10xxxxxx if valid UTF-8.
                    value = (value << UNICHAR_C(6)) | ((uchar)bytes[i] & UNICHAR_C(0x3F));

                    // Transition to the next DFA state.
                    state = next_UTF8_DFA[(const uint8_t)state + byte_to_character_class[bytes[i]]];
                }

                // Verify the encoded character was well-formed.
                if (state == DFA_ACCEPTANCE_STATE)
                {
                    ret = seqlen;
                    *c = value;
                }
                else
                {
                    ret = -1; // Malformed UTF-8 sequence.
                    *c = UNICHAR_C(0xFFFD);
                }
            }
        }
    }

    return ret;
}

static int32_t decode16(const uint16_t *text, int32_t length, int32_t *index, uchar *c, ByteSwap16 swap)
{
    // LCOV_EXCL_START
    assert(text);
    assert(index);
    assert(c);
    assert(swap);
    // LCOV_EXCL_STOP

    int32_t ret = UNSET;
    const int32_t text_offset = *index;

    // Check for the end of the string.
    if ((length >= 0) && (text_offset >= length))
    {
        *c = UNICHAR_C('\0');
        ret = 0;
    }
    else if ((length < 0) && (text[text_offset] == (uint16_t)0x0000))
    {
        *c = UNICHAR_C('\0');
        ret = 0;
    }
    else
    {
        // Extract the first code unit (a 16-bit word) which might be a high surrogate.
        const uchar word = (uchar)swap(text[text_offset]);

        // Check if this UTF-16 character is in the Basic Multilingual Plane.
        if (is_low_surrogate(word) || is_high_surrogate(word))
        {
            // A High Surrogate must be followed by a Low Surrogate.
            // Check if there is room for a subsequent 16-bit word in the string;
            // if there isn't then the character is erroneously encoded.
            if ((length >= 0) && ((text_offset + 1) >= length))
            {
                *c = UNICHAR_C(0xFFFD);
                *index += 1;
                ret = -1;
            }
            else if (text[text_offset + 1] == (uint16_t)0x0)
            {
                *c = UNICHAR_C(0xFFFD);
                *index += 1;
                ret = -1;
            }
            else
            {
                // If the word is not in the Basic Multilingual Plane, then it must be a High Surrogate.
                // If it's not a High Surrogate, then this isn't valid UTF-16.
                const uchar next_word = (uchar)swap(text[text_offset + 1]);
                if (word >= UNICHAR_C(0xDC00))
                {
                    *c = UNICHAR_C(0xFFFD);
                    ret = -1;
                }
                // Verify the next word is a Low Surrogate.
                else if (next_word <= UNICHAR_C(0xDBFF))
                {
                    *c = UNICHAR_C(0xFFFD);
                    ret = -1;
                }
                else
                {
                    static const uchar SURROGATE_OFFSET = (UNICHAR_C(0x10000) - (UNICHAR_C(0xD800) << UNICHAR_C(10))) - UNICHAR_C(0xDC00);
                    *c = ((word << 10) + next_word) + SURROGATE_OFFSET;
                    ret = 2;
                }
                *index += 2;
            }
        }
        else
        {
            // This character is in the Basic Multilingual Plane.
            *c = word;
            *index += 1;
            ret = 1;
        }
    }

    return ret;
}

CHARISMA_API int32_t utf16be_decode(const uint16_t *text, int32_t length, int32_t *index, uchar *c) // cppcheck-suppress misra-c2012-8.7 ; Public functions must have external linkage.
{
    return decode16(text, length, index, c, &swap16_be);
}

CHARISMA_API int32_t utf16le_decode(const uint16_t *text, int32_t length, int32_t *index, uchar *c) // cppcheck-suppress misra-c2012-8.7 ; Public functions must have external linkage.
{
    return decode16(text, length, index, c, &swap16_le);
}

CHARISMA_API int32_t utf16_decode(const uint16_t *text, int32_t length, int32_t *index, uchar *c) // cppcheck-suppress misra-c2012-8.7 ; Public functions must have external linkage.
{
    return decode16(text, length, index, c, &swap16_nop);
}

static int32_t decode32(const uint32_t *text, int32_t length, int32_t *index, uchar *c, ByteSwap32 swap)
{
    // LCOV_EXCL_START
    assert(text);
    assert(index);
    assert(c);
    assert(swap);
    // LCOV_EXCL_STOP

    int32_t ret = UNSET;
    const int32_t text_offset = *index;

    // Check for the end of the string.
    if ((length >= 0) && (text_offset >= length))
    {
        *c = UNICHAR_C('\0');
        ret = 0;
    }
    else
    {
        const uchar scalar = (uchar)swap(text[text_offset]);
        if ((length < 0) && (scalar == (uchar)(0x0)))
        {
            *c = UNICHAR_C('\0');
            ret = 0;
        }
        else
        {
            // Verify the UTF-32 code point is a valid Unicode scalar value.
            if (!is_valid_scalar(scalar))
            {
                *c = UNICHAR_C(0xFFFD);
                ret = -1;
            }
            else
            {
                *c = scalar;
                ret = 1;
            }
            *index += 1;
        }
    }

    return ret;
}

CHARISMA_API int32_t utf32be_decode(const uint32_t *text, int32_t length, int32_t *index, uchar *c) // cppcheck-suppress misra-c2012-8.7 ; Public functions must have external linkage.
{
    return decode32(text, length, index, c, &swap32_be);
}

CHARISMA_API int32_t utf32le_decode(const uint32_t *text, int32_t length, int32_t *index, uchar *c) // cppcheck-suppress misra-c2012-8.7 ; Public functions must have external linkage.
{
    return decode32(text, length, index, c, &swap32_le);
}

CHARISMA_API int32_t utf32_decode(const uint32_t *text, int32_t length, int32_t *index, uchar *c) // cppcheck-suppress misra-c2012-8.7 ; Public functions must have external linkage.
{
    return decode32(text, length, index, c, &swap32_nop);
}

CHARISMA_API int32_t utf8_encode(uchar c, uint8_t *buf) // cppcheck-suppress misra-c2012-8.7 ; Public functions must have external linkage.
{
    // LCOV_EXCL_START
    assert(buf);
    // LCOV_EXCL_STOP
    int32_t ret = UNSET;
    if (!is_valid_scalar(c))
    {
        ret = -1;
    }
    else if (c <= UNICHAR_C(0x7F))
    {
        buf[0] = (uint8_t)c;
        ret = 1;
    }
    else if (c <= UNICHAR_C(0x7FF))
    {
        buf[0] = (uint8_t)(c >> UNICHAR_C(6)) | (uint8_t)0xC0;
        buf[1] = (uint8_t)(c & UNICHAR_C(0x3F)) | (uint8_t)0x80;
        ret = 2;
    }
    else if (c <= UNICHAR_C(0xFFFF))
    {
        buf[0] = (uint8_t)(c >> UNICHAR_C(12)) | (uint8_t)0xE0;
        buf[1] = (uint8_t)((c >> UNICHAR_C(6)) & UNICHAR_C(0x3F)) | (uint8_t)0x80;
        buf[2] = (uint8_t)(c & UNICHAR_C(0x3F)) | (uint8_t)0x80;
        ret = 3;
    }
    else
    {
        buf[0] = (uint8_t)(c >> UNICHAR_C(18)) | (uint8_t)0xF0;
        buf[1] = (uint8_t)((c >> UNICHAR_C(12)) & UNICHAR_C(0x3F)) | (uint8_t)0x80;
        buf[2] = (uint8_t)((c >> UNICHAR_C(6)) & UNICHAR_C(0x3F)) | (uint8_t)0x80;
        buf[3] = (uint8_t)(c & UNICHAR_C(0x3F)) | (uint8_t)0x80;
        ret = 4;
    }
    return ret;
}

static int32_t encode16(uchar c, uint16_t *buf, ByteSwap16 swap)
{
    // LCOV_EXCL_START
    assert(buf);
    assert(swap);
    // LCOV_EXCL_STOP

    int32_t ret = UNSET;

    if (!is_valid_scalar(c))
    {
        ret = -1;
    }
    else if (c <= UNICHAR_C(0xFFFF))
    {
        buf[0] = swap((uint16_t)c);
        ret = 1;
    }
    else
    {
        const uchar LEAD_OFFSET = UNICHAR_C(0xD800) - (UNICHAR_C(0x10000) >> UNICHAR_C(10));
        const uchar high = LEAD_OFFSET + (c >> UNICHAR_C(10));
        const uchar low = UNICHAR_C(0xDC00) + (c & UNICHAR_C(0x3FF));
        // LCOV_EXCL_START
        assert(high <= UNICHAR_C(0xFFFF));
        assert(low <= UNICHAR_C(0xFFFF));
        // LCOV_EXCL_STOP
        buf[0] = swap((uint16_t)high);
        buf[1] = swap((uint16_t)low);
        ret = 2;
    }

    return ret;
}

CHARISMA_API int32_t utf16be_encode(uchar c, uint16_t *buf) // cppcheck-suppress misra-c2012-8.7 ; Public functions must have external linkage.
{
    return encode16(c, buf, &swap16_be);
}

CHARISMA_API int32_t utf16le_encode(uchar c, uint16_t *buf) // cppcheck-suppress misra-c2012-8.7 ; Public functions must have external linkage.
{
    return encode16(c, buf, &swap16_le);
}

CHARISMA_API int32_t utf16_encode(uchar c, uint16_t *buf) // cppcheck-suppress misra-c2012-8.7 ; Public functions must have external linkage.
{
    return encode16(c, buf, &swap16_nop);
}

static int32_t encode32(uchar c, uint32_t *buf, ByteSwap32 swap) // cppcheck-suppress misra-c2012-8.7 ; Public functions must have external linkage.
{
    // LCOV_EXCL_START
    assert(buf);
    assert(swap);
    // LCOV_EXCL_STOP

    int32_t ret = UNSET;

    if (!is_valid_scalar(c))
    {
        ret = -1;
    }
    else
    {
        buf[0] = swap(c);
        ret = 1;
    }

    return ret;
}

CHARISMA_API int32_t utf32be_encode(uchar c, uint32_t *buf) // cppcheck-suppress misra-c2012-8.7 ; Public functions must have external linkage.
{
    return encode32(c, buf, &swap32_be);
}

CHARISMA_API int32_t utf32le_encode(uchar c, uint32_t *buf) // cppcheck-suppress misra-c2012-8.7 ; Public functions must have external linkage.
{
    return encode32(c, buf, &swap32_le);
}

CHARISMA_API int32_t utf32_encode(uchar c, uint32_t *buf) // cppcheck-suppress misra-c2012-8.7 ; Public functions must have external linkage.
{
    return encode32(c, buf, &swap32_nop);
}
