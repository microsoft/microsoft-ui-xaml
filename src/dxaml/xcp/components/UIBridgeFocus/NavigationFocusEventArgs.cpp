// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "NavigationFocusEventArgs.h"

using namespace xaml_hosting;

namespace DirectUI
{
    //
    // Bug 17347047: CodeGen should not generate factory code for non activatable event args classes
    //
    // Both NavigationGotFocusEventArgs and NavigationLosingFocusEventArgs are naver going
    // to be activated, they cannot be used in markup code.
    //
    _Check_return_ IActivationFactory* CreateActivationFactory_DesktopWindowXamlSourceGotFocusEventArgs()
    {
        IFCFAILFAST(E_NOTIMPL);
        return nullptr;
    }

    _Check_return_ IActivationFactory* CreateActivationFactory_DesktopWindowXamlSourceTakeFocusRequestedEventArgs()
    {
        IFCFAILFAST(E_NOTIMPL);
        return nullptr;
    }
}
