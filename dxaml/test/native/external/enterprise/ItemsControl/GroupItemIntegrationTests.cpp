// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "GroupItemIntegrationTests.h"
#include <SafeEventRegistration.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TreeHelper.h>
#include <TestCleanupWrapper.h>
#include <FocusTestHelper.h>

#undef GetClassName // Conflicts with automation peer method of the same name

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

#define XAML_NAMESPACES L"xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace ItemsControl {

    // Records every SelectStyle call made for a group container and returns a style that tags the GroupItem.
    ref class RecordingGroupStyleSelector : public xaml_controls::StyleSelector
    {
    internal:
        RecordingGroupStyleSelector()
        {
            m_style = ref new xaml::Style();
            m_style->TargetType = xaml_controls::GroupItem::typeid;
            m_style->Setters->Append(ref new xaml::Setter(xaml::FrameworkElement::TagProperty, ref new Platform::String(L"fromSelector")));
            m_selectedGroups = ref new Platform::Collections::Vector<Platform::Object^>();
            m_nonGroupItemContainers = 0;
        }

        xaml::Style^ m_style;
        Platform::Collections::Vector<Platform::Object^>^ m_selectedGroups;
        unsigned int m_nonGroupItemContainers;

    public:
        xaml::Style^ SelectStyleCore(Platform::Object^ item, xaml::DependencyObject^ container) override
        {
            // The legacy generator passes the CollectionViewGroup as the item and the GroupItem as the container.
            auto group = dynamic_cast<xaml_data::ICollectionViewGroup^>(item);
            if (group != nullptr)
            {
                m_selectedGroups->Append(group->Group);
            }
            if (dynamic_cast<xaml_controls::GroupItem^>(container) == nullptr)
            {
                ++m_nonGroupItemContainers;
            }
            return m_style;
        }
    };

    //
    // Class & Test Setup
    //
    bool GroupItemIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool GroupItemIntegrationTests::TestCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Helpers
    //
    Platform::Collections::Vector<Platform::Object^>^ GroupItemIntegrationTests::MakeGroup(std::initializer_list<const wchar_t*> items)
    {
        auto group = ref new Platform::Collections::Vector<Platform::Object^>();
        for (auto item : items)
        {
            group->Append(ref new Platform::String(item));
        }
        return group;
    }

    xaml_controls::GroupItem^ GroupItemIntegrationTests::FindGroupItem(xaml::FrameworkElement^ itemsControl, Platform::Object^ group)
    {
        auto groupItems = ref new Platform::Collections::Vector<xaml_controls::GroupItem^>();
        TreeHelper::GetVisualChildrenByType<xaml_controls::GroupItem>(itemsControl, groupItems);
        for (xaml_controls::GroupItem^ groupItem : groupItems)
        {
            if (Platform::Object::ReferenceEquals(groupItem->Content, group))
            {
                return groupItem;
            }
        }
        return nullptr;
    }

    unsigned int GroupItemIntegrationTests::CountGroupItems(xaml::FrameworkElement^ itemsControl)
    {
        auto groupItems = ref new Platform::Collections::Vector<xaml_controls::GroupItem^>();
        TreeHelper::GetVisualChildrenByType<xaml_controls::GroupItem>(itemsControl, groupItems);
        return groupItems->Size;
    }

    // Loads the markup, binds its CollectionViewSource named 'cvs' to the grouped source and waits for layout.
    xaml::FrameworkElement^ GroupItemIntegrationTests::LoadGroupedContent(
        Platform::String^ xamlString,
        Platform::Collections::Vector<Platform::Object^>^ groups)
    {
        xaml::FrameworkElement^ root = nullptr;
        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

        RunOnUIThread([&]()
        {
            root = safe_cast<xaml::FrameworkElement^>(xaml_markup::XamlReader::Load(xamlString));
            VERIFY_IS_NOT_NULL(root);
            loadedRegistration.Attach(root, [loadedEvent]() { loadedEvent->Set(); });

            auto cvs = safe_cast<xaml_data::CollectionViewSource^>(root->FindName(L"cvs"));
            VERIFY_IS_NOT_NULL(cvs);
            cvs->Source = groups;
            cvs->IsSourceGrouped = true;

            TestServices::WindowHelper->WindowContent = root;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
        return root;
    }

    // Identity lookup (Object::ReferenceEquals compares COM identity, unlike handle equality).
    static bool TryGetGroupIndex(Platform::Collections::Vector<Platform::Object^>^ groups, Platform::Object^ group, _Out_ unsigned int* index)
    {
        for (unsigned int i = 0; i < groups->Size; i++)
        {
            if (Platform::Object::ReferenceEquals(groups->GetAt(i), group))
            {
                *index = i;
                return true;
            }
        }
        *index = 0;
        return false;
    }

    static bool ContainsTextBlockWithText(xaml::FrameworkElement^ parent, Platform::String^ text)
    {
        auto textBlocks = ref new Platform::Collections::Vector<xaml_controls::TextBlock^>();
        TreeHelper::GetVisualChildrenByType<xaml_controls::TextBlock>(parent, textBlocks);
        for (xaml_controls::TextBlock^ textBlock : textBlocks)
        {
            if (textBlock->Text == text)
            {
                return true;
            }
        }
        return false;
    }

    //
    // Test Cases
    //
    void GroupItemIntegrationTests::HidesIfEmptyCollapsesAndRestoresGroupItem()
    {
        TestCleanupWrapper cleanup;

        auto filled = MakeGroup({ L"a1", L"a2" });
        auto empty = MakeGroup({});
        auto last = MakeGroup({ L"c1" });
        auto groups = ref new Platform::Collections::Vector<Platform::Object^>();
        groups->Append(filled);
        groups->Append(empty);
        groups->Append(last);

        // Both ItemsControls use the default (legacy) StackPanel items panel and share the grouped source.
        auto root = LoadGroupedContent(
            L"<StackPanel " XAML_NAMESPACES L">"
            L"  <StackPanel.Resources><CollectionViewSource x:Name='cvs'/></StackPanel.Resources>"
            L"  <ItemsControl x:Name='hiding' ItemsSource='{Binding Source={StaticResource cvs}}'>"
            L"    <ItemsControl.GroupStyle><GroupStyle HidesIfEmpty='True'/></ItemsControl.GroupStyle>"
            L"  </ItemsControl>"
            L"  <ItemsControl x:Name='showing' ItemsSource='{Binding Source={StaticResource cvs}}'>"
            L"    <ItemsControl.GroupStyle><GroupStyle HidesIfEmpty='False'/></ItemsControl.GroupStyle>"
            L"  </ItemsControl>"
            L"</StackPanel>",
            groups);

        xaml::FrameworkElement^ hiding = nullptr;
        xaml::FrameworkElement^ showing = nullptr;

        RunOnUIThread([&]()
        {
            hiding = safe_cast<xaml::FrameworkElement^>(root->FindName(L"hiding"));
            showing = safe_cast<xaml::FrameworkElement^>(root->FindName(L"showing"));

            VERIFY_ARE_EQUAL(3u, CountGroupItems(hiding));
            VERIFY_ARE_EQUAL(3u, CountGroupItems(showing));

            auto hiddenGroupItem = FindGroupItem(hiding, empty);
            VERIFY_IS_NOT_NULL(hiddenGroupItem);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, hiddenGroupItem->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, FindGroupItem(hiding, filled)->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, FindGroupItem(hiding, last)->Visibility);

            // Contrast: without HidesIfEmpty the empty group's GroupItem stays visible.
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, FindGroupItem(showing, empty)->Visibility);

            LOG_OUTPUT(L"Adding an item to the empty group.");
            empty->Append(ref new Platform::String(L"b1"));
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto restoredGroupItem = FindGroupItem(hiding, empty);
            VERIFY_IS_NOT_NULL(restoredGroupItem);
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, restoredGroupItem->Visibility);
            VERIFY_IS_TRUE(ContainsTextBlockWithText(restoredGroupItem, L"b1"));
            VERIFY_ARE_EQUAL(3u, CountGroupItems(hiding));
        });
    }

    void GroupItemIntegrationTests::RemovingGroupClearsGroupItemContainer()
    {
        TestCleanupWrapper cleanup;

        auto first = MakeGroup({ L"a1" });
        auto removed = MakeGroup({ L"b1", L"b2" });
        auto third = MakeGroup({ L"c1" });
        auto groups = ref new Platform::Collections::Vector<Platform::Object^>();
        groups->Append(first);
        groups->Append(removed);
        groups->Append(third);

        auto root = LoadGroupedContent(
            L"<Grid " XAML_NAMESPACES L">"
            L"  <Grid.Resources><CollectionViewSource x:Name='cvs'/></Grid.Resources>"
            L"  <ItemsControl x:Name='itemsControl' ItemsSource='{Binding Source={StaticResource cvs}}'>"
            L"    <ItemsControl.GroupStyle>"
            L"      <GroupStyle>"
            L"        <GroupStyle.HeaderTemplate><DataTemplate><TextBlock Text='group header'/></DataTemplate></GroupStyle.HeaderTemplate>"
            L"      </GroupStyle>"
            L"    </ItemsControl.GroupStyle>"
            L"  </ItemsControl>"
            L"</Grid>",
            groups);

        xaml_controls::ItemsControl^ itemsControl = nullptr;
        xaml_controls::GroupItem^ removedGroupItem = nullptr;

        RunOnUIThread([&]()
        {
            itemsControl = safe_cast<xaml_controls::ItemsControl^>(root->FindName(L"itemsControl"));
            auto headerTemplate = itemsControl->GroupStyle->GetAt(0)->HeaderTemplate;
            VERIFY_IS_NOT_NULL(headerTemplate);

            removedGroupItem = FindGroupItem(itemsControl, removed);
            VERIFY_IS_NOT_NULL(removedGroupItem);
            // Preconditions: the generator forwarded the group data and header template to the GroupItem.
            VERIFY_IS_TRUE(Platform::Object::ReferenceEquals(removed, removedGroupItem->Content));
            VERIFY_IS_TRUE(headerTemplate->Equals(removedGroupItem->ContentTemplate));
            VERIFY_ARE_EQUAL(3u, CountGroupItems(itemsControl));

            LOG_OUTPUT(L"Removing the middle group.");
            groups->RemoveAt(1);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // The unlinked container no longer holds the group data or the header template.
            VERIFY_IS_NULL(removedGroupItem->Content);
            VERIFY_IS_TRUE(xaml::DependencyProperty::UnsetValue->Equals(
                removedGroupItem->ReadLocalValue(xaml_controls::ContentControl::ContentProperty)));
            VERIFY_IS_NULL(removedGroupItem->ContentTemplate);
            VERIFY_IS_TRUE(xaml::DependencyProperty::UnsetValue->Equals(
                removedGroupItem->ReadLocalValue(xaml_controls::ContentControl::ContentTemplateProperty)));

            // The other groups are unaffected.
            VERIFY_ARE_EQUAL(2u, CountGroupItems(itemsControl));
            VERIFY_IS_NOT_NULL(FindGroupItem(itemsControl, first));
            VERIFY_IS_NOT_NULL(FindGroupItem(itemsControl, third));
            VERIFY_IS_NULL(FindGroupItem(itemsControl, removed));
        });
    }

    void GroupItemIntegrationTests::ContainerStyleAndSelectorAppliedToGroupItems()
    {
        TestCleanupWrapper cleanup;

        auto groupA = MakeGroup({ L"a1" });
        auto groupB = MakeGroup({ L"b1" });
        auto groupC = MakeGroup({ L"c1" });
        auto groups = ref new Platform::Collections::Vector<Platform::Object^>();
        groups->Append(groupA);
        groups->Append(groupB);
        groups->Append(groupC);

        RecordingGroupStyleSelector^ selector = nullptr;
        RunOnUIThread([&]() { selector = ref new RecordingGroupStyleSelector(); });

        // 'selected' gets its ContainerStyleSelector before the source is bound, so it is used during preparation.
        xaml::FrameworkElement^ root = nullptr;
        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);
        RunOnUIThread([&]()
        {
            root = safe_cast<xaml::FrameworkElement^>(xaml_markup::XamlReader::Load(
                L"<StackPanel " XAML_NAMESPACES L">"
                L"  <StackPanel.Resources><CollectionViewSource x:Name='cvs'/></StackPanel.Resources>"
                L"  <ItemsControl x:Name='styled' ItemsSource='{Binding Source={StaticResource cvs}}'>"
                L"    <ItemsControl.GroupStyle>"
                L"      <GroupStyle>"
                L"        <GroupStyle.ContainerStyle>"
                L"          <Style TargetType='GroupItem'><Setter Property='Tag' Value='fromContainerStyle'/></Style>"
                L"        </GroupStyle.ContainerStyle>"
                L"      </GroupStyle>"
                L"    </ItemsControl.GroupStyle>"
                L"  </ItemsControl>"
                L"  <ItemsControl x:Name='selected'>"
                L"    <ItemsControl.GroupStyle><GroupStyle/></ItemsControl.GroupStyle>"
                L"  </ItemsControl>"
                L"</StackPanel>"));
            VERIFY_IS_NOT_NULL(root);
            loadedRegistration.Attach(root, [loadedEvent]() { loadedEvent->Set(); });

            auto selected = safe_cast<xaml_controls::ItemsControl^>(root->FindName(L"selected"));
            selected->GroupStyle->GetAt(0)->ContainerStyleSelector = selector;

            auto cvs = safe_cast<xaml_data::CollectionViewSource^>(root->FindName(L"cvs"));
            cvs->Source = groups;
            cvs->IsSourceGrouped = true;
            selected->ItemsSource = cvs->View;

            TestServices::WindowHelper->WindowContent = root;
        });
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto styled = safe_cast<xaml_controls::ItemsControl^>(root->FindName(L"styled"));
            auto selected = safe_cast<xaml_controls::ItemsControl^>(root->FindName(L"selected"));
            auto containerStyle = styled->GroupStyle->GetAt(0)->ContainerStyle;
            VERIFY_IS_NOT_NULL(containerStyle);

            for (auto group : { groupA, groupB, groupC })
            {
                auto styledGroupItem = FindGroupItem(styled, group);
                VERIFY_IS_NOT_NULL(styledGroupItem);
                VERIFY_IS_TRUE(containerStyle->Equals(styledGroupItem->Style));
                VERIFY_ARE_EQUAL(Platform::StringReference(L"fromContainerStyle"), safe_cast<Platform::String^>(styledGroupItem->Tag));

                auto selectedGroupItem = FindGroupItem(selected, group);
                VERIFY_IS_NOT_NULL(selectedGroupItem);
                VERIFY_IS_TRUE(selector->m_style->Equals(selectedGroupItem->Style));
                VERIFY_ARE_EQUAL(Platform::StringReference(L"fromSelector"), safe_cast<Platform::String^>(selectedGroupItem->Tag));

                unsigned int index = 0;
                VERIFY_IS_TRUE(TryGetGroupIndex(selector->m_selectedGroups, group, &index));
            }

            LOG_OUTPUT(L"Selector called %u times.", selector->m_selectedGroups->Size);
            VERIFY_IS_TRUE(selector->m_selectedGroups->Size >= 3u);
            VERIFY_ARE_EQUAL(0u, selector->m_nonGroupItemContainers);
        });
    }

    void GroupItemIntegrationTests::UserDeclaredGroupItemIsNotGeneratorPrepared()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ItemsControl^ itemsControl = nullptr;
        xaml_controls::GroupItem^ declared = nullptr;
        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ItemsControl, Loaded);

        RunOnUIThread([&]()
        {
            itemsControl = ref new xaml_controls::ItemsControl();
            loadedRegistration.Attach(itemsControl, [loadedEvent]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = itemsControl;
        });
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            declared = ref new xaml_controls::GroupItem();
            declared->Content = ref new Platform::String(L"declared");
            itemsControl->Items->Append(declared);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // A GroupItem is a UIElement, so it is its own container and keeps the content set by the app.
            VERIFY_IS_TRUE(declared->Equals(itemsControl->ContainerFromIndex(0)));
            VERIFY_IS_NOT_NULL(xaml_media::VisualTreeHelper::GetParent(declared));
            VERIFY_ARE_EQUAL(Platform::StringReference(L"declared"), safe_cast<Platform::String^>(declared->Content));

            itemsControl->Items->RemoveAt(0);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_NULL(xaml_media::VisualTreeHelper::GetParent(declared));
            VERIFY_ARE_EQUAL(Platform::StringReference(L"declared"), safe_cast<Platform::String^>(declared->Content));
            VERIFY_ARE_EQUAL(0u, itemsControl->Items->Size);
        });
    }

    void GroupItemIntegrationTests::GroupItemCreatesGroupItemAutomationPeer()
    {
        TestCleanupWrapper cleanup;

        auto first = MakeGroup({ L"a1", L"a2" });
        auto second = MakeGroup({ L"b1" });
        auto groups = ref new Platform::Collections::Vector<Platform::Object^>();
        groups->Append(first);
        groups->Append(second);

        auto root = LoadGroupedContent(
            L"<Grid " XAML_NAMESPACES L">"
            L"  <Grid.Resources><CollectionViewSource x:Name='cvs'/></Grid.Resources>"
            L"  <ListView x:Name='listView' ItemsSource='{Binding Source={StaticResource cvs}}'>"
            L"    <ListView.ItemsPanel><ItemsPanelTemplate><VirtualizingStackPanel/></ItemsPanelTemplate></ListView.ItemsPanel>"
            L"    <ListView.GroupStyle>"
            L"      <GroupStyle><GroupStyle.HeaderTemplate><DataTemplate><TextBlock Text='header'/></DataTemplate></GroupStyle.HeaderTemplate></GroupStyle>"
            L"    </ListView.GroupStyle>"
            L"  </ListView>"
            L"</Grid>",
            groups);

        RunOnUIThread([&]()
        {
            auto listView = safe_cast<xaml_controls::ListView^>(root->FindName(L"listView"));
            auto listViewPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(listView);
            VERIFY_IS_NOT_NULL(listViewPeer);
            VERIFY_IS_NOT_NULL(listViewPeer->GetChildren());

            auto groupItem = FindGroupItem(listView, first);
            VERIFY_IS_NOT_NULL(groupItem);

            auto peer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(groupItem);
            VERIFY_IS_NOT_NULL(dynamic_cast<xaml_automation_peers::GroupItemAutomationPeer^>(peer));
            VERIFY_ARE_EQUAL(Platform::StringReference(L"GroupItem"), peer->GetClassName());
            VERIFY_ARE_EQUAL(xaml_automation_peers::AutomationControlType::Group, peer->GetAutomationControlType());

            // The item peers of the group appear in item order (after any header content peers).
            auto itemNames = ref new Platform::Collections::Vector<Platform::String^>();
            for (xaml_automation_peers::AutomationPeer^ child : peer->GetChildren())
            {
                auto itemPeer = dynamic_cast<xaml_automation_peers::ItemAutomationPeer^>(child);
                if (itemPeer != nullptr)
                {
                    itemNames->Append(safe_cast<Platform::String^>(itemPeer->Item));
                }
            }
            LOG_OUTPUT(L"GroupItem peer has %u item peers.", itemNames->Size);
            VERIFY_ARE_EQUAL(2u, itemNames->Size);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"a1"), itemNames->GetAt(0));
            VERIFY_ARE_EQUAL(Platform::StringReference(L"a2"), itemNames->GetAt(1));
        });
    }

    void GroupItemIntegrationTests::GroupHeaderKeyDownIsNotConsumedOutsideSemanticZoom()
    {
        TestCleanupWrapper cleanup;

        auto first = MakeGroup({ L"a1" });
        auto groups = ref new Platform::Collections::Vector<Platform::Object^>();
        groups->Append(first);

        auto root = LoadGroupedContent(
            L"<Grid " XAML_NAMESPACES L">"
            L"  <Grid.Resources><CollectionViewSource x:Name='cvs'/></Grid.Resources>"
            L"  <ListView x:Name='listView' ItemsSource='{Binding Source={StaticResource cvs}}'>"
            L"    <ListView.ItemsPanel><ItemsPanelTemplate><VirtualizingStackPanel/></ItemsPanelTemplate></ListView.ItemsPanel>"
            L"    <ListView.GroupStyle>"
            L"      <GroupStyle><GroupStyle.HeaderTemplate><DataTemplate>"
            L"        <ContentControl x:Name='focusableHeader' IsTabStop='True' Content='header'/>"
            L"      </DataTemplate></GroupStyle.HeaderTemplate></GroupStyle>"
            L"    </ListView.GroupStyle>"
            L"  </ListView>"
            L"</Grid>",
            groups);

        xaml_controls::GroupItem^ groupItem = nullptr;
        xaml_controls::Control^ focusableHeader = nullptr;
        auto keyDownEvent = std::make_shared<Event>();
        auto observedKey = std::make_shared<::Windows::System::VirtualKey>(::Windows::System::VirtualKey::None);
        auto observedHandled = std::make_shared<bool>(true);
        xaml_input::KeyEventHandler^ keyDownHandler = nullptr;

        RunOnUIThread([&]()
        {
            auto listView = safe_cast<xaml::FrameworkElement^>(root->FindName(L"listView"));
            groupItem = FindGroupItem(listView, first);
            VERIFY_IS_NOT_NULL(groupItem);
            focusableHeader = safe_cast<xaml_controls::Control^>(TreeHelper::GetVisualChildByName(groupItem, L"focusableHeader"));
            VERIFY_IS_NOT_NULL(focusableHeader);

            // Observe the key after it bubbled through the GroupItem's HeaderContent part (handledEventsToo).
            keyDownHandler = ref new xaml_input::KeyEventHandler(
                [keyDownEvent, observedKey, observedHandled](Platform::Object^, xaml_input::KeyRoutedEventArgs^ args)
                {
                    *observedKey = args->Key;
                    *observedHandled = args->Handled;
                    keyDownEvent->Set();
                });
            groupItem->AddHandler(xaml::UIElement::KeyDownEvent, keyDownHandler, true /*handledEventsToo*/);
        });

        FocusTestHelper::EnsureFocus(focusableHeader, xaml::FocusState::Pointer);

        LOG_OUTPUT(L"Pressing Enter in the group header.");
        TestServices::KeyboardHelper->Enter();
        keyDownEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(::Windows::System::VirtualKey::Enter, *observedKey);
            // Without a SemanticZoom owner there is no view to toggle, so the header must not swallow the key.
            VERIFY_IS_FALSE(*observedHandled);
            groupItem->RemoveHandler(xaml::UIElement::KeyDownEvent, keyDownHandler);
        });
    }

    void GroupItemIntegrationTests::RecycledGroupItemsRestoreState()
    {
        TestCleanupWrapper cleanup;

        const unsigned int groupCount = 40;
        const unsigned int emptyGroupIndex = 3;
        auto groups = ref new Platform::Collections::Vector<Platform::Object^>();
        std::vector<Platform::Collections::Vector<Platform::Object^>^> typedGroups;
        for (unsigned int groupIndex = 0; groupIndex < groupCount; groupIndex++)
        {
            auto group = ref new Platform::Collections::Vector<Platform::Object^>();
            if (groupIndex != emptyGroupIndex)
            {
                for (unsigned int itemIndex = 0; itemIndex < 2; itemIndex++)
                {
                    wchar_t text[32] = {};
                    swprintf_s(text, L"g%02u-i%u", groupIndex, itemIndex);
                    group->Append(ref new Platform::String(text));
                }
            }
            groups->Append(group);
            typedGroups.push_back(group);
        }

        auto root = LoadGroupedContent(
            L"<Grid " XAML_NAMESPACES L">"
            L"  <Grid.Resources><CollectionViewSource x:Name='cvs'/></Grid.Resources>"
            L"  <ListView x:Name='listView' Height='200' Width='200' HorizontalAlignment='Left' VerticalAlignment='Top'"
            L"      VirtualizingStackPanel.VirtualizationMode='Recycling' ItemsSource='{Binding Source={StaticResource cvs}}'>"
            L"    <ListView.ItemsPanel><ItemsPanelTemplate><VirtualizingStackPanel/></ItemsPanelTemplate></ListView.ItemsPanel>"
            L"    <ListView.GroupStyle>"
            L"      <GroupStyle HidesIfEmpty='True'><GroupStyle.HeaderTemplate><DataTemplate><TextBlock Text='header'/></DataTemplate></GroupStyle.HeaderTemplate></GroupStyle>"
            L"    </ListView.GroupStyle>"
            L"  </ListView>"
            L"</Grid>",
            groups);

        xaml_controls::ListView^ listView = nullptr;
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;

        // Every realized GroupItem must show its own group's items and the HidesIfEmpty visibility of that group.
        auto verifyRealizedGroupItems = [&](unsigned int* realizedCount)
        {
            auto groupItems = ref new Platform::Collections::Vector<xaml_controls::GroupItem^>();
            TreeHelper::GetVisualChildrenByType<xaml_controls::GroupItem>(listView, groupItems);
            *realizedCount = 0;
            for (xaml_controls::GroupItem^ groupItem : groupItems)
            {
                unsigned int groupIndex = 0;
                if (groupItem->Content == nullptr || !TryGetGroupIndex(groups, groupItem->Content, &groupIndex))
                {
                    continue; // A recycled container waiting in the recycle queue is not linked to a group.
                }
                ++(*realizedCount);

                auto group = typedGroups[groupIndex];
                const auto expectedVisibility = group->Size == 0 ? xaml::Visibility::Collapsed : xaml::Visibility::Visible;
                LOG_OUTPUT(L"Group %u: visibility %d, expected %d.", groupIndex, static_cast<int>(groupItem->Visibility), static_cast<int>(expectedVisibility));
                VERIFY_ARE_EQUAL(expectedVisibility, groupItem->Visibility);

                if (group->Size > 0)
                {
                    VERIFY_IS_TRUE(ContainsTextBlockWithText(groupItem, safe_cast<Platform::String^>(group->GetAt(0))));
                }
            }
        };

        RunOnUIThread([&]()
        {
            listView = safe_cast<xaml_controls::ListView^>(root->FindName(L"listView"));
            scrollViewer = TreeHelper::GetVisualChildByType<xaml_controls::ScrollViewer>(listView);
            VERIFY_IS_NOT_NULL(scrollViewer);

            unsigned int realized = 0;
            verifyRealizedGroupItems(&realized);
            LOG_OUTPUT(L"Initially realized groups: %u", realized);
            // Precondition: the panel virtualizes, so only a subset of the groups is realized.
            VERIFY_IS_TRUE(realized > 0 && realized < groupCount);

            scrollViewer->ChangeView(nullptr, scrollViewer->ScrollableHeight, nullptr, true /*disableAnimation*/);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            unsigned int realized = 0;
            verifyRealizedGroupItems(&realized);
            LOG_OUTPUT(L"Realized groups at the end: %u", realized);
            VERIFY_IS_TRUE(realized > 0);
            VERIFY_IS_NULL(FindGroupItem(listView, groups->GetAt(0)));

            scrollViewer->ChangeView(nullptr, 0.0, nullptr, true /*disableAnimation*/);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // The first groups (including the hidden empty one) are realized again from recycled containers.
            unsigned int realized = 0;
            verifyRealizedGroupItems(&realized);
            LOG_OUTPUT(L"Realized groups after scrolling back: %u", realized);
            VERIFY_IS_NOT_NULL(FindGroupItem(listView, groups->GetAt(0)));
            auto emptyGroupItem = FindGroupItem(listView, groups->GetAt(emptyGroupIndex));
            VERIFY_IS_NOT_NULL(emptyGroupItem);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, emptyGroupItem->Visibility);
        });
    }

} } } } } }
