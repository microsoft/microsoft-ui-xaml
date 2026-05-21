// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "ElementDeferralTests.h"
#include "ElementTraits.h"
#include <XamlTailored.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <TreeHelper.h>
#include <DisableErrorReportingScopeGuard.h>
#include <Utils.h>
#include <array>
#include "XDeferCustomControl.h"
#include "CustomTypes.XamlTypeInfo.g.h"
#include "XDeferCustomPageNested.xaml.h"
#include "XDeferCustomPageEvents.xaml.h"
#include "XDeferCustomPageResourcesDT.xaml.h"

using namespace std;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace WEX::Logging;
using namespace WEX::Common;
using WeakReference = Platform::WeakReference;

using Colors = Microsoft::UI::Colors;
using Color = ::Windows::UI::Color;

namespace Microsoft::UI::Xaml::Tests::Framework {

    #pragma region Shared data and helpers

    // SHARED MARKUP

    static const wchar_t* s_deferLoadStrategyLazy = L"x:DeferLoadStrategy='Lazy'";
    static const wchar_t* s_realizeTrue = L"x:Load='True'";
    static const wchar_t* s_realizeFalse = L"x:Load='False'";

    static const wchar_t* s_markup0 =
        L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
        L"  <Button x:Name='deferred0' %s Content='deferred0'/>"
        L"</Grid>";

    static const wchar_t* s_markup1 =
        L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
        L"  <Button x:Name='deferred0' x:DeferLoadStrategy='Lazy' Content='deferred0'/>"
        L"  <Button x:Name='deferred1' x:DeferLoadStrategy='Lazy' Content='deferred1'/>"
        L"  <Button x:Name='deferred2' x:DeferLoadStrategy='Lazy'/>"
        L"</Grid>";

    static const wchar_t* s_markup2 =
        L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
        L"  <Button Content='normal0'/>"
        L"  <Button x:Name='deferred0' x:DeferLoadStrategy='Lazy' Content='deferred0'/>"
        L"  <Button Content='normal1'/>"
        L"  <Button x:Name='deferred1' x:DeferLoadStrategy='Lazy' Content='deferred1'/>"
        L"  <Button Content='normal2'/>"
        L"  <Button x:Name='deferred2' x:DeferLoadStrategy='Lazy' Content='deferred2'/>"
        L"</Grid>";

    static const wchar_t* s_markup3 =
        L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
        L"  <Button x:Name='normal0' Content='normal0'/>"
        L"  <Button x:Name='deferred0' x:DeferLoadStrategy='Lazy' Content='deferred0'/>"
        L"  <Button x:Name='normal1' Content='normal1'/>"
        L"  <Button x:Name='deferred1' x:DeferLoadStrategy='Lazy' Content='deferred1'/>"
        L"</Grid>";

    static const wchar_t* s_markup4 =
        L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
        L"  <Button x:Name='deferred0' x:DeferLoadStrategy='Lazy' Content='deferred0'/>"
        L"  <Button x:Name='normal0' Content='normal0'/>"
        L"  <Button x:Name='deferred1' x:DeferLoadStrategy='Lazy' Content='deferred1'/>"
        L"  <Button x:Name='normal1' Content='normal1'/>"
        L"  <Button x:Name='deferred2' x:DeferLoadStrategy='Lazy' Content='deferred2'/>"
        L"</Grid>";

    static const wchar_t* s_markup5 =
        L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
        L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
        L"  <Grid x:Name='innerPanel'>"
        L"    <Button x:Name='deferred0' %s Content='deferred0'/>"
        L"  </Grid>"
        L"</Grid>";

    static const wchar_t* s_markup6 =
        L"<ContentControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
        L"                xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
        L"                Content='abc'>"
        L"  <ContentControl.ContentTemplate>"
        L"    <DataTemplate>"
        L"      <StackPanel>"
        L"        <TextBlock x:Name='element' %s/>"
        L"      </StackPanel>"
        L"    </DataTemplate>"
        L"  </ContentControl.ContentTemplate>"
        L"</ContentControl>";

    static const wchar_t* s_markup7 =
        L"<ControlTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
        L"                 xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
        L"                 TargetType='Control'>"
        L"  <Grid x:Name='root'>"
        L"    <TextBlock x:Name='deferred0' %s/>"
        L"  </Grid>"
        L"</ControlTemplate>";

    // HELPERS

    enum class ElementType
    {
        Normal,
        Deferred
    };

    static Platform::String^ GetElementName(ElementType elementType, unsigned index)
    {
        return ref new Platform::String((wstring((elementType == ElementType::Normal) ? L"normal" : L"deferred") + to_wstring(index)).c_str());
    }

    template <typename T>
    static T^ LoadXaml(const wchar_t* xaml)
    {
        T^ result = nullptr;

        RunOnUIThread([&]()
        {
            result = safe_cast<T^>(Markup::XamlReader::Load(Platform::StringReference(xaml)));
        });

        return result;
    }

    template <typename T>
    static T^ LoadXamlEnterAndWait(const wchar_t* xaml)
    {
        T^ result = nullptr;

        RunOnUIThread([&]()
        {
            result = safe_cast<T^>(Markup::XamlReader::Load(Platform::StringReference(xaml)));
            TestServices::WindowHelper->WindowContent = result;
        });

        TestServices::WindowHelper->WaitForIdle();

        return result;
    }

