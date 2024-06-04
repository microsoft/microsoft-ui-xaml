// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DragEventArgs.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(DragEventArgs)
    {
    public:
        _Check_return_ HRESULT get_DataImpl(_COM_Outptr_ wadt::IDataPackage** ppValue);
        _Check_return_ HRESULT put_DataImpl(_In_opt_ wadt::IDataPackage* pValue);
        _Check_return_ HRESULT get_DataViewImpl(_COM_Outptr_ wadt::IDataPackageView** ppValue);
        _Check_return_ HRESULT get_ModifiersImpl(_Out_ wadt::DragDrop::DragDropModifiers* pValue);
        _Check_return_ HRESULT get_DragUIOverrideImpl(_COM_Outptr_ xaml::IDragUIOverride** ppValue);

        _Check_return_ HRESULT GetDeferralImpl(
            _Outptr_ xaml::IDragOperationDeferral** returnValue);

        static _Check_return_ HRESULT GetIsDeferred(
            _In_ CDragEventArgs* args,
            _Out_ bool* isDeferred);

    private:
        _Check_return_ HRESULT GetWinRtDragInfo(_Outptr_ mui::DragDrop::IDragInfo** ppDragInfo);

        CDragEventArgs* GetCorePeerAsCDragEventArgs();

        ctl::ComPtr<xaml::IDragUIOverride> m_spDragUIOverride;
    };
}
