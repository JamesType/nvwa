// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2016-2024 Wu Yongwei <wuyongwei at gmail dot com>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any
 * damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute
 * it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must
 *    not claim that you wrote the original software.  If you use this
 *    software in a product, an acknowledgement in the product
 *    documentation would be appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must
 *    not be misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source
 *    distribution.
 *
 * This file is part of Stones of Nvwa:
 *      https://github.com/adah1972/nvwa
 *
 */

/**
 * @file  file_line_reader.cpp
 *
 * Code for file_line_reader, an easy-to-use line-based file reader.
 *
 * @date  2024-05-20
 */

#include "file_line_reader.h"   // file_line_reader
#include <stdio.h>              // fgets/fread/size_t
#include <string.h>             // memcpy/strlen
#include <utility>              // std::move/swap
#include "_nvwa.h"              // NVWA_NAMESPACE_*

NVWA_NAMESPACE_BEGIN

const size_t BUFFER_SIZE = 256;

/**
 * Constructs the beginning iterator.
 *
 * @param reader  pointer to the file_line_reader object
 */
file_line_reader::iterator::iterator(file_line_reader* reader)
    : _M_reader(reader),
      _M_line(new char[BUFFER_SIZE]),
      _M_capacity(BUFFER_SIZE)
{
    ++*this;
}

/** Destuctor. */
file_line_reader::iterator::~iterator()
{
    delete[] _M_line;
}

/**
 * Copy-constructor.  The line content will be copied to the newly
 * constructed iterator.
 *
 * @param rhs  the iterator to copy from
 */
file_line_reader::iterator::iterator(const iterator& rhs)
    : _M_reader(rhs._M_reader),
      _M_offset(rhs._M_offset),
      _M_size(rhs._M_size),
      _M_capacity(rhs._M_capacity)
{
    if (rhs._M_line) {
        _M_line = new char[rhs._M_capacity];
        memcpy(_M_line, rhs._M_line, _M_size + 1);
    }
}

/**
 * Assignment.  The line content will be copied to the newly constructed
 * iterator.
 *
 * @param rhs  the iterator to copy from
 */
file_line_reader::iterator&
file_line_reader::iterator::operator=(const iterator& rhs)
{
    iterator temp(rhs);
    swap(temp);
    return *this;
}

/**
 * Move constructor. The line content will be moved to the newly
 * constructed iterator.
 *
 * @param rhs  the iterator to move from
 */
file_line_reader::iterator::iterator(iterator&& rhs) noexcept
    : _M_reader(rhs._M_reader),
      _M_offset(rhs._M_offset),
      _M_line(rhs._M_line),
      _M_size(rhs._M_size),
      _M_capacity(rhs._M_capacity)
{
    rhs._M_reader = nullptr;
    rhs._M_offset = 0;
    rhs._M_line = nullptr;
    rhs._M_size = 0;
    rhs._M_capacity = 0;
}

/**
 * Move assignment. The line content will be moved to the newly
 * constructed iterator.
 *
 * @param rhs  the iterator to move from
 */
file_line_reader::iterator&
file_line_reader::iterator::operator=(iterator&& rhs) noexcept
{
    iterator temp(std::move(rhs));
    swap(temp);
    return *this;
}

/**
 * Swaps the iterator with another.
 *
 * @param rhs  the iterator to swap with
 */
void file_line_reader::iterator::swap(
    file_line_reader::iterator& rhs) noexcept
{
    std::swap(_M_reader, rhs._M_reader);
    std::swap(_M_offset, rhs._M_offset);
    std::swap(_M_line, rhs._M_line);
    std::swap(_M_size, rhs._M_size);
    std::swap(_M_capacity, rhs._M_capacity);
}

/**
 * Constructor.
 *
 * @param stream     the file stream to read from
 * @param delimiter  the delimiter between text `lines' (default to LF)
 * @param strip      enumerator about whether to strip the delimiter
 */
file_line_reader::file_line_reader(FILE* stream, char delimiter,
                                   strip_type strip)
    : _M_stream(stream),
      _M_delimiter(delimiter),
      _M_strip_delimiter(strip == strip_delimiter)
{
    if (delimiter == '\n') {
        _M_buffer = nullptr;
    } else {
        _M_buffer = new char[BUFFER_SIZE];
    }
}

/** Destructor. */
file_line_reader::~file_line_reader()
{
    delete[] _M_buffer;
}

namespace {

char* expand(char* data, size_t size, size_t capacity)
{
    char* new_ptr = new char[capacity];
    memcpy(new_ptr, data, size);
    delete[] data;
    return new_ptr;
}

void get_line(char*& output, size_t& capacity, bool& found_delimiter,
              size_t& write_pos, FILE* stream)
{
    for (;;) {
        if (!fgets(output + write_pos,
                   static_cast<int>(capacity - write_pos), stream)) {
            break;
        }
        size_t len = strlen(output + write_pos);
        write_pos += len;
        if (output[write_pos - 1] == '\n') {
            found_delimiter = true;
            break;
        }
        if (write_pos + 1 == capacity) {
            output = expand(output, write_pos, capacity * 2);
            capacity *= 2;
        }
    }
}

} // unnamed namespace

/**
 * Reads content from the file stream.  If necessary, the receiving
 * buffer will be expanded so that it is big enough to contain all the
 * line content.
 *
 * @param[in,out] output    initial receiving buffer
 * @param[out]    size      size of the line
 * @param[in,out] capacity  capacity of the initial receiving buffer on
 *                          entering the function; it can be increased
 *                          when necessary
 * @return                  \c true if line content is returned;
 *                          \c false otherwise
 */
bool file_line_reader::read(char*& output, size_t& size, size_t& capacity)
{
    bool found_delimiter = false;
    size_t write_pos = 0;

    if (_M_delimiter == '\n') {
        get_line(output, capacity, found_delimiter, write_pos, _M_stream);
    } else {
        for (;;) {
            if (_M_read_pos == _M_size) {
                _M_read_pos = 0;
                _M_size = fread(_M_buffer, 1, BUFFER_SIZE, _M_stream);
                if (_M_size == 0) {
                    break;
                }
            }
            char ch = _M_buffer[_M_read_pos++];
            if (write_pos + 1 == capacity) {
                output = expand(output, write_pos, capacity * 2);
                capacity *= 2;
            }
            output[write_pos++] = ch;
            if (ch == _M_delimiter) {
                found_delimiter = true;
                break;
            }
        }
    }
    _M_offset += write_pos;

    if (write_pos != 0) {
        if (found_delimiter && _M_strip_delimiter) {
            --write_pos;
        }
        output[write_pos] = '\0';
        size = write_pos;
        return true;
    } else {
        return false;
    }
}

NVWA_NAMESPACE_END
