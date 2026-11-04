// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "PathIconIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Local
{
    // IconNoGridOptimization removes the Grid used by FontIcon and BitmapIcon.
    // An app can still use a PathIcon as a workaround to inject custom content
    // inside an IconElement, such as with this custom ContentIcon subclass.
    ref class ContentIcon sealed : public xaml_controls::PathIcon
    {
    public:
        ContentIcon()
        {
            Loaded += ref new xaml::RoutedEventHandler(this, &ContentIcon::OnLoaded);
        }

        property xaml::UIElement^ Content;

    private:
        void OnLoaded(Platform::Object^, xaml::RoutedEventArgs^)
        {
            // PathIcon is expected to always have a Grid as the root of its subtree.
            VERIFY_ARE_EQUAL(1, xaml_media::VisualTreeHelper::GetChildrenCount(this));
            auto grid = dynamic_cast<xaml_controls::Grid^>(
                xaml_media::VisualTreeHelper::GetChild(this, 0));
            VERIFY_IS_NOT_NULL(grid);

            if (Content != nullptr)
            {
                grid->Children->Clear();
                grid->Children->Append(Content);
            }
        }
    };
}

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace PathIcon {

    bool PathIconIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool PathIconIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool PathIconIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void PathIconIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::PathIcon>::CanInstantiate();
    }

    void PathIconIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::PathIcon>::CanEnterAndLeaveLiveTree();
    }

    void PathIconIntegrationTests::CanSetAndGetProperties()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]
        {
            auto pathIcon = ref new xaml_controls::PathIcon();

            // Verify default values for PathIcon properties.
            VERIFY_IS_NULL(pathIcon->Data);

            // Verify default values for PathIcon properties.
            auto data = ref new xaml_media::PathGeometry();
            pathIcon->Data = data;
            VERIFY_IS_TRUE(pathIcon->Data->Equals(data));

            auto foreground = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
            pathIcon->Foreground = foreground;
            VERIFY_ARE_EQUAL(pathIcon->Foreground, foreground);
        });
    }

    void PathIconIntegrationTests::ChildAfterLayoutIsGrid()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::PathIcon^ pathIcon = nullptr;

        VERIFY_IS_TRUE(xaml_settings::XamlOptionalChanges::IsChangeEnabled(
            xaml_settings::XamlChangeId::IconNoGridOptimization));

        RunOnUIThread([&]
        {
            pathIcon = ref new xaml_controls::PathIcon();
            TestServices::WindowHelper->WindowContent = pathIcon;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]
        {
            VERIFY_ARE_EQUAL(1, xaml_media::VisualTreeHelper::GetChildrenCount(pathIcon));

            // IconNoGridOptimization removes the Grid used by FontIcon and BitmapIcon.
            // Verify that the Grid still exists for PathIcon, in case any app is using
            // PathIcon as a workaround to inject custom content inside an IconElement.
            VERIFY_IS_NOT_NULL(dynamic_cast<xaml_controls::Grid^>(
                xaml_media::VisualTreeHelper::GetChild(pathIcon, 0)));
        });
    }

    void PathIconIntegrationTests::PathIconSubclassCanReplaceGridContent()
    {
        TestCleanupWrapper cleanup;
        Local::ContentIcon^ contentIcon = nullptr;
        xaml_shapes::Rectangle^ content = nullptr;

        VERIFY_IS_TRUE(xaml_settings::XamlOptionalChanges::IsChangeEnabled(
            xaml_settings::XamlChangeId::IconNoGridOptimization));

        RunOnUIThread([&]
        {
            content = ref new xaml_shapes::Rectangle();
            contentIcon = ref new Local::ContentIcon();
            contentIcon->Content = content;

            auto root = ref new xaml_controls::Grid();
            root->Children->Append(contentIcon);
            TestServices::WindowHelper->WindowContent = root;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]
        {
            // Find the expected Grid created by PathIcon
            VERIFY_ARE_EQUAL(1, xaml_media::VisualTreeHelper::GetChildrenCount(contentIcon));
            auto grid = dynamic_cast<xaml_controls::Grid^>(
                xaml_media::VisualTreeHelper::GetChild(contentIcon, 0));
            VERIFY_IS_NOT_NULL(grid);

            // Verify that the ContentIcon class successfully inserted its custom content
            VERIFY_ARE_EQUAL(1u, grid->Children->Size);
            VERIFY_ARE_EQUAL(content, grid->Children->GetAt(0));
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::PathIcon
