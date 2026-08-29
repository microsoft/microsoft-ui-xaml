// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "HitTestStickyHeaders.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <FileLoader.h>
#include <TestCleanupWrapper.h>
#include <RuntimeEnabledFeatureOverride.h>
#include "collection.h"
#include "HitTestBasic.h"
#include "HitTestTransform3D.h"

#include "StickyHeadersHelper.h"

using namespace std;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Media3D;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace ::Windows::Foundation::Collections;

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Input { namespace HitTest {

Platform::String^ HitTestStickyHeaders::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\input\\HitTest\\";
}

bool HitTestStickyHeaders::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool HitTestStickyHeaders::TestCleanup()
{
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void HitTestStickyHeaders::NoTx3D()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    int numGroups = 2;

    xaml_controls::Grid^ rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"StickyHeadersWithButtons.xaml"));
    VERIFY_IS_NOT_NULL(rootGrid);

    // Set up the sticky header item source, etc.
    SetupStickyHeaders(rootGrid, numGroups);

    // Set the window content
    HitTestBasic::WindowSetup(rootGrid);

    // Get the sticky headers that we'll tap
    std::vector<UIElement^> headers = GetHeaderElements(rootGrid, numGroups);

    std::vector<wf::Point> points0;
    points0.push_back(wf::Point(51.0f, 51.0f)); // Top left
    points0.push_back(wf::Point(249.0f, 51.0f)); // Top right
    points0.push_back(wf::Point(51.0f, 99.0f)); // Bottom left
    HitTestBasic::VerifyTappedPoints(rootGrid, headers[0], points0, 20, true);

    std::vector<wf::Point> points1;
    points1.push_back(wf::Point(51.0f, 334.0f)); // Top left
    points1.push_back(wf::Point(249.0f, 334.0f)); // Top right
    points1.push_back(wf::Point(51.0f, 382.0f)); // Bottom left
    HitTestBasic::VerifyTappedPoints(rootGrid, headers[1], points1, 20, true);

    ::Windows::Foundation::Rect expectedBounds = {50.0f,50.0f,200.0f,59.0f};
    HitTestTransform3D::VerifyGetGlobalBounds(headers[0], &expectedBounds);

    expectedBounds = {50.0f,333.0f,200.0f,59.0f};
    HitTestTransform3D::VerifyGetGlobalBounds(headers[1], &expectedBounds);
}

void HitTestStickyHeaders::Tx3DDefaultPerspective()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    int numGroups = 2;

    xaml_controls::Grid^ rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"StickyHeadersWithButtons.xaml"));
    VERIFY_IS_NOT_NULL(rootGrid);

    // Set up the sticky header item source, etc.
    SetupStickyHeaders(rootGrid, numGroups);

    // Apply any transforms
    xaml_controls::Grid^ innerGrid = nullptr;
    RunOnUIThread([&]()
    {
        innerGrid = safe_cast<xaml_controls::Grid^>(rootGrid->FindName(L"innerGrid"));
        VERIFY_IS_NOT_NULL(innerGrid);

        innerGrid->Transform3D = ref new PerspectiveTransform3D();
    });

    // Set the window content
    HitTestBasic::WindowSetup(rootGrid);

    // Get the sticky headers that we'll tap
    std::vector<UIElement^> headers = GetHeaderElements(rootGrid, numGroups);

    std::vector<wf::Point> points0;
    points0.push_back(wf::Point(51.0f, 51.0f)); // Top left
    points0.push_back(wf::Point(249.0f, 51.0f)); // Top right
    points0.push_back(wf::Point(51.0f, 99.0f)); // Bottom left
    HitTestBasic::VerifyTappedPoints(rootGrid, headers[0], points0, 20, true);

    std::vector<wf::Point> points1;
    points1.push_back(wf::Point(51.0f, 334.0f)); // Top left
    points1.push_back(wf::Point(249.0f, 334.0f)); // Top right
    points1.push_back(wf::Point(51.0f, 382.0f)); // Bottom left
    HitTestBasic::VerifyTappedPoints(rootGrid, headers[1], points1, 20, true);

    xaml_controls::Button^ headerButton = GetHeaderButton(headers[0]);
    ::Windows::Foundation::Rect expectedBounds = {50.0f,50.0f,200.0f,50.0f};
    HitTestTransform3D::VerifyGetGlobalBounds(headerButton, &expectedBounds);

    headerButton = GetHeaderButton(headers[1]);
    expectedBounds = {50.0f,333.0f,200.0f,50.0f};
    HitTestTransform3D::VerifyGetGlobalBounds(headerButton, &expectedBounds);
}

