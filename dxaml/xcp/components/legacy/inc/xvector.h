// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <iterator>

// DEPRECATED: Do NOT use xvector or friends in new code. Use std::vector.
template <typename TData>
class xvector;

template <typename TData>
class _xvector_base;

template <typename TData>
class _xvector_base_const_iterator
{
public:
    typedef std::random_access_iterator_tag iterator_category;
    typedef TData value_type;
    typedef XINT32 difference_type;
    typedef TData* pointer;
    typedef TData& reference;

    friend class xvector<TData>;
    friend class _xvector_base<TData>;

    _xvector_base_const_iterator()
        : m_Upto(static_cast<XUINT32>(-1))
        , m_Container(NULL)
    {
    }

    _xvector_base_const_iterator(_In_ XUINT32 uptoIndex, _In_ const xvector<TData>* container)
        : m_Upto(uptoIndex)
        , m_Container(container)
    {
        // This is deliberately <= An end iterator has uptoIndex == container->size()
        ASSERT(this->IsEndIterator() || (uptoIndex <= container->size()));
    }

    const TData& operator*() const
    {
        ASSERT(this->m_Container != NULL);
        ASSERT(this->m_Upto < this->m_Container->size());
        return *(this->m_Container->TypedPointerAt(m_Upto));
    }

    const TData* operator->() const
    {
        ASSERT(this->m_Container != NULL);
        ASSERT(this->m_Upto < this->m_Container->size());
        return &**this;
    }
protected:
    bool IsEndIterator() const
    {
        return (this->m_Upto == static_cast<XUINT32>(-1));
    }
    bool AreEqual(_In_ const _xvector_base_const_iterator<TData>& rhs) const
    {
        ASSERT(this->m_Container == rhs.m_Container);
        ASSERT(this->m_Container != NULL);
        return this->m_Upto == rhs.m_Upto;
    }

    bool AreNotEqual(_In_ const _xvector_base_const_iterator<TData>& rhs) const
    {
        return !this->AreEqual(rhs);
    }

    void AssignToThis(_In_ const _xvector_base_const_iterator<TData>& rhs)
    {
        if (this != &rhs)
        {
            this->m_Upto = rhs.m_Upto;
            this->m_Container = rhs.m_Container;
        }
    }

protected:
    XUINT32 m_Upto;
    const xvector<TData>* m_Container;
};

template <typename TData>
class _xvector_const_reverse_iterator : public _xvector_base_const_iterator<TData>
{
public:
    using difference_type = typename _xvector_base_const_iterator<TData>::difference_type;

    _xvector_const_reverse_iterator()
    {
    }

    _xvector_const_reverse_iterator(_In_ XUINT32 uptoIndex, _In_ const xvector<TData>* container)
        : _xvector_base_const_iterator<TData>(uptoIndex, container)
    {
    }

    _xvector_const_reverse_iterator<TData>& operator++()
    {
        // ++preincrement
        ASSERT(this->m_Container != NULL);
        ASSERT(!this->IsEndIterator());

        if (!this->IsEndIterator())
        {
            this->m_Upto--;
        }

        return *this;
    }

    _xvector_const_reverse_iterator<TData>& operator+=(_In_ XINT32 increment)
    {
        ASSERT(increment > -1);
        ASSERT(increment == 0 || !this->IsEndIterator());

        if (increment == 0 || !this->IsEndIterator())
        {
            ASSERT (this->m_Upto >= static_cast<XUINT32>(increment));
            if (this->m_Upto < static_cast<XUINT32>(increment))
            {
                this->m_Upto = static_cast<XUINT32>(-1);
            }
            else
            {
                this->m_Upto = this->m_Upto - increment;
            }
        }
        return *this;
    }

    // NOTE: To compile in gcc the parameter must be 'naked' int
    // it can't be a typedef.
    _xvector_const_reverse_iterator<TData> operator++(int)
    {
        // postincrement++
        ASSERT(!this->IsEndIterator());
        _xvector_const_reverse_iterator<TData> ret = *this;
        ++*this;
        return ret;
    }

