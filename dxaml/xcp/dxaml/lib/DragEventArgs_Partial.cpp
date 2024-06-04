// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DragEventArgs.g.h"
#include "CoreEventArgsGroup.h"
#include "DragDropInternal.h"
#include "IDragOperationDeferralTarget.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT DirectUI::DragEventArgs::get_DataImpl(_COM_Outptr_ wadt::IDataPackage** pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spDataInspectable;
    ctl::ComPtr<wadt::IDataPackage> spData;

    CEventArgs* const pCorePeer = GetCorePeer();

    IFC(static_cast<CDragEventArgs*>(pCorePeer)->get_Data(&spDataInspectable));
    IFC(spDataInspectable.As(&spData));

    *pValue = spData.Detach();

Cleanup:
    ReleaseInterfaceNoNULL(pCorePeer);
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::DragEventArgs::put_DataImpl(_In_ wadt::IDataPackage* value)
{
    HRESULT hr = S_OK;
    xref_ptr<CDragEventArgs> spCoreDragEventArgs;

    spCoreDragEventArgs.attach(GetCorePeerAsCDragEventArgs());
    IFC(spCoreDragEventArgs->put_Data(value));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::DragEventArgs::get_DataViewImpl(_COM_Outptr_ wadt::IDataPackageView** ppValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<mui::DragDrop::IDragInfo> spDragInfo;
    *ppValue = nullptr;

    IFC(GetWinRtDragInfo(&spDragInfo));

    if (spDragInfo != nullptr)
    {
        IFC(spDragInfo->get_Data(ppValue));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::DragEventArgs::get_ModifiersImpl(_Out_ wadt::DragDrop::DragDropModifiers* pValue)
{
    static_assert(wadt::DragDrop::DragDropModifiers::DragDropModifiers_None == static_cast<wadt::DragDrop::DragDropModifiers>(mui::DragDrop::DragDropModifiers::DragDropModifiers_None));
    static_assert(wadt::DragDrop::DragDropModifiers::DragDropModifiers_Shift == static_cast<wadt::DragDrop::DragDropModifiers>(mui::DragDrop::DragDropModifiers::DragDropModifiers_Shift));
    static_assert(wadt::DragDrop::DragDropModifiers::DragDropModifiers_Control == static_cast<wadt::DragDrop::DragDropModifiers>(mui::DragDrop::DragDropModifiers::DragDropModifiers_Control));
    static_assert(wadt::DragDrop::DragDropModifiers::DragDropModifiers_Alt == static_cast<wadt::DragDrop::DragDropModifiers>(mui::DragDrop::DragDropModifiers::DragDropModifiers_Alt));
    static_assert(wadt::DragDrop::DragDropModifiers::DragDropModifiers_LeftButton == static_cast<wadt::DragDrop::DragDropModifiers>(mui::DragDrop::DragDropModifiers::DragDropModifiers_LeftButton));
    static_assert(wadt::DragDrop::DragDropModifiers::DragDropModifiers_MiddleButton == static_cast<wadt::DragDrop::DragDropModifiers>(mui::DragDrop::DragDropModifiers::DragDropModifiers_MiddleButton));
    static_assert(wadt::DragDrop::DragDropModifiers::DragDropModifiers_RightButton == static_cast<wadt::DragDrop::DragDropModifiers>(mui::DragDrop::DragDropModifiers::DragDropModifiers_RightButton));

    HRESULT hr = S_OK;
    ctl::ComPtr<mui::DragDrop::IDragInfo> spDragInfo;

    IFC(GetWinRtDragInfo(&spDragInfo));

    auto modifiers = mui::DragDrop::DragDropModifiers::DragDropModifiers_None;
    if (spDragInfo != nullptr)
    {
        IFC(spDragInfo->get_Modifiers(&modifiers));
    }

    *pValue = static_cast<wadt::DragDrop::DragDropModifiers>(modifiers);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::DragEventArgs::GetDeferralImpl(_Outptr_ IDragOperationDeferral** returnValue)

{
    xref_ptr<CDragEventArgs> spCoreDragEventArgs;

    ctl::ComPtr<DragOperationDeferral> spDeferral;
    ctl::ComPtr<IInspectable> spRaiseDragDropEventAsyncOperationAsI;
    ctl::ComPtr<wf::IAsyncOperation<wadt::DataPackageOperation>> spIAsyncOperation;

    *returnValue = nullptr;

    spCoreDragEventArgs.attach(GetCorePeerAsCDragEventArgs());

    IFC_RETURN(spCoreDragEventArgs->GetDragDropAsyncOperation(&spRaiseDragDropEventAsyncOperationAsI));

    IFC_RETURN(spRaiseDragDropEventAsyncOperationAsI.As(&spIAsyncOperation));

    ctl::ComPtr<IDragOperationDeferralTarget> spDeferralTarget;
    IFC_RETURN(spIAsyncOperation.As(&spDeferralTarget));

    IFC_RETURN(ctl::make<DragOperationDeferral>(spDeferralTarget.Get(), &spDeferral));
    // Give a reference to the event args to the deferral so that it can update the Accepted operation asynchronously
    spDeferral->SetDragEventArgs(this);

    *returnValue = spDeferral.Detach();

    return S_OK;
}

_Check_return_ HRESULT DirectUI::DragEventArgs::GetWinRtDragInfo(_Outptr_ mui::DragDrop::IDragInfo** ppDragInfo)
{
    HRESULT hr = S_OK;

    xref_ptr<CDragEventArgs> spCoreDragEventArgs;
    ctl::ComPtr<IInspectable> spWinRtCoreDragInfoAsInspectable;
    ctl::ComPtr<mui::DragDrop::IDragInfo> spWinRtCoreDragInfo;

    *ppDragInfo = nullptr;

    spCoreDragEventArgs.attach(GetCorePeerAsCDragEventArgs());

    IFC(spCoreDragEventArgs->GetWinRtDragInfo(&spWinRtCoreDragInfoAsInspectable));
    IFC(spWinRtCoreDragInfoAsInspectable.As(&spWinRtCoreDragInfo));

    *ppDragInfo = spWinRtCoreDragInfo.Detach();
Cleanup:
    RRETURN(hr);
}

CDragEventArgs* DirectUI::DragEventArgs::GetCorePeerAsCDragEventArgs()
{
    CEventArgs* const pCoreArgs = GetCorePeer();

    return static_cast<CDragEventArgs*>(pCoreArgs);
}

/*static*/
_Check_return_ HRESULT  DirectUI::DragEventArgs::GetIsDeferred(
    _In_ CDragEventArgs* args,
    _Out_ bool* isDeferred)
{
    *isDeferred = false;

    ASSERT(args);
    ctl::ComPtr<IInspectable> spRaiseDragDropEventAsyncOperationAsInspectable;

    IFC_RETURN(args->GetDragDropAsyncOperation(&spRaiseDragDropEventAsyncOperationAsInspectable));

    if (spRaiseDragDropEventAsyncOperationAsInspectable != nullptr)
    {
        ctl::ComPtr<wf::IAsyncOperation<wadt::DataPackageOperation>> spRaiseDragDropEventAsyncOperation;

        IFC_RETURN(spRaiseDragDropEventAsyncOperationAsInspectable.As(&spRaiseDragDropEventAsyncOperation));
        *isDeferred = spRaiseDragDropEventAsyncOperation.Cast<RaiseDragDropEventAsyncOperation>()->IsDeferred();
    }
    return S_OK;
}

_Check_return_ HRESULT DirectUI::DragEventArgs::get_DragUIOverrideImpl(
    _COM_Outptr_ xaml::IDragUIOverride** ppValue)
{
    *ppValue = nullptr;
    if (!m_spDragUIOverride)
    {
        xref_ptr<CDragEventArgs> spCoreDragEventArgs;
        ctl::ComPtr<IInspectable> spRaiseDragDropEventAsyncOperationAsI;

        spCoreDragEventArgs.attach(GetCorePeerAsCDragEventArgs());
        IFC_RETURN(spCoreDragEventArgs->GetDragDropAsyncOperation(&spRaiseDragDropEventAsyncOperationAsI));

        ctl::ComPtr<DragUIOverride> spDragUIOverride;
        ctl::ComPtr<wf::IAsyncOperation<wadt::DataPackageOperation>> spRaiseDragDropEventAsyncOperation;
        ctl::ComPtr<IInspectable> spSource;

        auto eventType = spCoreDragEventArgs->GetDragDropEventType();
        const bool updateCoreDragUI = (eventType == DragDropMessageType::DragEnter) || (eventType == DragDropMessageType::DragOver);
        if (spRaiseDragDropEventAsyncOperationAsI != nullptr)
        {
            IFC_RETURN(spRaiseDragDropEventAsyncOperationAsI.As(&spRaiseDragDropEventAsyncOperation));
            IFC_RETURN(static_cast<IRoutedEventArgs*>(this)->get_OriginalSource(spSource.GetAddressOf()));
        }
        IFC_RETURN(ctl::make<DragUIOverride>(
            spRaiseDragDropEventAsyncOperation.Get(),
            spSource.Get(),
            updateCoreDragUI,
            &spDragUIOverride));
        IFC_RETURN(spDragUIOverride.As(&m_spDragUIOverride));
    }
    IFC_RETURN(m_spDragUIOverride.CopyTo(ppValue));
    return S_OK;
}
