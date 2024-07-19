// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LayoutInformation.h"
#include "FrameworkElement.g.h"

using namespace DirectUI;

LayoutInformation::LayoutInformation()
{
}

HRESULT LayoutInformation::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_primitives::ILayoutInformationStatics)))
    {
        *ppObject = static_cast<xaml_primitives::ILayoutInformationStatics*>(this);
    }
    else
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    RRETURN(S_OK);
}

IFACEMETHODIMP
LayoutInformation::GetLayoutExceptionElement(
    _In_ IInspectable* pDispatcher,
    _Outptr_ xaml::IUIElement** ppElement)
{
    HRESULT hr = S_OK;
    CValue value;
    CDependencyObject* pCoreDO = NULL;
    DependencyObject* pDO = NULL;

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCEXPECT(pCore);

    IFC(CoreImports::LayoutInformation_GetLayoutExceptionElement(
        pCore->GetHandle(),
        &value));

    pCoreDO = value.AsObject();
    if (pCoreDO != NULL)
    {
        IFC(pCore->GetPeer(pCoreDO, &pDO));
        IFC(ctl::do_query_interface(*ppElement, pDO));
    }
    else
    {
        *ppElement = NULL;
    }

Cleanup:
    ctl::release_interface(pDO);
    RRETURN(hr);
}

#pragma prefast( push )
#pragma prefast( disable: 26007 "prefast doesn't understand pFloats is an array of floating point numbers ")

IFACEMETHODIMP
LayoutInformation::GetLayoutSlot(
    _In_ xaml::IFrameworkElement* pElement,
    _Out_ wf::Rect* pSlot)
{
    HRESULT hr = S_OK;
    XUINT32 cFloats = 0;
    XFLOAT* pFloats = NULL;

    IFCPTR(pElement);
    IFCPTR(pSlot);

    IFC(GetRawLayoutData(pElement, cFloats, pFloats));

    if (cFloats >= 8)
    {
        pSlot->X = pFloats[4];
        pSlot->Y = pFloats[5];
        pSlot->Width = pFloats[6];
        pSlot->Height = pFloats[7];
    }
    else
    {
        pSlot->X = pSlot->Y = pSlot->Width = pSlot->Height = 0;
    }

Cleanup:
    delete [] pFloats;
    RRETURN(hr);
}

IFACEMETHODIMP
LayoutInformation::GetAvailableSize(
    _In_ xaml::IUIElement* element,
    _Out_ wf::Size* availableSize)
{
    *availableSize = { 0, 0 };
    if (element)
    {
        auto uiElement = static_cast<UIElement*>(element);
        IFC_RETURN(uiElement->CheckThread());
        auto layoutStorage = static_cast<CUIElement*>(uiElement->GetHandle())->GetLayoutStorage();
        if (layoutStorage)
        {
            auto previousAvailableSize = layoutStorage->m_previousAvailableSize;
            *availableSize = { previousAvailableSize.width, previousAvailableSize.height };
        }
    }
    else
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

_Check_return_ HRESULT
LayoutInformation::GetRawLayoutData(
    _In_ xaml::IFrameworkElement* pElement,
    _Out_ XUINT32& cFloats,
    _Out_writes_(cFloats) XFLOAT*& pFloats)
{
    HRESULT hr = S_OK;
    IFC(CoreImports::CFrameworkElement_GetLayoutInformation(
        static_cast<CFrameworkElement*>(static_cast<FrameworkElement*>(pElement)->GetHandle()),
        &cFloats, &pFloats));

Cleanup:
    RRETURN(hr);
}


namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_LayoutInformation()
    {
        RRETURN(ctl::ActivationFactoryCreator<DirectUI::LayoutInformation>::CreateActivationFactory());
    }
}


#pragma prefast( pop )
