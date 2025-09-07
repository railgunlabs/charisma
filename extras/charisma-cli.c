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

// The Charisma command-line interface reads Unicode characters from stdin
// and converts them to the specified character encoding form.

// This program does not attempt to be MISRA C compliant.

#include "charisma.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

// The longest UTF-* code unit sequence appears in UTF-8 which can encode characters
// using up to four code units (bytes).
#define LONGEST_CODE_UNIT_SEQUENCE 4

enum
{
    EXIT_NO_ERROR,
    EXIT_DECODE_ERROR,
    EXIT_GENERAL_ERROR,
    EXIT_OPTION_ERROR,
};

enum encoding
{
    UNKNOWN,
    UTF8,
    UTF16LE,
    UTF16BE,
    UTF32LE,
    UTF32BE,
};

struct options
{
    enum encoding from_encoding;
    enum encoding to_encoding;
    bool repair;
};

static int process_buffer(const struct options *opts, const void *bytes, int32_t bytes_count, int32_t *bytes_processed, int32_t total_bytes_processed, bool is_end)
{
    uchar codepoint = 0x0;
    int32_t code_unit_index = 0;
    int32_t code_unit_count = 0;
    int32_t prev_byte_index = 0;

    switch (opts->from_encoding)
    {
    case UTF8:
        code_unit_count = bytes_count;
        break;

    case UTF16LE:
    case UTF16BE:
        code_unit_count = bytes_count / sizeof(uint16_t);
        break;

    case UTF32LE:
    case UTF32BE:
        code_unit_count = bytes_count / sizeof(uint32_t);
        break;
    }

    for (;;)
    {
        // Decode the next Unicode scalar value from the byte stream.
        int32_t result = 0;
        switch (opts->from_encoding)
        {
        case UTF8:
            prev_byte_index = code_unit_index;
            result = utf8_decode(bytes, code_unit_count, &code_unit_index, &codepoint);
            break;

        case UTF16LE:
            prev_byte_index = code_unit_index * sizeof(uint16_t);
            result = utf16le_decode(bytes, code_unit_count, &code_unit_index, &codepoint);
            break;

        case UTF16BE:
            prev_byte_index = code_unit_index * sizeof(uint16_t);
            result = utf16be_decode(bytes, code_unit_count, &code_unit_index, &codepoint);
            break;

        case UTF32LE:
            prev_byte_index = code_unit_index * sizeof(uint32_t);
            result = utf32le_decode(bytes, code_unit_count, &code_unit_index, &codepoint);
            break;

        case UTF32BE:
            prev_byte_index = code_unit_index * sizeof(uint32_t);
            result = utf32be_decode(bytes, code_unit_count, &code_unit_index, &codepoint);
            break;

        default:
            return EXIT_GENERAL_ERROR;
        }

        // The entire buffer was read successfully.
        if (result == 0)
        {
            *bytes_processed = bytes_count;
            break;
        }

        // If a malformed character sequence was found.
        if (result < 0)
        {
            // If the malformed character is near the end of the buffer, then that could mean
            // it's actually a truncated character. In this case, rewind to the point prior
            // to this character and return success. This keeps the truncated character
            // in the buffer until more bytes can be appended (presumably completing
            // the character).
            if (!is_end)
            {
                if ((prev_byte_index + LONGEST_CODE_UNIT_SEQUENCE) >= bytes_count)
                {
                    *bytes_processed = prev_byte_index;
                    break;
                }
            }

            // If user doesn't want malformed characters repaired, then exit with an error.
            if (!opts->repair)
            {
                fprintf(stderr, "error: malformed character at byte: %d\n", total_bytes_processed + prev_byte_index);
                return EXIT_DECODE_ERROR;
            }
        }

        // Re-encode the code point.
        uint8_t buf[4];
        int32_t buflen;
        switch (opts->to_encoding)
        {
        case UTF8:
            buflen = utf8_encode(codepoint, (void *)buf);
            break;

        case UTF16LE:
            buflen = utf16le_encode(codepoint, (void *)buf) * sizeof(uint16_t);
            break;

        case UTF16BE:
            buflen = utf16be_encode(codepoint, (void *)buf) * sizeof(uint16_t);
            break;

        case UTF32LE:
            buflen = utf32le_encode(codepoint, (void *)buf) * sizeof(uint32_t);
            break;

        case UTF32BE:
            buflen = utf32be_encode(codepoint, (void *)buf) * sizeof(uint32_t);
            break;

        default:
            return EXIT_GENERAL_ERROR;
        }

        // Write the bytes as-is to standard output.
        for (int32_t i = 0; i < buflen; i++)
        {
            putc(buf[i], stdout);
        }
    }

    return EXIT_NO_ERROR;
}

