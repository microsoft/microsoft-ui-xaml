// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "HitTestTransform3D.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <FileLoader.h>
#include <TestCleanupWrapper.h>
#include <RuntimeEnabledFeatureOverride.h>
#include "collection.h"
#include "HitTestBasic.h"
#include <WUCRenderingScopeGuard.h>
#include <WindowsNumerics.h>

using namespace std;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Media3D;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace ::Windows::Foundation::Collections;
using namespace Microsoft::UI::Xaml::Hosting;

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;


using namespace Microsoft::UI::Xaml::Input;


namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Input { namespace HitTest {

Platform::String^ HitTestTransform3D::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\input\\HitTest\\";
}

bool HitTestTransform3D::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool HitTestTransform3D::TestCleanup()
{
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void HitTestTransform3D::DefaultCompositeAndPerspectiveInternal()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);
    xaml_controls::Button^ button = nullptr;
    Microsoft::UI::Composition::Visual^ handOffVisual = nullptr;

    RunOnUIThread([&]()
    {
        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        grid->Transform3D = ref new PerspectiveTransform3D();
        VERIFY_IS_NOT_NULL(grid->Transform3D);
        button->Transform3D = ref new CompositeTransform3D();
        VERIFY_IS_NOT_NULL(button->Transform3D);
    });

    std::vector<wf::Point> points;
    RunOnUIThread([&]()
    {
        points = HitTestBasic::GetHitTestingPoints(static_cast<FrameworkElement^>(button), grid);
    });
    TestServices::WindowHelper->WaitForIdle();
    HitTestBasic::VerifyTappedPoints(grid, button, points);

    ::Windows::Foundation::Rect expectedBounds = {50.0f,50.0f,200.0f,200.0f};
    VerifyGetGlobalBounds(button, &expectedBounds);

    // Now run the test case again, this time after having requested the HandOff visual, which triggers a different code path.
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Requesting HandOff Visual");
        handOffVisual = safe_cast<Microsoft::UI::Composition::Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(button));
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::WindowHelper->WaitForIdle();
    HitTestBasic::VerifyTappedPoints(grid, button, points);
    VerifyGetGlobalBounds(button, &expectedBounds);
}

void HitTestTransform3D::ScaleXYSmallInternal()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    points.push_back(wf::Point(51.0f, 51.0f)); // Top left
    points.push_back(wf::Point(149.0f, 51.0f)); // Top right
    points.push_back(wf::Point(51.0f, 149.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);
    xaml_controls::Button^ button = nullptr;
    Microsoft::UI::Composition::Visual^ handOffVisual = nullptr;

    RunOnUIThread([&]()
    {
        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        button->Transform3D = GenerateScaleTransform3D(0.5f, 0.5f, 1.0f);
        VERIFY_IS_NOT_NULL(button->Transform3D);
    });

    HitTestBasic::VerifyTappedPoints(grid, button, points);

    ::Windows::Foundation::Rect expectedBounds = {50.0f,50.0f,100.0f,100.0f};
    VerifyGetGlobalBounds(button, &expectedBounds);

    // Now run the test case again, this time after having requested the HandOff visual, which triggers a different code path.
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Requesting HandOff Visual");
        handOffVisual = safe_cast<Microsoft::UI::Composition::Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(button));
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::WindowHelper->WaitForIdle();
    HitTestBasic::VerifyTappedPoints(grid, button, points);
    VerifyGetGlobalBounds(button, &expectedBounds);
}

void HitTestTransform3D::ScaleXYLarge()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);
    xaml_controls::Button^ button = nullptr;

    RunOnUIThread([&]()
    {
        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        button->Transform3D = GenerateScaleTransform3D(1.5f, 1.5f, 1.0f);
        VERIFY_IS_NOT_NULL(button->Transform3D);
    });

    std::vector<wf::Point> points;
    RunOnUIThread([&]()
    {
        points = HitTestBasic::GetHitTestingPoints(static_cast<FrameworkElement^>(button), grid);
    });
    TestServices::WindowHelper->WaitForIdle();
    HitTestBasic::VerifyTappedPoints(grid, button, points);

    ::Windows::Foundation::Rect expectedBounds = {50.0f,50.0f,300.0f,300.0f};
    VerifyGetGlobalBounds(button, &expectedBounds);
}

void HitTestTransform3D::TranslateXYInternal()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    points.push_back(wf::Point(71.0f, 71.0f)); // Top left
    points.push_back(wf::Point(269.0f, 71.0f)); // Top right
    points.push_back(wf::Point(71.0f, 269.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);
    xaml_controls::Button^ button = nullptr;
    Microsoft::UI::Composition::Visual^ handOffVisual = nullptr;

    RunOnUIThread([&]()
    {
        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        button->Transform3D = GenerateTranslateTransform3D(20, 20, 0);
        VERIFY_IS_NOT_NULL(button->Transform3D);
    });
    
    HitTestBasic::VerifyTappedPoints(grid, button, points);

    ::Windows::Foundation::Rect expectedBounds = {70.0f,70.0f,200.0f,200.0f};
    VerifyGetGlobalBounds(button, &expectedBounds);

    // Now run the test case again, this time after having requested the HandOff visual, which triggers a different code path.
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Requesting HandOff Visual");
        handOffVisual = safe_cast<Microsoft::UI::Composition::Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(button));
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::WindowHelper->WaitForIdle();
    HitTestBasic::VerifyTappedPoints(grid, button, points);
    VerifyGetGlobalBounds(button, &expectedBounds);
}

void HitTestTransform3D::TranslateAndScaleSameElement()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    points.push_back(wf::Point(71.0f, 71.0f)); // Top left
    points.push_back(wf::Point(169.0f, 71.0f)); // Top right
    points.push_back(wf::Point(71.0f, 169.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);
    xaml_controls::Button^ button = nullptr;

    RunOnUIThread([&]()
    {
        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        button->Transform3D = GenerateCompositeTransform3D(0.5, 0.5, 1.0, 20, 20, 0, 0, 0, 0);
        VERIFY_IS_NOT_NULL(button->Transform3D);
    });
    
    HitTestBasic::VerifyTappedPoints(grid, button, points);

    ::Windows::Foundation::Rect expectedBounds = {70.0f,70.0f,100.0f,100.0f};
    VerifyGetGlobalBounds(button, &expectedBounds);
}

void HitTestTransform3D::TranslateParentScaleChild()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    points.push_back(wf::Point(71.0f, 71.0f)); // Top left
    points.push_back(wf::Point(169.0f, 71.0f)); // Top right
    points.push_back(wf::Point(71.0f, 169.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);
    xaml_controls::Button^ button = nullptr;

    RunOnUIThread([&]()
    {
        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        grid->Transform3D = GenerateTranslateTransform3D(20, 20, 0);
        VERIFY_IS_NOT_NULL(grid->Transform3D);

        button->Transform3D = GenerateScaleTransform3D(0.5f, 0.5f, 1.0f);
        VERIFY_IS_NOT_NULL(button->Transform3D);
    });
    
    HitTestBasic::VerifyTappedPoints(grid, button, points);

    ::Windows::Foundation::Rect expectedBounds = {70.0f,70.0f,100.0f,100.0f};
    VerifyGetGlobalBounds(button, &expectedBounds);
}

