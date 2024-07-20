// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AutomationCValue.h"
#include "UiaWrapper.h"
#include "AutomationPeer.g.h"
#include "IRawElementProviderSimple.g.h"
#include "ItemsControlAutomationPeer.g.h"
#include "AutomationPeerCollection.g.h"
#include "AutomationProperty.g.h"
#include "UIAEnums.g.h"
#include "AutomationPeerAnnotationCollection.g.h"
#include <xstrutil.h>
#include "localizedResource.h"
#include "UIElement.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

static _Check_return_ HRESULT BoxEnumValueHelper(
    _Out_ Automation::CValue* result,
    _In_ UINT value)
{
    CValue temp;
    IFC_RETURN(CValueBoxer::BoxEnumValue(&temp, value));
    IFC_RETURN(result->ConvertFrom(temp));
    return S_OK;
}

static _Check_return_ HRESULT UnboxEnumValueHelper(
    _In_ const Automation::CValue* box,
    _In_opt_ const CClassInfo* pSourceType,
    _Out_ UINT* result)
{
    CValue temp;
    IFC_RETURN(box->ConvertTo(temp));
    IFC_RETURN(CValueBoxer::UnboxEnumValue(&temp, pSourceType, result));
    return S_OK;
}

template <typename T>
static _Check_return_ HRESULT BoxValueHelper(
    _Out_ Automation::CValue* result,
    _In_ T value)
{
    CValue temp;
    IFC_RETURN(CValueBoxer::BoxValue(&temp, value));
    IFC_RETURN(result->ConvertFrom(temp));
    return S_OK;
}

static _Check_return_ HRESULT BoxObjectValueHelper(
    _Out_ Automation::CValue* result,
    _In_opt_ const CClassInfo* pSourceType,
    _In_opt_ IInspectable* value,
    _In_ BoxerBuffer* buffer,
    _Outptr_result_maybenull_ DependencyObject** ppMOR,
    _In_opt_ BOOLEAN bPreserveObjectIdentity)
{
    CValue temp;
    IFC_RETURN(CValueBoxer::BoxObjectValue(&temp, pSourceType, value, buffer, ppMOR, bPreserveObjectIdentity));
    IFC_RETURN(result->ConvertFrom(temp));
    return S_OK;
}

static _Check_return_ HRESULT UnboxObjectValueHelper(
    _In_ const Automation::CValue* box,
    _In_opt_ const CClassInfo* pTargetType,
    _Out_ IInspectable** result)
{
    CValue temp;
    IFC_RETURN(box->ConvertTo(temp));
    IFC_RETURN(CValueBoxer::UnboxObjectValue(&temp, pTargetType, result));
    return S_OK;
}

// Initializes a new instance of the AutomationPeer class.
AutomationPeer::AutomationPeer()
{
}

// Deconstructor
AutomationPeer::~AutomationPeer()
{
}

_Check_return_ HRESULT DirectUI::AutomationPeer::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    *ppObject = nullptr;

    // This is to support XAML UIA merged tree, with this change XAML now supports native UIA nodes directly
    // hosted in UIA tree for XAML app. native nodes can exist independently in XAML UIA tree and they can
    // host XAML APs as well. The native UIA nodes will be directly dealing with UIACore calls like navigation,
    // if they are directly connected with an AP, they will be required to return native node associated with
    // that AP. To suffice that requirement XAML AP now allows external code to QI for IREPF and returns the
    // associated CUIAWrapper which is basically XAML AP's representation in UIA world.
    // Currently XAML AP and CUIAWrapper have each others' weak reference presence, this design needs a re-visit
    // and XAML APs life time can be bound to CUIAWrapper TFS Id: 579423.
    if (InlineIsEqualGUID(iid, __uuidof(IRawElementProviderFragment)))
    {
        IUIAWrapper *pObjectNoRef = nullptr;
        ctl::ComPtr<IRawElementProviderFragment> spRawProviderFrag;

        pObjectNoRef = (static_cast<CAutomationPeer*>(GetHandle()))->GetUIAWrapper();

        if (pObjectNoRef == nullptr)
        {
            DXamlCore* pCore = DXamlCore::GetFromDependencyObject(this);
            if (pCore)
            {
                xref_ptr<CUIAWrapper> spUIAWrapper;
                IFC_RETURN(pCore->CreateProviderForAP(static_cast<CAutomationPeer*>(GetHandle()), spUIAWrapper.ReleaseAndGetAddressOf()));
                if (spUIAWrapper)
                {
                    IFC_NOTRACE_RETURN(spUIAWrapper->QueryInterface(__uuidof(IRawElementProviderFragment), &spRawProviderFrag));
                }
            }
        }

        if (pObjectNoRef)
        {
            IFC_NOTRACE_RETURN((static_cast<CUIAWrapper*>(pObjectNoRef))->QueryInterface(__uuidof(IRawElementProviderFragment), &spRawProviderFrag));
        }

        if (!spRawProviderFrag)
        {
            IFC_NOTRACE_RETURN(E_NOINTERFACE);
        }

        *ppObject = spRawProviderFrag.Detach();

        return S_OK;
    }

    return DirectUI::AutomationPeerGenerated::QueryInterfaceImpl(iid, ppObject);
}