static int encode_input(const struct options *opts)
{
    char buffer[4096];
    int32_t buffer_length = 0;
    int32_t total_bytes_processed = 0;

    for (;;)
    {
        // The buffer should typically be clared before the next call to read(), but if it isn't, then
        // that means there might have been a truncated code point.
        assert(buffer_length <= LONGEST_CODE_UNIT_SEQUENCE);

        // Read a page of bytes from standard input (stdin).
        const ssize_t bytes_read = read(STDIN_FILENO, &buffer[buffer_length], sizeof(buffer) - LONGEST_CODE_UNIT_SEQUENCE);
        if (bytes_read == 0)
        {
            break;
        }
        else if (bytes_read < 0)
        {
            return EXIT_GENERAL_ERROR;
        }
        buffer_length += (int32_t)bytes_read;

        // Process the bytes queued up so far in the buffer.
        int32_t bytes_processed = 0;
        const int result = process_buffer(opts, (const void *)buffer, buffer_length, &bytes_processed, total_bytes_processed, false);
        if (result != EXIT_NO_ERROR)
        {
            return result;
        }

        // Check for signed integer overflow.
        if (total_bytes_processed > INT32_MAX - bytes_processed)
        {
            fprintf(stderr, "error: maximum number of bytes processed: 0x%x\n", INT32_MAX);
            return EXIT_GENERAL_ERROR;
        }

        // Remove the bytes that have been processed by shifting down the contents of the buffer.
        total_bytes_processed += bytes_processed;
        buffer_length -= bytes_processed;
        memmove(buffer, &buffer[bytes_processed], buffer_length);
    }

    // Process any remaining bytes in the buffer.
    if (buffer_length > 0)
    {
        int32_t bytes_processed = 0;
        return process_buffer(opts, buffer, buffer_length, &bytes_processed, total_bytes_processed, true);
    }

    return EXIT_NO_ERROR;
}

static void display_help(void)
{
    puts("Usage: charisma [-r] -f ENCODING -t ENCODING");
    puts("");
    puts("Charisma is a command-line interface to the C library of the same");
    puts("name. This program reads Unicode text from stdin, converts it to");
    puts("the specified encoding form, and writes the result to stdout.");
    puts("Errors are written to stderr.");
    puts("");
    puts("Options:");
    puts("  -f ENCODING, --from=ENCODING");
    puts("                      Input character encoding (read from stdin).");
    puts("                      See documentation for option '-t' for valid");
    puts("                      values for ENCODING.");
    puts("  -t ENCODING, --to=ENCODING");
    puts("                      Output character encoding (written to stdout).");
    puts("                      Where ENCODING is one of:");
    puts("                        utf8");
    puts("                        utf16     (native byte-order)");
    puts("                        utf16be");
    puts("                        utf16le");
    puts("                        utf32     (native byte-order)");
    puts("                        utf32be");
    puts("                        utf32le");
    puts("");
    puts("  -r, --repair        Replace malformed character sequences with the");
    puts("                      Unicode replacement character (U+FFFD). If this");
    puts("                      option is omitted, then Charisma will exit if");
    puts("                      a malformed byte sequence is detected.");
    puts("");
    puts("  -v, --version       Prints the Charisma version and exits.");
    puts("  -h, --help          Prints this help message and exits.");
    puts("");
    puts("Exit status:");
    puts("  0  if OK,");
    puts("  1  if the input is malformed,");
    puts("  2  if a general error occured while processing the input,");
    puts("  3  if an invalid command-line option is specified.");
    puts("");
    puts("Charisma website and online documentation: <https://railgunlabs.com/charisma/");
    puts("Charisma repository: <https://github.com/railgunlabs/charisma/");
    puts("");
    puts("Charisma is Free Software distributed under the GNU General Public License");
    puts("version 3 as published by the Free Software Foundation. You may also");
    puts("license Charisma under a commercial license, as set out at");
    puts("<https://railgunlabs.com/charisma/license/>.");
}

