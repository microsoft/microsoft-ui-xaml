// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DragDropInternal.h"
#include "ListViewBaseItem.g.h"
#include "CoreEventArgsGroup.h"
#include "microsoft.ui.input.dragdrop.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

DragDrop::DragDrop()
    : m_isDragDropInProgress(FALSE)
    , m_isWinRTDragDropInProgress(false)
    , m_cItemCount(0)
    , m_allowedOperations(wadt::DataPackageOperation::DataPackageOperation_Copy |
                          wadt::DataPackageOperation::DataPackageOperation_Move |
                          wadt::DataPackageOperation::DataPackageOperation_Link)
{
}

DragDrop::~DragDrop()
{
    VERIFYHR(CleanupDragDrop());
}

_Check_return_ HRESULT DragDrop::CleanupDragDrop()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spDragItemUIE;
    ctl::ComPtr<ListViewBaseItem> spDragItemLVBI;

    IFC(m_wrPrimaryDraggedItemWeakRef.As(&spDragItemUIE));
    if (spDragItemUIE)
    {
        if (SUCCEEDED(ListViewBaseItem::QueryFor(spDragItemUIE.Get(), &spDragItemLVBI)))
        {
            IFC(spDragItemLVBI->SetDragVisualCaptured(FALSE /*dragVisualCaptured*/));

            IGNOREHR(spDragItemLVBI->SetDragIsGrabbed(FALSE));
        }
    }

    m_isDragDropInProgress = FALSE;
    m_spCurrentDragData = nullptr;
    m_wrCurrentDragSourceWeakRef = nullptr;
    m_wrPrimaryDraggedItemWeakRef = nullptr;
    m_spCapturedDragSource = nullptr;

    if (m_spDragDropVisual)
    {
        IFC(m_spDragDropVisual->Hide());
    }
    m_cItemCount = 0;

Cleanup:
    m_spDragDropVisual = nullptr;
    RRETURN(hr);
}

_Check_return_ HRESULT DragDrop::DragStart(
    _In_ xaml::IUIElement* pDragSource,
    _In_ xaml::IUIElement* pDragItem,
    _In_ wadt::IDataPackage* pData,
    _In_ DragDropVisual* pDragVisual,
    _In_ wf::Point dragStartPoint,
    _In_ INT32 cItemCount,
    _In_ BOOLEAN shouldFireEvent)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ListViewBaseItem> spDragItemLVBI;

    IFCPTR(pDragSource);
    IFCPTR(pDragItem);
    IFCPTR(pData);
    IFCPTR(pDragVisual);
    IFC(0 < cItemCount ? S_OK : E_INVALIDARG);

    // Cannot do more than 1 drag and drop at a time.
    IFCEXPECT(!m_isDragDropInProgress);

    IFC(ctl::AsWeak<IUIElement>(pDragSource, &m_wrCurrentDragSourceWeakRef));
    IFC(ctl::AsWeak<IUIElement>(pDragItem, &m_wrPrimaryDraggedItemWeakRef));

    // dragStartPoint is relative to us. CoreImports::DragDrop_RaiseEvent needs it relative to the root visual.
    IFC(UpdateUserDragPosition(dragStartPoint));

    m_spDragDropVisual = pDragVisual;

    IFC(m_spDragDropVisual->Show());

    IFC(m_spDragDropVisual->SetPosition(m_userDragPosition));

    m_isDragDropInProgress = TRUE;
    m_spCurrentDragData = pData;

    if(shouldFireEvent)
    {
        ctl::ComPtr<IUIElement> dragSource(pDragSource);
        CContentRoot* contentRoot = VisualTree::GetContentRootForElement(dragSource.Cast<UIElement>()->GetHandle());

        IFC(CoreImports::DragDrop_RaiseEvent(contentRoot, static_cast<CCoreServices*>(DXamlCore::GetCurrent()->GetHandle()), DragDropMessageType::DragEnter, GetUserDragPositionAsXPOINTF()));
    }

    if (SUCCEEDED(ListViewBaseItem::QueryFor(pDragItem, &spDragItemLVBI)))
    {
        IFC(spDragItemLVBI->SetDragIsGrabbed(TRUE));
    }
    m_cItemCount = cItemCount;

Cleanup:
    if(FAILED(hr))
    {
        VERIFYHR(CleanupDragDrop());
    }
    RRETURN(hr);
}

