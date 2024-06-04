// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextElement.g.h"
#include "TextPointer.g.h"
#include "AutomationPeer.g.h"
#include "TextPointerWrapper.g.h"
#include "XamlRoot_Partial.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

TextElement::~TextElement()
{
    ResetAutomationPeer();
}

_Check_return_ HRESULT TextElement::get_NameImpl(_Out_ HSTRING* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::DependencyObject_Name, pValue));
}

_Check_return_ HRESULT TextElement::get_ContentStartImpl(
    _Outptr_ xaml_docs::ITextPointer** pValue
    )
{
    HRESULT hr = S_OK;
    xaml_docs::ITextPointer* pContentStart = NULL;
    CValue result;
    CDependencyObject* pDO = NULL;
    ctl::ComPtr<DependencyObject> spContentStartInternalPointer;

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCPTR(pCore);

    IFCPTR(pValue);
    *pValue = NULL;

    IFC(CoreImports::TextElement_GetEdge(
        static_cast<CTextElement*>(GetHandle()),
        CTextPointerWrapper::ElementEdge::ContentStart,
        &result));

    pDO = result.AsObject();
    if (pDO != NULL)
    {
        IFC(pCore->GetPeer(pDO, &spContentStartInternalPointer));
        IFC(DirectUI::TextPointer::CreateInstanceWithInternalPointer(
            spContentStartInternalPointer.Cast<TextPointerWrapper>(),
            &pContentStart));
    }

    *pValue = pContentStart;
    pContentStart = NULL;

Cleanup:
    ReleaseInterface(pContentStart);
    RRETURN(hr);
}

_Check_return_ HRESULT TextElement::get_ContentEndImpl(
    _Outptr_ xaml_docs::ITextPointer** pValue
    )
{
    HRESULT hr = S_OK;
    xaml_docs::ITextPointer* pContentEnd = NULL;
    CValue result;
    CDependencyObject* pDO = NULL;
    ctl::ComPtr<DependencyObject> spContentEndInternalPointer;

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCPTR(pCore);

    IFCPTR(pValue);
    *pValue = NULL;

    IFC(CoreImports::TextElement_GetEdge(
        static_cast<CTextElement*>(GetHandle()),
        CTextPointerWrapper::ElementEdge::ContentEnd,
        &result));

    pDO = result.AsObject();
    if (pDO != NULL)
    {
        IFC(pCore->GetPeer(pDO, &spContentEndInternalPointer));
        IFC(DirectUI::TextPointer::CreateInstanceWithInternalPointer(
            spContentEndInternalPointer.Cast<TextPointerWrapper>(),
            &pContentEnd));
    }

    *pValue = pContentEnd;
    pContentEnd = NULL;

Cleanup:
    ReleaseInterface(pContentEnd);
    RRETURN(hr);
}

_Check_return_ HRESULT TextElement::get_ElementStartImpl(
    _Outptr_ xaml_docs::ITextPointer** pValue
    )
{
    HRESULT hr = S_OK;
    xaml_docs::ITextPointer* pElementStart = NULL;
    CValue result;
    CDependencyObject* pDO = NULL;
    ctl::ComPtr<DependencyObject> spElementStartInternalPointer;

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCPTR(pCore);

    IFCPTR(pValue);
    *pValue = NULL;

    IFC(CoreImports::TextElement_GetEdge(
        static_cast<CTextElement*>(GetHandle()),
        CTextPointerWrapper::ElementEdge::ElementStart,
        &result));

    pDO = result.AsObject();
    if (pDO != NULL)
    {
        IFC(pCore->GetPeer(pDO, &spElementStartInternalPointer));
        IFC(DirectUI::TextPointer::CreateInstanceWithInternalPointer(
            spElementStartInternalPointer.Cast<TextPointerWrapper>(),
            &pElementStart));
    }

    *pValue = pElementStart;
    pElementStart = NULL;

Cleanup:
    ReleaseInterface(pElementStart);
    RRETURN(hr);
}

_Check_return_ HRESULT TextElement::get_ElementEndImpl(
    _Outptr_ xaml_docs::ITextPointer** pValue
    )
{
    HRESULT hr = S_OK;
    xaml_docs::ITextPointer* pElementEnd = NULL;
    CValue result;
    CDependencyObject* pDO = NULL;
    ctl::ComPtr<DependencyObject> spElementEndInternalPointer;

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCPTR(pCore);

    IFCPTR(pValue);
    *pValue = NULL;

    IFC(CoreImports::TextElement_GetEdge(
        static_cast<CTextElement*>(GetHandle()),
        CTextPointerWrapper::ElementEdge::ElementEnd,
        &result));

    pDO = result.AsObject();
    if (pDO != NULL)
    {
        IFC(pCore->GetPeer(pDO, &spElementEndInternalPointer));
        IFC(DirectUI::TextPointer::CreateInstanceWithInternalPointer(
            spElementEndInternalPointer.Cast<TextPointerWrapper>(),
            &pElementEnd));
    }

    *pValue = pElementEnd;
    pElementEnd = NULL;

Cleanup:
    ReleaseInterface(pElementEnd);
    RRETURN(hr);
}

