// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "PlaneProjectionTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <FileLoader.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <WUCRenderingScopeGuard.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Media::Media3D;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

bool PlaneProjectionTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool PlaneProjectionTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool PlaneProjectionTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void PlaneProjectionTests::AnimateToIdentity()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    PlaneProjection^ projection;
    Storyboard^ storyboard;

    ::Windows::Foundation::TimeSpan timeSpan100s;
    timeSpan100s.Duration = 1000000000L;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating tree with a PlaneProjection.");

        projection = ref new PlaneProjection();
        projection->RotationY = 30;

        Canvas^ canvas = ref new Canvas();
        canvas->Width = 50;
        canvas->Height = 50;
        canvas->Background = ref new SolidColorBrush(Colors::Red);
        canvas->Projection = projection;

        Canvas^ root = ref new Canvas();
        root->Children->Append(canvas);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Animate PlaneProjection to identity.");

        DoubleAnimation^ da = ref new DoubleAnimation();
        da->From = 30.0;
        da->To = 360.0;
        da->Duration = DurationHelper::FromTimeSpan(timeSpan100s);
        Storyboard::SetTarget(da, projection);
        Storyboard::SetTargetProperty(da, L"RotationY");

        storyboard = ref new Storyboard();
        storyboard->Children->Append(da);

        LOG_OUTPUT(L"> Starting the Storyboard marks the PlaneProjection's cached 4x4 matrix as dirty.");
        storyboard->Begin();

        LOG_OUTPUT(L"> Accessing the ProjectionMatrix property consumes the dirty flag.");
        auto matrix = projection->ProjectionMatrix;
    });
    wh->SynchronouslyTickUIThread(2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> The Storyboard going into Filling should mark the PlaneProjection's cached 4x4 matrix as dirty again.");
        storyboard->Seek(timeSpan100s);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"identity");
}

} } } } } }
