// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CoreShutdownTests.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <wil/result_macros.h>
#include <SafeEventRegistration.h>
#include <TestEvent.h>
#include <wrl.h>
#if defined(MUX_TIP_REAL)
#define wil_details_GetKernelBaseModuleHandle() GetModuleHandleW(L"kernelbase.dll")
#include <tip/tip.h>
#undef wil_details_GetKernelBaseModuleHandle
#endif

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace WEX::Logging;
using namespace WEX::Common;
using namespace Microsoft;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Lifetime {

bool CoreShutdownTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool CoreShutdownTests::TestSetup()
{
    return true;
}

bool CoreShutdownTests::TestCleanup()
{
    // Deliberately don't shut down core in test cleanup, because the tests manually do this themselves.
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void CoreShutdownTests::OffThreadReleaseBeforeIslandCreation()
{
    TestServices::WindowHelper->InitializeXaml();
    auto shutdownGuard = wil::scope_exit([]
    {
        TestServices::WindowHelper->ShutdownXaml();
    });

    xaml::RoutedEventArgs^ args = nullptr;
    RunOnUIThread([&]
    {
        VERIFY_IS_NULL(TestServices::WindowHelper->WindowContent);
        args = ref new xaml::RoutedEventArgs();
    });

    // Releasing off the UI thread queues cleanup, but there is no XAML island to tick build-tree work.
    args = nullptr;
    TestServices::WindowHelper->WaitForIdle();
}

void CoreShutdownTests::OffThreadReleaseWakesIdleCleanup()
{
    struct ReleaseState
    {
        Event destroyed;
        DWORD threadId = 0;
    };
    class ReleaseSignal final : public WRL::RuntimeClass<IInspectable>
    {
        InspectableClass(L"LifetimeTests.ReleaseSignal", BaseTrust);
    public:
        explicit ReleaseSignal(std::shared_ptr<ReleaseState> state) : m_state(std::move(state)) {}
        ~ReleaseSignal()
        {
            m_state->threadId = GetCurrentThreadId();
            m_state->destroyed.Set();
        }
    private:
        std::shared_ptr<ReleaseState> m_state;
    };

    TestServices::WindowHelper->InitializeXaml();
    auto shutdownGuard = wil::scope_exit([]
    {
        TestServices::WindowHelper->ShutdownXaml();
    });
    auto state = std::make_shared<ReleaseState>();
    DWORD uiThreadId = 0;
    xaml_controls::Grid^ element = nullptr;
    RunOnUIThread([&]
    {
        uiThreadId = GetCurrentThreadId();
        TestServices::WindowHelper->WindowContent = ref new xaml_controls::Grid();
        element = ref new xaml_controls::Grid();
        auto signal = WRL::Make<ReleaseSignal>(state);
        VERIFY_IS_NOT_NULL(signal.Get());
        element->Tag = reinterpret_cast<Platform::Object^>(static_cast<IInspectable*>(signal.Get()));
    });
    TestServices::WindowHelper->WaitForIdle();
    VERIFY_IS_FALSE(state->destroyed.HasFired());
    VERIFY_ARE_NOT_EQUAL(uiThreadId, GetCurrentThreadId());

    // Only a passive native destructor notification is observed after the off-thread release.
    // No GC, WaitForIdle, rendering request, or queue enumeration may help the cleanup wake.
    element = nullptr;
    const bool destroyed = state->destroyed.WaitForNoThrow(std::chrono::seconds(5));
    VERIFY_IS_TRUE(destroyed);
    if (destroyed)
    {
        VERIFY_ARE_EQUAL(uiThreadId, state->threadId);
    }
}

void CoreShutdownTests::CleanupTipCompletesAcrossTicks()
{
#if !defined(MUX_TIP_REAL)
    Log::Result(TestResults::Skipped, L"The public TiP stub has no running tests to observe.");
#else
    struct ReleaseState
    {
        Event destroyed;
        DWORD delayMs = 0;
    };
    class ReleaseSignal final : public WRL::RuntimeClass<IInspectable>
    {
        InspectableClass(L"LifetimeTests.TipReleaseSignal", BaseTrust);
    public:
        explicit ReleaseSignal(std::shared_ptr<ReleaseState> state) : m_state(std::move(state)) {}
        ~ReleaseSignal()
        {
            m_state->destroyed.Set();
            Sleep(m_state->delayMs);
        }
    private:
        std::shared_ptr<ReleaseState> m_state;
    };
    // UIAffinityReleaseQueueCleanupTest is process-scoped, so its running sample can be opened by measure id.
    constexpr unsigned int cleanupMeasure = 64884230;

    TestServices::WindowHelper->InitializeXaml();
    auto shutdownGuard = wil::scope_exit([]
    {
        TestServices::WindowHelper->ShutdownXaml();
    });
    std::shared_ptr<ReleaseState> states[3];
    xaml_controls::Grid^ elements[3] = {};
    // The first release outlasts the 40 ms cleanup budget, so the drain spans ticks.
    RunOnUIThread([&]
    {
        TestServices::WindowHelper->WindowContent = ref new xaml_controls::Grid();
        for (int i = 0; i < 3; ++i)
        {
            states[i] = std::make_shared<ReleaseState>();
            elements[i] = ref new xaml_controls::Grid();
            auto signal = WRL::Make<ReleaseSignal>(states[i]);
            elements[i]->Tag = reinterpret_cast<Platform::Object^>(static_cast<IInspectable*>(signal.Get()));
        }
        states[0]->delayMs = 60;
    });
    TestServices::WindowHelper->WaitForIdle();
    VERIFY_IS_FALSE(static_cast<bool>(tip::open_dynamic(cleanupMeasure)));

    elements[0] = nullptr;
    elements[1] = nullptr;
    VERIFY_IS_TRUE(states[0]->destroyed.WaitForNoThrow(std::chrono::seconds(5)));
    // The first pass is still releasing, so its sample is running.
    GUID sampleId{};
    {
        auto running = tip::open_dynamic(cleanupMeasure);
        VERIFY_IS_TRUE(static_cast<bool>(running));
        sampleId = running.test_id();
    }
    // Arrives before the queue drains, so it joins the running sample.
    elements[2] = nullptr;
    VERIFY_IS_TRUE(states[1]->destroyed.WaitForNoThrow(std::chrono::seconds(5)));
    VERIFY_IS_TRUE(states[2]->destroyed.WaitForNoThrow(std::chrono::seconds(5)));
    TestServices::WindowHelper->WaitForIdle();

    // The drain completed the sample and left none running.
    VERIFY_IS_FALSE(static_cast<bool>(tip::open_dynamic(sampleId, cleanupMeasure)));
    VERIFY_IS_FALSE(static_cast<bool>(tip::open_dynamic(cleanupMeasure)));
#endif
}

// Exposed an edge case in our peer cleanup
// When the core is shutdown, we walk all the DOs, and disconnect the framework peers from the core
// objects, which gives the core objects a chance to tear down with an intact core.
// EventArgs are somewhat special because they aren't DOs, but they still have a core CEventArgs object
// to tear down. RoutedEventArgs are even more special because they keep a ref on a CDependencyObject (the event source)
// so their teardown may involve releasing the final ref on a CDependencyObject, so we need to be sure to perform this
// as well when we shutdown the core.
//
// The crash dumps typically manifest as memory corruption in the finalizer thread, running after the core is gone.
// This test attempts to deterministically simulate that situation by deliberately getting a RoutedEventArgs object to outlive the core
void CoreShutdownTests::RoutedEventArgsCleanup()
{
    TestServices::WindowHelper->InitializeXaml();

    xaml::SizeChangedEventArgs^ preservedArgs = nullptr;
    auto shutdownGuard = wil::scope_exit([]
    {
        TestServices::WindowHelper->ShutdownXaml();
    });

    {
        auto eventRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, SizeChanged);
        auto eventWrapper = std::make_shared<Event>();
        xaml_shapes::Rectangle^ source = nullptr;

        RunOnUIThread([&]
        {
            source = ref new xaml_shapes::Rectangle();
            source->Width = 100;
            source->Height = 100;

            eventRegistration.Attach(
                source,
                ref new xaml::SizeChangedEventHandler(
                    [&](Platform::Object^, xaml::SizeChangedEventArgs^ args)
            {
                eventWrapper->Set();
                preservedArgs = args;
            }));

            TestServices::WindowHelper->WindowContent = source;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]
        {
            source->Width = 200;
        });
        TestServices::WindowHelper->WaitForIdle();

        eventWrapper->WaitForDefault();
        VERIFY_IS_TRUE(eventWrapper->HasFired());
        VERIFY_IS_NOT_NULL(preservedArgs);

        // Clear the tree
        RunOnUIThread([&]
        {
            source = nullptr;
            TestServices::WindowHelper->WindowContent = nullptr;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Shutdown Xaml. This causes the core to shutdown all the DOs and other objects needing shutdown
        // logic. This should release the core CEventArgs from the peer
        shutdownGuard.reset();
    }

    // Kill the args. deliberately off-thread
    preservedArgs = nullptr;
}

// Regression test for the shutdown access violation in CInputServices::GetPrimaryRegisteredIslandInputSite.
//
// A TextBox that still has focus when the core is torn down is kept alive by the text core rather than
// by the visual tree, so it outlives ResetCoreWindowVisualTree and is destroyed by CCoreServices::~CCoreServices
// at "delete m_pTextCore". That teardown synchronously re-enters input:
//   CTextBoxBase::Destroy -> OnTxInPlaceDeactivate -> ShowGrippers -> TxGetWindow
//     -> CDependencyObject::GetElementIslandInputSite -> CInputServices::GetPrimaryRegisteredIslandInputSite
// m_inputServices is an xref_ptr, so resetting it before the text core is deleted destroys CInputServices
// and that callback then runs on a null instance, faulting while reading m_islandInputSiteRegistrations.
//
// The test deliberately leaves the focused TextBox in the tree across shutdown. It fails as an access
// violation (0xC0000005) if the reset is ordered before the text core teardown.
void CoreShutdownTests::FocusedTextBoxAtCoreShutdown()
{
    TestServices::WindowHelper->InitializeXaml();

    auto shutdownGuard = wil::scope_exit([]
    {
        TestServices::WindowHelper->ShutdownXaml();
    });

    xaml_controls::TextBox^ textBox = nullptr;

    RunOnUIThread([&]
    {
        textBox = ref new xaml_controls::TextBox();
        textBox->Text = L"focused text input at shutdown";
        textBox->ContextFlyout = nullptr;
        textBox->SelectionFlyout = nullptr;
        TestServices::WindowHelper->WindowContent = textBox;
    });
    TestServices::WindowHelper->WaitForIdle();

    // Focus activates the RichEdit host in place, which is what arms the deactivation callback below.
    RunOnUIThread([&]
    {
        VERIFY_IS_TRUE(textBox->Focus(xaml::FocusState::Programmatic));
    });
    TestServices::WindowHelper->WaitForIdle();

    // Release the test's reference but deliberately leave the TextBox focused and in the tree, so the
    // text core owns the final reference and drops it during core destruction.
    RunOnUIThread([&]
    {
        textBox = nullptr;
    });

    shutdownGuard.reset();
}

} } } } } }