_Check_return_ HRESULT DragDrop::Drop(_In_ BOOLEAN shouldFireEvent)
{
    HRESULT hr = S_OK;
    HRESULT cleanupHr = S_OK;

    // No drag was started.
    IFCEXPECT(m_isDragDropInProgress);

    if(shouldFireEvent)
    {
        ctl::ComPtr<IUIElement> spCurrentDragSourceAsUIE;
        IFC(m_wrCurrentDragSourceWeakRef.As<IUIElement>(&spCurrentDragSourceAsUIE));
        CContentRoot* contentRoot = VisualTree::GetContentRootForElement(spCurrentDragSourceAsUIE.Cast<UIElement>()->GetHandle());

        IFC(CoreImports::DragDrop_RaiseEvent(contentRoot, static_cast<CCoreServices*>(DXamlCore::GetCurrent()->GetHandle()), DragDropMessageType::Drop, GetUserDragPositionAsXPOINTF()));
    }

Cleanup:
    if(shouldFireEvent || FAILED(hr))
    {
        cleanupHr = CleanupDragDrop();
    }

    if(SUCCEEDED(hr))
    {
        // Return the hr of CleanupDragDrop if the main method body succeeded.
        hr = cleanupHr;
    }
    RRETURN(hr);
}

_Check_return_ HRESULT DragDrop::DragCancel(_In_ BOOLEAN shouldFireEvent)
{
    HRESULT hr = S_OK;
    HRESULT cleanupHr = S_OK;

    // No drag was started.
    IFCEXPECT(m_isDragDropInProgress);

    if(shouldFireEvent)
    {
        ctl::ComPtr<IUIElement> spCurrentDragSourceAsUIE;
        IFC(m_wrCurrentDragSourceWeakRef.As<IUIElement>(&spCurrentDragSourceAsUIE));
        CContentRoot* contentRoot = VisualTree::GetContentRootForElement(spCurrentDragSourceAsUIE.Cast<UIElement>()->GetHandle());

        IFC(CoreImports::DragDrop_RaiseEvent(contentRoot, static_cast<CCoreServices*>(DXamlCore::GetCurrent()->GetHandle()), DragDropMessageType::DragLeave, GetUserDragPositionAsXPOINTF()));
    }
Cleanup:
    if(shouldFireEvent || FAILED(hr))
    {
        cleanupHr = CleanupDragDrop();
    }

    if(SUCCEEDED(hr))
    {
        // Return the hr of CleanupDragDrop if the main method body succeeded.
        hr = cleanupHr;
    }
    RRETURN(hr);
}

_Check_return_ HRESULT DragDrop::DragMove(
    _In_ wf::Point newDragPosition,
    _Out_opt_ wf::Point* pCalculatedIconTopLeft,
    _In_ BOOLEAN shouldFireEvent)
{
    HRESULT hr = S_OK;

    // No drag was started.
    IFCEXPECT(m_isDragDropInProgress);
    IFCPTR(m_spDragDropVisual.Get());

    if (pCalculatedIconTopLeft != NULL)
    {
        pCalculatedIconTopLeft->X = 0;
        pCalculatedIconTopLeft->Y = 0;
    }

    IFC(UpdateUserDragPosition(newDragPosition));

    if(shouldFireEvent)
    {
        ctl::ComPtr<IUIElement> spCurrentDragSourceAsUIE;
        IFC(m_wrCurrentDragSourceWeakRef.As<IUIElement>(&spCurrentDragSourceAsUIE));
        CContentRoot* contentRoot = VisualTree::GetContentRootForElement(spCurrentDragSourceAsUIE.Cast<UIElement>()->GetHandle());

        IFC(CoreImports::DragDrop_RaiseEvent(contentRoot, static_cast<CCoreServices*>(DXamlCore::GetCurrent()->GetHandle()), DragDropMessageType::DragOver, GetUserDragPositionAsXPOINTF()));
    }

    IFC(m_spDragDropVisual->SetPosition(m_userDragPosition));
    if (pCalculatedIconTopLeft != NULL)
    {
        if (shouldFireEvent)
        {
            IFC(m_spDragDropVisual->GetTopLeftPosition(pCalculatedIconTopLeft));
        }
        else
        {
            *pCalculatedIconTopLeft = newDragPosition;
        }
    }

Cleanup:
    RRETURN(hr);
}

