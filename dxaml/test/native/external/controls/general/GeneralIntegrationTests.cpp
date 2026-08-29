// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "GeneralIntegrationTests.h"

#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <FileLoader.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <WUCRenderingScopeGuard.h>
#include <EnablePseudoLoc.h>

#include <collection.h>
#include <TreeHelper.h>
#include <Utils.h>

#include <CommonInputHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace RuntimeFeatureBehavior;
using namespace Platform;

// Pretty much identical to the WRL's InspectableClass macro
// except this one does not have a QueryInterface implimentation
#define InspectableClassNoQueryInterface(runtimeClassName, trustLevel) \
    public: \
        static const wchar_t* STDMETHODCALLTYPE InternalGetRuntimeClassName() noexcept \
        { \
            static_assert((RuntimeClassT::ClassFlags::value & ::Microsoft::WRL::WinRtClassicComMix) == ::Microsoft::WRL::WinRt || \
                (RuntimeClassT::ClassFlags::value & ::Microsoft::WRL::WinRtClassicComMix) == ::Microsoft::WRL::WinRtClassicComMix, \
                    "'InspectableClass' macro must not be used with ClassicCom clasess."); \
            static_assert(__is_base_of(::Microsoft::WRL::Details::RuntimeClassBase, RuntimeClassT), "'InspectableClass' macro can only be used with ::Windows::WRL::RuntimeClass types"); \
            static_assert(!__is_base_of(IActivationFactory, RuntimeClassT), "Incorrect usage of IActivationFactory interface. Make sure that your RuntimeClass doesn't implement IActivationFactory interface use ::Windows::WRL::ActivationFactory instead or 'InspectableClass' macro is not used on ::Windows::WRL::ActivationFactory"); \
            return runtimeClassName; \
        } \
        static TrustLevel STDMETHODCALLTYPE InternalGetTrustLevel() noexcept \
        { \
            return trustLevel; \
        } \
        STDMETHOD(GetRuntimeClassName)(_Out_ HSTRING* runtimeName) \
        { \
            *runtimeName = nullptr; \
            HRESULT hr = S_OK; \
            const wchar_t *name = InternalGetRuntimeClassName(); \
            if (name != nullptr) \
            { \
                hr = ::WindowsCreateString(name, static_cast<UINT32>(::wcslen(name)), runtimeName); \
            } \
            return hr; \
        } \
        STDMETHOD(GetTrustLevel)(_Out_ TrustLevel* trustLvl) \
        { \
            *trustLvl = trustLevel; \
            return S_OK; \
        } \
        STDMETHOD_(ULONG, Release)() \
        { \
            return RuntimeClass::Release(); \
        } \
        STDMETHOD_(ULONG, AddRef)() \
        { \
            return RuntimeClass::AddRef(); \
        } \
        STDMETHOD(GetIids)(_Out_ ULONG *iidCount, \
            _When_(*iidCount == 0, _At_(*iids, _Post_null_)) \
            _When_(*iidCount > 0, _At_(*iids, _Post_notnull_)) \
            _Outptr_result_buffer_maybenull_(*iidCount) _Result_nullonfailure_ IID **iids) \
        { \
            return __super::GetIids(iidCount, iids); \
        } \
    private:

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace General {

    bool GeneralIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool GeneralIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool GeneralIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    template <typename T>
    void GeneralIntegrationTests::VerifyDeferredHeaderPresentersScenario(bool setHeaderBeforeApplyTemplate)
    {
        TestCleanupWrapper cleanup;

        T^ control = nullptr;

        RunOnUIThread([&]()
        {
            control = ref new T();
            TestServices::WindowHelper->WindowContent = TreeHelper::WrapInGrid(control);;

            if (setHeaderBeforeApplyTemplate)
            {
                VERIFY_ARE_EQUAL(FindNamedElements(control, L"HeaderContentPresenter").size(), 0U);
                control->Header = L"header";
                VERIFY_ARE_EQUAL(FindNamedElements(control, L"HeaderContentPresenter").size(), 0U);
            }
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            if (!setHeaderBeforeApplyTemplate)
            {
                VERIFY_ARE_EQUAL(FindNamedElements(control, L"HeaderContentPresenter").size(), 0U);
                control->Header = L"header";
            }

            std::vector<FrameworkElement^> header = FindNamedElements(control, L"HeaderContentPresenter");
            VERIFY_ARE_EQUAL(header.size(), 1U);

            xaml_controls::ContentPresenter^ cp = dynamic_cast<xaml_controls::ContentPresenter^>(header[0]);
            VERIFY_IS_NOT_NULL(cp);
            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(cp->Content), safe_cast<Platform::String^>(control->Header));
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    template <typename T>
    void GeneralIntegrationTests::VerifyDeferredHeaderPresentersScenario()
    {
        VerifyDeferredHeaderPresentersScenario<T>(true);
        VerifyDeferredHeaderPresentersScenario<T>(false);
    }

    void GeneralIntegrationTests::VerifyDeferredHeaderPresenters()
    {
        // HeaderContentPresenter in template is deferred.  Setting Header/HeaderTemplate property should realize it.
        VerifyDeferredHeaderPresentersScenario<xaml_controls::ComboBox>();
        VerifyDeferredHeaderPresentersScenario<xaml_controls::PasswordBox>();
        VerifyDeferredHeaderPresentersScenario<xaml_controls::RichEditBox>();
        VerifyDeferredHeaderPresentersScenario<xaml_controls::Slider>();
        VerifyDeferredHeaderPresentersScenario<xaml_controls::TextBox>();
        VerifyDeferredHeaderPresentersScenario<xaml_controls::ToggleSwitch>();
    }

    void GeneralIntegrationTests::DoesDefaultForegroundBrushMatchAppTheme()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::TextBlock^ textblock = nullptr;
        auto appTheme = xaml::ApplicationTheme::Dark;

        RunOnUIThread([&]()
        {
            appTheme = Application::Current->RequestedTheme;

            auto root = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Grid x:Name='grid' Background='{ThemeResource ApplicationPageBackgroundThemeBrush}'>"
                L"        <TextBlock Text='some text'/>"
                L"    </Grid>"
                L"    <TextBlock x:Name='textblock' Text='some other text'/>"
                L"</StackPanel>"));

            // To verify the fix , we make sure there is a text element in a sub-tree
            // with a theme that is different from the app theme.
            auto grid = safe_cast<xaml_controls::Grid^>(root->FindName(L"grid"));
            grid->RequestedTheme = (appTheme == xaml::ApplicationTheme::Dark ? xaml::ElementTheme::Light : xaml::ElementTheme::Dark);

            textblock = safe_cast<xaml_controls::TextBlock^>(root->FindName(L"textblock"));

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto foregroundBrush = safe_cast<xaml_media::SolidColorBrush^>(textblock->Foreground);

            wu::Color expectedColor = {255, 255, 255, 255 };
            if (appTheme == xaml::ApplicationTheme::Light)
            {
                expectedColor = { 255, 0, 0, 0 };
            }

            VERIFY_ARE_EQUAL(foregroundBrush->Color.A, expectedColor.A);
            VERIFY_ARE_EQUAL(foregroundBrush->Color.R, expectedColor.R);
            VERIFY_ARE_EQUAL(foregroundBrush->Color.G, expectedColor.G);
            VERIFY_ARE_EQUAL(foregroundBrush->Color.B, expectedColor.B);
        });
    }

    void GeneralIntegrationTests::VerifyThatSystemColorsCanBeOverridden()
    {
        TestCleanupWrapper cleanup;

        // This test verifies that it is possible for an app to replace the default set of Colors that are used by the framework controls.
        // The app provides a set of named Colors in Dark and Light ThemeDictionaries.
        // In order for this to work correctly, the resources must be added at app start-up time, before any UI has rendered.
        // Also, once the resources have been added, the change cannot be undone.
        // For this reason, this test method runs with IsolationLevel:Method.

        // This is important for the scenario where an app author wants to provide a palette containing only TV-Safe colors.

        // To test this scenario, we merge in custom sets of Colors for Dark and Light and then render a set of controls
        // We validate that the resulting UIElementTree only contains colors from the set we provided (plus the accent color).
        // We also test switching between Dark and Light theme to make sure that the tree gets updated appropriately.

        xaml_controls::StackPanel^ rootPanel;
        wfc::IVector<unsigned>^ lightColors;
        wfc::IVector<unsigned>^ darkColors;

        // We load Dark and Light ThemeDictionaries that contain overridden color resources.
        // The custom Dark palette consists of shades of Blue and Yellow.
        // The custom Light palette consists of shades of Teal and Red
        auto rd = safe_cast<xaml::ResourceDictionary^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SystemColorOverrideResourceDictionary.xaml"));

        RunOnUIThread([&]()
        {
            auto rdDark = safe_cast<xaml::ResourceDictionary^>(rd->ThemeDictionaries->Lookup(L"Dark"));
            auto rdLight = safe_cast<xaml::ResourceDictionary^>(rd->ThemeDictionaries->Lookup(L"Light"));

            lightColors = GetColorValuesFromResourceDictionary(rdLight);
            darkColors = GetColorValuesFromResourceDictionary(rdDark);

            xaml::Application::Current->Resources->MergedDictionaries->Append(rd);

            // We try out some sample Xaml containing a selection of elements. We don't try to exhaustively test every
            // control in every visual state.
            rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                                xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                                RequestedTheme="Dark" Background="Black" >
                        <Button Content="Button" />
                        <ToggleSwitch IsOn="True" Header="ToggleSwitch" />
                        <CheckBox IsChecked="True" Content="CheckBox" />
                        <RadioButton IsChecked="True" Content="RadioButton" />
                        <Slider Value="40" Minimum="0" Maximum="100" />
                        <HyperlinkButton Content="HyperlinkButton" />
                        <AppBarButton Icon="Accept" Content="Accept" />
                        <AppBarToggleButton Icon="Back" IsChecked="True" Content="Back" />
                        <DatePicker />
                        <TimePicker />
                        <ComboBox>
                            <ComboBoxItem Content="Item 1" />
                            <ComboBoxItem Content="Item 2" IsSelected="True" />
                        </ComboBox>
                        <TextBox Text="TextBox" />
                        <TextBlock Text="TextBlock" />
                        <PasswordBox Password="Password" />
                        <RichTextBlock >
                            <Paragraph>
                                Text in RichTextBlock
                                <Hyperlink NavigateUri="http://www.bing.com" >http://www.bing.com</Hyperlink>
                            </Paragraph>
                        </RichTextBlock>
                    </StackPanel>)"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate Dark Colors.");
        TestServices::Utilities->VerifyUIElementTreeContainsOnlyValidColors("Dark", darkColors);

        RunOnUIThread([&]()
        {
            rootPanel->RequestedTheme = xaml::ElementTheme::Light;
            rootPanel->Background = ref new xaml_media::SolidColorBrush(mu::Colors::White);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate Light Colors.");
        TestServices::Utilities->VerifyUIElementTreeContainsOnlyValidColors("Light", lightColors);
    }

    void GeneralIntegrationTests::VerifyCreationDoesNotQIOuter()
    {
        TestCleanupWrapper cleanup;

        class CountingInspectable : public wrl::RuntimeClass<IInspectable>
        {
            InspectableClassNoQueryInterface(L"CountingInspectable", BaseTrust);

        public:
            STDMETHOD(QueryInterface)(REFIID riid, _Outptr_ void **object)
            {
                m_queryCount++;
                return RuntimeClass::QueryInterface(riid, object);
            }

            int GetQueryInterfaceCallCount() const
            {
                return m_queryCount;
            }

        private:
            int m_queryCount{ 0 };
        };

        RunOnUIThread([&]()
        {
            wrl::ComPtr<CountingInspectable> countingOuter;
            VERIFY_SUCCEEDED(wrl::MakeAndInitialize<CountingInspectable>(&countingOuter));

            Platform::Object^ factory;
            Platform::Object^ inner;

            LOG_OUTPUT(L"Validate Button");
            THROW_IF_FAILED(wf::GetActivationFactory(wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.Controls.Button").Get(), reinterpret_cast<IInspectable**>(&factory)));
            safe_cast<xaml_controls::IButtonFactory^>(factory)->CreateInstance(reinterpret_cast<Platform::Object^>(static_cast<IInspectable*>(countingOuter.Get())), &inner);
            VERIFY_ARE_EQUAL(0, countingOuter.Get()->GetQueryInterfaceCallCount());

            LOG_OUTPUT(L"Validate ComboBox");
            THROW_IF_FAILED(wf::GetActivationFactory(wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.Controls.ComboBox").Get(), reinterpret_cast<IInspectable**>(&factory)));
            safe_cast<xaml_controls::IComboBoxFactory^>(factory)->CreateInstance(reinterpret_cast<Platform::Object^>(static_cast<IInspectable*>(countingOuter.Get())), &inner);
            VERIFY_ARE_EQUAL(0, countingOuter.Get()->GetQueryInterfaceCallCount());

            LOG_OUTPUT(L"Validate ComboBoxItem");
            THROW_IF_FAILED(wf::GetActivationFactory(wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.Controls.ComboBoxItem").Get(), reinterpret_cast<IInspectable**>(&factory)));
            safe_cast<xaml_controls::IComboBoxItemFactory^>(factory)->CreateInstance(reinterpret_cast<Platform::Object^>(static_cast<IInspectable*>(countingOuter.Get())), &inner);
            VERIFY_ARE_EQUAL(0, countingOuter.Get()->GetQueryInterfaceCallCount());

            LOG_OUTPUT(L"Validate ListBox");
            THROW_IF_FAILED(wf::GetActivationFactory(wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.Controls.ListBox").Get(), reinterpret_cast<IInspectable**>(&factory)));
            safe_cast<xaml_controls::IListBoxFactory^>(factory)->CreateInstance(reinterpret_cast<Platform::Object^>(static_cast<IInspectable*>(countingOuter.Get())), &inner);
            VERIFY_ARE_EQUAL(0, countingOuter.Get()->GetQueryInterfaceCallCount());

            LOG_OUTPUT(L"Validate ListView"); // Validates ListViewBase.
            THROW_IF_FAILED(wf::GetActivationFactory(wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.Controls.ListView").Get(), reinterpret_cast<IInspectable**>(&factory)));
            safe_cast<xaml_controls::IListViewFactory^>(factory)->CreateInstance(reinterpret_cast<Platform::Object^>(static_cast<IInspectable*>(countingOuter.Get())), &inner);
            VERIFY_ARE_EQUAL(0, countingOuter.Get()->GetQueryInterfaceCallCount());

            LOG_OUTPUT(L"Validate ListViewItem");
            THROW_IF_FAILED(wf::GetActivationFactory(wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.Controls.ListViewItem").Get(), reinterpret_cast<IInspectable**>(&factory)));
            safe_cast<xaml_controls::IListViewItemFactory^>(factory)->CreateInstance(reinterpret_cast<Platform::Object^>(static_cast<IInspectable*>(countingOuter.Get())), &inner);
            VERIFY_ARE_EQUAL(0, countingOuter.Get()->GetQueryInterfaceCallCount());

            LOG_OUTPUT(L"Validate Page");
            THROW_IF_FAILED(wf::GetActivationFactory(wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.Controls.Page").Get(), reinterpret_cast<IInspectable**>(&factory)));
            safe_cast<xaml_controls::IPageFactory^>(factory)->CreateInstance(reinterpret_cast<Platform::Object^>(static_cast<IInspectable*>(countingOuter.Get())), &inner);
            VERIFY_ARE_EQUAL(0, countingOuter.Get()->GetQueryInterfaceCallCount());

            LOG_OUTPUT(L"Validate RichEditBox");
            THROW_IF_FAILED(wf::GetActivationFactory(wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.Controls.RichEditBox").Get(), reinterpret_cast<IInspectable**>(&factory)));
            safe_cast<xaml_controls::IRichEditBoxFactory^>(factory)->CreateInstance(reinterpret_cast<Platform::Object^>(static_cast<IInspectable*>(countingOuter.Get())), &inner);
            VERIFY_ARE_EQUAL(0, countingOuter.Get()->GetQueryInterfaceCallCount());

            LOG_OUTPUT(L"Validate TextBox");
            THROW_IF_FAILED(wf::GetActivationFactory(wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.Controls.TextBox").Get(), reinterpret_cast<IInspectable**>(&factory)));
            safe_cast<xaml_controls::ITextBoxFactory^>(factory)->CreateInstance(reinterpret_cast<Platform::Object^>(static_cast<IInspectable*>(countingOuter.Get())), &inner);
            VERIFY_ARE_EQUAL(0, countingOuter.Get()->GetQueryInterfaceCallCount());

            LOG_OUTPUT(L"Validate ToolTip");
            THROW_IF_FAILED(wf::GetActivationFactory(wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.Controls.ToolTip").Get(), reinterpret_cast<IInspectable**>(&factory)));
            safe_cast<xaml_controls::IToolTipFactory^>(factory)->CreateInstance(reinterpret_cast<Platform::Object^>(static_cast<IInspectable*>(countingOuter.Get())), &inner);
            VERIFY_ARE_EQUAL(0, countingOuter.Get()->GetQueryInterfaceCallCount());
        });
    }

    wfc::IVector<unsigned>^ GeneralIntegrationTests::GetColorValuesFromResourceDictionary(xaml::ResourceDictionary^ resourceDictionary)
    {
        auto colors = ref new Platform::Collections::Vector<unsigned int>();

        auto iter = resourceDictionary->First();
        while (iter->HasCurrent)
        {
            auto color = safe_cast<wu::Color>(iter->Current->Value);
            unsigned colorAsInt = ((unsigned)color.A << 24) + ((unsigned)color.R << 16) + ((unsigned)color.G << 8) + color.B;
            colors->Append(colorAsInt);
            iter->MoveNext();
        }

        auto accentColor = TestServices::ThemingHelper->AccentColor;
        colors->Append(accentColor);

        return colors;
    }

    void GeneralIntegrationTests::CanParseGenericXaml()
    {
        TestCleanupWrapper cleanup;
        // Verify that we can load the contents of generic.xaml with XamlReader::Load.
        // Ordinarily, generic.xaml is loaded by the runtime indirectly via the xbf files. However the Visual Studio designer directly
        // loads generic.xaml. This test validates that scenario.

        auto dictionaryFromGenericXaml = safe_cast<xaml::ResourceDictionary^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"generic.xaml"));
        VERIFY_IS_NOT_NULL(dictionaryFromGenericXaml);

        RunOnUIThread([&]()
        {
            VERIFY_IS_NOT_NULL(dictionaryFromGenericXaml->Lookup(L"ApplicationPageBackgroundThemeBrush"));
        });
    }

    void GeneralIntegrationTests::VerifyNotMissingKeyForThemeDictionariesInGenericXaml()
    {
        TestCleanupWrapper cleanup;

        auto file = GetResourcesPath() + L"generic.xaml";
        LOG_OUTPUT(L"Load %s", file->Data());
        auto rd = safe_cast<xaml::ResourceDictionary^>(LoadXamlFileOnUIThread(file));
        VERIFY_IS_NOT_NULL(rd);

        RunOnUIThread([&]()
        {
            auto rdDark = safe_cast<xaml::ResourceDictionary^>(rd->ThemeDictionaries->Lookup(L"Default"));
            auto rdLight = safe_cast<xaml::ResourceDictionary^>(rd->ThemeDictionaries->Lookup(L"Light"));
            auto rdHC = safe_cast<xaml::ResourceDictionary^>(rd->ThemeDictionaries->Lookup(L"HighContrast"));
            VERIFY_IS_NOT_NULL(rdDark);
            VERIFY_IS_NOT_NULL(rdLight);
            VERIFY_IS_NOT_NULL(rdHC);

            std::map<std::wstring, int> dict;
            std::vector<xaml::ResourceDictionary^> rds = { rdDark, rdLight, rdHC};

            for (auto const& themeDict: rds)
            {
                auto iter = themeDict->First();
                while (iter->HasCurrent)
                {
                    std::wstring key(safe_cast<String^>(iter->Current->Key)->Data());
                    dict[key]++;
                    iter->MoveNext();
                }
            }

            for (auto const& item : dict)
            {
                if (item.second != 3)
                {
                    LOG_OUTPUT(L"Missing Key %s in dark, light or HC", static_cast<LPCWSTR>(item.first.c_str()));
                    VERIFY_ARE_EQUAL(item.second, 3);
                }
            }
        });
    }

    // Validates the default focus border properties based on the app and element themes.
    void GeneralIntegrationTests::VerifyFocusVisualDefaultValues()
    {
        xaml::ApplicationTheme originalTheme;
        RunOnUIThread([&]()
        {
            originalTheme = Application::Current->RequestedTheme;
        });
        const xaml::ApplicationTheme otherTheme = (originalTheme == xaml::ApplicationTheme::Dark ? xaml::ApplicationTheme::Light : xaml::ApplicationTheme::Dark);

        TestCleanupWrapper cleanup([&]
        {
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Restoring original theme.");
                TestServices::ThemingHelper->SetApplicationRequestedTheme(originalTheme);
            });
            TestServices::WindowHelper->WaitForIdle();
        });

        auto radioButtonRequestedTheme = xaml::ElementTheme::Dark;
        xaml_controls::StackPanel^ stackPanel = nullptr;
        xaml_controls::RadioButton^ radioButton = nullptr;
        xaml_primitives::ToggleButton^ toggleButton = nullptr;
        xaml_shapes::Rectangle^ rectangle = nullptr;

        RunOnUIThread([&]()
        {
            stackPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <RadioButton x:Name='radioButton'/>"
                L"    <ToggleButton x:Name='toggleButton'/>"
                L"    <Rectangle x:Name='rectangle'/>"
                L"</StackPanel>"));

            radioButton = safe_cast<xaml_controls::RadioButton^>(stackPanel->FindName(L"radioButton"));
            toggleButton = safe_cast<xaml_primitives::ToggleButton^>(stackPanel->FindName(L"toggleButton"));
            rectangle = safe_cast<xaml_shapes::Rectangle^>(stackPanel->FindName(L"rectangle"));

            TestServices::WindowHelper->WindowContent = stackPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            wu::Color borderColor = safe_cast<xaml_media::SolidColorBrush^>(rectangle->FocusVisualSecondaryBrush)->Color;
            wu::Color expectedColor = { 228, 255, 255, 255 };
            if (originalTheme == xaml::ApplicationTheme::Dark)
            {
                expectedColor = { 179, 0, 0, 0 };
            }
            VERIFY_ARE_EQUAL(borderColor.A, expectedColor.A);
            VERIFY_ARE_EQUAL(borderColor.R, expectedColor.R);
            VERIFY_ARE_EQUAL(borderColor.G, expectedColor.G);
            VERIFY_ARE_EQUAL(borderColor.B, expectedColor.B);

            borderColor = safe_cast<xaml_media::SolidColorBrush^>(rectangle->FocusVisualPrimaryBrush)->Color;
            expectedColor = { 179, 0, 0, 0 };
            if (originalTheme == xaml::ApplicationTheme::Dark)
            {
                expectedColor = { 255, 255, 255, 255 };
            }

            VERIFY_ARE_EQUAL(borderColor.A, expectedColor.A);
            VERIFY_ARE_EQUAL(borderColor.R, expectedColor.R);
            VERIFY_ARE_EQUAL(borderColor.G, expectedColor.G);
            VERIFY_ARE_EQUAL(borderColor.B, expectedColor.B);

            borderColor = safe_cast<xaml_media::SolidColorBrush^>(radioButton->FocusVisualSecondaryBrush)->Color;
            expectedColor = { 228, 255, 255, 255 };
            if (originalTheme == xaml::ApplicationTheme::Dark)
            {
                expectedColor = { 179, 0, 0, 0 };
            }
            VERIFY_ARE_EQUAL(borderColor.A, expectedColor.A);
            VERIFY_ARE_EQUAL(borderColor.R, expectedColor.R);
            VERIFY_ARE_EQUAL(borderColor.G, expectedColor.G);
            VERIFY_ARE_EQUAL(borderColor.B, expectedColor.B);

            borderColor = safe_cast<xaml_media::SolidColorBrush^>(radioButton->FocusVisualPrimaryBrush)->Color;
            expectedColor = { 179, 0, 0, 0 };
            if (originalTheme == xaml::ApplicationTheme::Dark)
            {
                expectedColor = { 255, 255, 255, 255 };
            }

            VERIFY_ARE_EQUAL(borderColor.A, expectedColor.A);
            VERIFY_ARE_EQUAL(borderColor.R, expectedColor.R);
            VERIFY_ARE_EQUAL(borderColor.G, expectedColor.G);
            VERIFY_ARE_EQUAL(borderColor.B, expectedColor.B);

            VERIFY_ARE_EQUAL(rectangle->FocusVisualMargin, xaml::Thickness({ 0,0,0,0 }));
            VERIFY_ARE_EQUAL(radioButton->FocusVisualMargin, xaml::Thickness({ -7, -3, -7, -3 }));
            VERIFY_ARE_EQUAL(toggleButton->FocusVisualMargin, xaml::Thickness({ -3,-3,-3,-3 }));

            VERIFY_ARE_EQUAL(rectangle->FocusVisualSecondaryThickness, xaml::Thickness({ 1,1,1,1 }));
            VERIFY_ARE_EQUAL(rectangle->FocusVisualPrimaryThickness, xaml::Thickness({ 2,2,2,2 }));
            VERIFY_ARE_EQUAL(radioButton->FocusVisualSecondaryThickness, xaml::Thickness({ 1,1,1,1 }));
            VERIFY_ARE_EQUAL(radioButton->FocusVisualPrimaryThickness, xaml::Thickness({ 2,2,2,2 }));
            VERIFY_ARE_EQUAL(toggleButton->FocusVisualSecondaryThickness, xaml::Thickness({ 1,1,1,1 }));
            VERIFY_ARE_EQUAL(toggleButton->FocusVisualPrimaryThickness, xaml::Thickness({ 2,2,2,2 }));
        });

        RunOnUIThread([&]()
        {
            radioButtonRequestedTheme = (originalTheme == xaml::ApplicationTheme::Dark ? xaml::ElementTheme::Dark : xaml::ElementTheme::Light);
            TestServices::ThemingHelper->SetApplicationRequestedTheme(otherTheme);
            radioButton->RequestedTheme = radioButtonRequestedTheme;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            wu::Color borderColor = safe_cast<xaml_media::SolidColorBrush^>(rectangle->FocusVisualSecondaryBrush)->Color;
            wu::Color expectedColor = { 179, 255, 255, 255 };
            if (otherTheme == xaml::ApplicationTheme::Dark)
            {
                expectedColor = { 228, 0, 0, 0 };
            }
            VERIFY_ARE_EQUAL(borderColor.A, expectedColor.A);
            VERIFY_ARE_EQUAL(borderColor.R, expectedColor.R);
            VERIFY_ARE_EQUAL(borderColor.G, expectedColor.G);
            VERIFY_ARE_EQUAL(borderColor.B, expectedColor.B);

            borderColor = safe_cast<xaml_media::SolidColorBrush^>(rectangle->FocusVisualPrimaryBrush)->Color;
            expectedColor = { 228, 0, 0, 0 };
            if (otherTheme == xaml::ApplicationTheme::Dark)
            {
                expectedColor = { 179, 255, 255, 255 };
            }

            VERIFY_ARE_EQUAL(borderColor.A, expectedColor.A);
            VERIFY_ARE_EQUAL(borderColor.R, expectedColor.R);
            VERIFY_ARE_EQUAL(borderColor.G, expectedColor.G);
            VERIFY_ARE_EQUAL(borderColor.B, expectedColor.B);


            borderColor = safe_cast<xaml_media::SolidColorBrush^>(radioButton->FocusVisualSecondaryBrush)->Color;
            expectedColor = { 228, 255, 255, 255 };
            if (radioButtonRequestedTheme == xaml::ElementTheme::Dark)
            {
                expectedColor = { 179, 0, 0, 0 };
            }
            VERIFY_ARE_EQUAL(borderColor.A, expectedColor.A);
            VERIFY_ARE_EQUAL(borderColor.R, expectedColor.R);
            VERIFY_ARE_EQUAL(borderColor.G, expectedColor.G);
            VERIFY_ARE_EQUAL(borderColor.B, expectedColor.B);

            borderColor = safe_cast<xaml_media::SolidColorBrush^>(radioButton->FocusVisualPrimaryBrush)->Color;
            expectedColor = { 179, 0, 0, 0 };
            if (radioButtonRequestedTheme == xaml::ElementTheme::Dark)
            {
                expectedColor = { 255, 255, 255, 255 };
            }

            VERIFY_ARE_EQUAL(borderColor.A, expectedColor.A);
            VERIFY_ARE_EQUAL(borderColor.R, expectedColor.R);
            VERIFY_ARE_EQUAL(borderColor.G, expectedColor.G);
            VERIFY_ARE_EQUAL(borderColor.B, expectedColor.B);
        });
    }

    // Validates the ability to set and get focus border properties.
    void GeneralIntegrationTests::SetFocusVisualValues()
    {
        TestCleanupWrapper cleanup;

        auto requestedTheme = xaml::ApplicationTheme::Dark;
        xaml_controls::StackPanel^ stackPanel = nullptr;
        xaml_controls::RadioButton^ radioButton = nullptr;
        xaml_primitives::ToggleButton^ toggleButton = nullptr;
        xaml_shapes::Rectangle^ rectangle = nullptr;

        RunOnUIThread([&]()
        {
            requestedTheme = Application::Current->RequestedTheme;

            stackPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <RadioButton x:Name='radioButton'/>"
                L"    <ToggleButton x:Name='toggleButton'/>"
                L"    <Rectangle x:Name='rectangle'/>"
                L"</StackPanel>"));

            radioButton = safe_cast<xaml_controls::RadioButton^>(stackPanel->FindName(L"radioButton"));
            toggleButton = safe_cast<xaml_primitives::ToggleButton^>(stackPanel->FindName(L"toggleButton"));
            rectangle = safe_cast<xaml_shapes::Rectangle^>(stackPanel->FindName(L"rectangle"));

            TestServices::WindowHelper->WindowContent = stackPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            wu::Color expectedColor = { 1, 2, 3, 4 };

            // Read/write brushes properties
            safe_cast<xaml_media::SolidColorBrush^>(rectangle->FocusVisualSecondaryBrush)->Color = expectedColor;
            wu::Color borderColor = safe_cast<xaml_media::SolidColorBrush^>(rectangle->FocusVisualSecondaryBrush)->Color;

            VERIFY_ARE_EQUAL(borderColor.A, expectedColor.A);
            VERIFY_ARE_EQUAL(borderColor.R, expectedColor.R);
            VERIFY_ARE_EQUAL(borderColor.G, expectedColor.G);
            VERIFY_ARE_EQUAL(borderColor.B, expectedColor.B);

            safe_cast<xaml_media::SolidColorBrush^>(rectangle->FocusVisualPrimaryBrush)->Color = expectedColor;
            borderColor = safe_cast<xaml_media::SolidColorBrush^>(rectangle->FocusVisualPrimaryBrush)->Color;

            VERIFY_ARE_EQUAL(borderColor.A, expectedColor.A);
            VERIFY_ARE_EQUAL(borderColor.R, expectedColor.R);
            VERIFY_ARE_EQUAL(borderColor.G, expectedColor.G);
            VERIFY_ARE_EQUAL(borderColor.B, expectedColor.B);


            safe_cast<xaml_media::SolidColorBrush^>(radioButton->FocusVisualSecondaryBrush)->Color = expectedColor;
            borderColor = safe_cast<xaml_media::SolidColorBrush^>(radioButton->FocusVisualSecondaryBrush)->Color;

            VERIFY_ARE_EQUAL(borderColor.A, expectedColor.A);
            VERIFY_ARE_EQUAL(borderColor.R, expectedColor.R);
            VERIFY_ARE_EQUAL(borderColor.G, expectedColor.G);
            VERIFY_ARE_EQUAL(borderColor.B, expectedColor.B);

            safe_cast<xaml_media::SolidColorBrush^>(radioButton->FocusVisualPrimaryBrush)->Color = expectedColor;
            borderColor = safe_cast<xaml_media::SolidColorBrush^>(radioButton->FocusVisualPrimaryBrush)->Color;

            VERIFY_ARE_EQUAL(borderColor.A, expectedColor.A);
            VERIFY_ARE_EQUAL(borderColor.R, expectedColor.R);
            VERIFY_ARE_EQUAL(borderColor.G, expectedColor.G);
            VERIFY_ARE_EQUAL(borderColor.B, expectedColor.B);


            // Read/write thickness properties
            rectangle->FocusVisualMargin = xaml::Thickness({ 6, 7, 8, 9 });
            VERIFY_ARE_EQUAL(rectangle->FocusVisualMargin, xaml::Thickness({ 6, 7, 8, 9 }));

            radioButton->FocusVisualMargin = xaml::Thickness({ 5, 6, 7, 8 });
            VERIFY_ARE_EQUAL(radioButton->FocusVisualMargin, xaml::Thickness({ 5, 6, 7, 8 }));

            toggleButton->FocusVisualMargin = xaml::Thickness({ 4, 5, 6, 7 });
            VERIFY_ARE_EQUAL(toggleButton->FocusVisualMargin, xaml::Thickness({ 4, 5, 6, 7 }));

            rectangle->FocusVisualSecondaryThickness = xaml::Thickness({ 1, 2, 3, 4 });
            VERIFY_ARE_EQUAL(rectangle->FocusVisualSecondaryThickness, xaml::Thickness({ 1, 2, 3, 4 }));

            rectangle->FocusVisualPrimaryThickness = xaml::Thickness({ 4, 3, 2, 1 });
            VERIFY_ARE_EQUAL(rectangle->FocusVisualPrimaryThickness, xaml::Thickness({ 4, 3, 2, 1 }));

            radioButton->FocusVisualSecondaryThickness = xaml::Thickness({ 10, 11, 12, 13 });
            VERIFY_ARE_EQUAL(radioButton->FocusVisualSecondaryThickness, xaml::Thickness({ 10, 11, 12, 13 }));

            radioButton->FocusVisualPrimaryThickness = xaml::Thickness({ 13, 12, 11, 10 });
            VERIFY_ARE_EQUAL(radioButton->FocusVisualPrimaryThickness, xaml::Thickness({ 13, 12, 11, 10 }));

            // Read/write Application.FocusVisualKind property
            Application::Current->FocusVisualKind = xaml::FocusVisualKind::HighVisibility;
            VERIFY_ARE_EQUAL(Application::Current->FocusVisualKind, xaml::FocusVisualKind::HighVisibility);
            Application::Current->FocusVisualKind = xaml::FocusVisualKind::DottedLine;
            VERIFY_ARE_EQUAL(Application::Current->FocusVisualKind, xaml::FocusVisualKind::DottedLine);
            Application::Current->FocusVisualKind = xaml::FocusVisualKind::HighVisibility;
            VERIFY_ARE_EQUAL(Application::Current->FocusVisualKind, xaml::FocusVisualKind::HighVisibility);
        });
    }

#if WI_IS_FEATURE_PRESENT(Feature_WUXCPreviewTypes)
    void GeneralIntegrationTests::PanelProtectedBorderAndCornerProperties()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(600, 600));
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        // Tests the rendering of:
        //   Panel::BorderBrushProtected
        //   Panel::BorderThicknessProtected
        //   Panel::CornerRadiusProtected
        //
        // Since these are protected properties, we create a derrived TestPanel to expose the properties publicly.
        TestTypes::TestPanel^ testPanel;

        RunOnUIThread([&]()
        {
            testPanel = ref new TestTypes::TestPanel();
            testPanel->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Green);
            testPanel->Width = 100;
            testPanel->Height = 100;

            testPanel->BorderThickness = xaml::Thickness(20);
            testPanel->BorderBrush = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
            testPanel->CornerRadius = xaml::CornerRadius(5);

            TestServices::WindowHelper->WindowContent = testPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

        // Verify that changing the properties causes the panel to re-render correctly.
        LOG_OUTPUT(L"Changing border properties: Grid.BorderThickness and Grid.Padding.");
        RunOnUIThread([&]()
        {
            testPanel->BorderThickness = xaml::Thickness(20, 5, 0, 0);
            testPanel->BorderBrush = ref new xaml_media::SolidColorBrush(mu::Colors::Yellow);
            testPanel->CornerRadius = xaml::CornerRadius(7);
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");
    }
#endif

    void GeneralIntegrationTests::VerifyPseudoLocalizedControls()
    {
        VerifyPseudoLocalizedControlsPrivate();
    }

    void GeneralIntegrationTests::VerifyPseudoLocalizedControlsForNonDesktop()
    {
        VerifyPseudoLocalizedControlsPrivate();
    }

    void GeneralIntegrationTests::VerifyPseudoLocalizedControlsPrivate()
    {
        TestCleanupWrapper cleanup;
        EnablePseudoLoc pseudoLoc;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::StackPanel();

            rootPanel->Children->Append(ref new xaml_controls::AppBar());
            rootPanel->Children->Append(ref new xaml_controls::AppBarButton());
            rootPanel->Children->Append(ref new xaml_controls::AppBarElementContainer());
            rootPanel->Children->Append(ref new xaml_controls::AppBarSeparator());
            rootPanel->Children->Append(ref new xaml_controls::AppBarToggleButton());
            rootPanel->Children->Append(ref new xaml_controls::AutoSuggestBox());
            rootPanel->Children->Append(ref new xaml_controls::Button());
            rootPanel->Children->Append(ref new xaml_controls::CalendarDatePicker());
            rootPanel->Children->Append(ref new xaml_controls::CalendarView());
            rootPanel->Children->Append(ref new xaml_controls::CheckBox());

            auto comboBox = ref new xaml_controls::ComboBox();
            comboBox->Items->Append(ref new xaml_controls::ComboBoxItem());
            rootPanel->Children->Append(comboBox);

            auto commandBar = ref new xaml_controls::CommandBar();
            commandBar->PrimaryCommands->Append(ref new xaml_controls::AppBarButton());
            commandBar->SecondaryCommands->Append(ref new xaml_controls::AppBarButton());
            rootPanel->Children->Append(commandBar);

            rootPanel->Children->Append(ref new xaml_controls::DatePicker());
            rootPanel->Children->Append(ref new xaml_controls::FlipView());

            auto hub = ref new xaml_controls::Hub();
            hub->Sections->Append(ref new xaml_controls::HubSection());
            rootPanel->Children->Append(hub);

            auto listBox = ref new xaml_controls::ListBox();
            listBox->Items->Append(ref new xaml_controls::ListBoxItem());
            rootPanel->Children->Append(listBox);

            auto listView = ref new xaml_controls::ListView();
            listView->Items->Append(ref new xaml_controls::ListViewItem());
            rootPanel->Children->Append(listView);

            rootPanel->Children->Append(ref new xaml_controls::PasswordBox());

            auto pivot = ref new xaml_controls::Pivot();
            pivot->Items->Append(ref new xaml_controls::PivotItem());
            rootPanel->Children->Append(pivot);

            rootPanel->Children->Append(ref new xaml_controls::RadioButton());
            //rootPanel->Children->Append(ref new xaml_controls::RatingControl());
            rootPanel->Children->Append(ref new xaml_controls::RichEditBox());
            rootPanel->Children->Append(ref new xaml_controls::RichTextBlock());
            rootPanel->Children->Append(ref new xaml_controls::ScrollViewer());
            rootPanel->Children->Append(ref new xaml_controls::SemanticZoom());
            rootPanel->Children->Append(ref new xaml_controls::Slider());
            rootPanel->Children->Append(ref new xaml_controls::SplitView());
            rootPanel->Children->Append(ref new xaml_controls::TextBlock());
            rootPanel->Children->Append(ref new xaml_controls::TextBox());
            rootPanel->Children->Append(ref new xaml_controls::TimePicker());
            rootPanel->Children->Append(ref new xaml_controls::ToggleSwitch());
            rootPanel->Children->Append(ref new xaml_controls::UserControl());

            loadedRegistration.Attach(rootPanel, [loadedEvent]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    //
    // Utilities
    //
    Platform::String^ GeneralIntegrationTests::GetResourcesPath() const
    {
        return GetPackageFolder() + L"resources\\native\\controls\\general\\";
    }

    std::vector<FrameworkElement^> GeneralIntegrationTests::FindNamedElements(DependencyObject^ parent, Platform::String^ name)
    {
        std::vector<FrameworkElement^> results;

        if (parent != nullptr)
        {
            FindNamedElementsInternal(parent, name, results);
        }

        return results;
    }

    void GeneralIntegrationTests::FindNamedElementsInternal(DependencyObject^ parent, Platform::String^ name, std::vector<FrameworkElement^>& results)
    {
        FrameworkElement^ parentAsFE = dynamic_cast<FrameworkElement^>(parent);

        if (parentAsFE != nullptr &&
            parentAsFE->Name == name)
        {
            results.push_back(parentAsFE);
        }

        for (int i = 0; i < xaml_media::VisualTreeHelper::GetChildrenCount(parent); ++i)
        {
            FindNamedElementsInternal(
                xaml_media::VisualTreeHelper::GetChild(parent, i),
                name,
                results);
        }
    }
} } } } } } // Microsoft::UI::Xaml::Tests::Controls::General
