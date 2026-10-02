// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Hosting {

        // Verifies the host boundary for the public Window display APIs (IWindow12::Show,
        // IWindow12::Show(WindowShowOptions) and IWindow12::Hide).
        //
        // These tests run UAP-hosted. Under a non-ClassicDesktop windowing model
        // WindowFactory::get_CurrentImpl returns DXamlCore's dummy window, which is backed by
        // UWPWindowImpl. That is exactly the non-desktop host we need: UWPWindowImpl reports
        // SupportsPublicDisplayApis() == false, so every public display API must return E_NOTIMPL
        // while the private IWindowPrivate lifecycle contract stays unchanged.
        class WindowDisplayApiHostTests : public WEX::TestClass<WindowDisplayApiHostTests>
        {
        public:
            BEGIN_TEST_CLASS(WindowDisplayApiHostTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"32301317-5c46-4350-8af6-a06552076e89;3192b2bd-30c5-4c19-a6c1-9856b940df63")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(PublicShowReturnsNotImplementedOnNonDesktopHost)
                TEST_METHOD_PROPERTY(L"Description", L"Public Window.Show() returns E_NOTIMPL on a non-desktop window and leaves visibility unchanged.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PublicShowWithOptionsReturnsNotImplementedOnNonDesktopHost)
                TEST_METHOD_PROPERTY(L"Description", L"Public Window.Show(WindowShowOptions) returns E_NOTIMPL on a non-desktop window, with and without an options object.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PublicHideReturnsNotImplementedOnNonDesktopHost)
                TEST_METHOD_PROPERTY(L"Description", L"Public Window.Hide() returns E_NOTIMPL on a non-desktop window and leaves visibility unchanged.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PublicAndPrivateHideDispatchSeparately)
                TEST_METHOD_PROPERTY(L"Description", L"On the same Window object IWindow12::Hide returns E_NOTIMPL while IWindowPrivate::Hide still succeeds.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PrivateHideSucceedsWithoutSoundPlayerService)
                TEST_METHOD_PROPERTY(L"Description", L"IWindowPrivate::Hide succeeds when no ElementSoundPlayer service has been created.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PrivateHideSucceedsWithSoundPlayerService)
                TEST_METHOD_PROPERTY(L"Description", L"IWindowPrivate::Hide succeeds when an ElementSoundPlayer service exists, and the service is still usable afterwards.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PlacementMembersRemainAvailableOnNonDesktopHost)
                TEST_METHOD_PROPERTY(L"Description", L"Only the display APIs are gated: the placement members on IWindow12 still succeed on a non-desktop window.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()
        };

    } }
} } } }
