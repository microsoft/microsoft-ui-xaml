// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "StowedExceptionTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

#include <errorcontext.h>
#include <ErrorContextStructure.h>
#include <IXamlTestHooks-errors.h>
#include <AutoBStr.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Stowed {

bool StowedExceptionTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool StowedExceptionTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool StowedExceptionTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

class RoFailFastWithErrorContextInternal2Mock
{
public:
    RoFailFastWithErrorContextInternal2Mock() {}
    ~RoFailFastWithErrorContextInternal2Mock()
    {
        Detach();
    }

    void Set(std::function<decltype(RoFailFastWithErrorContextInternal2)> mock)
    {
        m_mock = mock;
        Private::Infrastructure::IXamlErrorTestHooks^ winrtErrorTestHooks = TestServices::WindowHelper->GetErrorHandlingTestHooks();
        auto errorTestHooks = reinterpret_cast<::IXamlErrorTestHooks*>(winrtErrorTestHooks);
        errorTestHooks->SetRoFailFastMock(&m_mock);
    }

    void Detach()
    {
        if (m_mock)
        {
            Private::Infrastructure::IXamlErrorTestHooks^ winrtErrorTestHooks = TestServices::WindowHelper->GetErrorHandlingTestHooks();
            auto errorTestHooks = reinterpret_cast<::IXamlErrorTestHooks*>(winrtErrorTestHooks);
            errorTestHooks->SetRoFailFastMock(nullptr);
            m_mock = nullptr;
        }
    }

private:
    std::function<decltype(RoFailFastWithErrorContextInternal2)> m_mock{};
};