    _xvector_const_reverse_iterator<TData>& operator--()
    {
        // --predecrement
        ASSERT(!this->IsEndIterator());
        ASSERT(this->m_Container != NULL);

        if (this->m_Upto < this->m_Container->size())
        {
            this->m_Upto++;
        }
        return *this;
    }

    _xvector_const_reverse_iterator<TData>& operator-=(_In_ XINT32 increment)
    {
        // TODO: This can wrap.
        ASSERT(!this->IsEndIterator());
        XINT32 newValue = this->m_Upto + increment;

        if (this->m_Upto < this->m_Container->size())
        {
            if (newValue > static_cast<XINT32>(this->m_Container->size()))
            {
                this->m_Upto = this->m_Container->size();
            }
            else
            {
                this->m_Upto = newValue;
            }
        }
        return *this;
    }

    _xvector_const_reverse_iterator<TData> operator--(int)
    {
        // postdecrement--
        ASSERT(!this->IsEndIterator());
        _xvector_const_reverse_iterator ret = *this;
        --*this;
        return ret;
    }


    bool operator==(_In_ const _xvector_const_reverse_iterator<TData>& rhs) const
    {
        return this->AreEqual(rhs);
    }

    bool operator!=(_In_ const _xvector_const_reverse_iterator<TData>& rhs) const
    {
        return this->AreNotEqual(rhs);
    }

    _xvector_const_reverse_iterator<TData>& operator=(_In_ const _xvector_const_reverse_iterator<TData>& rhs)
    {
        this->AssignToThis(rhs);
        return *this;
    }
};

template <typename TData>
class _xvector_reverse_iterator
    : public _xvector_const_reverse_iterator<TData>
{
public:
    using difference_type = typename _xvector_const_reverse_iterator<TData>::difference_type;

    _xvector_reverse_iterator()
    {
    }

    _xvector_reverse_iterator(_In_ XUINT32 uptoIndex, _In_ xvector<TData>* container)
        : _xvector_const_reverse_iterator< TData >(uptoIndex, container)
    {
    }

    TData& operator*() const
    {
        ASSERT(this->m_Container != NULL);
        ASSERT(this->m_Upto < this->m_Container->size());
        return *(this->m_Container->TypedPointerAt(this->m_Upto));
    }

    TData* operator->() const
    {
        ASSERT(this->m_Container != NULL);
        ASSERT(this->m_Upto < this->m_Container->size());
        return &**this;
    }

    _xvector_reverse_iterator<TData>& operator++()
    {
        // ++preincrement (the cast to base const class is intentional)
        ++(*static_cast<_xvector_const_reverse_iterator<TData>*>(this));
        return *this;
    }

    _xvector_reverse_iterator<TData>& operator+=(_In_ XINT32 increment)
    {
        (*static_cast<_xvector_const_reverse_iterator<TData>*>(this)) += increment;
        return *this;
    }

    _xvector_reverse_iterator<TData> operator++(int)
    {
        // postincrement++
        _xvector_reverse_iterator<TData> ret = *this;
        ++*this;
        return ret;
    }


    _xvector_reverse_iterator<TData>& operator--()
    {
        // --predecrement (the cast to base const class is intentional)
        --(*static_cast<_xvector_const_reverse_iterator<TData>*>(this));
        return *this;
    }

    _xvector_reverse_iterator<TData>& operator-=(_In_ XINT32 decrement)
    {
        (*static_cast<_xvector_const_reverse_iterator<TData>*>(this)) -= decrement;
        return *this;
    }

    _xvector_reverse_iterator<TData> operator--(int)
    {
        // postdecrement--
        _xvector_reverse_iterator<TData> ret = *this;
        --*this;
        return ret;
    }
};


template <typename TData>
class _xvector_const_iterator : public _xvector_base_const_iterator<TData>
{
public:
    using difference_type = typename _xvector_base_const_iterator<TData>::difference_type;

    _xvector_const_iterator()
    {
    }

    _xvector_const_iterator(_In_ XUINT32 uptoIndex, _In_ const xvector<TData>* container)
        : _xvector_base_const_iterator<TData>(uptoIndex, container)
    {
    }

