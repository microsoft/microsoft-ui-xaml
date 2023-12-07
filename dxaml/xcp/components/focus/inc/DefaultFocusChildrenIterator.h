// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WeakReferenceSource.h>
#include <TrackerPtr.h>

namespace FocusProperties
{
    class DefaultFocusChildrenIterator :
        public wfc::IIterator<xaml::DependencyObject*>,
        public ctl::WeakReferenceSource
    {
        INSPECTABLE_CLASS(L"Windows.Foundation.Collections.IIterator`1<Microsoft.UI.Xaml.DependencyObject>");

        BEGIN_INTERFACE_MAP(DefaultFocusChildrenIterator, ctl::WeakReferenceSource)
            INTERFACE_ENTRY(DefaultFocusChildrenIterator, wfc::IIterator<xaml::DependencyObject*>)
        END_INTERFACE_MAP(DefaultFocusChildrenIterator, ctl::WeakReferenceSource)

    public:
        static _Check_return_ HRESULT CreateInstance(
            _In_ xaml::IDependencyObject* parent,
            _Outptr_ wfc::IIterator<xaml::DependencyObject*>** instance);

        _Check_return_ IFACEMETHOD(get_Current)(_Outptr_ xaml::IDependencyObject** current) final;

        _Check_return_ IFACEMETHOD(get_HasCurrent)(_Out_ BOOLEAN *hasCurrent) final;

        _Check_return_ IFACEMETHOD(MoveNext)(_Out_ BOOLEAN *hasCurrent) final;

    protected:
        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** object) final;

    private:
        unsigned GetCount();
        _Check_return_ HRESULT GetAt(_In_ unsigned index, _Outptr_ xaml::IDependencyObject** value);

        DirectUI::TrackerPtr<xaml::IDependencyObject> m_parent;
        unsigned m_index = 0u;
    };
}
