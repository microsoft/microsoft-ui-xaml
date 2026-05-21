// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <TestEvent.h>
#include "GroupedDataSource.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace SemanticZoom {

    class SemanticZoomIntegrationTests : public WEX::TestClass<SemanticZoomIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(SemanticZoomIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"Hosting:Mode", L"UAP") // DCPP NoCoreWindow mode - SemanticZoom tests fail get stuck "Waiting for BuildTreeService to finish..."

            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"309fb554-f012-4ac9-bcf1-833e4a372493;375cd7bd-e448-4315-b2a1-bc02d75b0c4f;3302e9ea-a838-4b3c-9784-254de755a0bd")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a SemanticZoom.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a SemanticZoom from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanZoomOutToKeysList)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully zoom out to the view showing list of group keys.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanZoomInToKey)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully zoom in to a list of items grouped by a key.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetAndGetProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully set and get SemanticZom specific properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SezoBasicWithDefaultTemplateGrouped)
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SezoBasicWithSezoTemplateGrouped)
            TEST_METHOD_PROPERTY(L"Description", L"Validates Sezo with Sezo template")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        TEST_METHOD(SezoBasicWithDefaultTemplateUnGrouped)

        BEGIN_TEST_METHOD(SezoBasicWithSezoTemplateUnGrouped)
            TEST_METHOD_PROPERTY(L"Description", L"Validates Sezo with Sezo template")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SezoZovItemHitTestableWhenGroupIsEmpty)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we dont stomp on IsHitTestVisible property on Zoomed out view gridview item if group is empty")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateToggleActiveViewWithKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Validates Sezo can be activated with Enter key on header or gamepad A")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateToggleActiveViewWithGamePad)
            TEST_METHOD_PROPERTY(L"Description", L"Validates Sezo can be activated with Enter key on header or gamepad A")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()


        BEGIN_TEST_METHOD(ValidateToggleActiveViewWithHeaderTap)
            TEST_METHOD_PROPERTY(L"Description", L"Validates Sezo can be activated with tapping on header")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SwitchActiveViewUsingUiaInvokePatternOnListView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ListView in Semantic Zoom can switch views using UIAutomation's Invoke Pattern.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SwitchActiveViewUsingUiaInvokePatternOnGridView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that GridView in Semantic Zoom can switch views using UIAutomation's Invoke Pattern.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetSkipFocusSubtreeOnEnteringVisualTree)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure SkipFocusSubTree is set correctly when SemanticZoom leaves and enter the visual tree.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

    private:
        xaml_controls::SemanticZoom^ SetupSemanticZoomTest();
        void DoToggleActiveView(
            xaml_controls::SemanticZoom^ semanticZoom,
            std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> viewChangeStartedEvent,
            std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> viewChangeCompletedEvent);

        enum class SemanticZoomTemplate
        {
            Default,
            SemanticZoom_WinBlue
        };

        xaml_controls::SemanticZoom^ CreateSemanticZoomWithTemplateAndPopulate(SemanticZoomTemplate sTemplate, bool grouped, xaml_controls::ListView^& sezoZoomedInView, xaml_controls::GridView^& sezoZoomedOutView);
        xaml_controls::SemanticZoom^ CreateSemanticZoomWithTemplateAndPopulate(SemanticZoomTemplate sTemplate, bool grouped, xaml_controls::ListView^& sezoZoomedInView, xaml_controls::GridView^& sezoZoomedOutView, int numGroups, int numItemsPerGroup);
        void SemanticZoomIntegrationTests::SezoBasicWithTemplate(SemanticZoomTemplate sTemplate, bool grouped);

        Platform::Collections::Vector<Object^>^  GetGroupedData(int groupCount, int itemCount)
        {
            auto data = ref new Platform::Collections::Vector<Object^>();

            for (int groupIndex = 0; groupIndex < groupCount; groupIndex++)
            {
                auto groupHeader = ref new GroupedHeader("g" + groupIndex.ToString());
                for (int itemIndex = 0; itemIndex < itemCount; itemIndex++)
                {
                    groupHeader->Append("g" + groupIndex.ToString() + ": item" + itemIndex.ToString());
                }

                data->Append(groupHeader);
            }

            return data;
        }

        String^ GetSezoWithTemplate(SemanticZoomTemplate sTemplate);

        String^ sezoTemplate = ref new String(
        L"   <Style TargetType='SemanticZoom'>                                                                                                                            "\
        L"       <Setter Property='Padding' Value='3' />                                                                                                                                 "\
        L"       <Setter Property='Background' Value='Transparent' />                                                                                                                    "\
        L"       <Setter Property='BorderThickness' Value='0' />                                                                                                                         "\
        L"       <Setter Property='IsTabStop' Value='False' />                                                                                                                           "\
        L"       <Setter Property='TabNavigation' Value='Once' />                                                                                                                        "\
        L"       <Setter Property='ScrollViewer.HorizontalScrollMode' Value='Enabled' />                                                                                                 "\
        L"       <Setter Property='ScrollViewer.IsHorizontalRailEnabled' Value='False' />                                                                                                "\
        L"       <Setter Property='ScrollViewer.VerticalScrollMode' Value='Disabled' />                                                                                                  "\
        L"       <Setter Property='ScrollViewer.IsVerticalRailEnabled' Value='False' />                                                                                                  "\
        L"       <Setter Property='Template'>                                                                                                                                            "\
        L"           <Setter.Value>                                                                                                                                                      "\
        L"               <ControlTemplate TargetType='SemanticZoom'>                                                                                                                     "\
        L"                   <Grid>                                                                                                                                                      "\
        L"                       <VisualStateManager.VisualStateGroups>                                                                                                                  "\
        L"                           <VisualStateGroup x:Name='SemanticZoomStates'>                                                                                                      "\
        L"                               <VisualState x:Name='ZoomInView'>                                                                                                               "\
        L"                                   <Storyboard>                                                                                                                                "\
        L"                                       <FadeOutThemeAnimation TargetName='ZoomedOutPresenter' />                                                                               "\
        L"                                       <FadeInThemeAnimation TargetName='ZoomedInPresenter' />                                                                                 "\
        L"                                   </Storyboard>                                                                                                                               "\
        L"                               </VisualState>                                                                                                                                  "\
        L"                               <VisualState x:Name='ZoomOutView'>                                                                                                              "\
        L"                                   <Storyboard>                                                                                                                                "\
        L"                                       <FadeOutThemeAnimation TargetName='ZoomedInPresenter' />                                                                                "\
        L"                                       <FadeInThemeAnimation TargetName='ZoomedOutPresenter' />                                                                                "\
        L"                                   </Storyboard>                                                                                                                               "\
        L"                               </VisualState>                                                                                                                                  "\
        L"                           </VisualStateGroup>                                                                                                                                 "\
        L"                           <VisualStateGroup x:Name='ZoomOutButtonStates'>                                                                                                     "\
        L"                               <VisualStateGroup.Transitions>                                                                                                                  "\
        L"                                   <VisualTransition From='ZoomOutButtonVisible' To='ZoomOutButtonHidden'>                                                                     "\
        L"                                       <Storyboard BeginTime='0:0:3'>                                                                                                          "\
        L"                                           <FadeOutThemeAnimation TargetName='ZoomOutButton' />                                                                                "\
        L"                                           <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='IsHitTestVisible'                                                         "\
        L"                                                                          Storyboard.TargetName='ZoomOutButton'>                                                               "\
        L"                                               <DiscreteObjectKeyFrame KeyTime='0'>                                                                                            "\
        L"                                                   <DiscreteObjectKeyFrame.Value>                                                                                              "\
        L"                                                       <x:Boolean>False</x:Boolean>                                                                                            "\
        L"                                                   </DiscreteObjectKeyFrame.Value>                                                                                             "\
        L"                                               </DiscreteObjectKeyFrame>                                                                                                       "\
        L"                                           </ObjectAnimationUsingKeyFrames>                                                                                                    "\
        L"                                       </Storyboard>                                                                                                                           "\
        L"                                   </VisualTransition>                                                                                                                         "\
        L"                               </VisualStateGroup.Transitions>                                                                                                                 "\
        L"                               <VisualState x:Name='ZoomOutButtonVisible'>                                                                                                     "\
        L"                                   <Storyboard>                                                                                                                                "\
        L"                                       <FadeInThemeAnimation TargetName='ZoomOutButton' />                                                                                     "\
        L"                                   </Storyboard>                                                                                                                               "\
        L"                               </VisualState>                                                                                                                                  "\
        L"                               <VisualState x:Name='ZoomOutButtonHidden'>                                                                                                      "\
        L"                                   <Storyboard>                                                                                                                                "\
        L"                                       <FadeOutThemeAnimation TargetName='ZoomOutButton' />                                                                                    "\
        L"                                       <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='IsHitTestVisible'                                                             "\
        L"                                                                      Storyboard.TargetName='ZoomOutButton'>                                                                   "\
        L"                                           <DiscreteObjectKeyFrame KeyTime='0'>                                                                                                "\
        L"                                               <DiscreteObjectKeyFrame.Value>                                                                                                  "\
        L"                                                   <x:Boolean>False</x:Boolean>                                                                                                "\
        L"                                               </DiscreteObjectKeyFrame.Value>                                                                                                 "\
        L"                                           </DiscreteObjectKeyFrame>                                                                                                           "\
        L"                                       </ObjectAnimationUsingKeyFrames>                                                                                                        "\
        L"                                   </Storyboard>                                                                                                                               "\
        L"                               </VisualState>                                                                                                                                  "\
        L"                           </VisualStateGroup>                                                                                                                                 "\
        L"                       </VisualStateManager.VisualStateGroups>                                                                                                                 "\
        L"                       <ScrollViewer x:Name='ScrollViewer'                                                                                                                     "\
        L"                                     HorizontalScrollMode='{TemplateBinding ScrollViewer.HorizontalScrollMode}'                                                                "\
        L"                                     HorizontalScrollBarVisibility='Hidden'                                                                                                    "\
        L"                                     VerticalScrollMode='{TemplateBinding ScrollViewer.VerticalScrollMode}'                                                                    "\
        L"                                     VerticalScrollBarVisibility='Hidden'                                                                                                      "\
        L"                                     IsHorizontalRailEnabled='{TemplateBinding ScrollViewer.IsHorizontalRailEnabled}'                                                          "\
        L"                                     IsVerticalRailEnabled='{TemplateBinding ScrollViewer.IsVerticalRailEnabled}'                                                              "\
        L"                                     ZoomMode='Enabled' IsZoomChainingEnabled='True'                                                                                           "\
        L"                                     IsZoomInertiaEnabled='False'                                                                                                              "\
        L"                                     IsScrollInertiaEnabled='True'                                                                                                             "\
        L"                                     HorizontalContentAlignment='Center'                                                                                                       "\
        L"                                     VerticalContentAlignment='Center'                                                                                                         "\
        L"                                     MinZoomFactor='0.5'                                                                                                                       "\
        L"                                     MaxZoomFactor='1.0'                                                                                                                       "\
        L"                                     AutomationProperties.AccessibilityView='Raw'>                                                                                             "\
        L"                           <ScrollViewer.Template>                                                                                                                             "\
        L"                               <ControlTemplate TargetType='ScrollViewer'>                                                                                                     "\
        L"                                   <ScrollContentPresenter x:Name='ScrollContentPresenter'                                                                                     "\
        L"                                                           ContentTemplate='{TemplateBinding ContentTemplate}'                                                                 "\
        L"                                                           Margin='{TemplateBinding Padding}' />                                                                               "\
        L"                               </ControlTemplate>                                                                                                                              "\
        L"                           </ScrollViewer.Template>                                                                                                                            "\
        L"                           <Border RenderTransformOrigin='0, 0'                                                                                                                "\
        L"                                   Background='{TemplateBinding Background}'                                                                                                   "\
        L"                                   BorderBrush='{TemplateBinding BorderBrush}'                                                                                                 "\
        L"                                   BorderThickness='{TemplateBinding BorderThickness}'>                                                                                        "\
        L"                               <Border.RenderTransform>                                                                                                                        "\
        L"                                   <CompositeTransform x:Name='ManipulatedElementTransform' />                                                                                 "\
        L"                               </Border.RenderTransform>                                                                                                                       "\
        L"                               <Grid Margin='{TemplateBinding Padding}'>                                                                                                       "\
        L"                                   <ContentPresenter x:Name='ZoomedInPresenter'                                                                                                "\
        L"                                                     RenderTransformOrigin='0.5, 0.5'                                                                                          "\
        L"                                                     Visibility='Collapsed'                                                                                                    "\
        L"                                                     IsHitTestVisible='False'                                                                                                  "\
        L"                                                     Content='{TemplateBinding ZoomedInView}'>                                                                                 "\
        L"                                       <ContentPresenter.RenderTransform>                                                                                                      "\
        L"                                           <CompositeTransform x:Name='ZoomedInTransform' />                                                                                   "\
        L"                                       </ContentPresenter.RenderTransform>                                                                                                     "\
        L"                                   </ContentPresenter>                                                                                                                         "\
        L"                                   <ContentPresenter x:Name='ZoomedOutPresenter'                                                                                               "\
        L"                                                     RenderTransformOrigin='0.5, 0.5'                                                                                          "\
        L"                                                     Content='{TemplateBinding ZoomedOutView}'                                                                                 "\
        L"                                                     IsHitTestVisible='False'                                                                                                  "\
        L"                                                     Visibility='Collapsed'>                                                                                                   "\
        L"                                       <ContentPresenter.RenderTransform>                                                                                                      "\
        L"                                           <CompositeTransform x:Name='ZoomedOutTransform' />                                                                                  "\
        L"                                       </ContentPresenter.RenderTransform>                                                                                                     "\
        L"                                   </ContentPresenter>                                                                                                                         "\
        L"                               </Grid>                                                                                                                                         "\
        L"                           </Border>                                                                                                                                           "\
        L"                       </ScrollViewer>                                                                                                                                         "\
        L"                       <Button x:Name='ZoomOutButton'                                                                                                                          "\
        L"                               IsTabStop='False'                                                                                                                               "\
        L"                               Margin='0,0,7,24'                                                                                                                               "\
        L"                               HorizontalAlignment='Right'                                                                                                                     "\
        L"                               VerticalAlignment='Bottom'>                                                                                                                     "\
        L"                           <Button.Template>                                                                                                                                   "\
        L"                               <ControlTemplate TargetType='Button'>                                                                                                           "\
        L"                                   <Border x:Name='Root' Height='21' Width='21'                                                                                                "\
        L"                                           Background='{ThemeResource SemanticZoomButtonBackgroundThemeBrush}'                                                                 "\
        L"                                           BorderBrush='{ThemeResource SemanticZoomButtonBorderThemeBrush}' BorderThickness='1'>                                               "\
        L"                                       <VisualStateManager.VisualStateGroups>                                                                                                  "\
        L"                                           <VisualStateGroup x:Name='NormalStates'>                                                                                            "\
        L"                                               <VisualState x:Name='Normal' />                                                                                                 "\
        L"                                               <VisualState x:Name='PointerOver'>                                                                                              "\
        L"                                                   <Storyboard>                                                                                                                "\
        L"                                                       <ObjectAnimationUsingKeyFrames Storyboard.TargetName='Root' Storyboard.TargetProperty='Background'>                     "\
        L"                                                           <DiscreteObjectKeyFrame KeyTime='0' Value='{ThemeResource SemanticZoomButtonPointerOverBackgroundThemeBrush}' />    "\
        L"                                                       </ObjectAnimationUsingKeyFrames>                                                                                        "\
        L"                                                       <ObjectAnimationUsingKeyFrames Storyboard.TargetName='Root' Storyboard.TargetProperty='BorderBrush'>                    "\
        L"                                                           <DiscreteObjectKeyFrame KeyTime='0' Value='{ThemeResource SemanticZoomButtonPointerOverBorderThemeBrush}' />        "\
        L"                                                       </ObjectAnimationUsingKeyFrames>                                                                                        "\
        L"                                                       <ObjectAnimationUsingKeyFrames Storyboard.TargetName='Glyph' Storyboard.TargetProperty='Foreground'>                    "\
        L"                                                           <DiscreteObjectKeyFrame KeyTime='0' Value='{ThemeResource SemanticZoomButtonPointerOverForegroundThemeBrush}' />    "\
        L"                                                       </ObjectAnimationUsingKeyFrames>                                                                                        "\
        L"                                                   </Storyboard>                                                                                                               "\
        L"                                               </VisualState>                                                                                                                  "\
        L"                                               <VisualState x:Name='Pressed'>                                                                                                  "\
        L"                                                   <Storyboard>                                                                                                                "\
        L"                                                       <ObjectAnimationUsingKeyFrames Storyboard.TargetName='Root' Storyboard.TargetProperty='Background'>                     "\
        L"                                                           <DiscreteObjectKeyFrame KeyTime='0' Value='{ThemeResource SemanticZoomButtonPressedBackgroundThemeBrush}' />        "\
        L"                                                       </ObjectAnimationUsingKeyFrames>                                                                                        "\
        L"                                                       <ObjectAnimationUsingKeyFrames Storyboard.TargetName='Root' Storyboard.TargetProperty='BorderBrush'>                    "\
        L"                                                           <DiscreteObjectKeyFrame KeyTime='0' Value='{ThemeResource SemanticZoomButtonPressedBorderThemeBrush}' />            "\
        L"                                                       </ObjectAnimationUsingKeyFrames>                                                                                        "\
        L"                                                       <ObjectAnimationUsingKeyFrames Storyboard.TargetName='Glyph' Storyboard.TargetProperty='Foreground'>                    "\
        L"                                                           <DiscreteObjectKeyFrame KeyTime='0' Value='{ThemeResource SemanticZoomButtonPressedForegroundThemeBrush}' />        "\
        L"                                                       </ObjectAnimationUsingKeyFrames>                                                                                        "\
        L"                                                   </Storyboard>                                                                                                               "\
        L"                                               </VisualState>                                                                                                                  "\
        L"                                           </VisualStateGroup>                                                                                                                 "\
        L"                                       </VisualStateManager.VisualStateGroups>                                                                                                 "\
        L"                                       <TextBlock x:Name='Glyph' Text='???' UseLayoutRounding='False' Margin='0,0,0,1' HorizontalAlignment='Center' VerticalAlignment='Center'   "\
        L"                                                  Foreground='{ThemeResource SemanticZoomButtonForegroundThemeBrush}'                                                          "\
        L"                                                  FontFamily='{ThemeResource SymbolThemeFontFamily}'                                                                           "\
        L"                                                  FontSize='{ThemeResource SemanticZoomButtonFontSize}'                                                                        "\
        L"                                                  IsHitTestVisible='False'                                                                                                     "\
        L"                                                  AutomationProperties.AccessibilityView='Raw' />                                                                              "\
        L"                                   </Border>                                                                                                                                   "\
        L"                               </ControlTemplate>                                                                                                                              "\
        L"                           </Button.Template>                                                                                                                                  "\
        L"                       </Button>                                                                                                                                               "\
        L"                   </Grid>                                                                                                                                                     "\
        L"               </ControlTemplate>                                                                                                                                              "\
        L"           </Setter.Value>                                                                                                                                                     "\
        L"       </Setter>                                                                                                                                                               "\
        L"   </Style>");
    };

} } } } } }
