// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "InputManager.g.h"
#include "InputServices.h"
#include "XamlRoot.g.h"

using namespace DirectUI;

_Check_return_ HRESULT
InputManagerFactory::GetLastInputDeviceTypeForXamlRootImpl(
    _In_ xaml::IXamlRoot* xamlRootInterface,
    _Out_ xaml_input::LastInputDeviceType* pReturnValue)
{
    IFCPTR_RETURN(xamlRootInterface);

    ctl::ComPtr<XamlRoot> xamlRoot;
    IFC_RETURN(xamlRootInterface->QueryInterface(IID_PPV_ARGS(&xamlRoot)));

    CContentRoot* contentRoot = xamlRoot->GetVisualTreeNoRef()->GetContentRootNoRef();
    *pReturnValue = static_cast<xaml_input::LastInputDeviceType>(contentRoot->GetInputManager().GetLastInputDeviceType());

    return S_OK;
}
