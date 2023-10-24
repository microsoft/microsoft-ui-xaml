// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CDependencyObject;
class CDOCollection;

namespace FocusProperties
{
    class FocusChildrenIteratorWrapper
    {
    public:
        explicit FocusChildrenIteratorWrapper(_In_ CDOCollection* children)
            : m_children(children)
        { }

        explicit FocusChildrenIteratorWrapper(_In_ wfc::IIterator<xaml::DependencyObject*>* iterator)
            : m_customIterator(iterator)
        { }

        FocusChildrenIteratorWrapper(FocusChildrenIteratorWrapper&&) = default;
        FocusChildrenIteratorWrapper& operator=(FocusChildrenIteratorWrapper&&) = default;

        bool TryMoveNext(_Outptr_result_maybenull_ CDependencyObject** childNoRef);

    private:
        CDOCollection* m_children = nullptr;
        unsigned int m_index = 0;
        ctl::ComPtr<wfc::IIterator<xaml::DependencyObject*>> m_customIterator;
    };
}
