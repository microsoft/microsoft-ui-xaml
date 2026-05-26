// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <new>
#include <type_traits>
#include <utility>

// CompactInlineVector: A vector with inline storage that uses only 16 bytes of overhead.
//   - Designed to be an easy drop-in replacement for std::vector when you want some SBO / internal storage
//   - Designed to be called from one thread
//   - No exception support (will call XAML_FAIL_FAST on errors instead of throwing)
//
// Memory layout (16 bytes total overhead):
//   - m_data (8 bytes): Pointer to data (inline or heap)
//   - m_size (4 bytes): Number of elements
//   - m_capacity (4 bytes): Allocated capacity
//
// Template parameters:
//   - T: Element type
//   - InlineCapacity: Number of elements to store inline (default: calculated to fit in cache line)

template <typename T, std::size_t InlineCapacity = (64 - 16) / sizeof(T)>
class CompactInlineVector
{
    static_assert(alignof(T) <= alignof(std::max_align_t), "T requires special alignment not supported");
    static_assert(InlineCapacity > 0, "InlineCapacity must be at least 1");

public:
    // Standard type aliases
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // Constructors
    CompactInlineVector() noexcept
    {
        SetInlineData();
    }

    // Copy constructor is deleted to prevent accidental copies.
    // Use DeepCopyTo() for explicit copying.
    CompactInlineVector(const CompactInlineVector& other) = delete;

    // Move constructor
    // (noexcept only when T's move constructor is noexcept)
    CompactInlineVector(CompactInlineVector&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        SetInlineData();
        // Just forward to move assignment operator for simplicity
        *this = std::move(other);
    }

    ~CompactInlineVector()
    {
        clear();
        if (IsUsingHeap())
        {
            FreeHeap(GetRawPointer());
        }
    }

    // Copy assignment is deleted to prevent accidental copies.
    // Use DeepCopyTo() for explicit copying.
    CompactInlineVector& operator=(const CompactInlineVector& other) = delete;

    // Move assignment
    CompactInlineVector& operator=(CompactInlineVector&& other) noexcept(
        std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>)
    {
        if (this != &other)
        {
            clear();
            if (IsUsingHeap())
            {
                FreeHeap(GetRawPointer());
            }

            if (other.IsUsingHeap())
            {
                m_data = other.m_data;
                m_size = other.m_size;
                m_capacity = other.m_capacity;
                other.SetInlineData();
                other.SetSize(0);
                other.SetCapacity(InlineCapacity);
            }
            else
            {
                SetInlineData();
                SetCapacity(InlineCapacity);
                for (size_type i = 0; i < other.size(); ++i)
                {
                    new (data() + i) T(std::move(other[i]));
                }
                SetSize(other.size());
                other.clear();
            }
        }
        return *this;
    }

    // Element access
    reference operator[](size_type pos) noexcept { ASSERT(pos < size()); return data()[pos]; }
    const_reference operator[](size_type pos) const noexcept { ASSERT(pos < size()); return data()[pos]; }

    reference front() noexcept { ASSERT(size() > 0); return data()[0]; }
    const_reference front() const noexcept { ASSERT(size() > 0); return data()[0]; }

    reference back() noexcept { ASSERT(size() > 0); return data()[size() - 1]; }
    const_reference back() const noexcept { ASSERT(size() > 0); return data()[size() - 1]; }

    pointer data() noexcept { return GetRawPointer(); }
    const_pointer data() const noexcept { return GetRawPointer(); }

    // Iterators
    iterator begin() noexcept { return data(); }
    const_iterator begin() const noexcept { return data(); }

    iterator end() noexcept { return data() + size(); }
    const_iterator end() const noexcept { return data() + size(); }

    reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }

    reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

    [[nodiscard]] bool empty() const noexcept { return size() == 0; }

    size_type size() const noexcept
    {
        return m_size;
    }

    size_type max_size() const noexcept
    {
        return static_cast<size_type>(UINT32_MAX);
    }

    void reserve(size_type newCapacity)
    {
        if (newCapacity > capacity())
        {
            GrowTo(newCapacity);
        }
    }

    size_type capacity() const noexcept
    {
        return m_capacity;
    }

    void shrink_to_fit()
    {
        if (size() <= InlineCapacity && IsUsingHeap())
        {
            // Move back to inline storage
            T* heapData = GetRawPointer();
            size_type currentSize = size();

            SetInlineData();
            for (size_type i = 0; i < currentSize; ++i)
            {
                new (data() + i) T(std::move(heapData[i]));
                heapData[i].~T();
            }
            FreeHeap(heapData);
            SetCapacity(InlineCapacity);
            SetSize(currentSize);
        }
        else if (size() < capacity() && IsUsingHeap())
        {
            // Shrink heap allocation
            T* oldData = GetRawPointer();
            size_type currentSize = size();
            T* newData = AllocateHeap(currentSize);

            for (size_type i = 0; i < currentSize; ++i)
            {
                new (newData + i) T(std::move(oldData[i]));
                oldData[i].~T();
            }
            FreeHeap(oldData);
            SetHeapData(newData);
            SetCapacity(currentSize);
        }
    }

    // Modifiers
    void clear() noexcept
    {
        for (size_type i = 0; i < size(); ++i)
        {
            data()[i].~T();
        }
        SetSize(0);
    }

    iterator insert(const_iterator pos, const T& value)
    {
        return insert(pos, 1, value);
    }

    iterator insert(const_iterator pos, T&& value)
    {
        size_type index = pos - begin();
        EnsureCapacity(size() + 1);
        ShiftElementsRight(index, 1);

        if (index < size())
        {
            data()[index] = std::move(value);
        }
        else
        {
            new (data() + index) T(std::move(value));
        }

        SetSize(size() + 1);
        return begin() + index;
    }

    iterator insert(const_iterator pos, size_type count, const T& value)
    {
        if (count == 0) return const_cast<iterator>(pos);

        size_type index = pos - begin();
        EnsureCapacity(size() + count);
        ShiftElementsRight(index, count);

        // Insert new elements
        for (size_type i = 0; i < count; ++i)
        {
            size_type insertPos = index + i;
            if (insertPos < size())
            {
                data()[insertPos] = value;
            }
            else
            {
                new (data() + insertPos) T(value);
            }
        }

        SetSize(size() + count);
        return begin() + index;
    }

    template <typename... Args>
    iterator emplace(const_iterator pos, Args&&... args)
    {
        size_type index = pos - begin();
        EnsureCapacity(size() + 1);
        ShiftElementsRight(index, 1);

        if (index < size())
        {
            data()[index].~T();
        }
        new (data() + index) T(std::forward<Args>(args)...);

        SetSize(size() + 1);
        return begin() + index;
    }

    iterator erase(const_iterator pos)
    {
        return erase(pos, pos + 1);
    }

    iterator erase(const_iterator first, const_iterator last)
    {
        size_type startIndex = first - begin();
        size_type endIndex = last - begin();
        size_type count = endIndex - startIndex;

        if (count == 0) return const_cast<iterator>(first);

        // Destroy erased elements
        for (size_type i = startIndex; i < endIndex; ++i)
        {
            data()[i].~T();
        }

        // Move remaining elements
        size_type remaining = size() - endIndex;
        for (size_type i = 0; i < remaining; ++i)
        {
            new (data() + startIndex + i) T(std::move(data()[endIndex + i]));
            data()[endIndex + i].~T();
        }

        SetSize(size() - count);
        return begin() + startIndex;
    }

    void push_back(const T& value)
    {
        EnsureCapacity(size() + 1);
        new (data() + size()) T(value);
        SetSize(size() + 1);
    }

    void push_back(T&& value)
    {
        EnsureCapacity(size() + 1);
        new (data() + size()) T(std::move(value));
        SetSize(size() + 1);
    }

    template <typename... Args>
    reference emplace_back(Args&&... args)
    {
        EnsureCapacity(size() + 1);
        new (data() + size()) T(std::forward<Args>(args)...);
        SetSize(size() + 1);
        return back();
    }

    void pop_back()
    {
        ASSERT(size() > 0);
        data()[size() - 1].~T();
        SetSize(size() - 1);
    }

    void resize(size_type count)
    {
        if (count < size())
        {
            for (size_type i = count; i < size(); ++i)
            {
                data()[i].~T();
            }
        }
        else if (count > size())
        {
            reserve(count);
            for (size_type i = size(); i < count; ++i)
            {
                new (data() + i) T();
            }
        }
        SetSize(count);
    }

    // Query inline status
    bool is_inline() const noexcept { return !IsUsingHeap(); }
    static constexpr size_type inline_capacity() noexcept { return InlineCapacity; }

    // Explicit deep copy - use this when you intentionally need a copy.
    void DeepCopyTo(CompactInlineVector& dest) const
    {
        dest.clear();
        dest.reserve(size());
        for (size_type i = 0; i < size(); ++i)
        {
            new (dest.data() + i) T(data()[i]);
        }
        dest.SetSize(size());
    }

private:
    // m_data: pointer to either inline storage or heap-allocated storage
    // We detect heap allocation by checking capacity() > InlineCapacity
    T* m_data;
    std::uint32_t m_size = 0;
    std::uint32_t m_capacity = static_cast<std::uint32_t>(InlineCapacity);

    // Inline storage - comes after the 16-byte header
    alignas(T) unsigned char m_inlineStorage[sizeof(T) * InlineCapacity];

    bool IsUsingHeap() const noexcept
    {
        return capacity() > InlineCapacity;
    }

    T* GetRawPointer() const noexcept
    {
        return m_data;
    }

    void SetInlineData() noexcept
    {
        m_data = reinterpret_cast<T*>(m_inlineStorage);
    }

    void SetHeapData(T* ptr) noexcept
    {
        m_data = ptr;
    }

    void SetSize(size_type newSize) noexcept
    {
        m_size = static_cast<uint32_t>(newSize);
    }

    void SetCapacity(size_type newCapacity) noexcept
    {
        m_capacity = static_cast<uint32_t>(newCapacity);
    }

    static T* AllocateHeap(size_type count)
    {
        void* ptr = ::operator new(sizeof(T) * count);
        return static_cast<T*>(ptr);
    }

    static void FreeHeap(T* ptr)
    {
        ::operator delete(ptr);
    }

    // Shift elements at [index, size()) right by count positions.
    // Handles the boundary between constructed and unconstructed memory.
    void ShiftElementsRight(size_type index, size_type count)
    {
        if (count == 0 || index >= size()) return;

        for (size_type i = size() + count - 1; i >= index + count; --i)
        {
            if (i >= size())
            {
                new (data() + i) T(std::move(data()[i - count]));
            }
            else
            {
                data()[i] = std::move(data()[i - count]);
            }
        }
    }

    void EnsureCapacity(size_type required)
    {
        if (required > capacity())
        {
            size_type newCapacity = (std::max)(required, capacity() * 2);
            GrowTo(newCapacity);
        }
    }

    void GrowTo(size_type newCapacity)
    {
        T* newData = AllocateHeap(newCapacity);
        T* oldData = GetRawPointer();
        size_type currentSize = size();

        for (size_type i = 0; i < currentSize; ++i)
        {
            new (newData + i) T(std::move(oldData[i]));
            oldData[i].~T();
        }

        if (IsUsingHeap())
        {
            FreeHeap(oldData);
        }

        SetHeapData(newData);
        SetCapacity(newCapacity);
    }
};

