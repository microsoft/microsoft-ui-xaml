// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <WUCRenderingScopeGuard.h>
#include "FeatureFlags.h"
#include <TestEvent.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class MotionTests : public WEX::TestClass<MotionTests>
{
public:
    BEGIN_TEST_CLASS(MotionTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)

    TEST_METHOD_CLEANUP(TestCleanup)

#if WI_IS_FEATURE_PRESENT(Feature_XamlMotionSystemHoldbacks)

    TEST_METHOD(BasicShownHiddenEvent)

    TEST_METHOD(ShownEvent_AddToTree)

    TEST_METHOD(ShownEvent_VisibilityVisible)

    TEST_METHOD(ShownEvent_PopupOpen)

    TEST_METHOD(HiddenEvent_RemoveFromTree)

    TEST_METHOD(HiddenEvent_VisibilityCollapsed)

    TEST_METHOD(HiddenEvent_PopupClose)

    TEST_METHOD(ShownHiddenEvent_RepeatedlyAddToTree)

    TEST_METHOD(ShownHiddenEvent_CannotMixWithECP)

    TEST_METHOD(ShownHiddenEvent_TrackEffectiveVisibility)

    BEGIN_TEST_METHOD(ShownHiddenEvent_ChangeLayoutDuringShown)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShownEvent_RemoveDuringShown)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HiddenEvent_AddDuringHidden)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    TEST_METHOD(KeepAlive_RequestReleaseAPI)

    TEST_METHOD(KeepAlive_HiddenEventHandler)

    TEST_METHOD(KeepAlive_HiddenEventHandlerPopup)

    TEST_METHOD(KeepAlive_HiddenEventHandlerPopupContent)

private:
    void EventHandlerHelper(Platform::Object^ sender, Platform::Object^ args, UIElement^ expectedSender, std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> event);

#endif
};

} } } } } }