_Check_return_ HRESULT AutomationPeer::SetParentImpl(_In_ xaml_automation_peers::IAutomationPeer* parent)
{
    HRESULT hr = S_OK;
   IFC(CoreImports::SetAutomationPeerParent(static_cast<CAutomationPeer*>(GetHandle()),
       static_cast<CAutomationPeer*>((static_cast<AutomationPeer*>(parent))->GetHandle())));
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT AutomationPeer::GetPatternCoreImpl(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue)
{
    RRETURN(S_OK);
}

_Check_return_ HRESULT AutomationPeer::GetChildrenImpl(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    HRESULT hr = S_OK;
    wfc::IVector<xaml_automation_peers::AutomationPeer*>* pAPChildren = NULL;
    IAutomationPeer* pAP = NULL;
    UINT nCount = 0;

    *returnValue = NULL;
    IFC(GetChildrenCoreProtected(&pAPChildren));
    if (pAPChildren)
    {
        IFC(pAPChildren->get_Size(&nCount));

        // Defining a set of nodes as children implies that all the children must target this node as their parent. We ensure that
        // relationship here for managed peer objects.
        for (UINT i = 0; i < nCount; i++)
        {
            IFC(pAPChildren->GetAt(i, &pAP));
            if (pAP)
            {
                IFC(CoreImports::SetAutomationPeerParent(static_cast<CAutomationPeer*>(static_cast<AutomationPeer*>(pAP)->GetHandle()),
                    static_cast<CAutomationPeer*>(GetHandle())));
            }
            ReleaseInterface(pAP);
        }
    }

    *returnValue = pAPChildren;
    pAPChildren = NULL;

Cleanup:
    ReleaseInterface(pAP);
    ReleaseInterface(pAPChildren);
    RRETURN(hr);
}

_Check_return_ HRESULT AutomationPeer::GetControlledPeersImpl(_Outptr_ wfc::IVectorView<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    HRESULT hr = S_OK;

    IFC(GetControlledPeersCoreProtected(returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT AutomationPeer::ShowContextMenuImpl()
{
    RRETURN(ShowContextMenuCoreProtected());
}

_Check_return_ HRESULT AutomationPeer::GetPeerFromPointImpl(_In_ wf::Point point, _Outptr_ xaml_automation_peers::IAutomationPeer** returnValue)
{
    HRESULT hr = S_OK;

    IFC(GetPeerFromPointCoreProtected(point, returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT AutomationPeer::GetAcceleratorKeyCoreImpl(_Out_ HSTRING* returnValue)
{
    return wrl_wrappers::HStringReference(STR_LEN_PAIR(L"")).CopyTo(returnValue);
}

_Check_return_ HRESULT AutomationPeer::GetAccessKeyCoreImpl(_Out_ HSTRING* returnValue)
{
    return wrl_wrappers::HStringReference(STR_LEN_PAIR(L"")).CopyTo(returnValue);
}

_Check_return_ HRESULT AutomationPeer::GetAutomationControlTypeCoreImpl(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    *returnValue = xaml_automation_peers::AutomationControlType::AutomationControlType_Custom;
    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::GetAutomationIdCoreImpl(_Out_ HSTRING* returnValue)
{
    return wrl_wrappers::HStringReference(STR_LEN_PAIR(L"")).CopyTo(returnValue);
}

_Check_return_ HRESULT AutomationPeer::GetBoundingRectangleCoreImpl(_Out_ wf::Rect* returnValue)
{
    returnValue->X = 0;
    returnValue->Y = 0;
    returnValue->Width = 0;
    returnValue->Height = 0;

    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::GetChildrenCoreImpl(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    return S_OK;
}

// Custom APs can override NavigateCoreImpl to manage the navigation of APs completley by themselves.
// In addition to that they can also use it to return native UIA nodes which is why the return
// type is IInspectable* instead of an IAutomationPeer*. The default implementation still uses
// GetChildren and GetParent to have backward compatibility. This method also deprecates GetParent.
_Check_return_ HRESULT AutomationPeer::NavigateCoreImpl(_In_ xaml_automation_peers::AutomationNavigationDirection direction, _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml_automation_peers::AutomationPeer*>> spAPChildren;
    ctl::ComPtr<IAutomationPeer> spAP;
    ctl::ComPtr<IInspectable> spAPAsInspectable;
    UINT nCount = 0;

    *ppReturnValue = nullptr;

    switch (direction)
    {
    case xaml_automation_peers::AutomationNavigationDirection_FirstChild:
    {
        IFC(GetChildren(&spAPChildren));
        if (spAPChildren)
        {
            IFC(spAPChildren->get_Size(&nCount));
            if (nCount > 0)
            {
                IFC(spAPChildren->GetAt(0, &spAP));
            }
        }
        break;
    }
    case xaml_automation_peers::AutomationNavigationDirection_LastChild:
    {
        IFC(GetChildren(&spAPChildren));
        if (spAPChildren)
        {
            IFC(spAPChildren->get_Size(&nCount));
            if (nCount > 0)
            {
                IFC(spAPChildren->GetAt(nCount-1, &spAP));
            }
        }
        break;
    }
    case xaml_automation_peers::AutomationNavigationDirection_PreviousSibling:
    {
        // Prev/Next needs to make sure to handle case where parent is root window, GetParent will be null in that case.
        ctl::ComPtr<IAutomationPeer> spAPParent;
        UINT index = 0;
        BOOLEAN found = false;

        IFC(GetParent(&spAPParent));
        if (spAPParent)
        {
            IFC(spAPParent->GetChildren(&spAPChildren));
            if (spAPChildren)
            {
                IFC(spAPChildren->IndexOf(this, &index, &found));
                if (found && index > 0)
                {
                    IFC(spAPChildren->GetAt(index - 1, &spAP));
                }
            }
        }
        break;
    }
    case xaml_automation_peers::AutomationNavigationDirection_NextSibling:
    {
        ctl::ComPtr<IAutomationPeer> spAPParent;
        UINT index = 0;
        BOOLEAN found = false;

        IFC(GetParent(&spAPParent));
        if (spAPParent)
        {
            IFC(spAPParent->GetChildren(&spAPChildren));
            if (spAPChildren)
            {
                IFC(spAPChildren->get_Size(&nCount));
                IFC(spAPChildren->IndexOf(this, &index, &found));
                ASSERT(nCount == 0? found == false : true);
                if (found && index < nCount - 1)
                {
                    IFC(spAPChildren->GetAt(index + 1, &spAP));
                }
            }
        }
        break;
    }
    case xaml_automation_peers::AutomationNavigationDirection_Parent:
    {
        IFC(GetParent(&spAP));
        break;
    }
    default:
        IFC(E_NOT_SUPPORTED);
    }

    IFC(spAP.As(&spAPAsInspectable));
    *ppReturnValue = spAPAsInspectable.Detach();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT AutomationPeer::GetClassNameCoreImpl(_Out_ HSTRING* returnValue)
{
    return wrl_wrappers::HStringReference(STR_LEN_PAIR(L"")).CopyTo(returnValue);
}

_Check_return_ HRESULT AutomationPeer::GetClickablePointCoreImpl(_Out_ wf::Point* returnValue)
{
    returnValue->X = 0;
    returnValue->Y = 0;

    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::GetHelpTextCoreImpl(_Out_ HSTRING* returnValue)
{
    return wrl_wrappers::HStringReference(STR_LEN_PAIR(L"")).CopyTo(returnValue);
}

_Check_return_ HRESULT AutomationPeer::GetItemStatusCoreImpl(_Out_ HSTRING* returnValue)
{
    return wrl_wrappers::HStringReference(STR_LEN_PAIR(L"")).CopyTo(returnValue);
}

_Check_return_ HRESULT AutomationPeer::GetItemTypeCoreImpl(_Out_ HSTRING* returnValue)
{
    return wrl_wrappers::HStringReference(STR_LEN_PAIR(L"")).CopyTo(returnValue);
}

_Check_return_ HRESULT AutomationPeer::GetLabeledByCoreImpl(
    _Outptr_ xaml_automation_peers::IAutomationPeer**  returnValue)
{
    *returnValue = nullptr;

    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::GetLocalizedControlTypeCoreImpl(_Out_ HSTRING* returnValue)
{
    xaml_automation_peers::AutomationControlType apType;

    HRESULT hr = GetAutomationControlType(&apType);
    if (hr == S_FALSE)
    {
        // PopupRoot only has a control type if a light dismiss popup is on top. Otherwise it's not a control and
        // returns S_FALSE. Allow it and return a null string.
        *returnValue = nullptr;
        return hr;
    }
    IFC_RETURN(hr);

    switch (apType)
    {
    case xaml_automation_peers::AutomationControlType::AutomationControlType_Button:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_BUTTON, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_Calendar:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_CALENDAR, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_CheckBox:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_CHECKBOX, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_ComboBox:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_COMBOBOX, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_Edit:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_EDIT, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_Hyperlink:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_HYPERLINK, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_Image:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_IMAGE, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_ListItem:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_LISTITEM, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_List:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_LIST, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_Menu:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_MENU, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_MenuBar:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_MENUBAR, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_MenuItem:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_MENUITEM, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_ProgressBar:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_PROGRESSBAR, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_RadioButton:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_RADIOBUTTON, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_ScrollBar:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_SCROLLBAR, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_Slider:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_SLIDER, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_Spinner:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_SPINNER, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_StatusBar:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_STATUSBAR, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_Tab:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_TAB, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_TabItem:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_TABITEM, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_Text:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_TEXT, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_ToolBar:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_TOOLBAR, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_ToolTip:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_TOOLTIP, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_Tree:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_TREE, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_TreeItem:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_TREEITEM, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_Custom:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_CUSTOM, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_Group:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_GROUP, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_Thumb:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_THUMB, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_DataGrid:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_DATAGRID, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_DataItem:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_DATAITEM, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_Document:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_DOCUMENT, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_SplitButton:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_SPLITBUTTON, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_Window:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_WINDOW, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_Pane:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_PANE, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_Header:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_HEADER, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_HeaderItem:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_HEADERITEM, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_Table:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_TABLE, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_TitleBar:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_TITLEBAR, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_Separator:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_SEPARATOR, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_SemanticZoom:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_SEMANTICZOOM, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_AppBar:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_APPBAR, returnValue));
        break;
    case xaml_automation_peers::AutomationControlType::AutomationControlType_FlipView:
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_FLIPVIEW, returnValue));
        break;
    default:
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::GetNameCoreImpl(_Out_ HSTRING* returnValue)
{
    return wrl_wrappers::HStringReference(STR_LEN_PAIR(L"")).CopyTo(returnValue);
}

_Check_return_ HRESULT AutomationPeer::GetOrientationCoreImpl(_Out_ xaml_automation_peers::AutomationOrientation* returnValue)
{
    *returnValue = xaml_automation_peers::AutomationOrientation::AutomationOrientation_None;

    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::GetLiveSettingCoreImpl(_Out_ xaml_automation_peers::AutomationLiveSetting* returnValue)
{
    *returnValue = xaml_automation_peers::AutomationLiveSetting::AutomationLiveSetting_Off;
    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::GetPositionInSetCoreImpl(_Out_ INT* returnValue)
{
    *returnValue = -1;
    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::GetSizeOfSetCoreImpl(_Out_ INT* returnValue)
{
    *returnValue = -1;
    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::GetLevelCoreImpl(_Out_ INT* returnValue)
{
    *returnValue = -1;
    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::GetControlledPeersCoreImpl(_Outptr_ wfc::IVectorView<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    RRETURN(S_OK);
}

_Check_return_ HRESULT AutomationPeer::GetAnnotationsCoreImpl(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeerAnnotation*>** returnValue)
{
    RRETURN(S_OK);
}

_Check_return_ HRESULT AutomationPeer::GetLandmarkTypeCoreImpl(_Out_ xaml_automation_peers::AutomationLandmarkType* returnValue)
{
    *returnValue = xaml_automation_peers::AutomationLandmarkType::AutomationLandmarkType_None;
    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::GetLocalizedLandmarkTypeCoreImpl(_Out_ HSTRING* returnValue)
{
    return wrl_wrappers::HStringReference(STR_LEN_PAIR(L"")).CopyTo(returnValue);
}

_Check_return_ HRESULT AutomationPeer::HasKeyboardFocusCoreImpl(_Out_ BOOLEAN* returnValue)
{
    return (static_cast<CAutomationPeer*>(GetHandle())->HasKeyboardFocusHelper(returnValue));
}

_Check_return_ HRESULT AutomationPeer::IsContentElementCoreImpl(_Out_ BOOLEAN* returnValue)
{
    *returnValue = false;
    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::IsControlElementCoreImpl(_Out_ BOOLEAN* returnValue)
{
    *returnValue = false;
    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::IsEnabledCoreImpl(_Out_ BOOLEAN* returnValue)
{
    *returnValue = true;
    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::IsKeyboardFocusableCoreImpl(_Out_ BOOLEAN* returnValue)
{
    *returnValue = false;
    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::IsOffscreenCoreImpl(_Out_ BOOLEAN* returnValue)
{
    *returnValue = false;
    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::IsPasswordCoreImpl(_Out_ BOOLEAN* returnValue)
{
    *returnValue = false;
    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::IsRequiredForFormCoreImpl(_Out_ BOOLEAN* returnValue)
{
    *returnValue = false;
    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::SetFocusCoreImpl()
{
    // Lets keep this as it is, that is getting the value from core, Core layer has some exceptional logic that depends upon
    // focus manager.

    IFC_RETURN(static_cast<CAutomationPeer*>(GetHandle())->SetFocusHelper());

    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::SetAutomationFocusImpl()
{
    (static_cast<CAutomationPeer*>(GetHandle())->SetAutomationFocusHelper());
    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::ShowContextMenuCoreImpl()
{
    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::IsPeripheralCoreImpl(_Out_ BOOLEAN* pRetVal)
{
    *pRetVal = false;
    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::IsDataValidForFormCoreImpl(_Out_ BOOLEAN* pRetVal)
{
    // RS1 Bug 6889554 - While this is different than the default value that UIA Core normally returns, we
    // felt the current default of false does not make the most sense for the scenario(s) in which it is used.
    *pRetVal = true;
    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::GetFullDescriptionCoreImpl(_Out_ HSTRING* pRetVal)
{
    return wrl_wrappers::HStringReference(STR_LEN_PAIR(L"")).CopyTo(pRetVal);
}

_Check_return_ HRESULT AutomationPeer::GetDescribedByCoreImpl(_Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    *returnValue = nullptr;
    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::GetFlowsToCoreImpl(_Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    *returnValue = nullptr;
    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::GetFlowsFromCoreImpl(_Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    *returnValue = nullptr;
    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::GetCultureCoreImpl(_Out_ INT* returnValue)
{
    return (static_cast<CAutomationPeer*>(GetHandle())->GetCultureHelper(returnValue));
}

_Check_return_ HRESULT AutomationPeer::GetHeadingLevelCoreImpl(_Out_ xaml_automation_peers::AutomationHeadingLevel* returnValue)
{
    *returnValue = xaml_automation_peers::AutomationHeadingLevel::AutomationHeadingLevel_None;
    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::IsDialogCoreImpl(_Out_ BOOLEAN* returnValue)
{
    *returnValue = false;
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationPeer::get_EventsSource(_Outptr_ xaml_automation_peers::IAutomationPeer** pValue)
{
    *pValue = m_tpEventsSource.Get();
    if(*pValue)
    {
        AddRefInterface(*pValue);
    }

    RRETURN(S_OK);
}

IFACEMETHODIMP DirectUI::AutomationPeer::put_EventsSource(_In_ xaml_automation_peers::IAutomationPeer* value)
{
    CDependencyObject* pCurrentAutomationPeer = NULL;
    CDependencyObject* pEventsSourceAutomationPeer = NULL;

    SetPtrValue(m_tpEventsSource, value);
    pCurrentAutomationPeer = this->GetHandle();
    if(m_tpEventsSource)
    {
        pEventsSourceAutomationPeer = static_cast<AutomationPeer*>(value)->GetHandle();
    }
    // Making sure that setting EventsSource null gets transferred to Core.
    static_cast<CAutomationPeer*>(pCurrentAutomationPeer)->SetAPEventsSource(static_cast<CAutomationPeer*>(pEventsSourceAutomationPeer));
    RRETURN(S_OK);
}

_Check_return_ HRESULT AutomationPeer::GetParentImpl(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue)
{
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spTargetAsAutomationPeer;

    CAutomationPeer* pAutomationPeerCore = static_cast<CAutomationPeer*>(GetHandle())->GetAPParent();

    if (pAutomationPeerCore)
    {
        IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(pAutomationPeerCore, &spTarget));
        if (spTarget)
        {
            IFC_RETURN(spTarget.As(&spTargetAsAutomationPeer));
        }
    }

    *returnValue = spTargetAsAutomationPeer.Detach();

    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::InvalidatePeerImpl()
{
    (static_cast<CAutomationPeer*>(GetHandle())->InvalidatePeer());

    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::RaiseAutomationEventImpl(
    _In_ xaml_automation_peers::AutomationEvents eventId)
{
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spEventsSource;

    IFC_RETURN(get_EventsSource(&spEventsSource));

    if (spEventsSource)
    {
        static_cast<CAutomationPeer*>(spEventsSource.Cast<AutomationPeer>()->GetHandle())->RaiseAutomationEvent((UIAXcp::APAutomationEvents)eventId);
    }
    else
    {
        static_cast<CAutomationPeer*>(GetHandle())->RaiseAutomationEvent((UIAXcp::APAutomationEvents)eventId);
    }

    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::RaiseStructureChangedEventImpl(
    _In_ xaml_automation_peers::AutomationStructureChangeType structureChangeType,
    _In_opt_ xaml_automation_peers::IAutomationPeer* pChild)
{
    CValue newValue;
    CValue oldValue;
    UIAXcp::APAutomationProperties ePropertiesEnum;
    oldValue.SetNull();

    switch (static_cast<AutomationStructureChangeType>(structureChangeType))
    {
        case AutomationStructureChangeType::ChildAdded:
        {
            ePropertiesEnum = UIAXcp::APStructureChangeType_ChildAddedProperty;
            newValue.SetNull();
            break;
        }

        case AutomationStructureChangeType::ChildRemoved:
        {
            // UIAutomationCore expects runtime id of the removed child to be returned, henceforth it's required to be non-null.
            IFCPTR_RETURN(pChild);

            XINT32 runtimeId = static_cast<CAutomationPeer*>(static_cast<AutomationPeer*>(pChild)->GetHandle())->GetRuntimeId();
            newValue.WrapSignedArray(1, &runtimeId);
            ePropertiesEnum = UIAXcp::APStructureChangeType_ChildRemovedProperty;
            break;
        }

        case AutomationStructureChangeType::ChildrenBulkAdded:
        {
            ePropertiesEnum = UIAXcp::APStructureChangeType_ChildrenBulkAddedProperty;
            newValue.SetNull();
            break;
        }

        case AutomationStructureChangeType::ChildrenBulkRemoved:
        {
            ePropertiesEnum = UIAXcp::APStructureChangeType_ChildrenBulkRemovedProperty;
            newValue.SetNull();
            break;
        }

        case AutomationStructureChangeType::ChildrenInvalidated:
        {
            ePropertiesEnum = UIAXcp::APStructureChangeType_ChildrenInvalidatedProperty;
            newValue.SetNull();
            break;
        }

        case AutomationStructureChangeType::ChildrenReordered:
        {
            ePropertiesEnum = UIAXcp::APStructureChangeType_ChildernReorderedProperty;
            newValue.SetNull();
            break;
        }

        default:
        {
            IFC_RETURN(E_UNEXPECTED);
        }
    }

    IFC_RETURN(CoreImports::AutomationRaiseAutomationPropertyChanged(static_cast<CAutomationPeer*>(GetHandle()), ePropertiesEnum, oldValue, newValue));

    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::RaisePropertyChangedEventImpl(
    _In_ xaml_automation::IAutomationProperty* pAutomationProperty,
    _In_ IInspectable* pPropertyValueOld,
    _In_ IInspectable* pPropertyValueNew)
{
    HRESULT hr = S_OK;

    CValue valueOld;
    CValue valueNew;
    BoxerBuffer oldBuffer;
    BoxerBuffer newBuffer;
    DependencyObject *pObjectForCoreOld = NULL;
    DependencyObject *pObjectForCoreNew = NULL;

    AutomationPropertiesEnum ePropertiesEnum;
    static_cast<AutomationProperty*>(pAutomationProperty)->GetAutomationPropertiesEnum(&ePropertiesEnum);

    if (ePropertiesEnum != AutomationPropertiesEnum::ControlledPeersProperty)
    {
        IFC(CValueBoxer::BoxObjectValue(&valueOld, /* pSourceType */ NULL, pPropertyValueOld, &oldBuffer, &pObjectForCoreOld));
        IFC(CValueBoxer::BoxObjectValue(&valueNew, /* pSourceType */ NULL, pPropertyValueNew, &newBuffer, &pObjectForCoreNew));
    }
    else
    {
        // Ideally, we shall verify and marshall the oldValue(vectorView) and newValue(vectorView). But, in this case
        // considering Narrator doesn't care for old and new values for ControllerFor its okay to ignore those values.
        valueOld = CValue();
        valueNew = CValue();
    }

    IFC(RaisePropertyChangedEvent(pAutomationProperty, valueOld, valueNew));

Cleanup:
    ctl::release_interface(pObjectForCoreOld);
    ctl::release_interface(pObjectForCoreNew);

    RRETURN(hr);
}


_Check_return_ HRESULT AutomationPeer::RaisePropertyChangedEvent(
    _In_ xaml_automation::IAutomationProperty* pAutomationProperty,
    _In_ const CValue& oldValue,
    _In_ const CValue& newValue)
{
    HRESULT hr = S_OK;

    AutomationPropertiesEnum ePropertiesEnum;

    static_cast<AutomationProperty*>(pAutomationProperty)->GetAutomationPropertiesEnum(&ePropertiesEnum);
    IFC(CoreImports::AutomationRaiseAutomationPropertyChanged(static_cast<CAutomationPeer*>(GetHandle()), (UIAXcp::APAutomationProperties)ePropertiesEnum, oldValue, newValue));

Cleanup:
    RRETURN(hr);
}


_Check_return_ HRESULT AutomationPeer::RaiseTextEditTextChangedEventImpl(
    _In_ xaml_automation::AutomationTextEditChangeType pAutomationProperty,
    _In_ wfc::IVectorView<HSTRING>* pChangedData)
{
    CValue cValue;
    ctl::ComPtr<IInspectable> spChangedDataAsInspectable;
    ctl::ComPtr<wfc::IVectorView<HSTRING>> spChangedData(pChangedData);
    IFC_RETURN(spChangedData.As(&spChangedDataAsInspectable));

    IFCPTR_RETURN(spChangedDataAsInspectable.Get());
    cValue.SetIInspectableAddRef(spChangedDataAsInspectable.Get());

    switch (pAutomationProperty)
    {
    case xaml_automation::AutomationTextEditChangeType::AutomationTextEditChangeType_None:
        static_cast<CAutomationPeer*>(GetHandle())->RaiseTextEditTextChangedEvent(UIAXcp::AutomationTextEditChangeType::AutomationTextEditChangeType_None, &cValue);
        break;
    case xaml_automation::AutomationTextEditChangeType::AutomationTextEditChangeType_AutoCorrect:
        static_cast<CAutomationPeer*>(GetHandle())->RaiseTextEditTextChangedEvent(UIAXcp::AutomationTextEditChangeType::AutomationTextEditChangeType_AutoCorrect, &cValue);
        break;
    case xaml_automation::AutomationTextEditChangeType::AutomationTextEditChangeType_Composition:
        static_cast<CAutomationPeer*>(GetHandle())->RaiseTextEditTextChangedEvent(UIAXcp::AutomationTextEditChangeType::AutomationTextEditChangeType_Composition, &cValue);
        break;
    case xaml_automation::AutomationTextEditChangeType::AutomationTextEditChangeType_CompositionFinalized:
        static_cast<CAutomationPeer*>(GetHandle())->RaiseTextEditTextChangedEvent(UIAXcp::AutomationTextEditChangeType::AutomationTextEditChangeType_CompositionFinalized, &cValue);
        break;
    }
    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::RaiseNotificationEventImpl(
    xaml_automation_peers::AutomationNotificationKind notificationKind,
    xaml_automation_peers::AutomationNotificationProcessing notificationProcessing,
    _In_opt_ HSTRING displayString,
    _In_ HSTRING activityId)
{
    xstring_ptr xDisplayString;
    xstring_ptr xActivityId;
    IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(displayString, &xDisplayString));
    IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(activityId, &xActivityId));

    static_cast<CAutomationPeer*>(GetHandle())->RaiseNotificationEvent(
        static_cast<UIAXcp::AutomationNotificationKind>(notificationKind),
        static_cast<UIAXcp::AutomationNotificationProcessing>(notificationProcessing),
        xDisplayString,
        xActivityId);

    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::GetPeerFromPointCoreImpl(
    _In_ wf::Point point,
    _Outptr_ xaml_automation_peers::IAutomationPeer** ppReturnValue)
{
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spThis(this);
    *ppReturnValue = spThis.Detach();

    return S_OK;
}

// Custom APs can override GetElementFromPointCoreImpl to manage the hit-testing of APs completley
// by themselves. In addition to that they can also use it to return native UIA nodes as applicable
// which is why the return type is IInspectable* instead of an IAutomationPeer*. The default
// implementation still uses GetPeerFromPoint for backward compatibility.
// This method deprecates GetPeerFromPoint/Core.
_Check_return_ HRESULT AutomationPeer::GetElementFromPointCoreImpl(
    _In_ wf::Point point,
    _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IAutomationPeer> spAutomationPeerFromPoint;
    ctl::ComPtr<IInspectable> spAPAsInspectable;

    *ppReturnValue = NULL;

    IFC(GetPeerFromPoint(point, &spAutomationPeerFromPoint));

    IFC(spAutomationPeerFromPoint.As(&spAPAsInspectable));
    *ppReturnValue = spAPAsInspectable.Detach();

Cleanup:
    RRETURN(hr);
}

// Custom APs can override GetFocusedElementCoreImpl for managing focus of APs/native uia nodes completley
// by themselves.
_Check_return_ HRESULT AutomationPeer::GetFocusedElementCoreImpl(_Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IAutomationPeer> spFocusedElement(this);
    ctl::ComPtr<IInspectable> spAPAsInspectable;

    *ppReturnValue = NULL;

    IFC(spFocusedElement.As(&spAPAsInspectable));
    *ppReturnValue = spAPAsInspectable.Detach();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT AutomationPeer::PeerFromProviderImpl(
    _In_ xaml_automation::Provider::IIRawElementProviderSimple* pProvider,
    _Outptr_ xaml_automation_peers::IAutomationPeer** ppReturnValue)
{
    HRESULT hr = S_OK;

    IFC(PeerFromProviderStatic(pProvider, ppReturnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT AutomationPeer::PeerFromProviderStatic(
    _In_ xaml_automation::Provider::IIRawElementProviderSimple* pProvider,
    _Outptr_ xaml_automation_peers::IAutomationPeer** ppReturnValue)
{
    HRESULT hr = S_OK;

    IFC(static_cast<IRawElementProviderSimple*>(pProvider)->GetAutomationPeer(ppReturnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT AutomationPeer::ProviderFromPeerImpl(
    _In_ xaml_automation_peers::IAutomationPeer* pAutomationPeer,
    _Outptr_ xaml_automation::Provider::IIRawElementProviderSimple** ppReturnValue)
{
    HRESULT hr = S_OK;

    IFC(ProviderFromPeerStatic(pAutomationPeer, ppReturnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT AutomationPeer::ProviderFromPeerStatic(
     _In_ xaml_automation_peers::IAutomationPeer* pAutomationPeer,
    _Outptr_ xaml_automation::Provider::IIRawElementProviderSimple** ppReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppReturnValue);
    *ppReturnValue = NULL;

    if (pAutomationPeer)
    {
        IFC(ctl::ComObject<DirectUI::IRawElementProviderSimple>::CreateInstance(ppReturnValue));
        IFC(static_cast<DirectUI::IRawElementProviderSimple*>(*ppReturnValue)->SetAutomationPeer(pAutomationPeer));
    }

Cleanup:
    RRETURN(hr);
}

// Removes the leading and trailing spaces in the provided string and returns the trimmed version
// or an empty string when no characters are left.
// Because it is recommended to set an AppBarButton, AppBarToggleButton, MenuFlyoutItem or ToggleMenuFlyoutItem's
// KeyboardAcceleratorTextOverride to a single space to hide their keyboard accelerator UI, this trimming method
// prevents automation tools like Narrator from emitting a space when navigating to such an element.
_Check_return_ HRESULT AutomationPeer::GetTrimmedKeyboardAcceleratorTextOverrideStatic(
    _In_ wrl_wrappers::HString& keyboardAcceleratorTextOverride,
    _Out_ HSTRING* returnValue)
{
    // Return an empty string when the provided keyboardAcceleratorTextOverride is already empty.
    if (!WindowsIsStringEmpty(keyboardAcceleratorTextOverride.Get()))
    {
        wrl_wrappers::HStringReference strSpace(L" ");
        wrl_wrappers::HString trimmedKeyboardAcceleratorTextOverride;

        IFCFAILFAST(WindowsTrimStringStart(keyboardAcceleratorTextOverride.Get(), strSpace.Get(), trimmedKeyboardAcceleratorTextOverride.GetAddressOf()));

        // Return an empty string when the remaining string is empty.
        if (!WindowsIsStringEmpty(trimmedKeyboardAcceleratorTextOverride.Get()))
        {
            // Trim the trailing spaces as well.
            IFCFAILFAST(WindowsTrimStringEnd(keyboardAcceleratorTextOverride.Get(), strSpace.Get(), trimmedKeyboardAcceleratorTextOverride.GetAddressOf()));
            IFCFAILFAST(trimmedKeyboardAcceleratorTextOverride.CopyTo(returnValue));
            return S_OK;
        }
    }

    // Return an empty string
    wrl_wrappers::HStringReference(STR_LEN_PAIR(L"")).CopyTo(returnValue);
    return S_OK;
}

// Notify Owner to release AP as no UIA client is holding on to it.
_Check_return_ HRESULT AutomationPeer::NotifyNoUIAClientObjectToOwner()
{
    return S_OK;
}

// Generate EventsSource for this AP, we only want to generate EventsSource for FrameworkElementAPs,
// for others it's responsibility of APP author to set one during creation of object.
_Check_return_ HRESULT AutomationPeer::GenerateAutomationPeerEventsSource(_In_ xaml_automation_peers::IAutomationPeer* pAPParent)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IFrameworkElementAutomationPeer* pContainerItemAP = NULL;

    pContainerItemAP = ctl::query_interface<xaml_automation_peers::IFrameworkElementAutomationPeer>(this);
    if(pContainerItemAP)
    {
        IFC(ItemsControlAutomationPeer::GenerateAutomationPeerEventsSourceStatic(pContainerItemAP, pAPParent));
    }

Cleanup:
    ReleaseInterface(pContainerItemAP);
    RRETURN(hr);
}

// Notify Corresponding core object about managed owner(UI) being dead.
void AutomationPeer::NotifyManagedUIElementIsDead()
{
    CDependencyObject* pAutomationPeer = GetHandle();
    if (pAutomationPeer)
    {
        static_cast<CAutomationPeer*>(pAutomationPeer)->NotifyManagedUIElementIsDead();
    }
}

_Check_return_ HRESULT AutomationPeer::RaisePropertyChangedEventById(_In_ UIAXcp::APAutomationProperties propertyId, _In_ HSTRING oldValue, _In_ HSTRING newValue)
{
    HRESULT hr = S_OK;
    CValue cvalueOld;
    CValue cvalueNew;

    IFC(CValueBoxer::BoxValue(&cvalueOld, oldValue));
    IFC(CValueBoxer::BoxValue(&cvalueNew, newValue));
    IFC(CoreImports::AutomationRaiseAutomationPropertyChanged(static_cast<CAutomationPeer*>(GetHandle()), propertyId, cvalueOld, cvalueNew));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT AutomationPeer::RaisePropertyChangedEventById(_In_ UIAXcp::APAutomationProperties propertyId, _In_ BOOLEAN oldValue, _In_ BOOLEAN newValue)
{
    HRESULT hr = S_OK;
    CValue cvalueOld;
    CValue cvalueNew;

    cvalueOld.SetBool(!!oldValue);
    cvalueNew.SetBool(!!newValue);
    IFC(CoreImports::AutomationRaiseAutomationPropertyChanged(static_cast<CAutomationPeer*>(GetHandle()), propertyId, cvalueOld, cvalueNew));

Cleanup:
    RRETURN(hr);
}

// Get the string value from the specified target AutomationPeer
_Check_return_ HRESULT AutomationPeer::GetAutomationPeerStringValue(
    _In_ CDependencyObject* nativeTarget,
    _In_ UIAXcp::APAutomationProperties eProperty,
    _Out_writes_z_(*pcText) WCHAR* psText,
    _Inout_ XINT32* pcText)
{
    HRESULT hr = S_OK;
    DependencyObject* pTarget = NULL;
    IAutomationPeer* pTargetAsAutomationPeer = NULL;
    wrl_wrappers::HString strValue;
    LPCWSTR psTextTemp = NULL;
    XUINT32 pcTextTemp = 0;

    IFCPTR(nativeTarget);
    IFCPTR(psText);
    *psText = 0;

    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &pTarget));
    IFC(ctl::do_query_interface(pTargetAsAutomationPeer, pTarget));

    switch (static_cast<AutomationPropertiesEnum>(eProperty))
    {
    case AutomationPropertiesEnum::AcceleratorKeyProperty:
        IFC(pTargetAsAutomationPeer->GetAcceleratorKey(strValue.GetAddressOf()));
        break;
    case AutomationPropertiesEnum::AccessKeyProperty:
        IFC(pTargetAsAutomationPeer->GetAccessKey(strValue.GetAddressOf()));
        break;
    case AutomationPropertiesEnum::AutomationIdProperty:
        IFC(pTargetAsAutomationPeer->GetAutomationId(strValue.GetAddressOf()));
        break;
    case AutomationPropertiesEnum::ClassNameProperty:
        IFC(pTargetAsAutomationPeer->GetClassName(strValue.GetAddressOf()));
        break;
    case AutomationPropertiesEnum::HelpTextProperty:
        IFC(pTargetAsAutomationPeer->GetHelpText(strValue.GetAddressOf()));
        break;
    case AutomationPropertiesEnum::ItemStatusProperty:
        IFC(pTargetAsAutomationPeer->GetItemStatus(strValue.GetAddressOf()));
        break;
    case AutomationPropertiesEnum::ItemTypeProperty:
        IFC(pTargetAsAutomationPeer->GetItemType(strValue.GetAddressOf()));
        break;
    case AutomationPropertiesEnum::LocalizedControlTypeProperty:
        IFC(pTargetAsAutomationPeer->GetLocalizedControlType(strValue.GetAddressOf()));
        break;
    case AutomationPropertiesEnum::NameProperty:
        IFC(pTargetAsAutomationPeer->GetName(strValue.GetAddressOf()));
        break;
    case AutomationPropertiesEnum::LocalizedLandmarkTypeProperty:
        {
            IFC(pTargetAsAutomationPeer->GetLocalizedLandmarkType(strValue.GetAddressOf()));
        }
        break;
    case AutomationPropertiesEnum::FullDescriptionProperty:
        {
            IFC(pTargetAsAutomationPeer->GetFullDescription(strValue.GetAddressOf()));
            break;
        }
    default:
        ASSERT(FALSE);
        break;
    }

    psTextTemp = strValue.GetRawBuffer(&pcTextTemp);
    IFC(psTextTemp ? hr : E_FAIL);
    if( pcTextTemp > static_cast<UINT>(*pcText) )
    {
        *pcText = static_cast<INT>(pcTextTemp);

        // NOTRACE because this API uses the common pattern where the caller first invokes
        // it with a default small buffer. If that's not large enough, this API fails and
        // the caller retries with a dynamically allocated large buffer. NOTRACE avoids
        // the error noise from the first try failing.
        // See CAutomationPeer::GetAutomationPeerStringValueFromManaged.
        IFC_NOTRACE(E_FAIL);
    }
    xstrncpy(psText, psTextTemp, pcTextTemp);
    *pcText = static_cast<INT>(pcTextTemp);

Cleanup:
    ctl::release_interface(pTarget);
    ReleaseInterface(pTargetAsAutomationPeer);
    RRETURN(hr);
}

// Get the int value from the specified target AutomationPeer
_Check_return_ HRESULT AutomationPeer::GetAutomationPeerIntValue(
    _In_ CDependencyObject* nativeTarget,
    _In_ UIAXcp::APAutomationProperties eProperty,
    _Inout_ XINT32* pcReturnValue)
{
    HRESULT hr = S_OK;
    DependencyObject* pTarget = NULL;
    IAutomationPeer* pTargetAsAutomationPeer = NULL;
    BOOLEAN bReturnValue = FALSE;

    IFCPTR(nativeTarget);

    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &pTarget));
    IFC(ctl::do_query_interface(pTargetAsAutomationPeer, pTarget));

    switch (static_cast<AutomationPropertiesEnum>(eProperty))
    {
    case AutomationPropertiesEnum::ControlTypeProperty:
        IFC(pTargetAsAutomationPeer->GetAutomationControlType((xaml_automation_peers::AutomationControlType*)pcReturnValue));
        break;
    case AutomationPropertiesEnum::OrientationProperty:
        IFC(pTargetAsAutomationPeer->GetOrientation((xaml_automation_peers::AutomationOrientation*)pcReturnValue));
        break;
    case AutomationPropertiesEnum::LiveSettingProperty:
        IFC(pTargetAsAutomationPeer->GetLiveSetting((xaml_automation_peers::AutomationLiveSetting*)pcReturnValue));
        break;
    case AutomationPropertiesEnum::HasKeyboardFocusProperty:
        IFC(pTargetAsAutomationPeer->HasKeyboardFocus(&bReturnValue));
        *pcReturnValue = bReturnValue ? -1 : 0;
        break;
    case AutomationPropertiesEnum::IsContentElementProperty:
        IFC(pTargetAsAutomationPeer->IsContentElement(&bReturnValue));
        *pcReturnValue = bReturnValue ? -1 : 0;
        break;
    case AutomationPropertiesEnum::IsControlElementProperty:
        IFC(pTargetAsAutomationPeer->IsControlElement(&bReturnValue));
        *pcReturnValue = bReturnValue ? -1 : 0;
        break;
    case AutomationPropertiesEnum::IsEnabledProperty:
        IFC(pTargetAsAutomationPeer->IsEnabled(&bReturnValue));
        *pcReturnValue = bReturnValue ? -1 : 0;
        break;
    case AutomationPropertiesEnum::IsKeyboardFocusableProperty:
        IFC(pTargetAsAutomationPeer->IsKeyboardFocusable(&bReturnValue));
        *pcReturnValue = bReturnValue ? -1 : 0;
        break;
    case AutomationPropertiesEnum::IsOffscreenProperty:
        IFC(pTargetAsAutomationPeer->IsOffscreen(&bReturnValue));
        *pcReturnValue = bReturnValue ? -1 : 0;
        break;
    case AutomationPropertiesEnum::IsPasswordProperty:
        IFC(pTargetAsAutomationPeer->IsPassword(&bReturnValue));
        *pcReturnValue = bReturnValue ? -1 : 0;
        break;
    case AutomationPropertiesEnum::IsRequiredForFormProperty:
        IFC(pTargetAsAutomationPeer->IsRequiredForForm(&bReturnValue));
        *pcReturnValue = bReturnValue ? -1 : 0;
        break;
    case AutomationPropertiesEnum::PositionInSetProperty:
        IFC(static_cast<AutomationPeer*>(pTargetAsAutomationPeer)->GetPositionInSet(pcReturnValue));
        break;
    case AutomationPropertiesEnum::CultureProperty:
        IFC(static_cast<AutomationPeer*>(pTargetAsAutomationPeer)->GetCulture(pcReturnValue));
        break;
    case AutomationPropertiesEnum::SizeOfSetProperty:
        IFC(static_cast<AutomationPeer*>(pTargetAsAutomationPeer)->GetSizeOfSet(pcReturnValue));
        break;
    case AutomationPropertiesEnum::LevelProperty:
        IFC(static_cast<AutomationPeer*>(pTargetAsAutomationPeer)->GetLevel(pcReturnValue));
        break;
    case AutomationPropertiesEnum::LandmarkTypeProperty:
        IFC(static_cast<AutomationPeer*>(pTargetAsAutomationPeer)->GetLandmarkType((xaml_automation_peers::AutomationLandmarkType*)pcReturnValue));
        break;
    case AutomationPropertiesEnum::IsPeripheralProperty:
        {
            IFC(pTargetAsAutomationPeer->IsPeripheral(&bReturnValue));
            *pcReturnValue = bReturnValue ? -1 : 0;
            break;
        }
    case AutomationPropertiesEnum::IsDataValidForFormProperty:
        {
            IFC(pTargetAsAutomationPeer->IsDataValidForForm(&bReturnValue));
            *pcReturnValue = bReturnValue ? -1 : 0;
            break;
        }
    case AutomationPropertiesEnum::HeadingLevelProperty:
        IFC(static_cast<AutomationPeer*>(pTargetAsAutomationPeer)->GetHeadingLevel((xaml_automation_peers::AutomationHeadingLevel*)pcReturnValue));
        break;
    case AutomationPropertiesEnum::IsDialogProperty:
        {
            IFC(pTargetAsAutomationPeer->IsDialog(&bReturnValue));
            *pcReturnValue = bReturnValue ? -1 : 0;
            break;
        }
    default:
        ASSERT(FALSE);
        break;
    }

Cleanup:
    ctl::release_interface(pTarget);
    ReleaseInterface(pTargetAsAutomationPeer);

    RRETURN(hr);
}

// Get the point position of the specified AutomationPeer
_Check_return_ HRESULT AutomationPeer::GetAutomationPeerPointValue(
    _In_ CDependencyObject* nativeTarget,
    _In_ UIAXcp::APAutomationProperties eProperty,
    _Out_ XPOINTF* pReturnPoint)
{
    HRESULT hr = S_OK;
    DependencyObject* pTarget = NULL;
    IAutomationPeer* pTargetAsAutomationPeer = NULL;
    wf::Point pointValue = {};

    IFCPTR(nativeTarget);

    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &pTarget));
    IFC(ctl::do_query_interface(pTargetAsAutomationPeer, pTarget));

    IFC(pTargetAsAutomationPeer->GetClickablePoint(&pointValue));

    pReturnPoint->x = pointValue.X;
    pReturnPoint->y = pointValue.Y;

Cleanup:
    ctl::release_interface(pTarget);
    ReleaseInterface(pTargetAsAutomationPeer);

    RRETURN(hr);
}

_Check_return_ HRESULT AutomationPeer::GetAutomationPeerRectValue(
    _In_ CDependencyObject* nativeTarget,
    _In_ UIAXcp::APAutomationProperties eProperty,
    _Out_ XRECTF* pReturnRect)
{
    HRESULT hr = S_OK;
    DependencyObject* pTarget = NULL;
    IAutomationPeer* pTargetAsAutomationPeer = NULL;
    wf::Rect rectValue;

    IFCPTR(nativeTarget);

    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &pTarget));
    IFC(ctl::do_query_interface(pTargetAsAutomationPeer, pTarget));

    IFC(pTargetAsAutomationPeer->GetBoundingRectangle(&rectValue));

    pReturnRect->X = rectValue.X;
    pReturnRect->Y = rectValue.Y;
    pReturnRect->Width = rectValue.Width;
    pReturnRect->Height = rectValue.Height;

Cleanup:
    ctl::release_interface(pTarget);
    ReleaseInterface(pTargetAsAutomationPeer);

    RRETURN(hr);
}

_Check_return_ HRESULT AutomationPeer::GetAutomationPeerAPValue(
    _In_ CDependencyObject* nativeTarget,
    _In_ UIAXcp::APAutomationProperties eProperty,
    _Inout_ ::CDependencyObject** ppReturnAP)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<IAutomationPeer> spTargetAsAutomationPeer;
    ctl::ComPtr<IAutomationPeer> spReturnedAP;

    IFCPTR(nativeTarget);

    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &spTarget));
    IFC(spTarget.As(&spTargetAsAutomationPeer));

    switch (eProperty)
    {
    case UIAXcp::APLabeledByProperty:
        IFC(spTargetAsAutomationPeer->GetLabeledBy(&spReturnedAP));
        break;
    default:
        IFC(E_NOTIMPL);
    }

    if (spReturnedAP)
    {
        CDependencyObject* pAutomationPeer = spReturnedAP.Cast<AutomationPeer>()->GetHandle();
        *ppReturnAP = static_cast<CAutomationPeer*>(pAutomationPeer);
        IFC(CoreImports::DependencyObject_AddRef(pAutomationPeer));
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_ HRESULT AutomationPeer::GetAutomationPeerDOValue(
    _In_ CDependencyObject* nativeTarget,
    _In_ UIAXcp::APAutomationProperties eProperty,
    _Outptr_ ::CDependencyObject** ppReturnDO)
{
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<IAutomationPeer> spTargetAsAutomationPeer;

    unsigned size = 0;

    IFCPTR_RETURN(nativeTarget);
    IFCPTR_RETURN(ppReturnDO);
    *ppReturnDO = NULL;

    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &spTarget));
    spTarget.As(&spTargetAsAutomationPeer);

    switch (eProperty)
    {
        case UIAXcp::APControlledPeersProperty:
        {
            ctl::ComPtr<wfc::IVectorView<xaml_automation_peers::AutomationPeer*>> spAutomationPeerVector;
            ctl::ComPtr<AutomationPeerCollection> spColleciton;

            IFC_RETURN(spTargetAsAutomationPeer->GetControlledPeers(&spAutomationPeerVector));

            if (spAutomationPeerVector)
            {
                IFC_RETURN(spAutomationPeerVector->get_Size(&size));
            }
            if (size == 0)
            {
                return S_OK;
            }

            // AGCore doesn't know about DXAML VectorViews. So, we have to marshal to a CAutomationPeerCollection.
            IFC_RETURN(ctl::make(&spColleciton));
            for (unsigned i = 0; i < size; ++i)
            {
                ctl::ComPtr<IAutomationPeer> spPeer;
                IFC_RETURN(spAutomationPeerVector->GetAt(i, &spPeer));
                IFC_RETURN(spColleciton->Append(spPeer.Get()));
            }
            *ppReturnDO = spColleciton->GetHandle();
            IFC_RETURN(CoreImports::DependencyObject_AddRef(*ppReturnDO));
            break;
        }

        case UIAXcp::APAnnotationsProperty:
        {
            ctl::ComPtr<wfc::IVector<xaml_automation_peers::AutomationPeerAnnotation*>> spAnnotationVector;
            ctl::ComPtr<AutomationPeerAnnotationCollection> spColleciton;

            IFC_RETURN(spTargetAsAutomationPeer->GetAnnotations(&spAnnotationVector));

            if (spAnnotationVector)
            {
                IFC_RETURN(spAnnotationVector->get_Size(&size));
            }
            if (size == 0)
            {
                return S_OK;
            }

            // AGCore doesn't know about DXAML Vectors. So, we have to marshal to a CAutomationPeerAnnotationCollection.
            IFC_RETURN(ctl::make(&spColleciton));
            for (unsigned i = 0; i < size; ++i)
            {
                ctl::ComPtr<IAutomationPeerAnnotation> spAnnotation;
                IFC_RETURN(spAnnotationVector->GetAt(i, &spAnnotation));
                IFC_RETURN(spColleciton->Append(spAnnotation.Get()));
            }
            *ppReturnDO = spColleciton->GetHandle();
            IFC_RETURN(CoreImports::DependencyObject_AddRef(*ppReturnDO));
            break;
        }

        case UIAXcp::APDescribedByProperty:
        case UIAXcp::APFlowsToProperty:
        case UIAXcp::APFlowsFromProperty:
            GetAutomationPeerDOValueFromIterable(nativeTarget, eProperty, ppReturnDO);
            break;

        default:
            ASSERT(!"Incorrect APAutomationProperties in GetAutomationPeerDOValue");
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::CallAutomationPeerMethod(
    _In_ CDependencyObject* nativeTarget,
    _In_ XINT32 CallAutomationPeerMethod)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spTarget;

    IFCPTR(nativeTarget);

    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &spTarget));

    if (CallAutomationPeerMethod == 0 /* SetFocus */)
    {
        ctl::ComPtr<IAutomationPeer> spTargetAsAutomationPeer;
        IFC(spTarget.As(&spTargetAsAutomationPeer));
        IFC(spTargetAsAutomationPeer->SetFocus());
    }
    else if (CallAutomationPeerMethod == 1 /* ShowContextMenu */)
    {
        ctl::ComPtr<IAutomationPeer> spTargetAsAutomationPeer;
        IFC(spTarget.As(&spTargetAsAutomationPeer));
        IFC(spTargetAsAutomationPeer->ShowContextMenu());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT AutomationPeer::Navigate(
    _In_ CDependencyObject* nativeTarget,
    _In_ UIAXcp::AutomationNavigationDirection direction,
    _Outptr_result_maybenull_ ::CDependencyObject** ppReturnAPAsDO,
    _Outptr_result_maybenull_ IUnknown** ppReturnIREPFAsUnk)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<IAutomationPeer> spTargetAsAutomationPeer;
    ctl::ComPtr<IInspectable> spResult;

    *ppReturnAPAsDO = nullptr;
    *ppReturnIREPFAsUnk = nullptr;

    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &spTarget));
    IFC(spTarget.As(&spTargetAsAutomationPeer));

    IFC(spTargetAsAutomationPeer->Navigate((xaml_automation_peers::AutomationNavigationDirection)(direction), &spResult));

    // handle and verify if returned object is an AP or a native UIA node
    if (spResult)
    {
        RetrieveNativeNodeOrAPFromIInspectable(spResult.Get(), ppReturnAPAsDO, ppReturnIREPFAsUnk);
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT AutomationPeer::GetAutomationPeerChildren(
    _In_ CDependencyObject* nativeTarget,
    _In_ XUINT32 CallAutomationPeerMethod,
    _Inout_ XINT32* pcReturnAPChildren,
    __deref_inout_ecount(*pcReturnAPChildren) ::CDependencyObject*** pppReturnAPChildren)
{
    HRESULT hr = S_OK;

    DependencyObject* pTarget = NULL;
    IAutomationPeer* pTargetAsAutomationPeer = NULL;
    wfc::IVector<xaml_automation_peers::AutomationPeer*>* pChildren = NULL;

    IFCPTR(nativeTarget);
    IFCPTR(pcReturnAPChildren);
    IFCPTR(pppReturnAPChildren);

    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &pTarget));
    IFC(ctl::do_query_interface(pTargetAsAutomationPeer, pTarget));

    if (CallAutomationPeerMethod == 0 /* Count */ || CallAutomationPeerMethod == 1 /* Children */)
    {
        XUINT32 nChildrenCount = 0;

        IFC(pTargetAsAutomationPeer->GetChildren(&pChildren));
        if(pChildren != NULL)
        {
            IFC(pChildren->get_Size(&nChildrenCount));
            if (CallAutomationPeerMethod == 1)
            {
                if(static_cast<UINT>(*pcReturnAPChildren) < nChildrenCount)
                {
                    IFC(E_FAIL);
                }

                if (CallAutomationPeerMethod == 1 /* Children */)
                {
                    IAutomationPeer *pAP = NULL;

                    for (XUINT32 nIndex = 0; nIndex < nChildrenCount; ++nIndex)
                    {
                        IFC(pChildren->GetAt(nIndex, &pAP));
                        IFCPTR(pAP);

                        (*pppReturnAPChildren)[nIndex] = static_cast<AutomationPeer*>(pAP)->GetHandle();

                        ReleaseInterface(pAP);
                    }
                }
            }

            // In the case where CallAutomationPeerMethod==0 we need to return the required size.
            // OACR thinks we are blowing up the array, but we are not.
            _Analysis_assume_(nChildrenCount <= static_cast<UINT>(*pcReturnAPChildren));
            *pcReturnAPChildren = static_cast<INT>(nChildrenCount);
        }
    }
    else
    {
        ASSERT(FALSE);
    }

Cleanup:
    ctl::release_interface(pTarget);
    ReleaseInterface(pTargetAsAutomationPeer);
    ReleaseInterface(pChildren);
    RRETURN(hr);
}

// Get the specified pattern on the target AutomationPeer
_Check_return_ HRESULT AutomationPeer::GetPattern(
    _In_ CDependencyObject* nativeTarget,
    _Outptr_ CDependencyObject** nativeInterface,
    _In_ UIAXcp::APPatternInterface eInterface)
{

    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<IAutomationPeer> spTargetAsAutomationPeer;
    ctl::ComPtr<IFrameworkElementAutomationPeer> spTargetAsFEAP;
    ctl::ComPtr<IInspectable> spObject;
    ctl::ComPtr<DependencyObject> spPatternObject;

    IFCPTR_RETURN(nativeInterface);
    *nativeInterface = nullptr;

    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &spTarget));
    IFC_RETURN(spTarget.As(&spTargetAsAutomationPeer));

    IFC_RETURN(spTargetAsAutomationPeer->GetPattern((xaml_automation_peers::PatternInterface)eInterface, &spObject));

    if (spObject == nullptr)
    {
        spTargetAsFEAP = spTarget.AsOrNull<IFrameworkElementAutomationPeer>();
        if(spTargetAsFEAP != nullptr)
        {
            IFC_RETURN(spTargetAsFEAP.Cast<FrameworkElementAutomationPeer>()->GetDefaultPattern((xaml_automation_peers::PatternInterface)eInterface, &spObject));
        }
    }

    if (spObject)
    {
        IFC_RETURN(ExternalObjectReference::ConditionalWrap(spObject.Get(), &spPatternObject));
        *nativeInterface = spPatternObject->GetHandle();

        // Peg the object; the core is responsible for un-pegging.
        spPatternObject->PegNoRef();
    }

    return S_OK;
}

_Check_return_ HRESULT AutomationPeer::GetElementFromPoint(
    _In_ CDependencyObject* nativeTarget,
    _In_ const CValue& param,
    _Outptr_result_maybenull_ ::CDependencyObject** ppReturnAPAsDO,
    _Outptr_result_maybenull_ IUnknown** ppReturnIREPFAsUnk)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<IAutomationPeer> spTargetAsAutomationPeer;
    ctl::ComPtr<IInspectable> spResult;
    wf::Point screenPoint;

    *ppReturnAPAsDO = nullptr;
    *ppReturnIREPFAsUnk = nullptr;

    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &spTarget));
    IFC(spTarget.As(&spTargetAsAutomationPeer));

    // No need to type-cast, float is expected.
    screenPoint.X = param.AsPoint()->x;
    screenPoint.Y = param.AsPoint()->y;

    IFC(spTargetAsAutomationPeer->GetElementFromPoint(screenPoint, &spResult));

    // handle and verify if returned object is an AP or a native UIA node
    if (spResult)
    {
        RetrieveNativeNodeOrAPFromIInspectable(spResult.Get(), ppReturnAPAsDO, ppReturnIREPFAsUnk);
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT AutomationPeer::GetFocusedElement(
    _In_ CDependencyObject* nativeTarget,
    _Outptr_result_maybenull_ ::CDependencyObject** ppReturnAPAsDO,
    _Outptr_result_maybenull_ IUnknown** ppReturnIREPFAsUnk)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<IAutomationPeer> spTargetAsAutomationPeer;
    ctl::ComPtr<IInspectable> spResult;

    *ppReturnAPAsDO = nullptr;
    *ppReturnIREPFAsUnk = nullptr;

    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &spTarget));
    IFC(spTarget.As(&spTargetAsAutomationPeer));

    IFC(spTargetAsAutomationPeer->GetFocusedElement(&spResult));

    // handle and verify if returned object is an AP or a native UIA node
    if (spResult)
    {
        RetrieveNativeNodeOrAPFromIInspectable(spResult.Get(), ppReturnAPAsDO, ppReturnIREPFAsUnk);
    }

Cleanup:
    RRETURN(hr);
}

// Invoke the specified pattern methods
_Check_return_ HRESULT AutomationPeer::UIATextRangeInvoke(
    _In_ CDependencyObject* nativeTarget,
    _In_ XINT32 eFunction,
    _In_ XINT32 cParams,
    _In_ void* pvParams,
    _Out_ Automation::CValue* pReturnVal) noexcept
{
    HRESULT hr = S_OK;
    DependencyObject* pMOROrTarget = NULL;
    IInspectable* pMOROrTargetAsIInspectable = NULL;
    IInspectable* pTargetAsIInspectable = NULL;
    xaml_automation_peers::IAutomationPeer* pAP = NULL;
    IInspectable* pAttributeValue = NULL;
    BoxerBuffer buffer;
    DependencyObject* pDO = NULL;
    Automation::CValue* pValueParams = NULL;
    xaml_automation::Provider::ITextRangeProvider* pTextRangeProvider = NULL;
    xaml_automation::Provider::ITextRangeProvider2* pTextRangeProvider2 = NULL;
    xaml_automation::Provider::ITextRangeProvider* pTextRangeProviderOther = NULL;
    xaml_automation::Provider::IIRawElementProviderSimple** pChildren = NULL;
    xaml_automation::Provider::IIRawElementProviderSimple* pRawElementProvider = NULL;
    DependencyObject* pPatternObject = NULL;
    wrl_wrappers::HString strValue;
    LPCWSTR psTextTemp = NULL;
    XUINT32 pcTextTemp = 0;
    XINT32 iAttributeValue = -1;
    DOUBLE* pDoubleArray = NULL;
    DOUBLE* pDoubleCurrent = NULL;
    void** ppCurrentItem = NULL;
    UINT nLength = 0;
    BOOLEAN bRetVal = FALSE;
    XINT32 iRetVal = -1;
    xaml_automation::Text::TextPatternRangeEndpoint rangeEndpoint;
    xaml_automation::Text::TextPatternRangeEndpoint targetRangeEndpoint;
    xaml_automation::Text::TextUnit textUnit;
    BOOLEAN isBackward = FALSE;
    BOOLEAN isIgnoreCase = FALSE;
    BOOLEAN alignToTop = FALSE;
    BOOLEAN bIsUnsetValue = FALSE;
    DXamlCore* pCore = DXamlCore::GetCurrent();
    INT enumValue = 0;
    CValue tempValue;
    CSolidColorBrush *pBrush = NULL;

    IFCPTR(nativeTarget);

    // Get the target AutomationPeer
    IFC(pCore->GetPeer(nativeTarget, &pMOROrTarget));
    IFC(ctl::do_query_interface(pMOROrTargetAsIInspectable, pMOROrTarget));
    CValueBoxer::UnwrapExternalObjectReferenceIfPresent(pMOROrTargetAsIInspectable, &pTargetAsIInspectable);
    IFCPTR(pTargetAsIInspectable);

    // Assign CValue parameters
    pValueParams = (Automation::CValue*)pvParams;
    IFC(ctl::do_query_interface(pTextRangeProvider, pTargetAsIInspectable));
    ctl::release_interface(pMOROrTarget);
    ReleaseInterface(pMOROrTargetAsIInspectable);
    ReleaseInterface(pTargetAsIInspectable);

    if (pTextRangeProvider)
    {
        // ITextRangeProvider inferface name is hard coded like as below and it's tracking with bug#32555
        switch (eFunction)
        {
            case 0: // AddToSelection
                IFC(pTextRangeProvider->AddToSelection());
                    break;
            case 1: // Clone
                IFCPTR(pReturnVal);
                IFC(pTextRangeProvider->Clone(&pTextRangeProviderOther));
                if (pTextRangeProviderOther)
                {
                    IFC(ExternalObjectReference::ConditionalWrap(pTextRangeProviderOther, &pPatternObject));
                    pReturnVal->m_pdoValue = pPatternObject->GetHandle();

                    // Peg the object; the core is responsible for un-pegging.
                    pPatternObject->PegNoRef();
                }
                else
                {
                    pReturnVal->m_pdoValue = NULL;
                }
                break;
            case 2: // Compare
                IFCPTR(pReturnVal);
                IFCPTR(pValueParams);
                IFC(pCore->GetPeer((&pValueParams[0])->m_pdoValue, &pMOROrTarget));
                IFC(ctl::do_query_interface(pMOROrTargetAsIInspectable, pMOROrTarget));
                CValueBoxer::UnwrapExternalObjectReferenceIfPresent(pMOROrTargetAsIInspectable, &pTargetAsIInspectable);
                IFCPTR(pTargetAsIInspectable);
                IFC(ctl::do_query_interface(pTextRangeProviderOther, pTargetAsIInspectable));
                IFC(pTextRangeProvider->Compare(pTextRangeProviderOther, &bRetVal));
                pReturnVal->SetBool(bRetVal);
                break;
            case 3: // CompareEndpoints
                IFCPTR(pReturnVal);
                IFCPTR(pValueParams);
                rangeEndpoint = (xaml_automation::Text::TextPatternRangeEndpoint)(&pValueParams[0])->m_nValue;
                IFC(pCore->GetPeer((&pValueParams[1])->m_pdoValue, &pMOROrTarget));
                IFC(ctl::do_query_interface(pMOROrTargetAsIInspectable, pMOROrTarget));
                CValueBoxer::UnwrapExternalObjectReferenceIfPresent(pMOROrTargetAsIInspectable, &pTargetAsIInspectable);
                IFCPTR(pTargetAsIInspectable);
                IFC(ctl::do_query_interface(pTextRangeProviderOther, pTargetAsIInspectable));
                targetRangeEndpoint = (xaml_automation::Text::TextPatternRangeEndpoint)(&pValueParams[2])->m_nValue;
                IFC(pTextRangeProvider->CompareEndpoints(rangeEndpoint, pTextRangeProviderOther, targetRangeEndpoint, &iRetVal));
                pReturnVal->SetSigned(iRetVal);
                break;
            case 4: // ExpandToEnclosingUnit
                IFCPTR(pValueParams);
                textUnit = (xaml_automation::Text::TextUnit)(&pValueParams[0])->m_nValue;
                IFC(pTextRangeProvider->ExpandToEnclosingUnit(textUnit));
                break;
            case 5: // FindAttribute
                IFCPTR(pReturnVal);
                IFCPTR(pValueParams);
                iAttributeValue = pValueParams[0].m_iValue;

                IFC(UnboxObjectValueHelper(&pValueParams[1], nullptr, &pAttributeValue));
                isBackward = pValueParams[2].m_nValue != 0;

                IFC(pTextRangeProvider->FindAttribute(iAttributeValue, pAttributeValue, isBackward, &pTextRangeProviderOther));
                if (pTextRangeProviderOther)
                {
                    IFC(ExternalObjectReference::ConditionalWrap(pTextRangeProviderOther, &pPatternObject));
                    pReturnVal->SetObjectAddRef(pPatternObject->GetHandle());

                    // Peg the object; the core is responsible for un-pegging.
                    pPatternObject->PegNoRef();
                }
                else
                {
                    pReturnVal->SetNull();
                }
                break;
            case 6: // FindText
                IFCPTR(pReturnVal);
                IFCPTR(pValueParams);
                IFC(GetStringFromCValue(&pValueParams[0], strValue.GetAddressOf()));
                isBackward = pValueParams[1].m_nValue != 0;
                isIgnoreCase = pValueParams[2].m_nValue != 0;

                IFC(pTextRangeProvider->FindText(strValue.Get(), isBackward, isIgnoreCase, &pTextRangeProviderOther));
                if (pTextRangeProviderOther)
                {
                    IFC(ExternalObjectReference::ConditionalWrap(pTextRangeProviderOther, &pPatternObject));
                    pReturnVal->m_pdoValue = pPatternObject->GetHandle();

                    // Peg the object; the core is responsible for un-pegging.
                    pPatternObject->PegNoRef();
                }
                else
                {
                    pReturnVal->m_pdoValue = NULL;
                }
                break;
            case 7: // GetAttributeValue
                IFCPTR(pReturnVal);
                IFCPTR(pValueParams);
                iAttributeValue = pValueParams[0].m_iValue;
                // Different attribute values needs to be boxed differently, also there is no way to differentiate Enum
                // via Inspectable as Inspectable loses Enum info so that needs to be handled here.
                IFC(pTextRangeProvider->GetAttributeValue(iAttributeValue, &pAttributeValue));
                if (pAttributeValue)
                {
                    IFC(DependencyPropertyFactory::IsUnsetValue(pAttributeValue, bIsUnsetValue));
                    // Mixed Attribute is returned as UnsetValue in TextRangeAdapter::GetAttributeValueImpl(),
                    // Set it to type IUnknown with value null here, so it can be detected in CUIATextRangeProviderWrapper::GetAttributeValueImpl
                    // and the real global mixed attribute can be returned to UIACore.
                    if (bIsUnsetValue)
                    {
                        pReturnVal->SetIUnknownNoRef(nullptr);
                        break;
                    }
                    switch (static_cast<AutomationTextAttributesEnum>(iAttributeValue))
                    {
                    case AutomationTextAttributesEnum::AnimationStyleAttribute:
                    case AutomationTextAttributesEnum::BulletStyleAttribute:
                    case AutomationTextAttributesEnum::CapStyleAttribute:
                    case AutomationTextAttributesEnum::CultureAttribute:
                    case AutomationTextAttributesEnum::FontWeightAttribute:
                    case AutomationTextAttributesEnum::HorizontalTextAlignmentAttribute:
                    case AutomationTextAttributesEnum::OutlineStylesAttribute:
                    case AutomationTextAttributesEnum::OverlineStyleAttribute:
                    case AutomationTextAttributesEnum::StrikethroughStyleAttribute:
                    case AutomationTextAttributesEnum::TextFlowDirectionsAttribute:
                    case AutomationTextAttributesEnum::UnderlineStyleAttribute:
                    case AutomationTextAttributesEnum::StyleIdAttribute:
                    case AutomationTextAttributesEnum::SelectionActiveEndAttribute:
                    case AutomationTextAttributesEnum::CaretPositionAttribute:
                    case AutomationTextAttributesEnum::CaretBidiModeAttribute:
                        IFC(ctl::do_get_value(enumValue, pAttributeValue));
                        IFC(BoxEnumValueHelper(pReturnVal, static_cast<UINT>(enumValue)));
                        break;
                    case AutomationTextAttributesEnum::BackgroundColorAttribute:
                    case AutomationTextAttributesEnum::ForegroundColorAttribute:
                    case AutomationTextAttributesEnum::OverlineColorAttribute:
                    case AutomationTextAttributesEnum::StrikethroughColorAttribute:
                    case AutomationTextAttributesEnum::UnderlineColorAttribute:
                        IFC(CValueBoxer::BoxObjectValue(&tempValue, NULL, pAttributeValue, &buffer, &pDO, TRUE));
                        if (tempValue.GetType() == valueSigned)
                        {
                            pReturnVal->SetEnum(tempValue.AsSigned());
                        }
                        else if (tempValue.GetType() == valueObject)
                        {
                            pBrush = do_pointer_cast<CSolidColorBrush>(tempValue.AsObject());
                            if (pBrush != NULL)
                            {
                                pReturnVal->SetEnum(((pBrush->m_rgb & 0X00ff0000) >> 16) | (pBrush->m_rgb & 0X0000ff00) | ((pBrush->m_rgb & 0X000000ff) << 16));
                            }
                            else
                            {
                                pReturnVal->SetEnum(0);
                            }
                        }
                        else if (tempValue.IsEnum())
                        {
                            IFC(pReturnVal->ConvertFrom(tempValue));
                        }
                        break;
                    case AutomationTextAttributesEnum::LinkAttribute:
                        {
                            IFC(ctl::do_get_value(pTextRangeProviderOther, pAttributeValue));
                            if (pTextRangeProviderOther)
                            {
                                IFC(ExternalObjectReference::ConditionalWrap(pTextRangeProviderOther, &pPatternObject));
                                pReturnVal->SetObjectAddRef(pPatternObject->GetHandle());

                                // Peg the object; the core is responsible for un-pegging.
                                pPatternObject->PegNoRef();
                            }
                            else
                            {
                                pReturnVal->SetNull();
                            }
                        }
                        break;
                    case AutomationTextAttributesEnum::TabsAttribute:
                        {
                            unsigned size = 0;
                            ctl::ComPtr<wfc::IVectorView<XDOUBLE>> spDoubleVector;

                            ctl::ComPtr<IInspectable> spAttributeValue = pAttributeValue;
                            IFC(spAttributeValue.As(&spDoubleVector));
                            if (spDoubleVector)
                            {
                                IFC(spDoubleVector->get_Size(&size));
                            }

                            if (size == 0)
                            {
                                pReturnVal->SetNull();
                            }
                            else
                            {
                                XDOUBLE* prgTabAttributes = new XDOUBLE[size];
                                for (unsigned i = 0; i < size; i++)
                                {
                                    XDOUBLE value = 0;
                                    IFC(spDoubleVector->GetAt(i, &value));
                                    prgTabAttributes[i] = value;
                                }
                                pReturnVal->SetDoubleArray(size, prgTabAttributes);
                            }
                        }
                        break;
                    case AutomationTextAttributesEnum::AnnotationTypesAttribute:
                        {
                            unsigned size = 0;
                            ctl::ComPtr<wfc::IVectorView<XINT32>> spIntVector;

                            ctl::ComPtr<IInspectable> spAttributeValue = pAttributeValue;
                            IFC(spAttributeValue.As(&spIntVector));
                            if (spIntVector)
                            {
                                IFC(spIntVector->get_Size(&size));
                            }

                            if (size == 0)
                            {
                                pReturnVal->SetNull();
                            }
                            else
                            {
                                XINT32* prgTabs = new XINT32[size];
                                for (unsigned i = 0; i < size; i++)
                                {
                                    XINT32 value = 0;
                                    IFC(spIntVector->GetAt(i, &value));
                                    prgTabs[i] = value;
                                }
                                pReturnVal->SetSignedArray(size, prgTabs);
                            }
                        }
                        break;
                    case AutomationTextAttributesEnum::AnnotationObjectsAttribute:
                        {
                            ctl::ComPtr<wfc::IVectorView<xaml_automation_peers::AutomationPeer*>> spAutomationPeerVector;
                            ctl::ComPtr<AutomationPeerCollection> spCollection;
                            unsigned size = 0;
                            CDependencyObject* pCDO = nullptr;

                            ctl::ComPtr<IInspectable> spAttributeValue = pAttributeValue;
                            IFC(spAttributeValue.As(&spAutomationPeerVector));

                            if (spAutomationPeerVector)
                            {
                                IFC(spAutomationPeerVector->get_Size(&size));
                            }

                            if (size == 0)
                            {
                                pReturnVal->SetIUnknownNoRef(nullptr);
                            }
                            else
                            {
                                // AGCore doesn't know about DXAML VectorViews. So, we have to marshal to a CAutomationPeerCollection.
                                IFC(ctl::make(&spCollection));
                                for (unsigned i = 0; i < size; ++i)
                                {
                                    ctl::ComPtr<IAutomationPeer> spPeer;
                                    IFC(spAutomationPeerVector->GetAt(i, &spPeer));
                                    IFC(spCollection->Append(spPeer.Get()));
                                }
                                pCDO = spCollection->GetHandle();
                                AddRefInterface(pCDO);
                                pReturnVal->SetPointer(pCDO);
                            }
                        }
                        break;

                    case AutomationTextAttributesEnum::FontNameAttribute:
                    case AutomationTextAttributesEnum::StyleNameAttribute:
                    case AutomationTextAttributesEnum::FontSizeAttribute:
                    case AutomationTextAttributesEnum::IndentationFirstLineAttribute:
                    case AutomationTextAttributesEnum::IndentationLeadingAttribute:
                    case AutomationTextAttributesEnum::IndentationTrailingAttribute:
                    case AutomationTextAttributesEnum::IsHiddenAttribute:
                    case AutomationTextAttributesEnum::IsItalicAttribute:
                    case AutomationTextAttributesEnum::IsReadOnlyAttribute:
                    case AutomationTextAttributesEnum::MarginBottomAttribute:
                    case AutomationTextAttributesEnum::MarginLeadingAttribute:
                    case AutomationTextAttributesEnum::MarginTopAttribute:
                    case AutomationTextAttributesEnum::MarginTrailingAttribute:
                    case AutomationTextAttributesEnum::IsSubscriptAttribute:
                    case AutomationTextAttributesEnum::IsSuperscriptAttribute:
                    case AutomationTextAttributesEnum::IsActiveAttribute:
                    default:
                        IFC(BoxObjectValueHelper(pReturnVal, nullptr, pAttributeValue, &buffer, &pDO, TRUE));
                        break;
                    }
                }
                break;
            case 8: // GetBoundingRectangles
                IFCPTR(pReturnVal);
                IFC(pTextRangeProvider->GetBoundingRectangles(&nLength, &pDoubleArray));
                pReturnVal->SetSigned(nLength);
                break;
            case 9: // GetBoundingRectangles
                IFCPTR(pReturnVal);
                IFC(pTextRangeProvider->GetBoundingRectangles(&nLength, &pDoubleArray));
                nLength = MIN(pReturnVal->GetArrayElementCount(), nLength);
                pReturnVal->SetArrayElementCount(nLength);
                pDoubleCurrent = (DOUBLE*)pReturnVal->m_pvValue;
                for (UINT i = 0; i < nLength; i++)
                {
                    // We use the Min above to ensure that the number of elements we process is within the range of both
                    // arrays.  The logic is a little too tricky for PREfast.  Here we let PREfast know 'i' is in range.
                    _Analysis_assume_(i == 0);
                    pDoubleCurrent[i] = pDoubleArray[i];
                }
                break;
            case 10: // GetChildren
                IFCPTR(pReturnVal);
                IFC(pTextRangeProvider->GetChildren(&nLength, &pChildren));
                pReturnVal->SetSigned(nLength);
                break;
            case 11: // GetChildren
                IFCPTR(pReturnVal);
                IFC(pTextRangeProvider->GetChildren(&nLength, &pChildren));
                pReturnVal->SetArrayElementCount(MIN(pReturnVal->GetArrayElementCount(), nLength));
                ppCurrentItem = (void**)pReturnVal->m_pvValue;
                for (XUINT32 i = 0; i < pReturnVal->GetArrayElementCount(); i++)
                {
                    _Analysis_assume_(i == 0);
                    IFC(static_cast<IRawElementProviderSimple*>(pChildren[i])->GetAutomationPeer(&pAP));
                    if (pAP)
                    {
                        // We use the Min above to ensure that the number of elements we process is within the range of both
                        // arrays.  The logic is a little too tricky for PREfast.  Here we let PREfast know 'i' is in range.
                        ppCurrentItem[i] = static_cast<AutomationPeer*>(pAP)->GetHandle();
                    }
                    ReleaseInterface(pAP);
                }
                break;
            case 12: // GetEnclosingElement
                IFCPTR(pReturnVal);
                IFC(pTextRangeProvider->GetEnclosingElement(&pRawElementProvider));
                IFC(static_cast<IRawElementProviderSimple*>(pRawElementProvider)->GetAutomationPeer(&pAP));
                if (pAP)
                {
                    pReturnVal->m_pdoValue = static_cast<AutomationPeer*>(pAP)->GetHandle();
                }
                else
                {
                    pReturnVal->m_pdoValue = NULL;
                }
                break;
            case 13: // GetText
                IFCPTR(pReturnVal);
                IFCPTR(pValueParams);
                // -1 for the length of Text is a supported value, hence we have to include it.
                // When length asked for is -1, expected result is whole Text.
                if(pValueParams[0].m_iValue < -1 )
                {
                    IFC(ErrorHelper::OriginateError(AgError(UIA_GETTEXT_OUTOFRANGE_LENGTH)));
                }
                IFC(pTextRangeProvider->GetText(pValueParams[0].m_iValue, strValue.GetAddressOf()));
                psTextTemp = strValue.GetRawBuffer(&pcTextTemp);
                if(pValueParams[0].m_iValue != -1)
                {
                    pcTextTemp = MIN(static_cast<UINT>(pValueParams[0].m_iValue), pcTextTemp);
                }
                pReturnVal->SetSigned(pcTextTemp);
                break;
            case 14: // GetText
                IFCPTR(pReturnVal);
                IFCPTR(pValueParams);
                IFC(pTextRangeProvider->GetText(pValueParams[0].m_iValue, strValue.GetAddressOf()));
                IFC(SetCValueFromString(pReturnVal, strValue.Get()))
                break;
            case 15: // Move
                IFCPTR(pReturnVal);
                IFCPTR(pValueParams);
                textUnit = (xaml_automation::Text::TextUnit)(&pValueParams[0])->m_nValue;
                IFC(pTextRangeProvider->Move(textUnit, pValueParams[1].m_iValue, &iRetVal));
                pReturnVal->SetSigned(iRetVal);
                break;
            case 16: // MoveEndpointByRange
                IFCPTR(pValueParams);
                rangeEndpoint = (xaml_automation::Text::TextPatternRangeEndpoint)(&pValueParams[0])->m_nValue;
                IFC(pCore->GetPeer((&pValueParams[1])->m_pdoValue, &pMOROrTarget));
                IFC(ctl::do_query_interface(pMOROrTargetAsIInspectable, pMOROrTarget));
                CValueBoxer::UnwrapExternalObjectReferenceIfPresent(pMOROrTargetAsIInspectable, &pTargetAsIInspectable);
                IFCPTR(pTargetAsIInspectable);
                IFC(ctl::do_query_interface(pTextRangeProviderOther, pTargetAsIInspectable));
                targetRangeEndpoint = (xaml_automation::Text::TextPatternRangeEndpoint)(&pValueParams[2])->m_nValue;
                IFC(pTextRangeProvider->MoveEndpointByRange(rangeEndpoint, pTextRangeProviderOther, targetRangeEndpoint));
                break;
            case 17: // MoveEndpointByUnit
                IFCPTR(pReturnVal);
                IFCPTR(pValueParams);
                rangeEndpoint = (xaml_automation::Text::TextPatternRangeEndpoint)(&pValueParams[0])->m_nValue;
                textUnit = (xaml_automation::Text::TextUnit)(&pValueParams[1])->m_nValue;
                IFC(pTextRangeProvider->MoveEndpointByUnit(rangeEndpoint, textUnit, (&pValueParams[2])->m_iValue /*count*/, &iRetVal));
                pReturnVal->SetSigned(iRetVal);
                break;
            case 18: // RemoveFromSelection
                IFC(pTextRangeProvider->RemoveFromSelection());
                break;
            case 19: // ScrollIntoView
                IFCPTR(pValueParams);
                alignToTop = (&pValueParams[0])->m_nValue != 0;
                IFC(pTextRangeProvider->ScrollIntoView(alignToTop));
                break;
            case 20: // Select
                IFC(pTextRangeProvider->Select());
                break;
            case 21: // ShowContextMenu
                pTextRangeProvider2 = ctl::query_interface<xaml_automation::Provider::ITextRangeProvider2>(pTextRangeProvider);
                if (pTextRangeProvider2)
                {
                    IFC(pTextRangeProvider2->ShowContextMenu());
                }
                else
                {
                    IFC(E_FAIL);
                }
                break;
            case 22: // IsITextRangeProvider2
                pTextRangeProvider2 = ctl::query_interface<xaml_automation::Provider::ITextRangeProvider2>(pTextRangeProvider);
                if (pTextRangeProvider2)
                {
                    IFC(BoxValueHelper(pReturnVal, TRUE));
                }
                else
                {
                    IFC(BoxValueHelper(pReturnVal, FALSE));
                }
                break;
            default:
                IFC(E_FAIL);
        }
    }

Cleanup:
    ctl::release_interface(pMOROrTarget);
    ctl::release_interface(pPatternObject);
    ReleaseInterface(pMOROrTargetAsIInspectable);
    ReleaseInterface(pTargetAsIInspectable);
    ReleaseInterface(pAP);
    ReleaseInterface(pTextRangeProvider);
    ReleaseInterface(pTextRangeProvider2);
    ReleaseInterface(pTextRangeProviderOther);
    ReleaseInterface(pRawElementProvider);
    ReleaseInterface(pAttributeValue);
    if(pChildren)
    {
        for(XUINT32 i=0; i<nLength; i++)
        {
            ReleaseInterface(pChildren[i]);
        }
    }
    DELETE_ARRAY(pChildren);
    DELETE_ARRAY(pDoubleArray);
    ctl::release_interface(pDO);
    RRETURN(hr);
}

// Invoke the specified pattern methods
_Check_return_ HRESULT AutomationPeer::UIAPatternInvoke(
    _In_ CDependencyObject* nativeTarget,
    _In_ UIAXcp::APPatternInterface ePatternInterface,
    _In_ XINT32 eFunction,
    _In_ XINT32 cParams,
    _In_ void* pvParams,
    _Out_ Automation::CValue* pReturnVal) noexcept
{
    HRESULT hr = S_OK;
    DependencyObject* pMOROrTarget = NULL;
    IInspectable* pMOROrTargetAsIInspectable = NULL;
    IInspectable* pTargetAsIInspectable = NULL;
    Automation::CValue* pValueParams = NULL;
    wrl_wrappers::HString strValue;
    HSTRING* phsArray = NULL;
    INT iValue = 0;
    DOUBLE dValue = 0.0;
    BOOLEAN bValue = FALSE;
    LPCWSTR psTextTemp = NULL;
    XUINT32 pcTextTemp = 0;
    UINT nLength = 0;
    INT* pIntArray = NULL;
    wu::Color color;

    void** ppCurrentItem = NULL;
    xaml_automation_peers::IAutomationPeer* pAP = NULL;
    xaml_automation::Provider::IIRawElementProviderSimple* pProvider = NULL;
    xaml_automation::Provider::IInvokeProvider* pInvokeProvider = NULL;
    xaml_automation::Provider::IDockProvider* pDockProvider = NULL;
    xaml_automation::Provider::IExpandCollapseProvider* pExpandCollapseProvider = NULL;
    xaml_automation::Provider::IValueProvider* pValueProvider = NULL;
    xaml_automation::Provider::IGridItemProvider* pGridItemProvider = NULL;
    xaml_automation::Provider::IGridProvider* pGridProvider = NULL;
    xaml_automation::Provider::IMultipleViewProvider* pMultipleViewProvider = NULL;
    xaml_automation::Provider::IRangeValueProvider* pRangeValueProvider = NULL;
    xaml_automation::Provider::IScrollItemProvider* pScrollItemProvider = NULL;
    xaml_automation::Provider::IScrollProvider* pScrollProvider = NULL;
    xaml_automation::Provider::ISelectionItemProvider* pSelectionItemProvider = NULL;
    xaml_automation::Provider::ISelectionProvider* pSelectionProvider = NULL;
    xaml_automation::Provider::ITableItemProvider* pTableItemProvider = NULL;
    xaml_automation::Provider::ITableProvider* pTableProvider = NULL;
    xaml_automation::Provider::IToggleProvider* pToggleProvider = NULL;
    xaml_automation::Provider::ITransformProvider* pTransformProvider = NULL;
    xaml_automation::Provider::ITransformProvider2* pTransformProvider2 = NULL;
    xaml_automation::Provider::IVirtualizedItemProvider* pVirtualizedItemProvider = NULL;
    xaml_automation::Provider::IItemContainerProvider* pItemContainerProvider = NULL;
    xaml_automation::Provider::IWindowProvider* pWindowProvider = NULL;
    xaml_automation::Provider::ITextProvider* pTextProvider = NULL;
    xaml_automation::Provider::ITextProvider2* pTextProvider2 = NULL;
    xaml_automation::Provider::ITextChildProvider* pTextChildProvider = NULL;
    xaml_automation::Provider::IAnnotationProvider* pAnnotationProvider = NULL;
    xaml_automation::Provider::ITextRangeProvider* pTextRangeProvider = NULL;
    xaml_automation::Provider::IIRawElementProviderSimple* pRawElementProvider = NULL;
    xaml_automation::Provider::IIRawElementProviderSimple** pSelections = NULL;
    xaml_automation::Provider::IIRawElementProviderSimple** pTableItems = NULL;
    xaml_automation::Provider::IIRawElementProviderSimple** pTableHeaders = NULL;
    xaml_automation::Provider::IIRawElementProviderSimple** pAnnotationObjects = NULL;
    xaml_automation::Provider::ITextRangeProvider** pTextRangeSelections = NULL;
    xaml_automation::Provider::IDragProvider* pDragProvider = NULL;
    xaml_automation::Provider::IDropTargetProvider* pDropTargetProvider = NULL;
    xaml_automation::Provider::IObjectModelProvider* pObjectModelProvider = NULL;
    xaml_automation::Provider::ISpreadsheetProvider* pSpreadsheetProvider = NULL;
    xaml_automation::Provider::ISpreadsheetItemProvider* pSpreadsheetItemProvider = NULL;
    xaml_automation::Provider::IStylesProvider* pStylesProvider = NULL;
    xaml_automation::Provider::ISynchronizedInputProvider* pSynchronizedInputProvider = NULL;
    ctl::ComPtr<xaml_automation::Provider::ITextEditProvider> spTextEditProvider = NULL;
    ctl::ComPtr<xaml_automation::Provider::ICustomNavigationProvider> spCustomNavigationProvider = NULL;

    // Some functions return a reference to an array that should not be deleted.
    xaml_automation::Provider::IIRawElementProviderSimple** pNoDeleteRepsArray = NULL;
    xaml_automation::AnnotationType * pAnnotationType = NULL;
    UIAXcp::AnnotationType* pCurrentAnnotationType = NULL;

    DependencyObject* pStartAfter = NULL;
    DependencyObject* pChildDO = NULL;
    DependencyObject* pAnnotationDO = NULL;
    DependencyObject* pPatternObject = NULL;
    IAutomationPeer* pStartAfterAsAutomationPeer = NULL;
    xaml_automation::Provider::IIRawElementProviderSimple* pStartAfterAsRaw = NULL;
    xaml_automation::IAutomationProperty* pAutomationProperty = NULL;
    IInspectable* propertyValueAsInspectable = NULL;
    IInspectable* objetModelInspectable = NULL;
    IUnknown* objetModelIUnknown = NULL;
    XPOINTF* pPoint = NULL;
    wf::Point screenPoint;
    xaml_automation::SupportedTextSelection supportedTextSelection;
    xaml_automation_peers::AutomationNavigationDirection navigationDirection;
    DXamlCore* pCore = DXamlCore::GetCurrent();

    // Get the target AutomationPeer
    IFC(pCore->GetPeer(nativeTarget, &pMOROrTarget));
    IFC(ctl::do_query_interface(pMOROrTargetAsIInspectable, pMOROrTarget));
    CValueBoxer::UnwrapExternalObjectReferenceIfPresent(pMOROrTargetAsIInspectable, &pTargetAsIInspectable);
    IFCPTR(pTargetAsIInspectable);

    // Assign CValue parameters
    pValueParams = (Automation::CValue*)pvParams;

    switch (ePatternInterface)
    {
        case xaml_automation_peers::PatternInterface_Invoke:
            IFC(ctl::do_query_interface(pInvokeProvider, pTargetAsIInspectable));
            if (pInvokeProvider)
            {
                if (eFunction == 0)
                {
                    IFC(pInvokeProvider->Invoke());
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_Dock:
            IFC(ctl::do_query_interface(pDockProvider, pTargetAsIInspectable));
            if (pDockProvider)
            {
                xaml_automation::DockPosition eDockPosition;
                switch (eFunction)
                {
                    case 0:
                        IFC(pDockProvider->get_DockPosition(&eDockPosition));
                        IFC(BoxEnumValueHelper(pReturnVal, eDockPosition));
                        break;
                    case 1:
                        IFC(UnboxEnumValueHelper(&pValueParams[0], nullptr, (UINT*)&eDockPosition));
                        IFC(pDockProvider->SetDockPosition(eDockPosition));
                        break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_ExpandCollapse:
            IFC(ctl::do_query_interface(pExpandCollapseProvider, pTargetAsIInspectable));
            if (pExpandCollapseProvider)
            {
                switch (eFunction)
                {
                    case 0:
                        IFC(pExpandCollapseProvider->Collapse());
                        break;
                    case 1:
                        IFC(pExpandCollapseProvider->Expand());
                        break;
                    case 2:
                        xaml_automation::ExpandCollapseState eExpandCollapseState;
                        IFC(pExpandCollapseProvider->get_ExpandCollapseState(&eExpandCollapseState));
                        IFC(BoxEnumValueHelper(pReturnVal, eExpandCollapseState));
                        break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_Value:
            IFC(ctl::do_query_interface(pValueProvider, pTargetAsIInspectable));
            if (pValueProvider)
            {
                switch (eFunction)
                {
                    case 0:
                        IFC(pValueProvider->get_IsReadOnly(&bValue));
                        IFC(BoxValueHelper(pReturnVal, bValue));
                        break;
                    case 1:
                        IFC(GetStringFromCValue(&pValueParams[0], strValue.GetAddressOf()));
                        IFC(pValueProvider->SetValue(strValue.Get()));
                        break;
                    case 2:
                        IFC(pValueProvider->get_Value(strValue.GetAddressOf()));
                        psTextTemp = strValue.GetRawBuffer(&pcTextTemp);
                        IFC(BoxValueHelper(pReturnVal, (INT)pcTextTemp));
                        break;
                    case 3:
                        IFC(pValueProvider->get_Value(strValue.GetAddressOf()));
                        IFC(SetCValueFromString(pReturnVal, strValue.Get()));
                        break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_GridItem:
            IFC(ctl::do_query_interface(pGridItemProvider, pTargetAsIInspectable));
            if (pGridItemProvider)
            {
                switch (eFunction)
                {
                    case 0:
                        IFC(pGridItemProvider->get_Column(&iValue));
                        IFC(BoxValueHelper(pReturnVal, iValue));
                        break;
                    case 1:
                        IFC(pGridItemProvider->get_ColumnSpan(&iValue));
                        IFC(BoxValueHelper(pReturnVal, iValue));
                        break;
                    case 2:
                        IFC(pGridItemProvider->get_ContainingGrid(&pProvider));
                        if (pProvider)
                        {
                            IFC((static_cast<IRawElementProviderSimple*>(pProvider))->GetAutomationPeer(&pAP));
                            if(pAP)
                            {
                                pReturnVal->m_pdoValue = static_cast<AutomationPeer*>(pAP)->GetHandle();
                            }
                        }
                        break;
                    case 3:
                        IFC(pGridItemProvider->get_Row(&iValue));
                        IFC(BoxValueHelper(pReturnVal, iValue));
                        break;
                    case 4:
                        IFC(pGridItemProvider->get_RowSpan(&iValue));
                        IFC(BoxValueHelper(pReturnVal, iValue));
                        break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_Grid:
            IFC(ctl::do_query_interface(pGridProvider, pTargetAsIInspectable));
            if (pGridProvider)
            {
                switch (eFunction)
                {
                    case 0:
                        IFC(pGridProvider->get_ColumnCount(&iValue));
                        IFC(BoxValueHelper(pReturnVal, iValue));
                        break;
                    case 1:
                        IFC(pGridProvider->GetItem((&pValueParams[0])->m_iValue, (&pValueParams[1])->m_iValue, &pProvider));
                        if (pProvider)
                        {
                            IFC((static_cast<IRawElementProviderSimple*>(pProvider))->GetAutomationPeer(&pAP));

                            if(pAP)
                            {
                                pReturnVal->m_pdoValue = static_cast<AutomationPeer*>(pAP)->GetHandle();
                            }
                        }
                        break;
                    case 2:
                        IFC(pGridProvider->get_RowCount(&iValue));
                        IFC(BoxValueHelper(pReturnVal, iValue));
                        break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_MultipleView:
            IFC(ctl::do_query_interface(pMultipleViewProvider, pTargetAsIInspectable));
            if (pMultipleViewProvider)
            {
                INT* pIntCurrent = NULL;
                switch (eFunction)
                {
                    case 0:
                        IFC(pMultipleViewProvider->get_CurrentView(&iValue));
                        IFC(BoxValueHelper(pReturnVal, iValue));
                        break;
                    case 1:
                        IFC(pMultipleViewProvider->GetSupportedViews(&nLength, &pIntArray));
                        IFC(BoxValueHelper(pReturnVal, (INT)nLength));
                        break;
                    case 2:
                        IFC(pMultipleViewProvider->GetSupportedViews(&nLength, &pIntArray));
                        nLength = MIN(pReturnVal->GetArrayElementCount(), nLength);
                        pReturnVal->SetArrayElementCount(nLength);
                        pIntCurrent = (INT*)pReturnVal->m_pvValue;
                        for (UINT i = 0; i < nLength; i++)
                        {
                            // We use the Min above to ensure that the number of elements we process is within the range of both
                            // arrays.  The logic is a little too tricky for PREfast.  Here we let PREfast know 'i' is in range.
                            _Analysis_assume_(i == 0);
                            pIntCurrent[i] = pIntArray[i];
                        }
                        break;
                    case 3:
                        IFC(pMultipleViewProvider->GetViewName((&pValueParams[0])->m_iValue, strValue.GetAddressOf()));
                        psTextTemp = strValue.GetRawBuffer(&pcTextTemp);
                        IFC(BoxValueHelper(pReturnVal, (INT)pcTextTemp));
                        break;
                    case 4:
                        IFC(pMultipleViewProvider->GetViewName((&pValueParams[0])->m_iValue, strValue.GetAddressOf()));
                        IFC(SetCValueFromString(pReturnVal, strValue.Get()));
                        break;
                    case 5:
                        IFC(pMultipleViewProvider->SetCurrentView((&pValueParams[0])->m_iValue));
                        break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_RangeValue:
            IFC(ctl::do_query_interface(pRangeValueProvider, pTargetAsIInspectable));
            if (pRangeValueProvider)
            {
                switch (eFunction)
                {
                    case 0:
                        IFC(pRangeValueProvider->get_IsReadOnly(&bValue));
                        IFC(BoxValueHelper(pReturnVal, bValue));
                        break;
                    case 1:
                        IFC(pRangeValueProvider->get_LargeChange(&dValue));
                        IFC(BoxValueHelper(pReturnVal, dValue));
                        break;
                    case 2:
                        IFC(pRangeValueProvider->get_Maximum(&dValue));
                        IFC(BoxValueHelper(pReturnVal, dValue));
                        break;
                    case 3:
                        IFC(pRangeValueProvider->get_Minimum(&dValue));
                        IFC(BoxValueHelper(pReturnVal, dValue));
                        break;
                    case 4:
                        IFC(pRangeValueProvider->SetValue((&pValueParams[0])->m_eValue));
                        break;
                    case 5:
                        IFC(pRangeValueProvider->get_SmallChange(&dValue));
                        IFC(BoxValueHelper(pReturnVal, dValue));
                        break;
                    case 6:
                        IFC(pRangeValueProvider->get_Value(&dValue));
                        IFC(BoxValueHelper(pReturnVal, dValue));
                        break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_ScrollItem:
            IFC(ctl::do_query_interface(pScrollItemProvider, pTargetAsIInspectable));
            if (pScrollItemProvider)
            {
                switch (eFunction)
                {
                    case 0:
                        IFC(pScrollItemProvider->ScrollIntoView());
                        break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_Scroll:
            IFC(ctl::do_query_interface(pScrollProvider, pTargetAsIInspectable));
            if (pScrollProvider)
            {
                switch (eFunction)
                {
                    case 0:
                        IFC(pScrollProvider->get_HorizontallyScrollable(&bValue));
                        IFC(BoxValueHelper(pReturnVal, bValue));
                        break;
                    case 1:
                        IFC(pScrollProvider->get_HorizontalScrollPercent(&dValue));
                        IFC(BoxValueHelper(pReturnVal, dValue));
                        break;
                    case 2:
                        IFC(pScrollProvider->get_HorizontalViewSize(&dValue));
                        IFC(BoxValueHelper(pReturnVal, dValue));
                        break;
                    case 3:
                        IFC(pScrollProvider->Scroll((xaml_automation::ScrollAmount)(&pValueParams[0])->m_nValue, (xaml_automation::ScrollAmount)(&pValueParams[1])->m_nValue));
                        break;
                    case 4:
                        IFC(pScrollProvider->SetScrollPercent((&pValueParams[0])->m_eValue, (&pValueParams[1])->m_eValue));
                        break;
                    case 5:
                        IFC(pScrollProvider->get_VerticallyScrollable(&bValue));
                        IFC(BoxValueHelper(pReturnVal, bValue));
                        break;
                    case 6:
                        IFC(pScrollProvider->get_VerticalScrollPercent(&dValue));
                        IFC(BoxValueHelper(pReturnVal, dValue));
                        break;
                    case 7:
                        IFC(pScrollProvider->get_VerticalViewSize(&dValue));
                        IFC(BoxValueHelper(pReturnVal, dValue));
                        break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_SelectionItem:
            IFC(ctl::do_query_interface(pSelectionItemProvider, pTargetAsIInspectable));
            if (pSelectionItemProvider)
            {
                switch (eFunction)
                {
                    case 0:
                        IFC(pSelectionItemProvider->AddToSelection());
                        break;
                    case 1:
                        IFC(pSelectionItemProvider->get_IsSelected(&bValue));
                        IFC(BoxValueHelper(pReturnVal, bValue));
                        break;
                    case 2:
                        IFC(pSelectionItemProvider->RemoveFromSelection());
                        break;
                    case 3:
                        IFC(pSelectionItemProvider->Select());
                        break;
                    case 4:
                        IFC(pSelectionItemProvider->get_SelectionContainer(&pProvider));
                        if (pProvider)
                        {
                            IFC((static_cast<IRawElementProviderSimple*>(pProvider))->GetAutomationPeer(&pAP));
                            if(pAP)
                            {
                                pReturnVal->m_pdoValue = static_cast<AutomationPeer*>(pAP)->GetHandle();
                            }
                        }
                        break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_Selection:
            IFC(ctl::do_query_interface(pSelectionProvider, pTargetAsIInspectable));
            if (pSelectionProvider)
            {
                switch (eFunction)
                {
                    case 0:
                        IFC(pSelectionProvider->get_CanSelectMultiple(&bValue));
                        IFC(BoxValueHelper(pReturnVal, bValue));
                        break;
                    case 1: // The first method is to get the length of buffer then second call is for copying buffer
                        IFC(pSelectionProvider->GetSelection(&nLength, &pSelections));
                        pReturnVal->SetSigned(nLength);
                        break;
                    case 2:
                        IFC(pSelectionProvider->GetSelection(&nLength, &pSelections));
                        pReturnVal->SetArrayElementCount(MIN(pReturnVal->GetArrayElementCount(), nLength));
                        ppCurrentItem = (void**)pReturnVal->m_pvValue;
                        for (XUINT32 i = 0; i < pReturnVal->GetArrayElementCount(); i++)
                        {
                            _Analysis_assume_(i == 0);
                            if (pSelections[i])
                            {
                                IFC(static_cast<IRawElementProviderSimple*>(pSelections[i])->GetAutomationPeer(&pAP));
                                if (pAP)
                                {
                                    ppCurrentItem[i] = static_cast<AutomationPeer*>(pAP)->GetHandle();
                                }
                            }
                            ReleaseInterface(pAP);
                        }
                        break;
                    case 3:
                        IFC(pSelectionProvider->get_IsSelectionRequired(&bValue));
                        IFC(BoxValueHelper(pReturnVal, bValue));
                        break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_TableItem:
            IFC(ctl::do_query_interface(pTableItemProvider, pTargetAsIInspectable));
            if (pTableItemProvider)
            {
                switch (eFunction)
                {
                    case 0: // The first method is to get the length of buffer then second call is for copying buffer
                        IFC(pTableItemProvider->GetColumnHeaderItems(&nLength, &pTableItems));
                        pReturnVal->SetSigned(nLength);
                        break;
                    case 1:
                        IFC(pTableItemProvider->GetColumnHeaderItems(&nLength, &pTableItems));
                        pReturnVal->SetArrayElementCount(MIN(pReturnVal->GetArrayElementCount(), nLength));
                        ppCurrentItem = (void**)pReturnVal->m_pvValue;
                        for (XUINT32 i = 0; i < pReturnVal->GetArrayElementCount(); i++)
                        {
                            _Analysis_assume_(i == 0);
                            if(pTableItems[i])
                            {
                                IFC(static_cast<IRawElementProviderSimple*>(pTableItems[i])->GetAutomationPeer(&pAP));
                                if (pAP)
                                {
                                    ppCurrentItem[i] = static_cast<AutomationPeer*>(pAP)->GetHandle();
                                }
                            }
                            ReleaseInterface(pAP);
                        }
                        break;
                    case 2: // The first method is to get the length of buffer then second call is for copying buffer
                        IFC(pTableItemProvider->GetRowHeaderItems(&nLength, &pTableItems));
                        pReturnVal->SetSigned(nLength);
                        break;
                    case 3:
                        IFC(pTableItemProvider->GetRowHeaderItems(&nLength, &pTableItems));
                        pReturnVal->SetArrayElementCount(MIN(pReturnVal->GetArrayElementCount(), nLength));
                        ppCurrentItem = (void**)pReturnVal->m_pvValue;
                        for (XUINT32 i = 0; i < pReturnVal->GetArrayElementCount(); i++)
                        {
                            _Analysis_assume_(i == 0);
                            if(pTableItems[i])
                            {
                                IFC(static_cast<IRawElementProviderSimple*>(pTableItems[i])->GetAutomationPeer(&pAP));
                                if (pAP)
                                {
                                    ppCurrentItem[i] = static_cast<AutomationPeer*>(pAP)->GetHandle();
                                }
                            }
                            ReleaseInterface(pAP);
                        }
                        break;
                }
            }
            break;
        case xaml_automation_peers::PatternInterface_Table:
            IFC(ctl::do_query_interface(pTableProvider, pTargetAsIInspectable));
            if (pTableProvider)
            {
                xaml_automation::RowOrColumnMajor eRowOrColumnMajor;
                switch (eFunction)
                {
                    case 0: // The first method is to get the length of buffer then second call is for copying buffer
                        IFC(pTableProvider->GetColumnHeaders(&nLength, &pTableHeaders));
                        pReturnVal->SetSigned(nLength);
                        break;
                    case 1:
                        IFC(pTableProvider->GetColumnHeaders(&nLength, &pTableHeaders));
                        pReturnVal->SetArrayElementCount(MIN(pReturnVal->GetArrayElementCount(), nLength));
                        ppCurrentItem = (void**)pReturnVal->m_pvValue;
                        for (XUINT32 i = 0; i < pReturnVal->GetArrayElementCount(); i++)
                        {
                            _Analysis_assume_(i == 0);
                            if(pTableHeaders[i])
                            {
                                IFC(static_cast<IRawElementProviderSimple*>(pTableHeaders[i])->GetAutomationPeer(&pAP));
                                if (pAP)
                                {
                                    ppCurrentItem[i] = static_cast<AutomationPeer*>(pAP)->GetHandle();
                                }
                            }
                            ReleaseInterface(pAP);
                        }
                        break;
                    case 2: // The first method is to get the length of buffer then second call is for copying buffer
                        IFC(pTableProvider->GetRowHeaders(&nLength, &pTableHeaders));
                        pReturnVal->SetSigned(nLength);
                        break;
                    case 3:
                        IFC(pTableProvider->GetRowHeaders(&nLength, &pTableHeaders));
                        pReturnVal->SetArrayElementCount(MIN(pReturnVal->GetArrayElementCount(), nLength));
                        ppCurrentItem = (void**)pReturnVal->m_pvValue;
                        for (XUINT32 i = 0; i < pReturnVal->GetArrayElementCount(); i++)
                        {
                            _Analysis_assume_(i == 0);
                            if(pTableHeaders[i])
                            {
                                IFC(static_cast<IRawElementProviderSimple*>(pTableHeaders[i])->GetAutomationPeer(&pAP));
                                if (pAP)
                                {
                                    ppCurrentItem[i] = static_cast<AutomationPeer*>(pAP)->GetHandle();
                                }
                            }
                            ReleaseInterface(pAP);
                        }
                        break;
                    case 4:
                        IFC(pTableProvider->get_RowOrColumnMajor(&eRowOrColumnMajor));
                        IFC(BoxEnumValueHelper(pReturnVal, eRowOrColumnMajor));
                        break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_Toggle:
            IFC(ctl::do_query_interface(pToggleProvider, pTargetAsIInspectable));
            if (pToggleProvider)
            {
                xaml_automation::ToggleState eToggleState;
                switch (eFunction)
                {
                    case 0:
                        IFC(pToggleProvider->Toggle());
                        break;
                    case 1:
                        IFC(pToggleProvider->get_ToggleState(&eToggleState));
                        IFC(BoxEnumValueHelper(pReturnVal, eToggleState));
                        break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_Transform:
            IFC(ctl::do_query_interface(pTransformProvider, pTargetAsIInspectable));
            if (pTransformProvider)
            {
                switch (eFunction)
                {
                    case 0:
                        IFC(pTransformProvider->get_CanMove(&bValue));
                        IFC(BoxValueHelper(pReturnVal, bValue));
                        break;
                    case 1:
                        IFC(pTransformProvider->get_CanResize(&bValue));
                        IFC(BoxValueHelper(pReturnVal, bValue));
                        break;
                    case 2:
                        IFC(pTransformProvider->get_CanRotate(&bValue));
                        IFC(BoxValueHelper(pReturnVal, bValue));
                        break;
                    case 3:
                        IFC(pTransformProvider->Move((&pValueParams[0])->m_eValue, (&pValueParams[1])->m_eValue));
                        break;
                    case 4:
                        IFC(pTransformProvider->Resize((&pValueParams[0])->m_eValue, (&pValueParams[1])->m_eValue));
                        break;
                    case 5:
                        IFC(pTransformProvider->Rotate((&pValueParams[0])->m_eValue));
                        break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_Transform2:
            pTransformProvider2 = ctl::query_interface<xaml_automation::Provider::ITransformProvider2>(pTargetAsIInspectable);
            if (pTransformProvider2)
            {
                switch (eFunction)
                {
                    case 0:
                        IFC(pTransformProvider2->get_CanZoom(&bValue));
                        IFC(BoxValueHelper(pReturnVal, bValue));
                        break;
                    case 1:
                        IFC(pTransformProvider2->get_ZoomLevel(&dValue));
                        IFC(BoxValueHelper(pReturnVal, dValue));
                        break;
                    case 2:
                        IFC(pTransformProvider2->get_MaxZoom(&dValue));
                        IFC(BoxValueHelper(pReturnVal, dValue));
                        break;
                    case 3:
                        IFC(pTransformProvider2->get_MinZoom(&dValue));
                        IFC(BoxValueHelper(pReturnVal, dValue));
                        break;
                    case 4:
                        IFC(pTransformProvider2->Zoom((&pValueParams[0])->m_eValue));
                        break;
                    case 5:
                        IFC(pTransformProvider2->ZoomByUnit((xaml_automation::ZoomUnit)(&pValueParams[0])->m_nValue));
                        break;
                    // For IsTransformProvider2, If the object is of type ITransformProvider2 return True
                    case 6:
                        IFC(BoxValueHelper(pReturnVal, TRUE));
                        break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_ItemContainer:
            IFC(ctl::do_query_interface(pItemContainerProvider, pTargetAsIInspectable));
            if (pItemContainerProvider)
            {
                IFC(GetAutomationPropertyFromUIAXcpEnum((UIAXcp::APAutomationProperties)pValueParams[1].m_iValue, &pAutomationProperty));
                IFC(GetPropertyValueFromCValue((UIAXcp::APAutomationProperties)pValueParams[1].m_iValue, pValueParams[2], &propertyValueAsInspectable));
                switch(eFunction)
                {
                    case 0:
                        if(pValueParams[0].m_pdoValue)
                        {
                            IFC(pCore->GetPeer(pValueParams[0].m_pdoValue, &pStartAfter));
                            IFC(ctl::do_query_interface(pStartAfterAsAutomationPeer, pStartAfter));
                            IFC(static_cast<AutomationPeer*>(pStartAfterAsAutomationPeer)->ProviderFromPeer(pStartAfterAsAutomationPeer, &pStartAfterAsRaw));
                        }

                        IFC(pItemContainerProvider->FindItemByProperty(pStartAfterAsRaw, pAutomationProperty, propertyValueAsInspectable, &pProvider));
                        if(pProvider)
                        {
                            IFC(static_cast<IRawElementProviderSimple*>(pProvider)->GetAutomationPeer(&pAP));
                            if(pAP)
                            {
                                pReturnVal->m_pdoValue = static_cast<AutomationPeer*>(pAP)->GetHandle();
                            }
                        }
                        break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_VirtualizedItem:
            IFC(ctl::do_query_interface(pVirtualizedItemProvider, pTargetAsIInspectable));
            if (pVirtualizedItemProvider)
            {
                switch(eFunction)
                {
                    case 0:
                        IFC(pVirtualizedItemProvider->Realize());
                        break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_Window:
            IFC(ctl::do_query_interface(pWindowProvider, pTargetAsIInspectable));
            if (pWindowProvider)
            {
                switch (eFunction)
                {
                    case 0:
                        IFC(pWindowProvider->Close());
                        break;
                    case 1:
                        IFC(pWindowProvider->get_IsModal(&bValue));
                        IFC(BoxValueHelper(pReturnVal, bValue));
                        break;
                    case 2:
                        IFC(pWindowProvider->get_IsTopmost(&bValue));
                        IFC(BoxValueHelper(pReturnVal, bValue));
                        break;
                    case 3:
                        IFC(pWindowProvider->get_Maximizable(&bValue));
                        IFC(BoxValueHelper(pReturnVal, bValue));
                        break;
                    case 4:
                        IFC(pWindowProvider->get_Minimizable(&bValue));
                        IFC(BoxValueHelper(pReturnVal, bValue));
                        break;
                    case 5:
                        IFC(pWindowProvider->SetVisualState((xaml_automation::WindowVisualState)(&pValueParams[0])->m_nValue));
                        break;
                    case 6:
                        IFC(pWindowProvider->WaitForInputIdle((&pValueParams[0])->m_iValue, &bValue));
                        IFC(BoxValueHelper(pReturnVal, bValue));
                        break;
                    case 7:
                        xaml_automation::WindowInteractionState eWindowInteractionState;
                        IFC(pWindowProvider->get_InteractionState(&eWindowInteractionState));
                        IFC(BoxEnumValueHelper(pReturnVal, eWindowInteractionState));
                        break;
                    case 8:
                        xaml_automation::WindowVisualState eWindowVisualState;
                        IFC(pWindowProvider->get_VisualState(&eWindowVisualState));
                        IFC(BoxEnumValueHelper(pReturnVal, eWindowVisualState));
                        break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_Text:
            IFC(ctl::do_query_interface(pTextProvider, pTargetAsIInspectable));
            if(pTextProvider)
            {
                switch(eFunction)
                {
                    case 0: // The first method is to get the length of buffer then second call is for copying buffer
                        IFC(pTextProvider->GetSelection(&nLength, &pTextRangeSelections));
                        pReturnVal->SetSigned(nLength);
                        break;
                    case 1:
                        IFC(pTextProvider->GetSelection(&nLength, &pTextRangeSelections));
                        pReturnVal->SetArrayElementCount(MIN(pReturnVal->GetArrayElementCount(), nLength));
                        ppCurrentItem = (void**)pReturnVal->m_pvValue;
                        for (XUINT32 i = 0; i < pReturnVal->GetArrayElementCount(); i++)
                        {
                            _Analysis_assume_(i == 0);
                            if (pTextRangeSelections[i])
                            {
                                IFC(ExternalObjectReference::ConditionalWrap(pTextRangeSelections[i], &pPatternObject));
                                ppCurrentItem[i] = pPatternObject->GetHandle();

                                // Peg the object; the core is responsible for un-pegging.
                                pPatternObject->PegNoRef();
                            }
                            ctl::release_interface(pPatternObject);
                        }
                        break;
                    case 2:
                        IFC(pTextProvider->GetVisibleRanges(&nLength, &pTextRangeSelections));
                        pReturnVal->SetSigned(nLength);
                        break;
                    case 3:
                        IFC(pTextProvider->GetVisibleRanges(&nLength, &pTextRangeSelections));
                        pReturnVal->SetArrayElementCount(MIN(pReturnVal->GetArrayElementCount(), nLength));
                        ppCurrentItem = (void**)pReturnVal->m_pvValue;
                        for (XUINT32 i = 0; i < pReturnVal->GetArrayElementCount(); i++)
                        {
                            _Analysis_assume_(i == 0);
                            if (pTextRangeSelections[i])
                            {
                                IFC(ExternalObjectReference::ConditionalWrap(pTextRangeSelections[i], &pPatternObject));
                                ppCurrentItem[i] = pPatternObject->GetHandle();

                                // Peg the object; the core is responsible for un-pegging.
                                pPatternObject->PegNoRef();
                            }
                            ctl::release_interface(pPatternObject);
                        }
                        break;

                    case 4:
                        IFC(pCore->GetPeer((&pValueParams[0])->m_pdoValue, &pChildDO));
                        IFC(ctl::do_query_interface(pAP, pChildDO));
                        IFC(static_cast<AutomationPeer*>(pAP)->ProviderFromPeer(pAP, &pRawElementProvider));
                        IFC(pTextProvider->RangeFromChild(pRawElementProvider, &pTextRangeProvider));
                        if (pTextRangeProvider)
                        {
                            IFC(ExternalObjectReference::ConditionalWrap(pTextRangeProvider, &pPatternObject));
                            pReturnVal->m_pdoValue = pPatternObject->GetHandle();

                            // Peg the object; the core is responsible for un-pegging.
                            pPatternObject->PegNoRef();
                        }
                        else
                        {
                            pReturnVal->m_pdoValue = NULL;
                        }
                        break;
                    case 5:
                        IFC((pValueParams[0]).GetPoint(pPoint));
                        screenPoint.X = pPoint->x;
                        screenPoint.Y = pPoint->y;
                        IFC(pTextProvider->RangeFromPoint(screenPoint, &pTextRangeProvider));
                        if (pTextRangeProvider)
                        {
                            IFC(ExternalObjectReference::ConditionalWrap(pTextRangeProvider, &pPatternObject));
                            pReturnVal->m_pdoValue = pPatternObject->GetHandle();

                            // Peg the object; the core is responsible for un-pegging.
                            pPatternObject->PegNoRef();
                        }
                        else
                        {
                            pReturnVal->m_pdoValue = NULL;
                        }
                        break;
                    case 6:
                        IFC(pTextProvider->get_DocumentRange(&pTextRangeProvider));
                        if (pTextRangeProvider)
                        {
                            IFC(ExternalObjectReference::ConditionalWrap(pTextRangeProvider, &pPatternObject));
                            pReturnVal->m_pdoValue = pPatternObject->GetHandle();

                            // Peg the object; the core is responsible for un-pegging.
                            pPatternObject->PegNoRef();
                        }
                        else
                        {
                            pReturnVal->m_pdoValue = NULL;
                        }
                        break;
                    case 7:
                        IFC(pTextProvider->get_SupportedTextSelection(&supportedTextSelection));
                        pReturnVal->m_nValue = (UINT)supportedTextSelection;
                        break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_Text2:
            pTextProvider2 = ctl::query_interface<xaml_automation::Provider::ITextProvider2>(pTargetAsIInspectable);
            if (pTextProvider2)
            {
                switch (eFunction)
                {
                    case 0:
                        IFC(pCore->GetPeer((&pValueParams[0])->m_pdoValue, &pAnnotationDO));
                        IFC(ctl::do_query_interface(pAP, pAnnotationDO));
                        IFC(static_cast<AutomationPeer*>(pAP)->ProviderFromPeer(pAP, &pRawElementProvider));
                        IFC(pTextProvider2->RangeFromAnnotation(pRawElementProvider, &pTextRangeProvider));
                        if (pTextRangeProvider)
                        {
                            IFC(ExternalObjectReference::ConditionalWrap(pTextRangeProvider, &pPatternObject));
                            pReturnVal->m_pdoValue = pPatternObject->GetHandle();

                            // Peg the object; the core is responsible for un-pegging.
                            pPatternObject->PegNoRef();
                        }
                        else
                        {
                            pReturnVal->m_pdoValue = NULL;
                        }
                        break;

                    case 1:
                        IFC(pTextProvider2->GetCaretRange(&bValue, &pTextRangeProvider));
                        IFC(BoxValueHelper(pReturnVal, bValue));
                        if (pTextRangeProvider)
                        {
                            IFC(ExternalObjectReference::ConditionalWrap(pTextRangeProvider, &pPatternObject));
                            pReturnVal->m_pdoValue = pPatternObject->GetHandle();

                            // Peg the object; the core is responsible for un-pegging.
                            pPatternObject->PegNoRef();
                        }
                        else
                        {
                            pReturnVal->m_pdoValue = NULL;
                        }
                        break;
                    // For IsITextProvider2, If the object is of type ITextProvider2 return True
                    case 2:
                        IFC(BoxValueHelper(pReturnVal, TRUE));
                        break;
                }
            }
            else
            {
                if(eFunction == 2)
                {
                    // For IsITextProvider2, If the object is not of type ITextProvider2 return False
                    IFC(BoxValueHelper(pReturnVal, FALSE));
                }
                else
                {
                    IFC(E_FAIL);
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_TextChild:
            IFC(ctl::do_query_interface(pTextChildProvider, pTargetAsIInspectable));
            if(pTextChildProvider)
            {
                switch(eFunction)
                {
                    case 0:
                        IFC(pTextChildProvider->get_TextContainer(&pProvider));
                        if(pProvider)
                        {
                            IFC(static_cast<IRawElementProviderSimple*>(pProvider)->GetAutomationPeer(&pAP));
                            if(pAP)
                            {
                                pReturnVal->m_pdoValue = static_cast<AutomationPeer*>(pAP)->GetHandle();
                            }
                        }
                        break;
                    case 1:
                        IFC(pTextChildProvider->get_TextRange(&pTextRangeProvider));
                        if (pTextRangeProvider)
                        {
                            IFC(ExternalObjectReference::ConditionalWrap(pTextRangeProvider, &pPatternObject));
                            pReturnVal->m_pdoValue = pPatternObject->GetHandle();

                            // Peg the object; the core is responsible for un-pegging.
                            pPatternObject->PegNoRef();
                        }
                        else
                        {
                            pReturnVal->m_pdoValue = NULL;
                        }
                        break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_Annotation:
            IFC(ctl::do_query_interface(pAnnotationProvider, pTargetAsIInspectable));
            if (pAnnotationProvider)
            {
                switch (eFunction)
                {
                    case 0:
                        IFC(pAnnotationProvider->get_AnnotationTypeId(&iValue));
                        IFC(BoxValueHelper(pReturnVal, iValue));
                        break;
                    case 1:
                        IFC(pAnnotationProvider->get_AnnotationTypeName(strValue.GetAddressOf()));
                        psTextTemp = strValue.GetRawBuffer(&pcTextTemp);
                        IFC(BoxValueHelper(pReturnVal, (INT)pcTextTemp));
                        break;
                    case 2:
                        IFC(pAnnotationProvider->get_AnnotationTypeName(strValue.GetAddressOf()));
                        IFC(SetCValueFromString(pReturnVal, strValue.Get()));
                        break;
                    case 3:
                        IFC(pAnnotationProvider->get_Author(strValue.GetAddressOf()));
                        psTextTemp = strValue.GetRawBuffer(&pcTextTemp);
                        IFC(BoxValueHelper(pReturnVal, (INT)pcTextTemp));
                        break;
                    case 4:
                        IFC(pAnnotationProvider->get_Author(strValue.GetAddressOf()));
                        IFC(SetCValueFromString(pReturnVal, strValue.Get()));
                        break;
                    case 5:
                        IFC(pAnnotationProvider->get_DateTime(strValue.GetAddressOf()));
                        psTextTemp = strValue.GetRawBuffer(&pcTextTemp);
                        IFC(BoxValueHelper(pReturnVal, (INT)pcTextTemp));
                        break;
                    case 6:
                        IFC(pAnnotationProvider->get_DateTime(strValue.GetAddressOf()));
                        IFC(SetCValueFromString(pReturnVal, strValue.Get()));
                        break;
                    case 7:
                        IFC(pAnnotationProvider->get_Target(&pProvider));
                        if (pProvider)
                        {
                            IFC((static_cast<IRawElementProviderSimple*>(pProvider))->GetAutomationPeer(&pAP));
                            if(pAP)
                            {
                                pReturnVal->m_pdoValue = static_cast<AutomationPeer*>(pAP)->GetHandle();
                            }
                        }
                        break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_Drag:
            IFC(ctl::do_query_interface(pDragProvider, pTargetAsIInspectable));
            if (pDragProvider)
            {
                switch (eFunction)
                {
                case 0:
                    IFC(pDragProvider->get_IsGrabbed(&bValue));
                    IFC(BoxValueHelper(pReturnVal, bValue));
                    break;
                case 1:
                    IFC(pDragProvider->get_DropEffect(strValue.GetAddressOf()));
                    IFC(BoxValueHelper(pReturnVal, strValue.Get()));
                    break;
                case 2:
                    IFC(pDragProvider->get_DropEffects(&nLength, &phsArray));
                    IFC(BoxArrayOfHStrings(pReturnVal, nLength, phsArray));
                    break;
                case 3:
                    IFC(pDragProvider->GetGrabbedItems(&nLength, &pNoDeleteRepsArray));
                    IFC(BoxArrayOfRawElementProviderSimple(pReturnVal, nLength, pNoDeleteRepsArray));
                    break;
                default:
                    ASSERT(!"Missing function for IDragProvider.");
                    break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_DropTarget:
            IFC(ctl::do_query_interface(pDropTargetProvider, pTargetAsIInspectable));
            if (pDropTargetProvider)
            {
                switch (eFunction)
                {
                case 0:
                    IFC(pDropTargetProvider->get_DropEffect(strValue.GetAddressOf()));
                    IFC(BoxValueHelper(pReturnVal, strValue.Get()));
                    break;
                case 1:
                    IFC(pDropTargetProvider->get_DropEffects(&nLength, &phsArray));
                    IFC(BoxArrayOfHStrings(pReturnVal, nLength, phsArray));
                    break;
                default:
                    ASSERT(!"Missing function for IDropTargetProvider.");
                    break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_ObjectModel:
            IFC(ctl::do_query_interface(pObjectModelProvider, pTargetAsIInspectable));
            if (pObjectModelProvider)
            {
                switch (eFunction)
                {
                case 0:
                    IFC(pObjectModelProvider->GetUnderlyingObjectModel(&objetModelInspectable));
                    IFC(ctl::do_query_interface(objetModelIUnknown, objetModelInspectable));
                    pReturnVal->SetIUnknownNoRef(objetModelIUnknown);
                    objetModelIUnknown = nullptr;
                    break;

                default:
                    ASSERT(!"Missing function for IObjectModelProvider.");
                    break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_Spreadsheet:
            IFC(ctl::do_query_interface(pSpreadsheetProvider, pTargetAsIInspectable));
            if (pSpreadsheetProvider)
            {
                switch (eFunction)
                {
                case 0:
                    IFCPTR(pReturnVal);
                    IFCPTR(pValueParams);
                    IFC(GetStringFromCValue(&pValueParams[0], strValue.GetAddressOf()));
                    IFC(pSpreadsheetProvider->GetItemByName(strValue.Get(), &pProvider));
                    if (pProvider)
                    {
                        IFC((static_cast<IRawElementProviderSimple*>(pProvider))->GetAutomationPeer(&pAP));
                        if(pAP)
                        {
                            pReturnVal->m_pdoValue = static_cast<AutomationPeer*>(pAP)->GetHandle();
                        }
                    }
                    break;
                default:
                    ASSERT(!"Missing function for ISpreadsheetProvider.");
                    break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_SpreadsheetItem:
            IFC(ctl::do_query_interface(pSpreadsheetItemProvider, pTargetAsIInspectable));
            if (pSpreadsheetItemProvider)
            {
                switch (eFunction)
                {
                case 0:
                    IFC(pSpreadsheetItemProvider->get_Formula(strValue.GetAddressOf()));
                    IFC(BoxValueHelper(pReturnVal, strValue.Get()));
                    break;
                case 1: // The first method is to get the length of buffer then second call is for copying buffer
                    IFC(pSpreadsheetItemProvider->GetAnnotationObjects(&nLength, &pAnnotationObjects));
                    pReturnVal->SetSigned(nLength);
                    break;
                case 2:
                    IFC(pSpreadsheetItemProvider->GetAnnotationObjects(&nLength, &pAnnotationObjects));
                    pReturnVal->SetArrayElementCount(MIN(pReturnVal->GetArrayElementCount(), nLength));
                    ppCurrentItem = (void**)pReturnVal->m_pvValue;
                    for (XUINT32 i = 0; i < pReturnVal->GetArrayElementCount(); i++)
                    {
                        _Analysis_assume_(i == 0);
                        if (pAnnotationObjects[i])
                        {
                            IFC(static_cast<IRawElementProviderSimple*>(pAnnotationObjects[i])->GetAutomationPeer(&pAP));
                            if (pAP)
                            {
                                ppCurrentItem[i] = static_cast<AutomationPeer*>(pAP)->GetHandle();
                            }
                        }
                        ReleaseInterface(pAP);
                    }
                    break;
                case 3: // The first method is to get the length of buffer then second call is for copying buffer
                    IFC(pSpreadsheetItemProvider->GetAnnotationTypes(&nLength, &pAnnotationType));
                    pReturnVal->SetSigned(nLength);
                    break;
                case 4:
                    IFC(pSpreadsheetItemProvider->GetAnnotationTypes(&nLength, &pAnnotationType));
                    pReturnVal->SetArrayElementCount(MIN(pReturnVal->GetArrayElementCount(), nLength));
                    pCurrentAnnotationType = (UIAXcp::AnnotationType*)pReturnVal->m_pvValue;
                    for (XUINT32 i = 0; i < pReturnVal->GetArrayElementCount(); i++)
                    {
                        _Analysis_assume_(i == 0);
                        pCurrentAnnotationType[i] = (UIAXcp::AnnotationType)pAnnotationType[i];
                    }
                    break;

                default:
                    ASSERT(!"Missing function for ISpreadsheetItemProvider.");
                    break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_Styles:
            IFC(ctl::do_query_interface(pStylesProvider, pTargetAsIInspectable));
            if (pStylesProvider)
            {
                switch (eFunction)
                {
                case 0:
                    IFC(pStylesProvider->get_ExtendedProperties(strValue.GetAddressOf()));
                    IFC(BoxValueHelper(pReturnVal, strValue.Get()));
                    break;
                case 1:
                    IFC(pStylesProvider->get_FillColor(&color));
                    IFC(BoxValueHelper(pReturnVal, color));
                    break;
                case 2:
                    IFC(pStylesProvider->get_FillPatternColor(&color));
                    IFC(BoxValueHelper(pReturnVal, color));
                    break;
                case 3:
                    IFC(pStylesProvider->get_FillPatternStyle(strValue.GetAddressOf()));
                    IFC(BoxValueHelper(pReturnVal, strValue.Get()));
                    break;
                case 4:
                    IFC(pStylesProvider->get_Shape(strValue.GetAddressOf()));
                    IFC(BoxValueHelper(pReturnVal, strValue.Get()));
                    break;
                case 5:
                    IFC(pStylesProvider->get_StyleId(&iValue));
                    IFC(BoxValueHelper(pReturnVal, iValue));
                    break;
                case 6:
                    IFC(pStylesProvider->get_StyleName(strValue.GetAddressOf()));
                    IFC(BoxValueHelper(pReturnVal, strValue.Get()));
                    break;
                default:
                    ASSERT(!"Missing function for IStylesProvider.");
                    break;
                }
            }
            break;

        case xaml_automation_peers::PatternInterface_SynchronizedInput:
            IFC(ctl::do_query_interface(pSynchronizedInputProvider, pTargetAsIInspectable));
            if (pSynchronizedInputProvider)
            {
                switch (eFunction)
                {
                case 0:
                    IFC(pSynchronizedInputProvider->Cancel());
                    break;
                case 1:
                    IFC(pSynchronizedInputProvider->StartListening((xaml_automation::SynchronizedInputType)pValueParams[0].m_iValue));
                    break;
                default:
                    ASSERT(!"Missing function for ISynchronizedInputProvider.");
                    break;
                }
            }
            break;
        case xaml_automation_peers::PatternInterface_TextEdit:
            {
                ctl::ComPtr<IInspectable> spTargetAsIInspectable(pTargetAsIInspectable);
                spTextEditProvider = spTargetAsIInspectable.AsOrNull<xaml_automation::Provider::ITextEditProvider>();
                if (spTextEditProvider)
                {
                    switch (eFunction)
                    {
                    case 0:
                        IFC(spTextEditProvider->GetActiveComposition(&pTextRangeProvider));
                        if (pTextRangeProvider)
                        {
                            IFC(ExternalObjectReference::ConditionalWrap(pTextRangeProvider, &pPatternObject));
                            pReturnVal->SetObjectAddRef(pPatternObject->GetHandle());

                            // Peg the object; the core is responsible for un-pegging.
                            pPatternObject->PegNoRef();
                        }
                        else
                        {
                            pReturnVal->SetNull();
                        }
                        break;
                    case 1:
                        IFC(spTextEditProvider->GetConversionTarget(&pTextRangeProvider));
                        if (pTextRangeProvider)
                        {
                            IFC(ExternalObjectReference::ConditionalWrap(pTextRangeProvider, &pPatternObject));
                            pReturnVal->SetObjectAddRef(pPatternObject->GetHandle());

                            // Peg the object; the core is responsible for un-pegging.
                            pPatternObject->PegNoRef();
                        }
                        else
                        {
                            pReturnVal->SetNull();
                        }
                        break;
                    default:
                        ASSERT(!"Missing function for ITextEditProvider.");
                        break;
                    }
                }
            }
            break;
        case xaml_automation_peers::PatternInterface_CustomNavigation:
            {
                ctl::ComPtr<IInspectable> spTargetAsIInspectable(pTargetAsIInspectable);
                ctl::ComPtr<IInspectable> spResult;
                xref_ptr<::CDependencyObject> spReturnAPAsDO;
                ctl::ComPtr<IUnknown> spReturnIREPFAsUnk;
                spCustomNavigationProvider = spTargetAsIInspectable.AsOrNull<xaml_automation::Provider::ICustomNavigationProvider>();
                if (spCustomNavigationProvider)
                {
                    switch (eFunction)
                    {
                    case 0:
                        navigationDirection = (xaml_automation_peers::AutomationNavigationDirection)(&pValueParams[0])->m_nValue;
                        IFC(spCustomNavigationProvider->NavigateCustom(navigationDirection, &spResult));
                        if (spResult)
                        {
                            RetrieveNativeNodeOrAPFromIInspectable(spResult.Get(), spReturnAPAsDO.ReleaseAndGetAddressOf(), &spReturnIREPFAsUnk);
                        }
                        IFCEXPECT(2 == pReturnVal->GetArrayElementCount());
                        ppCurrentItem = (void**)pReturnVal->m_pvValue;
                        ppCurrentItem[0] = spReturnAPAsDO.detach();
                        ppCurrentItem[1] = spReturnIREPFAsUnk.Detach();
                        break;
                    default:
                        ASSERT(!"Missing function for ICustomNavigationProvider.");
                        break;
                    }
                }
            }
            break;

        default:
            ASSERT(!"Unknown pattern interface.");
            break;
    }


Cleanup:
    ctl::release_interface(pMOROrTarget);
    ctl::release_interface(pStartAfter);
    ctl::release_interface(pChildDO);
    ctl::release_interface(pAnnotationDO);
    ctl::release_interface(pPatternObject);
    ReleaseInterface(pAP);
    ReleaseInterface(pStartAfterAsAutomationPeer);
    ReleaseInterface(pStartAfterAsRaw);
    ReleaseInterface(pAutomationProperty);
    ReleaseInterface(propertyValueAsInspectable);
    ReleaseInterface(objetModelInspectable);
    ReleaseInterface(pProvider);
    ReleaseInterface(pRawElementProvider);
    ReleaseInterface(pMOROrTargetAsIInspectable);
    ReleaseInterface(pTargetAsIInspectable);
    ReleaseInterface(pInvokeProvider);
    ReleaseInterface(pDockProvider);
    ReleaseInterface(pExpandCollapseProvider);
    ReleaseInterface(pValueProvider);
    ReleaseInterface(pGridItemProvider);
    ReleaseInterface(pGridProvider);
    ReleaseInterface(pMultipleViewProvider);
    ReleaseInterface(pRangeValueProvider);
    ReleaseInterface(pScrollItemProvider);
    ReleaseInterface(pScrollProvider);
    ReleaseInterface(pSelectionItemProvider);
    ReleaseInterface(pSelectionProvider);
    ReleaseInterface(pTableItemProvider);
    ReleaseInterface(pTableProvider);
    ReleaseInterface(pToggleProvider);
    ReleaseInterface(pTransformProvider);
    ReleaseInterface(pVirtualizedItemProvider);
    ReleaseInterface(pItemContainerProvider);
    ReleaseInterface(pWindowProvider);
    ReleaseInterface(pTextProvider);
    ReleaseInterface(pTextProvider2);
    ReleaseInterface(pTextChildProvider);
    ReleaseInterface(pTextRangeProvider);
    ReleaseInterface(pRawElementProvider);
    ReleaseInterface(pDragProvider);
    ReleaseInterface(pDropTargetProvider);
    ReleaseInterface(pTransformProvider2);
    ReleaseInterface(pObjectModelProvider);
    ReleaseInterface(pSpreadsheetProvider);
    ReleaseInterface(pSpreadsheetItemProvider);
    ReleaseInterface(pStylesProvider);
    ReleaseInterface(pSynchronizedInputProvider);

    DELETE_ARRAY(pAnnotationType);
    if(pSelections)
    {
        for(XUINT32 i=0; i<nLength; i++)
        {
            ReleaseInterface(pSelections[i]);
        }
    }
    DELETE_ARRAY(pSelections);
    if(pAnnotationObjects)
    {
        for(XUINT32 i=0; i<nLength; i++)
        {
            ReleaseInterface(pAnnotationObjects[i]);
        }
    }
    DELETE_ARRAY(pAnnotationObjects);
    if(pTableItems)
    {
        for(XUINT32 i=0; i<nLength; i++)
        {
            ReleaseInterface(pTableItems[i]);
        }
    }
    DELETE_ARRAY(pTableItems);
    if(pTableHeaders)
    {
        for(XUINT32 i=0; i<nLength; i++)
        {
            ReleaseInterface(pTableHeaders[i]);
        }
    }
    DELETE_ARRAY(pTableHeaders);
    if(pTextRangeSelections)
    {
        for(XUINT32 i=0; i<nLength; i++)
        {
            ReleaseInterface(pTextRangeSelections[i]);
        }
    }
    DELETE_ARRAY(pTextRangeSelections);
    if (phsArray)
    {
        for(XUINT32 i=0; i<nLength; i++)
        {
            DELETE_STRING(phsArray[i]);
        }
    }
    DELETE_ARRAY(phsArray);
    DELETE_ARRAY(pIntArray);
    RRETURN(hr);
}

// Callback from Core to Notify Owner to realse AP as no UIA client is holding on to it.
_Check_return_ HRESULT AutomationPeer::NotifyNoUIAClientObjectForAP(_In_ CDependencyObject* nativeTarget)
{
    HRESULT hr = S_OK;
    DependencyObject* pTarget = NULL;
    xaml_automation_peers::IAutomationPeer* pAP = NULL;

    IFCPTR(nativeTarget);
    IFC(DXamlCore::GetCurrent()->TryGetPeer(nativeTarget, &pTarget));
    if(pTarget)
    {
        IFC(ctl::do_query_interface(pAP, pTarget));
        IFCPTR(pAP);
        (static_cast<AutomationPeer*>(pAP))->NotifyNoUIAClientObjectToOwner();
    }

Cleanup:
    ctl::release_interface(pTarget);
    ReleaseInterface(pAP);
    RRETURN(hr);
}

// Callback from Core to generate EventsSource for this AP if one exist.
_Check_return_ HRESULT AutomationPeer::GenerateAutomationPeerEventsSource(_In_ CDependencyObject* nativeTarget, _In_ CDependencyObject* nativeTargetParent)
{
    HRESULT hr = S_OK;
    DependencyObject* pTarget = NULL;
    DependencyObject* pTargetParent = NULL;
    xaml_automation_peers::IAutomationPeer* pAP = NULL;
    xaml_automation_peers::IAutomationPeer* pAPParent = NULL;
    DXamlCore* pCore = DXamlCore::GetCurrent();

    IFCPTR(nativeTarget);
    IFCPTR(nativeTargetParent);

    IFC(pCore->TryGetPeer(nativeTarget, &pTarget));
    IFC(pCore->TryGetPeer(nativeTargetParent, &pTargetParent));
    if(pTarget && pTargetParent)
    {
        IFC(ctl::do_query_interface(pAP, pTarget));
        IFC(ctl::do_query_interface(pAPParent, pTargetParent));
        IFCPTR(pAP);
        IFCPTR(pAPParent);
        IFC((static_cast<AutomationPeer*>(pAP))->GenerateAutomationPeerEventsSource(pAPParent));
    }

Cleanup:
    ctl::release_interface(pTarget);
    ctl::release_interface(pTargetParent);
    ReleaseInterface(pAP);
    ReleaseInterface(pAPParent);
    RRETURN(hr);
}

void AutomationPeer::RetrieveNativeNodeOrAPFromIInspectable(
    _In_ IInspectable* pAccessibleNode,
    _Outptr_result_maybenull_ ::CDependencyObject** ppReturnAPAsDO,
    _Outptr_result_maybenull_ IUnknown** ppReturnIREPFAsUnk)
{
    ctl::ComPtr<IInspectable> spAccessibleNode(pAccessibleNode);
    ctl::ComPtr<IAutomationPeer> spAP;
    ctl::ComPtr<IUnknown> spIREPFAsUnk;
    xref_ptr<::CDependencyObject> spReturnAPAsDO;

    *ppReturnAPAsDO = nullptr;
    *ppReturnIREPFAsUnk = nullptr;

    spAP = spAccessibleNode.AsOrNull<xaml_automation_peers::IAutomationPeer>();
    if (spAP)
    {
        spReturnAPAsDO = spAP.Cast<AutomationPeer>()->GetHandle();
    }
    else
    {
        spIREPFAsUnk = spAccessibleNode.AsOrNull<IUnknown>();
    }

    *ppReturnAPAsDO = spReturnAPAsDO.detach();
    *ppReturnIREPFAsUnk = spIREPFAsUnk.Detach();
}

_Check_return_ HRESULT AutomationPeer::GetAutomationPropertyFromUIAXcpEnum(_In_ UIAXcp::APAutomationProperties eProperty, _Outptr_ xaml_automation::IAutomationProperty** ppAutomationProperty)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation::IAutomationElementIdentifiersStatics> spAutomationElementIdentifiersStatics;
    ctl::ComPtr<xaml_automation::ISelectionItemPatternIdentifiersStatics> spSelectionItemPatternIdentifiersStatics;
    ctl::ComPtr<xaml_automation::IAutomationProperty> spAutomationProperty;

    *ppAutomationProperty = nullptr;

    // AutomationElementIdentifiers MUST be cached. Because AutomationProperties don't expose any public API
    // surface to tell them apart we must rely on pointer comparisons. We rely on Jupiter's ActivationFactory cache
    // for this behavior.
    IFC(ctl::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_AutomationElementIdentifiers).Get(),
        &spAutomationElementIdentifiersStatics));

    switch (static_cast<AutomationPropertiesEnum>(eProperty))
    {
        case AutomationPropertiesEnum::EmptyProperty:
            // spAutomationProperty is already set to null.
            break;
        case AutomationPropertiesEnum::AutomationIdProperty:
            IFC(spAutomationElementIdentifiersStatics->get_AutomationIdProperty(&spAutomationProperty));
            break;
        case AutomationPropertiesEnum::NameProperty:
            IFC(spAutomationElementIdentifiersStatics->get_NameProperty(&spAutomationProperty));
            break;
        case AutomationPropertiesEnum::ControlTypeProperty:
            IFC(spAutomationElementIdentifiersStatics->get_ControlTypeProperty(&spAutomationProperty));
            break;
        case AutomationPropertiesEnum::IsSelectedProperty:
            IFC(ctl::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_SelectionItemPatternIdentifiers).Get(),
                &spSelectionItemPatternIdentifiersStatics));
            IFC(spSelectionItemPatternIdentifiersStatics->get_IsSelectedProperty(&spAutomationProperty));
            break;
        default:
            IFC(ErrorHelper::OriginateError(AgError(UIA_OPERATION_CANNOT_BE_PERFORMED)));
            IFC(static_cast<HRESULT>(UIA_E_INVALIDOPERATION));
            break;
    }

    *ppAutomationProperty = spAutomationProperty.Detach();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT AutomationPeer::GetPropertyValueFromCValue(_In_ UIAXcp::APAutomationProperties eProperty, _In_ Automation::CValue value, _Outptr_ IInspectable** ppInspectable)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strValue;
    IInspectable* pInspectable = NULL;
    switch (static_cast<AutomationPropertiesEnum>(eProperty))
    {
        case AutomationPropertiesEnum::AutomationIdProperty:
        case AutomationPropertiesEnum::NameProperty:
            IFC(GetStringFromCValue(&value, strValue.GetAddressOf()));
            IFC(DirectUI::PropertyValue::CreateFromString(strValue.Get(), &pInspectable));
            break;
        case AutomationPropertiesEnum::ControlTypeProperty:
            IFC(DirectUI::PropertyValue::CreateFromInt32(value.m_iValue, &pInspectable));
            break;
        case AutomationPropertiesEnum::IsSelectedProperty:
            IFC(DirectUI::PropertyValue::CreateFromBoolean(!!value.m_nValue, &pInspectable));
            break;
    }
    *ppInspectable = pInspectable;
    pInspectable = NULL;

Cleanup:
    ReleaseInterface(pInspectable);
    RRETURN(hr);
}

_Check_return_ HRESULT AutomationPeer::GetStringFromCValue(
    _In_ Automation::CValue* pBox,
    _Outptr_ HSTRING* phValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pBox);
    IFCPTR(phValue);

    if (pBox->IsNull())
    {
        *phValue = NULL;
    }
    else
    {
        IFCEXPECT(pBox->GetType() == valueString);
        xruntime_string_ptr strRuntimeValue;
        IFC(pBox->AsString().Promote(&strRuntimeValue));
        *phValue = strRuntimeValue.DetachHSTRING();
    }

Cleanup:
    RRETURN(hr);

}

_Check_return_ HRESULT AutomationPeer::SetCValueFromString(
    _In_ Automation::CValue* pBox,
    _In_opt_ HSTRING value)
{
    HRESULT hr = S_OK;

    IFCPTR(pBox);

    if (value != NULL)
    {
        xstring_ptr strValue;
        IFC(xstring_ptr::CloneRuntimeStringHandle(value, &strValue));
        pBox->SetString(std::move(strValue));
    }
    else
    {
        pBox->SetString(xstring_ptr::NullString());
    }
Cleanup:
    RRETURN(hr);
}

// Internal helper method to call ListenExists()
_Check_return_ HRESULT AutomationPeer::ListenerExistsHelper(
    _In_ xaml_automation_peers::AutomationEvents eventId,
    _Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;
    BOOLEAN bAutomationListener = FALSE;
    IActivationFactory* pActivationFactory = NULL;
    xaml_automation_peers::IAutomationPeerStatics* pAutomationPeerStatics = NULL;

    IFCPTR(pReturnValue);

    pActivationFactory = ctl::ActivationFactoryCreator<DirectUI::AutomationPeerFactory>::CreateActivationFactory();
    IFC(ctl::do_query_interface(pAutomationPeerStatics, pActivationFactory));
    IFC(pAutomationPeerStatics->ListenerExists(eventId, &bAutomationListener));

    *pReturnValue = bAutomationListener;

Cleanup:
    ReleaseInterface(pActivationFactory);
    ReleaseInterface(pAutomationPeerStatics);

    RRETURN(hr);
}

_Check_return_ HRESULT AutomationPeerFactory::ListenerExistsImpl(
 _In_ xaml_automation_peers::AutomationEvents eventId,
 _Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
    bool bListenerExist = false;

    IFC(CoreImports::AutomationListenerExists(static_cast<CCoreServices*>(DXamlCore::GetCurrent()->GetHandle()), (UIAXcp::APAutomationEvents)eventId, &bListenerExist));
    *returnValue = !!bListenerExist;

Cleanup:
    RRETURN(hr);
}

// Static function to provide a mechanism for making sure uniqueness of runtimeIds across the board.
// As with this change we are allowing a merge of XAML APs and native UIA nodes, there needs to be a
// common place for managing runtimeIds, this static function serves the task.
_Check_return_ HRESULT AutomationPeerFactory::GenerateRawElementProviderRuntimeIdImpl(_Out_ xaml_automation_peers::RawElementProviderRuntimeId* pReturnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::RawElementProviderRuntimeId rawElementProviderRuntimeId;
    IDXamlCore* pCore = DXamlServices::GetDXamlCore();

    if (pCore)
    {
        rawElementProviderRuntimeId.Part2 = pCore->GenerateRawElementProviderRuntimeId();

        // Each node provides a locally unique ID which is appended to the ID of the containing provider fragment root.
        // This is accomplished by specifying UiaAppendRuntimeId as the first value in the array.
        rawElementProviderRuntimeId.Part1 = UiaAppendRuntimeId;
    }

    *pReturnValue = rawElementProviderRuntimeId;

    RRETURN(hr);
}

HRESULT AutomationPeer::BoxArrayOfHStrings(Automation::CValue* pReturnVal, XINT32 nLength, HSTRING* phsArray)
{
    HRESULT hr = S_OK;
    Automation::CValue* pValueArray = NULL;
    XINT32 cValueArray = 0;

    pReturnVal->SetNull();
    if (nLength < 1)
    {
        goto Cleanup;
    }

    pValueArray = new Automation::CValue[nLength];

    for (XINT32 i = 0; i < nLength; ++i)
    {
        IFC(BoxValueHelper(&pValueArray[cValueArray], phsArray[i]));
        ++cValueArray;
    }

    pReturnVal->SetPointer(pValueArray);
    pReturnVal->SetArrayElementCount(cValueArray);
    pValueArray = nullptr;

Cleanup:
    delete[] pValueArray;
    RRETURN(hr);
}

BOOLEAN AutomationPeer::ArePropertyChangedListeners()
{
    bool bListenerExist = false;
    IGNOREHR(CoreImports::AutomationListenerExists(static_cast<CCoreServices*>(DXamlCore::GetCurrent()->GetHandle()), UIAXcp::APAutomationEvents::AEPropertyChanged, &bListenerExist));
    return !!bListenerExist;
}

HRESULT AutomationPeer::RaiseEventIfListener(
    _In_ UIElement* pUie,
    _In_ xaml_automation_peers::AutomationEvents eventId)
{
    HRESULT hr = S_OK;
    BOOLEAN bListener = FALSE;

    IFCPTR(pUie);

    IFC(AutomationPeer::ListenerExistsHelper(eventId, &bListener));
    if (bListener)
    {
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAP;

        IFC(pUie->GetOrCreateAutomationPeer(&spAP));
        if(spAP)
        {
            IFC(spAP->RaiseAutomationEvent(eventId));
        }
    }

Cleanup:
    RRETURN(hr);
}

HRESULT AutomationPeer::BoxArrayOfRawElementProviderSimple(Automation::CValue* pReturnVal, XINT32 nLength, xaml_automation::Provider::IIRawElementProviderSimple** pRepsArray)
{
    HRESULT hr = S_OK;
    CDependencyObject** pDoArray = NULL;
    XINT32 cDoArray = 0;

    pReturnVal->SetNull();
    if (nLength < 1)
    {
        goto Cleanup;
    }

    pDoArray = new CDependencyObject*[nLength];

    for (int i = 0; i < nLength; ++i)
    {
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAp = NULL;
        IFC(static_cast<IRawElementProviderSimple*>(pRepsArray[i])->GetAutomationPeer(&spAp));
        pDoArray[cDoArray++] = spAp.Cast<AutomationPeer>()->GetHandleAddRef();
    }

    pReturnVal->SetPointer(pDoArray);
    pReturnVal->SetArrayElementCount(cDoArray);
    pDoArray = nullptr;
    cDoArray = 0;

Cleanup:
    if (pDoArray)
    {
        while (cDoArray--)
        {
            ReleaseInterface(pDoArray[cDoArray]);
        }
        delete[] pDoArray;
    }

    RRETURN(hr);
}

_Check_return_ HRESULT AutomationPeer::GetAutomationPeerDOValueFromIterable(
    _In_ CDependencyObject* nativeTarget,
    _In_ UIAXcp::APAutomationProperties eProperty,
    _Outptr_::CDependencyObject** ppReturnDO)
{
    IFCPTR_RETURN(nativeTarget);
    IFCPTR_RETURN(ppReturnDO);
    *ppReturnDO = nullptr;

    ctl::ComPtr<DependencyObject> spTarget;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &spTarget));

    ctl::ComPtr<wfc::IIterable<xaml_automation_peers::AutomationPeer*>> spAutomationPeerIterable;

    ctl::ComPtr<AutomationPeer> spTargetAsAutomationPeer;
    IFC_RETURN(spTarget.As(&spTargetAsAutomationPeer));

    switch (eProperty)
    {
        case UIAXcp::APDescribedByProperty:
            IFC_RETURN(spTargetAsAutomationPeer->GetDescribedByCoreProtected(&spAutomationPeerIterable));
            break;
        case UIAXcp::APFlowsToProperty:
            IFC_RETURN(spTargetAsAutomationPeer->GetFlowsToCoreProtected(&spAutomationPeerIterable));
            break;
        case UIAXcp::APFlowsFromProperty:
            IFC_RETURN(spTargetAsAutomationPeer->GetFlowsFromCoreProtected(&spAutomationPeerIterable));
            break;
    }

    unsigned size = 0;
    ctl::ComPtr<AutomationPeerCollection> spCollection;

    if (spAutomationPeerIterable)
    {
        // AGCore doesn't know about DXAML Iterables. So, we have to marshal to a CAutomationPeerCollection.
        ctl::ComPtr<wfc::IIterator<xaml_automation_peers::AutomationPeer*>> spIter;
        ctl::ComPtr<IAutomationPeer> spAP;
        IFC_RETURN(ctl::make(&spCollection));

        IFC_RETURN(spAutomationPeerIterable->First(&spIter));
        boolean hasCurrent = false;
        IFC_RETURN(spIter->get_HasCurrent(&hasCurrent));
        while (hasCurrent)
        {
            IFC_RETURN(spIter->get_Current(&spAP));

            // Bug 25840857: Narrator touch investigation outside Launcher context menus crashes Shell
            // We're encountering timing issues where we try to access FlowsTo of a PopupRootAutomationPeer after all
            // popups have been closed. In that case PopupRootAutomationPeer::GetLightDismissingPopupAP will call
            // CPopupRoot::GetTopmostPopupInLightDismissChain to get the next popup, but there won't be one.
            // We'll then add null to the spAutomationPeerIterable list. Skip this entry - AutomationPeerCollection
            // returns E_INVALIDARG when you try to add null into it, which causes a fail fast for the 10X shell.
            if (spAP)
            {
                ++size;
                IFC_RETURN(spCollection->Append(spAP.Get()));
            }

            IFC_RETURN(spIter->MoveNext(&hasCurrent));
        }
    }

    if (size == 0)
    {
        return S_OK;
    }

    *ppReturnDO = spCollection->GetHandle();
    IFC_RETURN(CoreImports::DependencyObject_AddRef(*ppReturnDO));

    return S_OK;
}
