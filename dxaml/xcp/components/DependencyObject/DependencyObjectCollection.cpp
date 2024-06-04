// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "DOCollection.h"


#pragma region Modern DOCollection interface
CDOCollection::storage_type::const_iterator CDOCollection::begin() const
{
    return m_items.begin();
}

CDOCollection::storage_type::const_iterator CDOCollection::end() const
{
    return m_items.end();
}

size_t CDOCollection::size() const
{
    return m_items.size();
}

bool CDOCollection::empty() const
{
    return m_items.empty();
}

CDependencyObject* const& CDOCollection::operator[](_In_ size_t index) const
{
    return m_items[index];
}

void CDOCollection::swap(_In_ size_t n1, _In_ size_t n2)
{
    std::swap(m_items[n1], m_items[n2]);
}

void CDOCollection::push_back(_In_ const xref_ptr<CDependencyObject>& dependencyObject)
{
    VERIFYHR(Append(dependencyObject.get()));
    // We could THROW_IF_FAILED here but since exceptions aren't introduced
    // widely in our codebase just yet I'll spare everyone the trouble
    // of worying about the catch block. In practice the only way this should
    // fail is via a bad allocation and that FAIL_FASTs.
}

void CDOCollection::insert(_In_ CDOCollection::storage_type::const_iterator position, const xref_ptr<CDependencyObject>& val)
{
    VERIFYHR(Insert(static_cast<UINT32>(position - begin()), val.get()));
}

void CDOCollection::clear()
{
    VERIFYHR(Clear());
    // It appears that this operation can never fail either. No HRESULT
    // needed here.
}

void CDOCollection::remove(_In_ CDependencyObject* obj)
{
    auto removed = Remove(obj);
    if (removed)
    {
        removed->Release();
    }
}

xref_ptr<CDependencyObject> CDOCollection::back() const
{
    return xref_ptr<CDependencyObject>(m_items.back());
}

void CDOCollection::pop_back()
{
    void* object = RemoveAt(static_cast<XUINT32>(m_items.size() - 1));
    static_cast<CDependencyObject*>(object)->Release();
}
#pragma endregion