    _xvector_const_iterator<TData>& operator++()
    {
        // ++preincrement
        ASSERT(!this->IsEndIterator());
        ASSERT(this->m_Container != NULL);

        if (this->m_Upto < this->m_Container->size())
        {
            this->m_Upto++;
        }
        return *this;
    }

    _xvector_const_iterator<TData>& operator--()
    {
        // --predecrement
        ASSERT(this->m_Container != NULL);

        if (this->m_Upto > 0)
        {
            this->m_Upto--;
        }
        return *this;
    }

    _xvector_const_iterator<TData>& operator+=(_In_ XINT32 increment)
    {
        // TODO: This can wrap.
        ASSERT(!this->IsEndIterator());
        XINT32 newValue = this->m_Upto + increment;

        if (this->m_Upto < this->m_Container->size())
        {
            if (newValue > static_cast<XINT32>(this->m_Container->size()))
            {
                this->m_Upto = this->m_Container->size();
            }
            else
            {
                this->m_Upto = newValue;
            }
        }
        return *this;
    }

   _xvector_const_iterator<TData>& operator-=(_In_ XINT32 decrement)
    {
        XINT32 newValue = this->m_Upto - decrement;

        if (this->m_Upto < this->m_Container->size())
        {
            if (newValue < 0)
            {
                this->m_Upto = 0;
            }
            else
            {
                this->m_Upto = newValue;
            }
        }
        return *this;
    }

    _xvector_const_iterator<TData> operator++(int)
    {
        // postincrement++
        ASSERT(!this->IsEndIterator());
        _xvector_const_iterator<TData> ret = *this;
        ++*this;
        return ret;
    }

    _xvector_const_iterator<TData> operator--(int)
    {
        // postdecrement--
        _xvector_const_iterator<TData> ret = *this;
        --*this;
        return ret;
    }

    bool operator==(_In_ const _xvector_const_iterator<TData>& rhs) const
    {
        return this->AreEqual(rhs);
    }

    bool operator!=(_In_ const _xvector_const_iterator<TData>& rhs) const
    {
        return this->AreNotEqual(rhs);
    }

    _xvector_const_iterator<TData>& operator=(_In_ const _xvector_const_iterator<TData>& rhs)
    {
        this->AssignToThis(rhs);
        return *this;
    }

    _xvector_const_iterator<TData> operator+(difference_type offset) const
    {
        _xvector_const_iterator<TData> temp = *this;
        return temp += offset;
    }

    _xvector_const_iterator<TData> operator-(difference_type offset) const
    {
        _xvector_const_iterator<TData> temp = *this;
        return temp -= offset;
    }

    difference_type operator-(const _xvector_const_iterator<TData>& right) const
    {
        return static_cast<difference_type>(m_Upto)-static_cast<difference_type>(right.m_Upto);
    }
};

template <typename TData>
class _xvector_iterator
    : public _xvector_const_iterator<TData>
{
public:
    using difference_type = typename _xvector_const_iterator<TData>::difference_type;

    _xvector_iterator()
    {
    }

    _xvector_iterator(_In_ XUINT32 uptoIndex, _In_ xvector<TData>* container)
        : _xvector_const_iterator< TData >(uptoIndex, container)
    {
    }

    TData& operator*() const
    {
        ASSERT(this->m_Container != NULL);
        ASSERT(this->m_Upto < this->m_Container->size());
        return *(this->m_Container->TypedPointerAt(this->m_Upto));
    }

    TData* operator->() const
    {
        ASSERT(this->m_Container != NULL);
        ASSERT(this->m_Upto < this->m_Container->size());
        return &**this;
    }

    _xvector_iterator<TData>& operator++()
    {
        // ++preincrement (the cast to base const class is intentional)
        ++(*static_cast<_xvector_const_iterator<TData>*>(this));
        return *this;
    }

    _xvector_iterator<TData>& operator--()
    {
        // --predecrement (the cast to base const class is intentional)
        --(*static_cast<_xvector_const_iterator<TData>*>(this));
        return *this;
    }

    _xvector_iterator<TData>& operator+=(_In_ XINT32 increment)
    {
        (*static_cast<_xvector_const_iterator<TData>*>(this)) += increment;
        return *this;
    }

    _xvector_iterator<TData>& operator-=(_In_ XINT32 increment)
    {
        (*static_cast<_xvector_const_iterator<TData>*>(this)) -= increment;
        return *this;
    }

    _xvector_iterator<TData> operator++(int)
    {
        // postincrement++
        _xvector_iterator<TData> ret = *this;
        ++*this;
        return ret;
    }

    _xvector_iterator<TData> operator--(int)
    {
        // postdecrement--
        _xvector_iterator<TData> ret = *this;
        --*this;
        return ret;
    }

    _xvector_iterator<TData> operator+(XINT32 offset) const
    {
        _xvector_iterator<TData> temp = *this;
        return temp += offset;
    }

    _xvector_iterator<TData> operator-(XINT32 offset) const
    {
        _xvector_iterator<TData> temp = *this;
        return temp -= offset;
    }

    difference_type operator-(const _xvector_iterator<TData>& right) const
    {
        return static_cast<difference_type>(m_Upto)-static_cast<difference_type>(right.m_Upto);
    }
};



