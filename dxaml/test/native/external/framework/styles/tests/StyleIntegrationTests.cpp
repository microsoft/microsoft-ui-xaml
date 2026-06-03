// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <FileLoader.h>
#include <Utils.h>
#include <TestCleanupWrapper.h>
#include <DisableErrorReportingScopeGuard.h>
#include <StringUtilities.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <TreeHelper.h>
#include "StyleIntegrationTests.h"
#include "TestCustomResourceLoader.h"
#include <CustomControl.h>
#include <ControlWithAttachedProperty.h>
#include <CustomUserControl.h>
#include <CustomObject.h>
#include <DefaultStyleControl.h>
#include <DefaultStyleResourceUriControl.h>
#include <XamlResourcePropertyBagOverrider.h>
#include "CustomTypes.XamlTypeInfo.g.h"
#include "ppltasks.h"

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Text;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Media;

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace ::Tests::Native::External::Framework;

using namespace test_infra;
using namespace std;

using Colors = Microsoft::UI::Colors;
using Color = ::Windows::UI::Color;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Styles {


    class MultiClassRegistrator
    {
    public:
        static void RegisterDependencyProperties()
        {
            CustomControl::RegisterDependencyProperties();
            ControlWithAttachedProperty::RegisterDependencyProperties();
            CustomUserControl::RegisterDependencyProperties();
        }

        static void ClearDependencyProperties()
        {
            CustomControl::ClearDependencyProperties();
            ControlWithAttachedProperty::ClearDependencyProperties();
            CustomUserControl::ClearDependencyProperties();
        }
    };

    StyleIntegrationTests::StyleIntegrationTests()
    {

    }

    bool StyleIntegrationTests::ClassSetup()
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

    bool StyleIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider(), ref new CustomMetadataRegistrar<MultiClassRegistrator>());

        RunOnUIThread([&]()
        {
            // Ensure we have a chance to set the window content, so that future loads don't suffer
            // from timing issues
            m_rootCanvas = safe_cast<Canvas^>(XamlReader::Load(
                L"<Canvas  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"         xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"</Canvas>"
                ));

            TestServices::WindowHelper->WindowContent = m_rootCanvas;
        });

        return true;
    }

    bool StyleIntegrationTests::TestCleanup()
    {
        // Release our root element, then drain the UI affinity release queue
        m_rootCanvas = nullptr;
        test_infra::TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();

        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    Platform::String^ GetVariationFileName(const wchar_t* baseName, bool isExplicit)
    {
        // Get the deployment directory, and then append our test's directory to the end
        auto deploymentDir = GetTestDeploymentDir();
        auto filenameEnd = isExplicit ? L".explicit.xaml" : L".implicit.xaml";

        return ref new Platform::String(deploymentDir + L"resources\\native\\framework\\styles\\" + baseName + filenameEnd);
    }

    Platform::String^ GetVariationFileName(const wchar_t* fileName)
    {
        // Get the deployment directory, and then append our test's directory to the end
        auto deploymentDir = GetTestDeploymentDir();
        return ref new Platform::String(deploymentDir + L"resources\\native\\framework\\styles\\" + fileName + L".xaml");
    }

    void StyleIntegrationTests::CanStyleSetPropertiesOnCustomControl()
    {
        TestCleanupWrapper cleanup;
        TestStyleForCustomControl(true);
        TestStyleForCustomControl(false);
    }

    void StyleIntegrationTests::TestStyleForCustomControl(bool isExplicit)
    {
        auto canvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetVariationFileName(L"CustomControl", isExplicit)));

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = m_rootCanvas;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            m_rootCanvas->Children->Clear();
            m_rootCanvas->Children->Append(canvas);

            auto targetControl = safe_cast<CustomControl^>(canvas->Children->GetAt(0));

            auto styleString = isExplicit ? L"explicit style" : L"implicit style";

            VERIFY_IS_TRUE(128.0 == targetControl->Width,
                wstring(L"Width property was not set by the ").append(styleString).c_str());
            VERIFY_IS_TRUE(L"A string" == targetControl->CustomString,
                wstring(L"CustomString property was not set by the ").append(styleString).c_str());
            VERIFY_IS_TRUE(123 == targetControl->CustomInt,
                wstring(L"CustomInt property was not set by the ").append(styleString).c_str());
            VERIFY_IS_TRUE(L"abc" == safe_cast<String^>(targetControl->WorkingTag),
                wstring(L"WorkingTag property was not set by the ").append(styleString).c_str());

            VERIFY_IS_TRUE(256.0 == Canvas::GetTop(targetControl),
                wstring(L"Canvas.Top attached property was not set by the ").append(styleString).c_str());
            VERIFY_IS_TRUE(L"customAttached" == ControlWithAttachedProperty::GetCustomAttached(targetControl),
                wstring(L"ControlWithAttachedProperty.CustomAttached attached property was not set by the ").append(styleString).c_str());
        });
    }

    void StyleIntegrationTests::CanStyleSetPropertiesOnMultipleCustomControls()
    {
        TestCleanupWrapper cleanup;
        TestStyleForMultipleCustomControls(true);
        TestStyleForMultipleCustomControls(false);
    }

    void StyleIntegrationTests::TestStyleForMultipleCustomControls(bool isExplicit)
    {
        auto canvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetVariationFileName(L"MultipleCustomControls", isExplicit)));

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = m_rootCanvas;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            m_rootCanvas->Children->Clear();
            m_rootCanvas->Children->Append(canvas);

            auto targetControl1 = safe_cast<CustomControl^>(canvas->Children->GetAt(0));
            auto targetControl2 = safe_cast<CustomControl^>(canvas->Children->GetAt(1));

            auto styleString = isExplicit ? L"explicit style" : L"implicit style";

            VERIFY_IS_TRUE(128.0 == targetControl1->Width,
                wstring(L"Width property was not set by the ").append(styleString).c_str());
            VERIFY_IS_TRUE(L"A string" == targetControl1->CustomString,
                wstring(L"CustomString property was not set by the ").append(styleString).c_str());
            VERIFY_IS_TRUE(123 == targetControl1->CustomInt,
                wstring(L"CustomInt property was not set by the ").append(styleString).c_str());
            VERIFY_IS_TRUE(L"abc" == safe_cast<String^>(targetControl1->WorkingTag),
                wstring(L"WorkingTag property was not set by the ").append(styleString).c_str());
            VERIFY_IS_TRUE(ThicknessHelper::FromUniformLength(2) == targetControl1->CustomThickness,
                wstring(L"CustomThickness property was not set by the ").append(styleString).c_str());

            VERIFY_IS_TRUE(256.0 == Canvas::GetTop(targetControl1),
                wstring(L"Canvas.Top attached property was not set by the ").append(styleString).c_str());
            VERIFY_IS_TRUE(L"customAttached" == ControlWithAttachedProperty::GetCustomAttached(targetControl1),
                wstring(L"ControlWithAttachedProperty.CustomAttached attached property was not set by the ").append(styleString).c_str());

            VERIFY_IS_TRUE(128.0 == targetControl2->Width,
                wstring(L"Width property was not set by the ").append(styleString).c_str());
            VERIFY_IS_TRUE(L"A string" == targetControl2->CustomString,
                wstring(L"CustomString property was not set by the ").append(styleString).c_str());
            VERIFY_IS_TRUE(123 == targetControl2->CustomInt,
                wstring(L"CustomInt property was not set by the ").append(styleString).c_str());
            VERIFY_IS_TRUE(L"abc" == safe_cast<String^>(targetControl2->WorkingTag),
                wstring(L"WorkingTag property was not set by the ").append(styleString).c_str());
            VERIFY_IS_TRUE(ThicknessHelper::FromUniformLength(2) == targetControl2->CustomThickness,
                wstring(L"CustomThickness property was not set by the ").append(styleString).c_str());

            VERIFY_IS_TRUE(256.0 == Canvas::GetTop(targetControl2),
                wstring(L"Canvas.Top attached property was not set by the ").append(styleString).c_str());
            VERIFY_IS_TRUE(L"customAttached" == ControlWithAttachedProperty::GetCustomAttached(targetControl2),
                wstring(L"ControlWithAttachedProperty.CustomAttached attached property was not set by the ").append(styleString).c_str());
        });
    }

    void StyleIntegrationTests::CanStyleSetPropertiesOnMultipleDifferentControls()
    {
        TestCleanupWrapper cleanup;
        TestStyleForMultipleDifferentControls(true);
        TestStyleForMultipleDifferentControls(false);
    }

    void StyleIntegrationTests::TestStyleForMultipleDifferentControls(bool isExplicit)
    {
        auto canvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetVariationFileName(L"MultipleDifferentControls", isExplicit)));

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = m_rootCanvas;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            m_rootCanvas->Children->Clear();
            m_rootCanvas->Children->Append(canvas);

            auto styleString = isExplicit ? L"explicit style" : L"implicit style";

            for (auto it = canvas->Children->First(); it->HasCurrent; it->MoveNext())
            {
                auto currentElement = safe_cast<FrameworkElement^>(it->Current);

                VERIFY_IS_TRUE(128.0 == currentElement->Width,
                    (wstring(L"Width property for control ") + currentElement->GetType()->FullName->Data() + L" property was not set by the " + styleString).c_str());

                VERIFY_IS_TRUE(64.0 == currentElement->Height,
                    (wstring(L"Height property for control ") + currentElement->GetType()->FullName->Data() + L" property was not set by the " + styleString).c_str());
            }
        });
    }

    void StyleIntegrationTests::CanStyleSetPropertiesOnCustomUserControl()
    {
        TestCleanupWrapper cleanup;
        TestStyleForCustomUserControl(true);
        TestStyleForCustomUserControl(false);
    }

    void StyleIntegrationTests::TestStyleForCustomUserControl(bool isExplicit)
    {
        auto canvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetVariationFileName(L"CustomUserControl", isExplicit)));

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = m_rootCanvas;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            m_rootCanvas->Children->Clear();
            m_rootCanvas->Children->Append(canvas);

            auto targetControl = safe_cast<CustomUserControl^>(canvas->Children->GetAt(0));

            auto styleString = isExplicit ? L"explicit style" : L"implicit style";

            VERIFY_IS_TRUE(128.0 == targetControl->Width,
                wstring(L"Width property was not set by the ").append(styleString).c_str());
            VERIFY_IS_TRUE(L"A string" == targetControl->CustomString,
                wstring(L"CustomString property was not set by the ").append(styleString).c_str());
            VERIFY_IS_TRUE(123 == targetControl->CustomInt,
                wstring(L"CustomInt property was not set by the ").append(styleString).c_str());
            VERIFY_IS_TRUE(L"abc" == safe_cast<String^>(targetControl->WorkingTag),
                wstring(L"WorkingTag property was not set by the ").append(styleString).c_str());

            VERIFY_IS_TRUE(256.0 == Canvas::GetTop(targetControl),
                wstring(L"Canvas.Top attached property was not set by the ").append(styleString).c_str());
            VERIFY_IS_TRUE(L"customAttached" == ControlWithAttachedProperty::GetCustomAttached(targetControl),
                wstring(L"ControlWithAttachedProperty.CustomAttached attached property was not set by the ").append(styleString).c_str());
        });
    }

    void StyleIntegrationTests::CanStyleSetControlTemplateUsingStaticResource()
    {
        TestCleanupWrapper cleanup;
        TestControlTemplateStyleFromStaticResource(true);
        TestControlTemplateStyleFromStaticResource(false);
    }

    void StyleIntegrationTests::TestControlTemplateStyleFromStaticResource(bool isExplicit)
    {
        auto canvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetVariationFileName(L"ControlTemplateStyleFromStaticResource", isExplicit)));

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = m_rootCanvas;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            m_rootCanvas->Children->Clear();
            m_rootCanvas->Children->Append(canvas);

            auto targetControl = safe_cast<CustomControl^>(canvas->Children->GetAt(0));
            targetControl->ApplyTemplate();

            auto textBlockFromTemplate = safe_cast<TextBlock^>(targetControl->PublicGetTemplateChild(L"textBlockFromTemplate"));

            auto styleString = isExplicit ? L"explicit style" : L"implicit style";

            VERIFY_IS_TRUE(L"Monty Python" == textBlockFromTemplate->Text,
                wstring(L"Template was not set by the ").append(styleString).c_str());
        });
    }

    void StyleIntegrationTests::CanStyleSetControlTemplateDirectlyExplicit()
    {
        TestCleanupWrapper cleanup;
        TestControlTemplateStyleDirect(true);
    }

    void StyleIntegrationTests::CanStyleSetControlTemplateDirectlyImplicit()
    {
        TestCleanupWrapper cleanup;
        TestControlTemplateStyleDirect(false);
    }

    void StyleIntegrationTests::TestControlTemplateStyleDirect(bool isExplicit)
    {
        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = m_rootCanvas;
        });
        TestServices::WindowHelper->WaitForIdle();

        Canvas^ canvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetVariationFileName(L"ControlTemplateStyleDirect", isExplicit)));

        RunOnUIThread([&]()
        {
            m_rootCanvas->Children->Clear();
            m_rootCanvas->Children->Append(canvas);

            auto targetControl = safe_cast<CustomControl^>(canvas->Children->GetAt(0));
            targetControl->ApplyTemplate();

            auto textBlockFromTemplate = safe_cast<TextBlock^>(targetControl->PublicGetTemplateChild(L"textBlockFromTemplate"));
            auto styleString = isExplicit ? L"explicit style" : L"implicit style";

            VERIFY_IS_TRUE(L"Monty Python" == textBlockFromTemplate->Text,
                wstring(L"Template was not set by the ").append(styleString).c_str());
        });

    }

    void StyleIntegrationTests::CanStyleSetterDetectMissingValue()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([]()
        {
            Platform::String^ xamlString =
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"    <Grid.Resources>"
                L"        <Style x:Key='myStyle' TargetType='TextBox'>"
                L"            <Setter Property='Width'/>"
                L"        </Style>"
                L"    </Grid.Resources>"
                L"</Grid>";

            VERIFY_THROWS_WINRT(XamlReader::Load(xamlString), Platform::Exception^, L"XAML parse exception should be thrown when a style setter's value is missing");
        });
    }

    void StyleIntegrationTests::CanStyleSetterDetectMissingProperty()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([]()
        {
            Platform::String^ xamlString =
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"    <Grid.Resources>"
                L"        <Style x:Key='myStyle' TargetType='TextBox'>"
                L"            <Setter Value='32'/>"
                L"        </Style>"
                L"    </Grid.Resources>"
                L"</Grid>";

            VERIFY_THROWS_WINRT(XamlReader::Load(xamlString), Platform::Exception^, L"XAML parse exception should be thrown when a style setter's property is missing");
        });
    }

    void StyleIntegrationTests::CanSetStyleDirectlyInsideElement()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            Platform::String^ xamlString =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                L"    <Canvas.Style>"
                L"        <Style TargetType='Canvas'>"
                L"            <Style.Setters>"
                L"                <Setter Property='Width' Value='32'/>"
                L"            </Style.Setters>"
                L"        </Style>"
                L"    </Canvas.Style>"
                L"</Canvas>";

            auto canvas = safe_cast<Canvas^>(XamlReader::Load(xamlString));
            VERIFY_ARE_EQUAL(32, canvas->Width, L"Canvas.Width property should be set by the style");
        });
    }

    void StyleIntegrationTests::CanSetStyleDirectlyInsideElement_TargetPropertyPath()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            Platform::String^ xamlString =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                L"    <Canvas.Style>"
                L"        <Style TargetType='Canvas'>"
                L"            <Style.Setters>"
                L"                <Setter Target='Width' Value='32'/>"
                L"            </Style.Setters>"
                L"        </Style>"
                L"    </Canvas.Style>"
                L"</Canvas>";

            auto canvas = safe_cast<Canvas^>(XamlReader::Load(xamlString));
            VERIFY_ARE_EQUAL(32, canvas->Width, L"Canvas.Width property should be set by the style");
        });
    }

    void StyleIntegrationTests::CanSetAttachedDP()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            Platform::String^ xamlString =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"        xmlns:local='using:Tests.Native.External.Framework'>"
                L"    <Canvas.Resources>"
                L"        <Style x:Key='myStyle' TargetType='local:CustomControl'>"
                L"            <Setter Property='local:ControlWithAttachedProperty.CustomAttached' Value='Attached String'/>"
                L"            <Setter Property='Canvas.Left' Value='123'/>"
                L"        </Style>"
                L"    </Canvas.Resources>"
                L"    <local:CustomControl Style='{StaticResource myStyle}'/>"
                L"</Canvas>";

            auto canvas = safe_cast<Canvas^>(XamlReader::Load(xamlString));
            auto targetControl = safe_cast<CustomControl^>(canvas->Children->GetAt(0));

            VERIFY_IS_TRUE(L"Attached String" == ControlWithAttachedProperty::GetCustomAttached(targetControl), L"ControlWithAttachedProperty.CustomAttached property should be set by the style");
            VERIFY_ARE_EQUAL(123, Canvas::GetLeft(targetControl), L"Canvas.Left attached property should be set by the style");

        });
    }

    void StyleIntegrationTests::CanSetAttachedDP_TargetPropertyPath()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            Platform::String^ xamlString =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"        xmlns:local='using:Tests.Native.External.Framework'>"
                L"    <Canvas.Resources>"
                L"        <Style x:Key='myStyle' TargetType='local:CustomControl'>"
                L"            <Setter Target='(local:ControlWithAttachedProperty.CustomAttached)' Value='Attached String'/>"
                L"            <Setter Target='(Canvas.Left)' Value='123'/>"
                L"        </Style>"
                L"    </Canvas.Resources>"
                L"    <local:CustomControl Style='{StaticResource myStyle}'/>"
                L"</Canvas>";

            auto canvas = safe_cast<Canvas^>(XamlReader::Load(xamlString));
            auto targetControl = safe_cast<CustomControl^>(canvas->Children->GetAt(0));

            VERIFY_IS_TRUE(L"Attached String" == ControlWithAttachedProperty::GetCustomAttached(targetControl), L"ControlWithAttachedProperty.CustomAttached property should be set by the style");
            VERIFY_ARE_EQUAL(123, Canvas::GetLeft(targetControl), L"Canvas.Left attached property should be set by the style");

        });
    }

    void StyleIntegrationTests::CanSetStyleOnElementWithExistingAttribute()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            Platform::String^ xamlString =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                L"    <Button Content='Abc'>"
                L"        <Button.Style>"
                L"            <Style TargetType='Button'>"
                L"                <Setter Property='Width' Value='128'/>"
                L"            </Style>"
                L"        </Button.Style>"
                L"    </Button>"
                L"</Canvas>";

            auto canvas = safe_cast<Canvas^>(XamlReader::Load(xamlString));
            auto button = safe_cast<Button^>(canvas->Children->GetAt(0));

            VERIFY_IS_TRUE(L"Abc" == button->Content->ToString(), L"Button.Content property was not set");
            VERIFY_ARE_EQUAL(128, button->Width, L"Button.Width property should be set by the style");

        });
    }

    void StyleIntegrationTests::CanStyleSetterDetectNonDP()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([]()
        {
            Platform::String^ xamlString =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"        xmlns:local='using:Tests.Native.External.Framework'>"
                L"    <Canvas.Resources>"
                L"        <Style x:Key='myStyle' TargetType='local:CustomControl'>"
                L"            <Setter Property='NotDP' Value='32.0'/>"
                L"        </Style>"
                L"    </Canvas.Resources>"
                L"    <local:CustomControl Style='{StaticResource myStyle}'/>"
                L"</Canvas>";

            VERIFY_THROWS_WINRT(XamlReader::Load(xamlString), Platform::COMException^, L"Setting a non-DP in style setter should throw a parse error");
        });
    }

    void StyleIntegrationTests::CanStyleSetterDetectNonexistentProperty()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([]()
        {
            Platform::String^ xamlString =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"        xmlns:local='using:Tests.Native.External.Framework'>"
                L"    <Canvas.Resources>"
                L"        <Style x:Key='myStyle' TargetType='local:CustomControl'>"
                L"            <Setter Property='NonexistentProperty' Value='32.0'/>"
                L"        </Style>"
                L"    </Canvas.Resources>"
                L"    <local:CustomControl Style='{StaticResource myStyle}'/>"
                L"</Canvas>";

            VERIFY_THROWS_WINRT(XamlReader::Load(xamlString), Platform::COMException^, L"Setting a nonexistent property in style setter should throw a parse error");
        });
    }

    void StyleIntegrationTests::CanSetStyleWithTargetTypeAndEmptyControlTemplate()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            Platform::String^ xamlString =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Canvas.Resources>"
                L"        <Style x:Key='myStyle' TargetType='Button'>"
                L"            <Setter Property='Template'>"
                L"                <Setter.Value>"
                L"                    <ControlTemplate TargetType='Button'/>"
                L"                </Setter.Value>"
                L"            </Setter>"
                L"        </Style>"
                L"    </Canvas.Resources>"
                L"    <Button x:Name='button' Style='{StaticResource myStyle}'/>"
                L"</Canvas>";

            auto canvas = safe_cast<Canvas^>(XamlReader::Load(xamlString));
            VERIFY_IS_NOT_NULL(canvas);

            auto button = safe_cast<Button^>(canvas->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button, L"Should be able to find the button");
            VERIFY_IS_NOT_NULL(button->Template, L"Template should be applied to button");
        });
    }

    void StyleIntegrationTests::CanSetStyleBasedOnAnotherStyle()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            Platform::String^ xamlString =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Canvas.Resources>"
                L"        <Style x:Key='S1' TargetType='Button'>"
                L"            <Setter Property='Background' Value='Blue' />"
                L"        </Style>"
                L"        <Style x:Key='S2' TargetType='Button' BasedOn='{StaticResource S1}'>"
                L"            <Setter Property='Foreground' Value='Pink' />"
                L"            <Setter Property='Margin' Value='22' />"
                L"        </Style>"
                L"    </Canvas.Resources>"
                L"    <StackPanel>"
                L"        <Button x:Name='baseline' Height='80' Width='250' Content='Baseline'/>"
                L"        <Button x:Name='button1' Height='80' Width='250' Content='S1' Style='{StaticResource S1}'/>"
                L"        <Button x:Name='button2' Height='80' Width='250' Content='S2' Style='{StaticResource S2}'/>"
                L"    </StackPanel>"
                L"</Canvas>";

            auto canvas = safe_cast<Canvas^>(XamlReader::Load(xamlString));
            auto button1 = safe_cast<Button^>(canvas->FindName(L"button1"));
            auto button2 = safe_cast<Button^>(canvas->FindName(L"button2"));

            auto button1BgBrush = safe_cast<SolidColorBrush^>(button1->Background);
            auto button2BgBrush = safe_cast<SolidColorBrush^>(button2->Background);

            VERIFY_ARE_EQUAL(Colors::Blue, button1BgBrush->Color, L"Button1.Background should be Blue");
            VERIFY_ARE_EQUAL(Colors::Blue, button2BgBrush->Color, L"Button2.Background should also be Blue");
            VERIFY_ARE_EQUAL(button2->Style->BasedOn, button1->Style, L"Button2 should have a Style BasedOn the Style for Button1");
        });
    }

    void StyleIntegrationTests::CanCreateOptimizedStyle()
    {
        TestCleanupWrapper cleanup;

        TestCanCreateOptimizedStyle(false);
    }

    void StyleIntegrationTests::CanCreateOptimizedStyle_TargetPropertyPath()
    {
        TestCleanupWrapper cleanup;

        TestCanCreateOptimizedStyle(true);
    }

    void StyleIntegrationTests::TestCanCreateOptimizedStyle(bool useSetterTarget)
    {
        RunOnUIThread([useSetterTarget]()
        {
            Platform::String^ xamlString;

            if (useSetterTarget)
            {
                xamlString =
                    L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Canvas.Resources>"
                    L"        <Style x:Key='S1' TargetType='Button'>"
                    L"            <Setter Target='Background' Value='Blue' />"
                    L"        </Style>"
                    L"    </Canvas.Resources>"
                    L"    <StackPanel>"
                    L"        <Button x:Name='button1' Height='80' Width='250' Content='S1' Style='{StaticResource S1}'/>"
                    L"    </StackPanel>"
                    L"</Canvas>";
            }
            else
            {
                xamlString =
                    L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Canvas.Resources>"
                    L"        <Style x:Key='S1' TargetType='Button'>"
                    L"            <Setter Property='Background' Value='Blue' />"
                    L"        </Style>"
                    L"    </Canvas.Resources>"
                    L"    <StackPanel>"
                    L"        <Button x:Name='button1' Height='80' Width='250' Content='S1' Style='{StaticResource S1}'/>"
                    L"    </StackPanel>"
                    L"</Canvas>";
            }

            auto canvas = safe_cast<Canvas^>(XamlReader::Load(xamlString));
            auto button1 = safe_cast<Button^>(canvas->FindName(L"button1"));

            // Force seal the style to ensure style is optimized
            // -- necessary until style optimization is fully implemented.
            button1->Style->Seal();

            auto brush = safe_cast<SolidColorBrush^>(button1->Background);
            VERIFY_ARE_EQUAL(Colors::Blue, brush->Color, L"Button1.Background should be Blue");
        });

        // Verify order of multiple setters for same properties.
        RunOnUIThread([useSetterTarget]()
        {
            Platform::String^ xamlString;

            if (useSetterTarget)
            {
                xamlString =
                    L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Canvas.Resources>"
                    L"        <Style x:Key='S1' TargetType='Button'>"
                    L"            <Setter Target='Background' Value='Blue' />"
                    L"            <Setter Target='Background' Value='Red' />"
                    L"        </Style>"
                    L"    </Canvas.Resources>"
                    L"    <StackPanel>"
                    L"        <Button x:Name='button1' Height='80' Content='S1' Style='{StaticResource S1}'/>"
                    L"    </StackPanel>"
                    L"</Canvas>";
            }
            else
            {
                xamlString =
                    L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Canvas.Resources>"
                    L"        <Style x:Key='S1' TargetType='Button'>"
                    L"            <Setter Property='Background' Value='Blue' />"
                    L"            <Setter Property='Background' Value='Red' />"
                    L"        </Style>"
                    L"    </Canvas.Resources>"
                    L"    <StackPanel>"
                    L"        <Button x:Name='button1' Height='80' Content='S1' Style='{StaticResource S1}'/>"
                    L"    </StackPanel>"
                    L"</Canvas>";
            }

            auto canvas = safe_cast<Canvas^>(XamlReader::Load(xamlString));
            auto button1 = safe_cast<Button^>(canvas->FindName(L"button1"));

            // Force seal the style to ensure style is optimized
            // -- necessary until style optimization is fully implemented.
            button1->Style->Seal();

            auto brush = safe_cast<SolidColorBrush^>(button1->Background);
            VERIFY_ARE_EQUAL(brush->Color, Colors::Red, L"Button Background should be Red brush");
        });

        // Verify order of multiple setters of same property only from basedon setters.
        RunOnUIThread([useSetterTarget]()
        {
            Platform::String^ xamlString;

            if (useSetterTarget)
            {
                xamlString =
                    L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Canvas.Resources>"
                    L"        <Style x:Key='S1' TargetType='Button'>"
                    L"            <Setter Target='Background' Value='Blue' />"
                    L"            <Setter Target='Background' Value='Red' />"
                    L"        </Style>"
                    L"        <Style x:Key='S2' TargetType='Button' BasedOn='{StaticResource S1}'>"
                    L"        </Style>"
                    L"    </Canvas.Resources>"
                    L"    <StackPanel>"
                    L"        <Button x:Name='button1' Height='80' Content='S2' Style='{StaticResource S2}'/>"
                    L"    </StackPanel>"
                    L"</Canvas>";
            }
            else
            {
                xamlString =
                    L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Canvas.Resources>"
                    L"        <Style x:Key='S1' TargetType='Button'>"
                    L"            <Setter Property='Background' Value='Blue' />"
                    L"            <Setter Property='Background' Value='Red' />"
                    L"        </Style>"
                    L"        <Style x:Key='S2' TargetType='Button' BasedOn='{StaticResource S1}'>"
                    L"        </Style>"
                    L"    </Canvas.Resources>"
                    L"    <StackPanel>"
                    L"        <Button x:Name='button1' Height='80' Content='S2' Style='{StaticResource S2}'/>"
                    L"    </StackPanel>"
                    L"</Canvas>";
            }

            auto canvas = safe_cast<Canvas^>(XamlReader::Load(xamlString));
            auto button1 = safe_cast<Button^>(canvas->FindName(L"button1"));

            // Force seal the style to ensure style is optimized
            // -- necessary until style optimization is fully implemented.
            button1->Style->Seal();

            auto brush = safe_cast<SolidColorBrush^>(button1->Background);
            VERIFY_ARE_EQUAL(brush->Color, Colors::Red, L"Button Background should be Red brush");
        });

        // Verify order of multiple setters with basedon setters for same properties.
        RunOnUIThread([useSetterTarget]()
        {
            Platform::String^ xamlString;

            if (useSetterTarget)
            {
                xamlString =
                    L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Canvas.Resources>"
                    L"        <Style x:Key='S1' TargetType='Button'>"
                    L"            <Setter Target='Background' Value='Blue' />"
                    L"            <Setter Target='Background' Value='Red' />"
                    L"        </Style>"
                    L"        <Style x:Key='S2' TargetType='Button' BasedOn='{StaticResource S1}'>"
                    L"            <Setter Target='Background' Value='Pink' />"
                    L"            <Setter Target='Background' Value='Yellow' />"
                    L"        </Style>"
                    L"    </Canvas.Resources>"
                    L"    <StackPanel>"
                    L"        <Button x:Name='button1' Height='80' Content='S2' Style='{StaticResource S2}'/>"
                    L"    </StackPanel>"
                    L"</Canvas>";
            }
            else
            {
                xamlString =
                    L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Canvas.Resources>"
                    L"        <Style x:Key='S1' TargetType='Button'>"
                    L"            <Setter Property='Background' Value='Blue' />"
                    L"            <Setter Property='Background' Value='Red' />"
                    L"        </Style>"
                    L"        <Style x:Key='S2' TargetType='Button' BasedOn='{StaticResource S1}'>"
                    L"            <Setter Property='Background' Value='Pink' />"
                    L"            <Setter Property='Background' Value='Yellow' />"
                    L"        </Style>"
                    L"    </Canvas.Resources>"
                    L"    <StackPanel>"
                    L"        <Button x:Name='button1' Height='80' Content='S2' Style='{StaticResource S2}'/>"
                    L"    </StackPanel>"
                    L"</Canvas>";
            }

            auto canvas = safe_cast<Canvas^>(XamlReader::Load(xamlString));
            auto button1 = safe_cast<Button^>(canvas->FindName(L"button1"));

            // Force seal the style to ensure style is optimized
            // -- necessary until style optimization is fully implemented.
            button1->Style->Seal();

            auto brush = safe_cast<SolidColorBrush^>(button1->Background);
            VERIFY_ARE_EQUAL(brush->Color, Colors::Yellow, L"Button Background should be Yellow brush");
        });
    }

    void StyleIntegrationTests::CanFaultInOptimizedStyleSetters()
    {
        TestCleanupWrapper cleanup;

        // Verify order of multiple setters for different properties.
        RunOnUIThread([this]()
        {
            Platform::String^ xamlString =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Canvas.Resources>"
                L"        <Style x:Key='S1' TargetType='Button'>"
                L"            <Setter Property='Background' Value='Blue' />"
                L"            <Setter Property='Width' Value='100' />"
                L"        </Style>"
                L"    </Canvas.Resources>"
                L"    <StackPanel>"
                L"        <Button x:Name='button1' Height='80' Content='S1' Style='{StaticResource S1}'/>"
                L"    </StackPanel>"
                L"</Canvas>";

            auto canvas = safe_cast<Canvas^>(XamlReader::Load(xamlString));
            auto button1 = safe_cast<Button^>(canvas->FindName(L"button1"));

            // Force seal the style to ensure style is optimized
            // -- necessary until style optimization is fully implemented.
            button1->Style->Seal();

            auto setters = button1->Style->Setters;
            VERIFY_ARE_EQUAL(setters->Size, 2U, L"Setters count should be 2");

            // Background Blue
            auto setter = safe_cast<Setter^>(setters->GetAt(0));
            VERIFY_ARE_EQUAL(setter->Property, Control::BackgroundProperty, L"Setter property should be Control::BackgroundProperty");
            auto brush = safe_cast<SolidColorBrush^>(setter->Value);
            VERIFY_ARE_EQUAL(Colors::Blue, brush->Color, L"Setter value should be Blue brush");

            // Width 100
            setter = safe_cast<Setter^>(setters->GetAt(1));
            VERIFY_ARE_EQUAL(setter->Property, Control::WidthProperty, L"Setter property should be Control::WidthProperty");
            auto width = safe_cast<double>(setter->Value);
            VERIFY_ARE_EQUAL(width, 100, L"Setter value should be 100");

            // Get actual property values on styled element
            brush = safe_cast<SolidColorBrush^>(button1->Background);
            VERIFY_ARE_EQUAL(brush->Color, Colors::Blue, L"Button Background should be Blue brush");
            VERIFY_ARE_EQUAL(button1->Width, 100.0, L"Button Width should be 100");
        });

        // Verify order of multiple setters for same properties.
        RunOnUIThread([this]()
        {
            Platform::String^ xamlString =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Canvas.Resources>"
                L"        <Style x:Key='S1' TargetType='Button'>"
                L"            <Setter Property='Background' Value='Blue' />"
                L"            <Setter Property='Width' Value='100' />"
                L"            <Setter Property='Background' Value='Red' />"
                L"        </Style>"
                L"    </Canvas.Resources>"
                L"    <StackPanel>"
                L"        <Button x:Name='button1' Height='80' Content='S1' Style='{StaticResource S1}'/>"
                L"    </StackPanel>"
                L"</Canvas>";

            auto canvas = safe_cast<Canvas^>(XamlReader::Load(xamlString));
            auto button1 = safe_cast<Button^>(canvas->FindName(L"button1"));

            // Force seal the style to ensure style is optimized
            // -- necessary until style optimization is fully implemented.
            button1->Style->Seal();

            auto setters = button1->Style->Setters;
            VERIFY_ARE_EQUAL(setters->Size, 3U, L"Setters count should be 3");

            // Background Blue
            auto setter = safe_cast<Setter^>(setters->GetAt(0));
            VERIFY_ARE_EQUAL(setter->Property, Control::BackgroundProperty, L"Setter property should be Control::BackgroundProperty");
            auto brush = safe_cast<SolidColorBrush^>(setter->Value);
            VERIFY_ARE_EQUAL(Colors::Blue, brush->Color, L"Setter value should be Blue brush");

            // Width 100
            setter = safe_cast<Setter^>(setters->GetAt(1));
            VERIFY_ARE_EQUAL(setter->Property, Control::WidthProperty, L"Setter property should be Control::WidthProperty");
            auto width = safe_cast<double>(setter->Value);
            VERIFY_ARE_EQUAL(width, 100, L"Setter value should be 100");

            // Background Red
            setter = safe_cast<Setter^>(setters->GetAt(2));
            VERIFY_ARE_EQUAL(setter->Property, Control::BackgroundProperty, L"Setter property should be Control::BackgroundProperty");
            brush = safe_cast<SolidColorBrush^>(setter->Value);
            VERIFY_ARE_EQUAL(Colors::Red, brush->Color, L"Setter value should be Red brush");

            // Get actual property values on styled element
            brush = safe_cast<SolidColorBrush^>(button1->Background);
            VERIFY_ARE_EQUAL(brush->Color, Colors::Red, L"Button Background should be Red brush");
            VERIFY_ARE_EQUAL(button1->Width, 100, L"Button Width should be 100");
        });

        // Verify order of multiple setters of same property only from basedon setters.
        RunOnUIThread([]()
        {
            Platform::String^ xamlString =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Canvas.Resources>"
                L"        <Style x:Key='S1' TargetType='Button'>"
                L"            <Setter Property='Background' Value='Blue' />"
                L"            <Setter Property='Background' Value='Red' />"
                L"        </Style>"
                L"        <Style x:Key='S2' TargetType='Button' BasedOn='{StaticResource S1}'>"
                L"        </Style>"
                L"    </Canvas.Resources>"
                L"    <StackPanel>"
                L"        <Button x:Name='button1' Height='80' Content='S2' Style='{StaticResource S2}'/>"
                L"    </StackPanel>"
                L"</Canvas>";

            auto canvas = safe_cast<Canvas^>(XamlReader::Load(xamlString));
            auto button1 = safe_cast<Button^>(canvas->FindName(L"button1"));

            // Force seal the style to ensure style is optimized
            // -- necessary until style optimization is fully implemented.
            button1->Style->Seal();

            VERIFY_IS_NOT_NULL(button1->Style->BasedOn, L"Style BasedOn property should not be null");

            auto setters = button1->Style->Setters;
            VERIFY_ARE_EQUAL(setters->Size, 0U, L"Setters count should be 0");

            setters = button1->Style->BasedOn->Setters;
            VERIFY_ARE_EQUAL(setters->Size, 2U, L"BasedOn Setters count should be 2");

            // BasedOn Background Blue
            auto setter = safe_cast<Setter^>(setters->GetAt(0));
            VERIFY_ARE_EQUAL(setter->Property, Control::BackgroundProperty, L"Setter property should be Control::BackgroundProperty");
            auto brush = safe_cast<SolidColorBrush^>(setter->Value);
            VERIFY_ARE_EQUAL(Colors::Blue, brush->Color, L"Setter value should be Blue brush");

            // BasedOn Background Red
            setter = safe_cast<Setter^>(setters->GetAt(1));
            VERIFY_ARE_EQUAL(setter->Property, Control::BackgroundProperty, L"Setter property should be Control::BackgroundProperty");
            brush = safe_cast<SolidColorBrush^>(setter->Value);
            VERIFY_ARE_EQUAL(Colors::Red, brush->Color, L"Setter value should be Red brush");
        });

        // Verify order of multiple setters with basedon setters for same properties.
        RunOnUIThread([this]()
        {
            Platform::String^ xamlString =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Canvas.Resources>"
                L"        <Style x:Key='S1' TargetType='Button'>"
                L"            <Setter Property='Background' Value='Blue' />"
                L"            <Setter Property='Background' Value='Red' />"
                L"        </Style>"
                L"        <Style x:Key='S2' TargetType='Button' BasedOn='{StaticResource S1}'>"
                L"            <Setter Property='Background' Value='Pink' />"
                L"            <Setter Property='Background' Value='Yellow' />"
                L"        </Style>"
                L"    </Canvas.Resources>"
                L"    <StackPanel>"
                L"        <Button x:Name='button1' Height='80' Content='S2' Style='{StaticResource S2}'/>"
                L"    </StackPanel>"
                L"</Canvas>";

            auto canvas = safe_cast<Canvas^>(XamlReader::Load(xamlString));
            auto button1 = safe_cast<Button^>(canvas->FindName(L"button1"));

            // Force seal the style to ensure style is optimized
            // -- necessary until style optimization is fully implemented.
            button1->Style->Seal();

            VERIFY_IS_NOT_NULL(button1->Style->BasedOn, L"Style BasedOn property should not be null");

            // BasedOn Setters
            auto setters = button1->Style->BasedOn->Setters;
            VERIFY_ARE_EQUAL(setters->Size, 2U, L"BasedOn Setters count should be 2");

            // BasedOn Background Blue
            auto setter = safe_cast<Setter^>(setters->GetAt(0));
            VERIFY_ARE_EQUAL(setter->Property, Control::BackgroundProperty, L"Setter property should be Control::BackgroundProperty");
            auto brush = safe_cast<SolidColorBrush^>(setter->Value);
            VERIFY_ARE_EQUAL(Colors::Blue, brush->Color, L"Setter value should be Blue brush");

            // BasedOn Background Red
            setter = safe_cast<Setter^>(setters->GetAt(1));
            VERIFY_ARE_EQUAL(setter->Property, Control::BackgroundProperty, L"Setter property should be Control::BackgroundProperty");
            brush = safe_cast<SolidColorBrush^>(setter->Value);
            VERIFY_ARE_EQUAL(Colors::Red, brush->Color, L"Setter value should be Red brush");

            // Non-BasedOn Setters
            setters = button1->Style->Setters;
            VERIFY_ARE_EQUAL(setters->Size, 2U, L"Setters count should be 2");

            // Background Pink
            setter = safe_cast<Setter^>(setters->GetAt(0));
            VERIFY_ARE_EQUAL(setter->Property, Control::BackgroundProperty, L"Setter property should be Control::BackgroundProperty");
            brush = safe_cast<SolidColorBrush^>(setter->Value);
            VERIFY_ARE_EQUAL(Colors::Pink, brush->Color, L"Setter value should be Pink brush");

            // Background Yellow
            setter = safe_cast<Setter^>(setters->GetAt(1));
            VERIFY_ARE_EQUAL(setter->Property, Control::BackgroundProperty, L"Setter property should be Control::BackgroundProperty");
            brush = safe_cast<SolidColorBrush^>(setter->Value);
            VERIFY_ARE_EQUAL(Colors::Yellow, brush->Color, L"Setter value should be Yellow brush");

            // Get actual property values on styled element
            brush = safe_cast<SolidColorBrush^>(button1->Background);
            VERIFY_ARE_EQUAL(brush->Color, Colors::Yellow, L"Button Background should be Yellow brush");
        });
    }

    void StyleIntegrationTests::CanUseRegularStyleWithOptimizedBasedOnStyle()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([this]()
        {
            Style^ basedOn = nullptr;

            // Create optimized style.
            {
                RuntimeEnabledFeatureOverride featureStyleOpt(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableStyleOptimization, true);

                basedOn = ref new Style();
                basedOn->TargetType = Button::typeid;
                auto setter = ref new Setter();
                setter->Property = Control::WidthProperty;
                setter->Value = "10";
                basedOn->Setters->Append(setter);
                setter = ref new Setter();
                setter->Property = Control::HeightProperty;
                setter->Value = "10";
                basedOn->Setters->Append(setter);
                basedOn->Seal(); // Force seal to convert to optimized style
            }

            // Create non-optimized style.
            Style^ style = ref new Style();
            style->TargetType = Button::typeid;
            auto setter = ref new Setter();
            setter->Property = Control::HeightProperty;
            setter->Value = "20";
            style->Setters->Append(setter);

            // Set non-optimized style's BasedOn property to the optimized style.
            style->BasedOn = basedOn;

            // Seal the non-optimized style.
            style->Seal();

            VERIFY_IS_NOT_NULL(style->BasedOn, L"Style BasedOn property should not be null");

            Button^ button = ref new Button();
            button->Style = style;
            m_rootCanvas->Children->Clear();
            m_rootCanvas->Children->Append(button);
            m_rootCanvas->UpdateLayout();

            // Verify effective values
            VERIFY_ARE_EQUAL(10.0, safe_cast<double>(button->GetValue(Control::WidthProperty)), L"Button width should be 10");
            VERIFY_ARE_EQUAL(20.0, safe_cast<double>(button->GetValue(Control::HeightProperty)), L"Button height should be 20");

            // BasedOn Setters
            auto setters = style->BasedOn->Setters;
            VERIFY_ARE_EQUAL(setters->Size, 2U, L"BasedOn Setters count should be 2");

            setter = safe_cast<Setter^>(setters->GetAt(0));
            VERIFY_ARE_EQUAL(setter->Property, Control::WidthProperty, L"Setter property should be Control::WidthProperty");
            VERIFY_ARE_EQUAL(10.0, safe_cast<double>(setter->Value), L"Setter value should be 10");

            setter = safe_cast<Setter^>(setters->GetAt(1));
            VERIFY_ARE_EQUAL(setter->Property, Control::HeightProperty, L"Setter property should be Control::HeightProperty");
            VERIFY_ARE_EQUAL(10.0, safe_cast<double>(setter->Value), L"Setter value should be 10");

            // Non-BasedOn Setters
            setters = style->Setters;
            VERIFY_ARE_EQUAL(setters->Size, 1U, L"Setters count should be 1");

            setter = safe_cast<Setter^>(setters->GetAt(0));
            VERIFY_ARE_EQUAL(setter->Property, Control::HeightProperty, L"Setter property should be Control::HeightProperty");
            VERIFY_ARE_EQUAL(20.0, safe_cast<double>(setter->Value), L"Setter value should be 20");
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void StyleIntegrationTests::CanUseOptimizedStyleWithRegularBasedOnStyle()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([this]()
        {
            Style^ style = nullptr;

            // Create non-optimized style.
            Style^ basedOn = ref new Style();
            basedOn->TargetType = Button::typeid;
            auto setter = ref new Setter();
            setter->Property = Control::WidthProperty;
            setter->Value = "10";
            basedOn->Setters->Append(setter);
            setter = ref new Setter();
            setter->Property = Control::HeightProperty;
            setter->Value = "10";
            basedOn->Setters->Append(setter);
            basedOn->Seal(); // Force seal before enabling style optimization below

            // Create optimized style.
            {
                RuntimeEnabledFeatureOverride featureStyleOpt(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableStyleOptimization, true);

                style = ref new Style();
                style->TargetType = Button::typeid;
                setter = ref new Setter();
                setter->Property = Control::HeightProperty;
                setter->Value = "20";
                style->Setters->Append(setter);

                // Set optimized style's BasedOn property to the non-optimized style.
                style->BasedOn = basedOn;

                // Seal the optimized style.
                // Converts to optimized style.
                style->Seal();
            }

            VERIFY_IS_NOT_NULL(style->BasedOn, L"Style BasedOn property should not be null");

            Button^ button = ref new Button();
            button->Style = style;
            m_rootCanvas->Children->Clear();
            m_rootCanvas->Children->Append(button);
            m_rootCanvas->UpdateLayout();

            // Verify effective values
            VERIFY_ARE_EQUAL(10.0, safe_cast<double>(button->GetValue(Control::WidthProperty)), L"Button width should be 10");
            VERIFY_ARE_EQUAL(20.0, safe_cast<double>(button->GetValue(Control::HeightProperty)), L"Button height should be 20");

            // BasedOn Setters
            auto setters = style->BasedOn->Setters;
            VERIFY_ARE_EQUAL(setters->Size, 2U, L"BasedOn Setters count should be 2");

            setter = safe_cast<Setter^>(setters->GetAt(0));
            VERIFY_ARE_EQUAL(setter->Property, Control::WidthProperty, L"Setter property should be Control::WidthProperty");
            VERIFY_ARE_EQUAL(10.0, safe_cast<double>(setter->Value), L"Setter value should be 10");

            setter = safe_cast<Setter^>(setters->GetAt(1));
            VERIFY_ARE_EQUAL(setter->Property, Control::HeightProperty, L"Setter property should be Control::HeightProperty");
            VERIFY_ARE_EQUAL(10.0, safe_cast<double>(setter->Value), L"Setter value should be 10");

            // Non-BasedOn Setters
            setters = style->Setters;
            VERIFY_ARE_EQUAL(setters->Size, 1U, L"Setters count should be 1");

            setter = safe_cast<Setter^>(setters->GetAt(0));
            VERIFY_ARE_EQUAL(setter->Property, Control::HeightProperty, L"Setter property should be Control::HeightProperty");
            VERIFY_ARE_EQUAL(20.0, safe_cast<double>(setter->Value), L"Setter value should be 20");
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void StyleIntegrationTests::CanUseOptimizedStyleWithRegularNestedBasedOnStyles()
    {
        TestCleanupWrapper cleanup;
        Panel^ panel = nullptr;
        TextBlock^ textBlock = nullptr;

        RunOnUIThread([this]()
        {
            Style^ style = nullptr;

            // Create "nested" non-optimized style.
            Style^ nestedBasedOn = ref new Style();
            nestedBasedOn->TargetType = Button::typeid;
            auto setter = ref new Setter();
            setter->Property = Control::WidthProperty;
            setter->Value = "10";
            nestedBasedOn->Setters->Append(setter);
            setter = ref new Setter();
            setter->Property = Control::HeightProperty;
            setter->Value = "10";
            nestedBasedOn->Setters->Append(setter);

            // Create non-optimized style based on the first non-optimized style.
            Style^ basedOn = ref new Style();
            basedOn->TargetType = Button::typeid;
            setter = ref new Setter();
            setter->Property = Control::MaxWidthProperty;
            setter->Value = "10";
            basedOn->Setters->Append(setter);
            basedOn->BasedOn = nestedBasedOn;
            basedOn->Seal(); // Force seal before enabling style optimization below

            // Create optimized style.
            {
                RuntimeEnabledFeatureOverride featureStyleOpt(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableStyleOptimization, true);

                style = ref new Style();
                style->TargetType = Button::typeid;
                setter = ref new Setter();
                setter->Property = Control::HeightProperty;
                setter->Value = "20";
                style->Setters->Append(setter);

                // Set optimized style's BasedOn property to the top non-optimized style.
                style->BasedOn = basedOn;

                // Seal the optimized style.
                // Converts to optimized style.
                style->Seal();
            }

            VERIFY_IS_NOT_NULL(style->BasedOn, L"Style's BasedOn property should not be null");
            VERIFY_IS_NOT_NULL(style->BasedOn->BasedOn, L"BasedOn style's BasedOn property should not be null");

            Button^ button = ref new Button();
            button->Style = style;
            m_rootCanvas->Children->Clear();
            m_rootCanvas->Children->Append(button);
            m_rootCanvas->UpdateLayout();

            // Verify effective values
            VERIFY_ARE_EQUAL(10.0, safe_cast<double>(button->GetValue(Control::WidthProperty)), L"Button width should be 10");
            VERIFY_ARE_EQUAL(20.0, safe_cast<double>(button->GetValue(Control::HeightProperty)), L"Button height should be 20");
            VERIFY_ARE_EQUAL(10.0, safe_cast<double>(button->GetValue(Control::MaxWidthProperty)), L"Button MaxWidth should be 10");

            // Nested BasedOn Setters
            auto setters = style->BasedOn->BasedOn->Setters;
            VERIFY_ARE_EQUAL(setters->Size, 2U, L"Nested BasedOn Setters count should be 2");

            setter = safe_cast<Setter^>(setters->GetAt(0));
            VERIFY_ARE_EQUAL(setter->Property, Control::WidthProperty, L"Setter property should be Control::WidthProperty");
            VERIFY_ARE_EQUAL(10.0, safe_cast<double>(setter->Value), L"Setter value should be 10");

            setter = safe_cast<Setter^>(setters->GetAt(1));
            VERIFY_ARE_EQUAL(setter->Property, Control::HeightProperty, L"Setter property should be Control::HeightProperty");
            VERIFY_ARE_EQUAL(10.0, safe_cast<double>(setter->Value), L"Setter value should be 10");

            // "Parent" BasedOn Setters
            setters = style->BasedOn->Setters;
            VERIFY_ARE_EQUAL(setters->Size, 1U, L"Top BasedOn Setters count should be 1");

            setter = safe_cast<Setter^>(setters->GetAt(0));
            VERIFY_ARE_EQUAL(setter->Property, Control::MaxWidthProperty, L"Setter property should be Control::MaxWidthProperty");
            VERIFY_ARE_EQUAL(10.0, safe_cast<double>(setter->Value), L"Setter value should be 10");

            // Non-BasedOn Setters
            setters = style->Setters;
            VERIFY_ARE_EQUAL(setters->Size, 1U, L"Non-BasedOn Setters count should be 1");

            setter = safe_cast<Setter^>(setters->GetAt(0));
            VERIFY_ARE_EQUAL(setter->Property, Control::HeightProperty, L"Setter property should be Control::HeightProperty");
            VERIFY_ARE_EQUAL(20.0, safe_cast<double>(setter->Value), L"Setter value should be 20");
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void StyleIntegrationTests::CanUseRegularStyleWithRegularBasedOnStyle()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([this]()
        {
            Style^ style = nullptr;

            // Create non-optimized style.
            Style^ basedOn = ref new Style();
            basedOn->TargetType = Button::typeid;
            auto setter = ref new Setter();
            setter->Property = Control::WidthProperty;
            setter->Value = "10";
            basedOn->Setters->Append(setter);
            setter = ref new Setter();
            setter->Property = Control::HeightProperty;
            setter->Value = "10";
            basedOn->Setters->Append(setter);
            basedOn->Seal();

            // Create non-optimized style.
            style = ref new Style();
            style->TargetType = Button::typeid;
            setter = ref new Setter();
            setter->Property = Control::HeightProperty;
            setter->Value = "20";
            style->Setters->Append(setter);

            // Set style's BasedOn property.
            style->BasedOn = basedOn;

            // Seal the style.
            style->Seal();

            VERIFY_IS_NOT_NULL(style->BasedOn, L"Style BasedOn property should not be null");

            Button^ button = ref new Button();
            button->Style = style;
            m_rootCanvas->Children->Clear();
            m_rootCanvas->Children->Append(button);
            m_rootCanvas->UpdateLayout();

            // Verify effective values
            VERIFY_ARE_EQUAL(10.0, safe_cast<double>(button->GetValue(Control::WidthProperty)), L"Button width should be 10");
            VERIFY_ARE_EQUAL(20.0, safe_cast<double>(button->GetValue(Control::HeightProperty)), L"Button height should be 20");

            // BasedOn Setters
            auto setters = style->BasedOn->Setters;
            VERIFY_ARE_EQUAL(setters->Size, 2U, L"BasedOn Setters count should be 2");

            setter = safe_cast<Setter^>(setters->GetAt(0));
            VERIFY_ARE_EQUAL(setter->Property, Control::WidthProperty, L"Setter property should be Control::WidthProperty");
            VERIFY_ARE_EQUAL(10.0, safe_cast<double>(setter->Value), L"Setter value should be 10");

            setter = safe_cast<Setter^>(setters->GetAt(1));
            VERIFY_ARE_EQUAL(setter->Property, Control::HeightProperty, L"Setter property should be Control::HeightProperty");
            VERIFY_ARE_EQUAL(10.0, safe_cast<double>(setter->Value), L"Setter value should be 10");

            // Non-BasedOn Setters
            setters = style->Setters;
            VERIFY_ARE_EQUAL(setters->Size, 1U, L"Setters count should be 1");

            setter = safe_cast<Setter^>(setters->GetAt(0));
            VERIFY_ARE_EQUAL(setter->Property, Control::HeightProperty, L"Setter property should be Control::HeightProperty");
            VERIFY_ARE_EQUAL(20.0, safe_cast<double>(setter->Value), L"Setter value should be 20");
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void StyleIntegrationTests::CanSetStyleWithStringValueOnComplexTypeWithoutTypeConverter()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([]()
        {
            auto dp = DependencyProperty::RegisterAttached(L"CanSetStyleWithStringValueOnComplexTypeWithoutTypeConverter_DP", float::typeid, Border::typeid, nullptr);

            auto border = ref new Border();

            auto setter = ref new Setter();
            setter->Property = dp;
            setter->Value = L"42";

            auto style = ref new Style();
            style->TargetType = Border::typeid;
            style->Setters->Append(setter);

            border->Style = style;

            // At the moment there's no type converter for floats, so we don't expect the style to have assigned anything
            // to our DP. If we ever add a type converter for floats, this test will start failing. Update the test to
            // verify this DP now has the value 42.
            VERIFY_ARE_EQUAL(0.0f, safe_cast<float>(border->GetValue(dp)));
        });
    }

    void StyleIntegrationTests::CanSetStyleWithStringValueOnCustomFontFamilyProperty()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            auto dp = DependencyProperty::RegisterAttached(L"CanSetStyleWithStringValueOnCustomFontFamilyProperty_DP", FontFamily::typeid, Border::typeid, nullptr);

            auto border = ref new Border();

            auto setter = ref new Setter();
            setter->Property = dp;
            setter->Value = L"Helvetica";

            auto style = ref new Style();
            style->TargetType = Border::typeid;
            style->Setters->Append(setter);

            border->Style = style;

            // At the moment there's no type converter for floats, so we don't expect the style to have assigned anything
            // to our DP. If we ever add a type converter for floats, this test will start failing. Update the test to
            // verify this DP now has the value 42.
            VERIFY_ARE_STRINGS_EQUAL(L"Helvetica", static_cast<FontFamily^>(border->GetValue(dp))->Source->Data());
        });
    }

    void StyleIntegrationTests::CanUseSingleStyleFromParentInlineResources()
    {
        TestCleanupWrapper cleanup;

        auto panel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetVariationFileName(L"OneStyleParentInlineResources")));

        RunOnUIThread([&]()
        {
            auto button = safe_cast<Button^>(panel->Children->GetAt(0));
            auto brush = safe_cast<SolidColorBrush^>(button->Background);
            VERIFY_ARE_EQUAL(Colors::Blue, brush->Color, L"Button.Background should be Blue");
        });
    }

    void StyleIntegrationTests::CanUseTwoStylesFromParentInlineResources()
    {
        TestCleanupWrapper cleanup;

        auto panel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetVariationFileName(L"TwoStylesParentInlineResources")));

        RunOnUIThread([&]()
        {
            auto button = safe_cast<Button^>(panel->Children->GetAt(0));
            auto brush = safe_cast<SolidColorBrush^>(button->Background);
            VERIFY_ARE_EQUAL(Colors::Blue, brush->Color, L"Button.Background should be Blue");

            button = safe_cast<Button^>(panel->Children->GetAt(1));
            brush = safe_cast<SolidColorBrush^>(button->Background);
            VERIFY_ARE_EQUAL(Colors::Red, brush->Color, L"Button.Background should be Red");
        });
    }

    void StyleIntegrationTests::CanUseStyleNestedInStyle()
    {
        TestCleanupWrapper cleanup;

        auto panel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetVariationFileName(L"StyleNestedInStyle")));

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = m_rootCanvas;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            m_rootCanvas->Children->Clear();
            m_rootCanvas->Children->Append(panel);
            m_rootCanvas->UpdateLayout();

            auto listView = safe_cast<ListView^>(panel->Children->GetAt(0));
            auto item = safe_cast<ListViewItem^>(listView->Items->GetAt(0));

            auto brush = safe_cast<SolidColorBrush^>(item->Background);
            VERIFY_ARE_EQUAL(Colors::Blue, brush->Color, L"ListViewItem.Background should be Blue");

            listView = safe_cast<ListView^>(panel->Children->GetAt(1));
            item = safe_cast<ListViewItem^>(listView->Items->GetAt(1));

            brush = safe_cast<SolidColorBrush^>(item->Background);
            VERIFY_ARE_EQUAL(Colors::Red, brush->Color, L"ListViewItem.Background should be Red");

            m_rootCanvas->Children->Clear();
        });
    }

    void StyleIntegrationTests::CanUseStyleFromThemeResources()
    {
        TestCleanupWrapper cleanup;

        auto panel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetVariationFileName(L"StyleInThemeResources")));

        RunOnUIThread([&]()
        {
            auto button = safe_cast<Button^>(panel->Children->GetAt(0));
            auto brush = safe_cast<SolidColorBrush^>(button->Background);
            VERIFY_ARE_EQUAL(Colors::Blue, brush->Color, L"Button.Background should be Blue");

            button = safe_cast<Button^>(panel->Children->GetAt(1));
            brush = safe_cast<SolidColorBrush^>(button->Background);
            VERIFY_ARE_EQUAL(Colors::Red, brush->Color, L"Button.Background should be Red");
        });
    }

    void StyleIntegrationTests::CanUseStyleFromMergedResources()
    {
        TestCleanupWrapper cleanup;

        auto panel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetVariationFileName(L"StyleInMergedResources")));

        RunOnUIThread([&]()
        {
            auto button = safe_cast<Button^>(panel->Children->GetAt(0));
            auto brush = safe_cast<SolidColorBrush^>(button->Background);
            VERIFY_ARE_EQUAL(Colors::Blue, brush->Color, L"Button.Background should be Blue");

            button = safe_cast<Button^>(panel->Children->GetAt(1));
            brush = safe_cast<SolidColorBrush^>(button->Background);
            VERIFY_ARE_EQUAL(Colors::Red, brush->Color, L"Button.Background should be Red");

            button = safe_cast<Button^>(panel->Children->GetAt(2));
            brush = safe_cast<SolidColorBrush^>(button->Background);
            VERIFY_ARE_EQUAL(Colors::Purple, brush->Color, L"Button.Background should be Purple");
        });
    }

    void StyleIntegrationTests::CanUseStyleWithNestedTemplateVSM()
    {
        TestCleanupWrapper cleanup;
        TestCanUseStyleWithNestedTemplateVSM(false);
        TestCanUseStyleWithNestedTemplateVSM(true);
    }

    void StyleIntegrationTests::TestCanUseStyleWithNestedTemplateVSM(bool isExplicit)
    {
        auto panel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetVariationFileName(L"StyleWithNestedTemplateVSM", isExplicit)));

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = m_rootCanvas;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            m_rootCanvas->Children->Clear();
            m_rootCanvas->Children->Append(panel);
            m_rootCanvas->UpdateLayout();

            auto button = safe_cast<Button^>(panel->Children->GetAt(0));
            auto brush = safe_cast<SolidColorBrush^>(button->Background);
            VERIFY_ARE_EQUAL(Colors::Blue, brush->Color, L"Button.Background should be Blue");

            m_rootCanvas->Children->Clear();
        });
    }

    void StyleIntegrationTests::CanUseTwoStylesWithNestedTemplateVSM()
    {
        TestCleanupWrapper cleanup;

        // Implicit
        auto panel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetVariationFileName(L"TwoStylesWithNestedTemplateVSM", false)));

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = m_rootCanvas;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            m_rootCanvas->Children->Clear();
            m_rootCanvas->Children->Append(panel);
            m_rootCanvas->UpdateLayout();

            auto button = safe_cast<Button^>(panel->Children->GetAt(0));
            auto brush = safe_cast<SolidColorBrush^>(button->Background);
            VERIFY_ARE_EQUAL(Colors::Blue, brush->Color, L"Button.Background should be Blue");

            auto hyperlinkbutton = safe_cast<HyperlinkButton^>(panel->Children->GetAt(1));
            brush = safe_cast<SolidColorBrush^>(hyperlinkbutton->Background);
            VERIFY_ARE_EQUAL(Colors::Red, brush->Color, L"HyperlinkButton.Background should be Red");

            m_rootCanvas->Children->Clear();
        });

        // Explicit
        panel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetVariationFileName(L"TwoStylesWithNestedTemplateVSM", true)));

        RunOnUIThread([&]()
        {
            m_rootCanvas->Children->Clear();
            m_rootCanvas->Children->Append(panel);
            m_rootCanvas->UpdateLayout();

            auto button = safe_cast<Button^>(panel->Children->GetAt(0));
            auto brush = safe_cast<SolidColorBrush^>(button->Background);
            VERIFY_ARE_EQUAL(Colors::Blue, brush->Color, L"Button.Background should be Blue");

            button = safe_cast<Button^>(panel->Children->GetAt(1));
            brush = safe_cast<SolidColorBrush^>(button->Background);
            VERIFY_ARE_EQUAL(Colors::Red, brush->Color, L"Button.Background should be Red");

            m_rootCanvas->Children->Clear();
        });
    }

    void StyleIntegrationTests::CanUseStyleWithDeepNestedTemplateVSM()
    {
        TestCleanupWrapper cleanup;
        TestCanUseStyleWithDeepNestedTemplateVSM(false);
        TestCanUseStyleWithDeepNestedTemplateVSM(true);
    }

    void StyleIntegrationTests::TestCanUseStyleWithDeepNestedTemplateVSM(bool isExplicit)
    {
        auto panel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetVariationFileName(L"StyleWithDeepNestedTemplateVSM", isExplicit)));

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = m_rootCanvas;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            m_rootCanvas->Children->Clear();
            m_rootCanvas->Children->Append(panel);
            m_rootCanvas->UpdateLayout();

            auto button = safe_cast<Button^>(panel->Children->GetAt(0));
            auto brush = safe_cast<SolidColorBrush^>(button->Background);
            VERIFY_ARE_EQUAL(Colors::Blue, brush->Color, L"Button.Background should be Blue");

            m_rootCanvas->Children->Clear();
        });
    }

    void StyleIntegrationTests::CanUseStyleWithThemeResource()
    {
        TestCleanupWrapper cleanup;

        auto panel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetVariationFileName(L"StyleWithThemeResource")));

        RunOnUIThread([&]()
        {
            auto button = safe_cast<Button^>(panel->Children->GetAt(0));
            auto brush = safe_cast<SolidColorBrush^>(button->Background);
            VERIFY_ARE_EQUAL(Colors::Blue, brush->Color, L"Button.Background should be Blue");
        });
    }

    void StyleIntegrationTests::CanUseStyleWithCustomResource()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto guard = wil::scope_exit([]
            {
                Microsoft::UI::Xaml::Resources::CustomXamlResourceLoader::Current = nullptr;
            });

            String^ xamlString =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Canvas.Resources>"
                L"        <Style x:Key='S1' TargetType='Button'>"
                L"            <Setter Property='Background' Value='{CustomResource blueBrush}' />"
                L"            <Setter Property='Width' Value='100' />"
                L"        </Style>"
                L"    </Canvas.Resources>"
                L"    <Button x:Name='button1' Height='80' Content='S1' Style='{StaticResource S1}'/>"
                L"</Canvas>";

            TestCustomResourceLoader^ customResourceLoader = ref new TestCustomResourceLoader();
            Microsoft::UI::Xaml::Resources::CustomXamlResourceLoader::Current = customResourceLoader;
            auto blueBrush = ref new SolidColorBrush(Colors::Blue);
            customResourceLoader->AddResource(L"blueBrush", L"Microsoft.UI.Xaml.Setter", L"Value", L"Windows.Foundation.Object", blueBrush);

            auto canvas = safe_cast<Canvas^>(XamlReader::Load(xamlString));
            auto button = safe_cast<Button^>(canvas->Children->GetAt(0));
            auto brush = safe_cast<SolidColorBrush^>(button->Background);
            VERIFY_ARE_EQUAL(Colors::Blue, brush->Color, L"Button.Background should be Blue");
        });
    }

    // This test makes sure that we don't hit an assert in OnManagedPeerCreated() when
    // we unset and reset a style.  The style definition in this case has a setter that sets a
    // style-typed property.  A peer is created for the inner style when the setter is first applied.
    // An assert happens on the second set if the parent style didn't keep the original peer alive.
    void StyleIntegrationTests::CanStyleResetPropertyOfDependencyObjectType()
    {
        TestCleanupWrapper cleanup;

        auto canvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetVariationFileName(L"CustomControlCustomStyleProperty")));

        RunOnUIThread([&]()
        {
            m_rootCanvas->Children->Clear();
            m_rootCanvas->Children->Append(canvas);

            auto targetControl = safe_cast<CustomControl^>(canvas->Children->GetAt(0));
            auto style1 = safe_cast<xaml::Style^>(canvas->Resources->Lookup(L"S1"));

            LOG_OUTPUT(L"Setting style to S1");
            targetControl->Style = style1;

            LOG_OUTPUT(L"Setting style to null");
            targetControl->Style = nullptr;

            LOG_OUTPUT(L"Setting style to S1");
            targetControl->Style = style1;

            LOG_OUTPUT(L"Done setting style");
        });
    }

    void StyleIntegrationTests::StyleBasedOnSelfError()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel.Resources>"
                L"      <SolidColorBrush x:Key='BlueBrush' Color='Blue' />"
                L"      <SolidColorBrush x:Key='RedBrush' Color='Red' />"
                L"      <Style x:Key='IconStyle' BasedOn='{StaticResource IconStyle}' TargetType='TextBlock'>"
                L"          <Setter Property='Foreground' Value='{StaticResource RedBrush}' />"
                L"      </Style>"
                L"  </StackPanel.Resources>"
                L"  <TextBlock Style='{StaticResource IconStyle}' Text='Test' FontFamily='Arial' />"
                L"</StackPanel>";

            VERIFY_THROWS_WINRT(XamlReader::Load(xamlString), Platform::COMException^, L"An exception should be thrown when Style.BasedOn is set to the style itself.");
        });
    }

    void StyleIntegrationTests::StyleWithReferenceCycleError()
    {
        // Leak: StyleWithReferenceCycleError test leaks
        TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel.Resources>"
                L"      <Style x:Key='ButtonStyleBase' TargetType='ButtonBase'>"
                L"          <Setter Property='Foreground' Value='Blue' />"
                L"          <Setter Property='Template'>"
                L"            <Setter.Value>"
                L"              <ControlTemplate TargetType='Button'>"
                L"                <Border>"
                L"                  <Button x:Name='InnerButton' Style='{StaticResource ButtonStyle1}' />"
                L"                </Border>"
                L"              </ControlTemplate>"
                L"            </Setter.Value>"
                L"          </Setter>"
                L"      </Style>"
                L"      <Style x:Key='ButtonStyle1' BasedOn='{StaticResource ButtonStyleBase}' TargetType='ButtonBase'>"
                L"          <Setter Property='Background' Value='Red' />"
                L"      </Style>"
                L"  </StackPanel.Resources>"
                L"  <Button Style='{StaticResource ButtonStyle1}' Content='Test' />"
                L"</StackPanel>";

            auto panel = static_cast<Panel^>(XamlReader::Load(xamlString));
            auto outerButton = safe_cast<Button^>(panel->Children->GetAt(0));
            outerButton->ApplyTemplate();
            auto innerButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(outerButton, L"InnerButton"));
            VERIFY_THROWS_WINRT(innerButton->ApplyTemplate(), Platform::COMException^);
        });
    }

    void StyleIntegrationTests::StyleBasedOnSameKey()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel.Resources>"
                L"      <Style x:Key='ButtonStyle1' TargetType='ButtonBase'>"
                L"          <Setter Property='Background' Value='Blue' />"
                L"          <Setter Property='Foreground' Value='Green' />"
                L"      </Style>"
                L"  </StackPanel.Resources>"
                L"  <StackPanel>"
                L"    <StackPanel.Resources>"
                L"        <SolidColorBrush x:Key='RedBrush' Color='Red' />"
                L"        <Style x:Key='ButtonStyle1' BasedOn='{StaticResource ButtonStyle1}' TargetType='ButtonBase'>"
                L"            <Setter Property='Background' Value='{StaticResource RedBrush}' />"
                L"        </Style>"
                L"    </StackPanel.Resources>"
                L"    <Button Style='{StaticResource ButtonStyle1}' Content='Test' />"
                L"  </StackPanel>"
                L"</StackPanel>";

            auto panel = static_cast<Panel^>(XamlReader::Load(xamlString));
            auto panel2 = safe_cast<Panel^>(panel->Children->GetAt(0));
            auto button = safe_cast<Button^>(panel2->Children->GetAt(0));
            auto brush = safe_cast<SolidColorBrush^>(button->Background);
            VERIFY_IS_NOT_NULL(brush);
            VERIFY_ARE_EQUAL(Colors::Red, brush->Color, L"Button.Background should be Red");
            brush = safe_cast<SolidColorBrush^>(button->Foreground);
            VERIFY_IS_NOT_NULL(brush);
            VERIFY_ARE_EQUAL(Colors::Green, brush->Color, L"Button.Foreground should be Green");
        });
    }

    void StyleIntegrationTests::StyleBasedOnBuiltinStyleWithSameKey()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel.Resources>"
                L"      <SolidColorBrush x:Key='RedBrush' Color='Red' />"
                L"      <Style x:Key='TextBlockButtonStyle' BasedOn='{StaticResource TextBlockButtonStyle}' TargetType='ButtonBase'>"
                L"          <Setter Property='Background' Value='{StaticResource RedBrush}' />"
                L"      </Style>"
                L"  </StackPanel.Resources>"
                L"  <Button Style='{StaticResource TextBlockButtonStyle}' Content='Test' />"
                L"</StackPanel>";

            auto panel = static_cast<Panel^>(XamlReader::Load(xamlString));
            auto button = safe_cast<Button^>(panel->Children->GetAt(0));
            auto brush = safe_cast<SolidColorBrush^>(button->Background);
            VERIFY_IS_NOT_NULL(brush);
            VERIFY_ARE_EQUAL(Colors::Red, brush->Color, L"Button.Background should be Red");
        });
    }

    void StyleIntegrationTests::StyleBasedOnSameKeyBasedOnAnother()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel.Resources>"
                L"      <SolidColorBrush x:Key='BlueBrush' Color='Blue' />"
                L"      <Style x:Key='ButtonStyleBase' TargetType='ButtonBase'>"
                L"          <Setter Property='Foreground' Value='Green' />"
                L"      </Style>"
                L"      <Style x:Key='ButtonStyle1' BasedOn='{StaticResource ButtonStyleBase}' TargetType='ButtonBase'>"
                L"          <Setter Property='Background' Value='{StaticResource BlueBrush}' />"
                L"      </Style>"
                L"  </StackPanel.Resources>"
                L"  <StackPanel>"
                L"    <StackPanel.Resources>"
                L"        <SolidColorBrush x:Key='RedBrush' Color='Red' />"
                L"        <Style x:Key='ButtonStyleBase' TargetType='ButtonBase'>"
                L"            <Setter Property='Foreground' Value='Pink' />"
                L"        </Style>"
                L"        <Style x:Key='ButtonStyle1' BasedOn='{StaticResource ButtonStyle1}' TargetType='ButtonBase'>"
                L"            <Setter Property='Background' Value='{StaticResource RedBrush}' />"
                L"        </Style>"
                L"    </StackPanel.Resources>"
                L"    <Button Style='{StaticResource ButtonStyle1}' Content='Test' />"
                L"  </StackPanel>"
                L"</StackPanel>";

            auto panel = static_cast<Panel^>(XamlReader::Load(xamlString));
            auto panel2 = safe_cast<Panel^>(panel->Children->GetAt(0));
            auto button = safe_cast<Button^>(panel2->Children->GetAt(0));

            // Button's Background should come from the ButtonStyle1 style in its parent's resources.
            auto brush = safe_cast<SolidColorBrush^>(button->Background);
            VERIFY_IS_NOT_NULL(brush);
            VERIFY_ARE_EQUAL(Colors::Red, brush->Color, L"Button.Background should be Red");

            // Button's Foreground should come from the ButtonStyleBase style defined in its grandparent's resources.
            brush = safe_cast<SolidColorBrush^>(button->Foreground);
            VERIFY_IS_NOT_NULL(brush);
            VERIFY_ARE_EQUAL(Colors::Green, brush->Color, L"Button.Foreground should be Green");
        });
    }

    void StyleIntegrationTests::StyleWithStaticResourceInTemplate()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel.Resources>"
                L"      <SolidColorBrush x:Key='RedBrush' Color='Pink' />" // fake RedBrush
                L"      <SolidColorBrush x:Key='GreenBrush' Color='Green' />"
                L"      <Style x:Key='ButtonStyle1' TargetType='ButtonBase'>"
                L"          <Setter Property='Background' Value='Blue' />"
                L"          <Setter Property='Template'>"
                L"            <Setter.Value>"
                L"              <ControlTemplate TargetType='Button'>"
                L"                <Border>"
                L"                    <Button x:Name='InnerButton1' Style='{StaticResource ButtonStyle2}' />"
                L"                </Border>"
                L"              </ControlTemplate>"
                L"            </Setter.Value>"
                L"          </Setter>"
                L"      </Style>"
                L"      <Style x:Key='ButtonStyle2' TargetType='ButtonBase'>"
                L"          <Setter Property='Background' Value='Blue' />"
                L"          <Setter Property='Template'>"
                L"            <Setter.Value>"
                L"              <ControlTemplate TargetType='Button'>"
                L"                <Border>"
                L"                  <Border.Resources>"
                L"                    <SolidColorBrush x:Key='RedBrush' Color='Red' />" // real RedBrush
                L"                    <SolidColorBrush x:Key='BlueBrush' Color='Blue' />"
                L"                  </Border.Resources>"
                L"                  <Grid>"
                L"                    <Button x:Name='InnerButton2' Background='{StaticResource RedBrush}' Foreground='{StaticResource GreenBrush}' />"
                L"                  </Grid>"
                L"                </Border>"
                L"              </ControlTemplate>"
                L"            </Setter.Value>"
                L"          </Setter>"
                L"      </Style>"
                L"  </StackPanel.Resources>"
                L"  <Button Style='{StaticResource ButtonStyle1}' Content='Test' />"
                L"</StackPanel>";

            auto panel = static_cast<Panel^>(XamlReader::Load(xamlString));
            auto outerButton = safe_cast<Button^>(panel->Children->GetAt(0));
            outerButton->ApplyTemplate();
            auto innerButton1 = safe_cast<Button^>(TreeHelper::GetVisualChildByName(outerButton, L"InnerButton1"));
            innerButton1->ApplyTemplate();
            auto innerButton2 = safe_cast<Button^>(TreeHelper::GetVisualChildByName(innerButton1, L"InnerButton2"));

            // Verify that InnerButton2's Background comes from the template's RedBrush resource,
            // not the fake RedBrush defined in the outer resources.
            auto brush = safe_cast<SolidColorBrush^>(innerButton2->Background);
            VERIFY_IS_NOT_NULL(brush);
            VERIFY_ARE_EQUAL(Colors::Red, brush->Color, L"InnerButton2's Background should be Red");

            // Verify that InnerButton2's Background is found in the outer resources.
            brush = safe_cast<SolidColorBrush^>(innerButton2->Foreground);
            VERIFY_IS_NOT_NULL(brush);
            VERIFY_ARE_EQUAL(Colors::Green, brush->Color, L"InnerButton2's Foreground should be Green");
        });
    }

    void StyleIntegrationTests::StyleSetterWithAutoLengthValue()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel.Resources>"
                L"      <SolidColorBrush x:Key='BlueBrush' Color='Blue' />"
                L"      <Style x:Key='ButtonStyle' TargetType='Button'>"
                L"          <Setter Property='Width' Value='Auto' />"
                L"      </Style>"
                L"  </StackPanel.Resources>"
                L"  <Button Style='{StaticResource ButtonStyle}' />"
                L"</StackPanel>";

            auto panel = static_cast<Panel^>(XamlReader::Load(xamlString));

            auto button1 = safe_cast<Button^>(panel->Children->GetAt(0));
            VERIFY_IS_TRUE(!!_isnan(button1->Width), L"Button.Width should be NaN");
        });
    }

    void StyleIntegrationTests::StyleSetterWithEnumValue()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel.Resources>"
                L"      <Style x:Key='S1' TargetType='Button'>"
                L"          <Setter Property='HorizontalContentAlignment' Value='Stretch' />"
                L"      </Style>"
                L"  </StackPanel.Resources>"
                L"  <Button Style='{StaticResource S1}' />"
                L"</StackPanel>";

            auto panel = static_cast<Panel^>(XamlReader::Load(xamlString));
            auto button = safe_cast<Button^>(panel->Children->GetAt(0));
            VERIFY_IS_TRUE(button->HorizontalContentAlignment == HorizontalAlignment::Stretch, L"Button.HorizontalContentAlignment should be Stretch");
        });
    }

    void StyleIntegrationTests::StyleSetterWithCustomEnumValue()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"        xmlns:local='using:Tests.Native.External.Framework'>"
                L"  <StackPanel.Resources>"
                L"      <Style x:Key='S1' TargetType='local:CustomControl'>"
                L"          <Setter Property='CustomEnum' Value='CustomEnumValue1' />"
                L"      </Style>"
                L"  </StackPanel.Resources>"
                L"  <local:CustomControl Style='{StaticResource S1}' />"
                L"</StackPanel>";

            auto panel = static_cast<Panel^>(XamlReader::Load(xamlString));
            auto control = safe_cast<CustomControl^>(panel->Children->GetAt(0));
            VERIFY_IS_TRUE(control->CustomEnum == CustomEnumValues::CustomEnumValue1, L"CustomControl.CustomEnum should be CustomEnumValue1");
        });
    }

    void StyleIntegrationTests::StyleSetterWithBinding()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel.Resources>"
                L"    <SolidColorBrush x:Key='blueBrush' Color='Blue' />"
                L"    <SolidColorBrush x:Key='redBrush' Color='Red' />"
                L"  </StackPanel.Resources>"
                L"  <Button Content='Test' DataContext='{StaticResource blueBrush}'>"
                L"    <Button.Style>"
                L"      <Style TargetType='Button'>"
                L"        <Setter Property='Background' Value='{Binding}' />"
                L"        <Setter Property='BorderBrush' Value='Green' />"
                L"      </Style>"
                L"    </Button.Style>"
                L"  </Button>"
                L"</StackPanel>";

            // Verify that the value is null.
            // If the binding were set directly (instead of via a setter),
            // the value would be the same as the referenced resource.
            auto panel = static_cast<Panel^>(XamlReader::Load(xamlString));
            auto button = safe_cast<Button^>(panel->Children->GetAt(0));
            VERIFY_IS_NULL(button->Background, L"Button.Background should be null");

            // Verify that updating the DataContext doesn't affect the target.
            // If the binding were set directly (instead of via a setter),
            // the update would change the target.
            auto brush = safe_cast<SolidColorBrush^>(panel->Resources->Lookup(L"redBrush"));
            button->DataContext = brush;

            VERIFY_IS_NULL(button->Background, L"Button.Background should be null");

            // Verify that the non-binding setter works.
            brush = safe_cast<SolidColorBrush^>(button->BorderBrush);
            VERIFY_ARE_EQUAL(Colors::Green, brush->Color, L"Button.BorderBrush should be Green");
        });
    }

    void StyleIntegrationTests::StyleSetterWithStaticResourceBinding()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel.Resources>"
                L"    <SolidColorBrush x:Key='blueBrush' Color='Blue' />"
                L"    <SolidColorBrush x:Key='redBrush' Color='Red' />"
                L"  </StackPanel.Resources>"
                L"  <Button Content='Test'>"
                L"    <Button.Style>"
                L"      <Style TargetType='Button'>"
                L"        <Setter Property='Background' Value='{Binding Mode=OneTime, Source={StaticResource blueBrush}}' />"
                L"        <Setter Property='Foreground' Value='{Binding Mode=OneWay, Source={StaticResource redBrush}}' />"
                L"        <Setter Property='BorderBrush' Value='Green' />"
                L"      </Style>"
                L"    </Button.Style>"
                L"  </Button>"
                L"</StackPanel>";

            // Verify that a binding with Source set to a StaticResource works.
            // Check that bindings work in multiple setters.
            auto panel = static_cast<Panel^>(XamlReader::Load(xamlString));
            auto button = safe_cast<Button^>(panel->Children->GetAt(0));
            auto brush = safe_cast<SolidColorBrush^>(button->Background);
            VERIFY_IS_NOT_NULL(brush);
            VERIFY_ARE_EQUAL(Colors::Blue, brush->Color, L"Button.Background should be Blue");
            brush = safe_cast<SolidColorBrush^>(button->Foreground);
            VERIFY_IS_NOT_NULL(brush);
            VERIFY_ARE_EQUAL(Colors::Red, brush->Color, L"Button.Foreground should be Red");

            // Verify that the non-binding setter works.
            brush = safe_cast<SolidColorBrush^>(button->BorderBrush);
            VERIFY_ARE_EQUAL(Colors::Green, brush->Color, L"Button.BorderBrush should be Green");
        });
    }

    void StyleIntegrationTests::StyleSetterWithElementNameBinding()
    {
        TestCleanupWrapper cleanup;
        Panel^ panel;

        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button Background='Blue' x:Name='enSource' /> "
                L"  <Button Content='Test'>"
                L"    <Button.Style>"
                L"      <Style TargetType='Button'>"
                L"        <Setter Property='Background' Value='{Binding Background, ElementName=enSource}' />"
                L"        <Setter Property='BorderBrush' Value='Green' />"
                L"      </Style>"
                L"    </Button.Style>"
                L"  </Button>"
                L"</StackPanel>";

            panel = static_cast<Panel^>(XamlReader::Load(xamlString));
            TestServices::WindowHelper->WindowContent = panel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Verify that the value is null.
            // If the binding were set directly (instead of via a setter),
            // the value would be the same as the source.
            auto button = safe_cast<Button^>(panel->Children->GetAt(1));
            VERIFY_IS_NULL(button->Background, L"Button.Background should be null");

            // Verify that updating the source doesn't affect the target.
            // If the binding were set directly (instead of via a setter),
            // the update would change the target.
            button = safe_cast<Button^>(panel->Children->GetAt(0));
            auto newBrush = ref new SolidColorBrush();
            newBrush->Color = Colors::Red;
            button->Background = newBrush;

            button = safe_cast<Button^>(panel->Children->GetAt(1));
            VERIFY_IS_NULL(button->Background, L"Button.Background should be null");

            // Verify that the non-binding setter works.
            auto brush = safe_cast<SolidColorBrush^>(button->BorderBrush);
            VERIFY_ARE_EQUAL(Colors::Green, brush->Color, L"Button.BorderBrush should be Green");
        });
    }

    void StyleIntegrationTests::StyleSetterWithElementNameTwoWayBinding()
    {
        TestCleanupWrapper cleanup;
        Panel^ panel;

        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button Background='Blue' x:Name='enSource' /> "
                L"  <Button Content='Test'>"
                L"    <Button.Style>"
                L"      <Style TargetType='Button'>"
                L"        <Setter Property='Background' Value='{Binding Background, Mode=TwoWay, ElementName=enSource}' />"
                L"        <Setter Property='BorderBrush' Value='Green' />"
                L"      </Style>"
                L"    </Button.Style>"
                L"  </Button>"
                L"</StackPanel>";

            panel = static_cast<Panel^>(XamlReader::Load(xamlString));
            TestServices::WindowHelper->WindowContent = panel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Verify that the value is null.
            // If the binding were set directly (instead of via a setter),
            // the value would be the same as the source.
            auto button = safe_cast<Button^>(panel->Children->GetAt(1));
            VERIFY_IS_NULL(button->Background, L"Button.Background should be null");

            // Verify that updating the target doesn't affect the source.
            // If the binding were set directly (instead of via a setter),
            // the update would change the source.
            auto newBrush = ref new SolidColorBrush();
            newBrush->Color = Colors::Red;
            button->Background = newBrush;

            button = safe_cast<Button^>(panel->Children->GetAt(0));
            auto brush = safe_cast<SolidColorBrush^>(button->Background);
            VERIFY_ARE_EQUAL(Colors::Blue, brush->Color, L"Button.Background should be Blue");

            // Verify that the non-binding setter works.
            button = safe_cast<Button^>(panel->Children->GetAt(1));
            brush = safe_cast<SolidColorBrush^>(button->BorderBrush);
            VERIFY_ARE_EQUAL(Colors::Green, brush->Color, L"Button.BorderBrush should be Green");
        });
    }

    void StyleIntegrationTests::StyleSetterWithSelfBinding()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button Content='Test' Height='100'>"
                L"    <Button.Style>"
                L"      <Style TargetType='Button'>"
                L"        <Setter Property='Width' Value='{Binding Height, Mode=OneWay, RelativeSource={RelativeSource Self}}' />"
                L"        <Setter Property='BorderBrush' Value='Green' />"
                L"      </Style>"
                L"    </Button.Style>"
                L"  </Button>"
                L"</StackPanel>";

            // Verify that Width is the default (NaN).
            // If the binding were set directly (instead of via a setter),
            // the value would be equal to the source (100.0).
            auto panel = static_cast<Panel^>(XamlReader::Load(xamlString));
            auto button = safe_cast<Button^>(panel->Children->GetAt(0));
            VERIFY_IS_TRUE(!!_isnan(button->Width), L"Button.Width should be NaN");

            // Verify that the non-binding setter works.
            auto brush = safe_cast<SolidColorBrush^>(button->BorderBrush);
            VERIFY_ARE_EQUAL(Colors::Green, brush->Color, L"Button.BorderBrush should be Green");
        });
    }

    void StyleIntegrationTests::StyleWithNestedResources()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Button.Style>"
                L"    <Style TargetType='Button'>"
                L"      <Setter Property='Background' Value='Red' />"
                L"      <Setter Property='Template'>"
                L"        <Setter.Value>"
                L"          <ControlTemplate TargetType='Button'>"
                L"            <Border x:Name='InnerBorder' Background='Green'>"
                L"              <Border.Resources>"
                L"                  <SolidColorBrush x:Key='BlueBrush' Color='Blue' />"
                L"                  <SolidColorBrush x:Key='RedBrush' Color='Red' />"
                L"              </Border.Resources>"
                L"              <CheckBox x:Name='InnerControl' Background='{StaticResource RedBrush}' />"
                L"            </Border>"
                L"          </ControlTemplate>"
                L"        </Setter.Value>"
                L"      </Setter>"
                L"    </Style>"
                L"    </Button.Style>"
                L"</Button>";

            auto control = safe_cast<Button^>(XamlReader::Load(xamlString));
            VERIFY_IS_NOT_NULL(control->Style);

            control->ApplyTemplate();
            auto checkBox = safe_cast<CheckBox^>(TreeHelper::GetVisualChildByName(control, L"InnerControl"));
            VERIFY_IS_NOT_NULL(checkBox);
            VERIFY_ARE_EQUAL(static_cast<SolidColorBrush^>(checkBox->Background)->Color, Colors::Red);
        });
    }

    void StyleIntegrationTests::CanSetNullStyleSetterValue()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([]()
        {
            Platform::String^ xamlString =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Canvas.Resources>"
                L"        <Style x:Key='myStyle' TargetType='Button'>"
                L"            <Setter Property='Background' Value='{x:Null}' />"
                L"        </Style>"
                L"    </Canvas.Resources>"
                L"    <Button x:Name='button' Style='{StaticResource myStyle}' />"
                L"</Canvas>";

            auto canvas = safe_cast<Canvas^>(XamlReader::Load(xamlString));
            VERIFY_IS_NOT_NULL(canvas);

            auto button = safe_cast<Button^>(canvas->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);
            VERIFY_IS_NULL(button->Background);

            // Fault in setters and verify that the setter value is null.
            auto style = safe_cast<Style^>(canvas->Resources->Lookup(L"myStyle"));
            auto setter = safe_cast<Setter^>(style->Setters->GetAt(0));
            VERIFY_ARE_EQUAL(setter->Property, Control::BackgroundProperty);
            VERIFY_IS_NULL(setter->Value);
        });

        RunOnUIThread([]()
        {
            Platform::String^ xamlString =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"        xmlns:local='using:Tests.Native.External.Framework'>"
                L"    <Canvas.Resources>"
                L"        <Style x:Key='myStyle' TargetType='Button'>"
                L"            <Setter Property='local:ControlWithAttachedProperty.CustomAttachedBrush' Value='{x:Null}' />"
                L"        </Style>"
                L"    </Canvas.Resources>"
                L"    <Button x:Name='button' Style='{StaticResource myStyle}'>"
                L"      <Button.Template>"
                L"        <ControlTemplate TargetType='Button'>"
                L"            <StackPanel x:Name='innerPanel' "
                L"                        Background='{TemplateBinding local:ControlWithAttachedProperty.CustomAttachedBrush}' />"
                L"        </ControlTemplate>"
                L"      </Button.Template>"
                L"    </Button>"
                L"</Canvas>";

            auto canvas = safe_cast<Canvas^>(XamlReader::Load(xamlString));
            VERIFY_IS_NOT_NULL(canvas);

            auto button = safe_cast<Button^>(canvas->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);
            VERIFY_IS_NULL(button->GetValue(ControlWithAttachedProperty::CustomAttachedBrushProperty));

            button->ApplyTemplate();

            // Verify the bound value is null.
            auto innerPanel = safe_cast<StackPanel^>(TreeHelper::GetVisualChildByName(button, L"innerPanel"));
            VERIFY_IS_NULL(innerPanel->Background);

            // Fault in setters and verify that the setter value is null.
            auto style = safe_cast<Style^>(canvas->Resources->Lookup(L"myStyle"));
            auto setter = safe_cast<Setter^>(style->Setters->GetAt(0));
            VERIFY_ARE_EQUAL(setter->Property, ControlWithAttachedProperty::CustomAttachedBrushProperty);
            VERIFY_IS_NULL(setter->Value);
        });
    }

    void StyleIntegrationTests::StyleInVSM()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Button.Template>"
                L"      <ControlTemplate TargetType='Button'>"
                L"          <StackPanel Background='Transparent'>"
                L""
                L"            <Button"
                L"              x:Name='ContentHost'"
                L"              Content='Foo'"
                L"              Style='{x:Null}' />"
                L""
                L"            <VisualStateManager.VisualStateGroups>"
                L"              <VisualStateGroup x:Name='CommonStates'>"
                L"                <VisualState x:Name='Normal' />"
                L"                <VisualState x:Name='VisualState1'>"
                L"                  <Storyboard>"
                L"                    <ObjectAnimationUsingKeyFrames Storyboard.TargetName='ContentHost' Storyboard.TargetProperty='Style'>"
                L"                      <DiscreteObjectKeyFrame KeyTime='0'>"
                L"                        <DiscreteObjectKeyFrame.Value>"
                L"                          <Style TargetType='Button'>"
                L"                            <Setter Property='Background' Value='Red' />"
                L"                          </Style>"
                L"                        </DiscreteObjectKeyFrame.Value>"
                L"                      </DiscreteObjectKeyFrame>"
                L"                    </ObjectAnimationUsingKeyFrames>"
                L"                  </Storyboard>"
                L"                </VisualState>"
                L"              </VisualStateGroup>"
                L"            </VisualStateManager.VisualStateGroups>"
                L"          </StackPanel>"
                L"      </ControlTemplate>"
                L"    </Button.Template>"
                L"</Button>";

            auto parent = safe_cast<Button^>(XamlReader::Load(xamlString));

            parent->ApplyTemplate();
            auto child = safe_cast<Button^>(TreeHelper::GetVisualChildByName(parent, L"ContentHost"));

            VERIFY_IS_NULL(child->Style);

            // Got to visual state, and verify the style was applied.
            VisualStateManager::GoToState(parent, "VisualState1", true);
            VERIFY_IS_NOT_NULL(child->Style);
            VERIFY_ARE_EQUAL(static_cast<SolidColorBrush^>(child->Background)->Color, Colors::Red);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void StyleIntegrationTests::StyleInVSMWithNestedTemplate()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Button.Template>"
                L"      <ControlTemplate TargetType='Button'>"
                L"          <StackPanel Background='Transparent'>"
                L""
                L"            <Button"
                L"              x:Name='ContentHost'"
                L"              Content='Foo'"
                L"              Style='{x:Null}' />"
                L""
                L"            <VisualStateManager.VisualStateGroups>"
                L"              <VisualStateGroup x:Name='CommonStates'>"
                L"                <VisualState x:Name='Normal' />"
                L"                <VisualState x:Name='VisualState1'>"
                L"                  <Storyboard>"
                L"                    <ObjectAnimationUsingKeyFrames Storyboard.TargetName='ContentHost' Storyboard.TargetProperty='Style'>"
                L"                      <DiscreteObjectKeyFrame KeyTime='0'>"
                L"                        <DiscreteObjectKeyFrame.Value>"
                L"                          <Style TargetType='Button'>"
                L"                            <Setter Property='Background' Value='Red' />"
                L"                            <Setter Property='Template'>"
                L"                              <Setter.Value>"
                L"                                <ControlTemplate TargetType='Button'>"
                L"                                  <Border x:Name='InnerBorder' Background='Green'>"
                L"                                    <CheckBox x:Name='InnerControl' />"
                L"                                  </Border>"
                L"                                </ControlTemplate>"
                L"                              </Setter.Value>"
                L"                            </Setter>"
                L"                          </Style>"
                L"                        </DiscreteObjectKeyFrame.Value>"
                L"                      </DiscreteObjectKeyFrame>"
                L"                    </ObjectAnimationUsingKeyFrames>"
                L"                  </Storyboard>"
                L"                </VisualState>"
                L"              </VisualStateGroup>"
                L"            </VisualStateManager.VisualStateGroups>"
                L"          </StackPanel>"
                L"      </ControlTemplate>"
                L"    </Button.Template>"
                L"</Button>";

            auto parent = safe_cast<Button^>(XamlReader::Load(xamlString));

            parent->ApplyTemplate();
            auto child = safe_cast<Button^>(TreeHelper::GetVisualChildByName(parent, L"ContentHost"));

            VERIFY_IS_NULL(child->Style);

            // Got to visual state.
            VisualStateManager::GoToState(parent, "VisualState1", true);
            VERIFY_IS_NOT_NULL(child->Style);

            // Apply styled template, and verify the CheckBox exists.
            child->ApplyTemplate();
            auto checkBox = safe_cast<CheckBox^>(TreeHelper::GetVisualChildByName(child, L"InnerControl"));
            VERIFY_IS_NOT_NULL(checkBox);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void StyleIntegrationTests::StyleInVSMWithNestedResources()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Button.Template>"
                L"      <ControlTemplate TargetType='Button'>"
                L"          <StackPanel Background='Transparent'>"
                L""
                L"            <Button"
                L"              x:Name='ContentHost'"
                L"              Content='Foo'"
                L"              Style='{x:Null}' />"
                L""
                L"            <VisualStateManager.VisualStateGroups>"
                L"              <VisualStateGroup x:Name='CommonStates'>"
                L"                <VisualState x:Name='Normal' />"
                L"                <VisualState x:Name='VisualState1'>"
                L"                  <Storyboard>"
                L"                    <ObjectAnimationUsingKeyFrames Storyboard.TargetName='ContentHost' Storyboard.TargetProperty='Style'>"
                L"                      <DiscreteObjectKeyFrame KeyTime='0'>"
                L"                        <DiscreteObjectKeyFrame.Value>"
                L"                          <Style TargetType='Button'>"
                L"                            <Setter Property='Background' Value='Red' />"
                L"                            <Setter Property='Template'>"
                L"                              <Setter.Value>"
                L"                                <ControlTemplate TargetType='Button'>"
                L"                                  <Border x:Name='InnerBorder' Background='Green'>"
                L"                                    <Border.Resources>"
                L"                                        <SolidColorBrush x:Key='BlueBrush' Color='Blue' />"
                L"                                        <SolidColorBrush x:Key='RedBrush' Color='Red' />"
                L"                                    </Border.Resources>"
                L"                                    <CheckBox x:Name='InnerControl' Background='{StaticResource RedBrush}' />"
                L"                                  </Border>"
                L"                                </ControlTemplate>"
                L"                              </Setter.Value>"
                L"                            </Setter>"
                L"                          </Style>"
                L"                        </DiscreteObjectKeyFrame.Value>"
                L"                      </DiscreteObjectKeyFrame>"
                L"                    </ObjectAnimationUsingKeyFrames>"
                L"                  </Storyboard>"
                L"                </VisualState>"
                L"              </VisualStateGroup>"
                L"            </VisualStateManager.VisualStateGroups>"
                L"          </StackPanel>"
                L"      </ControlTemplate>"
                L"    </Button.Template>"
                L"</Button>";

            auto parent = safe_cast<Button^>(XamlReader::Load(xamlString));

            parent->ApplyTemplate();
            auto child = safe_cast<Button^>(TreeHelper::GetVisualChildByName(parent, L"ContentHost"));

            VERIFY_IS_NULL(child->Style);

            // Got to visual state.
            VisualStateManager::GoToState(parent, "VisualState1", true);
            VERIFY_IS_NOT_NULL(child->Style);

            // Apply styled template, and verify the CheckBox background set via the static resource.
            child->ApplyTemplate();
            auto checkBox = safe_cast<CheckBox^>(TreeHelper::GetVisualChildByName(child, L"InnerControl"));
            VERIFY_IS_NOT_NULL(checkBox);
            VERIFY_ARE_EQUAL(static_cast<SolidColorBrush^>(checkBox->Background)->Color, Colors::Red);
        });
    }

    void StyleIntegrationTests::StyleInVSMWithNestedVSM()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Button.Template>"
                L"      <ControlTemplate TargetType='Button'>"
                L"          <StackPanel Background='Transparent'>"
                L""
                L"            <Button"
                L"              x:Name='ContentHost'"
                L"              Content='Foo'"
                L"              Style='{x:Null}' />"
                L""
                L"            <VisualStateManager.VisualStateGroups>"
                L"              <VisualStateGroup x:Name='CommonStates'>"
                L"                <VisualState x:Name='Normal' />"
                L"                <VisualState x:Name='VisualState1'>"
                L"                  <Storyboard>"
                L"                    <ObjectAnimationUsingKeyFrames Storyboard.TargetName='ContentHost' Storyboard.TargetProperty='Style'>"
                L"                      <DiscreteObjectKeyFrame KeyTime='0'>"
                L"                        <DiscreteObjectKeyFrame.Value>"
                L"                          <Style TargetType='Button'>"
                L"                            <Setter Property='Background' Value='Red' />"
                L"                            <Setter Property='Template'>"
                L"                              <Setter.Value>"
                L"                                <ControlTemplate TargetType='Button'>"
                L"                                  <Border x:Name='InnerBorder' Background='Green'>"
                L"                                    <CheckBox x:Name='InnerControl' />"
                L"                                    <VisualStateManager.VisualStateGroups>"
                L"                                      <VisualStateGroup>"
                L"                                        <VisualState x:Name='VisualState2'>"
                L"                                          <Storyboard>"
                L"                                            <ObjectAnimationUsingKeyFrames "
                L"                                                      Storyboard.TargetName='InnerBorder' "
                L"                                                      Storyboard.TargetProperty='Background'>"
                L"                                              <DiscreteObjectKeyFrame KeyTime='0' Value='Blue' />"
                L"                                            </ObjectAnimationUsingKeyFrames>"
                L"                                          </Storyboard>"
                L"                                        </VisualState>"
                L"                                      </VisualStateGroup>"
                L"                                    </VisualStateManager.VisualStateGroups>"
                L"                                  </Border>"
                L"                                </ControlTemplate>"
                L"                              </Setter.Value>"
                L"                            </Setter>"
                L"                          </Style>"
                L"                        </DiscreteObjectKeyFrame.Value>"
                L"                      </DiscreteObjectKeyFrame>"
                L"                    </ObjectAnimationUsingKeyFrames>"
                L"                  </Storyboard>"
                L"                </VisualState>"
                L"              </VisualStateGroup>"
                L"            </VisualStateManager.VisualStateGroups>"
                L"          </StackPanel>"
                L"      </ControlTemplate>"
                L"    </Button.Template>"
                L"</Button>";

            auto parent = safe_cast<Button^>(XamlReader::Load(xamlString));

            parent->ApplyTemplate();
            auto child = safe_cast<Button^>(TreeHelper::GetVisualChildByName(parent, L"ContentHost"));

            VERIFY_IS_NULL(child->Style);

            // Got to visual state.
            VisualStateManager::GoToState(parent, "VisualState1", true);
            VERIFY_IS_NOT_NULL(child->Style);

            // Apply styled template, and verify the border background is the local value (Green).
            child->ApplyTemplate();
            auto border = safe_cast<Border^>(TreeHelper::GetVisualChildByName(child, L"InnerBorder"));
            VERIFY_IS_NOT_NULL(border);
            VERIFY_ARE_EQUAL(static_cast<SolidColorBrush^>(border->Background)->Color, Colors::Green);

            // Got to the inner visual state, and verify the border background is the VSM value (Blue).
            VisualStateManager::GoToState(child, "VisualState2", true);
            VERIFY_ARE_EQUAL(static_cast<SolidColorBrush^>(border->Background)->Color, Colors::Blue);
        });
    }

    void StyleIntegrationTests::VerifyCoreStyleSettersReleased()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            CustomObject::ClearInstanceCount();

            auto customObject = ref new CustomObject();

            auto setter = ref new Setter();
            setter->Property = FrameworkElement::TagProperty;
            setter->Value = customObject;
            customObject = nullptr;

            auto style = ref new Style();
            style->TargetType = Button::typeid;
            style->Setters->Append(setter);
            setter = nullptr;

            auto button = ref new Button();
            button->Style = style;
            style = nullptr;

            button = nullptr;

            VERIFY_ARE_EQUAL(CustomObject::GetInstanceCount(), 0U);
        });
    }

    void StyleIntegrationTests::VerifyCoreBasedOnStyleSettersReleased()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Fault in style with brush in owned setter and CustomObject in BasedOn setter.");
        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      xmlns:local='using:Tests.Native.External.Framework'>"
                L"    <Grid.Resources>"
                L"      <Style x:Key='ButtonStyleBase' TargetType='Button'>"
                L"         <Setter Property='Tag'>"
                L"           <Setter.Value>"
                L"             <local:CustomObject />"
                L"           </Setter.Value>"
                L"         </Setter>"
                L"      </Style>"
                L"      <Style x:Key='ButtonStyle1' BasedOn='{StaticResource ButtonStyleBase}' TargetType='Button'>"
                L"          <Setter Property='Background' Value='Red' />"
                L"      </Style>"
                L"    </Grid.Resources>"
                L"    <Button x:Name='Button1' Style='{StaticResource ButtonStyle1}' Content='Test' />"
                L"</Grid>";

            CustomObject::ClearInstanceCount();

            auto panel = safe_cast<Panel^>(XamlReader::Load(xamlString));

            auto button = safe_cast<Button^>(panel->Children->GetAt(0));
            auto derived = safe_cast<Style^>(button->Style);

            // Fault in the setters.
            derived->Setters;
            auto tag = button->Tag;
            VERIFY_ARE_EQUAL(CustomObject::GetInstanceCount(), 1U);

            // Release everything and verify the setter value is released.
            button = nullptr;
            panel = nullptr;
            derived = nullptr;
            tag = nullptr;

            VERIFY_ARE_EQUAL(CustomObject::GetInstanceCount(), 0U);
        });

        LOG_OUTPUT(L"Fault in style with CustomObject in owned setter and brush in BasedOn setter.");
        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      xmlns:local='using:Tests.Native.External.Framework'>"
                L"    <Grid.Resources>"
                L"      <Style x:Key='ButtonStyleBase' TargetType='Button'>"
                L"          <Setter Property='Background' Value='Red' />"
                L"      </Style>"
                L"      <Style x:Key='ButtonStyle1' BasedOn='{StaticResource ButtonStyleBase}' TargetType='Button'>"
                L"         <Setter Property='Tag'>"
                L"           <Setter.Value>"
                L"             <local:CustomObject />"
                L"           </Setter.Value>"
                L"         </Setter>"
                L"      </Style>"
                L"    </Grid.Resources>"
                L"    <Button x:Name='Button1' Style='{StaticResource ButtonStyle1}' Content='Test' />"
                L"</Grid>";

            CustomObject::ClearInstanceCount();

            auto panel = safe_cast<Panel^>(XamlReader::Load(xamlString));

            auto button = safe_cast<Button^>(panel->Children->GetAt(0));
            auto derived = safe_cast<Style^>(button->Style);

            // Fault in the setters.
            derived->Setters;
            auto tag = button->Tag;
            VERIFY_ARE_EQUAL(CustomObject::GetInstanceCount(), 1U);

            // Release everything and verify the setter value is released.
            button = nullptr;
            panel = nullptr;
            derived = nullptr;
            tag = nullptr;

            VERIFY_ARE_EQUAL(CustomObject::GetInstanceCount(), 0U);
        });

        LOG_OUTPUT(L"Fault in style that overrides BasedOn setter.");
        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      xmlns:local='using:Tests.Native.External.Framework'>"
                L"    <Grid.Resources>"
                L"      <Style x:Key='ButtonStyleBase' TargetType='Button'>"
                L"         <Setter Property='Tag'>"
                L"           <Setter.Value>"
                L"             <local:CustomObject />"
                L"           </Setter.Value>"
                L"         </Setter>"
                L"      </Style>"
                L"      <Style x:Key='ButtonStyle1' BasedOn='{StaticResource ButtonStyleBase}' TargetType='Button'>"
                L"         <Setter Property='Tag'>"
                L"           <Setter.Value>"
                L"             <local:CustomObject />"
                L"           </Setter.Value>"
                L"         </Setter>"
                L"      </Style>"
                L"    </Grid.Resources>"
                L"    <Button x:Name='Button1' Style='{StaticResource ButtonStyle1}' Content='Test' />"
                L"</Grid>";

            CustomObject::ClearInstanceCount();

            auto panel = safe_cast<Panel^>(XamlReader::Load(xamlString));

            auto button = safe_cast<Button^>(panel->Children->GetAt(0));
            auto derived = safe_cast<Style^>(button->Style);

            // Fault in the setters.
            derived->Setters;
            auto tag = button->Tag;
            VERIFY_ARE_EQUAL(CustomObject::GetInstanceCount(), 2U);

            // Release everything and verify the setter value is released.
            button = nullptr;
            panel = nullptr;
            derived = nullptr;
            tag = nullptr;

            VERIFY_ARE_EQUAL(CustomObject::GetInstanceCount(), 0U);
        });
    }

    void StyleIntegrationTests::CanSetDefaultStyle()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        RunOnUIThread([&]()
        {
            auto defaultStyleControl = ref new DefaultStyleControl();
            TestServices::WindowHelper->WindowContent = defaultStyleControl;
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyUIElementTree();
    }

    void StyleIntegrationTests::CanSetDefaultStyleResourceUri()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        RunOnUIThread([&]()
        {
            auto defaultStyleResourceUriControl = ref new DefaultStyleResourceUriControl();
            TestServices::WindowHelper->WindowContent = defaultStyleResourceUriControl;
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyUIElementTree();
    }

    void StyleIntegrationTests::DefaultStyleResourceUriWithEmptyDefaultStyleKey()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        RunOnUIThread([&]()
        {
            auto defaultStyleResourceUriControl = ref new DefaultStyleResourceUriControlWithEmptyDefaultStyleKey();
            TestServices::WindowHelper->WindowContent = defaultStyleResourceUriControl;
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyUIElementTree();
    }

    void StyleIntegrationTests::DefaultStyleResourceUriWithInvalidDefaultStyleKey()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

        RunOnUIThread([&]()
        {
            auto defaultStyleResourceUriControl = ref new DefaultStyleResourceUriControlWithInvalidDefaultStyleKey();
            TestServices::WindowHelper->WindowContent = defaultStyleResourceUriControl;
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyUIElementTree();
    }

    void StyleIntegrationTests::CanFaultInUnsealedStyleAndAddSetter()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Fault in and add setter to style before applying it.");
        RunOnUIThread([this]()
        {
            Platform::String^ xamlString =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Canvas.Resources>"
                L"        <SolidColorBrush x:Key='RedBrush' Color='Red' />"
                L"        <Style x:Key='S1' TargetType='Button'>"
                L"            <Setter Property='Background' Value='{StaticResource RedBrush}' />"
                L"        </Style>"
                L"    </Canvas.Resources>"
                L"    <StackPanel>"
                L"        <Button x:Name='button1' Height='80' Content='Test' />"
                L"    </StackPanel>"
                L"</Canvas>";

            auto canvas = safe_cast<Canvas^>(XamlReader::Load(xamlString));
            auto button1 = safe_cast<Button^>(canvas->FindName(L"button1"));

            auto style = safe_cast<Style^>(canvas->Resources->Lookup(L"S1"));

            // This will fault in the setters.
            auto setters = style->Setters;
            VERIFY_ARE_EQUAL(setters->Size, 1U, L"Setters count should be 1");

            // Add setter.
            auto setter = ref new Setter();
            setter->Property = Control::ForegroundProperty;
            setter->Value = "Blue";
            setters->Append(setter);

            // Set button's style and verify values.
            VERIFY_IS_FALSE(style->IsSealed);
            button1->Style = style;
            VERIFY_IS_TRUE(style->IsSealed);
            VERIFY_ARE_EQUAL(setters->Size, 2U, L"Setter count should be 2");
            auto brush = safe_cast<SolidColorBrush^>(button1->Background);
            VERIFY_ARE_EQUAL(brush->Color, Colors::Red, L"Button Background should be Red brush");
            brush = safe_cast<SolidColorBrush^>(button1->Foreground);
            VERIFY_ARE_EQUAL(brush->Color, Colors::Blue, L"Button Foreground should be Blue brush");

            // Try to add another setter to style.
            // Verify it throws because it's sealed now.
            setter = ref new Setter();
            setter->Property = Control::WidthProperty;
            setter->Value = "10";
            VERIFY_THROWS_WINRT(setters->Append(setter), Platform::COMException^);
        });

        LOG_OUTPUT(L"Fault in and add setter to derived style before applying it.");
        RunOnUIThread([this]()
        {
            Platform::String^ xamlString =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Canvas.Resources>"
                L"        <SolidColorBrush x:Key='RedBrush' Color='Red' />"
                L"        <Style x:Key='S1' TargetType='Button'>"
                L"            <Setter Property='Height' Value='10' />"
                L"        </Style>"
                L"        <Style x:Key='S2' TargetType='Button' BasedOn='{StaticResource S1}'>"
                L"            <Setter Property='Background' Value='{StaticResource RedBrush}' />"
                L"        </Style>"
                L"    </Canvas.Resources>"
                L"    <StackPanel>"
                L"        <Button x:Name='button1' Content='Test' />"
                L"    </StackPanel>"
                L"</Canvas>";

            auto canvas = safe_cast<Canvas^>(XamlReader::Load(xamlString));
            auto button1 = safe_cast<Button^>(canvas->FindName(L"button1"));

            auto derived = safe_cast<Style^>(canvas->Resources->Lookup(L"S2"));

            // This will fault in the derived style's setters.
            auto setters = derived->Setters;
            VERIFY_ARE_EQUAL(setters->Size, 1U, L"Setters count should be 1");

            // Add setter to derived style.
            auto setter = ref new Setter();
            setter->Property = Control::ForegroundProperty;
            setter->Value = "Blue";
            setters->Append(setter);

            // Set button's style to derived style and verify values.
            VERIFY_IS_FALSE(derived->IsSealed);
            button1->Style = derived;
            VERIFY_IS_TRUE(derived->IsSealed);
            VERIFY_ARE_EQUAL(setters->Size, 2U, L"Setter count should be 2");
            auto brush = safe_cast<SolidColorBrush^>(button1->Background);
            VERIFY_ARE_EQUAL(brush->Color, Colors::Red, L"Button Background should be Red brush");
            brush = safe_cast<SolidColorBrush^>(button1->Foreground);
            VERIFY_ARE_EQUAL(brush->Color, Colors::Blue, L"Button Foreground should be Blue brush");
            VERIFY_ARE_EQUAL(button1->Height, 10, L"Button Height should be 10");

            // Try to add another setter to derived style.
            // Verify it throws because it's sealed now.
            setter = ref new Setter();
            setter->Property = Control::WidthProperty;
            setter->Value = "10";
            VERIFY_THROWS_WINRT(setters->Append(setter), Platform::COMException^);
        });

        LOG_OUTPUT(L"Fault in and add setter to BasedOn style before applying derived style.");
        RunOnUIThread([this]()
        {
            Platform::String^ xamlString =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Canvas.Resources>"
                L"        <SolidColorBrush x:Key='RedBrush' Color='Red' />"
                L"        <Style x:Key='S1' TargetType='Button'>"
                L"            <Setter Property='Background' Value='{StaticResource RedBrush}' />"
                L"        </Style>"
                L"        <Style x:Key='S2' TargetType='Button' BasedOn='{StaticResource S1}'>"
                L"            <Setter Property='Height' Value='10' />"
                L"        </Style>"
                L"    </Canvas.Resources>"
                L"    <StackPanel>"
                L"        <Button x:Name='button1' Content='Test' />"
                L"    </StackPanel>"
                L"</Canvas>";

            auto canvas = safe_cast<Canvas^>(XamlReader::Load(xamlString));
            auto button1 = safe_cast<Button^>(canvas->FindName(L"button1"));

            auto basedOn = safe_cast<Style^>(canvas->Resources->Lookup(L"S1"));

            // This will fault in the BasedOn style's setters.
            auto setters = basedOn->Setters;
            VERIFY_ARE_EQUAL(setters->Size, 1U, L"Setters count should be 1");

            // Add setter to BasedOn style.
            auto setter = ref new Setter();
            setter->Property = Control::ForegroundProperty;
            setter->Value = "Blue";
            setters->Append(setter);

            // Set button's style to derived style and verify values.
            auto derived = safe_cast<Style^>(canvas->Resources->Lookup(L"S2"));
            VERIFY_IS_FALSE(derived->IsSealed);
            VERIFY_IS_FALSE(basedOn->IsSealed);
            button1->Style = derived;
            VERIFY_IS_TRUE(derived->IsSealed);
            VERIFY_IS_TRUE(basedOn->IsSealed);
            VERIFY_ARE_EQUAL(setters->Size, 2U, L"Setter count should be 2");
            auto brush = safe_cast<SolidColorBrush^>(button1->Background);
            VERIFY_ARE_EQUAL(brush->Color, Colors::Red, L"Button Background should be Red brush");
            brush = safe_cast<SolidColorBrush^>(button1->Foreground);
            VERIFY_ARE_EQUAL(brush->Color, Colors::Blue, L"Button Foreground should be Blue brush");
            VERIFY_ARE_EQUAL(button1->Height, 10, L"Button Height should be 10");

            // Try to add another setter to BasedOn style.
            // Verify it throws because it's sealed now.
            setter = ref new Setter();
            setter->Property = Control::WidthProperty;
            setter->Value = "10";
            VERIFY_THROWS_WINRT(setters->Append(setter), Platform::COMException^);
        });

        LOG_OUTPUT(L"Fault in and add setter to BasedOn style, then create and apply derived style.");
        RunOnUIThread([this]()
        {
            Platform::String^ xamlString =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Canvas.Resources>"
                L"        <SolidColorBrush x:Key='RedBrush' Color='Red' />"
                L"        <Style x:Key='S1' TargetType='Button'>"
                L"            <Setter Property='Background' Value='{StaticResource RedBrush}' />"
                L"        </Style>"
                L"    </Canvas.Resources>"
                L"    <StackPanel>"
                L"        <Button x:Name='button1' Content='Test' />"
                L"    </StackPanel>"
                L"</Canvas>";

            auto canvas = safe_cast<Canvas^>(XamlReader::Load(xamlString));
            auto button1 = safe_cast<Button^>(canvas->FindName(L"button1"));

            auto basedOn = safe_cast<Style^>(canvas->Resources->Lookup(L"S1"));

            // This will fault in the BasedOn style's setters.
            auto setters = basedOn->Setters;
            VERIFY_ARE_EQUAL(setters->Size, 1U, L"Setters count should be 1");

            // Add setter to BasedOn style.
            auto setter = ref new Setter();
            setter->Property = Control::ForegroundProperty;
            setter->Value = "Blue";
            setters->Append(setter);

            // Create optimized derived style.
            Style^ derived = nullptr;
            {
                RuntimeEnabledFeatureOverride featureStyleOpt(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableStyleOptimization, true);

                derived = ref new Style();
                derived->TargetType = Button::typeid;
                setter = ref new Setter();
                setter->Property = Control::HeightProperty;
                setter->Value = "10";
                derived->Setters->Append(setter);

                // Set optimized style's BasedOn property to the non-optimized style.
                derived->BasedOn = basedOn;

                // Seal the optimized style.
                // Converts to optimized style.
                VERIFY_IS_FALSE(derived->IsSealed);
                VERIFY_IS_FALSE(basedOn->IsSealed);
                derived->Seal();
                VERIFY_IS_TRUE(derived->IsSealed);
                VERIFY_IS_TRUE(basedOn->IsSealed);
            }

            // Set button's style to derived style and verify values.
            button1->Style = derived;
            VERIFY_ARE_EQUAL(setters->Size, 2U, L"Setter count should be 2");
            auto brush = safe_cast<SolidColorBrush^>(button1->Background);
            VERIFY_ARE_EQUAL(brush->Color, Colors::Red, L"Button Background should be Red brush");
            brush = safe_cast<SolidColorBrush^>(button1->Foreground);
            VERIFY_ARE_EQUAL(brush->Color, Colors::Blue, L"Button Foreground should be Blue brush");
            VERIFY_ARE_EQUAL(button1->Height, 10, L"Button Height should be 10");

            // Try to add another setter to BasedOn style.
            // Verify it throws because it's sealed now.
            setter = ref new Setter();
            setter->Property = Control::WidthProperty;
            setter->Value = "10";
            VERIFY_THROWS_WINRT(setters->Append(setter), Platform::COMException^);
        });

        LOG_OUTPUT(L"Fault in and add setter to derived style after applying BasedOn style.");
        RunOnUIThread([this]()
        {
            Platform::String^ xamlString =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Canvas.Resources>"
                L"        <SolidColorBrush x:Key='RedBrush' Color='Red' />"
                L"        <Style x:Key='S1' TargetType='Button'>"
                L"            <Setter Property='Height' Value='10' />"
                L"        </Style>"
                L"        <Style x:Key='S2' TargetType='Button' BasedOn='{StaticResource S1}'>"
                L"            <Setter Property='Background' Value='{StaticResource RedBrush}' />"
                L"        </Style>"
                L"    </Canvas.Resources>"
                L"    <StackPanel>"
                L"        <Button x:Name='button1' Content='Test' Style='{StaticResource S1}' />"
                L"    </StackPanel>"
                L"</Canvas>";

            auto canvas = safe_cast<Canvas^>(XamlReader::Load(xamlString));
            auto button1 = safe_cast<Button^>(canvas->FindName(L"button1"));

            auto basedOn = safe_cast<Style^>(canvas->Resources->Lookup(L"S1"));
            auto derived = safe_cast<Style^>(canvas->Resources->Lookup(L"S2"));

            VERIFY_IS_TRUE(basedOn->IsSealed);

            // This will fault in the derived style's setters.
            auto setters = derived->Setters;
            VERIFY_ARE_EQUAL(setters->Size, 1U, L"Setters count should be 1");

            // Add setter to derived style.
            auto setter = ref new Setter();
            setter->Property = Control::ForegroundProperty;
            setter->Value = "Blue";
            setters->Append(setter);

            // Set button's style to derived style and verify values.
            VERIFY_IS_FALSE(derived->IsSealed);
            button1->Style = derived;
            VERIFY_IS_TRUE(derived->IsSealed);
            VERIFY_ARE_EQUAL(setters->Size, 2U, L"Setter count should be 2");
            auto brush = safe_cast<SolidColorBrush^>(button1->Background);
            VERIFY_ARE_EQUAL(brush->Color, Colors::Red, L"Button Background should be Red brush");
            brush = safe_cast<SolidColorBrush^>(button1->Foreground);
            VERIFY_ARE_EQUAL(brush->Color, Colors::Blue, L"Button Foreground should be Blue brush");
            VERIFY_ARE_EQUAL(button1->Height, 10, L"Button Height should be 10");

            // Try to add another setter to derived style.
            // Verify it throws because it's sealed now.
            setter = ref new Setter();
            setter->Property = Control::WidthProperty;
            setter->Value = "10";
            VERIFY_THROWS_WINRT(setters->Append(setter), Platform::COMException^);
        });
    }


    void StyleIntegrationTests::PageThemeResourceCustomTargetProperty()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Reference as local value.");
        Panel^ panel = nullptr;
        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      xmlns:local = 'using:Tests.Native.External.Framework'>"
                L"    <StackPanel.Resources>"
                L"      <ResourceDictionary>"
                L"        <ResourceDictionary.ThemeDictionaries>"
                L"          <ResourceDictionary x:Key='Dark'>"
                L"            <Thickness x:Key='MyThickness'>2,2,2,2</Thickness>"
                L"          </ResourceDictionary>"
                L"          <ResourceDictionary x:Key='Light'>"
                L"            <Thickness x:Key='MyThickness'>3,3,3,3</Thickness>"
                L"          </ResourceDictionary>"
                L"        </ResourceDictionary.ThemeDictionaries>"
                L"      </ResourceDictionary>"
                L"    </StackPanel.Resources>"
                L"    <local:CustomUserControl CustomThickness='{ThemeResource MyThickness}' />"
                L"    <StackPanel>"
                L"      <StackPanel.Resources>"
                L"        <ResourceDictionary>"
                L"          <ResourceDictionary.ThemeDictionaries>"
                L"            <ResourceDictionary x:Key='Dark'>"
                L"              <Thickness x:Key='MyThickness'>4,4,4,4</Thickness>"
                L"            </ResourceDictionary>"
                L"            <ResourceDictionary x:Key='Light'>"
                L"              <Thickness x:Key='MyThickness'>5,5,5,5</Thickness>"
                L"            </ResourceDictionary>"
                L"          </ResourceDictionary.ThemeDictionaries>"
                L"        </ResourceDictionary>"
                L"      </StackPanel.Resources>"
                L"      <local:CustomUserControl CustomThickness='{ThemeResource MyThickness}' />"
                L"    </StackPanel>"
                L"</StackPanel>";

            panel = safe_cast<Panel^>(XamlReader::Load(xamlString));

            panel->RequestedTheme = ElementTheme::Dark;

            TestServices::WindowHelper->WindowContent = panel;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            {
                auto innerPanel = safe_cast<Panel^>(panel->Children->GetAt(1));
                auto button2 = safe_cast<CustomUserControl^>(innerPanel->Children->GetAt(0));
                VERIFY_IS_TRUE(ThicknessHelper::FromUniformLength(4) == button2->CustomThickness);

                auto button1 = safe_cast<CustomUserControl^>(panel->Children->GetAt(0));
                VERIFY_IS_TRUE(ThicknessHelper::FromUniformLength(2) == button1->CustomThickness);
            }

            // move the inner button to the root panel
            {
                auto innerPanel = safe_cast<Panel^>(panel->Children->GetAt(1));
                auto button2 = safe_cast<CustomUserControl^>(innerPanel->Children->GetAt(0));
                innerPanel->Children->RemoveAt(0);
                panel->Children->Append(button2);
            }

            panel->RequestedTheme = ElementTheme::Light;

            {
                auto button2 = safe_cast<CustomUserControl^>(panel->Children->GetAt(2));
                VERIFY_IS_TRUE(ThicknessHelper::FromUniformLength(3) == button2->CustomThickness);

                auto button1 = safe_cast<CustomUserControl^>(panel->Children->GetAt(0));
                VERIFY_IS_TRUE(ThicknessHelper::FromUniformLength(3) == button1->CustomThickness);
            }

            panel = nullptr;
            TestServices::WindowHelper->WindowContent = nullptr;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Reference via theme style.");
        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      xmlns:local = 'using:Tests.Native.External.Framework'>"
                L"    <StackPanel.Resources>"
                L"      <ResourceDictionary>"
                L"        <ResourceDictionary.ThemeDictionaries>"
                L"          <ResourceDictionary x:Key='Dark'>"
                L"            <Thickness x:Key='MyThickness'>2,2,2,2</Thickness>"
                L"            <Style x:Key='myStyle' TargetType='local:CustomUserControl'>"
                L"                <Setter Property='CustomThickness' Value='{ThemeResource MyThickness}' />"
                L"            </Style>"
                L"          </ResourceDictionary>"
                L"          <ResourceDictionary x:Key='Light'>"
                L"            <Thickness x:Key='MyThickness'>3,3,3,3</Thickness>"
                L"            <Style x:Key='myStyle' TargetType='local:CustomUserControl'>"
                L"                <Setter Property='CustomThickness' Value='{ThemeResource MyThickness}' />"
                L"            </Style>"
                L"          </ResourceDictionary>"
                L"        </ResourceDictionary.ThemeDictionaries>"
                L"      </ResourceDictionary>"
                L"    </StackPanel.Resources>"
                L"    <local:CustomUserControl Style='{ThemeResource myStyle}' />"
                L"    <StackPanel>"
                L"      <StackPanel.Resources>"
                L"        <ResourceDictionary>"
                L"          <ResourceDictionary.ThemeDictionaries>"
                L"            <ResourceDictionary x:Key='Dark'>"
                L"              <Thickness x:Key='MyThickness'>4,4,4,4</Thickness>"
                L"            </ResourceDictionary>"
                L"            <ResourceDictionary x:Key='Light'>"
                L"              <Thickness x:Key='MyThickness'>5,5,5,5</Thickness>"
                L"            </ResourceDictionary>"
                L"          </ResourceDictionary.ThemeDictionaries>"
                L"        </ResourceDictionary>"
                L"      </StackPanel.Resources>"
                L"      <local:CustomUserControl Style='{ThemeResource myStyle}' />"
                L"    </StackPanel>"
                L"</StackPanel>";

            panel = safe_cast<Panel^>(XamlReader::Load(xamlString));

            panel->RequestedTheme = ElementTheme::Dark;

            TestServices::WindowHelper->WindowContent = panel;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            {
                auto innerPanel = safe_cast<Panel^>(panel->Children->GetAt(1));
                auto button2 = safe_cast<CustomUserControl^>(innerPanel->Children->GetAt(0));
                VERIFY_IS_TRUE(ThicknessHelper::FromUniformLength(4) == button2->CustomThickness);

                auto button1 = safe_cast<CustomUserControl^>(panel->Children->GetAt(0));
                VERIFY_IS_TRUE(ThicknessHelper::FromUniformLength(2) == button1->CustomThickness);
            }

            // move the inner button to the root panel
            {
                auto innerPanel = safe_cast<Panel^>(panel->Children->GetAt(1));
                auto button2 = safe_cast<CustomUserControl^>(innerPanel->Children->GetAt(0));
                innerPanel->Children->RemoveAt(0);
                panel->Children->Append(button2);
            }

            panel->RequestedTheme = ElementTheme::Light;

            {
                auto button2 = safe_cast<CustomUserControl^>(panel->Children->GetAt(2));
                VERIFY_IS_TRUE(ThicknessHelper::FromUniformLength(3) == button2->CustomThickness);

                auto button1 = safe_cast<CustomUserControl^>(panel->Children->GetAt(0));
                VERIFY_IS_TRUE(ThicknessHelper::FromUniformLength(3) == button1->CustomThickness);
            }

            // go live to force implicit style update
            TestServices::WindowHelper->WindowContent = panel;
        });
    }

    void StyleIntegrationTests::PageThemeResourceCustomSourceObject()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Reference as local value.");
        Panel^ panel = nullptr;
        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      xmlns:local = 'using:Tests.Native.External.Framework'>"
                L"    <StackPanel.Resources>"
                L"      <ResourceDictionary>"
                L"        <ResourceDictionary.ThemeDictionaries>"
                L"          <ResourceDictionary x:Key='Dark'>"
                L"            <local:CustomObject x:Key='myCustomObject' />"
                L"            <local:CustomEnumValues x:Key='myCustomEnum'>1</local:CustomEnumValues>"
                L"          </ResourceDictionary>"
                L"          <ResourceDictionary x:Key='Light'>"
                L"            <local:CustomObject x:Key='myCustomObject' />"
                L"            <local:CustomEnumValues x:Key='myCustomEnum'>0</local:CustomEnumValues>"
                L"          </ResourceDictionary>"
                L"        </ResourceDictionary.ThemeDictionaries>"
                L"      </ResourceDictionary>"
                L"    </StackPanel.Resources>"
                L"    <local:CustomControl WorkingTag='{ThemeResource myCustomObject}' "
                L"                         CustomEnum='{ThemeResource myCustomEnum}' />"
                L"</StackPanel>";

            CustomObject::ClearInstanceCount();

            panel = safe_cast<Panel^>(XamlReader::Load(xamlString));

            panel->RequestedTheme = ElementTheme::Dark;

            TestServices::WindowHelper->WindowContent = panel;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            auto control = safe_cast<CustomControl^>(panel->Children->GetAt(0));

            VERIFY_IS_NOT_NULL(control->WorkingTag);
            VERIFY_ARE_EQUAL(CustomObject::GetInstanceCount(), 1U);
            VERIFY_IS_TRUE(control->CustomEnum == CustomEnumValues::CustomEnumValue1);

            panel->RequestedTheme = ElementTheme::Light;

            VERIFY_IS_NOT_NULL(control->WorkingTag);
            VERIFY_ARE_EQUAL(CustomObject::GetInstanceCount(), 2U);
            VERIFY_IS_TRUE(control->CustomEnum == CustomEnumValues::CustomEnumValue0);

            panel = nullptr;
            TestServices::WindowHelper->WindowContent = nullptr;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Reference via theme style.");
        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      xmlns:local = 'using:Tests.Native.External.Framework'>"
                L"    <StackPanel.Resources>"
                L"      <ResourceDictionary>"
                L"        <ResourceDictionary.ThemeDictionaries>"
                L"          <ResourceDictionary x:Key='Dark'>"
                L"            <local:CustomObject x:Key='myCustomObject' />"
                L"            <Style x:Key='myStyle' TargetType='local:CustomControl'>"
                L"                <Setter Property='WorkingTag' Value='{ThemeResource myCustomObject}' />"
                L"            </Style>"
                L"          </ResourceDictionary>"
                L"          <ResourceDictionary x:Key='Light'>"
                L"            <local:CustomEnumValues x:Key='myCustomEnum'>1</local:CustomEnumValues>"
                L"            <Style x:Key='myStyle' TargetType='local:CustomControl'>"
                L"                <Setter Property='CustomEnum' Value='{ThemeResource myCustomEnum}' />"
                L"            </Style>"
                L"          </ResourceDictionary>"
                L"        </ResourceDictionary.ThemeDictionaries>"
                L"      </ResourceDictionary>"
                L"    </StackPanel.Resources>"
                L"    <local:CustomControl Style='{ThemeResource myStyle}' />"
                L"</StackPanel>";

            CustomObject::ClearInstanceCount();

            panel = safe_cast<Panel^>(XamlReader::Load(xamlString));

            panel->RequestedTheme = ElementTheme::Dark;

            TestServices::WindowHelper->WindowContent = panel;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            auto control = safe_cast<CustomControl^>(panel->Children->GetAt(0));

            VERIFY_IS_NOT_NULL(control->WorkingTag);
            VERIFY_ARE_EQUAL(CustomObject::GetInstanceCount(), 1U);

            panel->RequestedTheme = ElementTheme::Light;

            VERIFY_IS_NULL(control->WorkingTag);
            VERIFY_IS_TRUE(control->CustomEnum == CustomEnumValues::CustomEnumValue1);

        });
    }

    void StyleIntegrationTests::StyleSetterWithUid()
    {
        TestCleanupWrapper cleanup;

        // Create resource replacement values that will be used
        // for x:Uid at load time via the override.
        std::map<std::wstring, std::vector<std::pair<std::wstring, std::wstring>>> map;
        std::vector<std::pair<std::wstring, std::wstring>> entries;
        entries.push_back(std::make_pair(L"Value", L"Blue"));
        map.emplace(L"Setter1", std::move(entries));
        std::vector<std::pair<std::wstring, std::wstring>> entries2;
        entries.push_back(std::make_pair(L"Property", L"Width"));
        entries.push_back(std::make_pair(L"Value", L"200"));
        map.emplace(L"Setter2", std::move(entries));

        // Verify a setter value is replaced with a given resource replacement value.
        // Check with both XBF enabled and disabled.
        Platform::String^ xaml1 =
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"    <StackPanel.Style>"
            L"        <Style TargetType='StackPanel'>"
            L"            <Style.Setters>"
            L"                <Setter Property='Background' Value='Red' x:Uid='Setter1' />"
            L"            </Style.Setters>"
            L"        </Style>"
            L"    </StackPanel.Style>"
            L"</StackPanel>";

        RunOnUIThread([&]()
        {
            RuntimeEnabledFeatureOverride featureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);
            XamlResourcePropertyBagOverrider propertyBagOverride(&map);

            Panel^ panel = safe_cast<Panel^>(XamlReader::Load(xaml1));
            auto brush = safe_cast<SolidColorBrush^>(panel->Background);
            VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);
        });

        RunOnUIThread([&]()
        {
            RuntimeEnabledFeatureOverride featureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);
            XamlResourcePropertyBagOverrider propertyBagOverride(&map);

            Panel^ panel = safe_cast<Panel^>(XamlReader::Load(xaml1));
            auto brush = safe_cast<SolidColorBrush^>(panel->Background);
            VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);

            TestServices::WindowHelper->WindowContent = panel;
        });

        // Verify multiple setters' values are replaced with given resource replacement values,
        // and surrounding setters without x:Uid also work as expected.
        // Check with both XBF enabled and disabled.
        Platform::String^ xaml2 =
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"    <StackPanel.Style>"
            L"        <Style TargetType='StackPanel'>"
            L"            <Style.Setters>"
            L"                <Setter Property='Height' Value='100' />"
            L"                <Setter Property='Background' Value='Red' x:Uid='Setter1' />"
            L"                <Setter Property='Width' Value='100' />"
            L"                <Setter Property='Height' Value='100' x:Uid='Setter2' />"
            L"                <Setter Property='Opacity' Value='0.5' />"
            L"            </Style.Setters>"
            L"        </Style>"
            L"    </StackPanel.Style>"
            L"</StackPanel>";

        RunOnUIThread([&]()
        {
            RuntimeEnabledFeatureOverride featureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);
            XamlResourcePropertyBagOverrider propertyBagOverride(&map);

            Panel^ panel = safe_cast<Panel^>(XamlReader::Load(xaml2));
            auto brush = safe_cast<SolidColorBrush^>(panel->Background);
            VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);
            VERIFY_ARE_EQUAL(100, panel->Height);
            VERIFY_ARE_EQUAL(200, panel->Width);
            VERIFY_ARE_EQUAL(0.5, panel->Opacity);
        });

        RunOnUIThread([&]()
        {
            RuntimeEnabledFeatureOverride featureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);
            XamlResourcePropertyBagOverrider propertyBagOverride(&map);

            Panel^ panel = safe_cast<Panel^>(XamlReader::Load(xaml2));
            auto brush = safe_cast<SolidColorBrush^>(panel->Background);
            VERIFY_ARE_EQUAL(Colors::Blue, brush->Color);
            VERIFY_ARE_EQUAL(100, panel->Height);
            VERIFY_ARE_EQUAL(200, panel->Width);
            VERIFY_ARE_EQUAL(0.5, panel->Opacity);
        });

        // Verify setters with x:Uid continue working when matching replacement values aren't provided.
        // Check with both XBF enabled and disabled.
        RunOnUIThread([&]()
        {
            RuntimeEnabledFeatureOverride featureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);

            Panel^ panel = safe_cast<Panel^>(XamlReader::Load(xaml2));
            auto brush = safe_cast<SolidColorBrush^>(panel->Background);
            VERIFY_ARE_EQUAL(Colors::Red, brush->Color);
            VERIFY_ARE_EQUAL(100, panel->Height);
            VERIFY_ARE_EQUAL(100, panel->Width);
            VERIFY_ARE_EQUAL(0.5, panel->Opacity);
        });

        RunOnUIThread([&]()
        {
            RuntimeEnabledFeatureOverride featureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);

            Panel^ panel = safe_cast<Panel^>(XamlReader::Load(xaml2));
            auto brush = safe_cast<SolidColorBrush^>(panel->Background);
            VERIFY_ARE_EQUAL(Colors::Red, brush->Color);
            VERIFY_ARE_EQUAL(100, panel->Height);
            VERIFY_ARE_EQUAL(100, panel->Width);
            VERIFY_ARE_EQUAL(0.5, panel->Opacity);
        });
    }

    void StyleIntegrationTests::StyleSetterWithBadUidProperty()
    {
        TestCleanupWrapper cleanup;

        std::map<std::wstring, std::vector<std::pair<std::wstring, std::wstring>>> map;
        std::vector<std::pair<std::wstring, std::wstring>> entries;
        entries.push_back(std::make_pair(L"Foo", L"Blue"));
        map.emplace(L"Setter1", std::move(entries));

        // Verify an exception occurs when a given resource replacement property name
        // isn't a valid Setter property. Check with both XBF enabled and disabled.
        Platform::String^ xaml1 =
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"    <StackPanel.Style>"
            L"        <Style TargetType='StackPanel'>"
            L"            <Style.Setters>"
            L"                <Setter Property='Background' Value='Red' x:Uid='Setter1' />"
            L"            </Style.Setters>"
            L"        </Style>"
            L"    </StackPanel.Style>"
            L"</StackPanel>";

        RunOnUIThread([&]()
        {
            RuntimeEnabledFeatureOverride featureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);
            XamlResourcePropertyBagOverrider propertyBagOverride(&map);
            VERIFY_THROWS_WINRT(XamlReader::Load(xaml1), Platform::Exception^);
        });

        RunOnUIThread([&]()
        {
            RuntimeEnabledFeatureOverride featureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);
            XamlResourcePropertyBagOverrider propertyBagOverride(&map);
            VERIFY_THROWS_WINRT(XamlReader::Load(xaml1), Platform::Exception^);
        });
    }

    void StyleIntegrationTests::StyleChangeWithThemeRefs()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"Verify that property values update correctly when a tree change occurs with an element "
                   L"that has theme refs applied via a style that itself is resolved from a theme ref.");
        {
            Page^ page = nullptr;
            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"        <Page.Resources>"
                    L"            <SolidColorBrush x:Key='MyBrush' Color='Red' />"
                    L"            <FontWeight x:Key='MyFontWeight'>Normal</FontWeight>"
                    L"            <x:Double x:Key='MyFontSize'>40</x:Double>"
                    L"        </Page.Resources>"
                    L"    <StackPanel>"
                    L"       <UserControl x:Name='userControl1'>"
                    L"            <UserControl.Resources>"
                    L"                <Style x:Key='MyStyle' TargetType='TextBlock'>"
                    L"                    <Setter Property='Foreground' Value='{ThemeResource MyBrush}'/>"
                    L"                </Style>"
                    L"            </UserControl.Resources>"
                    L"            <TextBlock x:Name='textBlock1' Text='Title' Style='{ThemeResource MyStyle}' TextWrapping='NoWrap'/>"
                    L"       </UserControl>"
                    L"       <UserControl x:Name='userControl2'>"
                    L"            <UserControl.Resources>"
                    L"                <Style x:Key='MyStyle' TargetType='TextBlock'>"
                    L"                    <Setter Property='FontSize' Value='{ThemeResource MyFontSize}' />"
                    L"                    <Setter Property='FontWeight' Value='{ThemeResource MyFontWeight}' />"
                    L"                </Style>"
                    L"            </UserControl.Resources>"
                    L"            <TextBlock x:Name='textBlock2' Text='Title' Style='{ThemeResource MyStyle}' TextWrapping='NoWrap'/>"
                    L"       </UserControl>"
                    L"    </StackPanel>"
                    L"</Page>";

                page = safe_cast<Page^>(XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = page;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto panel = safe_cast<Panel^>(page->Content);
                auto userControl1 = safe_cast<UserControl^>(panel->Children->GetAt(0));
                auto userControl2 = safe_cast<UserControl^>(panel->Children->GetAt(1));
                auto textBlock1 = safe_cast<TextBlock^>(userControl1->Content);
                auto textBlock2 = safe_cast<TextBlock^>(userControl2->Content);
                auto brush1 = safe_cast<SolidColorBrush^>(textBlock1->Foreground);
                auto brush2 = safe_cast<SolidColorBrush^>(textBlock2->Foreground);
                VERIFY_ARE_EQUAL(Colors::Red, brush1->Color);
                VERIFY_ARE_NOT_EQUAL(Colors::Red, brush2->Color);
                VERIFY_ARE_NOT_EQUAL(40.0, textBlock1->FontSize);
                VERIFY_ARE_EQUAL(40.0, textBlock2->FontSize);

                // Swap tree locations of the TextBlocks
                userControl1->Content = nullptr;
                userControl2->Content = nullptr;
                userControl1->Content = textBlock2;
                userControl2->Content = textBlock1;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto panel = safe_cast<Panel^>(page->Content);
                auto userControl1 = safe_cast<UserControl^>(panel->Children->GetAt(0));
                auto userControl2 = safe_cast<UserControl^>(panel->Children->GetAt(1));
                auto textBlock2 = safe_cast<TextBlock^>(userControl1->Content);
                auto textBlock1 = safe_cast<TextBlock^>(userControl2->Content);
                auto brush1 = safe_cast<SolidColorBrush^>(textBlock1->Foreground);
                auto brush2 = safe_cast<SolidColorBrush^>(textBlock2->Foreground);
                VERIFY_ARE_EQUAL(Colors::Red, brush2->Color);
                VERIFY_ARE_NOT_EQUAL(Colors::Red, brush1->Color);
                VERIFY_ARE_NOT_EQUAL(40.0, textBlock2->FontSize);
                VERIFY_ARE_EQUAL(40.0, textBlock1->FontSize);

                // Swap tree locations of the TextBlocks again
                userControl1->Content = nullptr;
                userControl2->Content = nullptr;
                userControl1->Content = textBlock1;
                userControl2->Content = textBlock2;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto panel = safe_cast<Panel^>(page->Content);
                auto userControl1 = safe_cast<UserControl^>(panel->Children->GetAt(0));
                auto userControl2 = safe_cast<UserControl^>(panel->Children->GetAt(1));
                auto textBlock1 = safe_cast<TextBlock^>(userControl1->Content);
                auto textBlock2 = safe_cast<TextBlock^>(userControl2->Content);
                auto brush1 = safe_cast<SolidColorBrush^>(textBlock1->Foreground);
                auto brush2 = safe_cast<SolidColorBrush^>(textBlock2->Foreground);
                VERIFY_ARE_EQUAL(Colors::Red, brush1->Color);
                VERIFY_ARE_NOT_EQUAL(Colors::Red, brush2->Color);
                VERIFY_ARE_NOT_EQUAL(40.0, textBlock1->FontSize);
                VERIFY_ARE_EQUAL(40.0, textBlock2->FontSize);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        LOG_OUTPUT(L"Verify that property values update correctly when a theme change occurs with an element "
                   L"that has theme refs applied via a style that itself is resolved from a theme ref.");
        {
            Page^ page = nullptr;
            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"        <Page.Resources>"
                    L"            <SolidColorBrush x:Key='MyBrush' Color='Red' />"
                    L"            <FontWeight x:Key='MyFontWeight'>Normal</FontWeight>"
                    L"            <x:Double x:Key='MyFontSize'>40</x:Double>"
                    L"        </Page.Resources>"
                    L"    <StackPanel>"
                    L"       <UserControl x:Name='userControl1'>"
                    L"            <UserControl.Resources>"
                    L"                <ResourceDictionary>"
                    L"                    <ResourceDictionary.ThemeDictionaries>"
                    L"                        <ResourceDictionary x:Key='Dark'>"
                    L"                            <Style x:Key='MyStyle' TargetType='TextBlock'>"
                    L"                                <Setter Property='Foreground' Value='{ThemeResource MyBrush}'/>"
                    L"                            </Style>"
                    L"                        </ResourceDictionary>"
                    L"                        <ResourceDictionary x:Key='Light'>"
                    L"                            <Style x:Key='MyStyle' TargetType='TextBlock'>"
                    L"                                <Setter Property='FontSize' Value='{ThemeResource MyFontSize}' />"
                    L"                                <Setter Property='FontWeight' Value='{ThemeResource MyFontWeight}' />"
                    L"                            </Style>"
                    L"                        </ResourceDictionary>"
                    L"                   </ResourceDictionary.ThemeDictionaries>"
                    L"                </ResourceDictionary>"
                    L"            </UserControl.Resources>"
                    L"            <TextBlock x:Name='textBlock' Text='Title' Style='{ThemeResource MyStyle}' TextWrapping='NoWrap'/>"
                    L"       </UserControl>"
                    L"    </StackPanel>"
                    L"</Page>";

                TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Dark);

                page = safe_cast<Page^>(XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = page;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                auto panel = safe_cast<Panel^>(page->Content);
                auto userControl = safe_cast<UserControl^>(panel->Children->GetAt(0));
                auto textBlock = safe_cast<TextBlock^>(userControl->Content);
                auto brush = safe_cast<SolidColorBrush^>(textBlock->Foreground);
                VERIFY_ARE_EQUAL(Colors::Red, brush->Color);
                VERIFY_ARE_NOT_EQUAL(40.0, textBlock->FontSize);

                TestServices::ThemingHelper->SetApplicationRequestedTheme(ApplicationTheme::Light);

                brush = safe_cast<SolidColorBrush^>(textBlock->Foreground);
                VERIFY_ARE_NOT_EQUAL(Colors::Red, brush->Color);
                VERIFY_ARE_EQUAL(40.0, textBlock->FontSize);
            });
        }
    }

    void StyleIntegrationTests::StandardStylesTest1()
    {
        TestCleanupWrapper cleanup;

        StackPanel^ root;
        StackPanel^ stackPanel;
        TextBlock^ tb_BaseTextBlockStyle;
        TextBlock^ tb_HeaderTextBlockStyle;
        TextBlock^ tb_SubheaderTextBlockStyle;
        TextBlock^ tb_TitleTextBlockStyle;
        TextBlock^ tb_BodyTextBlockStyle;

        RichTextBlock^ rtb_BaseRichTextBlockStyle;
        RichTextBlock^ rtb_BodyRichTextBlockStyle;

        TextBlock^ tb_SubtitleTextBlockStyle;
        TextBlock^ tb_CaptionTextBlockStyle;

        Button^ btn_NavigationBackButtonNormalStyle;
        Button^ btn_NavigationBackButtonSmallStyle;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<StackPanel"
                L"      xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"      xmlns:local='using:JupiterSUnitDrts.ControlExtensibility.Styles'"
                L"      Width='800' Height='600'"
                L"      x:Name='LayoutRoot'>"
                L"  <Grid>"
                L"      <Grid.RowDefinitions>"
                L"          <RowDefinition Height='Auto'></RowDefinition>"
                L"          <RowDefinition Height='*'></RowDefinition>"
                L"      </Grid.RowDefinitions>"
                L"      <StackPanel Grid.Row='0'>"
                L"          <Button x:Name='_btnDebug' Content='Debug1' />"
                L"      </StackPanel>"
                L"      <StackPanel Grid.Row='1' x:Name='Stack1'>"
                L"          <TextBlock x:Name='tb_BaseTextBlockStyle' Style = '{StaticResource BaseTextBlockStyle}' Text = 'Test BaseTextBlockStyle' />"
                L"          <TextBlock x:Name='tb_HeaderTextBlockStyle' Style = '{StaticResource HeaderTextBlockStyle}' Text = 'Test HeaderTextBlockStyle' />"
                L"          <TextBlock x:Name='tb_SubheaderTextBlockStyle' Style = '{StaticResource SubheaderTextBlockStyle}' Text = 'Test SubheaderTextBlockStyle' />"
                L"          <TextBlock x:Name='tb_TitleTextBlockStyle' Style = '{StaticResource TitleTextBlockStyle}' Text = 'Test TitleTextBlockStyle' />"
                L"          <TextBlock x:Name='tb_SubtitleTextBlockStyle' Style = '{StaticResource SubtitleTextBlockStyle}' Text = 'Test SubtitleTextBlockStyle' />"
                L"          <TextBlock x:Name='tb_BodyTextBlockStyle' Style = '{StaticResource BodyTextBlockStyle}' Text = 'Test BodyTextBlockStyle' />"
                L"          <TextBlock x:Name='tb_CaptionTextBlockStyle' Style = '{StaticResource CaptionTextBlockStyle}' Text = 'Test CaptionTextBlockStyle' />"
                L"          <RichTextBlock x:Name = 'rtb_BaseRichTextBlockStyle' Style = '{StaticResource BaseRichTextBlockStyle}'>"
                L"              <Paragraph>Testing BaseRichTextBlockStyle</Paragraph>"
                L"          </RichTextBlock>"
                L"          <RichTextBlock x:Name = 'rtb_BodyRichTextBlockStyle' Style = '{StaticResource BodyRichTextBlockStyle}'>"
                L"              <Paragraph>Testing BodyRichTextBlockStyle</Paragraph>"
                L"          </RichTextBlock>"
                L"          <Button x:Name = 'btn_NavigationBackButtonNormalStyle' Style = '{StaticResource NavigationBackButtonNormalStyle}'>"
                L"          </Button>"
                L"          <Button x:Name = 'btn_NavigationBackButtonSmallStyle' Style = '{StaticResource NavigationBackButtonSmallStyle}'>"
                L"          </Button>"
                L"      </StackPanel>"
                L"  </Grid>"
                L"</StackPanel>";

            root = safe_cast<StackPanel^>(XamlReader::Load(xamlString));
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            stackPanel = safe_cast<StackPanel^>(root->FindName(L"Stack1"));
            tb_BaseTextBlockStyle = safe_cast<TextBlock^>(stackPanel->FindName(L"tb_BaseTextBlockStyle"));
            tb_HeaderTextBlockStyle = safe_cast<TextBlock^>(stackPanel->FindName(L"tb_HeaderTextBlockStyle"));
            tb_SubheaderTextBlockStyle = safe_cast<TextBlock^>(stackPanel->FindName(L"tb_SubheaderTextBlockStyle"));
            tb_TitleTextBlockStyle = safe_cast<TextBlock^>(stackPanel->FindName(L"tb_TitleTextBlockStyle"));
            tb_BodyTextBlockStyle = safe_cast<TextBlock^>(stackPanel->FindName(L"tb_BodyTextBlockStyle"));

            rtb_BaseRichTextBlockStyle = safe_cast<RichTextBlock^>(stackPanel->FindName(L"rtb_BaseRichTextBlockStyle"));
            rtb_BodyRichTextBlockStyle = safe_cast<RichTextBlock^>(stackPanel->FindName(L"rtb_BodyRichTextBlockStyle"));

            tb_SubtitleTextBlockStyle = safe_cast<TextBlock^>(stackPanel->FindName(L"tb_SubtitleTextBlockStyle"));
            tb_CaptionTextBlockStyle = safe_cast<TextBlock^>(stackPanel->FindName(L"tb_CaptionTextBlockStyle"));

            btn_NavigationBackButtonNormalStyle = safe_cast<Button^>(stackPanel->FindName(L"btn_NavigationBackButtonNormalStyle"));
            btn_NavigationBackButtonSmallStyle = safe_cast<Button^>(stackPanel->FindName(L"btn_NavigationBackButtonSmallStyle"));

            // BaseTextBlockStyle
            VERIFY_ARE_EQUAL(TextTrimming::CharacterEllipsis, tb_BaseTextBlockStyle->TextTrimming, L"tb_BaseTextBlockStyle.TextTrimming was not applied correctly.");
            VERIFY_ARE_EQUAL(TextLineBounds::Full, tb_BaseTextBlockStyle->TextLineBounds, L"tb_BaseTextBlockStyle.TextLineBounds was not applied correctly.");
            VERIFY_ARE_EQUAL(OpticalMarginAlignment::None, tb_BaseTextBlockStyle->OpticalMarginAlignment, L"tb_BaseTextBlockStyle.OpticalMarginAlignment was not applied correctly.");

            // HeaderTextBlockStyle
            VERIFY_ARE_EQUAL(46, tb_HeaderTextBlockStyle->FontSize, L"tb_HeaderTextBlockStyle was not applied correctly.");

            // SubheaderTextBlockStyle
            VERIFY_ARE_EQUAL(FontWeights::Light.Weight, tb_SubheaderTextBlockStyle->FontWeight.Weight, L"tb_SubheaderTextBlockStyle was not applied correctly.");

            // TitleTextBlockStyle
            VERIFY_ARE_EQUAL(FontWeights::SemiBold.Weight, tb_TitleTextBlockStyle->FontWeight.Weight, L"tb_TitleTextBlockStyle was not applied correctly.");

            // BodyTextBlockStyle
            VERIFY_ARE_EQUAL(FontWeights::Normal.Weight, tb_BodyTextBlockStyle->FontWeight.Weight, L"tb_BodyTextBlockStyle was not applied correctly.");

            // BaseRichTextBlockStyle
            VERIFY_ARE_EQUAL(TextTrimming::None, rtb_BaseRichTextBlockStyle->TextTrimming, L"rtb_BaseRichTextBlockStyle.TextTrimming was not applied correctly.");
            VERIFY_ARE_EQUAL(TextWrapping::Wrap, rtb_BaseRichTextBlockStyle->TextWrapping, L"rtb_BaseRichTextBlockStyle.TextWrapping was not applied correctly.");
            VERIFY_ARE_EQUAL(TextLineBounds::Full, rtb_BaseRichTextBlockStyle->TextLineBounds, L"rtb_BaseRichTextBlockStyle.TextLineBounds was not applied correctly.");
            VERIFY_ARE_EQUAL(OpticalMarginAlignment::TrimSideBearings, rtb_BaseRichTextBlockStyle->OpticalMarginAlignment, L"rtb_BaseRichTextBlockStyle.OpticalMarginAlignment was not applied correctly.");

            // BodyRichTextBlockStyle
            VERIFY_ARE_EQUAL(FontWeights::Normal.Weight, rtb_BodyRichTextBlockStyle->FontWeight.Weight, L"rtb_BodyRichTextBlockStyle was not applied correctly.");

            // SubtitleTextBlockStyle
            VERIFY_ARE_EQUAL(FontWeights::SemiBold.Weight, tb_SubtitleTextBlockStyle->FontWeight.Weight, L"tb_SubtitleTextBlockStyle was not applied correctly.");

            // CaptionTextBlockStyle
            VERIFY_ARE_EQUAL(12, tb_CaptionTextBlockStyle->FontSize, L"tb_CaptionTextBlockStyle was not applied correctly.");
            VERIFY_ARE_EQUAL(FontWeights::Normal.Weight, tb_CaptionTextBlockStyle->FontWeight.Weight, L"tb_CaptionTextBlockStyle was not applied correctly.");

            // NavigationBackButtonNormal/SmallStyle
            VERIFY_ARE_EQUAL(FontWeights::Normal.Weight, btn_NavigationBackButtonNormalStyle->FontWeight.Weight, L"btn_NavigationBackButtonNormalStyle was not applied correctly.");
            VERIFY_ARE_EQUAL(FontWeights::Normal.Weight, btn_NavigationBackButtonSmallStyle->FontWeight.Weight, L"btn_NavigationBackButtonSmallStyle was not applied correctly.");
        });
    }

    void StyleIntegrationTests::StandardStylesTest2()
    {
        TestCleanupWrapper cleanup;

        StackPanel^ root;
        StackPanel^ stackPanel;
        TextBlock^ tb_BaseTextBlockStyle;
        TextBlock^ tb_HeaderTextBlockStyle;
        TextBlock^ tb_SubheaderTextBlockStyle;
        TextBlock^ tb_TitleTextBlockStyle;
        TextBlock^ tb_SubtitleTextBlockStyle;
        TextBlock^ tb_BodyTextBlockStyle;
        TextBlock^ tb_CaptionTextBlockStyle;

        RichTextBlock^ rtb_BaseRichTextBlockStyle;
        RichTextBlock^ rtb_BodyRichTextBlockStyle;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<StackPanel"
                L"    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"    xmlns:local='using:JupiterSUnitDrts.ControlExtensibility.Styles'"
                L"    Width='800' Height='600'"
                L"    x:Name='LayoutRoot'>"
                L"  <StackPanel.Resources>"
                L"    <Style x:Key='BaseTextBlockStyle' TargetType='TextBlock'>"
                L"      <Setter Property='Foreground' Value='{ThemeResource ApplicationForegroundThemeBrush}'/>"
                L"      <Setter Property='FontSize' Value='{ThemeResource ControlContentThemeFontSize}'/>"
                L"      <Setter Property='FontFamily' Value='{ThemeResource ContentControlThemeFontFamily}'/>"
                L"      <Setter Property='TextTrimming' Value='CharacterEllipsis'/>"
                L"      <Setter Property='TextWrapping' Value='Wrap'/>"
                L"      <Setter Property='Typography.StylisticSet20' Value='True'/>"
                L"      <Setter Property='Typography.DiscretionaryLigatures' Value='True'/>"
                L"      <Setter Property='Typography.CaseSensitiveForms' Value='True'/>"
                L"      <Setter Property='LineHeight' Value='20'/>"
                L"      <Setter Property='LineStackingStrategy' Value='BlockLineHeight'/>"
                L"    </Style>"
                L"    <Style x:Key='HeaderTextBlockStyle' TargetType='TextBlock' BasedOn='{StaticResource BaseTextBlockStyle}'>"
                L"      <Setter Property='FontSize' Value='76'/>"
                L"      <Setter Property='FontWeight' Value='Light'/>"
                L"      <Setter Property='LineHeight' Value='40'/>"
                L"      <Setter Property='RenderTransform'>"
                L"        <Setter.Value>"
                L"          <TranslateTransform X='-2' Y='8'/>"
                L"        </Setter.Value>"
                L"      </Setter>"
                L"    </Style>"
                L"    <Style x:Key='SubheaderTextBlockStyle' TargetType='TextBlock' BasedOn='{StaticResource BaseTextBlockStyle}'>"
                L"      <Setter Property='FontSize' Value='26.667'/>"
                L"      <Setter Property='FontWeight' Value='SemiLight'/>"
                L"      <Setter Property='LineHeight' Value='30'/>"
                L"      <Setter Property='RenderTransform'>"
                L"        <Setter.Value>"
                L"          <TranslateTransform X='-1' Y='6'/>"
                L"        </Setter.Value>"
                L"      </Setter>"
                L"    </Style>"
                L"    <Style x:Key='TitleTextBlockStyle' TargetType='TextBlock' BasedOn='{StaticResource BaseTextBlockStyle}'>"
                L"      <Setter Property='FontWeight' Value='Light'/>"
                L"    </Style>"
                L"    <Style x:Key='SubtitleTextBlockStyle' TargetType='TextBlock' BasedOn='{StaticResource BaseTextBlockStyle}'>"
                L"      <Setter Property='FontWeight' Value='SemiBold'/>"
                L"    </Style>"
                L"    <Style x:Key='BodyTextBlockStyle' TargetType='TextBlock' BasedOn='{StaticResource BaseTextBlockStyle}'>"
                L"      <Setter Property='FontWeight' Value='Light'/>"
                L"    </Style>"
                L"    <Style x:Key='CaptionTextBlockStyle' TargetType='TextBlock' BasedOn='{StaticResource BaseTextBlockStyle}'>"
                L"      <Setter Property='FontSize' Value='22'/>"
                L"      <Setter Property='Foreground' Value='{ThemeResource ApplicationSecondaryForegroundThemeBrush}'/>"
                L"    </Style>"
                L"    <Style x:Key='BaseRichTextBlockStyle' TargetType='RichTextBlock'>"
                L"      <Setter Property='Foreground' Value='{ThemeResource ApplicationForegroundThemeBrush}'/>"
                L"      <Setter Property='FontSize' Value='{ThemeResource ControlContentThemeFontSize}'/>"
                L"      <Setter Property='FontFamily' Value='{ThemeResource ContentControlThemeFontFamily}'/>"
                L"      <Setter Property='TextTrimming' Value='WordEllipsis'/>"
                L"      <Setter Property='TextWrapping' Value='NoWrap'/>"
                L"      <Setter Property='Typography.StylisticSet20' Value='True'/>"
                L"      <Setter Property='Typography.DiscretionaryLigatures' Value='True'/>"
                L"      <Setter Property='Typography.CaseSensitiveForms' Value='True'/>"
                L"      <Setter Property='LineHeight' Value='20'/>"
                L"      <Setter Property='LineStackingStrategy' Value='BlockLineHeight'/>"
                L"      <!-- Properly align text along its baseline -->"
                L"      <Setter Property='RenderTransform'>"
                L"        <Setter.Value>"
                L"          <TranslateTransform X='-1' Y='4'/>"
                L"        </Setter.Value>"
                L"      </Setter>"
                L"    </Style>"
                L"    <Style x:Key='BodyRichTextBlockStyle' TargetType='RichTextBlock' BasedOn='{StaticResource BaseRichTextBlockStyle}'>"
                L"      <Setter Property='FontWeight' Value='Light'/>"
                L"    </Style>"
                L"  </StackPanel.Resources>"
                L"  <Button Content='BtnSync' />"
                L"  <Grid>"
                L"    <Grid.RowDefinitions>"
                L"      <RowDefinition Height='Auto'></RowDefinition>"
                L"      <RowDefinition Height='*'></RowDefinition>"
                L"    </Grid.RowDefinitions>"
                L"    <StackPanel Grid.Row='0'>"
                L"      <Button x:Name='_btnDebug' Content='Debug1' />"
                L"    </StackPanel>"
                L"    <StackPanel Grid.Row='1' x:Name='Stack1'>"
                L"      <TextBlock x:Name='tb_BaseTextBlockStyle' Style='{StaticResource BaseTextBlockStyle}' Text='Test BaseTextBlockStyle' />"
                L"      <TextBlock x:Name='tb_HeaderTextBlockStyle' Style='{StaticResource HeaderTextBlockStyle}' Text='Test HeaderTextBlockStyle' />"
                L"      <TextBlock x:Name='tb_SubheaderTextBlockStyle' Style='{StaticResource SubheaderTextBlockStyle}' Text='Test SubheaderTextBlockStyle' />"
                L"      <TextBlock x:Name='tb_TitleTextBlockStyle' Style='{StaticResource TitleTextBlockStyle}' Text='Test TitleTextBlockStyle' />"
                L"      <TextBlock x:Name='tb_SubtitleTextBlockStyle' Style='{StaticResource SubtitleTextBlockStyle}' Text='Test SubtitleTextBlockStyle' />"
                L"      <TextBlock x:Name='tb_BodyTextBlockStyle' Style='{StaticResource BodyTextBlockStyle}' Text='Test BodyTextBlockStyle' />"
                L"      <TextBlock x:Name='tb_CaptionTextBlockStyle' Style='{StaticResource CaptionTextBlockStyle}' Text='Test CaptionTextBlockStyle' />"
                L"      <RichTextBlock x:Name='rtb_BaseRichTextBlockStyle' Style='{StaticResource BaseRichTextBlockStyle}'>"
                L"        <Paragraph>Testing BaseRichTextBlockStyle</Paragraph>"
                L"      </RichTextBlock>"
                L"      <RichTextBlock x:Name='rtb_BodyRichTextBlockStyle' Style='{StaticResource BodyRichTextBlockStyle}'>"
                L"        <Paragraph>Testing BodyRichTextBlockStyle</Paragraph>"
                L"      </RichTextBlock>"
                L"    </StackPanel>"
                L"  </Grid>"
                L"</StackPanel>";

            root = safe_cast<StackPanel^>(XamlReader::Load(xamlString));
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            stackPanel = safe_cast<StackPanel^>(root->FindName(L"Stack1"));
            tb_BaseTextBlockStyle = safe_cast<TextBlock^>(stackPanel->FindName(L"tb_BaseTextBlockStyle"));
            tb_HeaderTextBlockStyle = safe_cast<TextBlock^>(stackPanel->FindName(L"tb_HeaderTextBlockStyle"));
            tb_SubheaderTextBlockStyle = safe_cast<TextBlock^>(stackPanel->FindName(L"tb_SubheaderTextBlockStyle"));
            tb_TitleTextBlockStyle = safe_cast<TextBlock^>(stackPanel->FindName(L"tb_TitleTextBlockStyle"));
            tb_SubtitleTextBlockStyle = safe_cast<TextBlock^>(stackPanel->FindName(L"tb_SubtitleTextBlockStyle"));
            tb_BodyTextBlockStyle = safe_cast<TextBlock^>(stackPanel->FindName(L"tb_BodyTextBlockStyle"));
            tb_CaptionTextBlockStyle = safe_cast<TextBlock^>(stackPanel->FindName(L"tb_CaptionTextBlockStyle"));

            rtb_BaseRichTextBlockStyle = safe_cast<RichTextBlock^>(stackPanel->FindName(L"rtb_BaseRichTextBlockStyle"));
            rtb_BodyRichTextBlockStyle = safe_cast<RichTextBlock^>(stackPanel->FindName(L"rtb_BodyRichTextBlockStyle"));

            VERIFY_ARE_EQUAL(TextTrimming::CharacterEllipsis, tb_BaseTextBlockStyle->TextTrimming, L"tb_BaseTextBlockStyle was not applied correctly.");
            VERIFY_ARE_EQUAL(76, tb_HeaderTextBlockStyle->FontSize, L"tb_HeaderTextBlockStyle was not applied correctly.");
            VERIFY_ARE_EQUAL(FontWeights::SemiLight.Weight, tb_SubheaderTextBlockStyle->FontWeight.Weight, L"tb_SubheaderTextBlockStyle was not applied correctly.");
            VERIFY_ARE_EQUAL(FontWeights::Light.Weight, tb_TitleTextBlockStyle->FontWeight.Weight, L"tb_TitleTextBlockStyle was not applied correctly.");
            VERIFY_ARE_EQUAL(FontWeights::SemiBold.Weight, tb_SubtitleTextBlockStyle->FontWeight.Weight, L"tb_SubtitleTextBlockStyle was not applied correctly.");
            VERIFY_ARE_EQUAL(FontWeights::Light.Weight, tb_BodyTextBlockStyle->FontWeight.Weight, L"tb_BodyTextBlockStyle was not applied correctly.");
            VERIFY_ARE_EQUAL(22, tb_CaptionTextBlockStyle->FontSize, L"tb_CaptionTextBlockStyle was not applied correctly.");

            VERIFY_ARE_EQUAL(TextWrapping::NoWrap, rtb_BaseRichTextBlockStyle->TextWrapping, L"rtb_BaseRichTextBlockStyle was not applied correctly.");
            VERIFY_ARE_EQUAL(FontWeights::Light.Weight, rtb_BodyRichTextBlockStyle->FontWeight.Weight, L"rtb_BodyRichTextBlockStyle was not applied correctly.");

        });
    }

    void StyleIntegrationTests::VerifyImplicitStyleInvalidation()
    {
        TestCleanupWrapper cleanup;

        StackPanel^ root;
        StackPanel^ childPanel;
        TextBlock^ outerTextBlock;
        TextBlock^ nestedTextBlock;

        Platform::String^ markup =
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <StackPanel.Resources>"
            L"    <ResourceDictionary>"
            L"      <Style TargetType='TextBlock'>"
            L"        <Setter Property='Foreground' Value='Black' />"
            L"      </Style>"
            L"    </ResourceDictionary>"
            L"  </StackPanel.Resources>"
            L"  <StackPanel>"
            L"    <StackPanel.Resources>"
            L"      <ResourceDictionary>"
            L"        <Style TargetType='TextBlock'>"
            L"          <Setter Property='Foreground' Value='Blue' />"
            L"        </Style>"
            L"      </ResourceDictionary>"
            L"    </StackPanel.Resources>"
            L"    <TextBlock x:Name='OuterTextBlock' Text='It is up with the blue and gold' />"
            L"    <StackPanel x:Name='ChildPanel'>"
            L"      <StackPanel.Resources>"
            L"        <ResourceDictionary>"
            L"          <Style TargetType='TextBlock'>"
            L"            <Setter Property='Foreground' Value='Red' />"
            L"          </Style>"
            L"        </ResourceDictionary>"
            L"      </StackPanel.Resources>"
            L"      <TextBlock x:Name='NestedTextBlock' Text='Down with the red' />"
            L"    </StackPanel>"
            L"  </StackPanel>"
            L"</StackPanel>";

        Platform::String^ dictionaryWithNoImplicitStyle =
            L"<ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' />";

        Platform::String^ dictionaryWithImplicitStyle =
            L"<ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <Style TargetType='TextBlock'>"
            L"    <Setter Property='Foreground' Value='Pink' />"
            L"  </Style>"
            L"</ResourceDictionary>";

        RunOnUIThread([&]()
        {
            root = safe_cast<StackPanel^>(XamlReader::Load(markup));
            root->RequestedTheme = ElementTheme::Light;

            childPanel = safe_cast<StackPanel^>(root->FindName(L"ChildPanel"));
            outerTextBlock = safe_cast<TextBlock^>(root->FindName(L"OuterTextBlock"));
            nestedTextBlock = safe_cast<TextBlock^>(root->FindName(L"NestedTextBlock"));

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(Colors::Blue, safe_cast<SolidColorBrush^>(outerTextBlock->Foreground)->Color);
            VERIFY_ARE_EQUAL(Colors::Red, safe_cast<SolidColorBrush^>(nestedTextBlock->Foreground)->Color);
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Replacing nested ResourceDictionary with one that does not contain implicit style");
            childPanel->Resources = safe_cast<ResourceDictionary^>(XamlReader::Load(dictionaryWithNoImplicitStyle));
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(Colors::Blue, safe_cast<SolidColorBrush^>(outerTextBlock->Foreground)->Color);
            VERIFY_ARE_EQUAL(Colors::Blue, safe_cast<SolidColorBrush^>(nestedTextBlock->Foreground)->Color);
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Replacing nested ResourceDictionary with one that contains an implicit style");
            childPanel->Resources = safe_cast<ResourceDictionary^>(XamlReader::Load(dictionaryWithImplicitStyle));
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(Colors::Blue, safe_cast<SolidColorBrush^>(outerTextBlock->Foreground)->Color);
            VERIFY_ARE_EQUAL(Colors::Pink, safe_cast<SolidColorBrush^>(nestedTextBlock->Foreground)->Color);
        });
    }

    void StyleIntegrationTests::VerifyImplicitStyleInvalidationInThemeDictionaries()
    {
        TestCleanupWrapper cleanup;

        StackPanel^ root;
        StackPanel^ childPanel;
        TextBlock^ outerTextBlock;
        TextBlock^ nestedTextBlock;

        Platform::String^ markup =
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <StackPanel.Resources>"
            L"    <ResourceDictionary>"
            L"      <Style TargetType='TextBlock'>"
            L"        <Setter Property='Foreground' Value='Black' />"
            L"      </Style>"
            L"    </ResourceDictionary>"
            L"  </StackPanel.Resources>"
            L"  <StackPanel>"
            L"    <StackPanel.Resources>"
            L"      <ResourceDictionary>"
            L"        <Style TargetType='TextBlock'>"
            L"          <Setter Property='Foreground' Value='Blue' />"
            L"        </Style>"
            L"      </ResourceDictionary>"
            L"    </StackPanel.Resources>"
            L"    <TextBlock x:Name='OuterTextBlock' Text='It is up with the blue and gold' />"
            L"    <StackPanel x:Name='ChildPanel'>"
            L"      <StackPanel.Resources>"
            L"        <ResourceDictionary>"
            L"          <ResourceDictionary.ThemeDictionaries>"
            L"            <ResourceDictionary x:Key='Light'>"
            L"              <Style TargetType='TextBlock'>"
            L"                <Setter Property='Foreground' Value='Red' />"
            L"              </Style>"
            L"            </ResourceDictionary>"
            L"            <ResourceDictionary x:Key='Dark'>"
            L"              <Style TargetType='TextBlock'>"
            L"                <Setter Property='Foreground' Value='Purple' />"
            L"              </Style>"
            L"            </ResourceDictionary>"
            L"          </ResourceDictionary.ThemeDictionaries>"
            L"        </ResourceDictionary>"
            L"      </StackPanel.Resources>"
            L"      <TextBlock x:Name='NestedTextBlock' Text='Down with the red' />"
            L"    </StackPanel>"
            L"  </StackPanel>"
            L"</StackPanel>";

        RunOnUIThread([&]()
        {
            root = safe_cast<StackPanel^>(XamlReader::Load(markup));

            outerTextBlock = safe_cast<TextBlock^>(root->FindName(L"OuterTextBlock"));
            nestedTextBlock = safe_cast<TextBlock^>(root->FindName(L"NestedTextBlock"));

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            root->RequestedTheme = ElementTheme::Light;

            VERIFY_ARE_EQUAL(Colors::Blue, safe_cast<SolidColorBrush^>(outerTextBlock->Foreground)->Color);
            VERIFY_ARE_EQUAL(Colors::Red, safe_cast<SolidColorBrush^>(nestedTextBlock->Foreground)->Color);
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Changing theme to dark");
            root->RequestedTheme = ElementTheme::Dark;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(Colors::Blue, safe_cast<SolidColorBrush^>(outerTextBlock->Foreground)->Color);
            VERIFY_ARE_EQUAL(Colors::Purple, safe_cast<SolidColorBrush^>(nestedTextBlock->Foreground)->Color);
        });
    }

    void StyleIntegrationTests::StyleSetterWithNullStaticResource()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel.Resources>"
                L"      <SolidColorBrush x:Key='BlueBrush' Color='Blue' />"
                L"      <NullExtension x:Key='ANullBrush' />"
                L"      <NullExtension x:Key='ASecondNullBrush' />"
                L"      <Style x:Key='ButtonStyle' TargetType='Button'>"
                L"          <Setter Property='Background' Value='{StaticResource ANullBrush}' />"
                L"          <Setter Property='Foreground' Value='{StaticResource ASecondNullBrush}' />"
                L"      </Style>"
                L"  </StackPanel.Resources>"
                L"  <Button Style='{StaticResource ButtonStyle}' />"
                L"</StackPanel>";

            auto panel = static_cast<Panel^>(XamlReader::Load(xamlString));

            auto button1 = safe_cast<Button^>(panel->Children->GetAt(0));
            VERIFY_IS_NULL(button1->Background);
            VERIFY_IS_NULL(button1->Foreground);
        });
    }

    void StyleIntegrationTests::StyleSetterWithNullThemeResource()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([&]()
        {
            String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel.Resources>"
                L"    <ResourceDictionary>"
                L"      <ResourceDictionary.ThemeDictionaries>"
                L"        <ResourceDictionary x:Key='Default'>"
                L"          <NullExtension x:Key='ANullBrush' />"
                L"          <NullExtension x:Key='ASecondNullBrush' />"
                L"        </ResourceDictionary>"
                L"      </ResourceDictionary.ThemeDictionaries>"
                L"    </ResourceDictionary>"
                L"      <SolidColorBrush x:Key='BlueBrush' Color='Blue' />"
                L"      <Style x:Key='ButtonStyle' TargetType='Button'>"
                L"          <Setter Property='Background' Value='{ThemeResource ANullBrush}' />"
                L"          <Setter Property='Foreground' Value='{ThemeResource ASecondNullBrush}' />"
                L"      </Style>"
                L"  </StackPanel.Resources>"
                L"  <Button Style='{StaticResource ButtonStyle}' />"
                L"</StackPanel>";

            auto panel = static_cast<Panel^>(XamlReader::Load(xamlString));

            auto button1 = safe_cast<Button^>(panel->Children->GetAt(0));
            VERIFY_IS_NULL(button1->Background);
            VERIFY_IS_NULL(button1->Foreground);
        });
    }

    void StyleIntegrationTests::VerifySetterWithInvalidAttachedPropertyTargetDoesNotAV()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            // Check Setter.Target
            Platform::String^ xamlString =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                L"    <Canvas.Style>"
                L"        <Style TargetType='Canvas'>"
                L"            <Style.Setters>"
                L"                <Setter Target='(foo.bar)' Value='32'/>"
                L"            </Style.Setters>"
                L"        </Style>"
                L"    </Canvas.Style>"
                L"</Canvas>";

            VERIFY_THROWS_WINRT(XamlReader::Load(xamlString), Platform::Exception^, L"XAML parse exception should be thrown when a style setter's target is an attached property owned by an unknown type");

            // Check Setter.Property
            xamlString =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                L"    <Canvas.Style>"
                L"        <Style TargetType='Canvas'>"
                L"            <Style.Setters>"
                L"                <Setter Property='foo.bar' Value='32'/>"
                L"            </Style.Setters>"
                L"        </Style>"
                L"    </Canvas.Style>"
                L"</Canvas>";

            VERIFY_THROWS_WINRT(XamlReader::Load(xamlString), Platform::Exception^, L"XAML parse exception should be thrown when a style setter's property is an attached property owned by an unknown type");
        });
    }

} } } } } }
