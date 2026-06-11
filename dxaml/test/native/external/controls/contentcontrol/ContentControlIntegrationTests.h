// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ContentControl {

    class ContentControlIntegrationTests : public WEX::TestClass<ContentControlIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(ContentControlIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"465cba5c-d9c4-40ac-933a-f238efc26016;a69ddfa4-5142-4bed-887d-6d0ca14a3473;cc5953d4-6553-42e5-8c02-80720aa9d842")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a ContentControl.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a ContentControl from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetAndGetContentProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully get/set the ContentControl.Content property.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetAndGetContentTemplateProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully get/set the ContentControl.ContentTemplate property.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()
            
        BEGIN_TEST_METHOD(CanGetContentTemplateRootProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully get the ContentControl.ContentTemplateRoot property, even after new template is set.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesContentTemplateSelectorChooseTemplateBasedOnContent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we the content template selector chooses a template based on the content.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetAndGetContentTransitionsProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully get/set the ContentControl.ContentTransition property.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoNotPropragateMeasureDirtyDownWhenReassignSamePropertyValue)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we don't propragate measure dirty flag down when reassign a same value on a property.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
       END_TEST_METHOD()

        //
        // Platform:Desktop
        //

        //
        // Platform:Phone
        //

    };

} } } } } }