void HitTestTransform3D::ScaleParentTranslateChild()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    points.push_back(wf::Point(61.0f, 61.0f)); // Top left
    points.push_back(wf::Point(159.0f, 61.0f)); // Top right
    points.push_back(wf::Point(61.0f, 159.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);
    xaml_controls::Button^ button = nullptr;

    RunOnUIThread([&]()
    {
        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        grid->Transform3D = GenerateScaleTransform3D(0.5f, 0.5f, 1.0f);
        VERIFY_IS_NOT_NULL(grid->Transform3D);

        button->Transform3D = GenerateTranslateTransform3D(20, 20, 0);
        VERIFY_IS_NOT_NULL(button->Transform3D);
    });
    
    HitTestBasic::VerifyTappedPoints(grid, button, points, 15 /*offset*/, false /*verify using taps*/);

    ::Windows::Foundation::Rect expectedBounds = {60.0f,60.0f,100.0f,100.0f};
    VerifyGetGlobalBounds(button, &expectedBounds);
}

void HitTestTransform3D::RotationXNoPerspective()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    points.push_back(wf::Point(51.0f, 51.0f)); // Top left
    points.push_back(wf::Point(249.0f, 51.0f)); // Top right
    points.push_back(wf::Point(51.0f, 190.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);
    xaml_controls::Button^ button = nullptr;

    RunOnUIThread([&]()
    {
        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        button->Transform3D = GenerateRotationTransform3D(45, 0, 0);
        VERIFY_IS_NOT_NULL(button->Transform3D);
    });
    
    HitTestBasic::VerifyTappedPoints(grid, button, points);

    ::Windows::Foundation::Rect expectedBounds = {50.0f,50.0f,200.0f,141.42f};
    VerifyGetGlobalBounds(button, &expectedBounds);
}

void HitTestTransform3D::RotationXDefaultPerspectiveInternal()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    points.push_back(wf::Point(51.0f, 51.0f)); // Top left
    points.push_back(wf::Point(249.0f, 51.0f)); // Top right
    points.push_back(wf::Point(63.0f, 185.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);
    xaml_controls::Button^ button = nullptr;
    Microsoft::UI::Composition::Visual^ handOffVisual = nullptr;

    RunOnUIThread([&]()
    {
        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        grid->Transform3D = ref new PerspectiveTransform3D();
        VERIFY_IS_NOT_NULL(grid->Transform3D);

        button->Transform3D = GenerateRotationTransform3D(45, 0, 0);
        VERIFY_IS_NOT_NULL(button->Transform3D);
    });
    
    HitTestBasic::VerifyTappedPoints(grid, button, points);

    ::Windows::Foundation::Rect expectedBounds = {50.0f,50.0f,200.0f,136.29f};
    VerifyGetGlobalBounds(button, &expectedBounds);

    // Now run the test case again, this time after having requested the HandOff visual, which triggers a different code path.
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Requesting HandOff Visual");
        handOffVisual = safe_cast<Microsoft::UI::Composition::Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(button));
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::WindowHelper->WaitForIdle();
    HitTestBasic::VerifyTappedPoints(grid, button, points);
    VerifyGetGlobalBounds(button, &expectedBounds);
}

void HitTestTransform3D::RotationYDefaultPerspective()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    points.push_back(wf::Point(51.0f, 51.0f)); // Top left
    points.push_back(wf::Point(197.0f, 34.0f)); // Top right
    points.push_back(wf::Point(51.0f, 249.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);
    xaml_controls::Button^ button = nullptr;

    RunOnUIThread([&]()
    {
        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        grid->Transform3D = ref new PerspectiveTransform3D();
        VERIFY_IS_NOT_NULL(grid->Transform3D);

        button->Transform3D = GenerateRotationTransform3D(0, 45, 0);
        VERIFY_IS_NOT_NULL(button->Transform3D);
    });
    
    HitTestBasic::VerifyTappedPoints(grid, button, points);

    ::Windows::Foundation::Rect expectedBounds = {50.0f,33.53f,148.24f,232.94f};
    VerifyGetGlobalBounds(button, &expectedBounds);
}

void HitTestTransform3D::PerspectiveParentGridWithSiblingTransforms()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    xaml_controls::Button^ tlbutton = nullptr; // top left
    xaml_controls::Button^ trbutton = nullptr; // top right
    xaml_controls::Button^ blbutton = nullptr; // bottom left
    xaml_controls::Button^ brbutton = nullptr; // bottom right

    std::vector<wf::Point> tlPoints;
    tlPoints.push_back(wf::Point(51.0f, 51.0f)); // Top left
    tlPoints.push_back(wf::Point(169.0f, 51.0f)); // Top right
    tlPoints.push_back(wf::Point(51.0f, 169.0f)); // Bottom left

    std::vector<wf::Point> trPoints;
    trPoints.push_back(wf::Point(241.0f, 91.0f)); // Top left
    trPoints.push_back(wf::Point(358.0f, 91.0f)); // Top right
    trPoints.push_back(wf::Point(238.0f, 175.0f)); // Bottom left

    std::vector<wf::Point> blPoints;
    blPoints.push_back(wf::Point(49.0f, 221.0f)); // Top left
    blPoints.push_back(wf::Point(152.0f, 223.0f)); // Top right
    blPoints.push_back(wf::Point(53.0f, 324.0f)); // Bottom left

    std::vector<wf::Point> brPoints;
    brPoints.push_back(wf::Point(221.0f, 221.0f)); // Top left
    brPoints.push_back(wf::Point(338.0f, 221.0f)); // Top right
    brPoints.push_back(wf::Point(220.0f, 295.0f)); // Bottom left

    ::Windows::Foundation::Rect tlexpectedBounds = {50.0f,50.0f,120.0f,120.0f};
    ::Windows::Foundation::Rect trexpectedBounds = {236.87f,90.0f,123.13f,86.82f};
    //Windows::Foundation::Rect blexpectedBounds = {0.0f,0.0f,0.0f,0.0f};
    ::Windows::Foundation::Rect brexpectedBounds = {218.44f,220.0f,121.56f,76.65f};

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"FourGridWithButtons.xaml"));
    VERIFY_IS_NOT_NULL(grid);

    RunOnUIThread([&]()
    {
        tlbutton = safe_cast<Button^>(grid->FindName(L"TopLeftButton"));
        VERIFY_IS_NOT_NULL(tlbutton);
        trbutton = safe_cast<Button^>(grid->FindName(L"TopRightButton"));
        VERIFY_IS_NOT_NULL(trbutton);
        blbutton = safe_cast<Button^>(grid->FindName(L"BottomLeftButton"));
        VERIFY_IS_NOT_NULL(blbutton);
        brbutton = safe_cast<Button^>(grid->FindName(L"BottomRightButton"));
        VERIFY_IS_NOT_NULL(brbutton);

        grid->Transform3D = ref new PerspectiveTransform3D();
        VERIFY_IS_NOT_NULL(grid->Transform3D);

        // Make sure all sibling transforms are present
        tlbutton->Transform3D = GenerateScaleTransform3D(0.8f, 0.8f, 1.0f);
        VERIFY_IS_NOT_NULL(tlbutton->Transform3D);

        trbutton->RenderTransform = GenerateRenderTransform();
        VERIFY_IS_NOT_NULL(trbutton->RenderTransform);
        trbutton->Transform3D = GenerateCompositeTransform3D(/*scale*/0.8f, 0.8f, 1.0f, /*translate*/20, 20, 0, /*rotate*/45, 0, 0);
        VERIFY_IS_NOT_NULL(trbutton->Transform3D);

        blbutton->Projection = GeneratePlaneProjection();
        VERIFY_IS_NOT_NULL(blbutton->Projection);
        blbutton->Transform3D = GenerateRotationTransform3D(0, 45, 0);
        VERIFY_IS_NOT_NULL(blbutton->Transform3D);

        brbutton->Transform3D = GenerateCompositeTransform3D(/*scale*/0.8f, 0.8f, 1.0f, /*translate*/20, 20, 0, /*rotate*/45, 0, 0);
        VERIFY_IS_NOT_NULL(brbutton->Transform3D);
    });

    LOG_OUTPUT(L"## Verify TopLeft: ScaleTx3D ##");
    HitTestBasic::VerifyTappedPoints(grid, tlbutton, tlPoints);

    VerifyGetGlobalBounds(tlbutton, &tlexpectedBounds);

    LOG_OUTPUT(L"## Verify TopRight: RT & Complex CTx3D ##");
    HitTestBasic::VerifyTappedPoints(grid, trbutton, trPoints);

    VerifyGetGlobalBounds(trbutton, &trexpectedBounds);

    LOG_OUTPUT(L"## Verify BottomLeft: Projection & Complex CTx3D ##");
    HitTestBasic::VerifyTappedPoints(grid, blbutton, blPoints);

    // TODO: Fix global bounds with projection/Transform3D interop
    //VerifyGetGlobalBounds(blbutton, &blexpectedBounds);

    LOG_OUTPUT(L"## Verify BottomRight: Complex CTx3D ##");
    HitTestBasic::VerifyTappedPoints(grid, brbutton, brPoints);

    VerifyGetGlobalBounds(brbutton, &brexpectedBounds);
}

