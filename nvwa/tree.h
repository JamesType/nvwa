// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2017-2025 Wu Yongwei <wuyongwei at gmail dot com>
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
 * @file  tree.h
 *
 * A generic tree class template and the traversal utilities.  Using
 * this file requires a C++11-compliant compiler.
 *
 * @date  2025-07-06
 */

#ifndef NVWA_TREE_H
#define NVWA_TREE_H

#include <assert.h>             // assert
#include <stddef.h>             // ptrdiff_t
#include <algorithm>            // std::remove_if
#include <iterator>             // std::begin/end/make_move_iterator
#include <memory>               // std::unique_ptr/shared_ptr
#include <ostream>              // std::ostream
#include <stack>                // std::stack
#include <string>               // std::string
#include <tuple>                // std::tuple/make_tuple
#include <type_traits>          // std::decay
#include <utility>              // std::declval/forward/move/pair/...
#include <vector>               // std::vector
#include "_nvwa.h"              // NVWA_NAMESPACE_*
#include "c++_features.h"       // NVWA_CXX11_REQUIRES

NVWA_NAMESPACE_BEGIN

/** Policy class for how to store members. */
enum class storage_policy {
    unique,  ///< Members are directly owned
    shared,  ///< Members may be shared and passed around
};

#ifndef NVWA_TREE_DEFAULT_STORAGE_POLICY
#define NVWA_TREE_DEFAULT_STORAGE_POLICY storage_policy::shared
#endif

/** Declaration of policy class to generate the smart pointer type. */
template <typename _Tp, storage_policy _Policy>
struct smart_ptr;

/** Partial specialization to get std::unique_ptr. */
template <typename _Tp>
struct smart_ptr<_Tp, storage_policy::unique> {
    typedef std::unique_ptr<_Tp> type;
};

/** Partial specialization to get std::shared_ptr. */
template <typename _Tp>
struct smart_ptr<_Tp, storage_policy::shared> {
    typedef std::shared_ptr<_Tp> type;
};

/**
 * Basic tree (node) class template that owns all its children.
 */
template <typename _Tp,
          storage_policy _Policy = NVWA_TREE_DEFAULT_STORAGE_POLICY>
class tree {
public:
    typedef _Tp                                     value_type;
    typedef typename smart_ptr<tree, _Policy>::type tree_ptr;
    typedef std::vector<tree_ptr>                   children_type;
    typedef typename children_type::iterator        iterator;
    typedef typename children_type::const_iterator  const_iterator;

    tree() = default;
    template <typename _Up,
              NVWA_CXX11_REQUIRES(!std::is_same_v<std::decay_t<_Up>, tree>)>
    explicit tree(_Up&& value, children_type children = {})
        : _M_value(std::forward<_Up>(value)),
          _M_children(std::move(children))
    {
    }
    _Tp& value() &
    {
        return _M_value;
    }
    const _Tp& value() const &
    {
        return _M_value;
    }
    _Tp&& value() &&
    {
        return std::move(_M_value);
    }
    tree_ptr& child(unsigned index)
    {
        return _M_children[index];
    }
    const tree_ptr& child(unsigned index) const
    {
        return _M_children[index];
    }
    void push_back(tree_ptr ptr)
    {
        _M_children.push_back(std::move(ptr));
    }
    void pop_back()
    {
        _M_children.pop_back();
    }
    iterator begin()
    {
        return _M_children.begin();
    }
    const_iterator begin() const
    {
        return _M_children.begin();
    }
    const_iterator cbegin() const
    {
        return _M_children.begin();
    }
    iterator end()
    {
        return _M_children.end();
    }
    const_iterator end() const
    {
        return _M_children.end();
    }
    const_iterator cend() const
    {
        return _M_children.end();
    }
    tree_ptr& front()
    {
        return _M_children.front();
    }
    const tree_ptr& front() const
    {
        return _M_children.front();
    }
    tree_ptr& back()
    {
        return _M_children.back();
    }
    const tree_ptr& back() const
    {
        return _M_children.back();
    }
    bool has_child() const
    {
        return !_M_children.empty();
    }
    size_t child_count() const
    {
        return _M_children.size();
    }

