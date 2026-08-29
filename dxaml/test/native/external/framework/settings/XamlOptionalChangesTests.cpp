// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "XamlOptionalChangesTests.h"
#include <XamlTailored.h>
#include <wrl/wrappers/corewrappers.h>

#undef GetCurrentTime

#include <Microsoft.UI.Xaml.h>

using namespace WEX::Logging;
using namespace WEX::Common;
using namespace Microsoft::WRL;

namespace xaml_settings_abi = ABI::Microsoft::UI::Xaml::Settings;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Settings {

// ---------------------------------------------------------------------------
// Helper: acquire IXamlOptionalChangesStatics from the in-proc DLL factory.
// ---------------------------------------------------------------------------
static ComPtr<xaml_settings_abi::IXamlOptionalChangesStatics> GetStatics()
{
    HMODULE hModXAML{};
    VERIFY_IS_TRUE(
        GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          L"Microsoft.UI.Xaml.dll", &hModXAML) == TRUE,
        L"Unable to get the handle for Microsoft.UI.Xaml.dll.");

    typedef HRESULT(WINAPI* FpnDllGetActivationFactory)(HSTRING, IActivationFactory**);
    auto pfn = reinterpret_cast<FpnDllGetActivationFactory>(
        GetProcAddress(hModXAML, "DllGetActivationFactory"));
    VERIFY_IS_NOT_NULL(pfn);

    ComPtr<IActivationFactory> factory;
    VERIFY_SUCCEEDED(pfn(
        WRL::Wrappers::HStringReference(
            L"Microsoft.UI.Xaml.Settings.XamlOptionalChanges").Get(),
        &factory));

    ComPtr<xaml_settings_abi::IXamlOptionalChangesStatics> statics;
    VERIFY_SUCCEEDED(factory.As(&statics));
    return statics;
}

static const auto c_iconNoGrid = xaml_settings_abi::XamlChangeId_IconNoGridOptimization;

// ---------------------------------------------------------------------------
// Setup / Cleanup
// ---------------------------------------------------------------------------

