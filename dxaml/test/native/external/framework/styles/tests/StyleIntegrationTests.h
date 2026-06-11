// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <CustomMetadataRegistrar.h>
#include <memory>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Styles {

    class StyleIntegrationTests
    {

    public:
        StyleIntegrationTests();

        BEGIN_TEST_CLASS(StyleIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"f62323d4-fd46-4c98-aa85-334db95ba8f6;0a9cdf5f-1e1b-4b1d-9659-b354bf5f4ca6")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)

        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanStyleSetPropertiesOnCustomControl)
            TEST_METHOD_PROPERTY(L"Description",
                L"Check that properties on a custom control can be set via styles")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanStyleSetPropertiesOnMultipleCustomControls)
            TEST_METHOD_PROPERTY(L"Description",
                L"Check that properties on multiple custom controls can be set via styles")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanStyleSetPropertiesOnMultipleDifferentControls)
            TEST_METHOD_PROPERTY(L"Description",
                L"Check that properties on multiple different controls can be set via styles")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanStyleSetPropertiesOnCustomUserControl)
            TEST_METHOD_PROPERTY(L"Description",
                L"Check that properties on a custom user control can be set via styles")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanStyleSetControlTemplateUsingStaticResource)
            TEST_METHOD_PROPERTY(L"Description",
                L"Check that a control template can be set via a style in a static resource")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanStyleSetControlTemplateDirectlyExplicit)
            TEST_METHOD_PROPERTY(L"Description",
                L"Check that a control template can be set via an explicit style directly")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanStyleSetControlTemplateDirectlyImplicit)
            TEST_METHOD_PROPERTY(L"Description",
                L"Check that a control template can be set via an implicit style directly")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanStyleSetterDetectMissingValue)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that if a style setter doesn't have a defined value, a XAML parse exception is thrown")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanStyleSetterDetectMissingProperty)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that if a style setter doesn't have a defined property, a XAML parse exception is thrown")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetStyleDirectlyInsideElement)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that we can set a style directly inside an element")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleAvailableInCompatModeEvenWhenNotLive)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that a style is available in compat mode even when not live")
            // This test expects style to be applied during CreationComplete to test the old mode. Disable the optimization.
            TEST_METHOD_PROPERTY(L"Data:PerfOptIn", L"{false}")
            TEST_METHOD_PROPERTY(L"Data:XamlOptionalChanges", L"{DelayApplyStyleOptimization:false}")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleNotAvailableInNewPerfOptInBehaviorWhenNotLive)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that a style is not available in the new PerfOptIn behavior when not live")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetStyleDirectlyInsideElement_TargetPropertyPath)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that we can set a style directly inside an element using Setter.Target instead of Setter.Property")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetStyleOnElementWithExistingAttribute)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that we can set a style on an element that already has an attribute defined")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetAttachedDP)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that setting a style can set on an attached DependencyProperty")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetAttachedDP_TargetPropertyPath)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that setting a style can set on an attached DependencyProperty using Setter.Target instead of Setter.Property")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanStyleSetterDetectNonDP)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that applying a style setter to a non-DependencyProperty results in a XAML parse error")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanStyleSetterDetectNonexistentProperty)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that applying a style setter to a nonexistent property results in a XAML parse error")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetStyleWithTargetTypeAndEmptyControlTemplate)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that setting a style with a target type and a matching, empty ControlTemplate doesn't fail")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetStyleBasedOnAnotherStyle)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that a style based on another style can be applied")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanCreateOptimizedStyle)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that an optimized style can be created from a regular style")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanCreateOptimizedStyle_TargetPropertyPath)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that an optimized style can be created from a regular style using Setter.Target instead of Setter.Property")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanFaultInOptimizedStyleSetters)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that the setters of an optimized style can be read")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseRegularStyleWithOptimizedBasedOnStyle)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseOptimizedStyleWithRegularBasedOnStyle)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseOptimizedStyleWithRegularNestedBasedOnStyles)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseRegularStyleWithRegularBasedOnStyle)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetStyleWithStringValueOnComplexTypeWithoutTypeConverter)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetStyleWithStringValueOnCustomFontFamilyProperty)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseSingleStyleFromParentInlineResources)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseTwoStylesFromParentInlineResources)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseStyleNestedInStyle)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseStyleFromThemeResources)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseStyleFromMergedResources)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseStyleWithNestedTemplateVSM)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseTwoStylesWithNestedTemplateVSM)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseStyleWithDeepNestedTemplateVSM)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseStyleWithThemeResource)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseStyleWithCustomResource)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanStyleResetPropertyOfDependencyObjectType)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleBasedOnSelfError)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that a style based on itself causes a exception while loading, but does not cause a stack overflow.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleWithReferenceCycleError)
            // This test crashes if the element is added to the tree. Leave it in compat mode where the style is applied during CreationComplete.
            TEST_METHOD_PROPERTY(L"Data:PerfOptIn", L"{false}")
            TEST_METHOD_PROPERTY(L"Data:XamlOptionalChanges", L"{DelayApplyStyleOptimization:false}")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleBasedOnSameKey)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleBasedOnBuiltinStyleWithSameKey)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleBasedOnSameKeyBasedOnAnother)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleWithStaticResourceInTemplate)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleSetterWithAutoLengthValue)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleSetterWithEnumValue)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleSetterWithCustomEnumValue)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleSetterWithStaticResourceBinding)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleSetterWithElementNameBinding)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleSetterWithElementNameTwoWayBinding)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleSetterWithSelfBinding)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleSetterWithBinding)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleWithNestedResources)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleInVSM)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleInVSMWithNestedTemplate)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleInVSMWithNestedResources)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleInVSMWithNestedVSM)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCoreStyleSettersReleased)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCoreBasedOnStyleSettersReleased)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanFaultInUnsealedStyleAndAddSetter)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetNullStyleSetterValue)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetDefaultStyle)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verifies the style of a custom control with DefaultStyleKey set")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetDefaultStyleResourceUri)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verifies the style of a custom control with DefaultStyleResourceUri set")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DefaultStyleResourceUriWithEmptyDefaultStyleKey)
            TEST_METHOD_PROPERTY(L"Description",
                L"Tests a custom control with a valid DefaultStyleResourceUri and an empty DefaultStyleKey")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DefaultStyleResourceUriWithInvalidDefaultStyleKey)
            TEST_METHOD_PROPERTY(L"Description",
                L"Tests a custom control with a valid DefaultStyleResourceUri and an invalid DefaultStyleKey")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PageThemeResourceCustomTargetProperty)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PageThemeResourceCustomSourceObject)
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleSetterWithUid)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleSetterWithBadUidProperty)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleChangeWithThemeRefs)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StandardStylesTest1)
            TEST_METHOD_PROPERTY(L"Description",
                L"Jupiter Standard Styles Tests (Ported from legacy:StandardStylesP0)")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StandardStylesTest2)
            TEST_METHOD_PROPERTY(L"Description",
                L"Jupiter Standard Styles Tests (Ported from legacy:StandardStylesP0)")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyImplicitStyleInvalidation)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that implicit styles are properly invalidated for a subtree when a ResourceDictionary is added to the subtree root")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyImplicitStyleInvalidationInThemeDictionaries)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that implicit styles are properly invalidated for a subtree when a ResourceDictionary is added to the subtree root and its ThemeDictionaries contains an implicit style")
            TEST_METHOD_PROPERTY(L"Ignore", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleSetterWithNullStaticResource)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that a Style with a Setter whose value is a StaticResource resolving to 'null' works.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StyleSetterWithNullThemeResource)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that a Style with a Setter whose value is a StaticResource resolving to 'null' works.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySetterWithInvalidAttachedPropertyTargetDoesNotAV)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that a Style with a Setter whose Property/Target is an invalid attached property does not result in AV")
        END_TEST_METHOD()

    private:
        Microsoft::UI::Xaml::Controls::Canvas^ m_rootCanvas;

        void TestStyleForCustomControl(bool isExplicit);
        void TestStyleForMultipleCustomControls(bool isExplicit);
        void TestStyleForMultipleDifferentControls(bool isExplicit);
        void TestStyleForCustomUserControl(bool isExplicit);
        void TestControlTemplateStyleFromStaticResource(bool isExplicit);
        void TestControlTemplateStyleDirect(bool isExplicit);
        void TestCanUseStyleWithNestedTemplateVSM(bool isExplicit);
        void TestCanUseStyleWithDeepNestedTemplateVSM(bool isExplicit);
        void TestCanCreateOptimizedStyle(bool useSetterTarget);
    };
} } } } } }

