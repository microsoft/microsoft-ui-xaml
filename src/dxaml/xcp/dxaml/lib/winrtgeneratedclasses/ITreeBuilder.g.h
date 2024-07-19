// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#pragma once

namespace DirectUI
{
    interface __declspec(uuid("a28ba838-4842-42bd-abdd-8d271c1b6d3e")) ITreeBuilder : public IInspectable
    {
        // Properties.
        IFACEMETHOD(get_IsRegisteredForCallbacks)(_Out_ BOOLEAN* pValue) = 0;
        IFACEMETHOD(put_IsRegisteredForCallbacks)(_In_ BOOLEAN value) = 0;

        // Events.

        // Methods.
        IFACEMETHOD(IsBuildTreeSuspended)(_Out_ BOOLEAN* pReturnValue) = 0;
        IFACEMETHOD(BuildTree)(_Out_ BOOLEAN* pReturnValue) = 0;
        IFACEMETHOD(ShutDownDeferredWork)() = 0;
    };
}