    // Removes children iteratively, in case the recursive destruction
    // of children causes problems to stack use.  This algorithm does
    // not take extra space, but can possibly iterate through the nodes
    // more than once.
    void remove_children()
    {
        while (has_child()) {
            children_type* children_ptr = &_M_children;
            while (!children_ptr->empty()) {
                while (children_ptr->back() &&
                       children_ptr->back()->has_child()) {
                    children_ptr = &children_ptr->back()->_M_children;
                }
                children_ptr->pop_back();
            }
        }
    }

    template <typename... Args>
    void set_children(Args&&... args)
    {
        // initializer_list cannot be used to initialize a list of
        // unique_ptrs; thus this workaround
        tree_ptr init[] = {std::forward<Args>(args)...};
        children_type children(std::make_move_iterator(std::begin(init)),
                               std::make_move_iterator(std::end(init)));
        _M_children.swap(children);
    }

    template <typename... Args>
    static children_type make_children(Args&&... args)
    {
        tree_ptr init[] = {std::forward<Args>(args)...};
        children_type children(std::make_move_iterator(std::begin(init)),
                               std::make_move_iterator(std::end(init)));
        return children;
    }

    template <typename _Up>
    static tree_ptr create(_Up&& value, children_type children)
    {
        return tree_ptr(new tree(std::forward<_Up>(value),
                                 std::move(children)));
    }

    // Destroys node and removes children iteratively, in case the
    // recursive destruction of children causes stack problems.  It can
    // be used when there are more than two children in a node (space
    // overhead would occur then), but is optimized for the two-child
    // case.
    [[deprecated("remove_children is probably a better alternative")]]
    static void destroy(tree_ptr& ptr)
    {
        auto current = std::move(ptr);
        auto parent = null();
        while (current) {
            if (current->_M_children.empty()) {
                // Remove current node and go up
                current = std::move(parent);
                continue;
            }
            if (parent) {
                /* Transform
                 *
                 *                   4  <-- parent
                 *                  / \
                 *    current -->  2   5
                 *                / \
                 *               1   3
                 *
                 * to
                 *
                 *                          parent -->  (null)
                 *    current -->  2
                 *                / \
                 *               1   4
                 *                  / \
                 *                 3   5
                 */
                assert(parent->_M_children.size() > 0 &&    // Ensured below
                       parent->_M_children[0] == nullptr);  // Was current
                if (parent->_M_children.size() > 1 &&
                    parent->_M_children[1] != nullptr) {
                    if (current->_M_children.size() > 2) {
                        parent->_M_children.insert(
                            parent->_M_children.end(),
                            std::make_move_iterator(
                                current->_M_children.begin() + 2),
                            std::make_move_iterator(
                                current->_M_children.end()));
                    }
                    avoid_sole(current->_M_children);
                    parent->_M_children[0] = std::move(current->_M_children[1]);
                    current->_M_children[1] = std::move(parent);
                } else {
                    // Safe to remove, as its children are nullptr
                    parent = nullptr;
                }
            }
            // Now parent is null
            if (current->_M_children[0]) {
                // Go down the left child
                parent = std::move(current);
                current = std::move(parent->_M_children[0]);
            } else if (current->_M_children.size() == 2) {
                // Remove current node and go down the (only) right child
                current = std::move(current->_M_children[1]);
            } else {
                // More than two children and the leftmost child is null
                remove_null_but_avoid_sole(current->_M_children);
            }
        }
    }

    static constexpr tree_ptr null()
    {
        return tree_ptr();
    }

private:
    static void remove_null_but_avoid_sole(children_type& children)
    {
        children.erase(std::remove_if(children.begin(), children.end(),
                                      [](const tree_ptr& ptr) {
                                          return ptr == nullptr;
                                      }),
                       children.end());
        avoid_sole(children);
    }

    static void avoid_sole(children_type& children)
    {
        if (children.size() == 1) {
            children.resize(2);
        }
    }

    _Tp           _M_value{};
    children_type _M_children;
};

/**
 * Creates a tree without any children.
 *
 * @param value  the value to assign to the tree node
 * @return       the smart pointer to the newly created tree
 */
template <storage_policy _Policy = NVWA_TREE_DEFAULT_STORAGE_POLICY,
          typename _Tp>
typename tree<typename std::decay<_Tp>::type, _Policy>::tree_ptr
create_tree(_Tp&& value)
{
    return tree<typename std::decay<_Tp>::type, _Policy>::create(
        std::forward<_Tp>(value), {});
}

/**
 * Creates a tree with children.
 *
 * @param value  the value to assign to the tree node
 * @param args   the smart pointers to children of the tree
 * @return       the smart pointer to the newly created tree
 */