bool XamlOptionalChangesTests::ClassSetup()
{
    Microsoft::UI::Xaml::Tests::Common::CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool XamlOptionalChangesTests::TestSetup()
{
    // Reset the process-wide optional-changes state via the DLL's test hook
    // so each test starts with a clean, unlocked bitmask.
    TestServices::Utilities->ResetOptionalChanges();
    return true;
}

bool XamlOptionalChangesTests::TestCleanup()
{
    // Re-lock so XAML is back in its normal post-init state for any
    // subsequent tests in the same process.
    auto statics = GetStatics();
    BOOLEAN dummy;
    statics->Lock(&dummy);
    return true;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

void XamlOptionalChangesTests::CanEnableAndQueryBeforeLock()
{
    auto statics = GetStatics();
    BOOLEAN val = FALSE;

    // Not locked (reset cleared it)
    VERIFY_SUCCEEDED(statics->IsLocked(&val));
    VERIFY_IS_FALSE(!!val);

    // Initially disabled
    VERIFY_SUCCEEDED(statics->IsChangeEnabled(c_iconNoGrid, &val));
    VERIFY_IS_FALSE(!!val);

    // Enable — recognized id pre-lock returns TRUE.
    BOOLEAN mutated = FALSE;
    VERIFY_SUCCEEDED(statics->EnableChange(c_iconNoGrid, &mutated));
    VERIFY_IS_TRUE(!!mutated);
    VERIFY_SUCCEEDED(statics->IsChangeEnabled(c_iconNoGrid, &val));
    VERIFY_IS_TRUE(!!val);

    // Disable — recognized id pre-lock returns TRUE.
    mutated = FALSE;
    VERIFY_SUCCEEDED(statics->DisableChange(c_iconNoGrid, &mutated));
    VERIFY_IS_TRUE(!!mutated);
    VERIFY_SUCCEEDED(statics->IsChangeEnabled(c_iconNoGrid, &val));
    VERIFY_IS_FALSE(!!val);
}

void XamlOptionalChangesTests::LockReturnsTrueOnFirstCall()
{
    auto statics = GetStatics();
    BOOLEAN result = FALSE;

    // First lock succeeds
    VERIFY_SUCCEEDED(statics->Lock(&result));
    VERIFY_IS_TRUE(!!result);

    // Second lock is a no-op
    VERIFY_SUCCEEDED(statics->Lock(&result));
    VERIFY_IS_FALSE(!!result);

    // IsLocked confirms
    BOOLEAN locked = FALSE;
    VERIFY_SUCCEEDED(statics->IsLocked(&locked));
    VERIFY_IS_TRUE(!!locked);
}

void XamlOptionalChangesTests::EnableChangeFailsAfterLock()
{
    auto statics = GetStatics();
    BOOLEAN dummy;
    VERIFY_SUCCEEDED(statics->Lock(&dummy));

    BOOLEAN mutated = TRUE;
    HRESULT hr = statics->EnableChange(c_iconNoGrid, &mutated);
    VERIFY_ARE_EQUAL(hr, E_ILLEGAL_STATE_CHANGE);
    VERIFY_IS_FALSE(!!mutated);
}

void XamlOptionalChangesTests::DisableChangeFailsAfterLock()
{
    auto statics = GetStatics();
    BOOLEAN dummy;
    VERIFY_SUCCEEDED(statics->Lock(&dummy));

    BOOLEAN mutated = TRUE;
    HRESULT hr = statics->DisableChange(c_iconNoGrid, &mutated);
    VERIFY_ARE_EQUAL(hr, E_ILLEGAL_STATE_CHANGE);
    VERIFY_IS_FALSE(!!mutated);
}

void XamlOptionalChangesTests::IsChangeEnabledWorksAfterLock()
{
    auto statics = GetStatics();
    BOOLEAN val = FALSE;
    BOOLEAN mutated = FALSE;

    // Enable, then lock
    VERIFY_SUCCEEDED(statics->EnableChange(c_iconNoGrid, &mutated));
    VERIFY_IS_TRUE(!!mutated);
    BOOLEAN dummy;
    VERIFY_SUCCEEDED(statics->Lock(&dummy));

    // Query still works after lock
    VERIFY_SUCCEEDED(statics->IsChangeEnabled(c_iconNoGrid, &val));
    VERIFY_IS_TRUE(!!val);
}

void XamlOptionalChangesTests::UnrecognizedValueContract()
{
    auto statics = GetStatics();

    // Sentinel: _Reserved (= 0) is a non-recognized placeholder value, used here
    // to exercise the "unrecognized XamlChangeId" contract.
    const auto sentinel = xaml_settings_abi::XamlChangeId__Reserved;

    // Pre-lock: EnableChange returns S_OK with out=FALSE (unrecognized), no bit set.
    BOOLEAN mutated = TRUE;
    VERIFY_SUCCEEDED(statics->EnableChange(sentinel, &mutated));
    VERIFY_IS_FALSE(!!mutated);
    BOOLEAN enabled = TRUE;
    VERIFY_SUCCEEDED(statics->IsChangeEnabled(sentinel, &enabled));
    VERIFY_IS_FALSE(!!enabled);

    // Pre-lock: DisableChange behaves identically.
    mutated = TRUE;
    VERIFY_SUCCEEDED(statics->DisableChange(sentinel, &mutated));
    VERIFY_IS_FALSE(!!mutated);

    // Lock the state.
    BOOLEAN dummy;
    VERIFY_SUCCEEDED(statics->Lock(&dummy));

    // Post-lock: Enable/Disable throw E_ILLEGAL_STATE_CHANGE even for unrecognized values.
    mutated = TRUE;
    VERIFY_ARE_EQUAL(statics->EnableChange(sentinel, &mutated), E_ILLEGAL_STATE_CHANGE);
    VERIFY_IS_FALSE(!!mutated);
    mutated = TRUE;
    VERIFY_ARE_EQUAL(statics->DisableChange(sentinel, &mutated), E_ILLEGAL_STATE_CHANGE);
    VERIFY_IS_FALSE(!!mutated);

    // IsChangeEnabled never throws, returns FALSE for unrecognized values.
    enabled = TRUE;
    VERIFY_SUCCEEDED(statics->IsChangeEnabled(sentinel, &enabled));
    VERIFY_IS_FALSE(!!enabled);
}

} } } } } }