void HitTestTransform3D::PerspectiveParentGridWithSiblingTransforms2DAnd3D()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    xaml_controls::Button^ tlbutton = nullptr; // top left
    xaml_controls::Button^ trbutton = nullptr; // top right
    xaml_controls::Button^ blbutton = nullptr; // bottom left
    xaml_controls::Button^ brbutton = nullptr; // bottom right

    std::vector<wf::Point> tlPoints;
    tlPoints.push_back(wf::Point(51.0f, 51.0f)); // Top left
    tlPoints.push_back(wf::Point(169.0f, 51.0f)); // Top right
    tlPoints.push_back(wf::Point(51.0f, 169.0f)); // Bottom left

    std::vector<wf::Point> trPoints;
    trPoints.push_back(wf::Point(201.0f, 51.0f)); // Top left
    trPoints.push_back(wf::Point(349.0f, 51.0f)); // Top right
    trPoints.push_back(wf::Point(201.0f, 199.0f)); // Bottom left

    std::vector<wf::Point> blPoints;
    blPoints.push_back(wf::Point(51.0f, 202.0f)); // Top left
    blPoints.push_back(wf::Point(149.0f, 202.0f)); // Top right
    blPoints.push_back(wf::Point(51.0f, 349.0f)); // Bottom left

    std::vector<wf::Point> brPoints;
    brPoints.push_back(wf::Point(221.0f, 221.0f)); // Top left
    brPoints.push_back(wf::Point(368.0f, 221.0f)); // Top right
    brPoints.push_back(wf::Point(221.0f, 369.0f)); // Bottom left


    ::Windows::Foundation::Rect tlexpectedBounds = {50.0f,50.0f,120.0f,120.0f};
    ::Windows::Foundation::Rect trexpectedBounds = {200.0f,50.0f,150.00f,150.00f};
    ::Windows::Foundation::Rect blexpectedBounds = {50.0f,200.0f,100.85f,167.79f};
    ::Windows::Foundation::Rect brexpectedBounds = {220.00f,220.0f,150.0f,150.0f};

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"FourGridWithButtons.xaml"));
    VERIFY_IS_NOT_NULL(grid);

    RunOnUIThread([&]()
    {
        tlbutton = safe_cast<Button^>(grid->FindName(L"TopLeftButton"));
        VERIFY_IS_NOT_NULL(tlbutton);
        trbutton = safe_cast<Button^>(grid->FindName(L"TopRightButton"));
        VERIFY_IS_NOT_NULL(trbutton);
        blbutton = safe_cast<Button^>(grid->FindName(L"BottomLeftButton"));
        VERIFY_IS_NOT_NULL(blbutton);
        brbutton = safe_cast<Button^>(grid->FindName(L"BottomRightButton"));
        VERIFY_IS_NOT_NULL(brbutton);

        grid->Transform3D = ref new PerspectiveTransform3D();
        VERIFY_IS_NOT_NULL(grid->Transform3D);

        // Make sure all sibling transforms are present
        tlbutton->Transform3D = GenerateScaleTransform3D(0.8f, 0.8f, 1.0f);
        VERIFY_IS_NOT_NULL(tlbutton->Transform3D);

        blbutton->Transform3D = GenerateRotationTransform3D(0, 45, 0);
        VERIFY_IS_NOT_NULL(blbutton->Transform3D);

        brbutton->RenderTransform = GenerateRenderTransform();
        VERIFY_IS_NOT_NULL(brbutton->RenderTransform);
    });

    LOG_OUTPUT(L"## Verify TopLeft: ScaleTx3D ##");
    HitTestBasic::VerifyTappedPoints(grid, tlbutton, tlPoints);

    VerifyGetGlobalBounds(tlbutton, &tlexpectedBounds);

    LOG_OUTPUT(L"## Verify TopRight: RT ##");
    HitTestBasic::VerifyTappedPoints(grid, trbutton, trPoints);

    VerifyGetGlobalBounds(trbutton, &trexpectedBounds);

    LOG_OUTPUT(L"## Verify BottomLeft: Complex CTx3D ##");
    HitTestBasic::VerifyTappedPoints(grid, blbutton, blPoints);

    VerifyGetGlobalBounds(blbutton, &blexpectedBounds);

    LOG_OUTPUT(L"## Verify BottomRight: Nothing ##");
    HitTestBasic::VerifyTappedPoints(grid, brbutton, brPoints);

    VerifyGetGlobalBounds(brbutton, &brexpectedBounds);
}

