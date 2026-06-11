// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CollectionViewSource.g.h"

namespace DirectUI
{

    typedef CFrameworkEventSource<
        ICVSViewChangedEventSource,
        ICVSViewChangedHandler,
        IInspectable,
        IInspectable> CVSViewChangedEventSource;

    PARTIAL_CLASS(CollectionViewSource)
    {

    protected:

        CollectionViewSource();
        ~CollectionViewSource() override;

        // Handle the custom property changed event and call the
        // OnPropertyChanged methods.
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

    public:

        _Check_return_ HRESULT GetValue(_In_ const CDependencyProperty* pProperty, _Out_ IInspectable** ppValue) override;
        _Check_return_ HRESULT GetCVSViewChangedSource(_Outptr_ ICVSViewChangedEventSource **ppSource);

    private:

        _Check_return_ HRESULT OnSourceChanged(_In_ IInspectable *pOldSource, _In_ IInspectable *pNewSource);
        _Check_return_ HRESULT OnIsSourceGroupedChanged();
        _Check_return_ HRESULT OnItemsPathChanged();
        _Check_return_ HRESULT EnsureView(_In_ IInspectable *pNewSource);

        static bool IsSourceValid(_In_ IInspectable *pSource);

    protected:

        void OnReferenceTrackerWalk(INT walkType ) final;

    private:

        CVSViewChangedEventSource *m_pCVSViewChangedEventSource;
    };
}
