// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <StringUtilities.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <ppltasks.h>
#include <RuntimeEnabledFeatureOverride.h>
#include "NamingIntegrationTests.h"
#include "NamingCustomControl.h"
#include "NamingUserControl.h"
#include "CustomTypes.XamlTypeInfo.g.h"


using namespace Platform;
using namespace Concurrency;
using namespace Microsoft::WRL;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Controls::Primitives;

using namespace test_infra;
using namespace std;
using namespace ::Tests::Native::External::Framework::Naming;


String^ connectedGridName = "ConnectedGrid";

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework { namespace Naming {

        bool NamingIntegrationTests::ClassSetup()
        {
            // It's very important to call EnsureInitialized on TestServices
            // from ClassSetup. This method will wait for the window to be
            // activated on launch, which avoids a race condition that will block
            // input from being routed to the app. It will also wait for the
            // debugger to attach when the waitForDebugger runtime parameter is
            // specified.
            CommonTestSetupHelper::CommonTestClassSetup();

            return true;
        }

        bool NamingIntegrationTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider());
            return true;
        }

        bool NamingIntegrationTests::TestCleanup()
        {
                test_infra::TestServices::WindowHelper->ShutdownXaml();
                return true;
        }

        void NamingIntegrationTests::LooseXamlTest()
        {
            TestCleanupWrapper cleanup;
            RunOnUIThread([&]()
            {

                String^ testXamlString = nullptr;

                testXamlString = ""
                    "<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                    "    <Rectangle Name='R1' />"
                    "</Grid>";

                auto grid = static_cast<Grid^>(XamlReader::Load(testXamlString));
                VERIFY_IS_NOT_NULL(grid);

                auto r2 = ref new Microsoft::UI::Xaml::Shapes::Rectangle();
                r2->Name = "R2";
                grid->Children->Append(r2);


                auto r1 = static_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(grid->FindName("R1"));
                VERIFY_IS_NOT_NULL(r1);

                auto r2b = static_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(grid->FindName("R2"));
                VERIFY_IS_NOT_NULL(r2b);
            });
        }


        void NamingIntegrationTests::DuplicateNameTest()
        {
            TestCleanupWrapper cleanup;
            Grid^ grid = nullptr;

            RunOnUIThread([&]()
            {

                String^ testXamlString = nullptr;

                testXamlString = ""
                    "<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                    "    <Rectangle Name='R1' />"
                    "</Grid>";

                grid = static_cast<Grid^>(XamlReader::Load(testXamlString));
                VERIFY_IS_NOT_NULL(grid);

                auto r1a = static_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(grid->FindName("R1"));
                VERIFY_IS_NOT_NULL(r1a);

                TestServices::WindowHelper->WindowContent = grid;

            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto r1b = static_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(grid->FindName("R1"));
                VERIFY_IS_NOT_NULL(r1b);

                auto r2 = ref new Microsoft::UI::Xaml::Shapes::Rectangle();
                r2->Name = "R1";
                grid->Children->Append(r2);

                r1b = static_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(grid->FindName("R1"));
                VERIFY_ARE_EQUAL(r1b, r2);

            });

        }


        void NamingIntegrationTests::DataTemplateTest()
        {
            TestCleanupWrapper cleanup;
            RunOnUIThread([&]()
            {

                String^ testXamlString = ""
                    "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    "  <ContentControl Name='contentControl'>"
                    "    <DataTemplate x:Name='dataTemplate'>"
                    "      <Grid>"
                    "        <Rectangle Name='R1' />"
                    "      </Grid>"
                    "    </DataTemplate>"
                    "  </ContentControl>"
                    "</StackPanel>";

                auto stackPanel = static_cast<StackPanel^>(XamlReader::Load(testXamlString));
                auto contentControl = stackPanel->FindName("contentControl");
                VERIFY_IS_NOT_NULL(contentControl);

                auto dataTemplate = static_cast<DataTemplate^>(stackPanel->FindName("dataTemplate"));
                VERIFY_IS_NOT_NULL(dataTemplate);

                auto grid = static_cast<Grid^>(dataTemplate->LoadContent());
                VERIFY_IS_NOT_NULL(grid);

                auto r2 = ref new Microsoft::UI::Xaml::Shapes::Rectangle();
                r2->Name = "R2";
                grid->Children->Append(r2);

                auto r1 = static_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(grid->FindName("R1"));
                VERIFY_IS_NOT_NULL(r1);

                r2 = static_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(grid->FindName("R2"));
                VERIFY_IS_NOT_NULL(r2);


            });
        }


        void NamingIntegrationTests::PopupTest()
        {
            TestCleanupWrapper cleanup;
            RunOnUIThread([&]()
            {
                auto popup = ref new Popup();
                auto grid = ref new Grid();
                auto r1 = ref new Microsoft::UI::Xaml::Shapes::Rectangle();
                r1->Name = "R1";
                grid->Children->Append(r1);
                r1 = static_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(grid->FindName("R1"));
                VERIFY_IS_NULL(r1);

                popup->Child = grid;
                r1 = static_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(grid->FindName("R1"));
                VERIFY_IS_NULL(r1);

                popup->IsOpen = true;
                r1 = static_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(grid->FindName("R1"));
                VERIFY_IS_NOT_NULL(r1);
            });
        }


        DependencyObject^ NamingIntegrationTests::CreateElementWithName(String^ type, String^ name)
        {
            Platform::String^ xamlContent =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"        x:Name='canvas2'>\r\n"
                L"    <" + type + L" x:Name='" + name + L"' />\r\n"
                L"</Canvas>";

            Canvas^ canvas = safe_cast<Canvas^>(XamlReader::Load(xamlContent));
            DependencyObject^ result = canvas->Children->GetAt(0);
            canvas->Children->RemoveAt(0);
            return result;
        }

        void NamingIntegrationTests::UserControlTest()
        {
            TestCleanupWrapper cleanup;
            Microsoft::UI::Xaml::Shapes::Rectangle^ rectangle1;
            Page^ page1;
            Grid^ nameScope;

            RunOnUIThread([&]()
            {
                // Create a custom page ( a derived user control)
                page1 = ref new Page();
                Application::LoadComponent(
                    page1,
                    ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/framework/naming/NamingPage.xaml"),
                    Primitives::ComponentResourceLocation::Application);

                // Find a simple named element in a UserControl
                rectangle1 = static_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(page1->FindName("Rectangle1"));
                VERIFY_IS_NOT_NULL(rectangle1);

                // Have VSM do a name lookup from a visual state and from a visual transition.
                VisualStateManager::GoToState(page1, "State", false);
                VERIFY_ARE_EQUAL( rectangle1->Visibility, Visibility::Collapsed);

                // Find a name inside a property that's not a visual child.
                auto dataContextGrid = static_cast<Grid^>(page1->FindName("DataContextGrid"));
                VERIFY_IS_NOT_NULL(dataContextGrid);

                // Create another copy of the same custom user control
                auto page2 = ref new Page();
                Application::LoadComponent(
                    page2,
                    ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/framework/naming/NamingPage.xaml"),
                    Primitives::ComponentResourceLocation::Application);

                // Check that the user control's definition name.
                VERIFY_ARE_EQUAL(page1->Name, ref new String(L"DefinitionRootName"));

                // Name the two instances of the user control and put them into a tree

                page1->Name = "UC1";
                VERIFY_ARE_EQUAL(page1->Name, ref new String(L"UC1"));
                page2->Name = "UC2";

                nameScope = static_cast<Grid^>(XamlReader::Load("<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'/>"));
                nameScope->Children->Append( page1 );
                nameScope->Children->Append( page2 );


                // Should be able to add a named element to an ItemsControl.Items that was created in markup
                // (This used to rely on PostParseRegisterNames)
                auto itemsControl = static_cast<ItemsControl^>(page1->FindName("ItemsControl1"));
                VERIFY_IS_NOT_NULL(itemsControl);
                auto textBlock = ref new TextBlock();
                auto textBlockName = ref new String(L"RuntimeTextBlock");
                //auto textBlock = dynamic_cast<TextBlock^>(CreateElementWithName(L"TextBlock", textBlockName));
                textBlock->Name = textBlockName;
                itemsControl->Items->Append( textBlock );
                textBlock = static_cast<TextBlock^>(page1->FindName(textBlockName));
                VERIFY_IS_NOT_NULL(textBlock);


                // Verify that from the outer tree, we can't find a named element inside the user control trees
                rectangle1 = static_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(nameScope->FindName("Rectangle1"));
                VERIFY_IS_NULL(rectangle1);

                // We should still be able to find the user control's definition name, if searching from the user control itself.
                page1 = static_cast<Page^>(page1->FindName("DefinitionRootName"));
                VERIFY_IS_NOT_NULL(page1);

                // But searching again from the user control, we shouldn't be able to find its usage name.
                page1 = static_cast<Page^>(page1->FindName("UC1"));
                VERIFY_IS_NULL(page1);

                // And from the outer name scope, we shouldn't be able to find the user control's definition name.
                page1 = static_cast<Page^>(nameScope->FindName("DefinitionRootName"));
                VERIFY_IS_NULL(page1);

                // But we should be able to find its usage name.
                page1 = static_cast<Page^>(nameScope->FindName("UC1"));
                VERIFY_IS_NOT_NULL(page1);

            });
        }

        void NamingIntegrationTests::ControlTemplateTest()
        {
            TestCleanupWrapper cleanup;
            RunOnUIThread([&]()
            {
                String^ xamlString =
                    L"<local:NamingCustomControl"
                    L"  xmlns='http://schemas.microsoft.com/client/2007' "
                    L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"  xmlns:local='using:Tests.Native.External.Framework.Naming'>"
                    L"     <local:NamingCustomControl.Template>"
                    L"        <ControlTemplate>"
                    L"           <Grid x:Name='rootGrid'>"
                    L"               <Grid x:Name='nestedGrid'/>"
                    L"           </Grid>"
                    L"        </ControlTemplate>"
                    L"     </local:NamingCustomControl.Template>"
                    L"</local:NamingCustomControl>";

                auto control = static_cast<NamingCustomControl^>(XamlReader::Load(xamlString));
                VERIFY_IS_NOT_NULL(control);

                control->ApplyTemplate();

                VERIFY_IS_TRUE(control->Validate());

            });
        }

        void NamingIntegrationTests::ContentPresenterTest()
        {
            TestCleanupWrapper cleanup;
            RunOnUIThread([&]()
            {
                String^ xamlString=
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"<ContentPresenter >"
                    L"  <ContentPresenter.Content>"
                    L"      <Grid>"
                    L"          <TextBlock x:Name='TB1'/>"
                    L"      </Grid>"
                    L"  </ContentPresenter.Content>"
                    L"</ContentPresenter>"
                    L"</Grid>";

                auto cp = static_cast<Grid^>(XamlReader::Load(xamlString));
                auto tb1 = static_cast<TextBlock^>(cp->FindName("TB1"));
                VERIFY_IS_NOT_NULL(tb1);

            });
        }

        // Validate put_Name calls that happen during parse.
        void NamingIntegrationTests::MidParseSetNameTest()
        {
            TestCleanupWrapper cleanup;
            Microsoft::UI::Xaml::Shapes::Rectangle^ rectangle1;
            Page^ page1;
            Grid^ nameScope;

            RunOnUIThread([&]()
            {
                // Create a custom user control, where the markup has a ConnectionId on a Grid.  During LoadComponent, MyPage's
                // Connect method will consequently be called, and in this method it sets a name on that Grid.
                page1 = ref new MyPage();
                Application::LoadComponent(
                    page1,
                    ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/framework/naming/NamingPage.xaml"),
                    Primitives::ComponentResourceLocation::Application);

                // We should be able to FindName for the name that was set during the Connect callback.
                auto connectedGrid = static_cast<Grid^>(page1->FindName(connectedGridName));
                VERIFY_IS_NOT_NULL(connectedGrid);
            });
        }

        // Validate x:Name on a DataTemplate inside a ResourceDictionary works with x:ConnectionId based
        // name hookups.
        void NamingIntegrationTests::ValidateNamedDataTemplateInDictionaryWithConnectionId()
        {
            TestCleanupWrapper cleanup;
            RunOnUIThread([&]()
            {
                MyPage^ page1;

                // Create a custom user control, where the markup has a ConnectionId on a DataTemplate.  During LoadComponent, MyPage's
                // Connect method will consequently be called, and in this method it connects the DataTemplate to a named field.
                page1 = ref new MyPage();
                Application::LoadComponent(
                    page1,
                    ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/framework/naming/NamingPage.xaml"),
                    Primitives::ComponentResourceLocation::Application);

                auto namedDataTemplate = page1->NamedDataTemplate();
                VERIFY_IS_NOT_NULL(namedDataTemplate);

                auto layoutRoot = static_cast<Grid^>(page1->FindName(L"layoutRoot"));
                auto foundTemplate = static_cast<DataTemplate^>(layoutRoot->Resources->Lookup(L"namedDataTemplate"));
                VERIFY_IS_NOT_NULL(foundTemplate);
                VERIFY_ARE_EQUAL(namedDataTemplate, foundTemplate);
            });
        }

        void NamingIntegrationTests::NestedUserControlWithCustomContent()
        {
            TestCleanupWrapper cleanup;
            RunOnUIThread([&]()
            {
                auto page1 = ref new MyPage();
                Application::LoadComponent(
                    page1,
                    ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/framework/naming/NamingPage.xaml"),
                    Primitives::ComponentResourceLocation::Application);

                auto NamingUserControl1 = static_cast<UserControl^>(page1->FindName(L"NamingUserControl1"));
                VERIFY_IS_NOT_NULL(NamingUserControl1->FindName(L"Canvas1"));
            });
        }

        void NamingIntegrationTests::UserControlWithinDataTemplateCanStillFindItsElements()
        {
            TestCleanupWrapper cleanup;
            FrameworkElement^ control = nullptr;

            RunOnUIThread([&]()
            {
                auto page1 = ref new MyPage();
                Application::LoadComponent(
                    page1,
                    ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/framework/naming/NamingPage.xaml"),
                    Primitives::ComponentResourceLocation::Application);

                control = static_cast<FrameworkElement^>(page1->FindName(L"ContentControl1"));
                VERIFY_IS_NOT_NULL(control);

                TestServices::WindowHelper->WindowContent = page1;
                page1->UpdateLayout();
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto contentPresenter = static_cast<FrameworkElement^>(VisualTreeHelper::GetChild(control, 0));
                VERIFY_IS_NOT_NULL(contentPresenter);
                auto userControl = static_cast<FrameworkElement^>(VisualTreeHelper::GetChild(contentPresenter, 0));
                VERIFY_IS_NOT_NULL(userControl);

                VERIFY_IS_NOT_NULL(userControl->FindName(L"GridInUserControl"));
            });
        }

        void NamingIntegrationTests::StoryboardCanResolveNamescopeOwnerViaMentor()
        {
            TestCleanupWrapper cleanup;
            MyPage^ page1 = nullptr;
            xaml_shapes::Rectangle^ rect = nullptr;
            Storyboard^ storyboard = nullptr;
            auto storyboardCompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed);
            auto storyboardCompletedEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                page1 = ref new MyPage();
                Application::LoadComponent(
                    page1,
                    ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/framework/naming/NamingPage.xaml"),
                    Primitives::ComponentResourceLocation::Application);

                rect = static_cast<xaml_shapes::Rectangle^>(page1->FindName(L"Rectangle1"));
                storyboard = static_cast<Storyboard^>(page1->FindName(L"Storyboard1"));

                storyboardCompletedRegistration.Attach(storyboard,
                    ref new wf::EventHandler<Object^>(
                    [storyboardCompletedEvent]
                    (Object^ sender, Object^ e)
                {
                    storyboardCompletedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = page1;
                page1->UpdateLayout();
            });

            RunOnUIThread([&]()
            {
                storyboard->Begin();
            });

            storyboardCompletedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(1, Canvas::GetLeft(rect));
            });
        }

        void NamingIntegrationTests::UserControlWithDuplicateChildNameRegistrationPrecedence()
        {
            TestCleanupWrapper cleanup;
            MyPage^ page1 = nullptr;

            RunOnUIThread([&]()
            {
                page1 = ref new MyPage();
                Application::LoadComponent(
                    page1,
                    ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/framework/naming/NamedPageWithDuplicateChildName.xaml"),
                    Primitives::ComponentResourceLocation::Application);

                auto grid = safe_cast<Grid^>(page1->FindName(L"DefinitionRootName"));
                VERIFY_IS_TRUE(grid != nullptr);
            });
        }

        void NamingIntegrationTests::ItemsPanelTemplateNamescopeMemberNamRegistrationTest(bool shouldRegister)
        {
            TestCleanupWrapper cleanup;
            GridView^ gridView = nullptr;

            RunOnUIThread([&]()
            {
                auto page1 = ref new MyPage();
                Application::LoadComponent(
                    page1,
                    ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/framework/naming/NamingPage.xaml"),
                    Primitives::ComponentResourceLocation::Application);

                TestServices::WindowHelper->WindowContent = page1;

                gridView = safe_cast<GridView^>(page1->FindName(L"MyGridView"));
                VERIFY_IS_NOT_NULL(gridView);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto border = static_cast<FrameworkElement^>(VisualTreeHelper::GetChild(gridView, 0));
                VERIFY_IS_NOT_NULL(border);
                auto scrollViewer = static_cast<FrameworkElement^>(VisualTreeHelper::GetChild(border, 0));
                VERIFY_IS_NOT_NULL(scrollViewer);
                auto scrollViewerBorder = static_cast<FrameworkElement^>(VisualTreeHelper::GetChild(scrollViewer, 0));
                VERIFY_IS_NOT_NULL(scrollViewerBorder);
                auto scrollViewerGrid = static_cast<FrameworkElement^>(VisualTreeHelper::GetChild(scrollViewerBorder, 0));
                VERIFY_IS_NOT_NULL(scrollViewerGrid);
                auto scrollViewerContentPresenter = static_cast<FrameworkElement^>(VisualTreeHelper::GetChild(scrollViewerGrid, 0));
                VERIFY_IS_NOT_NULL(scrollViewerContentPresenter);
                auto itemsPresenter = static_cast<FrameworkElement^>(VisualTreeHelper::GetChild(scrollViewerContentPresenter, 0));
                VERIFY_IS_NOT_NULL(itemsPresenter);
                auto itemsPanel = static_cast<FrameworkElement^>(VisualTreeHelper::GetChild(itemsPresenter, 1));
                VERIFY_IS_NOT_NULL(itemsPanel);
                VERIFY_ARE_STRINGS_EQUAL(L"itemPanel", itemsPanel->Name->Data());

                if (shouldRegister)
                {
                    VERIFY_IS_NOT_NULL(itemsPanel->FindName("itemPanel"));
                }
                else
                {
                    // Verify we can't find itemPanel when we search within itemPanel.
                    VERIFY_IS_NULL(itemsPanel->FindName("itemPanel"));
                }
            });
        }

        void NamingIntegrationTests::ItemsPanelTemplateNamescopeMembersDoRegisterTheirName()
        {
            ItemsPanelTemplateNamescopeMemberNamRegistrationTest(true);
        }

        void NamingIntegrationTests::ValidateNameRegistrationWithExisitingStandardNamescopeOwner()
        {
            TestCleanupWrapper cleanup;
            Grid^ root;

            RunOnUIThread([&root]()
            {
                root = safe_cast<Grid^>(XamlReader::Load(
                    L"<Grid  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                    L"         xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                    L"</Grid>"
                    ));

                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([root]()
            {
                auto userControl = ref new UserControl();
                root->Children->Append(userControl);

                Application::LoadComponent(
                    userControl,
                    ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/framework/naming/NamedUserControlWithNamedChild.xaml"),
                    Primitives::ComponentResourceLocation::Application);

                auto pic = safe_cast<Image^>(userControl->FindName(L"pic"));
                VERIFY_IS_NOT_NULL(pic);
                auto control = safe_cast<UserControl^>(userControl->FindName(L"UserControl1"));
                VERIFY_IS_NOT_NULL(control);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void NamingIntegrationTests::ValidateNameRegistrationWithoutExisitingStandardNamescopeOwner()
        {
            TestCleanupWrapper cleanup;
            UserControl^ userControl;

            RunOnUIThread([&userControl]()
            {
                userControl = ref new UserControl();

                Application::LoadComponent(
                    userControl,
                    ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/framework/naming/NamedUserControlWithNamedChild.xaml"),
                    Primitives::ComponentResourceLocation::Application);

                auto pic = safe_cast<Image^>(userControl->FindName(L"pic"));
                VERIFY_IS_NOT_NULL(pic);
                auto control = safe_cast<UserControl^>(userControl->FindName(L"UserControl1"));
                VERIFY_IS_NOT_NULL(control);
            });
        }

        void NamingIntegrationTests::CanDataBindNameProperty()
        {
            Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream;
            featureEnforceXbfV2Stream.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);

            TestCleanupWrapper cleanup;
            RunOnUIThread([&]()
            {
                auto uc1 = static_cast<UserControl^>(XamlReader::Load("<UserControl xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\" Name=\"{Binding Source='foo'}\" />"));
                VERIFY_ARE_STRINGS_EQUAL(L"foo", uc1->Name->Data());
            });
        }

} } } } } }


// Used by NamingIntegrationTests::MidParseSetNameTest
void ::Tests::Native::External::Framework::Naming::MyPage::Connect(int id, Platform::Object^ target)
{
    if (id==100)
    {
        auto grid = static_cast<Grid^>(target);
        grid->Name = connectedGridName;
    }
    else if (id==101)
    {
        auto dataTemplate = static_cast<DataTemplate^>(target);
        _namedDataTemplate = dataTemplate;
    }
}





