// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <memory>
#include <map>
#include <tuple>
#include <wrl\module.h>
#include <wil\resource.h>
#include "XamlOM.WinUI.h"
#include <TestCleanupWrapper.h>
#include "XamlDiagnosticsHelper.h"
#include "XamlDiagnosticsTestBase.h"

using namespace Microsoft::UI::Xaml::Controls;
namespace wrl = Microsoft::WRL;
namespace test_common = Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools {
    namespace XamlDiagnostics {

        class XamlDiagnosticsTests : public BaseTestClass<XamlDiagnosticsTests>
        {
        public:
            BEGIN_TEST_CLASS(XamlDiagnosticsTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"sdk\\inc\\xamlom.idl")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e22a917c-ad18-4a09-bff9-d3ca3e5ee0b8")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)

            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(TestGetEnums)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully call into XamlDiagnostics and get enums.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCreateInstance)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully call into XamlDiagnostics and create an instance.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestGetSetClearProperty)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully call into XamlDiagnostics and get/set/clear property values.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestSetPropertyBinding)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully call into XamlDiagnostics and set properties with Bindings as a value.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestGetComponents)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully call into XamlDiagnostics and get components.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestGetHandlesAndIInspectables)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully call into XamlDiagnostics and get and IInspectables.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestHitTest)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully call into XamlDiagnostics and hit test the tree.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // HITTest API deprecated for IslandsOnly initialization type
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestHitTestReturnsInvisibleElements)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we hit testing returns invisible elements.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // HITTest API deprecated for IslandsOnly initialization type
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestHitTestReturnsDisabledElements)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that hit testing returns disabled elements.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // HITTest API deprecated for IslandsOnly initialization type
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestHitTestDoesntReturnElementsNotInTree)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that hit testing doesn't return elements that aren't in the live tree.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // HITTest API deprecated for IslandsOnly initialization type
                END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestHitTestDoesntReturnCollapsedElements)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that hit testing doesn't return elements that are collapsed.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // HITTest API deprecated for IslandsOnly initialization type
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestHitTestAfterChangingVisiblity)
                TEST_METHOD_PROPERTY(L"Description", L"This test get's a little fancy. We are going to make sure that once an element goes "
                                     L"from collapsed to visible, that we can now hit test it.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // HITTest API deprecated for IslandsOnly initialization type
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestRegisterInstance)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully call into XamlDiagnostics and register an custom instance.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestUnadviseVisualTreeChange)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully call into XamlDiagnostics and stop tree change notifications.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestUiLayer)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully call into XamlDiagnostics and draw on the UI layer.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestGetThemeResourceProperties)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully unbox a ThemeResource and get properties on it.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanSetAutoOnWidthProperty)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully set the width of a rectangle to Auto.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanSetValueOnCustomProperty)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully set a value on a custom DP.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestDoubleAndWidthHaveDifferentValues)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that when set to 'NaN' that width returns as 'Auto' and that a double "
                L"property returns as '-1.#IND' which is interpreted as 'NaN'.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // Some XamlDiagnostics tests still failing
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCorrectBaseValueSourceOnRowAndColumnDefinitions)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that when a Grid.RowDefinition or Grid.ColumnDefinition has a size > 0, that "
                                     L"we correctly report the collection as being set locally")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestProvideSourceForURI)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we get the source value for Windows.Foundation.URI properties/objects.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanSetURISource)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can set the URI source for properties/objects.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestProvideSourceForFontFamily)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we get the source value for FontFamily objects.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanSetSourceForFontFamily)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can set source value for FontFamily objects.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestBackgroundOnGrid)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully get the background on a grid.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestAutoOnSetters)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully get a property on a setter (VSM setters included) "
                                     L"a value is set to 'Auto'")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestSetEnum)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create an instance of enum and set it on an instance.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCreateColorAndSetToBrushProperty)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a Windows.UI.Color object and set it to a brush handle.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestReturnCorrectRootsInUAP)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we don't return the PrintRoot, TransitionRoot, or VisualDiagnosticsRoot to the callback.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestReturnCorrectRootsInWPF)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we don't return the PrintRoot, TransitionRoot, or VisualDiagnosticsRoot to the callback.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // Some XamlDiagnostics tests still failing
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestMultipleCallbacks)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we don't allow multiple callbacks.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestDataBoundProperties)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we data bound properties can be distinguished from non-databound properties.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestGetHandleToOwnedObject)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the call to GetIInspectableFromHandle returns an owned object.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestGetCalendarDatePickerDefault)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can get Calendar Date Picker default value after being set locally.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestGetUnnamedProperties)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the runtime correctly populates debug information for unnamed property")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // Some XamlDiagnostics tests still failing
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestGetCalendarViewProperties)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the call to GetIInspectableFromHandle returns an owned object.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCreateInstanceOfCustomType)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can create an instance of a custom type.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestGetPropertyValueAndIndex)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can get a property's index and value from an object based off only the property's name.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestGetPropertyIndexAttached)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can get an attached property's index and value from an object based off only the property's name.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestGetPropertyIndexAndValueCustom)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that GetPropertyIndex and GetLocalProperty work on a custom property.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestGetPropertyIndexAndValueCollection)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that GetPropertyIndex and GetLocalProperty work on a collection (verifies they update handle map correctly).")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestAppAnalysisIntegrationWithLVT)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the InstanceHandles returned from AppAnalysis will match what XamlDiag has so elements can be linked to the LVT.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestPropertyChainBinding)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the value returned by an object with a binding is the binding itself, not the bound value")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestMetadataNullValueBits)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the IsValueNull metadatabit is set correctly for properties")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCreateInstanceDependencyProperty)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that CreateInstance succeeds when creating a dependency property")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestObjectIdentity)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that InstanceHandles correctly represent an objects identity")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestResourceDataTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that having a custom DataTemplate property that isn't registered as a DP which uses a StaticResource won't crash the framework when debugging is enabled")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyDontCleanupPropertiesForLiveElements)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that HandleMap.Release properties doesnt cleanup properties for elements in the LVT.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestThemeResourceInStyle)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that modifying a Style's property when it uses a ThemeResource for a different property works")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestAddNewSetterInStyle)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that modifying a Style with SetProperty that should add a Setter does")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // WPF_HOSTING_MODE_FAILURE - Error: Verify: IsTrue(wcscmp(colorProperty.Value, L"Yellow") == 0)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestChangeSetterProperty)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that modifying the Property of a Style Setter works as expected")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestClearRenderProperty)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies clearing a matrix transform property works correctly")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestGetPropertyNull)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that getting a property which is null doesn't cause a crash")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestEvaluatedValue)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies we get the evaluated value for an invalid binding")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestBasedOnSetterChange)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that modifications to Style Setters through SetProperty properly affect other Styles which are BasedOn the changed Style")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
           END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestChangedBasedOnProperty)
                TEST_METHOD_PROPERTY(L"Description", L"Validates changing the BasedOn property of a Style works")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestClearPropertyBinding)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that clearing a Binding works as expected")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(GetPropertyIndexFailsForInvalidProperty)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that calling GetPropertyIndex with a property name that doesn't match the object, fails.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(GetAttachedPropertyWithUnknownOwnerType)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can don't crash with an attached property with null declaring type.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyGetBuiltinStyleProperties)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we get built-in style properties even though they report as BaseValueSourceStyle")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifySourceChainReflectsPrecedence)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the order of the sources in the source chain returned from GetPropertyValuesChain represents"
                                                     L"the precedence of XAML's property system")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyOnDemandTransitionCollections)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that accessing a property chain value for ItemsPresenter.HeaderTransitions, forcing the creation of that on-demand property, does not cause a crash")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(GetPropertyChainReturnsDesiredSize)
                TEST_METHOD_PROPERTY(L"Description", L"Verify we add the desired size to the property chain")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CorrectlyValidateFakeProperties)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ClearTypesOnNonPlatformTypes)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateCorrectNamescopes)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanSetNull)
                TEST_METHOD_PROPERTY(L"Description", L"Verify we can set a property to null using a handle value of 0")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanGetSetSimpleProperty)
                TEST_METHOD_PROPERTY(L"Description", L"Verify we can get and set simple properties")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanGetSetNonDP)
                TEST_METHOD_PROPERTY(L"Description", L"Verify we can get and set custom user non-DP properties")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanUseIPropertyValueTypes)
                TEST_METHOD_PROPERTY(L"Description", L"Verify we can get and set types used by IPropertyValue")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyMutationEvents)
               TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
               TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")    // WPF_HOSTING_MODE_FAILURE - Fails because it loads Xaml with Popup IsOpen="true"
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyGetCallbackWhenNoDispatcherQueues)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // DCPP: XamlDiagnosticsTests::VerifyGetCallbackWhenNoDispatcherQueues leaks
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ModifyMediaPlayerElementSource)
               TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            
        private:
            HRESULT DrawUI(Grid^ pGrid);

            Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper SetupGridWithCallback(wrl::ComPtr<Microsoft::UI::Xaml::Tests::Common::VisualTreeServiceCallback>& callback);
            Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper LoadXamlFromFunction(const std::function<UIElement^()> func, wrl::ComPtr<Microsoft::UI::Xaml::Tests::Common::VisualTreeServiceCallback>& callback);
            void TestModifyResourceDictionarySource(InstanceHandle resources, const wrl::ComPtr<Microsoft::UI::Xaml::Tests::Common::VisualTreeServiceCallback>& callback);
            std::vector<InstanceHandle> DoHitTest(const RECT& rect);
            void TestReturnCorrectRootsHelper(unsigned numberOfRoots);

        };
    }
} } } } }
