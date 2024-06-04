// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MenuFlyoutItemBase.g.h"
#include "MenuFlyoutPresenter.g.h"
#include "MenuFlyout.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;


// Get the parent MenuFlyoutPresenter.
_Check_return_ HRESULT
MenuFlyoutItemBase::GetParentMenuFlyoutPresenter(
    _Outptr_ MenuFlyoutPresenter** ppParentMenuFlyoutPresenter)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IMenuFlyoutPresenter> spParent;
    
    IFCPTR(ppParentMenuFlyoutPresenter);
    *ppParentMenuFlyoutPresenter = NULL;
    
    IFC(m_wrParentMenuFlyoutPresenter.As(&spParent));
    *ppParentMenuFlyoutPresenter = static_cast<MenuFlyoutPresenter*>(spParent.Detach());

Cleanup:
    RRETURN(hr);
}

// Sets the parent MenuFlyoutPresenter.
_Check_return_ HRESULT MenuFlyoutItemBase::SetParentMenuFlyoutPresenter(
    _In_opt_ MenuFlyoutPresenter* pParentMenuFlyoutPresenter)
{
    RRETURN(ctl::AsWeak(ctl::as_iinspectable(pParentMenuFlyoutPresenter), &m_wrParentMenuFlyoutPresenter));
}

_Check_return_ HRESULT
MenuFlyoutItemBase::GetShouldBeNarrow(
    _Out_ bool *pShouldBeNarrow)
{
    ctl::ComPtr<MenuFlyoutPresenter> spPresenter;

    IFCPTR_RETURN(pShouldBeNarrow);

    *pShouldBeNarrow = false;

    IFC_RETURN(GetParentMenuFlyoutPresenter(&spPresenter));
    if (spPresenter)
    {
        ctl::ComPtr<MenuFlyout> spParentFlyout;

        IFC_RETURN(spPresenter->GetParentMenuFlyout(&spParentFlyout));
        if (spParentFlyout != nullptr)
        {
            *pShouldBeNarrow =
                (spParentFlyout->GetInputDeviceTypeUsedToOpen() == DirectUI::InputDeviceType::Mouse) ||
                (spParentFlyout->GetInputDeviceTypeUsedToOpen() == DirectUI::InputDeviceType::Pen) ||
                (spParentFlyout->GetInputDeviceTypeUsedToOpen() == DirectUI::InputDeviceType::Keyboard);
        }
    }

    return S_OK;
}