void HitTestTransform3D::PerspectiveParentGridWithSiblingNestedPerspectiveTransforms()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    xaml_controls::Grid^ tlGrid = nullptr; // top left
    xaml_controls::Grid^ trGrid = nullptr; // top right
    xaml_controls::Grid^ blGrid = nullptr; // bottom left
    xaml_controls::Grid^ brGrid = nullptr; // bottom right

    xaml_controls::Button^ tlbutton = nullptr; // top left
    xaml_controls::Button^ trbutton = nullptr; // top right
    xaml_controls::Button^ blbutton = nullptr; // bottom left
    xaml_controls::Button^ brbutton = nullptr; // bottom right

    std::vector<wf::Point> tlPoints;
    tlPoints.push_back(wf::Point(51.0f, 51.0f)); // Top left
    tlPoints.push_back(wf::Point(198.0f, 51.0f)); // Top right
    tlPoints.push_back(wf::Point(66.0f, 159.0f)); // Bottom left

    std::vector<wf::Point> trPoints;
    trPoints.push_back(wf::Point(203.0f, 51.0f)); // Top left
    trPoints.push_back(wf::Point(348.0f, 51.0f)); // Top right
    trPoints.push_back(wf::Point(209.0f, 151.0f)); // Bottom left

    std::vector<wf::Point> blPoints;
    blPoints.push_back(wf::Point(51.0f, 201.0f)); // Top left
    blPoints.push_back(wf::Point(149.0f, 201.0f)); // Top right
    blPoints.push_back(wf::Point(51.0f, 348.0f)); // Bottom left

    std::vector<wf::Point> brPoints;
    brPoints.push_back(wf::Point(201.0f, 201.0f)); // Top left
    brPoints.push_back(wf::Point(308.0f, 193.0f)); // Top right
    brPoints.push_back(wf::Point(201.0f, 349.0f)); // Bottom left

    // This test stacks a PerspectiveTransform3D with a PlaneProjection. It will render differently depending on whether
    // those transforms flatten or preserve 3D, which gives different global bounds in the end. Test for either.
    ::Windows::Foundation::Rect tlexpectedBounds = {50.0f,50.0f,150.0f,110.28f};
    ::Windows::Foundation::Rect trexpectedBounds_FlattenDepth = {200.0f,50.0f,150.0f,103.09f};
    ::Windows::Foundation::Rect trexpectedBounds_PreserveDepth = {200.0f,50.0f,150.0f,107.19f};
    ::Windows::Foundation::Rect blexpectedBounds = {50.0f,200.0f,100.85f,167.80f};
    ::Windows::Foundation::Rect brexpectedBounds_FlattenDepth = {200.0f,191.10f,109.75f,167.80f};
    ::Windows::Foundation::Rect brexpectedBounds_PreserveDepth = {200.0f,189.90f,124.53f,190.39f};

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridWithNestedGridsAndButtons.xaml"));
    VERIFY_IS_NOT_NULL(grid);

    RunOnUIThread([&]()
    {
        tlGrid = safe_cast<Grid^>(grid->FindName(L"TopLeftGrid"));
        VERIFY_IS_NOT_NULL(tlGrid);
        trGrid = safe_cast<Grid^>(grid->FindName(L"TopRightGrid"));
        VERIFY_IS_NOT_NULL(trGrid);
        blGrid = safe_cast<Grid^>(grid->FindName(L"BottomLeftGrid"));
        VERIFY_IS_NOT_NULL(blGrid);
        brGrid = safe_cast<Grid^>(grid->FindName(L"BottomRightGrid"));
        VERIFY_IS_NOT_NULL(brGrid);

        tlbutton = safe_cast<Button^>(grid->FindName(L"TopLeftButton"));
        VERIFY_IS_NOT_NULL(tlbutton);
        trbutton = safe_cast<Button^>(grid->FindName(L"TopRightButton"));
        VERIFY_IS_NOT_NULL(trbutton);
        blbutton = safe_cast<Button^>(grid->FindName(L"BottomLeftButton"));
        VERIFY_IS_NOT_NULL(blbutton);
        brbutton = safe_cast<Button^>(grid->FindName(L"BottomRightButton"));
        VERIFY_IS_NOT_NULL(brbutton);

        grid->Transform3D = ref new PerspectiveTransform3D();
        VERIFY_IS_NOT_NULL(grid->Transform3D);

        // Give the right grids a nested perspective
        trGrid->Transform3D = ref new PerspectiveTransform3D();
        VERIFY_IS_NOT_NULL(trGrid->Transform3D);
        brGrid->Transform3D = ref new PerspectiveTransform3D();
        VERIFY_IS_NOT_NULL(brGrid->Transform3D);

        // The left and right buttons have the same transform applied,
        // but since the right buttons have a new parent perspective, their
        // transforms look different.
        tlbutton->Transform3D = GenerateRotationTransform3D(45, 0, 0);
        VERIFY_IS_NOT_NULL(tlbutton->Transform3D);

        trbutton->Transform3D = GenerateRotationTransform3D(45, 0, 0);
        VERIFY_IS_NOT_NULL(trbutton->Transform3D);

        blbutton->Transform3D = GenerateRotationTransform3D(0, 45, 0);
        VERIFY_IS_NOT_NULL(blbutton->Transform3D);

        brbutton->Transform3D = GenerateRotationTransform3D(0, 45, 0);
        VERIFY_IS_NOT_NULL(brbutton->Transform3D);
    });

    LOG_OUTPUT(L"Verify TopLeft");
    HitTestBasic::VerifyTappedPoints(grid, tlbutton, tlPoints);

    VerifyGetGlobalBounds(tlbutton, &tlexpectedBounds);

    LOG_OUTPUT(L"Verify TopRight");
    HitTestBasic::VerifyTappedPoints(grid, trbutton, trPoints);

    VerifyGetGlobalBounds(trbutton, trexpectedBounds_FlattenDepth, trexpectedBounds_PreserveDepth);

    LOG_OUTPUT(L"Verify BottomLeft");
    HitTestBasic::VerifyTappedPoints(grid, blbutton, blPoints);

    VerifyGetGlobalBounds(blbutton, &blexpectedBounds);

    LOG_OUTPUT(L"Verify BottomRight");
    HitTestBasic::VerifyTappedPoints(grid, brbutton, brPoints);

    VerifyGetGlobalBounds(brbutton, brexpectedBounds_FlattenDepth, brexpectedBounds_PreserveDepth);
}

void HitTestTransform3D::ScaleAndRenderTransformTranslateSameElementInternal()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    points.push_back(wf::Point(71.0f, 71.0f)); // Top left
    points.push_back(wf::Point(169.0f, 71.0f)); // Top right
    points.push_back(wf::Point(71.0f, 169.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);
    xaml_controls::Button^ button = nullptr;
    Microsoft::UI::Composition::Visual^ handOffVisual = nullptr;

    RunOnUIThread([&]()
    {
        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        button->RenderTransform = GenerateRenderTransform();
        VERIFY_IS_NOT_NULL(button->RenderTransform);

        button->Transform3D = GenerateScaleTransform3D(0.5f, 0.5f, 1.0f);
        VERIFY_IS_NOT_NULL(button->Transform3D);
    });

    HitTestBasic::VerifyTappedPoints(grid, button, points);

    ::Windows::Foundation::Rect expectedBounds = {70.0f,70.0f,100.0f,100.0f};
    VerifyGetGlobalBounds(button, &expectedBounds);

    // Now run the test case again, this time after having requested the HandOff visual, which triggers a different code path.
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Requesting HandOff Visual");
        handOffVisual = safe_cast<Microsoft::UI::Composition::Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(button));
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::WindowHelper->WaitForIdle();
    HitTestBasic::VerifyTappedPoints(grid, button, points);
    VerifyGetGlobalBounds(button, &expectedBounds);
}

void HitTestTransform3D::ScaleParentRenderTransformTranslateChild()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    points.push_back(wf::Point(61.0f, 61.0f)); // Top left
    points.push_back(wf::Point(159.0f, 61.0f)); // Top right
    points.push_back(wf::Point(61.0f, 159.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);
    xaml_controls::Button^ button = nullptr;

    RunOnUIThread([&]()
    {
        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        grid->Transform3D = GenerateScaleTransform3D(0.5f, 0.5f, 1.0f);
        VERIFY_IS_NOT_NULL(grid->Transform3D);

        button->RenderTransform = GenerateRenderTransform();
        VERIFY_IS_NOT_NULL(button->RenderTransform);

    });

    HitTestBasic::VerifyTappedPoints(grid, button, points);

    ::Windows::Foundation::Rect expectedBounds = {60.0f,60.0f,100.0f,100.0f};
    VerifyGetGlobalBounds(button, &expectedBounds);
}

