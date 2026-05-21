// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ScrollViewer {

 class ScrollViewerInListBoxTests : public WEX::TestClass<ScrollViewerInListBoxTests>
 {
 public:
 BEGIN_TEST_CLASS(ScrollViewerInListBoxTests)
 TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
 TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
 TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"1bb20c90-a558-491b-b76d-55bdb9a46911;eadeac67-1552-4876-a67e-dc1fcb8a6e25")
 TEST_CLASS_PROPERTY(L"Classification", L"Integration")
 END_TEST_CLASS()

 TEST_CLASS_SETUP(ClassSetup)
 TEST_CLASS_CLEANUP(ClassCleanup)
 TEST_METHOD_SETUP(TestSetup)
 TEST_METHOD_CLEANUP(TestCleanup)

 BEGIN_TEST_METHOD(ChangeZoomFactorOfVSP)
 TEST_METHOD_PROPERTY(L"Description", L"Validates that ZoomToFactor properly changes the view for a ScrollViewer hosting a VirtualizingStackPanel.")
 TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
 TEST_METHOD_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_RS5) // Test fails with baseline file mismatch with output file only on RS4
 TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
 END_TEST_METHOD()

 BEGIN_TEST_METHOD(ChangeZoomFactorOfVSPWithPen)
 TEST_METHOD_PROPERTY(L"Description", L"Validates that using pen input to change the ZoomToFactor properly changes the view for a ScrollViewer hosting a VirtualizingStackPanel.")
 TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
 TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
 TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
 END_TEST_METHOD()
 private:
 void ChangeZoomFactorOfVSP(bool usePen);
 };

} } } } } }
