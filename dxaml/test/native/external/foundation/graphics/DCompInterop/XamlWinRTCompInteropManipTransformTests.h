// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics {

        class XamlWinRTCompInteropManipTransformTests : public WEX::TestClass<XamlWinRTCompInteropManipTransformTests>
        {
        public:
            BEGIN_TEST_CLASS(XamlWinRTCompInteropManipTransformTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(BasicGetScrollViewerManipulationPropertySet)
                TEST_METHOD_PROPERTY(L"Description", L"Exercises the GetScrollViewerManipulationPropertySet method to access a WinRT manipulation transform Composition PropertySet for a ScrollViewer.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AnimateVisualOffset)
                TEST_METHOD_PROPERTY(L"Description", L"Binds manipulation transform offset to a visual")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ChangeViewDuringArrange_Desktop)
                TEST_METHOD_PROPERTY(L"Description", L"Check that manipulation transform respects the adjustment offset. ChangeView during Arrange results in an adjustment offset being applied.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ChangeViewDuringArrange_Phone)
                TEST_METHOD_PROPERTY(L"Description", L"Check that manipulation transform respects the adjustment offset. ChangeView during Arrange results in an adjustment offset being applied.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Phone")
            END_TEST_METHOD()

        private:
            void ChangeViewDuringArrangeCommon(float expectedX);
        };
    } }
} } } }
