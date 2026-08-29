// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <NavigateFocusResult.h>

using namespace xaml_hosting;

namespace DirectUI
{
    class __declspec(novtable) NavigateFocusResultFactory:
       public ctl::AbstractActivationFactory
        , public xaml_hosting::IXamlSourceFocusNavigationResultFactory
    {
        BEGIN_INTERFACE_MAP(NavigateFocusResultFactory, ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(NavigateFocusResultFactory, xaml_hosting::IXamlSourceFocusNavigationResultFactory)
        END_INTERFACE_MAP(NavigateFocusResultFactory, ctl::AbstractActivationFactory)

    public:
        IFACEMETHOD(CreateInstance)(_In_ boolean focusMoved, _Outptr_ IXamlSourceFocusNavigationResult** ppInstance)
        {
            wrl::ComPtr<IXamlSourceFocusNavigationResult> result = wrl::Make<NavigateFocusResult>(!!focusMoved);
            IFC_RETURN(result.CopyTo(ppInstance));
            return S_OK;
        }
    };

    _Check_return_ IActivationFactory* CreateActivationFactory_XamlSourceFocusNavigationResult()
    {
        return ctl::ActivationFactoryCreator<NavigateFocusResultFactory>::CreateActivationFactory();
    }
}

