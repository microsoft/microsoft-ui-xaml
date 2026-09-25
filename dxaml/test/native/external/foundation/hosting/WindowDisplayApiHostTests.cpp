// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "WindowDisplayApiHostTests.h"
#include <XamlTailored.h>
#include <Microsoft.UI.Xaml.h>
#include <microsoft.ui.xaml.coretypes2.h>
#include "TestCleanupWrapper.h"

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Hosting {

        namespace {

            // Snapshot of Window.Visible. The dummy window has no CoreWindow wrapper, so the read
            // may legitimately fail. Capturing the outcome as a pair lets the test assert "no
            // observable change" whether the read succeeds or fails.
            struct VisibilitySnapshot
            {
                bool succeeded = true;
                HRESULT hr = S_OK;
                bool visible = false;
            };

            VisibilitySnapshot CaptureVisibility(xaml::Window^ window)
            {
                VisibilitySnapshot snapshot;

                try
                {
                    snapshot.visible = window->Visible;
                }
                catch (Platform::Exception^ e)
                {
                    snapshot.succeeded = false;
                    snapshot.hr = e->HResult;
                }

                return snapshot;
            }

            void VerifyVisibilityUnchanged(const VisibilitySnapshot& before, const VisibilitySnapshot& after)
            {
                VERIFY_ARE_EQUAL(before.succeeded, after.succeeded);
                VERIFY_ARE_EQUAL(before.hr, after.hr);
                VERIFY_ARE_EQUAL(before.visible, after.visible);
            }

            // The public display APIs live on IWindow12. Calling them through the raw ABI keeps the
            // HRESULT intact instead of letting the C++/CX projection turn E_NOTIMPL into an
            // exception, which makes the assertions unambiguous.
            ::Microsoft::WRL::ComPtr<::ABI::Microsoft::UI::Xaml::IWindow12> GetPublicDisplayApis(xaml::Window^ window)
            {
                ::Microsoft::WRL::ComPtr<IInspectable> inspectable(reinterpret_cast<IInspectable*>(window));
                ::Microsoft::WRL::ComPtr<::ABI::Microsoft::UI::Xaml::IWindow12> window12;
                VERIFY_SUCCEEDED(inspectable.As(&window12));
                VERIFY_IS_NOT_NULL(window12.Get());
                return window12;
            }

            xaml::IWindowPrivate^ GetPrivateApis(xaml::Window^ window)
            {
                xaml::IWindowPrivate^ windowPrivate = dynamic_cast<xaml::IWindowPrivate^>(window);
                VERIFY_IS_NOT_NULL(windowPrivate);
                return windowPrivate;
            }

            // Under UAP hosting the windowing model is not ClassicDesktop, so Window::Current is
            // DXamlCore's dummy window. That window is backed by UWPWindowImpl, which is the
            // non-desktop host these tests need.
            xaml::Window^ GetNonDesktopWindow()
            {
                xaml::Window^ window = xaml::Window::Current;
                VERIFY_IS_NOT_NULL(window);
                return window;
            }
        }

        bool WindowDisplayApiHostTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool WindowDisplayApiHostTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool WindowDisplayApiHostTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void WindowDisplayApiHostTests::PublicShowReturnsNotImplementedOnNonDesktopHost()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                xaml::Window^ window = GetNonDesktopWindow();
                auto window12 = GetPublicDisplayApis(window);

                const VisibilitySnapshot before = CaptureVisibility(window);

                VERIFY_ARE_EQUAL(E_NOTIMPL, window12->Show());

                // Calling it twice must stay just as inert.
                VERIFY_ARE_EQUAL(E_NOTIMPL, window12->Show());

                VerifyVisibilityUnchanged(before, CaptureVisibility(window));
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void WindowDisplayApiHostTests::PublicShowWithOptionsReturnsNotImplementedOnNonDesktopHost()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                xaml::Window^ window = GetNonDesktopWindow();
                auto window12 = GetPublicDisplayApis(window);

                const VisibilitySnapshot before = CaptureVisibility(window);

                // Null options are rejected by the generated argument check, which runs before
                // the host gate. That check is the enforced public contract for this parameter.
                VERIFY_ARE_EQUAL(E_INVALIDARG, window12->ShowWithOptions(nullptr));

                xaml::WindowShowOptions^ options = ref new xaml::WindowShowOptions();
                ::Microsoft::WRL::ComPtr<IInspectable> optionsInspectable(reinterpret_cast<IInspectable*>(options));
                ::Microsoft::WRL::ComPtr<::ABI::Microsoft::UI::Xaml::IWindowShowOptions> abiOptions;
                VERIFY_SUCCEEDED(optionsInspectable.As(&abiOptions));

                VERIFY_ARE_EQUAL(E_NOTIMPL, window12->ShowWithOptions(abiOptions.Get()));

                VerifyVisibilityUnchanged(before, CaptureVisibility(window));
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void WindowDisplayApiHostTests::PublicHideReturnsNotImplementedOnNonDesktopHost()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                xaml::Window^ window = GetNonDesktopWindow();
                auto window12 = GetPublicDisplayApis(window);

                const VisibilitySnapshot before = CaptureVisibility(window);

                VERIFY_ARE_EQUAL(E_NOTIMPL, window12->Hide());
                VERIFY_ARE_EQUAL(E_NOTIMPL, window12->Hide());

                VerifyVisibilityUnchanged(before, CaptureVisibility(window));
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void WindowDisplayApiHostTests::PublicAndPrivateHideDispatchSeparately()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                xaml::Window^ window = GetNonDesktopWindow();
                auto window12 = GetPublicDisplayApis(window);
                xaml::IWindowPrivate^ windowPrivate = GetPrivateApis(window);

                // Same object, two interfaces, two different results. This is the assertion that
                // proves the public entry points did not get wired to the private implementation.
                VERIFY_ARE_EQUAL(E_NOTIMPL, window12->Hide());

                // Throws if the private path regressed to E_NOTIMPL.
                windowPrivate->Hide();

                // And the public path is still gated after the private one ran.
                VERIFY_ARE_EQUAL(E_NOTIMPL, window12->Hide());
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void WindowDisplayApiHostTests::PrivateHideSucceedsWithoutSoundPlayerService()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                xaml::Window^ window = GetNonDesktopWindow();
                xaml::IWindowPrivate^ windowPrivate = GetPrivateApis(window);

                // Nothing in this test touches ElementSoundPlayer, so DXamlCore has no sound
                // player service and Hide takes the early-out path.
                windowPrivate->Hide();
                windowPrivate->Hide();
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void WindowDisplayApiHostTests::PrivateHideSucceedsWithSoundPlayerService()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                xaml::Window^ window = GetNonDesktopWindow();
                xaml::IWindowPrivate^ windowPrivate = GetPrivateApis(window);

                const xaml::ElementSoundPlayerState originalState = xaml::ElementSoundPlayer::State;

                try
                {
                    // Reading Volume forces DXamlCore to create the ElementSoundPlayerService, so
                    // Hide takes the TearDownAudioGraph path. No sound is played, so this does not
                    // depend on an audio endpoint being present.
                    xaml::ElementSoundPlayer::State = xaml::ElementSoundPlayerState::On;
                    const double originalVolume = xaml::ElementSoundPlayer::Volume;

                    windowPrivate->Hide();

                    // The service must survive the teardown and stay usable.
                    VERIFY_ARE_EQUAL(originalVolume, xaml::ElementSoundPlayer::Volume);
                    xaml::ElementSoundPlayer::Volume = 0.5;
                    VERIFY_ARE_EQUAL(0.5, xaml::ElementSoundPlayer::Volume);

                    // A second Hide after the graph was torn down must still succeed.
                    windowPrivate->Hide();

                    xaml::ElementSoundPlayer::Volume = originalVolume;
                }
                catch (...)
                {
                    xaml::ElementSoundPlayer::State = originalState;
                    throw;
                }

                xaml::ElementSoundPlayer::State = originalState;
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void WindowDisplayApiHostTests::PlacementMembersRemainAvailableOnNonDesktopHost()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                xaml::Window^ window = GetNonDesktopWindow();
                auto window12 = GetPublicDisplayApis(window);

                // Only Show/Hide are gated. The placement members stay callable on a non-desktop
                // window and report "nothing persisted" instead of failing.
                ::Microsoft::WRL::Wrappers::HString persistPlacementId;
                VERIFY_SUCCEEDED(window12->get_PersistPlacementId(persistPlacementId.GetAddressOf()));

                ::boolean useAutomaticPersistence = true;
                VERIFY_SUCCEEDED(window12->get_UseAutomaticPlacementPersistence(&useAutomaticPersistence));
                VERIFY_IS_FALSE(!!useAutomaticPersistence);

                VERIFY_SUCCEEDED(window12->put_UseAutomaticPlacementPersistence(true));

                // Null options are rejected by the generated argument check before dispatch.
                ::boolean applied = true;
                VERIFY_ARE_EQUAL(E_INVALIDARG, window12->TryApplyInitialPlacement(nullptr, &applied));
                VERIFY_IS_TRUE(!!applied);

                xaml::WindowShowOptions^ options = ref new xaml::WindowShowOptions();
                ::Microsoft::WRL::ComPtr<IInspectable> optionsInspectable(reinterpret_cast<IInspectable*>(options));
                ::Microsoft::WRL::ComPtr<::ABI::Microsoft::UI::Xaml::IWindowShowOptions> abiOptions;
                VERIFY_SUCCEEDED(optionsInspectable.As(&abiOptions));

                applied = true;
                VERIFY_SUCCEEDED(window12->TryApplyInitialPlacement(abiOptions.Get(), &applied));
                VERIFY_IS_FALSE(!!applied);

                ::Microsoft::WRL::ComPtr<::ABI::Microsoft::UI::Xaml::IWindowPlacement> placement;
                ::boolean hasPlacement = true;
                VERIFY_SUCCEEDED(window12->TryGetPlacement(&placement, &hasPlacement));
                VERIFY_IS_FALSE(!!hasPlacement);
                VERIFY_IS_NULL(placement.Get());
            });

            TestServices::WindowHelper->WaitForIdle();
        }

    } }
} } } }