// --------------------------------------------------------

template<typename T>
void _xvector_base_construct_at(
    void* location,
    _In_ const T& val
    )
{
    new (location) T(val);
}

template<typename T>
void _xvector_base_get_item_at(
    T* location,
    T& val
    )
{
    val = *location;
}

template <typename T>
class _xvector_base
{
private:
    // Poison constructors
    _xvector_base& operator=(_In_ const _xvector_base& other);
    _xvector_base(_In_ const _xvector_base& other);
public:
    static const XUINT32 DefaultStartSize = 8;

protected:
    XUINT8* m_data;
    XUINT32 m_count;
    XUINT32 m_actualSize;
    XUINT32 m_StartOffset;

#if XCP_MONITOR
    bool m_isContainingWeakPointers;
#endif

public:
    typedef _xvector_iterator<T> iterator;
    typedef _xvector_const_iterator<T> const_iterator;
    typedef _xvector_reverse_iterator<T> reverse_iterator;
    typedef _xvector_const_reverse_iterator<T> const_reverse_iterator;
    typedef T value_type;

public:
    _xvector_base()
    {
#if XCP_MONITOR
        m_isContainingWeakPointers = false;
#endif
        ResetMembers();
    }

    _xvector_base(_In_ XUINT32 startingSize)
    {
#if XCP_MONITOR
        m_isContainingWeakPointers = false;
#endif
        ResetMembers();

        VERIFYHR(reserve(startingSize));
    }

    ~_xvector_base(void)
    {
        clear();
    }

    XUINT32 size() const        { return this->m_count;           }
    bool empty() const          { return this->size() == 0;       }
    XUINT32 capacity() const    { return this->m_actualSize;      }
    HRESULT front(T& val) const { return this->get_item(0, val);  }

    HRESULT back(T& val) const
    {
        return (this->m_count == 0)
                    ? E_FAIL
                    : this->get_item(this->m_count - 1, val);
    }

    HRESULT get_item(_In_ XUINT32 index, T& val) const
    {
        IFC_RETURN(this->CheckRange(index));
        _xvector_base_get_item_at(this->TypedPointerAt(index), val);
        return S_OK;
    }

    HRESULT set_item(_In_ XUINT32 index, _In_ const T& val)
    {
        IFC_RETURN(this->CheckRange(index));
        this->DestroyAt(index);
        this->ConstructAt(index, val);

        return S_OK;
    }

    HRESULT reserve(_In_ XUINT32 requestedSize)
    {
        IFC_RETURN(this->ExpandStorageIfNeeded(requestedSize));
        return S_OK;
    }

    HRESULT fill_back(_In_ const T& val, _In_ XUINT32 count)
    {
        HRESULT hr = S_OK;
        XUINT32 oldCount = m_count;
        IFC(this->ExpandStorageIfNeeded(m_count + count));
        this->m_count += count;
        IFC(this->ConstructRange(oldCount, count, val));

Cleanup:
        if (FAILED(hr))
        {
            this->m_count = oldCount;
        }
        RRETURN(hr);
    }

