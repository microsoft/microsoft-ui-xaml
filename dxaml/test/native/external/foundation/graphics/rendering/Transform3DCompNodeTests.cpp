// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "Transform3DCompNodeTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <WUCRenderingScopeGuard.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Media3D;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

bool Transform3DCompNodeTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool Transform3DCompNodeTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool Transform3DCompNodeTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void Transform3DCompNodeTests::LifetimeTest()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = nullptr;
    xaml_shapes::Rectangle^ rect = nullptr;
    CompositeTransform3D^ transform = nullptr;

    RunOnUIThread([&]()
    {
        rootCanvas = ref new Canvas();
        rootCanvas->Width = 400;
        rootCanvas->Height = 300;

        rect = ref new xaml_shapes::Rectangle();
        rect->Width = 100;
        rect->Height = 100;
        rect->Fill = ref new SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(0x99, 0xff, 0x00, 0xff));

        transform = ref new CompositeTransform3D();
        transform->TranslateX = 20;

        rect->Transform3D = transform;

        rootCanvas->Children->Append(rect);

        TestServices::WindowHelper->WindowContent = rootCanvas;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"WithTransform").GetString());

    // Null transform.
    RunOnUIThread([&]()
    {
        rect->Transform3D = nullptr;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"WithoutTransform").GetString());

    // Add transform back.
    RunOnUIThread([&]()
    {
        rect->Transform3D = transform;
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"WithTransform").GetString());

    VERIFY_IS_TRUE(true);
}

} } } } } }
