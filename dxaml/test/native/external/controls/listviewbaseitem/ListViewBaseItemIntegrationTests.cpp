// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
 //  Copyright (c) Microsoft Corporation.  All rights reserved.

#include "pch.h"
#include "ListViewBaseItemIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <RuntimeEnabledFeaturesEnum.h>
#include <ControlHelper.h>
#include <WUCRenderingScopeGuard.h>

#include <StoryboardMonitorWrapper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ListViewBaseItem {

    class RoundedChromeDictionaryHelper
    {
    private:
        xaml::ResourceDictionary^ m_customDictionary = nullptr;

    public:
        RoundedChromeDictionaryHelper(bool useRoundedCorners) {

            // If we aren't using rounded chrome, we don't have to do anything.  This will just be a no-op.
            if (!useRoundedCorners) return;

            // Set up a dictionary and add it to the application.
            RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Creating custom resource dictionary for rounded chrome.");
                    m_customDictionary = safe_cast<xaml::ResourceDictionary^>(xaml_markup::XamlReader::Load(
                        LR"(
                        <ResourceDictionary
                            xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <x:Boolean x:Key="ListViewBaseItemRoundedChromeEnabled">True</x:Boolean>
                        </ResourceDictionary>)"));

                    Application::Current->Resources->MergedDictionaries->Append(m_customDictionary);
                });
        }

        ~RoundedChromeDictionaryHelper()
        {
            // If we didn't actually set the dictionary, no-op
            if (!m_customDictionary) return;

            RunOnUIThread([&]()
                {
                    auto mergedDictionaries = Application::Current->Resources->MergedDictionaries;

                    // Find the custom dictionary
                    uint32 index;
                    VERIFY_IS_TRUE(mergedDictionaries->IndexOf(m_customDictionary, &index));

                    // Then remove it and make sure we tell Xaml to reset any cached dictionaries they might have.
                    LOG_OUTPUT(L"Removing dictionary with ListViewBaseItemRoundedChromeEnabled=True.");
                    mergedDictionaries->RemoveAt(index);

                    // ListViewBaseItem will cache the value, so make sure to clear that cache
                    LOG_OUTPUT(L"Invoking Utilities.DeleteResourceDictionaryCaches.");
                    TestServices::Utilities->DeleteResourceDictionaryCaches();
                });
        }
    };

    template<typename T>
    static void GoToVisualStates(xaml_controls::Grid^ rootPanel)
    {
        T^ normal = nullptr;
        T^ pointerOver = nullptr;
        T^ disabled = nullptr;
        T^ normalSelected = nullptr;
        T^ pointerOverSelected = nullptr;
        T^ disabledSelected = nullptr;
        T^ multiSelect = nullptr;
        T^ multiSelectSelected = nullptr;
        T^ multiSelectDisabled = nullptr;

        T^ multiSelectSelectedDisabled = nullptr;

        RunOnUIThread([&]()
        {
            normal = safe_cast<T^>(rootPanel->FindName(L"normal"));
            pointerOver = safe_cast<T^>(rootPanel->FindName(L"pointerOver"));
            disabled = safe_cast<T^>(rootPanel->FindName(L"disabled"));
            normalSelected = safe_cast<T^>(rootPanel->FindName(L"normalSelected"));
            pointerOverSelected = safe_cast<T^>(rootPanel->FindName(L"pointerOverSelected"));
            disabledSelected = safe_cast<T^>(rootPanel->FindName(L"disabledSelected"));
            multiSelect = safe_cast<T^>(rootPanel->FindName(L"multiSelect"));
            multiSelectSelected = safe_cast<T^>(rootPanel->FindName(L"multiSelectSelected"));
            multiSelectDisabled = safe_cast<T^>(rootPanel->FindName(L"multiSelectDisabled"));
            multiSelectSelectedDisabled = safe_cast<T^>(rootPanel->FindName(L"multiSelectSelectedDisabled"));
        });

        RunOnUIThread([&]()
        {
            VisualStateManager::GoToState(normal, "Normal", false);

            VisualStateManager::GoToState(pointerOver, "PointerOver", false);

            VisualStateManager::GoToState(disabled, "Disabled", false);

            VisualStateManager::GoToState(normalSelected, "Selected", false);

            VisualStateManager::GoToState(pointerOverSelected, "PointerOverSelected", false);

            VisualStateManager::GoToState(disabledSelected, "Disabled", false);
            VisualStateManager::GoToState(disabledSelected, "Selected", false);

            VisualStateManager::GoToState(multiSelect, "MultiSelectEnabled", false);

            VisualStateManager::GoToState(multiSelectSelected, "MultiSelectEnabled", false);
            VisualStateManager::GoToState(multiSelectSelected, "Selected", false);

            VisualStateManager::GoToState(multiSelectDisabled, "MultiSelectEnabled", false);
            VisualStateManager::GoToState(multiSelectDisabled, "Disabled", false);

            VisualStateManager::GoToState(multiSelectSelectedDisabled, "MultiSelectEnabled", false);
            VisualStateManager::GoToState(multiSelectSelectedDisabled, "Selected", false);
            VisualStateManager::GoToState(multiSelectSelectedDisabled, "Disabled", false);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    template<typename T>
    static xaml_controls::Grid^ SetupMoCoWithVariousVisualStates(Platform::String^ type, Platform::String^ style)
    {
        xaml_controls::Grid^ rootPanel = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <Grid.ColumnDefinitions>"
                L"        <ColumnDefinition/>"
                L"        <ColumnDefinition/>"
                L"    </Grid.ColumnDefinitions>"
                L"    <StackPanel Grid.Column='0'>"
                L"        <" + type + L" " + style + L" x:Name='normal' Content='Normal'/>"
                L"        <" + type + L" " + style + L" x:Name='pointerOver' Content='Pointer Over'/>"
                L"        <" + type + L" " + style + L" x:Name='disabled' Content='Disabled'/>"
                L"        <" + type + L" " + style + L" x:Name='normalSelected' Content='Selected'/>"
                L"        <" + type + L" " + style + L" x:Name='pointerOverSelected' Content='Pointer Over Selected'/>"
                L"        <" + type + L" " + style + L" x:Name='disabledSelected' Content='Disabled Selected'/>"
                L"    </StackPanel>"
                L"    <StackPanel Grid.Column='1'>"
                L"        <" + type + L" " + style + L" x:Name='multiSelect' Content='Multi Select'/>"
                L"        <" + type + L" " + style + L" x:Name='multiSelectSelected' Content='MS Selected'/>"
                L"        <" + type + L" " + style + L" x:Name='multiSelectDisabled' Content='MS Disabled'/>"
                L"        <" + type + L" " + style + L" x:Name='multiSelectSelectedDisabled' Content='MS Selected Disabled'/>"
                L"    </StackPanel>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        GoToVisualStates<T>(rootPanel);

        return rootPanel;
    }

    template<typename T>
    static void ValidateVisualStyle(Platform::String^ type, Platform::String^ style, DCompRendering renderingMode = DCompRendering::WUCCompleteSynchronousCompTree)
    {
        // MockDComp should be injected and detached per test, since it keeps information like the surfaces
        // that get created.
        WUCRenderingScopeGuard guard(renderingMode, false /*resizeWindow*/);

        // Set a consistent window size and zoom scale, so that the DComp tree is predictable.
        TestServices::WindowHelper->SetWindowSizeOverrideWithScale(wf::Size(400.0f, 600.0f), 1.0f);

        SetupMoCoWithVariousVisualStates<T>(type, style);

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    template<typename T>
    static void VerifyDragOverState(Microsoft::UI::Xaml::Controls::ListViewBase^ listViewBase)
    {
        ::Windows::Foundation::Point itemCenter = {};
        ::Windows::Foundation::Point folderCenter = {};
        Microsoft::UI::Xaml::FrameworkElement^ itemAsFE = nullptr;
        Microsoft::UI::Xaml::FrameworkElement^ folderAsFE = nullptr;
        Microsoft::UI::Xaml::VisualStateGroup^ commonStatesVisualStateGroup = nullptr;
        auto stateChangedEvent = std::make_shared<Event>();
        auto stateChangedRegistration = CreateSafeEventRegistration(xaml::VisualStateGroup, CurrentStateChanged);

        RunOnUIThread([&]()
        {
            // get the 1st item (a folder)
            folderAsFE = safe_cast<xaml::FrameworkElement^>(listViewBase->ContainerFromIndex(0));
            VERIFY_IS_NOT_NULL(folderAsFE);

            // get the 4th item (not a folder)
            itemAsFE = safe_cast<xaml::FrameworkElement^>(listViewBase->ContainerFromIndex(3));
            VERIFY_IS_NOT_NULL(itemAsFE);

            // get the elements' centers
            folderCenter = xaml::Tests::Common::ControlHelper::GetCenterOfElement(folderAsFE);
            itemCenter = xaml::Tests::Common::ControlHelper::GetCenterOfElement(itemAsFE);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Register for VisualStateGroup.CurrentStateChanged for the Folder's DragStates VSG
            auto templateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(safe_cast<T^>(folderAsFE), 0));
            auto groups = xaml::VisualStateManager::GetVisualStateGroups(templateRoot);

            VERIFY_IS_TRUE(groups->Size > 0);

            // we want to find the DragStates VSG
            for (unsigned int i = 0; i < groups->Size; ++i)
            {
                commonStatesVisualStateGroup = groups->GetAt(i);

                if (commonStatesVisualStateGroup->Name == L"DragStates")
                {
                    break;
                }
            }

            stateChangedRegistration.Attach(commonStatesVisualStateGroup, ref new xaml::VisualStateChangedEventHandler(
                [stateChangedEvent](Platform::Object^, xaml::VisualStateChangedEventArgs^ args)
            {
                if (args->NewState->Name == L"DragOver")
                {
                    stateChangedEvent->Set();
                }
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        // Drag the item to the folder
        TestServices::InputHelper->PressHoldAndPanFromCenter(itemAsFE,
                                                                static_cast<int>(folderCenter.X - itemCenter.X) /* relX */,
                                                                static_cast<int>(folderCenter.Y - itemCenter.Y) /* relY */,
                                                                0.1 /* velocityFactor */,
                                                                1000 /* holdTime */);
        TestServices::WindowHelper->WaitForIdle();

        // this applies only to Desktop runs
        if (TestServices::Utilities->IsDesktop)
        {
            // Workaround for a known issue
            TestServices::WindowHelper->RestoreForegroundWindow();
        }

        stateChangedEvent->WaitForDefault();
    }

    //
    // Class & Test Setup
    //
    bool IntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool IntegrationTests::ClassCleanup()
    {
        return true;
    }

    bool IntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool IntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void IntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::ListViewItem>::CanInstantiate();
        Generic::DependencyObjectTests<xaml_controls::GridViewItem>::CanInstantiate();
    }

    void IntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::ListViewItem>::CanEnterAndLeaveLiveTree();
        Generic::FrameworkElementTests<xaml_controls::GridViewItem>::CanEnterAndLeaveLiveTree();
    }

    void IntegrationTests::ValidateVisualStyleListViewItem()
    {
        ValidateVisualStyle<xaml_controls::ListViewItem>(L"ListViewItem", L"Style = '{ThemeResource ListViewItemExpanded}'");
    }

    void IntegrationTests::ValidateVisualStyleGridViewItem()
    {
        ValidateVisualStyle<xaml_controls::GridViewItem>(L"GridViewItem", L"Style = '{ThemeResource GridViewItemExpanded}'");
    }

    void IntegrationTests::ValidateVisualStyleLVIChrome()
    {
        // no style means use default which is Chrome
        ValidateVisualStyle<xaml_controls::ListViewItem>(L"ListViewItem", L"" /* style */);
    }

    void IntegrationTests::ValidateVisualStyleGVIChrome()
    {
        // no style means use default which is Chrome
        ValidateVisualStyle<xaml_controls::GridViewItem>(L"GridViewItem", L"" /* style */);
    }

    void IntegrationTests::ValidateMultiLineTextBlock()
    {
        // MockDComp should be injected and detached per test, since it keeps information like the surfaces
        // that get created.
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        // Set a consistent window size and zoom scale, so that the DComp tree is predictable.
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400.0f, 600.0f));

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <ListView Margin='0,12,0,0' Background='LightCoral' SelectionMode='Multiple' Width='300'>"
                L"        <ListViewItem Background='LightBlue'>"
                L"            <TextBlock Text='Using long words is important however using longer words is of utmost importance' TextWrapping='WrapWholeWords' />"
                L"        </ListViewItem>"
                L"        <ListViewItem Background='LightGreen'>"
                L"            <TextBlock Text='Using long words is important however using longer words is of utmost importance' TextTrimming='CharacterEllipsis' />"
                L"        </ListViewItem>"
                L"    </ListView>"
                L"    <ListView Margin='0,12,0,0' Background='LightCoral' SelectionMode='Multiple' Width='300' ItemContainerStyle='{ThemeResource ListViewItemExpanded}'>"
                L"        <ListViewItem Background='LightBlue'>"
                L"            <TextBlock Text='Using long words is important however using longer words is of utmost importance' TextWrapping='WrapWholeWords' />"
                L"        </ListViewItem>"
                L"        <ListViewItem Background='LightGreen'>"
                L"            <TextBlock Text='Using long words is important however using longer words is of utmost importance' TextTrimming='CharacterEllipsis' />"
                L"        </ListViewItem>"
                L"    </ListView>"
                L"</StackPanel>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void IntegrationTests::ValidateRTLSupportForCheckMark()
    {
        // MockDComp should be injected and detached per test, since it keeps information like the surfaces
        // that get created.
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        // Set a consistent window size and zoom scale, so that the DComp tree is predictable.
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400.0f, 600.0f));

        xaml_controls::ListView^ listView = nullptr;
        xaml_controls::GridView^ gridView = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <ListView x:Name='listView' SelectionMode='Multiple' Width='300' FlowDirection='RightToLeft'>"
                L"        <ListViewItem Content='Item 1' />"
                L"        <ListViewItem Content='Item 2' Style='{ThemeResource ListViewItemExpanded}'/>"
                L"    </ListView>"
                L"    <GridView x:Name='gridView' SelectionMode='Multiple' Width='300' FlowDirection='RightToLeft'>"
                L"        <GridViewItem Content='Item 1' />"
                L"        <GridViewItem Content='Item 2' Style='{ThemeResource GridViewItemExpanded}'/>"
                L"    </GridView>"
                L"</StackPanel>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            gridView = safe_cast<xaml_controls::GridView^>(rootPanel->FindName(L"gridView"));
            VERIFY_IS_NOT_NULL(gridView);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            listView->SelectAll();
            gridView->SelectAll();
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void IntegrationTests::DisplayListAndGridViews()
    {
        TestCleanupWrapper cleanup;

        DisplayListAndGridViews(
            false /*isInteractive*/, // Use 'true' for manual interactive testing
            true  /*enableRoundedListViewBaseItemChrome*/,
            false /*forceSelectionIndicatorModeInline*/,
            false /*forceSelectionIndicatorModeOverlay*/);
    }

    void IntegrationTests::DisplayListAndGridViewsWithSelectionIndicatorModeInline()
    {
        TestCleanupWrapper cleanup;

        DisplayListAndGridViews(
            false /*isInteractive*/, // Use 'true' for manual interactive testing
            true  /*enableRoundedListViewBaseItemChrome*/,
            true  /*forceSelectionIndicatorModeInline*/,
            false /*forceSelectionIndicatorModeOverlay*/);
    }

    void IntegrationTests::DisplayListAndGridViewsWithSelectionIndicatorModeOverlay()
    {
        TestCleanupWrapper cleanup;

        DisplayListAndGridViews(
            false /*isInteractive*/, // Use 'true' for manual interactive testing
            true  /*enableRoundedListViewBaseItemChrome*/,
            false /*forceSelectionIndicatorModeInline*/,
            true  /*forceSelectionIndicatorModeOverlay*/);
    }

    void IntegrationTests::DisplayListAndGridViewsWithOldStyles()
    {
        TestCleanupWrapper cleanup;

        DisplayListAndGridViews(
            false /*isInteractive*/, // Use 'true' for manual interactive testing
            false /*enableRoundedListViewBaseItemChrome*/,
            false /*forceSelectionIndicatorModeInline*/,
            false /*forceSelectionIndicatorModeOverlay*/);
    }

    void IntegrationTests::DisplayListAndGridViews(
        bool isInteractive,
        bool enableRoundedListViewBaseItemChrome,
        bool forceSelectionIndicatorModeInline,
        bool forceSelectionIndicatorModeOverlay)
    {
        RoundedChromeDictionaryHelper dictionaryHelper(enableRoundedListViewBaseItemChrome);

        RuntimeEnabledFeatureOverride featureSelectionIndicatorMode;

        if (forceSelectionIndicatorModeInline)
        {
            featureSelectionIndicatorMode.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceSelectionIndicatorModeInline, true);
        }

        if (forceSelectionIndicatorModeOverlay)
        {
            featureSelectionIndicatorMode.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceSelectionIndicatorModeOverlay, true);
        }

        xaml_controls::CheckBox^ chkExit = nullptr;
        xaml_controls::CheckBox^ chkListViewIsEnabled = nullptr;
        xaml_controls::CheckBox^ chkGridViewIsEnabled = nullptr;
        xaml_controls::CheckBox^ chkEnableRoundedListViewBaseItemChrome = nullptr;
        xaml_controls::CheckBox^ chkForceSelectionIndicatorModeInline = nullptr;
        xaml_controls::CheckBox^ chkForceSelectionIndicatorModeOverlay = nullptr;
        xaml_controls::ListView^ listView1 = nullptr;
        xaml_controls::GridView^ gridView1 = nullptr;
        xaml_controls::ListViewItem^ listViewItem1 = nullptr;
        xaml_controls::GridViewItem^ gridViewItem1 = nullptr;
        xaml_controls::ComboBox^ cmbListViewSelectionMode = nullptr;
        xaml_controls::ComboBox^ cmbGridViewSelectionMode = nullptr;

        auto chkListViewIsEnabledCheckedRegistration = CreateSafeEventRegistration(xaml_controls::CheckBox, Checked);
        auto chkListViewIsEnabledUncheckedRegistration = CreateSafeEventRegistration(xaml_controls::CheckBox, Unchecked);
        auto chkGridViewIsEnabledCheckedRegistration = CreateSafeEventRegistration(xaml_controls::CheckBox, Checked);
        auto chkGridViewIsEnabledUncheckedRegistration = CreateSafeEventRegistration(xaml_controls::CheckBox, Unchecked);
        auto cmbListViewSelectionModeSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, SelectionChanged);
        auto cmbGridViewSelectionModeSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, SelectionChanged);
        auto cmbListViewSelectionModeSelectionChangedEvent = std::make_shared<Event>();
        auto cmbGridViewSelectionModeSelectionChangedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <StackPanel.Resources>"
                L"        <x:Double x:Key='ListViewItemDisabledThemeOpacity'>0.3</x:Double>"
                L"        <x:Boolean x:Key='ListViewItemSelectionIndicatorVisualEnabled'>True</x:Boolean>"
                L""
                L"        <CornerRadius x:Key='GridViewItemCornerRadius'>4</CornerRadius>"
                L"        <CornerRadius x:Key='GridViewItemCheckBoxCornerRadius'>3</CornerRadius>"
                L"        <Thickness x:Key='GridViewItemSelectedBorderThickness'>2</Thickness>"
                L"        <SolidColorBrush x:Key='GridViewItemBackground' Color='#00000000' />"
                L"        <SolidColorBrush x:Key='GridViewItemForeground' Color='#FFFFFFFF' />"
                L"        <SolidColorBrush x:Key='GridViewItemPointerOverBorderBrush' Color='#37000000' />"
                L"        <SolidColorBrush x:Key='GridViewItemSelectedBorderBrush' Color='#FF01C0FB' />"
                L"        <SolidColorBrush x:Key='GridViewItemSelectedPointerOverBorderBrush' Color='#FF49D3FF' />"
                L"        <SolidColorBrush x:Key='GridViewItemSelectedPressedBorderBrush' Color='#FF01B7F6' />"
                L"        <SolidColorBrush x:Key='GridViewItemSelectedDisabledBorderBrush' Color='#28FFFFFF' />"
                L"        <SolidColorBrush x:Key='GridViewItemSelectedInnerBorderBrush' Color='#FF454545' />"
                L"        <SolidColorBrush x:Key='GridViewItemCheckBrush' Color='#D2000000' />"
                L"        <SolidColorBrush x:Key='GridViewItemCheckPressedBrush' Color='#80000000' />"
                L"        <SolidColorBrush x:Key='GridViewItemCheckDisabledBrush' Color='#87FFFFFF' />"
                L"        <SolidColorBrush x:Key='GridViewItemCheckBoxBrush' Color='#B21C1C1C' />"
                L"        <SolidColorBrush x:Key='GridViewItemCheckBoxPointerOverBrush' Color='#FF1A1A1A' />"
                L"        <SolidColorBrush x:Key='GridViewItemCheckBoxPressedBrush' Color='#FF131313' />"
                L"        <SolidColorBrush x:Key='GridViewItemCheckBoxDisabledBrush' Color='#28FFFFFF' />"
                L"        <SolidColorBrush x:Key='GridViewItemCheckBoxSelectedBrush' Color='#FF01C0FB' />"
                L"        <SolidColorBrush x:Key='GridViewItemCheckBoxSelectedPointerOverBrush' Color='#FF49D3FF' />"
                L"        <SolidColorBrush x:Key='GridViewItemCheckBoxSelectedPressedBrush' Color='#FF01B7F6' />"
                L"        <SolidColorBrush x:Key='GridViewItemCheckBoxSelectedDisabledBrush' Color='#28FFFFFF' />"
                L"        <SolidColorBrush x:Key='GridViewItemCheckBoxBorderBrush' Color='#8AFFFFFF' />"
                L"        <SolidColorBrush x:Key='GridViewItemCheckBoxPointerOverBorderBrush' Color='#8AFFFFFF' />"
                L"        <SolidColorBrush x:Key='GridViewItemCheckBoxPressedBorderBrush' Color='#28FFFFFF' />"
                L"        <SolidColorBrush x:Key='GridViewItemCheckBoxDisabledBorderBrush' Color='#28FFFFFF' />"
                L"        <SolidColorBrush x:Key='GridViewItemBackgroundPointerOver' Color='#08FFFFFF' />"
                L"        <SolidColorBrush x:Key='GridViewItemForegroundPointerOver' Color='#C8FFFFFF' />"
                L"        <SolidColorBrush x:Key='GridViewItemBackgroundSelected' Color='#0FFFFFFF' />"
                L"        <SolidColorBrush x:Key='GridViewItemForegroundSelected' Color='#FFFFFFFF' />"
                L"        <SolidColorBrush x:Key='GridViewItemBackgroundSelectedPointerOver' Color='#08FFFFFF' />"
                L"        <SolidColorBrush x:Key='GridViewItemBackgroundPressed' Color='#0FFFFFFF' />"
                L"        <SolidColorBrush x:Key='GridViewItemBackgroundSelectedPressed' Color='#0FFFFFFF' />"
                L"        <SolidColorBrush x:Key='GridViewItemBackgroundSelectedDisabled' Color='#08FFFFFF' />"
                L""
                L"        <Style TargetType='GridViewItem'>"
                L"            <Setter Property='FontFamily' Value='{ThemeResource ContentControlThemeFontFamily}' />"
                L"            <Setter Property='FontSize' Value='{ThemeResource ControlContentThemeFontSize}' />"
                L"            <Setter Property='Background' Value='{ThemeResource GridViewItemBackground}' />"
                L"            <Setter Property='Foreground' Value='{ThemeResource GridViewItemForeground}' />"
                L"            <Setter Property='TabNavigation' Value='Local' />"
                L"            <Setter Property='IsHoldingEnabled' Value='True' />"
                L"            <Setter Property='HorizontalContentAlignment' Value='Center' />"
                L"            <Setter Property='VerticalContentAlignment' Value='Center' />"
                L"            <Setter Property='Margin' Value='0,0,4,4' />"
                L"            <Setter Property='MinWidth' Value='{ThemeResource GridViewItemMinWidth}' />"
                L"            <Setter Property='MinHeight' Value='{ThemeResource GridViewItemMinHeight}' />"
                L"            <Setter Property='AllowDrop' Value='True' />"
                L"            <Setter Property='UseSystemFocusVisuals' Value='{StaticResource UseSystemFocusVisuals}' />"
                L"            <Setter Property='FocusVisualMargin' Value='-2' />"
                L"            <Setter Property='Template'>"
                L"                <Setter.Value>"
                L"                    <ControlTemplate TargetType='GridViewItem'>"
                L"                        <ListViewItemPresenter ContentTransitions='{TemplateBinding ContentTransitions}'"
                L"                            x:Name='Root'"
                L"                            Control.IsTemplateFocusTarget='True'"
                L"                            FocusVisualMargin='{TemplateBinding FocusVisualMargin}'"
                L"                            SelectionCheckMarkVisualEnabled='{ThemeResource GridViewItemSelectionCheckMarkVisualEnabled}'"
                L"                            CheckBrush='{ThemeResource GridViewItemCheckBrush}'"
                L"                            CheckPressedBrush='{ThemeResource GridViewItemCheckPressedBrush}'"
                L"                            CheckDisabledBrush='{ThemeResource GridViewItemCheckDisabledBrush}'"
                L"                            CheckBoxBrush='{ThemeResource GridViewItemCheckBoxBrush}'"
                L"                            CheckBoxPointerOverBrush='{ThemeResource GridViewItemCheckBoxPointerOverBrush}'"
                L"                            CheckBoxPressedBrush='{ThemeResource GridViewItemCheckBoxPressedBrush}'"
                L"                            CheckBoxDisabledBrush='{ThemeResource GridViewItemCheckBoxDisabledBrush}'"
                L"                            CheckBoxSelectedBrush='{ThemeResource GridViewItemCheckBoxSelectedBrush}'"
                L"                            CheckBoxSelectedPointerOverBrush='{ThemeResource GridViewItemCheckBoxSelectedPointerOverBrush}'"
                L"                            CheckBoxSelectedPressedBrush='{ThemeResource GridViewItemCheckBoxSelectedPressedBrush}'"
                L"                            CheckBoxSelectedDisabledBrush='{ThemeResource GridViewItemCheckBoxSelectedDisabledBrush}'"
                L"                            CheckBoxBorderBrush='{ThemeResource GridViewItemCheckBoxBorderBrush}'"
                L"                            CheckBoxPointerOverBorderBrush='{ThemeResource GridViewItemCheckBoxPointerOverBorderBrush}'"
                L"                            CheckBoxPressedBorderBrush='{ThemeResource GridViewItemCheckBoxPressedBorderBrush}'"
                L"                            CheckBoxDisabledBorderBrush='{ThemeResource GridViewItemCheckBoxDisabledBorderBrush}'"
                L"                            DragBackground='{ThemeResource GridViewItemDragBackground}'"
                L"                            DragForeground='{ThemeResource GridViewItemDragForeground}'"
                L"                            FocusBorderBrush='{ThemeResource GridViewItemFocusBorderBrush}'"
                L"                            PlaceholderBackground='{ThemeResource GridViewItemPlaceholderBackground}'"
                L"                            PointerOverBorderBrush='{ThemeResource GridViewItemPointerOverBorderBrush}'"
                L"                            PointerOverBackground='{ThemeResource GridViewItemBackgroundPointerOver}'"
                L"                            PointerOverForeground='{ThemeResource GridViewItemForegroundPointerOver}'"
                L"                            SelectedBackground='{ThemeResource GridViewItemBackgroundSelected}'"
                L"                            SelectedForeground='{ThemeResource GridViewItemForegroundSelected}'"
                L"                            SelectedPointerOverBackground='{ThemeResource GridViewItemBackgroundSelectedPointerOver}'"
                L"                            PressedBackground='{ThemeResource GridViewItemBackgroundPressed}'"
                L"                            SelectedPressedBackground='{ThemeResource GridViewItemBackgroundSelectedPressed}'"
                L"                            SelectedDisabledBackground='{ThemeResource GridViewItemBackgroundSelectedDisabled}'"
                L"                            DisabledOpacity='{ThemeResource ListViewItemDisabledThemeOpacity}'"
                L"                            DragOpacity='{ThemeResource ListViewItemDragThemeOpacity}'"
                L"                            ReorderHintOffset='{ThemeResource GridViewItemReorderHintThemeOffset}'"
                L"                            HorizontalContentAlignment='{TemplateBinding HorizontalContentAlignment}'"
                L"                            VerticalContentAlignment='{TemplateBinding VerticalContentAlignment}'"
                L"                            ContentMargin='{TemplateBinding Padding}'"
                L"                            CheckMode='{ThemeResource GridViewItemCheckMode}'"
                L"                            SelectedBorderThickness='{ThemeResource GridViewItemSelectedBorderThickness}'"
                L"                            SelectedBorderBrush='{ThemeResource GridViewItemSelectedBorderBrush}'"
                L"                            SelectedPointerOverBorderBrush='{ThemeResource GridViewItemSelectedPointerOverBorderBrush}'"
                L"                            SelectedPressedBorderBrush='{ThemeResource GridViewItemSelectedPressedBorderBrush}'"
                L"                            SelectedDisabledBorderBrush='{ThemeResource GridViewItemSelectedDisabledBorderBrush}'"
                L"                            SelectedInnerBorderBrush='{ThemeResource GridViewItemSelectedInnerBorderBrush}'"
                L"                            CornerRadius='{ThemeResource GridViewItemCornerRadius}'"
                L"                            CheckBoxCornerRadius='{ThemeResource GridViewItemCheckBoxCornerRadius}'>"
                L"                        </ListViewItemPresenter>"
                L"                    </ControlTemplate>"
                L"                </Setter.Value>"
                L"            </Setter>"
                L"        </Style>"
                L""
                L"        <CornerRadius x:Key='ListViewItemCornerRadius'>4</CornerRadius>"
                L"        <CornerRadius x:Key='ListViewItemCheckBoxCornerRadius'>3</CornerRadius>"
                L"        <CornerRadius x:Key='ListViewItemSelectionIndicatorCornerRadius'>1.5</CornerRadius>"
                L"        <SolidColorBrush x:Key='ListViewItemBackground' Color='#00000000' />"
                L"        <SolidColorBrush x:Key='ListViewItemForeground' Color='#FFFFFFFF' />"
                L"        <SolidColorBrush x:Key='ListViewItemCheckBrush' Color='#D2000000' />"
                L"        <SolidColorBrush x:Key='ListViewItemCheckPressedBrush' Color='#80000000' />"
                L"        <SolidColorBrush x:Key='ListViewItemCheckDisabledBrush' Color='#87FFFFFF' />"
                L"        <SolidColorBrush x:Key='ListViewItemCheckBoxBrush' Color='#B21C1C1C' />"
                L"        <SolidColorBrush x:Key='ListViewItemCheckBoxPointerOverBrush' Color='#FF1A1A1A' />"
                L"        <SolidColorBrush x:Key='ListViewItemCheckBoxPressedBrush' Color='#FF131313' />"
                L"        <SolidColorBrush x:Key='ListViewItemCheckBoxDisabledBrush' Color='#19000000' />"
                L"        <SolidColorBrush x:Key='ListViewItemCheckBoxSelectedBrush' Color='#FF01C0FB' />"
                L"        <SolidColorBrush x:Key='ListViewItemCheckBoxSelectedPointerOverBrush' Color='#FF49D3FF' />"
                L"        <SolidColorBrush x:Key='ListViewItemCheckBoxSelectedPressedBrush' Color='#FF01B7F6' />"
                L"        <SolidColorBrush x:Key='ListViewItemCheckBoxSelectedDisabledBrush' Color='#28FFFFFF' />"
                L"        <SolidColorBrush x:Key='ListViewItemCheckBoxBorderBrush' Color='#8AFFFFFF' />"
                L"        <SolidColorBrush x:Key='ListViewItemCheckBoxPointerOverBorderBrush' Color='#8AFFFFFF' />"
                L"        <SolidColorBrush x:Key='ListViewItemCheckBoxPressedBorderBrush' Color='#8AFFFFFF' />"
                L"        <SolidColorBrush x:Key='ListViewItemCheckBoxDisabledBorderBrush' Color='#8AFFFFFF' />"
                L"        <SolidColorBrush x:Key='ListViewItemBackgroundPointerOver' Color='#08FFFFFF' />"
                L"        <SolidColorBrush x:Key='ListViewItemForegroundPointerOver' Color='#C8FFFFFF' />"
                L"        <SolidColorBrush x:Key='ListViewItemBackgroundSelected' Color='#0FFFFFFF' />"
                L"        <SolidColorBrush x:Key='ListViewItemForegroundSelected' Color='#FFFFFFFF' />"
                L"        <SolidColorBrush x:Key='ListViewItemBackgroundSelectedPointerOver' Color='#08FFFFFF' />"
                L"        <SolidColorBrush x:Key='ListViewItemBackgroundPressed' Color='#0FFFFFFF' />"
                L"        <SolidColorBrush x:Key='ListViewItemBackgroundSelectedPressed' Color='#0FFFFFFF' />"
                L"        <SolidColorBrush x:Key='ListViewItemBackgroundSelectedDisabled' Color='#08FFFFFF' />"
                L"        <SolidColorBrush x:Key='ListViewItemSelectionIndicatorBrush' Color='#FF01C0FB' />"
                L"        <SolidColorBrush x:Key='ListViewItemSelectionIndicatorPointerOverBrush' Color='#FF01C0FB' />"
                L"        <SolidColorBrush x:Key='ListViewItemSelectionIndicatorPressedBrush' Color='#FF01C0FB' />"
                L"        <SolidColorBrush x:Key='ListViewItemSelectionIndicatorDisabledBrush' Color='#28FFFFFF' />"
                L"        <x:Double x:Key='ListViewItemMinHeight'>40</x:Double>"
                L""
                L"        <Style TargetType='ListViewItem'>"
                L"            <Setter Property='FontFamily' Value='{ThemeResource ContentControlThemeFontFamily}' />"
                L"            <Setter Property='FontSize' Value='{ThemeResource ControlContentThemeFontSize}' />"
                L"            <Setter Property='Background' Value='{ThemeResource ListViewItemBackground}' />"
                L"            <Setter Property='Foreground' Value='{ThemeResource ListViewItemForeground}' />"
                L"            <Setter Property='TabNavigation' Value='LocaL' />"
                L"            <Setter Property='IsHoldingEnabled' Value='True' />"
                L"            <Setter Property='Padding' Value='16,0,12,0' />"
                L"            <Setter Property='HorizontalContentAlignment' Value='Left' />"
                L"            <Setter Property='VerticalContentAlignment' Value='Center' />"
                L"            <Setter Property='MinWidth' Value='{ThemeResource ListViewItemMinWidth}' />"
                L"            <Setter Property='MinHeight' Value='{ThemeResource ListViewItemMinHeight}' />"
                L"            <Setter Property='AllowDrop' Value='True' />"
                L"            <Setter Property='UseSystemFocusVisuals' Value='{StaticResource UseSystemFocusVisuals}' />"
                L"            <Setter Property='FocusVisualMargin' Value='0' />"
                L"            <Setter Property='Template'>"
                L"                <Setter.Value>"
                L"                    <ControlTemplate TargetType='ListViewItem'>"
                L"                        <ListViewItemPresenter ContentTransitions='{TemplateBinding ContentTransitions}'"
                L"                            x:Name='Root'"
                L"                            Control.IsTemplateFocusTarget='True'"
                L"                            FocusVisualMargin='{TemplateBinding FocusVisualMargin}'"
                L"                            SelectionCheckMarkVisualEnabled='{ThemeResource ListViewItemSelectionCheckMarkVisualEnabled}'"
                L"                            CheckBrush='{ThemeResource ListViewItemCheckBrush}'"
                L"                            CheckPressedBrush='{ThemeResource ListViewItemCheckPressedBrush}'"
                L"                            CheckDisabledBrush='{ThemeResource ListViewItemCheckDisabledBrush}'"
                L"                            CheckBoxBrush='{ThemeResource ListViewItemCheckBoxBrush}'"
                L"                            CheckBoxPointerOverBrush='{ThemeResource ListViewItemCheckBoxPointerOverBrush}'"
                L"                            CheckBoxPressedBrush='{ThemeResource ListViewItemCheckBoxPressedBrush}'"
                L"                            CheckBoxDisabledBrush='{ThemeResource ListViewItemCheckBoxDisabledBrush}'"
                L"                            CheckBoxSelectedBrush='{ThemeResource ListViewItemCheckBoxSelectedBrush}'"
                L"                            CheckBoxSelectedPointerOverBrush='{ThemeResource ListViewItemCheckBoxSelectedPointerOverBrush}'"
                L"                            CheckBoxSelectedPressedBrush='{ThemeResource ListViewItemCheckBoxSelectedPressedBrush}'"
                L"                            CheckBoxSelectedDisabledBrush='{ThemeResource ListViewItemCheckBoxSelectedDisabledBrush}'"
                L"                            CheckBoxBorderBrush='{ThemeResource ListViewItemCheckBoxBorderBrush}'"
                L"                            CheckBoxPointerOverBorderBrush='{ThemeResource ListViewItemCheckBoxPointerOverBorderBrush}'"
                L"                            CheckBoxPressedBorderBrush='{ThemeResource ListViewItemCheckBoxPressedBorderBrush}'"
                L"                            CheckBoxDisabledBorderBrush='{ThemeResource ListViewItemCheckBoxDisabledBorderBrush}'"
                L"                            DragBackground='{ThemeResource ListViewItemDragBackground}'"
                L"                            DragForeground='{ThemeResource ListViewItemDragForeground}'"
                L"                            FocusBorderBrush='{ThemeResource ListViewItemFocusBorderBrush}'"
                L"                            FocusSecondaryBorderBrush='{ThemeResource ListViewItemFocusSecondaryBorderBrush}'"
                L"                            PlaceholderBackground='{ThemeResource ListViewItemPlaceholderBackground}'"
                L"                            PointerOverBackground='{ThemeResource ListViewItemBackgroundPointerOver}'"
                L"                            PointerOverForeground='{ThemeResource ListViewItemForegroundPointerOver}'"
                L"                            SelectedBackground='{ThemeResource ListViewItemBackgroundSelected}'"
                L"                            SelectedForeground='{ThemeResource ListViewItemForegroundSelected}'"
                L"                            SelectedPointerOverBackground='{ThemeResource ListViewItemBackgroundSelectedPointerOver}'"
                L"                            SelectionIndicatorVisualEnabled='{ThemeResource ListViewItemSelectionIndicatorVisualEnabled}'"
                L"                            SelectionIndicatorBrush='{ThemeResource ListViewItemSelectionIndicatorBrush}'"
                L"                            SelectionIndicatorPointerOverBrush='{ThemeResource ListViewItemSelectionIndicatorPointerOverBrush}'"
                L"                            SelectionIndicatorPressedBrush='{ThemeResource ListViewItemSelectionIndicatorPressedBrush}'"
                L"                            SelectionIndicatorDisabledBrush='{ThemeResource ListViewItemSelectionIndicatorDisabledBrush}'"
                L"                            PressedBackground='{ThemeResource ListViewItemBackgroundPressed}'"
                L"                            SelectedPressedBackground='{ThemeResource ListViewItemBackgroundSelectedPressed}'"
                L"                            SelectedDisabledBackground='{ThemeResource ListViewItemBackgroundSelectedDisabled}'"
                L"                            DisabledOpacity='{ThemeResource ListViewItemDisabledThemeOpacity}'"
                L"                            DragOpacity='{ThemeResource ListViewItemDragThemeOpacity}'"
                L"                            ReorderHintOffset='{ThemeResource ListViewItemReorderHintThemeOffset}'"
                L"                            HorizontalContentAlignment='{TemplateBinding HorizontalContentAlignment}'"
                L"                            VerticalContentAlignment='{TemplateBinding VerticalContentAlignment}'"
                L"                            ContentMargin='{TemplateBinding Padding}'"
                L"                            CheckMode='{ThemeResource ListViewItemCheckMode}'"
                L"                            CornerRadius='{ThemeResource ListViewItemCornerRadius}'"
                L"                            SelectionIndicatorCornerRadius='{ThemeResource ListViewItemSelectionIndicatorCornerRadius}'"
                L"                            CheckBoxCornerRadius='{ThemeResource ListViewItemCheckBoxCornerRadius}'>"
                L"                        </ListViewItemPresenter>"
                L"                    </ControlTemplate>"
                L"                </Setter.Value>"
                L"            </Setter>"
                L"        </Style>"
                L""
                L"    </StackPanel.Resources>"
                L"    <StackPanel Orientation='Horizontal'>"
                L"        <CheckBox x:Name='chkExit' Content='Exit?' Margin='2'/>"
                L"        <CheckBox x:Name='chkEnableRoundedListViewBaseItemChrome' Content='EnableRoundedListViewBaseItemChrome?' Margin='2' IsEnabled='False'/>"
                L"    </StackPanel>"
                L"    <StackPanel Orientation='Horizontal'>"
                L"        <CheckBox x:Name='chkForceSelectionIndicatorModeInline' Content='ForceSelectionIndicatorModeInline?' Margin='2' IsEnabled='False'/>"
                L"        <CheckBox x:Name='chkForceSelectionIndicatorModeOverlay' Content='ForceSelectionIndicatorModeOverlay?' Margin='2' IsEnabled='False'/>"
                L"    </StackPanel>"
                L"    <StackPanel Orientation='Horizontal'>"
                L"        <StackPanel>"
                L"            <StackPanel Orientation='Horizontal'>"
                L"                <TextBlock Text='ListView - SelectionMode=' VerticalAlignment='Center' Margin='2'/>"
                L"                <ComboBox x:Name='cmbListViewSelectionMode' SelectedIndex='2'>"
                L"                    <ComboBoxItem>None</ComboBoxItem>"
                L"                    <ComboBoxItem>Single</ComboBoxItem>"
                L"                   <ComboBoxItem>Multiple</ComboBoxItem>"
                L"                   <ComboBoxItem>Extended</ComboBoxItem>"
                L"                </ComboBox>"
                L"            </StackPanel>"
                L"            <CheckBox x:Name='chkListViewIsEnabled' Content='IsEnabled?' IsChecked='True' Margin='2'/>"
                L"            <ListView x:Name='listView1' CanReorderItems='True' AllowDrop='True' SelectionMode='Multiple' Width='300' HorizontalAlignment='Left' Margin='2'>"
                L"                <ListViewItem x:Name='listViewItem1' Content='List Item 1'/>"
                L"                <ListViewItem x:Name='listViewItem2' Content='List Item 2'/>"
                L"                <ListViewItem x:Name='listViewItem3' Content='List Item 3' IsSelected='true'/>"
                L"            </ListView>"
                L"        </StackPanel>"
                L"        <StackPanel>"
                L"            <StackPanel Orientation='Horizontal'>"
                L"                <TextBlock Text='GridView - SelectionMode=' VerticalAlignment='Center' Margin='2'/>"
                L"                <ComboBox x:Name='cmbGridViewSelectionMode' SelectedIndex='2'>"
                L"                    <ComboBoxItem>None</ComboBoxItem>"
                L"                    <ComboBoxItem>Single</ComboBoxItem>"
                L"                    <ComboBoxItem>Multiple</ComboBoxItem>"
                L"                    <ComboBoxItem>Extended</ComboBoxItem>"
                L"                </ComboBox>"
                L"            </StackPanel>"
                L"            <CheckBox x:Name='chkGridViewIsEnabled' Content='IsEnabled?' IsChecked='True' Margin='2'/>"
                L"            <GridView x:Name='gridView1' CanReorderItems='True' AllowDrop='True' SelectionMode='Multiple' Width='300' HorizontalAlignment='Left' Margin='2'>"
                L"                <GridViewItem x:Name='gridViewItem1' Content='Item 1' MinWidth='120' MinHeight='60'/>"
                L"                <GridViewItem x:Name='gridViewItem2' Content='Item 2' MinWidth='120' MinHeight='60'/>"
                L"                <GridViewItem x:Name='gridViewItem3' Content='Item 3' MinWidth='120' MinHeight='60' IsSelected='true'/>"
                L"            </GridView>"
                L"        </StackPanel>"
                L"    </StackPanel>"
                L"    <StackPanel Orientation='Horizontal'>"
                L"        <StackPanel>"
                L"            <TextBlock Text='ListView - SelectionMode=Extended:' Margin='2'/>"
                L"            <ListView x:Name='listView2' CanReorderItems='True' AllowDrop='True' SelectionMode='Extended' Width='300' HorizontalAlignment='Left' Margin='2'>"
                L"                <ListViewItem x:Name='listViewItem4' Content='List Item 4' Height='5'/>"
                L"                <ListViewItem x:Name='listViewItem5' Content='List Item 5' Height='70'/>"
                L"                <ListViewItem x:Name='listViewItem6' Content='List Item 6' Height='90' IsSelected='true'/>"
                L"            </ListView>"
                L"        </StackPanel>"
                L"        <StackPanel>"
                L"            <TextBlock Text='GridView - SelectionMode=Extended:' Margin='2'/>"
                L"            <GridView x:Name='gridView2' CanReorderItems='True' AllowDrop='True' SelectionMode='Extended' Width='300' HorizontalAlignment='Left' Margin='2'>"
                L"                <GridViewItem x:Name='gridViewItem4' Content='Item 4' MinWidth='120' MinHeight='60'/>"
                L"                <GridViewItem x:Name='gridViewItem5' Content='Item 5' MinWidth='120' MinHeight='60'/>"
                L"                <GridViewItem x:Name='gridViewItem6' Content='Item 6' MinWidth='120' MinHeight='60' IsSelected='true'/>"
                L"            </GridView>"
                L"        </StackPanel>"
                L"    </StackPanel>"
                L"    <StackPanel Orientation='Horizontal'>"
                L"        <StackPanel>"
                L"            <TextBlock Text='ListView - SelectionMode=Single:' Margin='2'/>"
                L"            <ListView x:Name='listView3' CanReorderItems='True' AllowDrop='True' SelectionMode='Single' Width='300' HorizontalAlignment='Left' Margin='2'>"
                L"                <ListViewItem x:Name='listViewItem7' Content='List Item 7'/>"
                L"                <ListViewItem x:Name='listViewItem8' Content='List Item 8'/>"
                L"                <ListViewItem x:Name='listViewItem9' Content='List Item 9' IsSelected='true'/>"
                L"            </ListView>"
                L"        </StackPanel>"
                L"        <StackPanel>"
                L"            <TextBlock Text='GridView - SelectionMode=Single:' Margin='2'/>"
                L"            <GridView x:Name='gridView3' CanReorderItems='True' AllowDrop='True' SelectionMode='Single' Width='300' HorizontalAlignment='Left' Margin='2'>"
                L"                <GridViewItem x:Name='gridViewItem7' Content='Item 7' MinWidth='120' MinHeight='60'/>"
                L"                <GridViewItem x:Name='gridViewItem8' Content='Item 8' MinWidth='120' MinHeight='60'/>"
                L"                <GridViewItem x:Name='gridViewItem9' Content='Item 9' MinWidth='120' MinHeight='60' IsSelected='true'/>"
                L"            </GridView>"
                L"        </StackPanel>"
                L"    </StackPanel>"
                L"</StackPanel>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            chkExit = safe_cast<xaml_controls::CheckBox^>(rootPanel->FindName(L"chkExit"));
            VERIFY_IS_NOT_NULL(chkExit);

            chkEnableRoundedListViewBaseItemChrome = safe_cast<xaml_controls::CheckBox^>(rootPanel->FindName(L"chkEnableRoundedListViewBaseItemChrome"));
            VERIFY_IS_NOT_NULL(chkEnableRoundedListViewBaseItemChrome);

            chkForceSelectionIndicatorModeInline = safe_cast<xaml_controls::CheckBox^>(rootPanel->FindName(L"chkForceSelectionIndicatorModeInline"));
            VERIFY_IS_NOT_NULL(chkForceSelectionIndicatorModeInline);

            chkForceSelectionIndicatorModeOverlay = safe_cast<xaml_controls::CheckBox^>(rootPanel->FindName(L"chkForceSelectionIndicatorModeOverlay"));
            VERIFY_IS_NOT_NULL(chkForceSelectionIndicatorModeOverlay);

            chkListViewIsEnabled = safe_cast<xaml_controls::CheckBox^>(rootPanel->FindName(L"chkListViewIsEnabled"));
            VERIFY_IS_NOT_NULL(chkListViewIsEnabled);

            chkGridViewIsEnabled = safe_cast<xaml_controls::CheckBox^>(rootPanel->FindName(L"chkGridViewIsEnabled"));
            VERIFY_IS_NOT_NULL(chkGridViewIsEnabled);

            listView1 = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView1"));
            VERIFY_IS_NOT_NULL(listView1);

            listViewItem1 = safe_cast<xaml_controls::ListViewItem^>(rootPanel->FindName(L"listViewItem1"));
            VERIFY_IS_NOT_NULL(listViewItem1);

            gridView1 = safe_cast<xaml_controls::GridView^>(rootPanel->FindName(L"gridView1"));
            VERIFY_IS_NOT_NULL(gridView1);

            gridViewItem1 = safe_cast<xaml_controls::GridViewItem^>(rootPanel->FindName(L"gridViewItem1"));
            VERIFY_IS_NOT_NULL(gridViewItem1);

            cmbListViewSelectionMode = safe_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"cmbListViewSelectionMode"));
            VERIFY_IS_NOT_NULL(cmbListViewSelectionMode);

            cmbGridViewSelectionMode = safe_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"cmbGridViewSelectionMode"));
            VERIFY_IS_NOT_NULL(cmbGridViewSelectionMode);

            chkListViewIsEnabledCheckedRegistration.Attach(chkListViewIsEnabled, ref new xaml::RoutedEventHandler(
                [&](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    listView1->IsEnabled = true;
                }));

            chkListViewIsEnabledUncheckedRegistration.Attach(chkListViewIsEnabled, ref new xaml::RoutedEventHandler(
                [&](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    listView1->IsEnabled = false;
                }));

            chkGridViewIsEnabledCheckedRegistration.Attach(chkGridViewIsEnabled, ref new xaml::RoutedEventHandler(
                [&](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    gridView1->IsEnabled = true;
                }));

            chkGridViewIsEnabledUncheckedRegistration.Attach(chkGridViewIsEnabled, ref new xaml::RoutedEventHandler(
                [&](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    gridView1->IsEnabled = false;
                }));

            cmbListViewSelectionModeSelectionChangedRegistration.Attach(cmbListViewSelectionMode, ref new xaml_controls::SelectionChangedEventHandler(
                [cmbListViewSelectionModeSelectionChangedEvent, listView1](Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^)
                {
                    listView1->SelectionMode = (xaml_controls::ListViewSelectionMode)safe_cast<xaml_controls::ComboBox^>(sender)->SelectedIndex;
                    cmbListViewSelectionModeSelectionChangedEvent->Set();
                }));

            cmbGridViewSelectionModeSelectionChangedRegistration.Attach(cmbGridViewSelectionMode, ref new xaml_controls::SelectionChangedEventHandler(
                [cmbGridViewSelectionModeSelectionChangedEvent, gridView1](Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^)
                {
                    gridView1->SelectionMode = (xaml_controls::ListViewSelectionMode)safe_cast<xaml_controls::ComboBox^>(sender)->SelectedIndex;
                    cmbGridViewSelectionModeSelectionChangedEvent->Set();
                }));

            chkEnableRoundedListViewBaseItemChrome->IsChecked = enableRoundedListViewBaseItemChrome;
            chkForceSelectionIndicatorModeInline->IsChecked = forceSelectionIndicatorModeInline;
            chkForceSelectionIndicatorModeOverlay->IsChecked = forceSelectionIndicatorModeOverlay;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            listViewItem1->IsSelected = true;
            gridViewItem1->IsSelected = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            listViewItem1->IsSelected = false;
            gridViewItem1->IsSelected = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        if (isInteractive)
        {
            bool exit = false;
            do
            {
                TestServices::WindowHelper->SynchronouslyTickUIThread(50);

                RunOnUIThread([&]()
                {
                    exit = chkExit->IsChecked->Value;
                });
            }
            while (!exit);

            TestServices::WindowHelper->WaitForIdle();
        }
    }

    void IntegrationTests::ValidateChromeNoPointerOverBrush()
    {
        // MockDComp should be injected and detached per test, since it keeps information like the surfaces
        // that get created.
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        // Set a consistent window size and zoom scale, so that the DComp tree is predictable.
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400.0f, 600.0f));

        xaml_controls::ListViewItem^ pointerOver = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <Grid.Resources>"
                L"        <Style TargetType='ListViewItem'>"
                L"            <Setter Property = 'FontFamily' Value = '{ThemeResource ContentControlThemeFontFamily}' />"
                L"            <Setter Property = 'FontSize' Value = '{ThemeResource ControlContentThemeFontSize}' />"
                L"            <Setter Property='Background' Value='Transparent' />"
                L"            <Setter Property='TabNavigation' Value='Local' />"
                L"            <Setter Property='IsHoldingEnabled' Value='True' />"
                L"            <Setter Property='Padding' Value='12,0,12,0' />"
                L"            <Setter Property='HorizontalContentAlignment' Value='Left' />"
                L"            <Setter Property='VerticalContentAlignment' Value='Center' />"
                L"            <Setter Property='MinWidth' Value='{ThemeResource ListViewItemMinWidth}' />"
                L"            <Setter Property='MinHeight' Value='{ThemeResource ListViewItemMinHeight}' />"
                L"            <Setter Property='Template'>"
                L"                <Setter.Value>"
                L"                    <ControlTemplate TargetType='ListViewItem'>"
                L"                        <ListViewItemPresenter"
                L"                            ContentTransitions='{TemplateBinding ContentTransitions}'"
                L"                            SelectionCheckMarkVisualEnabled='True'"
                L"                            CheckBrush='{ThemeResource SystemControlBackgroundBaseMediumHighBrush}'"
                L"                            CheckBoxBrush='{ThemeResource SystemControlBackgroundBaseMediumHighBrush}'"
                L"                            DragBackground='{ThemeResource ListViewItemDragBackgroundThemeBrush}'"
                L"                            DragForeground='{ThemeResource ListViewItemDragForegroundThemeBrush}'"
                L"                            FocusBorderBrush='{ThemeResource SystemControlForegroundAltHighBrush}'"
                L"                            FocusSecondaryBorderBrush='{ThemeResource SystemControlForegroundBaseHighBrush}'"
                L"                            PlaceholderBackground='{ThemeResource ListViewItemPlaceholderBackgroundThemeBrush}'"
                L"                            PointerOverBackground='{ThemeResource SystemControlForegroundListLowBrush}'"
                L"                            SelectedBackground='{ThemeResource SystemControlHighlightAltListAccentLowBrush}'"
                L"                            SelectedForeground='{ThemeResource ListViewItemSelectedForegroundThemeBrush}'"
                L"                            SelectedPointerOverBackground='{ThemeResource SystemControlHighlightAltListAccentMediumBrush}'"
                L"                            PressedBackground='{ThemeResource SystemControlForegroundListMediumBrush}'"
                L"                            SelectedPressedBackground='{ThemeResource SystemControlHighlightAltListAccentHighBrush}'"
                L"                            DisabledOpacity='{ThemeResource ListViewItemDisabledThemeOpacity}'"
                L"                            DragOpacity='{ThemeResource ListViewItemDragThemeOpacity}'"
                L"                            ReorderHintOffset='{ThemeResource ListViewItemReorderHintThemeOffset}'"
                L"                            HorizontalContentAlignment='{TemplateBinding HorizontalContentAlignment}'"
                L"                            VerticalContentAlignment='{TemplateBinding VerticalContentAlignment}'"
                L"                            ContentMargin='{TemplateBinding Padding}'"
                L"                            CheckMode='Inline' />"
                L"                    </ControlTemplate>"
                L"                </Setter.Value>"
                L"            </Setter>"
                L"        </Style>"
                L"    </Grid.Resources>"
                L"    <ListViewItem x:Name='pointerOver' Content='Item 1' />"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            pointerOver = safe_cast<xaml_controls::ListViewItem^>(rootPanel->FindName(L"pointerOver"));
            VERIFY_IS_NOT_NULL(pointerOver);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VisualStateManager::GoToState(pointerOver, "PointerOver", false);
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void IntegrationTests::ValidateSelectedForegroundBrush()
    {
        // MockDComp should be injected and detached per test, since it keeps information like the surfaces
        // that get created.
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        // Set a consistent window size and zoom scale, so that the DComp tree is predictable.
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400.0f, 600.0f));

        xaml_controls::ListViewItem^ pointerOver = nullptr;
        xaml_controls::ListViewItem^ selected = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <Grid.Resources>"
                L"        <Style TargetType='ListViewItem'>"
                L"            <Setter Property = 'FontFamily' Value = '{ThemeResource ContentControlThemeFontFamily}' />"
                L"            <Setter Property = 'FontSize' Value = '{ThemeResource ControlContentThemeFontSize}' />"
                L"            <Setter Property='Background' Value='Transparent' />"
                L"            <Setter Property='TabNavigation' Value='Local' />"
                L"            <Setter Property='IsHoldingEnabled' Value='True' />"
                L"            <Setter Property='Padding' Value='12,0,12,0' />"
                L"            <Setter Property='HorizontalContentAlignment' Value='Left' />"
                L"            <Setter Property='VerticalContentAlignment' Value='Center' />"
                L"            <Setter Property='MinWidth' Value='{ThemeResource ListViewItemMinWidth}' />"
                L"            <Setter Property='MinHeight' Value='{ThemeResource ListViewItemMinHeight}' />"
                L"            <Setter Property='Template'>"
                L"                <Setter.Value>"
                L"                    <ControlTemplate TargetType='ListViewItem'>"
                L"                        <ListViewItemPresenter"
                L"                            ContentTransitions='{TemplateBinding ContentTransitions}'"
                L"                            SelectionCheckMarkVisualEnabled='True'"
                L"                            CheckBrush='{ThemeResource SystemControlBackgroundBaseMediumHighBrush}'"
                L"                            CheckBoxBrush='{ThemeResource SystemControlBackgroundBaseMediumHighBrush}'"
                L"                            DragBackground='{ThemeResource ListViewItemDragBackgroundThemeBrush}'"
                L"                            DragForeground='{ThemeResource ListViewItemDragForegroundThemeBrush}'"
                L"                            FocusBorderBrush='{ThemeResource SystemControlForegroundAltHighBrush}'"
                L"                            FocusSecondaryBorderBrush='{ThemeResource SystemControlForegroundBaseHighBrush}'"
                L"                            PlaceholderBackground='{ThemeResource ListViewItemPlaceholderBackgroundThemeBrush}'"
                L"                            PointerOverBackground='{ThemeResource SystemControlForegroundListLowBrush}'"
                L"                            PointerOverForeground='Blue'"
                L"                            SelectedBackground='{ThemeResource SystemControlHighlightAltListAccentLowBrush}'"
                L"                            SelectedForeground='Red'"
                L"                            SelectedPointerOverBackground='{ThemeResource SystemControlHighlightAltListAccentMediumBrush}'"
                L"                            PressedBackground='{ThemeResource SystemControlForegroundListMediumBrush}'"
                L"                            SelectedPressedBackground='{ThemeResource SystemControlHighlightAltListAccentHighBrush}'"
                L"                            DisabledOpacity='{ThemeResource ListViewItemDisabledThemeOpacity}'"
                L"                            DragOpacity='{ThemeResource ListViewItemDragThemeOpacity}'"
                L"                            ReorderHintOffset='{ThemeResource ListViewItemReorderHintThemeOffset}'"
                L"                            HorizontalContentAlignment='{TemplateBinding HorizontalContentAlignment}'"
                L"                            VerticalContentAlignment='{TemplateBinding VerticalContentAlignment}'"
                L"                            ContentMargin='{TemplateBinding Padding}'"
                L"                            CheckMode='Inline' />"
                L"                    </ControlTemplate>"
                L"                </Setter.Value>"
                L"            </Setter>"
                L"        </Style>"
                L"    </Grid.Resources>"
                L"    <StackPanel>"
                L"        <ListViewItem x:Name='pointerOver' Content='PointerOver' />"
                L"        <ListViewItem x:Name='selected' Content='Selected' />"
                L"    </StackPanel>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            pointerOver = safe_cast<xaml_controls::ListViewItem^>(rootPanel->FindName(L"pointerOver"));
            VERIFY_IS_NOT_NULL(pointerOver);

            selected = safe_cast<xaml_controls::ListViewItem^>(rootPanel->FindName(L"selected"));
            VERIFY_IS_NOT_NULL(selected);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VisualStateManager::GoToState(pointerOver, "PointerOver", false);

            VisualStateManager::GoToState(selected, "Selected", false);
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void IntegrationTests::ValidateChangeCheckMode()
    {
        // MockDComp should be injected and detached per test, since it keeps information like the surfaces
        // that get created.
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        // Set a consistent window size and zoom scale, so that the DComp tree is predictable.
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400.0f, 600.0f));

        xaml_controls::ListViewItem^ selected = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <ListViewItem x:Name='selected' Content='Selected' Width='200' Height='100' />"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            selected = safe_cast<xaml_controls::ListViewItem^>(rootPanel->FindName(L"selected"));
            VERIFY_IS_NOT_NULL(selected);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VisualStateManager::GoToState(selected, "Selected", false);
            VisualStateManager::GoToState(selected, "MultiSelectEnabled", false);
        });
        TestServices::WindowHelper->WaitForIdle();

        // change the CheckMode from inline (default for ListViewItem) to overlay
        RunOnUIThread([&]()
        {
            xaml_primitives::ListViewItemPresenter^ lvip = static_cast<xaml_primitives::ListViewItemPresenter^>(xaml_media::VisualTreeHelper::GetChild(selected, 0));

            lvip->CheckMode = xaml_primitives::ListViewItemPresenterCheckMode::Overlay;
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void IntegrationTests::CanUseGridViewItemPresenter()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ListViewItem^ pointerOver = nullptr;
        xaml_controls::ListViewItem^ selected = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <Grid.Resources>"
                L"        <Style TargetType='ListViewItem'>"
                L"            <Setter Property = 'FontFamily' Value = '{ThemeResource ContentControlThemeFontFamily}' />"
                L"            <Setter Property = 'FontSize' Value = '{ThemeResource ControlContentThemeFontSize}' />"
                L"            <Setter Property='Background' Value='Transparent' />"
                L"            <Setter Property='TabNavigation' Value='Local' />"
                L"            <Setter Property='IsHoldingEnabled' Value='True' />"
                L"            <Setter Property='Padding' Value='12,0,12,0' />"
                L"            <Setter Property='HorizontalContentAlignment' Value='Left' />"
                L"            <Setter Property='VerticalContentAlignment' Value='Center' />"
                L"            <Setter Property='MinWidth' Value='{ThemeResource ListViewItemMinWidth}' />"
                L"            <Setter Property='MinHeight' Value='{ThemeResource ListViewItemMinHeight}' />"
                L"            <Setter Property='Template'>"
                L"                <Setter.Value>"
                L"                    <ControlTemplate TargetType='ListViewItem'>"
                L"                        <GridViewItemPresenter"
                L"                            ContentTransitions='{TemplateBinding ContentTransitions}'"
                L"                            SelectionCheckMarkVisualEnabled='True'"
                L"                            CheckBrush='{ThemeResource SystemControlBackgroundBaseMediumHighBrush}'"
                L"                            DragBackground='{ThemeResource ListViewItemDragBackgroundThemeBrush}'"
                L"                            DragForeground='{ThemeResource ListViewItemDragForegroundThemeBrush}'"
                L"                            FocusBorderBrush='{ThemeResource SystemControlForegroundAltHighBrush}'"
                L"                            PlaceholderBackground='{ThemeResource ListViewItemPlaceholderBackgroundThemeBrush}'"
                L"                            PointerOverBackground='{ThemeResource SystemControlForegroundListLowBrush}'"
                L"                            SelectedBackground='{ThemeResource SystemControlHighlightAltListAccentLowBrush}'"
                L"                            SelectedForeground='Red'"
                L"                            SelectedPointerOverBackground='{ThemeResource SystemControlHighlightAltListAccentMediumBrush}'"
                L"                            DisabledOpacity='{ThemeResource ListViewItemDisabledThemeOpacity}'"
                L"                            DragOpacity='{ThemeResource ListViewItemDragThemeOpacity}'"
                L"                            ReorderHintOffset='{ThemeResource ListViewItemReorderHintThemeOffset}'"
                L"                            HorizontalContentAlignment='{TemplateBinding HorizontalContentAlignment}'"
                L"                            VerticalContentAlignment='{TemplateBinding VerticalContentAlignment}'"
                L"                            ContentMargin='{TemplateBinding Padding}' />"
                L"                    </ControlTemplate>"
                L"                </Setter.Value>"
                L"            </Setter>"
                L"        </Style>"
                L"    </Grid.Resources>"
                L"    <StackPanel>"
                L"        <ListViewItem x:Name='pointerOver' Content='PointerOver' />"
                L"        <ListViewItem x:Name='selected' Content='Selected' />"
                L"    </StackPanel>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            pointerOver = safe_cast<xaml_controls::ListViewItem^>(rootPanel->FindName(L"pointerOver"));
            VERIFY_IS_NOT_NULL(pointerOver);

            selected = safe_cast<xaml_controls::ListViewItem^>(rootPanel->FindName(L"selected"));
            VERIFY_IS_NOT_NULL(selected);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VisualStateManager::GoToState(pointerOver, "PointerOver", false);

            VisualStateManager::GoToState(selected, "Selected", false);
        });
        TestServices::WindowHelper->WaitForIdle();

        // no crash
    }

    void IntegrationTests::VerifyLVIPHasSameSizeAsLVI()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ListViewItem^ listViewItem = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <ListViewItem x:Name='listViewItem' Width='88' Content='Item #1'/>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listViewItem = safe_cast<xaml_controls::ListViewItem^>(rootPanel->FindName(L"listViewItem"));
            VERIFY_IS_NOT_NULL(listViewItem);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            xaml_primitives::ListViewItemPresenter^ lvip = static_cast<xaml_primitives::ListViewItemPresenter^>(xaml_media::VisualTreeHelper::GetChild(listViewItem, 0));

            VERIFY_ARE_EQUAL(lvip->ActualWidth, listViewItem->ActualWidth);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void IntegrationTests::VerifyListViewItemContentTemplateRoot()
    {
        TestCleanupWrapper cleanup;

        VerifyListViewItemContentTemplateRoot(true /*enableRoundedListViewBaseItemChrome*/);
    }

    void IntegrationTests::VerifyListViewItemContentTemplateRootWithOldStyles()
    {
        TestCleanupWrapper cleanup;

        VerifyListViewItemContentTemplateRoot(false /*enableRoundedListViewBaseItemChrome*/);
    }

    void IntegrationTests::VerifyListViewItemContentTemplateRoot(bool enableRoundedListViewBaseItemChrome)
    {
        RoundedChromeDictionaryHelper dictionaryHelper(enableRoundedListViewBaseItemChrome);

        xaml_controls::ListViewItem^ listViewItem1 = nullptr;
        xaml_controls::ListViewItem^ listViewItem2 = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <ListViewItem x:Name='listViewItem1' Content='Item #1'/>"
                L"    <ListViewItem x:Name='listViewItem2'>"
                L"        <Button x:Name='btnLVI2' Content='Item #2 Button'/>"
                L"    </ListViewItem>"
                L"</StackPanel>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listViewItem1 = safe_cast<xaml_controls::ListViewItem^>(rootPanel->FindName(L"listViewItem1"));
            VERIFY_IS_NOT_NULL(listViewItem1);

            listViewItem2 = safe_cast<xaml_controls::ListViewItem^>(rootPanel->FindName(L"listViewItem2"));
            VERIFY_IS_NOT_NULL(listViewItem2);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto listViewItemPresenter1 = static_cast<xaml_primitives::ListViewItemPresenter^>(xaml_media::VisualTreeHelper::GetChild(listViewItem1, 0));
            VERIFY_IS_NOT_NULL(listViewItemPresenter1);
            auto listViewItemContentTemplateRoot1 = listViewItem1->ContentTemplateRoot;
            VERIFY_IS_NOT_NULL(listViewItemContentTemplateRoot1);
            auto textBlock = static_cast<xaml_controls::TextBlock^>(listViewItemContentTemplateRoot1);
            VERIFY_IS_NOT_NULL(textBlock);
            auto listViewItemPresenterContent1 = listViewItemPresenter1->Content;
            VERIFY_IS_NOT_NULL(listViewItemPresenterContent1);
            auto listViewItemPresenterContentAsString1 = dynamic_cast<Platform::String^>(listViewItemPresenterContent1);
            VERIFY_ARE_EQUAL(listViewItemPresenterContentAsString1, textBlock->Text);

            auto listViewItemPresenter2 = static_cast<xaml_primitives::ListViewItemPresenter^>(xaml_media::VisualTreeHelper::GetChild(listViewItem2, 0));
            VERIFY_IS_NOT_NULL(listViewItemPresenter2);
            auto listViewItemContentTemplateRoot2 = listViewItem2->ContentTemplateRoot;
            VERIFY_IS_NOT_NULL(listViewItemContentTemplateRoot2);
            auto button = static_cast<xaml_controls::Button^>(listViewItemContentTemplateRoot2);
            VERIFY_IS_NOT_NULL(button);
            auto listViewItemPresenterContent2 = listViewItemPresenter2->Content;
            VERIFY_IS_NOT_NULL(listViewItemPresenterContent2);
            VERIFY_ARE_EQUAL(static_cast<xaml_controls::Button^>(listViewItemPresenterContent2), button);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void IntegrationTests::VerifyLVIFocusRect()
    {
        // MockDComp should be injected and detached per test, since it keeps information like the surfaces
        // that get created.
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        // Set a consistent window size and zoom scale, so that the DComp tree is predictable.
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400.0f, 600.0f));

        xaml_controls::ListViewItem^ listViewItem = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <ListViewItem x:Name='listViewItem' Width='88' Height='44' Content='Something' />"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listViewItem = safe_cast<xaml_controls::ListViewItem^>(rootPanel->FindName(L"listViewItem"));
            VERIFY_IS_NOT_NULL(listViewItem);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VisualStateManager::GoToState(listViewItem, "Focused", false);
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void IntegrationTests::VerifyNoPressedStateWhenPanning()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ListView^ listView = nullptr;
        xaml_controls::ListViewItem^ listViewItem = nullptr;

        xaml::VisualStateGroup^ commonStatesVisualStateGroup;
        auto stateChangedEvent = std::make_shared<Event>();
        auto stateChangedRegistration = CreateSafeEventRegistration(xaml::VisualStateGroup, CurrentStateChanged);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <ListView x:Name='listView' Width='200' Height='300' HorizontalAlignment='Center' VerticalAlignment='Center' ItemContainerStyle='{ThemeResource ListViewItemExpanded}'>"
                L"    </ListView>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            for (int i = 0; i < 20; ++i)
            {
                listView->Items->Append(i);
            }

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            listViewItem = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(0));
            VERIFY_IS_NOT_NULL(listViewItem);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Register for VisualStateGroup.CurrentStateChanged
            auto templateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(listViewItem, 0));
            auto groups = xaml::VisualStateManager::GetVisualStateGroups(templateRoot);

            VERIFY_IS_TRUE(groups->Size > 0);

            commonStatesVisualStateGroup = groups->GetAt(0);

            stateChangedRegistration.Attach(commonStatesVisualStateGroup, ref new xaml::VisualStateChangedEventHandler(
                [stateChangedEvent](Platform::Object^, xaml::VisualStateChangedEventArgs^)
            {
                stateChangedEvent->Set();
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Flick(listView, FlickDirection::North);
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_FALSE(stateChangedEvent->HasFired());
    }

    void IntegrationTests::VerifyPressedStateWhenTapping()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ListView^ listView = nullptr;
        xaml_controls::ListViewItem^ listViewItem = nullptr;

        xaml::VisualStateGroup^ commonStatesVisualStateGroup;
        auto stateChangedEvent = std::make_shared<Event>();
        auto stateChangedRegistration = CreateSafeEventRegistration(xaml::VisualStateGroup, CurrentStateChanged);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <ListView x:Name='listView' Width='200' Height='300' HorizontalAlignment='Center' VerticalAlignment='Center'"
                L"              ItemContainerStyle='{ThemeResource ListViewItemExpanded}' SelectionMode='Single'>"
                L"    </ListView>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            for (int i = 0; i < 20; ++i)
            {
                listView->Items->Append(i);
            }

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            listViewItem = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(0));
            VERIFY_IS_NOT_NULL(listViewItem);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Register for VisualStateGroup.CurrentStateChanged
            auto templateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(listViewItem, 0));
            auto groups = xaml::VisualStateManager::GetVisualStateGroups(templateRoot);

            VERIFY_IS_TRUE(groups->Size > 0);

            commonStatesVisualStateGroup = groups->GetAt(0);

            stateChangedRegistration.Attach(commonStatesVisualStateGroup, ref new xaml::VisualStateChangedEventHandler(
                [stateChangedEvent](Platform::Object^, xaml::VisualStateChangedEventArgs^ args)
            {
                stateChangedEvent->Set();
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Tap(listViewItem);
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_TRUE(stateChangedEvent->HasFired());
    }

    void IntegrationTests::VerifyChangeThemeInMultiSelect()
    {
        // MockDComp should be injected and detached per test, since it keeps information like the surfaces
        // that get created.
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        // Set a consistent window size and zoom scale, so that the DComp tree is predictable.
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400.0f, 600.0f));

        xaml_controls::StackPanel^ rootPanel = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <ListView Width='300' SelectionMode='Multiple' SelectedIndex='1'>"
                L"        <ListViewItem Content='ListViewItem'/>"
                L"        <ListViewItem Content='Selected ListViewItem'/>"
                L"    </ListView>"
                L"    <GridView Width='300' SelectionMode='Multiple' SelectedIndex='1'>"
                L"        <GridViewItem Content='GridViewItem'/>"
                L"        <GridViewItem Content='Selected GridViewItem'/>"
                L"    </GridView>"
                L"</StackPanel>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // change the theme
            rootPanel->RequestedTheme = xaml::ElementTheme::Light;
            rootPanel->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::White);
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void IntegrationTests::VerifyDragOverStateForLVI()
    {
        // Leak: DragOverState tests leaks ContentPresenter
       TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

        TestCleanupWrapper cleanup;

        const unsigned int itemsCount = 20;
        Microsoft::UI::Xaml::Controls::ListView^ listView = nullptr;
        auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <ListView x:Name='listView' Width='400' Height='400' CanReorderItems='True' AllowDrop='True'/>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            choosingItemContainerRegistration.Attach(listView,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
                [](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
            {
                auto lvi = ref new xaml_controls::ListViewItem();

                lvi->Style = dynamic_cast<xaml::Style^>(xaml_markup::XamlReader::Load(
                    L"<Style xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' TargetType='ListViewItem' BasedOn='{StaticResource ListViewItemExpanded}'>"
                    L"  <Setter Property='Template'>"
                    L"    <Setter.Value>"
                    L"      <ControlTemplate TargetType='ListViewItem'>"
                    L"        <Grid x:Name='ContentBorder'"
                    L"              Background='{TemplateBinding Background}'"
                    L"              BorderBrush='{TemplateBinding BorderBrush}'"
                    L"              BorderThickness='{TemplateBinding BorderThickness}'>"
                    L"          <VisualStateManager.VisualStateGroups>"
                    L"            <VisualStateGroup x:Name='DragStates'>"
                    L"              <VisualState x:Name='NotDragging'/>"
                    L"              <VisualState x:Name='Dragging'>"
                    L"                <Storyboard>"
                    L"                  <DoubleAnimation Storyboard.TargetName='ContentBorder'"
                    L"                                   Storyboard.TargetProperty='Opacity'"
                    L"                                   Duration='0'"
                    L"                                   To='{ThemeResource ListViewItemDragThemeOpacity}'/>"
                    L"                  <DragItemThemeAnimation TargetName='ContentBorder'/>"
                    L"                </Storyboard>"
                    L"              </VisualState>"
                    L"              <VisualState x:Name='DraggingTarget'>"
                    L"                <Storyboard>"
                    L"                  <DropTargetItemThemeAnimation TargetName='ContentBorder'/>"
                    L"                </Storyboard>"
                    L"              </VisualState>"
                    L"              <VisualState x:Name='DraggedPlaceholder'>"
                    L"                <Storyboard>"
                    L"                  <FadeOutThemeAnimation TargetName='ContentBorder'/>"
                    L"                </Storyboard>"
                    L"              </VisualState>"
                    L"              <VisualState x:Name='DragOver'>"
                    L"                <Storyboard>"
                    L"                  <DropTargetItemThemeAnimation TargetName='ContentBorder'/>"
                    L"                </Storyboard>"
                    L"              </VisualState>"
                    L"            </VisualStateGroup>"
                    L"          </VisualStateManager.VisualStateGroups>"
                    L"          <ContentPresenter x:Name='ContentPresenter'"
                    L"                            ContentTransitions='{TemplateBinding ContentTransitions}'"
                    L"                            ContentTemplate='{TemplateBinding ContentTemplate}'"
                    L"                            Content='{TemplateBinding Content}'"
                    L"                            HorizontalAlignment='{TemplateBinding HorizontalContentAlignment}'"
                    L"                            VerticalAlignment='{TemplateBinding VerticalContentAlignment}'"
                    L"                            Margin='{TemplateBinding Padding}'/>"
                    L"        </Grid>"
                    L"      </ControlTemplate>"
                    L"    </Setter.Value>"
                    L"  </Setter>"
                    L"</Style>"
                    ));

                // we will treat the first item as a folder
                if (args->ItemIndex == 0)
                {
                    lvi->AllowDrop = true;
                }

                args->ItemContainer = lvi;
                args->IsContainerPrepared = true;
            }));

            Platform::Collections::Vector<Platform::Object^>^ items = ref new Platform::Collections::Vector<Platform::Object^>(itemsCount);
            VERIFY_IS_NOT_NULL(items);

            int count = 0;
            for (auto i : items)
            {
                i = count++;
            }

            listView->ItemsSource = items;
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        VerifyDragOverState<xaml_controls::ListViewItem>(listView);
    }

    void IntegrationTests::VerifyDragOverStateForGVI()
    {
        TestCleanupWrapper cleanup;

        const unsigned int itemsCount = 20;
        Microsoft::UI::Xaml::Controls::GridView^ gridView = nullptr;
        auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <GridView x:Name='gridView' Width='400' Height='400' CanReorderItems='True' AllowDrop='True'/>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            gridView = safe_cast<xaml_controls::GridView^>(rootPanel->FindName(L"gridView"));
            VERIFY_IS_NOT_NULL(gridView);

            choosingItemContainerRegistration.Attach(gridView,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
                [](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
            {
                auto gvi = ref new xaml_controls::GridViewItem();

                gvi->Style = dynamic_cast<xaml::Style^>(xaml_markup::XamlReader::Load(
                    L"<Style xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' TargetType='GridViewItem'>"
                    L"  <Setter Property='Template'>"
                    L"    <Setter.Value>"
                    L"      <ControlTemplate TargetType='GridViewItem'>"
                    L"        <Grid x:Name='ContentBorder'"
                    L"              Background='{TemplateBinding Background}'"
                    L"              BorderBrush='{TemplateBinding BorderBrush}'"
                    L"              BorderThickness='{TemplateBinding BorderThickness}'>"
                    L"          <VisualStateManager.VisualStateGroups>"
                    L"            <VisualStateGroup x:Name='DragStates'>"
                    L"              <VisualState x:Name='NotDragging'/>"
                    L"              <VisualState x:Name='Dragging'>"
                    L"                <Storyboard>"
                    L"                  <DoubleAnimation Storyboard.TargetName='ContentBorder'"
                    L"                                   Storyboard.TargetProperty='Opacity'"
                    L"                                   Duration='0'"
                    L"                                   To='{ThemeResource ListViewItemDragThemeOpacity}'/>"
                    L"                  <DragItemThemeAnimation TargetName='ContentBorder'/>"
                    L"                </Storyboard>"
                    L"              </VisualState>"
                    L"              <VisualState x:Name='DraggingTarget'>"
                    L"                <Storyboard>"
                    L"                  <DropTargetItemThemeAnimation TargetName='ContentBorder'/>"
                    L"                </Storyboard>"
                    L"              </VisualState>"
                    L"              <VisualState x:Name='DraggedPlaceholder'>"
                    L"                <Storyboard>"
                    L"                  <FadeOutThemeAnimation TargetName='ContentBorder'/>"
                    L"                </Storyboard>"
                    L"              </VisualState>"
                    L"              <VisualState x:Name='DragOver'>"
                    L"                <Storyboard>"
                    L"                  <DropTargetItemThemeAnimation TargetName='ContentBorder'/>"
                    L"                </Storyboard>"
                    L"              </VisualState>"
                    L"            </VisualStateGroup>"
                    L"          </VisualStateManager.VisualStateGroups>"
                    L"          <ContentPresenter x:Name='ContentPresenter'"
                    L"                            ContentTransitions='{TemplateBinding ContentTransitions}'"
                    L"                            ContentTemplate='{TemplateBinding ContentTemplate}'"
                    L"                            Content='{TemplateBinding Content}'"
                    L"                            HorizontalAlignment='{TemplateBinding HorizontalContentAlignment}'"
                    L"                            VerticalAlignment='{TemplateBinding VerticalContentAlignment}'"
                    L"                            Margin='{TemplateBinding Padding}'/>"
                    L"        </Grid>"
                    L"      </ControlTemplate>"
                    L"    </Setter.Value>"
                    L"  </Setter>"
                    L"</Style>"
                    ));

                // we will treat the first item as a folder
                if (args->ItemIndex == 0)
                {
                    gvi->AllowDrop = true;
                }

                args->ItemContainer = gvi;
                args->IsContainerPrepared = true;
            }));

            Platform::Collections::Vector<Platform::Object^>^ items = ref new Platform::Collections::Vector<Platform::Object^>(itemsCount);
            VERIFY_IS_NOT_NULL(items);

            int count = 0;
            for (auto i : items)
            {
                i = count++;
            }

            gridView->ItemsSource = items;
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        VerifyDragOverState<xaml_controls::GridViewItem>(gridView);
    }

    void IntegrationTests::ValidateChromeAnimationCommandsLifetime()
    {
        TestCleanupWrapper cleanup;
        
        ValidateChromeAnimationCommandsLifetime(false /*forceSelectionIndicatorVisualEnabled*/);
    }

    void IntegrationTests::ValidateChromeAnimationCommandsLifetimeWithSelectionIndicator()
    {
        TestCleanupWrapper cleanup;

        ValidateChromeAnimationCommandsLifetime(true /*forceSelectionIndicatorVisualEnabled*/);
    }

    void IntegrationTests::ValidateChromeAnimationCommandsLifetime(bool forceSelectionIndicatorVisualEnabled)
    {
        RuntimeEnabledFeatureOverride featureSelectionIndicatorVisualEnabled;

        if (forceSelectionIndicatorVisualEnabled)
        {
            featureSelectionIndicatorVisualEnabled.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceSelectionIndicatorVisualEnabled, true);
        }

        xaml_controls::ListView^ listView = nullptr;
        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ListView, Loaded);

        RunOnUIThread([&]()
        {
            auto data = ref new Platform::Collections::Vector<Platform::String^>();
            data->Append("Item One");

            auto emptyItemTemplate = safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"</DataTemplate>"));

            listView = ref new xaml_controls::ListView();
            listView->ItemsSource = data;
            listView->VerticalAlignment = xaml::VerticalAlignment::Top;
            listView->ItemTemplate = emptyItemTemplate;

            loadedRegistration.Attach(listView, ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"Host loaded.");
                loadedEvent->Set();
            }));
            TestServices::WindowHelper->WindowContent = listView;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Tap(listView);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            listView = nullptr;
            TestServices::WindowHelper->WindowContent = nullptr;
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void IntegrationTests::VerifyLVIPWorksWithVSM()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ListViewItem^ listViewItem = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"   <ListViewItem x:Name='listViewItem'>"
                L"      <ListViewItem.Style>"
                L"        <Style TargetType='ListViewItem'>"
                L"            <Setter Property='Template'>"
                L"                <Setter.Value>"
                L"                    <ControlTemplate TargetType='ListViewItem'>"
                L"                        <ListViewItemPresenter x:Name='Root' Background='Blue'>"
                L"                            <VisualStateManager.VisualStateGroups>"
                L"                                <VisualStateGroup x:Name='CommonStates'>"
                L"                                    <VisualState x:Name='PointerOver'>"
                L"                                        <VisualState.Setters>"
                L"                                            <Setter Target='Root.Background' Value='Green' />"
                L"                                        </VisualState.Setters>"
                L"                                    </VisualState>"
                L"                                </VisualStateGroup>"
                L"                            </VisualStateManager.VisualStateGroups>"
                L"                        </ListViewItemPresenter>"
                L"                    </ControlTemplate>"
                L"                </Setter.Value>"
                L"            </Setter>"
                L"         </Style>"
                L"      </ListViewItem.Style>"
                L"  </ListViewItem>"
                L"</Grid>"));

            VERIFY_IS_NOT_NULL(rootPanel);

            listViewItem = safe_cast<xaml_controls::ListViewItem^>(rootPanel->FindName(L"listViewItem"));
            VERIFY_IS_NOT_NULL(listViewItem);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto lvip = safe_cast<xaml_primitives::ListViewItemPresenter^>(xaml_media::VisualTreeHelper::GetChild(listViewItem, 0));

            LOG_OUTPUT(L"Check that default background is Blue");
            xaml_media::SolidColorBrush^ scb = safe_cast<xaml_media::SolidColorBrush^>(lvip->Background);
            VERIFY_ARE_EQUAL(scb->Color, Microsoft::UI::Colors::Blue);

            VisualStateManager::GoToState(listViewItem, "PointerOver", false);

            LOG_OUTPUT(L"Check that after going to PointerOver VSM state we switched to Green.");
            scb = safe_cast<xaml_media::SolidColorBrush^>(lvip->Background);
            VERIFY_ARE_EQUAL(scb->Color, Microsoft::UI::Colors::Green);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void IntegrationTests::LoadXamlAndVerifyMockDCompOutput(Platform::String^ xaml)
    {
        // MockDComp should be injected and detached per test, since it keeps information like the surfaces
        // that get created.
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        // Set a consistent window size and zoom scale, so that the DComp tree is predictable.
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400.0f, 600.0f));

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(xaml));
            VERIFY_IS_NOT_NULL(rootPanel);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void IntegrationTests::ValidateLVIPRevealBackgroundOnTopOfContent()
    {
        LoadXamlAndVerifyMockDCompOutput(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
            L"    <Grid.Resources>"
            L"        <Style TargetType='ListViewItem'>"
            L"            <Setter Property = 'FontFamily' Value = '{ThemeResource ContentControlThemeFontFamily}' />"
            L"            <Setter Property = 'FontSize' Value = '{ThemeResource ControlContentThemeFontSize}' />"
            L"            <Setter Property='Background' Value='Transparent' />"
            L"            <Setter Property='TabNavigation' Value='Local' />"
            L"            <Setter Property='IsHoldingEnabled' Value='True' />"
            L"            <Setter Property='Padding' Value='12,0,12,0' />"
            L"            <Setter Property='HorizontalContentAlignment' Value='Left' />"
            L"            <Setter Property='VerticalContentAlignment' Value='Center' />"
            L"            <Setter Property='MinWidth' Value='{ThemeResource ListViewItemMinWidth}' />"
            L"            <Setter Property='MinHeight' Value='{ThemeResource ListViewItemMinHeight}' />"
            L"            <Setter Property='Template'>"
            L"                <Setter.Value>"
            L"                    <ControlTemplate TargetType='ListViewItem'>"
            L"                        <ListViewItemPresenter"
            L"                            ContentTransitions='{TemplateBinding ContentTransitions}'"
            L"                            SelectionCheckMarkVisualEnabled='True'"
            L"                            CheckBrush='{ThemeResource SystemControlBackgroundBaseMediumHighBrush}'"
            L"                            CheckBoxBrush='{ThemeResource SystemControlBackgroundBaseMediumHighBrush}'"
            L"                            DragBackground='{ThemeResource ListViewItemDragBackgroundThemeBrush}'"
            L"                            DragForeground='{ThemeResource ListViewItemDragForegroundThemeBrush}'"
            L"                            FocusBorderBrush='{ThemeResource SystemControlForegroundAltHighBrush}'"
            L"                            FocusSecondaryBorderBrush='{ThemeResource SystemControlForegroundBaseHighBrush}'"
            L"                            PlaceholderBackground='{ThemeResource ListViewItemPlaceholderBackgroundThemeBrush}'"
            L"                            PointerOverBackground='{ThemeResource SystemControlForegroundListLowBrush}'"
            L"                            SelectedBackground='{ThemeResource SystemControlHighlightAltListAccentLowBrush}'"
            L"                            SelectedForeground='{ThemeResource ListViewItemSelectedForegroundThemeBrush}'"
            L"                            SelectedPointerOverBackground='{ThemeResource SystemControlHighlightAltListAccentMediumBrush}'"
            L"                            PressedBackground='{ThemeResource SystemControlForegroundListMediumBrush}'"
            L"                            SelectedPressedBackground='{ThemeResource SystemControlHighlightAltListAccentHighBrush}'"
            L"                            DisabledOpacity='{ThemeResource ListViewItemDisabledThemeOpacity}'"
            L"                            DragOpacity='{ThemeResource ListViewItemDragThemeOpacity}'"
            L"                            ReorderHintOffset='{ThemeResource ListViewItemReorderHintThemeOffset}'"
            L"                            HorizontalContentAlignment='{TemplateBinding HorizontalContentAlignment}'"
            L"                            VerticalContentAlignment='{TemplateBinding VerticalContentAlignment}'"
            L"                            ContentMargin='{TemplateBinding Padding}'"
            L"                            CheckMode='Inline'"
            L"                            RevealBackground = '{ThemeResource ListViewItemRevealBackground}'"
            L"                            RevealBorderThickness = '{ThemeResource ListViewItemRevealBorderThemeThickness}'"
            L"                            RevealBorderBrush = '{ThemeResource ListViewItemRevealBorderBrush}'"
            L"                            RevealBackgroundShowsAboveContent = 'True' />"
            L"                    </ControlTemplate>"
            L"                </Setter.Value>"
            L"            </Setter>"
            L"        </Style>"
            L"    </Grid.Resources>"
            L"    <ListViewItem x:Name='pointerOver' Content='Item 1' />"
            L"</Grid>");
    }

    void IntegrationTests::ValidateLVIPRevealBackgroundBelowContent()
    {
        LoadXamlAndVerifyMockDCompOutput(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
            L"    <Grid.Resources>"
            L"        <Style TargetType='ListViewItem'>"
            L"            <Setter Property = 'FontFamily' Value = '{ThemeResource ContentControlThemeFontFamily}' />"
            L"            <Setter Property = 'FontSize' Value = '{ThemeResource ControlContentThemeFontSize}' />"
            L"            <Setter Property='Background' Value='Transparent' />"
            L"            <Setter Property='TabNavigation' Value='Local' />"
            L"            <Setter Property='IsHoldingEnabled' Value='True' />"
            L"            <Setter Property='Padding' Value='12,0,12,0' />"
            L"            <Setter Property='HorizontalContentAlignment' Value='Left' />"
            L"            <Setter Property='VerticalContentAlignment' Value='Center' />"
            L"            <Setter Property='MinWidth' Value='{ThemeResource ListViewItemMinWidth}' />"
            L"            <Setter Property='MinHeight' Value='{ThemeResource ListViewItemMinHeight}' />"
            L"            <Setter Property='Template'>"
            L"                <Setter.Value>"
            L"                    <ControlTemplate TargetType='ListViewItem'>"
            L"                        <ListViewItemPresenter"
            L"                            ContentTransitions='{TemplateBinding ContentTransitions}'"
            L"                            SelectionCheckMarkVisualEnabled='True'"
            L"                            CheckBrush='{ThemeResource SystemControlBackgroundBaseMediumHighBrush}'"
            L"                            CheckBoxBrush='{ThemeResource SystemControlBackgroundBaseMediumHighBrush}'"
            L"                            DragBackground='{ThemeResource ListViewItemDragBackgroundThemeBrush}'"
            L"                            DragForeground='{ThemeResource ListViewItemDragForegroundThemeBrush}'"
            L"                            FocusBorderBrush='{ThemeResource SystemControlForegroundAltHighBrush}'"
            L"                            FocusSecondaryBorderBrush='{ThemeResource SystemControlForegroundBaseHighBrush}'"
            L"                            PlaceholderBackground='{ThemeResource ListViewItemPlaceholderBackgroundThemeBrush}'"
            L"                            PointerOverBackground='{ThemeResource SystemControlForegroundListLowBrush}'"
            L"                            SelectedBackground='{ThemeResource SystemControlHighlightAltListAccentLowBrush}'"
            L"                            SelectedForeground='{ThemeResource ListViewItemSelectedForegroundThemeBrush}'"
            L"                            SelectedPointerOverBackground='{ThemeResource SystemControlHighlightAltListAccentMediumBrush}'"
            L"                            PressedBackground='{ThemeResource SystemControlForegroundListMediumBrush}'"
            L"                            SelectedPressedBackground='{ThemeResource SystemControlHighlightAltListAccentHighBrush}'"
            L"                            DisabledOpacity='{ThemeResource ListViewItemDisabledThemeOpacity}'"
            L"                            DragOpacity='{ThemeResource ListViewItemDragThemeOpacity}'"
            L"                            ReorderHintOffset='{ThemeResource ListViewItemReorderHintThemeOffset}'"
            L"                            HorizontalContentAlignment='{TemplateBinding HorizontalContentAlignment}'"
            L"                            VerticalContentAlignment='{TemplateBinding VerticalContentAlignment}'"
            L"                            ContentMargin='{TemplateBinding Padding}'"
            L"                            CheckMode='Inline'"
            L"                            RevealBackground = 'Yellow'"
            L"                            RevealBorderThickness = '2'"
            L"                            RevealBorderBrush = 'Blue'"
            L"                            RevealBackgroundShowsAboveContent = 'False' />"
            L"                    </ControlTemplate>"
            L"                </Setter.Value>"
            L"            </Setter>"
            L"        </Style>"
            L"    </Grid.Resources>"
            L"    <ListViewItem x:Name='pointerOver' Content='Item 1' />"
            L"</Grid>");
    }

    void IntegrationTests::ValidateListViewItemRevealBackgroundShowsAboveContentStyle()
    {
        LoadXamlAndVerifyMockDCompOutput(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
            L"    <Grid.Resources>"
            L"        <Style TargetType='ListViewItem' BasedOn='{StaticResource ListViewItemRevealBackgroundShowsAboveContentStyle}'>"
            L"        </Style>"
            L"    </Grid.Resources>"
            L"    <ListViewItem Content='Item 1' />"
            L"</Grid>");
    }

    void IntegrationTests::ValidateGridViewItemRevealBackgroundShowsAboveContentStyle()
    {
        LoadXamlAndVerifyMockDCompOutput(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
            L"    <Grid.Resources>"
            L"        <Style TargetType='GridViewItem' BasedOn='{StaticResource GridViewItemRevealBackgroundShowsAboveContentStyle}'>"
            L"        </Style>"
            L"    </Grid.Resources>"
            L"    <GridViewItem Content='Item 1' />"
            L"</Grid>");
    }

    void IntegrationTests::ValidateGridViewItemRevealBackgroundAndBorder()
    {
        Platform::String^ xaml =
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
            L"    <Grid.Resources>"
            L"        <SolidColorBrush x:Key='GridViewItemRevealBackground' Color='#FF00FF00'/>"
            L"        <SolidColorBrush x:Key='GridViewItemRevealBorderBrush' Color='#FF00FFFF'/>"
            L"        <Style TargetType='GridViewItem' BasedOn='{StaticResource GridViewItemRevealStyle}'>"
            L"        </Style>"
            L"    </Grid.Resources>"
            L"    <StackPanel>"
            L"        <GridViewItem x:Name='GVI1' Content='Item 1' />"
            L"        <GridViewItem x:Name='GVI2' Content='Item 2' IsEnabled='false'/>"
            L"    </StackPanel>"
            L"</Grid>";

        LoadXamlAndVerifyMockDCompOutput(xaml);
    }


    void IntegrationTests::ValidateGridViewItemRevealDisabledStates()
    {
        Platform::String^ xaml =
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
            L"    <Grid.Resources>"
            L"        <SolidColorBrush x:Key='GridViewItemRevealBackground' Color='#FF00FF00'/>"
            L"        <SolidColorBrush x:Key='GridViewItemRevealBorderBrush' Color='#FF00FFFF'/>"
            L"        <Style TargetType='GridViewItem' BasedOn='{StaticResource GridViewItemRevealStyle}'>"
            L"        </Style>"
            L"    </Grid.Resources>"
            L"    <StackPanel>"
            L"        <GridViewItem x:Name='GVI1' Content='Item 1' />"
            L"        <GridViewItem x:Name='GVI2' Content='Item 2' IsEnabled='false'/>"
            L"    </StackPanel>"
            L"</Grid>";

        auto validationRules = ref new Platform::String(DefaultUIElementTreeValidationRules);

        xaml_controls::Grid^ rootPanel = nullptr;
        RunOnUIThread([&]()
        {
            // Set a consistent window size and zoom scale, so that the DComp tree is predictable.
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400.0f, 600.0f));

            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(xaml));
            VERIFY_IS_NOT_NULL(rootPanel);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyUIElementTreeWithRulesInline("Default", validationRules);

        RunOnUIThread([&]()
        {
            auto gvi1 = dynamic_cast<xaml_controls::GridViewItem^>(rootPanel->FindName("GVI1"));
            auto gvi2 = dynamic_cast<xaml_controls::GridViewItem^>(rootPanel->FindName("GVI2"));
            VERIFY_IS_NOT_NULL(gvi1);
            VERIFY_IS_NOT_NULL(gvi2);

            gvi1->IsEnabled = false;
            gvi2->IsEnabled = true;
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyUIElementTreeWithRulesInline("EnabledChanged", validationRules);

        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    }

    void IntegrationTests::ValidateListViewItemFocusPropertyThemeChange()
    {
        Platform::String^ xaml =
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root' RequestedTheme='Dark'>"
            L"    <Grid.Resources>"
            L"      <ResourceDictionary>"
            L"        <Style TargetType='ListViewItem'>"
            L"          <Setter Property='FocusVisualPrimaryThickness' Value='10' />"
            L"          <Setter Property='FocusVisualSecondaryThickness' Value='5' />"
            L"        </Style>"
            L"        <ResourceDictionary.ThemeDictionaries>"
            L"          <ResourceDictionary x:Key='Light'>"
            L"             <SolidColorBrush x:Key='ListViewItemFocusVisualPrimaryBrush' Color='Red'/>"
            L"             <SolidColorBrush x:Key='ListViewItemFocusVisualSecondaryBrush' Color='Yellow'/>"
            L"          </ResourceDictionary>"
            L"          <ResourceDictionary x:Key='Dark'>"
            L"            <SolidColorBrush x:Key='ListViewItemFocusVisualPrimaryBrush' Color='Blue'/>"
            L"            <SolidColorBrush x:Key='ListViewItemFocusVisualSecondaryBrush' Color='Green'/>"
            L"          </ResourceDictionary>"
            L"        </ResourceDictionary.ThemeDictionaries>"
            L"      </ResourceDictionary>"
            L"    </Grid.Resources>"
            L"    <StackPanel>"
            L"        <ListViewItem x:Name='lvi' Content='Primary' FocusVisualPrimaryBrush='{ThemeResource ListViewItemFocusVisualPrimaryBrush}' FocusVisualSecondaryBrush='{ThemeResource ListViewItemFocusVisualSecondaryBrush}'/>"
            L"        <ListViewItem x:Name='lvi_default' Content='Primary2' />"
            L"    </StackPanel>"
            L"</Grid>";

        LOG_OUTPUT(L"Verify user specified ThemeResource");
        TestSelectorItemFocusThemeChanges(
            xaml,
            "lvi",
            Microsoft::UI::Colors::Red,
            Microsoft::UI::Colors::Yellow,
            Microsoft::UI::Colors::Blue,
            Microsoft::UI::Colors::Green);

        LOG_OUTPUT(L"Verify default ThemeResource");

        // Default FocusVisualPrimaryBrush refers to FocusStrokeColorOuterBrush, which is:
        // Light: #E4000000
        // Dark (default): #FFFFFFFF
        // Default FocusVisualSecondaryBrush refers to FocusStrokeColorInnerBrush, which is:
        // Light: #B3FFFFFF
        // Dark (default): #B3000000
        TestSelectorItemFocusThemeChanges(
            xaml,
            "lvi_default",
            Microsoft::UI::ColorHelper::FromArgb(0xE4, 0x00, 0x00, 0x00),
            Microsoft::UI::ColorHelper::FromArgb(0xB3, 0xFF, 0xFF, 0xFF),
            Microsoft::UI::ColorHelper::FromArgb(0xFF, 0xFF, 0xFF, 0xFF),
            Microsoft::UI::ColorHelper::FromArgb(0xB3, 0x00, 0x00, 0x00));
    }

    void IntegrationTests::ValidateGridViewItemFocusPropertyThemeChange()
    {
        Platform::String^ xaml =
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root' RequestedTheme='Dark'>"
            L"    <Grid.Resources>"
            L"      <ResourceDictionary>"
            L"        <Style TargetType='GridViewItem'>"
            L"          <Setter Property='FocusVisualPrimaryThickness' Value='10' />"
            L"          <Setter Property='FocusVisualSecondaryThickness' Value='5' />"
            L"        </Style>"
            L"        <ResourceDictionary.ThemeDictionaries>"
            L"          <ResourceDictionary x:Key='Light'>"
            L"             <SolidColorBrush x:Key='GridViewItemFocusVisualPrimaryBrush' Color='Red'/>"
            L"             <SolidColorBrush x:Key='GridViewItemFocusVisualSecondaryBrush' Color='Yellow'/>"
            L"          </ResourceDictionary>"
            L"          <ResourceDictionary x:Key='Dark'>"
            L"            <SolidColorBrush x:Key='GridViewItemFocusVisualPrimaryBrush' Color='Blue'/>"
            L"            <SolidColorBrush x:Key='GridViewItemFocusVisualSecondaryBrush' Color='Green'/>"
            L"          </ResourceDictionary>"
            L"        </ResourceDictionary.ThemeDictionaries>"
            L"      </ResourceDictionary>"
            L"    </Grid.Resources>"
            L"    <StackPanel>"
            L"        <GridViewItem x:Name='gvi' Content='Focused' FocusVisualPrimaryBrush='{ThemeResource GridViewItemFocusVisualPrimaryBrush}' FocusVisualSecondaryBrush='{ThemeResource GridViewItemFocusVisualSecondaryBrush}'/>"
            L"        <GridViewItem x:Name='gvi_default' Content='Focused' />"
            L"    </StackPanel>"
            L"</Grid>";

        LOG_OUTPUT(L"Verify user specified ThemeResource");
        TestSelectorItemFocusThemeChanges(
            xaml,
            "gvi",
            Microsoft::UI::Colors::Red,
            Microsoft::UI::Colors::Yellow,
            Microsoft::UI::Colors::Blue,
            Microsoft::UI::Colors::Green);

        LOG_OUTPUT(L"Verify default ThemeResource");

        // Default FocusVisualPrimaryBrush refers to FocusStrokeColorOuterBrush, which is:
        // Light: #E4000000
        // Dark (default): #FFFFFFFF
        // Default FocusVisualSecondaryBrush refers to FocusStrokeColorInnerBrush, which is:
        // Light: #B3FFFFFF
        // Dark (default): #B3000000
        TestSelectorItemFocusThemeChanges(
            xaml,
            "gvi_default",
            Microsoft::UI::ColorHelper::FromArgb(0xE4, 0x00, 0x00, 0x00),
            Microsoft::UI::ColorHelper::FromArgb(0xB3, 0xFF, 0xFF, 0xFF),
            Microsoft::UI::ColorHelper::FromArgb(0xFF, 0xFF, 0xFF, 0xFF),
            Microsoft::UI::ColorHelper::FromArgb(0xB3, 0x00, 0x00, 0x00));
    }

    // Validates ListView item reordering with keyboard in LTR/RTL flows, and horizontal/vertical layouts.
    void IntegrationTests::VerifyKeyboardReordering()
    {
        VerifyKeyboardReordering(true  /*horizontalPanel*/, false /*rightToLeft*/);
        VerifyKeyboardReordering(true  /*horizontalPanel*/, true  /*rightToLeft*/);
        VerifyKeyboardReordering(false /*horizontalPanel*/, false /*rightToLeft*/);
        VerifyKeyboardReordering(false /*horizontalPanel*/, true  /*rightToLeft*/);
    }

    void IntegrationTests::VerifyKeyboardReordering(bool horizontalPanel, bool rightToLeft)
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ListView^ listView = nullptr;
        xaml_controls::ListViewItem^ listViewItem = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  x:Name='root' FlowDirection='LeftToRight'>"
                L"  <ListView x:Name='listView' Width='400' Height='400' HorizontalAlignment='Left' VerticalAlignment='Top' CanDragItems='True' CanReorderItems='True' AllowDrop='True'/>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            if (rightToLeft)
            {
                rootPanel->FlowDirection = xaml::FlowDirection::RightToLeft;
            }

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            if (horizontalPanel)
            {
                xaml_controls::ItemsStackPanel^ itemsPanel = safe_cast<xaml_controls::ItemsStackPanel^>(listView->ItemsPanelRoot);
                VERIFY_IS_NOT_NULL(itemsPanel);

                itemsPanel->Orientation = xaml_controls::Orientation::Horizontal;
            }

            for (int i = 0; i < 5; ++i)
            {
                listView->Items->Append(i);
            }
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            listViewItem = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(2));
            VERIFY_IS_NOT_NULL(listViewItem);

            LOG_OUTPUT(L"Focusing middle item.");
            listViewItem->Focus(FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving it with Shift/Alt/Right.");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_alt#$d$_right#$u$_right#$u$_alt#$u$_shift");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // In Horizontal+RightToLeft mode, the moved item's index becomes 1, otherwise it becomes 3.
            VERIFY_ARE_EQUAL(listViewItem, safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex((horizontalPanel && rightToLeft) ? 1 : 3)));
        });

        LOG_OUTPUT(L"Moving it with Shift/Alt/Down.");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_alt#$d$_down#$u$_down#$u$_alt#$u$_shift");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // In Horizontal+RightToLeft mode, the moved item's index becomes 0, otherwise it becomes 4.
            VERIFY_ARE_EQUAL(listViewItem, safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex((horizontalPanel && rightToLeft) ? 0 : 4)));
        });

        LOG_OUTPUT(L"Moving it with Shift/Alt/Up.");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_alt#$d$_up#$u$_up#$u$_alt#$u$_shift");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // In Horizontal+RightToLeft mode, the moved item's index becomes 1, otherwise it becomes 3.
            VERIFY_ARE_EQUAL(listViewItem, safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex((horizontalPanel && rightToLeft) ? 1 : 3)));
        });

        LOG_OUTPUT(L"Moving it with Shift/Alt/Left.");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_alt#$d$_left#$u$_left#$u$_alt#$u$_shift");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // In all modes, the moved item's index becomes 2.
            VERIFY_ARE_EQUAL(listViewItem, safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(2)));
        });
    }

    //
    // Private Methods
    //

    void IntegrationTests::TestSelectorItemFocusThemeChanges(
            Platform::String^ xaml,
            Platform::String^ elementName,
            ::Windows::UI::Color lightPrimary,
            ::Windows::UI::Color lightSecondary,
            ::Windows::UI::Color darkPrimary,
            ::Windows::UI::Color darkSecondary)
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        xaml_controls::Primitives::SelectorItem^ item = nullptr;
        xaml_controls::Panel^ rootPanel = nullptr;
        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Panel^> (xaml_markup::XamlReader::Load(xaml));
            VERIFY_IS_NOT_NULL(rootPanel);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, elementName+ "_None");
        RunOnUIThread([&]()
        {
            item = dynamic_cast<xaml_controls::Primitives::SelectorItem^>(rootPanel->FindName(elementName));
            VERIFY_IS_NOT_NULL(item);

            VERIFY_ARE_EQUAL(darkPrimary, safe_cast<xaml_media::SolidColorBrush^>(item->FocusVisualPrimaryBrush)->Color);
            VERIFY_ARE_EQUAL(darkSecondary, safe_cast<xaml_media::SolidColorBrush^>(item->FocusVisualSecondaryBrush)->Color);
            item->Focus(FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, elementName+ "_Dark");

        RunOnUIThread([&]()
        {
            rootPanel->RequestedTheme = ElementTheme::Light;
            // FocusVisual properties don't update while element is focused. We need to remove
            // keyboard focus first
            item->Focus(FocusState::Pointer);
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            item->Focus(FocusState::Keyboard); // Refocus to get updates
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, elementName+ "_Light");

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(lightPrimary, safe_cast<xaml_media::SolidColorBrush^>(item->FocusVisualPrimaryBrush)->Color);
            VERIFY_ARE_EQUAL(lightSecondary, safe_cast<xaml_media::SolidColorBrush^>(item->FocusVisualSecondaryBrush)->Color);
        });
    }

    bool IntegrationTests::AreBuffersEqual(::Windows::Storage::Streams::IBuffer^ buffer1, ::Windows::Storage::Streams::IBuffer^ buffer2)
    {
        Platform::Array<unsigned char>^ dataBuffer1 = GetRenderDataBuffer(buffer1);
        Platform::Array<unsigned char>^ dataBuffer2 = GetRenderDataBuffer(buffer2);

        if (buffer1->Length != buffer2->Length)
        {
            LOG_OUTPUT(L"Buffer lengths are not equal: chrome(%d), expanded(%d)", buffer1->Length, buffer2->Length);
            return false;
        }

        for (unsigned int i = 0; i < buffer1->Length; ++i)
        {
            if ((int)dataBuffer1->Data[i] != (int)dataBuffer2->Data[i])
            {
                LOG_OUTPUT(L"Different Color at buffer index %d: chrome(%d) expanded(%d)", i, (int)dataBuffer1->Data[i], (int)dataBuffer2->Data[i]);
                return false;
            }
        }

        return true;
    }

    Platform::Array<byte>^ IntegrationTests::GetRenderDataBuffer(::Windows::Storage::Streams::IBuffer^ buffer)
    {
        Platform::Array<unsigned char>^ dataBuffer = nullptr;

        RunOnUIThread([&]()
        {
            // get the bytes in the buffer
            ::Windows::Storage::Streams::DataReader^ dataReader = ::Windows::Storage::Streams::DataReader::FromBuffer(buffer);

            dataBuffer = ref new Platform::Array<unsigned char>(buffer->Length);
            dataReader->ReadBytes(dataBuffer);
        });
        TestServices::WindowHelper->WaitForIdle();

        return dataBuffer;
    }

    ::Windows::Storage::Streams::IBuffer^ IntegrationTests::GetRenderStreamBuffer()
    {
        auto renderedEvent = std::make_shared<Event>();
        auto getPixelsEvent = std::make_shared<Event>();

        Microsoft::UI::Xaml::Media::Imaging::RenderTargetBitmap^ renderTargetBitmap = nullptr;
        ::Windows::Storage::Streams::IBuffer^ buffer = nullptr;

        RunOnUIThread([&]()
        {
            renderTargetBitmap = ref new Microsoft::UI::Xaml::Media::Imaging::RenderTargetBitmap();

            // render
            auto renderAsyncAction = renderTargetBitmap->RenderAsync(TestServices::WindowHelper->WindowContent);

            auto renderCallback = ref new ::Windows::Foundation::AsyncActionCompletedHandler(
                [renderedEvent](::Windows::Foundation::IAsyncAction^ operation, ::Windows::Foundation::AsyncStatus)
            {
                renderedEvent->Set();
            });

            VERIFY_IS_NOT_NULL(renderCallback);
            renderAsyncAction->Completed = renderCallback;
        });

        renderedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // get the pixels
            auto getPixelsAsyncOperation = renderTargetBitmap->GetPixelsAsync();

            auto getPixelsCallback = ref new ::Windows::Foundation::AsyncOperationCompletedHandler<::Windows::Storage::Streams::IBuffer^>(
                [&buffer, getPixelsEvent](::Windows::Foundation::IAsyncOperation<::Windows::Storage::Streams::IBuffer^>^ operation, ::Windows::Foundation::AsyncStatus)
            {
                // get the buffer
                buffer = operation->GetResults();
                getPixelsEvent->Set();
            });

            VERIFY_IS_NOT_NULL(getPixelsCallback);
            getPixelsAsyncOperation->Completed = getPixelsCallback;
        });

        getPixelsEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        return buffer;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::ListViewBaseItem
