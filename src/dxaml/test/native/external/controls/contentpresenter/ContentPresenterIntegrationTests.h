// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <MetadataCleanup.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ContentPresenter {

    class ContentPresenterIntegrationTests : public WEX::TestClass<ContentPresenterIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(ContentPresenterIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"465cba5c-d9c4-40ac-933a-f238efc26016;a69ddfa4-5142-4bed-887d-6d0ca14a3473;b34da8d2-333d-40a9-a19c-94b1f9785580")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a ContentPresenter.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a ContentPresenter from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDefaultPropertyValues)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the default property values are correct.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetAndGetContentProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully get/set the ContentPresenter.Content property.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetAndGetContentTemplateProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully get/set the ContentPresenter.ContentTemplate property.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeContentTemplateProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the ContentPresenter.ContentTemplate property can be set multiple times.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesContentTemplateSelectorChooseTemplateBasedOnContent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we the content template selector chooses a template based on the content.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetAndGetContentTransitionsProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully get/set the ContentPresenter.ContentTransitions property.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesKerningTakeEffect)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting Typography.Kerning on a ContentPresenter properly propagates down.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateTextProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully set/get the text properties on a ContentPresenter.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanImplicitlySetAndPropagateDataContext)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that when we set the content, the data context is implicitly set and correctly propagated.")
           
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyBorderChrome)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that border properties works with ContentPresenter.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //

        //
        // Platform:Phone
        //

    private:
        Microsoft::UI::Xaml::Controls::ContentPresenter^ CreateContentPresenterWithBorder(unsigned int width, unsigned int height);

        void TestElementWithRenderTargetBitmap(xaml::UIElement^ element, std::function<void(Platform::Array<uint32_t>^, int, int)> verifyFunc);

        unsigned int GetPixelIndex(unsigned int x, unsigned int y, unsigned int w);

        uint32_t ArgbToUint32(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    };

} } } } } }