void HitTestTransform3D::RenderTransformTranslateParentTranslateChild()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    points.push_back(wf::Point(81.0f, 81.0f)); // Top left
    points.push_back(wf::Point(279.0f, 81.0f)); // Top right
    points.push_back(wf::Point(81.0f, 279.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);
    xaml_controls::Button^ button = nullptr;

    RunOnUIThread([&]()
    {
        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        grid->RenderTransform = GenerateRenderTransform();
        VERIFY_IS_NOT_NULL(grid->RenderTransform);

        button->Transform3D = GenerateTranslateTransform3D(10, 10, 0);
        VERIFY_IS_NOT_NULL(button->Transform3D);
    });

    HitTestBasic::VerifyTappedPoints(grid, button, points);

    ::Windows::Foundation::Rect expectedBounds = {80.0f,80.0f,200.0f,200.0f};
    VerifyGetGlobalBounds(button, &expectedBounds);
}

void HitTestTransform3D::PerspectiveOuterScaleInnerRenderTransformTranslateChild()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    points.push_back(wf::Point(61.0f, 61.0f)); // Top left
    points.push_back(wf::Point(159.0f, 61.0f)); // Top right
    points.push_back(wf::Point(61.0f, 159.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);

    xaml_controls::Button^ button = nullptr;
    Grid^ innerGrid = nullptr;

    RunOnUIThread([&]()
    {
        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        innerGrid = safe_cast<Grid^>(grid->FindName(L"InnerGrid"));
        VERIFY_IS_NOT_NULL(innerGrid);

        grid->Transform3D = ref new PerspectiveTransform3D();
        VERIFY_IS_NOT_NULL(grid->Transform3D);

        innerGrid->Transform3D = GenerateScaleTransform3D(0.5f, 0.5f, 1.0f);
        VERIFY_IS_NOT_NULL(innerGrid->Transform3D);

        button->RenderTransform = GenerateRenderTransform();
        VERIFY_IS_NOT_NULL(button->RenderTransform);

    });

    HitTestBasic::VerifyTappedPoints(grid, button, points);

    ::Windows::Foundation::Rect expectedBounds = {60.0f,60.0f,100.0f,100.0f};
    VerifyGetGlobalBounds(button, &expectedBounds);
}

void HitTestTransform3D::PerspectiveOuterRenderTransformTranslateInnerRotationChild()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    points.push_back(wf::Point(71.0f, 71.0f)); // Top left
    points.push_back(wf::Point(269.0f, 71.0f)); // Top right
    points.push_back(wf::Point(83.0f, 203.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);

    xaml_controls::Button^ button = nullptr;
    Grid^ innerGrid = nullptr;

    RunOnUIThread([&]()
    {
        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        innerGrid = safe_cast<Grid^>(grid->FindName(L"InnerGrid"));
        VERIFY_IS_NOT_NULL(innerGrid);

        grid->Transform3D = ref new PerspectiveTransform3D();
        VERIFY_IS_NOT_NULL(grid->Transform3D);

        innerGrid->RenderTransform = GenerateRenderTransform();
        VERIFY_IS_NOT_NULL(innerGrid->RenderTransform);

        button->Transform3D = GenerateRotationTransform3D(45, 0, 0);
        VERIFY_IS_NOT_NULL(button->Transform3D);
    });

    HitTestBasic::VerifyTappedPoints(grid, button, points);

    ::Windows::Foundation::Rect expectedBounds = {70.0f,70.0f,200.0f,133.81f};
    VerifyGetGlobalBounds(button, &expectedBounds);
}

void HitTestTransform3D::PerspectiveOuterRotationInnerProjectionChildInternal(bool useTranslation, bool isWUCMode)
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    std::vector<wf::Point> pointsWithoutTranslation;
    std::vector<wf::Point> pointsWithTranslation;

    pointsWithoutTranslation.push_back(wf::Point(51.0f, 75.0f)); // Top left
    pointsWithoutTranslation.push_back(wf::Point(201.0f, 62.0f)); // Top right
    pointsWithoutTranslation.push_back(wf::Point(55.0f, 213.0f)); // Bottom left

    pointsWithTranslation.push_back(wf::Point(60.0f, 95.0f)); // Top left
    pointsWithTranslation.push_back(wf::Point(220.0f, 85.0f)); // Top right
    pointsWithTranslation.push_back(wf::Point(68.0f, 237.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);

    xaml_controls::Button^ button = nullptr;
    Grid^ innerGrid = nullptr;
    Microsoft::UI::Composition::Visual^ handOffVisual = nullptr;

    RunOnUIThread([&]()
    {
        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        innerGrid = safe_cast<Grid^>(grid->FindName(L"InnerGrid"));
        VERIFY_IS_NOT_NULL(innerGrid);

        grid->Transform3D = ref new PerspectiveTransform3D();
        VERIFY_IS_NOT_NULL(grid->Transform3D);

        innerGrid->Transform3D = GenerateRotationTransform3D(0, 45, 0);
        VERIFY_IS_NOT_NULL(innerGrid->Transform3D);

        points = pointsWithoutTranslation;
        if (useTranslation)
        {
            if (!isWUCMode)
            {
                // If we're not in WUC mode, we can add the TranslateTransform now.
                // If we're in WUC mode this combination is currently not supported.
                button->RenderTransform = GenerateRenderTransform();
                VERIFY_IS_NOT_NULL(button->RenderTransform);
                points = pointsWithTranslation;
            }
        }

        button->Projection = GeneratePlaneProjection();
        VERIFY_IS_NOT_NULL(button->Projection);
    });

    HitTestBasic::VerifyTappedPoints(grid, button, points);

    // TODO: Fix global bounds with projection/Transform3D interop
    //Windows::Foundation::Rect expectedBounds = {0.0f,0.0f,0.0f,0.0f};
    //VerifyGetGlobalBounds(button, &expectedBounds);

    // Now run the test case again, this time after having requested the HandOff visual, which triggers a different code path.
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Requesting HandOff Visual");
        handOffVisual = safe_cast<Microsoft::UI::Composition::Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(button));

        if (useTranslation)
        {
            if (isWUCMode)
            {
                // If we're in WUC mode, we do support Translation + Projection, test this combination by adding a Translation now.
                ElementCompositionPreview::SetIsTranslationEnabled(button, true);
                handOffVisual->Properties->InsertVector3(L"Translation", {20, 20, 0});
            }
            points = pointsWithTranslation;
        }
        else
        {
            points = pointsWithoutTranslation;
        }
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::WindowHelper->WaitForIdle();
    HitTestBasic::VerifyTappedPoints(grid, button, points);
}

void HitTestTransform3D::PerspectiveOuterRotationProjectionChild()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    points.push_back(wf::Point(47.0f, 75.0f)); // Top left
    points.push_back(wf::Point(202.0f, 62.0f)); // Top right
    points.push_back(wf::Point(56.0f, 215.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);

    xaml_controls::Button^ button = nullptr;

    RunOnUIThread([&]()
    {
        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        grid->Transform3D = ref new PerspectiveTransform3D();
        VERIFY_IS_NOT_NULL(grid->Transform3D);

        button->Transform3D = GenerateRotationTransform3D(0, 45, 0);
        VERIFY_IS_NOT_NULL(button->Transform3D);
        button->Projection = GeneratePlaneProjection();
        VERIFY_IS_NOT_NULL(button->Projection);
    });

    HitTestBasic::VerifyTappedPoints(grid, button, points);

    // TODO: Fix global bounds with projection/Transform3D interop
    //Windows::Foundation::Rect expectedBounds = {0.0f,0.0f,0.0f,0.0f};
    //VerifyGetGlobalBounds(button, &expectedBounds);
}

