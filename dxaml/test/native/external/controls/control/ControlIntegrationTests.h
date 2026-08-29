// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Control {

    class ControlIntegrationTests : public WEX::TestClass<ControlIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(ControlIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"465cba5c-d9c4-40ac-933a-f238efc26016;a69ddfa4-5142-4bed-887d-6d0ca14a3473;b34da8d2-333d-40a9-a19c-94b1f9785580")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(ValidateDefaultPropertyValues)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the default property values are correct.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesFireIsEnabledChanged)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the IsEnabledChange event fires when changing the Control.IsEnabled property.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetCustomControlTemplate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that custom control templates work.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetNestedPopupControlTemplate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that nested popup templates work.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Has template with Popup IsOpen="true".  This doesn't work in islands
                                                            // because the Xaml runtime doesn't have a PopupRoot yet to
                                                            // host the open popup.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoNotPropragateMeasureDirtyDownWhenReassignSamePropertyValue)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we don't propragate measure dirty flag down when reassign a same value on a property.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesTemplateBindingMatchThemeWhenChangedDuringAnimation)
            TEST_METHOD_PROPERTY(L"Description", L"Validates when you update a theme while a template bound property is animated that "
                                                 L"it gets restored to the new theme value when the animation ends.")
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //

        //
        // Platform:Phone
        //

    };

} } } } } }
