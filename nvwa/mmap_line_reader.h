// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2016-2025 Wu Yongwei <wuyongwei at gmail dot com>
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
 * @file  mmap_line_reader.h
 *
 * Header file for mmap_line_reader and mmap_line_reader_sv, easy-to-use
 * line-based file readers.  It is implemented with memory-mapped file APIs.
 *
 * @date  2025-02-02
 */

#ifndef NVWA_MMAP_LINE_READER_H
#define NVWA_MMAP_LINE_READER_H

#include <assert.h>             // assert
#include <stddef.h>             // ptrdiff_t/size_t
#include <iterator>             // std::forward_iterator_tag
#include "_nvwa.h"              // NVWA_NAMESPACE_*
#include "c++_features.h"       // HAVE_CXX17_STRING_VIEW/HAVE_CXX20_RANGES
#include "mmap_reader_base.h"   // nvwa::mmap_reader_base

#include <string>               // std::string
#if HAVE_CXX17_STRING_VIEW
#include <string_view>          // std::string_view
#endif

NVWA_NAMESPACE_BEGIN

/** Class template to allow iteration over all lines of a mmappable file. */
template <typename _Tp>
class basic_mmap_line_reader : private mmap_reader_base {
public:
    /** Iterator that contains the line content. */
    class iterator {  // implements ForwardIterator
    public:
        typedef _Tp                       value_type;
        typedef const value_type*         pointer;
        typedef const value_type&         reference;
        typedef ptrdiff_t                 difference_type;
        typedef std::forward_iterator_tag iterator_category;

        iterator() = default;
        explicit iterator(basic_mmap_line_reader* reader)
            : _M_reader(reader)
        {
            ++*this;
        }

        reference operator*() const noexcept
        {
            assert(_M_reader != nullptr);
            return _M_line;
        }
        pointer operator->() const noexcept
        {
            assert(_M_reader != nullptr);
            return &_M_line;
        }
        iterator& operator++()
        {
            if (!_M_reader->read(_M_line, _M_offset)) {
                _M_reader = nullptr;
                _M_offset = 0;
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

    private:
        basic_mmap_line_reader* _M_reader{};
        size_t                  _M_offset{};
        value_type              _M_line;
    };

    /** Enumeration of whether the delimiter should be stripped. */
    enum strip_type {
        strip_delimiter,     ///< The delimiter should be stripped
        no_strip_delimiter,  ///< The delimiter should be retained
    };

    basic_mmap_line_reader() = default;
    explicit basic_mmap_line_reader(const char* path,
                                    char        delimiter = '\n',
                                    strip_type  strip = strip_delimiter)
        : mmap_reader_base(path),
          _M_delimiter(delimiter),
          _M_strip_delimiter(strip == strip_delimiter)
    {
    }
#if NVWA_WINDOWS
    explicit basic_mmap_line_reader(const wchar_t* path,
                                    char           delimiter = '\n',
                                    strip_type     strip = strip_delimiter)
        : mmap_reader_base(path),
          _M_delimiter(delimiter),
          _M_strip_delimiter(strip == strip_delimiter)
    {
    }
#endif
#if NVWA_UNIX
    explicit basic_mmap_line_reader(int        fd,
                                    char       delimiter = '\n',
                                    strip_type strip = strip_delimiter)
        : mmap_reader_base(fd),
          _M_delimiter(delimiter),
          _M_strip_delimiter(strip == strip_delimiter)
    {
    }
#endif

    using mmap_reader_base::open;
    using mmap_reader_base::close;
    using mmap_reader_base::is_open;

    void set_delimiter(char delimiter, strip_type strip = strip_delimiter)
    {
        _M_delimiter = delimiter;
        _M_strip_delimiter = strip == strip_delimiter;
    }

    iterator begin()
    {
        return iterator(this);
    }
    iterator end() const noexcept
    {
        return {};
    }

    bool read(_Tp& output, size_t& offset);

private:
    char  _M_delimiter{'\n'};
    bool  _M_strip_delimiter{true};
};

/**
 * Reads content from the mmaped file.
 *
 * @param[out]    output  object to receive the line
 * @param[in,out] offset  offset of reading pos on entry; end offset on exit
 * @return                \c true if line content is returned; \c false
 *                        otherwise
 */
template <typename _Tp>
bool basic_mmap_line_reader<_Tp>::read(_Tp& output, size_t& offset)
{
    if (offset == size()) {
        return false;
    }

    size_t pos = offset;
    bool found_delimiter = false;
    while (pos < size()) {
        char ch = data()[pos++];
        if (ch == _M_delimiter) {
            found_delimiter = true;
            break;
        }
    }

    output = _Tp(data() + offset,
                 pos - offset - (found_delimiter && _M_strip_delimiter));
    offset = pos;
    return true;
}

typedef basic_mmap_line_reader<std::string>      mmap_line_reader;
#if HAVE_CXX17_STRING_VIEW
typedef basic_mmap_line_reader<std::string_view> mmap_line_reader_sv;
#endif

NVWA_NAMESPACE_END

#if HAVE_CXX20_RANGES
#include <ranges>

template <>
inline constexpr bool std::ranges::enable_view<NVWA::mmap_line_reader> =
    true;
template <>
inline constexpr bool std::ranges::enable_view<NVWA::mmap_line_reader_sv> =
    true;
#endif

#endif // NVWA_MMAP_LINE_READER_H
