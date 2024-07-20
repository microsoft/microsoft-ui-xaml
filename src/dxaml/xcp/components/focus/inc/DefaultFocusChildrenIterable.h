// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WeakReferenceSource.h>
#include <TrackerPtr.h>

namespace FocusProperties
{
    class DefaultFocusChildrenIterable :
        public wfc::IIterable<xaml::DependencyObject*>,
        public ctl::WeakReferenceSource
    {
        INSPECTABLE_CLASS(L"Windows.Foundation.Collections.IIterable`1<Microsoft.UI.Xaml.DependencyObject>");

        BEGIN_INTERFACE_MAP(DefaultFocusChildrenIterable, ctl::WeakReferenceSource)
            INTERFACE_ENTRY(DefaultFocusChildrenIterable, wfc::IIterable<xaml::DependencyObject*>)
        END_INTERFACE_MAP(DefaultFocusChildrenIterable, ctl::WeakReferenceSource)

    public:
        static _Check_return_ HRESULT CreateInstance(
                _In_ xaml::IDependencyObject* parent,
                _Outptr_ wfc::IIterable<xaml::DependencyObject*>** instance);

        _Check_return_ IFACEMETHOD(First)(_Outptr_ wfc::IIterator<xaml::DependencyObject*> **first) final;

    protected:
        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** object) final;

    private:
        DirectUI::TrackerPtr<xaml::IDependencyObject> m_parent;
    };
}