// This RPInvoke prepares an IDragEventArgs instance by supplying the current
// data package and allowed operations as its Data.
_Check_return_ HRESULT DragDrop::PopulateDragEventArgs(
    _In_ CDragEventArgs* pEventArgs)
{
    ctl::ComPtr<wadt::IDataPackage> spData;

    DragDrop* dragDrop = DXamlCore::GetCurrent()->GetDragDrop();
    IFC_RETURN(dragDrop->GetCurrentDragData(&spData));
    IFC_RETURN(pEventArgs->put_Data(spData.Get()));

    wadt::DataPackageOperation allowedOperations;
    IFC_RETURN(dragDrop->GetAllowedOperations(&allowedOperations));
    IFC_RETURN(pEventArgs->put_AllowedOperations(static_cast<DirectUI::DataPackageOperation>(static_cast<UINT>(allowedOperations))));

    return S_OK;
}

// This RPInvoke calls a helper method that checks if the source is the UIElement that sets it and sets
// m_shouldClearCustomVisual to indicate whether or not custom visual should be cleared.
/* static */
_Check_return_ HRESULT DragDrop::CheckIfCustomVisualShouldBeCleared(
    _In_ CDependencyObject* source)
{
    ctl::ComPtr<DependencyObject> spDragLeaveUIElementAsDO;

    if (SUCCEEDED(DXamlCore::GetCurrent()->GetPeer(source, &spDragLeaveUIElementAsDO)))
    {
        // Some DO such as RootVisual doesn't have a framework peer, in that case, we will skip the check
        DXamlCore::GetCurrent()->GetDragDrop()->CheckIfCustomVisualShouldBeCleared(ctl::as_iinspectable(spDragLeaveUIElementAsDO.Get()));
    }

    return S_OK;
}



// Helper method that checks if the source is the UIElement that sets it and sets
// m_shouldClearCustomVisual to indicate whether or not custom visual should be cleared.
void DragDrop::CheckIfCustomVisualShouldBeCleared(
    _In_ IInspectable* sourceAsInspectable)
{
    ASSERT(sourceAsInspectable);
    if (ctl::are_equal(m_spCustomVisualSetterIInspectable.Get(), sourceAsInspectable))
    {
        m_shouldClearCustomVisual = true;
        m_spCustomVisualSetterIInspectable = nullptr;
    }
}

// This is called when
// - App sets the custom drag visual on the drop site,
// we cache the setter UIElement and we no longer need to clear the override visual.
// - Core Drop/LeaveAsync is called, setterUIElement is null, to reset the states.
void DragDrop::SetCustomVisualSetterUIElement(_In_opt_ IInspectable* setterUIElement)
{
    // Set custom visual setter and do not clear override once this is called
    m_shouldClearCustomVisual = false;
    m_spCustomVisualSetterIInspectable = setterUIElement;
}

_Check_return_ HRESULT DragDrop::UpdateUserDragPosition(_In_ wf::Point newDragPosition)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<xaml_media::IGeneralTransform> spToRoot;
    ctl::ComPtr<IUIElement> spCurrentDragSourceAsUIE;

    IFC(m_wrCurrentDragSourceWeakRef.As<IUIElement>(&spCurrentDragSourceAsUIE));
    IFC(spCurrentDragSourceAsUIE->TransformToVisual(NULL, &spToRoot));
    IFC(spToRoot->TransformPoint(newDragPosition, &m_userDragPosition));

Cleanup:
    RRETURN(hr);
}

XPOINTF DragDrop::GetUserDragPositionAsXPOINTF()
{
    return {m_userDragPosition.X, m_userDragPosition.Y};
}

_Check_return_ HRESULT DragDrop::HidePrimaryDraggedItem()
{
    ctl::ComPtr<IUIElement> spDragItemUIE;
    ctl::ComPtr<ListViewBaseItem> spDragItemLVBI;

    IFC_RETURN(m_wrPrimaryDraggedItemWeakRef.As(&spDragItemUIE));
    if (spDragItemUIE)
    {
        if (SUCCEEDED(ListViewBaseItem::QueryFor(spDragItemUIE.Get(), &spDragItemLVBI)))
        {
            IFC_RETURN(spDragItemLVBI->SetDragVisualCaptured(TRUE /*dragVisualCaptured*/));
        }
    }

    return S_OK;
}

HRESULT DragDrop::CreateDragOperation(_Outptr_ mui::DragDrop::IDragOperation **ppOperation)
{
    *ppOperation = nullptr;
    IFC_RETURN(wf::ActivateInstance(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Input_DragDrop_DragOperation).Get(),
        ppOperation));
    return S_OK;
}
