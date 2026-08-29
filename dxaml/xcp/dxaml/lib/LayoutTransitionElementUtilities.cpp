// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LayoutTransitionElementUtilities.g.h"
#include "UIElement.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

namespace DirectUI
{

    _Check_return_ HRESULT
        LayoutTransitionElementUtilitiesFactory::CreateLayoutTransitionElementImpl(
        _In_ xaml::IUIElement* pSource,
        _In_opt_ xaml::IUIElement* pParent,
        _Outptr_ xaml::IUIElement** ppTransitionElement)
    {
        HRESULT hr = S_OK;
        CUIElement* pLayoutTransitionElement = nullptr;
        ctl::ComPtr<DependencyObject> spLteAsDO;

        *ppTransitionElement = nullptr;

        IFC(CoreImports::LayoutTransitionElement_Create(
            DXamlCore::GetCurrent()->GetHandle(),
            static_cast<CUIElement*>(static_cast<UIElement*>(pSource)->GetHandle()),
            pParent != nullptr ? static_cast<CUIElement*>(static_cast<UIElement*>(pParent)->GetHandle()) : nullptr,
            false,
            &pLayoutTransitionElement));

        IFC(DXamlCore::GetCurrent()->GetPeer(pLayoutTransitionElement, &spLteAsDO));
        IFC(spLteAsDO.CopyTo(ppTransitionElement));

    Cleanup:
        ReleaseInterface(pLayoutTransitionElement);
        RRETURN(hr);
    }

    _Check_return_ HRESULT
        LayoutTransitionElementUtilitiesFactory::DestroyLayoutTransitionElementImpl(
        _In_ xaml::IUIElement* pSource,
        _In_opt_ xaml::IUIElement* pParent,
        _In_ xaml::IUIElement* pTransitionElement)
    {
        HRESULT hr = S_OK;

        IFC(CoreImports::LayoutTransitionElement_Destroy(
            DXamlCore::GetCurrent()->GetHandle(),
            static_cast<CUIElement*>(static_cast<UIElement*>(pSource)->GetHandle()),
            pParent != nullptr ? static_cast<CUIElement*>(static_cast<UIElement*>(pParent)->GetHandle()) : nullptr,
            static_cast<CUIElement*>(static_cast<UIElement*>(pTransitionElement)->GetHandle())));


    Cleanup:
        RRETURN(hr);
    }
}
