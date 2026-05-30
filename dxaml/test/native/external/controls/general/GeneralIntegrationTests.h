// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <FeatureFlags.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace General {

    class GeneralIntegrationTests : public WEX::TestClass<GeneralIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(GeneralIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"ab3da967-c6a7-4e5b-b3c4-c53c6c46175c;a69ddfa4-5142-4bed-887d-6d0ca14a3473;cc5953d4-6553-42e5-8c02-80720aa9d842")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(VerifyDeferredHeaderPresenters)
            TEST_METHOD_PROPERTY(L"Description", L"Verify HeaderContentPresenters are deferred in controls and setting Header will realize them")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesDefaultForegroundBrushMatchAppTheme)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the default foregroung brush matches the app theme.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFocusVisualDefaultValues)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the default focus border properties based on the app and element themes.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetFocusVisualValues)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ability to set and get focus border properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyThatSystemColorsCanBeOverridden)
            TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method") // The merged resources will only have an effect if they are added during startup before any UI has been rendered.
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the Color Ramp palette can be replaced at the app level via ThemeDictionaries")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCreationDoesNotQIOuter)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that derived class is not QI'd during creation")
        END_TEST_METHOD()

#if WI_IS_FEATURE_PRESENT(Feature_WUXCPreviewTypes)
        BEGIN_TEST_METHOD(PanelProtectedBorderAndCornerProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Tests rendering of Panel properties: BorderBrushProtected, BorderThicknessProtected, CornerRadiusProtected")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()
#endif

        BEGIN_TEST_METHOD(CanParseGenericXaml)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can correctly parse generic.xaml using XamlReader::Load")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyNotMissingKeyForThemeDictionariesInGenericXaml)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that every key defined in ThemeDictionary should exist in Dark, HC and Light theme")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()
        //
        // Platform:Desktop
        //

        BEGIN_TEST_METHOD(VerifyPseudoLocalizedControls)
            TEST_METHOD_PROPERTY(L"Description", L"Loads a large collection of controls into a single stack panel to ensure that none of them cause obvious problems when localized.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        //
        // Platform:OneCore and WindowsCore
        //

        BEGIN_TEST_METHOD(VerifyPseudoLocalizedControlsForNonDesktop)
            TEST_METHOD_PROPERTY(L"Description", L"Loads a large collection of controls into a single stack panel to ensure that none of them cause obvious problems when localized.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"Desktop")
        END_TEST_METHOD()            

    private:
        inline Platform::String^ GetResourcesPath() const;

        void ValidationWorker(std::function<void(Platform::String^)> validator);
        void VerifyPseudoLocalizedControlsPrivate();

        std::vector<FrameworkElement^> FindNamedElements(DependencyObject^ parent, Platform::String^ name);
        void FindNamedElementsInternal(DependencyObject^ parent, Platform::String^ name, std::vector<FrameworkElement^>& results);

        template <typename T> void VerifyDeferredHeaderPresentersScenario();
        template <typename T> void VerifyDeferredHeaderPresentersScenario(bool setHeaderBeforeApplyTemplate);

        wfc::IVector<unsigned>^ GetColorValuesFromResourceDictionary(xaml::ResourceDictionary^ resourceDictionary);
    };

    

} } } } } }

#if WI_IS_FEATURE_PRESENT(Feature_WUXCPreviewTypes)
namespace TestTypes
{
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    public ref class TestPanel : Microsoft::UI::Xaml::Controls::Panel
    {
    public:
        property xaml_media::Brush^ BorderBrush
        {
            xaml_media::Brush^ get()
            {
                return BorderBrushProtected;
            }
            void set(xaml_media::Brush^ value)
            {
                BorderBrushProtected = value;
            }
        }

        property xaml::Thickness BorderThickness
        {
            xaml::Thickness get()
            {
                return BorderThicknessProtected;
            }
            void set(xaml::Thickness value)
            {
                BorderThicknessProtected = value;
            }
        }

        property xaml::CornerRadius CornerRadius
        {
            xaml::CornerRadius get()
            {
                return CornerRadiusProtected;
            }
            void set(xaml::CornerRadius value)
            {
                CornerRadiusProtected = value;
            }
        }
    };
}
#endif

