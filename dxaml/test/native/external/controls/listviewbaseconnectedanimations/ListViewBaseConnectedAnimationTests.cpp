// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ListViewBaseConnectedAnimationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <ControlHelper.h>
#include <RuntimeEnabledFeaturesEnum.h>
#include <WUCRenderingScopeGuard.h>
#include <CustomPropertySupport.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Tests::Controls::ListViewBaseConnectedAnimations;

ref class TestDataObject sealed : public Microsoft::UI::Xaml::Tests::Common::CustomPropertyProviderBase
{
public:
    TestDataObject(Platform::String^ dataString)
    {
        m_dataString = dataString;
    }

    Platform::String^ GetStringRepresentation() override
    {
        return m_dataString;
    }

protected:
    void AddCustomProperties() override
    {
        AddCustomProperty(L"DataString", Platform::String::typeid,
            MAKEPROPGET(TestDataObject^, m_dataString),
            MAKEPROPSET(TestDataObject^, m_dataString, Platform::String^)
        );
    }

public:
    property Platform::String^ m_dataString;
};

//
// Class & Test Setup
//
bool ConnectedAnimationTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool ConnectedAnimationTests::ClassCleanup()
{
    return true;
}

bool ConnectedAnimationTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool ConnectedAnimationTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

//
// Test Cases
//
void ConnectedAnimationTests::AnimationTest()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Platform::Collections::Vector<TestDataObject^>^ itemList = ref new Platform::Collections::Vector<TestDataObject^>();

    itemList->Append(ref new TestDataObject(L"Apple"));
    itemList->Append(ref new TestDataObject(L"Orange"));
    itemList->Append(ref new TestDataObject(L"Pear"));
    itemList->Append(ref new TestDataObject(L"Grape"));
    itemList->Append(ref new TestDataObject(L"Bannana"));
    itemList->Append(ref new TestDataObject(L"Plum"));
    itemList->Append(ref new TestDataObject(L"Tomato"));
    itemList->Append(ref new TestDataObject(L"Avacado"));
    itemList->Append(ref new TestDataObject(L"Passion Fruit"));
    itemList->Append(ref new TestDataObject(L"Tangerine"));
    itemList->Append(ref new TestDataObject(L"Blackberry"));
    itemList->Append(ref new TestDataObject(L"Blueberry"));
    itemList->Append(ref new TestDataObject(L"Huckleberry"));
    itemList->Append(ref new TestDataObject(L"Breadfruit"));
    itemList->Append(ref new TestDataObject(L"Apricot"));
    itemList->Append(ref new TestDataObject(L"Acai"));
    itemList->Append(ref new TestDataObject(L"Cherry"));
    itemList->Append(ref new TestDataObject(L"Guava"));
    itemList->Append(ref new TestDataObject(L"Pinapple"));
    itemList->Append(ref new TestDataObject(L"Lime"));
    itemList->Append(ref new TestDataObject(L"Papaya"));

    ListView^ listview;
    Grid^ rootPanel;
    Grid^ primaryPanel;
    Grid^ secondaryPanel;
    Grid^ target;
    TextBlock^ targetText;
    xaml_animation::ConnectedAnimationService^ service;

    auto caCompletedRegistration = CreateSafeEventRegistration(xaml_animation::ConnectedAnimation, Completed);
    auto caCompletedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        rootPanel = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>\n"
            L"   <Grid x:Name='PrimaryPanel'>\n"
            L"       <ListView x:Name='list'>\n"
            L"           <ListView.ItemTemplate>\n"
            L"              <DataTemplate>\n"
            L"                  <Grid x:Name='SourceGrid' Margin='5,5,5,5' Width='200' Background='Gray'>\n"
            L"                      <TextBlock x:Name='SourceText' FontSize='25' Text='{Binding DataString}' Margin='10,10,10,10' HorizontalAlignment='Center'/>\n"
            L"                  </Grid>\n"
            L"              </DataTemplate>\n"
            L"           </ListView.ItemTemplate>\n"
            L"       </ListView>\n"
            L"    </Grid>\n"
            L"</Grid>"));
        VERIFY_IS_NOT_NULL(rootPanel);
        TestServices::WindowHelper->WindowContent = rootPanel;

        Grid^ rootPanel2 = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>\n"
            L"   <Grid x:Name='SecondaryPanel'>\n"
            L"       <Grid x:Name='Target' HorizontalAlignment='Stretch' VerticalAlignment='Stretch' Margin='10,10,10,10' Background='Blue'>\n"
            L"           <TextBlock x:Name='TargetText' FontSize='60' Text='Fruit' HorizontalAlignment='Center' VerticalAlignment='Center'/>\n"
            L"       </Grid>\n"
            L"    </Grid>\n"
            L"</Grid>"));

        listview = safe_cast<ListView^>(rootPanel->FindName(L"list"));
        VERIFY_IS_NOT_NULL(listview);
        primaryPanel = safe_cast<Grid^>(rootPanel->FindName(L"PrimaryPanel"));
        VERIFY_IS_NOT_NULL(primaryPanel);
        secondaryPanel = safe_cast<Grid^>(rootPanel2->FindName(L"SecondaryPanel"));
        VERIFY_IS_NOT_NULL(secondaryPanel);
        target = safe_cast<Grid^>(rootPanel2->FindName(L"Target"));
        VERIFY_IS_NOT_NULL(target);
        targetText = safe_cast<TextBlock^>(rootPanel2->FindName(L"TargetText"));
        VERIFY_IS_NOT_NULL(targetText);

        listview->ItemsSource = itemList;

        service = xaml_animation::ConnectedAnimationService::GetForCurrentView();
        VERIFY_IS_NOT_NULL(service);
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Create List Item to static item animation");
    RunOnUIThread([&]()
    {
        ConnectedAnimation^ animation = listview->PrepareConnectedAnimation(L"Test", itemList->GetAt(4), L"SourceText");
        VERIFY_IS_NOT_NULL(animation);
        rootPanel->Children->Clear();
    });
    TestServices::WindowHelper->WaitForIdle();
    RunOnUIThread([&]()
    {
        ConnectedAnimation^ animation = service->GetAnimation(L"Test");
        VERIFY_IS_NOT_NULL(animation);
        rootPanel->Children->Append(secondaryPanel);
        animation->TryStart(targetText);

        caCompletedRegistration.Attach(animation, ref new wf::TypedEventHandler<xaml_animation::ConnectedAnimation^, Platform::Object^>([caCompletedEvent](xaml_animation::ConnectedAnimation^ sender, Platform::Object^ e)
        {
            LOG_OUTPUT(L"Completed Event Fired");
            caCompletedEvent->Set();
        }));
    });
    caCompletedEvent->WaitForDefault();
    caCompletedRegistration.Detach();
    TestServices::WindowHelper->WaitForIdle();

    // Now go the other way
    LOG_OUTPUT(L"Create static item to List Item animation");

    caCompletedEvent->Reset();
    RunOnUIThread([&]()
    {
        ConnectedAnimation^ animation = service->PrepareToAnimate(L"ReverseTest", targetText);
        VERIFY_IS_NOT_NULL(animation);
        rootPanel->Children->Clear();

    });
    TestServices::WindowHelper->WaitForIdle();
    RunOnUIThread([&]()
    {
        ConnectedAnimation^ animation = service->GetAnimation(L"ReverseTest");
        VERIFY_IS_NOT_NULL(animation);
        rootPanel->Children->Append(primaryPanel);
        listview->TryStartConnectedAnimationAsync(animation, itemList->GetAt(4), L"SourceText");
        caCompletedRegistration.Attach(animation, ref new wf::TypedEventHandler<xaml_animation::ConnectedAnimation^, Platform::Object^>([caCompletedEvent](xaml_animation::ConnectedAnimation^ sender, Platform::Object^ e)
        {
            LOG_OUTPUT(L"Completed Event Fired");
            caCompletedEvent->Set();
        }));
    });
    caCompletedEvent->WaitForDefault();
    caCompletedRegistration.Detach();
    TestServices::WindowHelper->WaitForIdle();
}
