// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlCompositionBrushBase.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(XamlCompositionBrushBase)
    {
    public:
        static _Check_return_ HRESULT OnConnectedFromCore(_In_ CDependencyObject* object);
        _Check_return_ HRESULT OnConnectedImpl();

        static _Check_return_ HRESULT OnDisconnectedFromCore(_In_ CDependencyObject* object);
        _Check_return_ HRESULT OnDisconnectedImpl();

        static _Check_return_ HRESULT OnElementConnectedFromCore(_In_ CDependencyObject* object, _In_ CDependencyObject* connectedElement);

        _Check_return_ HRESULT get_CompositionBrushImpl(_Outptr_result_maybenull_ WUComp::ICompositionBrush** ppValue);
        _Check_return_ HRESULT put_CompositionBrushImpl(_In_opt_ WUComp::ICompositionBrush* pValue);

        static bool HasPrivateOverrides(_In_ CDependencyObject* object);

        _Check_return_ HRESULT SetBrushForXamlRootImpl(_In_ IInspectable* contentRoot, _In_ WUComp::ICompositionBrush* brush);
        _Check_return_ HRESULT GetBrushForXamlRootImpl(_In_ IInspectable* contentRoot, _Outptr_result_maybenull_ WUComp::ICompositionBrush** brush);
        _Check_return_ HRESULT ClearCompositionBrushMapImpl();
        _Check_return_ HRESULT ClearBrushForXamlRootImpl(_In_ IInspectable* contentRoot, _In_ WUComp::ICompositionBrush* brush);

    private:
        _Check_return_ HRESULT OnConnectedHelper();
        _Check_return_ HRESULT OnDisconnectedHelper();

        xaml_media::IXamlCompositionBrushBaseOverridesPrivate* GetPrivateOverridesNoRef();

        bool m_didQueryForPrivateOverrides = false;
        xaml_media::IXamlCompositionBrushBaseOverridesPrivate* m_privateOverridesNoRef{};
    };
}

