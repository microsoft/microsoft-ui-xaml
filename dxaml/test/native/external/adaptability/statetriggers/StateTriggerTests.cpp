// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "TestCleanupWrapper.h"
#include "StateTriggerTests.h"
#include <RuntimeEnabledFeatureOverride.h>

#include <CustomTypeMetadataProvider.h>
#include <StateTrigger.CustomStateTrigger.h>
#include <StateTrigger.CustomStateTrigger.xaml.h>

#include <SafeEventRegistration.h>

using namespace ::Windows::Foundation;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace ::Windows::Storage::Streams;
using namespace ::Windows::Foundation::Collections;

using namespace Private::Tests::Adaptability::StateTriggerTests;

// Tests should work on phone and qualifier dimensions should not exceed 480 x 800

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Adaptability {

        Platform::String^ StateTriggersIntegrationTests::textBlockName;
        bool StateTriggersIntegrationTests::ClassSetup()
        {
            LOG_OUTPUT(L"ClassSetup()");
            CommonTestSetupHelper::CommonTestClassSetup();

            textBlockName = "VisualStateName";
            return true;
        }

        bool StateTriggersIntegrationTests::ClassCleanup()
        {
            LOG_OUTPUT(L"ClassCleanup()");

            return true;
        }

        bool StateTriggersIntegrationTests::TestSetup()
        {
            LOG_OUTPUT(L"TestSetup()");
            m_isTemplatedControl = false;
            test_infra::TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider(), ref new CustomMetadataRegistrar<CustomStateTrigger>());

            return true;
        }

        bool StateTriggersIntegrationTests::TestCleanup()
        {
            LOG_OUTPUT(L"TestCleanup()");
            TestServices::WindowHelper->VerifyTestCleanup();
            test_infra::TestServices::WindowHelper->ShutdownXaml();

            return true;
        }

        void StateTriggersIntegrationTests::BasicMarkup()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"    <StackPanel>\r\n"
                L"    </StackPanel>\r\n"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            TestServices::WindowHelper->WaitForIdle();
        }

        void StateTriggersIntegrationTests::UserControl()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"    <StackPanel>\r\n"
                L"      <VisualStateManager.VisualStateGroups>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>\r\n"
                L"              <VisualState x:Name='VisualState1'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='300' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"      </VisualStateManager.VisualStateGroups>\r\n"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>\r\n"
                L"    </StackPanel>\r\n"
                L"</UserControl>";

            Platform::String^ xamlContents2 =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"    <StackPanel>\r\n"
                L"      <VisualStateManager.VisualStateGroups>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>\r\n"
                L"              <VisualState x:Name='VisualState1'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='300' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                  <VisualState.Setters>"
                L"                      <Setter Target='VisualStateName.Text' Value='VisualState1'/>"
                L"                  </VisualState.Setters>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"      </VisualStateManager.VisualStateGroups>\r\n"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>\r\n"
                L"    </StackPanel>\r\n"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
            TestServices::WindowHelper->WaitForIdle();

            rootUserControl = CreateXamlTree(xamlContents2);
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
            TestServices::WindowHelper->WaitForIdle();
        }

        void StateTriggersIntegrationTests::UserControl_Groups()
        {
            LOG_OUTPUT(L"ParseQualifier");

            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"    <StackPanel>\r\n"
                L"      <VisualStateManager.VisualStateGroups>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup2'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup3'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup4'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup5'/>\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup6'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup7'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup8'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup9'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup10'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup11'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup12'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup13'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup14'>\r\n"
                L"              <VisualState x:Name='VisualState1'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='0' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"      </VisualStateManager.VisualStateGroups>\r\n"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>\r\n"
                L"    </StackPanel>\r\n"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
        }

        void StateTriggersIntegrationTests::UserControl_Groups_VisualStates()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"    <StackPanel>\r\n"
                L"      <VisualStateManager.VisualStateGroups>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup2'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup3'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup4'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup5'/>\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup6'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup7'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup8'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup9'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup10'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup11'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup12'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup13'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup14'>\r\n"
                L"              <VisualState x:Name='VisualState1'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='100'/>\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState2'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='0' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState3'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='10000' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState4'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='350' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState4' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState5'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='100' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState5' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"      </VisualStateManager.VisualStateGroups>\r\n"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>\r\n"
                L"    </StackPanel>\r\n"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState4");
        }

        void StateTriggersIntegrationTests::UserControl_Groups_VisualStates_StateTriggers()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"    <StackPanel>\r\n"
                L"      <VisualStateManager.VisualStateGroups>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup2'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup3'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup4'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup5'>\r\n"
                L"              <VisualState x:Name='VisualState10'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='385' MinWindowHeight='100'/>\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName1'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState10' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState11'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='99999' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName1'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState11' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup6'>\r\n"
                L"              <VisualState x:Name='VisualState12'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='100'/>\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName2'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState12' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState13'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='0' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName2'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState13' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup7'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup8'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup9'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup10'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup11'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup12'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup13'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup14'>\r\n"
                L"              <VisualState x:Name='VisualState1'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='100'/>\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName3'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState2'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='0' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName3'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState3'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='10000' MinWindowHeight='10000'/>\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName3'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState4'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='385' MinWindowHeight='0' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName3'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState4' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState5'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='100' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName3'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState5' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"      </VisualStateManager.VisualStateGroups>\r\n"
                L"    <TextBlock x:Name='VisualStateName1' Text='Default'/>\r\n"
                L"    <TextBlock x:Name='VisualStateName2' Text='Default'/>\r\n"
                L"    <TextBlock x:Name='VisualStateName3' Text='Default'/>\r\n"
                L"    </StackPanel>\r\n"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState10", "VisualStateName1");
            VerifyCurrentVisualState(rootUserControl, L"VisualState12", "VisualStateName2");
            VerifyCurrentVisualState(rootUserControl, L"VisualState4", "VisualStateName3");
        }

        void StateTriggersIntegrationTests::TemplatedControl()
        {
            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"        <VisualStateGroup x:Name='VisualStateGroup1'>"
            L"            <VisualState x:Name='VisualState1'/>"
            L"            <VisualState x:Name='VisualState2'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='600' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState3'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='500' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <TextBlock x:Name='VisualStateName' Text='Default' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
        }

        void StateTriggersIntegrationTests::TemplatedControl_Groups()
        {
            LOG_OUTPUT(L"ParseQualifier");

            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"        <VisualStateGroup x:Name='VisualStateGroup1'>"
            L"            <VisualState x:Name='VisualState1'/>"
            L"            <VisualState x:Name='VisualState2'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='600' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState3'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='500' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"        <VisualStateGroup x:Name='VisualStateGroup3'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup4'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup5'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup6'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup7'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup8'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup9'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup10'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup11'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup12'/>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <TextBlock x:Name='VisualStateName' Text='Default' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
        }

        void StateTriggersIntegrationTests::TemplatedControl_Groups_VisualStates()
        {
            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"        <VisualStateGroup x:Name='VisualStateGroup1'>"
            L"            <VisualState x:Name='VisualState1'/>"
            L"            <VisualState x:Name='VisualState2'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='600' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState3'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='500' />\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='1000' MinWindowHeight='10'/>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='100' />\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='101' />\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='105' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='3' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='4' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='10' MinWindowWidth='1' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='10' MinWindowWidth='10' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='11' MinWindowWidth='10' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='12' MinWindowWidth='10' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"        <VisualStateGroup x:Name='VisualStateGroup3'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup4'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup5'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup6'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup7'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup8'>"
            L"            <VisualState x:Name='VisualState4'/>"
            L"            <VisualState x:Name='VisualState5'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState4' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='1000' MinWindowHeight='100'/>\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"        <VisualStateGroup x:Name='VisualStateGroup9'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup10'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup11'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup12'>"
            L"            <VisualState x:Name='VisualState6'/>"
            L"            <VisualState x:Name='VisualState7'/>"
            L"            <VisualState x:Name='VisualState8'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState8' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
            L"                <VisualState.StateTriggers>\r\n"
            L"                  <AdaptiveTrigger MinWindowWidth='600' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='1' />\r\n"
            L"                </VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState9'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState9' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='500' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <TextBlock x:Name='VisualStateName' Text='Default' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState8");
        }

        void StateTriggersIntegrationTests::TemplatedControlWithTransitions()
        {
            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"        <VisualStateGroup x:Name='VisualStateGroup1'>"
            L"            <VisualStateGroup.Transitions>"
            L"                <VisualTransition From='VisualState1' To='VisualState2'>"
            L"                    <Storyboard x:Name='VisualTransitionStoryboard'>"
            L"                        <FadeOutThemeAnimation TargetName='Rect1' />"
            L"                    </Storyboard>"
            L"                </VisualTransition>"
            L"            </VisualStateGroup.Transitions>"
            L"            <VisualState x:Name='VisualState1'/>"
            L"            <VisualState x:Name='VisualState2'>"
            L"                <Storyboard>"
            L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetName='Rect1'"
            L"                         Storyboard.TargetProperty='Width'>"
            L"                         <DiscreteObjectKeyFrame KeyTime='0' Value='1000' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='600' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState3'>"
            L"                <Storyboard>"
            L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetName='Rect1'"
            L"                         Storyboard.TargetProperty='Width'>"
            L"                         <DiscreteObjectKeyFrame KeyTime='0' Value='1000' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='500' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <Rectangle Width='100' Height='100' Fill='Red' x:Name='Rect1' />"
            L"    <Rectangle Width='100' Height='100' Fill='Blue' x:Name='Rect2' />"
            L"    <TextBlock x:Name='VisualStateName' Text='Default' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
        }

        void StateTriggersIntegrationTests::ContentControl()
        {
            LOG_OUTPUT(L"ParseQualifier");
            Platform::String^ xamlContents =
            L"<ContentControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                    L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"<ContentControl.Template>\r\n"
                    L"<ControlTemplate>\r\n"
                        L"<StackPanel>\r\n"
                            L"<VisualStateManager.VisualStateGroups>\r\n"
                                L"<VisualStateGroup>\r\n"
                                    L"<VisualState x:Name='VisualState1'>\r\n"
                                        L"<Storyboard>"
                                            L"<ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                                                L"<DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                                            L"</ObjectAnimationUsingKeyFrames>"
                                        L"</Storyboard>"
                                        L"<VisualState.StateTriggers>\r\n"
                                            L"<AdaptiveTrigger MinWindowWidth='200' />\r\n"
                                        L"</VisualState.StateTriggers>\r\n"
                                    L"</VisualState>\r\n"
                                    L"<VisualState x:Name='VisualState2'>\r\n"
                                        L"<Storyboard>"
                                            L"<ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                                                L"<DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
                                            L"</ObjectAnimationUsingKeyFrames>"
                                        L"</Storyboard>"
                                        L"<VisualState.StateTriggers>\r\n"
                                            L"<AdaptiveTrigger MinWindowWidth='310' />\r\n"
                                        L"</VisualState.StateTriggers>\r\n"
                                    L"</VisualState>\r\n"
                                L"</VisualStateGroup>\r\n"
                            L"</VisualStateManager.VisualStateGroups>\r\n"
                            L"<TextBlock x:Name='VisualStateName' Text='Default' />"
                            L"<Rectangle Width='1000' Height='100' Fill='Red' x:Name='Rect1' />"
                            L"<Rectangle Width='1000' Height='100' Fill='Blue' x:Name='Rect2' />"
                        L"</StackPanel>\r\n"
                    L"</ControlTemplate>\r\n"
                L"</ContentControl.Template>\r\n"
            L"</ContentControl>\r\n";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
        }

        void StateTriggersIntegrationTests::Grid()
        {
            Platform::String^ xamlContents =
            L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"             xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"<Grid>"
                    L"<VisualStateManager.VisualStateGroups>"
                        L"<VisualStateGroup>"
                            L"<VisualState x:Name='wideState'>"
                                L"<VisualState.StateTriggers>"
                                    L"<AdaptiveTrigger MinWindowWidth='100' />"
                                L"</VisualState.StateTriggers>"
                                L"<Storyboard>"
                                    L"<ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                                        L"<DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                                    L"</ObjectAnimationUsingKeyFrames>"
                                L"</Storyboard>"
                            L"</VisualState>"
                            L"<VisualState x:Name='narrowState'>"
                                L"<VisualState.StateTriggers>"
                                    L"<AdaptiveTrigger MinWindowWidth='0' />"
                                L"</VisualState.StateTriggers>"
                                L"<Storyboard>"
                                    L"<ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                                        L"<DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
                                    L"</ObjectAnimationUsingKeyFrames>"
                                L"</Storyboard>"
                            L"</VisualState>"
                        L"</VisualStateGroup>"
                    L"</VisualStateManager.VisualStateGroups>"
                    L"<StackPanel x:Name='panel' Background='Red' HorizontalAlignment='Center' VerticalAlignment='Center'>"
                        L"<Button x:Name='button1' Content='Look' Margin='30' Background='Blue'/>"
                        L"<Button x:Name='button2' Content='I am' Margin='30' Background='Green'/>"
                        L"<Button x:Name='button3' Content='adapting' Margin='30' Background='Blue'/>"
                        L"<Button x:Name='button4' Content='so' Margin='30' Background='Green'/>"
                        L"<Button x:Name='button5' Content='beautifully' Margin='30' Background='Blue'/>"
                        L"<TextBlock x:Name='VisualStateName' Text='Default' />"
                    L"</StackPanel>"
                L"</Grid>"
            L"</UserControl>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = false;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
        }

       void StateTriggersIntegrationTests::GridWithSetters()
        {
            Platform::String^ xamlContents =
            L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"             xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"<Grid>"
                    L"<VisualStateManager.VisualStateGroups>"
                        L"<VisualStateGroup>"
                            L"<VisualState x:Name='wideState'>"
                                L"<VisualState.StateTriggers>"
                                    L"<AdaptiveTrigger MinWindowWidth='200' />"
                                L"</VisualState.StateTriggers>"
                                L"<VisualState.Setters>"
                                    L"<Setter Target='VisualStateName.Text' Value='VisualState1'/>"
                                L"</VisualState.Setters>"
                            L"</VisualState>"
                            L"<VisualState x:Name='narrowState'>"
                                L"<VisualState.StateTriggers>"
                                    L"<AdaptiveTrigger MinWindowWidth='0' />"
                                L"</VisualState.StateTriggers>"
                                L"<VisualState.Setters>"
                                    L"<Setter Target='VisualStateName.Text' Value='VisualState2'/>"
                                L"</VisualState.Setters>"
                            L"</VisualState>"
                        L"</VisualStateGroup>"
                    L"</VisualStateManager.VisualStateGroups>"
                    L"<StackPanel x:Name='panel' Background='Red' HorizontalAlignment='Center' VerticalAlignment='Center'>"
                        L"<Button x:Name='button1' Content='Look' Margin='30' Background='Blue'/>"
                        L"<Button x:Name='button2' Content='I am' Margin='30' Background='Green'/>"
                        L"<Button x:Name='button3' Content='adapting' Margin='30' Background='Blue'/>"
                        L"<Button x:Name='button4' Content='so' Margin='30' Background='Green'/>"
                        L"<Button x:Name='button5' Content='beautifully' Margin='30' Background='Blue'/>"
                        L"<TextBlock x:Name='VisualStateName' Text='Default' />"
                    L"</StackPanel>"
                L"</Grid>"
            L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
        }

        void StateTriggersIntegrationTests::UserControlWithExtensibleStateTriggers()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>\r\n"
                L"    <StackPanel>\r\n"
                L"      <VisualStateManager.VisualStateGroups>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>\r\n"
                L"              <VisualState x:Name='VisualState1'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='300' />\r\n"
                L"                      <local:CustomStateTrigger/>\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState2'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <local:CustomStateTrigger TriggerValue='True'/>\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"      </VisualStateManager.VisualStateGroups>\r\n"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>\r\n"
                L"    </StackPanel>\r\n"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
        }

        void StateTriggersIntegrationTests::TemplatedControlWithExtensibleStateTrigger()
        {
            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>\r\n"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"        <VisualStateGroup x:Name='VisualStateGroup1'>"
            L"            <VisualState x:Name='VisualState1'/>"
            L"            <VisualState x:Name='VisualState2'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='600' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState3'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='500' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                                L"<local:CustomStateTrigger TriggerValue='True' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <TextBlock x:Name='VisualStateName' Text='Default' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState3");
        }

        void StateTriggersIntegrationTests::UserControlWithExtensibleStateTriggersTrigger()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>\r\n"
                L"    <StackPanel>\r\n"
                L"      <VisualStateManager.VisualStateGroups>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>\r\n"
                L"              <VisualState x:Name='VisualState1'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='300' />\r\n"
                L"                      <local:CustomStateTrigger/>\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState2'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <local:CustomStateTrigger x:Name='ExtensibleTrigger'/>\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"      </VisualStateManager.VisualStateGroups>\r\n"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>\r\n"
                L"    </StackPanel>\r\n"
                L"</UserControl>";

            TestCleanupWrapper cleanup;

            // Verify initial VisualState selection
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
            auto stateTrigger = static_cast<CustomStateTrigger^>(VerifyGetNamedObject(rootUserControl, L"ExtensibleTrigger"));

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                stateTrigger->TriggerValue = false;
            });

            VerifyCurrentVisualState(rootUserControl, L"VisualState1");

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                stateTrigger->TriggerValue = true;
            });

            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
        }

        void StateTriggersIntegrationTests::TemplatedControlWithMultipleExtensibleStateTriggersTrigger()
        {
            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
            L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>\r\n"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"        <VisualStateGroup x:Name='VisualStateGroup1'>"
            L"            <VisualState x:Name='VisualState1'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
            L"                <VisualState.StateTriggers>\r\n"
            L"                  <AdaptiveTrigger MinWindowWidth='450' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='1' />\r\n"
            L"                </VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState2'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
            L"                <VisualState.StateTriggers>\r\n"
            L"                  <local:CustomStateTrigger x:Name='ExtensibleTrigger1'/>\r\n"
            L"                  <AdaptiveTrigger MinWindowWidth='400' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='1' />\r\n"
            L"                </VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState3'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
            L"              <VisualState.StateTriggers>\r\n"
            L"                  <local:CustomStateTrigger x:Name='ExtensibleTrigger2'/>\r\n"
            L"                  <AdaptiveTrigger MinWindowWidth='300' />\r\n"
            L"                  <AdaptiveTrigger MinWindowWidth='100' />\r\n"
            L"                  <AdaptiveTrigger MinWindowWidth='101' />\r\n"
            L"                  <AdaptiveTrigger MinWindowWidth='105' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='1' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='3' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='4' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='10' MinWindowWidth='1' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='10' MinWindowWidth='10' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='11' MinWindowWidth='10' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='12' MinWindowWidth='10' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='1' />\r\n"
            L"              </VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"        <VisualStateGroup x:Name='VisualStateGroup3'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup4'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup5'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup6'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup7'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup8'>"
            L"            <VisualState x:Name='VisualState4'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState4' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
            L"              <VisualState.StateTriggers>\r\n"
            L"                  <local:CustomStateTrigger x:Name='ExtensibleTrigger3'/>\r\n"
            L"              </VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState5'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState5' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
            L"              <VisualState.StateTriggers>\r\n"
            L"                  <AdaptiveTrigger MinWindowWidth='450' MinWindowHeight='100'/>\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='1' />\r\n"
            L"              </VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"        <VisualStateGroup x:Name='VisualStateGroup9'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup10'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup11'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup12'>"
            L"            <VisualState x:Name='VisualState6'/>"
            L"            <VisualState x:Name='VisualState7'/>"
            L"            <VisualState x:Name='VisualState8'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState8' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
            L"                <VisualState.StateTriggers>\r\n"
            L"                  <AdaptiveTrigger MinWindowWidth='400' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='1' />\r\n"
            L"                </VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState9'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState9' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
            L"                <VisualState.StateTriggers>\r\n"
            L"                  <AdaptiveTrigger MinWindowWidth='300' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='1' />\r\n"
            L"                 </VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <TextBlock x:Name='VisualStateName' Text='Default' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState8");

            // VS 2, 3, 5
            auto stateTrigger1 = static_cast<CustomStateTrigger^>(VerifyGetNamedObject(rootUserControl, L"ExtensibleTrigger1"));
            auto stateTrigger2 = static_cast<CustomStateTrigger^>(VerifyGetNamedObject(rootUserControl, L"ExtensibleTrigger2"));
            auto stateTrigger3 = static_cast<CustomStateTrigger^>(VerifyGetNamedObject(rootUserControl, L"ExtensibleTrigger3"));

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Set StateTrigger1 to true");
                stateTrigger1->TriggerValue = true;
                LOG_OUTPUT(L"Set StateTrigger2 to true");
                stateTrigger2->TriggerValue = true;
                LOG_OUTPUT(L"Set StateTrigger3 to true");
                stateTrigger3->TriggerValue = true;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState4");

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Set StateTrigger1 to false");
                stateTrigger1->TriggerValue = false;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState3");

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Set StateTrigger2 to false");
                stateTrigger2->TriggerValue = false;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Set StateTrigger3 to false");
                stateTrigger3->TriggerValue = false;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState5");
        }

        void StateTriggersIntegrationTests::UserControlWithMultipleExtensibleStateTriggersTrigger()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>\r\n"
                L"    <StackPanel>\r\n"
                L"      <VisualStateManager.VisualStateGroups>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup2'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup3'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup4'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup5'>\r\n"
                L"              <VisualState x:Name='VisualState10'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='385' MinWindowHeight='100'/>\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName1'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState10' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState11'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='99999' />\r\n"
                L"                      <local:CustomStateTrigger x:Name='ExtensibleTrigger1'/>"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName1'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState11' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup />\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup6'>\r\n"
                L"              <VisualState x:Name='VisualState12'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='100'/>\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName2'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState12' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState13'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='0' />\r\n"
                L"                      <local:CustomStateTrigger x:Name='ExtensibleTrigger2'/>"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName2'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState13' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup7'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup8'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup9'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup10'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup11'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup12'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup13'/>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup14'>\r\n"
                L"              <VisualState x:Name='VisualState1'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='100'/>\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName3'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState2'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='0' />\r\n"
                L"                      <local:CustomStateTrigger x:Name='ExtensibleTrigger3'/>"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName3'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState3'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='10000' MinWindowHeight='10000'/>\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName3'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState4'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='385' MinWindowHeight='0' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName3'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState4' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState5'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='100' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName3'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState5' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"      </VisualStateManager.VisualStateGroups>\r\n"
                L"    <TextBlock x:Name='VisualStateName1' Text='Default'/>\r\n"
                L"    <TextBlock x:Name='VisualStateName2' Text='Default'/>\r\n"
                L"    <TextBlock x:Name='VisualStateName3' Text='Default'/>\r\n"
                L"    </StackPanel>\r\n"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState10", L"VisualStateName1");
            VerifyCurrentVisualState(rootUserControl, L"VisualState12", L"VisualStateName2");
            VerifyCurrentVisualState(rootUserControl, L"VisualState4", L"VisualStateName3");

            // VS 10, 11, 12
            auto stateTrigger1 = static_cast<CustomStateTrigger^>(VerifyGetNamedObject(rootUserControl, L"ExtensibleTrigger1"));
            auto stateTrigger2 = static_cast<CustomStateTrigger^>(VerifyGetNamedObject(rootUserControl, L"ExtensibleTrigger2"));
            auto stateTrigger3 = static_cast<CustomStateTrigger^>(VerifyGetNamedObject(rootUserControl, L"ExtensibleTrigger3"));

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Set StateTrigger1 to true");
                stateTrigger1->TriggerValue = true;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState11", L"VisualStateName1");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Set StateTrigger2 to true");
                stateTrigger2->TriggerValue = true;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState13", L"VisualStateName2");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Set StateTrigger3 to true");
                stateTrigger3->TriggerValue = true;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState2", L"VisualStateName3");

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Set StateTrigger3 to false");
                stateTrigger3->TriggerValue = false;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState4", L"VisualStateName3");

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Set StateTrigger1 to false");
                stateTrigger1->TriggerValue = false;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState10", L"VisualStateName1");

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Set StateTrigger2 to false");
                stateTrigger2->TriggerValue = false;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState12", L"VisualStateName2");
        }

        void StateTriggersIntegrationTests::TemplatedControlWithExtensibleStateTriggerTrigger()
        {
            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"        <VisualStateGroup x:Name='VisualStateGroup1'>"
            L"            <VisualState x:Name='VisualState1'/>"
            L"            <VisualState x:Name='VisualState2'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='600' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState3'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='500' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                                L"<local:CustomStateTrigger x:Name='ExtensibleTrigger' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <TextBlock x:Name='VisualStateName' Text='Default' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;

            // Verify initial VisualState selection
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
            auto stateTrigger = static_cast<CustomStateTrigger^>(VerifyGetNamedObject(rootUserControl, L"ExtensibleTrigger"));

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                stateTrigger->TriggerValue = true;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState3");

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                stateTrigger->TriggerValue = false;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
        }

        void StateTriggersIntegrationTests::TemplatedControlWithExtensibleStateTriggerFault()
        {
            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>\r\n"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"        <VisualStateGroup x:Name='VisualStateGroup1'>"
            L"            <VisualState x:Name='VisualState1'/>"
            L"            <VisualState x:Name='VisualState2'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='600' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState3'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='500' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                                L"<local:CustomStateTrigger TriggerValue='True' x:Name='ExtensibleTrigger'/>\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <TextBlock x:Name='VisualStateName' Text='Default' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;

            // Verify initial VisualState selection
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState3");
            auto stateTrigger = static_cast<CustomStateTrigger^>(VerifyGetNamedObject(rootUserControl, L"ExtensibleTrigger"));

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                stateTrigger->TriggerValue = false;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
        }

        void StateTriggersIntegrationTests::TemplatedControlWithExtensibleStateTriggerFaultNamed()
        {
            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>\r\n"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"        <VisualStateGroup x:Name='VisualStateGroup1'>"
            L"            <VisualState x:Name='VisualState1'/>"
            L"            <VisualState x:Name='VisualState2'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='600' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState3'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='500' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <TextBlock x:Name='VisualStateName' Text='Default' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;

            // Verify initial VisualState selection
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
            //auto stateTrigger = static_cast<CustomStateTrigger^>(VerifyGetNamedObject(rootUserControl, L"MyStateTrigger"));

            // Verify VisualStateGroups have not been created

            // Fault-In VisualStateGroups
            CauseVisualStateGroupCollectionToFaultIn(rootUserControl);

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                //stateTrigger->TriggerValue = false;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");

            // Verify VisualStateGroups have been created

            // Get StateTriggerCollection and confirm AdaptiveTriggers exist

            // Get StateTriggerCollection and confirm ExtensiveTriggers were moved
            //
            IVector<Microsoft::UI::Xaml::StateTriggerBase ^>^ stateTriggers = GetStateTriggerCollection(rootUserControl, 0, 1);
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"StateTrigger count: %d", stateTriggers->Size);
            });

            IVector<Microsoft::UI::Xaml::StateTriggerBase ^>^ stateTriggers2 = GetStateTriggerCollection(rootUserControl, 0, 2);
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"StateTrigger count: %d", stateTriggers2->Size);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void StateTriggersIntegrationTests::TemplatedControlWithExtensibleStateTriggerFaultNamedTest()
        {
            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>\r\n"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"        <VisualStateGroup x:Name='VisualStateGroup1'>"
            L"            <VisualState x:Name='VisualState1'/>"
            L"            <VisualState x:Name='VisualState2'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='600' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState3'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='500' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                                L"<local:CustomStateTrigger x:Name='MyStateTrigger' TriggerValue='True' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <TextBlock x:Name='VisualStateName' Text='Default' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;

            // Verify initial VisualState selection
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState3");
            auto stateTrigger = static_cast<CustomStateTrigger^>(VerifyGetNamedObject(rootUserControl, L"MyStateTrigger"));

            // Verify VisualStateGroups have not been created

            // Fault-In VisualStateGroups
            //CauseVisualStateGroupCollectionToFaultIn(rootUserControl);

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                stateTrigger->TriggerValue = false;
            });

            VerifyCurrentVisualState(rootUserControl, L"VisualState2");

            // Verify VisualStateGroups have been created

            // Get StateTriggerCollection and confirm AdaptiveTriggers exist

            // Get StateTriggerCollection and confirm ExtensiveTriggers were moved
        }

        void StateTriggersIntegrationTests::TemplatedControlWithExtensibleStateTriggerFaultNamed2()
        {
            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>\r\n"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"        <VisualStateGroup x:Name='VisualStateGroup1'>"
            L"            <VisualState x:Name='VisualState1'/>"
            L"            <VisualState x:Name='VisualState2'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='600' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState3'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger x:Name='AdaptiveTrigger1' MinWindowWidth='500' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                                L"<local:CustomStateTrigger x:Name='MyStateTrigger' TriggerValue='True' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <TextBlock x:Name='VisualStateName' Text='Default' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;

            m_isTemplatedControl = true;

            // Verify initial VisualState selection
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState3");
            auto stateTrigger = static_cast<CustomStateTrigger^>(VerifyGetNamedObject(rootUserControl, L"MyStateTrigger"));

            // Verify VisualStateGroups have not been created

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                stateTrigger->TriggerValue = false;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                stateTrigger->TriggerValue = true;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState3");

            // Fault-In VisualStateGroups
            CauseVisualStateGroupCollectionToFaultIn(rootUserControl);
            VerifyCurrentVisualState(rootUserControl, L"VisualState3");

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                stateTrigger->TriggerValue = false;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");

            // Get StateTriggerCollection and confirm ExtensibleTriggers were moved
            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                stateTrigger->TriggerValue = true;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState3");
        }

        void StateTriggersIntegrationTests::TemplatedControlWithExtensibleStateTriggerFaultGetStateTriggers()
        {
            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>\r\n"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"        <VisualStateGroup x:Name='VisualStateGroup1'>"
            L"            <VisualState x:Name='VisualState1'/>"
            L"            <VisualState x:Name='VisualState2'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='600' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState3'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger x:Name='AdaptiveTrigger1' MinWindowWidth='500' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                                L"<local:CustomStateTrigger x:Name='MyStateTrigger' TriggerValue='True' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <TextBlock x:Name='VisualStateName' Text='Default' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;

            m_isTemplatedControl = true;

            // Verify initial VisualState selection
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState3");
            auto stateTrigger = static_cast<CustomStateTrigger^>(VerifyGetNamedObject(rootUserControl, L"MyStateTrigger"));

            // Verify VisualStateGroups have not been created

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                stateTrigger->TriggerValue = false;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");

            IVector<Microsoft::UI::Xaml::StateTriggerBase ^>^ stateTriggers = GetStateTriggerCollection(rootUserControl, 0, 2);
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"StateTrigger count: %d", stateTriggers->Size);
                LOG_OUTPUT(L"Removing StateTrigger: %d", 3);
                stateTriggers->RemoveAt(0);
                LOG_OUTPUT(L"StateTrigger count: %d", stateTriggers->Size);

                AdaptiveTrigger^ adaptiveTrigger = ref new AdaptiveTrigger();
                adaptiveTrigger->MinWindowWidth = 100;
                adaptiveTrigger->MinWindowHeight = 1;
                LOG_OUTPUT(L"Appending StateTrigger");
                stateTriggers->Append(adaptiveTrigger);
                LOG_OUTPUT(L"StateTrigger count: %d", stateTriggers->Size);
            });
        }

        void StateTriggersIntegrationTests::UserControlModifyAdaptiveTriggerInCode()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"    <StackPanel>\r\n"
                L"      <VisualStateManager.VisualStateGroups>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>\r\n"
                L"              <VisualState x:Name='VisualState1'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger x:Name='AdaptiveTrigger1' MinWindowWidth='300' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState2'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger x:Name='AdaptiveTrigger2' MinWindowWidth='450' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"      </VisualStateManager.VisualStateGroups>\r\n"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>\r\n"
                L"    </StackPanel>\r\n"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");

            auto adaptiveTrigger1 = static_cast<AdaptiveTrigger^>(VerifyGetNamedObject(rootUserControl, L"AdaptiveTrigger1"));
            RunOnUIThread([&]()
            {
                adaptiveTrigger1->MinWindowWidth = 451;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");

            RunOnUIThread([&]()
            {
                adaptiveTrigger1->MinWindowWidth = 449;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");

            RunOnUIThread([&]()
            {
                adaptiveTrigger1->MinWindowWidth = 450;
                adaptiveTrigger1->MinWindowHeight = 1;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");

            auto adaptiveTrigger2 = static_cast<AdaptiveTrigger^>(VerifyGetNamedObject(rootUserControl, L"AdaptiveTrigger2"));
            RunOnUIThread([&]()
            {
                adaptiveTrigger2->MinWindowWidth = 450;
                adaptiveTrigger2->MinWindowHeight = 2;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
        }

        void StateTriggersIntegrationTests::TemplatedControlModifyAdaptiveTriggerInCode()
        {
            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"        <VisualStateGroup x:Name='VisualStateGroup1'>"
            L"            <VisualState x:Name='VisualState1'/>"
            L"            <VisualState x:Name='VisualState2'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger x:Name='AdaptiveTrigger2' MinWindowWidth='450' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState3'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger x:Name='AdaptiveTrigger3' MinWindowWidth='300' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <TextBlock x:Name='VisualStateName' Text='Default' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");

            auto adaptiveTrigger2 = static_cast<AdaptiveTrigger^>(VerifyGetNamedObject(rootUserControl, L"AdaptiveTrigger2"));
            RunOnUIThread([&]()
            {
                adaptiveTrigger2->MinWindowWidth = 299;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState3");

            RunOnUIThread([&]()
            {
                adaptiveTrigger2->MinWindowWidth = 301;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");

            RunOnUIThread([&]()
            {
                adaptiveTrigger2->MinWindowWidth = 300;
                adaptiveTrigger2->MinWindowHeight = 2;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");

            auto adaptiveTrigger3 = static_cast<AdaptiveTrigger^>(VerifyGetNamedObject(rootUserControl, L"AdaptiveTrigger3"));
            RunOnUIThread([&]()
            {
                adaptiveTrigger3->MinWindowWidth = 300;
                adaptiveTrigger3->MinWindowHeight = 3;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState3");
        }

        void StateTriggersIntegrationTests::UserControlModifyCollectionInUserCode()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"    <StackPanel>\r\n"
                L"      <VisualStateManager.VisualStateGroups>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>\r\n"
                L"              <VisualState x:Name='VisualState1'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger x:Name='AdaptiveTrigger1' MinWindowWidth='300' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState2'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger x:Name='AdaptiveTrigger2' MinWindowWidth='450' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"      </VisualStateManager.VisualStateGroups>\r\n"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>\r\n"
                L"    </StackPanel>\r\n"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");

            // Get the StateTriggerCollection using public APIs
            auto stateTriggerCollection = GetStateTriggerCollection(rootUserControl, 0, 1);

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"StateTrigger count: %d", stateTriggerCollection->Size);
                LOG_OUTPUT(L"Removing StateTrigger: %d", 0);
                stateTriggerCollection->RemoveAt(0);
                LOG_OUTPUT(L"StateTrigger count: %d", stateTriggerCollection->Size);
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");

            RunOnUIThread([&]()
            {
                AdaptiveTrigger^ adaptiveTrigger = ref new AdaptiveTrigger();
                adaptiveTrigger->MinWindowWidth = 301;
                LOG_OUTPUT(L"Appending StateTrigger");
                stateTriggerCollection->Append(adaptiveTrigger);
                LOG_OUTPUT(L"StateTrigger count: %d", stateTriggerCollection->Size);
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");

            // Get the StateTriggerCollection using public APIs
            auto stateTriggerCollection1 = GetStateTriggerCollection(rootUserControl, 0, 0);
            RunOnUIThread([&]()
            {
                AdaptiveTrigger^ adaptiveTrigger = ref new AdaptiveTrigger();
                adaptiveTrigger->MinWindowWidth = 451;
                LOG_OUTPUT(L"Appending StateTrigger");
                stateTriggerCollection1->Append(adaptiveTrigger);
                LOG_OUTPUT(L"StateTrigger count: %d", stateTriggerCollection1->Size);
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
        }

        void StateTriggersIntegrationTests::TemplatedControlModifyCollectionInUserCode()
        {
            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>\r\n"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"        <VisualStateGroup x:Name='VisualStateGroup1'>"
            L"            <VisualState x:Name='VisualState1'/>"
            L"            <VisualState x:Name='VisualState2'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='450' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                                L"<local:CustomStateTrigger />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState3'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='300' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                                L"<local:CustomStateTrigger />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <TextBlock x:Name='VisualStateName' Text='Default' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");

            // Get the StateTriggerCollection using public APIs
            auto stateTriggerCollection = GetStateTriggerCollection(rootUserControl, 0, 1);

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"StateTrigger count: %d", stateTriggerCollection->Size);
                LOG_OUTPUT(L"Removing StateTrigger: %d", 0);
                stateTriggerCollection->RemoveAt(1);
                LOG_OUTPUT(L"StateTrigger count: %d", stateTriggerCollection->Size);
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState3");

            RunOnUIThread([&]()
            {
                AdaptiveTrigger^ adaptiveTrigger = ref new AdaptiveTrigger();
                adaptiveTrigger->MinWindowWidth = 451;
                LOG_OUTPUT(L"Appending StateTrigger");
                stateTriggerCollection->Append(adaptiveTrigger);
                LOG_OUTPUT(L"StateTrigger count: %d", stateTriggerCollection->Size);
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
        }

        void StateTriggersIntegrationTests::UserControlExtensibleStateTriggerBinding()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>\r\n"
                L"    <StackPanel x:Name='MyStackPanel' IsHitTestVisible='True'>\r\n"
                L"      <VisualStateManager.VisualStateGroups>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>\r\n"
                L"              <VisualState x:Name='VisualState1'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                  <local:CustomStateTrigger TriggerValue='{Binding ElementName=MyStackPanel, Path=IsHitTestVisible}'/>"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState2'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowHeight='1' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"      </VisualStateManager.VisualStateGroups>\r\n"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>\r\n"
                L"    </StackPanel>\r\n"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
        }

        void StateTriggersIntegrationTests::TemplatedControlExtensibleStateTriggerBinding()
        {
            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>\r\n"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel x:Name='MyStackPanel' IsHitTestVisible='True'>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"        <VisualStateGroup x:Name='VisualStateGroup1'>"
            L"            <VisualState x:Name='VisualState1'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
            L"              <VisualState.StateTriggers>\r\n"
            L"                  <local:CustomStateTrigger TriggerValue='{Binding ElementName=MyStackPanel, Path=IsHitTestVisible}'/>"
            L"              </VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState2'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
            L"              <VisualState.StateTriggers>\r\n"
            L"                      <AdaptiveTrigger MinWindowHeight='1' />\r\n"
            L"              </VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <TextBlock x:Name='VisualStateName' Text='Default' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
        }

        void StateTriggersIntegrationTests::TemplatedControlExtensibleStateTriggerBindingModifyInUserCode()
        {
            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
            L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>\r\n"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel x:Name='TemplateStackPanel' IsHitTestVisible='True'>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"        <VisualStateGroup x:Name='VisualStateGroup1'>"
            L"            <VisualState x:Name='VisualState1'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
            L"                <VisualState.StateTriggers>\r\n"
            L"                  <AdaptiveTrigger MinWindowWidth='650' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='1' />\r\n"
            L"                </VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState2'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
            L"                <VisualState.StateTriggers>\r\n"
            L"                  <AdaptiveTrigger MinWindowWidth='600' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='1' />\r\n"
            L"                </VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState3'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
            L"              <VisualState.StateTriggers>\r\n"
            L"                  <local:CustomStateTrigger x:Name='ExtensibleTrigger2'/>\r\n"
            L"                  <AdaptiveTrigger MinWindowWidth='500' />\r\n"
            L"                  <AdaptiveTrigger MinWindowWidth='100' />\r\n"
            L"                  <AdaptiveTrigger MinWindowWidth='101' />\r\n"
            L"                  <AdaptiveTrigger MinWindowWidth='105' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='1' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='3' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='4' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='10' MinWindowWidth='1' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='10' MinWindowWidth='10' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='11' MinWindowWidth='10' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='12' MinWindowWidth='10' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='1' />\r\n"
            L"              </VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"        <VisualStateGroup x:Name='VisualStateGroup3'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup4'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup5'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup6'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup7'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup8'>"
            L"            <VisualState x:Name='VisualState4'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState4' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
            L"              <VisualState.StateTriggers>\r\n"
            L"                  <local:CustomStateTrigger x:Name='ExtensibleTrigger3'/>\r\n"
            L"              </VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState5'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState5' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
            L"              <VisualState.StateTriggers>\r\n"
            L"                  <AdaptiveTrigger MinWindowWidth='1000' MinWindowHeight='100'/>\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='1' />\r\n"
            L"              </VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"        <VisualStateGroup x:Name='VisualStateGroup9'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup10'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup11'/>"
            L"        <VisualStateGroup x:Name='VisualStateGroup12'>"
            L"            <VisualState x:Name='VisualState6'/>"
            L"            <VisualState x:Name='VisualState7'/>"
            L"            <VisualState x:Name='VisualState8'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState8' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
            L"                <VisualState.StateTriggers>\r\n"
            L"                  <AdaptiveTrigger MinWindowWidth='600' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='1' />\r\n"
            L"                </VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState9'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState9' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
            L"                <VisualState.StateTriggers>\r\n"
            L"                  <AdaptiveTrigger MinWindowWidth='500' />\r\n"
            L"                  <AdaptiveTrigger MinWindowHeight='1' />\r\n"
            L"                  <local:CustomStateTrigger x:Name='ExtensibleTrigger11' TriggerValue='{Binding ElementName=TemplateStackPanel, Path=IsHitTestVisible}'/>\r\n"
            L"                 </VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <TextBlock x:Name='VisualStateName' Text='Default' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState9");

            auto stateTrigger1 = static_cast<CustomStateTrigger^>(VerifyGetNamedObject(rootUserControl, L"ExtensibleTrigger11"));

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Set StateTrigger1 to true");
                stateTrigger1->TriggerValue = false;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState8");

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Set StateTrigger1 to false");
                stateTrigger1->TriggerValue = true;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState9");

            // Fault-In VisualStateGroups
            CauseVisualStateGroupCollectionToFaultIn(rootUserControl);

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Set StateTrigger1 to true");
                stateTrigger1->TriggerValue = false;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState8");

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Set StateTrigger1 to false");
                stateTrigger1->TriggerValue = true;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState9");
       }

       void StateTriggersIntegrationTests::UserControlWithStateTriggersTrigger()
       {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>\r\n"
                L"    <StackPanel>\r\n"
                L"      <VisualStateManager.VisualStateGroups>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>\r\n"
                L"              <VisualState x:Name='VisualState1'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='300' />\r\n"
                L"                      <StateTrigger/>\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState2'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <StateTrigger x:Name='MyStateTrigger'/>\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"      </VisualStateManager.VisualStateGroups>\r\n"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>\r\n"
                L"    </StackPanel>\r\n"
                L"</UserControl>";

            TestCleanupWrapper cleanup;


            // Verify initial VisualState selection
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
            auto stateTrigger = static_cast<StateTrigger^>(VerifyGetNamedObject(rootUserControl, L"MyStateTrigger"));

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                stateTrigger->IsActive = false;
            });

            VerifyCurrentVisualState(rootUserControl, L"VisualState1");

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                stateTrigger->IsActive = true;
            });

            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
       }

       void StateTriggersIntegrationTests::TemplatedControlWithStateTriggerTrigger()
       {
            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"        <VisualStateGroup x:Name='VisualStateGroup1'>"
            L"            <VisualState x:Name='VisualState1'/>"
            L"            <VisualState x:Name='VisualState2'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='600' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState3'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='500' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                                L"<StateTrigger x:Name='BasicTrigger' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <TextBlock x:Name='VisualStateName' Text='Default' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;

            // Verify initial VisualState selection
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
            auto stateTrigger = static_cast<StateTrigger^>(VerifyGetNamedObject(rootUserControl, L"BasicTrigger"));

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                stateTrigger->IsActive = true;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState3");

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                stateTrigger->IsActive = false;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
       }

       void StateTriggersIntegrationTests::UserControlWithUnnamedVisualStates()
       {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"    <StackPanel>\r\n"
                L"      <VisualStateManager.VisualStateGroups>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>\r\n"
                L"              <VisualState>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='300' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"      </VisualStateManager.VisualStateGroups>\r\n"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>\r\n"
                L"    </StackPanel>\r\n"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
        }

        void StateTriggersIntegrationTests::TemplatedControlWithUnnamedVisualStates()
        {
            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"        <VisualStateGroup x:Name='VisualStateGroup1'>"
            L"            <VisualState/>"
            L"            <VisualState>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='600' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='500' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <TextBlock x:Name='VisualStateName' Text='Default' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
        }

        void StateTriggersIntegrationTests::TemplatedControlWithExtensibleStateTriggerFaultUnnamedVisualStates()
        {
            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>\r\n"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"        <VisualStateGroup x:Name='VisualStateGroup1'>"
            L"            <VisualState/>"
            L"            <VisualState>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='600' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='500' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                                L"<local:CustomStateTrigger x:Name='MyStateTrigger' TriggerValue='True' />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <TextBlock x:Name='VisualStateName' Text='Default' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;

            // Verify initial VisualState selection
            auto rootUserControl = CreateXamlTree(xamlContents);

            VerifyCurrentVisualState(rootUserControl, L"VisualState3");
            auto stateTrigger = static_cast<CustomStateTrigger^>(VerifyGetNamedObject(rootUserControl, L"MyStateTrigger"));

            // Fault-In VisualStateGroups
            CauseVisualStateGroupCollectionToFaultIn(rootUserControl);

            // Programmatically change the ExtensibleTrigger and verify VisualState change
            RunOnUIThread([&]()
            {
                stateTrigger->TriggerValue = false;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");

            TestServices::WindowHelper->WaitForIdle();
        }

        void StateTriggersIntegrationTests::UserControlResize()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"             xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"    <StackPanel>\r\n"
                L"      <VisualStateManager.VisualStateGroups>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>\r\n"
                L"              <VisualState>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='1000' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='600' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"      </VisualStateManager.VisualStateGroups>\r\n"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>\r\n"
                L"    </StackPanel>\r\n"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
            ResizeDesktopWindow(801, 768);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
            MaximizeDesktopWindow();
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
            ResizeDesktopWindow(900, 768);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
            MaximizeDesktopWindow();
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
            ResizeDesktopWindow(400, 768);
            VerifyCurrentVisualState(rootUserControl, L"Default");
            MaximizeDesktopWindow();
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
        }

        void StateTriggersIntegrationTests::TemplatedControlResize()
        {
            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"        <VisualStateGroup x:Name='VisualStateGroup1'>"
            L"            <VisualState/>"
            L"            <VisualState>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
            L"              <VisualState.StateTriggers>\r\n"
            L"                  <AdaptiveTrigger MinWindowWidth='1000' />\r\n"
            L"              </VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
            L"              <VisualState.StateTriggers>\r\n"
            L"                  <AdaptiveTrigger MinWindowWidth='800' />\r\n"
            L"              </VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <TextBlock x:Name='VisualStateName' Text='Default' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
            ResizeDesktopWindow(300, 768);
            VerifyCurrentVisualState(rootUserControl, L"Default");
            MaximizeDesktopWindow();
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
            ResizeDesktopWindow(900, 768);
            VerifyCurrentVisualState(rootUserControl, L"VisualState3");
            MaximizeDesktopWindow();
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
        }

        void StateTriggersIntegrationTests::GridManualVisualStateTransitions()
        {
            Platform::String^ xamlContents =
            L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"             xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"<Grid>"
                    L"<VisualStateManager.VisualStateGroups>"
                        L"<VisualStateGroup>"
                            L"<VisualState x:Name='wideState'>"
                                L"<VisualState.StateTriggers>"
                                    L"<AdaptiveTrigger MinWindowWidth='200' />"
                                L"</VisualState.StateTriggers>"
                                L"<VisualState.Setters>"
                                    L"<Setter Target='VisualStateName.Text' Value='VisualState1'/>"
                                L"</VisualState.Setters>"
                            L"</VisualState>"
                            L"<VisualState x:Name='narrowState'>"
                                L"<VisualState.StateTriggers>"
                                    L"<AdaptiveTrigger MinWindowWidth='0' />"
                                L"</VisualState.StateTriggers>"
                                L"<VisualState.Setters>"
                                    L"<Setter Target='VisualStateName.Text' Value='VisualState2'/>"
                                L"</VisualState.Setters>"
                            L"</VisualState>"
                        L"</VisualStateGroup>"
                    L"</VisualStateManager.VisualStateGroups>"
                    L"<StackPanel x:Name='panel' Background='Red' HorizontalAlignment='Center' VerticalAlignment='Center'>"
                        L"<Button x:Name='button1' Content='Look' Margin='30' Background='Blue'/>"
                        L"<Button x:Name='button2' Content='I am' Margin='30' Background='Green'/>"
                        L"<Button x:Name='button3' Content='adapting' Margin='30' Background='Blue'/>"
                        L"<Button x:Name='button4' Content='so' Margin='30' Background='Green'/>"
                        L"<Button x:Name='button5' Content='beautifully' Margin='30' Background='Blue'/>"
                        L"<TextBlock x:Name='VisualStateName' Text='Default' />"
                    L"</StackPanel>"
                L"</Grid>"
            L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
            ChangeVisualState(rootUserControl, "narrowState");
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
            ChangeVisualState(rootUserControl, "wideState");
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
        }

        void StateTriggersIntegrationTests::UserControlSettersResize()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"             xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"    <StackPanel>\r\n"
                L"      <VisualStateManager.VisualStateGroups>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>\r\n"
                L"              <VisualState>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='1000' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                  <VisualState.Setters>"
                L"                      <Setter Target='VisualStateName.Text' Value='VisualState1'/>"
                L"                  </VisualState.Setters>"
                L"              </VisualState>\r\n"
                L"              <VisualState>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='600' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                  <VisualState.Setters>"
                L"                      <Setter Target='VisualStateName.Text' Value='VisualState2'/>"
                L"                  </VisualState.Setters>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"      </VisualStateManager.VisualStateGroups>\r\n"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>\r\n"
                L"    </StackPanel>\r\n"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
            ResizeDesktopWindow(801, 768);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
            MaximizeDesktopWindow();
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
            ResizeDesktopWindow(900, 768);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
            MaximizeDesktopWindow();
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
            ResizeDesktopWindow(400, 768);
            VerifyCurrentVisualState(rootUserControl, L"Default");
            MaximizeDesktopWindow();
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
        }

        void StateTriggersIntegrationTests::TemplatedControlSettersResize()
        {
            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"        <VisualStateGroup x:Name='VisualStateGroup1'>"
            L"            <VisualState/>"
            L"            <VisualState>"
            L"                <VisualState.Setters>"
            L"                  <Setter Target='VisualStateName.Text' Value='VisualState2'/>"
            L"                </VisualState.Setters>"
            L"              <VisualState.StateTriggers>\r\n"
            L"                  <AdaptiveTrigger MinWindowWidth='1000' />\r\n"
            L"              </VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState>"
            L"                <VisualState.Setters>"
            L"                  <Setter Target='VisualStateName.Text' Value='VisualState3'/>"
            L"                </VisualState.Setters>"
            L"              <VisualState.StateTriggers>\r\n"
            L"                  <AdaptiveTrigger MinWindowWidth='800' />\r\n"
            L"              </VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <TextBlock x:Name='VisualStateName' Text='Default' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
            ResizeDesktopWindow(300, 768);
            VerifyCurrentVisualState(rootUserControl, L"Default");
            MaximizeDesktopWindow();
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
            ResizeDesktopWindow(900, 768);
            VerifyCurrentVisualState(rootUserControl, L"VisualState3");
            MaximizeDesktopWindow();
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
        }

        void StateTriggersIntegrationTests::UserControlSettersExtensibleStateTriggerBinding()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>\r\n"
                L"    <StackPanel x:Name='MyStackPanel' IsHitTestVisible='True'>\r\n"
                L"      <VisualStateManager.VisualStateGroups>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>\r\n"
                L"              <VisualState x:Name='VisualState1'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                  <local:CustomStateTrigger TriggerValue='{Binding ElementName=MyStackPanel, Path=IsHitTestVisible}'/>"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <VisualState.Setters>"
                L"                  <Setter Target='VisualStateName.Text' Value='VisualState1'/>"
                L"                </VisualState.Setters>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState2'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowHeight='1' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <VisualState.Setters>"
                L"                  <Setter Target='VisualStateName.Text' Value='VisualState2'/>"
                L"                </VisualState.Setters>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"      </VisualStateManager.VisualStateGroups>\r\n"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>\r\n"
                L"    </StackPanel>\r\n"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
        }

        void StateTriggersIntegrationTests::TemplatedControlSettersExtensibleStateTriggerBinding()
        {
            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>\r\n"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel x:Name='MyStackPanel' IsHitTestVisible='True'>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"        <VisualStateGroup x:Name='VisualStateGroup1'>"
            L"            <VisualState x:Name='VisualState1'>"
            L"                <VisualState.Setters>"
            L"                  <Setter Target='VisualStateName.Text' Value='VisualState1'/>"
            L"                </VisualState.Setters>"
            L"              <VisualState.StateTriggers>\r\n"
            L"                  <local:CustomStateTrigger TriggerValue='{Binding ElementName=MyStackPanel, Path=IsHitTestVisible}'/>"
            L"              </VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState2'>"
            L"                <VisualState.Setters>"
            L"                  <Setter Target='VisualStateName.Text' Value='VisualState2'/>"
            L"                </VisualState.Setters>"
            L"              <VisualState.StateTriggers>\r\n"
            L"                      <AdaptiveTrigger MinWindowHeight='1' />\r\n"
            L"              </VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <TextBlock x:Name='VisualStateName' Text='Default' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
        }

        void StateTriggersIntegrationTests::UserControlWithStateTriggers()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>\r\n"
                L"    <StackPanel>\r\n"
                L"      <VisualStateManager.VisualStateGroups>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>\r\n"
                L"              <VisualState x:Name='VisualState1'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='300' />\r\n"
                L"                      <StateTrigger/>\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState2'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <StateTrigger IsActive='True'/>\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"      </VisualStateManager.VisualStateGroups>\r\n"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>\r\n"
                L"    </StackPanel>\r\n"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Creating StateTrigger..");
                auto stateTrigger = ref new StateTrigger();
            });
        }

        void StateTriggersIntegrationTests::UserControlWithStateTriggersBinding()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>\r\n"
                L"    <StackPanel x:Name='MyStackPanel'>\r\n"
                L"      <VisualStateManager.VisualStateGroups>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>\r\n"
                L"              <VisualState x:Name='VisualState1'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='300' />\r\n"
                L"                      <StateTrigger/>\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState2'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <StateTrigger IsActive='{Binding ElementName=MyStackPanel, Path=IsHitTestVisible}'/>\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"      </VisualStateManager.VisualStateGroups>\r\n"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>\r\n"
                L"    </StackPanel>\r\n"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Creating StateTrigger..");
                auto stateTrigger = ref new StateTrigger();
            });
        }

        void StateTriggersIntegrationTests::UserControlCustomVSM()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>"
                L"    <StackPanel>"
                L"      <VisualStateManager.CustomVisualStateManager>"
                L"          <local:MyCustomVSM x:Name = 'MyCustomVSM' />"
                L"      </VisualStateManager.CustomVisualStateManager>"
                L"      <VisualStateManager.VisualStateGroups>"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>"
                L"              <VisualState x:Name='VisualState1'>"
                L"                  <VisualState.StateTriggers>"
                L"                      <AdaptiveTrigger MinWindowWidth='300' />"
                L"                      <local:CustomStateTrigger/>"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"              <VisualState x:Name='VisualState2'>"
                L"                  <VisualState.StateTriggers>"
                L"                      <local:CustomStateTrigger TriggerValue='True'/>"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"          </VisualStateGroup>"
                L"      </VisualStateManager.VisualStateGroups>"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>"
                L"    </StackPanel>"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
        }

        void StateTriggersIntegrationTests::UserControlCustomVSMUnnamed()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>"
                L"    <StackPanel>"
                L"      <VisualStateManager.CustomVisualStateManager>"
                L"          <local:MyCustomVSM x:Name = 'MyCustomVSM' />"
                L"      </VisualStateManager.CustomVisualStateManager>"
                L"      <VisualStateManager.VisualStateGroups>"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>"
                L"              <VisualState>"
                L"                  <VisualState.StateTriggers>"
                L"                      <AdaptiveTrigger MinWindowWidth='300' />"
                L"                      <local:CustomStateTrigger/>"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"              <VisualState>"
                L"                  <VisualState.StateTriggers>"
                L"                      <local:CustomStateTrigger TriggerValue='True'/>"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"          </VisualStateGroup>"
                L"      </VisualStateManager.VisualStateGroups>"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>"
                L"    </StackPanel>"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
        }

        void StateTriggersIntegrationTests::UserControlCustomVSMUnnamedResize()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>"
                L"    <StackPanel>"
                L"      <VisualStateManager.CustomVisualStateManager>"
                L"          <local:MyCustomVSMWithGoToStateCoreOverride x:Name = 'MyCustomVSM' />"
                L"      </VisualStateManager.CustomVisualStateManager>"
                L"      <VisualStateManager.VisualStateGroups>"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>"
                L"              <VisualState>"
                L"                  <VisualState.StateTriggers>"
                L"                      <AdaptiveTrigger MinWindowWidth='700' />"
                L"                      <local:CustomStateTrigger/>"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"              <VisualState>"
                L"                  <VisualState.StateTriggers>"
                L"                      <AdaptiveTrigger MinWindowWidth='900' />"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"          </VisualStateGroup>"
                L"      </VisualStateManager.VisualStateGroups>"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>"
                L"    </StackPanel>"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
            ResizeDesktopWindow(800, 768);
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
            MaximizeDesktopWindow();
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
            ResizeDesktopWindow(500, 768);
            VerifyCurrentVisualState(rootUserControl, L"Default");
            MaximizeDesktopWindow();
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
            ResizeDesktopWindow(400, 768);
            VerifyCurrentVisualState(rootUserControl, L"Default");
            MaximizeDesktopWindow();
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
        }

        void StateTriggersIntegrationTests::UserControlResourceDictionaryTrigger()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>"
                L"    <StackPanel>"
                L"    <StackPanel.Resources>"
                L"      <local:CustomStateTrigger TriggerValue='True' x:Key='customTrigger' />"
                L"    </StackPanel.Resources>"
                L"      <VisualStateManager.VisualStateGroups>"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>"
                L"              <VisualState StateTriggers = '{StaticResource customTrigger}'>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"          </VisualStateGroup>"
                L"      </VisualStateManager.VisualStateGroups>"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>"
                L"    </StackPanel>"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
        }

        void StateTriggersIntegrationTests::UserControlResourceDictionaryTrigger2()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>"
                L"    <StackPanel>"
                L"    <StackPanel.Resources>"
                L"      <local:CustomStateTrigger TriggerValue='True' x:Key='customTrigger' />"
                L"    </StackPanel.Resources>"
                L"      <VisualStateManager.VisualStateGroups>"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>"
                L"              <VisualState>"
                L"                  <VisualState.StateTriggers>"
                L"                      <StaticResource ResourceKey='customTrigger' />"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"          </VisualStateGroup>"
                L"      </VisualStateManager.VisualStateGroups>"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>"
                L"    </StackPanel>"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
        }

        void StateTriggersIntegrationTests::UserControlResourceDictionaryTrigger3()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>"
                L"    <StackPanel>"
                L"    <StackPanel.Resources>"
                L"      <local:CustomStateTrigger TriggerValue='True' x:Key='customTrigger' />"
                L"    </StackPanel.Resources>"
                L"      <VisualStateManager.VisualStateGroups>"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>"
                L"              <VisualState>"
                L"                  <VisualState.StateTriggers>"
                L"                      <StaticResource ResourceKey='customTrigger' />"
                L"                      <AdaptiveTrigger MinWindowWidth='3000' />"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"          </VisualStateGroup>"
                L"      </VisualStateManager.VisualStateGroups>"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>"
                L"    </StackPanel>"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
        }

        void StateTriggersIntegrationTests::UserControlResourceDictionaryTrigger4()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>"
                L"    <StackPanel>"
                L"    <StackPanel.Resources>"
                L"      <local:CustomStateTrigger TriggerValue='True' x:Key='customTrigger' />"
                L"    </StackPanel.Resources>"
                L"      <VisualStateManager.VisualStateGroups>"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>"
                L"              <VisualState>"
                L"                  <VisualState.StateTriggers>"
                L"                      <AdaptiveTrigger MinWindowWidth='3000' />"
                L"                      <StaticResource ResourceKey='customTrigger' />"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"          </VisualStateGroup>"
                L"      </VisualStateManager.VisualStateGroups>"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>"
                L"    </StackPanel>"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
        }

        void StateTriggersIntegrationTests::UserControlResourceDictionary1()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>"
                L"    <StackPanel>"
                L"    <StackPanel.Resources>"
                L"      <local:CustomStateTrigger TriggerValue='True' x:Key='customTrigger' />"
                L"    </StackPanel.Resources>"
                L"      <VisualStateManager.VisualStateGroups>"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>"
                L"              <VisualState>"
                L"                  <VisualState.StateTriggers>"
                L"                      <AdaptiveTrigger MinWindowWidth='300' />"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"              <VisualState>"
                L"                  <VisualState.StateTriggers>"
                L"                      <StaticResource ResourceKey='customTrigger' />"
                L"                      <AdaptiveTrigger MinWindowWidth='100' />"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"          </VisualStateGroup>"
                L"      </VisualStateManager.VisualStateGroups>"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>"
                L"    </StackPanel>"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
        }

        void StateTriggersIntegrationTests::UserControlResourceDictionary2()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>"
                L"    <StackPanel>"
                L"    <StackPanel.Resources>"
                L"      <local:CustomStateTrigger TriggerValue='True' x:Key='customTrigger' />"
                L"    </StackPanel.Resources>"
                L"      <VisualStateManager.VisualStateGroups>"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>"
                L"              <VisualState>"
                L"                  <VisualState.StateTriggers>"
                L"                      <AdaptiveTrigger MinWindowWidth='300' />"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"              <VisualState>"
                L"                  <VisualState.StateTriggers>"
                L"                      <AdaptiveTrigger MinWindowWidth='100' />"
                L"                      <StaticResource ResourceKey='customTrigger' />"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"          </VisualStateGroup>"
                L"      </VisualStateManager.VisualStateGroups>"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>"
                L"    </StackPanel>"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
       }

        void StateTriggersIntegrationTests::UserControlResourceDictionarySharedExtensibleTrigger()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>"
                L"    <StackPanel x:Name = 'rootStackPanel'>"
                L"    <StackPanel.Resources>"
                L"      <local:CustomStateTrigger TriggerValue='True' x:Key='customTrigger' />"
                L"    </StackPanel.Resources>"
                L"      <VisualStateManager.VisualStateGroups>"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>"
                L"              <VisualState>"
                L"                  <VisualState.StateTriggers>"
                L"                      <AdaptiveTrigger MinWindowWidth='300' />"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"              <VisualState>"
                L"                  <VisualState.StateTriggers>"
                L"                      <AdaptiveTrigger MinWindowWidth='100' />"
                L"                      <StaticResource ResourceKey='customTrigger' />"
                L"                      <AdaptiveTrigger MinWindowWidth='200' />"
                L"                      <AdaptiveTrigger MinWindowWidth='300' />"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"          </VisualStateGroup>"
                L"          <VisualStateGroup x:Name='VisualStateGroup2'>"
                L"              <VisualState>"
                L"                  <VisualState.StateTriggers>"
                L"                      <AdaptiveTrigger MinWindowWidth='450' />"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateNameGroup2'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"              <VisualState>"
                L"                  <VisualState.StateTriggers>"
                L"                      <StaticResource ResourceKey='customTrigger' />"
                L"                      <AdaptiveTrigger MinWindowWidth='100' />"
                L"                      <AdaptiveTrigger MinWindowWidth='200' />"
                L"                      <AdaptiveTrigger MinWindowWidth='300' />"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateNameGroup2'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState4' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"          </VisualStateGroup>"
                L"          <VisualStateGroup x:Name='VisualStateGroup3'>"
                L"              <VisualState>"
                L"                  <VisualState.StateTriggers>"
                L"                      <AdaptiveTrigger MinWindowWidth='300' />"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateNameGroup3'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState5' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"              <VisualState>"
                L"                  <VisualState.StateTriggers>"
                L"                      <AdaptiveTrigger MinWindowWidth='100' />"
                L"                      <AdaptiveTrigger MinWindowWidth='200' />"
                L"                      <AdaptiveTrigger MinWindowWidth='300' />"
                L"                      <StaticResource ResourceKey='customTrigger' />"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateNameGroup3'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState6' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"          </VisualStateGroup>"
                L"      </VisualStateManager.VisualStateGroups>"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>"
                L"    <TextBlock x:Name='VisualStateNameGroup2' Text='Default'/>"
                L"    <TextBlock x:Name='VisualStateNameGroup3' Text='Default'/>"
                L"    </StackPanel>"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
            VerifyCurrentVisualState(rootUserControl, L"VisualState4", L"VisualStateNameGroup2");
            VerifyCurrentVisualState(rootUserControl, L"VisualState6", L"VisualStateNameGroup3");
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
            auto stackPanel = static_cast<StackPanel^>(VerifyGetNamedObject(rootUserControl, L"rootStackPanel"));
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"XAML tree created");
                auto resources = stackPanel->Resources;
                auto stateTrigger = static_cast<CustomStateTrigger^>(resources->Lookup(L"customTrigger"));
                stateTrigger->TriggerValue = false;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
            VerifyCurrentVisualState(rootUserControl, L"VisualState3", L"VisualStateNameGroup2");
            VerifyCurrentVisualState(rootUserControl, L"VisualState5", L"VisualStateNameGroup3");
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
       }

        void StateTriggersIntegrationTests::UserControlResourceDictionarySharedAdaptiveTrigger()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>"
                L"    <StackPanel x:Name = 'rootStackPanel'>"
                L"    <StackPanel.Resources>"
                L"      <AdaptiveTrigger MinWindowWidth='302' x:Key='customTrigger'/>"
                L"    </StackPanel.Resources>"
                L"      <VisualStateManager.VisualStateGroups>"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>"
                L"              <VisualState>"
                L"                  <VisualState.StateTriggers>"
                L"                      <AdaptiveTrigger MinWindowWidth='301' />"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"              <VisualState>"
                L"                  <VisualState.StateTriggers>"
                L"                      <AdaptiveTrigger MinWindowWidth='100' />"
                L"                      <StaticResource ResourceKey='customTrigger' />"
                L"                      <AdaptiveTrigger MinWindowWidth='200' />"
                L"                      <AdaptiveTrigger MinWindowWidth='300' />"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"          </VisualStateGroup>"
                L"          <VisualStateGroup x:Name='VisualStateGroup2'>"
                L"              <VisualState>"
                L"                  <VisualState.StateTriggers>"
                L"                      <AdaptiveTrigger MinWindowWidth='301' />"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateNameGroup2'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"              <VisualState>"
                L"                  <VisualState.StateTriggers>"
                L"                      <StaticResource ResourceKey='customTrigger' />"
                L"                      <AdaptiveTrigger MinWindowWidth='100' />"
                L"                      <AdaptiveTrigger MinWindowWidth='200' />"
                L"                      <AdaptiveTrigger MinWindowWidth='300' />"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateNameGroup2'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState4' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"          </VisualStateGroup>"
                L"          <VisualStateGroup x:Name='VisualStateGroup3'>"
                L"              <VisualState>"
                L"                  <VisualState.StateTriggers>"
                L"                      <AdaptiveTrigger MinWindowWidth='301' />"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateNameGroup3'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState5' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"              <VisualState>"
                L"                  <VisualState.StateTriggers>"
                L"                      <AdaptiveTrigger MinWindowWidth='100' />"
                L"                      <AdaptiveTrigger MinWindowWidth='200' />"
                L"                      <AdaptiveTrigger MinWindowWidth='300' />"
                L"                      <StaticResource ResourceKey='customTrigger' />"
                L"                  </VisualState.StateTriggers>"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateNameGroup3'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState6' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>"
                L"          </VisualStateGroup>"
                L"      </VisualStateManager.VisualStateGroups>"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>"
                L"    <TextBlock x:Name='VisualStateNameGroup2' Text='Default'/>"
                L"    <TextBlock x:Name='VisualStateNameGroup3' Text='Default'/>"
                L"    </StackPanel>"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
            VerifyCurrentVisualState(rootUserControl, L"VisualState4", L"VisualStateNameGroup2");
            VerifyCurrentVisualState(rootUserControl, L"VisualState6", L"VisualStateNameGroup3");
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
            auto stackPanel = static_cast<StackPanel^>(VerifyGetNamedObject(rootUserControl, L"rootStackPanel"));
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"XAML tree created");
                auto resources = stackPanel->Resources;
                auto stateTrigger = static_cast<AdaptiveTrigger^>(resources->Lookup(L"customTrigger"));
                stateTrigger->MinWindowWidth = 1;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
            VerifyCurrentVisualState(rootUserControl, L"VisualState3", L"VisualStateNameGroup2");
            VerifyCurrentVisualState(rootUserControl, L"VisualState5", L"VisualStateNameGroup3");
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
       }

        void StateTriggersIntegrationTests::UserControlAdaptiveTriggerBinding()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"    <StackPanel x:Name='MyStackPanel'>\r\n"
                L"      <VisualStateManager.VisualStateGroups>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>\r\n"
                L"              <VisualState x:Name='VisualState1'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth='{Binding ElementName=MyStackPanel, Path=Height}' />"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"      </VisualStateManager.VisualStateGroups>\r\n"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>\r\n"
                L"    </StackPanel>\r\n"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
        }

        void StateTriggersIntegrationTests::NestedGrids()
        {
            Platform::String^ xamlContents =
            L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"             xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"    <Grid>\r\n"
            L"        <Grid.ColumnDefinitions>\r\n"
            L"            <ColumnDefinition x:Name='MyCol' Width='1*' />\r\n"
            L"            <ColumnDefinition x:Name='MyOtherCol' Width='1*' />\r\n"
            L"            <ColumnDefinition x:Name='MyOtherOtherCol' Width='1*' />\r\n"
            L"        </Grid.ColumnDefinitions>\r\n"
            L"        <VisualStateManager.VisualStateGroups>\r\n"
            L"            <VisualStateGroup>\r\n"
            L"                <VisualState>\r\n"
            L"                    <VisualState.StateTriggers>\r\n"
            L"                        <AdaptiveTrigger MinWindowWidth='0' />\r\n"
            L"                    </VisualState.StateTriggers>\r\n"
            L"                    <VisualState.Setters>\r\n"
            L"                        <Setter Target='MyCol.Width' Value='1*' />\r\n"
            L"                        <Setter Target='MyOtherCol.Width' Value='1*' />\r\n"
            L"                        <Setter Target='Grid1.Background' Value='Green' />\r\n"
            L"                    </VisualState.Setters>\r\n"
            L"                </VisualState>\r\n"
            L"                <VisualState>\r\n"
            L"                    <VisualState.StateTriggers>\r\n"
            L"                        <AdaptiveTrigger MinWindowWidth='300' />\r\n"
            L"                    </VisualState.StateTriggers>\r\n"
            L"                    <VisualState.Setters>\r\n"
            L"                        <Setter Target='MyCol.Width' Value='2*' />\r\n"
            L"                        <Setter Target='MyOtherCol.Width' Value='1*' />\r\n"
            L"                        <Setter Target='Grid1.Background' Value='Orange' />\r\n"
            L"                        <Setter Target='VisualStateName.Text' Value='VisualState2'/>"
            L"                    </VisualState.Setters>\r\n"
            L"                </VisualState>\r\n"
            L"                <VisualState>\r\n"
            L"                    <VisualState.StateTriggers>\r\n"
            L"                        <AdaptiveTrigger MinWindowWidth='9999' />\r\n"
            L"                    </VisualState.StateTriggers>\r\n"
            L"                    <VisualState.Setters>\r\n"
            L"                        <Setter Target='MyCol.Width' Value='3*' />\r\n"
            L"                        <Setter Target='MyOtherCol.Width' Value='1*' />\r\n"
            L"                        <Setter Target='Grid1.Background' Value='White' />\r\n"
            L"                    </VisualState.Setters>\r\n"
            L"                </VisualState>\r\n"
            L"            </VisualStateGroup>\r\n"
            L"        </VisualStateManager.VisualStateGroups>\r\n"
            L"        <Grid x:Name='Grid1' Grid.Column='0' Background='Red' />\r\n"
            L"        <Grid Grid.Column='1' Background='Blue' />\r\n"
            L"        <TextBlock Grid.Column='2' x:Name='VisualStateName' Text='Default'/>\r\n"
            L"    </Grid>\r\n"
            L"</UserControl>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = false;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");
        }

        void StateTriggersIntegrationTests::UserControlModifyProgrammaticallyAddedTriggers()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"    <StackPanel>\r\n"
                L"      <VisualStateManager.VisualStateGroups>\r\n"
                L"          <VisualStateGroup x:Name='VisualStateGroup1'>\r\n"
                L"              <VisualState x:Name='VisualState1'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger x:Name='AdaptiveTrigger1' MinWindowWidth='300' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState1' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name='VisualState2'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger x:Name='AdaptiveTrigger2' MinWindowWidth='450' />\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                <Storyboard>"
                L"                     <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
                L"                          <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
                L"                     </ObjectAnimationUsingKeyFrames>"
                L"                </Storyboard>"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"      </VisualStateManager.VisualStateGroups>\r\n"
                L"    <TextBlock x:Name='VisualStateName' Text='Default'/>\r\n"
                L"    </StackPanel>\r\n"
                L"</UserControl>";

            TestCleanupWrapper cleanup;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");

            // Get the StateTriggerCollection using public APIs
            auto stateTriggerCollection = GetStateTriggerCollection(rootUserControl, 0, 1);

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"StateTrigger count: %d", stateTriggerCollection->Size);
                LOG_OUTPUT(L"Removing StateTrigger: %d", 0);
                stateTriggerCollection->RemoveAt(0);
                LOG_OUTPUT(L"StateTrigger count: %d", stateTriggerCollection->Size);
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");

            RunOnUIThread([&]()
            {
                AdaptiveTrigger^ adaptiveTrigger = ref new AdaptiveTrigger();
                adaptiveTrigger->MinWindowWidth = 301;
                LOG_OUTPUT(L"Appending StateTrigger");
                stateTriggerCollection->Append(adaptiveTrigger);
                LOG_OUTPUT(L"StateTrigger count: %d", stateTriggerCollection->Size);
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");

            // Get the StateTriggerCollection using public APIs
            auto stateTriggerCollection1 = GetStateTriggerCollection(rootUserControl, 0, 0);
            AdaptiveTrigger^ adaptiveTrigger;
            RunOnUIThread([&]()
            {
                adaptiveTrigger = ref new AdaptiveTrigger();
                adaptiveTrigger->MinWindowWidth = 451;
                LOG_OUTPUT(L"Appending StateTrigger");
                stateTriggerCollection1->Append(adaptiveTrigger);
                LOG_OUTPUT(L"StateTrigger count: %d", stateTriggerCollection1->Size);
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");

            RunOnUIThread([&]()
            {
                adaptiveTrigger->MinWindowWidth = 299;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");

            RunOnUIThread([&]()
            {
                adaptiveTrigger->MinWindowWidth = 451;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState1");
        }

        void StateTriggersIntegrationTests::TemplatedControlModifyProgrammaticallyAddedTriggers()
        {
            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>\r\n"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel>"
            L"    <VisualStateManager.VisualStateGroups>"
            L"        <VisualStateGroup x:Name='VisualStateGroup1'>"
            L"            <VisualState x:Name='VisualState1'/>"
            L"            <VisualState x:Name='VisualState2'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState2' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='450' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                                L"<local:CustomStateTrigger />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"            <VisualState x:Name='VisualState3'>"
            L"                <Storyboard>"
            L"                  <ObjectAnimationUsingKeyFrames Storyboard.TargetProperty='Text' Storyboard.TargetName='VisualStateName'>"
            L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='VisualState3' />"
            L"                     </ObjectAnimationUsingKeyFrames>"
            L"                </Storyboard>"
                            L"<VisualState.StateTriggers>\r\n"
                                L"<AdaptiveTrigger MinWindowWidth='300' />\r\n"
                                L"<AdaptiveTrigger MinWindowHeight='1' />\r\n"
                                L"<local:CustomStateTrigger />\r\n"
                            L"</VisualState.StateTriggers>\r\n"
            L"            </VisualState>"
            L"        </VisualStateGroup>"
            L"    </VisualStateManager.VisualStateGroups>"
            L"    <TextBlock x:Name='VisualStateName' Text='Default' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;
            auto rootUserControl = CreateXamlTree(xamlContents);
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");

            // Get the StateTriggerCollection using public APIs
            auto stateTriggerCollection = GetStateTriggerCollection(rootUserControl, 0, 1);

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"StateTrigger count: %d", stateTriggerCollection->Size);
                LOG_OUTPUT(L"Removing StateTrigger: %d", 0);
                stateTriggerCollection->RemoveAt(1);
                LOG_OUTPUT(L"StateTrigger count: %d", stateTriggerCollection->Size);
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState3");

            AdaptiveTrigger^ adaptiveTrigger;
            RunOnUIThread([&]()
            {
                adaptiveTrigger = ref new AdaptiveTrigger();
                adaptiveTrigger->MinWindowWidth = 451;
                LOG_OUTPUT(L"Appending StateTrigger");
                stateTriggerCollection->Append(adaptiveTrigger);
                LOG_OUTPUT(L"StateTrigger count: %d", stateTriggerCollection->Size);
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState2");

            RunOnUIThread([&]()
            {
                adaptiveTrigger->MinWindowWidth = 100;
            });
            VerifyCurrentVisualState(rootUserControl, L"VisualState3");
        }

        void StateTriggersIntegrationTests::TemplatedPivotHeaderItem()
        {
            Platform::String^ xamlContents =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"        xmlns:local='using:Private.Tests.Adaptability.StateTriggerTests'>\r\n"
            L"  <Button.Resources>"
            L"        <Style x:Key='TabPivotStyle' TargetType='Pivot'>"
            L"            <Setter Property='Template'>"
            L"                <Setter.Value>"
            L"                    <ControlTemplate TargetType='Pivot'>"
            L"                        <Grid x:Name='RootElement'>"
            L"                            <Grid.Resources>"
            L"                                <Style x:Key='BaseContentControlStyle' TargetType='ContentControl'>"
            L"                                    <Setter Property='Template'>"
            L"                                        <Setter.Value>"
            L"                                            <ControlTemplate TargetType='ContentControl'>"
            L"                                                <ContentPresenter"
            L"                                                    Content='{TemplateBinding Content}'"
            L"                                                    ContentTemplate='{TemplateBinding ContentTemplate}'/>"
            L"                                            </ControlTemplate>"
            L"                                        </Setter.Value>"
            L"                                    </Setter>"
            L"                                </Style>"
            L"                                <!--  Template for a Tab item, must be fed a TabHeaderTemplate control as content  -->"
            L"                                <Style TargetType='PivotHeaderItem'>"
            L"                                    <Setter Property='Template'>"
            L"                                        <Setter.Value>"
            L"                                            <ControlTemplate TargetType='PivotHeaderItem'>"
            L"                                                <Grid x:Name='Grid' Background='{TemplateBinding Background}'>"
            L"                                                    <VisualStateManager.VisualStateGroups>"
            L"                                                        <VisualStateGroup x:Name='AdaptiveStates'>"
            L"                                                            <VisualState x:Name='Narrow'>"
            L"                                                            </VisualState>"
            L"                                                            <VisualState x:Name='Wide'>"
            L"                                                                <VisualState.StateTriggers>"
            L"                                                                    <AdaptiveTrigger MinWindowWidth='401' />"
            L"                                                                </VisualState.StateTriggers>"
            L"                                                                <VisualState.Setters>"
            L"                                                                    <Setter Target='ContentPresenter.Background' Value='Pink' />"
            L"                                                                </VisualState.Setters>"
            L"                                                            </VisualState>"
            L"                                                        </VisualStateGroup>"
            L"                                                    </VisualStateManager.VisualStateGroups>"
            L"                                                    <ContentPresenter"
            L"                                                        x:Name='ContentPresenter'"
            L"                                                        Content='{TemplateBinding Content}'"
            L"                                                        ContentTemplate='{TemplateBinding ContentTemplate}'>"
            L"                                                    </ContentPresenter>"
            L"                                                </Grid>"
            L"                                            </ControlTemplate>"
            L"                                        </Setter.Value>"
            L"                                    </Setter>"
            L"                                </Style>"
            L"                            </Grid.Resources>                              "
            L"                            <Grid.RowDefinitions>"
            L"                                <RowDefinition Height='Auto' />"
            L"                                <RowDefinition Height='*' />"
            L"                            </Grid.RowDefinitions>"
            L"                            <ContentControl"
            L"                                x:Name='TitleContentControl'"
            L"                                Content='{TemplateBinding Title}'"
            L"                                ContentTemplate='{TemplateBinding TitleTemplate}'/>"
            L"                            <Grid Grid.Row='1'>"
            L"                                <Grid.Resources>"
            L"                                </Grid.Resources>"
            L"                                <ScrollViewer"
            L"                                    x:Name='ScrollViewer'"
            L"                                    HorizontalSnapPointsType='MandatorySingle'"
            L"                                    Template='{StaticResource ScrollViewerScrollBarlessTemplate}'>"
            L"                                    <PivotPanel x:Name='Panel' VerticalAlignment='Stretch'>"
            L"                                        <Grid x:Name='PivotLayoutElement'>"
            L"                                            <Grid.RowDefinitions>"
            L"                                                <RowDefinition Height='Auto' />"
            L"                                                <RowDefinition Height='*' />"
            L"                                            </Grid.RowDefinitions>"
            L"                                            <Grid.ColumnDefinitions>"
            L"                                                <ColumnDefinition x:Name='PivotLayoutElementColumn1' Width='*' />"
            L"                                                <ColumnDefinition x:Name='PivotLayoutElementColumn2' Width='Auto' />"
            L"                                                <ColumnDefinition x:Name='PivotLayoutElementColumn3' Width='*' />"
            L"                                            </Grid.ColumnDefinitions>"
            L"                                            <Grid.RenderTransform>"
            L"                                                <CompositeTransform x:Name='PivotLayoutElementTranslateTransform' />"
            L"                                            </Grid.RenderTransform>"
            L"                                            <Border Grid.ColumnSpan='3'/>"
            L"                                            <ContentPresenter"
            L"                                                x:Name='LeftHeaderPresenter'"
            L"                                                Content='{TemplateBinding LeftHeader}'"
            L"                                                ContentTemplate='{TemplateBinding LeftHeaderTemplate}' />"
            L"                                            <ContentControl"
            L"                                                x:Name='HeaderClipper'"
            L"                                                Grid.Column='1'>"
            L"                                                <Grid Background='Transparent'>"
            L"                                                    <PivotHeaderPanel x:Name='StaticHeader' Visibility='Visible' />"
            L"                                                    <PivotHeaderPanel x:Name='Header'/>"
            L"                                                </Grid>"
            L"                                            </ContentControl>"
            L"                                            <ItemsPresenter x:Name='PivotItemPresenter' Grid.Row='1' Grid.ColumnSpan='3'>"
            L"                                                <ItemsPresenter.RenderTransform>"
            L"                                                    <TransformGroup>"
            L"                                                        <TranslateTransform x:Name='ItemsPresenterTranslateTransform' />"
            L"                                                        <CompositeTransform x:Name='ItemsPresenterCompositeTransform' />"
            L"                                                    </TransformGroup>"
            L"                                                </ItemsPresenter.RenderTransform>"
            L"                                            </ItemsPresenter> "
            L"                                        </Grid>"
            L"                                    </PivotPanel>"
            L"                                </ScrollViewer>"
            L"                            </Grid>"
            L"                        </Grid>"
            L"                    </ControlTemplate>"
            L"                </Setter.Value>"
            L"            </Setter>"
            L"        </Style>"
            L"    </Button.Resources>"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"    <Grid>"
            L"        <Pivot x:Name='MyTabPivot' Style='{StaticResource TabPivotStyle}'>"
            L"            <Pivot.Items>"
            L"                <PivotItem Header='MyItem'>"
            L"                    <TextBlock Text='Test'/>"
            L"                </PivotItem>"
            L"                <PivotItem Header='MyItem2'>"
            L"                    <TextBlock Text='Test'/>"
            L"                </PivotItem>"
            L"           </Pivot.Items>"
            L"        </Pivot>"
            L"    </Grid>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;
            auto rootUserControl = CreateXamlTree(xamlContents);
            MaximizeDesktopWindow();
            ResizeDesktopWindow(400, 768);
            MaximizeDesktopWindow();
        }

        void StateTriggersIntegrationTests::ExecuteVSParserError(bool LoadWithInitialTemplateValidation)
        {
            LOG_OUTPUT(L"ParseQualifier");

            Platform::String^ xamlContents =
            L"<ContentControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
            L"                xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"<ContentControl.Template>\r\n"
                    L"<ControlTemplate>\r\n"
                        L"<Grid x:Name='RootGrid'>\r\n"
                            L"<VisualStateManager.VisualStateGroups>\r\n"
                                L"<VisualStateGroup>\r\n"
                                    L"<VisualState>\r\n"
                                        L"<VisualState.Setters>\r\n"
                                                L"<Setter Target='RootGrid.Background' Value='Red'/>\r\n"
                                        L"</VisualState.Setters>\r\n"
                                    L"</VisualState>\r\n"
                                L"</VisualStateGroup>\r\n"
                            L"</VisualStateManager.VisualStateGroups>\r\n"
                        L"</Grid>\r\n"
                    L"</ControlTemplate>\r\n"
                L"</ContentControl.Template>\r\n"
            L"</ContentControl>\r\n";

            TestCleanupWrapper cleanup;
            m_isTemplatedControl = true;

            Control^ rootUserControl;
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"CreateXamlTree()");

                if(LoadWithInitialTemplateValidation)
                {
                    rootUserControl = safe_cast<Control^>(XamlReader::LoadWithInitialTemplateValidation(xamlContents));
                }
                else
                {
                    rootUserControl = safe_cast<Control^>(XamlReader::Load(xamlContents));
                }
                TestServices::WindowHelper->WindowContent = rootUserControl;
                LOG_OUTPUT(L"XAML tree created");
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void StateTriggersIntegrationTests::VSParserError()
        {
           RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);
           ExecuteVSParserError(false);
        }

        void StateTriggersIntegrationTests::VSParserErrorLegacy()
        {
           RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);
           ExecuteVSParserError(false);
        }

        void StateTriggersIntegrationTests::VSParserErrorTemplateValidation()
        {
           RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);
           ExecuteVSParserError(true);
        }

        void StateTriggersIntegrationTests::VSParserErrorTemplateValidationLegacy()
        {
           RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);
           ExecuteVSParserError(true);
        }

       void StateTriggersIntegrationTests::GridLegacy()
       {
           RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);
           Grid();
       }

       void StateTriggersIntegrationTests::GridWithSettersLegacy()
       {
           RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);
           GridWithSetters();
       }

        Control^ StateTriggersIntegrationTests::CreateXamlTree(Platform::String^ xamlString)
        {
            Control^ rootUserControl;
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"CreateXamlTree()");
                rootUserControl = safe_cast<Control^>(XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = rootUserControl;
                LOG_OUTPUT(L"XAML tree created");
            });

            TestServices::WindowHelper->WaitForIdle();

            return rootUserControl;
        }

        void StateTriggersIntegrationTests::VerifyCurrentVisualState(Control^ rootUserControl, Platform::String^ expectedVisualStateName, Platform::String^ targetName)
        {
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"VerifyCurrentVisualState: %s", expectedVisualStateName->Begin());

                TextBlock^ textBlock;
                if (!m_isTemplatedControl)
                {
                    LOG_OUTPUT(L"FindName: %s", targetName->Begin());
                    textBlock= safe_cast<TextBlock^>(rootUserControl->FindName(targetName));
                }
                else
                {
                    LOG_OUTPUT(L"GetTemplateChild: %s", targetName->Begin());
                    textBlock= safe_cast<TextBlock^>(rootUserControl->GetTemplateChild(targetName));
                }
                VERIFY_IS_NOT_NULL(textBlock);

                LOG_OUTPUT(L"Actual VisualState: %s", textBlock->Text->Begin());
                LOG_OUTPUT(L"Expected VisualState: %s", expectedVisualStateName->Begin());
                VERIFY_ARE_EQUAL(textBlock->Text, expectedVisualStateName);
            });
        }

        DependencyObject^ StateTriggersIntegrationTests::VerifyGetNamedObject(Control^ rootUserControl, Platform::String^ objectName)
        {
            DependencyObject^ depObject;

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"VerifyGetNamedObject: %s", objectName->Begin());

                if(!m_isTemplatedControl)
                {
                    LOG_OUTPUT(L"FindName: %s", objectName->Begin());
                    depObject = safe_cast<DependencyObject^>(rootUserControl->FindName(objectName));
                }
                else
                {
                    LOG_OUTPUT(L"GetTemplateChild: %s", objectName->Begin());
                    depObject = safe_cast<DependencyObject^>(rootUserControl->GetTemplateChild(objectName));
                }
                VERIFY_IS_NOT_NULL(depObject);
            });

            TestServices::WindowHelper->WaitForIdle();

            return depObject;
        }

        IVector<Microsoft::UI::Xaml::StateTriggerBase ^>^ StateTriggersIntegrationTests::GetStateTriggerCollection(Control^ rootUserControl, unsigned int groupIndex, unsigned int visualStateIndex)
        {
            IVector<Microsoft::UI::Xaml::StateTriggerBase ^>^ stateTriggers;

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"GetStateTriggerCollection(rootUserControl, groupIndex, visualStateIndex): %d, %d", groupIndex, visualStateIndex);

                auto firstChild = (FrameworkElement^)xaml_media::VisualTreeHelper::GetChild(rootUserControl, 0);
                VERIFY_IS_NOT_NULL(firstChild);

                auto groups = VisualStateManager::GetVisualStateGroups(firstChild);
                VERIFY_IS_NOT_NULL(groups);

                auto group = groups->GetAt(groupIndex);
                VERIFY_IS_NOT_NULL(group);

                auto visualStates = group->States;
                VERIFY_IS_NOT_NULL(visualStates);

                auto visualState = visualStates->GetAt(visualStateIndex);
                VERIFY_IS_NOT_NULL(visualState);

                stateTriggers = visualState->StateTriggers;
                VERIFY_IS_NOT_NULL(stateTriggers);
            });

            TestServices::WindowHelper->WaitForIdle();

            return stateTriggers;
        };

        void StateTriggersIntegrationTests::CauseVisualStateGroupCollectionToFaultIn(Control^ rootUserControl)
        {
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"CauseVisualStateGroupCollectionToFaultIn()");
                auto firstChild = (FrameworkElement^)xaml_media::VisualTreeHelper::GetChild(rootUserControl, 0);
                auto transitionStoryboard = VisualStateManager::GetVisualStateGroups(firstChild);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void StateTriggersIntegrationTests::ResizeDesktopWindow(unsigned int width, unsigned int height)
        {
            LOG_OUTPUT(L"Resizing window to %d x %d", width, height);
            TestServices::WindowHelper->SetDesktopWindowSize(width, height);
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(0, TestServices::WindowHelper->IsDesktopWindowMaximized());
        }

        void StateTriggersIntegrationTests::MaximizeDesktopWindow()
        {
            LOG_OUTPUT(L"Maximizing desktop window");
            TestServices::WindowHelper->MaximizeDesktopWindow();
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(1, TestServices::WindowHelper->IsDesktopWindowMaximized());
        }

        void StateTriggersIntegrationTests::ChangeVisualState(Control^ rootUserControl, Platform::String^ newVisualState, bool useTransitions)
        {
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"ChangeVisualState: %s", newVisualState->Begin());
                VisualStateManager::GoToState(rootUserControl, newVisualState, useTransitions);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void StateTriggersIntegrationTests::VisualStateGroupsNullState()
        {
            Platform::String^ xamlContents =
                L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"  <Grid Background = '{ThemeResource ApplicationPageBackgroundThemeBrush}'>\r\n"
                L"      <VisualStateManager.VisualStateGroups>\r\n"
                L"          <VisualStateGroup x:Name = 'ColorStates'>\r\n"
                L"              <VisualState x:Name = 'RedState'>\r\n"
                L"                  <VisualState.Setters>\r\n"
                L"                      <Setter Target = 'VisualStateName.Text' Value = 'RedState'/>\r\n"
                L"                  </VisualState.Setters>\r\n"
                L"              </VisualState>\r\n"
                L"              <VisualState x:Name = 'BlueState'>\r\n"
                L"                  <VisualState.Setters>\r\n"
                L"                      <Setter Target = 'VisualStateName.Text' Value = 'BlueState'/>\r\n"
                L"                  </VisualState.Setters>\r\n"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"          <VisualStateGroup x:Name = 'ReflowStates'>\r\n"
                L"              <VisualState x:Name = 'TriggerSize'>\r\n"
                L"                  <VisualState.StateTriggers>\r\n"
                L"                      <AdaptiveTrigger MinWindowWidth = '700'/>\r\n"
                L"                  </VisualState.StateTriggers>\r\n"
                L"                  <VisualState.Setters>\r\n"
                L"                      <Setter Target = 'StackPanel.Orientation' Value = 'Horizontal'/>\r\n"
                L"                  </VisualState.Setters>\r\n"
                L"              </VisualState>\r\n"
                L"          </VisualStateGroup>\r\n"
                L"      </VisualStateManager.VisualStateGroups>\r\n"
                L"      <StackPanel x:Name = 'StackPanel' Orientation = 'Vertical'>\r\n"
                L"          <TextBlock x:Name='VisualStateName' Text='Default' />"
                L"          <Rectangle Width='100' Height='100' Fill='Blue' />"
                L"      </StackPanel>\r\n"
                L"  </Grid>\r\n"
                L"</UserControl>";

            TestCleanupWrapper cleanup;

            auto stateChangedRegistration = CreateSafeEventRegistration(VisualStateGroup, CurrentStateChanged);
            auto stateChangedRegistration2 = CreateSafeEventRegistration(VisualStateGroup, CurrentStateChanged);
            VisualStateGroup^ colorStatesVisualStateGroup = nullptr;
            VisualStateGroup^ reflowStatesVisualStateGroup = nullptr;
            auto newState = std::make_shared<String^>();
            auto oldState = std::make_shared<String^>();
            auto reflowState = std::make_shared<String^>();

            auto rootUserControl = CreateXamlTree(xamlContents);

            RunOnUIThread([&]()
            {
                colorStatesVisualStateGroup = safe_cast<VisualStateGroup^>(rootUserControl->FindName("ColorStates"));
                reflowStatesVisualStateGroup = safe_cast<VisualStateGroup^>(rootUserControl->FindName("ReflowStates"));
            });

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));
            TestServices::WindowHelper->WaitForIdle();

            stateChangedRegistration.Attach(colorStatesVisualStateGroup, ref new xaml::VisualStateChangedEventHandler(
                [newState, oldState](Platform::Object^, xaml::VisualStateChangedEventArgs^ args)
            {
                // NewState should not be NULL because we are calling only available ColorStates
                VERIFY_IS_NOT_NULL(args->NewState);
                VERIFY_ARE_EQUAL(args->NewState->Name, *newState);

                if (args->OldState != nullptr)
                {
                    VERIFY_ARE_EQUAL(args->OldState->Name, *oldState);
                }
                else
                {
                    VERIFY_IS_TRUE(*oldState == "");
                }

                *oldState = args->NewState->Name;
            }));

            stateChangedRegistration2.Attach(reflowStatesVisualStateGroup, ref new xaml::VisualStateChangedEventHandler(
                [reflowState](Platform::Object^, xaml::VisualStateChangedEventArgs^ args)
            {
                // When window width is less than 700 we expect a non-existent state so NewState should be NULL
                if (args->NewState != nullptr)
                {
                    VERIFY_ARE_EQUAL(args->NewState->Name, *reflowState);
                }else
                {
                    VERIFY_IS_TRUE(*reflowState == "");
                }
            }));

            // Verify newState/oldState when previous visual state was NULL
            *newState = "RedState";
            *oldState = "";
            ChangeVisualState(rootUserControl, *newState);
            VerifyCurrentVisualState(rootUserControl, *newState);

            // Verify newState/oldState when previous visual state was RedState
            *newState = "BlueState";
            ChangeVisualState(rootUserControl, *newState);
            VerifyCurrentVisualState(rootUserControl, *newState);

            // Resize window to trigger ReflowStates
            *reflowState = "TriggerSize";
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(700, 400));
            TestServices::WindowHelper->WaitForIdle();

            // Validate ReflowState didn't affect ColorStates
            *newState = "RedState";
            ChangeVisualState(rootUserControl, *newState);
            VerifyCurrentVisualState(rootUserControl, *newState);

            // Resize window to trigger ReflowStates, we expect a null state
            *reflowState = "";
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));
            TestServices::WindowHelper->WaitForIdle();

            // Validate ReflowState didn't affect ColorStates
            *newState = "BlueState";
            ChangeVisualState(rootUserControl, *newState);
            VerifyCurrentVisualState(rootUserControl, *newState);
        }
     }
} } } }


