// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "FlyweightTests.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <Utils.h>
#include <array>
#include <algorithm>
#include <DisableErrorReportingScopeGuard.h>

using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace WEX::Logging;
using namespace WEX::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {

        TimeSpan TimeSpan_FromSeconds(double sec)
        {
            TimeSpan ts;
            ts.Duration = static_cast<INT64>(sec * 10000000);
            return ts;
        }

        bool FlyweightTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            featureEnforceXbfV2Stream.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);
            return true;
        }

        bool FlyweightTests::ClassCleanup()
        {
            return true;
        }
         
        bool FlyweightTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml();
            return true;
        }
        
        bool FlyweightTests::TestCleanup()
        {
            TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void FlyweightTests::ValidateGetterSetter()
        {
            TestCleanupWrapper cleanup;

            // Duration

            RunOnUIThread([&]()
            {
                auto obj = ref new ObjectAnimationUsingKeyFrames();
                auto fromDuration = DurationHelper::FromTimeSpan(TimeSpan_FromSeconds(30.0));

                obj->Duration = DurationHelper::Automatic;
                VERIFY_ARE_EQUAL(DurationHelper::Automatic, obj->Duration);

                obj->Duration = DurationHelper::Forever;
                VERIFY_ARE_EQUAL(DurationHelper::Forever, obj->Duration);

                obj->Duration = fromDuration;
                VERIFY_ARE_EQUAL(fromDuration, obj->Duration);

                obj->SetValue(ObjectAnimationUsingKeyFrames::DurationProperty, DurationHelper::Automatic);
                VERIFY_ARE_EQUAL(DurationHelper::Automatic, safe_cast<Duration>(obj->GetValue(ObjectAnimationUsingKeyFrames::DurationProperty)));
                VERIFY_ARE_EQUAL(DurationHelper::Automatic, safe_cast<Duration>(obj->ReadLocalValue(ObjectAnimationUsingKeyFrames::DurationProperty)));
                VERIFY_ARE_EQUAL(DurationHelper::Automatic, safe_cast<Duration>(obj->GetAnimationBaseValue(ObjectAnimationUsingKeyFrames::DurationProperty)));

                obj->SetValue(ObjectAnimationUsingKeyFrames::DurationProperty, DurationHelper::Forever);
                VERIFY_ARE_EQUAL(DurationHelper::Forever, safe_cast<Duration>(obj->GetValue(ObjectAnimationUsingKeyFrames::DurationProperty)));
                VERIFY_ARE_EQUAL(DurationHelper::Forever, safe_cast<Duration>(obj->ReadLocalValue(ObjectAnimationUsingKeyFrames::DurationProperty)));
                VERIFY_ARE_EQUAL(DurationHelper::Forever, safe_cast<Duration>(obj->GetAnimationBaseValue(ObjectAnimationUsingKeyFrames::DurationProperty)));

                obj->SetValue(ObjectAnimationUsingKeyFrames::DurationProperty, fromDuration);
                VERIFY_ARE_EQUAL(fromDuration, safe_cast<Duration>(obj->GetValue(ObjectAnimationUsingKeyFrames::DurationProperty)));
                VERIFY_ARE_EQUAL(fromDuration, safe_cast<Duration>(obj->ReadLocalValue(ObjectAnimationUsingKeyFrames::DurationProperty)));
                VERIFY_ARE_EQUAL(fromDuration, safe_cast<Duration>(obj->GetAnimationBaseValue(ObjectAnimationUsingKeyFrames::DurationProperty)));
            });

            // RepeatBehavior

            RunOnUIThread([&]()
            {
                auto obj = ref new ObjectAnimationUsingKeyFrames();

                // Value 4.0 is picked on purpose.  For legacy reasons RepeatBehavior internally uses float for count
                // and this value will be widened to the same value as 4.0 double making direct comparison possible.
                auto fromCount = RepeatBehaviorHelper::FromCount(4.0);
                auto fromDuration = RepeatBehaviorHelper::FromDuration(TimeSpan_FromSeconds(30.0));

                obj->RepeatBehavior = fromCount;
                VERIFY_ARE_EQUAL(fromCount, obj->RepeatBehavior);

                obj->RepeatBehavior = RepeatBehaviorHelper::Forever;
                VERIFY_ARE_EQUAL(RepeatBehaviorHelper::Forever, obj->RepeatBehavior);

                obj->RepeatBehavior = fromDuration;
                VERIFY_ARE_EQUAL(fromDuration, obj->RepeatBehavior);

                obj->SetValue(ObjectAnimationUsingKeyFrames::RepeatBehaviorProperty, fromCount);
                VERIFY_ARE_EQUAL(fromCount, safe_cast<RepeatBehavior>(obj->GetValue(ObjectAnimationUsingKeyFrames::RepeatBehaviorProperty)));
                VERIFY_ARE_EQUAL(fromCount, safe_cast<RepeatBehavior>(obj->ReadLocalValue(ObjectAnimationUsingKeyFrames::RepeatBehaviorProperty)));
                VERIFY_ARE_EQUAL(fromCount, safe_cast<RepeatBehavior>(obj->GetAnimationBaseValue(ObjectAnimationUsingKeyFrames::RepeatBehaviorProperty)));

                obj->SetValue(ObjectAnimationUsingKeyFrames::RepeatBehaviorProperty, RepeatBehaviorHelper::Forever);
                VERIFY_ARE_EQUAL(RepeatBehaviorHelper::Forever, safe_cast<RepeatBehavior>(obj->GetValue(ObjectAnimationUsingKeyFrames::RepeatBehaviorProperty)));
                VERIFY_ARE_EQUAL(RepeatBehaviorHelper::Forever, safe_cast<RepeatBehavior>(obj->ReadLocalValue(ObjectAnimationUsingKeyFrames::RepeatBehaviorProperty)));
                VERIFY_ARE_EQUAL(RepeatBehaviorHelper::Forever, safe_cast<RepeatBehavior>(obj->GetAnimationBaseValue(ObjectAnimationUsingKeyFrames::RepeatBehaviorProperty)));

                obj->SetValue(ObjectAnimationUsingKeyFrames::RepeatBehaviorProperty, fromDuration);
                VERIFY_ARE_EQUAL(fromDuration, safe_cast<RepeatBehavior>(obj->GetValue(ObjectAnimationUsingKeyFrames::RepeatBehaviorProperty)));
                VERIFY_ARE_EQUAL(fromDuration, safe_cast<RepeatBehavior>(obj->ReadLocalValue(ObjectAnimationUsingKeyFrames::RepeatBehaviorProperty)));
                VERIFY_ARE_EQUAL(fromDuration, safe_cast<RepeatBehavior>(obj->GetAnimationBaseValue(ObjectAnimationUsingKeyFrames::RepeatBehaviorProperty)));
            });

            // KeyTime

            RunOnUIThread([&]()
            {
                auto obj = ref new DiscreteObjectKeyFrame();
                auto fromDuration = KeyTimeHelper::FromTimeSpan(TimeSpan_FromSeconds(30.0));

                obj->KeyTime = fromDuration;
                VERIFY_ARE_EQUAL(fromDuration, obj->KeyTime);

                auto fromDuration2 = KeyTimeHelper::FromTimeSpan(TimeSpan_FromSeconds(40.0));
                obj->SetValue(ObjectKeyFrame::KeyTimeProperty, fromDuration2);
                VERIFY_ARE_EQUAL(fromDuration2, safe_cast<KeyTime>(obj->GetValue(ObjectKeyFrame::KeyTimeProperty)));
                VERIFY_ARE_EQUAL(fromDuration2, safe_cast<KeyTime>(obj->ReadLocalValue(ObjectKeyFrame::KeyTimeProperty)));
                VERIFY_ARE_EQUAL(fromDuration2, safe_cast<KeyTime>(obj->GetAnimationBaseValue(ObjectKeyFrame::KeyTimeProperty)));
            });
        }

        void FlyweightTests::ValidateInstantiateInResources()
        {
            TestCleanupWrapper cleanup;

            // Duration

            RunOnUIThread([&]()
            {
                auto fromDuration0 = DurationHelper::FromTimeSpan(TimeSpan());
                auto fromDuration1 = DurationHelper::FromTimeSpan(TimeSpan_FromSeconds(10.0));

                auto root = safe_cast<Grid^>(Markup::XamlReader::Load(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid.Resources>"
                    L"    <Duration x:Key='dur0'>0</Duration>"
                    L"    <Duration x:Key='dur1'>00:00:10</Duration>"
                    L"    <Duration x:Key='dur2'>Automatic</Duration>"
                    L"    <Duration x:Key='dur3'>Forever</Duration>"
                    L"    <Duration x:Key='dur4'></Duration>"
                    L"    <ObjectAnimationUsingKeyFrames x:Key='obj0' Duration='0'/>"
                    L"    <ObjectAnimationUsingKeyFrames x:Key='obj1' Duration='00:00:10'/>"
                    L"    <ObjectAnimationUsingKeyFrames x:Key='obj2' Duration='Automatic'/>"
                    L"    <ObjectAnimationUsingKeyFrames x:Key='obj3' Duration='Forever'/>"
                    L"    <ObjectAnimationUsingKeyFrames x:Key='obj4' Duration=''/>"
                    L"    <ObjectAnimationUsingKeyFrames x:Key='obj5' BeginTime='{StaticResource dur1}'/>"
                    L"  </Grid.Resources>"
                    L"</Grid>"));

                // as stand-alone object

                auto dur0 = safe_cast<double>(root->Resources->Lookup(L"dur0"));
                VERIFY_IS_TRUE(dur0 == 0.0);

                auto dur1 = safe_cast<double>(root->Resources->Lookup(L"dur1"));
                VERIFY_IS_TRUE(dur1 == 10.0);

                auto dur2 = safe_cast<Platform::String^>(root->Resources->Lookup(L"dur2"));
                VERIFY_IS_TRUE(dur2 == "Automatic");

                auto dur3 = safe_cast<Platform::String^>(root->Resources->Lookup(L"dur3"));
                VERIFY_IS_TRUE(dur3 == "Forever");

                auto dur4 = safe_cast<Platform::String^>(root->Resources->Lookup(L"dur4"));
                VERIFY_IS_TRUE(dur4 == "Automatic");

                // as property

                auto obj0 = safe_cast<ObjectAnimationUsingKeyFrames^>(root->Resources->Lookup(L"obj0"));
                VERIFY_IS_TRUE(DurationHelper::GetHasTimeSpan(obj0->Duration));
                VERIFY_ARE_EQUAL(fromDuration0, obj0->Duration);

                auto obj1 = safe_cast<ObjectAnimationUsingKeyFrames^>(root->Resources->Lookup(L"obj1"));
                VERIFY_IS_TRUE(DurationHelper::GetHasTimeSpan(obj1->Duration));
                VERIFY_ARE_EQUAL(fromDuration1, obj1->Duration);

                auto obj2 = safe_cast<ObjectAnimationUsingKeyFrames^>(root->Resources->Lookup(L"obj2"));
                VERIFY_ARE_EQUAL(DurationHelper::Automatic, obj2->Duration);

                auto obj3 = safe_cast<ObjectAnimationUsingKeyFrames^>(root->Resources->Lookup(L"obj3"));
                VERIFY_ARE_EQUAL(DurationHelper::Forever, obj3->Duration);

                auto obj4 = safe_cast<ObjectAnimationUsingKeyFrames^>(root->Resources->Lookup(L"obj4"));
                VERIFY_ARE_EQUAL(DurationHelper::Automatic, obj4->Duration);

                // Allow assigning TimeSpan
                auto obj5 = safe_cast<ObjectAnimationUsingKeyFrames^>(root->Resources->Lookup(L"obj5"));
                VERIFY_ARE_EQUAL(TimeSpan_FromSeconds(10.0).Duration, obj5->BeginTime->Value.Duration);
            });

            // RepeatBehavior

            RunOnUIThread([&]()
            {
                auto fromDuration0 = RepeatBehaviorHelper::FromDuration(TimeSpan());
                auto fromDuration1 = RepeatBehaviorHelper::FromDuration(TimeSpan_FromSeconds(10.0));
                auto fromCount1 = RepeatBehaviorHelper::FromCount(1.0);
                auto fromCount2 = RepeatBehaviorHelper::FromCount(2.0);

                auto root = safe_cast<Grid^>(Markup::XamlReader::Load(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid.Resources>"
                    L"    <RepeatBehavior x:Key='rb0'>0</RepeatBehavior>"
                    L"    <RepeatBehavior x:Key='rb1'>00:00:10</RepeatBehavior>"
                    L"    <RepeatBehavior x:Key='rb2'>2x</RepeatBehavior>"
                    L"    <RepeatBehavior x:Key='rb3'>Forever</RepeatBehavior>"
                    L"    <RepeatBehavior x:Key='rb4'></RepeatBehavior>"
                    L"    <ObjectAnimationUsingKeyFrames x:Key='obj0' RepeatBehavior='0'/>"
                    L"    <ObjectAnimationUsingKeyFrames x:Key='obj1' RepeatBehavior='00:00:10'/>"
                    L"    <ObjectAnimationUsingKeyFrames x:Key='obj2' RepeatBehavior='2x'/>"
                    L"    <ObjectAnimationUsingKeyFrames x:Key='obj3' RepeatBehavior='Forever'/>"
                    L"    <ObjectAnimationUsingKeyFrames x:Key='obj4' RepeatBehavior=''/>"
                    L"    <ObjectAnimationUsingKeyFrames x:Key='obj5' BeginTime='{StaticResource rb1}'/>"
                    L"  </Grid.Resources>"
                    L"</Grid>"));

                // as stand-alone object

                auto rb0 = safe_cast<double>(root->Resources->Lookup(L"rb0"));
                VERIFY_IS_TRUE(rb0 == 0.0);

                auto rb1 = safe_cast<double>(root->Resources->Lookup(L"rb1"));
                VERIFY_IS_TRUE(rb1 == 10.0);

                auto rb2 = safe_cast<Platform::String^>(root->Resources->Lookup(L"rb2"));
                VERIFY_IS_TRUE(rb2 == "2.000000x");

                auto rb3 = safe_cast<Platform::String^>(root->Resources->Lookup(L"rb3"));
                VERIFY_IS_TRUE(rb3 == "Forever");

                auto rb4 = safe_cast<Platform::String^>(root->Resources->Lookup(L"rb4"));
                VERIFY_IS_TRUE(rb4 == "1.000000x");

                // as property

                auto obj0 = safe_cast<ObjectAnimationUsingKeyFrames^>(root->Resources->Lookup(L"obj0"));
                VERIFY_IS_TRUE(RepeatBehaviorHelper::GetHasDuration(obj0->RepeatBehavior));
                VERIFY_ARE_EQUAL(fromDuration0, obj0->RepeatBehavior);

                auto obj1 = safe_cast<ObjectAnimationUsingKeyFrames^>(root->Resources->Lookup(L"obj1"));
                VERIFY_IS_TRUE(RepeatBehaviorHelper::GetHasDuration(obj1->RepeatBehavior));
                VERIFY_ARE_EQUAL(fromDuration1, obj1->RepeatBehavior);

                auto obj2 = safe_cast<ObjectAnimationUsingKeyFrames^>(root->Resources->Lookup(L"obj2"));
                VERIFY_IS_TRUE(RepeatBehaviorHelper::GetHasCount(obj2->RepeatBehavior));
                VERIFY_ARE_EQUAL(fromCount2, obj2->RepeatBehavior);

                auto obj3 = safe_cast<ObjectAnimationUsingKeyFrames^>(root->Resources->Lookup(L"obj3"));
                VERIFY_ARE_EQUAL(RepeatBehaviorHelper::Forever, obj3->RepeatBehavior);

                auto obj4 = safe_cast<ObjectAnimationUsingKeyFrames^>(root->Resources->Lookup(L"obj4"));
                VERIFY_IS_TRUE(RepeatBehaviorHelper::GetHasCount(obj4->RepeatBehavior));
                VERIFY_ARE_EQUAL(fromCount1, obj4->RepeatBehavior);

                // Allow assigning TimeSpan
                auto obj5 = safe_cast<ObjectAnimationUsingKeyFrames^>(root->Resources->Lookup(L"obj5"));
                VERIFY_ARE_EQUAL(TimeSpan_FromSeconds(10.0).Duration, obj5->BeginTime->Value.Duration);
            });

            // KeyTime

            RunOnUIThread([&]()
            {
                auto fromTimeSpan0 = KeyTimeHelper::FromTimeSpan(TimeSpan());
                auto fromTimeSpan1 = KeyTimeHelper::FromTimeSpan(TimeSpan_FromSeconds(10.0));

                auto root = safe_cast<Grid^>(Markup::XamlReader::Load(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid.Resources>"
                    L"    <KeyTime x:Key='kt0'>0</KeyTime>"
                    L"    <KeyTime x:Key='kt1'>00:00:10</KeyTime>"
                    L"    <KeyTime x:Key='kt2'></KeyTime>"
                    L"    <DiscreteObjectKeyFrame x:Key='obj0' KeyTime='0'/>"
                    L"    <DiscreteObjectKeyFrame x:Key='obj1' KeyTime='00:00:10'/>"
                    L"    <DiscreteObjectKeyFrame x:Key='obj2' KeyTime=''/>"
                    L"    <ObjectAnimationUsingKeyFrames x:Key='obj3' BeginTime='{StaticResource kt1}'/>"
                    L"  </Grid.Resources>"
                    L"</Grid>"));

                // as stand-alone object

                auto kt0 = safe_cast<double>(root->Resources->Lookup(L"kt0"));
                VERIFY_IS_TRUE(kt0 == 0.0);

                auto kt1 = safe_cast<double>(root->Resources->Lookup(L"kt1"));
                VERIFY_IS_TRUE(kt1 == 10.0);

                auto kt2 = safe_cast<double>(root->Resources->Lookup(L"kt2"));
                VERIFY_IS_TRUE(kt2 == 0.0);

                // as property

                auto obj0 = safe_cast<DiscreteObjectKeyFrame^>(root->Resources->Lookup(L"obj0"));
                VERIFY_ARE_EQUAL(fromTimeSpan0, obj0->KeyTime);

                auto obj1 = safe_cast<DiscreteObjectKeyFrame^>(root->Resources->Lookup(L"obj1"));
                VERIFY_ARE_EQUAL(fromTimeSpan1, obj1->KeyTime);

                auto obj2 = safe_cast<DiscreteObjectKeyFrame^>(root->Resources->Lookup(L"obj2"));
                VERIFY_ARE_EQUAL(fromTimeSpan0, obj2->KeyTime);

                // Allow assigning TimeSpan
                auto obj3 = safe_cast<ObjectAnimationUsingKeyFrames^>(root->Resources->Lookup(L"obj3"));
                VERIFY_ARE_EQUAL(TimeSpan_FromSeconds(10.0).Duration, obj3->BeginTime->Value.Duration);
            });
        }

        void FlyweightTests::ValidatePropertyChangedCallback()
        {
            TestCleanupWrapper cleanup;

            Grid^ root = nullptr;
            ObjectAnimationUsingKeyFrames^ obj0 = nullptr;
            DiscreteObjectKeyFrame^ obj1 = nullptr;
            int propertyChangedCounter = 0;

            auto handler = ref new DependencyPropertyChangedCallback([&](DependencyObject^ sender, DependencyProperty^ prop)
            {
                ++propertyChangedCounter;
            });

            RunOnUIThread([&]()
            {
                root = safe_cast<Grid^>(Markup::XamlReader::Load(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid.Resources>"
                    L"    <ObjectAnimationUsingKeyFrames x:Key='obj0'/>"
                    L"    <DiscreteObjectKeyFrame x:Key='obj1'/>"
                    L"  </Grid.Resources>"
                    L"</Grid>"));

                obj0 = safe_cast<ObjectAnimationUsingKeyFrames^>(root->Resources->Lookup(L"obj0"));
                obj1 = safe_cast<DiscreteObjectKeyFrame^>(root->Resources->Lookup(L"obj1"));
            });

            // Duration

            RunOnUIThread([&]()
            {
                propertyChangedCounter = 0;

                auto fromDuration = DurationHelper::FromTimeSpan(TimeSpan());

                auto token = obj0->RegisterPropertyChangedCallback(ObjectAnimationUsingKeyFrames::DurationProperty, handler);

                obj0->Duration = DurationHelper::Automatic;
                VERIFY_ARE_EQUAL(1, propertyChangedCounter);

                obj0->Duration = DurationHelper::Forever;
                VERIFY_ARE_EQUAL(2, propertyChangedCounter);

                obj0->Duration = fromDuration;
                VERIFY_ARE_EQUAL(3, propertyChangedCounter);

                // FUT[BK]: Even though the value has not changed (obj identity did), PC gets called...
                obj0->Duration = fromDuration;
                VERIFY_ARE_EQUAL(4, propertyChangedCounter);

                obj0->UnregisterPropertyChangedCallback(ObjectAnimationUsingKeyFrames::DurationProperty, token);
            });

            // RepeatBehavior

            RunOnUIThread([&]()
            {
                propertyChangedCounter = 0;

                auto fromDuration = RepeatBehaviorHelper::FromDuration(TimeSpan());
                auto fromCount = RepeatBehaviorHelper::FromCount(4.0);

                auto token = obj0->RegisterPropertyChangedCallback(ObjectAnimationUsingKeyFrames::RepeatBehaviorProperty, handler);

                obj0->RepeatBehavior = fromCount;
                VERIFY_ARE_EQUAL(1, propertyChangedCounter);

                obj0->RepeatBehavior = RepeatBehaviorHelper::Forever;
                VERIFY_ARE_EQUAL(2, propertyChangedCounter);

                obj0->RepeatBehavior = fromDuration;
                VERIFY_ARE_EQUAL(3, propertyChangedCounter);

                // FUT[BK]: Even though the value has not changed (obj identity did), PC gets called...
                obj0->RepeatBehavior = fromDuration;
                VERIFY_ARE_EQUAL(4, propertyChangedCounter);

                obj0->UnregisterPropertyChangedCallback(ObjectAnimationUsingKeyFrames::RepeatBehaviorProperty, token);
            });

            // KeyTime

            RunOnUIThread([&]()
            {
                propertyChangedCounter = 0;

                auto fromTimeSpan0 = KeyTimeHelper::FromTimeSpan(TimeSpan());
                auto fromTimeSpan1 = KeyTimeHelper::FromTimeSpan(TimeSpan_FromSeconds(10.0));

                auto token = obj1->RegisterPropertyChangedCallback(DiscreteObjectKeyFrame::KeyTimeProperty, handler);

                obj1->KeyTime = fromTimeSpan0;
                VERIFY_ARE_EQUAL(1, propertyChangedCounter);

                obj1->KeyTime = fromTimeSpan1;
                VERIFY_ARE_EQUAL(2, propertyChangedCounter);

                // FUT[BK]: Even though the value has not changed (obj identity did), PC gets called...
                obj1->KeyTime = fromTimeSpan1;
                VERIFY_ARE_EQUAL(3, propertyChangedCounter);

                obj1->UnregisterPropertyChangedCallback(DiscreteObjectKeyFrame::KeyTimeProperty, token);
            });
        }

        void FlyweightTests::ValidateBindingToObjectPropertyReadback()
        {
            TestCleanupWrapper cleanup;

            ContentControl^ cc0 = nullptr;
            ContentControl^ cc1 = nullptr;
            ContentControl^ cc2 = nullptr;

            ObjectAnimationUsingKeyFrames^ obj0 = nullptr;
            DiscreteObjectKeyFrame^ obj1 = nullptr;

            RunOnUIThread([&]()
            {
                auto root = safe_cast<Grid^>(Markup::XamlReader::Load(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid.Resources>"
                    L"    <ObjectAnimationUsingKeyFrames x:Key='obj0'/>"
                    L"    <DiscreteObjectKeyFrame x:Key='obj1'/>"
                    L"  </Grid.Resources>"
                    L"  <ContentControl x:Name='cc0' Content='{Binding Source={StaticResource obj0},Path=Duration}'/>"
                    L"  <ContentControl x:Name='cc1' Content='{Binding Source={StaticResource obj0},Path=RepeatBehavior}'/>"
                    L"  <ContentControl x:Name='cc2' Content='{Binding Source={StaticResource obj1},Path=KeyTime}'/>"
                    L"</Grid>"));

                cc0 = safe_cast<ContentControl^>(root->FindName(L"cc0"));
                cc1 = safe_cast<ContentControl^>(root->FindName(L"cc1"));
                cc2 = safe_cast<ContentControl^>(root->FindName(L"cc2"));

                obj0 = safe_cast<ObjectAnimationUsingKeyFrames^>(root->Resources->Lookup(L"obj0"));
                obj1 = safe_cast<DiscreteObjectKeyFrame^>(root->Resources->Lookup(L"obj1"));

                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                Platform::Object^ oo = cc2->Content;

                // Duration

                obj0->Duration = DurationHelper::Automatic;
                VERIFY_ARE_EQUAL(DurationHelper::Automatic, safe_cast<Duration>(cc0->Content));

                obj0->Duration = DurationHelper::Forever;
                VERIFY_ARE_EQUAL(DurationHelper::Forever, safe_cast<Duration>(cc0->Content));

                auto fromDuration = DurationHelper::FromTimeSpan(TimeSpan_FromSeconds(10.0));
                obj0->Duration = fromDuration;
                VERIFY_ARE_EQUAL(fromDuration, safe_cast<Duration>(cc0->Content));

                // RepeatBehavior

                obj0->RepeatBehavior = RepeatBehaviorHelper::FromCount(2.0);
                VERIFY_ARE_EQUAL(RepeatBehaviorHelper::FromCount(2.0), safe_cast<RepeatBehavior>(cc1->Content));

                obj0->RepeatBehavior = RepeatBehaviorHelper::Forever;
                VERIFY_ARE_EQUAL(RepeatBehaviorHelper::Forever, safe_cast<RepeatBehavior>(cc1->Content));

                auto fromDuration2 = RepeatBehaviorHelper::FromDuration(TimeSpan_FromSeconds(10.0));
                obj0->RepeatBehavior = fromDuration2;
                VERIFY_ARE_EQUAL(fromDuration2, safe_cast<RepeatBehavior>(cc1->Content));

                // KeyTime

                auto fromTimeSpan = KeyTimeHelper::FromTimeSpan(TimeSpan_FromSeconds(10.0));
                obj1->KeyTime = fromTimeSpan;
                VERIFY_ARE_EQUAL(fromTimeSpan, safe_cast<KeyTime>(cc2->Content));
            });
        }

        void FlyweightTests::ValidateBindingToResourceReadback()
        {
            TestCleanupWrapper cleanup;

            std::array<ContentControl^, 5> d;
            std::array<ContentControl^, 5> r;
            std::array<ContentControl^, 3> k;

            RunOnUIThread([&]()
            {
                auto root = safe_cast<Grid^>(Markup::XamlReader::Load(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid.Resources>"
                    L"    <Duration x:Key='dur0'>0</Duration>"
                    L"    <Duration x:Key='dur1'>00:00:10</Duration>"
                    L"    <Duration x:Key='dur2'>Automatic</Duration>"
                    L"    <Duration x:Key='dur3'>Forever</Duration>"
                    L"    <Duration x:Key='dur4'></Duration>"
                    L"    <RepeatBehavior x:Key='rb0'>0</RepeatBehavior>"
                    L"    <RepeatBehavior x:Key='rb1'>00:00:10</RepeatBehavior>"
                    L"    <RepeatBehavior x:Key='rb2'>2x</RepeatBehavior>"
                    L"    <RepeatBehavior x:Key='rb3'>Forever</RepeatBehavior>"
                    L"    <RepeatBehavior x:Key='rb4'></RepeatBehavior>"
                    L"    <KeyTime x:Key='kt0'>0</KeyTime>"
                    L"    <KeyTime x:Key='kt1'>00:00:10</KeyTime>"
                    L"    <KeyTime x:Key='kt2'></KeyTime>"
                    L"  </Grid.Resources>"
                    L"  <ContentControl x:Name='d0' Content='{Binding Source={StaticResource dur0}}'/>"
                    L"  <ContentControl x:Name='d1' Content='{Binding Source={StaticResource dur1}}'/>"
                    L"  <ContentControl x:Name='d2' Content='{Binding Source={StaticResource dur2}}'/>"
                    L"  <ContentControl x:Name='d3' Content='{Binding Source={StaticResource dur3}}'/>"
                    L"  <ContentControl x:Name='d4' Content='{Binding Source={StaticResource dur4}}'/>"
                    L"  <ContentControl x:Name='r0' Content='{Binding Source={StaticResource rb0}}'/>"
                    L"  <ContentControl x:Name='r1' Content='{Binding Source={StaticResource rb1}}'/>"
                    L"  <ContentControl x:Name='r2' Content='{Binding Source={StaticResource rb2}}'/>"
                    L"  <ContentControl x:Name='r3' Content='{Binding Source={StaticResource rb3}}'/>"
                    L"  <ContentControl x:Name='r4' Content='{Binding Source={StaticResource rb4}}'/>"
                    L"  <ContentControl x:Name='k0' Content='{Binding Source={StaticResource kt0}}'/>"
                    L"  <ContentControl x:Name='k1' Content='{Binding Source={StaticResource kt1}}'/>"
                    L"  <ContentControl x:Name='k2' Content='{Binding Source={StaticResource kt2}}'/>"
                    L"</Grid>"));

                int i = 0;

                for (auto iter : d)
                {
                    d[i] = safe_cast<ContentControl^>(root->FindName(ref new Platform::String(String().Format(L"d%d", i))));
                    ++i;
                }

                i = 0;

                for (auto iter : r)
                {
                    r[i] = safe_cast<ContentControl^>(root->FindName(ref new Platform::String(String().Format(L"r%d", i))));
                    ++i;
                }

                i = 0;

                for (auto iter : k)
                {
                    k[i] = safe_cast<ContentControl^>(root->FindName(ref new Platform::String(String().Format(L"k%d", i))));
                    ++i;
                }

                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Duration

                auto dur0 = safe_cast<double>(d[0]->Content);
                VERIFY_IS_TRUE(dur0 == 0.0);

                auto dur1 = safe_cast<double>(d[1]->Content);
                VERIFY_IS_TRUE(dur1 == 10.0);

                auto dur2 = safe_cast<Platform::String^>(d[2]->Content);
                VERIFY_IS_TRUE(dur2 == "Automatic");

                auto dur3 = safe_cast<Platform::String^>(d[3]->Content);
                VERIFY_IS_TRUE(dur3 == "Forever");

                auto dur4 = safe_cast<Platform::String^>(d[4]->Content);
                VERIFY_IS_TRUE(dur4 == "Automatic");

                // RepeatBehavior

                auto rb0 = safe_cast<double>(r[0]->Content);
                VERIFY_IS_TRUE(rb0 == 0.0);

                auto rb1 = safe_cast<double>(r[1]->Content);
                VERIFY_IS_TRUE(rb1 == 10.0);

                auto rb2 = safe_cast<Platform::String^>(r[2]->Content);
                VERIFY_IS_TRUE(rb2 == "2.000000x");

                auto rb3 = safe_cast<Platform::String^>(r[3]->Content);
                VERIFY_IS_TRUE(rb3 == "Forever");

                auto rb4 = safe_cast<Platform::String^>(r[4]->Content);
                VERIFY_IS_TRUE(rb4 == "1.000000x");

                // KeyTime

                auto kt0 = safe_cast<double>(k[0]->Content);
                VERIFY_IS_TRUE(kt0 == 0.0);

                auto kt1 = safe_cast<double>(k[1]->Content);
                VERIFY_IS_TRUE(kt1 == 10.0);

                auto kt2 = safe_cast<double>(k[2]->Content);
                VERIFY_IS_TRUE(kt2 == 0.0);

                // Clear references to avoid fake leaks

                std::for_each(d.begin(), d.end(), [](ContentControl^& v) { v = nullptr; });
                std::for_each(r.begin(), r.end(), [](ContentControl^& v) { v = nullptr; });
                std::for_each(k.begin(), k.end(), [](ContentControl^& v) { v = nullptr; });
            });
        }

        void FlyweightTests::ValidateResourceInObjectThrows()
        {
            TestCleanupWrapper cleanup;
            DisableErrorReportingScopeGuard disableErrors;

            RunOnUIThread([&]()
            {
                VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid.Resources>"
                    L"    <Duration x:Key='obj'>0</Duration>"
                    L"  </Grid.Resources>"
                    L"  <ContentControl Content='{StaticResource obj}'/>"
                    L"</Grid>"),
                    Platform::COMException^,
                    L"Duration as Content shoudl throw");

                VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid.Resources>"
                    L"    <RepeatBehavior x:Key='obj'>0</RepeatBehavior>"
                    L"  </Grid.Resources>"
                    L"  <ContentControl Content='{StaticResource obj}'/>"
                    L"</Grid>"),
                    Platform::COMException^,
                    L"RepeatBehavior as Content shoudl throw");

                VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid.Resources>"
                    L"    <KeyTime x:Key='obj'>0</KeyTime>"
                    L"  </Grid.Resources>"
                    L"  <ContentControl Content='{StaticResource obj}'/>"
                    L"</Grid>"),
                    Platform::COMException^,
                    L"KeyTime as Content shoudl throw");
            });
        }

        DependencyObject^ AnimateToNull(Platform::String^ objectName, Platform::String^ propertyName)
        {
            TestCleanupWrapper cleanup;

            DependencyObject^ obj = nullptr;
            Grid^ root = nullptr;
            Storyboard^ storyboard = nullptr;

            RunOnUIThread([&]()
            {
                root = safe_cast<Grid^>(Markup::XamlReader::Load(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid.Resources>"
                    L"    <ObjectAnimationUsingKeyFrames x:Key='obj0'/>"
                    L"    <DiscreteObjectKeyFrame x:Key='obj1'/>"
                    L"  </Grid.Resources>"
                    L"</Grid>"));

                obj = safe_cast<DependencyObject^>(root->Resources->Lookup(objectName));
                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->SynchronouslyTickUIThread(2);

            RunOnUIThread([&]()
            {
                auto keyFrame = ref new DiscreteObjectKeyFrame();
                keyFrame->Value = nullptr;
                keyFrame->KeyTime = KeyTimeHelper::FromTimeSpan(TimeSpan());

                auto objectAnim = ref new ObjectAnimationUsingKeyFrames();
                objectAnim->AllowDependentAnimations = true;

                TimeSpan ts; ts.Duration = 1;
                objectAnim->Duration = DurationHelper::FromTimeSpan(ts);
                objectAnim->KeyFrames->Append(keyFrame);
                objectAnim->FillBehavior = FillBehavior::HoldEnd;

                Storyboard::SetTarget(objectAnim, obj);
                Storyboard::SetTargetProperty(objectAnim, propertyName);

                storyboard = ref new Storyboard();
                storyboard->Children->Append(objectAnim);
                storyboard->Begin();
            });

            TestServices::WindowHelper->SynchronouslyTickUIThread(2);

            root = nullptr;
            storyboard = nullptr;

            return obj;
        }

        void FlyweightTests::ValidateAnimateToNull()
        {
            {
                ObjectAnimationUsingKeyFrames^ obj = safe_cast<ObjectAnimationUsingKeyFrames^>(AnimateToNull("obj0", "Duration"));

                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(DurationHelper::Automatic, obj->Duration);
                    obj = nullptr;
                });
            }

            {
                ObjectAnimationUsingKeyFrames^ obj = safe_cast<ObjectAnimationUsingKeyFrames^>(AnimateToNull("obj0", "RepeatBehavior"));

                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(RepeatBehaviorHelper::FromDuration(TimeSpan()), obj->RepeatBehavior);
                    obj = nullptr;
                });
            }

            {
                DiscreteObjectKeyFrame^ obj = safe_cast<DiscreteObjectKeyFrame^>(AnimateToNull("obj1", "KeyTime"));

                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(KeyTimeHelper::FromTimeSpan(TimeSpan()), obj->KeyTime);
                    obj = nullptr;
                });
            }

            LOG_OUTPUT(L"[We shouldn't crash]");
        }
    }
} } } }
