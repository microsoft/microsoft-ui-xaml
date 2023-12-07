// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CAutomationPeerAnnotation.g.h"
#include "AutomationPeerAnnotationCollection.h"
#include <windows.applicationmodel.core.h>
#include "APLock.h"
#include <XamlOneCoreTransforms.h>
#include "FrameworkElementAutomationPeer.h"
#include "CPopupAutomationPeer.g.h"
#include "Popup.h"
#include "RootScale.h"
#include <DependencyObject.h>

#pragma warning(disable:4996) // use of apis marked as [[deprecated("PrivateAPI")]]

// Constructor.
CUIAWrapper::CUIAWrapper(
    _In_ const UIAHostEnvironmentInfo& uiaHostEnvironmentInfo,
    _In_ IXcpHostSite *pHost,
    _In_ CUIAWindow *pWindow,
    _In_ CAutomationPeer *pAP,
    _In_ UIAIdentifiers *UIAIds)
    : m_uiaHostEnvironmentInfo(uiaHostEnvironmentInfo)
{
    XCP_WEAK(&m_pHost);
    m_pHost = pHost;
    m_pAP = pAP;
    m_pWindow = pWindow;
    pUIAIds = UIAIds;

    m_pUIAWindowValidator = m_pWindow->GetUIAWindowValidator();
    m_pAP->SetUIAWrapper(this);

    ctl::ComPtr<IAutomationPeerHwndInterop> interopPeer;
    if (m_pAP->GetDXamlPeer())
    {
        HRESULT result = ctl::iinspectable_cast(m_pAP->GetDXamlPeer())->QueryInterface<IAutomationPeerHwndInterop>(&interopPeer);
        if (result == S_OK && interopPeer != nullptr)
        {
            result = interopPeer->GetRawElementProviderSimple(&m_providerSimpleOverrider);
        }
    }
}

// Destructor.
CUIAWrapper::~CUIAWrapper()
{
    Deinit();
}

HRESULT CUIAWrapper::Create(
    _In_ const UIAHostEnvironmentInfo& uiaHostEnvironmentInfo,
    _In_ IXcpHostSite *pHost,
    _In_ CUIAWindow *pWindow,
    _In_ CAutomationPeer *pAP,
    _In_ UIAIdentifiers *UIAIds,
    _Outptr_ CUIAWrapper** ppUIAWrapper)
{
    HRESULT hr = S_OK;

    *ppUIAWrapper = static_cast<CUIAWrapper*>(pAP->GetUIAWrapper());
    if (!(*ppUIAWrapper))
    {
        *ppUIAWrapper = new CUIAWrapper(uiaHostEnvironmentInfo, pHost, pWindow, pAP, UIAIds);
    }
    else
    {
        (*ppUIAWrapper)->AddRef();
    }

    return hr;//RRETURN_REMOVAL
}

HRESULT CUIAWrapper::Deinit()
{
    m_pHost = nullptr;
    m_uiaHostEnvironmentInfo = {};
    if (m_pAP)
    {
        m_pAP->InvalidateUIAWrapper();
    }

    m_providerSimpleOverrider = nullptr;

    // Releasing Window Validator
    ReleaseInterface(m_pUIAWindowValidator);
    return S_OK;
}

void CUIAWrapper::Invalidate()
{
    m_pAP = nullptr;

    // Disconnecting this provider from UIAutomationCore to protect this leaking.
    IGNOREHR(UiaDisconnectProvider(this));
}

// IUnknown implementation.

ULONG STDMETHODCALLTYPE CUIAWrapper::AddRef()
{
    return CInterlockedReferenceCount::AddRef();
}

ULONG STDMETHODCALLTYPE CUIAWrapper::Release()
{
    return CInterlockedReferenceCount::Release();
}

HRESULT STDMETHODCALLTYPE CUIAWrapper::QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface)
{
    // Resolving IUnknown with IREPS2 as that's what is at top of the vTable.
    if (riid == __uuidof(IUnknown))
    {
        *ppInterface = static_cast<IUnknown*>(static_cast<IRawElementProviderSimple2*>(this));
    }
    else if (riid == __uuidof(IRawElementProviderSimple))
    {
        *ppInterface = static_cast<IRawElementProviderSimple*>(this);
    }
    else if (riid == __uuidof(IRawElementProviderSimple2))
    {
        *ppInterface = static_cast<IRawElementProviderSimple2*>(this);
    }
    else if (riid == __uuidof(IRawElementProviderFragment))
    {
        *ppInterface = static_cast<IRawElementProviderFragment*>(this);
    }
    else if (riid == __uuidof(IRawElementProviderAdviseEvents))
    {
        *ppInterface = static_cast<IRawElementProviderAdviseEvents*>(this);
    }
    else if (riid == __uuidof(IRawElementProviderVisualRelative) && XamlOneCoreTransforms::IsEnabled())
    {
        *ppInterface = static_cast<IRawElementProviderVisualRelative*>(this);
    }
    else if (riid == __uuidof(CUIAWrapper))
    {
        *ppInterface = static_cast<CUIAWrapper*>(this);
    }
    else
    {
        *ppInterface = nullptr;
        return E_NOINTERFACE;
    }

    static_cast<IUnknown*>(*ppInterface)->AddRef();
    return S_OK;
}


// IRawElementProviderSimple implementation

// Get Provider options.

HRESULT STDMETHODCALLTYPE CUIAWrapper::get_ProviderOptions( _Out_ ProviderOptions* pRetVal )
{
    HRESULT hr = S_OK;

    if (m_providerSimpleOverrider)
    {
        return m_providerSimpleOverrider->get_ProviderOptions(pRetVal);
    }

    APLock apLock(m_pAP);

    if (!m_pAP)
    {
        IFC_NOTRACE(E_FAIL);
    }

    if (pRetVal == nullptr) return E_INVALIDARG;

    *pRetVal = ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading;

Cleanup:
    return S_OK;
}

// Get the object that supports IInvokePattern.

