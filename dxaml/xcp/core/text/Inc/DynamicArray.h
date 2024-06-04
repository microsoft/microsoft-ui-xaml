// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Simple dynamically growing array. Supports only minimal collection functionality

#pragma once

//---------------------------------------------------------------------------
//
//  DynamicArray
//
//  Simple dynamically growing array. Supports only minimal collection functionality:
//      operator[]          - directly address indexed element
//      EnsureCapacity(min) - allocate memory for at least 'min' elements
//
//---------------------------------------------------------------------------
template <class T> class DynamicArray
{
public:
    DynamicArray();
    ~DynamicArray();

    // Gets the number of items contained in the collection.
    XUINT32 GetCount() const;

    // Sets the number of items contained in the collection.
    _Check_return_ HRESULT SetCount(_In_ XUINT32 count);

    // Index operator
    T &operator[](_In_ XUINT32 index);
    const T &operator[](_In_ XUINT32 index) const;

    // Adds an item to the collection.
    _Check_return_ HRESULT Add(_In_ T item);

    // Clears the collection.
    void Clear();

private:
    XUINT32 m_capacity;
    XUINT32 m_cElements;
    _Field_size_(m_capacity) T *m_pElements;

    // Ensures that the capacity of the collection is at least the specified value.
    _Check_return_ HRESULT EnsureCapacity(_In_ XUINT32 count);

};

//---------------------------------------------------------------------------
//
//  Initializes a new instance of the DynamicArray class.
//  Initially collection is empty.
//
//---------------------------------------------------------------------------
template <class T> DynamicArray<T>::DynamicArray()
    : m_capacity(0), m_cElements(0), m_pElements(NULL)
{
}

//---------------------------------------------------------------------------
//
//  Release resources associated with the DynamicArray.
//
//---------------------------------------------------------------------------
template <class T> DynamicArray<T>::~DynamicArray()
{
    delete [] m_pElements;
}

//---------------------------------------------------------------------------
//
//  Gets the number of items contained in the collection.
//
//---------------------------------------------------------------------------
template <class T> XUINT32 DynamicArray<T>::GetCount() const
{
    return m_cElements;
}

//---------------------------------------------------------------------------
//
//  Index operator.
//
//---------------------------------------------------------------------------
template <class T> T & DynamicArray<T>::operator[](_In_ XUINT32 index)
{
    ASSERT(index >= 0 && index < m_cElements);
    return m_pElements[index];
}

//---------------------------------------------------------------------------
//
//  Index operator.
//
//---------------------------------------------------------------------------
template <class T> const T & DynamicArray<T>::operator[](_In_ XUINT32 index) const
{
    ASSERT(index >= 0 && index < m_cElements);
    return m_pElements[index];
}

//---------------------------------------------------------------------------
//
//  Clears the collection.
//
//---------------------------------------------------------------------------
template <class T> void DynamicArray<T>::Clear()
{
    delete [] m_pElements;
    m_pElements = NULL;
    m_cElements = 0;
    m_capacity = 0;
}

//---------------------------------------------------------------------------
//
//  Member:
//      Sets the number of items contained in the collection.
//
//---------------------------------------------------------------------------
template <class T> _Check_return_ HRESULT DynamicArray<T>::SetCount(_In_ XUINT32 count)
{
    IFC_RETURN(EnsureCapacity(count));
    m_cElements = count;

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Adds an item to the collection.
//
//---------------------------------------------------------------------------
template <class T> _Check_return_ HRESULT DynamicArray<T>::Add(_In_ T item)
{
    IFC_RETURN(EnsureCapacity(m_cElements + 1));
    m_pElements[m_cElements] = item;
    m_cElements++;

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Ensures that the capacity of the collection is at least the specified value.
//
//  Notes:
//      This method grows the collection to at least the specified amount or 
//      by 50%, which ever is greater.
//
//---------------------------------------------------------------------------
template <class T> _Check_return_ HRESULT DynamicArray<T>::EnsureCapacity(_In_ XUINT32 count)
{
    HRESULT hr = S_OK;
    T *pNewElements = NULL;
    XUINT32 newCapacity;

    if (m_capacity < count)
    {
        if (m_capacity == 0)
        {
            newCapacity = 1;
        }
        else
        {
            newCapacity = m_capacity + (m_capacity >> 1);
            if (newCapacity < 4)
            {
                newCapacity = 4;
            }
        }
        if (newCapacity < count)
        {
            newCapacity = count;
        }

        pNewElements = new T[newCapacity];
        if (m_pElements != NULL)
        {
            std::uninitialized_copy(m_pElements, m_pElements + m_cElements, pNewElements);
            delete [] m_pElements;
        }

        m_pElements = pNewElements;
        m_capacity = newCapacity;
    }

    RRETURN(hr);//RRETURN_REMOVAL
}
