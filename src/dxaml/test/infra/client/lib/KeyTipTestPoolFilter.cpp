// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "KeyTipTestPoolFilter.h"
#include "XamlTailored.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace WEX::Common;
using namespace WEX::Logging;

namespace Private { namespace Infrastructure {

bool KeyTipTestPoolFilter::IsDirty()
{
    BOOLEAN areKeyTipsEnabled = FALSE;
    HRESULT returnCode = S_OK;

    wrl::ComPtr<xaml_input::IAccessKeyManagerStatics> akManager;
    LogThrow_IfFailed(wf::GetActivationFactory(wrl_wrappers::HStringReference(L"Microsoft.UI.Xaml.Input.AccessKeyManager").Get(),
        &akManager));

    RunOnUIThread([&]() {
        returnCode = akManager->get_AreKeyTipsEnabled(&areKeyTipsEnabled);
    });

    if (FAILED(returnCode))
    {
        Log::Warning(L"AccessKeyManager::AreKeyTipsEnabled return failure, there's likely no active input manager");
        return FALSE;
    }

    if (areKeyTipsEnabled)
    {
        return FALSE;
    }
    else
    {
        Log::Error(L"AccessKeyManager::AreKeyTipsEnabled was expected to be TRUE, but it's FALSE!");
        return TRUE;
    }
}

} }