    HRESULT emplace_back(_Outptr_ T **ppVal)
    {
        const XUINT32 index = m_count;
        IFC_RETURN(this->ExpandStorageIfNeeded());
        ++m_count;

        ASSERTSUCCEEDED(this->CheckRange(index));
        *ppVal = this->TypedPointerAt(index);

        return S_OK;
    }

    HRESULT push_back(_In_ const T& val)
    {
        IFC_RETURN(this->ExpandStorageIfNeeded());
        this->ConstructAt(m_count++, val);
        return S_OK;
    }

    HRESULT pop_back()
    {
        return (this->m_count == 0)
                    ? E_FAIL
                    : this->erase(m_count - 1);
    }

    HRESULT resize(_In_ XUINT32 newSize)
    {
        IFC_RETURN(this->ResizeBuffer(newSize));
        this->m_count = newSize;
        return S_OK;
    }

    HRESULT insert(_In_ XUINT32 index, _In_ const T& val)
    {
        IFCEXPECT_RETURN(index <= this->m_count);
        IFC_RETURN(this->ExpandStorageIfNeeded());
        IFC_RETURN(this->MoveRange(index + 1, index, this->m_count - index));
        this->ConstructAt(index, val);
        this->m_count++;
        return S_OK;
    }

    HRESULT erase(_In_ XUINT32 index)
    {
        return this->erase(index, 1);
    }

    HRESULT erase(_In_ XUINT32 index, _In_ XUINT32 numberOfItems)
    {
        IFC_RETURN(this->CheckRange(index, numberOfItems));
        IFC_RETURN(this->MoveRange(index, index + numberOfItems, this->m_count - index - numberOfItems));
        this->m_count -= numberOfItems;

#if XCP_MONITOR
        // For debug purposes, make our life easier and whenever this collection is emptied, we delete it.
        // This is better than for every xvector out there, calling clear when it is empty so that we don't
        // have outstanding allocations from resizing the vector buffer.
        if (this->m_count == 0u)
        {
            this->clear();
        }
#endif

        return S_OK;
    }

    HRESULT insert(_In_ _xvector_const_iterator<T> it, _In_ const T& val)
    {
        if(it.IsEndIterator())
        {
            return this->push_back(val);
        }
        else
        {
            return this->insert(it.m_Upto, val);
        }
    }

    HRESULT erase(_In_  _xvector_base_const_iterator<T> it)
    {
        if (it.IsEndIterator())
        {
            return E_FAIL;
        }
        else
        {
            return this->erase(it.m_Upto, 1);
        }
    }

    HRESULT erase(_In_ _xvector_base_const_iterator<T> it, _In_ XUINT32 numberOfItems)
    {
        if (it.IsEndIterator())
        {
            return E_FAIL;
        }
        else
        {
            XUINT32 index = it.m_Upto;
            IFC_RETURN(this->erase(index, numberOfItems));
        }
        return S_OK;
    }

    void clear()
    {
        IGNOREHR(this->DestroyRange(0, m_count));
        delete [] m_data;
        ResetMembers();
    }

    void XcpMarkWeakPointers()
    {
#if XCP_MONITOR
        // Don't call this when non-empty.
        ASSERT(m_data == NULL);
        ASSERT(m_count == 0);
        m_isContainingWeakPointers = true;
#endif
    }

protected:
    HRESULT CheckRange(_In_ XUINT32 index) const
    {
        return (index < this->m_count)
                ? S_OK
                : E_FAIL;
    }

    XUINT32 SizeOfNItems(_In_ XUINT32 numberOfItems) const
    {
        return numberOfItems * sizeof(T);
    }

    void ConstructAt(_In_ XUINT32 index, _In_ const T& val)
    {
        _xvector_base_construct_at(
            static_cast<void*>(this->RawPointerAt(index)),
            val);
    }

    void DestroyAt(_In_ XUINT32 index)
    {
        this->TypedPointerAt(index)->~T();
    }

    T* TypedPointerAt(_In_ XUINT32 index) const
    {
        return static_cast<T*>(static_cast<void*>(this->RawPointerAt(index)));
    }

