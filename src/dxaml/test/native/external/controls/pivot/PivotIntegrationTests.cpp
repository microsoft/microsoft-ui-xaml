// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "PivotIntegrationTests.h"

#include <Collection.h>
#include <TestCleanupWrapper.h>
#include <XamlTailored.h>
#include <StoryboardMonitorWrapper.h>

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <ControlHelper.h>
#include <TreeHelper.h>
#include <FileLoader.h>
#include <WUCRenderingScopeGuard.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace Microsoft::UI::Xaml::Input;
using namespace ::Windows::UI;

namespace Local {

    [wf::Metadata::WebHostHidden]
    public ref class SubClassedPivot sealed : public xaml_controls::Pivot
    {
    public:
        SubClassedPivot()
        {
        }

    }; // pubic ref class SubClassedPivot

} // namespace Local

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Pivot {

    bool PivotIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool PivotIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool PivotIntegrationTests::TestCleanup()
    {
        // Call below is here since there are two known tests which will not cleanup custom DPs in satellite DLLs.
        // 1. CanCreateSubClassedPivot - the metadata provider is never created at all, since the test is entirely
        //    in code-behind and news up the derived class and does not involve parser (which would load metadata providers).
        // 2. CanSetAndGetProperties - the metadata provider is created and cleaned up,
        //    but in XamlTypeInfoProvider::ResetDependencyProperties the Phone module isn't initialized so its DPs aren't reset.
        // EnsureSatelliteDLLCustomDPCleanup will cause force load and initialize all satellite DLLs right before MetadataAPI
        // is reset or destroyed at the end of test, which consequently will call into DLLs' ResetDependencyProperties and avoid
        // having stale custom DP references.
        // Please note that this method can also be called from individual tests to enable more granularity.  One requirement is
        // that it needs to be called before WindowHelper->ShutdownXaml().
        test_infra::TestServices::WindowHelper->EnsureSatelliteDLLCustomDPCleanup();

        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void PivotIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::Pivot>::CanInstantiate();
    }

    void PivotIntegrationTests::CanInstantiatePivotItem()
    {
        Generic::DependencyObjectTests<xaml_controls::PivotItem>::CanInstantiate();
    }

    void PivotIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::Pivot>::CanEnterAndLeaveLiveTree();
    }

    void PivotIntegrationTests::CanApplyIncompleteTemplate()
    {
        TestCleanupWrapper cleanup;

        auto pivot = SetupPivotTest(PivotContent::TextBlockContent);

        RunOnUIThread([&]()
        {
            auto pivotTemplate = safe_cast<xaml_controls::ControlTemplate^>(xaml_markup::XamlReader::Load(
                L"<ControlTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  TargetType='Pivot'>"
                L"  <Grid/>"
                L"</ControlTemplate>"
                ));

            pivot->Template = pivotTemplate;
            pivot->ApplyTemplate();
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void PivotIntegrationTests::CanReapplyTemplate()
    {
        TestCleanupWrapper cleanup;

        auto pivot = SetupPivotTest(PivotContent::TextBlockContent);

        RunOnUIThread([&]()
        {
            auto pivotTemplate = safe_cast<xaml_controls::ControlTemplate^>(xaml_markup::XamlReader::Load(
                LR"(<ControlTemplate
                    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                    TargetType='Pivot'>
                    <Grid
                        x:Name='RootElement'
                        HorizontalAlignment='{TemplateBinding HorizontalAlignment}'
                        VerticalAlignment='{TemplateBinding VerticalAlignment}'
                        Background='{TemplateBinding Background}'>
                        <Grid.RowDefinitions>
                            <RowDefinition Height='Auto' />
                            <RowDefinition Height='*' />
                        </Grid.RowDefinitions>
                        <ContentControl
                            x:Name='TitleContentControl'
                            IsTabStop='False'
                            Visibility='Collapsed'
                            Content='{TemplateBinding Title}'
                            ContentTemplate='{TemplateBinding TitleTemplate}'/>
                        <Grid Grid.Row='1'>
                            <ScrollViewer
                                x:Name='ScrollViewer'
                                Margin='{TemplateBinding Padding}'
                                HorizontalSnapPointsType='MandatorySingle'
                                HorizontalSnapPointsAlignment='Center'
                                HorizontalScrollBarVisibility='Hidden'
                                VerticalScrollMode='Disabled'
                                VerticalScrollBarVisibility='Disabled'
                                VerticalSnapPointsType='None'
                                VerticalContentAlignment='Stretch'
                                ZoomMode='Disabled'
                                Template='{StaticResource ScrollViewerScrollBarlessTemplate}'
                                BringIntoViewOnFocusChange='False'>
                                <PivotPanel x:Name='Panel' VerticalAlignment='Stretch'>
                                  <Grid x:Name='PivotLayoutElement'>
                                    <Grid.RowDefinitions>
                                      <RowDefinition Height='Auto' />
                                      <RowDefinition Height='*' />
                                    </Grid.RowDefinitions>
                                    <Grid.ColumnDefinitions>
                                      <ColumnDefinition Width='Auto' />
                                      <ColumnDefinition Width='*' />
                                      <ColumnDefinition Width='Auto' />
                                    </Grid.ColumnDefinitions>
                                    <Grid.RenderTransform>
                                      <CompositeTransform x:Name='PivotLayoutElementTranslateTransform' />
                                    </Grid.RenderTransform>
                                    <ContentPresenter
                                      x:Name='LeftHeaderPresenter'
                                      Content='{TemplateBinding LeftHeader}'
                                      ContentTemplate='{TemplateBinding LeftHeaderTemplate}'
                                      HorizontalAlignment='Stretch'
                                      VerticalAlignment='Stretch' />
                                    <ContentControl
                                      x:Name='HeaderClipper'
                                      Grid.Column='1'
                                      UseSystemFocusVisuals='True'
                                      HorizontalContentAlignment='Stretch'>
                                      <ContentControl.Clip>
                                        <RectangleGeometry x:Name='HeaderClipperGeometry' />
                                      </ContentControl.Clip>
                                      <Grid Background='Transparent'>
                                        <PivotHeaderPanel x:Name='StaticHeader' Visibility='Collapsed' />
                                        <PivotHeaderPanel x:Name='Header'>
                                          <PivotHeaderPanel.RenderTransform>
                                            <TransformGroup>
                                              <CompositeTransform x:Name='HeaderTranslateTransform' />
                                              <CompositeTransform x:Name='HeaderOffsetTranslateTransform' />
                                            </TransformGroup>
                                          </PivotHeaderPanel.RenderTransform>
                                        </PivotHeaderPanel>
                                      </Grid>
                                    </ContentControl>
                                    <Button
                                        x:Name='PreviousButton'
                                        Grid.Column='1'
                                        Width='20'
                                        Height='36'
                                        UseSystemFocusVisuals='False'
                                        Margin='{ThemeResource PivotNavButtonMargin}'
                                        IsTabStop='False'
                                        IsEnabled='False'
                                        HorizontalAlignment='Left'
                                        VerticalAlignment='Top'
                                        Opacity='0'
                                        Background='Transparent' />
                                    <Button
                                        x:Name='NextButton'
                                        Grid.Column='1'
                                        Width='20'
                                        Height='36'
                                        UseSystemFocusVisuals='False'
                                        Margin='{ThemeResource PivotNavButtonMargin}'
                                        IsTabStop='False'
                                        IsEnabled='False'
                                        HorizontalAlignment='Right'
                                        VerticalAlignment='Top'
                                        Opacity='0'
                                        Background='Transparent' />
                                    <ContentPresenter
                                        x:Name='RightHeaderPresenter'
                                        Grid.Column='2'
                                        Content='{TemplateBinding RightHeader}'
                                        ContentTemplate='{TemplateBinding RightHeaderTemplate}'
                                        HorizontalAlignment='Stretch'
                                        VerticalAlignment='Stretch' />
                                    <ItemsPresenter x:Name='PivotItemPresenter' Grid.Row='1' Grid.ColumnSpan='3'>
                                      <ItemsPresenter.RenderTransform>
                                        <TransformGroup>
                                          <TranslateTransform x:Name='ItemsPresenterTranslateTransform' />
                                          <CompositeTransform x:Name='ItemsPresenterCompositeTransform' />
                                        </TransformGroup>
                                      </ItemsPresenter.RenderTransform>
                                    </ItemsPresenter>
                                  </Grid>
                                </PivotPanel>
                            </ScrollViewer>
                        </Grid>
                    </Grid>
                </ControlTemplate>)"));

            pivot->Template = pivotTemplate;
            pivot->ApplyTemplate();
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void PivotIntegrationTests::ReleasePivotWhileUnloaded()
    {
        TestCleanupWrapper cleanup;

        auto pivot = SetupPivotTest(PivotContent::TextBlockContent);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Preparing the replacement of the Pivot in the visual tree.");
            xaml::Shapes::Rectangle^ rectangle = ref new xaml::Shapes::Rectangle();
            rectangle->Width = 100;
            rectangle->Height = 100;
            rectangle->Fill = ref new xaml_media::SolidColorBrush(Colors::Red);

            LOG_OUTPUT(L"Unloading the Pivot control.");
            TestServices::WindowHelper->WindowContent = rectangle;
        });

        LOG_OUTPUT(L"Waiting for idle.");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Selecting next Pivot item.");
            ++pivot->SelectedIndex;
        });

        LOG_OUTPUT(L"Waiting for idle.");
        TestServices::WindowHelper->WaitForIdle();
    }

    void PivotIntegrationTests::CanSelectNextItem()
    {
        TestCleanupWrapper cleanup;

        auto pivot = SetupPivotTest(PivotContent::TextBlockContent);

        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, SelectionChanged);

        RunOnUIThread([&]()
        {
            int currentSelectedIndex = pivot->SelectedIndex;
            VERIFY_IS_NOT_NULL(pivot->Items);
            pivot->SelectedItem = pivot->Items->GetAt(++currentSelectedIndex);
            pivot->SelectedIndex = currentSelectedIndex;

            VERIFY_ARE_EQUAL(pivot->SelectedItem, pivot->Items->GetAt(currentSelectedIndex));
            VERIFY_ARE_EQUAL(pivot->SelectedIndex, currentSelectedIndex);
        });
    }

    void PivotIntegrationTests::CanSelectItemDuringUnloading()
    {
        TestCleanupWrapper cleanup;

        int currentSelectedIndex = -1;
        xaml::Shapes::Rectangle^ rectangle = nullptr;
        auto pivot = SetupPivotTest(PivotContent::TextBoxContent);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Preparing the replacement of the Pivot in the visual tree.");
            rectangle = ref new xaml::Shapes::Rectangle();
            rectangle->Width = 100;
            rectangle->Height = 100;
            rectangle->Fill = ref new xaml_media::SolidColorBrush(Colors::Red);

            LOG_OUTPUT(L"Starting selection of the next Pivot item.");
            currentSelectedIndex = pivot->SelectedIndex;
            VERIFY_IS_NOT_NULL(pivot->Items);
            pivot->SelectedItem = pivot->Items->GetAt(++currentSelectedIndex);
            pivot->SelectedIndex = currentSelectedIndex;

            LOG_OUTPUT(L"Unloading the Pivot control.");
            TestServices::WindowHelper->WindowContent = rectangle;
        });

        LOG_OUTPUT(L"Waiting for idle.");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Verifying the selection change succeeded. Expected: %d", currentSelectedIndex);
            VERIFY_ARE_EQUAL(pivot->SelectedItem, pivot->Items->GetAt(currentSelectedIndex));
            VERIFY_ARE_EQUAL(pivot->SelectedIndex, currentSelectedIndex);

            LOG_OUTPUT(L"Changing selection again while the Pivot is unloaded.");
            pivot->SelectedItem = pivot->Items->GetAt(--currentSelectedIndex);
            pivot->SelectedIndex = currentSelectedIndex;
        });

        LOG_OUTPUT(L"Waiting for idle.");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Verifying the selection change succeeded. Expected: %d", currentSelectedIndex);
            VERIFY_ARE_EQUAL(pivot->SelectedItem, pivot->Items->GetAt(currentSelectedIndex));
            VERIFY_ARE_EQUAL(pivot->SelectedIndex, currentSelectedIndex);

            LOG_OUTPUT(L"Loading the Pivot control.");
            TestServices::WindowHelper->WindowContent = pivot;
        });

        LOG_OUTPUT(L"Waiting for idle.");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Unloading the Pivot control.");
            TestServices::WindowHelper->WindowContent = rectangle;

            LOG_OUTPUT(L"Changing selection again while the Pivot is unloading.");
            pivot->SelectedItem = pivot->Items->GetAt(++currentSelectedIndex);
            pivot->SelectedIndex = currentSelectedIndex;
        });

        LOG_OUTPUT(L"Waiting for idle.");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Verifying the selection change succeeded. Expected: %d", currentSelectedIndex);
            VERIFY_ARE_EQUAL(pivot->SelectedItem, pivot->Items->GetAt(currentSelectedIndex));
            VERIFY_ARE_EQUAL(pivot->SelectedIndex, currentSelectedIndex);

            LOG_OUTPUT(L"Reloading the Pivot control.");
            TestServices::WindowHelper->WindowContent = pivot;
        });

        LOG_OUTPUT(L"Waiting for idle.");
        TestServices::WindowHelper->WaitForIdle();
    }

    ref class DerivedPivotItem : Microsoft::UI::Xaml::Controls::PivotItem {  };
    void PivotIntegrationTests::CanInstantiateDerivedPivotItem()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            DerivedPivotItem^ myPivotItem = ref new DerivedPivotItem();
        });
    }

    void PivotIntegrationTests::CanDynamicallyAddItems()
    {
        TestCleanupWrapper cleanup;

        auto pivot = SetupPivotTest(PivotContent::TextBlockContent);

        RunOnUIThread([&]()
        {
            pivot->Items->InsertAt(0, ref new xaml_controls::PivotItem());
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(pivot->SelectedIndex, 1);

            pivot->Items->Append(ref new xaml_controls::PivotItem());
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(pivot->SelectedIndex, 1);
        });
    }

    void PivotIntegrationTests::CanDynamicallyChangeItems()
    {
        TestCleanupWrapper cleanup;

        auto pivot = SetupPivotTest(PivotContent::TextBlockContent);

        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, SelectionChanged);

        RunOnUIThread([&]()
        {
            selectionChangedRegistration.Attach(pivot, [selectionChangedEvent]() { selectionChangedEvent->Set(); });

            pivot->SelectedIndex = 1;
        });

        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            pivot->Items->SetAt(1, ref new xaml_controls::PivotItem());
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(pivot->SelectedIndex, 1);
        });
    }

    void PivotIntegrationTests::CanDynamicallyDeleteItems()
    {
        TestCleanupWrapper cleanup;

        auto pivot = SetupPivotTest(PivotContent::TextBlockContent);

        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, SelectionChanged);

        RunOnUIThread([&]()
        {
            selectionChangedRegistration.Attach(pivot, [selectionChangedEvent]() { selectionChangedEvent->Set(); });

            pivot->SelectedIndex = 1;
        });

        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            pivot->Items->RemoveAt(0);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(pivot->SelectedIndex, 0);

            pivot->Items->RemoveAt(0);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(pivot->SelectedIndex, 0);
        });
    }

    void PivotIntegrationTests::CanDynamicallyTransitionToAndFromHavingOneItem()
    {
        TestCleanupWrapper cleanup;

        auto pivot = SetupPivotTest(PivotContent::TextBlockContent);

        RunOnUIThread([&]()
        {
            pivot->Items->RemoveAt(2);
            pivot->Items->RemoveAt(1);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(pivot->SelectedIndex, 0);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            pivot->Items->InsertAt(0, ref new xaml_controls::PivotItem());
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(pivot->SelectedIndex, 1);
        });
    }

    void PivotIntegrationTests::CanSetAndGetProperties()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto pivot = ref new xaml_controls::Pivot();

            VERIFY_IS_FALSE(pivot->IsLocked);
            pivot->IsLocked = true;
            VERIFY_IS_TRUE(pivot->IsLocked);

            VERIFY_IS_NULL(pivot->HeaderTemplate);
            VERIFY_IS_NULL(pivot->ItemContainerStyle);
            VERIFY_IS_NULL(pivot->SelectedIndex);
            VERIFY_IS_NULL(pivot->SelectedItem);
            VERIFY_IS_NULL(pivot->Title);
            VERIFY_IS_NULL(pivot->TitleTemplate);
        });
    }

    void PivotIntegrationTests::CanPivotWithPaddleButtons()
    {
        // This test validates the behavior of the navigation buttons:
        // 1. We test both carousel and non-carousel modes
        // 2. We verify we can change selection forward / back by clicking the next / previous buttons
        // 3. We wrap in carousel mode
        // 4. We don't wrap in static mode. The next and previous buttons respectively disappear when we reach the end/start.
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(1000, 300));

        auto pivot = SetupPivotTest(PivotContent::TextBlockContent);
        xaml::FrameworkElement^ previousButton = nullptr;
        xaml::FrameworkElement^ nextButton = nullptr;
        int itemsCount = 0;

        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, SelectionChanged);

        selectionChangedRegistration.Attach(pivot,
            ref new xaml_controls::SelectionChangedEventHandler([selectionChangedEvent](Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ e) {
            selectionChangedEvent->Set();
        }));

        RunOnUIThread([&]()
        {
            previousButton = TreeHelper::GetVisualChildByName(pivot, L"PreviousButton");
            nextButton = TreeHelper::GetVisualChildByName(pivot, L"NextButton");
            VERIFY_IS_NOT_NULL(previousButton);
            VERIFY_IS_NOT_NULL(nextButton);

            LOG_OUTPUT(L"Set the pivot's width to a value such that not all headers will fit within its bounds.");
            pivot->Width = 200;
            itemsCount = static_cast<int>(pivot->Items->Size);
        });

        TestServices::WindowHelper->WaitForIdle();
        const auto boolValues = std::array<bool, 2> { true, false };
        for (bool useCarouselMode : boolValues)
        {
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Testing %s mode.", useCarouselMode ? L"carousel" : L"non-carousel");
                pivot->IsHeaderItemsCarouselEnabled = useCarouselMode;
            });

            TestServices::InputHelper->MoveMouse(nextButton);
            TestServices::WindowHelper->WaitForIdle();

            // Keep clicking on the right arrow to navigate until we get to the last item
            for (int i = 0; i < itemsCount - 1; ++i)
            {
                LOG_OUTPUT(L"Clicking on right arrow to change selection");
                TestServices::InputHelper->LeftMouseClick(nextButton);

                LOG_OUTPUT(L"Waiting for selection changed event from mouse click");
                selectionChangedEvent->WaitForDefault();

                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(i + 1, static_cast<int>(pivot->SelectedIndex));
                });
            }

            if (useCarouselMode)
            {
                LOG_OUTPUT(L"Test that we wrap in carousel mode.");
                TestServices::InputHelper->LeftMouseClick(nextButton);
                selectionChangedEvent->WaitForDefault();
                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(0, static_cast<int>(pivot->SelectedIndex));
                    pivot->SelectedIndex = itemsCount - 1;
                });
                selectionChangedEvent->WaitForDefault();
            }
            else
            {
                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Test that the next button is not shown in static mode because we are the end.");
                    VERIFY_ARE_EQUAL(0.0, nextButton->Opacity);
                    VERIFY_IS_FALSE(safe_cast<xaml_controls::Control^>(nextButton)->IsEnabled);
                });
            }

            TestServices::InputHelper->MoveMouse(previousButton);
            TestServices::WindowHelper->WaitForIdle();

            // Keep clicking on the left arrow to navigate until we get to the first item
            for (int i = 0; i < itemsCount - 1; ++i)
            {
                LOG_OUTPUT(L"Clicking on left arrow to change selection");
                TestServices::InputHelper->LeftMouseClick(previousButton);

                LOG_OUTPUT(L"Waiting for selection changed event from mouse click");
                selectionChangedEvent->WaitForDefault();

                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(itemsCount - 2 - i, static_cast<int>(pivot->SelectedIndex));
                });
            }

            if (useCarouselMode)
            {
                LOG_OUTPUT(L"Test that we wrap in carousel mode.");
                TestServices::InputHelper->LeftMouseClick(previousButton);
                selectionChangedEvent->WaitForDefault();
                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(itemsCount - 1, static_cast<int>(pivot->SelectedIndex));
                    pivot->SelectedIndex = 0;
                });
                selectionChangedEvent->WaitForDefault();
            }
            else
            {
                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Test that the previous button is not shown in static mode because we are the start.");
                    VERIFY_ARE_EQUAL(0.0, previousButton->Opacity);
                    VERIFY_IS_FALSE(safe_cast<xaml_controls::Control^>(previousButton)->IsEnabled);
                });
            }
        }
    }

    void PivotIntegrationTests::CanFallbackToNavigationButtonsVisibleIfStatesAreMissing()
    {
        TestCleanupWrapper cleanup;
        auto pivot = SetupPivotTest(PivotContent::TextBlockContent);
        xaml::FrameworkElement^ previousButton = nullptr;
        xaml::FrameworkElement^ nextButton = nullptr;
        int itemsCount = 0;

        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, SelectionChanged);
        auto stateChangedRegistration = CreateSafeEventRegistration(VisualStateGroup, CurrentStateChanged);
        std::queue<Platform::String^> expectedStates;
        {
            expectedStates.push(L"NavigationButtonsVisible");
            expectedStates.push(L"NavigationButtonsHidden");
            expectedStates.push(L"NavigationButtonsVisible");
        }

        selectionChangedRegistration.Attach(pivot,
            ref new xaml_controls::SelectionChangedEventHandler([selectionChangedEvent](Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ e) {
            selectionChangedEvent->Set();
        }));

        RunOnUIThread([&]()
        {
            previousButton = TreeHelper::GetVisualChildByName(pivot, L"PreviousButton");
            nextButton = TreeHelper::GetVisualChildByName(pivot, L"NextButton");
            VERIFY_IS_NOT_NULL(previousButton);
            VERIFY_IS_NOT_NULL(nextButton);

            LOG_OUTPUT(L"Set the pivot's width to a value such that not all headers will fit within its bounds.");
            pivot->Width = 200;
            pivot->IsHeaderItemsCarouselEnabled = false;
            itemsCount = static_cast<int>(pivot->Items->Size);

            LOG_OUTPUT(L"Remove the PreviousButtonVisible and NextButtonVisible states from the NavigationButtonsVisibility visual group.");
            auto groups = VisualStateManager::GetVisualStateGroups(safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(pivot, 0)));
            for (auto group : groups)
            {
                if (group->Name == L"NavigationButtonsVisibility")
                {
                    group->States->RemoveAt(2); // PreviousButtonVisible
                    group->States->RemoveAt(2); // NextButtonVisible

                    stateChangedRegistration.Attach(
                        group,
                        ref new Microsoft::UI::Xaml::VisualStateChangedEventHandler([&expectedStates](Platform::Object ^sender, Microsoft::UI::Xaml::VisualStateChangedEventArgs ^e) {
                        LOG_OUTPUT(L"Old state: %s", e->OldState->Name->Data());
                        LOG_OUTPUT(L"New state: %s", e->NewState->Name->Data());
                        VERIFY_ARE_EQUAL(expectedStates.front(), e->NewState->Name);
                        expectedStates.pop();
                    }));

                    break;
                }
            }
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Navigating to the last item with the next button.");
        TestServices::InputHelper->MoveMouse(nextButton);
        TestServices::WindowHelper->WaitForIdle();
        for (int i = 0; i < itemsCount - 1; ++i)
        {
            TestServices::InputHelper->LeftMouseClick(nextButton);
            selectionChangedEvent->WaitForDefault();
        }

        LOG_OUTPUT(L"Navigating to the fist item with the previous button.");
        TestServices::InputHelper->MoveMouse(previousButton);
        TestServices::WindowHelper->WaitForIdle();
        for (int i = 0; i < itemsCount - 1; ++i)
        {
            TestServices::InputHelper->LeftMouseClick(previousButton);
            selectionChangedEvent->WaitForDefault();
        }

        VERIFY_IS_TRUE(expectedStates.empty());
    }

    void PivotIntegrationTests::ValidateStaticHeadersStayInPlace()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(1000, 300));

        auto pivot = SetupPivotTest(PivotContent::TextBlockContent);
        xaml_primitives::PivotHeaderPanel^ staticHeader = nullptr;
        wf::Point originalSecondHeaderItemPosition(0, 0);

        RunOnUIThread([&]()
        {
            staticHeader = safe_cast<xaml_primitives::PivotHeaderPanel^>(TreeHelper::GetVisualChildByName(pivot, L"StaticHeader"));

            LOG_OUTPUT(L"Set the pivot's width to a value such that all headers will fit within its bounds.");
            pivot->Width = 1000;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"The first header item should be to the left of the second header item.");
            wf::Point firstHeaderItemPosition = staticHeader->Children->GetAt(0)->TransformToVisual(nullptr)->TransformPoint(wf::Point(0, 0));
            wf::Point secondHeaderItemPosition = staticHeader->Children->GetAt(1)->TransformToVisual(nullptr)->TransformPoint(wf::Point(0, 0));

            VERIFY_IS_LESS_THAN(firstHeaderItemPosition.X, secondHeaderItemPosition.X);

            LOG_OUTPUT(L"Now select the second header item.");
            pivot->SelectedIndex = 1;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"The first header item should still be to the left of the second header item.");
            wf::Point firstHeaderItemPosition = staticHeader->Children->GetAt(0)->TransformToVisual(nullptr)->TransformPoint(wf::Point(0, 0));
            wf::Point secondHeaderItemPosition = staticHeader->Children->GetAt(1)->TransformToVisual(nullptr)->TransformPoint(wf::Point(0, 0));

            VERIFY_IS_LESS_THAN(firstHeaderItemPosition.X, secondHeaderItemPosition.X);
        });
    }

    void PivotIntegrationTests::ValidateDynamicHeadersMove()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(1000, 300));

        auto pivot = SetupPivotTest(PivotContent::TextBlockContent);
        xaml_primitives::PivotHeaderPanel^ dynamicHeader = nullptr;

        RunOnUIThread([&]()
        {
            dynamicHeader = safe_cast<xaml_primitives::PivotHeaderPanel^>(TreeHelper::GetVisualChildByName(pivot, L"Header"));

            LOG_OUTPUT(L"Set the pivot's width to a value such that not all headers will fit within its bounds.");
            pivot->Width = 200;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"The first header item should be to the left of the second header item.");
            wf::Point firstHeaderItemPosition = dynamicHeader->Children->GetAt(0)->TransformToVisual(nullptr)->TransformPoint(wf::Point(0, 0));
            wf::Point secondHeaderItemPosition = dynamicHeader->Children->GetAt(1)->TransformToVisual(nullptr)->TransformPoint(wf::Point(0, 0));

            VERIFY_IS_LESS_THAN(firstHeaderItemPosition.X, secondHeaderItemPosition.X);

            LOG_OUTPUT(L"Now select the second header item.");
            pivot->SelectedIndex = 1;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"The first header item should now be to the right of the second header item.");
            wf::Point firstHeaderItemPosition = dynamicHeader->Children->GetAt(0)->TransformToVisual(nullptr)->TransformPoint(wf::Point(0, 0));
            wf::Point secondHeaderItemPosition = dynamicHeader->Children->GetAt(1)->TransformToVisual(nullptr)->TransformPoint(wf::Point(0, 0));

            VERIFY_IS_GREATER_THAN(firstHeaderItemPosition.X, secondHeaderItemPosition.X);
        });
    }

    void PivotIntegrationTests::CanSwitchBetweenStaticAndDynamicHeaders()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(1000, 300));

        auto pivot = SetupPivotTest(PivotContent::TextBlockContent);
        xaml::FrameworkElement^ dynamicHeader = nullptr;
        xaml::FrameworkElement^ staticHeader = nullptr;

        RunOnUIThread([&]()
        {
            dynamicHeader = TreeHelper::GetVisualChildByName(pivot, L"Header");
            staticHeader = TreeHelper::GetVisualChildByName(pivot, L"StaticHeader");

            LOG_OUTPUT(L"Set the pivot's width such that all of the headers will fit within the space provided.");
            pivot->Width = 1000;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"All of the headers presently fit within the space provided, so the static header should be visible.");
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, dynamicHeader->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, staticHeader->Visibility);

            LOG_OUTPUT(L"Now set the pivot's width such that they won't fit within the space provided anymore.");
            pivot->Width = 200;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Now the dynamic header should be visible.");
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, dynamicHeader->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, staticHeader->Visibility);

            LOG_OUTPUT(L"Now set the pivot's width back to where all headers will fit within its width.");
            pivot->Width = 1000;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Now the static header should be visible.");
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, dynamicHeader->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, staticHeader->Visibility);
        });
    }

    void PivotIntegrationTests::ValidateStaticHeaderPanelIsNotRequired()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(1000, 300));

        xaml_controls::Pivot^ pivot = nullptr;
        xaml_primitives::PivotHeaderPanel^ dynamicHeader = nullptr;

        auto pivotItemLoadedEvent = std::make_shared<Event>();
        auto pivotItemLoadingEvent = std::make_shared<Event>();
        auto finalViewChangedEvent = std::make_shared<Event>();

        auto pivotItemLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, PivotItemLoaded);
        auto pivotItemLoadingRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, PivotItemLoading);
        auto finalViewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        Platform::String^ xamlFile = GetPackageFolder() + L"resources\\native\\controls\\Pivot\\PivotWithoutStaticHeaders.xaml";
        pivot = safe_cast<xaml_controls::Pivot^>(LoadXamlFileOnUIThread(xamlFile));

        RunOnUIThread([&]()
        {
            pivotItemLoadedRegistration.Attach(pivot,
                ref new wf::TypedEventHandler<xaml_controls::Pivot^, xaml_controls::PivotItemEventArgs^>([pivotItemLoadedEvent](xaml_controls::Pivot^ sender, xaml_controls::PivotItemEventArgs^ args)
            {
                LOG_OUTPUT(L"PivotItemLoaded raised.");

                VERIFY_IS_NOT_NULL(args->Item);

                pivotItemLoadedEvent->Set();
            }));

            pivotItemLoadingRegistration.Attach(pivot,
                ref new wf::TypedEventHandler<xaml_controls::Pivot^, xaml_controls::PivotItemEventArgs^>([pivotItemLoadingEvent](xaml_controls::Pivot^ sender, xaml_controls::PivotItemEventArgs^ args)
            {
                LOG_OUTPUT(L"PivotItemLoading raised.");

                VERIFY_IS_NOT_NULL(args->Item);

                pivotItemLoadingEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = pivot;
        });

        pivotItemLoadedEvent->WaitForDefault();
        pivotItemLoadingEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            dynamicHeader = safe_cast<xaml_primitives::PivotHeaderPanel^>(TreeHelper::GetVisualChildByName(pivot, L"Header"));

            LOG_OUTPUT(L"Set the pivot's width to a value such that all headers will fit within its bounds.");
            pivot->Width = 1000;
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Now pan back and forth and verify that we're in dynamic header mode.");

        RunOnUIThread([&]()
        {
            auto scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(pivot, L"ScrollViewer"));

            finalViewChangedRegistration.Attach(
                scrollViewer,
                ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [&finalViewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args) {
                if (args->IsIntermediate == false)
                {
                    LOG_OUTPUT(L"finalViewChangedEvent->Set()");
                    finalViewChangedEvent->Set();
                }
            }));

            pivot->SelectedIndex = 1;
        });

        TestServices::WindowHelper->WaitForIdle();
        finalViewChangedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            wf::Point firstHeaderItemPosition = dynamicHeader->Children->GetAt(0)->TransformToVisual(nullptr)->TransformPoint(wf::Point(0, 0));
            wf::Point secondHeaderItemPosition = dynamicHeader->Children->GetAt(1)->TransformToVisual(nullptr)->TransformPoint(wf::Point(0, 0));

            VERIFY_IS_GREATER_THAN(firstHeaderItemPosition.X, secondHeaderItemPosition.X);

            pivot->SelectedIndex = 0;
        });

        TestServices::WindowHelper->WaitForIdle();
        finalViewChangedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            wf::Point firstHeaderItemPosition = dynamicHeader->Children->GetAt(0)->TransformToVisual(nullptr)->TransformPoint(wf::Point(0, 0));
            wf::Point secondHeaderItemPosition = dynamicHeader->Children->GetAt(1)->TransformToVisual(nullptr)->TransformPoint(wf::Point(0, 0));

            VERIFY_IS_LESS_THAN(firstHeaderItemPosition.X, secondHeaderItemPosition.X);
        });
    }

    void PivotIntegrationTests::ValidateUIElementTree()
    {
        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 800),
            1.f,
            []()
            {
                xaml_controls::StackPanel^ rootPanel = nullptr;
                xaml_controls::StackPanel^ verticalRootPanel1 = nullptr;
                xaml_controls::StackPanel^ verticalRootPanel2 = nullptr;

                xaml_controls::Pivot^ restItemsPivot = CreatePivot(PivotContent::TextBlockContent);
                xaml_controls::Pivot^ hoverItemsPivot = CreatePivot(PivotContent::TextBlockContent);
                xaml_controls::Pivot^ pressItemsPivot = CreatePivot(PivotContent::TextBlockContent);
                xaml_controls::Pivot^ focusedPivot = CreatePivot(PivotContent::TextBlockContent);

                xaml_controls::Pivot^ restNavButtonsPivot = CreatePivot(PivotContent::TextBlockContent);
                xaml_controls::Pivot^ hoverNavButtonsPivot = CreatePivot(PivotContent::TextBlockContent);
                xaml_controls::Pivot^ pressNavButtonsPivot = CreatePivot(PivotContent::TextBlockContent);

                xaml_controls::Pivot^ disabledPivot = CreatePivot(PivotContent::TextBlockContent);
                xaml_controls::Pivot^ lockedPivot = CreatePivot(PivotContent::TextBlockContent);
                xaml_controls::Pivot^ longTitlePivot = CreatePivot(PivotContent::TextBlockContent);

                RunOnUIThread([&]()
                {
                    rootPanel = ref new xaml_controls::StackPanel();
                    rootPanel->Orientation = xaml_controls::Orientation::Horizontal;
                    rootPanel->IsHitTestVisible = false;

                    verticalRootPanel1 = ref new xaml_controls::StackPanel();
                    verticalRootPanel1->Orientation = xaml_controls::Orientation::Vertical;
                    verticalRootPanel2 = ref new xaml_controls::StackPanel();
                    verticalRootPanel2->Orientation = xaml_controls::Orientation::Vertical;

                    lockedPivot->IsLocked = true;
                    longTitlePivot->Title = "Pivot Title, which is so long that it should be wrapping";
                    // Removing "Title" from some of the pivots so that they fit in one page.
                    restItemsPivot->Title = nullptr;
                    hoverItemsPivot->Title = nullptr;
                    pressItemsPivot->Title = nullptr;
                    restNavButtonsPivot->Title = nullptr;
                    hoverNavButtonsPivot->Title = nullptr;
                    pressNavButtonsPivot->Title = nullptr;
                    focusedPivot->Title = nullptr;

                    verticalRootPanel1->Children->Append(restItemsPivot);
                    verticalRootPanel1->Children->Append(hoverItemsPivot);
                    verticalRootPanel1->Children->Append(pressItemsPivot);
                    verticalRootPanel2->Children->Append(restNavButtonsPivot);
                    verticalRootPanel2->Children->Append(hoverNavButtonsPivot);
                    verticalRootPanel2->Children->Append(pressNavButtonsPivot);
                    verticalRootPanel1->Children->Append(disabledPivot);
                    verticalRootPanel1->Children->Append(lockedPivot);
                    verticalRootPanel2->Children->Append(longTitlePivot);
                    verticalRootPanel2->Children->Append(focusedPivot);

                    rootPanel->Children->Append(verticalRootPanel1);
                    rootPanel->Children->Append(verticalRootPanel2);

                    TestServices::WindowHelper->WindowContent = rootPanel;
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    xaml_primitives::PivotHeaderPanel^ hoverItemsPivotHeader = safe_cast<xaml_primitives::PivotHeaderPanel^>(TreeHelper::GetVisualChildByName(hoverItemsPivot, L"Header"));
                    VisualStateManager::GoToState(safe_cast<xaml_primitives::PivotHeaderItem^>(hoverItemsPivotHeader->Children->GetAt(0)), "SelectedPointerOver", false);
                    VisualStateManager::GoToState(safe_cast<xaml_primitives::PivotHeaderItem^>(hoverItemsPivotHeader->Children->GetAt(1)), "UnselectedPointerOver", false);

                    xaml_primitives::PivotHeaderPanel^ pressItemsPivotHeader = safe_cast<xaml_primitives::PivotHeaderPanel^>(TreeHelper::GetVisualChildByName(pressItemsPivot, L"Header"));
                    VisualStateManager::GoToState(safe_cast<xaml_primitives::PivotHeaderItem^>(pressItemsPivotHeader->Children->GetAt(0)), "SelectedPressed", false);
                    VisualStateManager::GoToState(safe_cast<xaml_primitives::PivotHeaderItem^>(pressItemsPivotHeader->Children->GetAt(1)), "UnselectedPressed", false);

                    VisualStateManager::GoToState(restNavButtonsPivot, "NavigationButtonsVisible", false);
                    VisualStateManager::GoToState(hoverNavButtonsPivot, "NavigationButtonsVisible", false);
                    VisualStateManager::GoToState(pressNavButtonsPivot, "NavigationButtonsVisible", false);

                    xaml_primitives::PivotHeaderPanel^ focusedItemPivotHeader = safe_cast<xaml_primitives::PivotHeaderPanel^>(TreeHelper::GetVisualChildByName(focusedPivot, L"Header"));
                    VisualStateManager::GoToState(safe_cast<xaml_primitives::PivotHeaderItem^>(focusedItemPivotHeader->Children->GetAt(0)), "Focused", false);
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    xaml_controls::Button^ hoverNavButtonsPivotPreviousButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(hoverNavButtonsPivot, "PreviousButton"));
                    xaml_controls::Button^ hoverNavButtonsPivotNextButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(hoverNavButtonsPivot, "NextButton"));
                    VisualStateManager::GoToState(hoverNavButtonsPivotPreviousButton, "PointerOver", true);
                    VisualStateManager::GoToState(hoverNavButtonsPivotNextButton, "PointerOver", true);

                    xaml_controls::Button^ pressNavButtonsPivotPreviousButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(pressNavButtonsPivot, "PreviousButton"));
                    xaml_controls::Button^ pressNavButtonsPivotNextButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(pressNavButtonsPivot, "NextButton"));
                    VisualStateManager::GoToState(pressNavButtonsPivotPreviousButton, "Pressed", true);
                    VisualStateManager::GoToState(pressNavButtonsPivotNextButton, "Pressed", true);

                    disabledPivot->IsEnabled = false;
                });
                TestServices::WindowHelper->WaitForIdle();

                return rootPanel;
            });
    }

    void PivotIntegrationTests::CanCreateSubClassedPivot()
    {
        RunOnUIThread([]()
        {
            auto pivotSubClass = ref new Local::SubClassedPivot();
        });
    }

    void PivotIntegrationTests::CanSetSelectedItemBeforeTemplateApplied()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto pivot = safe_cast<xaml_controls::Pivot^> (xaml_markup::XamlReader::Load(
                L"<Pivot xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Title='Test Pivot'>"
                L"  <PivotItem Header='Test PivotItem1'>"
                L"      <StackPanel Margin='20'>"
                L"          <TextBlock TextWrapping='Wrap' Text='Test SampleText for PivotItem1' />"
                L"      </StackPanel>"
                L"  </PivotItem>"
                L"  <PivotItem Header='Test PivotItem2'>"
                L"      <StackPanel Margin='20'>"
                L"          <TextBlock TextWrapping='Wrap' Text='Test SampleText for PivotItem2' />"
                L"      </StackPanel>"
                L"  </PivotItem>"
                L"  <PivotItem Header='Test PivotItem3'>"
                L"      <StackPanel Margin='20'>"
                L"          <TextBlock TextWrapping='Wrap' Text='Test SampleText for PivotItem3' />"
                L"      </StackPanel>"
                L"  </PivotItem>"
                L"</Pivot>"));

            pivot->SelectedItem = pivot->Items->GetAt(1);
        });
    }

    void PivotIntegrationTests::CanRetemplatePivot()
    {
        TestCleanupWrapper cleanup;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Panel, Loaded);

        auto pivotHost = safe_cast<xaml_controls::Grid^>(LoadXamlFileOnUIThread(GetPackageFolder() + L"resources\\native\\controls\\Pivot\\RetemplatedPivot.xaml"));

        RunOnUIThread([&]()
        {
            loadedRegistration.Attach(pivotHost,
                ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                loadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = pivotHost;
            pivotHost->UpdateLayout();
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto pivot = safe_cast<xaml_controls::Pivot^>(TreeHelper::GetVisualChildByName(pivotHost, L"RetemplatedPivot"));
            auto header = safe_cast<xaml_primitives::PivotHeaderPanel^>(TreeHelper::GetVisualChildByName(pivot, L"Header"));
            auto leftButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(pivot, L"LeftAreaButton"));
            auto rightButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(pivot, L"RightAreaButton"));

            const wf::Rect headerBounds = header->TransformToVisual(nullptr)->TransformBounds(wf::Rect{ 0.0f, 0.0f, static_cast<float>(header->ActualWidth), static_cast<float>(header->ActualHeight) });
            const wf::Rect firstChildBounds = header->Children->GetAt(0)->TransformToVisual(nullptr)->TransformBounds(wf::Rect{ 0.0f, 0.0f, static_cast<float>(header->ActualWidth), static_cast<float>(header->ActualHeight) });
            const wf::Rect leftButtonBounds = leftButton->TransformToVisual(nullptr)->TransformBounds(wf::Rect{ 0.0f, 0.0f, static_cast<float>(leftButton->ActualWidth), static_cast<float>(leftButton->ActualHeight) });
            const wf::Rect rightButtonBounds = rightButton->TransformToVisual(nullptr)->TransformBounds(wf::Rect{ 0.0f, 0.0f, static_cast<float>(rightButton->ActualWidth), static_cast<float>(rightButton->ActualHeight) });

            VERIFY_ARE_EQUAL(headerBounds.Height, leftButtonBounds.Height);
            VERIFY_ARE_EQUAL(headerBounds.Height, rightButtonBounds.Height);
            VERIFY_ARE_EQUAL(headerBounds.Height, firstChildBounds.Height);

            VERIFY_ARE_EQUAL(headerBounds.Y, leftButtonBounds.Y);
            VERIFY_ARE_EQUAL(headerBounds.Y, rightButtonBounds.Y);
            VERIFY_ARE_EQUAL(headerBounds.Y, firstChildBounds.Y);

            VERIFY_IS_LESS_THAN(leftButtonBounds.X, firstChildBounds.X);
            VERIFY_IS_LESS_THAN(firstChildBounds.X, rightButtonBounds.X);
        });
    }

    void PivotIntegrationTests::CanPresentLeftAndRightContentAreas()
    {
        TestCleanupWrapper cleanup;
        auto pivot = SetupPivotTest(PivotContent::TextBlockContent);

        RunOnUIThread([&]()
        {
            pivot->LeftHeader = L"Left Content";
            pivot->RightHeader = L"Right Content";
            pivot->LeftHeaderTemplate = safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='LeftButton' Content='{Binding}' VerticalAlignment='Stretch' />"
                L"</DataTemplate>"));
            pivot->RightHeaderTemplate = safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='RightButton' Content='{Binding}' VerticalAlignment='Stretch' />"
                L"</DataTemplate>"));

            // Enforce dynamic headers
            pivot->Width = 300.0;
            pivot->UpdateLayout();
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto header = safe_cast<xaml_primitives::PivotHeaderPanel^>(TreeHelper::GetVisualChildByName(pivot, L"Header"));
            auto leftContentPresenter = safe_cast<xaml_controls::ContentPresenter^>(TreeHelper::GetVisualChildByName(pivot, L"LeftHeaderPresenter"));
            auto rightContentPresenter = safe_cast<xaml_controls::ContentPresenter^>(TreeHelper::GetVisualChildByName(pivot, L"RightHeaderPresenter"));
            auto leftButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(leftContentPresenter, "LeftButton"));
            auto rightButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(rightContentPresenter, "RightButton"));

            VERIFY_ARE_EQUAL(ref new Platform::String(L"Left Content"), safe_cast<Platform::String^>(leftButton->Content));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"Right Content"), safe_cast<Platform::String^>(rightButton->Content));

            const wf::Rect headerBounds = header->TransformToVisual(nullptr)->TransformBounds(wf::Rect{ 0.0f, 0.0f, static_cast<float>(header->ActualWidth), static_cast<float>(header->ActualHeight) });
            const wf::Rect firstChildBounds = header->Children->GetAt(0)->TransformToVisual(nullptr)->TransformBounds(wf::Rect{ 0.0f, 0.0f, static_cast<float>(header->ActualWidth), static_cast<float>(header->ActualHeight) });
            const wf::Rect leftButtonBounds = leftButton->TransformToVisual(nullptr)->TransformBounds(wf::Rect{ 0.0f, 0.0f, static_cast<float>(leftButton->ActualWidth), static_cast<float>(leftButton->ActualHeight) });
            const wf::Rect rightButtonBounds = rightButton->TransformToVisual(nullptr)->TransformBounds(wf::Rect{ 0.0f, 0.0f, static_cast<float>(rightButton->ActualWidth), static_cast<float>(rightButton->ActualHeight) });

            VERIFY_ARE_EQUAL(headerBounds.Height, leftButtonBounds.Height);
            VERIFY_ARE_EQUAL(headerBounds.Height, rightButtonBounds.Height);
            VERIFY_ARE_EQUAL(headerBounds.Height, firstChildBounds.Height);

            VERIFY_ARE_EQUAL(headerBounds.Y, leftButtonBounds.Y);
            VERIFY_ARE_EQUAL(headerBounds.Y, rightButtonBounds.Y);
            VERIFY_ARE_EQUAL(headerBounds.Y, firstChildBounds.Y);

            VERIFY_IS_LESS_THAN(leftButtonBounds.X, firstChildBounds.X);
            VERIFY_IS_LESS_THAN(firstChildBounds.X, rightButtonBounds.X);
        });
    }

    void PivotIntegrationTests::CanFlickDiagonallyOnChildListViewWithoutMovingToNewItem()
    {
        TestCleanupWrapper cleanup;
        auto pivot = SetupPivotTest(PivotContent::ListViewContent);
        xaml_controls::ListView^ listView = nullptr;

        RunOnUIThread([&]()
        {
            listView = TreeHelper::GetVisualChildByType<xaml_controls::ListView>(pivot);
        });

        TestServices::InputHelper->Flick(listView, FlickDirection::NorthEast);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, pivot->SelectedIndex);
        });

        TestServices::InputHelper->Flick(listView, FlickDirection::SouthWest);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, pivot->SelectedIndex);
        });
    }

    void PivotIntegrationTests::CanFlickDiagonallyOnChildGridViewWithoutMovingToNewItem()
    {
        TestCleanupWrapper cleanup;
        auto pivot = SetupPivotTest(PivotContent::GridViewContent);
        xaml_controls::GridView^ gridView = nullptr;

        RunOnUIThread([&]()
        {
            gridView = TreeHelper::GetVisualChildByType<xaml_controls::GridView>(pivot);
        });

        TestServices::InputHelper->Flick(gridView, FlickDirection::NorthEast);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, pivot->SelectedIndex);
        });

        TestServices::InputHelper->Flick(gridView, FlickDirection::SouthWest);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, pivot->SelectedIndex);
        });
    }

    void PivotIntegrationTests::CanApplyPivotHeaderItemVerticalAlignment()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Pivot^ pivot;
        xaml_primitives::PivotHeaderPanel^ headersPanel;
        xaml_primitives::PivotHeaderItem ^headerItem1, ^headerItem2, ^headerItem3, ^headerItem4, ^headerItem5;

        RunOnUIThread([&]()
        {
            pivot = ref new xaml_controls::Pivot();
            for (int i = 0; i < 5; ++i)
            {
                auto item = ref new xaml_controls::PivotItem();
                item->Header = "Item #" + i.ToString();
                pivot->Items->Append(item);
            }

            TestServices::WindowHelper->WindowContent = pivot;
            pivot->Width = 1000;    // We want static headers.
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            headersPanel = safe_cast<xaml_primitives::PivotHeaderPanel^>(TreeHelper::GetVisualChildByName(pivot, "StaticHeader"));
            headerItem1 = safe_cast<xaml_primitives::PivotHeaderItem^>(headersPanel->Children->GetAt(0));
            headerItem2 = safe_cast<xaml_primitives::PivotHeaderItem^>(headersPanel->Children->GetAt(1));
            headerItem3 = safe_cast<xaml_primitives::PivotHeaderItem^>(headersPanel->Children->GetAt(2));
            headerItem4 = safe_cast<xaml_primitives::PivotHeaderItem^>(headersPanel->Children->GetAt(3));
            headerItem5 = safe_cast<xaml_primitives::PivotHeaderItem^>(headersPanel->Children->GetAt(4));

            // The first PivotHeaderItem will have a height of 100.
            // We will set the alignment on the next 4 to, respectively, top, center, bottom and stretch
            // and then validate their relative location. We also set their height to auto (NaN).
            // The crimson background is only there to be able for visual validation.
            headerItem1->Height = 100;
            headerItem2->VerticalAlignment = xaml::VerticalAlignment::Top;
            headerItem3->VerticalAlignment = xaml::VerticalAlignment::Center;
            headerItem4->VerticalAlignment = xaml::VerticalAlignment::Bottom;
            headerItem5->VerticalAlignment = xaml::VerticalAlignment::Stretch;

            // In order to more easily set the same properties on 4 header items, we'll temporarily put them all into an array that we can loop over.
            xaml_primitives::PivotHeaderItem^ headerItems[] = { headerItem2, headerItem3, headerItem4, headerItem5 };
            for (int i = 0; i < 4; ++i)
            {
                headerItems[i]->Height = std::numeric_limits<double>::quiet_NaN();
                headerItems[i]->Background = ref new xaml_media::SolidColorBrush(Colors::Crimson);
                headerItems[i] = nullptr;
            }
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Validate size.
            const double expectedHeight = static_cast<double>(headerItem2->DesiredSize.Height);
            VERIFY_ARE_EQUAL(expectedHeight, headerItem2->ActualHeight);
            VERIFY_ARE_EQUAL(expectedHeight, headerItem3->ActualHeight);
            VERIFY_ARE_EQUAL(expectedHeight, headerItem4->ActualHeight);
            VERIFY_ARE_EQUAL(100, headerItem5->ActualHeight);

            // Validate position
            const wf::Point emptyPoint = wf::Point(0.0f, 0.0f);
            VERIFY_ARE_EQUAL(0.0f, headerItem1->TransformToVisual(headersPanel)->TransformPoint(emptyPoint).Y);
            VERIFY_ARE_EQUAL((100.0f - expectedHeight) / 2.0f, headerItem3->TransformToVisual(headersPanel)->TransformPoint(emptyPoint).Y);
            VERIFY_ARE_EQUAL(100.0f - expectedHeight, headerItem4->TransformToVisual(headersPanel)->TransformPoint(emptyPoint).Y);
            VERIFY_ARE_EQUAL(0.0f, headerItem5->TransformToVisual(headersPanel)->TransformPoint(emptyPoint).Y);
        });
    }

    void PivotIntegrationTests::ValidatePivotItemsAreImmediatelyVisibleAfterSelectionChanged()
    {
        TestCleanupWrapper cleanup;
        auto pivot = SetupPivotTest(PivotContent::TextBoxExtraContent);

        RunOnUIThread([&]()
        {
            auto thirdItem = safe_cast<xaml_controls::PivotItem^>(pivot->Items->GetAt(2));
            auto thirdItemChild = TreeHelper::GetVisualChildByType<xaml_controls::Grid>(thirdItem);

            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, thirdItemChild->Visibility);

            // We want to simulate what Narrator does, which is setting SelectedItem instead of SelectedIndex.
            // Setting SelectedIndex doesn't result in Pivot::SetSelectedIndex() being called, which is what
            // results in the visibility being updated immediately.
            pivot->SelectedItem = thirdItem;

            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, thirdItemChild->Visibility);
        });
    }

    void PivotIntegrationTests::ValidateFocusIsMovedAfterSelectionChangedWhenPivotItemElementHasFocus()
    {
        TestCleanupWrapper cleanup;
        auto pivot = SetupPivotTest(PivotContent::TextBoxContent);

        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, SelectionChanged);

        RunOnUIThread([&]()
        {
            selectionChangedRegistration.Attach(pivot,
                ref new xaml_controls::SelectionChangedEventHandler([selectionChangedEvent](Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ e)
            {
                selectionChangedEvent->Set();
            }));

            auto firstItemTextBox = safe_cast<xaml_controls::TextBox^>(safe_cast<xaml_controls::PivotItem^>(pivot->Items->GetAt(0))->Content);
            firstItemTextBox->Focus(xaml::FocusState::Programmatic);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            pivot->SelectedIndex = 1;
        });

        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto secondItemTextBox = safe_cast<xaml_controls::TextBox^>(safe_cast<xaml_controls::PivotItem^>(pivot->Items->GetAt(1))->Content);
            auto focusedTextBox = safe_cast<xaml_controls::TextBox^>(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));

            VERIFY_ARE_EQUAL(secondItemTextBox, focusedTextBox);
        });

        RunOnUIThread([&]()
        {
            pivot->SelectedIndex = 2;
        });

        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto thirdItemTextBox = safe_cast<xaml_controls::TextBox^>(safe_cast<xaml_controls::PivotItem^>(pivot->Items->GetAt(2))->Content);
            auto focusedTextBox = safe_cast<xaml_controls::TextBox^>(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));

            VERIFY_ARE_EQUAL(thirdItemTextBox, focusedTextBox);
        });

        RunOnUIThread([&]()
        {
            pivot->SelectedIndex = 1;
        });

        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto secondItemTextBox = safe_cast<xaml_controls::TextBox^>(safe_cast<xaml_controls::PivotItem^>(pivot->Items->GetAt(1))->Content);
            auto focusedTextBox = safe_cast<xaml_controls::TextBox^>(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));

            VERIFY_ARE_EQUAL(secondItemTextBox, focusedTextBox);
        });

        RunOnUIThread([&]()
        {
            pivot->SelectedIndex = 0;
        });

        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto firstItemTextBox = safe_cast<xaml_controls::TextBox^>(safe_cast<xaml_controls::PivotItem^>(pivot->Items->GetAt(0))->Content);
            auto focusedTextBox = safe_cast<xaml_controls::TextBox^>(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));

            VERIFY_ARE_EQUAL(firstItemTextBox, focusedTextBox);
        });
    }

    void PivotIntegrationTests::NavigatingToPivotItemWithFocusableNonControlDoesNotCrash()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Pivot^ pivot = nullptr;
        auto pivotItemLoadedEvent = std::make_shared<Event>();
        auto pivotItemLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, PivotItemLoaded);
        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, SelectionChanged);
        auto gotFocusEvent = std::make_shared<Event>();
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

        RunOnUIThread([&]()
        {
            pivot = safe_cast<xaml_controls::Pivot^> (xaml_markup::XamlReader::Load(
                LR"(<Pivot xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Title='Test Pivot' Width='150'>
                      <PivotItem Header='1'>
                          <Button x:Name='button' Content='Test Button'/>
                      </PivotItem>
                      <PivotItem Header='2'>
                          <TextBlock x:Name='txbl'>
                              <Hyperlink x:Name='hyperlink'>Test Hyperlink</Hyperlink>
                          </TextBlock>
                      </PivotItem>
                </Pivot>)"));

            pivotItemLoadedRegistration.Attach(pivot, [pivotItemLoadedEvent]() { pivotItemLoadedEvent->Set(); });

            TestServices::WindowHelper->WindowContent = pivot;
        });

        pivotItemLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto firstItemButton = safe_cast<xaml_controls::Button^>(pivot->FindName(L"button"));
            selectionChangedRegistration.Attach(pivot, [selectionChangedEvent]() { selectionChangedEvent->Set(); });
            gotFocusRegistration.Attach(firstItemButton, [gotFocusEvent]() { gotFocusEvent->Set(); });
            firstItemButton->Focus(xaml::FocusState::Keyboard);
        });

        gotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            //Before RS4 this call would crash the test app
            pivot->SelectedIndex = 1;
        });

        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            //Grabbing the hyperlink directly by name doesn't work, as it is in the document tree of the TextBlock rather than the visual tree.
            auto hyperlink = safe_cast<Microsoft::UI::Xaml::Documents::Hyperlink^>(pivot->FindName(L"hyperlink"));
            VERIFY_IS_TRUE(FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(hyperlink));
            LOG_OUTPUT(L"Hyperlink has focus");
        });
    }

    void PivotIntegrationTests::ValidateContentTemplateRootExistsInPivotItemLoading()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Pivot^ pivot = nullptr;

        auto pivotItemLoadingEvent = std::make_shared<Event>();
        auto pivotItemLoadingRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, PivotItemLoading);

        RunOnUIThread([&]()
        {
            pivot = CreatePivot(PivotContent::TextBlockItemTemplateContent);

            pivotItemLoadingRegistration.Attach(pivot,
                ref new wf::TypedEventHandler<xaml_controls::Pivot^, xaml_controls::PivotItemEventArgs^>([pivotItemLoadingEvent](xaml_controls::Pivot^ sender, xaml_controls::PivotItemEventArgs^ args)
            {
                VERIFY_IS_NOT_NULL(args->Item);
                VERIFY_IS_NOT_NULL(args->Item->ContentTemplateRoot);

                pivotItemLoadingEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = pivot;
        });

        pivotItemLoadingEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void PivotIntegrationTests::CanProgrammaticallyAddPivotItemToEmptyCollection()
    {
        TestCleanupWrapper cleanup;
        auto pivot = SetupPivotTest(PivotContent::NoContent);

        RunOnUIThread([&]()
        {
            auto newPivotItem = ref new xaml_controls::PivotItem();
            pivot->Items->Append(newPivotItem);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void PivotIntegrationTests::ValidateTabOnlyStopsAtPivotAndContent()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::StackPanel^ rootPanel = nullptr;

        auto loadedEvent = std::make_shared<Event>();

        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, GotFocus);

        Platform::String^ xamlFile = GetPackageFolder() + L"resources\\native\\controls\\Pivot\\PivotForKeyboarding.xaml";
        rootPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(xamlFile));

        std::shared_ptr<Platform::String^> spFocusSequence = std::make_shared<Platform::String^>();

        Platform::String^ expectedFocusSequence1 = "[B0][T][LH][HeaderClipper][RH][B1][B4]";
        Platform::String^ expectedFocusSequence2 = "[B0][T][LH][HeaderClipper][RH][B3][B4]";

        RunOnUIThread([&]()
        {
            gotFocusRegistration.Attach(rootPanel, ref new xaml::RoutedEventHandler([spFocusSequence](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                Platform::String^ temp = *spFocusSequence;
                temp += "[" + safe_cast<xaml::FrameworkElement^>(args->OriginalSource)->Name + "]";
                *spFocusSequence = temp;
            }));

            loadedRegistration.Attach(rootPanel,
                ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                loadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        *spFocusSequence = "";

        LOG_OUTPUT(L"Tab 7 times to go to the first button, pivot title, left header, header items, right header, first pivot item button and then the last button.");

        //This block of code solves a test inconsistency, sometimes the framework puts tab focus on the first element in the tree when the test is loaded.
        //This checks to see if that is the case, if it is, then it is possible the spFocusSequence has already been updated with this if the gotFocus event handler
        //was attached before the test harness gave the element focus.  Also check for this.
        size_t tabCount = 7;
        RunOnUIThread([&]()
        {
            auto button0 = safe_cast<xaml_controls::Control^>(xaml_media::VisualTreeHelper::GetChild(rootPanel, 0));
            if (button0->FocusState != xaml::FocusState::Unfocused)
            {
                tabCount = 6;
                if ((*spFocusSequence)->Length() == 0)
                {
                    Platform::String^ temp = *spFocusSequence;
                    temp += "[B0]";
                    *spFocusSequence = temp;
                }
            }
        });

        TestServices::WindowHelper->WaitForIdle();

        for (size_t i = 0; i < tabCount; ++i)
        {
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
        }

        LOG_OUTPUT(L"Expected focus sequence: %s", expectedFocusSequence1->Data());
        LOG_OUTPUT(L"Actual focus sequence: %s", (*spFocusSequence)->Data());
        VERIFY_IS_TRUE(Platform::String::CompareOrdinal(expectedFocusSequence1, *spFocusSequence) == 0);

        LOG_OUTPUT(L"Change pivot items and verify that we still only have the pivot, the pivot's content, and outside the pivot as tab stops.");

        RunOnUIThread([&]()
        {
            xaml_controls::Pivot^ pivot = safe_cast<xaml_controls::Pivot^>(TreeHelper::GetVisualChildByName(rootPanel, L"P"));
            pivot->SelectedIndex = 2;
        });

        TestServices::WindowHelper->WaitForIdle();

        *spFocusSequence = "";

        RunOnUIThread([&]()
        {
            //On WindowsCore tab cycling is not present, since this test is not testing tab cycling we avoid this code path by setting focus on the top element.
            safe_cast<xaml_controls::Control^>(xaml_media::VisualTreeHelper::GetChild(rootPanel, 0))->Focus(xaml::FocusState::Programmatic);
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tab 6 times to go from the first button to the pivot title, left header, header items, right header, third pivot item button and then the last button.");

        for (size_t i = 0; i < 6; ++i)
        {
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
        }

        LOG_OUTPUT(L"Expected focus sequence: %s", expectedFocusSequence2->Data());
        LOG_OUTPUT(L"Actual focus sequence: %s", (*spFocusSequence)->Data());
        VERIFY_IS_TRUE(Platform::String::CompareOrdinal(expectedFocusSequence2, *spFocusSequence) == 0);
    }

    void PivotIntegrationTests::CanUseKeyboardKeysToChangeSelectedIndex()
    {
        CanUseKeyboardKeysToChangeSelectedIndexImpl(false /* shouldWrap */, false /* isRTL */);
    }

    void PivotIntegrationTests::CanUseKeyboardKeysToChangeSelectedIndexRightToLeft()
    {
        CanUseKeyboardKeysToChangeSelectedIndexImpl(false /* shouldWrap */, true /* isRTL */);
    }


    void PivotIntegrationTests::CanUseKeyboardKeysToChangeSelectedIndexImpl(bool shouldWrap, bool isRTL)
    {
        TestCleanupWrapper cleanup;
        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_controls::Pivot^ pivot = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

        Platform::String^ xamlFile = GetPackageFolder() + L"resources\\native\\controls\\Pivot\\PivotForKeyboarding.xaml";
        rootPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(xamlFile));

        RunOnUIThread([&]()
        {
            loadedRegistration.Attach(rootPanel,
                ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                loadedEvent->Set();
            }));

            if (isRTL)
            {
                rootPanel->FlowDirection = xaml::FlowDirection::RightToLeft;
            }

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Focus the pivot, then use the left and right arrow keys to navigate.");

        RunOnUIThread([&]()
        {
            pivot = safe_cast<xaml_controls::Pivot^>(TreeHelper::GetVisualChildByName(rootPanel, L"P"));
            safe_cast<xaml_controls::Control^>(TreeHelper::GetVisualChildByName(pivot, L"HeaderClipper"))->Focus(xaml::FocusState::Programmatic);
        });

        TestServices::WindowHelper->WaitForIdle();
        PressKeyboardLogicalRight(isRTL);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, pivot->SelectedIndex);
        });

        PressKeyboardLogicalRight(isRTL);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(2, pivot->SelectedIndex);
        });

        PressKeyboardLogicalLeft(isRTL);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, pivot->SelectedIndex);
        });

        PressKeyboardLogicalLeft(isRTL);
        TestServices::WindowHelper->WaitForIdle();
        PressKeyboardLogicalLeft(isRTL);
        TestServices::WindowHelper->WaitForIdle();

        if (shouldWrap)
        {
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(2, pivot->SelectedIndex);
            });

            PressKeyboardLogicalRight(isRTL);
            TestServices::WindowHelper->WaitForIdle();
        }

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, pivot->SelectedIndex);
        });

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_pagedown#$u$_pagedown#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, pivot->SelectedIndex);
        });

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_pagedown#$u$_pagedown#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(2, pivot->SelectedIndex);
        });

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_pageup#$u$_pageup#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, pivot->SelectedIndex);
        });

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_pageup#$u$_pageup#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_pageup#$u$_pageup#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(shouldWrap ? 2 : 0, pivot->SelectedIndex);
        });

        if (shouldWrap)
        {
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_pagedown#$u$_pagedown#$u$_ctrl");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, pivot->SelectedIndex);
            });
        }

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_end#$u$_end");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(2, pivot->SelectedIndex);
        });

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_home#$u$_home");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, pivot->SelectedIndex);
        });

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_tab#$u$_tab#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, pivot->SelectedIndex);
        });

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_tab#$u$_tab#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(2, pivot->SelectedIndex);
        });

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_shift#$d$_tab#$u$_tab#$u$_shift#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, pivot->SelectedIndex);
        });

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_shift#$d$_tab#$u$_tab#$u$_shift#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_shift#$d$_tab#$u$_tab#$u$_shift#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(shouldWrap ? 2 : 0, pivot->SelectedIndex);
        });
    }

    void PivotIntegrationTests::ValidateThatScrollViewerDoNotHandleInput()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_controls::Pivot^ pivot = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);
        auto finalViewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        Platform::String^ xamlFile = GetPackageFolder() + L"resources\\native\\controls\\Pivot\\PivotForKeyboarding.xaml";
        rootPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(xamlFile));
        unsigned viewChangedCounter = 0;

        RunOnUIThread([&]()
        {
            loadedRegistration.Attach(rootPanel,
                ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args) { loadedEvent->Set(); }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Focus the left header, then validate that left/right keyboard keys do nothing.");

        RunOnUIThread([&]()
        {
            pivot = safe_cast<xaml_controls::Pivot^>(TreeHelper::GetVisualChildByName(rootPanel, L"P"));
            safe_cast<xaml_controls::Control^>(TreeHelper::GetVisualChildByName(pivot, L"LH"))->Focus(FocusState::Keyboard);

            finalViewChangedRegistration.Attach(
                TreeHelper::GetVisualChildByType<xaml_controls::ScrollViewer>(pivot),
                ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [&](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args) { ++viewChangedCounter; }));
        });

        LOG_OUTPUT(L"Press Right/Left and validates the selected index doesn't change");

        TestServices::WindowHelper->WaitForIdle();
        TestServices::KeyboardHelper->Right();
        TestServices::WindowHelper->WaitForIdle();
        TestServices::KeyboardHelper->Left();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Scroll the mouse wheel and make sure the selected index doesn't change.");

        TestServices::InputHelper->MoveMouse(pivot);
        TestServices::InputHelper->ScrollMouseWheel(pivot, 10);
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_ARE_EQUAL(0u, viewChangedCounter);
    }

    void PivotIntegrationTests::NavigationButtonsShouldNotShowUpForLockedOrStaticHeaderPivot()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(1000, 300));

        auto pivot = SetupPivotTest(PivotContent::TextBlockContent);
        xaml_controls::ContentControl^ header;
        xaml::FrameworkElement^ previousButton;

        RunOnUIThread([&]()
        {
            header = safe_cast<xaml_controls::ContentControl^>(TreeHelper::GetVisualChildByName(pivot, L"HeaderClipper"));
            previousButton = TreeHelper::GetVisualChildByName(pivot, L"PreviousButton");
            VERIFY_IS_NOT_NULL(previousButton);

            LOG_OUTPUT(L"Setting up a locked pivot with dynamic headers.");
            pivot->IsLocked = true;
            pivot->Width = 100;
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Moving the mouse to the Pivot's header location. This should NOT cause the navigation buttons to appear.");
        TestServices::InputHelper->MoveMouse(header);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0.0f, previousButton->Opacity);

            LOG_OUTPUT(L"Unlock the pivot, switch to static headers and move the mouse on top of the header. This should NOT cause the navigation buttons to appear.");
            pivot->IsLocked = false;
            pivot->Width = 1000;
        });

        TestServices::InputHelper->MoveMouse(pivot);
        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->MoveMouse(header);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0.0f, previousButton->Opacity);
        });
    }

    void PivotIntegrationTests::CanRestylePivot()
    {
        TestCleanupWrapper cleanup;

        xaml_media::SolidColorBrush^ aliceBlue = nullptr;

        auto pivot = SetupPivotTest(PivotContent::TextBlockContent);

        RunOnUIThread([&]()
        {
            aliceBlue = ref new xaml_media::SolidColorBrush(mu::Colors::AliceBlue);

            wxaml_interop::TypeName type = wxaml_interop::TypeName();
            type.Name = "Microsoft.UI.Xaml.Controls.Pivot";
            type.Kind = wxaml_interop::TypeKind::Metadata;

            auto style = ref new xaml::Style(type);
            style->Setters->Append(ref new xaml::Setter(xaml_controls::Control::BackgroundProperty, aliceBlue));

            LOG_OUTPUT(L"Set the Pivot style!");
            pivot->Style = style;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(pivot->Background, aliceBlue);
        });
    }

    void PivotIntegrationTests::CanActivePivotHeaderItemChangeFromPressedToPointerOver()
    {
        TestCleanupWrapper cleanup;

        xaml_primitives::PivotHeaderItem^ activePivotHeaderItem = nullptr;
        xaml::VisualStateGroup^ selectionStatesVisualStateGroup;

        auto pivot = SetupPivotTest(PivotContent::TextBlockContent);

        RunOnUIThread([&]()
        {
            xaml_primitives::PivotHeaderPanel^ pivotHeader = safe_cast<xaml_primitives::PivotHeaderPanel^>(TreeHelper::GetVisualChildByName(pivot, L"Header"));
            activePivotHeaderItem = safe_cast<xaml_primitives::PivotHeaderItem^>(pivotHeader->Children->GetAt(0));

            auto templateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(activePivotHeaderItem, 0));
            selectionStatesVisualStateGroup = safe_cast<xaml::VisualStateGroup^>(templateRoot->FindName(L"SelectionStates"));

            pivot->Focus(xaml::FocusState::Programmatic); // ensures consistency across runs.
        });

        TestServices::WindowHelper->WaitForIdle();

        auto pivotHeaderPointerEnteredEvent = std::make_shared<Event>();
        auto pivotHeaderPointerEnteredRegistration = CreateSafeEventRegistration(xaml_primitives::PivotHeaderItem, PointerEntered);
        pivotHeaderPointerEnteredRegistration.Attach(activePivotHeaderItem, [&]()
        {
            LOG_OUTPUT(L"Pointer entered pivot header item.");
            pivotHeaderPointerEnteredEvent->Set();
        });

        auto pivotHeaderPointerPressedEvent = std::make_shared<Event>();
        auto pivotHeaderPointerPressedRegistration = CreateSafeEventRegistration(xaml_primitives::PivotHeaderItem, PointerPressed);
        pivotHeaderPointerPressedRegistration.Attach(activePivotHeaderItem, [&]()
        {
            LOG_OUTPUT(L"Pointer pressed pivot header item.");
            pivotHeaderPointerPressedEvent->Set();
        });

        TestServices::WindowHelper->WaitForIdle();

        //Pointer has to be moved to other object before moving it to desired control otherwise PointerOver event is not detected
        LOG_OUTPUT(L"Move mouse pointer over pivot");
        TestServices::InputHelper->MoveMouse(pivot);
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(ref new Platform::String(L"Selected"), selectionStatesVisualStateGroup->CurrentState->Name);
        });

        LOG_OUTPUT(L"Move mouse pointer over activePivotHeaderItem");
        TestServices::InputHelper->MoveMouse(activePivotHeaderItem);
        pivotHeaderPointerEnteredEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(ref new Platform::String(L"SelectedPointerOver"), selectionStatesVisualStateGroup->CurrentState->Name);
        });

        LOG_OUTPUT(L"Click activePivotHeaderItem");
        TestServices::InputHelper->MouseButtonDown(activePivotHeaderItem, 5, 5, MouseButton::Left);
        pivotHeaderPointerPressedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(ref new Platform::String(L"SelectedPressed"), selectionStatesVisualStateGroup->CurrentState->Name);
        });

        TestServices::InputHelper->MouseButtonUp(activePivotHeaderItem, 5, 5, MouseButton::Left);
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(ref new Platform::String(L"SelectedPointerOver"), selectionStatesVisualStateGroup->CurrentState->Name);
        });
    }

    void PivotIntegrationTests::CanPivotHeaderItemUpdateVisualsOnPointerCaptureLost()
    {
        TestCleanupWrapper cleanup;

        xaml_primitives::PivotHeaderItem^ pivotHeaderItem = nullptr;
        xaml::VisualStateGroup^ pivotHeaderItemVisualStateGroup;

        auto pivot = SetupPivotTest(PivotContent::TextBlockContent);

        RunOnUIThread([&]()
        {
            xaml_primitives::PivotHeaderPanel^ pivotHeader = safe_cast<xaml_primitives::PivotHeaderPanel^>(TreeHelper::GetVisualChildByName(pivot, L"Header"));
            pivotHeaderItem = safe_cast<xaml_primitives::PivotHeaderItem^>(pivotHeader->Children->GetAt(1));
            auto templateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(pivotHeaderItem, 0));
            auto groups = xaml::VisualStateManager::GetVisualStateGroups(templateRoot);
            pivotHeaderItemVisualStateGroup = safe_cast<xaml::VisualStateGroup^>(templateRoot->FindName(L"SelectionStates"));
        });

        TestServices::WindowHelper->WaitForIdle();

        auto pivotHeaderPointerPressedEvent = std::make_shared<Event>();
        auto pivotHeaderPointerPressedRegistration = CreateSafeEventRegistration(xaml_primitives::PivotHeaderItem, PointerPressed);

        pivotHeaderPointerPressedRegistration.Attach(
            pivotHeaderItem,
            ref new PointerEventHandler(
            [pivotHeaderPointerPressedEvent](Platform::Object^, PointerRoutedEventArgs^)
        {
            LOG_OUTPUT(L"Pointer pressed pivot header item.");
            pivotHeaderPointerPressedEvent->Set();
        }));

        auto pivotHeaderPointerCaptureLostEvent = std::make_shared<Event>();
        auto pivotHeaderPointerCaptureLostRegistration = CreateSafeEventRegistration(xaml_primitives::PivotHeaderItem, PointerCaptureLost);

        pivotHeaderPointerCaptureLostRegistration.Attach(
            pivotHeaderItem,
            ref new PointerEventHandler(
            [pivotHeaderPointerCaptureLostEvent](Platform::Object^, PointerRoutedEventArgs^)
        {
            LOG_OUTPUT(L"Pointer capture lost pivot header item.");
            pivotHeaderPointerCaptureLostEvent->Set();
        }));

        //Simulate touch over item and moving finger outside of control
        TestServices::InputHelper->PressHoldAndPanFromCenter(pivotHeaderItem, 0, 100, 1, 1000);
        pivotHeaderPointerPressedEvent->WaitForDefault();
        pivotHeaderPointerCaptureLostEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            auto currentState = pivotHeaderItemVisualStateGroup->CurrentState;
            //Verify that PivotHeaderItem returns to Unselected and doesn't stay UnselectedPressed
            VERIFY_ARE_EQUAL(ref new Platform::String(L"Unselected"), currentState->Name);
        });
    }

    void PivotIntegrationTests::CanPivotShowPaddleButtonsAfterAddingItem()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Pivot^ pivot = nullptr;
        xaml_controls::ContentControl^ header;
        xaml::FrameworkElement^ previousButton;

        RunOnUIThread([&]()
        {
            pivot = safe_cast<xaml_controls::Pivot^> (xaml_markup::XamlReader::Load(
                L"<Pivot xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Title='Test Pivot' Width='300'>"
                L"  <PivotItem Header='PItem1'>"
                L"      <StackPanel Margin='5'>"
                L"          <TextBlock TextWrapping='NoWrap' Text='Test SampleText for PivotItem1' />"
                L"      </StackPanel>"
                L"  </PivotItem>"
                L"  <PivotItem Header='PItem2'>"
                L"      <StackPanel Margin='5'>"
                L"          <TextBlock TextWrapping='NoWrap' Text='Test SampleText for PivotItem2' />"
                L"      </StackPanel>"
                L"  </PivotItem>"
                L"  <PivotItem Header='PItem3'>"
                L"      <StackPanel Margin='5'>"
                L"          <TextBlock TextWrapping='NoWrap' Text='Test SampleText for PivotItem3' />"
                L"      </StackPanel>"
                L"  </PivotItem>"
                L"</Pivot>"));

            TestServices::WindowHelper->WindowContent = pivot;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            header = safe_cast<xaml_controls::ContentControl^>(TreeHelper::GetVisualChildByName(pivot, L"HeaderClipper"));
            previousButton = TreeHelper::GetVisualChildByName(pivot, L"PreviousButton");
            VERIFY_IS_NOT_NULL(previousButton);
            //Add a new item to pivot, this should invalidate the pivot measure and activate the paddle buttons
            xaml_controls::PivotItem^ testItem = ref new xaml_controls::PivotItem();
            testItem->Header = "New Pivot Item";
            pivot->Items->Append(testItem);
        });
        TestServices::WindowHelper->WaitForIdle();

        //Pointer has to be moved to other object before moving it to desired control
        TestServices::InputHelper->MoveMouse(pivot);
        TestServices::WindowHelper->WaitForIdle();
        //Simulate mouse moving over the pivot header to make paddle buttons visible
        TestServices::InputHelper->MoveMouse(header);
        TestServices::WindowHelper->WaitForIdle();

        //Verify that paddle buttons are visible
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1.0f, previousButton->Opacity);
        });
    }

    void PivotIntegrationTests::CanNavigatePivotWithGamepad()
    {
        TestCleanupWrapper cleanup;

        InputDevice device = InputDevice::Gamepad;

        xaml_controls::Panel^ rootPanel = nullptr;
        xaml_controls::Pivot^ pivot = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Panel, Loaded);
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Panel, GotFocus);

        Platform::String^ xamlFile = GetPackageFolder() + L"resources\\native\\controls\\Pivot\\PivotForGamepadNavigation.xaml";
        rootPanel = safe_cast<xaml_controls::Panel^>(LoadXamlFileOnUIThread(xamlFile));

        std::shared_ptr<Platform::String^> focusSequence = std::make_shared<Platform::String^>();
        Platform::String^ expectedFocusSequence = "[LB][LHB][HeaderClipper][RHB][RB][RHB][HeaderClipper][CB1][TB1][BB][TB1][CB1][HeaderClipper]";

        RunOnUIThread([&]()
        {
            loadedRegistration.Attach(rootPanel,
                ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                loadedEvent->Set();
            }));

            gotFocusRegistration.Attach(rootPanel,
                ref new xaml::RoutedEventHandler([focusSequence](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                Platform::String^ temp = *focusSequence;
                temp += "[" + safe_cast<xaml::FrameworkElement^>(args->OriginalSource)->Name + "]";
                *focusSequence = temp;
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        *focusSequence = "";

        RunOnUIThread([&]()
        {
            pivot = safe_cast<xaml_controls::Pivot^>(TreeHelper::GetVisualChildByName(rootPanel, L"P"));
            safe_cast<xaml_controls::Control^>(TreeHelper::GetVisualChildByName(rootPanel, L"LB"))->Focus(FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Go right six times to go all the way to the right button.");
        CommonInputHelper::Right(device);
        TestServices::WindowHelper->WaitForIdle();
        CommonInputHelper::Right(device);
        TestServices::WindowHelper->WaitForIdle();
        CommonInputHelper::Right(device);
        TestServices::WindowHelper->WaitForIdle();
        CommonInputHelper::Right(device);
        TestServices::WindowHelper->WaitForIdle();
        CommonInputHelper::Right(device);
        TestServices::WindowHelper->WaitForIdle();
        CommonInputHelper::Right(device);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Go left four times to go back to the first element of the header panel.");
        CommonInputHelper::Left(device);
        TestServices::WindowHelper->WaitForIdle();
        CommonInputHelper::Left(device);
        TestServices::WindowHelper->WaitForIdle();
        CommonInputHelper::Left(device);
        TestServices::WindowHelper->WaitForIdle();
        CommonInputHelper::Left(device);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Go down three times to go to the bottom button.");
        CommonInputHelper::Down(device);
        TestServices::WindowHelper->WaitForIdle();
        CommonInputHelper::Down(device);
        TestServices::WindowHelper->WaitForIdle();
        CommonInputHelper::Down(device);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Go up three times to go back to the header panel.");
        CommonInputHelper::Up(device);
        TestServices::WindowHelper->WaitForIdle();
        CommonInputHelper::Up(device);
        TestServices::WindowHelper->WaitForIdle();
        CommonInputHelper::Up(device);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Expected focus sequence: %s", expectedFocusSequence->Data());
        LOG_OUTPUT(L"Actual focus sequence:   %s", (*focusSequence)->Data());
        VERIFY_IS_TRUE(Platform::String::CompareOrdinal(expectedFocusSequence, *focusSequence) == 0);
    }

    void PivotIntegrationTests::CanChangeHeadersWithGamepadShoulderButtons()
    {
        TestCleanupWrapper cleanup;
        CanChangeHeadersWithGamepadShoulderButtonsImpl(false /* isRTL */);
    }

    void PivotIntegrationTests::CanChangeHeadersWithGamepadShoulderButtonsRightToLeft()
    {
        TestCleanupWrapper cleanup;
        CanChangeHeadersWithGamepadShoulderButtonsImpl(true /* isRTL */);
    }

    void PivotIntegrationTests::CanChangeHeadersWithGamepadShoulderButtonsImpl(bool isRTL)
    {
        xaml_controls::Panel^ rootPanel = nullptr;
        xaml_controls::Pivot^ pivot = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto selectionChangedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Panel, Loaded);
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, SelectionChanged);

        Platform::String^ xamlFile = GetPackageFolder() + L"resources\\native\\controls\\Pivot\\PivotForGamepadNavigation.xaml";
        rootPanel = safe_cast<xaml_controls::Panel^>(LoadXamlFileOnUIThread(xamlFile));

        RunOnUIThread([&]()
        {
            loadedRegistration.Attach(rootPanel,
                ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                loadedEvent->Set();
            }));

            if (isRTL)
            {
                rootPanel->FlowDirection = xaml::FlowDirection::RightToLeft;
            }

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            pivot = safe_cast<xaml_controls::Pivot^>(TreeHelper::GetVisualChildByName(rootPanel, L"P"));

            selectionChangedRegistration.Attach(pivot,
                ref new xaml_controls::SelectionChangedEventHandler([selectionChangedEvent](Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ e)
            {
                selectionChangedEvent->Set();
            }));

            safe_cast<xaml_controls::Control^>(TreeHelper::GetVisualChildByName(rootPanel, L"LB"))->Focus(FocusState::Keyboard);
        });

        if (isRTL)
        {
            LOG_OUTPUT(L"Go left twice to go to the header panel.");
            TestServices::KeyboardHelper->GamepadDpadLeft();
            TestServices::KeyboardHelper->GamepadDpadLeft();
        }
        else
        {
            LOG_OUTPUT(L"Go right twice to go to the header panel.");
            TestServices::KeyboardHelper->GamepadDpadRight();
            TestServices::KeyboardHelper->GamepadDpadRight();
        }
        TestServices::WindowHelper->WaitForIdle();

        if (isRTL)
        {
            LOG_OUTPUT(L"Tap the left shoulder button to switch pivot items.");
            TestServices::KeyboardHelper->GamepadLeftShoulder();
        }
        else
        {
            LOG_OUTPUT(L"Tap the right shoulder button to switch pivot items.");
            TestServices::KeyboardHelper->GamepadRightShoulder();
        }
        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, pivot->SelectedIndex);
        });

        if (isRTL)
        {
            LOG_OUTPUT(L"Tap the right shoulder button to switch pivot items.");
            TestServices::KeyboardHelper->GamepadRightShoulder();
        }
        else
        {
            LOG_OUTPUT(L"Tap the left shoulder button to switch pivot items.");
            TestServices::KeyboardHelper->GamepadLeftShoulder();
        }
        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, pivot->SelectedIndex);
        });

        LOG_OUTPUT(L"Go down to go into the Pivot content.");
        TestServices::KeyboardHelper->GamepadDpadDown();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tapping the shoulder buttons should still switch pivot items.");
        if (isRTL)
        {
            TestServices::KeyboardHelper->GamepadLeftShoulder();
        }
        else
        {
            TestServices::KeyboardHelper->GamepadRightShoulder();
        }
        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, pivot->SelectedIndex);
        });

        if (isRTL)
        {
            TestServices::KeyboardHelper->GamepadRightShoulder();
        }
        else
        {
            TestServices::KeyboardHelper->GamepadLeftShoulder();
        }

        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, pivot->SelectedIndex);
        });
    }

    void PivotIntegrationTests::ValidatePivotHeadersDoNotWrapWithGamepad()
    {
        TestCleanupWrapper cleanup;

        InputDevice device = InputDevice::Gamepad;

        xaml_controls::Panel^ rootPanel = nullptr;
        xaml_controls::Pivot^ pivot = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto selectionChangedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Panel, Loaded);
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, SelectionChanged);

        Platform::String^ xamlFile = GetPackageFolder() + L"resources\\native\\controls\\Pivot\\PivotForGamepadNavigation.xaml";
        rootPanel = safe_cast<xaml_controls::Panel^>(LoadXamlFileOnUIThread(xamlFile));

        RunOnUIThread([&]()
        {
            loadedRegistration.Attach(rootPanel,
                ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                loadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            pivot = safe_cast<xaml_controls::Pivot^>(TreeHelper::GetVisualChildByName(rootPanel, L"P"));

            selectionChangedRegistration.Attach(pivot,
                ref new xaml_controls::SelectionChangedEventHandler([selectionChangedEvent](Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ e)
            {
                selectionChangedEvent->Set();
            }));

            safe_cast<xaml_controls::Control^>(TreeHelper::GetVisualChildByName(rootPanel, L"LB"))->Focus(FocusState::Keyboard);
        });

        LOG_OUTPUT(L"Go right twice to go to the header panel.");
        CommonInputHelper::Right(device);
        CommonInputHelper::Right(device);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tap right to switch pivot items.");
        CommonInputHelper::Right(device);

        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, pivot->SelectedIndex);
        });

        CommonInputHelper::Right(device);

        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(2, pivot->SelectedIndex);
        });

        LOG_OUTPUT(L"Now tap left to switch pivot items.");
        CommonInputHelper::Left(device);

        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, pivot->SelectedIndex);
        });

        CommonInputHelper::Left(device);

        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, pivot->SelectedIndex);
        });

        if (device == InputDevice::Gamepad)
        {
            LOG_OUTPUT(L"Tap the right shoulder button to switch pivot items.");
            TestServices::KeyboardHelper->GamepadRightShoulder();

            selectionChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(1, pivot->SelectedIndex);
            });

            TestServices::KeyboardHelper->GamepadRightShoulder();

            selectionChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(2, pivot->SelectedIndex);
            });

            LOG_OUTPUT(L"Tapping the right shoulder button now should not move anything, since we're at the last item.");
            TestServices::KeyboardHelper->GamepadRightShoulder();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(2, pivot->SelectedIndex);
            });

            LOG_OUTPUT(L"Now tap the left shoulder button to switch pivot items.");
            TestServices::KeyboardHelper->GamepadLeftShoulder();

            selectionChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(1, pivot->SelectedIndex);
            });

            TestServices::KeyboardHelper->GamepadLeftShoulder();

            selectionChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, pivot->SelectedIndex);
            });

            LOG_OUTPUT(L"Tapping the left shoulder button now should not move anything, since we're at the first item.");
            TestServices::KeyboardHelper->GamepadLeftShoulder();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, pivot->SelectedIndex);
            });

            LOG_OUTPUT(L"Go down to go into the Pivot content.");
            TestServices::KeyboardHelper->GamepadDpadDown();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Tap the right shoulder button to switch pivot items.");
            TestServices::KeyboardHelper->GamepadRightShoulder();

            selectionChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(1, pivot->SelectedIndex);
            });

            TestServices::KeyboardHelper->GamepadRightShoulder();

            selectionChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(2, pivot->SelectedIndex);
            });

            LOG_OUTPUT(L"Tapping the right shoulder button now should not move anything, since we're at the last item.");
            TestServices::KeyboardHelper->GamepadRightShoulder();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(2, pivot->SelectedIndex);
            });

            LOG_OUTPUT(L"Now tap the left shoulder button to switch pivot items.");
            TestServices::KeyboardHelper->GamepadLeftShoulder();

            selectionChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(1, pivot->SelectedIndex);
            });

            TestServices::KeyboardHelper->GamepadLeftShoulder();

            selectionChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, pivot->SelectedIndex);
            });

            LOG_OUTPUT(L"Tapping the left shoulder button now should not move anything, since we're at the first item.");
            TestServices::KeyboardHelper->GamepadLeftShoulder();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(0, pivot->SelectedIndex);
            });
        }
    }

    void PivotIntegrationTests::ValidateFootprint()
    {
        TestCleanupWrapper cleanup;

        double const expectedPivotContainerHeightWithTitle = 94;
        double const expectedPivotContainerHeightWithoutTitle = 48;

        xaml_controls::StackPanel^ rootPanel = nullptr;

        xaml_controls::Pivot^ noTitlePivot = CreatePivot(PivotContent::TextBlockContent);
        xaml_controls::Pivot^ titlePivot = CreatePivot(PivotContent::TextBlockContent);

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        RunOnUIThread([&]()
        {
            rootPanel = ref new xaml_controls::StackPanel();

            titlePivot->Title = L"Pivot Title";

            rootPanel->Children->Append(noTitlePivot);
            rootPanel->Children->Append(titlePivot);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto noTitlepivotHeaderPanel = safe_cast<xaml::FrameworkElement^>(TreeHelper::GetVisualChildByName(noTitlePivot, L"Header"));
            auto titlepivotHeaderPanel = safe_cast<xaml::FrameworkElement^>(TreeHelper::GetVisualChildByName(titlePivot, L"Header"));
            auto tileContentControl = safe_cast<xaml::FrameworkElement^>(TreeHelper::GetVisualChildByName(titlePivot, L"TitleContentControl"));

            auto actualPivotContainerHeightWithTitle = tileContentControl->ActualHeight +
                                                        tileContentControl->Margin.Top +
                                                        tileContentControl->Margin.Bottom +
                                                        titlepivotHeaderPanel->ActualHeight;

            VERIFY_ARE_EQUAL(expectedPivotContainerHeightWithoutTitle, noTitlepivotHeaderPanel->ActualHeight);
            VERIFY_ARE_EQUAL(expectedPivotContainerHeightWithTitle, actualPivotContainerHeightWithTitle);
        });
    }

    void PivotIntegrationTests::ValidateChangingItemsDoesNotChangeFocusState()
    {
        TestCleanupWrapper cleanup;

        Platform::String^ xamlFile = GetPackageFolder() + L"resources\\native\\controls\\Pivot\\PivotWithAlternatingContent.xaml";
        xaml_controls::Panel^ rootPanel = safe_cast<xaml_controls::Panel^>(LoadXamlFileOnUIThread(xamlFile));
        xaml_controls::Pivot^ pivot = nullptr;
        xaml::FrameworkElement^ focusElement = nullptr;

        xaml_controls::ContentControl^ headerClipper = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto selectionChangedEvent = std::make_shared<Event>();
        auto finalViewChangedEvent = std::make_shared<Event>();
        auto gotFocusEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Panel, Loaded);
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, SelectionChanged);
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, GotFocus);
        auto finalViewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        RunOnUIThread([&]()
        {
            loadedRegistration.Attach(rootPanel,
                ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                loadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            pivot = safe_cast<xaml_controls::Pivot^>(TreeHelper::GetVisualChildByName(rootPanel, L"Pivot"));

            selectionChangedRegistration.Attach(pivot,
                ref new xaml_controls::SelectionChangedEventHandler([selectionChangedEvent](Platform::Object^, xaml_controls::SelectionChangedEventArgs^)
            {
                selectionChangedEvent->Set();
            }));

            gotFocusRegistration.Attach(pivot, ref new xaml::RoutedEventHandler([gotFocusEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                gotFocusEvent->Set();
            }));

            headerClipper = safe_cast<xaml_controls::ContentControl^>(TreeHelper::GetVisualChildByName(pivot, L"HeaderClipper"));
            auto scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(pivot, L"ScrollViewer"));

            finalViewChangedRegistration.Attach(scrollViewer,
                ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [&finalViewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                if (args->IsIntermediate == false)
                {
                    finalViewChangedEvent->Set();
                }
            }));

            pivot->Focus(xaml::FocusState::Keyboard);
        });

        gotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml::FocusState::Keyboard, headerClipper->FocusState);
        });

        LOG_OUTPUT(L"Move to the next pivot item.  We should still have keyboard focus.");
        RunOnUIThread([&]()
        {
            pivot->SelectedIndex = 1;
        });

        selectionChangedEvent->WaitForDefault();
        finalViewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml::FocusState::Keyboard, headerClipper->FocusState);
        });

        LOG_OUTPUT(L"Move to the next pivot item again to get back to a pivot item with focusable content.");
        RunOnUIThread([&]()
        {
            pivot->SelectedIndex = 2;
        });

        selectionChangedEvent->WaitForDefault();
        finalViewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Get the first focusable element.
            focusElement = TreeHelper::GetVisualChildByType<xaml_controls::TextBox>(safe_cast<xaml::FrameworkElement^>(pivot->Items->GetAt(2)));
        });

        LOG_OUTPUT(L"Give pointer focus to the first focusable item, and then swipe left to go to the next pivot item.");
        TestServices::InputHelper->Tap(focusElement);
        
        gotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_NOT_EQUAL(xaml::FocusState::Keyboard, headerClipper->FocusState);
        });

        LOG_OUTPUT(L"Programmatically switch to the next pivot item.  We should still not have keyboard focus.");
        RunOnUIThread([&]()
        {
            pivot->SelectedIndex = 3;
        });

        selectionChangedEvent->WaitForDefault();
        finalViewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_NOT_EQUAL(xaml::FocusState::Keyboard, headerClipper->FocusState);
        });
    }

    void PivotIntegrationTests::ValidateEmptyPivotItemDoesNotCauseCrash()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::Pivot^ pivot = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto gotFocusEvent = std::make_shared<Event>();
        auto selectionChangedEvent = std::make_shared<Event>();
        auto finalViewChangedEvent = std::make_shared<Event>();

        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, SelectionChanged);
        auto finalViewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        Platform::String^ xamlFile = GetPackageFolder() + L"resources\\native\\controls\\Pivot\\PivotWithEmptyItems.xaml";
        rootPanel = safe_cast<xaml_controls::Grid^>(LoadXamlFileOnUIThread(xamlFile));

        RunOnUIThread([&]()
        {
            loadedRegistration.Attach(rootPanel,
                ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                loadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            pivot = safe_cast<xaml_controls::Pivot^>(TreeHelper::GetVisualChildByName(rootPanel, L"P"));
            auto button = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(rootPanel, L"B"));
            auto scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(pivot, L"ScrollViewer"));

            gotFocusRegistration.Attach(button,
                ref new xaml::RoutedEventHandler([gotFocusEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                gotFocusEvent->Set();
            }));

            selectionChangedRegistration.Attach(pivot,
                ref new xaml_controls::SelectionChangedEventHandler([selectionChangedEvent](Platform::Object^, xaml_controls::SelectionChangedEventArgs^)
            {
                selectionChangedEvent->Set();
            }));

            finalViewChangedRegistration.Attach(scrollViewer,
                ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [&finalViewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                if (args->IsIntermediate == false)
                {
                    finalViewChangedEvent->Set();
                }
            }));

            LOG_OUTPUT(L"Give the button focus to ensure that we have a focused element outside of the Pivot.");
            button->Focus(xaml::FocusState::Programmatic);
        });

        gotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Move to the next pivot item. We should not crash.");
        RunOnUIThread([&]()
        {
            pivot->SelectedIndex = 1;
        });

        selectionChangedEvent->WaitForDefault();
        finalViewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void PivotIntegrationTests::CanChangeHeaderTemplateAtRuntime()
    {
        TestCleanupWrapper cleanup;

        auto pivot = SetupPivotTest(PivotContent::TextBlockContent);

        RunOnUIThread([&]()
        {
            pivot->HeaderTemplate = safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <TextBlock Text='Template text' />"
                L"</DataTemplate>"));

        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto headerPanel = safe_cast<xaml_primitives::PivotHeaderPanel^>(TreeHelper::GetVisualChildByName(pivot, L"Header"));
            auto firstItem = safe_cast<xaml_controls::ContentControl^>(headerPanel->Children->GetAt(0));
            auto firstItemContent = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByType<xaml_controls::TextBlock>(firstItem));

            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(L"Template text", safe_cast<Platform::String^>(firstItemContent->Text)) == 0);
        });
    }

    void PivotIntegrationTests::CanChangeTitleAtRuntime()
    {
        TestCleanupWrapper cleanup;

        auto pivot = SetupPivotTest(PivotContent::TextBlockContent);

        RunOnUIThread([&]()
        {
            pivot->Title = L"New title";
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto titleControl = safe_cast<xaml_controls::ContentControl^>(TreeHelper::GetVisualChildByName(pivot, L"TitleContentControl"));

            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(L"New title", safe_cast<Platform::String^>(titleControl->Content)) == 0);
        });
    }

    void PivotIntegrationTests::ValidateInfiniteSpaceAvailableSnapsPivotToWindowWidth()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_controls::Pivot^ pivot = nullptr;

        auto rootPanelLoadedEvent = std::make_shared<Event>();
        auto rootPanelLoadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Orientation='Horizontal'>
                    <Pivot Title='Test Pivot'>
                        <PivotItem Header='PItem1'>
                            <StackPanel Margin='5'>
                                <TextBlock TextWrapping='NoWrap' Text='Test SampleText for PivotItem1' />
                            </StackPanel>
                        </PivotItem>
                        <PivotItem Header='PItem2'>
                            <StackPanel Margin='5'>
                                <TextBlock TextWrapping='NoWrap' Text='Test SampleText for PivotItem2' />
                            </StackPanel>
                        </PivotItem>
                        <PivotItem Header='PItem3'>
                            <StackPanel Margin='5'>
                                <TextBlock TextWrapping='NoWrap' Text='Test SampleText for PivotItem3' />
                            </StackPanel>
                        </PivotItem>
                    </Pivot>
                </StackPanel>)"));

            rootPanelLoadedRegistration.Attach(rootPanel, [rootPanelLoadedEvent]() { rootPanelLoadedEvent->Set(); });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        rootPanelLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            pivot = TreeHelper::GetVisualChildByType<xaml_controls::Pivot>(rootPanel);
            VERIFY_ARE_EQUAL(TestServices::WindowHelper->WindowBounds.Width, pivot->ActualWidth);
        });
    }

    void PivotIntegrationTests::CanApplySlideInAnimationGroupToElements()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Pivot^ pivot = nullptr;

        auto rootPanelLoadedEvent = std::make_shared<Event>();
        auto finalViewChangedEvent = std::make_shared<Event>();
        auto rootPanelLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, Loaded);
        auto finalViewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        RunOnUIThread([&]()
        {
            pivot = safe_cast<xaml_controls::Pivot^> (xaml_markup::XamlReader::Load(
                LR"(<Pivot Title='Test Pivot' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                    <PivotItem Header='PItem1'>
                        <StackPanel Margin='5'>
                            <TextBlock TextWrapping='NoWrap' Text='Test SampleText for PivotItem1' Pivot.SlideInAnimationGroup='Default' />
                        </StackPanel>
                    </PivotItem>
                    <PivotItem Header='PItem2'>
                        <StackPanel Margin='5'>
                            <TextBlock TextWrapping='NoWrap' Text='Test SampleText for PivotItem2' Pivot.SlideInAnimationGroup='GroupOne' />
                            <TextBlock TextWrapping='NoWrap' Text='Test SampleText for PivotItem2' Pivot.SlideInAnimationGroup='GroupTwo' />
                            <TextBlock TextWrapping='NoWrap' Text='Test SampleText for PivotItem2' Pivot.SlideInAnimationGroup='GroupThree' />
                        </StackPanel>
                    </PivotItem>
                </Pivot>)"));

            rootPanelLoadedRegistration.Attach(pivot, [rootPanelLoadedEvent]() { rootPanelLoadedEvent->Set(); });

            TestServices::WindowHelper->WindowContent = pivot;
        });

        rootPanelLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(pivot, L"ScrollViewer"));

            finalViewChangedRegistration.Attach(
                scrollViewer,
                ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [&finalViewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args) {
                if (args->IsIntermediate == false)
                {
                    finalViewChangedEvent->Set();
                }
            }));

            pivot->SelectedIndex = 1;
        });

        finalViewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void PivotIntegrationTests::ValidateCanAddItemsOnSelectionChanged()
    {
        TestCleanupWrapper cleanup;
        auto pivot = SetupPivotTest(PivotContent::TextBlockContent);

        auto finalViewChangedEvent = std::make_shared<Event>();
        auto finalViewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, SelectionChanged);

        RunOnUIThread([&]()
        {
            // Set the width of the pivot to 300 to ensure that the flick doesn't go too far.
            pivot->Width = 300;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(pivot, L"ScrollViewer"));

            finalViewChangedRegistration.Attach(scrollViewer,
                ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>([&finalViewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                if (args->IsIntermediate == false)
                {
                    finalViewChangedEvent->Set();
                }
            }));

            pivot->SelectedIndex = 1;
        });

        finalViewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            selectionChangedRegistration.Attach(pivot,
                ref new xaml_controls::SelectionChangedEventHandler([pivot](Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ e)
            {
                xaml_controls::PivotItem^ newItem = ref new xaml_controls::PivotItem();
                newItem->Header = L"PItem0";
                newItem->Content = L"PItem0";
                pivot->Items->InsertAt(0, newItem);
            }));
        });

        LOG_OUTPUT(L"Flick right to move to index 0.  This will add a new item at index 0.");
        TestServices::InputHelper->Flick(pivot, FlickDirection::East);

        finalViewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto headerPanel = safe_cast<xaml_primitives::PivotHeaderPanel^>(TreeHelper::GetVisualChildByName(pivot, L"Header"));
            int selectedIndex = -1;

            for (unsigned int i = 0; i < headerPanel->Children->Size; i++)
            {
                auto headerItemRoot = TreeHelper::GetVisualChildByName(safe_cast<xaml::FrameworkElement^>(headerPanel->Children->GetAt(i)), L"Grid");
                auto headerItemRootVisualStateGroups = xaml::VisualStateManager::GetVisualStateGroups(headerItemRoot);

                bool isSelected = false;

                for (unsigned int j = 0; j < headerItemRootVisualStateGroups->Size; j++)
                {
                    auto currentGroup = headerItemRootVisualStateGroups->GetAt(j);
                    auto currentState = currentGroup->CurrentState;

                    if (currentState != nullptr && Platform::String::CompareOrdinal(currentState->Name, L"Selected") == 0)
                    {
                        isSelected = true;
                        break;
                    }
                }

                if (isSelected)
                {
                    selectedIndex = i;
                    break;
                }
            }

            VERIFY_ARE_EQUAL(1, selectedIndex);
        });
    }

    void PivotIntegrationTests::ValidateHeaderFocusVisualPlacement()
    {
        TestCleanupWrapper cleanup;

        // When a Pivot header gets focus, the PivotHeaderItem of the currently selected item should go into the focus state.
        // This focus state should get updated as the SelectedIndex gets updated.
        // Moving the focus away from the Pivot header should result in none of the header items having focus.
        //
        // Note: This behavior is a little unusual because although the focus visual is shown on the PivotHeaderItems, the
        // PivotHeaderItems themselves never actually have focus: focus is actually on the header itself in the Pivot.

        auto pivot = SetupPivotTest(PivotContent::ListViewContent);

        xaml::UIElement^ headerSelectedPipe1;
        xaml::UIElement^ headerSelectedPipe2;
        xaml::UIElement^ headerSelectedPipe3;

        Platform::Collections::Vector<xaml_primitives::PivotHeaderItem^>^ pivotHeaderItems;
        xaml_controls::ContentControl^ headerContentControl;

        RunOnUIThread([&]()
        {
            // The default value of HeaderFocusVisualPlacement should be PivotHeaderFocusVisualPlacement::SelectedItemHeader
            // However this value doesn't effect the SelectedPipe as of the RS4 changes to pivots focus visuals
            VERIFY_ARE_EQUAL(xaml_controls::PivotHeaderFocusVisualPlacement::ItemHeaders, pivot->HeaderFocusVisualPlacement);

            pivotHeaderItems = ref new Platform::Collections::Vector<xaml_primitives::PivotHeaderItem^>();
            TreeHelper::GetVisualChildrenByType<xaml_primitives::PivotHeaderItem>(pivot, pivotHeaderItems);
            headerContentControl = safe_cast<xaml_controls::ContentControl^>(TreeHelper::GetVisualChildByName(pivot, "HeaderClipper"));

            headerSelectedPipe1 = safe_cast<xaml::UIElement^>(TreeHelper::GetVisualChildByName(pivotHeaderItems->GetAt(0), "SelectedPipe"));
            headerSelectedPipe2 = safe_cast<xaml::UIElement^>(TreeHelper::GetVisualChildByName(pivotHeaderItems->GetAt(1), "SelectedPipe"));
            headerSelectedPipe3 = safe_cast<xaml::UIElement^>(TreeHelper::GetVisualChildByName(pivotHeaderItems->GetAt(2), "SelectedPipe"));

            VERIFY_IS_TRUE(headerContentControl->UseSystemFocusVisuals, L"UseSystemFocusVisuals on headerContentControl should be true by default after the RS4 changes to pivot focus visuals");

            pivot->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, pivot->SelectedIndex);
            VERIFY_ARE_EQUAL(xaml::FocusState::Keyboard, headerContentControl->FocusState);
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, headerSelectedPipe1->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, headerSelectedPipe2->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, headerSelectedPipe3->Visibility);

            pivot->SelectedIndex = 1;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, pivot->SelectedIndex);
            VERIFY_ARE_EQUAL(xaml::FocusState::Keyboard, headerContentControl->FocusState);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, headerSelectedPipe1->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, headerSelectedPipe2->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, headerSelectedPipe3->Visibility);

            pivot->SelectedIndex = 0;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, pivot->SelectedIndex);
            VERIFY_ARE_EQUAL(xaml::FocusState::Keyboard, headerContentControl->FocusState);
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, headerSelectedPipe1->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, headerSelectedPipe2->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, headerSelectedPipe3->Visibility);
        });

        // Tab out of the Pivot Header.
        // Selection should be unaffected. All PivotHeaderItems should go to the Unfocused state.
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, pivot->SelectedIndex);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, headerContentControl->FocusState);
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, headerSelectedPipe1->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, headerSelectedPipe2->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, headerSelectedPipe3->Visibility);
        });

        // Switch HeaderFocusVisualPlacement to SelectedItemHeader
        // This property does not effect the selected pipe in RS4 with the pivot focus changes, verify this doesn't change anything.
        RunOnUIThread([&]()
        {
            pivot->HeaderFocusVisualPlacement = xaml_controls::PivotHeaderFocusVisualPlacement::SelectedItemHeader;
            VERIFY_IS_TRUE(headerContentControl->UseSystemFocusVisuals);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, headerContentControl->FocusState);
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, headerSelectedPipe1->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, headerSelectedPipe2->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, headerSelectedPipe3->Visibility);
        });

        // Tab out of the Pivot content back to the Pivot header:
        TestServices::KeyboardHelper->ShiftTab();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml::FocusState::Keyboard, headerContentControl->FocusState);
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, headerSelectedPipe1->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, headerSelectedPipe2->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, headerSelectedPipe3->Visibility);

            pivot->SelectedIndex = 1;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml::FocusState::Keyboard, headerContentControl->FocusState);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, headerSelectedPipe1->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, headerSelectedPipe2->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, headerSelectedPipe3->Visibility);

            pivot->HeaderFocusVisualPlacement = xaml_controls::PivotHeaderFocusVisualPlacement::ItemHeaders;
            VERIFY_IS_TRUE(headerContentControl->UseSystemFocusVisuals);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml::FocusState::Keyboard, headerContentControl->FocusState);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, headerSelectedPipe1->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, headerSelectedPipe2->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, headerSelectedPipe3->Visibility);
        });
    }

    void PivotIntegrationTests::ValidatePivotCanWrapWithTouchWhenCarouselIsEnabled()
    {
        LOG_OUTPUT(L"IsHeaderItemsCarouselEnabled is true, headers fit. Expected: touch wrap allowed.");
        {
            auto itemsWidth = std::vector<double>();
            itemsWidth.push_back(170.0);
            itemsWidth.push_back(150.0);
            itemsWidth.push_back(100.0);
            ValidatePivotCanWrapWithTouchWhenCarouselIsEnabledImpl(itemsWidth, 500.0, /*isHeaderItemsCarouselEnabled:*/ true);
        }
        LOG_OUTPUT(L"IsHeaderItemsCarouselEnabled is true, headers don't fit. Expected: touch wrap allowed.");
        {
            auto itemsWidth = std::vector<double>();
            itemsWidth.push_back(270.0);
            itemsWidth.push_back(250.0);
            itemsWidth.push_back(200.0);
            ValidatePivotCanWrapWithTouchWhenCarouselIsEnabledImpl(itemsWidth, 500.0, /*isHeaderItemsCarouselEnabled:*/ true);
        }
        LOG_OUTPUT(L"IsHeaderItemsCarouselEnabled is false, headers fit. Expected: touch wrap not allowed.");
        {
            auto itemsWidth = std::vector<double>();
            itemsWidth.push_back(170.0);
            itemsWidth.push_back(150.0);
            itemsWidth.push_back(100.0);
            ValidatePivotCanWrapWithTouchWhenCarouselIsEnabledImpl(itemsWidth, 500.0, /*isHeaderItemsCarouselEnabled:*/ false);
        }
        LOG_OUTPUT(L"IsHeaderItemsCarouselEnabled is false, headers don't fit.  Expected: touch wrap not allowed.");
        {
            auto itemsWidth = std::vector<double>();
            itemsWidth.push_back(270.0);
            itemsWidth.push_back(250.0);
            itemsWidth.push_back(200.0);
            ValidatePivotCanWrapWithTouchWhenCarouselIsEnabledImpl(itemsWidth, 500.0, /*isHeaderItemsCarouselEnabled:*/ false);
        }
    }

    void PivotIntegrationTests::ValidatePivotCanWrapWithTouchWhenCarouselIsEnabledImpl(
        const std::vector<double>& itemsWidth,
        double viewportSize,
        bool isHeaderItemsCarouselEnabled)
    {
        TestCleanupWrapper cleanup;
        auto pivot = SetupPivotWithItemWidthsAndViewportSize(itemsWidth, viewportSize, isHeaderItemsCarouselEnabled);
        RunOnUIThread([&]()
        {
            auto scrollViewer = TreeHelper::GetVisualChildByType<xaml_controls::ScrollViewer>(pivot);
            if (isHeaderItemsCarouselEnabled)
            {
                // We use a much bigger multiplier in pivot which allows us to wrap many times before
                // we need to normalize again.
                VERIFY_IS_LESS_THAN(viewportSize * 500, scrollViewer->ExtentWidth);
            }
            else
            {
                VERIFY_ARE_EQUAL(static_cast<int>(viewportSize * itemsWidth.size()), static_cast<int>(scrollViewer->ExtentWidth));
            }
        });
    }

    void PivotIntegrationTests::CanStaticHeadersScroll()
    {
        LOG_OUTPUT(L"Validating the case where headers don't fit in the viewport.");
        {
            auto itemsWidth = std::vector<double>();
            itemsWidth.push_back(170.0);
            itemsWidth.push_back(150.0);
            itemsWidth.push_back(100.0);
            itemsWidth.push_back(175.0);
            CanStaticHeadersScrollImpl(itemsWidth, 300.0);
        }

        LOG_OUTPUT(L"Validating the case where headers do fit in the viewport.");
        {
            auto itemsWidth = std::vector<double>();
            itemsWidth.push_back(120.0);
            itemsWidth.push_back(150.0);
            CanStaticHeadersScrollImpl(itemsWidth, 300.0);
        }

        LOG_OUTPUT(L"Validating the case where one header is bigger than the entire viewport.");
        {
            auto itemsWidth = std::vector<double>();
            itemsWidth.push_back(120.0);
            itemsWidth.push_back(350.0);
            itemsWidth.push_back(150.0);
            CanStaticHeadersScrollImpl(itemsWidth, 180.0);
        }
    }

    void PivotIntegrationTests::CanStaticHeadersScrollImpl(const std::vector<double>& itemsWidth, double viewportSize)
    {
        TestCleanupWrapper cleanup;
        auto pivot = SetupPivotWithItemWidthsAndViewportSize(itemsWidth, viewportSize, false /*isHeaderItemsCarouselEnabled*/);
        auto viewChangedEvent = std::make_shared<Event>();
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        xaml_primitives::PivotHeaderPanel^ staticHeaderPanel = nullptr;

        RunOnUIThread([&]()
        {
            staticHeaderPanel = safe_cast<Microsoft::UI::Xaml::Controls::Primitives::PivotHeaderPanel^>(TreeHelper::GetVisualChildByName(pivot, L"StaticHeader"));

            // Subscribe to ViewChanged
            auto scrollViewer = TreeHelper::GetVisualChildByType<xaml_controls::ScrollViewer>(pivot);
            viewChangedRegistration.Attach(
                scrollViewer,
                ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                    [&](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args) {
                if (args->IsIntermediate == false)
                {
                    LOG_OUTPUT(L"Final ViewChanged event.");
                    viewChangedEvent->Set();
                }
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        enum class SelectionChangeMode
        {
            Programmatic,
            Touch
        };
        const auto selectionChangeModes = std::array<SelectionChangeMode, 2> { SelectionChangeMode::Programmatic, SelectionChangeMode::Touch };
        for (SelectionChangeMode mode : selectionChangeModes)
        {
            LOG_OUTPUT(L"Selection mode: %s", mode == SelectionChangeMode::Programmatic ? L"Programmatic" : L"Touch");
            for (int i = 0; i < static_cast<int>(itemsWidth.size()) - 1; ++i)
            {
                LOG_OUTPUT(L"Validating curve at index %d", i);
                RunOnUIThread([&]()
                {
                    auto pointZero = wf::Point(0.0f, 0.0f);
                    auto headerTransform = staticHeaderPanel->TransformToVisual(pivot);
                    double actualHeaderOffset = (headerTransform->TransformPoint(pointZero)).X;
                    double expectedHeaderOffset = 0.0;
                    {
                        double
                            accumulatedItemWidths = 0.0,    // total width of all headers
                            a = 0.0, // amount of space at the left of the current item.
                            b = 0.0, // width of the current item.
                            c = 0.0, // amount of space at the right of the current item.
                            v = viewportSize;
                        for (int j = 0; j < static_cast<int>(itemsWidth.size()); ++j)
                        {
                            if (j == i)
                            {
                                a = accumulatedItemWidths;
                                b = itemsWidth[j];
                            }

                            accumulatedItemWidths += itemsWidth[j];
                        }

                        c = max(accumulatedItemWidths - (a + b), 0.0);

                        // This calculation aims to keep the proportions of what's visible before/after the selected item.
                        // That's why we divide by a + c (the total space available before/after the selected item) after
                        // multiplying by the viewport size that's not claimed by the selected item (v - std::min(b, v)).
                        // A more comprehensive explanation can be found in the Scrollable Static Pivot Headers design document.
                        expectedHeaderOffset = a + c == 0.0 ? 0.0 : -(max(a - a * (v - min(b, v)) / (a + c), 0.0));
                    }

                    LOG_OUTPUT(L"Expected header offset: %g", expectedHeaderOffset);
                    LOG_OUTPUT(L"Actual header offset: %g", actualHeaderOffset);
                    VERIFY_IS_TRUE(std::abs(expectedHeaderOffset - actualHeaderOffset) < 1.0);

                    LOG_OUTPUT(L"SelectedIndex: %i", pivot->SelectedIndex);
                    auto container = safe_cast<UIElement^>(pivot->ContainerFromIndex(pivot->SelectedIndex));
                    auto itemTransform = container->TransformToVisual(pivot);
                    double actualContainerOffset = (itemTransform->TransformPoint(pointZero)).X;

                    double expectedContainerOffset = 12.0;
                    LOG_OUTPUT(L"Expected container offset: %g", expectedContainerOffset);
                    LOG_OUTPUT(L"Actual container offset: %g", actualContainerOffset);
                    VERIFY_IS_TRUE(std::abs(expectedContainerOffset - actualContainerOffset) < 1.0);
                });

                LOG_OUTPUT(L"Selecting index %d", i + 1);
                if (mode == SelectionChangeMode::Programmatic)
                {
                    RunOnUIThread([&]() { ++pivot->SelectedIndex; });
                }
                else
                {
                    TestServices::InputHelper->PressHoldAndPanFromCenter(pivot, static_cast<int>(-viewportSize / 2.0), 0, 1, 1000);
                }

                viewChangedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();
            }

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Selecting index 0 again");
                pivot->SelectedIndex = 0;
            });
            viewChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
        }
    }

    Microsoft::UI::Xaml::Controls::Pivot^
    PivotIntegrationTests::SetupPivotWithItemWidthsAndViewportSize(const std::vector<double>& itemsWidth, double viewportSize, bool isHeaderItemsCarouselEnabled)
    {
        // The pivot in the xaml file doesn't have any margin/padding set on its header items.
        // In other words, the width that we will set programmatically in the item template will be the header items' width.
        auto pivot = safe_cast<xaml_controls::Pivot^>(LoadXamlFileOnUIThread(GetPackageFolder() + L"resources\\native\\controls\\Pivot\\PivotForStaticHeadersScrollTest.xaml"));
        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, Loaded);

        RunOnUIThread([&]()
        {
            auto itemsSource = ref new Platform::Collections::Vector<Platform::String^>();
            for (int i = 0; i < static_cast<int>(itemsWidth.size()); ++i)
            {
                itemsSource->Append(L"Item #" + i);
            }

            pivot->ItemsSource = itemsSource;
            pivot->Width = viewportSize;
            pivot->IsHeaderItemsCarouselEnabled = isHeaderItemsCarouselEnabled;

            loadedRegistration.Attach(
                pivot,
                [&]()
            {
                LOG_OUTPUT(L"Pivot Loaded");
                loadedEvent->Set();
            });
            TestServices::WindowHelper->WindowContent = pivot;
        });

        loadedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            auto staticHeaderPanel = safe_cast<Microsoft::UI::Xaml::Controls::Primitives::PivotHeaderPanel^>(TreeHelper::GetVisualChildByName(pivot, L"StaticHeader"));
            for (int i = 0; i < static_cast<int>(itemsWidth.size()); ++i)
            {
                safe_cast<FrameworkElement^>(staticHeaderPanel->Children->GetAt(i))->Width = itemsWidth[i];
            }
        });
        TestServices::WindowHelper->WaitForIdle();
        return pivot;
    }

    void PivotIntegrationTests::CanTogglePivotIsHeaderItemsCarouselEnabled()
    {
        TestCleanupWrapper cleanup;

        auto pivot = SetupPivotTest(PivotContent::TextBlockContent);

        RunOnUIThread([&]()
        {
            // The pivot has 3 items and a width of 200 will clip the last header.
            pivot->Width = 200;

            pivot->UpdateLayout();

            auto dynamicHeaderPanel = safe_cast<Microsoft::UI::Xaml::Controls::Primitives::PivotHeaderPanel^>(TreeHelper::GetVisualChildByName(pivot, L"Header"));
            auto staticHeaderPanel = safe_cast<Microsoft::UI::Xaml::Controls::Primitives::PivotHeaderPanel^>(TreeHelper::GetVisualChildByName(pivot, L"StaticHeader"));
            auto pointZero = wf::Point(0.0f, 0.0f);

            LOG_OUTPUT(L"Validate dynamic header configuration.");
            VERIFY_ARE_EQUAL(3u, dynamicHeaderPanel->Children->Size);
            VERIFY_ARE_EQUAL(0u, staticHeaderPanel->Children->Size);
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, dynamicHeaderPanel->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, staticHeaderPanel->Visibility);
            VERIFY_ARE_EQUAL(-49500.0, dynamicHeaderPanel->TransformToVisual(pivot)->TransformPoint(pointZero).X);

            pivot->IsHeaderItemsCarouselEnabled = false;
            pivot->UpdateLayout();

            LOG_OUTPUT(L"Validate static header configuration.");
            VERIFY_ARE_EQUAL(0u, dynamicHeaderPanel->Children->Size);
            VERIFY_ARE_EQUAL(3u, staticHeaderPanel->Children->Size);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, dynamicHeaderPanel->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, staticHeaderPanel->Visibility);
            VERIFY_ARE_EQUAL(0.0, staticHeaderPanel->TransformToVisual(pivot)->TransformPoint(pointZero).X);

            pivot->IsHeaderItemsCarouselEnabled = true;
            pivot->UpdateLayout();

            LOG_OUTPUT(L"Validate dynamic header configuration again.");
            VERIFY_ARE_EQUAL(3u, dynamicHeaderPanel->Children->Size);
            VERIFY_ARE_EQUAL(0u, staticHeaderPanel->Children->Size);
            VERIFY_ARE_EQUAL(xaml::Visibility::Visible, dynamicHeaderPanel->Visibility);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, staticHeaderPanel->Visibility);
            VERIFY_ARE_EQUAL(-49500.0, dynamicHeaderPanel->TransformToVisual(pivot)->TransformPoint(pointZero).X);
        });
    }

    void PivotIntegrationTests::CanPanAfterItemsAddedAtRuntime()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Pivot^ pivot = nullptr;
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, Loaded);

        RunOnUIThread([&]()
        {
            pivot = ref new xaml_controls::Pivot();

            loadedRegistration.Attach(pivot, [&]() { loadedEvent->Set(); });

            TestServices::WindowHelper->WindowContent = pivot;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(pivot, L"ScrollViewer"));
            VERIFY_ARE_EQUAL(xaml_controls::ScrollMode::Disabled, scrollViewer->HorizontalScrollMode);

            pivot->Items->Append(ref new xaml_controls::PivotItem());
            pivot->Items->Append(ref new xaml_controls::PivotItem());
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(xaml_controls::ScrollMode::Enabled, scrollViewer->HorizontalScrollMode);
        });
    }

    void PivotIntegrationTests::CanChangeSelectedItemUsingTap()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Pivot^ pivot = nullptr;
        xaml_controls::ContentControl^ secondItem = nullptr;
        xaml_controls::ContentControl^ thirdItem = nullptr;
        xaml_controls::Button^ thirdItemButton = nullptr;

        auto pivotItemLoadedEvent = std::make_shared<Event>();
        auto finalViewChangedEvent = std::make_shared<Event>();
        auto pivotItemLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, PivotItemLoaded);
        auto finalViewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);
        auto buttonClicked = CreateSafeEventRegistration(xaml_controls::Button, Click);

        RunOnUIThread([&]()
        {
            pivot = safe_cast<xaml_controls::Pivot^> (xaml_markup::XamlReader::Load(
                    LR"(<Pivot xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Title='Test Pivot' Width='300'>
                      <PivotItem Header='1'>Item 1</PivotItem>
                      <PivotItem Header='2'>Item 2</PivotItem>
                      <PivotItem Header='3'><Button>Item 3</Button></PivotItem>
                    </Pivot>)"));

            thirdItemButton = safe_cast<xaml_controls::Button^>(safe_cast<xaml_controls::PivotItem^>(pivot->Items->GetAt(2))->Content);
            buttonClicked.Attach(thirdItemButton, [pivot]() { pivot->SelectedIndex = 0; });

            pivotItemLoadedRegistration.Attach(pivot, [pivotItemLoadedEvent]() { pivotItemLoadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = pivot;
        });

        TestServices::WindowHelper->WaitForIdle();
        pivotItemLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(pivot, L"ScrollViewer"));

            finalViewChangedRegistration.Attach(
                scrollViewer,
                ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [&finalViewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args) {
                if (args->IsIntermediate == false)
                {
                    finalViewChangedEvent->Set();
                }
            }));

            auto staticHeader = safe_cast<xaml_primitives::PivotHeaderPanel^>(TreeHelper::GetVisualChildByName(pivot, L"StaticHeader"));
            secondItem = static_cast<xaml_controls::ContentControl^>(staticHeader->Children->GetAt(1));
            thirdItem = static_cast<xaml_controls::ContentControl^>(staticHeader->Children->GetAt(2));
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->Tap(secondItem);
        TestServices::WindowHelper->WaitForIdle();
        finalViewChangedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, pivot->SelectedIndex);
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->Tap(thirdItem);
        TestServices::WindowHelper->WaitForIdle();

        finalViewChangedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(2, pivot->SelectedIndex);
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->Tap(thirdItemButton);
        TestServices::WindowHelper->WaitForIdle();

        finalViewChangedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, pivot->SelectedIndex);
        });
    }

    void PivotIntegrationTests::CanScrollOuterScrollViewerWithMouseWheel()
    {
        TestCleanupWrapper cleanup;
        // This is the outer ScrollViewer. The inner ScrollViewer is inside Pivot's template.
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        auto loadedEvent = std::make_shared<Event>();
        auto scrollViewerLoaded = CreateSafeEventRegistration(xaml_controls::ScrollViewer, Loaded);

        RunOnUIThread([&]()
        {
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^> (xaml_markup::XamlReader::Load(
                LR"(<ScrollViewer Height='400' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                      <Pivot Height='800'>
                        <PivotItem Header='Item #1'>Content of item #1</PivotItem>
                      </Pivot>
                    </ScrollViewer>)"));

            scrollViewerLoaded.Attach(scrollViewer, [loadedEvent]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = scrollViewer;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Scroll the mouse wheel and then validate the outer ScrollViewer scrolls.");
        for (int i = 0; i < 2; ++i)
        {
            TestServices::InputHelper->MoveMouse(scrollViewer);
            TestServices::InputHelper->ScrollMouseWheel(scrollViewer, -10);
            TestServices::WindowHelper->WaitForIdle();
        }

        RunOnUIThread([&]()
        {
            VERIFY_IS_GREATER_THAN(scrollViewer->VerticalOffset, 0.0);
        });
    }

    void PivotIntegrationTests::CanScrollOuterScrollViewerWithFocusChange()
    {
        TestCleanupWrapper cleanup;

        // This is the outer ScrollViewer. The inner ScrollViewer is inside Pivot's template.
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        auto loadedEvent = std::make_shared<Event>();
        auto scrollViewerLoaded = CreateSafeEventRegistration(xaml_controls::ScrollViewer, Loaded);

        RunOnUIThread([&]()
        {
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^> (xaml_markup::XamlReader::Load(
                LR"(<ScrollViewer Height='400' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                      <Pivot Height='800'>
                        <PivotItem Header='Item #1'>
                          <Button x:Name='button' VerticalAlignment='Bottom'>Scroll to see me!</Button>
                        </PivotItem>
                      </Pivot>
                    </ScrollViewer>)"));

            scrollViewerLoaded.Attach(scrollViewer, [loadedEvent]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = scrollViewer;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            safe_cast<xaml_controls::Button^>(scrollViewer->FindName(L"button"))->Focus(xaml::FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_GREATER_THAN(scrollViewer->VerticalOffset, 0.0);
        });
    }

    void PivotIntegrationTests::PivotInSplitViewWithContentControlDoesNotCrash()
    {
        TestCleanupWrapper cleanup;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::SplitView, Loaded);

        RunOnUIThread([&]()
        {
            auto splitView = safe_cast<xaml_controls::SplitView^>(xaml_markup::XamlReader::Load(
                    LR"(<SplitView
                            xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                            Width='300'
                            IsPaneOpen='True'>
                        <SplitView.Template>
                            <ControlTemplate TargetType='SplitView'>
                                <Grid Background='{TemplateBinding Background}'>
                                <VisualStateManager.VisualStateGroups>
                                    <VisualStateGroup x:Name='DisplayModeStates'>
                                        <VisualState x:Name='Closed' />
                                        <VisualState x:Name='OpenOverlayLeft'>
                                            <Storyboard>
                                                <ObjectAnimationUsingKeyFrames Storyboard.TargetName='PaneRoot' Storyboard.TargetProperty='Visibility'>
                                                    <DiscreteObjectKeyFrame KeyTime='0:0:0' Value='Visible' />
                                                </ObjectAnimationUsingKeyFrames>
                                                <ObjectAnimationUsingKeyFrames Storyboard.TargetName='LightDismissLayer' Storyboard.TargetProperty='Visibility'>
                                                    <DiscreteObjectKeyFrame KeyTime='0:0:0' Value='Visible' />
                                                </ObjectAnimationUsingKeyFrames>
                                            </Storyboard>
                                        </VisualState>
                                    </VisualStateGroup>
                                </VisualStateManager.VisualStateGroups>
                                    <Grid x:Name='ContentRoot'>
                                        <Border Child='{TemplateBinding Content}'/>
                                        <Border x:Name='LightDismissLayer' Background='Transparent' Visibility='Collapsed'/>
                                    </Grid>
                                    <ContentControl
                                            x:Name='PaneRoot'
                                            AutomationProperties.Name='{TemplateBinding AutomationProperties.Name}'
                                            Background='{TemplateBinding PaneBackground}'
                                            Width='{Binding RelativeSource={RelativeSource TemplatedParent}, Path=TemplateSettings.OpenPaneLength}'
                                            HorizontalAlignment='Left'
                                            Visibility='Collapsed'>
                                        <ContentControl.RenderTransform>
                                            <CompositeTransform x:Name='PaneTransform' />
                                        </ContentControl.RenderTransform>
                                        <Grid>
                                            <Grid.Clip>
                                                <RectangleGeometry x:Name='PaneClipRectangle'>
                                                    <RectangleGeometry.Transform>
                                                        <CompositeTransform x:Name='PaneClipRectangleTransform'/>
                                                    </RectangleGeometry.Transform>
                                                </RectangleGeometry>
                                            </Grid.Clip>
                                            <Border Child='{TemplateBinding Pane}'/>
                                        </Grid>
                                    </ContentControl>
                                </Grid>
                            </ControlTemplate>
                        </SplitView.Template>
                        <SplitView.Pane>
                            <Pivot>
                                <PivotItem>
                                    <PivotItem.Header>
                                        <TextBlock>Hello</TextBlock>
                                    </PivotItem.Header>
                                </PivotItem>
                                <PivotItem>
                                    <PivotItem.Header>
                                        <TextBlock>Goodbye</TextBlock>
                                    </PivotItem.Header>
                                </PivotItem>
                            </Pivot>
                        </SplitView.Pane>
                    </SplitView>)"));

            loadedRegistration.Attach(splitView, [loadedEvent]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = splitView;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void PivotIntegrationTests::CanChangeGhostPivotHeaderItemTemplateWithoutCrash()
    {
        TestCleanupWrapper cleanup;

        auto loadedEvent = std::make_shared<Event>();
        auto selectionChangedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, Loaded);
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, SelectionChanged);

        xaml_controls::Pivot^ pivot = nullptr;

        RunOnUIThread([&]()
        {
            pivot = safe_cast<xaml_controls::Pivot^>(xaml_markup::XamlReader::Load(
                    LR"(<Pivot xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Width='200'>
                            <PivotItem Header='Item 1'>Item 1</PivotItem>
                            <PivotItem Header='Item 2'>Item 2</PivotItem>
                            <PivotItem Header='Item 3'>Item 3</PivotItem>
                        </Pivot>)"));

            loadedRegistration.Attach(pivot, [loadedEvent]() { loadedEvent->Set(); });
            selectionChangedRegistration.Attach(pivot, [selectionChangedEvent]() { selectionChangedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = pivot;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto newTemplate = safe_cast<xaml_controls::ControlTemplate^>(xaml_markup::XamlReader::Load(
                    LR"(<ControlTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' TargetType='PivotHeaderItem'>
                            <Grid>
                                <TextBlock Text='{TemplateBinding Content}' />
                            </Grid>
                        </ControlTemplate>)"));

            auto ghostItem = FindPivotHeaderItemWithContent(pivot, L"Item 3");
            ghostItem->Template = newTemplate;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            pivot->SelectedIndex = 1;
        });

        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void PivotIntegrationTests::VerifyFocusFollowerPosition()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Pivot^ pivot = nullptr;
        wf::Rect pivotItemBounds;
        wf::Rect focusFollowerBounds;

        RunOnUIThread([&]()
        {
            pivot = safe_cast<xaml_controls::Pivot^>(xaml_markup::XamlReader::Load(
                LR"(<Pivot xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                               HeaderFocusVisualPlacement="SelectedItemHeader">
                            <PivotItem Header='Item 1'>Item 1</PivotItem>
                            <PivotItem Header='Item 2222'>Item 2</PivotItem>
                            <PivotItem Header='Item 33333333'>Item 3</PivotItem>
                        </Pivot>)"));

            TestServices::WindowHelper->WindowContent = pivot;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            pivot->Focus(xaml::FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            pivotItemBounds = ControlHelper::GetBounds(FindPivotHeaderItemWithContent(pivot, L"Item 1"));
            auto focusFollower = TreeHelper::GetVisualChildByName(pivot, L"FocusFollower");
            VERIFY_IS_NOT_NULL(focusFollower);
            focusFollowerBounds = ControlHelper::GetBounds(focusFollower);

            LOG_OUTPUT(L"Verify focusFollower's bounds are aligned with headerItem's bounds");
            VERIFY_ARE_EQUAL(pivotItemBounds.X, focusFollowerBounds.X);
            VERIFY_ARE_EQUAL(pivotItemBounds.Width, focusFollowerBounds.Width);
        });

        LOG_OUTPUT(L"Move to the next PivotItem with the right arrow on keyboard.");
        TestServices::KeyboardHelper->Right();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(pivot->SelectedIndex, 1);
            pivotItemBounds = ControlHelper::GetBounds(FindPivotHeaderItemWithContent(pivot, L"Item 2222"));
            auto focusFollower = TreeHelper::GetVisualChildByName(pivot, L"FocusFollower");
            VERIFY_IS_NOT_NULL(focusFollower);
            focusFollowerBounds = ControlHelper::GetBounds(focusFollower);

            LOG_OUTPUT(L"Verify focusFollower's bounds are aligned with headerItem's bounds");
            VERIFY_ARE_EQUAL(pivotItemBounds.X, focusFollowerBounds.X);
            VERIFY_ARE_EQUAL(pivotItemBounds.Width, focusFollowerBounds.Width);
        });
    }

    xaml_controls::Pivot^ PivotIntegrationTests::CreatePivot(PivotContent content)
    {
        xaml_controls::Pivot^ pivot = nullptr;

        RunOnUIThread([&]()
        {
            switch (content)
            {
            case PivotContent::NoContent:
                pivot = safe_cast<xaml_controls::Pivot^> (xaml_markup::XamlReader::Load(
                    L"<Pivot xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Title='Test Pivot' Width='150'>"
                    L"</Pivot>"));
                break;

            case PivotContent::TextBlockContent:
                pivot = safe_cast<xaml_controls::Pivot^> (xaml_markup::XamlReader::Load(
                    L"<Pivot xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Title='Test Pivot' Width='150'>"
                    L"  <PivotItem Header='PItem1'>"
                    L"      <StackPanel Margin='5'>"
                    L"          <TextBlock TextWrapping='NoWrap' Text='Test SampleText for PivotItem1' />"
                    L"      </StackPanel>"
                    L"  </PivotItem>"
                    L"  <PivotItem Header='PItem2'>"
                    L"      <StackPanel Margin='5'>"
                    L"          <TextBlock TextWrapping='NoWrap' Text='Test SampleText for PivotItem2' />"
                    L"      </StackPanel>"
                    L"  </PivotItem>"
                    L"  <PivotItem Header='PItem3'>"
                    L"      <StackPanel Margin='5'>"
                    L"          <TextBlock TextWrapping='NoWrap' Text='Test SampleText for PivotItem3' />"
                    L"      </StackPanel>"
                    L"  </PivotItem>"
                    L"</Pivot>"));
                break;

            case PivotContent::TextBlockItemTemplateContent:
                pivot = safe_cast<xaml_controls::Pivot^> (xaml_markup::XamlReader::Load(
                    L"<Pivot xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Title='Test Pivot' Width='150'>"
                    L"  <Pivot.HeaderTemplate>"
                    L"    <DataTemplate>"
                    L"      <TextBlock Text='Header' />"
                    L"    </DataTemplate>"
                    L"  </Pivot.HeaderTemplate>"
                    L"  <Pivot.ItemTemplate>"
                    L"    <DataTemplate>"
                    L"      <StackPanel />"
                    L"    </DataTemplate>"
                    L"  </Pivot.ItemTemplate>"
                    L"</Pivot>"));

                {
                    wfc::IVector<Platform::String^>^ itemsCollection = ref new Platform::Collections::Vector<Platform::String^>;

                    // The data template doesn't actually depend on anything inside of its items,
                    // so we'll just populate the Pivot with placeholder items to cause the item template
                    // to be realized.
                    for (int i = 0; i < 3; i++)
                    {
                        itemsCollection->Append(L"Item Content");
                    }

                    pivot->ItemsSource = itemsCollection;
                }
                break;

            case PivotContent::TextBoxContent:
                pivot = safe_cast<xaml_controls::Pivot^> (xaml_markup::XamlReader::Load(
                    L"<Pivot xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Title='Test Pivot' Width='150'>"
                    L"  <PivotItem Header='PItem1'>"
                    L"      <TextBox TextWrapping='NoWrap' Text='Test SampleText for PivotItem1' />"
                    L"  </PivotItem>"
                    L"  <PivotItem Header='PItem2'>"
                    L"      <TextBox TextWrapping='NoWrap' Text='Test SampleText for PivotItem2' />"
                    L"  </PivotItem>"
                    L"  <PivotItem Header='PItem3'>"
                    L"      <TextBox TextWrapping='NoWrap' Text='Test SampleText for PivotItem3' />"
                    L"  </PivotItem>"
                    L"</Pivot>"));
                break;

            case PivotContent::TextBoxExtraContent:
                pivot = safe_cast<xaml_controls::Pivot^> (xaml_markup::XamlReader::Load(
                    L"<Pivot xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Title='Test Pivot' Width='150'>"
                    L"  <PivotItem Header='PItem1'>"
                    L"      <TextBox TextWrapping='NoWrap' Text='Test SampleText for PivotItem1' />"
                    L"  </PivotItem>"
                    L"  <PivotItem Header='PItem2'>"
                    L"      <TextBox TextWrapping='NoWrap' Text='Test SampleText for PivotItem2' />"
                    L"  </PivotItem>"
                    L"  <PivotItem Header='PItem3'>"
                    L"      <TextBox TextWrapping='NoWrap' Text='Test SampleText for PivotItem3' />"
                    L"  </PivotItem>"
                    L"  <PivotItem Header='PItem4'>"
                    L"      <TextBox TextWrapping='NoWrap' Text='Test SampleText for PivotItem4' />"
                    L"  </PivotItem>"
                    L"  <PivotItem Header='PItem5'>"
                    L"      <TextBox TextWrapping='NoWrap' Text='Test SampleText for PivotItem5' />"
                    L"  </PivotItem>"
                    L"</Pivot>"));
                break;

            case PivotContent::ListViewContent:
                pivot = safe_cast<xaml_controls::Pivot^> (xaml_markup::XamlReader::Load(
                    L"<Pivot xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Title='Test Pivot'>"
                    L"  <PivotItem Header='PItem1'>"
                    L"      <ListView>"
                    L"          <ListViewItem Content='List item 1' />"
                    L"          <ListViewItem Content='List item 2' />"
                    L"          <ListViewItem Content='List item 3' />"
                    L"          <ListViewItem Content='List item 4' />"
                    L"          <ListViewItem Content='List item 5' />"
                    L"          <ListViewItem Content='List item 6' />"
                    L"          <ListViewItem Content='List item 7' />"
                    L"          <ListViewItem Content='List item 8' />"
                    L"          <ListViewItem Content='List item 9' />"
                    L"          <ListViewItem Content='List item 10' />"
                    L"          <ListViewItem Content='List item 11' />"
                    L"          <ListViewItem Content='List item 12' />"
                    L"          <ListViewItem Content='List item 13' />"
                    L"          <ListViewItem Content='List item 14' />"
                    L"          <ListViewItem Content='List item 15' />"
                    L"          <ListViewItem Content='List item 16' />"
                    L"          <ListViewItem Content='List item 17' />"
                    L"          <ListViewItem Content='List item 18' />"
                    L"          <ListViewItem Content='List item 19' />"
                    L"      </ListView>"
                    L"  </PivotItem>"
                    L"  <PivotItem Header='PItem2'>"
                    L"      <ListView>"
                    L"          <ListViewItem Content='List item 1' />"
                    L"          <ListViewItem Content='List item 2' />"
                    L"          <ListViewItem Content='List item 3' />"
                    L"          <ListViewItem Content='List item 4' />"
                    L"          <ListViewItem Content='List item 5' />"
                    L"          <ListViewItem Content='List item 6' />"
                    L"          <ListViewItem Content='List item 7' />"
                    L"          <ListViewItem Content='List item 8' />"
                    L"          <ListViewItem Content='List item 9' />"
                    L"          <ListViewItem Content='List item 10' />"
                    L"          <ListViewItem Content='List item 11' />"
                    L"          <ListViewItem Content='List item 12' />"
                    L"          <ListViewItem Content='List item 13' />"
                    L"          <ListViewItem Content='List item 14' />"
                    L"          <ListViewItem Content='List item 15' />"
                    L"          <ListViewItem Content='List item 16' />"
                    L"          <ListViewItem Content='List item 17' />"
                    L"          <ListViewItem Content='List item 18' />"
                    L"          <ListViewItem Content='List item 19' />"
                    L"      </ListView>"
                    L"  </PivotItem>"
                    L"  <PivotItem Header='PItem3'>"
                    L"      <ListView>"
                    L"          <ListViewItem Content='List item 1' />"
                    L"          <ListViewItem Content='List item 2' />"
                    L"          <ListViewItem Content='List item 3' />"
                    L"          <ListViewItem Content='List item 4' />"
                    L"          <ListViewItem Content='List item 5' />"
                    L"          <ListViewItem Content='List item 6' />"
                    L"          <ListViewItem Content='List item 7' />"
                    L"          <ListViewItem Content='List item 8' />"
                    L"          <ListViewItem Content='List item 9' />"
                    L"          <ListViewItem Content='List item 10' />"
                    L"          <ListViewItem Content='List item 11' />"
                    L"          <ListViewItem Content='List item 12' />"
                    L"          <ListViewItem Content='List item 13' />"
                    L"          <ListViewItem Content='List item 14' />"
                    L"          <ListViewItem Content='List item 15' />"
                    L"          <ListViewItem Content='List item 16' />"
                    L"          <ListViewItem Content='List item 17' />"
                    L"          <ListViewItem Content='List item 18' />"
                    L"          <ListViewItem Content='List item 19' />"
                    L"      </ListView>"
                    L"  </PivotItem>"
                    L"</Pivot>"));

                break;

            case PivotContent::GridViewContent:
                pivot = safe_cast<xaml_controls::Pivot^> (xaml_markup::XamlReader::Load(
                    L"<Pivot xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Title='Test Pivot'>"
                    L"  <PivotItem Header='PItem1'>"
                    L"      <GridView>"
                    L"          <GridViewItem Content='Grid item 1' />"
                    L"          <GridViewItem Content='Grid item 2' />"
                    L"          <GridViewItem Content='Grid item 3' />"
                    L"          <GridViewItem Content='Grid item 4' />"
                    L"          <GridViewItem Content='Grid item 5' />"
                    L"          <GridViewItem Content='Grid item 6' />"
                    L"          <GridViewItem Content='Grid item 7' />"
                    L"          <GridViewItem Content='Grid item 8' />"
                    L"          <GridViewItem Content='Grid item 9' />"
                    L"          <GridViewItem Content='Grid item 10' />"
                    L"          <GridViewItem Content='Grid item 11' />"
                    L"          <GridViewItem Content='Grid item 12' />"
                    L"          <GridViewItem Content='Grid item 13' />"
                    L"          <GridViewItem Content='Grid item 14' />"
                    L"          <GridViewItem Content='Grid item 15' />"
                    L"          <GridViewItem Content='Grid item 16' />"
                    L"          <GridViewItem Content='Grid item 17' />"
                    L"          <GridViewItem Content='Grid item 18' />"
                    L"          <GridViewItem Content='Grid item 19' />"
                    L"      </GridView>"
                    L"  </PivotItem>"
                    L"  <PivotItem Header='PItem2'>"
                    L"      <GridView>"
                    L"          <GridViewItem Content='Grid item 1' />"
                    L"          <GridViewItem Content='Grid item 2' />"
                    L"          <GridViewItem Content='Grid item 3' />"
                    L"          <GridViewItem Content='Grid item 4' />"
                    L"          <GridViewItem Content='Grid item 5' />"
                    L"          <GridViewItem Content='Grid item 6' />"
                    L"          <GridViewItem Content='Grid item 7' />"
                    L"          <GridViewItem Content='Grid item 8' />"
                    L"          <GridViewItem Content='Grid item 9' />"
                    L"          <GridViewItem Content='Grid item 10' />"
                    L"          <GridViewItem Content='Grid item 11' />"
                    L"          <GridViewItem Content='Grid item 12' />"
                    L"          <GridViewItem Content='Grid item 13' />"
                    L"          <GridViewItem Content='Grid item 14' />"
                    L"          <GridViewItem Content='Grid item 15' />"
                    L"          <GridViewItem Content='Grid item 16' />"
                    L"          <GridViewItem Content='Grid item 17' />"
                    L"          <GridViewItem Content='Grid item 18' />"
                    L"          <GridViewItem Content='Grid item 19' />"
                    L"      </GridView>"
                    L"  </PivotItem>"
                    L"  <PivotItem Header='PItem3'>"
                    L"      <GridView>"
                    L"          <GridViewItem Content='Grid item 1' />"
                    L"          <GridViewItem Content='Grid item 2' />"
                    L"          <GridViewItem Content='Grid item 3' />"
                    L"          <GridViewItem Content='Grid item 4' />"
                    L"          <GridViewItem Content='Grid item 5' />"
                    L"          <GridViewItem Content='Grid item 6' />"
                    L"          <GridViewItem Content='Grid item 7' />"
                    L"          <GridViewItem Content='Grid item 8' />"
                    L"          <GridViewItem Content='Grid item 9' />"
                    L"          <GridViewItem Content='Grid item 10' />"
                    L"          <GridViewItem Content='Grid item 11' />"
                    L"          <GridViewItem Content='Grid item 12' />"
                    L"          <GridViewItem Content='Grid item 13' />"
                    L"          <GridViewItem Content='Grid item 14' />"
                    L"          <GridViewItem Content='Grid item 15' />"
                    L"          <GridViewItem Content='Grid item 16' />"
                    L"          <GridViewItem Content='Grid item 17' />"
                    L"          <GridViewItem Content='Grid item 18' />"
                    L"          <GridViewItem Content='Grid item 19' />"
                    L"      </GridView>"
                    L"  </PivotItem>"
                    L"</Pivot>"));

                break;

            default:
                VERIFY_FAIL(L"Unsupported PivotContent type.");
            }

            VERIFY_ARE_EQUAL(pivot->SelectedIndex, 0);
            switch (content)
            {
            case PivotContent::NoContent:
                VERIFY_ARE_EQUAL(pivot->Items->Size, 0u);
                break;
            case PivotContent::TextBoxExtraContent:
                VERIFY_ARE_EQUAL(pivot->Items->Size, 5u);
                break;
            default:
                VERIFY_ARE_EQUAL(pivot->Items->Size, 3u);
            }
        });

        return pivot;
    }

    xaml_controls::Pivot^ PivotIntegrationTests::SetupPivotTest(PivotContent content)
    {
        xaml_controls::Pivot^ pivot = nullptr;

        auto pivotItemLoadedEvent = std::make_shared<Event>();
        auto pivotItemLoadingEvent = std::make_shared<Event>();

        auto pivotItemLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, PivotItemLoaded);
        auto pivotItemLoadingRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, PivotItemLoading);

        RunOnUIThread([&]()
        {
            pivot = CreatePivot(content);

            pivotItemLoadedRegistration.Attach(pivot,
                ref new wf::TypedEventHandler<xaml_controls::Pivot^, xaml_controls::PivotItemEventArgs^>([pivotItemLoadedEvent](xaml_controls::Pivot^ sender, xaml_controls::PivotItemEventArgs^ args)
            {
                LOG_OUTPUT(L"PivotItemLoaded raised.");

                VERIFY_IS_NOT_NULL(args->Item);

                pivotItemLoadedEvent->Set();
            }));

            pivotItemLoadingRegistration.Attach(pivot,
                ref new wf::TypedEventHandler<xaml_controls::Pivot^, xaml_controls::PivotItemEventArgs^>([pivotItemLoadingEvent](xaml_controls::Pivot^ sender, xaml_controls::PivotItemEventArgs^ args)
            {
                LOG_OUTPUT(L"PivotItemLoading raised.");

                VERIFY_IS_NOT_NULL(args->Item);

                pivotItemLoadingEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = pivot;
        });

        if (content != PivotContent::NoContent)
        {
            pivotItemLoadedEvent->WaitForDefault();
            pivotItemLoadingEvent->WaitForDefault();

            LOG_OUTPUT(L"Clicking on pivot to reset focus state");
            test_infra::TestServices::InputHelper->LeftMouseClick(pivot);
        }

        TestServices::WindowHelper->WaitForIdle();

        return pivot;
    }

    void PivotIntegrationTests::DoesNotAnimateWhenAnimationsAreDisabled()
    {
        TestCleanupWrapper cleanup;

        // MOCK10_REMOVAL Disable animation

        xaml_controls::Pivot^ pivot = nullptr;
        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, Loaded);
        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, SelectionChanged);
        int currentSelectedIndex = 0;

        xaml::FrameworkElement^ secondHeaderItem = nullptr;

        // Initialize the pivot
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Adding a pivot control to the test page.");
            pivot = safe_cast<xaml_controls::Pivot^>(xaml_markup::XamlReader::Load(
                LR"(<Pivot xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Width='200'>
                        <PivotItem Header='Item 1'>Item 1</PivotItem>
                        <PivotItem Header='Item 2'>Item 2</PivotItem>
                        <PivotItem Header='Item 3'>Item 3</PivotItem>
                    </Pivot>)"));

            loadedRegistration.Attach(pivot, [&]() { loadedEvent->Set(); });
            selectionChangedRegistration.Attach(pivot, [selectionChangedEvent]() { selectionChangedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = pivot;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        auto storyboardMonitor = ref new StoryboardMonitorWrapper();
        storyboardMonitor->AttachStartedHandler(
            [&](xaml_animation::Storyboard^ storyboard, xaml::UIElement^ target)
        {
            LOG_OUTPUT(L"Inside the Storyboard's callback.");
            auto storyBoardsTimeSpan = storyboard->Duration.TimeSpan.Duration;

            for (unsigned int i = 0; i < storyboard->Children->Size; i++) // checking all durations for all Keyframes inside.
            {
                auto child = storyboard->Children->GetAt(i);
                auto childsTimeSpan = child->Duration.TimeSpan.Duration;

                if (auto doubleAnimation = dynamic_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(child))
                {
                    auto KeyFrames = doubleAnimation->KeyFrames;
                    for (unsigned int j = 0; j < KeyFrames->Size; j++)
                    {
                        auto span = KeyFrames->GetAt(j)->KeyTime.TimeSpan.Duration;
                        VERIFY_ARE_EQUAL(0L, span, L"Keyframe duration should be equal to zero.");
                    }
                }

                VERIFY_ARE_EQUAL(0L, childsTimeSpan, L"DoubleAnimation should have zero duration.");
            }

            VERIFY_ARE_EQUAL(0L, storyBoardsTimeSpan, L"StoryBoard's animation should have zero duration.");
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Programatically changing the selected item.");
            currentSelectedIndex = pivot->SelectedIndex;
            VERIFY_IS_NOT_NULL(pivot->Items);
            pivot->SelectedItem = pivot->Items->GetAt(++currentSelectedIndex);
            pivot->SelectedIndex = currentSelectedIndex;
        });

        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Verifying the change in the selected item.");
            VERIFY_ARE_EQUAL(pivot->SelectedItem, pivot->Items->GetAt(currentSelectedIndex));
            VERIFY_ARE_EQUAL(pivot->SelectedIndex, currentSelectedIndex);
        });

        RunOnUIThread([&]
        {
            LOG_OUTPUT(L"Tapping on the third item.");
            auto headerPanel = safe_cast<xaml_controls::Panel^>(TreeHelper::GetVisualChildByName(pivot, L"Header"));
            secondHeaderItem = safe_cast<xaml::FrameworkElement^>(headerPanel->Children->GetAt(1));
        });

        TestServices::InputHelper->Tap(secondHeaderItem);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]
        {
            LOG_OUTPUT(L"Verifying the tap worked fine.");
            auto secondHeaderItemPosition = secondHeaderItem->TransformToVisual(nullptr)->TransformPoint(wf::Point(0, 0));
            LOG_OUTPUT(L"Second Header Item Position: %f, %f", secondHeaderItemPosition.X, secondHeaderItemPosition.Y);

            VERIFY_IS_GREATER_THAN(secondHeaderItemPosition.X, 0.f);
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Focus the pivot, then use the left and right arrow keys to navigate.");
            safe_cast<xaml_controls::Control^>(TreeHelper::GetVisualChildByName(pivot, L"HeaderClipper"))->Focus(xaml::FocusState::Programmatic);
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::KeyboardHelper->Right();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(2, pivot->SelectedIndex);
        });

        TestServices::KeyboardHelper->Right();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, pivot->SelectedIndex);
        });

        TestServices::KeyboardHelper->Left();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(2, pivot->SelectedIndex);
        });
    }

    void PivotIntegrationTests::TransitionReentrancyOnDirtyRelayout()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Pivot^ pivot = nullptr;
        xaml::FrameworkElement^ secondHeaderItem = nullptr;
        double originalRightMargin = 0;

        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::Pivot, SelectionChanged);

        RunOnUIThread([&]
        {
            pivot = safe_cast<xaml_controls::Pivot^> (xaml_markup::XamlReader::Load(
                L"<Pivot xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                L"  <PivotItem Header='item1'/>"
                L"  <PivotItem Header='item2'/>"
                L"  <PivotItem Header='item3'/>"
                L"</Pivot>"));

            TestServices::WindowHelper->WindowContent = pivot;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]
        {           
            selectionChangedRegistration.Attach(pivot, [&selectionChangedEvent, &pivot, &originalRightMargin]()
            {
                originalRightMargin = pivot->Margin.Right;
                pivot->Margin = xaml::Thickness({ pivot->Margin.Left, pivot->Margin.Top, pivot->Margin.Right + 1, pivot->Margin.Bottom });
                pivot->UpdateLayout();
                selectionChangedEvent->Set(); 
            });
            auto headerPanel = safe_cast<xaml_controls::Panel^>(TreeHelper::GetVisualChildByName(pivot, L"StaticHeader"));
            secondHeaderItem = safe_cast<xaml::FrameworkElement^>(headerPanel->Children->GetAt(1));
        });

        TestServices::InputHelper->Tap(secondHeaderItem);
        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]
        {
            VERIFY_IS_GREATER_THAN(pivot->Margin.Right, originalRightMargin);
        });
    }


    xaml_primitives::PivotHeaderItem^ PivotIntegrationTests::FindPivotHeaderItemWithContent(xaml_controls::Pivot^ parentPivot, Platform::String^ content)
    {
        std::queue<xaml::DependencyObject^> objectQueue;

        // We'll do a breadth-first search to avoid needing a helper method for recursion
        // that would need to return a DependencyObject.
        objectQueue.push(parentPivot);

        while (!objectQueue.empty())
        {
            xaml::DependencyObject^ currentObject = objectQueue.front();
            objectQueue.pop();

            xaml_primitives::PivotHeaderItem^ headerItem = dynamic_cast<xaml_primitives::PivotHeaderItem^>(currentObject);

            if (headerItem != nullptr && dynamic_cast<Platform::String^>(headerItem->Content) == content)
            {
                return headerItem;
            }

            for (int i = 0; i < xaml_media::VisualTreeHelper::GetChildrenCount(currentObject); i++)
            {
                objectQueue.push(xaml_media::VisualTreeHelper::GetChild(currentObject, i));
            }
        }

        return nullptr;
    }


    void PivotIntegrationTests::PressKeyboardLogicalRight(bool isRTL)
    {
        if (isRTL)
        {
            TestServices::KeyboardHelper->Left();
        }
        else
        {
            TestServices::KeyboardHelper->Right();
        }
    }

    void PivotIntegrationTests::PressKeyboardLogicalLeft(bool isRTL)
    {
        if (isRTL)
        {
            TestServices::KeyboardHelper->Right();
        }
        else
        {
            TestServices::KeyboardHelper->Left();
        }
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::Pivot
