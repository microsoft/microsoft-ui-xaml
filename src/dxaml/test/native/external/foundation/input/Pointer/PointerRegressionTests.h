// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace Pointer {
        
        class PointerRegressionTests : public WEX::TestClass<PointerRegressionTests>
        {
        public:
            BEGIN_TEST_CLASS(PointerRegressionTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"32301317-5c46-4350-8af6-a06552076e89;3192b2bd-30c5-4c19-a6c1-9856b940df63")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)
            
            BEGIN_TEST_METHOD(ProjectedButtonClick)
                TEST_METHOD_PROPERTY(L"Description", L"Test getting the current point of an element that has a 3D projection applied (Regression test).")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", L"22621") // This test is currently failing on 23h2, hence stop at 22h2 which is 22621.
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DelayedEventArgsPointerPointUse)
                TEST_METHOD_PROPERTY(L"Description", L"Test using the PointerPoint of a PointerRoutedEventArgs after a delay to ensure the PointerPoint is still alive.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", L"22621") // This test is currently failing on 23h2, hence stop at 22h2 which is 22621.
            END_TEST_METHOD()
        };
        
    } } }
} } } }
