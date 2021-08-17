/*
 * Vector.h
 *
 *  Created on: 2021-8-18
 *      Author: mzh
 */

#ifndef VECTOR_H_
#define VECTOR_H_

#include <initializer_list>
#include "iterator.h"
#include "memory.h"
#include "util.h"
#include "exceptdef.h"

namespace mystl
{

#ifdef max
#pragma message("#undefing marco max")
#undef max
#endif // max

#ifdef min
#pragma message("#undefing marco min")
#undef min
#endif // min

template<typename T>
struct vector
{
private:
    static_assert(!std::is_same<bool, T>::value, "vector<bool> is abandoned in mystl");

public:
    // vector 的嵌套型别定义
    typedef mystl::allocator<T>                      allocator_type;
    typedef mystl::allocator<T>                      data_allocator;

    typedef typename allocator_type::value_type      value_type;
    typedef typename allocator_type::pointer         pointer;
    typedef typename allocator_type::const_pointer   const_pointer;
    typedef typename allocator_type::reference       reference;
    typedef typename allocator_type::const_reference const_reference;
    typedef typename allocator_type::size_type       size_type;
    typedef typename allocator_type::difference_type difference_type;

    typedef value_type*                              iterator;
    typedef const value_type*                        const_iterator;
    typedef mystl::reverse_iterator<iterator>        reverse_iterator;
    typedef mystl::reverse_iterator<const_iterator>  const_reverse_iterator;

    allocator_type get_allocator() { return data_allocator(); }

private:
    iterator begin_;  // 表示目前使用空间的头部
    iterator end_;    // 表示目前使用空间的尾部
    iterator cap_;    // 表示目前储存空间的尾部

public:
    // 构造、复制、移动、析构函数
    vector() noexcept
    { try_init(); }

    explicit vector(size_type n)
    { fill_init(n, value_type()); }

    vector(size_type n, const value_type& value)
    { fill_init(n, value); }

    template <typename Iter, typename std::enable_if
        <mystl::is_input_iterator<Iter>::value, int>::type = 0>
    vector(Iter first, Iter last)
    {
        MYSTL_DEBUG(first <= last);
        range_init(first, last);
    }

    vector(const vector& rhs)
    { range_init(rhs.begin_, rhs.end_); }

    vector(vector&& rhs) noexcept
        : begin_(rhs.begin_), end_(rhs.end_), cap_(rhs.cap_)
    {
        rhs.begin_ = nullptr;
        rhs.end_ = nullptr;
        rhs.cap_ = nullptr;
    }

    vector(std::initializer_list<value_type> list)
    { range_init(list.begin(), list.end()); }

public:
    // 容量相关操作
    bool empty() const noexcept
    { return begin_ == end_; }

    size_type size() const noexcept
    { return static_cast<size_type>(end_ - begin_); }

    size_type max_size() const noexcept
    { return static_cast<size_type>(-1) / sizeof(T); }

    size_type capacity() const noexcept
    { return static_cast<size_type>(cap_ - begin_); }

    void reserve(size_type n);

    void shrink_to_fit();

    // 访问元素相关操作
    reference operator[](size_type n)
    {
        MYSTL_DEBUG(n < size());
        return *(begin_ + n);
    }

    const_reference operator[](size_type n) const
    {
        MYSTL_DEBUG(n < size());
        return *(begin_ + n);
    }

    reference at(size_type n)
    {
        THROW_OUT_OF_RANGE_IF(!(n < size()), "vector<T>::at() subscript out of range");
        return (*this)[n];
    }

    const_reference at(size_type n) const
    {
        THROW_OUT_OF_RANGE_IF(!(n < size()), "vector<T>::at() subscript out of range");
        return (*this)[n];
    }

private:
    // helper functions

    // initialize / destroy
    void try_init() noexcept;
    void fill_init(size_type n, const value_type& value);
    void init_space(size_type size, size_type cap);
    template <class Iter>
    void range_init(Iter first, Iter last);

    // shrink_to_fit
    void reinsert(size_type size);
};

/*****************************************************************************************/
// helper function

// try_init 函数，若分配失败则忽略，不抛出异常
template <class T>
void vector<T>::try_init() noexcept
{
    try
    {
        begin_ = data_allocator::allocate(16);
        end_ = begin_;
        cap_ = begin_ + 16;
    }
    catch (...)
    {
        begin_ = nullptr;
        end_ = nullptr;
        cap_ = nullptr;
    }
}

// fill_init 函数
template <class T>
void vector<T>::fill_init(size_type n, const value_type& value)
{
    const size_type init_size = mystl::max(static_cast<size_type>(16), n);
    init_space(n, init_size);
    mystl::uninitialized_fill_n(begin_, n, value);
}

// init_space 函数
template <class T>
void vector<T>::init_space(size_type size, size_type cap)
{
    try
    {
        begin_ = data_allocator::allocate(cap);
        end_ = begin_ + size;
        cap_ = begin_ + cap;
    }
    catch (...)
    {
        begin_ = nullptr;
        end_ = nullptr;
        cap_ = nullptr;
        throw;
    }
}

// range_init 函数
template <class T>
template <class Iter>
void vector<T>::range_init(Iter first, Iter last)
{
    const size_type init_size = mystl::max(static_cast<size_type>(last - first),
                                         static_cast<size_type>(16));
    init_space(static_cast<size_type>(last - first), init_size);
    mystl::uninitialized_copy(first, last, begin_);
}

// 预留空间大小，当原容量小于要求大小时，才会重新分配
template <class T>
void vector<T>::reserve(size_type n)
{
    if (capacity() < n)
    {
        THROW_LENGTH_ERROR_IF(n > max_size(),
                          "n can not larger than max_size() in vector<T>::reserve(n)");
        const auto old_size = size();
        auto tmp = data_allocator::allocate(n);
        mystl::uninitialized_move(begin_, end_, tmp);
        data_allocator::deallocate(begin_, cap_ - begin_);
        begin_ = tmp;
        end_ = tmp + old_size;
        cap_ = begin_ + n;
    }
}

// 放弃多余的容量
template <class T>
void vector<T>::shrink_to_fit()
{
    if (end_ < cap_)
    {
        reinsert(size());
    }
}

// reinsert 函数
template <class T>
void vector<T>::reinsert(size_type size)
{
    auto new_begin = data_allocator::allocate(size);
    try
    {
        mystl::uninitialized_move(begin_, end_, new_begin);
    }
    catch (...)
    {
        data_allocator::deallocate(new_begin, size);
        throw;
    }
    data_allocator::deallocate(begin_, cap_ - begin_);
    begin_ = new_begin;
    end_ = begin_ + size;
    cap_ = begin_ + size;
}

}

#endif /* VECTOR_H_ */
