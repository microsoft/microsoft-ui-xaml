// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "JoltCollections.h"

namespace DirectUI
{

    // collection of ::Windows::Interal::String with IIterable<HString> interface.

    typedef std::vector<wrl_wrappers::HString> InternalStringVector;
    typedef InternalStringVector::iterator InternalStringIterator;

    class InternalStringCollection; // forward declaration

    class HStringIterator
        : public wfc::IIterator<HSTRING>
        , public ctl::WeakReferenceSource
    {
        BEGIN_INTERFACE_MAP(HStringIterator, WeakReferenceSource)
            INTERFACE_ENTRY(HStringIterator, wfc::IIterator<HSTRING>)
        END_INTERFACE_MAP(HStringIterator, WeakReferenceSource)

    protected:
        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(wfc::IIterator<HSTRING>)))
            {
                *ppObject = static_cast<wfc::IIterator<HSTRING>*>(this);
            }
            else
            {
                return WeakReferenceSource::QueryInterfaceImpl(iid, ppObject);
            }

            AddRefOuter();
            return S_OK;
        }

    public:
        HRESULT STDMETHODCALLTYPE get_Current(_Out_ HSTRING *pCurrent) override;

        HRESULT STDMETHODCALLTYPE get_HasCurrent(_Out_ boolean *hasCurrent) override;

        HRESULT STDMETHODCALLTYPE MoveNext(_Out_ boolean *hasCurrent) override;

    public:
        HRESULT Initialize(_In_ InternalStringCollection* pCollection);

    private:
        TrackerPtr<InternalStringCollection> m_tpCollection;
        unsigned m_currentIndex = 0;
    };

    class InternalStringCollection
        : public wfc::IIterable<HSTRING>
        , public ctl::WeakReferenceSource
    {
        friend class HStringIterator;

        BEGIN_INTERFACE_MAP(InternalStringCollection, WeakReferenceSource)
            INTERFACE_ENTRY(InternalStringCollection, wfc::IIterable<HSTRING>)
        END_INTERFACE_MAP(InternalStringCollection, WeakReferenceSource)

    protected:
        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(wfc::IIterable<HSTRING>)))
            {
                *ppObject = static_cast<wfc::IIterable<HSTRING>*>(this);
            }
            else
            {
                return WeakReferenceSource::QueryInterfaceImpl(iid, ppObject);
            }

            AddRefOuter();
            return S_OK;
        }

    public:
        HRESULT STDMETHODCALLTYPE First(_Outptr_result_maybenull_ wfc::IIterator<HSTRING> **ppFirst) override;

    public:
        void emplace_back(wrl_wrappers::HString&& value)
        {
            m_strings.emplace_back(std::move(value));
        }

        // more vector methods go here when needed.

    private:
        InternalStringVector m_strings;
    };


}