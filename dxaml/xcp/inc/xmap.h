// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "xlist.h"
#include "xvector.h"
#include <utility>
#include "DataStructureFunctionProvider.h"

// Forward declaration
template <typename TKey, typename TData>
class xchainedmap;

//Helper function declaration
template<typename TKey, typename TData>
void FindNextSlotAndIterator(
            const xchainedmap<TKey, TData>* pMap,
            XUINT32& slotIndex,
            _xlist_iterator< std::pair<TKey, TData> >& slotIterator);

template <typename TKey, typename TData>
class _xchainedmap_const_iterator
{
public:
    _xchainedmap_const_iterator()
        : m_SlotIndex(static_cast<XUINT32>(-1))
        , m_Container(NULL)
    {
        XCP_WEAK(&m_Container);
    }

    _xchainedmap_const_iterator(
                XUINT32 slotIndex,
                _xlist_iterator< std::pair<TKey, TData> > slotIterator,
                xchainedmap<TKey, TData>* container)
        : m_SlotIndex(slotIndex)
        , m_SlotIterator(slotIterator)
        , m_Container(container)
    {
        XCP_WEAK(&m_Container);
    }

    const std::pair<TKey, TData>& operator*() const
    {
        ASSERT(this->m_Container != NULL);
        return *this->m_SlotIterator;
    }

    const std::pair<TKey, TData>* operator->() const
    {
        ASSERT(this->m_Container != NULL);
        return &**this;
    }

    _xchainedmap_const_iterator<TKey, TData>& operator++()
    {
        // ++preincrement
        ASSERT(this->m_Container != NULL);

        FindNextSlotAndIterator<TKey, TData>(this->m_Container, this->m_SlotIndex, this->m_SlotIterator);

        return *this;
    }

    _xchainedmap_const_iterator<TKey, TData> operator++(int)
    {
        // postincrement++
        _xchainedmap_const_iterator<TKey, TData> ret = *this;
        ++*this;
        return ret;
    }

    bool operator==(const _xchainedmap_const_iterator<TKey, TData>& rhs) const
    {
        ASSERT(this->m_Container == rhs.m_Container);
        ASSERT(this->m_Container != NULL);
        return (this->m_SlotIndex == rhs.m_SlotIndex) && (this->m_SlotIterator == rhs.m_SlotIterator);
    }

    bool operator!=(const _xchainedmap_const_iterator<TKey, TData>& rhs) const
    {
        return !(*this == rhs);
    }

    _xchainedmap_const_iterator<TKey, TData>& operator=(const _xchainedmap_const_iterator<TKey, TData>& rhs)
    {
        if (this != &rhs)
        {
            this->m_Container = rhs.m_Container;
            this->m_SlotIndex = rhs.m_SlotIndex;
            this->m_SlotIterator = rhs.m_SlotIterator;
        }

        return *this;
    }

protected:
    XUINT32 m_SlotIndex;
    _xlist_iterator< std::pair<TKey, TData> > m_SlotIterator;
    xchainedmap<TKey, TData>* m_Container;
};

template <typename TKey, typename TData>
class _xchainedmap_iterator
    : public _xchainedmap_const_iterator<TKey, TData>
{
public:
    _xchainedmap_iterator(
                XUINT32 slotIndex,
                _xlist_iterator< std::pair<TKey, TData> > slotIterator,
                xchainedmap<TKey, TData>* container)
        : _xchainedmap_const_iterator<TKey, TData>(slotIndex, slotIterator, container)
    {
    }

    std::pair<TKey, TData>& operator*() const
    {
        ASSERT(this->m_Container != NULL);
        return *this->m_SlotIterator;
    }

    std::pair<TKey, TData>* operator->() const
    {
        ASSERT(this->m_Container != NULL);
        return &**this;
    }

    _xchainedmap_iterator<TKey, TData>& operator++()
    {
        // ++preincrement (the cast to base const class is intentional)
        ++(*static_cast<_xchainedmap_const_iterator<TKey, TData>*>(this));
        return *this;
    }

    _xchainedmap_iterator<TKey, TData> operator++(int)
    {
        // postincrement++
        _xchainedmap_iterator<TKey, TData> ret = *this;
        ++*this;
        return ret;
    }
};



template <typename TKey, typename TData>
class xchainedmap
{
private:
    typedef xpairlist<TKey, TData> TTableEntry;

private:
    // Poison constructors
    xchainedmap& operator=(const xchainedmap& other);
    xchainedmap(const xchainedmap& other);
public:
    //gcc doesn't like the params named "TKey" so using "TKeyF" :(
    template <typename TKeyF, typename TDataF> friend void FindNextSlotAndIterator(
            const xchainedmap< TKeyF, TDataF>* pMap,
            XUINT32& slotIndex,
            _xlist_iterator< std::pair<TKeyF, TDataF> >& slotIterator);

    typedef _xchainedmap_iterator<TKey, TData> iterator;
    typedef _xchainedmap_const_iterator<TKey, TData> const_iterator;

    typedef std::pair<TKey, TData> TPair;
    static const XUINT32 DefaultTableSize = 32;

    _xchainedmap_iterator<TKey, TData> begin()
    {
        typename TTableEntry::iterator slotIterator;
        XUINT32 slotIndex = static_cast<XUINT32>(-1);
        FindNextSlotAndIterator<TKey, TData>(this, slotIndex, slotIterator);
        return _xchainedmap_iterator<TKey, TData>(slotIndex, slotIterator, this);
    }