void StowedExceptionTests::VerifyFailFastOnErrors()
{
    DebugSettings^ debugSettings;
    bool origFailFastOnErrors = false;
    RunOnUIThread([&]
    {
        debugSettings = Application::Current->DebugSettings;
        origFailFastOnErrors = debugSettings->FailFastOnErrors;
        VERIFY_IS_FALSE(origFailFastOnErrors); // current default is false
    });

    TestCleanupWrapper cleanup([&]()
    {
        if (debugSettings != nullptr)
        {
            RunOnUIThread([&]
            {
                debugSettings->FailFastOnErrors = origFailFastOnErrors;
            });
        }
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    // Mock RoFailFastWithErrorContextInternal2() so we can see when it gets called and
    // what it gets called with.
    HRESULT hrFailFast = 0;
    bool failFastCalled = false;
    auto mockRoFailFastWithErrorContextInternal2 = RoFailFastWithErrorContextInternal2Mock();
    mockRoFailFastWithErrorContextInternal2.Set(
        [&](HRESULT hrError, ULONG cStowedExceptions, _In_reads_opt_(cStowedExceptions) PSTOWED_EXCEPTION_INFORMATION_V2 aStowedExceptionPointers[])
        {
            LOG_OUTPUT(L"-- RoFailFastWithErrorContextInternal2 called with hr=%x", hrError);
            hrFailFast = hrError;
            failFastCalled = true;
        });

    // First just set the property on the UI thread and make sure both thread see it.
    LOG_OUTPUT(L"Testing FailFastOnErrors property");
    RunOnUIThread([&]
    {
        LOG_OUTPUT(L"Toggle on UI thread.");
        debugSettings->FailFastOnErrors = true;
        VERIFY_IS_TRUE(debugSettings->FailFastOnErrors);
    });
    VERIFY_IS_TRUE(debugSettings->FailFastOnErrors);

    // Now do the same from the background thread.
    LOG_OUTPUT(L"Toggle on background thread.");
    debugSettings->FailFastOnErrors = false;
    VERIFY_IS_FALSE(debugSettings->FailFastOnErrors);
    RunOnUIThread([&]
    {
        VERIFY_IS_FALSE(debugSettings->FailFastOnErrors);
    });

    bool threwException = false;

    RunOnUIThread([&]
    {
        // First, validate an invalid argument in the API the test will use.
        LOG_OUTPUT(L"Calling function which should throw an exception.");
        try
        {
            VisualTreeHelper::GetChild(nullptr, 0);
        }
        catch (Platform::Exception^ e)
        {
            threwException = true;
            LOG_OUTPUT(L"Expected exception thrown.");
            VERIFY_ARE_EQUAL(e->HResult, E_INVALIDARG);
        }
    });

    VERIFY_IS_TRUE(threwException);
    VERIFY_IS_FALSE(failFastCalled);

    threwException = false;

    RunOnUIThread([&]
    {
        // Now enable FailFastOnErrors and check for FailFast.
        LOG_OUTPUT(L"Turning on FailFastOnErrors and triggering failure.");
        debugSettings->FailFastOnErrors = true;

        try
        {
            // Note:  Need to call a different function here, since otherwise
            // UpdateErrorContext() may decide this is just another IFC in the same
            // or inlined function for the same error.
            VisualTreeHelper::GetChildrenCount(nullptr);
        }
        catch (Platform::Exception^ e)
        {
            // FailFast should have happened first.  We only get to the exception
            // in this case because this test's RoFailFast mock doesn't failfast.
            VERIFY_IS_TRUE(failFastCalled, "in exception handler");
            threwException = true;
        }
    });

    VERIFY_IS_TRUE(failFastCalled);
    VERIFY_ARE_EQUAL(hrFailFast, E_INVALIDARG);
    VERIFY_IS_TRUE(threwException);

    threwException = false;
    hrFailFast = S_OK;
    failFastCalled = false;

    // Disable FailFastOnErrors and ensure we get exceptions again.
    RunOnUIThread([&]
    {
        LOG_OUTPUT(L"Turning FailFastOnErrors back off and triggering exception failure again.");
        debugSettings->FailFastOnErrors = false;
        try
        {
            VisualTreeHelper::GetChild(nullptr, 0);
        }
        catch (Platform::Exception^ e)
        {
            threwException = true;
            LOG_OUTPUT(L"Expected exception thrown.");
            VERIFY_ARE_EQUAL(e->HResult, E_INVALIDARG);
        }
    });

    VERIFY_IS_TRUE(threwException);
    VERIFY_IS_FALSE(failFastCalled);
}

void StowedExceptionTests::VerifyMultipleStowedExceptions()
{
    DebugSettings^ debugSettings;
    bool origFailFastOnErrors = false;
    RunOnUIThread([&]
    {
        debugSettings = Application::Current->DebugSettings;
        origFailFastOnErrors = debugSettings->FailFastOnErrors;
        VERIFY_IS_FALSE(origFailFastOnErrors); // current default is false
    });

    const DWORD TLS_UNINITIALIZED = TLS_OUT_OF_INDEXES;
    DWORD errorContextIndex = TLS_UNINITIALIZED;
    void* tlsValueToRestore = nullptr;

    TestCleanupWrapper cleanup([&]()
    {
        RunOnUIThread([&]
        {
            if (debugSettings != nullptr)
            {
                debugSettings->FailFastOnErrors = origFailFastOnErrors;
            }
            if (errorContextIndex != TLS_UNINITIALIZED)
            {
                TlsSetValue(errorContextIndex, tlsValueToRestore);
            }
        });
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    // Mock RoFailFastWithErrorContextInternal2() so we can see when it gets called and
    // what it gets called with.
    HRESULT hrFailFast = 0;
    bool failFastCalled = false;
    ULONG cFailFastStowedExceptions = 0;
    auto mockRoFailFastWithErrorContextInternal2 = RoFailFastWithErrorContextInternal2Mock();
    mockRoFailFastWithErrorContextInternal2.Set(
        [&](HRESULT hrError, ULONG cStowedExceptions, _In_reads_opt_(cStowedExceptions) PSTOWED_EXCEPTION_INFORMATION_V2 aStowedExceptionPointers[])
        {
            LOG_OUTPUT(L"-- RoFailFastWithErrorContextInternal2 called with hr=%x", hrError);
            hrFailFast = hrError;
            cFailFastStowedExceptions = cStowedExceptions;
            failFastCalled = true;
        });

    // Clear out any current ErrorContexts on the thread, saving the value to be restored at the end.
    typedef DWORD (__cdecl* GetErrorContextIndexFuncPtr)();
    auto pfnGetErrorContextIndex = reinterpret_cast<GetErrorContextIndexFuncPtr>(GetProcAddress(GetModuleHandle(L"Microsoft.UI.Xaml.dll"), "GetErrorContextIndex"));
    if (pfnGetErrorContextIndex)
    {
        errorContextIndex = pfnGetErrorContextIndex();
        if (errorContextIndex != TLS_UNINITIALIZED)
        {
            RunOnUIThread([&]
            {
                tlsValueToRestore = TlsGetValue(errorContextIndex);
                TlsSetValue(errorContextIndex, nullptr);
            });
        }
    }

    RunOnUIThread([&]
    {
        // Trigger an initial error without FailFast
        debugSettings->FailFastOnErrors = false;
        try { VisualTreeHelper::GetChild(nullptr, 0); } catch (Platform::Exception^ e) {}

        // Trigger a second error (with a different function!), this time with FailFast.
        LOG_OUTPUT(L"Turning on FailFastOnErrors and triggering failure.");
        debugSettings->FailFastOnErrors = true;
        try { VisualTreeHelper::GetChildrenCount(nullptr); } catch (Platform::Exception^ e) {}
    });

    VERIFY_IS_TRUE(failFastCalled);
    VERIFY_ARE_EQUAL(hrFailFast, E_INVALIDARG);
    VERIFY_ARE_EQUAL(cFailFastStowedExceptions, (ULONG)2);
}

void StowedExceptionTests::NoFailFastOnErrorsInDesignMode()
{
    DebugSettings^ debugSettings;
    bool origFailFastOnErrors = false;
    RunOnUIThread([&]
    {
        debugSettings = Application::Current->DebugSettings;
        origFailFastOnErrors = debugSettings->FailFastOnErrors;
        VERIFY_IS_FALSE(origFailFastOnErrors); // current default is false
    });

    TestCleanupWrapper cleanup([&]()
    {
        if (debugSettings != nullptr)
        {
            RunOnUIThread([&]
            {
                debugSettings->FailFastOnErrors = origFailFastOnErrors;
            });
        }

        LOG_OUTPUT(L"Cleaning up.");
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    // Mock RoFailFastWithErrorContextInternal2() so we can see when it gets called and
    // what it gets called with.
    bool failFastCalled = false;
    auto mockRoFailFastWithErrorContextInternal2 = RoFailFastWithErrorContextInternal2Mock();
    mockRoFailFastWithErrorContextInternal2.Set(
        [&](HRESULT hrError, ULONG cStowedExceptions, _In_reads_opt_(cStowedExceptions) PSTOWED_EXCEPTION_INFORMATION_V2 aStowedExceptionPointers[])
        {
            VERIFY_FAIL(L"Should not FailFast in design mode!");
            failFastCalled = true;
        });

    // Enable FailFastOnErrors in design mode, which should have no effect.
    LOG_OUTPUT(L"Enabling FailFastOnErrors.");
    debugSettings->FailFastOnErrors = true;

    bool threwException = false;

    RunOnUIThread([&]
    {
        LOG_OUTPUT(L"Calling function which should throw an exception.");
        try
        {
            VisualTreeHelper::GetChild(nullptr, 0);
        }
        catch (Platform::Exception^ e)
        {
            threwException = true;
            LOG_OUTPUT(L"Expected exception thrown.");
        }
    });

    VERIFY_IS_TRUE(threwException);
}

ref class CustomStackPanel sealed : public StackPanel
{
public:
    CustomStackPanel() {}

    void SetErrorEnabled(bool enabled) { m_errorEnabled = enabled; }

protected:
    ::Windows::Foundation::Size MeasureOverride(::Windows::Foundation::Size availableSize) override
    {
        if (m_errorEnabled)
        {
            return ::Windows::Foundation::Size(std::numeric_limits<FLOAT>::infinity()/*invalid!*/, 0);
        }
        else
        {
            return xaml_controls::StackPanel::MeasureOverride(availableSize);
        }
    }

private:
    bool m_errorEnabled = true;
};

void StowedExceptionTests::ErrorInfoCreateRestrictedErrorInfo()
{
    ::Windows::Foundation::EventRegistrationToken renderingEventToken = {};

    TestCleanupWrapper cleanup([&]()
    {
        if (renderingEventToken.Value != 0)
        {
            RunOnUIThread([&]()
            {
                CompositionTarget::Rendering::remove(renderingEventToken);
            });
        }

        // Specifically don't reset or WaitForIdle, since the failfast leaves the UI thread in a
        // broken state which no longer renders and will WaitForIdle forever. Instead, this test
        // runs isolated so we can/must just let the process exit.
        //     CAN'T CALL THIS: TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    RunOnUIThread([&]()
    {
        // Clear any COM/OriginateError exception currently stored on the thread.
        ::SetRestrictedErrorInfo(nullptr);
    });

    // Ensure the test starts with XAML not having any stowed exceptions
    Private::Infrastructure::IXamlErrorTestHooks^ winrtErrorTestHooks = TestServices::WindowHelper->GetErrorHandlingTestHooks();
    auto errorTestHooks = reinterpret_cast<::IXamlErrorTestHooks*>(winrtErrorTestHooks);
    errorTestHooks->ClearStowedExceptions();

    // The special element used to generate an error.
    CustomStackPanel^ customStackPanel = nullptr;

    auto failFastEvent = std::make_shared<Event>();

    // Mock RoFailFastWithErrorContextInternal2() so we can see when it gets called and
    // what it gets called with.
    HRESULT hrFailFast = 0;
    bool failFastCalled = false;
    ULONG cFailFastStowedExceptions = 0;
    auto mockRoFailFastWithErrorContextInternal2 = RoFailFastWithErrorContextInternal2Mock();
    mockRoFailFastWithErrorContextInternal2.Set(
        [&](HRESULT hrError, ULONG cStowedExceptions, _In_reads_opt_(cStowedExceptions) PSTOWED_EXCEPTION_INFORMATION_V2 aStowedExceptionPointers[])
        {
            LOG_OUTPUT(L"-- RoFailFastWithErrorContextInternal2 called with hr=%x", hrError);
            hrFailFast = hrError;
            cFailFastStowedExceptions = cStowedExceptions;
            failFastCalled = true;

            // turn off the error to avoid the UI thread getting stuck from a repeated error.
            customStackPanel->SetErrorEnabled(false);

            wrl::ComPtr<IRestrictedErrorInfo> restrictedErrorInfo;
            VERIFY_SUCCEEDED(::GetRestrictedErrorInfo(&restrictedErrorInfo));
            ::SetRestrictedErrorInfo(restrictedErrorInfo.Get()); // Get* removes, so put it back (so debugging sees the right state).
            VERIFY_IS_NOT_NULL(restrictedErrorInfo, "Expecting a restricted error on the thread from ReportUnhandledError");

            LOG_OUTPUT(L"-- Checking if the restricted error has a language exception as expected...");
            wrl::ComPtr<ILanguageExceptionErrorInfo> languageExceptionInfo;
            VERIFY_SUCCEEDED(restrictedErrorInfo.As(&languageExceptionInfo));
            VERIFY_IS_NOT_NULL(languageExceptionInfo); // restricted error should implement this

            wrl::ComPtr<IUnknown> languageException;
            VERIFY_SUCCEEDED(languageExceptionInfo->GetLanguageException(&languageException));
            VERIFY_IS_NOT_NULL(languageException, "Should have a language exception from ErrorInfo::CreateRestrictedErrorInfo");

            wrl::ComPtr<ILanguageExceptionStackBackTrace> languageExceptionStackBackTrace;
            VERIFY_SUCCEEDED(languageException.As(&languageExceptionStackBackTrace));
            VERIFY_IS_NOT_NULL(languageExceptionStackBackTrace);

            UINT_PTR stackBackTrace[2] = {};
            ULONG framesCaptured = -1;
            VERIFY_SUCCEEDED(languageExceptionStackBackTrace->GetStackBackTrace(ARRAYSIZE(stackBackTrace), stackBackTrace, &framesCaptured));
            VERIFY_ARE_EQUAL(framesCaptured, 0, "StackBackTrace must have zero frames to ensure this restricted error doesn't cause problems for !analyze.");

            // Make sure this is the last line to ensure the above tests complete before
            // any further cleanup by the test.
            failFastEvent->Set();
        });

    StackPanel^ rootPanel = nullptr;

    LOG_OUTPUT(L"Creating the main UI with no errors.");
    RunOnUIThread([&]
    {
        rootPanel = safe_cast<StackPanel^> (xaml_markup::XamlReader::Load(
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <Button x:Name='btn1' Width='150' Height='50' Content='Button1'/>"
            L"</StackPanel>"));

        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Add an element with bad MeasureOverride implementation");
    RunOnUIThread([&]
    {
        customStackPanel = ref new CustomStackPanel();
        customStackPanel->Width = 10;
        customStackPanel->Height = 10;
        customStackPanel->SetErrorEnabled(false);
        rootPanel->Children->Append(customStackPanel);

        // Don't turn on the problematic implementation of CustomStackPanel::MeasureOverride
        // until we in rendering (which will do a layout pass immediately after firing the
        // CompositionTarget.Rendering event).
        auto onRendering = ref new ::Windows::Foundation::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ o)
        {
            LOG_OUTPUT(L"[CompositionTarget.Rendering]");

            customStackPanel->SetErrorEnabled(true);
            customStackPanel->Height = customStackPanel->Height + 1; // tweak a layout property to force a measure
            CompositionTarget::Rendering::remove(renderingEventToken);
        });
        renderingEventToken = CompositionTarget::Rendering::add(onRendering);
    });

    // Wait until the RoFailFast has happened and finished.
    // This must wait for an event: WaitForIdle() won't work once a failfast has started.
    failFastEvent->WaitForDefault();

    VERIFY_IS_TRUE(failFastCalled);
    VERIFY_ARE_EQUAL(hrFailFast, E_FAIL);
    VERIFY_ARE_EQUAL(cFailFastStowedExceptions, 1, "Should only be the one originating exception.");
}

void StowedExceptionTests::ReportUnhandledErrorNonRestrictedError()
{
    ::Windows::Foundation::EventRegistrationToken renderingEventToken = {};

    TestCleanupWrapper cleanup([&]()
    {
        if (renderingEventToken.Value != 0)
        {
            RunOnUIThread([&]()
            {
                CompositionTarget::Rendering::remove(renderingEventToken);
            });
        }

        // Specifically don't reset or WaitForIdle, since the failfast leaves the UI thread in a
        // broken state which no longer renders and will WaitForIdle forever. Instead, this test
        // runs isolated so we can/must just let the process exit.
        //     CAN'T CALL THIS: TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    RunOnUIThread([&]()
    {
        // Clear any COM/OriginateError exception currently stored on the thread.
        ::SetRestrictedErrorInfo(nullptr);
    });

    // Ensure the test starts with XAML not having any stowed exceptions
    Private::Infrastructure::IXamlErrorTestHooks^ winrtErrorTestHooks = TestServices::WindowHelper->GetErrorHandlingTestHooks();
    auto errorTestHooks = reinterpret_cast<::IXamlErrorTestHooks*>(winrtErrorTestHooks);
    errorTestHooks->ClearStowedExceptions();

    // The special element used to generate an error.
    CustomStackPanel^ customStackPanel = nullptr;

    auto failFastEvent = std::make_shared<Event>();

    AutoBSTR descTestMessage = L"test non-RestrictedErrorInfo";

    // Mock RoFailFastWithErrorContextInternal2() so we can see when it gets called and
    // what it gets called with.
    HRESULT hrFailFast = 0;
    bool failFastCalled = false;
    auto mockRoFailFastWithErrorContextInternal2 = RoFailFastWithErrorContextInternal2Mock();
    mockRoFailFastWithErrorContextInternal2.Set(
        [&](HRESULT hrError, ULONG cStowedExceptions, _In_reads_opt_(cStowedExceptions) PSTOWED_EXCEPTION_INFORMATION_V2 aStowedExceptionPointers[])
        {
            LOG_OUTPUT(L"-- RoFailFastWithErrorContextInternal2 called with hr=%x", hrError);
            LOG_OUTPUT(L"** SUCCESS: Getting here without crashing is the goal!");
            hrFailFast = hrError;
            failFastCalled = true;

            // turn off the error to avoid the UI thread getting stuck from a repeated error.
            customStackPanel->SetErrorEnabled(false);

            // Check if the error contains our error message.
            // NOTE: The error message being there is *not* critical to this test. But currently
            //       the error message should be preserved and it is helpful for it to be preserved,
            //       so confirm the message is there.
            LOG_OUTPUT(L"-- Bonus test: validate the restricted description message.");
            wrl::ComPtr<IRestrictedErrorInfo> restrictedErrorInfo;
            VERIFY_SUCCEEDED(::GetRestrictedErrorInfo(&restrictedErrorInfo));
            ::SetRestrictedErrorInfo(restrictedErrorInfo.Get()); // Get* removes, so put it back (so debugging sees the right state).
            VERIFY_IS_NOT_NULL(restrictedErrorInfo, "Expecting a restricted error on the thread from ReportUnhandledError");

            AutoBSTR description, restrictedDescription, capabilitySid;
            HRESULT hrErrorInfo = S_OK;
            VERIFY_SUCCEEDED(restrictedErrorInfo->GetErrorDetails(
                description.ReleaseAndGetAddressOf(), &hrErrorInfo, restrictedDescription.ReleaseAndGetAddressOf(), capabilitySid.ReleaseAndGetAddressOf()));
            AutoBSTR::VerifyAreEqual(descTestMessage, restrictedDescription);

            // Make sure this is the last line to ensure the above tests complete before
            // any further cleanup by the test.
            failFastEvent->Set();
        });

    StackPanel^ rootPanel = nullptr;

    LOG_OUTPUT(L"Creating the main UI with no errors.");
    RunOnUIThread([&]
    {
        rootPanel = safe_cast<StackPanel^> (xaml_markup::XamlReader::Load(
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <Button x:Name='btn1' Width='150' Height='50' Content='Button1'/>"
            L"</StackPanel>"));

        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Add an element with bad MeasureOverride implementation");
    RunOnUIThread([&]
    {
        customStackPanel = ref new CustomStackPanel();
        customStackPanel->Width = 10;
        customStackPanel->Height = 10;
        customStackPanel->SetErrorEnabled(false);
        rootPanel->Children->Append(customStackPanel);

        // Don't turn on the problematic implementation of CustomStackPanel::MeasureOverride
        // until we in rendering (which will do a layout pass immediately after firing the
        // CompositionTarget.Rendering event).
        auto onRendering = ref new ::Windows::Foundation::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ o)
        {
            LOG_OUTPUT(L"[CompositionTarget.Rendering]");

            LOG_OUTPUT(L"Creating a base ErrorInfo, rather than a RestrictedErrorInfo, and set it on the thread.");
            wrl::ComPtr<ICreateErrorInfo> createErrorInfo;
            VERIFY_SUCCEEDED(::CreateErrorInfo(&createErrorInfo));
            VERIFY_SUCCEEDED(createErrorInfo->SetDescription(descTestMessage));
            wrl::ComPtr<IErrorInfo> errorInfo;
            VERIFY_SUCCEEDED(createErrorInfo.As(&errorInfo));
            VERIFY_SUCCEEDED(::SetErrorInfo(0 /*dwReserved*/, errorInfo.Get()));

            customStackPanel->SetErrorEnabled(true);
            customStackPanel->Height = customStackPanel->Height + 1; // tweak a layout property to force a measure
            CompositionTarget::Rendering::remove(renderingEventToken);
        });
        renderingEventToken = CompositionTarget::Rendering::add(onRendering);
    });

    // Wait until the RoFailFast has happened and finished.
    // This must wait for an event: WaitForIdle() won't work once a failfast has started.
    failFastEvent->WaitForDefault();

    VERIFY_IS_TRUE(failFastCalled);
    VERIFY_ARE_EQUAL(hrFailFast, E_FAIL);
}

} } } } } }
