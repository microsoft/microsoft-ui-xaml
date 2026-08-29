// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <TestEvent.h>
#include <SafeEventRegistration.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Primitives { namespace LoopingSelector {

    class LoopingSelectorIntegrationTests : public WEX::TestClass<LoopingSelectorIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(LoopingSelectorIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.Phone.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"2b4b21d5-e5c0-4fad-bed4-62030fe191a1;eadeac67-1552-4876-a67e-dc1fcb8a6e25")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(UIETree)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the UIElementTree for LoopingSelector")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TapOnItem)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that tapping on an item in a LoopingSelector selects that item")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TestPanning)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that panning in the LoopingSelection changes the selected item.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyHeightChangeDoesNotResultInMissingItems)
            TEST_METHOD_PROPERTY(L"Description", L"Regression coverage for a bug where changing the height of the LoopingSelector could result in some items not rendering.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(KeyboardNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that LoopingSelector changes selection in response to key presses of Up/Down/PageUp/PageDown/Home/End")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(LoopingSelectorItemPointerOverPressed)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that a LoopingSelectorItem goes to the PointerOver/Pressed states when interacted with by mouse")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TestUpDownButtons)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that clicking on the LoopingSelector's up/down buttons changes the selection.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(LoopingSelectorPanLoop)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that panning the LoopingSelector to the top will loop the last item, and that it is tappable")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(LoopingSelectorPanNoLoop)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that panning the LoopingSelector to the top with ShouldLoop disabled will not render the last item")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TestBalanceAndNormalize)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that panning the LoopingSelector properly handles the Balance and Normalize virtualization scenarios")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateTappingItemLoopsCorrectly)
            TEST_METHOD_PROPERTY(L"Description", L"Tapping on an item across a looping boundary should move that item into view.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

    private:
        void LoopingSelectorPanHelper(bool shouldLoop);

        void ValidatePositionsOfRealizedItems(xaml_primitives::LoopingSelector^ loopingSelector, unsigned int numberOfItemsShown, int expectedSelectedIndex);

        static xaml_primitives::LoopingSelector^ CreateTestLoopingSelector(int itemCount = 50);

        void VerifySelectionChangeWithKeyboard(
            xaml_primitives::LoopingSelector^ loopingSelector,
            std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& selectionChangedEvent,
            Platform::String^ keySeq,
            int expectedSelectedIndex);

        static xaml_primitives::LoopingSelectorItem^ GetContainingLoopingSelectorItem(xaml::FrameworkElement^ element);

        static double s_PanVelocityFactor;
    };

} } } } } } }

