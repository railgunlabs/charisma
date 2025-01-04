/*
 *  Charisma - Unicode® character encoder and decoder library.
 *  Copyright (c) 2025 Railgun Labs, LLC
 *
 *  This file is part of Charisma, distributed under a non-commercial use software
 *  license. For the full terms see the included LICENSE file. If you did not
 *  receive a LICENSE file or you would like to purchase a commercial license,
 *  contact us at <https://RailgunLabs.com/contact/>.
 */

#ifndef CHARISMA_H
#define CHARISMA_H

#include <stdint.h>

typedef uint32_t uchar;

int32_t utf8_decode(const uint8_t *text, int32_t length, int32_t *index, uchar *c);
int32_t utf16_decode(const uint16_t *text, int32_t length, int32_t *index, uchar *c);
int32_t utf16be_decode(const uint16_t *text, int32_t length, int32_t *index, uchar *c);
int32_t utf16le_decode(const uint16_t *text, int32_t length, int32_t *index, uchar *c);
int32_t utf32_decode(const uint32_t *text, int32_t length, int32_t *index, uchar *c);
int32_t utf32be_decode(const uint32_t *text, int32_t length, int32_t *index, uchar *c);
int32_t utf32le_decode(const uint32_t *text, int32_t length, int32_t *index, uchar *c);

int32_t utf8_encode(uchar c, uint8_t *buf);
int32_t utf16_encode(uchar c, uint16_t *buf);
int32_t utf16be_encode(uchar c, uint16_t *buf);
int32_t utf16le_encode(uchar c, uint16_t *buf);
int32_t utf32_encode(uchar c, uint32_t *buf);
int32_t utf32be_encode(uchar c, uint32_t *buf);
int32_t utf32le_encode(uchar c, uint32_t *buf);

#endif
