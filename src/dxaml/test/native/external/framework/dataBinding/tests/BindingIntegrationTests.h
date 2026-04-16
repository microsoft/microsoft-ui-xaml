// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <memory>
#include <CustomMetadataRegistrar.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace DataBinding {
        class BindingIntegrationTests
        {
        public:
            BEGIN_TEST_CLASS(BindingIntegrationTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(CanBindBackgroundToSolidColorBrush)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Ensure Binding Grid.Background to a data source of property type SolidColorBrush doesn't throw cast exception")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanUpdateTwoWayBindingOnTextBox)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Ensure Setting TextBox.Text on two-way binding target pushes value back to source")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanUpdateOneWayBindingWithValueConverter)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Ensure updating a one-way Binding through a ValueConverter updates the target, but not the width")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanBindToEnumProperty)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Ensure binding to an enum property works")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanBindObjectToString)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Ensure we can bind properties to non DPs")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanDefineAttachedPropertyBindingInCode)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Ensure that attached property bindings defined in code work properly")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanDefineAttachedPropertyBindingInXaml)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Ensure that binding and template binding to attached properties work")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DataSourceConversions)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Test data conversions")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(BindingWithElementName)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Test Binding using ElementName")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanManuallyPropagateDataContext)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Create a panel with a data bound TextBlock. Manually handle the panel's DataContextChanged event and verify both that we can manually set a property and that when we set the Handled flag to true the bindings aren't automatically applied")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanSetDataContext)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Create a tree with one node, set data context property, verify binding propagates")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanSetDataContextViaSetValue)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Create a tree with one node, set data context via SetValue, verify binding propagates")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanUpdateViaInpc)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Create a tree with one node, set data context and binding, verify property changed event updates binding")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanUpdateOnlyAfterSettingDataContext)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Create a tree with one node, set binding, verify unchanged value, then set data context and verify the binding propagated")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanClearValueByClearingDataContext)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Create a tree with one node, set binding and data context, verify binding propagates, then clear data context and verify the value is cleared")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanSetDataContextAtMultipleLevels)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Create a tree with multiple levels, set some data contexts at various levels, and verify that the correct bindings propagate")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanLocalValueOverrideBinding)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Create a tree set data context on root, create a binding on the leaf, then override that binding and verify the explicit value sticks")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanContentPresenterUpdateContentBinding)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Create a Binding for ContentPresenter.Content and ensure that the Content template is created on first layout.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanCreateRelativeSourceTemplatedParentBindingInCode)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Create a RelativeSource TemplatedParent binding in a ControlTemplate in code")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanBindRelativeSourceTemplatedParentInDataTemplateInControlTemplate)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Create a RelativeSource TemplatedParent binding in a DataTemplate in a ControlTemplate and verify that the binding resolves to the template parent")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanBindRelativeSourceTemplatedParentInItemsPanelTemplateInControlTemplate)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Create a RelativeSource TemplatedParent binding in an ItemsPanelTemplate in a ControlTemplate and verify that the binding resolves to the template parent (the ItemsPresenter)")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanMoveRelativeSourceTemplatedParentBindingTarget)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Create a RelativeSource TemplatedParent binding, then move that binding target into another ControlTemplate and verify the binding still resolves correctly")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanUpdateRelativeSourceTemplatedParentBindingWithValueConverter)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Create a RelativeSource TemplatedParent binding, and verify it updates through a value converter")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateValueConverterParameters)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Validate that all the correct parameters are forwarded to a ValueConverter")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanValueConverterExceptionPropagateToApp)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Validate that an exception thrown by a ValueConverter propagates to the app")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanValueConverterHandleSpecialXamlTypes)
                TEST_METHOD_PROPERTY(L"Description",
                    L"ValueConverters need to perform a type lookup, and Point, Rect, and Size need special handling")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanUnresolvedPropertyPathUseFallbackValue)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Create a binding that can't resolve its target property and verify the FallbackValue is picked up instead")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanTypeConversionFailureUseFallbackValue)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Verify that a FallbackValue is used when a TypeConverter fails, and ignored when it succeeds")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanTwoWayBindingRespectUpdateSourceProperty)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Verify that a TwoWay Binding doesn't automatically update the source if UpdateSourceTrigger::Explicit is set")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanTwoWayBindingRespectUpdateSourcePropertyWithValueConverter)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Verify that a TwoWay Binding doesn't automatically update the source if UpdateSourceTrigger::Explicit is set via a ValueConverter")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanTargetNullValueUpdateTarget)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Verify that TargetNullValue is picked up when the source is null")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanTargetNullValueUpdateSource)
                TEST_METHOD_PROPERTY(L"Description",
                L"Verify that a TwoWay binding sets null on the source when the target picks up the TargetNullValue")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanGetBindingExpressionGetBinding)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Verify that GetBindingExpression returns non-null when a binding is set")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanBindToCollectionIndexer)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Verify that binding to a collection with an indexer works")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanBindToUserControlContentProperty)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Verify that binding UserControl.ContentProperty with a string will not crash the app.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DoesFloatBindingExpressionTriggerChange)
                TEST_METHOD_PROPERTY(L"Description",
                L"Verify when a binding expression is set for a DOUBLE property and it evaluates to the "
                L"default value that it doesn't trigger a property change notification")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DoesBindingExpressionPersistAfterCoercion)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Verify that a binding expression isn't forgotten when Value is coerced so that it falls within a range")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanTemplateBindCoreSourcePropertyToCustomTargetProperty)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanBindChildInDependencyObjectCollection)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanBindToAnimation)
                // WPF_HOSTING_MODE_FAILURE - Animates to incorrect value, needs investigation.
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyDataTemplateExtensions)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that ExtensionInstance and IDataTemplateExtension function properly.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanUseThemeResourceBindingBetweenColorPropertyAndSolidColorBrushValue)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DataContextChangesInParentChildTree)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(InheritanceContextChange)
                TEST_METHOD_PROPERTY(L"Description", L"Test when an InheritanceContext change occurs, to start binding")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DataContextObjectIdentity)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanConvertFromStringToPrimitiveType)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanConvertFromStringToRefType)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanConvertFromStringToEnumType)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanConvertFromTypeToType)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanCreateTwoWayBindingToTextBlocks)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestBindingUpdatesOnDO)
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanCombineDataContextChangedEventWithDataContextBindingOnSameElement)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifySyncUpdatesFromBinding)
                TEST_METHOD_PROPERTY(L"Description",
                L"Core properties sent async change notifications in Blue, unless they"
                L"were set from the framework side. This was breaking TwoWay bindings in some apps")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(BindingWithMatchingSubstringPropertyName)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(BindingCustomPropertyProviderOneWayIndexLookup)
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(BindingIVectorOneWayIndexLookup)
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(BindingIVectorIndexLookupTwoWayConverter)
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(BindingIMapKeyLookupTwoWayConverter)
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(BindingCustomPropertyProviderIntLookupTwoWayConverter)
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(DataContextChangedPreservesObjectIdentity)
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(CanBindToComposedDOWithoutTypeInfo)
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(CanBindIncompatibleObjectToUnknownTypeProperty)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PropertyPathSteps)
                TEST_METHOD_PROPERTY(L"Description", L"Verify Property Path steps")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CollectionViewBinding)
                TEST_METHOD_PROPERTY(L"Description", L"Verify binding to CollectionView")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyBoolToVisibilityConverter)
                TEST_METHOD_PROPERTY(L"Description", L"Verify built-in bool to visibility (and vice versa) converter")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyBoolToVisibilityConverterWithINPCDataSource)
                TEST_METHOD_PROPERTY(L"Description", L"Verify built-in bool to visibility (and vice versa) converter when using an INPC data source.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyCanUseBoolToVisibilityConverterWithMarkup)
                TEST_METHOD_PROPERTY(L"Description", L"Verify built-in bool to visibility converter can be used by XAML markup.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyUpdateSourceTrigger_LostFocus)
                TEST_METHOD_PROPERTY(L"Description", L"Verify Binding behavior when UpdateSourceTrigger is set to LostFocus.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyUpdateSourceTrigger_LostFocus_RequiresUIElement)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that a Binding using UpdateSourceTrigger.LostFocus must have a Target derived from UIElement.")
            END_TEST_METHOD()

            TEST_METHOD(BindingWorksAfterValueCoercion)
            BEGIN_TEST_METHOD(BindingWorksAfterMaximumCoercion)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyFallbackValueCanUseThemeResource)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that the Binding.FallbackValue property can be a {ThemeResource} and responds to theme changes.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyTargetNullValueCanUseThemeResource)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that the Binding.TargetNullValue property can be a {ThemeResource} and responds to theme changes.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyBindingRefreshCountOnThemeChange)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that various configurations of Binding are refreshed the expected number of times when the theme changes.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyBindingSwallowsErrorsOnThemeChange)
                TEST_METHOD_PROPERTY(L"Description", L"Binding errors should be swalloed (and therefore non-fatal) when processing theme changes")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyTargetNullValueDoesNotCauseStackOverflow)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that use of TargetNullValue can't cause a stack overflow if the Converter returns null")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyBindingTraceConvertFailed)
                TEST_METHOD_PROPERTY(L"Description", L"Verify the BindingFailed event if the converter fails to convert the value.")
            END_TEST_METHOD()
            
            BEGIN_TEST_METHOD(VerifyBindingTraceIntIndexerConnectionFailed)
                TEST_METHOD_PROPERTY(L"Description", L"Verify the BindingFailed event if the binding couldn't connect to an integer indexer.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyBindingTracePropertyConnectionFailed)
                TEST_METHOD_PROPERTY(L"Description", L"Verify the BindingFailed event if the binding fails to connect to a property.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyBindingTraceStringIndexerConnectionFailed)
                TEST_METHOD_PROPERTY(L"Description", L"Verify the BindingFailed event if the binding couldn't connect to a string indexer.")
            END_TEST_METHOD()

        private:
        };

        private ref class LocalObject sealed
        {

        };

        private ref class CanvasWithoutTypeInfo sealed :
            public Microsoft::UI::Xaml::Controls::Canvas,
            public Microsoft::UI::Xaml::Data::ICustomPropertyProvider
        {
        public:
            virtual Microsoft::UI::Xaml::Data::ICustomProperty^ GetCustomProperty(Platform::String^ name) { return nullptr; }
            virtual Microsoft::UI::Xaml::Data::ICustomProperty^ GetIndexedProperty(Platform::String^ name, ::Windows::UI::Xaml::Interop::TypeName type) { return nullptr; }
            virtual Platform::String^ GetStringRepresentation() { return nullptr; }

            virtual property ::Windows::UI::Xaml::Interop::TypeName Type
            {
                ::Windows::UI::Xaml::Interop::TypeName get()
                {
                    ::Windows::UI::Xaml::Interop::TypeName value;
                    value.Kind = ::Windows::UI::Xaml::Interop::TypeKind::Custom;
                    value.Name = L"CanvasWithoutTypeInfo";
                    return value;
                }
            }
        };

        private ref class LocalDataTemplateExtension sealed : Microsoft::UI::Xaml::IDataTemplateExtension
        {
        public:
            virtual void ResetTemplate() {}
            virtual bool ProcessBinding(unsigned int phase) { phase; return false; }
            virtual int ProcessBindings(Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs^ args) { args; return -1; }
        };

} } } } } }