    static void EnterAndWait(FrameworkElement^ element)
    {
        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = TreeHelper::WrapInGrid(element);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    static void RemovePanelChildByReference(Panel^ panel, UIElement^ element)
    {
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(panel->Children->IndexOf(element, &indexToRemove));
        panel->Children->RemoveAt(indexToRemove);
    }

    static void RemoveItemsChildByReference(ItemsControl^ control, Platform::Object^ element)
    {
        unsigned indexToRemove = 0;
        VERIFY_IS_TRUE(control->Items->IndexOf(element, &indexToRemove));
        control->Items->RemoveAt(indexToRemove);
    }

    template <typename T, int size>
    static void ClearArray(array<T, size>& a)
    {
        for (auto& e : a)
        {
            e = nullptr;
        }
    }

    struct ElementFinder
    {
        ElementFinder(FrameworkElement^ scope)
            : m_scope(scope)
        {}

        virtual Platform::Object^ FindElement(Platform::String^ name) const = 0;

    protected:
        FrameworkElement^ m_scope;
    };

    struct TemplateElementFinder : public ElementFinder
    {
        TemplateElementFinder(FrameworkElement^ scope)
            : ElementFinder(scope)
        {}

        Platform::Object^ FindElement(Platform::String^ name) const override
        {
            return safe_cast<::Tests::Native::External::Framework::XDefer::CustomControl^>(m_scope)->PublicGetTemplateChild(name);
        }
    };

    struct RegularElementFinder : public ElementFinder
    {
        RegularElementFinder(FrameworkElement^ scope)
            : ElementFinder(scope)
        {}

        Platform::Object^ FindElement(Platform::String^ name) const override
        {
            return m_scope->FindName(name);
        }
    };

    void NoCommand::operator()(Platform::Object^)
    {
        VERIFY_FAIL(L"Should not call this");
    }

    void ShowButtonFlyoutCommand::operator()(Platform::Object^ o)
    {
        auto b = safe_cast<Button^>(o);
        b->Flyout->ShowAt(b);
    }

    void ShowAttachedFlyoutCommand::operator()(Platform::Object^ o)
    {
        FlyoutBase::ShowAttachedFlyout(safe_cast<FrameworkElement^>(o));
    }

    #pragma endregion

    #pragma region Setup

    bool ElementDeferralTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        featureEnforceXbfV2Stream.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);
        return true;
    }

    bool ElementDeferralTests::ClassCleanup()
    {
        return true;
    }

    bool ElementDeferralTests::TestSetup()
    {
        TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider());
        return true;
    }

    bool ElementDeferralTests::TestCleanup()
    {
        TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    #pragma endregion

    #pragma region Parser validation

    void ElementDeferralTests::ValidateDeferLoadStrategyValues()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='button' x:DeferLoadStrategy='Lazy'/>"
                L"</Grid>"));
        });

        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([&]()
        {
            VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='button' x:DeferLoadStrategy='abc'/>"
                L"</Grid>"),
                Platform::COMException^,
                L"XAML parse exception should be thrown x:DeferLoadStrategy is not valid");
        });
    }

    void ElementDeferralTests::ValidateRealizeValues()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='button' x:Load='True'/>"
                L"</Grid>"));

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='button' x:Load='False'/>"
                L"</Grid>"));

            // validate alternate casing (True vs. true)

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='button' x:Load='true'/>"
                L"</Grid>"));

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='button' x:Load='false'/>"
                L"</Grid>"));
        });

        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([&]()
        {
            VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='button' x:Load='abc'/>"
                L"</Grid>"),
                Platform::COMException^,
                L"XAML parse exception should be thrown x:Load is not valid");
        });
    }

    void ElementDeferralTests::ValidateNameSetOnDeferredElement()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            // NO THROW: Non-template element, x:Name

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='button' x:DeferLoadStrategy='Lazy'/>"
                L"</Grid>"));

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='button' x:Load='False'/>"
                L"</Grid>"));

            // NO THROW: Non-template element, Name as attribute

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button Name='button' x:DeferLoadStrategy='Lazy'/>"
                L"</Grid>"));

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button Name='button' x:Load='False'/>"
                L"</Grid>"));

            // NO THROW: Non-template element, Name as node

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:DeferLoadStrategy='Lazy'>"
                L"    <Button.Name>button</Button.Name>"
                L"  </Button>"
                L"</Grid>"));

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Load='False'>"
                L"    <Button.Name>button</Button.Name>"
                L"  </Button>"
                L"</Grid>"));

            // NO THROW: Data template element, x:Name

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<ContentControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"                xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <ContentControl.ContentTemplate>"
                L"    <DataTemplate>"
                L"      <Grid>"
                L"        <TextBlock x:Name='element' x:DeferLoadStrategy='Lazy'/>"
                L"      </Grid>"
                L"    </DataTemplate>"
                L"  </ContentControl.ContentTemplate>"
                L"</ContentControl>"));

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<ContentControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"                xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <ContentControl.ContentTemplate>"
                L"    <DataTemplate>"
                L"      <Grid>"
                L"        <TextBlock x:Name='element' x:Load='False'/>"
                L"      </Grid>"
                L"    </DataTemplate>"
                L"  </ContentControl.ContentTemplate>"
                L"</ContentControl>"));

            // NO THROW: Data template element, Name as attribute

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<ContentControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"                xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <ContentControl.ContentTemplate>"
                L"    <DataTemplate>"
                L"      <Grid>"
                L"        <TextBlock Name='element' x:DeferLoadStrategy='Lazy'/>"
                L"      </Grid>"
                L"    </DataTemplate>"
                L"  </ContentControl.ContentTemplate>"
                L"</ContentControl>"));

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<ContentControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"                xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <ContentControl.ContentTemplate>"
                L"    <DataTemplate>"
                L"      <Grid>"
                L"        <TextBlock Name='element' x:Load='False'/>"
                L"      </Grid>"
                L"    </DataTemplate>"
                L"  </ContentControl.ContentTemplate>"
                L"</ContentControl>"));

            // NO THROW: Control template element, Name as node

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<ContentControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"                xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <ContentControl.ContentTemplate>"
                L"    <DataTemplate>"
                L"      <Grid>"
                L"        <TextBlock x:DeferLoadStrategy='Lazy'>"
                L"          <TextBlock.Name>element</TextBlock.Name>"
                L"        </TextBlock>"
                L"      </Grid>"
                L"    </DataTemplate>"
                L"  </ContentControl.ContentTemplate>"
                L"</ContentControl>"));

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<ContentControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"                xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <ContentControl.ContentTemplate>"
                L"    <DataTemplate>"
                L"      <Grid>"
                L"        <TextBlock x:Load='False'>"
                L"          <TextBlock.Name>element</TextBlock.Name>"
                L"        </TextBlock>"
                L"      </Grid>"
                L"    </DataTemplate>"
                L"  </ContentControl.ContentTemplate>"
                L"</ContentControl>"));

            // NO THROW: Control template element, x:Name

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button>"
                L"    <Button.Style>"
                L"      <Style TargetType='Button'>"
                L"        <Setter Property='Template'>"
                L"          <Setter.Value>"
                L"            <ControlTemplate TargetType='Button'>"
                L"              <Grid>"
                L"                <Rectangle x:Name='element' x:DeferLoadStrategy='Lazy'/>"
                L"              </Grid>"
                L"            </ControlTemplate>"
                L"          </Setter.Value>"
                L"        </Setter>"
                L"      </Style>"
                L"    </Button.Style>"
                L"  </Button>"
                L"</Grid>"));

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button>"
                L"    <Button.Style>"
                L"      <Style TargetType='Button'>"
                L"        <Setter Property='Template'>"
                L"          <Setter.Value>"
                L"            <ControlTemplate TargetType='Button'>"
                L"              <Grid>"
                L"                <Rectangle x:Name='element' x:Load='False'/>"
                L"              </Grid>"
                L"            </ControlTemplate>"
                L"          </Setter.Value>"
                L"        </Setter>"
                L"      </Style>"
                L"    </Button.Style>"
                L"  </Button>"
                L"</Grid>"));

            // NO THROW: Control template element, Name as attribute

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button>"
                L"    <Button.Style>"
                L"      <Style TargetType='Button'>"
                L"        <Setter Property='Template'>"
                L"          <Setter.Value>"
                L"            <ControlTemplate TargetType='Button'>"
                L"              <Grid>"
                L"                <Rectangle Name='element' x:DeferLoadStrategy='Lazy'/>"
                L"              </Grid>"
                L"            </ControlTemplate>"
                L"          </Setter.Value>"
                L"        </Setter>"
                L"      </Style>"
                L"    </Button.Style>"
                L"  </Button>"
                L"</Grid>"));

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button>"
                L"    <Button.Style>"
                L"      <Style TargetType='Button'>"
                L"        <Setter Property='Template'>"
                L"          <Setter.Value>"
                L"            <ControlTemplate TargetType='Button'>"
                L"              <Grid>"
                L"                <Rectangle Name='element' x:Load='False'/>"
                L"              </Grid>"
                L"            </ControlTemplate>"
                L"          </Setter.Value>"
                L"        </Setter>"
                L"      </Style>"
                L"    </Button.Style>"
                L"  </Button>"
                L"</Grid>"));

            // NO THROW: Control template element, Name as node

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button>"
                L"    <Button.Style>"
                L"      <Style TargetType='Button'>"
                L"        <Setter Property='Template'>"
                L"          <Setter.Value>"
                L"            <ControlTemplate TargetType='Button'>"
                L"              <Grid>"
                L"                <Rectangle x:DeferLoadStrategy='Lazy'>"
                L"                  <Rectangle.Name>element</Rectangle.Name>"
                L"                </Rectangle>"
                L"              </Grid>"
                L"            </ControlTemplate>"
                L"          </Setter.Value>"
                L"        </Setter>"
                L"      </Style>"
                L"    </Button.Style>"
                L"  </Button>"
                L"</Grid>"));

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button>"
                L"    <Button.Style>"
                L"      <Style TargetType='Button'>"
                L"        <Setter Property='Template'>"
                L"          <Setter.Value>"
                L"            <ControlTemplate TargetType='Button'>"
                L"              <Grid>"
                L"                <Rectangle x:Load='False'>"
                L"                  <Rectangle.Name>element</Rectangle.Name>"
                L"                </Rectangle>"
                L"              </Grid>"
                L"            </ControlTemplate>"
                L"          </Setter.Value>"
                L"        </Setter>"
                L"      </Style>"
                L"    </Button.Style>"
                L"  </Button>"
                L"</Grid>"));
        });

        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([&]()
        {
            // THROW: Non-template element, no name

            VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:DeferLoadStrategy='Lazy'/>"
                L"</Grid>"),
                Platform::COMException^,
                L"XAML parse exception should be thrown if x:DeferLoadStrategy is set on unnamed element");

            VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Load='False'/>"
                L"</Grid>"),
                Platform::COMException^,
                L"XAML parse exception should be thrown if x:Load is set on unnamed element");

            // THROW: Data template element, no name

            VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                L"<ContentControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"                xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <ContentControl.ContentTemplate>"
                L"    <DataTemplate>"
                L"      <Grid>"
                L"        <TextBlock x:DeferLoadStrategy='Lazy'/>"
                L"      </Grid>"
                L"    </DataTemplate>"
                L"  </ContentControl.ContentTemplate>"
                L"</ContentControl>"),
                Platform::COMException^,
                L"XAML parse exception should be thrown if x:DeferLoadStrategy is set on unnamed element");

            VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                L"<ContentControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"                xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <ContentControl.ContentTemplate>"
                L"    <DataTemplate>"
                L"      <Grid>"
                L"        <TextBlock x:Load='False'/>"
                L"      </Grid>"
                L"    </DataTemplate>"
                L"  </ContentControl.ContentTemplate>"
                L"</ContentControl>"),
                Platform::COMException^,
                L"XAML parse exception should be thrown if x:Load is set on unnamed element");

            // THROW: Control template element, no name

            VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button>"
                L"    <Button.Style>"
                L"      <Style TargetType='Button'>"
                L"        <Setter Property='Template'>"
                L"          <Setter.Value>"
                L"            <ControlTemplate TargetType='Button'>"
                L"              <Grid>"
                L"                <Rectangle x:DeferLoadStrategy='Lazy'/>"
                L"              </Grid>"
                L"            </ControlTemplate>"
                L"          </Setter.Value>"
                L"        </Setter>"
                L"      </Style>"
                L"    </Button.Style>"
                L"  </Button>"
                L"</Grid>"),
                Platform::COMException^,
                L"XAML parse exception should be thrown if x:DeferLoadStrategy is set on unnamed element");

            VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button>"
                L"    <Button.Style>"
                L"      <Style TargetType='Button'>"
                L"        <Setter Property='Template'>"
                L"          <Setter.Value>"
                L"            <ControlTemplate TargetType='Button'>"
                L"              <Grid>"
                L"                <Rectangle x:Load='False'/>"
                L"              </Grid>"
                L"            </ControlTemplate>"
                L"          </Setter.Value>"
                L"        </Setter>"
                L"      </Style>"
                L"    </Button.Style>"
                L"  </Button>"
                L"</Grid>"),
                Platform::COMException^,
                L"XAML parse exception should be thrown if x:Load is set on unnamed element");

            // THROW: Non-template element, name on child element, but not on defered one.

            VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:DeferLoadStrategy='Lazy'>"
                L"    <Button x:Name='nondeferred'/>"
                L"  </Button>"
                L"</Grid>"),
                Platform::COMException^,
                L"XAML parse exception should be thrown if x:DeferLoadStrategy is set on unnamed element");

            VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Load='False'>"
                L"    <Button x:Name='nondeferred'/>"
                L"  </Button>"
                L"</Grid>"),
                Platform::COMException^,
                L"XAML parse exception should be thrown if x:Load is set on unnamed element");
        });
    }

    void ElementDeferralTests::ValidateDeferredElementIsNotRoot()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            // NO THROW: Non-template element, not root

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='button' x:DeferLoadStrategy='Lazy'/>"
                L"</Grid>"));

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='button' x:Load='False'/>"
                L"</Grid>"));
        });

        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([&]()
        {
            // THROW: Non-template element, root

            VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"      x:DeferLoadStrategy='Lazy'/>"),
                Platform::COMException^,
                L"XAML parse exception should be thrown if x:DeferLoadStrategy is set on root element");

            VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"      x:Load='False'/>"),
                Platform::COMException^,
                L"XAML parse exception should be thrown if x:Load is set on root element");

            // THROW: Data template element, root

            VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                L"<ContentControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"                xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <ContentControl.ContentTemplate>"
                L"    <DataTemplate>"
                L"      <TextBlock x:Name='element' x:DeferLoadStrategy='Lazy'/>"
                L"    </DataTemplate>"
                L"  </ContentControl.ContentTemplate>"
                L"</ContentControl>"),
                Platform::COMException^,
                L"XAML parse exception should be thrown if x:DeferLoadStrategy is set on root element");

            VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                L"<ContentControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"                xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <ContentControl.ContentTemplate>"
                L"    <DataTemplate>"
                L"      <TextBlock x:Name='element' x:Load='False'/>"
                L"    </DataTemplate>"
                L"  </ContentControl.ContentTemplate>"
                L"</ContentControl>"),
                Platform::COMException^,
                L"XAML parse exception should be thrown if x:Load is set on root element");

            // THROW: Control template element, root

            VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button>"
                L"    <Button.Style>"
                L"      <Style TargetType='Button'>"
                L"        <Setter Property='Template'>"
                L"          <Setter.Value>"
                L"            <ControlTemplate TargetType='Button'>"
                L"              <Rectangle x:Name='element' x:DeferLoadStrategy='Lazy'/>"
                L"            </ControlTemplate>"
                L"          </Setter.Value>"
                L"        </Setter>"
                L"      </Style>"
                L"    </Button.Style>"
                L"  </Button>"
                L"</Grid>"),
                Platform::COMException^,
                L"XAML parse exception should be thrown if x:DeferLoadStrategy is set on root element");

            VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button>"
                L"    <Button.Style>"
                L"      <Style TargetType='Button'>"
                L"        <Setter Property='Template'>"
                L"          <Setter.Value>"
                L"            <ControlTemplate TargetType='Button'>"
                L"              <Rectangle x:Name='element' x:Load='False'/>"
                L"            </ControlTemplate>"
                L"          </Setter.Value>"
                L"        </Setter>"
                L"      </Style>"
                L"    </Button.Style>"
                L"  </Button>"
                L"</Grid>"),
                Platform::COMException^,
                L"XAML parse exception should be thrown if x:Load is set on root element");
        });
    }

    void ElementDeferralTests::ValidateDeferLoadStrategyWithOtherAttributes()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='button' x:DeferLoadStrategy='Lazy' x:Uid='abc'/>"
                L"</Grid>"));

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='button' x:Load='False' x:Uid='abc'/>"
                L"</Grid>"));

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='button' x:DeferLoadStrategy='Lazy' x:Uid='uid' Tag='tag'/>"
                L"</Grid>"));

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='button' x:Load='False' x:Uid='uid' Tag='tag'/>"
                L"</Grid>"));

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button xmlns:y='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='button' x:DeferLoadStrategy='Lazy' x:Uid='uid' Tag='tag'/>"
                L"</Grid>"));

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button xmlns:y='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='button' x:Load='False' x:Uid='uid' Tag='tag'/>"
                L"</Grid>"));
        });

        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([&]()
        {
            // THROW: Duplicate content property in deferred element

            VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='button' x:DeferLoadStrategy='Lazy'>"
                L"    <Rectangle/>"
                L"    <Rectangle/>"
                L"  </Button>"
                L"</Grid>"),
                Platform::COMException^,
                L"XAML parse exception should be thrown if Content is set twice");

            VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='button' x:Load='False'>"
                L"    <Rectangle/>"
                L"    <Rectangle/>"
                L"  </Button>"
                L"</Grid>"),
                Platform::COMException^,
                L"XAML parse exception should be thrown if Content is set twice");

            // THROW: Duplicate deferal attribute in deferred element

            VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='button' x:DeferLoadStrategy='Lazy' x:DeferLoadStrategy='Lazy'/>"
                L"</Grid>"),
                Platform::COMException^,
                L"XAML parse exception should be thrown if x:DeferLoadStrategy is set twice");

            VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='button' x:Load='False' x:Load='False'/>"
                L"</Grid>"),
                Platform::COMException^,
                L"XAML parse exception should be thrown if x:Load is set twice");

            VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='button' x:Load='False' x:DeferLoadStrategy='Lazy'/>"
                L"</Grid>"),
                Platform::COMException^,
                L"XAML parse exception should be thrown if deferal is set twice");

            VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='button' x:DeferLoadStrategy='Lazy' x:Load='False'/>"
                L"</Grid>"),
                Platform::COMException^,
                L"XAML parse exception should be thrown if deferal is set twice");
        });
    }

    static void ValidateDeferredElementGetsNamespacesNormal(const wchar_t* deferAttribute)
    {
        Log::Comment(String().Format(L"Defer Attribute: %s", deferAttribute));

        RunOnUIThread([&]()
        {
            Grid^ rootPanel = LoadXaml<Grid>(
                String().Format(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid xmlns:alias='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"        alias:Name='innerGrid' %s>"
                    L"    <Button alias:Name='button' Content='deferred button'/>"
                    L"  </Grid>"
                    L"</Grid>",
                    deferAttribute));

            VERIFY_ARE_EQUAL(0U, rootPanel->Children->Size);

            Grid^ innerGrid = safe_cast<Grid^>(rootPanel->FindName(L"innerGrid"));
            VERIFY_IS_NOT_NULL(innerGrid);
            VERIFY_ARE_EQUAL(1U, innerGrid->Children->Size);

            Button^ button = safe_cast<Button^>(rootPanel->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);
        });
    }

    static void ValidateDeferredElementGetsNamespacesDT(const wchar_t* deferAttribute)
    {
        Log::Comment(String().Format(L"Defer Attribute: %s", deferAttribute));

        ContentControl^ contentControl = LoadXamlEnterAndWait<ContentControl>(
            String().Format(
                L"<ContentControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"                xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"                Content='abc'>"
                L"  <ContentControl.ContentTemplate>"
                L"    <DataTemplate>"
                L"      <StackPanel>"
                L"        <Button xmlns:alias='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"                alias:Name='element'"
                L"                %s>"
                L"          <TextBlock alias:Name='tb'/>"
                L"        </Button>"
                L"      </StackPanel>"
                L"    </DataTemplate>"
                L"  </ContentControl.ContentTemplate>"
                L"</ContentControl>",
                deferAttribute));

        RunOnUIThread([&]()
        {
            Panel^ panel = safe_cast<Panel^>(contentControl->ContentTemplateRoot);

            Button^ element = safe_cast<Button^>(panel->FindName(L"element"));
            VERIFY_IS_NOT_NULL(element);

            TextBlock^ tb = safe_cast<TextBlock^>(panel->FindName(L"tb"));
            VERIFY_IS_NOT_NULL(tb);
        });
    }

    static void ValidateDeferredElementGetsNamespacesCT(const wchar_t* deferAttribute)
    {
        using namespace ::Tests::Native::External::Framework::XDefer;

        Log::Comment(String().Format(L"Defer Attribute: %s", deferAttribute));

        RunOnUIThread([&]()
        {
            ControlTemplate^ controlTemplate = LoadXaml<ControlTemplate>(
                String().Format(
                    L"<ControlTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"                 xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"                 TargetType='Control'>"
                    L"  <Grid>"
                    L"    <Button xmlns:alias='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"            alias:Name='element'"
                    L"            %s>"
                    L"      <TextBlock alias:Name='tb'/>"
                    L"    </Button>"
                    L"  </Grid>"
                    L"</ControlTemplate>",
                    deferAttribute));

            CustomControl^ customControl = ref new CustomControl();
            customControl->Template = controlTemplate;
            customControl->ApplyTemplate();

            Button^ element = safe_cast<Button^>(customControl->PublicGetTemplateChild(L"element"));
            VERIFY_IS_NOT_NULL(element);

            TextBlock^ tb = safe_cast<TextBlock^>(customControl->PublicGetTemplateChild(L"tb"));
            VERIFY_IS_NOT_NULL(tb);
        });
    }

    void ElementDeferralTests::ValidateDeferredElementGetsNamespaces()
    {
        TestCleanupWrapper cleanup;

        // normal case

        ValidateDeferredElementGetsNamespacesNormal(s_deferLoadStrategyLazy);
        ValidateDeferredElementGetsNamespacesNormal(s_realizeFalse);

        // data template

        ValidateDeferredElementGetsNamespacesDT(s_deferLoadStrategyLazy);
        ValidateDeferredElementGetsNamespacesDT(s_realizeFalse);

        // control template

        ValidateDeferredElementGetsNamespacesCT(s_deferLoadStrategyLazy);
        ValidateDeferredElementGetsNamespacesCT(s_realizeFalse);
    }

    void ElementDeferralTests::ValidateDeferralIsIgnoredInLooseXAML()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;
        RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);

        RunOnUIThread([&]()
        {
            // NO THROW: Non-template element, loose XAML

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='deferred' x:DeferLoadStrategy='Lazy'/>"
                L"</Grid>"));

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='deferred' x:Load='False'/>"
                L"</Grid>"));

            // NO THROW: Data template element, loose XAML

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<ContentControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"                xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <ContentControl.ContentTemplate>"
                L"    <DataTemplate>"
                L"      <Grid>"
                L"        <TextBlock x:Name='deferred' x:DeferLoadStrategy='Lazy'/>"
                L"      </Grid>"
                L"    </DataTemplate>"
                L"  </ContentControl.ContentTemplate>"
                L"</ContentControl>"));

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<ContentControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"                xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <ContentControl.ContentTemplate>"
                L"    <DataTemplate>"
                L"      <Grid>"
                L"        <TextBlock x:Name='deferred' x:Load='False'/>"
                L"      </Grid>"
                L"    </DataTemplate>"
                L"  </ContentControl.ContentTemplate>"
                L"</ContentControl>"));

            // NO THROW: Control template element, loose XAML

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button>"
                L"    <Button.Style>"
                L"      <Style TargetType='Button'>"
                L"        <Setter Property='Template'>"
                L"          <Setter.Value>"
                L"            <ControlTemplate TargetType='Button'>"
                L"              <Grid>"
                L"                <Rectangle x:Name='deferred' x:DeferLoadStrategy='Lazy'/>"
                L"              </Grid>"
                L"            </ControlTemplate>"
                L"          </Setter.Value>"
                L"        </Setter>"
                L"      </Style>"
                L"    </Button.Style>"
                L"  </Button>"
                L"</Grid>"));

            VERIFY_NO_THROW(Markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button>"
                L"    <Button.Style>"
                L"      <Style TargetType='Button'>"
                L"        <Setter Property='Template'>"
                L"          <Setter.Value>"
                L"            <ControlTemplate TargetType='Button'>"
                L"              <Grid>"
                L"                <Rectangle x:Name='deferred' x:Load='False'/>"
                L"              </Grid>"
                L"            </ControlTemplate>"
                L"          </Setter.Value>"
                L"        </Setter>"
                L"      </Style>"
                L"    </Button.Style>"
                L"  </Button>"
                L"</Grid>"));
        });
    }

    #pragma endregion

    #pragma region Basic functionality

    static void ValidateDefaultDoesNotDeferScenario(int loadTime)
    {
        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"Load time: %d", loadTime));

        Grid^ rootPanel = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = LoadXaml<Grid>(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='button'/>"
                L"</Grid>");

            VERIFY_ARE_EQUAL(1U, rootPanel->Children->Size);
        });

        if (loadTime == 0)
        {
            EnterAndWait(rootPanel);
        }

        RunOnUIThread([&]()
        {
            Button^ button = safe_cast<Button^>(rootPanel->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);
            VERIFY_ARE_EQUAL(1U, rootPanel->Children->Size);
        });

        if (loadTime != -1)
        {
            TestServices::WindowHelper->WaitForIdle();
        }
    }

    void ElementDeferralTests::ValidateDefaultDoesNotDefer()
    {
        ValidateDefaultDoesNotDeferScenario(-1);
        ValidateDefaultDoesNotDeferScenario(0);
    }

    static void ValidateDeferDoesDeferScenario(int loadTime, const wchar_t* deferAttribute)
    {
        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"Load time: %d", loadTime));
        Log::Comment(String().Format(L"Defer Attribute: %s", deferAttribute));

        Grid^ rootPanel = LoadXaml<Grid>(
            String().Format(
                s_markup0,
                deferAttribute));

        if (loadTime == 0)
        {
            EnterAndWait(rootPanel);
        }

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL((deferAttribute == s_realizeTrue) ? 1U : 0U, rootPanel->Children->Size);

            Button^ button1 = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(button1);

            Button^ button2 = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(button2);

            VERIFY_IS_TRUE(button1 == button2);
            VERIFY_ARE_EQUAL(1U, rootPanel->Children->Size);
        });

        if (loadTime != -1)
        {
            TestServices::WindowHelper->WaitForIdle();
        }
    }

    void ElementDeferralTests::ValidateCanDeferInNonTemplate()
    {
        ValidateDeferDoesDeferScenario(-1, s_deferLoadStrategyLazy);
        ValidateDeferDoesDeferScenario(0, s_deferLoadStrategyLazy);

        ValidateDeferDoesDeferScenario(-1, s_realizeFalse);
        ValidateDeferDoesDeferScenario(0, s_realizeFalse);

        ValidateDeferDoesDeferScenario(-1, s_realizeTrue);
        ValidateDeferDoesDeferScenario(0, s_realizeTrue);
    }

    static void ValidateCanRedeferInNonTemplateScenario(const wchar_t* deferAttribute)
    {
        Log::Comment(String().Format(L"Defer Attribute: %s", deferAttribute));

        WeakReference elementWeakRef;

        Grid^ rootPanel = LoadXamlEnterAndWait<Grid>(
            String().Format(
                s_markup0,
                deferAttribute));

        RunOnUIThread([&]()
        {
            // Try it three times...
            for (int iter = 0; iter < 3; ++iter)
            {
                VERIFY_ARE_EQUAL(iter == 0 ? (deferAttribute == s_realizeTrue ? 1U : 0U) : 0U, rootPanel->Children->Size);

                Button^ button1 = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 0)));
                VERIFY_IS_NOT_NULL(button1);
                elementWeakRef = WeakReference(button1);

                VERIFY_ARE_EQUAL(1U, rootPanel->Children->Size);

                Markup::XamlMarkupHelper::UnloadObject(button1);
                button1 = nullptr;

                VERIFY_IS_NULL(elementWeakRef.Resolve<Button>());
            }
        });
    }

    void ElementDeferralTests::ValidateCanRedeferInNonTemplate()
    {
        TestCleanupWrapper cleanup;

        ValidateCanRedeferInNonTemplateScenario(s_realizeFalse);
        ValidateCanRedeferInNonTemplateScenario(s_realizeTrue);
    }

    static void ValidateCanDeferInDataTemplateSceanrio(const wchar_t* deferAttribute)
    {
        Log::Comment(String().Format(L"Defer Attribute: %s", deferAttribute));

        ContentControl^ contentControl = LoadXamlEnterAndWait<ContentControl>(
            String().Format(
                s_markup6,
                deferAttribute));

        RunOnUIThread([&]()
        {
            Panel^ panel = safe_cast<Panel^>(contentControl->ContentTemplateRoot);

            VERIFY_ARE_EQUAL(deferAttribute == s_realizeTrue ? 1U : 0U, panel->Children->Size);

            TextBlock^ element = safe_cast<TextBlock^>(panel->FindName(L"element"));
            VERIFY_IS_NOT_NULL(element);

            VERIFY_ARE_EQUAL(1U, panel->Children->Size);
        });
    }

    void ElementDeferralTests::ValidateCanDeferInDataTemplate()
    {
        TestCleanupWrapper cleanup;

        ValidateCanDeferInDataTemplateSceanrio(s_deferLoadStrategyLazy);
        ValidateCanDeferInDataTemplateSceanrio(s_realizeFalse);
        ValidateCanDeferInDataTemplateSceanrio(s_realizeTrue);
    }

    static void ValidateCanRedeferInDataTemplateScenario(const wchar_t* deferAttribute)
    {
        Log::Comment(String().Format(L"Defer Attribute: %s", deferAttribute));

        WeakReference elementWeakRef;

        ContentControl^ contentControl = LoadXamlEnterAndWait<ContentControl>(
            String().Format(
                s_markup6,
                deferAttribute));

        RunOnUIThread([&]()
        {
            Panel^ panel = safe_cast<Panel^>(contentControl->ContentTemplateRoot);

            // Try it three times...
            for (int iter = 0; iter < 3; ++iter)
            {
                VERIFY_ARE_EQUAL(iter == 0 ? (deferAttribute == s_realizeTrue ? 1U : 0U) : 0U, panel->Children->Size);

                TextBlock^ element = safe_cast<TextBlock^>(panel->FindName(L"element"));
                VERIFY_IS_NOT_NULL(element);
                elementWeakRef = WeakReference(element);

                VERIFY_ARE_EQUAL(1U, panel->Children->Size);

                Markup::XamlMarkupHelper::UnloadObject(element);
                element = nullptr;

                VERIFY_IS_NULL(elementWeakRef.Resolve<TextBlock>());
            }
        });
    }

    void ElementDeferralTests::ValidateCanRedeferInDataTemplate()
    {
        TestCleanupWrapper cleanup;

        ValidateCanRedeferInDataTemplateScenario(s_realizeFalse);
        ValidateCanRedeferInDataTemplateScenario(s_realizeTrue);
    }

    static void ValidateCanDeferInControlTemplateScenario(const wchar_t* deferAttribute)
    {
        using namespace ::Tests::Native::External::Framework::XDefer;

        Log::Comment(String().Format(L"Defer Attribute: %s", deferAttribute));

        CustomControl^ customControl = nullptr;
        Panel^ customControlRootPanel = nullptr;

        RunOnUIThread([&]()
        {
            ControlTemplate^ controlTemplate = LoadXaml<ControlTemplate>(
                String().Format(
                    s_markup7,
                    deferAttribute));

            customControl = ref new CustomControl();
            customControl->Template = controlTemplate;
            TestServices::WindowHelper->WindowContent = TreeHelper::WrapInGrid(customControl);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            customControlRootPanel = safe_cast<Panel^>(customControl->PublicGetTemplateChild(L"root"));
            VERIFY_IS_NOT_NULL(customControlRootPanel);

            VERIFY_ARE_EQUAL(deferAttribute == s_realizeTrue ? 1U : 0U, customControlRootPanel->Children->Size);

            TextBlock^ element = safe_cast<TextBlock^>(customControl->PublicGetTemplateChild(L"deferred0"));
            VERIFY_IS_NOT_NULL(element);

            VERIFY_ARE_EQUAL(1U, customControlRootPanel->Children->Size);
        });
    }

    void ElementDeferralTests::ValidateCanDeferInControlTemplate()
    {
        TestCleanupWrapper cleanup;

        ValidateCanDeferInControlTemplateScenario(s_deferLoadStrategyLazy);
        ValidateCanDeferInControlTemplateScenario(s_realizeFalse);
        ValidateCanDeferInControlTemplateScenario(s_realizeTrue);
    }

    static void ValidateCanRedeferInControlTemplateScenario(const wchar_t* deferAttribute)
    {
        using namespace ::Tests::Native::External::Framework::XDefer;

        Log::Comment(String().Format(L"Defer Attribute: %s", deferAttribute));

        WeakReference elementWeakRef;

        CustomControl^ customControl = nullptr;
        Panel^ customControlRootPanel = nullptr;

        RunOnUIThread([&]()
        {
            ControlTemplate^ controlTemplate = LoadXaml<ControlTemplate>(
                String().Format(
                    s_markup7,
                    deferAttribute));

            customControl = ref new CustomControl();
            customControl->Template = controlTemplate;
            TestServices::WindowHelper->WindowContent = TreeHelper::WrapInGrid(customControl);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            customControlRootPanel = safe_cast<Panel^>(customControl->PublicGetTemplateChild(L"root"));
            VERIFY_IS_NOT_NULL(customControlRootPanel);

            // Try it three times...
            for (int iter = 0; iter < 3; ++iter)
            {
                VERIFY_ARE_EQUAL(iter == 0 ? (deferAttribute == s_realizeTrue ? 1U : 0U) : 0U, customControlRootPanel->Children->Size);

                TextBlock^ element = safe_cast<TextBlock^>(customControl->PublicGetTemplateChild(L"deferred0"));
                VERIFY_IS_NOT_NULL(element);
                elementWeakRef = WeakReference(element);

                VERIFY_ARE_EQUAL(1U, customControlRootPanel->Children->Size);

                Markup::XamlMarkupHelper::UnloadObject(element);
                element = nullptr;

                VERIFY_IS_NULL(elementWeakRef.Resolve<TextBlock>());
            }
        });
    }

    void ElementDeferralTests::ValidateCanRedeferInControlTemplate()
    {
        TestCleanupWrapper cleanup;

        ValidateCanRedeferInControlTemplateScenario(s_realizeFalse);
        ValidateCanRedeferInControlTemplateScenario(s_realizeTrue);
    }

    static void ValidateDeclaringScopeDestroyedFirst()
    {
        TestCleanupWrapper cleanup;
        WeakReference rootWeakRef;

        Grid^ root = LoadXamlEnterAndWait<Grid>(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"      x:Name='declaringScope'>"
            L"  <StackPanel x:Name='parent'>"
            L"    <TextBlock x:Name='element' x:DeferLoadStrategy='Lazy'/>"
            L"  </StackPanel>"
            L"</Grid>");

        rootWeakRef = WeakReference(root);

        StackPanel^ parent = nullptr;

        RunOnUIThread([&]()
        {
            parent = safe_cast<StackPanel^>(root->FindName(L"parent"));
            VERIFY_IS_NOT_NULL(parent);
            root->Children->Clear();
            root = nullptr;
            TestServices::WindowHelper->WindowContent = nullptr;
        });

        TestServices::WindowHelper->WaitForIdle();

        // We need a GC collect here in WPF hosting mode because the WPF code
        // holds a reference to the Grid.
        TestServices::WindowHelper->GCCollect();

        RunOnUIThread([&]()
        {
            VERIFY_IS_NULL(rootWeakRef.Resolve<Grid>());
            parent = nullptr;
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    static void ValidateParentDestroyedFirst()
    {
        TestCleanupWrapper cleanup;
        WeakReference parentWeakRef;

        Grid^ root = LoadXamlEnterAndWait<Grid>(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"      x:Name='declaringScope'>"
            L"  <StackPanel x:Name='parent'>"
            L"    <TextBlock x:Name='element' x:DeferLoadStrategy='Lazy'/>"
            L"  </StackPanel>"
            L"</Grid>");

        RunOnUIThread([&]()
        {
            StackPanel^ parent = safe_cast<StackPanel^>(root->FindName(L"parent"));
            VERIFY_IS_NOT_NULL(parent);
            parentWeakRef = WeakReference(parent);
            root->Children->Clear();
            parent = nullptr;

            // Should not be possible to realize deferred after parent is destoyed
            TextBlock^ element = safe_cast<TextBlock^>(root->FindName(L"element"));
            VERIFY_IS_NULL(element);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            root = nullptr;
            TestServices::WindowHelper->WindowContent = nullptr;
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void ElementDeferralTests::ValidateLifetime()
    {
        ValidateDeclaringScopeDestroyedFirst();
        ValidateParentDestroyedFirst();
    }

    void ElementDeferralTests::ValidateCannotRealizeFromNonDeclaringScope()
    {
        TestCleanupWrapper cleanup;

        Grid^ root = LoadXamlEnterAndWait<Grid>(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"      x:Name='declaringScope'>"
            L"  <StackPanel x:Name='parent'>"
            L"    <TextBlock x:Name='element' x:DeferLoadStrategy='Lazy'/>"
            L"  </StackPanel>"
            L"</Grid>");

        StackPanel^ parent = nullptr;

        RunOnUIThread([&]()
        {
            parent = safe_cast<StackPanel^>(root->FindName(L"parent"));
            VERIFY_IS_NOT_NULL(parent);
            root->Children->Clear();
            TestServices::WindowHelper->WindowContent = nullptr;
        });

        TestServices::WindowHelper->WaitForIdle();

        Grid^ root2 = LoadXamlEnterAndWait<Grid>(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"      x:Name='nonDeclaringScope'/>");

        RunOnUIThread([&]()
        {
            root2->Children->Append(parent);
            TextBlock^ element = safe_cast<TextBlock^>(root2->FindName(L"element"));
            VERIFY_IS_NULL(element);
        });
    }

    void ElementDeferralTests::ValidateCanRealizeWhenChildrenCollectionLocked()
    {
        using namespace ::Tests::Native::External::Framework::XDefer;

        TestCleanupWrapper cleanup;

        CustomPageNested^ page = nullptr;
        CustomControl^ customControl = nullptr;
        Border^ level0Border = nullptr;
        StackPanel^ topLevelStackPanel = nullptr;
        Grid^ level1Grid = nullptr;

        RunOnUIThread([&]()
        {
            page = ref new CustomPageNested();

            topLevelStackPanel = safe_cast<StackPanel^>(page->FindName(L"topLevelStackPanel"));
            customControl = safe_cast<CustomControl^>(page->FindName(L"customControl2"));

            VERIFY_ARE_EQUAL(1U, topLevelStackPanel->Children->Size);
            VERIFY_ARE_EQUAL(customControl, safe_cast<CustomControl^>(topLevelStackPanel->Children->GetAt(0)));

            s_measureCallback = [&](CustomControl^ inst)
            {
                // realize border, but it cannot be inserted into collection yet.
                level0Border = safe_cast<Border^>(safe_cast<FrameworkElement^>(inst->Parent)->FindName(L"level0Border"));
                VERIFY_IS_NOT_NULL(level0Border);
                VERIFY_ARE_EQUAL(1U, topLevelStackPanel->Children->Size);
                VERIFY_ARE_EQUAL(customControl, safe_cast<CustomControl^>(topLevelStackPanel->Children->GetAt(0)));

                // second call should not fail, not add another instance.
                Border^ level0Border_2ndCall = safe_cast<Border^>(safe_cast<FrameworkElement^>(inst->Parent)->FindName(L"level0Border"));
                VERIFY_ARE_EQUAL(level0Border, level0Border_2ndCall);
                VERIFY_ARE_EQUAL(1U, topLevelStackPanel->Children->Size);
                VERIFY_ARE_EQUAL(customControl, safe_cast<CustomControl^>(topLevelStackPanel->Children->GetAt(0)));
            };

            s_onApplyTemplateCallback = [&](CustomControl^ inst)
            {
                // realized nested element in control's template
                level1Grid = safe_cast<Grid^>(inst->PublicGetTemplateChild(L"level1Grid"));
            };
        });

        EnterAndWait(page);

        RunOnUIThread([&]()
        {
            // level0Border should now be inserted
            VERIFY_ARE_EQUAL(2U, topLevelStackPanel->Children->Size);
            VERIFY_ARE_EQUAL(level0Border, safe_cast<Border^>(topLevelStackPanel->Children->GetAt(0)));
            VERIFY_ARE_EQUAL(customControl, safe_cast<CustomControl^>(topLevelStackPanel->Children->GetAt(1)));
            // template binding should work
            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(customControl->Content), safe_cast<Platform::String^>(level1Grid->Tag));
        });

        TestServices::WindowHelper->WaitForIdle();

        s_measureCallback = nullptr;
        s_onApplyTemplateCallback = nullptr;
    }

    #pragma endregion

    #pragma region Deferral of different types of elements

    static String GenerateXamlForDeferralForElementTypesInContainer(
        const wchar_t* pRootClass,
        const wchar_t* pContainerClass,
        const wchar_t* pContainerProperty,
        const wchar_t* pControlClass)
    {
        return String().Format(
            L"<%s xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <%s x:Name='container'>"
            L"    <%s.%s>"
            L"      <%s x:Name='deferred0' x:DeferLoadStrategy='Lazy'/>"
            L"    </%s.%s>"
            L"  </%s>"
            L"</%s>",
            pRootClass,
            pContainerClass,
            pContainerClass,
            pContainerProperty,
            pControlClass,
            pContainerClass,
            pContainerProperty,
            pContainerClass,
            pRootClass);
    }

    static String GenerateXamlForDeferralForElementTypesInFlyout(
        const wchar_t* pContainerClass,
        const wchar_t* pControlClass)
    {
        return String().Format(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Width='600' Height='600'>"
            L"  <Button x:Name='container'>"
            L"    <Button.Flyout>"
            L"      <%s>"
            L"        <%s x:Name='deferred0' x:DeferLoadStrategy='Lazy'/>"
            L"      </%s>"
            L"    </Button.Flyout>"
            L"  </Button>"
            L"</Grid>",
            pContainerClass,
            pControlClass,
            pContainerClass);
    }

    static String GenerateXamlForDeferralOfFlyout(
        const wchar_t* pContainerClass,
        const wchar_t* pContainerProperty,
        const wchar_t* pContentClass,
        const wchar_t* pControlClass)
    {
        return String().Format(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Width='600' Height='600'>"
            L"  <Button x:Name='container'>"
            L"    <%s.%s>"
            L"      <%s x:Name='deferred0' x:DeferLoadStrategy='Lazy'>"
            L"        <%s/>"
            L"      </%s>"
            L"    </%s.%s>"
            L"  </Button>"
            L"</Grid>",
            pContainerClass,
            pContainerProperty,
            pControlClass,
            pContentClass,
            pControlClass,
            pContainerClass,
            pContainerProperty);
    }

    static String GenerateXamlForDeferralForElementTypesInAppBar(
        const wchar_t* pControlClass)
    {
        return String().Format(
            L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <Page.TopAppBar>"
            L"    <AppBar>"
            L"      <%s x:Name='deferred0' x:DeferLoadStrategy='Lazy'/>"
            L"    </AppBar>"
            L"  </Page.TopAppBar>"
            L"  <Page.BottomAppBar>"
            L"    <AppBar>"
            L"      <%s x:Name='deferred1' x:DeferLoadStrategy='Lazy'/>"
            L"    </AppBar>"
            L"  </Page.BottomAppBar>"
            L"</Page>",
            pControlClass,
            pControlClass);
    }

    static String GenerateXamlForDeferralForElementTypesInCommandBar(
        const wchar_t* pControlClass,
        const wchar_t* pContainerProperty)
    {
        return String().Format(
            L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <Page.TopAppBar>"
            L"    <CommandBar ClosedDisplayMode='Compact' x:Name='container0'>"
            L"      <CommandBar.%s>"
            L"        <%s Icon='Accept'/>"
            L"        <%s x:Name='deferred0' Icon='Delete' x:DeferLoadStrategy='Lazy'/>"
            L"        <%s Icon='Pause'/>"
            L"      </CommandBar.%s>"
            L"    </CommandBar>"
            L"  </Page.TopAppBar>"
            L"  <Page.BottomAppBar>"
            L"    <CommandBar ClosedDisplayMode='Compact' x:Name='container1'>"
            L"      <CommandBar.%s>"
            L"        <%s Icon='Accept'/>"
            L"        <%s x:Name='deferred1' Icon='Delete' x:DeferLoadStrategy='Lazy'/>"
            L"        <%s Icon='Pause'/>"
            L"      </CommandBar.%s>"
            L"    </CommandBar>"
            L"  </Page.BottomAppBar>"
            L"</Page>",
            pContainerProperty,
            pControlClass,
            pControlClass,
            pControlClass,
            pContainerProperty,
            pContainerProperty,
            pControlClass,
            pControlClass,
            pControlClass,
            pContainerProperty);
    }

    static String GenerateXamlForDeferralForElementTypesInSeZo(
        const wchar_t* pRootClass,
        bool isZoomedInViewActive,
        const wchar_t* pContainerTestedProperty,
        const wchar_t* pContainerInertProperty,
        const wchar_t* pControlClass)
    {
        return String().Format(
            L"<%s xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <SemanticZoom x:Name='container' IsZoomedInViewActive='%s'>"
            L"    <SemanticZoom.%s>"
            L"      <%s x:Name='deferred0' x:DeferLoadStrategy='Lazy'>"
            L"        <TextBlock Text='in deferred 0'/>"
            L"        <TextBlock Text='in deferred 1'/>"
            L"        <TextBlock Text='in deferred 2'/>"
            L"      </%s>"
            L"    </SemanticZoom.%s>"
            L"    <SemanticZoom.%s>"
            L"      <%s x:Name='inert'>"
            L"        <TextBlock Text='in inert 0'/>"
            L"        <TextBlock Text='in inert 1'/>"
            L"        <TextBlock Text='in inert 2'/>"
            L"      </%s>"
            L"    </SemanticZoom.%s>"
            L"  </SemanticZoom>"
            L"</%s>",
            pRootClass,
            (isZoomedInViewActive) ? L"True" : L"False",
            pContainerTestedProperty,
            pControlClass,
            pControlClass,
            pContainerTestedProperty,
            pContainerInertProperty,
            pControlClass,
            pControlClass,
            pContainerInertProperty,
            pRootClass);
    }

    static String GenerateXamlForClearCollectionScenario(
        const wchar_t* pRootClass,
        const wchar_t* pContainerClass,
        const wchar_t* pContainerProperty,
        const wchar_t* pControlClass)
    {
        return String().Format(
            L"<%s xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <%s x:Name='container'>"
            L"    <%s.%s>"
            L"      <%s x:Name='deferred0' x:DeferLoadStrategy='Lazy'/>"
            L"      <%s x:Name='deferred1' x:DeferLoadStrategy='Lazy'/>"
            L"      <%s x:Name='deferred2' x:DeferLoadStrategy='Lazy'/>"
            L"    </%s.%s>"
            L"  </%s>"
            L"</%s>",
            pRootClass,
            pContainerClass,
            pContainerClass,
            pContainerProperty,
            pControlClass,
            pControlClass,
            pControlClass,
            pContainerClass,
            pContainerProperty,
            pContainerClass,
            pRootClass);
    }

    static String GenerateXamlForClearCollectionScenarioInMenuFlyout(
        const wchar_t* pControlClass)
    {
        return String().Format(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Width='600' Height='600'>"
            L"  <Button x:Name='container'>"
            L"    <Button.Flyout>"
            L"      <MenuFlyout>"
            L"        <%s x:Name='deferred0' x:DeferLoadStrategy='Lazy'/>"
            L"        <%s x:Name='deferred1' x:DeferLoadStrategy='Lazy'/>"
            L"        <%s x:Name='deferred2' x:DeferLoadStrategy='Lazy'/>"
            L"      </MenuFlyout>"
            L"    </Button.Flyout>"
            L"  </Button>"
            L"</Grid>",
            pControlClass,
            pControlClass,
            pControlClass);
    }

    // General scenario
    template <typename TOuterAccessor, typename TInnerElement>
    struct ValidateDeferralForElementTypesInContainerScenario
    {
        ValidateDeferralForElementTypesInContainerScenario()
        {
            typedef ElementTraits<Grid> TRootElementTraits;
            typedef EmptyAccessor<TInnerElement> TInnerAccessor;
            typedef PropertyTraits<TOuterAccessor::type, TOuterAccessor> TOuterPropertyTraits;
            typedef PropertyTraits<TInnerAccessor::type, TInnerAccessor> TInnerPropertyTraits;

            TestCleanupWrapper cleanup;

            Log::Comment(String().Format(
                L"* Testing %s in %s.%s",
                TInnerPropertyTraits::get_class_name(),
                TOuterPropertyTraits::get_class_name(),
                TOuterPropertyTraits::get_property_name()));

            TRootElementTraits::type^ root = LoadXamlEnterAndWait<TRootElementTraits::type>(
                (const wchar_t*)GenerateXamlForDeferralForElementTypesInContainer(
                    TRootElementTraits::get_class_name(),
                    TOuterPropertyTraits::get_class_name(),
                    TOuterPropertyTraits::get_property_name(),
                    TInnerPropertyTraits::get_class_name()).GetBuffer());

            TOuterPropertyTraits::type^ container = nullptr;
            TInnerPropertyTraits::type^ deferred0 = nullptr;
            WeakReference deferred0WeakRef;

            RunOnUIThread([&]()
            {
                container = safe_cast<TOuterPropertyTraits::type^>(root->FindName(L"container"));
                VERIFY_ARE_EQUAL(0U, TOuterPropertyTraits::get_size(container));

                deferred0 = safe_cast<TInnerPropertyTraits::type^>(container->FindName(GetElementName(ElementType::Deferred, 0)));
                VERIFY_IS_NOT_NULL(deferred0);
                deferred0WeakRef = WeakReference(deferred0);

                VERIFY_ARE_EQUAL(1U, TOuterPropertyTraits::get_size(container));
                VERIFY_IS_TRUE(safe_cast<TInnerPropertyTraits::type^>(TOuterPropertyTraits::get_value(container)) == deferred0);

                Markup::XamlMarkupHelper::UnloadObject(deferred0);
                deferred0 = nullptr;

                VERIFY_ARE_EQUAL(0U, TOuterPropertyTraits::get_size(container));
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_NULL(deferred0WeakRef.Resolve<TInnerPropertyTraits::type>());
            });

            RunOnUIThread([&]()
            {
                deferred0 = safe_cast<TInnerPropertyTraits::type^>(container->FindName(GetElementName(ElementType::Deferred, 0)));
                VERIFY_IS_NOT_NULL(deferred0);
                VERIFY_IS_NULL(deferred0WeakRef.Resolve<TInnerPropertyTraits::type>());

                VERIFY_ARE_EQUAL(1U, TOuterPropertyTraits::get_size(container));
            });

            TestServices::WindowHelper->WaitForIdle();
        }
    };

    // Anything in AppBar scenario
    template <typename TInnerElement>
    struct ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<AppBar>, TInnerElement>
    {
        ValidateDeferralForElementTypesInContainerScenario()
        {
            typedef ElementTraits<Page> TRootElementTraits;
            typedef ContentAccessor<AppBar> TOuterAccessor;
            typedef EmptyAccessor<TInnerElement> TInnerAccessor;
            typedef PropertyTraits<TOuterAccessor::type, TOuterAccessor> TOuterPropertyTraits;
            typedef PropertyTraits<TInnerAccessor::type, TInnerAccessor> TInnerPropertyTraits;

            TestCleanupWrapper cleanup;

            Log::Comment(String().Format(
                L"* Testing %s in %s.%s",
                TInnerPropertyTraits::get_class_name(),
                TOuterPropertyTraits::get_class_name(),
                TOuterPropertyTraits::get_property_name()));

            auto spTopOpenedEvent = std::make_shared<Event>();
            auto spTopClosedEvent = std::make_shared<Event>();
            auto spBottomOpenedEvent = std::make_shared<Event>();
            auto spBottomClosedEvent = std::make_shared<Event>();

            auto topOpenedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Opened);
            auto topClosedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Closed);
            auto bottomOpenedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Opened);
            auto bottomClosedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Closed);

            TRootElementTraits::type^ root = LoadXamlEnterAndWait<TRootElementTraits::type>(
                GenerateXamlForDeferralForElementTypesInAppBar(
                    TInnerPropertyTraits::get_class_name()));

            TInnerPropertyTraits::type^ deferred0 = nullptr;
            TInnerPropertyTraits::type^ deferred1 = nullptr;

            WeakReference deferred0WeakRef;
            WeakReference deferred1WeakRef;

            RunOnUIThread([&]()
            {
                topOpenedRegistration.Attach(root->TopAppBar, ref new wf::EventHandler<Platform::Object^>([spTopOpenedEvent](Platform::Object^ sender, Platform::Object^ e)
                {
                    spTopOpenedEvent->Set();
                }));

                topClosedRegistration.Attach(root->TopAppBar, ref new wf::EventHandler<Platform::Object^>([spTopClosedEvent](Platform::Object^ sender, Platform::Object^ e)
                {
                    spTopClosedEvent->Set();
                }));

                bottomOpenedRegistration.Attach(root->BottomAppBar, ref new wf::EventHandler<Platform::Object^>([spBottomOpenedEvent](Platform::Object^ sender, Platform::Object^ e)
                {
                    spBottomOpenedEvent->Set();
                }));

                bottomClosedRegistration.Attach(root->BottomAppBar, ref new wf::EventHandler<Platform::Object^>([spBottomClosedEvent](Platform::Object^ sender, Platform::Object^ e)
                {
                    spBottomClosedEvent->Set();
                }));
            });

            RunOnUIThread([&]()
            {
                deferred0 = safe_cast<TInnerPropertyTraits::type^>(root->FindName(GetElementName(ElementType::Deferred, 0)));
                VERIFY_IS_NOT_NULL(deferred0);
                deferred0WeakRef = WeakReference(deferred0);

                VERIFY_IS_TRUE(safe_cast<TInnerPropertyTraits::type^>(root->TopAppBar->Content) == deferred0);
                root->TopAppBar->IsOpen = true;
            });

            TestServices::WindowHelper->WaitForIdle();
            spTopOpenedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                root->TopAppBar->IsOpen = false;
            });

            spTopClosedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                deferred1 = safe_cast<TInnerPropertyTraits::type^>(root->FindName(GetElementName(ElementType::Deferred, 1)));
                VERIFY_IS_NOT_NULL(deferred1);
                deferred1WeakRef = WeakReference(deferred1);

                VERIFY_IS_TRUE(safe_cast<TInnerPropertyTraits::type^>(root->TopAppBar->Content) == deferred0);
                VERIFY_IS_TRUE(safe_cast<TInnerPropertyTraits::type^>(root->BottomAppBar->Content) == deferred1);
                root->BottomAppBar->IsOpen = true;
            });

            TestServices::WindowHelper->WaitForIdle();
            spBottomOpenedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                root->BottomAppBar->IsOpen = false;
            });

            spBottomClosedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                Markup::XamlMarkupHelper::UnloadObject(deferred0);
                deferred0 = nullptr;

                Markup::XamlMarkupHelper::UnloadObject(deferred1);
                deferred1 = nullptr;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_NULL(deferred0WeakRef.Resolve<TInnerPropertyTraits::type>());
                VERIFY_IS_NULL(deferred1WeakRef.Resolve<TInnerPropertyTraits::type>());
            });
        }
    };

    // AppBar in Page scenario
    template <typename TOuterAccessor>
    struct ValidateDeferralForElementTypesInPageScenario
    {
        ValidateDeferralForElementTypesInPageScenario()
        {
            typedef ElementTraits<Grid> TRootElementTraits;
            typedef EmptyAccessor<AppBar> TInnerAccessor;
            typedef PropertyTraits<TOuterAccessor::type, TOuterAccessor> TOuterPropertyTraits;
            typedef PropertyTraits<TInnerAccessor::type, TInnerAccessor> TInnerPropertyTraits;

            TestCleanupWrapper cleanup;

            Log::Comment(String().Format(
                L"* Testing %s in %s.%s",
                TInnerPropertyTraits::get_class_name(),
                TOuterPropertyTraits::get_class_name(),
                TOuterPropertyTraits::get_property_name()));

            TRootElementTraits::type^ root = LoadXamlEnterAndWait<TRootElementTraits::type>(
                (const wchar_t*)GenerateXamlForDeferralForElementTypesInContainer(
                    TRootElementTraits::get_class_name(),
                    TOuterPropertyTraits::get_class_name(),
                    TOuterPropertyTraits::get_property_name(),
                    TInnerPropertyTraits::get_class_name()).GetBuffer());

            TOuterPropertyTraits::type^ container = nullptr;
            TInnerPropertyTraits::type^ deferred0 = nullptr;
            WeakReference deferred0WeakRef;

            auto openedEvent = std::make_shared<Event>();
            auto closedEvent = std::make_shared<Event>();

            auto openedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Opened);
            auto closedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Closed);

            RunOnUIThread([&]()
            {
                container = safe_cast<TOuterPropertyTraits::type^>(root->FindName(L"container"));
                VERIFY_ARE_EQUAL(0U, TOuterPropertyTraits::get_size(container));

                deferred0 = safe_cast<TInnerPropertyTraits::type^>(container->FindName(GetElementName(ElementType::Deferred, 0)));
                VERIFY_IS_NOT_NULL(deferred0);
                deferred0WeakRef = WeakReference(deferred0);

                VERIFY_ARE_EQUAL(1U, TOuterPropertyTraits::get_size(container));
                VERIFY_IS_TRUE(safe_cast<TInnerPropertyTraits::type^>(TOuterPropertyTraits::get_value(container)) == deferred0);

                openedRegistration.Attach(deferred0, ref new wf::EventHandler<Platform::Object^>([openedEvent](Platform::Object^ sender, Platform::Object^ e)
                {
                    openedEvent->Set();
                }));

                closedRegistration.Attach(deferred0, ref new wf::EventHandler<Platform::Object^>([closedEvent](Platform::Object^ sender, Platform::Object^ e)
                {
                    closedEvent->Set();
                }));

                deferred0->IsOpen = true;
            });

            TestServices::WindowHelper->WaitForIdle();
            openedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                deferred0->IsOpen = false;
            });

            closedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                Markup::XamlMarkupHelper::UnloadObject(deferred0);
                deferred0 = nullptr;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // TODO[BK] leaks +1 ref - add bug
                // VERIFY_IS_NULL(deferred0WeakRef.Resolve<TInnerPropertyTraits::type>());
            });

            TestServices::WindowHelper->WaitForIdle();
        }
    };

    // ToggleSwitch On/Off content
    template <typename TOuterAccessor, typename TInnerElement>
    static void ValidateDeferralForElementTypesInToggleSwitchScenario()
    {
        typedef ElementTraits<Grid> TRootElementTraits;
        typedef EmptyAccessor<TInnerElement> TInnerAccessor;
        typedef PropertyTraits<TOuterAccessor::type, TOuterAccessor> TOuterPropertyTraits;
        typedef PropertyTraits<TInnerAccessor::type, TInnerAccessor> TInnerPropertyTraits;

        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(
            L"* Testing %s in %s.%s",
            TInnerPropertyTraits::get_class_name(),
            TOuterPropertyTraits::get_class_name(),
            TOuterPropertyTraits::get_property_name()));

        TRootElementTraits::type^ root = LoadXamlEnterAndWait<TRootElementTraits::type>(
            (const wchar_t*)GenerateXamlForDeferralForElementTypesInContainer(
                TRootElementTraits::get_class_name(),
                TOuterPropertyTraits::get_class_name(),
                TOuterPropertyTraits::get_property_name(),
                TInnerPropertyTraits::get_class_name()).GetBuffer());

        TOuterPropertyTraits::type^ container = nullptr;
        TInnerPropertyTraits::type^ deferred0 = nullptr;
        WeakReference deferred0WeakRef;

        RunOnUIThread([&]()
        {
            container = safe_cast<TOuterPropertyTraits::type^>(root->FindName(L"container"));
            VERIFY_ARE_EQUAL(1U, TOuterPropertyTraits::get_size(container));
            VERIFY_IS_NOT_NULL(safe_cast<Platform::String^>(TOuterPropertyTraits::get_value(container)));

            deferred0 = safe_cast<TInnerPropertyTraits::type^>(container->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(deferred0);
            deferred0WeakRef = WeakReference(deferred0);

            VERIFY_ARE_EQUAL(1U, TOuterPropertyTraits::get_size(container));
            VERIFY_IS_TRUE(safe_cast<TInnerPropertyTraits::type^>(TOuterPropertyTraits::get_value(container)) == deferred0);

            Markup::XamlMarkupHelper::UnloadObject(deferred0);
            deferred0 = nullptr;

            VERIFY_ARE_EQUAL(0U, TOuterPropertyTraits::get_size(container));
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_NULL(deferred0WeakRef.Resolve<TInnerPropertyTraits::type>());

            deferred0 = safe_cast<TInnerPropertyTraits::type^>(container->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(deferred0);

            VERIFY_ARE_EQUAL(1U, TOuterPropertyTraits::get_size(container));
            VERIFY_IS_TRUE(safe_cast<TInnerPropertyTraits::type^>(TOuterPropertyTraits::get_value(container)) == deferred0);
            VERIFY_IS_NULL(deferred0WeakRef.Resolve<TInnerPropertyTraits::type>());
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    // Anything in FlyoutBase
    template <typename TFlyoutAccessor, typename TInnerElement>
    static void ValidateDeferralForElementTypesInFlyoutBaseScenario()
    {
        typedef ElementTraits<Grid> TRootElementTraits;
        typedef FlyoutAccessor<Button> TOuterAccessor;
        typedef EmptyAccessor<TInnerElement> TInnerAccessor;
        typedef PropertyTraits<TOuterAccessor::type, TOuterAccessor> TOuterPropertyTraits;
        typedef PropertyTraits<TFlyoutAccessor::type, TFlyoutAccessor> TFlyoutPropertyTraits;
        typedef PropertyTraits<TInnerAccessor::type, TInnerAccessor> TInnerPropertyTraits;

        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(
            L"* Testing %s in %s.%s",
            TInnerPropertyTraits::get_class_name(),
            TFlyoutPropertyTraits::get_class_name(),
            TFlyoutPropertyTraits::get_property_name()));

        TRootElementTraits::type^ root = LoadXamlEnterAndWait<TRootElementTraits::type>(
            (const wchar_t*)GenerateXamlForDeferralForElementTypesInFlyout(
                TFlyoutPropertyTraits::get_class_name(),
                TInnerPropertyTraits::get_class_name()).GetBuffer());

        TOuterPropertyTraits::type^ container = nullptr;
        TFlyoutPropertyTraits::type^ flyout = nullptr;
        TInnerPropertyTraits::type^ deferred0 = nullptr;
        WeakReference deferred0WeakRef;

        auto flyoutOpenedEvent = std::make_shared<Event>();
        auto flyoutClosedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(TFlyoutPropertyTraits::type, Opened);
        auto closedRegistration = CreateSafeEventRegistration(TFlyoutPropertyTraits::type, Closed);

        RunOnUIThread([&]()
        {
            container = safe_cast<TOuterPropertyTraits::type^>(root->FindName(L"container"));
            VERIFY_IS_NOT_NULL(container);

            flyout = safe_cast<TFlyoutPropertyTraits::type^>(container->Flyout);
            VERIFY_IS_NOT_NULL(flyout);

            VERIFY_ARE_EQUAL(0U, TFlyoutPropertyTraits::get_size(flyout));

            deferred0 = safe_cast<TInnerPropertyTraits::type^>(root->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(deferred0);
            deferred0WeakRef = WeakReference(deferred0);

            VERIFY_IS_TRUE(safe_cast<TInnerPropertyTraits::type^>(TFlyoutPropertyTraits::get_value(flyout)) == deferred0);

            openedRegistration.Attach(flyout, ref new wf::EventHandler<Platform::Object^>([flyoutOpenedEvent](Platform::Object^, Platform::Object^)
            {
                flyoutOpenedEvent->Set();
            }));

            closedRegistration.Attach(flyout, ref new wf::EventHandler<Platform::Object^>([flyoutClosedEvent](Platform::Object^, Platform::Object^)
            {
                flyoutClosedEvent->Set();
            }));

            container->Flyout->ShowAt(container);
        });

        TestServices::WindowHelper->WaitForIdle();

        flyoutOpenedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            flyout->Hide();
        });

        flyoutClosedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            Markup::XamlMarkupHelper::UnloadObject(deferred0);
            deferred0 = nullptr;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_NULL(deferred0WeakRef.Resolve<TInnerPropertyTraits::type>());
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    // FlyoutBase in Button.Flyout or AttachedFlyout
    template <typename TOuterAccessor, typename TPresenterAccessor, typename TFlyoutElement, typename TContentElement>
    static void ValidateDeferralForFlyoutBaseInContainerScenario()
    {
        typedef ElementTraits<Grid> TRootElementTraits;
        typedef EmptyAccessor<TFlyoutElement> TFlyoutAccessor;
        typedef EmptyAccessor<TContentElement> TContentAccessor;
        typedef PropertyTraits<TOuterAccessor::type, TOuterAccessor> TOuterPropertyTraits;
        typedef PropertyTraits<TPresenterAccessor::type, TPresenterAccessor> TPresenterPropertyTraits;
        typedef PropertyTraits<TFlyoutAccessor::type, TFlyoutAccessor> TFlyoutPropertyTraits;
        typedef PropertyTraits<TContentAccessor::type, TContentAccessor> TContentPropertyTraits;

        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(
            L"* Testing %s in %s.%s",
            TFlyoutPropertyTraits::get_class_name(),
            TOuterPropertyTraits::get_class_name(),
            TOuterPropertyTraits::get_property_name()));

        TRootElementTraits::type^ root = LoadXamlEnterAndWait<TRootElementTraits::type>(
            (const wchar_t*)GenerateXamlForDeferralOfFlyout(
                TOuterPropertyTraits::get_class_name(),
                TOuterPropertyTraits::get_property_name(),
                TContentPropertyTraits::get_class_name(),
                TFlyoutPropertyTraits::get_class_name()).GetBuffer());

        TOuterPropertyTraits::type^ container = nullptr;
        TFlyoutPropertyTraits::type^ deferred0 = nullptr;
        WeakReference deferred0WeakRef;

        auto flyoutOpenedEvent = std::make_shared<Event>();
        auto flyoutClosedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(TFlyoutPropertyTraits::type, Opened);
        auto closedRegistration = CreateSafeEventRegistration(TFlyoutPropertyTraits::type, Closed);

        RunOnUIThread([&]()
        {
            container = safe_cast<TOuterPropertyTraits::type^>(root->FindName(L"container"));
            VERIFY_IS_NOT_NULL(container);

            deferred0 = safe_cast<TFlyoutPropertyTraits::type^>(TOuterPropertyTraits::get_value(container));
            VERIFY_IS_NULL(deferred0);

            deferred0 = safe_cast<TFlyoutPropertyTraits::type^>(root->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(deferred0);
            deferred0WeakRef = WeakReference(deferred0);

            VERIFY_IS_TRUE(safe_cast<TFlyoutPropertyTraits::type^>(TOuterPropertyTraits::get_value(container)) == deferred0);

            openedRegistration.Attach(deferred0, ref new wf::EventHandler<Platform::Object^>([flyoutOpenedEvent](Platform::Object^, Platform::Object^)
            {
                flyoutOpenedEvent->Set();
            }));

            closedRegistration.Attach(deferred0, ref new wf::EventHandler<Platform::Object^>([flyoutClosedEvent](Platform::Object^, Platform::Object^)
            {
                flyoutClosedEvent->Set();
            }));

            TOuterPropertyTraits::execute_command(container);
        });

        TestServices::WindowHelper->WaitForIdle();

        flyoutOpenedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            auto popups = VisualTreeHelper::GetOpenPopupsForXamlRoot(root->XamlRoot);
            VERIFY_IS_TRUE(popups->Size == 1);
            auto popup = popups->GetAt(0);
            TPresenterPropertyTraits::type^ presenter = safe_cast<TPresenterPropertyTraits::type^>(popup->Child);
            VERIFY_IS_NOT_NULL(dynamic_cast<TContentPropertyTraits::type^>(TPresenterPropertyTraits::get_value(presenter)));
            deferred0->Hide();
        });

        flyoutClosedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            Markup::XamlMarkupHelper::UnloadObject(deferred0);
            deferred0 = nullptr;

            // TODO[BK] +2 outstanding refs
            // VERIFY_IS_NULL(deferred0WeakRef.Resolve<TFlyoutPropertyTraits::type>());
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    // Any ICommandBarElement in CommandBar scenario
    template <typename TOuterAccessor, typename TInnerElement>
    struct ValidateDeferralForElementTypesInCommandBarScenario
    {
        ValidateDeferralForElementTypesInCommandBarScenario()
        {
            typedef ElementTraits<Page> TRootElementTraits;
            typedef EmptyAccessor<TInnerElement> TInnerAccessor;
            typedef PropertyTraits<TOuterAccessor::type, TOuterAccessor> TOuterPropertyTraits;
            typedef PropertyTraits<TInnerAccessor::type, TInnerAccessor> TInnerPropertyTraits;

            TestCleanupWrapper cleanup;

            Log::Comment(String().Format(
                L"* Testing %s in %s.%s",
                TInnerPropertyTraits::get_class_name(),
                TOuterPropertyTraits::get_class_name(),
                TOuterPropertyTraits::get_property_name()));

            TRootElementTraits::type^ root = LoadXamlEnterAndWait<TRootElementTraits::type>(
                GenerateXamlForDeferralForElementTypesInCommandBar(
                    TInnerPropertyTraits::get_class_name(),
                    TOuterPropertyTraits::get_property_name()));

            TOuterPropertyTraits::type^ container0 = nullptr;
            TOuterPropertyTraits::type^ container1 = nullptr;
            TInnerPropertyTraits::type^ deferred0 = nullptr;
            TInnerPropertyTraits::type^ deferred1 = nullptr;
            WeakReference deferred0WeakRef;
            WeakReference deferred1WeakRef;

            RunOnUIThread([&]()
            {
                container0 = safe_cast<TOuterPropertyTraits::type^>(root->FindName(L"container0"));
                VERIFY_ARE_EQUAL(2U, TOuterPropertyTraits::get_size(container0));

                deferred0 = safe_cast<TInnerPropertyTraits::type^>(root->FindName(GetElementName(ElementType::Deferred, 0)));
                VERIFY_IS_NOT_NULL(deferred0);
                deferred0WeakRef = WeakReference(deferred0);

                VERIFY_ARE_EQUAL(3U, TOuterPropertyTraits::get_size(container0));
                VERIFY_IS_TRUE(safe_cast<TInnerPropertyTraits::type^>(TOuterPropertyTraits::get_value(container0, 1)) == deferred0);

                container1 = safe_cast<TOuterPropertyTraits::type^>(root->FindName(L"container1"));
                VERIFY_ARE_EQUAL(2U, TOuterPropertyTraits::get_size(container1));

                deferred1 = safe_cast<TInnerPropertyTraits::type^>(root->FindName(GetElementName(ElementType::Deferred, 1)));
                VERIFY_IS_NOT_NULL(deferred1);
                deferred1WeakRef = WeakReference(deferred1);

                VERIFY_ARE_EQUAL(3U, TOuterPropertyTraits::get_size(container1));
                VERIFY_IS_TRUE(safe_cast<TInnerPropertyTraits::type^>(TOuterPropertyTraits::get_value(container1, 1)) == deferred1);

                Markup::XamlMarkupHelper::UnloadObject(deferred0);
                deferred0 = nullptr;
                VERIFY_ARE_EQUAL(2U, TOuterPropertyTraits::get_size(container0));

                Markup::XamlMarkupHelper::UnloadObject(deferred1);
                deferred1 = nullptr;
                VERIFY_ARE_EQUAL(2U, TOuterPropertyTraits::get_size(container1));
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_NULL(deferred0WeakRef.Resolve<TInnerPropertyTraits::type>());
                VERIFY_IS_NULL(deferred1WeakRef.Resolve<TInnerPropertyTraits::type>());
            });

            TestServices::WindowHelper->WaitForIdle();
        }
    };

    // SeZo scenario
    template <typename TTestedAccessor, typename TInertAccessor>
    static void ValidateDeferralForElementTypesInSeZoScenario()
    {
        typedef ElementTraits<Grid> TRootElementTraits;
        typedef EmptyAccessor<ListView> TInnerAccessor;
        typedef PropertyTraits<TTestedAccessor::type, TTestedAccessor> TTestedPropertyTraits;
        typedef PropertyTraits<TInertAccessor::type, TInertAccessor> TInertPropertyTraits;
        typedef PropertyTraits<TInnerAccessor::type, TInnerAccessor> TInnerPropertyTraits;

        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(
            L"* Testing %s in %s.%s",
            TInnerPropertyTraits::get_class_name(),
            TTestedPropertyTraits::get_class_name(),
            TTestedPropertyTraits::get_property_name()));

        TRootElementTraits::type^ root = LoadXamlEnterAndWait<TRootElementTraits::type>(
            (const wchar_t*)GenerateXamlForDeferralForElementTypesInSeZo(
                    TRootElementTraits::get_class_name(),
                    std::is_same<ZoomedInViewAccessor<SemanticZoom>, TInertAccessor>::value,
                    TTestedPropertyTraits::get_property_name(),
                    TInertPropertyTraits::get_property_name(),
                    TInnerPropertyTraits::get_class_name()).GetBuffer());

        TTestedPropertyTraits::type^ container = nullptr;
        TInnerPropertyTraits::type^ deferred0 = nullptr;
        TInnerPropertyTraits::type^ inert = nullptr;
        WeakReference deferred0WeakRef;

        auto viewChangeStartedEvent = std::make_shared<Event>();
        auto viewChangeStartedRegistration = CreateSafeEventRegistration(xaml_controls::SemanticZoom, ViewChangeStarted);

        RunOnUIThread([&]()
        {
            container = safe_cast<TTestedPropertyTraits::type^>(root->FindName(L"container"));
            inert = safe_cast<TInnerPropertyTraits::type^>(root->FindName(L"inert"));

            VERIFY_ARE_EQUAL(0U, TTestedPropertyTraits::get_size(container));
            VERIFY_ARE_EQUAL(1U, TInertPropertyTraits::get_size(container));
            VERIFY_IS_TRUE(safe_cast<TInnerPropertyTraits::type^>(TInertPropertyTraits::get_value(container)) == inert);

            viewChangeStartedRegistration.Attach(
                container,
                ref new xaml_controls::SemanticZoomViewChangedEventHandler([&](Platform::Object^, xaml_controls::SemanticZoomViewChangedEventArgs^ args)
            {
                deferred0 = safe_cast<TInnerPropertyTraits::type^>(container->FindName(GetElementName(ElementType::Deferred, 0)));
                VERIFY_IS_NOT_NULL(deferred0);
                deferred0WeakRef = WeakReference(deferred0);

                VERIFY_ARE_EQUAL(1U, TTestedPropertyTraits::get_size(container));
                VERIFY_ARE_EQUAL(1U, TInertPropertyTraits::get_size(container));
                VERIFY_IS_TRUE(safe_cast<TInnerPropertyTraits::type^>(TTestedPropertyTraits::get_value(container)) == deferred0);
                VERIFY_IS_TRUE(safe_cast<TInnerPropertyTraits::type^>(TInertPropertyTraits::get_value(container)) == inert);

                viewChangeStartedEvent->Set();
            }));

            container->ToggleActiveView();
        });

        viewChangeStartedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            container->ToggleActiveView();
        });

        viewChangeStartedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            Markup::XamlMarkupHelper::UnloadObject(deferred0);
            deferred0 = nullptr;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_NULL(deferred0WeakRef.Resolve<TInnerPropertyTraits::type>());
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    // General scenario
    template <typename TOuterAccessor, typename TInnerElement>
    struct ValidateDeferralForUnsupportedPropertiesThrowScenario
    {
        ValidateDeferralForUnsupportedPropertiesThrowScenario()
        {
            typedef ElementTraits<Grid> TRootElementTraits;
            typedef EmptyAccessor<TInnerElement> TInnerAccessor;
            typedef PropertyTraits<TOuterAccessor::type, TOuterAccessor> TOuterPropertyTraits;
            typedef PropertyTraits<TInnerAccessor::type, TInnerAccessor> TInnerPropertyTraits;

            TestCleanupWrapper cleanup;

            Log::Comment(String().Format(
                L"* Testing %s in %s.%s",
                TInnerPropertyTraits::get_class_name(),
                TOuterPropertyTraits::get_class_name(),
                TOuterPropertyTraits::get_property_name()));

            VERIFY_THROWS_WINRT(Markup::XamlReader::Load(
                ref new Platform::String(
                    (const wchar_t*)GenerateXamlForDeferralForElementTypesInContainer(
                        TRootElementTraits::get_class_name(),
                        TOuterPropertyTraits::get_class_name(),
                        TOuterPropertyTraits::get_property_name(),
                        TInnerPropertyTraits::get_class_name()).GetBuffer())),
                Platform::COMException^,
                L"XAML parse exception should be thrown");
        }
    };

    void ElementDeferralTests::ValidateDeferralForUnsupportedPropertiesThrow()
    {
        DisableErrorReportingScopeGuard disableErrors;

        ValidateDeferralForUnsupportedPropertiesThrowScenario<ContentAccessor<GridViewItemPresenter>, Button>();
        ValidateDeferralForUnsupportedPropertiesThrowScenario<ContentAccessor<ListViewItemPresenter>, Button>();
    }

    void ElementDeferralTests::ValidateDeferralForSupportedPropertiesInButtonBase()
    {
        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<AppBarButton>, Button>();
        ValidateDeferralForElementTypesInContainerScenario<IconAccessor<AppBarButton>, FontIcon>();

        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<AppBarToggleButton>, Button>();
        ValidateDeferralForElementTypesInContainerScenario<IconAccessor<AppBarToggleButton>, BitmapIcon>();

        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<Button>, Button>();
        ValidateDeferralForElementTypesInContainerScenario<CommandParameterAccessor<Button>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<CheckBox>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<HyperlinkButton>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<RadioButton>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<RepeatButton>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<ToggleButton>, Button>();
    }

    void ElementDeferralTests::ValidateDeferralForSupportedPropertiesInContentControl()
    {
        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<AppBar>, AppBarButton>();

        ValidateDeferralForElementTypesInCommandBarScenario<PrimaryCommandsAccessor<CommandBar>, AppBarButton>();
        ValidateDeferralForElementTypesInCommandBarScenario<SecondaryCommandsAccessor<CommandBar>, AppBarButton>();

        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<ContentControl>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<ContentDialog>, Button>();
        ValidateDeferralForElementTypesInContainerScenario<TitleAccessor<ContentDialog>, Button>();
        ValidateDeferralForElementTypesInContainerScenario<PrimaryButtonCommandParameterAccessor<ContentDialog>, Button>();
        ValidateDeferralForElementTypesInContainerScenario<SecondaryButtonCommandParameterAccessor<ContentDialog>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<FlyoutPresenter>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<Frame>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<GridViewHeaderItem>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<GroupItem>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<ListViewHeaderItem>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<ScrollViewer>, Button>();
        ValidateDeferralForElementTypesInContainerScenario<LeftHeaderAccessor<ScrollViewer>, Button>();
        ValidateDeferralForElementTypesInContainerScenario<TopHeaderAccessor<ScrollViewer>, Button>();
        ValidateDeferralForElementTypesInContainerScenario<TopLeftHeaderAccessor<ScrollViewer>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<ToolTip>, Button>();
    }

    void ElementDeferralTests::ValidateDeferralForSupportedPropertiesInControl()
    {
        ValidateDeferralForElementTypesInContainerScenario<HeaderAccessor<DatePicker>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<CommandParameterAccessor<MenuFlyoutItem>, Button>();

        // NEEDS TEST: ValidateDeferralForElementTypesInFlyoutBaseScenario<MenuItemsAccessor<MenuFlyoutSubItem>, MenuFlyoutItem>();

        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<Page>, Button>();

        ValidateDeferralForElementTypesInPageScenario<BottomAppBarAccessor<Page>>();
        ValidateDeferralForElementTypesInPageScenario<TopAppBarAccessor<Page>>();

        ValidateDeferralForElementTypesInContainerScenario<HeaderAccessor<PasswordBox>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<HeaderAccessor<RichEditBox>, Button>();

        ValidateDeferralForElementTypesInSeZoScenario<ZoomedInViewAccessor<SemanticZoom>, ZoomedOutViewAccessor<SemanticZoom>>();
        ValidateDeferralForElementTypesInSeZoScenario<ZoomedOutViewAccessor<SemanticZoom>, ZoomedInViewAccessor<SemanticZoom>>();

        ValidateDeferralForElementTypesInContainerScenario<HeaderAccessor<Slider>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<HeaderAccessor<TextBox>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<HeaderAccessor<TimePicker>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<HeaderAccessor<ToggleSwitch>, Button>();
        ValidateDeferralForElementTypesInToggleSwitchScenario<OffContentAccessor<ToggleSwitch>, Button>();
        ValidateDeferralForElementTypesInToggleSwitchScenario<OnContentAccessor<ToggleSwitch>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<UserControl>, Button>();
    }

    void ElementDeferralTests::ValidateDeferralForSupportedPropertiesInFlyoutBase()
    {
        // Leak: TemplateContent peer not being unpegged
        TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
        ValidateDeferralForFlyoutBaseInContainerScenario<FlyoutAccessor<Button>, ContentAccessor<FlyoutPresenter>, Flyout, ToggleSwitch>();
        ValidateDeferralForFlyoutBaseInContainerScenario<FlyoutAccessor<Button>, ItemsAccessor<MenuFlyoutPresenter>, MenuFlyout, ToggleMenuFlyoutItem>();

        ValidateDeferralForFlyoutBaseInContainerScenario<AttachedFlyoutAccessor<FrameworkElement>, ContentAccessor<FlyoutPresenter>, Flyout, ToggleSwitch>();
        ValidateDeferralForFlyoutBaseInContainerScenario<AttachedFlyoutAccessor<FrameworkElement>, ItemsAccessor<MenuFlyoutPresenter>, MenuFlyout, ToggleMenuFlyoutItem>();

        ValidateDeferralForElementTypesInFlyoutBaseScenario<ContentAccessor<Flyout>, Button>();

        ValidateDeferralForElementTypesInFlyoutBaseScenario<MenuItemsAccessor<MenuFlyout>, MenuFlyoutItem>();
    }

    void ElementDeferralTests::ValidateDeferralForSupportedPropertiesInFrameworkElement()
    {
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<ContentPresenter>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<FooterAccessor<ItemsPresenter>, Button>();
        // BUG: ValidateDeferralForElementTypesInContainerScenario<HeaderAccessor<ItemsPresenter>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Popup>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Viewbox>, Button>();
    }

    void ElementDeferralTests::ValidateDeferralForSupportedPropertiesInItemsControl()
    {
        ValidateDeferralForElementTypesInContainerScenario<HeaderAccessor<ComboBox>, Button>();
        ValidateDeferralForElementTypesInContainerScenario<ItemsAccessor<ComboBox>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ItemsAccessor<FlipView>, FlipViewItem>();

        ValidateDeferralForElementTypesInContainerScenario<ItemsAccessor<GridView>, GridViewItem>();
        ValidateDeferralForElementTypesInContainerScenario<FooterAccessor<GridView>, Button>();
        ValidateDeferralForElementTypesInContainerScenario<HeaderAccessor<GridView>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ItemsAccessor<ItemsControl>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ItemsAccessor<ListBox>, ListBoxItem>();

        ValidateDeferralForElementTypesInContainerScenario<ItemsAccessor<ListView>, ListViewItem>();
        ValidateDeferralForElementTypesInContainerScenario<FooterAccessor<ListView>, Button>();
        ValidateDeferralForElementTypesInContainerScenario<HeaderAccessor<ListView>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ItemsAccessor<MenuFlyoutPresenter>, MenuFlyoutItem>();
    }

    void ElementDeferralTests::ValidateDeferralForSupportedPropertiesInPanel()
    {
        ValidateDeferralForElementTypesInContainerScenario<ChildrenAccessor<Canvas>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ChildrenAccessor<Grid>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ChildrenAccessor<ItemsStackPanel>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ChildrenAccessor<ItemsWrapGrid>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ChildrenAccessor<RelativePanel>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ChildrenAccessor<StackPanel>, Button>();

        // BUG: ValidateDeferralForElementTypesInContainerScenario<ChildrenAccessor<SwapChainBackgroundPanel>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ChildrenAccessor<SwapChainPanel>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ChildrenAccessor<VariableSizedWrapGrid>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ChildrenAccessor<VirtualizingStackPanel>, Button>();

        // BUG: ValidateDeferralForElementTypesInContainerScenario<ChildrenAccessor<WrapGrid>, Button>();

        // BUG: ValidateDeferralForElementTypesInContainerScenario<ChildrenAccessor<CarouselPanel>, Button>();
    }

    void ElementDeferralTests::ValidateDeferralForSupportedPropertiesInSelectorItem()
    {
        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<ComboBoxItem>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<FlipViewItem>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<GridViewItem>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<ListBoxItem>, Button>();

        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<ListViewItem>, Button>();
    }

    void ElementDeferralTests::ValidateDeferralForSupportedTypesButtonBase()
    {
        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<AppBar>, AppBarButton>();
        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<AppBar>, AppBarSeparator>();
        ValidateDeferralForElementTypesInContainerScenario<ContentAccessor<AppBar>, AppBarToggleButton>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, Button>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, CheckBox>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, HyperlinkButton>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, RadioButton>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, RepeatButton>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, ToggleButton>();
    }

    void ElementDeferralTests::ValidateDeferralForSupportedTypesContentControl()
    {
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, AppBar>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, CommandBar>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, ContentControl>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, ContentDialog>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, FlyoutPresenter>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, Frame>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, GridViewHeaderItem>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, GroupItem>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, ListViewHeaderItem>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, ScrollViewer>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, ToolTip>();
    }

    void ElementDeferralTests::ValidateDeferralForSupportedTypesControl()
    {
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, DatePicker>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, MediaTransportControls>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, MenuFlyoutItem>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, MenuFlyoutSeparator>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, MenuFlyoutSubItem>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, Page>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, PasswordBox>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, RichEditBox>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, ScrollBar>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, SemanticZoom>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, Slider>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, TextBox>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, Thumb>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, TimePicker>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, ToggleMenuFlyoutItem>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, ToggleSwitch>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, UserControl>();
    }

    void ElementDeferralTests::ValidateDeferralForSupportedTypesFrameworkElement()
    {
        ValidateDeferralForElementTypesInContainerScenario<IconAccessor<AppBarToggleButton>, BitmapIcon>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, Border>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, ContentPresenter>();
        ValidateDeferralForElementTypesInContainerScenario<IconAccessor<AppBarToggleButton>, FontIcon>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, Image>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, ItemsPresenter>();
        ValidateDeferralForElementTypesInContainerScenario<IconAccessor<AppBarToggleButton>, PathIcon>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, Popup>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, RichTextBlock>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, RichTextBlockOverflow>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, ScrollContentPresenter>();
        ValidateDeferralForElementTypesInContainerScenario<IconAccessor<AppBarToggleButton>, SymbolIcon>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, TextBlock>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, TickBar>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, Viewbox>();
    }

    void ElementDeferralTests::ValidateDeferralForSupportedTypesInItemsControl()
    {
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, ComboBox>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, FlipView>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, GridView>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, ItemsControl>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, ListBox>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, ListView>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, MenuFlyoutPresenter>();
    }

    void ElementDeferralTests::ValidateDeferralForSupportedTypesPanel()
    {
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, Canvas>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, Grid>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, ItemsStackPanel>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, ItemsWrapGrid>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, RelativePanel>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, StackPanel>();
        // BUG: ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, SwapChainBackgroundPanel>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, SwapChainPanel>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, VariableSizedWrapGrid>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, VirtualizingStackPanel>();
        // BUG: ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, WrapGrid>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, CarouselPanel>();
    }

    void ElementDeferralTests::ValidateDeferralForSupportedTypesSelectorItem()
    {
        ValidateDeferralForElementTypesInContainerScenario<ItemsAccessor<ComboBox>, ComboBoxItem>();
        ValidateDeferralForElementTypesInContainerScenario<ItemsAccessor<FlipView>, FlipViewItem>();
        ValidateDeferralForElementTypesInContainerScenario<ItemsAccessor<GridView>, GridViewItem>();
        ValidateDeferralForElementTypesInContainerScenario<ItemsAccessor<ListBox>, ListBoxItem>();
        ValidateDeferralForElementTypesInContainerScenario<ItemsAccessor<ListView>, ListViewItem>();
    }

    void ElementDeferralTests::ValidateDeferralForSupportedTypesShapes()
    {
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, Microsoft::UI::Xaml::Shapes::Ellipse>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, Microsoft::UI::Xaml::Shapes::Line>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, Microsoft::UI::Xaml::Shapes::Path>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, Microsoft::UI::Xaml::Shapes::Polygon>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, Microsoft::UI::Xaml::Shapes::Polyline>();
        ValidateDeferralForElementTypesInContainerScenario<ChildAccessor<Border>, Microsoft::UI::Xaml::Shapes::Rectangle>();
    }

    static void ValidateProxyRemoved(FrameworkElement^ root, DependencyObject^ deferred, DependencyObject^ newValue, unsigned index)
    {
        DependencyObject^ deferred2 = safe_cast<DependencyObject^>(root->FindName(GetElementName(ElementType::Deferred, index)));

        if (deferred2 != nullptr)
        {
            if (deferred == nullptr && newValue == nullptr)
            {
                // Not realized, and value set to null.  Set value notification was not received
                VERIFY_FAIL(L"Proxy has not been removed");
            }
            else if (deferred != nullptr && deferred == deferred2)
            {
                // Name of deferred has not been cleared.
                Log::Comment(L"Realized element is still in nametable");
            }
            else
            {
                VERIFY_FAIL(L"Failed for some other reason");
            }
        }
    }

    template <typename TOuterAccessor, typename TInnerElement>
    struct ValidateReplacementOfContentScenario0
    {
        ValidateReplacementOfContentScenario0(bool realize, bool create)
        {
            typedef ElementTraits<Grid> TRootElementTraits;
            typedef EmptyAccessor<TInnerElement> TInnerAccessor;
            typedef PropertyTraits<TOuterAccessor::type, TOuterAccessor> TOuterPropertyTraits;
            typedef PropertyTraits<TInnerAccessor::type, TInnerAccessor> TInnerPropertyTraits;

            TestCleanupWrapper cleanup;

            TRootElementTraits::type^ root = LoadXamlEnterAndWait<TRootElementTraits::type>(
                (const wchar_t*)GenerateXamlForDeferralForElementTypesInContainer(
                    TRootElementTraits::get_class_name(),
                    TOuterPropertyTraits::get_class_name(),
                    TOuterPropertyTraits::get_property_name(),
                    TInnerPropertyTraits::get_class_name()).GetBuffer());

            TOuterPropertyTraits::type^ container = nullptr;
            TInnerPropertyTraits::type^ deferred = nullptr;

            RunOnUIThread([&]()
            {
                container = safe_cast<TOuterPropertyTraits::type^>(root->FindName(L"container"));
                VERIFY_ARE_EQUAL(0U, TOuterPropertyTraits::get_size(container));

                if (realize)
                {
                    deferred = safe_cast<TInnerPropertyTraits::type^>(container->FindName(GetElementName(ElementType::Deferred, 0)));
                    VERIFY_IS_NOT_NULL(deferred);
                    VERIFY_ARE_EQUAL(1U, TOuterPropertyTraits::get_size(container));
                    VERIFY_IS_TRUE(safe_cast<TInnerPropertyTraits::type^>(TOuterPropertyTraits::get_value(container)) == deferred);
                }

                TInnerPropertyTraits::type^ replacementElement = (create) ? ref new TInnerPropertyTraits::type() : nullptr;
                TOuterPropertyTraits::set_value(container, replacementElement);
                VERIFY_ARE_EQUAL((replacementElement != nullptr) ? 1U : 0U, TOuterPropertyTraits::get_size(container));
                VERIFY_IS_TRUE(safe_cast<TInnerPropertyTraits::type^>(TOuterPropertyTraits::get_value(container)) == replacementElement);

                ValidateProxyRemoved(root, deferred, replacementElement, 0);
            });

            TestServices::WindowHelper->WaitForIdle();
        }
    };

    template <typename TInnerElement>
    struct ValidateReplacementOfContentScenario0<ContentAccessor<Flyout>, TInnerElement>
    {
        ValidateReplacementOfContentScenario0(bool realize, bool create)
        {
            typedef ElementTraits<Grid> TRootElementTraits;
            typedef FlyoutAccessor<Button> TOuterAccessor;
            typedef ContentAccessor<Flyout> TFlyoutAccessor;
            typedef EmptyAccessor<TInnerElement> TInnerAccessor;
            typedef PropertyTraits<TOuterAccessor::type, TOuterAccessor> TOuterPropertyTraits;
            typedef PropertyTraits<TFlyoutAccessor::type, TFlyoutAccessor> TFlyoutPropertyTraits;
            typedef PropertyTraits<TInnerAccessor::type, TInnerAccessor> TInnerPropertyTraits;

            TestCleanupWrapper cleanup;

            TRootElementTraits::type^ root = LoadXamlEnterAndWait<TRootElementTraits::type>(
                (const wchar_t*)GenerateXamlForDeferralForElementTypesInFlyout(
                    TFlyoutPropertyTraits::get_class_name(),
                    TInnerPropertyTraits::get_class_name()).GetBuffer());

            TOuterPropertyTraits::type^ container = nullptr;
            TFlyoutPropertyTraits::type^ flyout = nullptr;
            TInnerPropertyTraits::type^ deferred = nullptr;

            auto flyoutOpenedEvent = std::make_shared<Event>();
            auto flyoutClosedEvent = std::make_shared<Event>();
            auto openedRegistration = CreateSafeEventRegistration(TFlyoutPropertyTraits::type, Opened);
            auto closedRegistration = CreateSafeEventRegistration(TFlyoutPropertyTraits::type, Closed);

            RunOnUIThread([&]()
            {
                container = safe_cast<TOuterPropertyTraits::type^>(root->FindName(L"container"));
                VERIFY_IS_NOT_NULL(container);

                flyout = dynamic_cast<TFlyoutPropertyTraits::type^>(container->Flyout);
                VERIFY_IS_NOT_NULL(flyout);

                if (realize)
                {
                    deferred = safe_cast<TInnerPropertyTraits::type^>(container->FindName(GetElementName(ElementType::Deferred, 0)));
                    VERIFY_IS_NOT_NULL(deferred);
                    VERIFY_ARE_EQUAL(1U, TFlyoutPropertyTraits::get_size(flyout));
                    VERIFY_IS_TRUE(safe_cast<TInnerPropertyTraits::type^>(TFlyoutPropertyTraits::get_value(flyout)) == deferred);
                }

                TInnerPropertyTraits::type^ replacementElement = (create) ? ref new TInnerPropertyTraits::type() : nullptr;
                TFlyoutPropertyTraits::set_value(flyout, replacementElement);
                VERIFY_ARE_EQUAL((replacementElement != nullptr) ? 1U : 0U, TFlyoutPropertyTraits::get_size(flyout));
                VERIFY_IS_TRUE(safe_cast<TInnerPropertyTraits::type^>(TFlyoutPropertyTraits::get_value(flyout)) == replacementElement);

                ValidateProxyRemoved(root, deferred, replacementElement, 0);

                openedRegistration.Attach(flyout, ref new wf::EventHandler<Platform::Object^>([flyoutOpenedEvent](Platform::Object^, Platform::Object^)
                {
                    flyoutOpenedEvent->Set();
                }));

                closedRegistration.Attach(flyout, ref new wf::EventHandler<Platform::Object^>([flyoutClosedEvent](Platform::Object^, Platform::Object^)
                {
                    flyoutClosedEvent->Set();
                }));

                container->Flyout->ShowAt(container);
            });

            TestServices::WindowHelper->WaitForIdle();

            flyoutOpenedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                flyout->Hide();
            });

            flyoutClosedEvent->WaitForDefault();
        }
    };

    // General scenario
    template <typename TOuterAccessor, typename TInnerElement = TextBox>
    static void ValidateReplacementOfContentScenario()
    {
        typedef EmptyAccessor<TInnerElement> TInnerAccessor;
        typedef PropertyTraits<TOuterAccessor::type, TOuterAccessor> TOuterPropertyTraits;
        typedef PropertyTraits<TInnerAccessor::type, TInnerAccessor> TInnerPropertyTraits;

        Log::Comment(String().Format(
            L"* Testing realized %s in %s.%s set to a different element",
            TInnerPropertyTraits::get_class_name(),
            TOuterPropertyTraits::get_class_name(),
            TOuterPropertyTraits::get_property_name()));

        ValidateReplacementOfContentScenario0<TOuterAccessor, TInnerElement>(true, true);

        Log::Comment(String().Format(
            L"* Testing realized %s in %s.%s set to NULL",
            TInnerPropertyTraits::get_class_name(),
            TOuterPropertyTraits::get_class_name(),
            TOuterPropertyTraits::get_property_name()));

        ValidateReplacementOfContentScenario0<TOuterAccessor, TInnerElement>(true, false);

        Log::Comment(String().Format(
            L"* Testing not realized %s in %s.%s set to a different element",
            TInnerPropertyTraits::get_class_name(),
            TOuterPropertyTraits::get_class_name(),
            TOuterPropertyTraits::get_property_name()));

        ValidateReplacementOfContentScenario0<TOuterAccessor, TInnerElement>(false, true);

        Log::Comment(String().Format(
            L"* Testing not realized %s in %s.%s set to NULL",
            TInnerPropertyTraits::get_class_name(),
            TOuterPropertyTraits::get_class_name(),
            TOuterPropertyTraits::get_property_name()));

        ValidateReplacementOfContentScenario0<TOuterAccessor, TInnerElement>(false, false);
    }

    void ElementDeferralTests::ValidateReplacementOfContentInButtonBase()
    {
        ValidateReplacementOfContentScenario<ContentAccessor<AppBarButton>>();
        ValidateReplacementOfContentScenario<IconAccessor<AppBarButton>, BitmapIcon>();

        ValidateReplacementOfContentScenario<ContentAccessor<AppBarToggleButton>>();
        ValidateReplacementOfContentScenario<IconAccessor<AppBarToggleButton>, BitmapIcon>();

        ValidateReplacementOfContentScenario<ContentAccessor<Button>>();
        ValidateReplacementOfContentScenario<CommandParameterAccessor<Button>>();

        ValidateReplacementOfContentScenario<ContentAccessor<CheckBox>>();

        ValidateReplacementOfContentScenario<ContentAccessor<HyperlinkButton>>();

        ValidateReplacementOfContentScenario<ContentAccessor<RadioButton>>();

        ValidateReplacementOfContentScenario<ContentAccessor<RepeatButton>>();

        ValidateReplacementOfContentScenario<ContentAccessor<ToggleButton>>();
    }

    void ElementDeferralTests::ValidateReplacementOfContentInContentControl()
    {
        ValidateReplacementOfContentScenario<ContentAccessor<AppBar>>();

        ValidateReplacementOfContentScenario<ContentAccessor<ContentControl>>();

        ValidateReplacementOfContentScenario<ContentAccessor<ContentDialog>>();
        ValidateReplacementOfContentScenario<TitleAccessor<ContentDialog>>();
        ValidateReplacementOfContentScenario<PrimaryButtonCommandParameterAccessor<ContentDialog>>();
        ValidateReplacementOfContentScenario<SecondaryButtonCommandParameterAccessor<ContentDialog>>();

        ValidateReplacementOfContentScenario<ContentAccessor<FlyoutPresenter>>();

        ValidateReplacementOfContentScenario<ContentAccessor<Frame>>();

        ValidateReplacementOfContentScenario<ContentAccessor<GridViewHeaderItem>>();

        ValidateReplacementOfContentScenario<ContentAccessor<GroupItem>>();

        ValidateReplacementOfContentScenario<ContentAccessor<ListViewHeaderItem>>();

        ValidateReplacementOfContentScenario<ContentAccessor<ScrollViewer>>();
        ValidateReplacementOfContentScenario<LeftHeaderAccessor<ScrollViewer>>();
        ValidateReplacementOfContentScenario<TopHeaderAccessor<ScrollViewer>>();
        ValidateReplacementOfContentScenario<TopLeftHeaderAccessor<ScrollViewer>>();

        ValidateReplacementOfContentScenario<ContentAccessor<ToolTip>>();
    }

    void ElementDeferralTests::ValidateReplacementOfContentInControl()
    {
        ValidateReplacementOfContentScenario<HeaderAccessor<DatePicker>>();

        ValidateReplacementOfContentScenario<ContentAccessor<Page>>();
        ValidateReplacementOfContentScenario<BottomAppBarAccessor<Page>, AppBar>();
        ValidateReplacementOfContentScenario<TopAppBarAccessor<Page>, AppBar>();

        ValidateReplacementOfContentScenario<HeaderAccessor<PasswordBox>>();

        ValidateReplacementOfContentScenario<HeaderAccessor<RichEditBox>>();

        ValidateReplacementOfContentScenario<ZoomedInViewAccessor<SemanticZoom>, ListView>();
        ValidateReplacementOfContentScenario<ZoomedOutViewAccessor<SemanticZoom>, GridView>();

        ValidateReplacementOfContentScenario<HeaderAccessor<Slider>>();

        ValidateReplacementOfContentScenario<HeaderAccessor<TextBox>>();

        ValidateReplacementOfContentScenario<HeaderAccessor<TimePicker>>();

        ValidateReplacementOfContentScenario<HeaderAccessor<ToggleSwitch>>();

        ValidateReplacementOfContentScenario<ContentAccessor<UserControl>>();
    }

    void ElementDeferralTests::ValidateReplacementOfContentInFrameworkElement()
    {
        ValidateReplacementOfContentScenario<ChildAccessor<Border>>();

        ValidateReplacementOfContentScenario<ContentAccessor<ContentPresenter>>();

        ValidateReplacementOfContentScenario<ContentAccessor<Flyout>, TextBlock>();

        ValidateReplacementOfContentScenario<FooterAccessor<ItemsPresenter>>();
        // BUG: ValidateReplacementOfContentScenario<HeaderAccessor<ItemsPresenter>>();

        ValidateReplacementOfContentScenario<ChildAccessor<Viewbox>>();

        ValidateReplacementOfContentScenario<ChildAccessor<Popup>>();
    }

    void ElementDeferralTests::ValidateReplacementOfContentInItemsControl()
    {
        ValidateReplacementOfContentScenario<HeaderAccessor<ComboBox>>();

        ValidateReplacementOfContentScenario<FooterAccessor<GridView>>();
        ValidateReplacementOfContentScenario<HeaderAccessor<GridView>>();

        ValidateReplacementOfContentScenario<FooterAccessor<ListView>>();
        ValidateReplacementOfContentScenario<HeaderAccessor<ListView>>();
    }

    void ElementDeferralTests::ValidateReplacementOfContentInSelectorItem()
    {
        ValidateReplacementOfContentScenario<ContentAccessor<ComboBoxItem>>();

        ValidateReplacementOfContentScenario<ContentAccessor<FlipViewItem>>();

        ValidateReplacementOfContentScenario<ContentAccessor<GridViewItem>>();

        ValidateReplacementOfContentScenario<ContentAccessor<ListBoxItem>>();

        ValidateReplacementOfContentScenario<ContentAccessor<ListViewItem>>();
    }

    // General scenario
    template <typename TOuterAccessor, typename TInnerElement>
    struct ValidateContentClearValueScenario0
    {
        ValidateContentClearValueScenario0(bool realize)
        {
            typedef ElementTraits<Grid> TRootElementTraits;
            typedef EmptyAccessor<TInnerElement> TInnerAccessor;
            typedef PropertyTraits<TOuterAccessor::type, TOuterAccessor> TOuterPropertyTraits;
            typedef PropertyTraits<TInnerAccessor::type, TInnerAccessor> TInnerPropertyTraits;

            TestCleanupWrapper cleanup;

            TRootElementTraits::type^ root = LoadXamlEnterAndWait<TRootElementTraits::type>(
                (const wchar_t*)GenerateXamlForDeferralForElementTypesInContainer(
                    TRootElementTraits::get_class_name(),
                    TOuterPropertyTraits::get_class_name(),
                    TOuterPropertyTraits::get_property_name(),
                    TInnerPropertyTraits::get_class_name()).GetBuffer());

            TOuterPropertyTraits::type^ container = nullptr;
            TInnerPropertyTraits::type^ deferred = nullptr;

            RunOnUIThread([&]()
            {
                container = safe_cast<TOuterPropertyTraits::type^>(root->FindName(L"container"));
                VERIFY_IS_NOT_NULL(container);

                VERIFY_ARE_EQUAL(0U, TOuterPropertyTraits::get_size(container));

                if (realize)
                {
                    deferred = safe_cast<TInnerPropertyTraits::type^>(root->FindName(GetElementName(ElementType::Deferred, 0)));
                    VERIFY_IS_NOT_NULL(deferred);
                    VERIFY_IS_TRUE(safe_cast<TInnerPropertyTraits::type^>(TOuterPropertyTraits::get_value(container)) == deferred);
                    VERIFY_ARE_EQUAL(1U, TOuterPropertyTraits::get_size(container));
                }

                TOuterPropertyTraits::clear_value(container);

                VERIFY_ARE_EQUAL(0U, TOuterPropertyTraits::get_size(container));
                VERIFY_IS_TRUE(TOuterPropertyTraits::get_value(container) == nullptr);

                ValidateProxyRemoved(root, deferred, nullptr, 0);
            });

            TestServices::WindowHelper->WaitForIdle();
        }
    };

    template <typename TInnerElement>
    struct ValidateContentClearValueScenario0<ContentAccessor<Flyout>, TInnerElement>
    {
        ValidateContentClearValueScenario0(bool realize)
        {
            typedef ElementTraits<Grid> TRootElementTraits;
            typedef ContentAccessor<Flyout> TFlyoutAccessor;
            typedef FlyoutAccessor<Button> TOuterAccessor;
            typedef EmptyAccessor<TInnerElement> TInnerAccessor;
            typedef PropertyTraits<TOuterAccessor::type, TOuterAccessor> TOuterPropertyTraits;
            typedef PropertyTraits<TFlyoutAccessor::type, TFlyoutAccessor> TFlyoutPropertyTraits;
            typedef PropertyTraits<TInnerAccessor::type, TInnerAccessor> TInnerPropertyTraits;

            TestCleanupWrapper cleanup;

            TRootElementTraits::type^ root = LoadXamlEnterAndWait<TRootElementTraits::type>(
                (const wchar_t*)GenerateXamlForDeferralForElementTypesInFlyout(
                    TFlyoutPropertyTraits::get_class_name(),
                    TInnerPropertyTraits::get_class_name()).GetBuffer());

            TOuterPropertyTraits::type^ container = nullptr;
            TInnerPropertyTraits::type^ deferred = nullptr;
            TFlyoutPropertyTraits::type^ flyout = nullptr;

            auto flyoutOpenedEvent = std::make_shared<Event>();
            auto flyoutClosedEvent = std::make_shared<Event>();
            auto openedRegistration = CreateSafeEventRegistration(TFlyoutPropertyTraits::type, Opened);
            auto closedRegistration = CreateSafeEventRegistration(TFlyoutPropertyTraits::type, Closed);

            RunOnUIThread([&]()
            {
                container = safe_cast<TOuterPropertyTraits::type^>(root->FindName(L"container"));
                VERIFY_IS_NOT_NULL(container);

                flyout = safe_cast<TFlyoutPropertyTraits::type^>(container->Flyout);
                VERIFY_IS_NOT_NULL(flyout);

                VERIFY_ARE_EQUAL(0U, TFlyoutPropertyTraits::get_size(flyout));

                if (realize)
                {
                    deferred = safe_cast<TInnerPropertyTraits::type^>(root->FindName(GetElementName(ElementType::Deferred, 0)));
                    VERIFY_IS_NOT_NULL(deferred);
                    VERIFY_IS_TRUE(safe_cast<TInnerPropertyTraits::type^>(TFlyoutPropertyTraits::get_value(flyout)) == deferred);
                    VERIFY_ARE_EQUAL(1U, TFlyoutPropertyTraits::get_size(flyout));
                }

                TFlyoutPropertyTraits::clear_value(flyout);

                VERIFY_ARE_EQUAL(0U, TFlyoutPropertyTraits::get_size(flyout));
                VERIFY_IS_TRUE(TFlyoutPropertyTraits::get_value(flyout) == nullptr);

                ValidateProxyRemoved(root, deferred, nullptr, 0);
            });

            TestServices::WindowHelper->WaitForIdle();
        }
    };

    template <typename TOuterAccessor, typename TInnerElement = Button>
    struct ValidateContentClearValueScenario
    {
        ValidateContentClearValueScenario()
        {
            typedef EmptyAccessor<TInnerElement> TInnerAccessor;
            typedef PropertyTraits<TOuterAccessor::type, TOuterAccessor> TOuterPropertyTraits;
            typedef PropertyTraits<TInnerAccessor::type, TInnerAccessor> TInnerPropertyTraits;

            Log::Comment(String().Format(
                L"* Testing realized %s in %s.%s is cleared",
                TInnerPropertyTraits::get_class_name(),
                TOuterPropertyTraits::get_class_name(),
                TOuterPropertyTraits::get_property_name()));

            ValidateContentClearValueScenario0<TOuterAccessor, TInnerElement>(true);

            Log::Comment(String().Format(
                L"* Testing not realized %s in %s.%s is cleared",
                TInnerPropertyTraits::get_class_name(),
                TOuterPropertyTraits::get_class_name(),
                TOuterPropertyTraits::get_property_name()));

            ValidateContentClearValueScenario0<TOuterAccessor, TInnerElement>(false);
        }
    };

    void ElementDeferralTests::ValidateContentClearValueInButtonBase()
    {
        ValidateContentClearValueScenario<ContentAccessor<AppBarButton>>();
        ValidateContentClearValueScenario<IconAccessor<AppBarButton>, BitmapIcon>();

        ValidateContentClearValueScenario<ContentAccessor<AppBarToggleButton>>();
        ValidateContentClearValueScenario<IconAccessor<AppBarToggleButton>, BitmapIcon>();

        ValidateContentClearValueScenario<ContentAccessor<Button>>();
        ValidateContentClearValueScenario<CommandParameterAccessor<Button>>();
        ValidateContentClearValueScenario<FlyoutAccessor<Button>, Flyout>();
        ValidateContentClearValueScenario<FlyoutAccessor<Button>, MenuFlyout>();

        ValidateContentClearValueScenario<ContentAccessor<CheckBox>>();

        ValidateContentClearValueScenario<ContentAccessor<HyperlinkButton>>();

        ValidateContentClearValueScenario<ContentAccessor<RadioButton>>();

        ValidateContentClearValueScenario<ContentAccessor<RepeatButton>>();

        ValidateContentClearValueScenario<ContentAccessor<ToggleButton>>();
    }

    void ElementDeferralTests::ValidateContentClearValueInContentControl()
    {
        ValidateContentClearValueScenario<ContentAccessor<AppBar>>();

        ValidateContentClearValueScenario<ContentAccessor<ContentControl>>();

        ValidateContentClearValueScenario<ContentAccessor<ContentDialog>>();
        ValidateContentClearValueScenario<TitleAccessor<ContentDialog>>();
        ValidateContentClearValueScenario<PrimaryButtonCommandParameterAccessor<ContentDialog>>();
        ValidateContentClearValueScenario<SecondaryButtonCommandParameterAccessor<ContentDialog>>();

        ValidateContentClearValueScenario<ContentAccessor<FlyoutPresenter>>();

        ValidateContentClearValueScenario<ContentAccessor<Frame>>();

        ValidateContentClearValueScenario<ContentAccessor<GridViewHeaderItem>>();

        ValidateContentClearValueScenario<ContentAccessor<GroupItem>>();

        ValidateContentClearValueScenario<ContentAccessor<ListViewHeaderItem>>();

        ValidateContentClearValueScenario<ContentAccessor<ScrollViewer>>();
        ValidateContentClearValueScenario<LeftHeaderAccessor<ScrollViewer>>();
        ValidateContentClearValueScenario<TopHeaderAccessor<ScrollViewer>>();
        ValidateContentClearValueScenario<TopLeftHeaderAccessor<ScrollViewer>>();

        ValidateContentClearValueScenario<ContentAccessor<ToolTip>>();
    }

    void ElementDeferralTests::ValidateContentClearValueInControl()
    {
        ValidateContentClearValueScenario<HeaderAccessor<DatePicker>>();

        ValidateContentClearValueScenario<ContentAccessor<Page>>();
        ValidateContentClearValueScenario<BottomAppBarAccessor<Page>, AppBar>();
        ValidateContentClearValueScenario<TopAppBarAccessor<Page>, AppBar>();

        ValidateContentClearValueScenario<HeaderAccessor<PasswordBox>>();

        ValidateContentClearValueScenario<HeaderAccessor<RichEditBox>>();

        // Disabled until fixed: ValidateContentClearValueScenario<ZoomedInViewAccessor<SemanticZoom>, ListView>();
        // Disabled until fixed: ValidateContentClearValueScenario<ZoomedOutViewAccessor<SemanticZoom>, GridView>();

        ValidateContentClearValueScenario<HeaderAccessor<Slider>>();

        ValidateContentClearValueScenario<HeaderAccessor<TextBox>>();

        ValidateContentClearValueScenario<HeaderAccessor<TimePicker>>();

        ValidateContentClearValueScenario<HeaderAccessor<ToggleSwitch>>();

        ValidateContentClearValueScenario<ContentAccessor<UserControl>>();
    }

    void ElementDeferralTests::ValidateContentClearValueInFrameworkElement()
    {
        ValidateContentClearValueScenario<ChildAccessor<Border>>();

        ValidateContentClearValueScenario<ContentAccessor<ContentPresenter>>();

        ValidateContentClearValueScenario<ContentAccessor<Flyout>, TextBlock>();

        ValidateContentClearValueScenario<FooterAccessor<ItemsPresenter>>();
        ValidateContentClearValueScenario<HeaderAccessor<ItemsPresenter>>();

        ValidateContentClearValueScenario<ChildAccessor<Popup>>();

        ValidateContentClearValueScenario<ChildAccessor<Viewbox>>();
    }

    void ElementDeferralTests::ValidateContentClearValueInItemsControl()
    {
        ValidateContentClearValueScenario<HeaderAccessor<ComboBox>>();

        ValidateContentClearValueScenario<FooterAccessor<GridView>>();
        ValidateContentClearValueScenario<HeaderAccessor<GridView>>();

        ValidateContentClearValueScenario<FooterAccessor<ListView>>();
        ValidateContentClearValueScenario<HeaderAccessor<ListView>>();
    }

    void ElementDeferralTests::ValidateContentClearValueInSelectorItem()
    {
        ValidateContentClearValueScenario<ContentAccessor<ComboBoxItem>>();

        ValidateContentClearValueScenario<ContentAccessor<FlipViewItem>>();

        ValidateContentClearValueScenario<ContentAccessor<GridViewItem>>();

        ValidateContentClearValueScenario<ContentAccessor<ListBoxItem>>();

        ValidateContentClearValueScenario<ContentAccessor<ListViewItem>>();
    }

    template <typename TOuterAccessor, typename TInnerElement = Button>
    struct ValidateContentClearCollectionScenario0
    {
        ValidateContentClearCollectionScenario0(bool realize)
        {
            typedef ElementTraits<Grid> TRootElementTraits;
            typedef EmptyAccessor<TInnerElement> TInnerAccessor;
            typedef PropertyTraits<TOuterAccessor::type, TOuterAccessor> TOuterPropertyTraits;
            typedef PropertyTraits<TInnerAccessor::type, TInnerAccessor> TInnerPropertyTraits;

            TestCleanupWrapper cleanup;

            TRootElementTraits::type^ root = LoadXamlEnterAndWait<TRootElementTraits::type>(
                (const wchar_t*)GenerateXamlForClearCollectionScenario(
                    TRootElementTraits::get_class_name(),
                    TOuterPropertyTraits::get_class_name(),
                    TOuterPropertyTraits::get_property_name(),
                    TInnerPropertyTraits::get_class_name()).GetBuffer());

            TOuterPropertyTraits::type^ container = nullptr;
            array<TInnerPropertyTraits::type^, 3> deferred = {};

            RunOnUIThread([&]()
            {
                container = safe_cast<TOuterPropertyTraits::type^>(root->FindName(L"container"));
                VERIFY_IS_NOT_NULL(container);

                VERIFY_ARE_EQUAL(0U, TOuterPropertyTraits::get_size(container));

                if (realize)
                {
                    for (int i = 0; i < 3; ++i)
                    {
                        deferred[i] = safe_cast<TInnerPropertyTraits::type^>(root->FindName(GetElementName(ElementType::Deferred, i)));
                        VERIFY_IS_NOT_NULL(deferred[i]);
                    }

                    VERIFY_ARE_EQUAL(3U, TOuterPropertyTraits::get_size(container));
                }

                TOuterPropertyTraits::clear_value(container);

                VERIFY_ARE_EQUAL(0U, TOuterPropertyTraits::get_size(container));

                for (int i = 0; i < 3; ++i)
                {
                    ValidateProxyRemoved(root, deferred[i], nullptr, i);
                }

                ClearArray(deferred);
            });

            TestServices::WindowHelper->WaitForIdle();
        }
    };

    template <typename TInnerElement>
    struct ValidateContentClearCollectionScenario0<MenuItemsAccessor<MenuFlyout>, TInnerElement>
    {
        ValidateContentClearCollectionScenario0(bool realize)
        {
            typedef ElementTraits<Grid> TRootElementTraits;
            typedef MenuItemsAccessor<MenuFlyout> TFlyoutAccessor;
            typedef FlyoutAccessor<Button> TOuterAccessor;
            typedef EmptyAccessor<TInnerElement> TInnerAccessor;
            typedef PropertyTraits<TOuterAccessor::type, TOuterAccessor> TOuterPropertyTraits;
            typedef PropertyTraits<TFlyoutAccessor::type, TFlyoutAccessor> TFlyoutPropertyTraits;
            typedef PropertyTraits<TInnerAccessor::type, TInnerAccessor> TInnerPropertyTraits;

            TestCleanupWrapper cleanup;

            TRootElementTraits::type^ root = LoadXamlEnterAndWait<TRootElementTraits::type>(
                (const wchar_t*)GenerateXamlForClearCollectionScenarioInMenuFlyout(
                    TInnerPropertyTraits::get_class_name()).GetBuffer());

            TOuterPropertyTraits::type^ container = nullptr;
            TFlyoutPropertyTraits::type^ flyout = nullptr;
            array<TInnerPropertyTraits::type^, 3> deferred = {};

            auto flyoutOpenedEvent = std::make_shared<Event>();
            auto flyoutClosedEvent = std::make_shared<Event>();
            auto openedRegistration = CreateSafeEventRegistration(TFlyoutPropertyTraits::type, Opened);
            auto closedRegistration = CreateSafeEventRegistration(TFlyoutPropertyTraits::type, Closed);

            RunOnUIThread([&]()
            {
                container = safe_cast<TOuterPropertyTraits::type^>(root->FindName(L"container"));
                VERIFY_IS_NOT_NULL(container);

                flyout = safe_cast<TFlyoutPropertyTraits::type^>(container->Flyout);
                VERIFY_IS_NOT_NULL(flyout);

                VERIFY_ARE_EQUAL(0U, TFlyoutPropertyTraits::get_size(flyout));

                if (realize)
                {
                    for (int i = 0; i < 3; ++i)
                    {
                        deferred[i] = safe_cast<TInnerPropertyTraits::type^>(root->FindName(GetElementName(ElementType::Deferred, i)));
                        VERIFY_IS_NOT_NULL(deferred[i]);
                    }

                    VERIFY_ARE_EQUAL(3U, TFlyoutPropertyTraits::get_size(flyout));
                }

                TFlyoutPropertyTraits::clear_value(flyout);

                VERIFY_ARE_EQUAL(0U, TFlyoutPropertyTraits::get_size(flyout));

                for (int i = 0; i < 3; ++i)
                {
                    ValidateProxyRemoved(root, deferred[i], nullptr, i);
                }

                ClearArray(deferred);
            });

            TestServices::WindowHelper->WaitForIdle();
        }
    };

    template <typename TOuterAccessor, typename TInnerElement = Button>
    static void ValidateContentClearCollectionScenario()
    {
        typedef EmptyAccessor<TInnerElement> TInnerAccessor;
        typedef PropertyTraits<TOuterAccessor::type, TOuterAccessor> TOuterPropertyTraits;
        typedef PropertyTraits<TInnerAccessor::type, TInnerAccessor> TInnerPropertyTraits;

        Log::Comment(String().Format(
            L"* Testing realized %s in %s.%s is cleared",
            TInnerPropertyTraits::get_class_name(),
            TOuterPropertyTraits::get_class_name(),
            TOuterPropertyTraits::get_property_name()));

        ValidateContentClearCollectionScenario0<TOuterAccessor, TInnerElement>(true);

        Log::Comment(String().Format(
            L"* Testing not realized %s in %s.%s is cleared",
            TInnerPropertyTraits::get_class_name(),
            TOuterPropertyTraits::get_class_name(),
            TOuterPropertyTraits::get_property_name()));

        ValidateContentClearCollectionScenario0<TOuterAccessor, TInnerElement>(false);
    }

    void ElementDeferralTests::ValidateContentClearCollection()
    {
        // Microsoft::UI::Xaml::Controls

        ValidateContentClearCollectionScenario<ChildrenAccessor<Canvas>>();
        ValidateContentClearCollectionScenario<ItemsAccessor<ComboBox>>();
        // NEED TEST: ValidateContentClearCollectionScenario<PrimaryCommandsAccessor<CommandBar>, >();
        // NEED TEST: ValidateContentClearCollectionScenario<SecondaryCommandsAccessor<CommandBar>, >();
        ValidateContentClearCollectionScenario<ItemsAccessor<FlipView>>();
        ValidateContentClearCollectionScenario<ChildrenAccessor<Grid>>();
        ValidateContentClearCollectionScenario<ItemsAccessor<GridView>>();
        ValidateContentClearCollectionScenario<ItemsAccessor<ItemsControl>>();
        ValidateContentClearCollectionScenario<ItemsAccessor<ListBox>>();
        ValidateContentClearCollectionScenario<ItemsAccessor<ListView>>();
        ValidateContentClearCollectionScenario<MenuItemsAccessor<MenuFlyout>, MenuFlyoutItem>();
        ValidateContentClearCollectionScenario<ItemsAccessor<MenuFlyoutPresenter>, MenuFlyoutItem>();
        // NEED TEST: ValidateContentClearCollectionScenario<MenuItemsAccessor<MenuFlyoutSubItem>>();
        ValidateContentClearCollectionScenario<ChildrenAccessor<RelativePanel>>();
        ValidateContentClearCollectionScenario<ChildrenAccessor<StackPanel>>();
        // NEED TEST: ValidateContentClearCollectionScenario<ChildrenAccessor<SwapChainBackgroundPanel>>();
        ValidateContentClearCollectionScenario<ChildrenAccessor<SwapChainPanel>>();
        ValidateContentClearCollectionScenario<ChildrenAccessor<VariableSizedWrapGrid>>();
        ValidateContentClearCollectionScenario<ChildrenAccessor<VirtualizingStackPanel>>();
        // BUG: ValidateContentClearCollectionScenario<ChildrenAccessor<WrapGrid>>();

        // Microsoft::UI::Xaml::Controls::Primitives

        // BUG: ValidateContentClearCollectionScenario<ChildrenAccessor<CalendarPanel>>();
        ValidateContentClearCollectionScenario<ChildrenAccessor<CarouselPanel>>();
    }

    #pragma endregion

    #pragma region Deferral in nested elements

    static void ValidateNestedDeferralScenario(int loadTime)
    {
        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"Load time: %d", loadTime));

        Grid^ rootPanel = nullptr;
        Grid^ innerGrid = nullptr;
        Button^ button = nullptr;

        WeakReference innerGridWeakRef;
        WeakReference buttonWeakRef;

        RunOnUIThread([&]()
        {
            rootPanel = LoadXaml<Grid>(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Grid x:Name='innerGrid' x:DeferLoadStrategy='Lazy'>"
                L"    <Button x:Name='button' x:DeferLoadStrategy='Lazy'/>"
                L"  </Grid>"
                L"</Grid>");
            VERIFY_ARE_EQUAL(0U, rootPanel->Children->Size);
        });

        if (loadTime == 0)
        {
            EnterAndWait(rootPanel);
        }

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0U, rootPanel->Children->Size);
            button = safe_cast<Button^>(rootPanel->FindName(L"button"));
            VERIFY_IS_NULL(button);
            VERIFY_ARE_EQUAL(0U, rootPanel->Children->Size);
        });

        if (loadTime == 1)
        {
            EnterAndWait(rootPanel);
        }

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0U, rootPanel->Children->Size);
            innerGrid = safe_cast<Grid^>(rootPanel->FindName(L"innerGrid"));
            VERIFY_IS_NOT_NULL(innerGrid);
            innerGridWeakRef = WeakReference(innerGrid);
            VERIFY_ARE_EQUAL(1U, rootPanel->Children->Size);
            VERIFY_ARE_EQUAL(0U, innerGrid->Children->Size);
        });

        if (loadTime == 2)
        {
            EnterAndWait(rootPanel);
        }

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1U, rootPanel->Children->Size);
            VERIFY_ARE_EQUAL(0U, innerGrid->Children->Size);
            button = safe_cast<Button^>(innerGrid->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);
            buttonWeakRef = WeakReference(button);
            VERIFY_ARE_EQUAL(1U, innerGrid->Children->Size);
        });

        if (loadTime == 3)
        {
            EnterAndWait(rootPanel);
        }

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1U, rootPanel->Children->Size);
            VERIFY_ARE_EQUAL(1U, innerGrid->Children->Size);
        });

        if (loadTime != -1)
        {
            TestServices::WindowHelper->WaitForIdle();
        }

        RunOnUIThread([&]()
        {
            Markup::XamlMarkupHelper::UnloadObject(button);
            button = nullptr;
        });

        if (loadTime != -1)
        {
            TestServices::WindowHelper->WaitForIdle();
        }

        RunOnUIThread([&]()
        {
            VERIFY_IS_NULL(buttonWeakRef.Resolve<Button>());

            Markup::XamlMarkupHelper::UnloadObject(innerGrid);
            innerGrid = nullptr;
        });

        if (loadTime != -1)
        {
            TestServices::WindowHelper->WaitForIdle();
        }

        RunOnUIThread([&]()
        {
            VERIFY_IS_NULL(innerGridWeakRef.Resolve<Grid>());
        });
    }

    void ElementDeferralTests::ValidateNestedDeferral()
    {
        ValidateNestedDeferralScenario(-1);
        ValidateNestedDeferralScenario(0);
        ValidateNestedDeferralScenario(1);
        ValidateNestedDeferralScenario(2);
        ValidateNestedDeferralScenario(3);
    }

    static void ValidateNestedDeferralScenario(
        const ElementFinder& finder,
        Page^ page,
        Border^ level0Border,
        bool validateEvents,
        bool validateDestruction)
    {
        using namespace ::Tests::Native::External::Framework::XDefer;

        Grid^ level1Grid = nullptr;
        Button^ level2Button = nullptr;

        RunOnUIThread([&]()
        {
            // Nested deferred elements cannot be realized yet...
            VERIFY_IS_NULL(finder.FindElement(L"level2Button"));

            // Realize nested on level 1
            level1Grid = safe_cast<Grid^>(finder.FindElement(L"level1Grid"));
            VERIFY_IS_NOT_NULL(level1Grid);
            VERIFY_ARE_EQUAL(level1Grid, safe_cast<Grid^>(level0Border->Child));
        });

        TestServices::WindowHelper->WaitForIdle();

        if (validateEvents)
        {
            // Nested should fire loaded event
            s_spLoadedEvent0->WaitForDefault();
        }

        RunOnUIThread([&]()
        {
            // Nested should be able to use binding and resolve resources form deferred parents and page
            VERIFY_ARE_EQUAL(0.5, level1Grid->Opacity);
            VERIFY_ARE_EQUAL(Colors::Red, safe_cast<SolidColorBrush^>(level1Grid->Background)->Color);
            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(page->DataContext), safe_cast<Platform::String^>(level1Grid->Tag));

            VERIFY_IS_NOT_NULL(finder.FindElement(L"Nondeferred"));

            // Realize level 2 nested
            level2Button = safe_cast<Button^>(finder.FindElement(L"level2Button"));
            VERIFY_IS_NOT_NULL(level2Button);
            VERIFY_ARE_EQUAL(level2Button, safe_cast<Button^>(level1Grid->Children->GetAt(0)));

            VERIFY_IS_NOT_NULL(finder.FindElement(L"Nondeferred"));
        });

        TestServices::WindowHelper->WaitForIdle();

        if (validateEvents)
        {
            s_spLoadedEvent1->WaitForDefault();
        }

        RunOnUIThread([&]()
        {
            // Nested should be able to use binding and resolve resources from deferred parents and page
            VERIFY_ARE_EQUAL(Colors::Blue, safe_cast<SolidColorBrush^>(level2Button->Background)->Color);
            VERIFY_ARE_EQUAL(Colors::Green, safe_cast<SolidColorBrush^>(level2Button->BorderBrush)->Color);
            VERIFY_ARE_EQUAL(Colors::Red, safe_cast<SolidColorBrush^>(level2Button->Foreground)->Color);
            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(page->DataContext), safe_cast<Platform::String^>(level2Button->Content));
        });

        if (validateDestruction)
        {
            RunOnUIThread([&]()
            {
                // Remove level 2 nested element
                level2Button = nullptr;
                level1Grid->Children->Clear();
            });

            TestServices::WindowHelper->WaitForIdle();

            if (validateEvents)
            {
                s_spUnloadedEvent1->WaitForDefault();
            }

            RunOnUIThread([&]()
            {
                // Level 2 should be inaccessible
                VERIFY_IS_NULL(finder.FindElement(L"level2Button"));
                level1Grid->Children->Clear();
                level0Border->Child = nullptr;
            });

            TestServices::WindowHelper->WaitForIdle();

            if (validateEvents)
            {
                s_spUnloadedEvent0->WaitForDefault();
            }

            RunOnUIThread([&]()
            {
                VERIFY_IS_NULL(finder.FindElement(L"level1Grid"));
                VERIFY_IS_NULL(finder.FindElement(L"level2Button"));
            });
        }
    }

    void ElementDeferralTests::ValidateNestedDeferralForNonTemplate()
    {
        using namespace ::Tests::Native::External::Framework::XDefer;

        TestCleanupWrapper cleanup;

        CustomPageNested^ page = nullptr;
        Border^ level0Border = nullptr;

        RunOnUIThread([&]()
        {
            page = ref new CustomPageNested();
            page->DataContext = L"String content";
        });

        EnterAndWait(page);

        RunOnUIThread([&]()
        {
            // Nested deferred elements cannot be realized yet...
            VERIFY_IS_NULL(page->FindName(L"level1Grid"));
            VERIFY_IS_NULL(page->FindName(L"level2Button"));

            // Realize parent
            level0Border = safe_cast<Border^>(page->FindName(L"level0Border"));
            VERIFY_IS_NOT_NULL(level0Border);
            VERIFY_IS_NULL(level0Border->Child);
        });

        ValidateNestedDeferralScenario(
            RegularElementFinder(page),
            page,
            level0Border,
            true,
            true);
    }

    void ElementDeferralTests::ValidateNestedDeferralForDataTemplate()
    {
        using namespace ::Tests::Native::External::Framework::XDefer;

        TestCleanupWrapper cleanup;

        CustomPageNested^ page = nullptr;
        ContentControl^ contentControl = nullptr;
        Border^ level0Border = nullptr;

        RunOnUIThread([&]()
        {
            page = ref new CustomPageNested();
            page->DataContext = L"String content";
        });

        EnterAndWait(page);

        RunOnUIThread([&]()
        {
            // Realize parent
            contentControl = safe_cast<ContentControl^>(page->FindName(L"contentControl"));
            VERIFY_IS_NOT_NULL(contentControl);
            VERIFY_IS_NOT_NULL(contentControl->Content);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            FrameworkElement^ templateRoot = safe_cast<FrameworkElement^>(contentControl->ContentTemplateRoot);
            level0Border = safe_cast<Border^>(templateRoot->FindName(L"level0Border"));
            VERIFY_IS_NULL(level0Border->Child);
        });

        ValidateNestedDeferralScenario(
            RegularElementFinder(level0Border),
            page,
            level0Border,
            false,
            false);
    }

    void ElementDeferralTests::ValidateNestedDeferralForControlTemplate()
    {
        using namespace ::Tests::Native::External::Framework::XDefer;

        TestCleanupWrapper cleanup;

        CustomPageNested^ page = nullptr;
        CustomControl^ customControl = nullptr;
        Border^ level0Border = nullptr;

        RunOnUIThread([&]()
        {
            page = ref new CustomPageNested();
            page->DataContext = L"String content";
        });

        EnterAndWait(page);

        RunOnUIThread([&]()
        {
            // Realize parent
            customControl = safe_cast<CustomControl^>(page->FindName(L"customControl"));
            VERIFY_IS_NOT_NULL(customControl);
            VERIFY_IS_NOT_NULL(customControl->Content);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            level0Border = safe_cast<Border^>(customControl->PublicGetTemplateChild(L"level0Border"));
            VERIFY_IS_NULL(level0Border->Child);
        });

        ValidateNestedDeferralScenario(
            TemplateElementFinder(customControl),
            page,
            level0Border,
            false,
            false);
    }

    static void ValidateNestedDeferredElementGetsDataContextScenario(const wchar_t* deferAttribute)
    {
        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"Defer Attribute: %s", deferAttribute));

        Grid^ rootPanel = LoadXamlEnterAndWait<Grid>(
            String().Format(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"      Tag='rootPanel'>"
                L"  <StackPanel x:Name='innerPanel' %s Tag='innerPanel'>"
                L"    <Button x:Name='b1' %s Content='{Binding Tag}'/>"
                L"    <Button x:Name='b2' %s Content='{Binding Tag}'/>"
                L"  </StackPanel>"
                L"</Grid>",
                deferAttribute,
                deferAttribute,
                deferAttribute));

        StackPanel^ innerPanel = nullptr;
        Button^ b1 = nullptr;
        Button^ b2 = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel->DataContext = rootPanel;

            innerPanel = safe_cast<StackPanel^>(rootPanel->FindName(L"innerPanel"));
            VERIFY_IS_NOT_NULL(innerPanel);

            b1 = safe_cast<Button^>(rootPanel->FindName(L"b1"));
            VERIFY_IS_NOT_NULL(b1);

            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(rootPanel->Tag), safe_cast<Platform::String^>(b1->Content));
            VERIFY_ARE_EQUAL(safe_cast<Grid^>(rootPanel->DataContext), safe_cast<Grid^>(b1->DataContext));

            innerPanel->DataContext = innerPanel;

            b2 = safe_cast<Button^>(rootPanel->FindName(L"b2"));
            VERIFY_IS_NOT_NULL(b2);

            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(innerPanel->Tag), safe_cast<Platform::String^>(b1->Content));
            VERIFY_ARE_EQUAL(safe_cast<StackPanel^>(innerPanel->DataContext), safe_cast<StackPanel^>(b1->DataContext));

            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(innerPanel->Tag), safe_cast<Platform::String^>(b2->Content));
            VERIFY_ARE_EQUAL(safe_cast<StackPanel^>(innerPanel->DataContext), safe_cast<StackPanel^>(b2->DataContext));
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void ElementDeferralTests::ValidateNestedDeferredElementGetsDataContext()
    {
        // Leak: TemplateContent peer not being unpegged
        TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

        ValidateNestedDeferredElementGetsDataContextScenario(s_deferLoadStrategyLazy);
        ValidateNestedDeferredElementGetsDataContextScenario(s_realizeTrue);
        ValidateNestedDeferredElementGetsDataContextScenario(s_realizeFalse);
    }

    #pragma endregion

    #pragma region Deferred elements and styles/resources

    static void ValidateDeferredElementCanUseResourcesOfParentScenario(int loadTime, const wchar_t* deferAttribute)
    {
        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"Load time: %d", loadTime));
        Log::Comment(String().Format(L"Defer Attribute: %s", deferAttribute));

        Grid^ rootPanel = LoadXaml<Grid>(
            String().Format(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Grid.Resources>"
                L"    <SolidColorBrush x:Key='RedBrush' Color='Red'/>"
                L"    <SolidColorBrush x:Key='OtherBrush' Color='Black'/>"
                L"  </Grid.Resources>"
                L"  <Button x:Name='button' %s Background='{StaticResource RedBrush}'/>"
                L"</Grid>",
                deferAttribute));

        if (loadTime == 0)
        {
            EnterAndWait(rootPanel);
        }

        RunOnUIThread([&]()
        {
            Button^ button = safe_cast<Button^>(rootPanel->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);
            VERIFY_IS_NOT_NULL(button->Background);
            VERIFY_ARE_EQUAL(Colors::Red, safe_cast<SolidColorBrush^>(button->Background)->Color);
        });

        if (loadTime != -1)
        {
            TestServices::WindowHelper->WaitForIdle();
        }
    }

    void ElementDeferralTests::ValidateDeferredElementCanUseResourcesOfParent()
    {
        ValidateDeferredElementCanUseResourcesOfParentScenario(-1, s_deferLoadStrategyLazy);
        ValidateDeferredElementCanUseResourcesOfParentScenario(0, s_deferLoadStrategyLazy);

        ValidateDeferredElementCanUseResourcesOfParentScenario(-1, s_realizeTrue);
        ValidateDeferredElementCanUseResourcesOfParentScenario(0, s_realizeTrue);

        ValidateDeferredElementCanUseResourcesOfParentScenario(-1, s_realizeFalse);
        ValidateDeferredElementCanUseResourcesOfParentScenario(0, s_realizeFalse);
    }

    static void ValidateChildrenOfDeferredElementCanUseResourcesOfParentScenario(const wchar_t* deferAttribute)
    {
        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"Defer Attribute: %s", deferAttribute));

        Grid^ rootPanel = LoadXaml<Grid>(
            String().Format(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='button1' %s>"
                L"    <Button.Resources>"
                L"      <SolidColorBrush x:Key='RedBrush' Color='Red'/>"
                L"      <SolidColorBrush x:Key='OtherBrush1' Color='Black'/>"
                L"    </Button.Resources>"
                L"    <Border x:Name='border1' Width='20' Height='20' Background='{StaticResource RedBrush}'/>"
                L"  </Button>"
                L"  <Button x:Name='button2' %s>"
                L"    <Button.Resources>"
                L"      <SolidColorBrush x:Key='GreenBrush' Color='Green'/>"
                L"      <SolidColorBrush x:Key='OtherBrush2' Color='Black'/>"
                L"    </Button.Resources>"
                L"    <Border x:Name='border2'>"
                L"      <Border.Resources>"
                L"        <SolidColorBrush x:Key='BlueBrush' Color='Blue'/>"
                L"        <SolidColorBrush x:Key='OtherBrush3' Color='Black'/>"
                L"      </Border.Resources>"
                L"      <Rectangle x:Name='rectangle2' Stroke='{StaticResource BlueBrush}' Fill='{StaticResource GreenBrush}'/>"
                L"    </Border>"
                L"  </Button>"
                L"</Grid>",
                deferAttribute,
                deferAttribute));

        EnterAndWait(rootPanel);

        RunOnUIThread([&]()
        {
            Button^ button1 = safe_cast<Button^>(rootPanel->FindName(L"button1"));
            VERIFY_IS_NOT_NULL(button1);

            Border^ border1 = safe_cast<Border^>(rootPanel->FindName(L"border1"));
            VERIFY_IS_NOT_NULL(border1);
            VERIFY_IS_NOT_NULL(border1->Background);
            VERIFY_ARE_EQUAL(Colors::Red, safe_cast<SolidColorBrush^>(border1->Background)->Color);

            Button^ button2 = safe_cast<Button^>(rootPanel->FindName(L"button2"));
            VERIFY_IS_NOT_NULL(button2);

            Border^ border2 = safe_cast<Border^>(rootPanel->FindName(L"border2"));
            VERIFY_IS_NOT_NULL(border2);

            Microsoft::UI::Xaml::Shapes::Rectangle^ rectangle2 = safe_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(rootPanel->FindName(L"rectangle2"));
            VERIFY_IS_NOT_NULL(rectangle2);
            VERIFY_IS_NOT_NULL(rectangle2->Stroke);
            VERIFY_ARE_EQUAL(Colors::Blue, safe_cast<SolidColorBrush^>(rectangle2->Stroke)->Color);
            VERIFY_IS_NOT_NULL(rectangle2->Fill);
            VERIFY_ARE_EQUAL(Colors::Green, safe_cast<SolidColorBrush^>(rectangle2->Fill)->Color);
        });
    }

    void ElementDeferralTests::ValidateChildrenOfDeferredElementCanUseResourcesOfParent()
    {
        ValidateChildrenOfDeferredElementCanUseResourcesOfParentScenario(s_deferLoadStrategyLazy);
        ValidateChildrenOfDeferredElementCanUseResourcesOfParentScenario(s_realizeTrue);
        ValidateChildrenOfDeferredElementCanUseResourcesOfParentScenario(s_realizeFalse);
    }

    static void ValidateDeferredElementInDataTemplateCanUseLocalResourcesScenario(Panel^ rootPanel)
    {
        // deferred element uses parent's resource
        Button^ element1 = safe_cast<Button^>(rootPanel->FindName(L"element1"));
        VERIFY_IS_NOT_NULL(element1);
        VERIFY_IS_NOT_NULL(element1->Background);
        VERIFY_ARE_EQUAL(safe_cast<SolidColorBrush^>(element1->Background)->Color, Colors::Red);

        // element uses resource of deferred parent
        Button^ element2 = safe_cast<Button^>(rootPanel->FindName(L"element2"));
        VERIFY_IS_NOT_NULL(element2);
        Microsoft::UI::Xaml::Shapes::Rectangle^ rectangle1 = safe_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(rootPanel->FindName(L"rectangle1"));
        VERIFY_IS_NOT_NULL(rectangle1);
        VERIFY_IS_NOT_NULL(rectangle1->Fill);
        VERIFY_ARE_EQUAL(safe_cast<SolidColorBrush^>(rectangle1->Fill)->Color, Colors::Green);
    }

    static void ValidateDeferredElementInDataTemplateCanUseLocalResourcesFromLoadXaml(const wchar_t* deferAttribute)
    {
        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"Defer Attribute: %s", deferAttribute));

        ContentControl^ contentControl = LoadXamlEnterAndWait<ContentControl>(
            String().Format(
                L"<ContentControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"                xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"                Content='abc'>"
                L"  <ContentControl.ContentTemplate>"
                L"    <DataTemplate>"
                L"      <StackPanel x:Name='rootPanel'>"
                L"        <StackPanel.Resources>"
                L"          <SolidColorBrush x:Key='RedBrush' Color='Red'/>"
                L"        </StackPanel.Resources>"
                L"        <Button x:Name='element1' %s Background='{StaticResource RedBrush}'/>"
                L"        <Button x:Name='element2' %s>"
                L"          <Button.Resources>"
                L"            <SolidColorBrush x:Key='GreenBrush' Color='Green'/>"
                L"          </Button.Resources>"
                L"          <Rectangle x:Name='rectangle1' Fill='{StaticResource GreenBrush}'/>"
                L"        </Button>"
                L"      </StackPanel>"
                L"    </DataTemplate>"
                L"  </ContentControl.ContentTemplate>"
                L"</ContentControl>",
                deferAttribute,
                deferAttribute));

        RunOnUIThread([&]()
        {
            FrameworkElement^ templateRoot = safe_cast<FrameworkElement^>(contentControl->ContentTemplateRoot);
            Panel^ rootPanel = safe_cast<Panel^>(templateRoot->FindName(L"rootPanel"));

            VERIFY_ARE_EQUAL((deferAttribute == s_realizeTrue) ? 2U : 0U, rootPanel->Children->Size);

            ValidateDeferredElementInDataTemplateCanUseLocalResourcesScenario(rootPanel);
        });
    }

    static void ValidateDeferredElementInDataTemplateCanUseLocalResourcesFromXbf()
    {
        using namespace ::Tests::Native::External::Framework::XDefer;

        TestCleanupWrapper cleanup;

        CustomPageResourcesDT^ page = nullptr;

        RunOnUIThread([&]()
        {
            page = ref new CustomPageResourcesDT();
        });

        EnterAndWait(page);

        RunOnUIThread([&]()
        {
            ContentControl^ contentControl = safe_cast<ContentControl^>(page->FindName(L"contentControl0"));
            FrameworkElement^ templateRoot = safe_cast<FrameworkElement^>(contentControl->ContentTemplateRoot);
            Panel^ rootPanel = safe_cast<Panel^>(templateRoot->FindName(L"rootPanel"));

            VERIFY_ARE_EQUAL(0U, rootPanel->Children->Size);

            ValidateDeferredElementInDataTemplateCanUseLocalResourcesScenario(rootPanel);
        });
    }

    static void ValidateDeferredElementInDataTemplateCanUsePageResourcesFromXbf()
    {
        using namespace ::Tests::Native::External::Framework::XDefer;

        TestCleanupWrapper cleanup;

        CustomPageResourcesDT^ page = nullptr;

        RunOnUIThread([&]()
        {
            page = ref new CustomPageResourcesDT();
        });

        EnterAndWait(page);

        RunOnUIThread([&]()
        {
            ContentControl^ contentControl = safe_cast<ContentControl^>(page->FindName(L"contentControl1"));
            FrameworkElement^ templateRoot = safe_cast<FrameworkElement^>(contentControl->ContentTemplateRoot);
            Panel^ rootPanel = safe_cast<Panel^>(templateRoot->FindName(L"rootPanel"));

            VERIFY_ARE_EQUAL(0U, rootPanel->Children->Size);

            // deferred element uses page resource
            TextBlock^ textBlock = safe_cast<TextBlock^>(rootPanel->FindName(L"textBlock"));
            VERIFY_IS_NOT_NULL(textBlock);
            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(contentControl->Content), safe_cast<Platform::String^>(textBlock->Text));
        });
    }

    void ElementDeferralTests::ValidateDeferredElementInDataTemplateCanUseResources()
    {
        ValidateDeferredElementInDataTemplateCanUseLocalResourcesFromLoadXaml(s_deferLoadStrategyLazy);
        ValidateDeferredElementInDataTemplateCanUseLocalResourcesFromLoadXaml(s_realizeTrue);
        ValidateDeferredElementInDataTemplateCanUseLocalResourcesFromLoadXaml(s_realizeFalse);
        ValidateDeferredElementInDataTemplateCanUseLocalResourcesFromXbf();
        ValidateDeferredElementInDataTemplateCanUsePageResourcesFromXbf();
    }

    static void ValidateDeferredElementInControlTemplateCanUseResourcesScenario(const wchar_t* deferAttribute)
    {
        using namespace ::Tests::Native::External::Framework::XDefer;

        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"Defer Attribute: %s", deferAttribute));

        CustomControl^ customControl = nullptr;

        RunOnUIThread([&]()
        {
            ControlTemplate^ controlTemplate = LoadXaml<ControlTemplate>(
                String().Format(
                    L"<ControlTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"                 xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"                 TargetType='Control'>"
                    L"  <StackPanel x:Name='rootPanel'>"
                    L"    <StackPanel.Resources>"
                    L"      <SolidColorBrush x:Key='RedBrush' Color='Red'/>"
                    L"      <SolidColorBrush x:Key='BlueBrush' Color='Blue'/>"
                    L"    </StackPanel.Resources>"
                    L"    <Button x:Name='element1' %s Background='{StaticResource RedBrush}'/>"
                    L"    <Button x:Name='element2' %s>"
                    L"      <Button.Resources>"
                    L"        <SolidColorBrush x:Key='GreenBrush' Color='Green'/>"
                    L"        <SolidColorBrush x:Key='GrayBrush' Color='Gray'/>"
                    L"      </Button.Resources>"
                    L"      <Rectangle x:Name='rectangle1' Fill='{StaticResource GreenBrush}'/>"
                    L"    </Button>"
                    L"    <Rectangle x:Name='rectangle2' %s Fill='{ThemeResource ApplicationForegroundThemeBrush}'/>"
                    L"    <Button x:Name='element3' %s>"
                    L"      <Rectangle x:Name='rectangle3' Fill='{ThemeResource ApplicationForegroundThemeBrush}'/>"
                    L"    </Button>"
                    L"  </StackPanel>"
                    L"</ControlTemplate>",
                    deferAttribute,
                    deferAttribute,
                    deferAttribute,
                    deferAttribute));

            customControl = ref new CustomControl();
            customControl->Template = controlTemplate;
            TestServices::WindowHelper->WindowContent = TreeHelper::WrapInGrid(customControl);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            Panel^ rootPanel = safe_cast<Panel^>(customControl->PublicGetTemplateChild(L"rootPanel"));
            SolidColorBrush^ applicationForegroundThemeBrush = safe_cast<SolidColorBrush^>(Application::Current->Resources->Lookup(L"ApplicationForegroundThemeBrush"));

            VERIFY_ARE_EQUAL((deferAttribute == s_realizeTrue) ? 4U : 0U, rootPanel->Children->Size);

            // deferred element uses parent's resource
            Button^ element1 = safe_cast<Button^>(customControl->PublicGetTemplateChild(L"element1"));
            VERIFY_IS_NOT_NULL(element1);
            VERIFY_IS_NOT_NULL(element1->Background);
            VERIFY_ARE_EQUAL(Colors::Red, safe_cast<SolidColorBrush^>(element1->Background)->Color);

            // element uses resource of deferred parent
            Button^ element2 = safe_cast<Button^>(customControl->PublicGetTemplateChild(L"element2"));
            VERIFY_IS_NOT_NULL(element2);
            Microsoft::UI::Xaml::Shapes::Rectangle^ rectangle1 = safe_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(customControl->PublicGetTemplateChild(L"rectangle1"));
            VERIFY_IS_NOT_NULL(rectangle1);
            VERIFY_IS_NOT_NULL(rectangle1->Fill);
            VERIFY_ARE_EQUAL(Colors::Green, safe_cast<SolidColorBrush^>(rectangle1->Fill)->Color);

            VERIFY_IS_NOT_NULL(applicationForegroundThemeBrush);

            // deferred element uses theme resources
            Microsoft::UI::Xaml::Shapes::Rectangle^ rectangle2 = safe_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(customControl->PublicGetTemplateChild(L"rectangle2"));
            VERIFY_IS_NOT_NULL(rectangle2);
            VERIFY_IS_NOT_NULL(rectangle2->Fill);
            VERIFY_ARE_EQUAL(applicationForegroundThemeBrush->Color, safe_cast<SolidColorBrush^>(rectangle2->Fill)->Color);

            // element with deferred parent uses theme resources
            Button^ element3 = safe_cast<Button^>(customControl->PublicGetTemplateChild(L"element3"));
            VERIFY_IS_NOT_NULL(element3);
            Microsoft::UI::Xaml::Shapes::Rectangle^ rectangle3 = safe_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(customControl->PublicGetTemplateChild(L"rectangle3"));
            VERIFY_IS_NOT_NULL(rectangle3);
            VERIFY_IS_NOT_NULL(rectangle3->Fill);
            VERIFY_ARE_EQUAL(applicationForegroundThemeBrush->Color, safe_cast<SolidColorBrush^>(rectangle3->Fill)->Color);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void ElementDeferralTests::ValidateDeferredElementInControlTemplateCanUseResources()
    {
        ValidateDeferredElementInControlTemplateCanUseResourcesScenario(s_deferLoadStrategyLazy);
        ValidateDeferredElementInControlTemplateCanUseResourcesScenario(s_realizeTrue);
        ValidateDeferredElementInControlTemplateCanUseResourcesScenario(s_realizeFalse);
    }

    static void ValidateDeferredElementCanUseStyleFromParentScenario(const wchar_t* deferAttribute)
    {
        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"Defer Attribute: %s", deferAttribute));

        Grid^ rootPanel = LoadXamlEnterAndWait<Grid>(
            String().Format(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Grid.Resources>"
                L"    <Style TargetType='Button'>"
                L"      <Setter Property='Background' Value='Green'/>"
                L"    </Style>"
                L"    <SolidColorBrush x:Key='OtherBrush' Color='Black'/>"
                L"  </Grid.Resources>"
                L"  <Button x:Name='deferred0' %s Content='deferred0'/>"
                L"</Grid>",
                deferAttribute));

        Button^ realized = nullptr;

        RunOnUIThread([&]()
        {
            realized = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(realized);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_NOT_NULL(realized->Background);
            VERIFY_ARE_EQUAL(Colors::Green, safe_cast<SolidColorBrush^>(realized->Background)->Color);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void ElementDeferralTests::ValidateDeferredElementCanUseStyleFromParent()
    {
        ValidateDeferredElementCanUseStyleFromParentScenario(s_deferLoadStrategyLazy);
        ValidateDeferredElementCanUseStyleFromParentScenario(s_realizeTrue);
        ValidateDeferredElementCanUseStyleFromParentScenario(s_realizeFalse);
    }

    static void ValidateChildrenOfDeferredElementCanUseStyleFromParentScenario(const wchar_t* deferAttribute)
    {
        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"Defer Attribute: %s", deferAttribute));

        Grid^ rootPanel = LoadXamlEnterAndWait<Grid>(
            String().Format(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button x:Name='deferred0' %s>"
                L"    <Button.Resources>"
                L"      <Style TargetType='TextBlock'>"
                L"        <Setter Property='Foreground' Value='Green'/>"
                L"      </Style>"
                L"      <SolidColorBrush x:Key='OtherBrush' Color='Black'/>"
                L"    </Button.Resources>"
                L"    <TextBlock x:Name='textBlock0' Text='hello'/>"
                L"  </Button>"
                L"</Grid>",
                deferAttribute));

        TextBlock^ textBlock0 = nullptr;
        Button^ realized = nullptr;

        RunOnUIThread([&]()
        {
            realized = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(realized);

            textBlock0 = safe_cast<TextBlock^>(rootPanel->FindName(L"textBlock0"));
            VERIFY_IS_NOT_NULL(textBlock0);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_NOT_NULL(textBlock0->Foreground);
            VERIFY_ARE_EQUAL(Colors::Green, safe_cast<SolidColorBrush^>(textBlock0->Foreground)->Color);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void ElementDeferralTests::ValidateChildrenOfDeferredElementCanUseStyleFromParent()
    {
        ValidateChildrenOfDeferredElementCanUseStyleFromParentScenario(s_deferLoadStrategyLazy);
        ValidateChildrenOfDeferredElementCanUseStyleFromParentScenario(s_realizeTrue);
        ValidateChildrenOfDeferredElementCanUseStyleFromParentScenario(s_realizeFalse);
    }

    #pragma endregion

    #pragma region Binding and DataContext

    static void ValidateElementCanBindToDeferredElementPropertyScenario(const wchar_t* deferAttribute)
    {
        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"Defer Attribute: %s", deferAttribute));

        Grid^ rootPanel = LoadXamlEnterAndWait<Grid>(
            String().Format(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"      Background='{Binding ElementName=button,Path=Background}'>"
                L"  <Button x:Name='button' %s Background='Yellow'/>"
                L"</Grid>",
                deferAttribute));

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1U, rootPanel->Children->Size); // button is already realized by binding
            Button^ button = safe_cast<Button^>(rootPanel->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);
            VERIFY_IS_NOT_NULL(rootPanel->Background);
            VERIFY_ARE_EQUAL(Colors::Yellow, safe_cast<SolidColorBrush^>(rootPanel->Background)->Color);
            VERIFY_ARE_EQUAL(1U, rootPanel->Children->Size);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void ElementDeferralTests::ValidateElementCanBindToDeferredElementProperty()
    {
        ValidateElementCanBindToDeferredElementPropertyScenario(s_deferLoadStrategyLazy);
        ValidateElementCanBindToDeferredElementPropertyScenario(s_realizeTrue);
        ValidateElementCanBindToDeferredElementPropertyScenario(s_realizeFalse);
    }

    static void ValidateDeferredElementCanBindToParentElementPropertyScenario(const wchar_t* deferAttribute)
    {
        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"Defer Attribute: %s", deferAttribute));

        Button^ button = nullptr;
        Grid^ rootPanel = LoadXaml<Grid>(
            String().Format(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"      x:Name='rootPanel'"
                L"      Background='Yellow'>"
                L"  <Grid x:Name='innerPanel'"
                L"        Background='Green'>"
                L"    <Button x:Name='button' %s Background='{Binding ElementName=rootPanel,Path=Background}'/>"
                L"  </Grid>"
                L"</Grid>",
                deferAttribute));

        RunOnUIThread([&]()
        {
            Panel^ innerPanel = safe_cast<Panel^>(rootPanel->FindName(L"innerPanel"));
            VERIFY_ARE_EQUAL(1U, rootPanel->Children->Size);
            VERIFY_ARE_EQUAL((deferAttribute == s_realizeTrue) ? 1U : 0U, innerPanel->Children->Size);
            button = safe_cast<Button^>(rootPanel->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);
            VERIFY_ARE_EQUAL(1U, innerPanel->Children->Size);
        });

        EnterAndWait(rootPanel);

        RunOnUIThread([&]()
        {
            VERIFY_IS_NOT_NULL(button->Background);
            VERIFY_ARE_EQUAL(Colors::Yellow, safe_cast<SolidColorBrush^>(button->Background)->Color);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void ElementDeferralTests::ValidateDeferredElementCanBindToParentElementProperty()
    {
        ValidateDeferredElementCanBindToParentElementPropertyScenario(s_deferLoadStrategyLazy);
        ValidateDeferredElementCanBindToParentElementPropertyScenario(s_realizeTrue);
        ValidateDeferredElementCanBindToParentElementPropertyScenario(s_realizeFalse);
    }

    static void ValidateDeferredElementCanBindInDataTemplateScenario(const wchar_t* deferAttribute)
    {
        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"Defer Attribute: %s", deferAttribute));

        ContentControl^ contentControl = LoadXamlEnterAndWait<ContentControl>(
            String().Format(
                L"<ContentControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"                xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"                Content='abc'>"
                L"  <ContentControl.ContentTemplate>"
                L"    <DataTemplate>"
                L"      <Grid x:Name='rootPanel'"
                L"            Tag='Yellow'>"
                L"        <StackPanel x:Name='innerPanel'"
                L"                    Tag='{Binding ElementName=element1,Path=Tag}'>"
                L"          <Button x:Name='element1' %s Tag='Green'/>"
                L"          <Button x:Name='element2' %s Content='{Binding}'/>"
                L"          <Button x:Name='element3' %s Tag='{Binding ElementName=rootPanel,Path=Tag}'/>"
                L"        </StackPanel>"
                L"      </Grid>"
                L"    </DataTemplate>"
                L"  </ContentControl.ContentTemplate>"
                L"</ContentControl>",
                deferAttribute,
                deferAttribute,
                deferAttribute));

        RunOnUIThread([&]()
        {
            Panel^ rootPanel = safe_cast<Panel^>(contentControl->ContentTemplateRoot);
            Panel^ innerPanel = safe_cast<Panel^>(rootPanel->FindName(L"innerPanel"));

            VERIFY_ARE_EQUAL((deferAttribute == s_realizeTrue) ? 3U : 1U, innerPanel->Children->Size);

            // parent binds to deferred child's property by element name
            Button^ element1 = safe_cast<Button^>(rootPanel->FindName(L"element1"));
            VERIFY_IS_NOT_NULL(element1);
            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(element1->Tag), safe_cast<Platform::String^>(innerPanel->Tag));
            VERIFY_ARE_EQUAL((deferAttribute == s_realizeTrue) ? 3U : 1U, innerPanel->Children->Size);
            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(contentControl->Content), safe_cast<Platform::String^>(element1->DataContext));

            // deferred binds to content
            Button^ element2 = safe_cast<Button^>(rootPanel->FindName(L"element2"));
            VERIFY_IS_NOT_NULL(element2);
            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(element2->Content), safe_cast<Platform::String^>(contentControl->Content));
            VERIFY_ARE_EQUAL((deferAttribute == s_realizeTrue) ? 3U : 2U, innerPanel->Children->Size);
            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(contentControl->Content), safe_cast<Platform::String^>(element2->DataContext));

            // deferred binds to parent's property by element name
            Button^ element3 = safe_cast<Button^>(rootPanel->FindName(L"element3"));
            VERIFY_IS_NOT_NULL(element3);
            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(element3->Tag), safe_cast<Platform::String^>(rootPanel->Tag));
            VERIFY_ARE_EQUAL(3U, innerPanel->Children->Size);
            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(contentControl->Content), safe_cast<Platform::String^>(element3->DataContext));
        });
    }

    void ElementDeferralTests::ValidateDeferredElementCanBindInDataTemplate()
    {
        ValidateDeferredElementCanBindInDataTemplateScenario(s_deferLoadStrategyLazy);
        ValidateDeferredElementCanBindInDataTemplateScenario(s_realizeTrue);
        ValidateDeferredElementCanBindInDataTemplateScenario(s_realizeFalse);
    }

    static void ValidateDeferredElementCanBindInControlTemplateScenario(const wchar_t* deferAttribute)
    {
        using namespace ::Tests::Native::External::Framework::XDefer;

        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"Defer Attribute: %s", deferAttribute));

        CustomControl^ customControl = nullptr;
        Panel^ rootPanel = nullptr;
        Panel^ innerPanel = nullptr;
        SolidColorBrush^ backgroundBrush = nullptr;
        Button^ element1 = nullptr;
        Button^ element2 = nullptr;
        Button^ element3 = nullptr;
        Button^ element4 = nullptr;

        RunOnUIThread([&]()
        {
            ControlTemplate^ controlTemplate = LoadXaml<ControlTemplate>(
                String().Format(
                    L"<ControlTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"                 xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"                 TargetType='Control'>"
                    L"  <Grid x:Name='rootPanel'"
                    L"        Tag='Yellow'"
                    L"        Background='Red'>"
                    L"    <StackPanel x:Name='innerPanel'"
                    L"                Tag='{Binding ElementName=element1,Path=Tag}'>"
                    L"      <Button x:Name='element1' %s Tag='Green'/>"
                    L"      <Button x:Name='element2' %s Tag='{Binding ElementName=rootPanel,Path=Tag}'/>"
                    L"      <Button x:Name='element3' %s Background='{TemplateBinding Background}'/>"
                    L"      <Button x:Name='element4' %s Background='{Binding Path=Background,RelativeSource={RelativeSource TemplatedParent}}'/>"
                    L"    </StackPanel>"
                    L"  </Grid>"
                    L"</ControlTemplate>",
                    deferAttribute,
                    deferAttribute,
                    deferAttribute,
                    deferAttribute));

            customControl = ref new CustomControl();
            customControl->Template = controlTemplate;
            TestServices::WindowHelper->WindowContent = TreeHelper::WrapInGrid(customControl);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<Panel^>(customControl->PublicGetTemplateChild(L"rootPanel"));
            innerPanel = safe_cast<Panel^>(customControl->PublicGetTemplateChild(L"innerPanel"));

            VERIFY_ARE_EQUAL((deferAttribute == s_realizeTrue) ? 4U : 1U, innerPanel->Children->Size);

            // parent binds to deferred child's property by element name
            element1 = safe_cast<Button^>(customControl->PublicGetTemplateChild(L"element1"));
            VERIFY_IS_NOT_NULL(element1);
            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(element1->Tag), safe_cast<Platform::String^>(innerPanel->Tag));
            VERIFY_ARE_EQUAL((deferAttribute == s_realizeTrue) ? 4U : 1U, innerPanel->Children->Size);

            // deferred binds to parent's property by element name
            element2 = safe_cast<Button^>(customControl->PublicGetTemplateChild(L"element2"));
            VERIFY_IS_NOT_NULL(element2);
            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(element2->Tag), safe_cast<Platform::String^>(rootPanel->Tag));
            VERIFY_ARE_EQUAL((deferAttribute == s_realizeTrue) ? 4U : 2U, innerPanel->Children->Size);

            // deferred binds to parent's property by template binding
            backgroundBrush = ref new SolidColorBrush(Microsoft::UI::Colors::DarkGreen);
            customControl->Background = backgroundBrush;
            element3 = safe_cast<Button^>(customControl->PublicGetTemplateChild(L"element3"));
            VERIFY_IS_NOT_NULL(element3);
            VERIFY_ARE_EQUAL((deferAttribute == s_realizeTrue) ? 4U : 3U, innerPanel->Children->Size);

            // deferred binds to parent's property by templated parent
            element4 = safe_cast<Button^>(customControl->PublicGetTemplateChild(L"element4"));
            VERIFY_IS_NOT_NULL(element4);
            VERIFY_ARE_EQUAL(4U, innerPanel->Children->Size);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(backgroundBrush->Color, safe_cast<SolidColorBrush^>(element3->Background)->Color);
            VERIFY_ARE_EQUAL(backgroundBrush->Color, safe_cast<SolidColorBrush^>(element4->Background)->Color);
        });
    }

    void ElementDeferralTests::ValidateDeferredElementCanBindInControlTemplate()
    {
        ValidateDeferredElementCanBindInControlTemplateScenario(s_deferLoadStrategyLazy);
        ValidateDeferredElementCanBindInControlTemplateScenario(s_realizeTrue);
        ValidateDeferredElementCanBindInControlTemplateScenario(s_realizeFalse);
    }

    static void ValidateTemplateBindingWhenRealizedFromOnApplyTemplateScenario(const wchar_t* deferAttribute)
    {
        using namespace ::Tests::Native::External::Framework::XDefer;

        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"Defer Attribute: %s", deferAttribute));

        CustomControl^ customControl = nullptr;
        Panel^ rootPanel = nullptr;
        SolidColorBrush^ backgroundBrush = nullptr;
        Button^ realized = nullptr;

        RunOnUIThread([&]()
        {
            ControlTemplate^ controlTemplate = LoadXaml<ControlTemplate>(
                String().Format(
                    L"<ControlTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"                 xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"                 TargetType='Control'>"
                    L"  <Grid x:Name='rootPanel'>"
                    L"    <Button x:Name='realize_in_applytemplate' %s Background='{TemplateBinding Background}'/>"
                    L"  </Grid>"
                    L"</ControlTemplate>",
                    deferAttribute));

            customControl = ref new CustomControl();
            customControl->Template = controlTemplate;

            backgroundBrush = ref new SolidColorBrush(Microsoft::UI::Colors::DarkGreen);
            customControl->Background = backgroundBrush;

            TestServices::WindowHelper->WindowContent = TreeHelper::WrapInGrid(customControl);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<Panel^>(customControl->PublicGetTemplateChild(L"rootPanel"));
            VERIFY_ARE_EQUAL(1U, rootPanel->Children->Size);
            realized = safe_cast<Button^>(customControl->GetRealizedInApplyTemplate());
            VERIFY_IS_NOT_NULL(realized);
            VERIFY_ARE_EQUAL(backgroundBrush->Color, safe_cast<SolidColorBrush^>(realized->Background)->Color);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void ElementDeferralTests::ValidateTemplateBindingWhenRealizedFromOnApplyTemplate()
    {
        ValidateTemplateBindingWhenRealizedFromOnApplyTemplateScenario(s_deferLoadStrategyLazy);
        ValidateTemplateBindingWhenRealizedFromOnApplyTemplateScenario(s_realizeTrue);
        ValidateTemplateBindingWhenRealizedFromOnApplyTemplateScenario(s_realizeFalse);
    }

    static void ValidateDeferredElementGetsDataContextScenario(const wchar_t* deferAttribute)
    {
        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"Defer Attribute: %s", deferAttribute));

        Grid^ rootPanel = LoadXamlEnterAndWait<Grid>(
            String().Format(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"      Tag='rootPanel'>"
                L"  <StackPanel x:Name='innerPanel' Tag='innerPanel'>"
                L"    <Button x:Name='b1' %s Content='{Binding Tag}'/>"
                L"    <Button x:Name='b2' %s Content='{Binding Tag}'/>"
                L"    <Button x:Name='b3' %s Content='{Binding Tag}' Tag='button#3'/>"
                L"  </StackPanel>"
                L"</Grid>",
                deferAttribute,
                deferAttribute,
                deferAttribute));

        StackPanel^ innerPanel = nullptr;
        Button^ b1 = nullptr;
        Button^ b2 = nullptr;
        Button^ b3 = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel->DataContext = rootPanel;

            innerPanel = safe_cast<StackPanel^>(rootPanel->FindName(L"innerPanel"));
            VERIFY_IS_NOT_NULL(innerPanel);

            b1 = safe_cast<Button^>(rootPanel->FindName(L"b1"));
            VERIFY_IS_NOT_NULL(b1);

            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(rootPanel->Tag), safe_cast<Platform::String^>(b1->Content));
            VERIFY_ARE_EQUAL(safe_cast<Grid^>(rootPanel->DataContext), safe_cast<Grid^>(b1->DataContext));

            innerPanel->DataContext = innerPanel;

            b2 = safe_cast<Button^>(rootPanel->FindName(L"b2"));
            VERIFY_IS_NOT_NULL(b2);

            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(innerPanel->Tag), safe_cast<Platform::String^>(b1->Content));
            VERIFY_ARE_EQUAL(safe_cast<StackPanel^>(innerPanel->DataContext), safe_cast<StackPanel^>(b1->DataContext));

            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(innerPanel->Tag), safe_cast<Platform::String^>(b2->Content));
            VERIFY_ARE_EQUAL(safe_cast<StackPanel^>(innerPanel->DataContext), safe_cast<StackPanel^>(b2->DataContext));

            b3 = safe_cast<Button^>(rootPanel->FindName(L"b3"));
            VERIFY_IS_NOT_NULL(b3);

            b3->DataContext = b3;

            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(b3->Tag), safe_cast<Platform::String^>(b3->Content));
            VERIFY_ARE_EQUAL(safe_cast<Button^>(b3->DataContext), safe_cast<Button^>(b3->DataContext));
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void ElementDeferralTests::ValidateDeferredElementGetsDataContext()
    {
        // Leak: TemplateContent peer not being unpegged
        TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

        ValidateDeferredElementGetsDataContextScenario(s_deferLoadStrategyLazy);
        ValidateDeferredElementGetsDataContextScenario(s_realizeTrue);
        ValidateDeferredElementGetsDataContextScenario(s_realizeFalse);
    }

    #pragma endregion

    #pragma region Ordering of elements

    static void ValidateOrderDependsOnDeclarationOrderScenario(const wchar_t* markup, const array<unsigned, 3>& realizeOrder, const array<unsigned, 3>& expectedOrder)
    {
        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"Realize order: %d, %d, %d", realizeOrder[0], realizeOrder[1], realizeOrder[2]));

        Grid^ rootPanel = LoadXamlEnterAndWait<Grid>(markup);

        RunOnUIThread([&]()
        {
            array<Button^, 3> buttons = {};

            for (unsigned index : realizeOrder)
            {
                buttons[index] = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, index)));
                VERIFY_IS_NOT_NULL(buttons[index]);
            }

            for (unsigned index = 0; index < expectedOrder.size(); ++index)
            {
                VERIFY_ARE_EQUAL(buttons[index], safe_cast<Button^>(rootPanel->Children->GetAt(expectedOrder[index])));
            }

            ClearArray(buttons);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void ElementDeferralTests::ValidateOrderDependsOnDeclarationOrder()
    {
        // final order should be independent of realization order, e.g. elements are in declared order

        array<unsigned, 3> scenarios[] = { { 0, 1, 2 }, { 0, 2, 1 },
                                            { 1, 0, 2 }, { 1, 2, 0 },
                                            { 2, 0, 1 }, { 2, 1, 0 } };

        // no normal elements in XAML
        for (auto& scenario : scenarios)
        {
            ValidateOrderDependsOnDeclarationOrderScenario(s_markup1, scenario, { 0, 1, 2 });
        }

        // normal elements interleaved in XAML
        for (auto& scenario : scenarios)
        {
            ValidateOrderDependsOnDeclarationOrderScenario(s_markup2, scenario, { 1, 3, 5 });
        }
    }

    void ElementDeferralTests::ValidateInsertionOfNormalElement()
    {
        TestCleanupWrapper cleanup;

        // deferred element de @ index 0
        // insert normal element @ index 0
        // realize de, should go to index 1
        Grid^ rootPanel = LoadXamlEnterAndWait<Grid>(s_markup1);

        RunOnUIThread([&]()
        {
            Button^ normal0 = ref new Button();

            rootPanel->Children->InsertAt(0, normal0);

            VERIFY_ARE_EQUAL(1U, rootPanel->Children->Size);

            Button^ deferred0 = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(deferred0);

            VERIFY_ARE_EQUAL(2U, rootPanel->Children->Size);

            VERIFY_ARE_EQUAL(normal0, safe_cast<Button^>(rootPanel->Children->GetAt(0)));
            VERIFY_ARE_EQUAL(deferred0, safe_cast<Button^>(rootPanel->Children->GetAt(1)));
        });

        TestServices::WindowHelper->WaitForIdle();

        // deferred element de @ index 0
        // append normal element (effectively @ index 0)
        // realize de, should go to index 0 and normal element gets moved to 1
        rootPanel = LoadXamlEnterAndWait<Grid>(s_markup1);

        RunOnUIThread([&]()
        {
            Button^ normal0 = ref new Button();

            rootPanel->Children->Append(normal0);

            VERIFY_ARE_EQUAL(1U, rootPanel->Children->Size);

            Button^ deferred0 = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(deferred0);

            VERIFY_ARE_EQUAL(2U, rootPanel->Children->Size);

            VERIFY_ARE_EQUAL(deferred0, safe_cast<Button^>(rootPanel->Children->GetAt(0)));
            VERIFY_ARE_EQUAL(normal0, safe_cast<Button^>(rootPanel->Children->GetAt(1)));
        });

        TestServices::WindowHelper->WaitForIdle();

        // deferred element de @ index 0, 1
        // realize de0, should go to index 0
        // insert normal element @ index 0
        // de0 goes to index 1
        // realize de1, should go to index 2
        rootPanel = LoadXamlEnterAndWait<Grid>(s_markup1);

        RunOnUIThread([&]()
        {
            Button^ normal0 = ref new Button();

            Button^ deferred0 = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(deferred0);

            VERIFY_ARE_EQUAL(1U, rootPanel->Children->Size);

            rootPanel->Children->InsertAt(0, normal0);

            VERIFY_ARE_EQUAL(2U, rootPanel->Children->Size);

            VERIFY_ARE_EQUAL(normal0, safe_cast<Button^>(rootPanel->Children->GetAt(0)));
            VERIFY_ARE_EQUAL(deferred0, safe_cast<Button^>(rootPanel->Children->GetAt(1)));

            Button^ deferred1 = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 1)));
            VERIFY_IS_NOT_NULL(deferred1);

            VERIFY_ARE_EQUAL(3U, rootPanel->Children->Size);

            VERIFY_ARE_EQUAL(normal0, safe_cast<Button^>(rootPanel->Children->GetAt(0)));
            VERIFY_ARE_EQUAL(deferred0, safe_cast<Button^>(rootPanel->Children->GetAt(1)));
            VERIFY_ARE_EQUAL(deferred1, safe_cast<Button^>(rootPanel->Children->GetAt(2)));
        });

        TestServices::WindowHelper->WaitForIdle();

        // deferred element de @ index 0, 1
        // realize de0, should go to index 0
        // insert normal element @ index 1
        // realize de1, should go to index 2
        rootPanel = LoadXamlEnterAndWait<Grid>(s_markup1);

        RunOnUIThread([&]()
        {
            Button^ normal0 = ref new Button();

            Button^ deferred0 = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(deferred0);

            VERIFY_ARE_EQUAL(1U, rootPanel->Children->Size);

            rootPanel->Children->InsertAt(1, normal0);

            VERIFY_ARE_EQUAL(2U, rootPanel->Children->Size);

            VERIFY_ARE_EQUAL(deferred0, safe_cast<Button^>(rootPanel->Children->GetAt(0)));
            VERIFY_ARE_EQUAL(normal0, safe_cast<Button^>(rootPanel->Children->GetAt(1)));

            Button^ deferred1 = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 1)));
            VERIFY_IS_NOT_NULL(deferred1);

            VERIFY_ARE_EQUAL(3U, rootPanel->Children->Size);

            VERIFY_ARE_EQUAL(deferred0, safe_cast<Button^>(rootPanel->Children->GetAt(0)));
            VERIFY_ARE_EQUAL(normal0, safe_cast<Button^>(rootPanel->Children->GetAt(1)));
            VERIFY_ARE_EQUAL(deferred1, safe_cast<Button^>(rootPanel->Children->GetAt(2)));
        });

        TestServices::WindowHelper->WaitForIdle();

        // deferred element de @ index 0, 1
        // realize de0, should go to index 0
        // append normal element @ index 1
        // realize de1, should go to index 1, normal element goes to 2
        rootPanel = LoadXamlEnterAndWait<Grid>(s_markup1);

        RunOnUIThread([&]()
        {
            Button^ normal0 = ref new Button();

            Button^ deferred0 = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(deferred0);

            VERIFY_ARE_EQUAL(1U, rootPanel->Children->Size);

            rootPanel->Children->Append(normal0);

            VERIFY_ARE_EQUAL(2U, rootPanel->Children->Size);

            VERIFY_ARE_EQUAL(deferred0, safe_cast<Button^>(rootPanel->Children->GetAt(0)));
            VERIFY_ARE_EQUAL(normal0, safe_cast<Button^>(rootPanel->Children->GetAt(1)));

            Button^ deferred1 = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 1)));
            VERIFY_IS_NOT_NULL(deferred1);

            VERIFY_ARE_EQUAL(3U, rootPanel->Children->Size);

            VERIFY_ARE_EQUAL(deferred0, safe_cast<Button^>(rootPanel->Children->GetAt(0)));
            VERIFY_ARE_EQUAL(deferred1, safe_cast<Button^>(rootPanel->Children->GetAt(1)));
            VERIFY_ARE_EQUAL(normal0, safe_cast<Button^>(rootPanel->Children->GetAt(2)));
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    static void ValidateRemovalOfNormalElementScenario(const array<int, 4>& scenario, const array<int, 2>& expected)
    {
        // positive index means realize deferredN
        // negative index means remove normal(-N-1)

        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"Scenario order: %d, %d, %d, %d", scenario[0], scenario[1], scenario[2], scenario[3]));

        Grid^ rootPanel = LoadXamlEnterAndWait<Grid>(s_markup3);

        RunOnUIThread([&]()
        {
            array<Button^, 2> normal = {};
            array<Button^, 2> realized = {};

            normal[0] = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Normal, 0)));
            normal[1] = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Normal, 1)));

            VERIFY_ARE_EQUAL(2U, rootPanel->Children->Size);

            for (int i : scenario)
            {
                if (i < 0)
                {
                    RemovePanelChildByReference(rootPanel, normal[-i - 1]);
                }
                else
                {
                    realized[i] = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, i)));
                    VERIFY_IS_NOT_NULL(realized[i]);
                }
            }

            for (unsigned i = 0; i < expected.size(); ++i)
            {
                Button^ expectedElement = nullptr;
                int expectedValue = expected[i];

                if (expectedValue < 0)
                {
                    expectedElement = normal[-expectedValue - 1];
                }
                else
                {
                    expectedElement = realized[expectedValue];
                }

                VERIFY_ARE_EQUAL(expectedElement, safe_cast<Button^>(rootPanel->Children->GetAt(i)));
            }

            ClearArray(normal);
            ClearArray(realized);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void ElementDeferralTests::ValidateRemovalOfNormalElement()
    {
        array<int, 4> scenarios[] =
            { { -2, -1, 0, 1 }, { -2, -1, 1, 0 }, { -2, 0, -1, 1 }, { -2, 0, 1, -1 }, { -2, 1, -1, 0 }, { -2, 1, 0, -1 },
                { -1, -2, 0, 1 }, { -1, -2, 1, 0 }, { -1, 0, -2, 1 }, { -1, 0, 1, -2 }, { -1, 1, -2, 0 }, { -1, 1, 0, -2 },
                { 0, -2, -1, 1 }, { 0, -2, 1, -1 }, { 0, -1, -2, 1 }, { 0, -1, 1, -2 }, { 0, 1, -2, -1 }, { 0, 1, -1, -2 },
                { 1, -2, -1, 0 }, { 1, -2, 0, -1 }, { 1, -1, -2, 0 }, { 1, -1, 0, -2 }, { 1, 0, -2, -1 }, { 1, 0, -1, -2 } };

        for (auto& scenario : scenarios)
        {
            ValidateRemovalOfNormalElementScenario(scenario, { 0, 1 });
        }
    }

    static void ValidateRealizedElementRemoveAndInsertScenario(unsigned deferredToRemove, int insertIndex, const array<int, 3>& expected)
    {
        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"deferredToRemove: %u, insertIndex: %d", deferredToRemove, insertIndex));

        Grid^ rootPanel = LoadXamlEnterAndWait<Grid>(s_markup4);

        RunOnUIThread([&]()
        {
            array<Button^, 2> normal = {};
            Button^ realized = nullptr;

            normal[0] = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Normal, 0)));
            normal[1] = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Normal, 1)));
            realized = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, deferredToRemove)));

            VERIFY_ARE_EQUAL(3U, rootPanel->Children->Size);

            RemovePanelChildByReference(rootPanel, realized);

            if (insertIndex >= 0)
            {
                rootPanel->Children->InsertAt(insertIndex, realized);
            }

            for (unsigned i = 0; i < expected.size(); ++i)
            {
                Button^ expectedElement = nullptr;
                int expectedValue = expected[i];

                if (expectedValue < 0)
                {
                    expectedElement = normal[-expectedValue - 1];
                }
                else
                {
                    expectedElement = realized;
                }

                VERIFY_ARE_EQUAL(expectedElement, safe_cast<Button^>(rootPanel->Children->GetAt(i)));
            }

            ClearArray(normal);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void ElementDeferralTests::ValidateRealizedElementRemoveAndInsert()
    {
        for (unsigned toMove = 0; toMove < 3; ++toMove)
        {
            ValidateRealizedElementRemoveAndInsertScenario(toMove, 0, { static_cast<int>(toMove), -1, -2 });
            ValidateRealizedElementRemoveAndInsertScenario(toMove, 1, { -1, static_cast<int>(toMove), -2 });
            ValidateRealizedElementRemoveAndInsertScenario(toMove, 2, { -1, -2, static_cast<int>(toMove) });
        }
    }

    static void ValidateDeferredElementCanBeRealizedAfterItsParentIsRemovedAndInsertedScenario0(int loadTime)
    {
        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"Can realize from grandparent and realized element is the same as seen by parent, load time: %d", loadTime));

        Grid^ rootPanel = LoadXaml<Grid>(
            String().Format(
                s_markup5,
                s_deferLoadStrategyLazy));

        Grid^ innerPanel = nullptr;
        Button^ realized0 = nullptr;
        Button^ realized1 = nullptr;
        WeakReference realizedWeakRef;

        RunOnUIThread([&]()
        {
            innerPanel = safe_cast<Grid^> (rootPanel->FindName(L"innerPanel"));
        });

        if (loadTime == 0)
        {
            EnterAndWait(rootPanel);
        }

        RunOnUIThread([&]()
        {
            realized0 = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(realized0);
            realizedWeakRef = WeakReference(realized0);
            VERIFY_ARE_EQUAL(1U, innerPanel->Children->Size);
            realized1 = safe_cast<Button^>(innerPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(realized1);
            VERIFY_IS_TRUE(realized0 == realized1);
        });

        if (loadTime != -1)
        {
            TestServices::WindowHelper->WaitForIdle();
        }

        RunOnUIThread([&]()
        {
            Markup::XamlMarkupHelper::UnloadObject(realized0);
            realized0 = nullptr;
            realized1 = nullptr;
        });

        if (loadTime != -1)
        {
            TestServices::WindowHelper->WaitForIdle();
        }

        RunOnUIThread([&]()
        {
            VERIFY_IS_NULL(realizedWeakRef.Resolve<Button>());
        });

        if (loadTime != -1)
        {
            TestServices::WindowHelper->WaitForIdle();
        }
    }

    static void ValidateDeferredElementCanBeRealizedAfterItsParentIsRemovedAndInsertedScenario1(int loadTime)
    {
        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"Can realize after parent is removed and grandparent cannot realize, load time: %d", loadTime));

        Grid^ rootPanel = LoadXaml<Grid>(
            String().Format(
                s_markup5,
                s_deferLoadStrategyLazy));

        Grid^ innerPanel = nullptr;
        Button^ realized0 = nullptr;
        Button^ realized1 = nullptr;

        if (loadTime == 0)
        {
            EnterAndWait(rootPanel);
        }

        RunOnUIThread([&]()
        {
            innerPanel = safe_cast<Grid^> (rootPanel->FindName(L"innerPanel"));
            rootPanel->Children->RemoveAt(0);
            VERIFY_ARE_EQUAL(0U, rootPanel->Children->Size);
            realized0 = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NULL(realized0);
            VERIFY_ARE_EQUAL(0U, rootPanel->Children->Size);
            realized1 = safe_cast<Button^>(innerPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NULL(realized1);
            VERIFY_ARE_EQUAL(0U, rootPanel->Children->Size);
            VERIFY_ARE_EQUAL(0U, innerPanel->Children->Size);
        });

        if (loadTime != -1)
        {
            TestServices::WindowHelper->WaitForIdle();
        }
    }

    static void ValidateDeferredElementCanBeRealizedAfterItsParentIsRemovedAndInsertedScenario2(int loadTime)
    {
        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"Can realize from parent after parent is removed and reinserted, load time: %d", loadTime));

        Grid^ rootPanel = LoadXaml<Grid>(
            String().Format(
                s_markup5,
                s_deferLoadStrategyLazy));

        Grid^ innerPanel = nullptr;
        Button^ realized0 = nullptr;
        Button^ realized1 = nullptr;
        WeakReference realizedWeakRef;

        if (loadTime == 0)
        {
            EnterAndWait(rootPanel);
        }

        RunOnUIThread([&]()
        {
            innerPanel = safe_cast<Grid^> (rootPanel->FindName(L"innerPanel"));
            rootPanel->Children->RemoveAt(0);
            VERIFY_ARE_EQUAL(0U, rootPanel->Children->Size);
            rootPanel->Children->Append(innerPanel);
        });

        if (loadTime == 1)
        {
            EnterAndWait(rootPanel);
        }

        RunOnUIThread([&]()
        {
            realized0 = safe_cast<Button^>(innerPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(realized0);
            realizedWeakRef = WeakReference(realized0);
            VERIFY_ARE_EQUAL(1U, innerPanel->Children->Size);
            realized1 = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(realized1);
            VERIFY_IS_TRUE(realized0 == realized1);
        });

        if (loadTime != -1)
        {
            TestServices::WindowHelper->WaitForIdle();
        }

        RunOnUIThread([&]()
        {
            Markup::XamlMarkupHelper::UnloadObject(realized0);
            realized0 = nullptr;
            realized1 = nullptr;
        });

        if (loadTime != -1)
        {
            TestServices::WindowHelper->WaitForIdle();
        }

        RunOnUIThread([&]()
        {
            VERIFY_IS_NULL(realizedWeakRef.Resolve<Button>());
        });

        if (loadTime != -1)
        {
            TestServices::WindowHelper->WaitForIdle();
        }
    }

    static void ValidateDeferredElementCanBeRealizedAfterItsParentIsRemovedAndInsertedScenario3(int loadTime)
    {
        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"Can realize from grandparent after parent is removed and reinserted, load time: %d", loadTime));

        Grid^ rootPanel = LoadXaml<Grid>(
            String().Format(
                s_markup5,
                s_deferLoadStrategyLazy));

        Grid^ innerPanel = nullptr;
        Button^ realized0 = nullptr;
        Button^ realized1 = nullptr;
        WeakReference realizedWeakRef;

        if (loadTime == 0)
        {
            EnterAndWait(rootPanel);
        }

        RunOnUIThread([&]()
        {
            innerPanel = safe_cast<Grid^> (rootPanel->FindName(L"innerPanel"));
            rootPanel->Children->RemoveAt(0);
            VERIFY_ARE_EQUAL(0U, rootPanel->Children->Size);
            rootPanel->Children->Append(innerPanel);
        });

        if (loadTime == 1)
        {
            EnterAndWait(rootPanel);
        }

        RunOnUIThread([&]()
        {
            realized0 = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(realized0);
            realizedWeakRef = WeakReference(realized0);
            VERIFY_ARE_EQUAL(1U, innerPanel->Children->Size);
            realized1 = safe_cast<Button^>(innerPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(realized1);
            VERIFY_IS_TRUE(realized0 == realized1);
        });

        if (loadTime != -1)
        {
            TestServices::WindowHelper->WaitForIdle();
        }

        RunOnUIThread([&]()
        {
            Markup::XamlMarkupHelper::UnloadObject(realized0);
            realized0 = nullptr;
            realized1 = nullptr;
        });

        if (loadTime != -1)
        {
            TestServices::WindowHelper->WaitForIdle();
        }

        RunOnUIThread([&]()
        {
            VERIFY_IS_NULL(realizedWeakRef.Resolve<Button>());
        });

        if (loadTime != -1)
        {
            TestServices::WindowHelper->WaitForIdle();
        }
    }

    void ElementDeferralTests::ValidateDeferredElementCanBeRealizedAfterItsParentIsRemovedAndInserted()
    {
        ValidateDeferredElementCanBeRealizedAfterItsParentIsRemovedAndInsertedScenario0(-1);
        ValidateDeferredElementCanBeRealizedAfterItsParentIsRemovedAndInsertedScenario0(0);
        ValidateDeferredElementCanBeRealizedAfterItsParentIsRemovedAndInsertedScenario1(-1);
        ValidateDeferredElementCanBeRealizedAfterItsParentIsRemovedAndInsertedScenario1(0);
        ValidateDeferredElementCanBeRealizedAfterItsParentIsRemovedAndInsertedScenario2(-1);
        ValidateDeferredElementCanBeRealizedAfterItsParentIsRemovedAndInsertedScenario2(0);
        ValidateDeferredElementCanBeRealizedAfterItsParentIsRemovedAndInsertedScenario2(1);
        ValidateDeferredElementCanBeRealizedAfterItsParentIsRemovedAndInsertedScenario3(-1);
        ValidateDeferredElementCanBeRealizedAfterItsParentIsRemovedAndInsertedScenario3(0);
        ValidateDeferredElementCanBeRealizedAfterItsParentIsRemovedAndInsertedScenario3(1);
    }

    static void VerifyChildAndRealize(UserControl^ uc, Border^ parent, bool realize)
    {
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_NULL(parent->Child);

            if (realize)
            {
                TextBlock^ deferred = safe_cast<TextBlock^>(uc->FindName(L"deferred"));
                VERIFY_IS_NOT_NULL(deferred);
                VERIFY_ARE_EQUAL(deferred, safe_cast<TextBlock^>(parent->Child));
            }
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    static void ValidateDeferredElementInUserControlCanBeRealizedAfterItIsRemovedAndInsertedScenario(int realizeTime)
    {
        TestCleanupWrapper cleanup;

        Grid^ host = nullptr;
        Border^ parent = nullptr;

        Grid^ rootPanel = nullptr;

        Log::Comment(String().Format(L"Can realize after parent in nested UserControls is removed and reinserted, realize time: %d", realizeTime));

        UserControl^ userControlPane = LoadXaml<UserControl>(
            L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"             xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <Grid x:Name='host'/>"
            L"</UserControl>");

        UserControl^ userControl = LoadXaml<UserControl>(
            L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"             xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <Border x:Name='parent'>"
            L"    <TextBlock x:Name='deferred' x:DeferLoadStrategy='Lazy'/>"
            L"  </Border>"
            L"</UserControl>");

        RunOnUIThread([&]()
        {
            rootPanel = ref new Grid();
            host = safe_cast<Grid^>(userControlPane->FindName(L"host"));
            VERIFY_IS_NOT_NULL(host);
            parent = safe_cast<Border^>(userControl->FindName(L"parent"));
            host->Children->Append(userControl);
            rootPanel->Children->Append(userControlPane);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        VerifyChildAndRealize(userControl, parent, (realizeTime == 0));

        if (realizeTime == 0)
        {
            return;
        }

        RunOnUIThread([&]()
        {
            host->Children->RemoveAt(0);
        });

        VerifyChildAndRealize(userControl, parent, (realizeTime == 1));

        if (realizeTime == 1)
        {
            return;
        }

        RunOnUIThread([&]()
        {
            host->Children->InsertAt(0, userControl);
        });

        VerifyChildAndRealize(userControl, parent, (realizeTime == 2));
    }

    void ElementDeferralTests::ValidateDeferredElementInUserControlCanBeRealizedAfterItIsRemovedAndInserted()
    {
        // realize before removing
        ValidateDeferredElementInUserControlCanBeRealizedAfterItIsRemovedAndInsertedScenario(0);

        // realize while removed
        ValidateDeferredElementInUserControlCanBeRealizedAfterItIsRemovedAndInsertedScenario(1);

        // realize after re-added
        ValidateDeferredElementInUserControlCanBeRealizedAfterItIsRemovedAndInsertedScenario(2);
    }

    #pragma endregion

    #pragma region Events

    static void ValidateRealizedElementFiresEventsScenario(bool nullOutContent)
    {
        using namespace ::Tests::Native::External::Framework::XDefer;

        TestCleanupWrapper cleanup;

        CustomPageEvents^ page = nullptr;
        Panel^ rootPanel = nullptr;
        Button^ button = nullptr;
        WeakReference buttonWeakRef;

        RunOnUIThread([&]()
        {
            page = ref new CustomPageEvents();
            TestServices::WindowHelper->WindowContent = TreeHelper::WrapInGrid(page);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<Panel^>(page->FindName(L"rootPanel"));
            VERIFY_ARE_EQUAL(0U, rootPanel->Children->Size);
            button = safe_cast<Button^>(page->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);
            buttonWeakRef = WeakReference(button);
            VERIFY_ARE_EQUAL(1U, rootPanel->Children->Size);
        });

        s_spLoadedEvent0->WaitForDefault();

        RunOnUIThread([&]()
        {
            if (nullOutContent)
            {
                page->Content = nullptr;
            }
            else
            {
                Markup::XamlMarkupHelper::UnloadObject(button);
                button = nullptr;
                page->reset_button_field();
            }
        });

        s_spUnloadedEvent0->WaitForDefault();

        if (!nullOutContent)
        {
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_NULL(buttonWeakRef.Resolve<Button>());
            });
        }

        TestServices::WindowHelper->WaitForIdle();
    }

    void ElementDeferralTests::ValidateRealizedElementFiresEvents()
    {
        ValidateRealizedElementFiresEventsScenario(true);
        ValidateRealizedElementFiresEventsScenario(false);
    }

    #pragma endregion

    #pragma region Animations

    static void ValidateStoryboardRealizesDeferredElementScenario(const wchar_t* deferAttribute)
    {
        TestCleanupWrapper cleanup;

        Log::Comment(String().Format(L"Defer Attribute: %s", deferAttribute));

        Grid^ rootPanel = LoadXamlEnterAndWait<Grid>(
            String().Format(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Grid.Resources>"
                L"    <Storyboard x:Key='sb'>"
                L"      <DoubleAnimation Storyboard.TargetName='deferred0'"
                L"                       Storyboard.TargetProperty='Opacity'"
                L"                       To='0.5'"
                L"                       Duration='0:0:0.25'/>"
                L"    </Storyboard>"
                L"    <SolidColorBrush x:Key='OtherBrush' Color='Black'/>"
                L"  </Grid.Resources>"
                L"  <Button x:Name='deferred0' %s Content='deferred0' Opacity='1'/>"
                L"</Grid>",
                deferAttribute));

        RunOnUIThread([&]()
        {
            Storyboard^ sb = safe_cast<Storyboard^>(rootPanel->Resources->Lookup(Platform::StringReference(L"sb")));
            VERIFY_ARE_EQUAL((deferAttribute == s_realizeTrue) ? 1U : 0U, rootPanel->Children->Size);
            sb->Begin();
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1U, rootPanel->Children->Size);
            Button^ realized = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(realized);
            VERIFY_ARE_EQUAL(0.5, realized->Opacity);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void ElementDeferralTests::ValidateStoryboardRealizesDeferredElement()
    {
        ValidateStoryboardRealizesDeferredElementScenario(s_deferLoadStrategyLazy);
        ValidateStoryboardRealizesDeferredElementScenario(s_realizeTrue);
        ValidateStoryboardRealizesDeferredElementScenario(s_realizeFalse);
    }

    #pragma endregion

    #pragma region Redefer specific

    static void ValidateCanRedeferWhenRealizedElementIsRemovedUIElementChildrenCollection()
    {
        WeakReference button2WeakRef;

        Grid^ rootPanel = LoadXamlEnterAndWait<Grid>(
            String().Format(
                s_markup0,
                s_realizeFalse));

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0U, rootPanel->Children->Size);

            // Just remove, but keep reference
            Button^ button1 = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(button1);

            VERIFY_ARE_EQUAL(1U, rootPanel->Children->Size);

            RemovePanelChildByReference(rootPanel, button1);

            VERIFY_ARE_EQUAL(0U, rootPanel->Children->Size);

            Markup::XamlMarkupHelper::UnloadObject(button1);

            // Remove and destroy
            Button^ button2 = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(button2);
            button2WeakRef = WeakReference(button2);

            VERIFY_IS_TRUE(button1 != button2);
            VERIFY_ARE_EQUAL(1U, rootPanel->Children->Size);

            RemovePanelChildByReference(rootPanel, button2);
            button2 = nullptr;
            VERIFY_IS_NULL(button2WeakRef.Resolve<Button>());

            VERIFY_ARE_EQUAL(0U, rootPanel->Children->Size);

            // can't realize... didn't call DeferTree, so proxy was not registered.
            Button^ button3 = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NULL(button3);

            VERIFY_ARE_EQUAL(0U, rootPanel->Children->Size);
        });
    }

    static void ValidateCanRedeferWhenRealizedElementIsRemovedItemsCollection()
    {
        WeakReference lvi2WeakRef;

        Grid^ rootPanel = LoadXamlEnterAndWait<Grid>(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
            L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <ListView x:Name='lv'>"
            L"    <ListViewItem x:Name='deferred0' x:Load='False' Content='deferred0'/>"
            L"  </ListView>"
            L"</Grid>");

        RunOnUIThread([&]()
        {
            ListView^ lv = safe_cast<ListView^>(rootPanel->FindName(L"lv"));

            VERIFY_ARE_EQUAL(0U, lv->Items->Size);

            // Just remove, but keep reference
            ListViewItem^ lvi1 = safe_cast<ListViewItem^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(lvi1);

            VERIFY_ARE_EQUAL(1U, lv->Items->Size);

            RemoveItemsChildByReference(lv, lvi1);

            VERIFY_ARE_EQUAL(0U, lv->Items->Size);

            Markup::XamlMarkupHelper::UnloadObject(lvi1);

            // Remove and destroy
            ListViewItem^ lvi2 = safe_cast<ListViewItem^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(lvi2);
            lvi2WeakRef = WeakReference(lvi2);

            VERIFY_IS_TRUE(lvi1 != lvi2);
            VERIFY_ARE_EQUAL(1U, lv->Items->Size);

            RemoveItemsChildByReference(lv, lvi2);
            lvi2 = nullptr;
            VERIFY_IS_NULL(lvi2WeakRef.Resolve<ListViewItem>());

            VERIFY_ARE_EQUAL(0U, lv->Items->Size);

            // can't realize... didn't call DeferTree, so proxy was not registered.
            //ListViewItem^ lvi3 = safe_cast<ListViewItem^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            //VERIFY_IS_NULL(lvi3);

            //VERIFY_ARE_EQUAL(0U, lv->Items->Size);
        });
    }

    void ElementDeferralTests::ValidateCanRedeferWhenRealizedElementIsRemoved()
    {
        TestCleanupWrapper cleanup;

        ValidateCanRedeferWhenRealizedElementIsRemovedUIElementChildrenCollection();
        ValidateCanRedeferWhenRealizedElementIsRemovedItemsCollection();
    }

    static void ValidateDeferTreeThrowsForNonRealizedElement()
    {
        DisableErrorReportingScopeGuard disableErrors;

        // Load markup with no x:Load
        Grid^ rootPanel = LoadXaml<Grid>(
            String().Format(
                s_markup5,
                L""));

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1U, rootPanel->Children->Size);

            // get a reference to non-deferred button
            Button^ button = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(button);

            VERIFY_THROWS_WINRT(
                Markup::XamlMarkupHelper::UnloadObject(button),
                Platform::InvalidArgumentException^,
                L"DeferTree throws for non-defered elements");
        });
    }

    static void ValidateDeferTreeThrowsForRealizedElementWithParentRemoved()
    {
        DisableErrorReportingScopeGuard disableErrors;
        WeakReference innerPanelWeakRef;

        Grid^ rootPanel = LoadXaml<Grid>(
            String().Format(
                s_markup5,
                s_realizeFalse));

        RunOnUIThread([&]()
        {
            Grid^ innerPanel = safe_cast<Grid^>(rootPanel->FindName(L"innerPanel"));
            VERIFY_IS_NOT_NULL(innerPanel);
            innerPanelWeakRef = WeakReference(innerPanel);

            Button^ button = safe_cast<Button^>(rootPanel->FindName(GetElementName(ElementType::Deferred, 0)));
            VERIFY_IS_NOT_NULL(button);

            RemovePanelChildByReference(rootPanel, innerPanel);
            innerPanel = nullptr;
            VERIFY_IS_NULL(innerPanelWeakRef.Resolve<Grid>());

            VERIFY_THROWS_WINRT(
                Markup::XamlMarkupHelper::UnloadObject(button),
                Platform::InvalidArgumentException^,
                L"DeferTree throws defered elements whose parents were removed");
        });
    }

    void ElementDeferralTests::ValidateDeferTreeThrowsForInvalidElements()
    {
        TestCleanupWrapper cleanup;

        ValidateDeferTreeThrowsForNonRealizedElement();
        ValidateDeferTreeThrowsForRealizedElementWithParentRemoved();
    }

    static void ValidateNestedDeferWithRealizeValueScenario(bool outerRealizeValue, bool innerRealizeValue, bool onlyDeferOuter)
    {
        Grid^ rootPanel = LoadXamlEnterAndWait<Grid>(
            String().Format(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel x:Name='outer' %s>"
                L"    <Button x:Name='inner' %s/>"
                L"  </StackPanel>"
                L"</Grid>",
                outerRealizeValue ? s_realizeTrue : s_realizeFalse,
                innerRealizeValue ? s_realizeTrue : s_realizeFalse));

        StackPanel^ outer = nullptr;
        Button^ inner = nullptr;

        WeakReference outerWeakRef;
        WeakReference innerWeakRef;

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(outerRealizeValue ? 1U : 0U, rootPanel->Children->Size);

            outer = safe_cast<StackPanel^>(rootPanel->FindName(L"outer"));
            VERIFY_IS_NOT_NULL(outer);
            outerWeakRef = WeakReference(outer);

            VERIFY_ARE_EQUAL(1U, rootPanel->Children->Size);

            VERIFY_ARE_EQUAL(innerRealizeValue ? 1U : 0U, outer->Children->Size);

            inner = safe_cast<Button^>(rootPanel->FindName(L"inner"));
            VERIFY_IS_NOT_NULL(inner);
            innerWeakRef = WeakReference(inner);

            VERIFY_ARE_EQUAL(1U, outer->Children->Size);
        });

        if (!onlyDeferOuter)
        {
            RunOnUIThread([&]()
            {
                Markup::XamlMarkupHelper::UnloadObject(inner);
                inner = nullptr;
                VERIFY_ARE_EQUAL(0U, outer->Children->Size);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_NULL(innerWeakRef.Resolve<Button>());
            });
        }

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            Markup::XamlMarkupHelper::UnloadObject(outer);
            outer = nullptr;
            VERIFY_ARE_EQUAL(0U, rootPanel->Children->Size);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_NULL(outerWeakRef.Resolve<Button>());
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void ElementDeferralTests::ValidateNestedDeferWithRealizeValue()
    {
        TestCleanupWrapper cleanup;

        ValidateNestedDeferWithRealizeValueScenario(false, false, false);
        ValidateNestedDeferWithRealizeValueScenario(false, true, false);
        ValidateNestedDeferWithRealizeValueScenario(true, false, false);
        ValidateNestedDeferWithRealizeValueScenario(true, true, false);
        ValidateNestedDeferWithRealizeValueScenario(false, false, true);
        ValidateNestedDeferWithRealizeValueScenario(false, true, true);
        ValidateNestedDeferWithRealizeValueScenario(true, false, true);
        ValidateNestedDeferWithRealizeValueScenario(true, true, true);
    }

        #pragma endregion
}