HRESULT STDMETHODCALLTYPE CUIAWrapper::GetPatternProvider(_In_ PATTERNID patternId, _Out_ IUnknown** pRetVal)
{
    if (!m_pUIAWindowValidator || !m_pUIAWindowValidator->IsValid())
    {
        IFC_NOTRACE_RETURN(E_FAIL);
    }

    IFC_NOTRACE_RETURN(GetPatternProviderImpl(patternId, pRetVal));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CUIAWrapper::GetPatternProviderImpl(_In_ PATTERNID patternId, _Out_ IUnknown** ppRetVal)
{
    *ppRetVal = nullptr;
    if (m_providerSimpleOverrider)
    {
        return m_providerSimpleOverrider->GetPatternProvider(patternId, ppRetVal);
    }

    UIAXcp::APPatternInterface pattern;
    wrl::ComPtr<IUIAProvider> obj;
    wrl::ComPtr<IUnknown> wrapper;
    APLock apLock(m_pAP);

    if (!m_pAP)
    {
        IFC_NOTRACE_RETURN(E_FAIL);
    }

    if (!m_pUIAWindowValidator || !m_pUIAWindowValidator->IsValid())
    {
        IFC_NOTRACE_RETURN(E_FAIL);
    }

    if (patternId == pUIAIds->Invoke_Pattern)
    {
        pattern = UIAXcp::PIInvoke;
    }
    else if (patternId == pUIAIds->Dock_Pattern)
    {
        pattern = UIAXcp::PIDock;
    }
    else if (patternId == pUIAIds->ExpandCollapse_Pattern)
    {
        pattern = UIAXcp::PIExpandCollapse;
    }
    else if (patternId == pUIAIds->GridItem_Pattern)
    {
        pattern = UIAXcp::PIGridItem;
    }
    else if (patternId == pUIAIds->Grid_Pattern)
    {
        pattern = UIAXcp::PIGrid;
    }
    else if (patternId == pUIAIds->MultipleView_Pattern)
    {
        pattern = UIAXcp::PIMultipleView;
    }
    else if (patternId == pUIAIds->RangeValue_Pattern)
    {
        pattern = UIAXcp::PIRangeValue;
    }
    else if (patternId == pUIAIds->ScrollItem_Pattern)
    {
        pattern = UIAXcp::PIScrollItem;
    }
    else if (patternId == pUIAIds->Scroll_Pattern)
    {
        pattern = UIAXcp::PIScroll;
    }
    else if (patternId == pUIAIds->SelectionItem_Pattern)
    {
        pattern = UIAXcp::PISelectionItem;
    }
    else if (patternId == pUIAIds->Selection_Pattern)
    {
        pattern = UIAXcp::PISelection;
    }
    else if (patternId == pUIAIds->TableItem_Pattern)
    {
        pattern = UIAXcp::PITableItem;
    }
    else if (patternId == pUIAIds->Table_Pattern)
    {
        pattern = UIAXcp::PITable;
    }
    else if (patternId == pUIAIds->Toggle_Pattern)
    {
        pattern = UIAXcp::PIToggle;
    }
    else if (patternId == pUIAIds->Transform_Pattern)
    {
        pattern = UIAXcp::PITransform;
    }
    else if (patternId == pUIAIds->Value_Pattern)
    {
        pattern = UIAXcp::PIValue;
    }
    else if (patternId == pUIAIds->Window_Pattern)
    {
        pattern = UIAXcp::PIWindow;
    }
    else if (patternId == pUIAIds->Text_Pattern)
    {
        pattern = UIAXcp::PIText;
    }
    else if (patternId == pUIAIds->ItemContainer_Pattern)
    {
        pattern = UIAXcp::PIItemContainer;
    }
    else if (patternId == pUIAIds->VirtualizedItem_Pattern)
    {
        pattern = UIAXcp::PIVirtualizedItem;
    }
    else if (patternId == pUIAIds->Text_Pattern2)
    {
        pattern = UIAXcp::PIText2;
    }
    else if (patternId == pUIAIds->TextChild_Pattern)
    {
        pattern = UIAXcp::PITextChild;
    }
    else if (patternId == pUIAIds->Annotation_Pattern)
    {
        pattern = UIAXcp::PIAnnotation;
    }
    else if (patternId == pUIAIds->Drag_Pattern)
    {
        pattern = UIAXcp::PIDrag;
    }
    else if (patternId == pUIAIds->DropTarget_Pattern)
    {
        pattern = UIAXcp::PIDropTarget;
    }
    else if (patternId == pUIAIds->ObjectModel_Pattern)
    {
        pattern = UIAXcp::PIObjectModel;
    }
    else if (patternId == pUIAIds->Spreadsheet_Pattern)
    {
        pattern = UIAXcp::PISpreadsheet;
    }
    else if (patternId == pUIAIds->SpreadsheetItem_Pattern)
    {
        pattern = UIAXcp::PISpreadsheetItem;
    }
    else if (patternId == pUIAIds->Styles_Pattern)
    {
        pattern = UIAXcp::PIStyles;
    }
    else if (patternId == pUIAIds->Transform_Pattern2)
    {
        pattern = UIAXcp::PITransform2;
    }
    else if (patternId == pUIAIds->SynchronizedInput_Pattern)
    {
        pattern = UIAXcp::PISynchronizedInput;
    }
    else if (patternId == pUIAIds->TextEdit_Pattern)
    {
        pattern = UIAXcp::PITextEdit;
    }
    else if (patternId == pUIAIds->CustomNavigation_Pattern)
    {
        pattern = UIAXcp::PICustomNavigation;
    }
    else
    {
        *ppRetVal = nullptr;
        return S_OK;
    }

        IFCPTR_RETURN(m_pAP);
        obj = m_pAP->GetPattern(pattern);

    if (obj)
    {
        switch (pattern)
        {
        case UIAXcp::PIInvoke:
            IFC_RETURN(CUIAInvokeProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PIDock:
            IFC_RETURN(CUIADockProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PIExpandCollapse:
            IFC_RETURN(CUIAExpandCollapseProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PIGridItem:
            IFC_RETURN(CUIAGridItemProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PIGrid:
            IFC_RETURN(CUIAGridProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PIMultipleView:
            IFC_RETURN(CUIAMultipleViewProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PIRangeValue:
            IFC_RETURN(CUIARangeValueProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PIScrollItem:
            IFC_RETURN(CUIAScrollItemProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PIScroll:
            IFC_RETURN(CUIAScrollProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PISelectionItem:
            IFC_RETURN(CUIASelectionItemProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PISelection:
            IFC_RETURN(CUIASelectionProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PITableItem:
            IFC_RETURN(CUIATableItemProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PITable:
            IFC_RETURN(CUIATableProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PIToggle:
            IFC_RETURN(CUIAToggleProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PITransform:
            IFC_RETURN(CUIATransformProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PIValue:
            IFC_RETURN(CUIAValueProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PIWindow:
            IFC_RETURN(CUIAWindowProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PIText:
            IFC_RETURN(CUIATextProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PIItemContainer:
            IFC_RETURN(CUIAItemContainerProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PIVirtualizedItem:
            IFC_RETURN(CUIAVirtualizedItemProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PIText2:
            IFC_RETURN(CUIATextProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PITextChild:
            IFC_RETURN(CUIATextChildProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PIAnnotation:
            IFC_RETURN(CUIAAnnotationProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PIDrag:
            IFC_RETURN(CUIADragProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PIDropTarget:
            IFC_RETURN(CUIADropTargetProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PIObjectModel:
            IFC_RETURN(CUIAObjectModelProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PISpreadsheet:
            IFC_RETURN(CUIASpreadsheetProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PISpreadsheetItem:
            IFC_RETURN(CUIASpreadsheetItemProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PIStyles:
            IFC_RETURN(CUIAStylesProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PITransform2:
            IFC_RETURN(CUIATransformProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PISynchronizedInput:
            IFC_RETURN(CUIASynchronizedInputProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PITextEdit:
            IFC_RETURN(CUIATextEditProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        case UIAXcp::PICustomNavigation:
            IFC_RETURN(CUIACustomNavigationProviderWrapper::Create(obj.Get(), m_pWindow, pUIAIds, &wrapper));
            break;
        default:
            *ppRetVal = nullptr;
            return S_OK;
        }
        *ppRetVal = wrapper.Detach();
    }
    else
    {
        wrapper = static_cast<IUnknown*>(m_pAP->GetUnwrappedPattern(patternId));
        *ppRetVal = wrapper.Detach();
    }

    return S_OK;
}

// Gets custom properties.

HRESULT STDMETHODCALLTYPE CUIAWrapper::GetPropertyValue(_In_ PROPERTYID propertyId, _Out_ VARIANT* pRetVal)
{
    if (!m_pUIAWindowValidator || !m_pUIAWindowValidator->IsValid())
    {
        IFC_NOTRACE_RETURN(E_FAIL);
    }

    IFC_NOTRACE_RETURN(GetPropertyValueImpl(propertyId, pRetVal))

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CUIAWrapper::GetPropertyValueImpl(_In_ PROPERTYID propertyId, _Out_ VARIANT* pRetVal)
{
    if (pRetVal == nullptr) return E_INVALIDARG;

    bool isInteropPeer = false;
    if (m_providerSimpleOverrider)
    {
        // We expect the overriding provider to return a value for a given property.
        // However, if the overriding provider does not know what to return for the property (E_NOTIMPL),
        // fall back on the behavior for InteropPeers where AutomationProperties are queried.
        HRESULT overriderResult = m_providerSimpleOverrider->GetPropertyValue(propertyId, pRetVal);
        if (overriderResult == E_NOTIMPL)
        {
            isInteropPeer = true;
        }
        else
        {
            return overriderResult;
        }
    }

    HRESULT hr = S_OK;
    XINT32 iResult = FALSE;
    UIAXcp::APAutomationControlType apControlType;
    UIAXcp::OrientationType apOrientationType;
    UIAXcp::LiveSetting apLiveSetting;
    UIAXcp::AutomationLandmarkType apLandmarkType;
    UIAXcp::AutomationHeadingLevel apHeadingLevel;
    APLock apLock(m_pAP);
    CAutomationPeerCollection *pApCollection = nullptr;

    if (!m_pAP)
    {
        return E_FAIL;
    }

    xstring_ptr strString;
    CAutomationPeer *pAP = nullptr;

    XPOINTF clickablePoint = {};

    if (propertyId == pUIAIds->ClassName_Property)
    {
        IFC(m_pAP->GetClassName(&strString));
        ConvertToVariant(strString, pRetVal);
    }
    else if (propertyId == pUIAIds->IsOffscreen_Property)
    {
        IFC(m_pAP->IsOffscreen(&iResult));
        ConvertToVariant(iResult!=0, pRetVal);
    }
    else if (propertyId == pUIAIds->AutomationId_Property)
    {
        IFC(m_pAP->GetAutomationId(&strString));
        ConvertToVariant(strString, pRetVal);
    }
    else if (propertyId == pUIAIds->ControlledPeers_Property)
    {
        // We provide ControlledPeers(ControllerFor) property from the Xaml Provider
        // even if it is an override provider for trident in case of WebView.
        // This is essentially because Xaml apps, if they set Controllerfor for this
        // scenario they do it on WebView control itself, not on the trident content.
        // Cortana is one example of such case.
        IFC(m_pAP->GetControlledPeers(&pApCollection));
        ConvertToVariant(pApCollection, pRetVal);
    }
    else if (propertyId == pUIAIds->ClickablePoint_Property)
    {
        IFC(m_pAP->GetClickablePoint(&clickablePoint));

        if (clickablePoint.x != 0 || clickablePoint.y != 0)
        {
            POINT ptScreen;

            ptScreen.x = static_cast<LONG>(clickablePoint.x);
            ptScreen.y = static_cast<LONG>(clickablePoint.y);

            // Convert the point from client to screen coordinate
            VERIFY(m_pWindow->TransformClientToScreen(m_uiaHostEnvironmentInfo, &ptScreen));

            clickablePoint.x = static_cast<XFLOAT>(ptScreen.x);
            clickablePoint.y = static_cast<XFLOAT>(ptScreen.y);

            ConvertToVariant(&clickablePoint, pRetVal);
        }
        else
        {
            pRetVal->vt = VT_EMPTY;
        }
    }
    else if (propertyId == pUIAIds->IsControlElement_Property)
    {
        IFC(m_pAP->IsControlElement(&iResult));
        ConvertToVariant(iResult!=0, pRetVal);
    }
    else if (propertyId == pUIAIds->IsContentElement_Property)
    {
        IFC(m_pAP->IsContentElement(&iResult));
        ConvertToVariant(iResult!=0, pRetVal);
    }
    else if (!isInteropPeer)
    {
        if (propertyId == pUIAIds->ControlType_Property)
        {
            IFC(m_pAP->GetAutomationControlType(&apControlType));
            ConvertToVariant(apControlType, pRetVal);
        }
        else if (propertyId == pUIAIds->LabeledBy_Property)
        {
            IFC(m_pAP->GetLabeledBy(&pAP));
            ConvertToVariant(pAP, pRetVal);
        }
        else if (propertyId == pUIAIds->ItemType_Property)
        {
            IFC(m_pAP->GetItemType(&strString));
            ConvertToVariant(strString, pRetVal);
        }
        else if (propertyId == pUIAIds->IsPassword_Property)
        {
            IFC(m_pAP->IsPassword(&iResult));
            ConvertToVariant(iResult!=0, pRetVal);
        }
        else if (propertyId == pUIAIds->LocalizedControlType_Property)
        {
            IFC(m_pAP->GetLocalizedControlType(&strString));
            ConvertToVariant(strString, pRetVal);
        }
        else if (propertyId == pUIAIds->Name_Property)
        {
            IFC(m_pAP->GetName(&strString));
            ConvertToVariant(strString, pRetVal);
        }
        else if (propertyId == pUIAIds->AcceleratorKey_Property)
        {
            IFC(m_pAP->GetAcceleratorKey(&strString));
            ConvertToVariant(strString, pRetVal);
        }
        else if (propertyId == pUIAIds->AccessKey_Property)
        {
            IFC(m_pAP->GetAccessKey(&strString));
            ConvertToVariant(strString, pRetVal);
        }
        else if (propertyId == pUIAIds->HasKeyboardFocus_Property)
        {
            IFC(m_pAP->HasKeyboardFocus(&iResult));
            ConvertToVariant(iResult!=0, pRetVal);
        }
        else if (propertyId == pUIAIds->IsKeyboardFocusable_Property)
        {
            IFC(m_pAP->IsKeyboardFocusable(&iResult));
            ConvertToVariant(iResult!=0, pRetVal);
        }
        else if (propertyId == pUIAIds->IsEnabled_Property)
        {
            IFC(m_pAP->IsEnabled(&iResult));
            ConvertToVariant(iResult!=0, pRetVal);
        }
        else if (propertyId == pUIAIds->HelpText_Property)
        {
            IFC(m_pAP->GetHelpText(&strString));
            ConvertToVariant(strString, pRetVal);
        }
        else if (propertyId == pUIAIds->Orientation_Property )
        {
            IFC(m_pAP->GetOrientation(&apOrientationType));
            ConvertToVariant(apOrientationType, pRetVal);
        }
        else if (propertyId == pUIAIds->IsRequiredForForm_Property)
        {
            IFC(m_pAP->IsRequiredForForm(&iResult));
            ConvertToVariant(iResult!=0, pRetVal);
        }
        else if (propertyId == pUIAIds->ItemStatus_Property)
        {
            IFC(m_pAP->GetItemStatus(&strString));
            ConvertToVariant(strString, pRetVal);
        }
        else if (propertyId == pUIAIds->LiveSetting_Property)
        {
            IFC(m_pAP->GetLiveSetting(&apLiveSetting));
            ConvertToVariant(apLiveSetting, pRetVal);
        }
        else if (propertyId == pUIAIds->RuntimeId_Property)
        {
            XUINT32 uRuntimeId = m_pAP->GetRuntimeId();
            ConvertToVariant(uRuntimeId, pRetVal);
        }
        else if (propertyId == pUIAIds->FrameworkId_Property)
        {
            // Don't localize this framework id name.
            ConvertToVariant(XSTRING_PTR_EPHEMERAL(L"XAML"), pRetVal);
        }
        else if (propertyId == pUIAIds->PreventKeyboardDisplayOnProgrammaticFocus_Property)
        {
            iResult = m_pAP->GetPreventKeyboardDisplayOnProgrammaticFocus();
            ConvertToVariant(iResult!=0, pRetVal);
        }
        else if (propertyId == pUIAIds->FlowsFrom_Property)
        {
            IFC(m_pAP->GetFlowsFrom(&pApCollection));
            ConvertToVariant(pApCollection, pRetVal);
        }
        else if (propertyId == pUIAIds->FlowsTo_Property)
        {
            IFC(m_pAP->GetFlowsTo(&pApCollection));
            ConvertToVariant(pApCollection, pRetVal);
        }
        else if (propertyId == pUIAIds->PositionInSet_Property)
        {
            IFC(m_pAP->GetPositionInSet(&iResult));
            ConvertToVariant(iResult, pRetVal);
        }
        else if (propertyId == pUIAIds->SizeOfSet_Property)
        {
            IFC(m_pAP->GetSizeOfSet(&iResult));
            ConvertToVariant(iResult, pRetVal);
        }
        else if (propertyId == pUIAIds->Level_Property)
        {
            IFC(m_pAP->GetLevel(&iResult));
            ConvertToVariant(iResult, pRetVal);
        }
        else if (propertyId == pUIAIds->AnnotationTypes_Property || propertyId == pUIAIds->AnnotationObjects_Property)
        {
            xref_ptr<CAutomationPeerAnnotationCollection> spAnnotationCollection;
            IFC(m_pAP->GetAnnotations(spAnnotationCollection.ReleaseAndGetAddressOf()));
            ConvertToVariant(spAnnotationCollection.get(), propertyId, pRetVal);
        }
        else if (propertyId == pUIAIds->LandmarkType_Property)
        {
            IFC(m_pAP->GetLandmarkType(&apLandmarkType));
            if (apLandmarkType == UIAXcp::AutomationLandmarkType_None)
            {
                // If no LandmarkType is provided, but a LocalizedLandmarkType is provided,
                // auto-set LandmarkType to Custom
                IFC(m_pAP->GetLocalizedLandmarkType(&strString));
                if (!strString.IsNullOrEmpty())
                {
                    apLandmarkType = UIAXcp::AutomationLandmarkType_Custom;
                }
            }
            ConvertToVariant(apLandmarkType, pRetVal);
        }
        else if (propertyId == pUIAIds->LocalizedLandmarkType_Property)
        {
            IFC(m_pAP->GetLocalizedLandmarkType(&strString));
            if (!strString.IsNullOrEmpty())
            {
                ConvertToVariant(strString, pRetVal);
            }
        }
        else if (propertyId == pUIAIds->IsPeripheral_Property)
        {
            IFC(m_pAP->IsPeripheral(&iResult));
            ConvertToVariant(iResult != 0, pRetVal);
        }
        else if (propertyId == pUIAIds->IsDataValidForForm_Property)
        {
            IFC(m_pAP->IsDataValidForForm(&iResult));
            ConvertToVariant(iResult != 0, pRetVal);
        }
        else if (propertyId == pUIAIds->FullDescription_Property)
        {
            IFC(m_pAP->GetFullDescription(&strString));
            ConvertToVariant(strString, pRetVal);
        }
        else if (propertyId == pUIAIds->DescribedBy_Property)
        {
            IFC(m_pAP->GetDescribedBy(&pApCollection));
            ConvertToVariant(pApCollection, pRetVal);
        }
        else if (propertyId == pUIAIds->Culture_Property)
        {
            IFC(m_pAP->GetCulture(&iResult));
            ConvertToVariant(iResult, pRetVal);
        }
        else if (propertyId == pUIAIds->HeadingLevel_Property)
        {
            IFC(m_pAP->GetHeadingLevel(&apHeadingLevel));
            ConvertToVariant(apHeadingLevel, pRetVal);
        }
        else if (propertyId == pUIAIds->IsDialog_Property)
        {
            IFC(m_pAP->IsDialog(&iResult));
            ConvertToVariant((iResult != 0), pRetVal);
        }
        else
        {
            pRetVal->vt = VT_EMPTY;
            // UI Automation will attempt to get the property from the host window.
        }
    }
    else // isInteropPeer
    {
        //UIA first asks the Interop Peer (for e.g. WebViewAutomationPeer) for property values,
        //If the value returned was Empty, it proceeds to ask the Main Provider (in case of WebView, Edge's Provider)
        //Which leads to several non-ideal scenarios for accessibility (see MSFT: 5704176), we let the developer control
        //the behavior by specifying the values via AutomationProperties.
        pRetVal->vt = VT_EMPTY;
        if (propertyId == pUIAIds->Name_Property)
        {
            IFC(m_pAP->GetName(&strString));
            ConvertToVariant(strString, pRetVal);
        }
    }

Cleanup:
    ReleaseInterface(pApCollection);

    RRETURN(hr);
}

// Gets the UI Automation CUIAWrapper for the host window. This CUIAWrapper supplies most properties.

HRESULT STDMETHODCALLTYPE CUIAWrapper::get_HostRawElementProvider(_Outptr_ IRawElementProviderSimple** pRetVal)
{
    if (m_providerSimpleOverrider)
    {
        return m_providerSimpleOverrider->get_HostRawElementProvider(pRetVal);
    }

    if (pRetVal == nullptr) return E_INVALIDARG;
    *pRetVal = nullptr;

    return S_OK;
}

// Sets the focus to this element
HRESULT STDMETHODCALLTYPE CUIAWrapper::ShowContextMenu()
{
    if (!m_pUIAWindowValidator || !m_pUIAWindowValidator->IsValid())
    {
        IFC_NOTRACE_RETURN(E_FAIL);
    }

    IFC_NOTRACE_RETURN(ShowContextMenuImpl());

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CUIAWrapper::ShowContextMenuImpl()
{
    APLock apLock(m_pAP);

    if (!m_pAP)
    {
        return E_FAIL;
    }

    IGNOREHR(m_pAP->ShowContextMenu());
    return S_OK;
}

// IRawElementProviderFragment implementation

// Gets the bounding rectangle of this element

HRESULT STDMETHODCALLTYPE CUIAWrapper::get_BoundingRectangle(_Out_ UiaRect * pRetVal)
{
    if (!m_pUIAWindowValidator || !m_pUIAWindowValidator->IsValid())
    {
        IFC_NOTRACE_RETURN(E_FAIL);
    }

    IFC_NOTRACE_RETURN(get_BoundingRectangleImpl(pRetVal));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CUIAWrapper::get_BoundingRectangleImpl(_Out_ UiaRect * pRetVal)
{
    HRESULT hr = S_OK;

    XRECTF rect;
    APLock apLock(m_pAP);

    if (pRetVal == nullptr) return E_INVALIDARG;

    if (!m_pAP)
    {
        return E_FAIL;
    }

    IFC(m_pAP->GetBoundingRectangle(&rect));

    // Convert client to screen coordinate position
    VERIFY(m_pWindow->TransformClientToScreen(m_uiaHostEnvironmentInfo, &rect));

    pRetVal->left = rect.X;
    pRetVal->top = rect.Y;
    pRetVal->width = rect.Width;
    pRetVal->height = rect.Height;

Cleanup:
    UIA_TRACE(L"CUIAWrapper::get_BoundingRectangleImpl return %x", hr);

    RRETURN(hr);
}

// Gets the root node of the fragment

HRESULT STDMETHODCALLTYPE CUIAWrapper::get_FragmentRoot(_Out_ IRawElementProviderFragmentRoot** pRetVal)
{
    HRESULT hr = S_OK;

    if (pRetVal == nullptr) return E_INVALIDARG;

    if (!m_pUIAWindowValidator || !m_pUIAWindowValidator->IsValid())
    {
        IFC_NOTRACE(E_FAIL);
    }

    m_pWindow->AddRef();
    *pRetVal = (IRawElementProviderFragmentRoot*)(m_pWindow);

Cleanup:
    if (FAILED(hr))
    {
        UIA_TRACE(L"CUIAWrapper::get_FragmentRoot return %x", hr);
    }
    RRETURN(hr);
}

// Retrieves an array of root fragments that are embedded in the UI Automation tree rooted at the current element
// Returns nullptr if there is no Automation Framework contained within
HRESULT STDMETHODCALLTYPE CUIAWrapper::GetEmbeddedFragmentRoots(_Out_ SAFEARRAY **pRetVal)
{
    *pRetVal = nullptr;

    if (!m_pAP)
    {
        return E_FAIL;
    }

    if (!m_pUIAWindowValidator || !m_pUIAWindowValidator->IsValid())
    {
        return E_FAIL;
    }

    return S_OK;
}

// Retrieves the runtime identifier of an element

HRESULT STDMETHODCALLTYPE CUIAWrapper::GetRuntimeId(_Out_ SAFEARRAY ** pRetVal)
{
    HRESULT hr = S_OK;

    if (!m_pUIAWindowValidator || !m_pUIAWindowValidator->IsValid())
    {
        IFC_NOTRACE(E_FAIL);
    }

    IFC_NOTRACE(GetRuntimeIdImpl(pRetVal));

Cleanup:
    UIA_TRACE(L"CUIAWrapper::GetRuntimeId return %x", hr);
    RRETURN(hr);
}

HRESULT STDMETHODCALLTYPE CUIAWrapper::GetRuntimeIdImpl(_Out_ SAFEARRAY ** pRetVal)
{
    HRESULT hr = S_OK;
    APLock apLock(m_pAP);
    IFCPTR(pRetVal);

    if (!m_pAP)
    {
        return E_FAIL;
    }

    // Interop peer will get the value from the default hwnd provider.
    if (m_providerSimpleOverrider == nullptr)
    {
        XUINT32 uRuntimeId = m_pAP->GetRuntimeId();

        int rId[] = { UiaAppendRuntimeId, uRuntimeId };

        unique_safearray safeArray(SafeArrayCreateVector(VT_I4, 0, 2));
        IFCOOMFAILFAST(safeArray.get());
        for (LONG i = 0; i < 2; i++)
        {
            IFC(SafeArrayPutElement(safeArray.get(), &i, static_cast<void*>(&(rId[i]))));
        }
        *pRetVal = safeArray.release();
    }

Cleanup:
    RRETURN(hr);
}

// Retrieves the UI Automation element in a specified direction within the UI Automation tree
HRESULT STDMETHODCALLTYPE CUIAWrapper::Navigate(_In_ NavigateDirection direction, _Out_ IRawElementProviderFragment ** pRetVal)
{
    UIA_TRACE(L"CUIAWrapper::Navigate %d", direction);

    HRESULT hr = S_OK;

    if (!m_pUIAWindowValidator || !m_pUIAWindowValidator->IsValid())
    {
        IFC_NOTRACE(E_FAIL);
    }

    IFC_NOTRACE(NavigateImpl(direction, pRetVal));

Cleanup:
    UIA_TRACE(L"CUIAWrapper::Navigate return %x", hr);
    RRETURN(hr);
}

HRESULT STDMETHODCALLTYPE CUIAWrapper::NavigateImpl(_In_ NavigateDirection direction, _Outptr_ IRawElementProviderFragment ** pRetVal)
{
    if (pRetVal == nullptr)
    {
        return E_INVALIDARG;
    }
    *pRetVal = nullptr;

    APLock apLock(m_pAP);

    if (!m_pAP)
    {
        return E_FAIL;
    }

    if (!m_pUIAWindowValidator || !m_pUIAWindowValidator->IsValid())
    {
        return E_FAIL;
    }

    xref_ptr<IRawElementProviderFragment> spFrag;
    xref_ptr<IUnknown> spUnkNativeNode;
    xref_ptr<CAutomationPeer> spAP;
    xref_ptr<IRawElementProviderSimple> spWindowLessREPS;

    switch(direction)
    {
        case NavigateDirection_FirstChild:
            if (m_pAP->IsHostedWindowlessElementProvider())
            {
                spWindowLessREPS = reinterpret_cast<IRawElementProviderSimple*>(m_pAP->GetWindowlessRawElementProviderSimple());
                IFCPTR_RETURN(spWindowLessREPS);
                IFC_RETURN(spWindowLessREPS->QueryInterface(__uuidof(IRawElementProviderFragment), reinterpret_cast<void**>(spFrag.ReleaseAndGetAddressOf())));
            }
            else
            {
                IFC_RETURN(m_pAP->Navigate(UIAXcp::AutomationNavigationDirection_FirstChild, spAP.ReleaseAndGetAddressOf(), spUnkNativeNode.ReleaseAndGetAddressOf()));
            }
            break;
        case NavigateDirection_LastChild:
            IFC_RETURN(m_pAP->Navigate(UIAXcp::AutomationNavigationDirection_LastChild, spAP.ReleaseAndGetAddressOf(), spUnkNativeNode.ReleaseAndGetAddressOf()));

            if (spAP == nullptr && spUnkNativeNode == nullptr)
            {
                if (m_pAP->IsHostedWindowlessElementProvider())
                {
                    spWindowLessREPS = reinterpret_cast<IRawElementProviderSimple*>(m_pAP->GetWindowlessRawElementProviderSimple());
                    IFCPTR_RETURN(spWindowLessREPS);
                    IFC_RETURN(spWindowLessREPS->QueryInterface(__uuidof(IRawElementProviderFragment), reinterpret_cast<void**>(spFrag.ReleaseAndGetAddressOf())));
                }
            }
            break;
        case NavigateDirection_NextSibling:
            IFC_RETURN(m_pAP->Navigate(UIAXcp::AutomationNavigationDirection_NextSibling, spAP.ReleaseAndGetAddressOf(), spUnkNativeNode.ReleaseAndGetAddressOf()));
            break;
        case NavigateDirection_PreviousSibling:
            IFC_RETURN(m_pAP->Navigate(UIAXcp::AutomationNavigationDirection_PreviousSibling, spAP.ReleaseAndGetAddressOf(), spUnkNativeNode.ReleaseAndGetAddressOf()));

            if (spAP == nullptr && spUnkNativeNode == nullptr)
            {
                xref_ptr<IUnknown> spUnkParentNativeNode;
                xref_ptr<CAutomationPeer> spAPParent;
                IFC_RETURN(m_pAP->Navigate(UIAXcp::AutomationNavigationDirection_Parent, spAPParent.ReleaseAndGetAddressOf(), spUnkParentNativeNode.ReleaseAndGetAddressOf()));

                if (spAPParent)
                {
                    if (spAPParent->IsHostedWindowlessElementProvider())
                    {
                        spWindowLessREPS = reinterpret_cast<IRawElementProviderSimple*>(spAPParent->GetWindowlessRawElementProviderSimple());
                        IFCPTR_RETURN(spWindowLessREPS);
                        IFC_RETURN(spWindowLessREPS->QueryInterface(__uuidof(IRawElementProviderFragment), reinterpret_cast<void**>(spFrag.ReleaseAndGetAddressOf())));
                    }
                }
            }
            break;
        case NavigateDirection_Parent:
            IFC_RETURN(m_pAP->Navigate(UIAXcp::AutomationNavigationDirection_Parent, spAP.ReleaseAndGetAddressOf(), spUnkNativeNode.ReleaseAndGetAddressOf()));
            break;
    }

    if (spUnkNativeNode)
    {
        IFC_RETURN(spUnkNativeNode->QueryInterface(__uuidof(IRawElementProviderFragment), reinterpret_cast<void**>(spFrag.ReleaseAndGetAddressOf())));
    }
    else
    {
        if (spAP)
        {
            xref_ptr<CUIAWrapper> spWrapper;
            xref_ptr<CAutomationPeer> spAPEventsSource;
            spAPEventsSource = spAP->GetAPEventsSource();
            if (spAPEventsSource)
            {
                spAP = spAPEventsSource;
            }

            // Normally we create the new CUIAWrapper under the same UIA window as this CUIAWrapper, but that's not always correct.
            // Windowed popups create their own CUIAHostWindow, and they should be rooted to that window. This CUIAWindow corresponds
            // to the main Xaml window.
            CUIAWindow* windowForNewWrapper = m_pWindow;
            if (spAP->OfTypeByIndex<KnownTypeIndex::PopupAutomationPeer>())
            {
                CPopupAutomationPeer* popupPeer = static_cast<CPopupAutomationPeer*>(spAP.get());
                CPopup* popup = static_cast<CPopup*>(popupPeer->GetDONoRef());
                // The UIA peer can outlive the element itself
                if (popup != nullptr)
                {
                    popup->EnsureUIAWindow();
                    if (popup->GetUIAWindow() != nullptr)
                    {
                        windowForNewWrapper = popup->GetUIAWindow();
                    }
                }
            }

            IFC_RETURN(windowForNewWrapper->CreateProviderForAP(spAP, spWrapper.ReleaseAndGetAddressOf()));
            IFC_RETURN(spWrapper->QueryInterface(__uuidof(IRawElementProviderFragment), reinterpret_cast<void**>(spFrag.ReleaseAndGetAddressOf())));
        }
        else
        {
            // the only reason that there is no parent is that we're at the root, in which case, we have to return the window/plugin
            if (direction == NavigateDirection_Parent)
            {
                IFC_RETURN(m_pWindow->QueryInterface(__uuidof(IRawElementProviderFragment), reinterpret_cast<void**>(spFrag.ReleaseAndGetAddressOf())));
            }
        }
    }

    *pRetVal = spFrag.detach();

    return S_OK;
}

// Sets the focus to this element
HRESULT STDMETHODCALLTYPE CUIAWrapper::SetFocus()
{
    HRESULT hr = S_OK;

    if (!m_pUIAWindowValidator || !m_pUIAWindowValidator->IsValid())
    {
        IFC_NOTRACE(E_FAIL);
    }

    IFC_NOTRACE(SetFocusImpl());

Cleanup:
    UIA_TRACE(L"CUIAWrapper::SetFocus return %x", hr);
    RRETURN(hr);
}

HRESULT STDMETHODCALLTYPE CUIAWrapper::SetFocusImpl()
{
    APLock apLock(m_pAP);

    if (!m_pAP)
    {
        return E_FAIL;
    }

    m_pAP->SetFocus();
    return S_OK;
}

_Check_return_ HRESULT CUIAWrapper::GetVisualRelativeBoundingRectangle(_Out_ UiaVisualRelativeRectangle* visualRelativeRect)
{
    if (!m_pUIAWindowValidator || !m_pUIAWindowValidator->IsValid())
    {
        IFC_NOTRACE_RETURN(E_FAIL);
    }

    ASSERT(XamlOneCoreTransforms::IsEnabled());

    APLock apLock(m_pAP);

    if (m_pAP == nullptr)
    {
        return E_FAIL;
    }

    // In OneCoreTransforms mode we have to deal with a complication regarding coordinate spaces:
    // AutomationPeer::GetBoundingRectangle can call out to public overrides via GetBoundingRectangleCore.
    // These public overrides (outside XAML) currently hand XAML back a rect in the RawClient coordinate space.
    // Post RS5, these peers will need to change to return RasterizedClient if they plan on using these peers
    // within an embedded CUI component, where RawClient != RasterizedClient.  For now these peers can only
    // be used correctly within a top-level window where RawClient == RasterizedClient.
    // AutomationPeers within XAML will return RasterizedClient, in accordance with this new contract.
    // Hence the conversion process looks like this:
    // 1) Call into the AutomationPeer's implementation of GetBoundingRectangle.  These are expected to return RasterizedClient.
    // 2) After retrieving this rect, convert from RasterizedClient back to Logical (same as Visual Relative)

    XRECTF rect;
    IFC_RETURN(m_pAP->GetBoundingRectangle(&rect));

    const auto physicalRect = rect;
    const auto scale = RootScale::GetRasterizationScaleForElement(m_pAP->GetRootNoRef());
    const auto logicalRect = physicalRect / scale;
    rect = logicalRect;

    visualRelativeRect->Rect.left = rect.X;
    visualRelativeRect->Rect.top = rect.Y;
    visualRelativeRect->Rect.width = rect.Width;
    visualRelativeRect->Rect.height = rect.Height;

// CONTENT-TODO: Lifted IXP doesn't support OneCoreTransforms UIA yet.
#if false
    visualRelativeRect->VisualReferenceId.Value = m_pWindow->GetVisualIdentifier();
#endif

    return S_OK;
}

_Check_return_ HRESULT CUIAWrapper::GetVisualRelativeCenterPoint(_Out_ UiaVisualRelativePoint* visualRelativePoint)
{
    UiaVisualRelativeRectangle boundingRect;
    IFC_RETURN(GetVisualRelativeBoundingRectangle(&boundingRect));

    visualRelativePoint->Point.x = boundingRect.Rect.left + boundingRect.Rect.width / 2;
    visualRelativePoint->Point.y = boundingRect.Rect.top + boundingRect.Rect.height / 2;
    visualRelativePoint->VisualReferenceId = boundingRect.VisualReferenceId;

    return S_OK;
}

_Check_return_ HRESULT CUIAWrapper::GetVisualRelativeClickablePoint(_Out_ UiaVisualRelativePoint* visualRelativePoint)
{
    if (!m_pUIAWindowValidator || !m_pUIAWindowValidator->IsValid())
    {
        IFC_NOTRACE_RETURN(E_FAIL);
    }

    ASSERT(XamlOneCoreTransforms::IsEnabled());

    APLock apLock(m_pAP);

    if (m_pAP == nullptr)
    {
        return E_FAIL;
    }

    // In OneCoreTransforms mode we have to deal with a complication regarding coordinate spaces:
    // AutomationPeer::GetBoundingRectangle can call out to public overrides via GetClickablePointCore.
    // These public overrides (outside XAML) currently hand XAML back a rect in the RawClient coordinate space.
    // Post RS5, these peers will need to change to return RasterizedClient if they plan on using these peers
    // within an embedded CUI component, where RawClient != RasterizedClient.  For now these peers can only
    // be used correctly within a top-level window where RawClient == RasterizedClient.
    // AutomationPeers within XAML will return RasterizedClient, in accordance with this new contract.
    // Hence the conversion process looks like this:
    // 1) Call into the AutomationPeer's implementation of GetClickablePoint.  These are expected to return RasterizedClient.
    // 2) After retrieving this rect, convert from RasterizedClient back to Logical (same as Visual Relative)

    const float scale = RootScale::GetRasterizationScaleForElement(m_pAP->GetRootNoRef());
    XPOINTF point = {};
    IFC_RETURN(m_pAP->GetClickablePoint(&point));
    const auto physicalPoint = point;
    const auto logicalPoint = physicalPoint / scale;
    point = logicalPoint;

    visualRelativePoint->Point.x = point.x;
    visualRelativePoint->Point.y = point.y;

// CONTENT-TODO: Lifted IXP doesn't support OneCoreTransforms UIA yet.
#if false
    visualRelativePoint->VisualReferenceId.Value = m_pAP->GetContext()->GetCoreWindowCompositionIslandId();
#endif

    return S_OK;
}

// IRawElementProviderAdviseEvents implementation

// Notifies the UI Automation provider when a UI Automation client begins listening for a specific event, including a property-changed event

HRESULT STDMETHODCALLTYPE CUIAWrapper::AdviseEventAdded(_In_ EVENTID eventId, _Out_ SAFEARRAY *propertyIDs)
{
    return S_OK;
}

// Notifies the UI Automation provider when a UI Automation client stops listening for a specific event, including a property-changed event
HRESULT STDMETHODCALLTYPE CUIAWrapper::AdviseEventRemoved(_In_ EVENTID eventId, _Out_ SAFEARRAY *propertyIDs)
{
    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   ConvertToVariant
//
//  Synopsis:
//     Helper functions to convert data types to Variants
//
//-------------------------------------------------------------------------
HRESULT CUIAWrapper::ConvertToVariant(_In_ XINT32 iValue, _Out_ VARIANT* pResult)
{
    ASSERT(pResult);

    pResult->vt = VT_I4;
    pResult->lVal = iValue;

    return S_OK;
}

HRESULT CUIAWrapper::ConvertToVariant(_In_ XUINT32 uValue, _Out_ VARIANT* pResult)
{
    ASSERT(pResult);

    pResult->vt = VT_UI4;
    pResult->lVal = uValue;

    return S_OK;
}

HRESULT CUIAWrapper::ConvertToVariant(_In_ const xstring_ptr_view& strString, _Out_ VARIANT* pResult)
{
    ASSERT(pResult);

    if (!strString.IsNullOrEmpty())
    {
        pResult->vt = VT_BSTR;
        pResult->bstrVal = SysAllocStringLen(strString.GetBuffer(), strString.GetCount());
        IFCOOM_RETURN(pResult->bstrVal);
    }
    else
    {
        pResult->vt = VT_EMPTY;
    }

    return S_OK;//RRETURN_REMOVAL
}

HRESULT CUIAWrapper::ConvertToVariant(_In_ CAutomationPeer *pAP, _Out_ VARIANT* pResult)
{
    xref_ptr<CUIAWrapper> wrapper;
    IUnknown *pUnk = nullptr;

    ASSERT(pResult);

    if (pAP)
    {
        if (!m_pUIAWindowValidator || !m_pUIAWindowValidator->IsValid())
        {
            IFC_NOTRACE_RETURN(E_FAIL);
        }

        IFC_RETURN(m_pWindow->CreateProviderForAP(pAP, wrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(wrapper->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&pUnk)));
        IFCPTR_RETURN(pUnk);

        pResult->vt = VT_UNKNOWN;
        pResult->punkVal = pUnk;
    }
    else
    {
        pResult->vt = VT_EMPTY;
    }

    return S_OK;
}


HRESULT CUIAWrapper::ConvertToVariantArr(_In_ CAutomationPeer *pAP, _Out_ VARIANT* pResult)
{
    xref_ptr<CUIAWrapper> wrapper;
    wrl::ComPtr<IUnknown> pUnk = nullptr;
    LONG lLbound = 0;

    ASSERT(pResult);

    if (pAP)
    {
        if (!m_pUIAWindowValidator || !m_pUIAWindowValidator->IsValid())
        {
            IFC_NOTRACE_RETURN(E_FAIL);
        }

        IFC_RETURN(m_pWindow->CreateProviderForAP(pAP, wrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(wrapper->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(pUnk.ReleaseAndGetAddressOf())));
        IFCPTR_RETURN(pUnk);

        unique_safearray safeArray(SafeArrayCreateVector(VT_UNKNOWN, 0, 1));
        IFCOOMFAILFAST(safeArray.get());

        IFC_RETURN(SafeArrayPutElement(safeArray.get(), &lLbound, static_cast<void*>(pUnk.Get())));
        pResult->vt = VT_UNKNOWN | VT_ARRAY;
        pResult->parray = safeArray.release();
    }
    else
    {
        pResult->vt = VT_EMPTY;
    }

    return S_OK;
}

HRESULT CUIAWrapper::ConvertToVariant(_In_ bool fValue, _Out_ VARIANT* pResult)
{
    ASSERT(pResult);

    pResult->vt = VT_BOOL;
    pResult->boolVal = fValue ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}

HRESULT CUIAWrapper::ConvertToVariant(_In_ UIAXcp::APAutomationControlType eValue, _Out_ VARIANT* pResult)
{
    if (!m_pUIAWindowValidator || !m_pUIAWindowValidator->IsValid())
    {
        IFC_NOTRACE_RETURN(E_FAIL);
    }

    ASSERT(pResult);

    pResult->vt = VT_I4;
    pResult->lVal = m_pWindow->ConvertEnumToId(eValue);

    return S_OK;
}

HRESULT CUIAWrapper::ConvertToVariant(_In_ UIAXcp::AutomationLandmarkType eValue, _Out_ VARIANT* pResult)
{
    if (!m_pUIAWindowValidator || !m_pUIAWindowValidator->IsValid())
    {
        IFC_NOTRACE_RETURN(E_FAIL);
    }

    ASSERT(pResult);

    pResult->vt = VT_I4;
    pResult->lVal = m_pWindow->ConvertEnumToId(eValue);

    return S_OK;
}

HRESULT CUIAWrapper::ConvertToVariant(_In_ UIAXcp::AutomationHeadingLevel eValue, _Out_ VARIANT* pResult)
{
    if (!m_pUIAWindowValidator || !m_pUIAWindowValidator->IsValid())
    {
        IFC_NOTRACE_RETURN(E_FAIL);
    }

    ASSERT(pResult);

    pResult->vt = VT_I4;
    pResult->lVal = m_pWindow->ConvertEnumToId(eValue);

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   ConvertToVariant
//
//  Synopsis:
//     Helper functions to convert point data types to Variants
//
//-------------------------------------------------------------------------

HRESULT CUIAWrapper::ConvertToVariant(_In_ XPOINTF *pPoint, _Out_ VARIANT* pResult)
{
    HRESULT hr = S_OK;

    LONG lLbound = 0;
    double ptValue = 0;

    ASSERT(pPoint);
    ASSERT(pResult);

    pResult->vt = VT_R8| VT_ARRAY;

    unique_safearray safeArray(SafeArrayCreateVector(VT_R8, 0, 2));
    IFCOOMFAILFAST(safeArray.get());

    // Add x Point value
    ptValue = static_cast<XDOUBLE>(pPoint->x);
    IFC(SafeArrayPutElement(safeArray.get(), &lLbound, static_cast<void*>(&ptValue)));

    // Add y Point value
    lLbound = 1;
    ptValue = static_cast<XDOUBLE>(pPoint->y);
    IFC(SafeArrayPutElement(safeArray.get(), &lLbound, static_cast<void*>(&ptValue)));

    pResult->parray = safeArray.release();

Cleanup:

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ConvertToVariant
//
//  Synopsis:
//     Helper functions to convert a collections of DependencyObjects into
//     a SAFEARRAY of IUnknown's
//
//-------------------------------------------------------------------------

HRESULT CUIAWrapper::ConvertToVariant(_In_ CDOCollection* pDos, _Out_ VARIANT* pResult)
{
    HRESULT hr = S_OK;
    CDependencyObject* pDO = nullptr;
    CAutomationPeer* pAP = nullptr;
    wrl::ComPtr<CUIAWrapper> wrapper;
    wrl::ComPtr<IUnknown> unk;
    unique_safearray safeArray = nullptr;
    LONG safeArrayCount = 0;
    XUINT32 count = 0;

    if (!m_pUIAWindowValidator || !m_pUIAWindowValidator->IsValid())
    {
        IFC_NOTRACE(E_FAIL);
    }

    ASSERT(pResult);

    count = pDos ? pDos->GetCount() : 0;
    if (count == 0)
    {
        VariantInit(pResult);
        goto Cleanup;
    }

    safeArray.reset(SafeArrayCreateVector(VT_UNKNOWN, 0, count));
    IFCOOMFAILFAST(safeArray.get());

    while (0 < count--)
    {
        pDO = pDos->GetItemImpl(safeArrayCount);
        IFC(DoPointerCast(pAP, pDO));
        IFCEXPECT(pAP);

        IFC(m_pWindow->CreateProviderForAP(pAP, &wrapper));
        IFC(wrapper->QueryInterface(__uuidof(IUnknown), &unk));
        IFCPTR(unk);

        IFC(SafeArrayPutElement(safeArray.get(), &safeArrayCount, static_cast<void*>(unk.Get())));
        ++safeArrayCount;
    }
    pResult->vt = VT_ARRAY | VT_UNKNOWN;
    pResult->parray = safeArray.release();

Cleanup:
    return hr;
}

HRESULT CUIAWrapper::ConvertToVariant(_In_opt_ CAutomationPeerAnnotationCollection* pAnnotations, _In_ PROPERTYID propertyId, _Out_ VARIANT* pResult)
{
    if (!m_pUIAWindowValidator || !m_pUIAWindowValidator->IsValid())
    {
        IFC_NOTRACE_RETURN(E_FAIL);
    }

    IFC_RETURN(m_pWindow->AutomationPeerAnnotationCollectionToVariant(pAnnotations, propertyId, pResult));

    return S_OK;
}

