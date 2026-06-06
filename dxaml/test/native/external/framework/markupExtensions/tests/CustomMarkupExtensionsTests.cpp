// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CustomMarkupExtensionsTests.h"
#include <ppltasks.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include "FileLoader.h"

#include <CustomTypeMetadataProvider.h>
#include <MarkupExtensionTests.CustomMarkupExtension.h>
#include <MarkupExtensionTests.CustomButton.h>
#include <CustomMetadataRegistrar.h>
#include <RuntimeEnabledFeatureOverride.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Markup;
using namespace ::Windows::UI::Xaml::Interop;

using namespace test_infra;
using namespace ::Tests::Native::External::Framework::MarkupExtensions;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Native { namespace External { namespace Framework
    {
        bool CustomMarkupExtensionTests::ClassSetup()
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

        bool CustomMarkupExtensionTests::TestSetup()
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
                ref new CustomMetadataRegistrar<::Tests::Native::External::Framework::MarkupExtensions::CustomButton>());

            return true;
        }

        bool CustomMarkupExtensionTests::TestCleanup()
        {
            // Shutdown the framework. The purpose of this is to deallocate everything that
            // was allocated during the test and get to a "idle" state. We can then verify test
            // cleanup and check for leaks.
            test_infra::TestServices::WindowHelper->ShutdownXaml();

            // It's very important to have your test clean up the window contents
            // when it completes. When creating new tests be sure to copy this
            // method over or implement it in a similar way. By cleaning
            // up the window content and waiting for the page to go idle you ensure
            // that if your test fails while the UI element tree is being torn down
            // that the failure is associated with your test and doesn't occur
            // nondeterministically in the future. By waiting for the page to go
            // idle you ensure that all transitions have completed and that jupiter
            // is in a 'tabula rasa' state for the next test.
            //
            // Use the TestCleanupWrapper in each test method to handle cleanup, even
            // in cases of failure or repeated runs. Use VerifyTestCleanup here to
            // ensure that the test was cleaned up correctly.
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void CustomMarkupExtensionTests::BasicCustomMarkupExtensionFunctionality()
        {
            TestCleanupWrapper cleanup;

            std::vector<std::tuple<const wchar_t*, int>> verificationParams;
            verificationParams.push_back(std::make_tuple(L"TextBlock0", 0));
            verificationParams.push_back(std::make_tuple(L"TextBlock1", 5));
            verificationParams.push_back(std::make_tuple(L"TextBlock2", 3));
            verificationParams.push_back(std::make_tuple(L"TextBlock3", 4));

            RunOnUIThread([&verificationParams]() {
                auto stackPanel = safe_cast<StackPanel^>(XamlReader::Load(
                    L"<StackPanel"
                    L"  xmlns='http://schemas.microsoft.com/client/2007' "
                    L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"  xmlns:local='using:Tests.Native.External.Framework.MarkupExtensions'>"
                    L"  <TextBlock x:Name='TextBlock0' Text='{local:PewPew LineNumber=0}' />"           // ME syntax, without 'Extension' suffix
                    L"  <TextBlock x:Name='TextBlock1' Text='{local:PewPewExtension LineNumber=5}' />"  // ME syntax, with 'Extension' suffix
                    L"  <TextBlock x:Name='TextBlock2'>"
                    L"    <TextBlock.Text>"
                    L"      <local:PewPew LineNumber='3' />"   // XAML element syntax, without 'Extension' suffix
                    L"    </TextBlock.Text>"
                    L"  </TextBlock>"
                    L"  <TextBlock x:Name='TextBlock3'>"
                    L"    <TextBlock.Text>"
                    L"      <local:PewPewExtension LineNumber='4' />"   // XAML element syntax, without 'Extension' suffix
                    L"    </TextBlock.Text>"
                    L"  </TextBlock>"
                    L"</StackPanel>"
                    ));

                for (const auto param : verificationParams)
                {
                    auto textblock = safe_cast<TextBlock^>(stackPanel->FindName(ref new Platform::String(std::get<0>(param))));
                    VERIFY_ARE_EQUAL(textblock->Text, PewPewExtension::LookupLineNumber(std::get<1>(param)));
                }

            });
        }

        void CustomMarkupExtensionTests::VerifyCustomMarkupExtensionAsTypeOfProperty()
        {
            TestCleanupWrapper cleanup;

            std::vector<std::tuple<const wchar_t*, int>> verificationParams;
            verificationParams.push_back(std::make_tuple(L"CustomButton0", 0));
            verificationParams.push_back(std::make_tuple(L"CustomButton1", 5));

            RunOnUIThread([&verificationParams]() {
                auto stackPanel = safe_cast<StackPanel^>(XamlReader::Load(
                    L"<StackPanel"
                    L"  xmlns='http://schemas.microsoft.com/client/2007' "
                    L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"  xmlns:local='using:Tests.Native.External.Framework.MarkupExtensions'>"
                    L"  <local:CustomButton x:Name='CustomButton0' CustomProperty='{local:PewPew LineNumber=0}' />"
                    L"  <local:CustomButton x:Name='CustomButton1'>"
                    L"    <local:CustomButton.CustomProperty>"
                    L"      <local:PewPew LineNumber='5' />"
                    L"    </local:CustomButton.CustomProperty>"
                    L"  </local:CustomButton>"
                    L"</StackPanel>"
                ));

                for (const auto param : verificationParams)
                {
                    auto customButton = safe_cast<CustomButton^>(stackPanel->FindName(ref new Platform::String(std::get<0>(param))));
                    VERIFY_ARE_EQUAL(customButton->CustomProperty->LineNumber, std::get<1>(param));
                }

            });
        }

        void CustomMarkupExtensionTests::VerifyNestedCustomMarkupExtension()
        {
            TestCleanupWrapper cleanup;

            std::vector<std::tuple<const wchar_t*, int>> verificationParams;
            verificationParams.push_back(std::make_tuple(L"TextBlock0", 0));
            verificationParams.push_back(std::make_tuple(L"TextBlock1", 5));
            verificationParams.push_back(std::make_tuple(L"TextBlock2", 3));
            verificationParams.push_back(std::make_tuple(L"TextBlock3", 4));

            RunOnUIThread([&verificationParams]() {
                auto stackPanel = safe_cast<StackPanel^>(XamlReader::Load(
                    L"<StackPanel"
                    L"  xmlns='http://schemas.microsoft.com/client/2007' "
                    L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"  xmlns:local='using:Tests.Native.External.Framework.MarkupExtensions'>"
                    L"  <TextBlock x:Name='TextBlock0' Text='{local:PewPew LineNumber={local:Addition Operand1=-5,Operand2=5}}' />"           // ME syntax, without 'Extension' suffix
                    L"  <TextBlock x:Name='TextBlock1' Text='{local:PewPewExtension LineNumber={local:Addition Operand1=2,Operand2=3}}' />"  // ME syntax, with 'Extension' suffix
                    L"  <TextBlock x:Name='TextBlock2'>"
                    L"    <TextBlock.Text>"
                    L"      <local:PewPew LineNumber='{local:Addition Operand1=1,Operand2=2}' />"   // XAML element syntax, without 'Extension' suffix
                    L"    </TextBlock.Text>"
                    L"  </TextBlock>"
                    L"  <TextBlock x:Name='TextBlock3'>"
                    L"    <TextBlock.Text>"
                    L"      <local:PewPewExtension LineNumber='{local:Addition Operand1=2,Operand2=2}' />"   // XAML element syntax, without 'Extension' suffix
                    L"    </TextBlock.Text>"
                    L"  </TextBlock>"
                    L"</StackPanel>"
                    ));

                for (const auto param : verificationParams)
                {
                    auto textblock = safe_cast<TextBlock^>(stackPanel->FindName(ref new Platform::String(std::get<0>(param))));
                    VERIFY_ARE_EQUAL(textblock->Text, PewPewExtension::LookupLineNumber(std::get<1>(param)));
                }

            });
        }

        void CustomMarkupExtensionTests::CanUseCustomMarkupExtensionInStyle()
        {
            TestCleanupWrapper cleanup;
            // Run this test in compat mode so style is applied immediately (during CreationComplete) without needing to be added to the tree.
            VERIFY_IS_FALSE(xaml_settings::XamlOptionalChanges::IsChangeEnabled(xaml_settings::XamlChangeId::DelayApplyStyleOptimization));

            std::vector<std::tuple<const wchar_t*, int>> verificationParams;
            verificationParams.push_back(std::make_tuple(L"TextBlock0", 0));
            verificationParams.push_back(std::make_tuple(L"TextBlock1", 5));

            RunOnUIThread([&verificationParams]() {
                auto stackPanel = safe_cast<StackPanel^>(XamlReader::Load(
                    L"<StackPanel"
                    L"  xmlns='http://schemas.microsoft.com/client/2007' "
                    L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"  xmlns:local='using:Tests.Native.External.Framework.MarkupExtensions'>"
                    L"  <StackPanel.Resources>"
                    L"    <ResourceDictionary>"
                    L"      <Style x:Key='TextBlockStyle' TargetType='TextBlock'>"
                    L"        <Setter Property='Text' Value='{local:PewPew LineNumber=0}' />"           // ME syntax, without 'Extension' suffix
                    L"      </Style>"
                    L"      <Style x:Key='TextBlockStyleWithSuffix' TargetType='TextBlock'>"
                    L"        <Setter Property='Text' Value='{local:PewPewExtension LineNumber=5}' />"  // ME syntax, with 'Extension' suffix
                    L"      </Style>"
                    L"    </ResourceDictionary>"
                    L"  </StackPanel.Resources>"
                    L"  <TextBlock x:Name='TextBlock0' Style='{StaticResource TextBlockStyle}' />"           // ME syntax, without 'Extension' suffix
                    L"  <TextBlock x:Name='TextBlock1' Style='{StaticResource TextBlockStyleWithSuffix}' />"  // ME syntax, with 'Extension' suffix
                    L"</StackPanel>"
                    ));

                for (const auto param : verificationParams)
                {
                    auto textblock = safe_cast<TextBlock^>(stackPanel->FindName(ref new Platform::String(std::get<0>(param))));
                    VERIFY_ARE_EQUAL(textblock->Text, PewPewExtension::LookupLineNumber(std::get<1>(param)));
                }

            });
        }

        void CustomMarkupExtensionTests::CanUseCustomMarkupExtensionAsResource()
        {
            TestCleanupWrapper cleanup;

            std::vector<std::tuple<const wchar_t*, int>> verificationParams;
            verificationParams.push_back(std::make_tuple(L"TextBlock0", 0));
            verificationParams.push_back(std::make_tuple(L"TextBlock1", 5));

            RunOnUIThread([&verificationParams]() {
                auto stackPanel = safe_cast<StackPanel^>(XamlReader::Load(
                    L"<StackPanel"
                    L"  xmlns='http://schemas.microsoft.com/client/2007' "
                    L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"  xmlns:local='using:Tests.Native.External.Framework.MarkupExtensions'>"
                    L"  <StackPanel.Resources>"
                    L"      <local:PewPew x:Key='TextBlockText0' LineNumber='0' />"
                    L"      <local:PewPew x:Key='TextBlockText1' LineNumber='5' />"
                    L"  </StackPanel.Resources>"
                    L"  <TextBlock x:Name='TextBlock0' Text='{StaticResource TextBlockText0}' />"
                    L"  <TextBlock x:Name='TextBlock1' Text='{StaticResource TextBlockText1}' />"
                    L"</StackPanel>"
                    ));

                for (const auto param : verificationParams)
                {
                    auto textblock = safe_cast<TextBlock^>(stackPanel->FindName(ref new Platform::String(std::get<0>(param))));
                    VERIFY_ARE_EQUAL(textblock->Text, PewPewExtension::LookupLineNumber(std::get<1>(param)));
                }

            });
        }

        void CustomMarkupExtensionTests::VerifyCustomMarkupExtensionThatReturnsBinding()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]() {
                auto stackPanel = safe_cast<StackPanel^>(XamlReader::Load(
                    L"<StackPanel"
                    L"  xmlns='http://schemas.microsoft.com/client/2007' "
                    L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"  xmlns:local='using:Tests.Native.External.Framework.MarkupExtensions'>"
                    L"  <StackPanel.Resources>"
                    L"    <Button x:Name='button' Tag='foobar' />"
                    L"  </StackPanel.Resources>"
                    L"  <TextBlock x:Name='textBlock' Text='{local:BindingFactoryExtension Path=Tag,Source={StaticResource button}}' />"
                    L"</StackPanel>"
                    ));

                auto button = safe_cast<Button^>(stackPanel->FindName(L"button"));
                auto textBlock = safe_cast<TextBlock^>(stackPanel->FindName(L"textBlock"));

                LOG_OUTPUT(L"Verifying initial text");
                VERIFY_ARE_EQUAL(textBlock->Text, Platform::StringReference(L"foobar"));

                LOG_OUTPUT(L"Changing Binding source");
                auto newValue = Platform::StringReference(L"it is up with the blue and gold");
                button->Tag = newValue;
                VERIFY_ARE_EQUAL(textBlock->Text, newValue);
            });
        }

        void CustomMarkupExtensionTests::VerifyIProvideValueTarget()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]() {
                Platform::Object^ targetObject;
                ProvideValueTargetProperty^ targetProperty;
                bool callbackInvoked = false;

                InvokeStaticCallbackExtension::SetStaticCallback([&targetObject, &targetProperty, &callbackInvoked](IXamlServiceProvider^ serviceProvider)
                {

                    auto provideValueTarget = safe_cast<IProvideValueTarget^>(serviceProvider->GetService(TypeName(IProvideValueTarget::typeid)));
                    targetObject = provideValueTarget->TargetObject;
                    targetProperty = safe_cast<ProvideValueTargetProperty^>(provideValueTarget->TargetProperty);

                    callbackInvoked = true;
                });

                auto stackPanel = safe_cast<StackPanel^>(XamlReader::Load(
                    L"<StackPanel"
                    L"  xmlns='http://schemas.microsoft.com/client/2007' "
                    L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"  xmlns:local='using:Tests.Native.External.Framework.MarkupExtensions'>"
                    L"  <Button x:Name='button' Tag='{local:InvokeStaticCallback}' />"
                    L"</StackPanel>"
                ));

                VERIFY_IS_TRUE(callbackInvoked, L"Custom markup extension should have invoked the callback!");

                auto button = safe_cast<Button^>(stackPanel->FindName(L"button"));
                VERIFY_ARE_EQUAL(safe_cast<Button^>(targetObject), button);
                VERIFY_ARE_EQUAL(targetProperty->Name, Platform::StringReference(L"Tag"));
                VerifyTypeNamesAreEqual(targetProperty->Type, TypeName(Platform::Object::typeid));
                VerifyTypeNamesAreEqual(targetProperty->DeclaringType, TypeName(FrameworkElement::typeid));
            });
        }

        void CustomMarkupExtensionTests::VerifyIXamlTypeResolver()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]() {
                bool callbackInvoked = false;
                InvokeStaticCallbackExtension::SetStaticCallback([&callbackInvoked](IXamlServiceProvider^ serviceProvider)
                {
                    auto xamlTypeResolver = safe_cast<IXamlTypeResolver^>(serviceProvider->GetService(TypeName(IXamlTypeResolver::typeid)));

                    // Lookup a custom type
                    auto customExtensionType = xamlTypeResolver->Resolve(Platform::StringReference(L"local:InvokeStaticCallback"));
                    VerifyTypeNamesAreEqual(customExtensionType, TypeName(InvokeStaticCallbackExtension::typeid));

                    // Lookup a builtin type
                    auto buttonType = xamlTypeResolver->Resolve(Platform::StringReference(L"Button"));
                    VerifyTypeNamesAreEqual(buttonType, TypeName(Button::typeid));

                    // Lookup a nonexistent type. This should throw
                    VERIFY_THROWS_WINRT(xamlTypeResolver->Resolve(Platform::StringReference(L"blahblah:Button")), Platform::COMException^, L"An exception should be thrown if IXamlTypeResolver is asked to resolve a type in a bad namespace.");
                    VERIFY_THROWS_WINRT(xamlTypeResolver->Resolve(Platform::StringReference(L"local:foobar")), Platform::COMException^, L"An exception should be thrown if IXamlTypeResolver is asked to resolve a non-existent type name in a good namespace.");

                    callbackInvoked = true;
                });

                auto stackPanel = safe_cast<StackPanel^>(XamlReader::Load(
                    L"<StackPanel"
                    L"  xmlns='http://schemas.microsoft.com/client/2007' "
                    L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"  xmlns:local='using:Tests.Native.External.Framework.MarkupExtensions'>"
                    L"  <Button x:Name='button' Tag='{local:InvokeStaticCallback}' />"
                    L"</StackPanel>"
                ));

                VERIFY_IS_TRUE(callbackInvoked, L"Custom markup extension should have invoked the callback!");
            });
        }

        void CustomMarkupExtensionTests::VerifyIRootObjectProvider()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]() {
                Platform::Object^ rootObject;
                bool callbackInvoked = false;
                InvokeStaticCallbackExtension::SetStaticCallback([&rootObject, &callbackInvoked](IXamlServiceProvider^ serviceProvider)
                {
                    auto rootObjectProvider = safe_cast<IRootObjectProvider^>(serviceProvider->GetService(TypeName(IRootObjectProvider::typeid)));
                    rootObject = rootObjectProvider->RootObject;

                    callbackInvoked = true;
                });

                auto stackPanel = safe_cast<StackPanel^>(XamlReader::Load(
                    L"<StackPanel"
                    L"  xmlns='http://schemas.microsoft.com/client/2007' "
                    L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"  xmlns:local='using:Tests.Native.External.Framework.MarkupExtensions'>"
                    L"  <Button x:Name='button' Tag='{local:InvokeStaticCallback}' />"
                    L"</StackPanel>"
                ));

                VERIFY_IS_TRUE(callbackInvoked, L"Custom markup extension should have invoked the callback!");

                VERIFY_ARE_EQUAL(safe_cast<StackPanel^>(rootObject), stackPanel);
            });
        }

        void CustomMarkupExtensionTests::VerifyIUriContext()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]() {
                auto uriToLoad = ref new ::Windows::Foundation::Uri("ms-appx:///Oski.xaml");
                bool callbackInvoked = false;
                InvokeStaticCallbackExtension::SetStaticCallback([&uriToLoad, &callbackInvoked](IXamlServiceProvider^ serviceProvider)
                {
                    auto uriContext = safe_cast<IUriContext^>(serviceProvider->GetService(TypeName(IUriContext::typeid)));

                    VERIFY_ARE_EQUAL(uriToLoad->AbsoluteCanonicalUri, uriContext->BaseUri->AbsoluteCanonicalUri);

                    callbackInvoked = true;
                });

                auto page = ref new Page();
                Application::LoadComponent(
                    page,
                    uriToLoad,
                    Primitives::ComponentResourceLocation::Application);

                VERIFY_IS_TRUE(callbackInvoked, L"Custom markup extension should have invoked the callback!");
            });
        }

        #pragma region Helpers

        /* static */ void CustomMarkupExtensionTests::VerifyTypeNamesAreEqual(::Windows::UI::Xaml::Interop::TypeName actual, ::Windows::UI::Xaml::Interop::TypeName expected)
        {
            // MetadataAPI::GetTypeNameByClassInfo *says* that according to CLR Object should be TypeKind_Primitive, but
            // in C++/Cx the compiler disagrees and thinks it should be TypeKind_Metadata. So who's right?!
            if (expected.Name != TypeName(Platform::Object::typeid).Name)
            {
                VERIFY_ARE_EQUAL(actual.Kind, expected.Kind);
            }
            VERIFY_ARE_EQUAL(actual.Name, expected.Name);
        }

        #pragma endregion

    } } }
} } } }