_Check_return_ HRESULT TextElement::FindNameImpl(
    _In_ HSTRING name, _Out_ IInspectable** ppReturnValue)
{
    *ppReturnValue = nullptr;

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCEXPECT_RETURN(pCore);

    auto namedObject = pCore->GetHandle()->TryGetElementByName(xephemeral_string_ptr(name), GetHandle());
    if (!namedObject) return S_OK;

    ctl::ComPtr<DependencyObject> dxamlLayerObject;
    IFC_RETURN(pCore->GetPeer(namedObject.get(), &dxamlLayerObject));
    CValueBoxer::UnwrapExternalObjectReferenceIfPresent(ctl::as_iinspectable(dxamlLayerObject.Get()), ppReturnValue);
    return S_OK;
}

_Check_return_ HRESULT
TextElement::GetOrCreateAutomationPeer(xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    IFCPTR(ppAutomationPeer);
    *ppAutomationPeer = NULL;

    if (!m_tpAP)
    {
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAP;
        if (FAILED(this->OnCreateAutomationPeer(&spAP)))
        {
            RRETURN(E_FAIL);
        }
        else if(!spAP)
        {
            RRETURN(S_FALSE);
        }

        SetPtrValue(m_tpAP, spAP.Get());
    }

    IFC(m_tpAP.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT TextElement::OnCreateAutomationPeer(xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    RRETURN(S_FALSE);
}

_Check_return_ HRESULT TextElement::AppendAutomationPeerChildren(_In_ wfc::IVector<xaml_automation_peers::AutomationPeer*>* pAutomationPeerChildren, _In_ INT startPos, _In_ INT endPos)
{
    return S_OK;
}

// called for creating automation peer on the target element
_Check_return_ HRESULT TextElement::OnCreateAutomationPeer(
    _In_ CDependencyObject* nativeTarget,
    _Out_ CAutomationPeer** returnAP)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAP;

    IFCEXPECT_ASSERT(nativeTarget);
    IFCEXPECT_ASSERT(returnAP);

    *returnAP = NULL;

    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &spTarget));
    IFC(spTarget.Cast<TextElement>()->GetOrCreateAutomationPeer(&spAP));
    if (hr == S_OK)
    {
        *returnAP = static_cast<CAutomationPeer*>(spAP.Cast<AutomationPeer>()->GetHandle());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT TextElement::get_AccessKeyScopeOwnerImpl(_Outptr_result_maybenull_ xaml::IDependencyObject** ppValue)
{
    IFC_RETURN(GetValueByKnownIndex(KnownPropertyIndex::TextElement_AccessKeyScopeOwner, ppValue));
    return S_OK;
}

_Check_return_ HRESULT TextElement::put_AccessKeyScopeOwnerImpl(_In_opt_ xaml::IDependencyObject* pValue)
{
    BOOLEAN isAKO = TRUE;

    ctl::ComPtr<IDependencyObject> ownerAsDO(pValue);

    // pValue == nullptr is valid input.  It means to set the scope owner as the root scope.
    if (pValue && !AccessKeys::IsValidAKOwnerType(ownerAsDO.Cast<TextElement>()->GetHandle()))
    {
        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_ACCESSKEYS_ACCESSKEYOWNER_CDO));
    }

    ctl::ComPtr<TextElement> owner = ownerAsDO.AsOrNull<TextElement>();

    if (owner)
    {
        IFC_RETURN(owner.Cast<TextElement>()->get_IsAccessKeyScope(&isAKO));
    }

    if (isAKO)
    {
        IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::TextElement_AccessKeyScopeOwner, pValue));
    }
    else
    {
        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_ACCESSKEYS_ACCESSKEYOWNER_ISSCOPE_FALSE));
    }

    return S_OK;
}

_Check_return_ HRESULT TextElement::get_XamlRootImpl(_Outptr_result_maybenull_ xaml::IXamlRoot** ppValue)
{
    ctl::ComPtr<xaml::IXamlRoot> xamlRoot = XamlRoot::GetForElementStatic(this);
    *ppValue = xamlRoot.Detach();
    return S_OK;
}

_Check_return_ HRESULT TextElement::put_XamlRootImpl(_In_opt_ xaml::IXamlRoot* pValue)
{
    auto xamlRoot = XamlRoot::GetForElementStatic(this).Get();
    if( pValue == xamlRoot )
    {
        return S_OK;
    }

    if( xamlRoot != nullptr )
    {
        IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(E_UNEXPECTED, ERROR_CANNOT_SET_XAMLROOT_WHEN_NOT_NULL));
    }

    IFC_RETURN(XamlRoot::SetForElementStatic(this, pValue));
    return S_OK;
}

void TextElement::ResetAutomationPeer()
{
    if (auto peg = m_tpAP.TryMakeAutoPeg())
    {
        m_tpAP.Cast<AutomationPeer>()->NotifyManagedUIElementIsDead();
        m_tpAP.Clear();
    }
}
