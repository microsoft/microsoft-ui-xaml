// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "VisualStateGroupCollectionCustomWriterBasicTests.h"
#include <ppltasks.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"

#include <RuntimeEnabledFeaturesEnum.h>
#include <IVisualStateGroupCollectionTestHooks.h>
#include <VisualStateGroupCollectionCustomRuntimeData.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {

        static const wchar_t* s_vsg1Name = L"VSG1";
        static const wchar_t* s_vs1Name = L"VS1";
        static const wchar_t* s_vs2Name = L"VS2";

        static const wchar_t* s_sampleVsg1 =
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x = 'http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"<VisualStateManager.VisualStateGroups>"
            L"    <VisualStateGroup x:Name='VSG1'>"
            L"        <VisualState x:Name='VS1'>"
            L"            <Storyboard Duration='0:0:0.1' >"
            L"                <DoubleAnimation Storyboard.TargetName='AnimationTarget' Storyboard.TargetProperty='Opacity' From='0' To='0.5'/>"
            L"            </Storyboard>"
            L"        </VisualState>"
            L"        <VisualState x:Name='VS2'/>"
            L"        <VisualStateGroup.Transitions>"
            L"            <VisualTransition GeneratedDuration='0:0:0.1'>"
            L"                <Storyboard Duration='0:0:0.1'>"
            L"                    <DoubleAnimation Storyboard.TargetName='AnimationTarget' Storyboard.TargetProperty='Opacity' From='1' To='0'/>"
            L"                </Storyboard>"
            L"            </VisualTransition>"
            L"        </VisualStateGroup.Transitions>"
            L"    </VisualStateGroup>"
            L"</VisualStateManager.VisualStateGroups>"
            L"</Grid>";

        static const wchar_t* s_sampleVsg2 =
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x = 'http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"<VisualStateManager.VisualStateGroups>"
            L"    <VisualStateGroup x:Name='VSG1'>"
            L"        <VisualStateGroup.Transitions>"
            L"            <VisualTransition From='VS1' To='VS2'></VisualTransition>"
            L"        </VisualStateGroup.Transitions>"
            L"        <VisualState x:Name='VS1'>"
            L"            <Storyboard Duration='0:0:0.1' >"
            L"                <DoubleAnimation Storyboard.TargetName='AnimationTarget' Storyboard.TargetProperty='Opacity' From='0' To='0.5'/>"
            L"            </Storyboard>"
            L"        </VisualState>"
            L"        <VisualState x:Name='VS2'/>"
            L"        <VisualStateGroup.Transitions>"
            L"            <VisualTransition GeneratedDuration='0:0:0.1'>"
            L"                <Storyboard Duration='0:0:0.1'>"
            L"                    <DoubleAnimation Storyboard.TargetName='AnimationTarget' Storyboard.TargetProperty='Opacity' From='1' To='0'/>"
            L"                </Storyboard>"
            L"            </VisualTransition>"
            L"        </VisualStateGroup.Transitions>"
            L"    </VisualStateGroup>"
            L"    <VisualStateGroup x:Name='VSG2'></VisualStateGroup>"
            L"</VisualStateManager.VisualStateGroups>"
            L"</Grid>";

        static const wchar_t* s_sampleVsg3 =
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x = 'http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"<VisualStateManager.VisualStateGroups>"
            L"</VisualStateManager.VisualStateGroups>"
            L"</Grid>";

        static const wchar_t* s_sampleVsg4 =
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x = 'http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"<VisualStateManager.VisualStateGroups>"
            L"    <VisualStateGroup Name='VSG1'>"
            L"        <VisualState x:Name='VS1'>"
            L"            <Storyboard Duration='0:0:0.1' >"
            L"                <DoubleAnimation Storyboard.TargetName='AnimationTarget' Storyboard.TargetProperty='Opacity' From='0' To='0.5'/>"
            L"            </Storyboard>"
            L"        </VisualState>"
            L"        <VisualState x:Name='VS2'/>"
            L"        <VisualStateGroup.Transitions>"
            L"            <VisualTransition GeneratedDuration='0:0:0.1'>"
            L"                <Storyboard Duration='0:0:0.1'>"
            L"                    <DoubleAnimation Storyboard.TargetName='AnimationTarget' Storyboard.TargetProperty='Opacity' From='1' To='0'/>"
            L"                </Storyboard>"
            L"            </VisualTransition>"
            L"        </VisualStateGroup.Transitions>"
            L"    </VisualStateGroup>"
            L"</VisualStateManager.VisualStateGroups>"
            L"</Grid>";

        static const wchar_t* s_sampleVsg5 =
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x = 'http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"<VisualStateManager.VisualStateGroups>"
            L"    <VisualStateGroup>"
            L"        <VisualStateGroup.Name>VSG1</VisualStateGroup.Name>"
            L"        <VisualState x:Name='VS1'>"
            L"            <Storyboard Duration='0:0:0.1' >"
            L"                <DoubleAnimation Storyboard.TargetName='AnimationTarget' Storyboard.TargetProperty='Opacity' From='0' To='0.5'/>"
            L"            </Storyboard>"
            L"        </VisualState>"
            L"        <VisualState x:Name='VS2'/>"
            L"        <VisualStateGroup.Transitions>"
            L"            <VisualTransition GeneratedDuration='0:0:0.1'>"
            L"                <Storyboard Duration='0:0:0.1'>"
            L"                    <DoubleAnimation Storyboard.TargetName='AnimationTarget' Storyboard.TargetProperty='Opacity' From='1' To='0'/>"
            L"                </Storyboard>"
            L"            </VisualTransition>"
            L"        </VisualStateGroup.Transitions>"
            L"    </VisualStateGroup>"
            L"</VisualStateManager.VisualStateGroups>"
            L"</Grid>";

        static const wchar_t* s_sampleVsg6 =
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x = 'http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"<VisualStateManager.VisualStateGroups>"
            L"    <VisualStateGroup>"
            L"        <VisualStateGroup.Name>VSG1</VisualStateGroup.Name>"
            L"        <VisualState x:Name='VS1'>"
            L"            <Storyboard Duration='0:0:0.1' >"
            L"                <DoubleAnimation Storyboard.TargetName='AnimationTarget' Storyboard.TargetProperty='Opacity' From='0' To='0.5'/>"
            L"            </Storyboard>"
            L"        </VisualState>"
            L"        <VisualState x:Name='VS2'/>"
            L"        <VisualStateGroup.Transitions>"
            L"            <VisualTransition GeneratedDuration='0:0:0.1' To='VS2' From='VS1'>"
            L"                <Storyboard Duration='0:0:0.1'>"
            L"                    <DoubleAnimation Storyboard.TargetName='AnimationTarget' Storyboard.TargetProperty='Opacity' From='1' To='0'/>"
            L"                </Storyboard>"
            L"            </VisualTransition>"
            L"        </VisualStateGroup.Transitions>"
            L"    </VisualStateGroup>"
            L"</VisualStateManager.VisualStateGroups>"
            L"</Grid>";

        bool VisualStateGroupCollectionCustomWriterBasicTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            featureParserCustomWriter.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::ParserCustomWriter, true);
            return true;
        }

        bool VisualStateGroupCollectionCustomWriterBasicTests::ClassCleanup()
        {
            return true;
        }

        bool VisualStateGroupCollectionCustomWriterBasicTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool VisualStateGroupCollectionCustomWriterBasicTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void VisualStateGroupCollectionCustomWriterBasicTests::ValidateVisualStateGroupStringExtraction()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&] () {
                auto vsgcContainingGrid = safe_cast<Grid^>(Markup::XamlReader::Load(
                    Platform::StringReference(s_sampleVsg1)));
                wrl::ComPtr<IVisualStateGroupCollectionTestHooks> vsgcTestHooks;
                GetVisualStateGroupTestHooksFromRootElement(vsgcContainingGrid, &vsgcTestHooks);

                auto visualStateGroupNames = vsgcTestHooks->GetVisualStateGroupNames();
                VERIFY_ARE_EQUAL(visualStateGroupNames->size(), static_cast<size_t>(1));
                VERIFY_ARE_EQUAL((*visualStateGroupNames)[0], std::wstring(s_vsg1Name));
            });
        }

        void VisualStateGroupCollectionCustomWriterBasicTests::ValidateVisualStateGroupStringNonDirectiveExtraction()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]() {
                auto vsgcContainingGrid = safe_cast<Grid^>(Markup::XamlReader::Load(
                    Platform::StringReference(s_sampleVsg4)));
                wrl::ComPtr<IVisualStateGroupCollectionTestHooks> vsgcTestHooks;
                GetVisualStateGroupTestHooksFromRootElement(vsgcContainingGrid, &vsgcTestHooks);

                auto visualStateGroupNames = vsgcTestHooks->GetVisualStateGroupNames();
                VERIFY_ARE_EQUAL(visualStateGroupNames->size(), static_cast<size_t>(1));
                VERIFY_ARE_EQUAL((*visualStateGroupNames)[0], std::wstring(s_vsg1Name));
            });
        }

        void VisualStateGroupCollectionCustomWriterBasicTests::ValidateVisualStateGroupStringAsElementExtraction()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]() {
                auto vsgcContainingGrid = safe_cast<Grid^>(Markup::XamlReader::Load(
                    Platform::StringReference(s_sampleVsg5)));
                wrl::ComPtr<IVisualStateGroupCollectionTestHooks> vsgcTestHooks;
                GetVisualStateGroupTestHooksFromRootElement(vsgcContainingGrid, &vsgcTestHooks);

                auto visualStateGroupNames = vsgcTestHooks->GetVisualStateGroupNames();
                VERIFY_ARE_EQUAL(visualStateGroupNames->size(), static_cast<size_t>(1));
                VERIFY_ARE_EQUAL((*visualStateGroupNames)[0], std::wstring(s_vsg1Name));
            });
        }

        void VisualStateGroupCollectionCustomWriterBasicTests::ValidateVisualStateStringExtraction()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&] () {
                auto vsgcContainingGrid = safe_cast<Grid^>(Markup::XamlReader::Load(
                    Platform::StringReference(s_sampleVsg1)));
                wrl::ComPtr<IVisualStateGroupCollectionTestHooks> vsgcTestHooks;
                GetVisualStateGroupTestHooksFromRootElement(vsgcContainingGrid, &vsgcTestHooks);

                auto visualStateNames = vsgcTestHooks->GetVisualStateNamesForGroup(0);
                VERIFY_ARE_EQUAL(visualStateNames->size(), static_cast<size_t>(2));
                VERIFY_ARE_EQUAL((*visualStateNames)[0], std::wstring(s_vs1Name));
                VERIFY_ARE_EQUAL((*visualStateNames)[1], std::wstring(s_vs2Name));
            });
        }

        void VisualStateGroupCollectionCustomWriterBasicTests::ValidateVisualStateGroupTransitionDetection()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]() {
                auto vsgcContainingGrid = safe_cast<Grid^>(Markup::XamlReader::Load(
                    Platform::StringReference(s_sampleVsg2)));
                wrl::ComPtr<IVisualStateGroupCollectionTestHooks> vsgcTestHooks;
                GetVisualStateGroupTestHooksFromRootElement(vsgcContainingGrid, &vsgcTestHooks);

                auto visualStateGroupNames = vsgcTestHooks->GetVisualStateGroupNames();
                VERIFY_ARE_EQUAL(visualStateGroupNames->size(), static_cast<size_t>(2));
                VERIFY_IS_TRUE(vsgcTestHooks->DoesVisualStateGroupHaveTransitions(0));
                VERIFY_IS_TRUE(!vsgcTestHooks->DoesVisualStateGroupHaveTransitions(1));
            });
        }

        void VisualStateGroupCollectionCustomWriterBasicTests::ValidateEmptyVisualStateGroupCollectionExtraction()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]() {
                auto vsgcContainingGrid = safe_cast<Grid^>(Markup::XamlReader::Load(
                    Platform::StringReference(s_sampleVsg3)));
                wrl::ComPtr<IVisualStateGroupCollectionTestHooks> vsgcTestHooks;
                GetVisualStateGroupTestHooksFromRootElement(vsgcContainingGrid, &vsgcTestHooks);

                auto visualStateGroupNames = vsgcTestHooks->GetVisualStateGroupNames();
                VERIFY_IS_FALSE(visualStateGroupNames);
            });
        }

        void VisualStateGroupCollectionCustomWriterBasicTests::ValidateBasicObjectCreation()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]() {
                auto vsgcContainingGrid = safe_cast<Grid^>(Markup::XamlReader::Load(
                    Platform::StringReference(s_sampleVsg1)));
                wrl::ComPtr<IVisualStateGroupCollectionTestHooks> vsgcTestHooks;
                GetVisualStateGroupTestHooksFromRootElement(vsgcContainingGrid, &vsgcTestHooks);

                wrl::ComPtr<IInspectable> createdObject(vsgcTestHooks->CreateStoryboard(0, 0));
                auto createdObjectInCX = reinterpret_cast<Platform::Object^>(createdObject.Get());

                xaml_animation::Storyboard^ createdStoryboard = dynamic_cast<xaml_animation::Storyboard^>(createdObjectInCX);
                VERIFY_IS_NOT_NULL(createdStoryboard);
            });
        }

        void VisualStateGroupCollectionCustomWriterBasicTests::GetVisualStateGroupTestHooksFromRootElement(
            FrameworkElement^ element, IVisualStateGroupCollectionTestHooks** ppTestHooks)
        {
            wrl::ComPtr<IInspectable> visualStateGroupCollection =
                reinterpret_cast<IInspectable*>(VisualStateManager::GetVisualStateGroups(element));
            wrl::ComPtr<IVisualStateGroupCollectionTestHooks> vsgcTestHooks;
            LogThrow_IfFailed(visualStateGroupCollection.As(&vsgcTestHooks));
            *ppTestHooks = vsgcTestHooks.Detach();
        }

    }
} } } }
