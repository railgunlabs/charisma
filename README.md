<picture>
  <source media="(prefers-color-scheme: dark)" srcset=".github/charisma-dark.svg">
  <source media="(prefers-color-scheme: light)" srcset=".github/charisma.svg">
  <img alt="Charisma" src=".github/charisma.svg" width="408px">
</picture>

**Char**isma is a Unicode® character decoder and encoder library that conforms to the MISRA C:2012 coding standard.
It provides functions for decoding and encoding characters _safely_ in UTF-8, UTF-16, and UTF-32 (big or little endian).
It can _recover_ from malformed characters, allowing decoding to continue.

[![Build Status](https://github.com/railgunlabs/charisma/actions/workflows/build.yml/badge.svg)](https://github.com/railgunlabs/charisma/actions/workflows/build.yml)

## Why?

There are many Unicode character decoders floating about, but most are **unsafe** and do not support recovering from malformed character sequences.
Attempting to decode or incorrectly recover from malformed text with these decoders can lead to security vulnerabilities.
It's critical for software that processes external text to use a _robust_ character decoder that can detect malformed character sequences.

## Features

* Safely decode and encode Unicode characters
* Safely recover from malformed character sequences
* Supports UTF-8, UTF-16-BE, UTF-16-LE, UTF-32-BE, and UTF-32-LE
* Supports both null terminated and non-null terminated strings
* Reentrant implementation
* Lightweight (< 200 semicolons)
* Extensively tested (see below)
* No dependencies

## MISRA C:2012 Compliance

Charisma honors all Required, Mandatory, and Advisory rules defined by MIRSA C:2012 and its four amendments.
The complete compliance table is [documented here](https://railgunlabs.com/charisma/manual/misra-compliance/).

## Ultra Portable

Charisma is _ultra portable_.
It's written in C99 and only requires a few features from libc which are listed in the following table.

| Header | Types | Macros |
| --- | --- | --- |
| **stdint.h** | `uint8_t`, `uint16_t`, <br/> `int32_t`, `uint32_t` | |
| **stdbool.h** | |  `bool`, `true`, `false` |
| **assert.h** | |  `assert` |

## How Charisma is Tested

* 100% branch coverage
* Unit tests
* Fuzz tests
* Static analysis
* Valgrind analysis
* Code sanitizers (UBSAN, ASAN, and MSAN)
* Extensive use of assert() and run-time checks

## Example

This code snippet demonstrates how to decode UTF-8 text.

```c
const char8_t *string = "The quick 갈색 🦊 กระโดด över the 怠け者 🐶.";
int32_t index = 0;
for (;;)
{
    uchar cp = 0x0;
    int32_t r = utf8_decode(string, -1, &index, &cp);
    if (r == 0)
    {
        break; // end of string
    }
    else if (r < 0)
    {
        // malformed character sequence
    }

    // Malformed character sequences will be
    // recovered from and returned as U+FFFD.
    printf("U+%04X\n", cp);
}
```

## Building

Download the [latest release](https://github.com/railgunlabs/charisma/releases/) and build with

```
$ ./configure
$ make
$ make install
```

or build with [CMake](https://cmake.org/).

## Related Work

Charisma is focused on decoding and encoding Unicode characters.
If you need Unicode algorithms, like normalization or collation, then use [Unicorn](https://github.com/railgunlabs/unicorn).

## License

Charisma is dual-licensed under the GNU Lesser General Public License version 3 (LGPL v3) and a proprietary license, which can be purchased from [Railgun Labs](https://railgunlabs.com/charisma/license/).

The unit tests are **not** open source.
Access to them is granted exclusively to commercial licensees.

_Unicode® is a registered trademark of Unicode, Inc. in the United States and other countries. This project is not in any way associated with or endorsed or sponsored by Unicode, Inc. (aka The Unicode Consortium)._
