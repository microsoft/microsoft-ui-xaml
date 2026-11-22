// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <functional>
#include <memory>
#include <vector>

namespace DirectUI
{
    // The property paths of ICollectionViewLiveShaping.Live*Properties and CollectionViewSource.Live*Properties
    // (WPF: ObservableCollection<string>). The owner hears every change through the callback. IUntypedVector is how
    // markup fills it (<x:String> items).
    class LiveShapingPropertyCollection final :
        public wrl::RuntimeClass<
            wrl::RuntimeClassFlags<wrl::RuntimeClassType::WinRt>,
            wfc::IIterable<HSTRING>,
            wfc::IVector<HSTRING>,
            wfc::IObservableVector<HSTRING>,
            IUntypedVector>
    {
        InspectableClass(nullptr /*internal*/, BaseTrust);

    public:
        using Strings = std::vector<wrl_wrappers::HString>;

    private:
        class Snapshot final :
            public wrl::RuntimeClass<
                wrl::RuntimeClassFlags<wrl::RuntimeClassType::WinRt>,
                wfc::IIterable<HSTRING>,
                wfc::IVectorView<HSTRING>>
        {
            InspectableClass(nullptr /*internal*/, BaseTrust);

        public:
            HRESULT RuntimeClassInitialize(std::shared_ptr<const Strings> strings)
            {
                m_strings = std::move(strings);
                return S_OK;
            }

            IFACEMETHODIMP First(_Outptr_result_maybenull_ wfc::IIterator<HSTRING>** ppFirst) override
            {
                return wrl::MakeAndInitialize<Iterator>(ppFirst, m_strings);
            }

            IFACEMETHODIMP GetAt(unsigned int index, _Out_ HSTRING* pItem) override
            {
                *pItem = nullptr;
                IFCEXPECTRC_RETURN(index < m_strings->size(), E_BOUNDS);
                return (*m_strings)[index].CopyTo(pItem);
            }

            IFACEMETHODIMP get_Size(_Out_ unsigned int* pSize) override
            {
                *pSize = static_cast<unsigned int>(m_strings->size());
                return S_OK;
            }

            IFACEMETHODIMP IndexOf(_In_opt_ HSTRING value, _Out_ unsigned int* pIndex, _Out_ boolean* pFound) override
            {
                return LiveShapingPropertyCollection::Find(*m_strings, value, pIndex, pFound);
            }

            IFACEMETHODIMP GetMany(unsigned int startIndex, unsigned int capacity, _Out_writes_to_(capacity, *pActual) HSTRING* pItems, _Out_ unsigned int* pActual) override
            {
                *pActual = 0;
                for (unsigned int i = startIndex; i < m_strings->size() && *pActual < capacity; ++i)
                {
                    IFC_RETURN((*m_strings)[i].CopyTo(&pItems[(*pActual)++]));
                }
                return S_OK;
            }

        private:
            std::shared_ptr<const Strings> m_strings;
        };

        class Iterator final :
            public wrl::RuntimeClass<wrl::RuntimeClassFlags<wrl::RuntimeClassType::WinRt>, wfc::IIterator<HSTRING>>
        {
            InspectableClass(nullptr /*internal*/, BaseTrust);

        public:
            HRESULT RuntimeClassInitialize(std::shared_ptr<const Strings> strings)
            {
                m_strings = std::move(strings);
                return S_OK;
            }

            IFACEMETHODIMP get_Current(_Out_ HSTRING* pCurrent) override
            {
                *pCurrent = nullptr;
                IFCEXPECTRC_RETURN(m_index < m_strings->size(), E_BOUNDS);
                return (*m_strings)[m_index].CopyTo(pCurrent);
            }

            IFACEMETHODIMP get_HasCurrent(_Out_ boolean* pHasCurrent) override
            {
                *pHasCurrent = m_index < m_strings->size();
                return S_OK;
            }

            IFACEMETHODIMP MoveNext(_Out_ boolean* pHasCurrent) override
            {
                if (m_index < m_strings->size())
                {
                    ++m_index;
                }
                return get_HasCurrent(pHasCurrent);
            }

            IFACEMETHODIMP GetMany(unsigned int capacity, _Out_writes_to_(capacity, *pActual) HSTRING* pItems, _Out_ unsigned int* pActual) override
            {
                *pActual = 0;
                while (m_index < m_strings->size() && *pActual < capacity)
                {
                    IFC_RETURN((*m_strings)[m_index++].CopyTo(&pItems[(*pActual)++]));
                }
                return S_OK;
            }

        private:
            std::shared_ptr<const Strings> m_strings;
            size_t m_index = 0;
        };

    public:
        void SetChangedCallback(std::function<HRESULT()> callback)
        {
            m_changed = std::move(callback);
        }

        // The paths, without raising anything (WPF's ApplyPropertiesToView copies them the same way).
        const Strings& Paths() const
        {
            return *m_strings;
        }

        _Check_return_ HRESULT SetPaths(_In_ const Strings& paths)
        {
            auto strings = std::make_shared<Strings>();
            strings->reserve(paths.size());
            for (const auto& path : paths)
            {
                wrl_wrappers::HString copy;
                IFC_RETURN(copy.Set(path.Get()));
                strings->push_back(std::move(copy));
            }
            m_strings = std::move(strings);
            IFC_RETURN(Raise(wfc::CollectionChange_Reset, 0));
            return S_OK;
        }

        // IIterable<HSTRING>
        IFACEMETHODIMP First(_Outptr_result_maybenull_ wfc::IIterator<HSTRING>** ppFirst) override
        {
            return wrl::MakeAndInitialize<Iterator>(ppFirst, m_strings);
        }

        // IVector<HSTRING>
        IFACEMETHODIMP GetAt(unsigned int index, _Out_ HSTRING* pItem) override
        {
            *pItem = nullptr;
            IFCEXPECTRC_RETURN(index < m_strings->size(), E_BOUNDS);
            return (*m_strings)[index].CopyTo(pItem);
        }

        IFACEMETHODIMP get_Size(_Out_ unsigned int* pSize) override
        {
            *pSize = static_cast<unsigned int>(m_strings->size());
            return S_OK;
        }

        IFACEMETHODIMP GetView(_Outptr_result_maybenull_ wfc::IVectorView<HSTRING>** ppView) override
        {
            return wrl::MakeAndInitialize<Snapshot>(ppView, m_strings);
        }

        IFACEMETHODIMP IndexOf(_In_opt_ HSTRING value, _Out_ unsigned int* pIndex, _Out_ boolean* pFound) override
        {
            return Find(*m_strings, value, pIndex, pFound);
        }

        IFACEMETHODIMP SetAt(unsigned int index, _In_opt_ HSTRING item) override
        {
            IFCEXPECTRC_RETURN(index < m_strings->size(), E_BOUNDS);
            auto strings = Copy();
            IFC_RETURN((*strings)[index].Set(item));
            m_strings = std::move(strings);
            return Raise(wfc::CollectionChange_ItemChanged, index);
        }

        IFACEMETHODIMP InsertAt(unsigned int index, _In_opt_ HSTRING item) override
        {
            IFCEXPECTRC_RETURN(index <= m_strings->size(), E_BOUNDS);
            wrl_wrappers::HString value;
            IFC_RETURN(value.Set(item));
            auto strings = Copy();
            strings->insert(strings->begin() + index, std::move(value));
            m_strings = std::move(strings);
            return Raise(wfc::CollectionChange_ItemInserted, index);
        }

        IFACEMETHODIMP RemoveAt(unsigned int index) override
        {
            IFCEXPECTRC_RETURN(index < m_strings->size(), E_BOUNDS);
            auto strings = Copy();
            strings->erase(strings->begin() + index);
            m_strings = std::move(strings);
            return Raise(wfc::CollectionChange_ItemRemoved, index);
        }

        IFACEMETHODIMP Append(_In_opt_ HSTRING item) override
        {
            return InsertAt(static_cast<unsigned int>(m_strings->size()), item);
        }

        IFACEMETHODIMP RemoveAtEnd() override
        {
            IFCEXPECTRC_RETURN(!m_strings->empty(), E_BOUNDS);
            return RemoveAt(static_cast<unsigned int>(m_strings->size() - 1));
        }

        IFACEMETHODIMP Clear() override
        {
            m_strings = std::make_shared<Strings>();
            return Raise(wfc::CollectionChange_Reset, 0);
        }

        IFACEMETHODIMP GetMany(unsigned int startIndex, unsigned int capacity, _Out_writes_to_(capacity, *pActual) HSTRING* pItems, _Out_ unsigned int* pActual) override
        {
            *pActual = 0;
            for (unsigned int i = startIndex; i < m_strings->size() && *pActual < capacity; ++i)
            {
                IFC_RETURN((*m_strings)[i].CopyTo(&pItems[(*pActual)++]));
            }
            return S_OK;
        }

        IFACEMETHODIMP ReplaceAll(unsigned int count, _In_reads_(count) HSTRING* pItems) override
        {
            auto strings = std::make_shared<Strings>(count);
            for (unsigned int i = 0; i < count; ++i)
            {
                IFC_RETURN((*strings)[i].Set(pItems[i]));
            }
            m_strings = std::move(strings);
            return Raise(wfc::CollectionChange_Reset, 0);
        }

        // IUntypedVector
        IFACEMETHODIMP UntypedAppend(_In_ IInspectable* pItem) override
        {
            wrl_wrappers::HString value;
            IFC_RETURN(Unbox(pItem, value.GetAddressOf()));
            return Append(value.Get());
        }

        IFACEMETHODIMP UntypedGetSize(_Out_ unsigned int* pSize) override
        {
            return get_Size(pSize);
        }

        IFACEMETHODIMP UntypedGetAt(_In_ unsigned int index, _COM_Outptr_ IInspectable** ppItem) override
        {
            *ppItem = nullptr;
            IFCEXPECTRC_RETURN(index < m_strings->size(), E_BOUNDS);
            return DirectUI::PropertyValue::CreateFromString((*m_strings)[index].Get(), ppItem);
        }

        IFACEMETHODIMP UntypedInsertAt(_In_ unsigned int index, _In_ IInspectable* pItem) override
        {
            wrl_wrappers::HString value;
            IFC_RETURN(Unbox(pItem, value.GetAddressOf()));
            return InsertAt(index, value.Get());
        }

        IFACEMETHODIMP UntypedRemoveAt(_In_ unsigned int index) override
        {
            return RemoveAt(index);
        }

        IFACEMETHODIMP UntypedClear() override
        {
            return Clear();
        }

        // IObservableVector<HSTRING>
        IFACEMETHODIMP add_VectorChanged(_In_ wfc::VectorChangedEventHandler<HSTRING>* pHandler, _Out_ EventRegistrationToken* pToken) override
        {
            return m_eventSource.Add(pHandler, pToken);
        }

        IFACEMETHODIMP remove_VectorChanged(EventRegistrationToken token) override
        {
            return m_eventSource.Remove(token);
        }

    private:
        static HRESULT Unbox(_In_ IInspectable* pItem, _Out_ HSTRING* pValue)
        {
            ctl::ComPtr<wf::IPropertyValue> spValue;
            *pValue = nullptr;
            IFC_RETURN(ctl::do_query_interface(spValue, pItem));
            IFC_RETURN(spValue->GetString(pValue));
            return S_OK;
        }

        static HRESULT Find(const Strings& strings, _In_opt_ HSTRING value, _Out_ unsigned int* pIndex, _Out_ boolean* pFound)
        {
            *pIndex = 0;
            *pFound = FALSE;
            for (size_t i = 0; i < strings.size(); ++i)
            {
                INT32 result = 1;
                IFC_RETURN(WindowsCompareStringOrdinal(strings[i].Get(), value, &result));
                if (result == 0)
                {
                    *pIndex = static_cast<unsigned int>(i);
                    *pFound = TRUE;
                    break;
                }
            }
            return S_OK;
        }

        // Copy on write, so an iterator or GetView snapshot is never invalidated.
        std::shared_ptr<Strings> Copy() const
        {
            auto strings = std::make_shared<Strings>();
            strings->reserve(m_strings->size() + 1);
            for (const auto& value : *m_strings)
            {
                wrl_wrappers::HString copy;
                IGNOREHR(copy.Set(value.Get()));
                strings->push_back(std::move(copy));
            }
            return strings;
        }

        HRESULT Raise(wfc::CollectionChange change, unsigned int index)
        {
            ctl::ComPtr<wfc::IVectorChangedEventArgs> spArgs;
            VectorChangedEventArgs* pArgs = nullptr;
            IFC_RETURN(ctl::ComObject<VectorChangedEventArgs>::CreateInstance(spArgs.ReleaseAndGetAddressOf()));
            spArgs.CastTo(&pArgs);
            IFC_RETURN(pArgs->put_CollectionChange(change));
            IFC_RETURN(pArgs->put_Index(index));
            IFC_RETURN(m_eventSource.InvokeAll(this, spArgs.Get()));
            if (m_changed)
            {
                IFC_RETURN(m_changed());
            }
            return S_OK;
        }

        std::shared_ptr<const Strings> m_strings = std::make_shared<Strings>();
        wrl::EventSource<wfc::VectorChangedEventHandler<HSTRING>> m_eventSource;
        std::function<HRESULT()> m_changed;
    };
}
