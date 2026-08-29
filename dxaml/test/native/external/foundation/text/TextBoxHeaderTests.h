// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {
        class TextBoxHeaderTests : public WEX::TestClass < TextBoxHeaderTests >
        {
        public:
            BEGIN_TEST_CLASS(TextBoxHeaderTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"32301317-5c46-4350-8af6-a06552076e89;3192b2bd-30c5-4c19-a6c1-9856b940df63")
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(UpdateTextBoxHeader)
                TEST_METHOD_PROPERTY(L"Description", L"Validates updating header for TextBox")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxHeaderScrollsIntoView)
                TEST_METHOD_PROPERTY(L"Description", L"Validates TextBox header is scrolled into view")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxHeaderDoesNotScrollIntoViewWhenViewPortIsSmall)
                TEST_METHOD_PROPERTY(L"Description", L"Validates TextBox header is not scrolled into view at the expense of textbox view")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxHeaderDoesNotScrollIntoViewIfNoGotFocus)
                TEST_METHOD_PROPERTY(L"Description", L"Validates TextBox header is not scrolled into view if the textbox is not receiving focus.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
                
                // This test currently attempts to exercise the feature by panning on the text box.  However, panning doesn't seem to exercise the
                // code that it is supposed to be testing.  In attempting to clean this up, it was found that the whole header scrolling thing might
                // not be working as expected.  So for now we will disable this test and file a bug to make sure everything is doing what it is
                // supposed to.  
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")

            END_TEST_METHOD()

        };
    } }
} } } }

