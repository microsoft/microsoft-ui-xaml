// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ScrollViewer {

    class ScrollViewerInFlipViewTests : public WEX::TestClass<ScrollViewerInFlipViewTests>
    {
    public:
        BEGIN_TEST_CLASS(ScrollViewerInFlipViewTests)
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(UnparentFlipViewDuringTapSelectionChange1)
            TEST_METHOD_PROPERTY(L"Description", L"Temporarily removes the FlipView control from the visual tree during a FlipViewItem selection change triggered by a next-button tap. FlipView.SelectedIndex is immediately changed after re-entry.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UnparentFlipViewDuringTapSelectionChange2)
            TEST_METHOD_PROPERTY(L"Description", L"Temporarily removes the FlipView control from the visual tree during a FlipViewItem selection change triggered by a next-button tap.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UnparentFlipViewDuringClickSelectionChange1)
            TEST_METHOD_PROPERTY(L"Description", L"Temporarily removes the FlipView control from the visual tree during a FlipViewItem selection change triggered by a next-button click. FlipView.SelectedIndex is immediately changed after re-entry.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UnparentFlipViewDuringClickSelectionChange2)
            TEST_METHOD_PROPERTY(L"Description", L"Temporarily removes the FlipView control from the visual tree during a FlipViewItem selection change triggered by a next-button click.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UnparentFlipViewDuringFlickSelectionChange1)
            TEST_METHOD_PROPERTY(L"Description", L"Temporarily removes the FlipView control from the visual tree during a FlipViewItem selection change triggered by a flick. FlipView.SelectedIndex is immediately changed after re-entry.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // XamlObjects events can be fired after its parent Island has been Disposed
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UnparentFlipViewDuringFlickSelectionChange2)
            TEST_METHOD_PROPERTY(L"Description", L"Temporarily removes the FlipView control from the visual tree during a FlipViewItem selection change triggered by a flick.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // XamlObjects events can be fired after its parent Island has been Disposed
        END_TEST_METHOD()

    private:
        void TemporarilyUnparentFlipViewDuringSelectionChange(bool flick, bool tapNextButton, bool changeSelectionOnReentry);
    };

} } } } } }
