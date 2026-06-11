// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <collection.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Input { namespace HitTest {

class HitTestStickyHeaders : public WEX::TestClass<HitTestStickyHeaders>
{
public:
    BEGIN_TEST_CLASS(HitTestStickyHeaders)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"aa6364d2-41fe-4bec-a849-584f1f309baf")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(NoTx3D)
        TEST_METHOD_PROPERTY(L"Description", L"Test StickyHeaders with no transform3D applied")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Crash in WUX.dll
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Tx3DDefaultPerspective)
        TEST_METHOD_PROPERTY(L"Description", L"Test StickyHeaders with only a perspective applied")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2. The contents are not rendering in window
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Tx3DRotYDefaultPerspective)
        TEST_METHOD_PROPERTY(L"Description", L"Test StickyHeaders with default perspective and Tx3D RotationY = 30")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2. The contents are not rendering in window
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Tx3DRotYOnHeadersDefaultPerspective)
        TEST_METHOD_PROPERTY(L"Description", L"Test StickyHeaders with RotY = 30, default perspective on root")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2. The contents are not rendering in window
    END_TEST_METHOD()

private:
    inline Platform::String^ GetResourcesPath() const;

    void SetupStickyHeaders(xaml_controls::Grid^ rootGrid, int numGroups);
    std::vector<UIElement^> GetHeaderElements(xaml_controls::Grid^ rootGrid, int numGroups);
    xaml_controls::Button^ GetHeaderButton(UIElement^ parentControl);
    Platform::Collections::Vector<Platform::Object^>^ CreateGroupedData(int numGroups, int numItemsPerGroup);
};

} } } } } } }