void HitTestTransform3D::RenderTransformTranslateOuterPerspectiveInnerRotationProjectionChild()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    points.push_back(wf::Point(67.0f, 95.0f)); // Top left
    points.push_back(wf::Point(222.0f, 82.0f)); // Top right
    points.push_back(wf::Point(76.0f, 234.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);

    xaml_controls::Button^ button = nullptr;
    Grid^ innerGrid = nullptr;

    RunOnUIThread([&]()
    {
        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        innerGrid = safe_cast<Grid^>(grid->FindName(L"InnerGrid"));
        VERIFY_IS_NOT_NULL(innerGrid);

        grid->RenderTransform = GenerateRenderTransform();
        VERIFY_IS_NOT_NULL(grid->RenderTransform);

        innerGrid->Transform3D = ref new PerspectiveTransform3D();
        VERIFY_IS_NOT_NULL(innerGrid->Transform3D);

        button->Transform3D = GenerateRotationTransform3D(0, 45, 0);
        VERIFY_IS_NOT_NULL(button->Transform3D);
        button->Projection = GeneratePlaneProjection();
        VERIFY_IS_NOT_NULL(button->Projection);
    });

    HitTestBasic::VerifyTappedPoints(grid, button, points);

    // TODO: Fix global bounds with projection/Transform3D interop
    //Windows::Foundation::Rect expectedBounds = {62.27f,53.53f,215.45f,232.94f};
    //VerifyGetGlobalBounds(button, &expectedBounds);
}

void HitTestTransform3D::ChangeBoundsAndRetestSingleElement()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    points.push_back(wf::Point(51.0f, 51.0f)); // Top left
    points.push_back(wf::Point(197.0f, 34.0f)); // Top right
    points.push_back(wf::Point(51.0f, 249.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);
    xaml_controls::Button^ button = nullptr;

    Microsoft::UI::Xaml::Media::Media3D::CompositeTransform3D^ transform;

    RunOnUIThread([&]()
    {
        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        grid->Transform3D = ref new PerspectiveTransform3D();
        VERIFY_IS_NOT_NULL(grid->Transform3D);

        transform = GenerateRotationTransform3D(0, 45, 0);
        button->Transform3D = transform;
        VERIFY_IS_NOT_NULL(button->Transform3D);
    });

    HitTestBasic::VerifyTappedPoints(grid, button, points);

    ::Windows::Foundation::Rect expectedBounds = {50.0f,33.53f,148.24f,232.94f};
    VerifyGetGlobalBounds(button, &expectedBounds);

    std::vector<wf::Point> translatedPoints;
    translatedPoints.push_back(wf::Point(71.0f, 71.0f)); // Top left
    // NOTE: The top right point in this case is not simply the previous top right point += 20.
    // Since the perspective isn't changed here (it's in the center of the original grid), when
    // the button is translated, the rotation relative to the camera respects the original perspective.
    // This is desired behavior.
    translatedPoints.push_back(wf::Point(220.0f, 59.0f)); // Top right
    translatedPoints.push_back(wf::Point(71.0f, 269.0f)); // Bottom left

    RunOnUIThread([&]()
    {
        transform->TranslateX = 20;
        transform->TranslateY = 20;
    });

    LOG_OUTPUT(L"## Verify points after translation ##");
    HitTestBasic::VerifyTappedPoints(grid, button, translatedPoints);

    ::Windows::Foundation::Rect translatedBounds = {70.0f,56.82f,151.54f,232.94f};
    VerifyGetGlobalBounds(button, &translatedBounds);
}

void HitTestTransform3D::AddDepth()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    points.push_back(wf::Point(51.0f, 51.0f)); // Top left
    points.push_back(wf::Point(249.0f, 51.0f)); // Top right
    points.push_back(wf::Point(51.0f, 249.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);
    xaml_controls::Button^ button = nullptr;

    Microsoft::UI::Xaml::Media::Media3D::CompositeTransform3D^ transform;

    RunOnUIThread([&]()
    {
        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        grid->Transform3D = ref new PerspectiveTransform3D();
        VERIFY_IS_NOT_NULL(grid->Transform3D);

        transform = GenerateRotationTransform3D(0, 0, 0);
        button->Transform3D = transform;
        VERIFY_IS_NOT_NULL(button->Transform3D);
    });

    HitTestBasic::VerifyTappedPoints(grid, button, points);

    ::Windows::Foundation::Rect expectedBounds = {50.0f,50.0f,200.0f,200.0f};
    VerifyGetGlobalBounds(button, &expectedBounds);

    std::vector<wf::Point> rotatedPoints;
    rotatedPoints.push_back(wf::Point(51.0f, 51.0f)); // Top left
    rotatedPoints.push_back(wf::Point(197.0f, 34.0f)); // Top right
    rotatedPoints.push_back(wf::Point(51.0f, 249.0f)); // Bottom left

    RunOnUIThread([&]()
    {
        transform->RotationY = 45;
    });

    LOG_OUTPUT(L"## Verify points after depth ##");
    HitTestBasic::VerifyTappedPoints(grid, button, rotatedPoints);

    ::Windows::Foundation::Rect rotatedBounds = {50.0f,33.53f,148.24f,232.94f};
    VerifyGetGlobalBounds(button, &rotatedBounds);
}

void HitTestTransform3D::LoseDepth()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> rotatedPoints;
    rotatedPoints.push_back(wf::Point(51.0f, 51.0f)); // Top left
    rotatedPoints.push_back(wf::Point(197.0f, 34.0f)); // Top right
    rotatedPoints.push_back(wf::Point(51.0f, 249.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);
    xaml_controls::Button^ button = nullptr;

    Microsoft::UI::Xaml::Media::Media3D::CompositeTransform3D^ transform;

    RunOnUIThread([&]()
    {
        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        grid->Transform3D = ref new PerspectiveTransform3D();
        VERIFY_IS_NOT_NULL(grid->Transform3D);

        transform = GenerateRotationTransform3D(0, 45, 0);
        button->Transform3D = transform;
        VERIFY_IS_NOT_NULL(button->Transform3D);
    });
    HitTestBasic::VerifyTappedPoints(grid, button, rotatedPoints);

    ::Windows::Foundation::Rect rotatedBounds = {50.0f,33.53f,148.24f,232.94f};
    VerifyGetGlobalBounds(button, &rotatedBounds);

    std::vector<wf::Point> points;
    points.push_back(wf::Point(51.0f, 51.0f)); // Top left
    points.push_back(wf::Point(249.0f, 51.0f)); // Top right
    points.push_back(wf::Point(51.0f, 249.0f)); // Bottom left

    RunOnUIThread([&]()
    {
        transform->RotationY = 0;
    });

    LOG_OUTPUT(L"## Verify points after depth ##");
    HitTestBasic::VerifyTappedPoints(grid, button, points);

    ::Windows::Foundation::Rect expectedBounds = {50.0f,50.0f,200.0f,200.0f};
    VerifyGetGlobalBounds(button, &expectedBounds);
}

