// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Input { namespace HitTest {

class GlobalBoundsTests : public WEX::TestClass<GlobalBoundsTests>
{
public:
    BEGIN_TEST_CLASS(GlobalBoundsTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"aa6364d2-41fe-4bec-a849-584f1f309baf")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)

    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    TEST_METHOD(GetGlobalBounds)

    TEST_METHOD(GetGlobalBounds_RenderTransform)

    TEST_METHOD(GetGlobalBounds_IncludesChildBounds)

    TEST_METHOD(GetGlobalBounds_Clip)

    TEST_METHOD(GetGlobalBounds_LayoutClip)
    TEST_METHOD(GetGlobalBounds_LayoutClipAsParentClip)

    TEST_METHOD(GetGlobalBounds_WindowClip)

    TEST_METHOD(FindElementsInHostCoordinates)
    TEST_METHOD(FindElementsInHostCoordinates3D)

    BEGIN_TEST_METHOD(FindElementsInHostCoordinatesWPF)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")   // By design - Tests that API throws without an explicit root element on WPF
    END_TEST_METHOD()

    TEST_METHOD(GetGlobalBounds_LTEEscapesClips)

    BEGIN_TEST_METHOD(GetGlobalBounds_ParentedPopupEscapesClips)
    END_TEST_METHOD()

    TEST_METHOD(GetGlobalBounds_ChildHas3D)

    TEST_METHOD(ProjectionMakesConcavePolygon)

    BEGIN_TEST_METHOD(FindElementsInHostCoordinates_SwapChainPanel)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Crash in test code
    END_TEST_METHOD()
    
    TEST_METHOD(FindElementsInHostCoordinates_BaseItemChrome)
    
private:
    void CompareElementIterators(std::vector<UIElement^>& expected, ::Windows::Foundation::Collections::IIterable<UIElement^>^ actual);
    void FindElementsInHostCoordinatesCommon(bool include3D);
};

} } } } } } }