    XUINT8* CreateRawBuffer(_In_ XUINT32 numberOfItems) const
    {
        XUINT8* rawBuffer = new XUINT8[this->SizeOfNItems(numberOfItems)];
        return rawBuffer;
    }

    XUINT8* RawPointerAt(_In_ XUINT32 index) const
    {
        ASSERT(index < capacity());
        ASSERT(this->m_data != NULL);

        return this->m_data + this->SizeOfNItems(this->WrappedOffset(index));
    }

    void ResetMembers()
    {
        this->m_data = NULL;
        this->m_count = 0;
        this->m_actualSize = 0;
        this->m_StartOffset = 0;
    }

    XUINT32 ExpandedBufferSize()
    {
        XUINT32 oldSize = capacity();

        if (oldSize < 8)
        {
            return 8;
        }
        else
        {
            return oldSize * 2;
        }
    }

    HRESULT ExpandStorageIfNeeded()
    {
        if (this->m_data == NULL)
        {
            IFC_RETURN(this->ExpandStorageIfNeeded(DefaultStartSize));
        }
        else if (this->m_count == this->capacity())
        {
            IFC_RETURN(this->ExpandStorageIfNeeded(this->ExpandedBufferSize()));
        }

        return S_OK;
    }

    HRESULT ExpandStorageIfNeeded(_In_ XUINT32 newBufferSize)
    {
        if (this->m_data == NULL)
        {
            ASSERT(this->m_count == 0);

            this->m_data = this->CreateRawBuffer(newBufferSize);
            this->m_actualSize = newBufferSize;
        }
        else if (newBufferSize > this->m_actualSize)
        {
            IFC_RETURN(this->ResizeBuffer(newBufferSize));
        }

        return S_OK;
    }

    HRESULT ResizeBuffer(_In_ XUINT32 newBufferSize)
    {
        XUINT8* newBuffer = NULL;

        if (newBufferSize < this->m_count)
        {
            IFC_RETURN(DestroyRange(newBufferSize, this->m_count - newBufferSize));
        }
        else if(newBufferSize > this->m_count)
        {
            newBuffer = this->CreateRawBuffer(newBufferSize);

            if ((this->m_StartOffset == 0) || (this->m_StartOffset + this->m_count <= this->m_actualSize))
            {
                memcpy(static_cast<void*>(newBuffer), static_cast<void*>(this->m_data + this->SizeOfNItems(this->m_StartOffset)), this->SizeOfNItems(this->m_count));
            }
            else
            {
                memcpy(static_cast<void*>(newBuffer), static_cast<void*>(this->m_data + this->SizeOfNItems(this->m_StartOffset)), this->SizeOfNItems(this->m_actualSize - this->m_StartOffset));
                memcpy(static_cast<void*>(newBuffer + this->SizeOfNItems(this->m_actualSize - this->m_StartOffset)), static_cast<void*>(this->m_data), this->SizeOfNItems(this->m_count - this->m_actualSize + this->m_StartOffset));
            }

            // TODO: Possibly some optimization to place this
            this->m_StartOffset = 0;

            delete[] this->m_data;
            this->m_data = newBuffer;
            this->m_actualSize  = newBufferSize;
        }
        return S_OK;
    }

    HRESULT CheckRange(_In_ XUINT32 startIndex, _In_ XUINT32 count) const
    {
        if (((count == 0) && (startIndex == 0)) || ((startIndex < this->m_count) && (count + startIndex <= this->m_count)))
        {
            return S_OK;
        }

        return E_FAIL;
    }

    HRESULT ConstructRange(_In_ XUINT32 startIndex, _In_ XUINT32 count, _In_ const T& val)
    {
        IFC_RETURN(this->CheckRange(startIndex, count));
        for (XUINT32 i = startIndex; i < startIndex + count; i++)
        {
            this->ConstructAt(i, val);
        }
        return S_OK;
    }

    HRESULT DestroyRange(_In_ XUINT32 startIndex, _In_ XUINT32 count)
    {
        IFC_RETURN(this->CheckRange(startIndex, count));
        for (XUINT32 i = startIndex; i < startIndex + count; i++)
        {
            this->DestroyAt(i);
        }
        return S_OK;
    }

