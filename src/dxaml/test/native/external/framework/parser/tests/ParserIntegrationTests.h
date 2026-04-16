// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlMetadataProviderOverrider.h>
#include <vector>
#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Parser {

    class ParserIntegrationTests
    {
    public:
        BEGIN_TEST_CLASS(ParserIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)

        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanControlTemplateNameHideVisualStateGroups)
            TEST_METHOD_PROPERTY(L"Description",
                L"Check that VisualStateGroup names defined in a ControlTemplate are not visible at the control level")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanGoToVisualStates)
            TEST_METHOD_PROPERTY(L"Description",
                L"Check that we can go to VisualStates")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UserControlNamescopeTest)
            TEST_METHOD_PROPERTY(L"Description", L"Check that named elements within a UserControl are accessible through FindName.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UserControlUsageNamescopeTest)
            TEST_METHOD_PROPERTY(L"Description", L"Check that named elements within a UserControl with a usage name are accessible through FindName.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UserControlNamescopeWithoutReferencedResourcesTest)
            TEST_METHOD_PROPERTY(L"Description", L"Check that named elements within a UserControl are accessible through FindName and test namescope membership.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UsageScopeAndControlTemplateTest)
            TEST_METHOD_PROPERTY(L"Description", L"Check that named elements within a UserControl and ControlTemplate are accessible through FindName and test namescope membership.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PrivateNamescopeCreatedByXamlReaderTest)
            TEST_METHOD_PROPERTY(L"Description", L"Check that named elements within a XamlReader tree scope are accessible through FindName and test namescope membership.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DuplicateUserControlAndTemplateNamesTest)
            TEST_METHOD_PROPERTY(L"Description", L"Check that the same name on element in UserControl and Template namescope do not collide.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(AddingElementsToPanelTest)
            TEST_METHOD_PROPERTY(L"Description", L"Check that the names of elements added dynamically to a stack panel are registered.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanActivateBuiltinEnum)
            TEST_METHOD_PROPERTY(L"Description", L"Check that the parser can activate built-in enums.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanAssignFontWeightToObjectProperty)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanParseEmptyStringForDoubleProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Check that the parser handles empty strings when assigning Double properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetPropertyPathOnCustomDP)
            TEST_METHOD_PROPERTY(L"Description", L"Check that the parser can set a property path on a custom DP.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetBrushOnCustomDP)
            TEST_METHOD_PROPERTY(L"Description", L"Check that the parser can set a brush on a custom DP.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanNestControlTemplateInsideVsmTest)
            TEST_METHOD_PROPERTY(L"Description", L"Check that we can nest a control template inside a VSM.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanGetIntOutOfParsedItemCollection)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanParsePointRectSize)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetApplicationRequestedTheme)
            TEST_METHOD_PROPERTY(L"Description", L"Check that we can set Application.RequestedTheme.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyThrowsExceptionOnInvalidLoadComponent)
            TEST_METHOD_PROPERTY(L"Description", L"Verify XamlParseException is thrown when invalid LoadComponent resource is passed.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyThrowsExceptionOnInvalidLoadFromSource)
            TEST_METHOD_PROPERTY(L"Description", L"Verify XamlParseException is thrown when invalid Source property resource is provided for MergedDictionary.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySystemColorBrushCanBeOverridenInApplicationResourceDictionary)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetUriPropertyOnNonFrameworkElementObject)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetSimpleProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that values can be set on simple properties through markup. Under a Velocity key because the only public simple properties are currently under Velocity.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ApplicationLoadComponentExpandsTemplatesUnderDesigner)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that Application.LoadComponent expands templates (for validation purposes) when the designer is active.")
            TEST_METHOD_PROPERTY(L"UAP:AppXManifest", L"AppxManifest.DesignMode.xml")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ResourceDictionary_SourceExpandsTemplatesUnderDesigner)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that loading markup via ResourceDictionary.Source expands templates (for validation purposes) when the designer is active.")
            TEST_METHOD_PROPERTY(L"UAP:AppXManifest", L"AppxManifest.DesignMode.xml")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanDefineNamespaceOnMergedDictionary)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that merged dictionaries can have custom XML namespace mappings.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ResourceAliasesInTemplates)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that specifying a Static/ThemeResource alias inside a template works as expected")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySetPropertyOnBaseClass)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that XBF generation doesn't fail when property is set on a base class that exists "
                L"in a namespace that doesn't have an xmlns")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCDATAHandling)
            TEST_METHOD_PROPERTY(L"Description", 
                L"Verify that CDATA sections in markup are treated as text and not ignored")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCollectionInitializationSyntax)
        TEST_METHOD_PROPERTY(L"Description",
            L"Verify collection is created with collection initialization syntax ")
        END_TEST_METHOD()

    private:
        Platform::String^ m_VsmNameScopingXaml;
        struct VsmTestStruct
        {
            Microsoft::UI::Xaml::Controls::Panel^ hostPanel;
            Microsoft::UI::Xaml::Controls::Button^ buttonWithGroupsAndVisualStates;
            Microsoft::UI::Xaml::Controls::Button^ buttonWithGroupsAndNoVisualStates;
            Microsoft::UI::Xaml::Controls::Button^ buttonWithNoGroupsAndNoVisualStates;
            std::vector<Microsoft::UI::Xaml::Controls::Control^> controlsUnderTest;
        };
        VsmTestStruct LoadVsmTestElements();
        void LoadXamlComponent(DependencyObject^ rootObject, Platform::String^ scenarioName);

        bool CompareFloatsWithEpsilon(float expected, float actual, float epsilon = 0.01f) const
        {
            bool areEqual = (expected - actual < epsilon) && (actual - expected < epsilon);

            if (!areEqual)
            {
                LOG_OUTPUT(L"Expected: %f; Actual: %f", expected, actual);
            }

            return areEqual;
        }

        DependencyProperty^ dpSetPropertyPathOnCustomDP_DP;
        DependencyProperty^ dpSetBrushOnCustomDP_DP;
        DependencyProperty^ dpCanSetUriPropertyOnNonFrameworkElementObject_DP;
    };
} } } } } }