void HitTestStickyHeaders::Tx3DRotYDefaultPerspective()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    int numGroups = 2;

    xaml_controls::Grid^ rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"StickyHeadersWithButtons.xaml"));
    VERIFY_IS_NOT_NULL(rootGrid);

    // Set up the sticky header item source, etc.
    SetupStickyHeaders(rootGrid, numGroups);

    // Apply any transforms
    xaml_controls::Grid^ innerGrid = nullptr;
    RunOnUIThread([&]()
    {
        innerGrid = safe_cast<xaml_controls::Grid^>(rootGrid->FindName(L"innerGrid"));
        VERIFY_IS_NOT_NULL(innerGrid);

        rootGrid->Transform3D = ref new PerspectiveTransform3D();

        CompositeTransform3D^ transform = ref new CompositeTransform3D();
        transform->RotationY = 30;
        innerGrid->Transform3D = transform;
    });

    // Set the window content
    HitTestBasic::WindowSetup(rootGrid);

    // Get the sticky headers that we'll tap
    std::vector<UIElement^> headers = GetHeaderElements(rootGrid, numGroups);

    std::vector<wf::Point> points0;
    points0.push_back(wf::Point(51.0f, 51.0f)); // Top left

// TODO_WinRT:  Fix weird clipping which is causing the top right point to fail validation
//    points0.push_back(wf::Point(216.0f, 33.0f)); // Top right

    points0.push_back(wf::Point(51.0f, 99.0f)); // Bottom left
    HitTestBasic::VerifyTappedPoints(rootGrid, headers[0], points0, 20, true);

    std::vector<wf::Point> points1;
    points1.push_back(wf::Point(51.0f, 334.0f)); // Top left
    points1.push_back(wf::Point(216.0f, 344.0f)); // Top right
    points1.push_back(wf::Point(51.0f, 382.0f)); // Bottom left
    HitTestBasic::VerifyTappedPoints(rootGrid, headers[1], points1, 20, true);

    xaml_controls::Button^ headerButton = GetHeaderButton(headers[0]);
    ::Windows::Foundation::Rect expectedBounds = {50.0f,27.78f,170.23f,72.22f};
    HitTestTransform3D::VerifyGetGlobalBounds(headerButton, &expectedBounds);

    headerButton = GetHeaderButton(headers[1]);
    expectedBounds = {50.0f,333.0f,170.23f,64.78f};
    HitTestTransform3D::VerifyGetGlobalBounds(headerButton, &expectedBounds);
}

void HitTestStickyHeaders::Tx3DRotYOnHeadersDefaultPerspective()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    int numGroups = 2;

    xaml_controls::Grid^ rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"StickyHeadersWithTransform3DButtons.xaml"));
    VERIFY_IS_NOT_NULL(rootGrid);

    // Set up the sticky header item source, etc.
    SetupStickyHeaders(rootGrid, numGroups);

    // Apply any transforms
    xaml_controls::Grid^ innerGrid = nullptr;
    RunOnUIThread([&]()
    {
        innerGrid = safe_cast<xaml_controls::Grid^>(rootGrid->FindName(L"innerGrid"));
        VERIFY_IS_NOT_NULL(innerGrid);

        rootGrid->Transform3D = ref new PerspectiveTransform3D();
    });

    // Set the window content
    HitTestBasic::WindowSetup(rootGrid);

    // Get the sticky headers that we'll tap
    std::vector<UIElement^> headers = GetHeaderElements(rootGrid, numGroups);

    std::vector<wf::Point> points0;
    points0.push_back(wf::Point(51.0f, 51.0f)); // Top left
    points0.push_back(wf::Point(219.0f, 51.0f)); // Top right
    points0.push_back(wf::Point(51.0f, 99.0f)); // Bottom left
    HitTestBasic::VerifyTappedPoints(rootGrid, headers[0], points0, 20, true);

    std::vector<wf::Point> points1;
    points1.push_back(wf::Point(51.0f, 334.0f)); // Top left
    points1.push_back(wf::Point(219.0f, 344.0f)); // Top right
    points1.push_back(wf::Point(51.0f, 382.0f)); // Bottom left
    HitTestBasic::VerifyTappedPoints(rootGrid, headers[1], points1, 20, true);

    // TODO: The HasDepth flag does not properly propagate up through LTEs (like the HeaderItems themselves)
    // However, the button below the LTE will still return the correct bounds since
    // we propagate HasDepth to an ancestor above the LTE.

    xaml_controls::Button^ headerButton = GetHeaderButton(headers[0]);
    ::Windows::Foundation::Rect expectedBounds = {50.0f,27.78f,170.23f,72.22f};
    HitTestTransform3D::VerifyGetGlobalBounds(headerButton, &expectedBounds);

    headerButton = GetHeaderButton(headers[1]);
    expectedBounds = {50.0f,333.0f,170.23f,64.78f};
    HitTestTransform3D::VerifyGetGlobalBounds(headerButton, &expectedBounds);
}

