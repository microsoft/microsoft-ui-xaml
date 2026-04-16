// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

using namespace Microsoft::UI::Xaml::Controls;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Tools {


        class XamlBindingHelperTests : public WEX::TestClass<XamlBindingHelperTests>
        {
        public:
            BEGIN_TEST_CLASS(XamlBindingHelperTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e22a917c-ad18-4a09-bff9-d3ca3e5ee0b8;4ad1ae36-e7c7-47c1-8811-9796ffbc49ed")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(ValidateSetWidthOnGrid)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that setting the width property on a grid works.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateThrowWhenSettingColorFromString)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we throw when trying to set a property from invalid type.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateSetColorOnGridBackgroundFromColor)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that setting the color property on the grid background works from a color type that gets boxed by WinRT.")
            END_TEST_METHOD()

        private:
            void SetupTests();

        };
    }
} } } }

// Test class used for the static setters
namespace Tests { namespace Native { namespace External { namespace Tools { namespace BindingHelpers {

    ref class MyPage : public Page, public Microsoft::UI::Xaml::Markup::IComponentConnector
    {        
    public:
        Platform::Object^ getDependencyObject() { return m_dependencyObject; }
        virtual void Connect(int connectionId, ::Platform::Object^ target);
        virtual Microsoft::UI::Xaml::Markup::IComponentConnector^ GetBindingConnector(int connectionId, ::Platform::Object^ target);

    private:
        // We need to hold onto the dependency object from the connect callback so that we can set the properties once it is in the live tree
        Platform::Object^ m_dependencyObject;
    };

    
}}}}}