void HitTestTransform3D::ChangeBoundsParentAndRetestChild()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    points.push_back(wf::Point(51.0f, 51.0f)); // Top left
    points.push_back(wf::Point(197.0f, 34.0f)); // Top right
    points.push_back(wf::Point(51.0f, 249.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);
    Grid^ innerGrid = nullptr;
    xaml_controls::Button^ button = nullptr;

    Microsoft::UI::Xaml::Media::Media3D::CompositeTransform3D^ transform;

    RunOnUIThread([&]()
    {
        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        grid->Transform3D = ref new PerspectiveTransform3D();
        VERIFY_IS_NOT_NULL(grid->Transform3D);

        innerGrid = safe_cast<Grid^>(grid->FindName(L"InnerGrid"));
        VERIFY_IS_NOT_NULL(innerGrid);

        transform = GenerateRotationTransform3D(0, 45, 0);
        innerGrid->Transform3D = transform;
        VERIFY_IS_NOT_NULL(innerGrid->Transform3D);
    });

    HitTestBasic::VerifyTappedPoints(grid, button, points);

    ::Windows::Foundation::Rect expectedBounds = {50.0f,33.53f,148.24f,232.94f};
    VerifyGetGlobalBounds(button, &expectedBounds);

    std::vector<wf::Point> translatedPoints;
    translatedPoints.push_back(wf::Point(71.0f, 71.0f)); // Top left
    translatedPoints.push_back(wf::Point(220.0f, 59.0f)); // Top right
    translatedPoints.push_back(wf::Point(71.0f, 269.0f)); // Bottom left

    RunOnUIThread([&]()
    {
        transform->TranslateX = 20;
        transform->TranslateY = 20;
    });

    // Verify that the child button's bounds are correct when the parent's transform changes
    LOG_OUTPUT(L"## Verify points after translation ##");
    HitTestBasic::VerifyTappedPoints(grid, button, translatedPoints);

    ::Windows::Foundation::Rect translatedBounds = {70.0f,56.82f,151.54f,232.94f};
    VerifyGetGlobalBounds(button, &translatedBounds);
}

void HitTestTransform3D::ScaleXYNonDefaultCenter()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    points.push_back(wf::Point(93.0f, 93.0f)); // Top left
    points.push_back(wf::Point(248.0f, 93.0f)); // Top right
    points.push_back(wf::Point(93.0f, 248.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);
    xaml_controls::Button^ button = nullptr;

    RunOnUIThread([&]()
    {
        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        auto transform = GenerateScaleTransform3D(0.8f, 0.8f, 1.0f);
        transform->CenterX = 200;
        transform->CenterY = 200;

        button->Transform3D = transform;
        VERIFY_IS_NOT_NULL(button->Transform3D);
    });

    HitTestBasic::VerifyTappedPoints(grid, button, points);

    ::Windows::Foundation::Rect expectedBounds = {90.0f,90.0f,160.0f,160.0f};
    VerifyGetGlobalBounds(button, &expectedBounds);
}

void HitTestTransform3D::RotationXNonDefaultCenter()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    points.push_back(wf::Point(37.0f, 105.0f)); // Top left
    points.push_back(wf::Point(265.0f, 105.0f)); // Top right
    points.push_back(wf::Point(51.0f, 249.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);
    xaml_controls::Button^ button = nullptr;

    RunOnUIThread([&]()
    {
        grid->Transform3D = ref new PerspectiveTransform3D();
        VERIFY_IS_NOT_NULL(grid->Transform3D);

        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        auto transform = GenerateRotationTransform3D(45, 0, 0);
        transform->CenterX = 200;
        transform->CenterY = 200;

        button->Transform3D = transform;
        VERIFY_IS_NOT_NULL(button->Transform3D);
    });

    // This test is consistently failing in RI-TP gates and the failure can't be reproduced anywhere else, even when running
    // the test from the RI-TP machine that hit the failure. It looks like the tap is missing, so switch to programmatic hit
    // testing instead. There are plenty of other tests to make sure that tap input works.
    HitTestBasic::VerifyTappedPoints(grid, button, points, 15 /*offset*/, false /*verify using taps*/);

    ::Windows::Foundation::Rect expectedBounds = {33.53f,101.76f,232.94f,148.24f};
    VerifyGetGlobalBounds(button, &expectedBounds);
}

void HitTestTransform3D::ScaleXYRotationXNonDefaultCenter()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    points.push_back(wf::Point(85.0f, 137.0f)); // Top left
    points.push_back(wf::Point(260.0f, 136.0f)); // Top right
    points.push_back(wf::Point(92.0f, 249.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"GridAndButton.xaml"));
    VERIFY_IS_NOT_NULL(grid);
    xaml_controls::Button^ button = nullptr;

    RunOnUIThread([&]()
    {
        grid->Transform3D = ref new PerspectiveTransform3D();
        VERIFY_IS_NOT_NULL(grid->Transform3D);

        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        auto transform = GenerateCompositeTransform3D(0.8f, 0.8f, 1.0f,
                                                      0.0f, 0.0f, 0.0f,
                                                      45.0f, 0.0f, 0.0f);
        transform->CenterX = 200;
        transform->CenterY = 200;

        button->Transform3D = transform;
        VERIFY_IS_NOT_NULL(button->Transform3D);
    });

    HitTestBasic::VerifyTappedPoints(grid, button, points);

    ::Windows::Foundation::Rect expectedBounds = {82.35f,135.19f,180.41f,114.81f};
    VerifyGetGlobalBounds(button, &expectedBounds);
}

void HitTestTransform3D::ClipOnlyPerspective()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    points.push_back(wf::Point(51.0f, 51.0f)); // Top left
    points.push_back(wf::Point(249.0f, 51.0f)); // Top right
    points.push_back(wf::Point(51.0f, 249.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"NestedGridsAndClips.xaml"));
    VERIFY_IS_NOT_NULL(grid);
    xaml_controls::Button^ button = nullptr;

    RunOnUIThread([&]()
    {
        grid->Transform3D = ref new PerspectiveTransform3D();
        VERIFY_IS_NOT_NULL(grid->Transform3D);

        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);
    });

    HitTestBasic::VerifyTappedPoints(grid, button, points);

    // Since adding a perspective does not introduce depth, we do not expect it to affect this
    // subtree, since we go down the 2D path.
    ::Windows::Foundation::Rect expectedBounds = {50.0f,50.0f,200.0f,200.0f};
    VerifyGetGlobalBounds(button, &expectedBounds);
}

void HitTestTransform3D::ClipWithChildRotation()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    std::vector<wf::Point> points;
    points.push_back(wf::Point(51.0f, 51.0f)); // Top left
    points.push_back(wf::Point(192.0f, 51.0f)); // Top right
    points.push_back(wf::Point(51.0f, 249.0f)); // Bottom left

    Grid^ grid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"NestedGridsAndClips.xaml"));
    VERIFY_IS_NOT_NULL(grid);
    xaml_controls::Button^ button = nullptr;

    RunOnUIThread([&]()
    {
        grid->Transform3D = ref new PerspectiveTransform3D();
        VERIFY_IS_NOT_NULL(grid->Transform3D);

        button = safe_cast<Button^>(grid->FindName(L"ChildButton"));
        VERIFY_IS_NOT_NULL(button);

        button->Transform3D = GenerateRotationTransform3D(0,-45,0);
        VERIFY_IS_NOT_NULL(button->Transform3D);
    });

    HitTestBasic::VerifyTappedPoints(grid, button, points);

    // TODO: Fix global bounds under clipping
    //Windows::Foundation::Rect expectedBounds = {50.0f,50.0f,0.0f,200.0f};
    //VerifyGetGlobalBounds(button, &expectedBounds);
}

void HitTestTransform3D::DefaultCompositeAndPerspectiveWUC()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    DefaultCompositeAndPerspectiveInternal();
}