std::vector<UIElement^> HitTestStickyHeaders::GetHeaderElements(xaml_controls::Grid^ rootGrid, int numGroups)
{
    std::vector<UIElement^> headers;
    RunOnUIThread([&](){
        xaml_controls::ListView^ listView = safe_cast<xaml_controls::ListView^>(rootGrid->FindName(L"listView"));
        VERIFY_IS_NOT_NULL(listView);

        // Unfortunately ListView has no API to retrieve group headers.
        // The workaround is to dig into its ItemsStackPanel, which has everyone in a flat list.
        // Headers are always stored first in the ItemsStackPanel collection, so we're interested
        // in the first items.
        for (int i = 0; i < numGroups; i++)
        {
            headers.push_back(safe_cast<UIElement^>(listView->ItemsPanelRoot->Children->GetAt(i)));
            VERIFY_IS_NOT_NULL(headers[i]);
        }
    });
    TestServices::WindowHelper->WaitForIdle();
    return headers;
}

xaml_controls::Button^ HitTestStickyHeaders::GetHeaderButton(UIElement^ parentControl)
{
    // Don't infinite loop if we can't find our button. Break at depth 10.
    int breaker = 10;
    DependencyObject^ element = parentControl;

    // Assumes a linear tree starting at parentControl, with depth <= 10, containing a WUX.Controls.Button.
    RunOnUIThread([&](){
        while (breaker > 0 && element != nullptr && element->GetType()->FullName != ref new Platform::String(L"Microsoft.UI.Xaml.Controls.Button"))
        {
            element = VisualTreeHelper::GetChild(element, 0);
            breaker--;
        }
    });

    VERIFY_IS_NOT_NULL(element);
    return static_cast<Button^>(element);
}

void HitTestStickyHeaders::SetupStickyHeaders(xaml_controls::Grid^ rootGrid, int numGroups)
{
    int numItemsPerGroup = 5;

    RunOnUIThread([&]()
    {
        xaml_data::CollectionViewSource^ cvs = safe_cast<xaml_data::CollectionViewSource^>(rootGrid->FindName(L"cvs"));
        VERIFY_IS_NOT_NULL(cvs);

        Platform::Collections::Vector<Platform::Object^>^ itemsSource = CreateGroupedData(numGroups, numItemsPerGroup);
        VERIFY_IS_NOT_NULL(itemsSource);

        cvs->Source = itemsSource;
    });
}

Platform::Collections::Vector<Platform::Object^>^ HitTestStickyHeaders::CreateGroupedData(int numGroups, int numItemsPerGroup)
{
    auto groupedData = ref new Platform::Collections::Vector<Platform::Object^>();

    for (int i = 0; i < numGroups; i++)
    {
        auto group = ref new Microsoft::UI::Xaml::Tests::Common::GroupedHeader(L"Group: " + i);
        VERIFY_IS_NOT_NULL(group);

        for (int j = 0; j < numItemsPerGroup; j++)
        {
            group->Append(L"Item: " + j);
        }
        groupedData->Append(group);
    }

    return groupedData;
}

} } } } } } }