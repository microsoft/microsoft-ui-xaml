// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <FileLoader.h>
#include <TestCleanupWrapper.h>
#include <DisableErrorReportingScopeGuard.h>
#include "ControlTemplateIntegrationTests.h"
#include <CustomControl.h>
#include <ChildControl.h>
#include "CustomTypes.XamlTypeInfo.g.h"

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Media;

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace ::Tests::Native::External::Framework;

using namespace test_infra;
using namespace std;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {
        namespace ControlTemplates {

    void RunTargetTypeTest(Control^ instance, Platform::String^ templateTargetType, bool isValid);

    bool ControlTemplateIntegrationTests::ClassSetup()
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
     
        bool ControlTemplateIntegrationTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider());
            return true;
        }

    bool ControlTemplateIntegrationTests::TestCleanup()
    {
        TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    Platform::String^ GetFilePath()
    {
        // Get the deployment directory, and then append our test's directory to the end
        auto deploymentDir = GetTestDeploymentDir();
        return ref new Platform::String(deploymentDir + L"resources\\native\\framework\\controlTemplates\\");
    }

    void ControlTemplateIntegrationTests::CanApplyTemplateToTargetType()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            // Instantiate various classes and subclasses and verify everything checks out
            RunTargetTypeTest(ref new CustomControl, L"CustomControl", true);
            RunTargetTypeTest(ref new CustomControl, L"OtherCustomControl", false);
            RunTargetTypeTest(ref new ChildControl, L"GrandchildControl", false);
            RunTargetTypeTest(ref new OtherCustomControl, L"NonExistentType", false);
        });
    }


    void RunTargetTypeTest(Control^ instance, Platform::String^ templateTargetType, bool isValid)
    {
        DisableErrorReportingScopeGuard disableErrors;

        Platform::String^ xamlString =
            L"<ControlTemplate xmlns='http://schemas.microsoft.com/client/2007' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"    xmlns:local='using:Tests.Native.External.Framework'"
            L"    TargetType='local:" + templateTargetType + L"'>"
            L"    <Grid>"
            L"        <TextBlock Text='I am from template!'/>"
            L"    </Grid>"
            L"</ControlTemplate>";

        try {
            auto controlTemplate = safe_cast<ControlTemplate^>(XamlReader::Load(xamlString));
            auto rootCanvas = ref new Canvas;
            rootCanvas->Children->Append(instance);
            instance->Template = controlTemplate;
            instance->ApplyTemplate();
        }
        catch(Platform::COMException^ ex) {
            auto message = L"Unexpectedly failed to apply ControlTemplate: " + ex->Message;
            VERIFY_IS_FALSE(isValid, message->Data());
            return;
        }
        VERIFY_IS_TRUE(
            isValid,
            WEX::Common::String().Format(
            L"Expected exception for ControlTemplate with TargetType %s applied to control with type %s",
            templateTargetType->Data(), instance->GetType()->FullName->Data()));
    }

    void ControlTemplateIntegrationTests::CanObserveOnApplyTemplateCall()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            Platform::String^ xamlString =
                L"<ControlTemplate xmlns='http://schemas.microsoft.com/client/2007' "
                L"                 xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"                 xmlns:local='using:Tests.Native.External.Framework'>"
                L"  <Canvas x:Name='templateChild'/>"
                L"</ControlTemplate>";
            auto controlTemplate = safe_cast<ControlTemplate^>(XamlReader::Load(xamlString));
            auto control = ref new CustomControl;
            control->OnApplyTemplateInvoked = false;
            control->Template = controlTemplate;
            control->ApplyTemplate();
            VERIFY_IS_TRUE(control->OnApplyTemplateInvoked, L"OnApplyTemplate should be invoked");
        });
    }

    void ControlTemplateIntegrationTests::CantApplyControlTemplateToUserControl()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([]()
        {
            Platform::String^ xamlString =
                L"<ControlTemplate xmlns='http://schemas.microsoft.com/client/2007' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <TextBlock x:Name='UserControlExplostionTest' Text='I am from template!'/>"
                L"</ControlTemplate>";
            auto controlTemplate = safe_cast<ControlTemplate^>(XamlReader::Load(xamlString));
            auto control = ref new UserControl;

            VERIFY_THROWS_WINRT(control->Template = controlTemplate, Platform::COMException^, L"UserControl.Template setter should throw an exception");
        });
    }

    CustomControl^ CreateNamedCustomControl(Platform::String^ name)
    {
        Platform::String^ xamlString =
            L"<local:CustomControl"
            L"  xmlns='http://schemas.microsoft.com/client/2007' "
            L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  xmlns:local='using:Tests.Native.External.Framework'"
            L"  x:Name='" + name + L"'/>";
        CustomControl^ result = nullptr;
        RunOnUIThread([&]()
        {
            result = safe_cast<CustomControl^>(XamlReader::Load(xamlString));
        });
        return result;
    }

    void ControlTemplateIntegrationTests::CanApplyControlTemplateWithRootElementNamedAsExistingElementInTree()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            Platform::String^ templateXaml =
                L"<ControlTemplate xmlns='http://schemas.microsoft.com/client/2007' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"     <TextBlock x:Name='templateNamescopeTest' Text='I am from template!'/>"
                L"</ControlTemplate>";
            auto controlTemplate = safe_cast<ControlTemplate^>(XamlReader::Load(templateXaml));

            auto control = CreateNamedCustomControl(L"templateNamescopeTest");
            auto rootCanvas = ref new Canvas;
            control->Template = controlTemplate;
            control->ApplyTemplate();
        });
    }

    void ControlTemplateIntegrationTests::CanGetTemplateChildFindControlTemplateChildren()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            Platform::String^ templateXaml =
                L"<ControlTemplate xmlns='http://schemas.microsoft.com/client/2007' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"     <Grid x:Name='grid'>"
                L"         <Canvas x:Name='canvas'>"
                L"             <TextBox x:Name='textBox'/>"
                L"         </Canvas>"
                L"         <TextBlock x:Name='textBlock' Text='I am from template!'/>"
                L"     </Grid>"
                L"</ControlTemplate>";
            auto controlTemplate = safe_cast<ControlTemplate^>(XamlReader::Load(templateXaml));

            auto control = CreateNamedCustomControl(L"templateNamescopeTest");
            auto rootCanvas = ref new Canvas;
            control->Template = controlTemplate;
            control->ApplyTemplate();

            auto grid = safe_cast<Grid^>(control->PublicGetTemplateChild(L"grid"));
            VERIFY_IS_NOT_NULL(grid, L"GetTemplateChild(\"grid\") shouldn't return null");

            auto canvas = safe_cast<Canvas^>(control->PublicGetTemplateChild(L"canvas"));
            VERIFY_IS_NOT_NULL(canvas, L"GetTemplateChild(\"canvas\") shouldn't return null");

            auto textBox = safe_cast<TextBox^>(control->PublicGetTemplateChild(L"textBox"));
            VERIFY_IS_NOT_NULL(textBox, L"GetTemplateChild(\"textBox\") shouldn't return null");

            auto textBlock = safe_cast<TextBlock^>(control->PublicGetTemplateChild(L"textBlock"));
            VERIFY_IS_NOT_NULL(textBlock, L"GetTemplateChild(\"textBlock\") shouldn't return null");
        });
    }

    void ControlTemplateIntegrationTests::CanGetTemplateChildIgnoreUnnamedChildrenAndOutOfScopeNames()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            Platform::String^ templateXaml =
                L"<ControlTemplate xmlns='http://schemas.microsoft.com/client/2007' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"      <TextBlock Text='I am from template!'/>"
                L"</ControlTemplate>";
            auto controlTemplate = safe_cast<ControlTemplate^>(XamlReader::Load(templateXaml));

            auto control = CreateNamedCustomControl(L"templateNamescopeTest");
            auto rootCanvas = ref new Canvas;
            control->Template = controlTemplate;
            control->ApplyTemplate();

            VERIFY_IS_NULL(control->PublicGetTemplateChild(L"nonexistentName"), L"GetTemplateChild(\"nonExistentName\") should return null");
            VERIFY_IS_NULL(control->PublicGetTemplateChild(L"templateNamescopeTest"), L"GetTemplateChild(\"templateNamescopeTest\") should return null");
        });
    }

    void ControlTemplateIntegrationTests::TargetTypeInXmlNamespacesTest()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            Platform::String^ customXmlNamespaceXaml =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' xmlns:local='using:Tests.Native.External.Framework'>"
                L"  <Canvas.Resources>"
                L"    <SolidColorBrush x:Name='brush' Color='Blue' />"
                L"    <ControlTemplate x:Name='template' TargetType='local:CustomControl'>"
                L"      <TextBlock />"
                L"    </ControlTemplate>"
                L"  </Canvas.Resources>"
                L"</Canvas>";
            Platform::String^ notExistingXmlNamespaceXaml =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Canvas.Resources>"
                L"    <SolidColorBrush x:Name='brush' Color='Blue' />"
                L"    <ControlTemplate x:Name='template' TargetType='local:CustomControl'>"
                L"      <TextBlock />"
                L"    </ControlTemplate>"
                L"  </Canvas.Resources>"
                L"</Canvas>";
            Platform::String^ redefineXmlNamespaceXaml =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' xmlns:local='using:Tests.Native.External'>"
                L"  <Canvas.Resources>"
                L"    <SolidColorBrush x:Name='brush' Color='Blue' />"
                L"    <ControlTemplate x:Name='template' TargetType='local:CustomControl' xmlns:local='using:Tests.Native.External.Framework'>"
                L"      <TextBlock />"
                L"    </ControlTemplate>"
                L"  </Canvas.Resources>"
                L"</Canvas>";

            auto runTest = [](Platform::String^ xamlString, bool isValid)
            {
                DisableErrorReportingScopeGuard disableErrors;

                try
                {
                    auto rootControl = safe_cast<Canvas^>(XamlReader::Load(xamlString));
                    auto controlTemplate = safe_cast<ControlTemplate^>(rootControl->Resources->Lookup(Platform::StringReference(L"template")));
                    auto instance = ref new CustomControl;
                    rootControl->Children->Append(instance);
                    instance->Template = controlTemplate;
                    instance->ApplyTemplate();

                    VERIFY_IS_TRUE(isValid, L"Exception should have been thrown by XamlReader::Load or Control::ApplyTemplate");
                }
                catch (Platform::Exception^ ex)
                {
                    if (isValid)
                    {
                        VERIFY_FAIL((L"Exception should not have been thrown. Exception message: " + ex->Message)->Data());
                    }
                }
            };

            runTest(customXmlNamespaceXaml, true);
            runTest(notExistingXmlNamespaceXaml, false);
            runTest(redefineXmlNamespaceXaml, true);
        });
    }

    void ControlTemplateIntegrationTests::DefaultControlTemplateTargetTypeIsControlTest()
    {
        TestCleanupWrapper cleanup;
        Grid^ grid = nullptr;
        Button^ button = nullptr;

        RunOnUIThread([&] () {
            Platform::String^ templateXaml =
                L"<ControlTemplate xmlns='http://schemas.microsoft.com/client/2007' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"    <Rectangle x:Name='innerRectangle' Width='{TemplateBinding Width}' Height='300' />\r\n"
                L"</ControlTemplate>\r\n";

            auto controlTemplate = safe_cast<ControlTemplate^>(XamlReader::Load(templateXaml));
            auto control = CreateNamedCustomControl(L"defaultTargetTypeTest");
            control->Template = controlTemplate;
            control->Width = 200;
            control->ApplyTemplate();

            auto innerRectangle = safe_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(control->PublicGetTemplateChild(L"innerRectangle"));
            VERIFY_IS_NOT_NULL(innerRectangle, L"GetTemplateChild(\"innerRectangle\") shouldn't return null");
            VERIFY_ARE_EQUAL(innerRectangle->Width, 200);
            VERIFY_ARE_EQUAL(innerRectangle->Height, 300);
        });
    }

} } } } } }