template <storage_policy _Policy = NVWA_TREE_DEFAULT_STORAGE_POLICY,
          typename _Tp, typename... Args>
typename tree<typename std::decay<_Tp>::type, _Policy>::tree_ptr
create_tree(_Tp&& value, Args&&... args)
{
    return tree<typename std::decay<_Tp>::type, _Policy>::create(
        std::forward<_Tp>(value),
        tree<_Tp, _Policy>::make_children(std::forward<Args>(args)...));
}

template <typename _Tree>
void print_tree(const typename _Tree::tree_ptr& ptr, std::ostream& os,
                const std::string& prefix)
{
    if (ptr) {
        os << ptr->value();
    } else {
        os << "(null)";
    }
    os << '\n';

    if (ptr == nullptr) {
        return;
    }

    for (const auto& child : *ptr) {
        bool is_last = &child == &ptr->back();
        os << prefix;
        os << (is_last ? "└──" : "├──");
        os << ' ';
        print_tree<_Tree>(child, os, prefix + (is_last ? "    " : "│   "));
    }
}

template <typename _TreePtr>
auto print_tree(const _TreePtr& ptr, std::ostream& os)
    -> decltype(typename std::decay_t<decltype(*ptr)>::tree_ptr(), void())
{
    using tree_type = std::decay_t<decltype(*ptr)>;
    print_tree<tree_type>(ptr, os, "");
}

/**
 * Iteration class for breadth-first traversal.  Mutating (adding or
 * removing children) or removing the part of the tree nodes at the
 * current traversal level has undefined behaviour.
 */
template <typename _Tree>
class breadth_first_iteration {
public:
    class iterator {
    public:
        typedef ptrdiff_t                 difference_type;
        typedef _Tree                     value_type;
        typedef _Tree*                    pointer;
        typedef _Tree&                    reference;
        typedef std::forward_iterator_tag iterator_category;

        iterator() = default;
        explicit iterator(pointer root) : _M_this_level({root})
        {
            _M_current = _M_this_level.begin();
        }

        reference operator*() const
        {
            assert(!empty());
            return **_M_current;
        }
        pointer operator->() const
        {
            assert(!empty());
            return &*_M_current;
        }
        iterator& operator++()
        {
            assert(!empty());
            for (auto& child : **_M_current) {
                if (child != _Tree::null()) {
                    _M_next_level.push_back(&*child);
                }
            }
            if (++_M_current == _M_this_level.end()) {
                _M_this_level = std::move(_M_next_level);
                _M_current = _M_this_level.begin();
            }
            return *this;
        }
        iterator operator++(int)
        {
            iterator temp(*this);
            ++*this;
            return temp;
        }
        bool empty() const
        {
            return _M_current == _M_this_level.end();
        }
        bool operator==(const iterator& rhs) const
        {
            return (empty() && rhs.empty()) || _M_current == rhs._M_current;
        }
        bool operator!=(const iterator& rhs) const
        {
            return !operator==(rhs);
        }

    private:
        typename std::vector<pointer>::iterator _M_current;
        std::vector<pointer>                    _M_this_level;
        std::vector<pointer>                    _M_next_level;
    };

    explicit breadth_first_iteration(_Tree& root) : _M_root(&root) {}
    iterator begin()
    {
        return iterator{_M_root};
    }
    iterator end()
    {
        return iterator{};
    }

private:
    _Tree* _M_root;
};

/**
 * Iteration class for depth-first traversal.  Mutating (adding or
 * removing children) or removing the part of the tree nodes that have
 * already been traversed has undefined behaviour.
 */
template <typename _Tree>
class depth_first_iteration {
public:
    class iterator {
    public:
        typedef ptrdiff_t                 difference_type;
        typedef _Tree                     value_type;
        typedef _Tree*                    pointer;
        typedef _Tree&                    reference;
        typedef std::forward_iterator_tag iterator_category;

        iterator() : _M_current(nullptr) {}
        explicit iterator(pointer root) : _M_current(root) {}

