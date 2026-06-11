// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <Versioning.h>
#include <FeatureFlags.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {

        class ResourceDictionaryBasicTests
        {
        public:
            ResourceDictionaryBasicTests() {}

            BEGIN_TEST_CLASS(ResourceDictionaryBasicTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"f62323d4-fd46-4c98-aa85-334db95ba8f6")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)

            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(VerifyResourceDictionarySize)
                TEST_METHOD_PROPERTY(L"Description",
                L"Validates that resource dictionary can successfully retrieve its item count")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanAddItemToResourceDictionary)
                TEST_METHOD_PROPERTY(L"Description",
                L"Validates adding items to Resource Dictionary through code")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanRemoveItemFromResourceDictionary)
                TEST_METHOD_PROPERTY(L"Description",
                L"Validates removing items from Resource Dictionary through code")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanAccessMultiLevelResources)
                TEST_METHOD_PROPERTY(L"Description",
                L"Validates successful retrieval of style using three levels of resources in separately defined dictionaries")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanGetStyleResourcesByName)
                TEST_METHOD_PROPERTY(L"Description",
                L"Validates retrieval of style using name property as key")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanOverrideResourceDictionaryKey)
                TEST_METHOD_PROPERTY(L"Description",
                L"Validates successful overriding of parent resource key by child panel")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyImplicitKeyResources)
                TEST_METHOD_PROPERTY(L"Description",
                L"Validates successful style retrieval by using control TargetType as implicit key")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(StringsStartingWith0)
                TEST_METHOD_PROPERTY(L"Description",
                L"Validates that we can add multiple unique keys starting with 0 without having ResourceDictionary consider them duplicates.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ThemeDictionariesDeclaredAfterContent)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Validates that ThemeDictionaries declared after the Content of a ResourceDictionary can still resolve keys from the Content.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyBindingAsAResource)
                TEST_METHOD_PROPERTY(L"Description",
                L"Validates that a Binding can be added as a dictionary item.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UndeferredThemeDictionaryDeclaredAfterContent)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Validates that an undeferred ThemeDictionary (i.e. single key) declared after the Content of a ResourceDictionary can still resolve keys from the Content.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyResourceDictionarySizeBeforeAndAfterLookup)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Validates that resource dictionary size is the same before and after a lookup.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyThemeDictionaryLookup)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Validates that theme dictionary lookups return the correct theme.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyStaticResourceAliasing)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Validates that theme dictionary lookups return the correct theme.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyOutOfOrderDeferredKeyReferences)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Validates that keys that come in reverse order in a deferred resource dictionary resolve correctly.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ThemeResourceMissing)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ThemeResourceMissingAfterThemeChange)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(StyleResourceWithXName)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValueTypeAndEnumResources)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ParseErrorsBubbleWhenLoadingAllDeferredResources)
                TEST_METHOD_PROPERTY(L"Description", L"Previously, when we invoked code paths that caused all the remaining deferred "
                    L"resources to be faulted in, a runtime parse error would be ignored and an empty key would be added to the collection. "
                    L"This resulted in a crash when we attempted to add the key to the dictionary upon trying to dereference the null DO pointer.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyGetMapView)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that GetView returns a IMapView which contains the expected contents.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyExplicitTypeName)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that resource dictionary can have an explicit resource key that matches "
                                                     L"an implicit resource key (type name).")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(XNameResourceWithReferenceToXNameResource)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UndeferResourcesWithForwardReferences)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that we can undefer all resources in a dictionary where a resource "
                                                     L"references another, following resource.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AppThemeResource)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that app theme resources override global definitions.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AppThemeResourceImplicitStyle)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that app theme resources with implicit style override global definitions.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AppThemeResourceUpdated)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that inserting new resources into app theme resources updates references in a live page.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PageThemeResource)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that page theme resources override global and app definitions.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PageThemeResourceViaGlobalTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that theme ref to global resource Style/ControlTemplate that has a "
                                                     L"theme ref to a page resource overrides global and app definitions.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PageThemeResourceFallback)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that theme value lookup gets the app definition when a resource isn't"
                                                     L"defined in the page theme dictionary for one of the themes.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PageThemeResourceMultiplePages)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that page theme resources in multiple pages correctly override global "
                                                     L"and app definitions.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PageThemeResourceViaThemeStyle)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that theme ref to page resource Style with theme ref to another page resource "
                                                     L"overrides global and app definitions.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PageThemeResourceMixedRefs)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that page theme resources override global and app definitions on multiple "
                                                     L"descendants that have different requested themes at their scope.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PageThemeResourceMixedRefsViaThemeTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that page theme resource ControlTemplate with a theme ref to another "
                                                     L"page theme resource overrides global and app definitions, including when"
                                                     L"the different page theme templates have different theme refs.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PageThemeResourceMixedRefsViaThemeStyle)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that page theme resource Style works when each theme's Style definition has "
                                                     L"a theme ref to another resource that's missing in the other theme's dictionary.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PageThemeResourceViaThemeTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that theme ref to page resource ControlTemplate that has a theme ref "
                                                     L"to another page resource overrides global and app definitions.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PageThemeResourceViaThemeStyleTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that theme ref to page resource Style w/ ControlTemplate that has a theme ref "
                                                     L"to another page resource overrides global and app definitions.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PageThemeResourceViaThemeImplicitStyle)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that page theme resources with implicit style override global and app definitions.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PageThemeResourceNested)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that page theme resources override global and app definitions when an ancestor "
                                                     L"of the reference has theme resources that do not override the target resource.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PageThemeResourceCustomResource)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that page theme resources override app definitions for keys defined "
                                                     L"by the app but not globally.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PageThemeResourceUpdated)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that inserting new resources into page theme resources in a live page doesn't update "
                                                     L"references in the page. The behavior is not enabled due to performance concerns.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(LooseXamlWithStaticResourceAsThemeResource)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VSLooseGenericXaml)
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AccentColorChangeUpdatesGlobalThemes)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ThemeResourceOutsideThemeDictionary)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ThemeDictionariesInMergedDictionaries)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MergedDictionariesInThemeDictionaries)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PageThemeResourceMultiplePagesOneOverride)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PageThemeResourceNestedOneOverride)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ThemeRefInThemeResource)
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PageResourceCanOverrideThemeResources)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ActualThemeBasic)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ActualThemeWithAppThemeChanges)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ActualThemeWithSystemThemeChanges)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ActualThemeWithHighContrast)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ActualThemeWithTreeChanges)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CorrectThemeChangesForMergedDictionaryInAppResources)
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyDictionaryReentryOnThemeChange)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyMarkupExtensionAsResource)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that markup extensions (like Binding or NullExtension) can be used as a resource in a ResourceDictionary")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyMarkupExtensionAsResource_NoXbfV2)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that markup extensions (like Binding or NullExtension) can be used as a resource in a ResourceDictionary when XBFv2 is turned off")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyCanAddNullResource)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that a key with a null value can be added a ResourceDictionary in code-behind")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DontCrashOnNonExistentKey)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that checking if a key exists doesnt crash")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FollowPopupsInThemeResourceResolution)
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyThatThemeResourceAliasesDontExist)
                TEST_METHOD_PROPERTY(L"Description", L"ThemeResource aliases (<ThemeResource x:Key='MyAlias' ResourceKey='SomeExistingKey' />) should be treated as non-existent resources, "
                    L"at least until they are actually implemented.")
            END_TEST_METHOD()

            TEST_METHOD(VerifyKeyNotFoundCacheInvalidation)

            BEGIN_TEST_METHOD(VerifyXamlResourceReferenceTraceSimpleStaticResource)
                TEST_METHOD_PROPERTY(L"Description", L"Verify XAML resource reference lookup tracing in the simple failure case: "
                    L"StaticResource can't be resolved.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyXamlResourceReferenceTraceSimpleThemeResource)
                TEST_METHOD_PROPERTY(L"Description", L"Verify XAML resource reference lookup tracing in the simple failure case: "
                    L"ThemeResource can't be resolved.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyXamlResourceReferenceTraceResourceDictionarySource)
                TEST_METHOD_PROPERTY(L"Description", L"Verify XAML resource reference lookup tracing when a ResourceDictionary set "
                    L"through the 'Source' property is in the search path.")
            END_TEST_METHOD()


        private:
            void AccentColorChangeUpdatesGlobalThemesTest();
            void MergedDictionariesInThemeDictionariesTest();
            void ThemeRefInThemeResourceTest();
        };
} } } } }