void HitTestTransform3D::PerspectivePlusPopupScaledWUC()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

    // It's important to have a plateau scale set...
    TestServices::WindowHelper->SetWindowSizeOverrideWithScale(wf::Size(500, 500), 2.0f);

    Grid^ root = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"PerspectivePlusPopup.xaml"));
    xaml_shapes::Rectangle^ targetRectangle;

    RunOnUIThread([&]()
    {
        auto rectWithPerspective = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"myRectangle"));
        targetRectangle = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"targetRectangle"));
        auto handOffVisual = safe_cast<Microsoft::UI::Composition::Visual^>(Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(rectWithPerspective));
        ::Windows::Foundation::Numerics::float4x4 perspectiveMatrix =
        {
            1.0f, // M11
            0.0, // M12
            0.0f, // M13
            0.0f, // M14
            0.0f, // M21
            1.0f, // M22
            0.0f, // M23
            0.0f, // M24
            0.0f, // M31
            0.0f, // M32
            1.0f, // M33
            -0.0001f, // M34
            0.0f, // M41
            0.0f, // M42
            0.0f, // M43
            1.0f  // M44
        };
        handOffVisual->TransformMatrix = perspectiveMatrix;

        TestServices::WindowHelper->WindowContent = root;
    });
    TestServices::WindowHelper->WaitForIdle();

    std::vector<wf::Point> points;
    RunOnUIThread([&]()
    {
        points = HitTestBasic::GetHitTestingPoints(static_cast<FrameworkElement^>(targetRectangle), root);

    });
    TestServices::WindowHelper->WaitForIdle();

    HitTestBasic::VerifyTappedPointsNoWindowSetup(true /* specifySubtreeRootElement */, targetRectangle, points);
}

void HitTestTransform3D::ScaleXYSmallWUC()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    ScaleXYSmallInternal();
}

void HitTestTransform3D::TranslateXYWUC()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    TranslateXYInternal();
}

void HitTestTransform3D::RotationXDefaultPerspectiveWUC()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    RotationXDefaultPerspectiveInternal();
}

void HitTestTransform3D::ScaleAndRenderTransformTranslateSameElementWUC()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    ScaleAndRenderTransformTranslateSameElementInternal();
}

void HitTestTransform3D::PerspectiveOuterRotationInnerProjectionChildWUC()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    PerspectiveOuterRotationInnerProjectionChildInternal(false /*useTranslation*/, true /*isWUCMode*/);
}

void HitTestTransform3D::PerspectiveOuterRotationInnerTranslationProjectionChildWUC()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    PerspectiveOuterRotationInnerProjectionChildInternal(true /*useTranslation*/, true /*isWUCMode*/);
}

void HitTestTransform3D::VerifyGetGlobalBounds(UIElement^ element, const ::Windows::Foundation::Rect* expectedRect)
{
    LOG_OUTPUT(L"Testing GetGlobalBounds");

    ::Windows::Foundation::Rect actualRect = TestServices::WindowHelper->TestGetGlobalBoundsForUIElement(element, false);

    VERIFY_IS_TRUE(HitTestTransform3D::CompareRectsWithEpsilon(expectedRect, &actualRect));
}

void HitTestTransform3D::VerifyGetGlobalBounds(UIElement^ element, const ::Windows::Foundation::Rect& expectedRect1, const ::Windows::Foundation::Rect& expectedRect2)
{
    LOG_OUTPUT(L"> Testing GetGlobalBounds, with two possible outcomes");

    ::Windows::Foundation::Rect actualRect = TestServices::WindowHelper->TestGetGlobalBoundsForUIElement(element, false);

    const bool isOne = HitTestTransform3D::CompareRectsWithEpsilon(&expectedRect1, &actualRect);
    const bool isTwo = HitTestTransform3D::CompareRectsWithEpsilon(&expectedRect2, &actualRect);
    VERIFY_IS_TRUE(isOne || isTwo);
}

bool HitTestTransform3D::CompareRectsWithEpsilon(const ::Windows::Foundation::Rect* expectedRect, const ::Windows::Foundation::Rect* actualRect, float epsilon)
{
    bool areEqual = CompareFloatsWithEpsilon(expectedRect->X, actualRect->X, epsilon)
        && CompareFloatsWithEpsilon(expectedRect->Y, actualRect->Y, epsilon)
        && CompareFloatsWithEpsilon(expectedRect->Width, actualRect->Width, epsilon)
        && CompareFloatsWithEpsilon(expectedRect->Height, actualRect->Height, epsilon);

    if (!areEqual)
    {
        LOG_OUTPUT(L"Expect: [X: %f, Y: %f, W: %f, H: %f]", expectedRect->X, expectedRect->Y, expectedRect->Width, expectedRect->Height);
        LOG_OUTPUT(L"Actual: [X: %f, Y: %f, W: %f, H: %f]", actualRect->X, actualRect->Y, actualRect->Width, actualRect->Height);
    }

    return areEqual;
}

bool HitTestTransform3D::CompareFloatsWithEpsilon(float a, float b, float epsilon)
{
    return (a - b < epsilon) && (b - a < epsilon);
}

Microsoft::UI::Xaml::Media::TranslateTransform^ HitTestTransform3D::GenerateRenderTransform()
{
    auto renderTransform = ref new TranslateTransform();
    renderTransform->X = 20;
    renderTransform->Y = 20;
    return renderTransform;
}

Microsoft::UI::Xaml::Media::PlaneProjection^ HitTestTransform3D::GeneratePlaneProjection()
{
    PlaneProjection^ projection = ref new PlaneProjection();
    projection->RotationX = 45;
    return projection;
}

Microsoft::UI::Xaml::Media::Media3D::CompositeTransform3D^ HitTestTransform3D::GenerateCompositeTransform3D(float scaleX, float scaleY, float scaleZ,
                                                                                                          float translateX, float translateY, float translateZ,
                                                                                                          float rotationX, float rotationY, float rotationZ)
{
    CompositeTransform3D^ compositeTransform3D = ref new CompositeTransform3D();
    compositeTransform3D->ScaleX = scaleX;
    compositeTransform3D->ScaleY = scaleY;
    compositeTransform3D->ScaleZ = scaleZ;
    compositeTransform3D->TranslateX = translateX;
    compositeTransform3D->TranslateY = translateY;
    compositeTransform3D->TranslateZ = translateZ;
    compositeTransform3D->RotationX = rotationX;
    compositeTransform3D->RotationY = rotationY;
    compositeTransform3D->RotationZ = rotationZ;

    return compositeTransform3D;
}

Microsoft::UI::Xaml::Media::Media3D::CompositeTransform3D^ HitTestTransform3D::GenerateScaleTransform3D(float scaleX, float scaleY, float scaleZ)
{
    return GenerateCompositeTransform3D(scaleX,scaleY,scaleZ, 0,0,0, 0,0,0);
}

Microsoft::UI::Xaml::Media::Media3D::CompositeTransform3D^ HitTestTransform3D::GenerateTranslateTransform3D(float translateX, float translateY, float translateZ)
{
    return GenerateCompositeTransform3D(1.0f,1.0f,1.0f, translateX,translateY,translateZ, 0,0,0);
}

Microsoft::UI::Xaml::Media::Media3D::CompositeTransform3D^ HitTestTransform3D::GenerateRotationTransform3D(float rotationX, float rotationY, float rotationZ)
{
    return GenerateCompositeTransform3D(1.0f,1.0f,1.0f, 0,0,0, rotationX,rotationY,rotationZ);
}


} } } } } } }