static enum encoding encoding_string_to_enum(const char *encoding)
{
    // Convert the raw encoding provided by the user, e.g. "UTF-16_BE" to
    // a normalized encoding form, e.g. "utf16be" for direct comparison.
    char normalized[16] = {0};
    size_t normalized_length = 0;
    while (*encoding)
    {
        const char ch = *encoding;
        if (ch != '_' && ch != '-' && ch < 127)
        {
            if (normalized_length < sizeof(normalized) - 1)
            {
                normalized[normalized_length] = tolower(ch);
                normalized_length += 1;
            }
        }
        encoding++;
    }

    // Match the character encoding against supported encodings.
    if (strcmp(normalized, "utf8") == 0)
    {
        return UTF8;
    }
    else if (strcmp(normalized, "utf16be") == 0)
    {
        return UTF16BE;
    }
    else if (strcmp(normalized, "utf16le") == 0)
    {
        return UTF16LE;
    }
    else if (strcmp(normalized, "utf16") == 0)
    {
#if defined(HAVE_BIG_ENDIAN)
        return UTF16BE;
#else
        return UTF16LE;
#endif
    }
    else if (strcmp(normalized, "utf32be") == 0)
    {
        return UTF32BE;
    }
    else if (strcmp(normalized, "utf32le") == 0)
    {
        return UTF32LE;   
    }
    else if (strcmp(normalized, "utf32") == 0)
    {
#if defined(HAVE_BIG_ENDIAN)
        return UTF32BE;
#else
        return UTF32LE;
#endif
    }
    else
    {
        return UNKNOWN;
    }
}

int main(int argc, char *argv[])
{
    struct options opts = {0};
    for (int index = 1; index < argc; index++)
    {
        const char *arg = argv[index];
        if (strcmp(arg, "-h") == 0 ||
            strcmp(arg, "--help") == 0)
        {
            display_help();
            return EXIT_SUCCESS;
        }

        if (strcmp(arg, "-v") == 0 ||
            strcmp(arg, "--version") == 0)
        {
            puts("1.1.0");
            return EXIT_SUCCESS;
        }

        if (strcmp(arg, "-r") == 0 ||
            strcmp(arg, "--repair") == 0)
        {
            opts.repair = true;
            continue;
        }

        if (strcmp(arg, "-f") == 0 || strcmp(arg, "-t") == 0 ||
            strncmp(arg, "--from=", 7) == 0 || strncmp(arg, "--to=", 5) == 0)
        {
            // Parse the character encoding.
            char direction;
            if (arg[1] != '-')
            {
                if (index == argc - 1)
                {
                    fprintf(stderr, "error: expected character encoding\n");
                    return EXIT_OPTION_ERROR;
                }
                direction = arg[1];
                index += 1;
                arg = argv[index];
            }
            else
            {
                direction = arg[2];
                if (arg[6] == '=')
                {
                    arg += 7;
                }
                else
                {
                    assert(arg[4] == '=');
                    arg += 5;
                }
            }

            const enum encoding encoding = encoding_string_to_enum(arg);
            if (encoding == UNKNOWN)
            {
                fprintf(stderr, "error: unsupported character encoding '%s'\n", arg);
                return EXIT_OPTION_ERROR;
            }

            if (direction == 'f')
            {
                opts.from_encoding = encoding;
            }
            else
            {
                assert(direction == 't');
                opts.to_encoding = encoding;
            }
            continue;
        }

        fprintf(stderr, "error: unknown option '%s'\n", arg);
        return EXIT_OPTION_ERROR;
    }

    if (argc < 2)
    {
        display_help();
        return EXIT_OPTION_ERROR;
    }

    if (opts.from_encoding == UNKNOWN)
    {
        fputs("error: missing --from\n", stderr);
        return EXIT_OPTION_ERROR;
    }

    if (opts.to_encoding == UNKNOWN)
    {
        opts.to_encoding = opts.from_encoding;
    }

    return encode_input(&opts);
}
