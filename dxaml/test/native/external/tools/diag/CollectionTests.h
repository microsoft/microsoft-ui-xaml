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

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools {
    namespace XamlDiagnostics {
        class CollectionTests : public BaseTestClass<CollectionTests>
        {
        public:
            BEGIN_TEST_CLASS(CollectionTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"sdk\\inc\\xamlom.idl")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e22a917c-ad18-4a09-bff9-d3ca3e5ee0b8")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"IsolationLevel", L"Class")
                TEST_CLASS_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(TestCollections)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully call into XamlDiagnostics and manipulate collections.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestNestedCollections)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully call into XamlDiagnostics and manipulate nested collections.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCommandBarCollectionOperations)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that XamlDiagnostics collection methods work with CommandBar collections")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyDontLeakCollection)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies we dont leak collections if user calls VisualTreeService.GetPropertyValuesChain")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestStyleSetterChangeThroughCollection)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that editing a Style's Setters through XamlDiag's collection APIs work")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestNonDependencyObjectCollection)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that our collection APIs work for an IVector which isn't a DependencyObject")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanFindNamedElementAddedToCollectionInTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can find named elements added to a control template.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyRowDefinitionsReportAsCollections)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that Grid.Row definitions report as collections.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AddingMergedDictionaryUpdatesReferences)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RemovingMergedDictionaryUpdatesReferences)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ClearingMergedDictionariesUpdatesReferences)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AddingMergedDictionaryDoesntOverrideParent)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            
            BEGIN_TEST_METHOD(RemovingMergedDictionaryDoesntOverrideParent)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ClearingSettersUpdatesApp)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RemovingSettersFromImplicitStyleUpdatesApp)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AddingMergedDictionaryUpdatesReferences2)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"UAP:AppXManifest", L"AppxManifest.DesignMode.xml")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanAddGroupedStyle)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateCollectionApisWithResourceDictionary)
                 TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // this test is unreliable
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateAddingSetterToStyleWithBasedOnWhileInvalid)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanAddNamedListBoxItem)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
        };
    }
} } } } }
