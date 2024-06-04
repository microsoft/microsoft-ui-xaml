// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Indexes.g.h"
#include <type_traits>

namespace Diagnostics {

    // Iterator for easily accessing enums.
    template <typename T>
    class EnumIterator {
        T value_ = T();
        typedef typename std::underlying_type<T>::type impl_type;
        static_assert(std::is_enum<T>::value, "EnumIterator requires an enum type");
    public:
        typedef std::size_t size_type;

        static T Begin();
        static T End();

        explicit EnumIterator(T value)
            : value_(value)
        {
        }

        EnumIterator() = default;
        EnumIterator(const EnumIterator&) = default;
        EnumIterator& operator=(const EnumIterator&) = default;
        EnumIterator& operator=(T value) { value_ = value; }

        EnumIterator& operator+=(size_type arg) { value_ = static_cast<T>(static_cast<impl_type>(value_) + arg); ASSERT(value_ <= End());  return *this; }
        EnumIterator& operator++() { ASSERT(value_ < End()); return (*this) += 1; }
        EnumIterator operator++(int) { EnumIterator result(*this); ASSERT(value_ < End()); ++*this; return result; }

        EnumIterator& operator-=(std::size_t arg) { value_ = static_cast<T>(static_cast<impl_type>(value_) - arg); ASSERT(value_ >= Begin()); return *this; }
        EnumIterator& operator--() { ASSERT(value_ > Begin()); return (*this) -= 1; }
        EnumIterator operator--(int) { EnumIterator result(*this); --*this; ASSERT(value_ > Begin()); return result; }

        T operator*() const { return value_; }
        T operator[](size_type arg) const { ASSERT(arg < static_cast<impl_type>(End())); return (EnumIterator(*this) + arg).value_; }
        const T* operator->() const { return &value_; }

        bool operator==(const EnumIterator& rhs) const { return value_ == rhs.value_; }
        bool operator!=(const EnumIterator& rhs) const { return value_ != rhs.value_; }
        bool operator<(const EnumIterator& rhs) const { return value_ < rhs.value_; }
        bool operator>(const EnumIterator& rhs) const { return value_ > rhs.value_; }
        bool operator<=(const EnumIterator& rhs) const { return value_ <= rhs.value_; }
        bool operator>=(const EnumIterator& rhs) const { return value_ >= rhs.value_; }
    };

    template<typename T>
    EnumIterator<T> begin(EnumIterator<T>)
    {
        return EnumIterator<T>(EnumIterator<T>::Begin());
    }

    template<typename T>
    EnumIterator<T> end(EnumIterator<T>)
    {
        return EnumIterator<T>(EnumIterator<T>::End());
    }
}