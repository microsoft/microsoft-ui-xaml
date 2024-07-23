// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "InputPointEventArgs.h"

class CDragEventArgs final : public CInputPointEventArgs
{
public:
    static _Check_return_ HRESULT Create(_In_ CCoreServices* pCore, _Outptr_ CDragEventArgs** ppArgs, _In_opt_ IInspectable* pWinRtDragInfo = nullptr, _In_opt_ IInspectable* pDragDropAsyncOperation = nullptr);

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    xstring_ptr m_strFilePaths;

    // This specifies if the EventArgs are created by a Drop Event
    // so that we can allow access to the GetData and GetFormats methods on IDataObject
    bool m_bAllowDataAccess{};

    // Copy the given object into m_spData (pointer copy only), then
    // peg the managed object reference contained within.
    _Check_return_ HRESULT put_Data(_In_ IInspectable* pNewData);

    _Check_return_ HRESULT get_Data(_Outptr_ IInspectable** ppData);

    DirectUI::DragDropMessageType GetDragDropEventType() const
    {
        return m_dragDropMessageType;
    }

    void SetDragDropEventType(_In_ DirectUI::DragDropMessageType dragDropMessageType)
    {
        m_dragDropMessageType = dragDropMessageType;
    }

    _Check_return_ HRESULT get_AllowDataAccess(__deref BOOLEAN* pValue)
    {
        *pValue = (BOOLEAN)m_bAllowDataAccess;
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT put_AllowDataAccess(_In_ BOOLEAN value)
    {
        m_bAllowDataAccess = !!value;
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT get_AcceptedOperation(_Out_ DirectUI::DataPackageOperation* pOperation);

    _Check_return_ HRESULT put_AcceptedOperation(_In_ DirectUI::DataPackageOperation operation);

    _Check_return_ HRESULT GetWinRtDragInfo(_Outptr_result_maybenull_ IInspectable** ppWinRtDragInfo);

    _Check_return_ HRESULT GetDragDropAsyncOperation(_Outptr_result_maybenull_ IInspectable** ppDragDropAsyncOperation);

    _Check_return_ HRESULT GetIsDeferred(_In_ CCoreServices* core, _Out_ bool* isDeferred);

    // Internal method to update the accepted operation when InputManager is using different EventArgs
    // in the same sequence
    void UpdateAcceptedOperation(_In_ DirectUI::DataPackageOperation operation);

    _Check_return_ HRESULT put_AllowedOperations(DirectUI::DataPackageOperation allowedOperations)
    {
        m_allowedOperations = allowedOperations;
        return S_OK;
    }

    _Check_return_ HRESULT get_AllowedOperations(DirectUI::DataPackageOperation* allowedOperations)
    {
        (*allowedOperations) = m_allowedOperations;
        return S_OK;
    }
protected:
    CDragEventArgs(_In_ CCoreServices* pCore) :
        CInputPointEventArgs(pCore)
    {
    }

    ~CDragEventArgs();

private:
    // The value of our Data property (a DataPackage stored within a ManagedObjectReference within a CValue).
    xref_ptr<IInspectable> m_spData;
    xref_ptr<IInspectable> m_spWinRtDragInfo;
    xref_ptr<IInspectable> m_spDragDropAsyncOperation;
    DirectUI::DataPackageOperation m_acceptedOperation = DirectUI::DataPackageOperation::DataPackageOperation_None;
    DirectUI::DragDropMessageType m_dragDropMessageType = DirectUI::DragDropMessageType::DragEnter;
    DirectUI::DataPackageOperation m_allowedOperations =
        DirectUI::DataPackageOperation::DataPackageOperation_Copy |
        DirectUI::DataPackageOperation::DataPackageOperation_Move |
        DirectUI::DataPackageOperation::DataPackageOperation_Link;
};
