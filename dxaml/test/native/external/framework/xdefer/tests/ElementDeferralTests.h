// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <CustomMetadataRegistrar.h>
#include <RuntimeEnabledFeatureOverride.h>

struct IVisualStateGroupCollectionTestHooks;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {
        class ElementDeferralTests : public WEX::TestClass<ElementDeferralTests>
        {
        public:
            BEGIN_TEST_CLASS(ElementDeferralTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_RS5) // DCPP: RS4 Test Failure: ElementDeferralTests fail on RS4
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            // Parser validation

            BEGIN_TEST_METHOD(ValidateDeferLoadStrategyValues)
                TEST_METHOD_PROPERTY(L"Description", L"Validate parser recognizes valid values and throws on unrecognized ones")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateRealizeValues)
                TEST_METHOD_PROPERTY(L"Description", L"Validate parser recognizes valid values and throws on unrecognized ones")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateNameSetOnDeferredElement)
                TEST_METHOD_PROPERTY(L"Description", L"Validate error is thrown when name is not set on deferred element")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferredElementIsNotRoot)
                TEST_METHOD_PROPERTY(L"Description", L"Validate top level element is not deferred")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferLoadStrategyWithOtherAttributes)
                TEST_METHOD_PROPERTY(L"Description", L"Validate x:DeferLoadStrategy is parsed with other directives and properties")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferredElementGetsNamespaces)
                TEST_METHOD_PROPERTY(L"Description", L"Validate deferred element and its children get namespaces defined on deferred element")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferralIsIgnoredInLooseXAML)
                TEST_METHOD_PROPERTY(L"Description", L"Validate parser does not throw when deferal is in text XAML")
            END_TEST_METHOD()

            // Basic functionality

            BEGIN_TEST_METHOD(ValidateDefaultDoesNotDefer)
                TEST_METHOD_PROPERTY(L"Description", L"Validate default does not defer")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateCanDeferInNonTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Validate can defer in normal element")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateCanRedeferInNonTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Validate can redefer in normal element")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateCanDeferInDataTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Validate can defer in DataTemplate")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateCanRedeferInDataTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Validate can redefer in DataTemplate")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateCanDeferInControlTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Validate can defer in ControlTemplate")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateCanRedeferInControlTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Validate can redefer in ControlTemplate")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateLifetime)
                TEST_METHOD_PROPERTY(L"Description", L"Validate lifetime management")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateCannotRealizeFromNonDeclaringScope)
                TEST_METHOD_PROPERTY(L"Description", L"Validate element cannot be realized from non-declaring scope")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateCanRealizeWhenChildrenCollectionLocked)
                TEST_METHOD_PROPERTY(L"Description", L"Validate element will be realized when parent's children collection is locked, but insertion is deferred until layout is complete.")
            END_TEST_METHOD()

            // Deferral of different types of elements

            BEGIN_TEST_METHOD(ValidateDeferralForUnsupportedPropertiesThrow)
                TEST_METHOD_PROPERTY(L"Description", L"Validates deferral in unsupported properties throws")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferralForSupportedPropertiesInButtonBase)
                TEST_METHOD_PROPERTY(L"Description", L"Validates deferral in supported properties")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferralForSupportedPropertiesInContentControl)
                TEST_METHOD_PROPERTY(L"Description", L"Validates deferral in supported properties")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferralForSupportedPropertiesInControl)
                TEST_METHOD_PROPERTY(L"Description", L"Validates deferral in supported properties")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferralForSupportedPropertiesInFlyoutBase)
                TEST_METHOD_PROPERTY(L"Description", L"Validates deferral in supported properties")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferralForSupportedPropertiesInFrameworkElement)
                TEST_METHOD_PROPERTY(L"Description", L"Validates deferral in supported properties")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferralForSupportedPropertiesInItemsControl)
                TEST_METHOD_PROPERTY(L"Description", L"Validates deferral in supported properties")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferralForSupportedPropertiesInPanel)
                TEST_METHOD_PROPERTY(L"Description", L"Validates deferral in supported properties")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferralForSupportedPropertiesInSelectorItem)
                TEST_METHOD_PROPERTY(L"Description", L"Validates deferral in supported properties")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferralForSupportedTypesButtonBase)
                TEST_METHOD_PROPERTY(L"Description", L"Validates deferral of supported types")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferralForSupportedTypesContentControl)
                TEST_METHOD_PROPERTY(L"Description", L"Validates deferral of supported types")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferralForSupportedTypesControl)
                TEST_METHOD_PROPERTY(L"Description", L"Validates deferral of supported types")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferralForSupportedTypesFrameworkElement)
                TEST_METHOD_PROPERTY(L"Description", L"Validates deferral of supported types")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferralForSupportedTypesInItemsControl)
                TEST_METHOD_PROPERTY(L"Description", L"Validates deferral of supported types")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferralForSupportedTypesPanel)
                TEST_METHOD_PROPERTY(L"Description", L"Validates deferral of supported types")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferralForSupportedTypesSelectorItem)
                TEST_METHOD_PROPERTY(L"Description", L"Validates deferral of supported types")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferralForSupportedTypesShapes)
                TEST_METHOD_PROPERTY(L"Description", L"Validates deferral of supported types")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateReplacementOfContentInButtonBase)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that controls with content can have their deferred content replaced")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateReplacementOfContentInContentControl)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that controls with content can have their deferred content replaced (part 2)")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateReplacementOfContentInControl)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that controls with content can have their deferred content replaced (part 2)")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateReplacementOfContentInFrameworkElement)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that controls with content can have their deferred content replaced (part 2)")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateReplacementOfContentInItemsControl)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that controls with content can have their deferred content replaced (part 2)")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateReplacementOfContentInSelectorItem)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that controls with content can have their deferred content replaced (part 2)")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateContentClearValueInButtonBase)
                TEST_METHOD_PROPERTY(L"Description", L"Validate Content Clear Value")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateContentClearValueInContentControl)
                TEST_METHOD_PROPERTY(L"Description", L"Validate Content Clear Value")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateContentClearValueInControl)
                TEST_METHOD_PROPERTY(L"Description", L"Validate Content Clear Value")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateContentClearValueInFrameworkElement)
                TEST_METHOD_PROPERTY(L"Description", L"Validate Content Clear Value")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateContentClearValueInItemsControl)
                TEST_METHOD_PROPERTY(L"Description", L"Validate Content Clear Value")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateContentClearValueInSelectorItem)
                TEST_METHOD_PROPERTY(L"Description", L"Validate Content Clear Value")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateContentClearCollection)
                TEST_METHOD_PROPERTY(L"Description", L"Validate Clear method also works with deferred elements")
            END_TEST_METHOD()

            // Deferral in nested elements

            BEGIN_TEST_METHOD(ValidateNestedDeferral)
                TEST_METHOD_PROPERTY(L"Description", L"Validate deferred elements can be nested, varies load times")
            END_TEST_METHOD()

            // Desktop only until tests support loading XBF on phone
            BEGIN_TEST_METHOD(ValidateNestedDeferralForNonTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Validate non-template deferred elements can be nested in non-template element")
            END_TEST_METHOD()

            // Desktop only until tests support loading XBF on phone
            BEGIN_TEST_METHOD(ValidateNestedDeferralForDataTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Validate data-template deferred elements can be nested in non-template element")
            END_TEST_METHOD()

            // Desktop only until tests support loading XBF on phone
            BEGIN_TEST_METHOD(ValidateNestedDeferralForControlTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Validate control-template deferred elements can be nested in non-template element")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateNestedDeferredElementGetsDataContext)
                TEST_METHOD_PROPERTY(L"Description", L"Validate nested deferred element gets data context")
            END_TEST_METHOD()

            // Deferred elements and styles/resources

            BEGIN_TEST_METHOD(ValidateDeferredElementCanUseResourcesOfParent)
                TEST_METHOD_PROPERTY(L"Description", L"Validate deferred element can use resources of parent")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateChildrenOfDeferredElementCanUseResourcesOfParent)
                TEST_METHOD_PROPERTY(L"Description", L"Validate children of deferred element can use resources of parent")
            END_TEST_METHOD()

            // Desktop only until tests support loading XBF on phone
            BEGIN_TEST_METHOD(ValidateDeferredElementInDataTemplateCanUseResources)
                TEST_METHOD_PROPERTY(L"Description", L"Validate deferred element can use resources in DT")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferredElementInControlTemplateCanUseResources)
                TEST_METHOD_PROPERTY(L"Description", L"Validate deferred element can use resources in CT")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferredElementCanUseStyleFromParent)
                TEST_METHOD_PROPERTY(L"Description", L"Validate deferred element can use style from parent")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateChildrenOfDeferredElementCanUseStyleFromParent)
                TEST_METHOD_PROPERTY(L"Description", L"Validate children of deferred element can use style from parent")
            END_TEST_METHOD()

            // Binding and DataContext

            BEGIN_TEST_METHOD(ValidateElementCanBindToDeferredElementProperty)
                TEST_METHOD_PROPERTY(L"Description", L"Validate Element Can Bind To Deferred Element Property")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferredElementCanBindToParentElementProperty)
                TEST_METHOD_PROPERTY(L"Description", L"Validate Deferred Element Can Bind To Parent Element Property")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferredElementCanBindInDataTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Validate Deferred Element Can Bind In DT")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferredElementCanBindInControlTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Validate Deferred Element Can Bind In CT")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateTemplateBindingWhenRealizedFromOnApplyTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Validate Deferred Element template bindings work when realized from OnApplyTemplate")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferredElementGetsDataContext)
                TEST_METHOD_PROPERTY(L"Description", L"Validate Deferred Element Gets Data Context")
            END_TEST_METHOD()

            // Ordering of elements

            BEGIN_TEST_METHOD(ValidateOrderDependsOnDeclarationOrder)
                TEST_METHOD_PROPERTY(L"Description", L"Validate final order of deferred/realized element depends on declaration order, not on realization order")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateInsertionOfNormalElement)
                TEST_METHOD_PROPERTY(L"Description", L"Non-deferred elements should affect order of deferred elements")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateRemovalOfNormalElement)
                TEST_METHOD_PROPERTY(L"Description", L"Non-deferred elements should affect order of deferred elements")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateRealizedElementRemoveAndInsert)
                TEST_METHOD_PROPERTY(L"Description", L"Validate realized element can be removed and inserted")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferredElementCanBeRealizedAfterItsParentIsRemovedAndInserted)
                TEST_METHOD_PROPERTY(L"Description", L"Validate Deferred Element Can Be Realized After Its Parent Is Removed And Inserted")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferredElementInUserControlCanBeRealizedAfterItIsRemovedAndInserted)
                TEST_METHOD_PROPERTY(L"Description", L"Validate deferred element can be realized after UserControl it's declared in removed and inserted")
            END_TEST_METHOD()

            // Events

            // Desktop only until tests support loading XBF on phone
            BEGIN_TEST_METHOD(ValidateRealizedElementFiresEvents)
                TEST_METHOD_PROPERTY(L"Description", L"Validate realized element fires loaded/unloaded events")
            END_TEST_METHOD()

            // Animations

            BEGIN_TEST_METHOD(ValidateStoryboardRealizesDeferredElement)
                TEST_METHOD_PROPERTY(L"Description", L"Validate Storyboard Realizes Deferred Element")
            END_TEST_METHOD()

            // Redefer specific

            BEGIN_TEST_METHOD(ValidateCanRedeferWhenRealizedElementIsRemoved)
                TEST_METHOD_PROPERTY(L"Description", L"")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDeferTreeThrowsForInvalidElements)
                TEST_METHOD_PROPERTY(L"Description", L"")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateNestedDeferWithRealizeValue)
                TEST_METHOD_PROPERTY(L"Description", L"")
            END_TEST_METHOD()

        private:
            Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream;
        };
    }
} } } }
