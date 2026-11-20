// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "HubSectionFocusAndAutomationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TreeHelper.h>
#include <TestCleanupWrapper.h>

#undef GetClassName // Conflicts with automation peer method of the same name

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

#define XAML_NAMESPACES L"xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Hub {

    // A 480px wide Hub with three 400px sections: only the first section (and part of the second) is in view initially.
    static const wchar_t* s_hubWithDefaultSection2 =
        L"<Grid " XAML_NAMESPACES L" Width='480' Height='400' HorizontalAlignment='Left' VerticalAlignment='Top'>"
        L"  <Hub x:Name='hub' DefaultSectionIndex='2'>"
        L"    <HubSection x:Name='item1' Header='item1' Width='400' IsHeaderInteractive='True'><DataTemplate><Border Height='100' Background='Red'/></DataTemplate></HubSection>"
        L"    <HubSection x:Name='item2' Header='item2' Width='400' IsHeaderInteractive='True'><DataTemplate><Border Height='100' Background='Green'/></DataTemplate></HubSection>"
        L"    <HubSection x:Name='item3' Header='item3' Width='400' IsHeaderInteractive='True'><DataTemplate><Border Height='100' Background='Blue'/></DataTemplate></HubSection>"
        L"  </Hub>"
        L"</Grid>";

    // The default section (index 1) has neither Header nor HeaderTemplate but is itself a tab stop.
    static const wchar_t* s_hubWithHeaderlessDefaultSection =
        L"<Grid " XAML_NAMESPACES L" Width='480' Height='400' HorizontalAlignment='Left' VerticalAlignment='Top'>"
        L"  <Hub x:Name='hub' DefaultSectionIndex='1'>"
        L"    <HubSection x:Name='item1' Header='item1' Width='400' IsHeaderInteractive='True'><DataTemplate><Border Height='100' Background='Red'/></DataTemplate></HubSection>"
        L"    <HubSection x:Name='item2' Width='400' IsTabStop='True'><DataTemplate><Border Height='100' Background='Green'/></DataTemplate></HubSection>"
        L"    <HubSection x:Name='item3' Header='item3' Width='400' IsHeaderInteractive='True'><DataTemplate><Border Height='100' Background='Blue'/></DataTemplate></HubSection>"
        L"  </Hub>"
        L"</Grid>";

    static const wchar_t* s_hubWithoutDefaultSection =
        L"<Grid " XAML_NAMESPACES L" Width='480' Height='400' HorizontalAlignment='Left' VerticalAlignment='Top'>"
        L"  <Hub x:Name='hub'>"
        L"    <HubSection x:Name='item1' Header='item1' Width='400'><DataTemplate><Border Height='100' Background='Red'/></DataTemplate></HubSection>"
        L"    <HubSection x:Name='item2' Header='item2' Width='400'><DataTemplate><Border Height='100' Background='Green'/></DataTemplate></HubSection>"
        L"    <HubSection x:Name='item3' Header='item3' Width='400'><DataTemplate><Border Height='100' Background='Blue'/></DataTemplate></HubSection>"
        L"  </Hub>"
        L"</Grid>";

    // Hub as the SemanticZoom zoomed-in view; its section headers feed the zoomed-out GridView.
    static const wchar_t* s_hubInSemanticZoom =
        L"<Grid " XAML_NAMESPACES L" Width='480' Height='600' HorizontalAlignment='Left' VerticalAlignment='Top'>"
        L"  <SemanticZoom x:Name='sezo'>"
        L"    <SemanticZoom.ZoomedInView>"
        L"      <Hub x:Name='hub'>"
        L"        <HubSection x:Name='item1' Header='item1' Width='400' IsHeaderInteractive='True'><DataTemplate><Border Height='100' Background='Red'/></DataTemplate></HubSection>"
        L"        <HubSection x:Name='item2' Header='item2' Width='400' IsHeaderInteractive='True'><DataTemplate><Border Height='100' Background='Green'/></DataTemplate></HubSection>"
        L"        <HubSection x:Name='item3' Header='item3' Width='400' IsHeaderInteractive='True'><DataTemplate><Border Height='100' Background='Blue'/></DataTemplate></HubSection>"
        L"      </Hub>"
        L"    </SemanticZoom.ZoomedInView>"
        L"    <SemanticZoom.ZoomedOutView>"
        L"      <GridView x:Name='zoomedOut' ItemsSource='{Binding ElementName=hub, Path=SectionHeaders}'>"
        L"        <GridView.ItemContainerStyle><Style TargetType='GridViewItem'><Setter Property='Width' Value='120'/><Setter Property='Height' Value='120'/></Style></GridView.ItemContainerStyle>"
        L"      </GridView>"
        L"    </SemanticZoom.ZoomedOutView>"
        L"  </SemanticZoom>"
        L"</Grid>";

    // Diagnostics only (must run on the UI thread): describes the focused element by type, x:Name and the
    // nearest named ancestors, e.g. "Button 'HeaderButton' in HubSection 'item3'".
    static std::wstring DescribeFocusedElement(xaml::XamlRoot^ xamlRoot)
    {
        auto focused = xaml_input::FocusManager::GetFocusedElement(xamlRoot);
        if (!focused)
        {
            return L"<null>";
        }

        std::wstring description(focused->GetType()->FullName->Data());
        auto element = dynamic_cast<xaml::FrameworkElement^>(focused);
        if (element)
        {
            description += L" '" + std::wstring(element->Name->Data()) + L"'";
            for (auto parent = xaml_media::VisualTreeHelper::GetParent(element); parent; parent = xaml_media::VisualTreeHelper::GetParent(parent))
            {
                auto section = dynamic_cast<xaml_controls::HubSection^>(parent);
                if (section)
                {
                    description += L" in HubSection '" + std::wstring(section->Name->Data()) + L"'";
                    break;
                }
                auto namedParent = dynamic_cast<xaml::FrameworkElement^>(parent);
                if (namedParent && namedParent->Name->Length() > 0)
                {
                    description += L" in " + std::wstring(namedParent->GetType()->FullName->Data()) + L" '" + std::wstring(namedParent->Name->Data()) + L"'";
                }
            }
        }
        return description;
    }

    bool HubSectionFocusAndAutomationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool HubSectionFocusAndAutomationTests::TestSetup()
    {
        TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool HubSectionFocusAndAutomationTests::TestCleanup()
    {
        TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    xaml_controls::Hub^ HubSectionFocusAndAutomationTests::LoadHub(Platform::String^ xamlString, _Out_ xaml::FrameworkElement^* root)
    {
        xaml_controls::Hub^ hub = nullptr;
        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

        // Focus assertions below require the test window to be the foreground window (same as FocusTestHelper).
        if (!TestServices::Utilities->IsOneCore && !TestServices::WindowHelper->IsFocusedWindow)
        {
            TestServices::WindowHelper->RestoreForegroundWindow();
            TestServices::WindowHelper->WaitForIdle();
        }

        RunOnUIThread([&]()
        {
            *root = safe_cast<xaml::FrameworkElement^>(xaml_markup::XamlReader::Load(xamlString));
            VERIFY_IS_NOT_NULL(*root);
            hub = safe_cast<xaml_controls::Hub^>((*root)->FindName(L"hub"));
            VERIFY_IS_NOT_NULL(hub);
            loadedRegistration.Attach(*root, [loadedEvent]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = *root;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
        return hub;
    }

    bool HubSectionFocusAndAutomationTests::IsSectionInView(xaml_controls::Hub^ hub, xaml_controls::HubSection^ section)
    {
        for (xaml_controls::HubSection^ sectionInView : hub->SectionsInView)
        {
            if (sectionInView->Equals(section))
            {
                return true;
            }
        }
        return false;
    }

    void HubSectionFocusAndAutomationTests::DefaultSectionIndexFocusesSectionHeader()
    {
        TestCleanupWrapper cleanup;
        xaml::FrameworkElement^ root = nullptr;
        auto hub = LoadHub(Platform::StringReference(s_hubWithDefaultSection2), &root);

        RunOnUIThread([&]()
        {
            auto item1 = safe_cast<xaml_controls::HubSection^>(root->FindName(L"item1"));
            auto item3 = safe_cast<xaml_controls::HubSection^>(root->FindName(L"item3"));
            auto headerButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(item3, L"HeaderButton"));
            auto seeMoreButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(item3, L"SeeMoreButton"));
            VERIFY_IS_NOT_NULL(headerButton);
            VERIFY_IS_NOT_NULL(seeMoreButton);

            // Outside a SemanticZoom, HubSection::UpdateHeaderButtonStyle (HubSection_Partial_DesktopOnly.cpp L35-78)
            // makes the header button look like a text block (IsTabStop=False, not hit-testable), and
            // IsHeaderInteractive=True shows the section's "See more" button. HubSection::TakeFocus therefore cannot
            // focus the header button and falls back to focusing the section, i.e. its first focusable element.
            VERIFY_IS_FALSE(headerButton->IsTabStop);
            VERIFY_IS_TRUE(seeMoreButton->Visibility == xaml::Visibility::Visible);

            auto focused = xaml_input::FocusManager::GetFocusedElement(root->XamlRoot);
            LOG_OUTPUT(L"Focused element: %s (test window focused: %d)",
                DescribeFocusedElement(root->XamlRoot).c_str(), TestServices::WindowHelper->IsFocusedWindow);
            // Focus moved into the DefaultSectionIndex section (item3), not into the first section.
            VERIFY_IS_TRUE(seeMoreButton->Equals(focused));

            // The default section was scrolled into view; the first section scrolled out.
            VERIFY_IS_TRUE(IsSectionInView(hub, item3));
            VERIFY_IS_FALSE(IsSectionInView(hub, item1));
        });
    }

    void HubSectionFocusAndAutomationTests::DefaultSectionWithoutHeaderFocusesSection()
    {
        TestCleanupWrapper cleanup;
        xaml::FrameworkElement^ root = nullptr;
        auto hub = LoadHub(Platform::StringReference(s_hubWithHeaderlessDefaultSection), &root);

        RunOnUIThread([&]()
        {
            auto item2 = safe_cast<xaml_controls::HubSection^>(root->FindName(L"item2"));
            auto focused = xaml_input::FocusManager::GetFocusedElement(root->XamlRoot);
            LOG_OUTPUT(L"Focused element: %s", focused ? focused->GetType()->FullName->Data() : L"<null>");

            // No header button to focus, so the section itself takes focus.
            VERIFY_IS_TRUE(item2->Equals(focused));
            VERIFY_IS_TRUE(IsSectionInView(hub, item2));
        });
    }

    void HubSectionFocusAndAutomationTests::SemanticZoomInFocusesDestinationSection()
    {
        TestCleanupWrapper cleanup;
        xaml::FrameworkElement^ root = nullptr;
        auto hub = LoadHub(Platform::StringReference(s_hubInSemanticZoom), &root);

        xaml_controls::SemanticZoom^ semanticZoom = nullptr;
        xaml_controls::HubSection^ item3 = nullptr;
        auto viewChangeCompletedEvent = std::make_shared<Event>();
        auto viewChangeCompletedRegistration = CreateSafeEventRegistration(xaml_controls::SemanticZoom, ViewChangeCompleted);

        RunOnUIThread([&]()
        {
            semanticZoom = safe_cast<xaml_controls::SemanticZoom^>(root->FindName(L"sezo"));
            item3 = safe_cast<xaml_controls::HubSection^>(root->FindName(L"item3"));
            VERIFY_IS_NOT_NULL(semanticZoom);
            VERIFY_IS_NOT_NULL(item3);
            VERIFY_IS_TRUE(semanticZoom->IsZoomedInViewActive);
            VERIFY_IS_FALSE(IsSectionInView(hub, item3));

            viewChangeCompletedRegistration.Attach(semanticZoom, ref new xaml_controls::SemanticZoomViewChangedEventHandler(
                [viewChangeCompletedEvent](Platform::Object^, xaml_controls::SemanticZoomViewChangedEventArgs^)
                {
                    viewChangeCompletedEvent->Set();
                }));
        });

        // The view changes are requested programmatically: injected taps on the header button do not zoom out on
        // current OS builds (HubIntegrationTests::ValidateSemanticZoomedOutView is limited to 22H2 for that reason).
        LOG_OUTPUT(L"Zooming out with SemanticZoom.ToggleActiveView.");
        RunOnUIThread([&]()
        {
            semanticZoom->ToggleActiveView();
        });
        const bool zoomedOutCompleted = viewChangeCompletedEvent->WaitForNoThrow();
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"After zoom-out: ViewChangeCompleted=%d, IsZoomedInViewActive=%d, focused element: %s (test window focused: %d)",
                zoomedOutCompleted, semanticZoom->IsZoomedInViewActive, DescribeFocusedElement(root->XamlRoot).c_str(),
                TestServices::WindowHelper->IsFocusedWindow);
        });
        VERIFY_IS_TRUE(zoomedOutCompleted);
        viewChangeCompletedEvent->Reset();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(semanticZoom->IsZoomedInViewActive);
            auto zoomedOut = safe_cast<xaml_controls::GridView^>(root->FindName(L"zoomedOut"));
            auto item3Tile = safe_cast<xaml_controls::Control^>(zoomedOut->ContainerFromIndex(2));
            VERIFY_IS_NOT_NULL(item3Tile);
            // A programmatic zoom-in starts from the zoomed-out view's focused item (ListViewBase::StartViewChangeFromImpl),
            // so focusing the third tile selects the third section as the destination.
            VERIFY_IS_TRUE(item3Tile->Focus(xaml::FocusState::Keyboard));
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Zooming in with SemanticZoom.ToggleActiveView from the focused third tile.");
        RunOnUIThread([&]()
        {
            semanticZoom->ToggleActiveView();
        });
        const bool zoomedInCompleted = viewChangeCompletedEvent->WaitForNoThrow();
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"After zoom-in: ViewChangeCompleted=%d, IsZoomedInViewActive=%d, focused element: %s",
                zoomedInCompleted, semanticZoom->IsZoomedInViewActive, DescribeFocusedElement(root->XamlRoot).c_str());
        });
        VERIFY_IS_TRUE(zoomedInCompleted);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(semanticZoom->IsZoomedInViewActive);
            auto item3HeaderButton = TreeHelper::GetVisualChildByName(item3, L"HeaderButton");
            VERIFY_IS_NOT_NULL(item3HeaderButton);

            auto focused = xaml_input::FocusManager::GetFocusedElement(root->XamlRoot);
            LOG_OUTPUT(L"Focused element: %s", DescribeFocusedElement(root->XamlRoot).c_str());
            // Focus is transferred to the destination section, not left in the zoomed-out view.
            VERIFY_IS_TRUE(item3HeaderButton->Equals(focused));
            VERIFY_IS_TRUE(IsSectionInView(hub, item3));
        });
    }

    void HubSectionFocusAndAutomationTests::HubSectionAutomationPeerScrollItemPattern()
    {
        TestCleanupWrapper cleanup;
        xaml::FrameworkElement^ root = nullptr;
        auto hub = LoadHub(Platform::StringReference(s_hubWithoutDefaultSection), &root);
        xaml_controls::HubSection^ item1 = nullptr;
        xaml_controls::HubSection^ item3 = nullptr;
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;

        RunOnUIThread([&]()
        {
            item1 = safe_cast<xaml_controls::HubSection^>(root->FindName(L"item1"));
            item3 = safe_cast<xaml_controls::HubSection^>(root->FindName(L"item3"));
            scrollViewer = TreeHelper::GetVisualChildByType<xaml_controls::ScrollViewer>(hub);
            VERIFY_IS_NOT_NULL(scrollViewer);
            // Precondition: the last section starts out of view.
            VERIFY_IS_FALSE(IsSectionInView(hub, item3));

            auto peer = ref new xaml_automation_peers::HubSectionAutomationPeer(item3);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"HubSection"), peer->GetClassName());

            auto scrollItemPattern = peer->GetPattern(xaml_automation_peers::PatternInterface::ScrollItem);
            VERIFY_IS_NOT_NULL(scrollItemPattern);
            // GetPatternCore returns the peer itself (through its IInspectable interface); compare identities through the same interface.
            VERIFY_IS_TRUE(Platform::Object::ReferenceEquals(peer, safe_cast<xaml_automation_peers::HubSectionAutomationPeer^>(scrollItemPattern)));
            VERIFY_IS_NULL(peer->GetPattern(xaml_automation_peers::PatternInterface::Invoke));

            LOG_OUTPUT(L"Invoking IScrollItemProvider::ScrollIntoView on the last section.");
            safe_cast<Microsoft::UI::Xaml::Automation::Provider::IScrollItemProvider^>(scrollItemPattern)->ScrollIntoView();
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Hub HorizontalOffset after ScrollIntoView(item3): %f", scrollViewer->HorizontalOffset);
            VERIFY_IS_TRUE(IsSectionInView(hub, item3));
            VERIFY_IS_TRUE(scrollViewer->HorizontalOffset > 0.0);

            LOG_OUTPUT(L"Invoking ScrollIntoView on the first section through a peer created for the element.");
            auto peer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(item1);
            VERIFY_IS_NOT_NULL(dynamic_cast<xaml_automation_peers::HubSectionAutomationPeer^>(peer));
            safe_cast<Microsoft::UI::Xaml::Automation::Provider::IScrollItemProvider^>(
                peer->GetPattern(xaml_automation_peers::PatternInterface::ScrollItem))->ScrollIntoView();
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Hub HorizontalOffset after ScrollIntoView(item1): %f", scrollViewer->HorizontalOffset);
            VERIFY_IS_TRUE(IsSectionInView(hub, item1));
            VERIFY_IS_FALSE(IsSectionInView(hub, item3));
            VERIFY_ARE_EQUAL(0.0, scrollViewer->HorizontalOffset);
        });
    }

    void HubSectionFocusAndAutomationTests::HubSectionDependencyPropertyIdentifiersMatchProperties()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto section = ref new xaml_controls::HubSection();
            auto headerTemplate = safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                L"<DataTemplate " XAML_NAMESPACES L"><TextBlock Text='{Binding}'/></DataTemplate>"));
            auto contentTemplate = safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                L"<DataTemplate " XAML_NAMESPACES L"><Border/></DataTemplate>"));

            VERIFY_IS_NOT_NULL(xaml_controls::HubSection::HeaderProperty);
            VERIFY_IS_NOT_NULL(xaml_controls::HubSection::HeaderTemplateProperty);
            VERIFY_IS_NOT_NULL(xaml_controls::HubSection::ContentTemplateProperty);
            VERIFY_IS_NOT_NULL(xaml_controls::HubSection::IsHeaderInteractiveProperty);

            section->SetValue(xaml_controls::HubSection::HeaderProperty, ref new Platform::String(L"header via DP"));
            VERIFY_ARE_EQUAL(Platform::StringReference(L"header via DP"), safe_cast<Platform::String^>(section->Header));

            section->SetValue(xaml_controls::HubSection::HeaderTemplateProperty, headerTemplate);
            VERIFY_IS_TRUE(headerTemplate->Equals(section->HeaderTemplate));

            section->SetValue(xaml_controls::HubSection::ContentTemplateProperty, contentTemplate);
            VERIFY_IS_TRUE(contentTemplate->Equals(section->ContentTemplate));

            VERIFY_IS_FALSE(safe_cast<bool>(section->GetValue(xaml_controls::HubSection::IsHeaderInteractiveProperty)));
            section->IsHeaderInteractive = true;
            VERIFY_IS_TRUE(safe_cast<bool>(section->GetValue(xaml_controls::HubSection::IsHeaderInteractiveProperty)));
        });
    }

} } } } } }
