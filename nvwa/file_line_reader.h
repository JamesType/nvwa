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
 * @file  file_line_reader.h
 *
 * Header file for file_line_reader, an easy-to-use line-based file reader.
 *
 * @date  2024-05-20
 */

#ifndef NVWA_FILE_LINE_READER_H
#define NVWA_FILE_LINE_READER_H

#include <assert.h>             // assert
#include <stddef.h>             // ptrdiff_t/size_t
#include <stdio.h>              // FILE
#include <iterator>             // std::input_iterator_tag
#include "_nvwa.h"              // NVWA_NAMESPACE_*

NVWA_NAMESPACE_BEGIN

/** Class to allow iteration over all lines of a text file. */
class file_line_reader {
public:
    /**
     * Iterator that contains the line content.
     *
     * The iterator \e owns the content.
     */
    class iterator {  // implements InputIterator
    public:
        typedef ptrdiff_t               difference_type;
        typedef char*                   value_type;
        typedef value_type*             pointer;
        typedef value_type&             reference;
        typedef std::input_iterator_tag iterator_category;

        iterator() = default;
        explicit iterator(file_line_reader* reader);
        ~iterator();

        iterator(const iterator& rhs);
        iterator& operator=(const iterator& rhs);
        iterator(iterator&& rhs) noexcept;
        iterator& operator=(iterator&& rhs) noexcept;

        void swap(iterator& rhs) noexcept;

        reference operator*()
        {
            assert(_M_reader != nullptr);
            return _M_line;
        }
        pointer operator->()
        {
            assert(_M_reader != nullptr);
            return &_M_line;
        }
        iterator& operator++()
        {
            if (!_M_reader->read(_M_line, _M_size, _M_capacity)) {
                _M_reader = nullptr;
                _M_offset = 0;
            } else {
                _M_offset = _M_reader->_M_offset;
            }
            return *this;
        }
        iterator operator++(int)
        {
            iterator temp(*this);
            ++*this;
            return temp;
        }

        bool operator==(const iterator& rhs) const noexcept
        {
            return _M_reader == rhs._M_reader && _M_offset == rhs._M_offset;
        }
        bool operator!=(const iterator& rhs) const noexcept
        {
            return !operator==(rhs);
        }

        size_t size() const { return _M_size; }

    private:
        file_line_reader* _M_reader{};
        size_t            _M_offset{};
        char*             _M_line{};
        size_t            _M_size{};
        size_t            _M_capacity{};
    };

    /** Enumeration of whether the delimiter should be stripped. */
    enum strip_type {
        strip_delimiter,     ///< The delimiter should be stripped
        no_strip_delimiter,  ///< The delimiter should be retained
    };

    explicit file_line_reader(FILE* stream, char delimiter = '\n',
                              strip_type strip = strip_delimiter);
    file_line_reader(const file_line_reader&) = delete;
    file_line_reader& operator=(const file_line_reader&) = delete;
    file_line_reader(file_line_reader&&) = default;
    file_line_reader& operator=(file_line_reader&&) = default;
    ~file_line_reader();

    iterator begin()
    {
        return iterator(this);
    }
    iterator end() const noexcept
    {
        return {};
    }
    bool read(char*& output, size_t& size, size_t& capacity);

private:
    FILE*  _M_stream;
    char   _M_delimiter;
    bool   _M_strip_delimiter;
    char*  _M_buffer;
    size_t _M_offset{};
    size_t _M_read_pos{};
    size_t _M_size{};
};

inline void swap(file_line_reader::iterator& lhs,
                 file_line_reader::iterator& rhs) noexcept
{
    lhs.swap(rhs);
}

NVWA_NAMESPACE_END

#endif // NVWA_FILE_LINE_READER_H