    XUINT32 WrappedOffset(_In_ XUINT32 uIndex) const
    {
        if (this->m_StartOffset == 0)
        {
            return uIndex;
        }

        ASSERT(this->m_actualSize > this->m_StartOffset);
        ASSERT(this->m_actualSize > uIndex);

        // Perf optimization - avoid using '%' operator because it isn't supported in hardware on ARM.
        XUINT32 offsetPlusIndex = this->m_StartOffset + uIndex;
        return offsetPlusIndex < this->m_actualSize ? offsetPlusIndex : offsetPlusIndex - this->m_actualSize;
    }

    HRESULT MoveRange(_In_ XUINT32 destIndex, _In_ XUINT32 sourceIndex, _In_ XUINT32 count)
    {
        if (sourceIndex > destIndex)
        {
            IFC_RETURN(this->DestroyRange(destIndex, sourceIndex - destIndex));

            if (count != 0)
            {
                // Moving left, and will destroy the gap.
                if (this->m_StartOffset == 0)
                {
                     memcpy(this->RawPointerAt(destIndex), this->RawPointerAt(sourceIndex), this->SizeOfNItems(count));
                }
                else
                {
                    // TODO: Handle double ended vector here.
                    return E_UNEXPECTED;
                }
            }
        }
        else if ((sourceIndex < destIndex) && (count != 0))
        {
            // Moving right
            memmove(RawPointerAt(destIndex), RawPointerAt(sourceIndex), SizeOfNItems(count));
        }
        return S_OK;
    }

    bool IsContiguous(_In_ XUINT32 index, _In_ XUINT32 count)
    {
        // TODO: Implement properly
        return (this->m_StartOffset == 0);
    }
};


// --------------------------------------------------------
template <typename T>
class xvector : public _xvector_base<T>
{
    // TODO: Insert
    // TODO: Max size
    // TODO: Remove fill_back
    // TODO: Allow shrink
private:
    // Poison constructors
    xvector& operator=(const xvector& other);
    xvector(const xvector& other);

public:

    typedef _xvector_iterator<T> iterator;
    typedef _xvector_const_iterator<T> const_iterator;
    typedef _xvector_reverse_iterator<T> reverse_iterator;
    typedef _xvector_const_reverse_iterator<T> const_reverse_iterator;

    friend class _xvector_base_const_iterator<T>;
    friend class _xvector_iterator<T>;
    friend class _xvector_const_iterator<T>;
    friend class _xvector_reverse_iterator<T>;
    friend class _xvector_const_reverse_iterator<T>;

    xvector(void)
    {
    }

    xvector(_In_ XUINT32 startingSize)
        : _xvector_base<T>(startingSize)
    {
    }

    ~xvector(void)
    {
    }

    void swap(xvector<T> & other)
    {
        XUINT8 * pTmp = this->m_data;
        this->m_data = other.m_data;
        other.m_data = pTmp;

        XUINT32 tmp = this->m_count;
        this->m_count = other.m_count;
        other.m_count = tmp;

        tmp = this->m_actualSize;
        this->m_actualSize = other.m_actualSize;
        other.m_actualSize = tmp;

        tmp = this->m_StartOffset;
        this->m_StartOffset = other.m_StartOffset;
        other.m_StartOffset = tmp;

#if XCP_MONITOR
        bool btmp = this->m_isContainingWeakPointers;
        this->m_isContainingWeakPointers = other.m_isContainingWeakPointers;
        other.m_isContainingWeakPointers = btmp;
#endif

    }

