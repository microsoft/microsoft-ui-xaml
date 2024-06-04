// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.ui.core.h>

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Hosting {

class NavigateFocusResult : public ::Microsoft::WRL::RuntimeClass<
    ::Microsoft::WRL::RuntimeClassFlags<::Microsoft::WRL::WinRtClassicComMix>,
    IXamlSourceFocusNavigationResult>
{
    InspectableClass(RuntimeClass_Microsoft_UI_Xaml_Hosting_XamlSourceFocusNavigationResult, TrustLevel::BaseTrust);
public:
    NavigateFocusResult(_In_ bool focusMoved);

    IFACEMETHOD(get_WasFocusMoved)(_Out_ boolean * result) override;

private:
    bool m_focusMoved;
};

} } } } XAML_ABI_NAMESPACE_END