        reference operator*() const
        {
            assert(!empty());
            return *_M_current;
        }
        pointer operator->() const
        {
            assert(!empty());
            return _M_current;
        }
        iterator& operator++()
        {
            assert(!empty());
            if (_M_current->cbegin() != _M_current->cend()) {
                _M_stack.push(std::make_pair(_M_current->cbegin(),
                                             _M_current->cend()));
            }
            for (;;) {
                if (_M_stack.empty()) {
                    _M_current = nullptr;
                    break;
                }
                auto& top = _M_stack.top();
                auto& next_node = top.first;
                auto& end_node = top.second;
                if (next_node != end_node) {
                    _M_current = &**next_node;
                    ++next_node;
                    if (_M_current != nullptr) {
                        break;
                    }
                } else {
                    _M_stack.pop();
                }
            }
            return *this;
        }
        iterator operator++(int)
        {
            iterator temp(*this);
            ++*this;
            return temp;
        }
        bool empty() const
        {
            return _M_current == nullptr;
        }
        bool operator==(const iterator& rhs) const
        {
            return _M_current == rhs._M_current;
        }
        bool operator!=(const iterator& rhs) const
        {
            return !operator==(rhs);
        }

    private:
        pointer _M_current;
        std::stack<std::pair<typename _Tree::const_iterator,
                             typename _Tree::const_iterator>>
            _M_stack;
    };

    explicit depth_first_iteration(_Tree& root) : _M_root(&root) {}
    iterator begin()
    {
        return iterator{_M_root};
    }
    iterator end()
    {
        return iterator{};
    }

private:
    _Tree* _M_root;
};

/**
 * Iteration class for in-order traversal.  Mutating (adding or removing
 * children) or removing the part of the tree nodes that have already
 * been traversed has undefined behaviour.
 */
template <typename _Tree>
class in_order_iteration {
public:
    class iterator {
    public:
        typedef ptrdiff_t                 difference_type;
        typedef _Tree                     value_type;
        typedef _Tree*                    pointer;
        typedef _Tree&                    reference;
        typedef std::forward_iterator_tag iterator_category;

        iterator() : _M_current(nullptr) {}
        explicit iterator(pointer root)
            : _M_current(find_leftmost_child(root))
        {
        }

        reference operator*() const
        {
            assert(!empty());
            return *_M_current;
        }
        pointer operator->() const
        {
            assert(!empty());
            return _M_current;
        }
        iterator& operator++()
        {
            assert(!empty());
            for (;;) {
                if (_M_stack.empty()) {
                    _M_current = nullptr;
                    break;
                }
                auto& top = _M_stack.top();
                auto& curr       = std::get<0>(top);
                auto& next_child = std::get<1>(top);
                auto& end_child  = std::get<2>(top);
                if (curr != nullptr) {
                    _M_current = curr;
                    curr = nullptr;  // Mark as traversed
                    break;
                } else {
                    // Iterate over non-leftmost children
                    while (next_child != end_child) {
                        auto it = next_child++;
                        if (*it) {
                            _M_current = find_leftmost_child(&**it);
                            return *this;
                        }
                    }
                    _M_stack.pop();
                }
            }
            return *this;
        }
        iterator operator++(int)
        {
            iterator temp(*this);
            ++*this;
            return temp;
        }
        bool empty() const
        {
            return _M_current == nullptr;
        }
        bool operator==(const iterator& rhs) const
        {
            return _M_current == rhs._M_current;
        }
        bool operator!=(const iterator& rhs) const
        {
            return !operator==(rhs);
        }

    private:
        pointer find_leftmost_child(pointer root)
        {
            using std::make_tuple;
            auto curr = root;
            for (;;) {
                if (!curr->has_child()) {
                    break;
                }
                auto left_child = curr->cbegin();
                auto next_child = left_child;
                if (next_child != curr->cend()) {
                    ++next_child;
                }
                if (*left_child) {
                    _M_stack.push(
                        make_tuple(curr, next_child, curr->cend()));
                    curr = &**left_child;
                } else {
                    _M_stack.push(
                        make_tuple(nullptr, next_child, curr->cend()));
                    break;
                }
            }
            return curr;
        }

        std::stack<std::tuple<pointer,
                              typename _Tree::const_iterator,
                              typename _Tree::const_iterator>>
            _M_stack;
        pointer _M_current;
    };

    explicit in_order_iteration(_Tree& root) : _M_root(&root) {}
    iterator begin()
    {
        return iterator{_M_root};
    }
    iterator end()
    {
        return iterator{};
    }

private:
    _Tree* _M_root;
};

template <template <typename> class _Iteration, typename _Tree>
_Iteration<_Tree> traverse(_Tree& root)
{
    return _Iteration<_Tree>{root};
}

NVWA_NAMESPACE_END

#endif // NVWA_TREE_H
