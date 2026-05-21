// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <StringUtilities.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <DisableErrorReportingScopeGuard.h>
#include <ppltasks.h>
#include "ParserIntegrationTests.h"
#include <FileLoader.h>
#include <Utils.h>
#include <TreeHelper.h>
#include <collection.h>
#include <algorithm>
#include <RuntimeEnabledFeatureOverride.h>

#include <CustomTypeMetadataProvider.h>
#include <ParserTests.CustomButton.h>
#include <CustomMetadataRegistrar.h>

using namespace Platform;
using namespace Concurrency;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;

using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace ::Tests::Native::External::Framework::Parser;
using namespace std;

using Colors = Microsoft::UI::Colors;
using Color = ::Windows::UI::Color;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Parser {

    void VerifyFindName(FrameworkElement^ control, Platform::String^ name, bool expectedResult, const wchar_t* remark);
    void VerifyGoToState(Microsoft::UI::Xaml::Controls::Control^ control, Platform::String^ stateName, bool expectedResult);
    IVector<VisualStateGroup^>^ GetVisualStateGroups(Microsoft::UI::Xaml::Controls::Control^ control);
    VisualStateGroup^ GetVSGByName(Microsoft::UI::Xaml::Controls::Control^ control, Platform::String^ vsgName);
    VisualState^ AddVisualState(Microsoft::UI::Xaml::Controls::Control^ control, Platform::String^ vsName, Platform::String^ vsgName, double targetWidth);
    void VerifyAreInSameNamescope(const vector<DependencyObject^>& listOfControls);
    DependencyObject^ FindNameByWalkingTree(DependencyObject^ element, String^ name);
    void VerifyAreInSeparateNamescope(const vector<DependencyObject^>& listOfControls1, const vector<DependencyObject^>& listOfControls2);
    DependencyObject^ CreateElementWithName(String^ type, String^ name);

    Platform::String^ GetFilePath()
    {
        // Get the deployment directory, and then append our test's directory to the end
        auto deploymentDir = GetTestDeploymentDir();
        return ref new Platform::String(deploymentDir + L"resources\\native\\framework\\parser\\");
    }

    bool ParserIntegrationTests::ClassSetup()
    {
        auto loaderFile = create_task(::Windows::Storage::StorageFile::GetFileFromPathAsync(GetFilePath() + L"VsmNameScopingTests.xaml"));
        auto loaderTask = create_task(::Windows::Storage::FileIO::ReadTextAsync(loaderFile.get()));

        // It's very important to call EnsureInitialized on TestServices
        // from ClassSetup. This method will wait for the window to be
        // activated on launch, which avoids a race condition that will block
        // input from being routed to the app. It will also wait for the
        // debugger to attach when the waitForDebugger runtime parameter is
        // specified.
        CommonTestSetupHelper::CommonTestClassSetup();

        m_VsmNameScopingXaml = loaderTask.get();

        return true;
    }

        bool ParserIntegrationTests::TestSetup()
        {
            // Initialize the framework before anything else. We need to make sure the test
            // is in a good state in case the framework was shut down after the previous test.
            // We pass in our registar to inform the WindowHelper that there is custom metadata
            // that needs to be initialized and subsequently cleared when we call ShutdownXaml.
            // Without the custom metadata provider types would not be activatable from XAML and certain
            // parts of Jupiter would fail to function (Jupiter will sometimes look up
            // properties by name, without this metadata that lookup will fail and create
            // lots of HRESULT spew).
            test_infra::TestServices::WindowHelper->InitializeXaml(
                ref new MetadataProvider(),
                // Note here that this is a templated ref class. Your custom types need to have static methods
                // for registering and clearing dependency properties. These must be named RegisterDependencyProperties
                // and ClearDependencyProperties.
                ref new CustomMetadataRegistrar<::Tests::Native::External::Framework::Parser::Primitive::CustomButton>());

            RunOnUIThread([&]()
            {
                dpSetPropertyPathOnCustomDP_DP = DependencyProperty::RegisterAttached(L"SetPropertyPathOnCustomDP_DP", PropertyPath::typeid, Border::typeid, nullptr);
                dpSetBrushOnCustomDP_DP = DependencyProperty::RegisterAttached(L"SetBrushOnCustomDP_DP", Brush::typeid, Border::typeid, nullptr);
                dpCanSetUriPropertyOnNonFrameworkElementObject_DP = DependencyProperty::RegisterAttached(L"CanSetUriPropertyOnNonFrameworkElementObject_DP", Uri::typeid, SolidColorBrush::typeid, nullptr);
            });

            return true;
        }

    bool ParserIntegrationTests::TestCleanup()
    {
        RunOnUIThread([&]()
        {
            dpSetPropertyPathOnCustomDP_DP = nullptr;
            dpSetBrushOnCustomDP_DP = nullptr;
            dpCanSetUriPropertyOnNonFrameworkElementObject_DP = nullptr;
        });

        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    ParserIntegrationTests::VsmTestStruct ParserIntegrationTests::LoadVsmTestElements()
    {
        VsmTestStruct result;
        RunOnUIThread([&]()
        {
            result.hostPanel = safe_cast<Panel^>(XamlReader::Load(m_VsmNameScopingXaml));
            TestServices::WindowHelper->WindowContent = result.hostPanel;
            result.hostPanel->UpdateLayout();
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            result.buttonWithGroupsAndVisualStates = safe_cast<Button^>(result.hostPanel->FindName(L"ButtonControlWithGroupsAndVisualStates"));
            result.controlsUnderTest.push_back(result.buttonWithGroupsAndVisualStates);
            VERIFY_IS_NOT_NULL(result.buttonWithGroupsAndVisualStates, L"Finding ButtonControlWithGroupsAndVisualStates");

            result.buttonWithGroupsAndNoVisualStates = safe_cast<Button^>(result.hostPanel->FindName(L"ButtonControlWithGroupsNoVisualStates"));
            result.controlsUnderTest.push_back(result.buttonWithGroupsAndNoVisualStates);
            VERIFY_IS_NOT_NULL(result.buttonWithGroupsAndNoVisualStates, L"Finding ButtonControlWithGroupsNoVisualStates");

            result.buttonWithNoGroupsAndNoVisualStates = safe_cast<Button^>(result.hostPanel->FindName(L"ButtonControlWithNoGroupsNoVisualStates"));
            result.controlsUnderTest.push_back(result.buttonWithNoGroupsAndNoVisualStates);
            VERIFY_IS_NOT_NULL(result.buttonWithNoGroupsAndNoVisualStates, L"Finding ButtonControlWithNoGroupsNoVisualStates");
        });

        return result;
    }

    void ParserIntegrationTests::CanControlTemplateNameHideVisualStateGroups()
    {
        TestCleanupWrapper cleanup;
        auto testStruct = LoadVsmTestElements();

        RunOnUIThread([&]()
        {
            for (auto& control : testStruct.controlsUnderTest)
            {
                // Verify that the VisualStateGroups aren't visible
                VerifyFindName(control, L"VsgInXaml", false, L"FindVSGDefinedInXaml");

                // Verify that the actual VisualStates also aren't visible
                VerifyFindName(control, L"VsInXaml", false, L"FindVSDefinedInXaml");
            }
        });
    }

    void ParserIntegrationTests::CanGoToVisualStates()
    {
        TestCleanupWrapper cleanup;
        auto testStruct = LoadVsmTestElements();

        RunOnUIThread([&]()
        {
            Platform::String^ vsName = L"VsInXaml";
            Platform::String^ vsgName = L"VsgInXaml";
            Platform::String^ vsNewStateName = L"VsAddedToExistingVsg";

            // Verify that the right visual states are present on the controls
            VerifyGoToState(testStruct.buttonWithGroupsAndVisualStates, vsName, true);
            VerifyGoToState(testStruct.buttonWithGroupsAndNoVisualStates, vsName, false);
            VerifyGoToState(testStruct.buttonWithNoGroupsAndNoVisualStates, vsName, false);

            // Now add a visual state in code and try to go there
            for (auto& control : testStruct.controlsUnderTest)
            {
                AddVisualState(control, vsNewStateName, vsgName, 150);
            }

            {
                DisableErrorReportingScopeGuard disableErrors;
                // This should throw an InvalidOperationException, cannot resolve targetName when going to state added programatically in a ControlTemplate
                VERIFY_THROWS_WINRT(VisualStateManager::GoToState(testStruct.buttonWithGroupsAndVisualStates, vsNewStateName, false), Platform::COMException^);
                VERIFY_THROWS_WINRT(VisualStateManager::GoToState(testStruct.buttonWithGroupsAndNoVisualStates, vsNewStateName, false), Platform::COMException^);
            }

            VerifyGoToState(testStruct.buttonWithNoGroupsAndNoVisualStates, vsNewStateName, false);
        });
    }

    void ParserIntegrationTests::LoadXamlComponent(DependencyObject^ rootObject, Platform::String^ scenarioName)
    {
        String^ componentLocation = "ms-appx:///" + "resources/native/framework/parser/" + scenarioName;

        Application::LoadComponent(
            rootObject,
            ref new ::Windows::Foundation::Uri(componentLocation),
            Primitives::ComponentResourceLocation::Application);

    }

    void ParserIntegrationTests::UserControlNamescopeTest()
    {
        TestCleanupWrapper cleanup;
        UserControl^ userControl = nullptr;
        StackPanel^  layoutRoot = nullptr;
        TextBlock^   textBlock  = nullptr;
        Storyboard^  layoutRootStoryboard = nullptr;
        Storyboard^  textBlockStoryboard  = nullptr;
        auto layoutRootStoryboardCompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed);
        auto layoutRootStoryboardCompletedEvent = std::make_shared<Event>();
        auto textBlockStoryboardCompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed);
        auto textBlockStoryboardCompletedEvent = std::make_shared<Event>();

        RunOnUIThread([&] () {
            userControl = ref new UserControl();
            LoadXamlComponent(userControl, "UserControlNamescopeWithStoryboard.xaml");

            VERIFY_IS_NOT_NULL(userControl);

            VerifyFindName(userControl, L"layoutRoot", true, L"FindChildrenInNamescope");
            VerifyFindName(userControl, L"textBlock", true, L"FindChildrenInNamescope");
            VerifyFindName(userControl, L"layoutRootStoryboard", true, L"FindChildrenInNamescope");
            VerifyFindName(userControl, L"textBlockStoryboard", true, L"FindChildrenInNamescope");
            layoutRoot = dynamic_cast<StackPanel^>(userControl->FindName("layoutRoot"));
            textBlock  = dynamic_cast<TextBlock^>(userControl->FindName("textBlock"));
            layoutRootStoryboard = dynamic_cast<Storyboard^>(userControl->FindName("layoutRootStoryboard"));
            textBlockStoryboard  = dynamic_cast<Storyboard^>(userControl->FindName("textBlockStoryboard"));

            layoutRootStoryboardCompletedRegistration.Attach(layoutRootStoryboard,
                ref new wf::EventHandler<Object^>(
                [layoutRootStoryboardCompletedEvent]
                (Object^ sender, Object^ e)
            {
                layoutRootStoryboardCompletedEvent->Set();
            }));
            textBlockStoryboardCompletedRegistration.Attach(textBlockStoryboard,
                ref new wf::EventHandler<Object^>(
                [textBlockStoryboardCompletedEvent]
                (Object^ sender, Object^ e)
            {
                textBlockStoryboardCompletedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = userControl;
            userControl->UpdateLayout();
        });

        RunOnUIThread([&] () {
            textBlockStoryboard->Begin();
        });

        textBlockStoryboardCompletedEvent->WaitForDefault();
        layoutRootStoryboardCompletedEvent->WaitForDefault();

        RunOnUIThread([&] () {
            VERIFY_ARE_EQUAL(64.0, textBlock->Height);
            VERIFY_ARE_EQUAL(128.0, layoutRoot->Width);
        });
    }


    void ParserIntegrationTests::UserControlUsageNamescopeTest()
    {
        TestCleanupWrapper cleanup;
        UserControl^ userControl = nullptr;
        ItemsControl^ itemsControl = nullptr;
        TextBlock^   textBlock  = nullptr;
        Storyboard^  itemsControlStoryboard = nullptr;
        Storyboard^  textBlockStoryboard  = nullptr;
        auto itemsControlStoryboardCompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed);
        auto itemsControlStoryboardCompletedEvent = std::make_shared<Event>();
        auto textBlockStoryboardCompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed);
        auto textBlockStoryboardCompletedEvent = std::make_shared<Event>();

        RunOnUIThread([&] () {
            userControl = ref new UserControl();
            LoadXamlComponent(userControl, "UserControlUsageNamescopeWithStoryboard.xaml");

            VERIFY_IS_NOT_NULL(userControl);

            VerifyFindName(userControl, L"itemsControl", true, L"FindChildrenInNamescope");
            VerifyFindName(userControl, L"textBlock", true, L"FindChildrenInNamescope");
            VerifyFindName(userControl, L"itemsControlStoryboard", true, L"FindChildrenInNamescope");
            VerifyFindName(userControl, L"textBlockStoryboard", true, L"FindChildrenInNamescope");
            itemsControl = dynamic_cast<ItemsControl^>(userControl->FindName("itemsControl"));
            textBlock  = dynamic_cast<TextBlock^>(userControl->FindName("textBlock"));
            itemsControlStoryboard = dynamic_cast<Storyboard^>(userControl->FindName("itemsControlStoryboard"));
            textBlockStoryboard  = dynamic_cast<Storyboard^>(userControl->FindName("textBlockStoryboard"));

            itemsControlStoryboardCompletedRegistration.Attach(itemsControlStoryboard,
                ref new wf::EventHandler<Object^>(
                [itemsControlStoryboardCompletedEvent]
                (Object^ sender, Object^ e)
            {
                itemsControlStoryboardCompletedEvent->Set();
            }));
            textBlockStoryboardCompletedRegistration.Attach(textBlockStoryboard,
                ref new wf::EventHandler<Object^>(
                [textBlockStoryboardCompletedEvent]
                (Object^ sender, Object^ e)
            {
                textBlockStoryboardCompletedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = userControl;
            userControl->UpdateLayout();
        });

        RunOnUIThread([&] () {
            textBlockStoryboard->Begin();
        });

        itemsControlStoryboardCompletedEvent->WaitForDefault();
        textBlockStoryboardCompletedEvent->WaitForDefault();

        RunOnUIThread([&] () {
            VERIFY_ARE_EQUAL(128.0, itemsControl->Width);
            VERIFY_ARE_EQUAL(64.0, textBlock->Height);
        });
    }

    void ParserIntegrationTests::UserControlNamescopeWithoutReferencedResourcesTest()
    {
        TestCleanupWrapper cleanup;
        UserControl^ userControl = nullptr;
        Grid^ layoutRoot = nullptr;
        ContentControl^ contentControl = nullptr;
        SolidColorBrush^ globalBrush = nullptr;
        SolidColorBrush^ layoutRootBrush = nullptr;

        RunOnUIThread([&] () {
            userControl = ref new UserControl();
            LoadXamlComponent(userControl, "UserControlNamescopeWithoutReferencedResources.xaml");

            VERIFY_IS_NOT_NULL(userControl);

            VerifyFindName(userControl, L"layoutRoot", true, L"FindChildrenInNamescope");
            VerifyFindName(userControl, L"contentControl", true, L"FindChildrenInNamescope");
            VerifyFindName(userControl, L"globalResourceBrush", true, L"FindChildrenInNamescope");
            VerifyFindName(userControl, L"layoutRootResourceBrush", true, L"FindChildrenInNamescope");
            layoutRoot = dynamic_cast<Grid^>(userControl->FindName("layoutRoot"));
            contentControl = dynamic_cast<ContentControl^>(userControl->FindName("contentControl"));
            globalBrush = dynamic_cast<SolidColorBrush^>(userControl->FindName("globalResourceBrush"));
            layoutRootBrush = dynamic_cast<SolidColorBrush^>(userControl->FindName("layoutRootResourceBrush"));

            VERIFY_IS_TRUE(globalBrush->Color == Colors::Yellow);
            VERIFY_IS_TRUE(layoutRootBrush->Color == Colors::Orange);

            vector<DependencyObject^> listOfControls;
            listOfControls.push_back(userControl);
            listOfControls.push_back(layoutRoot);
            listOfControls.push_back(contentControl);
            listOfControls.push_back(globalBrush);
            listOfControls.push_back(layoutRootBrush);

            VerifyAreInSameNamescope(listOfControls);
        });
    }

    void ParserIntegrationTests::UsageScopeAndControlTemplateTest()
    {
        TestCleanupWrapper cleanup;
        UserControl^ userControl = nullptr;
        Button^ button = nullptr;
        TextBlock^ textBlockFromButtonContent = nullptr;
        ItemsControl^ itemsControl = nullptr;
        Canvas^ canvasFromContent = nullptr;

        RunOnUIThread([&] () {
            userControl = ref new UserControl();
            LoadXamlComponent(userControl, "UsageScopeAndControlTemplate.xaml");

            VERIFY_IS_NOT_NULL(userControl);

            VerifyFindName(userControl, L"button", true, L"FindChildrenInNamescope");
            button = dynamic_cast<Button^>(userControl->FindName(L"button"));

            VerifyFindName(userControl, L"textBlockFromButtonContent", true, L"FindChildrenInNamescope");
            textBlockFromButtonContent = dynamic_cast<TextBlock^>(userControl->FindName(L"textBlockFromButtonContent"));

            VerifyFindName(userControl, L"itemsControl", true, L"FindChildrenInNamescope");
            itemsControl = dynamic_cast<ItemsControl^>(userControl->FindName(L"itemsControl"));

            VerifyFindName(userControl, L"canvasFromItemsControlContent", true, L"FindChildrenInNamescope");
            canvasFromContent = dynamic_cast<Canvas^>(userControl->FindName(L"canvasFromItemsControlContent"));

            VERIFY_IS_NULL(FindNameByWalkingTree(button, L"contentPresenterFromTemplate"));
            VERIFY_IS_NULL(FindNameByWalkingTree(itemsControl, L"itemsPresenterFromTemplate"));
            TestServices::WindowHelper->WindowContent = userControl;
            userControl->UpdateLayout();
        });

        RunOnUIThread([&] () {
            button->ApplyTemplate();
            itemsControl->ApplyTemplate();
        });

        RunOnUIThread([&] () {
            ContentPresenter^ contentPresenter = dynamic_cast<ContentPresenter^>(FindNameByWalkingTree(button, L"contentPresenterFromTemplate"));
            VERIFY_IS_NOT_NULL(contentPresenter);

            ItemsPresenter^ itemsPresenter = dynamic_cast<ItemsPresenter^>(FindNameByWalkingTree(itemsControl, L"itemsPresenterFromTemplate"));
            VERIFY_IS_NOT_NULL(itemsPresenter);

            vector<DependencyObject^> listOfControls1;
            listOfControls1.push_back(userControl);
            listOfControls1.push_back(button);
            listOfControls1.push_back(textBlockFromButtonContent);
            listOfControls1.push_back(itemsControl);
            listOfControls1.push_back(canvasFromContent);

            vector<DependencyObject^> listOfControls2;
            listOfControls2.push_back(contentPresenter);
            listOfControls2.push_back(itemsPresenter);

            VerifyAreInSeparateNamescope(listOfControls1, listOfControls2);

            vector<DependencyObject^> listOfControls3;
            listOfControls3.push_back(contentPresenter);

            vector<DependencyObject^> listOfControls4;
            listOfControls4.push_back(itemsPresenter);

            VerifyAreInSeparateNamescope(listOfControls3, listOfControls4);
        });
    }

    void ParserIntegrationTests::PrivateNamescopeCreatedByXamlReaderTest()
    {
        TestCleanupWrapper cleanup;
        Grid^ grid = nullptr;
        Canvas^ canvas = nullptr;
        TextBlock^ textBlock = nullptr;

        RunOnUIThread([&] () {
            grid = ref new Grid();
            grid->Name = "grid";
            TestServices::WindowHelper->WindowContent = grid;
            grid->UpdateLayout();
        });

        RunOnUIThread([&] () {
            Canvas^ xamlContentCanvas = nullptr;
            Platform::String^ xamlContent =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
                L"        x:Name='canvas2'>\r\n"
                L"    <TextBlock x:Name='textBlock' />\r\n"
                L"</Canvas>";

            canvas = safe_cast<Canvas^>(XamlReader::Load(xamlContent));
            VerifyFindName(canvas, L"canvas2", true, L"FindName called on root element loaded by XamlReader couldn't find this element");
            xamlContentCanvas = dynamic_cast<Canvas^>(canvas->FindName("canvas2"));
            VERIFY_ARE_EQUAL(canvas, xamlContentCanvas);

            VerifyFindName(canvas, L"textBlock", true, L"FindChildrenInNamescope");
            textBlock = dynamic_cast<TextBlock^>(canvas->FindName("textBlock"));

            vector<DependencyObject^> listOfControls1;
            listOfControls1.push_back(canvas);
            listOfControls1.push_back(textBlock);
            VerifyAreInSameNamescope(listOfControls1);

            grid->Children->Append(canvas);
            VerifyAreInSameNamescope(listOfControls1);

            listOfControls1.clear();
            listOfControls1.push_back(grid);

            vector<DependencyObject^> listOfControls2;
            listOfControls2.push_back(canvas);
            listOfControls2.push_back(textBlock);

            VerifyAreInSeparateNamescope(listOfControls1, listOfControls2);
        });
    }

    void ParserIntegrationTests::DuplicateUserControlAndTemplateNamesTest()
    {
        TestCleanupWrapper cleanup;
        UserControl^ userControl = nullptr;

        RunOnUIThread([&] () {
            userControl = ref new UserControl();
            LoadXamlComponent(userControl, "DuplicateNamesInUserControlAndTemplateScope.xaml");
            TestServices::WindowHelper->WindowContent = userControl;
            userControl->UpdateLayout();
        });
    }

    void ParserIntegrationTests::AddingElementsToPanelTest()
    {
        TestCleanupWrapper cleanup;
        StackPanel^ stackPanel = nullptr;

        RunOnUIThread([&] () {
            stackPanel = dynamic_cast<StackPanel^>(CreateElementWithName(L"StackPanel", L"stackPanel"));
            VERIFY_IS_NOT_NULL(stackPanel);
            TestServices::WindowHelper->WindowContent = stackPanel;
            stackPanel->UpdateLayout();
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&] () {
            auto canvas1 = dynamic_cast<Canvas^>(CreateElementWithName(L"Canvas", L"canvas1"));
            auto canvas2 = dynamic_cast<Canvas^>(CreateElementWithName(L"Canvas", L"canvas2"));
            VERIFY_IS_NOT_NULL(canvas1);
            VERIFY_IS_NOT_NULL(canvas2);

            stackPanel->Children->Append(canvas1);
            stackPanel->Children->Append(canvas2);

            vector<DependencyObject^> listOfControls;
            listOfControls.push_back(canvas1);
            listOfControls.push_back(canvas2);

            VerifyAreInSameNamescope(listOfControls);
        });
    }

    void ParserIntegrationTests::CanActivateBuiltinEnum()
    {
        RunOnUIThread([&]() {
            auto result = safe_cast<int>(XamlReader::Load(L"<ScrollingIndicatorMode xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>0</ScrollingIndicatorMode>"));
            VERIFY_ARE_EQUAL((int)Microsoft::UI::Xaml::Controls::Primitives::ScrollingIndicatorMode::None, result);
        });
    }

    void ParserIntegrationTests::CanAssignFontWeightToObjectProperty()
    {
        RunOnUIThread([&]() {
            auto result = safe_cast<Microsoft::UI::Xaml::Controls::Control^>(XamlReader::Load(
L"<Control xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>" +
"  <Control.Tag>" +
"    <FontWeight>Normal</FontWeight>" +
"  </Control.Tag>" +
"</Control>"));

            // In Blue, FontWeight was unboxed as an int. We need to preserve the compat
            // behavior, but would like to quirk it and enable genuine FontWeight
            // objects in Threshold and beyond. If the quirk is approved, and we get
            // the test infrastructure to run multiple version flavors, handle that case

            //VERIFY_ARE_EQUAL(400, safe_cast<wut::FontWeight>(result->Tag).Weight);
            VERIFY_ARE_EQUAL(400, safe_cast<int>(result->Tag));
        });
    }

    void ParserIntegrationTests::CanParseEmptyStringForDoubleProperty()
    {
        RunOnUIThread([&]() {
            auto result = safe_cast<DoubleAnimation^>(XamlReader::Load(
                L"<DoubleAnimation xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'" +
                " From='' " +
                " To='1.0' "
                " />"));
            VERIFY_ARE_EQUAL(nullptr, result->By);
            VERIFY_ARE_EQUAL(1.0, result->To->Value);
            VERIFY_ARE_EQUAL(0.0, result->From->Value);
        });
    }

    void ParserIntegrationTests::SetPropertyPathOnCustomDP()
    {
        RunOnUIThread([&]() {
            auto obj = safe_cast<DependencyObject^>(XamlReader::Load(L"<Border xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Border.SetPropertyPathOnCustomDP_DP='Test' />"));
            auto result = safe_cast<PropertyPath^>(obj->GetValue(dpSetPropertyPathOnCustomDP_DP));
            VERIFY_ARE_STRINGS_EQUAL(L"Test", result->Path->Data());
        });
    }

    void ParserIntegrationTests::SetBrushOnCustomDP()
    {
        RunOnUIThread([&]() {
            auto obj = safe_cast<DependencyObject^>(XamlReader::Load(L"<Border xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Border.SetBrushOnCustomDP_DP='Red' />"));
            auto result = safe_cast<SolidColorBrush^>(obj->GetValue(dpSetBrushOnCustomDP_DP));
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, result->Color);
        });
    }

    void ParserIntegrationTests::CanNestControlTemplateInsideVsmTest()
    {
        TestCleanupWrapper cleanup;
        Grid^ grid = nullptr;

        RunOnUIThread([&] () {
            grid = ref new Grid();
            grid->Name = "grid";
            TestServices::WindowHelper->WindowContent = grid;
            grid->UpdateLayout();
        });

        RunOnUIThread([&] () {
            ItemsControl^ xamlItemsControl = nullptr;
            Platform::String^ xamlContent =
        L"<ItemsControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
        L"              xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\r\n"
        L"              x:Name='myItemsControl'>\r\n"
        L"    <VisualStateManager.VisualStateGroups>\r\n"
        L"        <VisualStateGroup x:Name='Orientation'>\r\n"
        L"            <VisualState x:Name='Vertical'>\r\n"
        L"                <Storyboard>\r\n"
        L"                    <ObjectAnimationUsingKeyFrames Storyboard.TargetName='myItemsControl' Storyboard.TargetProperty='ItemsPanel'>\r\n"
        L"                        <DiscreteObjectKeyFrame KeyTime='0'>\r\n"
        L"                            <DiscreteObjectKeyFrame.Value>\r\n"
        L"                                <ItemsPanelTemplate>\r\n"
        L"                                    <ItemsStackPanel Orientation='Vertical' />\r\n"
        L"                                </ItemsPanelTemplate>\r\n"
        L"                            </DiscreteObjectKeyFrame.Value>\r\n"
        L"                        </DiscreteObjectKeyFrame>\r\n"
        L"                    </ObjectAnimationUsingKeyFrames>\r\n"
        L"                </Storyboard>\r\n"
        L"            </VisualState>\r\n"
        L"        </VisualStateGroup>\r\n"
        L"    </VisualStateManager.VisualStateGroups>\r\n"
        L"</ItemsControl>\r\n";

            xamlItemsControl = safe_cast<ItemsControl^>(XamlReader::Load(xamlContent));
            VerifyFindName(xamlItemsControl, L"myItemsControl", true, L"FindName called on root element loaded by XamlReader couldn't find this element");

            grid->Children->Append(xamlItemsControl);
        });
    }

    DependencyObject^ CreateElementWithName(String^ type, String^ name)
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

    DependencyObject^ FindNameByWalkingTree(DependencyObject^ element, String^ name)
    {
        String^ elementName = dynamic_cast<String^>(element->GetValue(FrameworkElement::NameProperty));
        if (elementName == name)
            return element;

        int childCount = VisualTreeHelper::GetChildrenCount(element);
        for (int i = 0; i < childCount; i++)
        {
            DependencyObject^ resultFromChild = FindNameByWalkingTree(VisualTreeHelper::GetChild(element, i), name);
            if (resultFromChild != nullptr)
            {
                return resultFromChild;
            }
        }

        return nullptr;
    }

    void VerifyAreInSameNamescope(const vector<DependencyObject^>& listOfControls)
    {
        for (DependencyObject^ firstElement : listOfControls)
        {
            FrameworkElement^ firstFrameworkElement = dynamic_cast<FrameworkElement^>(firstElement);
            if (firstFrameworkElement == nullptr)
            {
                continue;
            }

            for (DependencyObject^ secondElement : listOfControls)
            {
                String^ secondElementName = dynamic_cast<String^>(secondElement->GetValue(FrameworkElement::NameProperty));
                DependencyObject^ secondElementFromFirstNamescope = dynamic_cast<DependencyObject^>(firstFrameworkElement->FindName(secondElementName));
                VERIFY_ARE_EQUAL(secondElement, secondElementFromFirstNamescope);
            }
        }
    }

    void VerifyAreInSeparateNamescope(const vector<DependencyObject^>& listOfControls1, const vector<DependencyObject^>& listOfControls2)
    {
        for (DependencyObject^ firstElement : listOfControls1)
        {
            FrameworkElement^ firstFrameworkElement = dynamic_cast<FrameworkElement^>(firstElement);
            String^ firstElementName = dynamic_cast<String^>(firstFrameworkElement->GetValue(FrameworkElement::NameProperty));

            for (DependencyObject^ secondElement : listOfControls2)
            {
                FrameworkElement^ secondFrameworkElement = dynamic_cast<FrameworkElement^>(secondElement);
                String^ secondElementName = dynamic_cast<String^>(secondFrameworkElement->GetValue(FrameworkElement::NameProperty));

                if (firstFrameworkElement != nullptr && secondElementName != nullptr)
                {
                    VERIFY_IS_TRUE(secondElement != firstFrameworkElement->FindName(secondElementName));
                }

                if (secondFrameworkElement != nullptr && firstElementName != nullptr)
                {
                    VERIFY_IS_TRUE(firstElement != secondFrameworkElement->FindName(firstElementName));
                }
            }
        }
    }

    // Attempt to find the given name on the given control, check its success against expected result
    void VerifyFindName(Microsoft::UI::Xaml::FrameworkElement^ control, Platform::String^ name, bool expectedResult, const wchar_t* remark)
    {
        auto result = control->FindName(name);
        if (expectedResult) {
            VERIFY_IS_NOT_NULL(result,
                WEX::Common::String().Format(L"%s should be in control %s. REMARK:%s", name->Data(), control->Name->Data(), remark));
        }
        else {
            VERIFY_IS_NULL(result,
                WEX::Common::String().Format(L"%s should not be in control %s. REMARK:%s", name->Data(), control->Name->Data(), remark));
        }
    }

    // Attempt to go to the requested visual state
    void VerifyGoToState(Microsoft::UI::Xaml::Controls::Control^ control, Platform::String^ stateName, bool expectedResult)
    {
        DisableErrorReportingScopeGuard disableErrors;

        auto result = VisualStateManager::GoToState(control, stateName, false);
        if (expectedResult) {
            VERIFY_IS_TRUE(result,
                WEX::Common::String().Format(L"Should go to %s state for %s.", stateName->Data(), control->Name->Data()));
        }
        else {
            VERIFY_IS_FALSE(result,
                WEX::Common::String().Format(L"Should not go to %s state for %s.", stateName->Data(), control->Name->Data()));
        }
    }

    IVector<VisualStateGroup^>^ GetVisualStateGroups(Microsoft::UI::Xaml::Controls::Control^ control)
    {
        auto layoutRoot = safe_cast<FrameworkElement^>(TreeHelper::GetVisualChildByName(control, L"LayoutRoot"));
        VERIFY_IS_NOT_NULL(layoutRoot, WEX::Common::String().Format(L"Finding FE named 'LayoutRoot' in %s.", control->Name->Data()));
        return VisualStateManager::GetVisualStateGroups(layoutRoot);
    }

    VisualStateGroup^ GetVSGByName(Microsoft::UI::Xaml::Controls::Control^ control, Platform::String^ vsgName)
    {
        VisualStateGroup^ result = nullptr;
        auto visualStateGroups = GetVisualStateGroups(control);
        auto iter = std::find_if(
            begin(visualStateGroups),
            end(visualStateGroups),
            [vsgName](VisualStateGroup^ currentGroup)
        {
            return currentGroup->Name == vsgName;
        });

        if (iter != end(visualStateGroups))
        {
            result = *iter;
        }

        return result;
    }

    VisualState^ AddVisualState(Microsoft::UI::Xaml::Controls::Control^ control, Platform::String^ vsName, Platform::String^ vsgName, double targetWidth)
    {
        using namespace xaml_animation;

        VisualState^ vs = nullptr;
        auto vsg = GetVSGByName(control, vsgName);
        if (vsg)
        {
            vs = ref new VisualState;
            vs->SetValue(FrameworkElement::NameProperty, vsName);

            auto animation = ref new DoubleAnimation;
            TimeSpan span;
            span.Duration = 0;
            animation->Duration = DurationHelper::FromTimeSpan(span);
            animation->SetValue(DoubleAnimation::ToProperty, targetWidth);

            auto sb = ref new Storyboard;
            Storyboard::SetTargetName(animation, L"TargetElement");
            Storyboard::SetTargetProperty(animation, L"(FrameworkElement.Width)");
            sb->Children->Append(animation);
            vs->Storyboard = sb;

            vsg->States->Append(vs);
        }
        return vs;
    }

    void ParserIntegrationTests::CanGetIntOutOfParsedItemCollection()
    {
        RunOnUIThread([&]()
        {
            Platform::String^ xamlContent =
                L"<ItemsControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <ItemsControl.Items>"
                L"        <x:Int32>42</x:Int32>"
                L"    </ItemsControl.Items>"
                L"</ItemsControl>";

            auto itemsControl = safe_cast<ItemsControl^>(XamlReader::Load(xamlContent));
            auto result = safe_cast<int>(itemsControl->Items->GetAt(0));
            VERIFY_ARE_EQUAL(42, result);
        });
    }

    void ParserIntegrationTests::CanParsePointRectSize()
    {
        RunOnUIThread([&]()
        {
            VERIFY_IS_NOT_NULL(XamlReader::Load(L"<Point xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' />"));
            VERIFY_IS_NOT_NULL(XamlReader::Load(L"<Rect xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' />"));
            VERIFY_IS_NOT_NULL(XamlReader::Load(L"<Size xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' />"));
        });
    }

    void ParserIntegrationTests::CanSetApplicationRequestedTheme()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Light);
            VERIFY_ARE_EQUAL(Application::Current->RequestedTheme, ApplicationTheme::Light);
            TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Dark);
            VERIFY_ARE_EQUAL(Application::Current->RequestedTheme, ApplicationTheme::Dark);
        });
    }

    void ParserIntegrationTests::VerifyThrowsExceptionOnInvalidLoadComponent()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([&]()
        {
            auto userControl = ref new UserControl();
            VERIFY_THROWS_WINRT(LoadXamlComponent(userControl, "UserControlThatDoesNotExist.xaml"), Platform::Exception^, L"XAML parse exception should be thrown when we load a non existing xaml resource.");
        });
    }


    void ParserIntegrationTests::VerifyThrowsExceptionOnInvalidLoadFromSource()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"    <Grid.Resources>"
                L"        <ResourceDictionary>"
                L"            <ResourceDictionary.MergedDictionaries>"
                L"                <ResourceDictionary Source='Styles/DoesntExist.xaml'/>"
                L"            </ResourceDictionary.MergedDictionaries>"
                L"        </ResourceDictionary>"
                L"    </Grid.Resources>"
                L"</Grid>";

            VERIFY_THROWS_WINRT(XamlReader::Load(xamlString), Platform::Exception^, L"XAML parse exception should be thrown when we load merged dictionary resource that doesn't exist.");
        });
    }

    void ParserIntegrationTests::VerifySystemColorBrushCanBeOverridenInApplicationResourceDictionary()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            Application::Current->Resources->Insert(L"SystemAccentColor", Colors::Green);
            auto guard = wil::scope_exit([]
            {
                Application::Current->Resources->Remove(L"SystemAccentColor");
            });

            Platform::String^ xamlString =
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Grid.Resources>"
                L"        <SolidColorBrush x:Key='myBrush' Color='{StaticResource SystemAccentColor}' />"
                L"    </Grid.Resources>"
                L"</Grid>";
            auto grid = static_cast<Grid^>(XamlReader::Load(xamlString));
            auto scb = static_cast<SolidColorBrush^>(grid->Resources->Lookup(L"myBrush"));
            VERIFY_ARE_EQUAL(Colors::Green, scb->Color);


        });
    }

    void ParserIntegrationTests::CanSetUriPropertyOnNonFrameworkElementObject()
    {
        RunOnUIThread([&]() {
            auto obj = safe_cast<SolidColorBrush^>(XamlReader::Load(L"<SolidColorBrush xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' SolidColorBrush.CanSetUriPropertyOnNonFrameworkElementObject_DP='http://www.microsoft.com' />"));
            auto result = safe_cast<Uri^>(obj->GetValue(dpCanSetUriPropertyOnNonFrameworkElementObject_DP));
            VERIFY_ARE_STRINGS_EQUAL(L"http://www.microsoft.com/", result->ToString()->Data());
        });
    }

    void ParserIntegrationTests::CanSetSimpleProperties()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Validating loading markup with simple, strict properties");
            Platform::String^ xamlString =
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Border x:Name='uielement' CenterPoint='3.14,6.02,2.71' Rotation='3.43'"
                L"        TransformMatrix='3.1,4.1,5.9,2.6,5.3,5.8,9.7,9.3,2.3,8.4,6.2,6.4,3.3,8.3,2.7,9.5' />"
                L"    <Button x:Name='myButton' Content='I am a button' Rotation='3.43' />"
                L"</Grid>";
            auto grid = safe_cast<Grid^>(XamlReader::Load(xamlString));
            auto uielement = safe_cast<UIElement^>(grid->FindName(Platform::StringReference(L"uielement")));
            auto button = safe_cast<Button^>(grid->FindName(Platform::StringReference(L"myButton")));

            VERIFY_IS_TRUE(CompareFloatsWithEpsilon(3.14f, uielement->CenterPoint.x));
            VERIFY_IS_TRUE(CompareFloatsWithEpsilon(6.02f, uielement->CenterPoint.y));
            VERIFY_IS_TRUE(CompareFloatsWithEpsilon(2.71f, uielement->CenterPoint.z));

            VERIFY_IS_TRUE(CompareFloatsWithEpsilon(3.43f, uielement->Rotation));

            VERIFY_IS_TRUE(CompareFloatsWithEpsilon(3.1f, uielement->TransformMatrix.m11));
            VERIFY_IS_TRUE(CompareFloatsWithEpsilon(4.1f, uielement->TransformMatrix.m12));
            VERIFY_IS_TRUE(CompareFloatsWithEpsilon(5.9f, uielement->TransformMatrix.m13));
            VERIFY_IS_TRUE(CompareFloatsWithEpsilon(2.6f, uielement->TransformMatrix.m14));
            VERIFY_IS_TRUE(CompareFloatsWithEpsilon(5.3f, uielement->TransformMatrix.m21));
            VERIFY_IS_TRUE(CompareFloatsWithEpsilon(5.8f, uielement->TransformMatrix.m22));
            VERIFY_IS_TRUE(CompareFloatsWithEpsilon(9.7f, uielement->TransformMatrix.m23));
            VERIFY_IS_TRUE(CompareFloatsWithEpsilon(9.3f, uielement->TransformMatrix.m24));
            VERIFY_IS_TRUE(CompareFloatsWithEpsilon(2.3f, uielement->TransformMatrix.m31));
            VERIFY_IS_TRUE(CompareFloatsWithEpsilon(8.4f, uielement->TransformMatrix.m32));
            VERIFY_IS_TRUE(CompareFloatsWithEpsilon(6.2f, uielement->TransformMatrix.m33));
            VERIFY_IS_TRUE(CompareFloatsWithEpsilon(6.4f, uielement->TransformMatrix.m34));
            VERIFY_IS_TRUE(CompareFloatsWithEpsilon(3.3f, uielement->TransformMatrix.m41));
            VERIFY_IS_TRUE(CompareFloatsWithEpsilon(8.3f, uielement->TransformMatrix.m42));
            VERIFY_IS_TRUE(CompareFloatsWithEpsilon(2.7f, uielement->TransformMatrix.m43));
            VERIFY_IS_TRUE(CompareFloatsWithEpsilon(9.5f, uielement->TransformMatrix.m44));

            VERIFY_IS_TRUE(CompareFloatsWithEpsilon(3.43f, button->Rotation));
        });
    }

    void ParserIntegrationTests::ApplicationLoadComponentExpandsTemplatesUnderDesigner()
    {
        TestCleanupWrapper cleanup;
        RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);

        RunOnUIThread([&]() {
            auto panel = ref new StackPanel();

            VERIFY_THROWS_WINRT(
                Application::LoadComponent(
                    panel,
                    ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/framework/parser/ErrorInUnusedTemplate.xaml"),
                    Primitives::ComponentResourceLocation::Application), Platform::Exception^);
        });
    }

    void ParserIntegrationTests::ResourceDictionary_SourceExpandsTemplatesUnderDesigner()
    {
        TestCleanupWrapper cleanup;
        RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);

        RunOnUIThread([&]() {
            auto panel = ref new StackPanel();

            VERIFY_THROWS_WINRT(
                Application::LoadComponent(
                    panel,
                    ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/framework/parser/ReferenceToResourceDictionaryWithInvalidTemplate.xaml"),
                    Primitives::ComponentResourceLocation::Application), Platform::Exception^);
        });
    }

    void ParserIntegrationTests::CanDefineNamespaceOnMergedDictionary()
    {
        TestCleanupWrapper cleanup;

        auto loadMarkupAndVerify = [&](bool enforceXbfV2Stream)
        {
            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, enforceXbfV2Stream);

            Platform::String^ markup =
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Grid.Resources>"
                    L"        <ResourceDictionary>"
                    L"            <ResourceDictionary.MergedDictionaries>"
                    L"                <ResourceDictionary xmlns:alias='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                    L"                    <alias:SolidColorBrush x:Key='mybrush' Color='Pink' />"
                    L"                    <alias:SolidColorBrush x:Key='mybrush2' Color='Purple' />"
                    L"                </ResourceDictionary>"
                    L"            </ResourceDictionary.MergedDictionaries>"
                    L"            <ResourceDictionary.ThemeDictionaries>"
                    L"                <ResourceDictionary xmlns:alias='http://schemas.microsoft.com/winfx/2006/xaml/presentation' x:Key='Dark'>"
                    L"                    <alias:SolidColorBrush x:Key='dummy' Color='Green' />"
                    L"                    <alias:SolidColorBrush x:Key='dummy2' Color='Red' />"
                    L"                </ResourceDictionary>"
                    L"                <ResourceDictionary xmlns:alias='http://schemas.microsoft.com/winfx/2006/xaml/presentation' x:Key='Light'>"
                    L"                    <alias:SolidColorBrush x:Key='dummy' Color='Blue' />"
                    L"                    <alias:SolidColorBrush x:Key='dummy2' Color='Yellow' />"
                    L"                </ResourceDictionary>"
                    L"            </ResourceDictionary.ThemeDictionaries>"
                    L"        </ResourceDictionary>"
                    L"    </Grid.Resources>"
                    L"    <Rectangle x:Name='myrectangle' Fill='{StaticResource mybrush}' />"
                    L"    <Rectangle x:Name='myrectangle2' Fill='{StaticResource mybrush2}' />"
                    L"</Grid>";

            Grid^ grid;
            RunOnUIThread([&]() {
                grid = safe_cast<Grid^>(XamlReader::Load(markup));
                TestServices::WindowHelper->WindowContent = grid;
                grid->UpdateLayout();
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                auto rectangle = dynamic_cast<xaml_shapes::Rectangle^>(grid->FindName(L"myrectangle"));
                auto rectangle2 = dynamic_cast<xaml_shapes::Rectangle^>(grid->FindName(L"myrectangle2"));
                VERIFY_ARE_EQUAL(Colors::Pink, safe_cast<SolidColorBrush^>(rectangle->Fill)->Color);
                VERIFY_ARE_EQUAL(Colors::Purple, safe_cast<SolidColorBrush^>(rectangle2->Fill)->Color);
            });
        };

        LOG_OUTPUT(L"Testing with XBFv2");
        loadMarkupAndVerify(true);

        LOG_OUTPUT(L"Testing without XBFv2");
        loadMarkupAndVerify(false);
    }

    void ParserIntegrationTests::ResourceAliasesInTemplates()
    {
        // A resource alias is a Static/ThemeResource used in a ResourceDictionary via
        // object-element syntax, thereby providing an alias for another resource

        TestCleanupWrapper cleanup;

        auto loadMarkupAndVerify = [&](bool enforceXbfV2Stream)
        {
            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, enforceXbfV2Stream);

            Platform::String^ markup =
                    L"<Grid"
                    L"    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Grid.Resources>"
                    L"        <SolidColorBrush x:Key='MyBrush' Color='Blue' />"
                    L"        <SolidColorBrush x:Key='MyBrush2' Color='Pink' />"
                    L"    </Grid.Resources>"
                    L"    <Button>"
                    L"        <Button.Template>"
                    L"            <ControlTemplate TargetType='Button'>"
                    L"                <Grid>"
                    L"                    <Grid.Resources>"
                    L"                        <StaticResource x:Key='MyBrush' ResourceKey='MyBrush2' />"
                    L"                    </Grid.Resources>"
                    L"                    <Rectangle Width='50' Height='50' Fill='{StaticResource MyBrush}' />"
                    L"                </Grid>"
                    L"            </ControlTemplate>"
                    L"        </Button.Template>"
                    L"    </Button>"
                    L"</Grid>";

            Grid^ grid;
            RunOnUIThread([&]() {
                grid = safe_cast<Grid^>(XamlReader::Load(markup));
                TestServices::WindowHelper->WindowContent = grid;
                grid->UpdateLayout();
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                auto button = dynamic_cast<Button^>(VisualTreeHelper::GetChild(grid, 0));
                auto templateRoot = dynamic_cast<FrameworkElement^>(VisualTreeHelper::GetChild(button, 0));
                auto rectangle = dynamic_cast<xaml_shapes::Rectangle^>(VisualTreeHelper::GetChild(templateRoot, 0));
                VERIFY_ARE_EQUAL(Colors::Pink, safe_cast<SolidColorBrush^>(rectangle->Fill)->Color);
            });
        };

        LOG_OUTPUT(L"Testing with XBFv2");
        loadMarkupAndVerify(true);

        LOG_OUTPUT(L"Testing without XBFv2");
        loadMarkupAndVerify(false);
    }

    void ParserIntegrationTests::VerifySetPropertyOnBaseClass()
    {
        TestCleanupWrapper cleanup;

        auto loadMarkupAndVerify = [&](bool enforceXbfV2Stream)
        {
            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, enforceXbfV2Stream);

            Platform::String^ markup =
                    L"<Grid"
                    L"    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"    xmlns:local='using:Tests.Native.External.Framework.Parser.Control'>"
                    L"    <local:AwesomeParserButton CustomProperty='foobar' />"
                    L"</Grid>";

            Grid^ grid;
            RunOnUIThread([&]() {
                grid = safe_cast<Grid^>(XamlReader::Load(markup));
                TestServices::WindowHelper->WindowContent = grid;
                grid->UpdateLayout();
            });
            TestServices::WindowHelper->WaitForIdle();
        };

        LOG_OUTPUT(L"Testing with XBFv2");
        loadMarkupAndVerify(true);

        LOG_OUTPUT(L"Testing without XBFv2");
        loadMarkupAndVerify(false);
    }

    void ParserIntegrationTests::VerifyCDATAHandling() 
    {
        TestCleanupWrapper cleanup;

        auto loadMarkupAndVerify = [&](bool enforceXbfV2Stream)
        {
            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, enforceXbfV2Stream);

            Platform::String^ markup =
                L"<Grid"
                L"    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"    xmlns:local='using:Tests.Native.External.Framework.Parser.Control'>"
                L"    <TextBlock>"
                L"        <![CDATA[Hello from Seattle! It is nice & sunny today; we hope it will stay like this through the weekend.]]>"
                L"    </TextBlock>"
                L"</Grid>";

            Grid^ grid;
            RunOnUIThread([&]() {
                grid = safe_cast<Grid^>(XamlReader::Load(markup));
                TestServices::WindowHelper->WindowContent = grid;
                grid->UpdateLayout();
                });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                auto textBlock = dynamic_cast<TextBlock^>(VisualTreeHelper::GetChild(grid, 0));
                VERIFY_ARE_EQUAL(Platform::StringReference(L"Hello from Seattle! It is nice & sunny today; we hope it will stay like this through the weekend."), textBlock->Text);
            });
        };

        LOG_OUTPUT(L"Testing with XBFv2");
        loadMarkupAndVerify(true);

        LOG_OUTPUT(L"Testing without XBFv2");
        loadMarkupAndVerify(false);
     }

    void ParserIntegrationTests::VerifyCollectionInitializationSyntax()
    {
        auto loadMarkupAndVerify = [&](bool enforceXbfV2Stream)
        {
            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, enforceXbfV2Stream);
            DisableErrorReportingScopeGuard disableErrors;

            LOG_OUTPUT(L"Test ColumnDefinitionCollection initialization");
            RunOnUIThread([&]()
            {
                Grid^ grid = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"  ColumnDefinitions='50, Auto, *'>"
                    L"</Grid>"));

                ColumnDefinitionCollection^ cds = grid->ColumnDefinitions;
                VERIFY_ARE_EQUAL((cds->GetAt(0)->Width).Value, 50);
                VERIFY_ARE_EQUAL((cds->GetAt(0)->Width).GridUnitType, xaml::GridUnitType::Pixel);
                VERIFY_ARE_EQUAL((cds->GetAt(1)->Width).GridUnitType, xaml::GridUnitType::Auto);
                VERIFY_ARE_EQUAL((cds->GetAt(2)->Width).Value, 1);
                VERIFY_ARE_EQUAL((cds->GetAt(2)->Width).GridUnitType, xaml::GridUnitType::Star);
            });

            LOG_OUTPUT(L"Throws invalid argument if both syntaxes are combined in collection initialization");
            RunOnUIThread([&]()
            {
                VERIFY_THROWS_WINRT((xaml_markup::XamlReader::Load(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"  ColumnDefinitions='*, Auto, 500' RowDefinitions='*, Auto, 500'>"
                    L"      <Grid.ColumnDefinitions>"
                    L"          <ColumnDefinition Width='*'/>"
                    L"          <ColumnDefinition Width='Auto'/>"
                    L"          <ColumnDefinition Width='500'/>"
                    L"      </Grid.ColumnDefinitions>"
                    L"      <Grid.RowDefinitions>"
                    L"          <RowDefinition Height='*'/>"
                    L"          <RowDefinition Height='Auto'/>"
                    L"          <RowDefinition Height='500'/>"
                    L"      </Grid.RowDefinitions>"
                    L"</Grid>")), Platform::Exception^);
            });

            LOG_OUTPUT(L"Test DefinitionCollections initialization with irregular whitespace");
            RunOnUIThread([&]()
            {
                Grid^ grid = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"  ColumnDefinitions='   *, Auto, 500' RowDefinitions='*,    Auto,     500'>"
                    L"</Grid>"));

                VERIFY_ARE_EQUAL((grid->ColumnDefinitions->GetAt(0)->Width).Value, 1);
                VERIFY_ARE_EQUAL((grid->ColumnDefinitions->GetAt(0)->Width).GridUnitType, xaml::GridUnitType::Star);
                VERIFY_ARE_EQUAL((grid->ColumnDefinitions->GetAt(1)->Width).GridUnitType, xaml::GridUnitType::Auto);
                VERIFY_ARE_EQUAL((grid->ColumnDefinitions->GetAt(2)->Width).Value, 500);
                VERIFY_ARE_EQUAL((grid->ColumnDefinitions->GetAt(2)->Width).GridUnitType, xaml::GridUnitType::Pixel);

                VERIFY_ARE_EQUAL((grid->RowDefinitions->GetAt(0)->Height).Value, 1);
                VERIFY_ARE_EQUAL((grid->RowDefinitions->GetAt(0)->Height).GridUnitType, xaml::GridUnitType::Star);
                VERIFY_ARE_EQUAL((grid->RowDefinitions->GetAt(1)->Height).GridUnitType, xaml::GridUnitType::Auto);
                VERIFY_ARE_EQUAL((grid->RowDefinitions->GetAt(2)->Height).Value, 500);
                VERIFY_ARE_EQUAL((grid->RowDefinitions->GetAt(2)->Height).GridUnitType, xaml::GridUnitType::Pixel);
            });

            LOG_OUTPUT(L"ColumnDefinitionCollection initialization throws invalid argument for empty string");
            RunOnUIThread([&]()
            {
                VERIFY_THROWS_WINRT((xaml_markup::XamlReader::Load(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"  ColumnDefinitions=''>"
                    L"</Grid>")), Platform::Exception^);
            });

            LOG_OUTPUT(L"ColumnDefinitionCollection initialization throws invalid argument for string with empty items");
            RunOnUIThread([&]()
            {
                VERIFY_THROWS_WINRT((xaml_markup::XamlReader::Load(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"  ColumnDefinitions='50,, *'>"
                    L"</Grid>")), Platform::Exception^);
            });

            LOG_OUTPUT(L"ColumnDefinitionCollection initialization throws invalid argument for string with invalid items");
            RunOnUIThread([&]()
            {
                VERIFY_THROWS_WINRT((xaml_markup::XamlReader::Load(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"  ColumnDefinitions='50, a, *'>"
                    L"</Grid>")), Platform::Exception^);
            });
        };
        
        LOG_OUTPUT(L"Testing with XBFv2");
        loadMarkupAndVerify(true);

        LOG_OUTPUT(L"Testing without XBFv2");
        loadMarkupAndVerify(false);
    }

} } } } } }