    _xchainedmap_iterator<TKey, TData> end()
    {
        return _xchainedmap_iterator<TKey, TData>(this->m_table.size(), typename TTableEntry::iterator(), this);
    }

    _xchainedmap_const_iterator<TKey, TData> begin() const
    {
        typename TTableEntry::iterator slotIterator;
        XUINT32 slotIndex = static_cast<XUINT32>(-1);
        FindNextSlotAndIterator<TKey, TData>(this, slotIndex, slotIterator);
        return _xchainedmap_const_iterator<TKey, TData>(slotIndex, slotIterator, this);
    }

    _xchainedmap_const_iterator<TKey, TData> end() const
    {
        return _xchainedmap_const_iterator<TKey,TData>(this->m_table.size(), typename TTableEntry::iterator(), this);
    }

    xchainedmap(XUINT32 startingSize = DefaultTableSize)
        : m_count(0)
    {
        this->m_table.fill_back(NULL, startingSize);
    }

    ~xchainedmap()
    {
        Clear();
    }

    void Clear()
    {
        TTableEntry* list = NULL;
        for (XUINT32 i = 0; i < this->m_table.size(); i++)
        {
            IGNOREHR(this->m_table.get_item(i, list));
            delete list;
            list = NULL;
            IGNOREHR(this->m_table.set_item(i, NULL));
        }
        this->m_count = 0;
    }

    HRESULT Add(const TKey& key, const TData& data)
    {
        HRESULT hr = S_OK;
        TTableEntry* tableSlot = NULL;
        XUINT32 tableIndex = CalculateTruncatedHash(key);

        IFC(this->m_table.get_item(tableIndex, tableSlot));
        if (tableSlot == NULL)
        {
            // No entry in this slot yet.
            tableSlot = new TTableEntry();
            IFC(this->m_table.set_item(tableIndex, tableSlot));
        }

        if (tableSlot->ContainsKey(key))
        {
            // TODO: Should this be an E_ code?
            hr = S_FALSE;
        }
        else
        {
            IFC(tableSlot->push_back(TPair(key, data)));
            this->m_count++;
        }
Cleanup:
        RRETURN(hr);
    }

    HRESULT Get(const TKey& key, TData& outData) const
    {
        HRESULT hr = S_OK;
        TTableEntry* tableSlot = NULL;
        XUINT32 tableIndex = CalculateTruncatedHash(key);
        IFC(this->m_table.get_item(tableIndex, tableSlot));

        if (tableSlot != NULL)
        {
            IFC(tableSlot->Get(key, outData));
        }
        else
        {
            hr = S_FALSE;
        }

Cleanup:
        RRETURN(hr);
    }

    bool ContainsKey(const TKey& key) const
    {
        TData outData;
        return this->Get(key, outData) == S_OK;
    }

    HRESULT Remove(const TKey& key, TData& outData)
    {
        HRESULT hr = S_OK;
        TPair removedPair;
        IFC(this->Remove(key, removedPair));
        outData = removedPair.second;

Cleanup:
        RRETURN(hr);
    }

    HRESULT Remove(const TKey& key, TPair& outData)
    {
        HRESULT hr = S_FALSE;
        TTableEntry* tableSlot = NULL;
        XUINT32 tableIndex = this->CalculateTruncatedHash(key);
        IFC(this->m_table.get_item(tableIndex, tableSlot));

        if (tableSlot != NULL)
        {
            IFC(tableSlot->RemoveByKey(key, outData));
            this->m_count--;
        }

Cleanup:
        RRETURN(hr);
    }

    XUINT32 Count() const
    {
        return this->m_count;
    }

private:
    XUINT32 CalculateTruncatedHash(const TKey& key) const
    {
         return DataStructureFunctionProvider<TKey>::Hash(key) % this->ActualHashSize();
    }

    XUINT32 ActualHashSize() const
    {
        return this->m_table.capacity();
    }



private:
    xvector<TTableEntry*> m_table;
    XUINT32 m_count;
};


//Helper function implementation
template<typename TKey, typename TData>
void FindNextSlotAndIterator(
            const xchainedmap<TKey, TData>* pMap,
            XUINT32& slotIndex,
            _xlist_iterator< std::pair<TKey, TData> >& slotIterator)
{
    XUINT32 startIndex = 0;
    ASSERT(pMap != NULL);
    ASSERT((slotIndex == static_cast<XUINT32>(-1)) || (slotIndex < pMap->m_table.size()));
    if (slotIndex != static_cast<XUINT32>(-1))
    {
        ++slotIterator;
        if(!(slotIterator.IsEnd()))
        {
            return;
        }
        startIndex = slotIndex + 1;
    }

    // This is deliberately less or equal.
    for (XUINT32 i = startIndex; i <= pMap->m_table.size(); i++)
    {
        xpairlist<TKey, TData>* slot = NULL;
        if (i < pMap->m_table.size())
        {
            IGNOREHR(pMap->m_table.get_item(i, slot));
        }

        if (slot != NULL)
        {
            if (!slot->empty())
            {
                slotIterator = slot->begin();
                slotIndex = i;
                return;
            }
        }
    }

    slotIndex = pMap->m_table.size();
    slotIterator = _xlist_iterator< std::pair<TKey, TData> >();
}

