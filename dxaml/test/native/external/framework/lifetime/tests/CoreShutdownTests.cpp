// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CoreShutdownTests.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <wil/result_macros.h>
#include <SafeEventRegistration.h>
#include <TestEvent.h>

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