    T* data()                                           { return this->m_count ? reinterpret_cast<T *>(this->TypedPointerAt(0)) : nullptr; }
    const T* data() const                               { return this->m_count ? this->TypedPointerAt(0) : nullptr; }
    _xvector_iterator<T> begin()                        { return _xvector_iterator<T>(0, this);                     }
    _xvector_iterator<T> end()                          { return _xvector_iterator<T>(this->m_count, this);               }
    _xvector_const_iterator<T> begin() const            { return _xvector_const_iterator<T>(0, this);               }
    _xvector_const_iterator<T> end() const              { return _xvector_const_iterator<T>(this->m_count, this);         }
    _xvector_reverse_iterator<T> rbegin()               { return (this->m_count == 0) ? this->rend() : _xvector_reverse_iterator<T>(this->m_count - 1, this);         }
    _xvector_reverse_iterator<T> rend()                 { return _xvector_reverse_iterator<T>(static_cast<XUINT32>(-1), this);                      }
    _xvector_const_reverse_iterator<T> rbegin() const   { return (this->m_count == 0) ? this->rend() : _xvector_const_reverse_iterator<T>(this->m_count - 1, this);  }
    _xvector_const_reverse_iterator<T> rend() const     { return _xvector_const_reverse_iterator<T>(static_cast<XUINT32>(-1), this);               }

    T& unsafe_get_item(XUINT32 index)
    {
        ASSERTSUCCEEDED(this->CheckRange(index));
        return *(this->TypedPointerAt(index));
    }

    using _xvector_base<T>::back;

    T& back()
    {
        return unsafe_get_item(this->size() - 1);
    }

    const T& unsafe_get_item(XUINT32 index) const
    {
        ASSERTSUCCEEDED(this->CheckRange(index));
        return *(this->TypedPointerAt(index));
    }

    const T& operator[](XUINT32 index) const
    {
        return unsafe_get_item(index);
    }

    T& operator[](XUINT32 index)
    {
        return unsafe_get_item(index);
    }
};

// --------------------------------------------------------------------

template <typename T>
class xdevector : public xvector<T>
{
    // TODO: Insert
    // TODO: Max size
    // TODO: Remove fill_back
    // TODO: Allow shrink
private:
    // Poison constructors
    xdevector& operator=(_In_ const xdevector& other);
    xdevector(_In_ const xdevector& other);

public:
    friend class _xvector_iterator<T>;
    friend class _xvector_const_iterator<T>;
    friend class _xvector_reverse_iterator<T>;
    friend class _xvector_const_reverse_iterator<T>;

    xdevector(void)
    {
    }

    xdevector(_In_ XUINT32 startingSize)
        : _xvector_base<T>(startingSize)
    {
    }

    ~xdevector(void)
    {
    }

    HRESULT push_front(_In_ const T& val)
    {
        IFC_RETURN(this->ExpandStorageIfNeeded());
        this->m_StartOffset = this->WrappedOffset(static_cast<XUINT32>(-1));
        this->ConstructAt(0, val);
        this->m_count++;
        return S_OK;
    }

    HRESULT pop_front()
    {
        if (this->m_count == 0)
        {
            return E_FAIL;
        }

        this->DestroyAt(0);
        this->m_StartOffset = this->WrappedOffset(1);
        this->m_count--;
        return S_OK;
    }

    T* data()                                           { return this->m_count ? reinterpret_cast<T *>(this->TypedPointerAt(0)) : nullptr; }
    const T* data() const                               { return this->m_count ? this->TypedPointerAt(0) : nullptr; }
    _xvector_iterator<T> begin()                        { return _xvector_iterator<T>(0, this);                     }
    _xvector_iterator<T> end()                          { return _xvector_iterator<T>(this->m_count, this);               }
    _xvector_const_iterator<T> begin() const            { return _xvector_const_iterator<T>(0, this);               }
    _xvector_const_iterator<T> end() const              { return _xvector_const_iterator<T>(this->m_count, this);         }
    _xvector_reverse_iterator<T> rbegin()               { return (this->m_count == 0) ? this->rend() : _xvector_reverse_iterator<T>(this->m_count - 1, this);         }
    _xvector_reverse_iterator<T> rend()                 { return _xvector_reverse_iterator<T>(static_cast<XUINT32>(-1), this);                      }
    _xvector_const_reverse_iterator<T> rbegin() const   { return (this->m_count == 0) ? this->rend() : _xvector_const_reverse_iterator<T>(this->m_count - 1, this);   }
    _xvector_const_reverse_iterator<T> rend() const     { return _xvector_const_reverse_iterator<T>(static_cast<XUINT32>(-1), this);                }
};
