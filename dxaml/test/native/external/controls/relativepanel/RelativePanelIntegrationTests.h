// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace RelativePanel {

    class RelativePanelIntegrationTests : public WEX::TestClass<RelativePanelIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(RelativePanelIntegrationTests)  
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")


            TEST_METHOD_PROPERTY(L"Classification", L"Integration")

            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"1bb20c90-a558-491b-b76d-55bdb9a46911;57e0de30-efb3-4001-9ccc-b38032fd1974;bdc5c68b-03c1-4217-832c-4c09f99946f4")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_CLASS_CLEANUP(ClassCleanup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(CanPerformLayout)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the panel can perform layout correctly.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanParseDeferredElements)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the panel can perform layout correctly when using deferred elements.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBeEmpty)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the panel can render with no children.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanArrangePhysicallyImpossibleDefinitions)
         TEST_METHOD_PROPERTY(L"Description", L"Verifies that the panel does not fail even if contraints specify a definition that is physically impossible to arrange.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ThrowsExceptionForCircularDependencies)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that circular dependencies throw an InvalidOperationException.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ThrowsExceptionForNameNotFound)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that an InvalidOperationException is thrown when a named element is not found among the children of the panel.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ThrowsExceptionForReferenceNotFound)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that an InvalidOperationException is thrown when a referenced element is not found among the children of the panel.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ThrowsExceptionForInvalidType)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that an InvalidOperationException is thrown when the attached properties are set to a non-UIElement.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ThrowsExceptionForInvalidParsingOfDeferredElements)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that an exception is thrown during XBF generation when setting RelativePanel properties on deferred elements using unsupported syntax (e.g. markup extensions).")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(BorderChrome)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that border properties works for RelativePanel")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyPropertyChangesInvalidateMeasure)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that changing the RelativePanel attached properties invalidates measure on the RelativePanel.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyLayoutViaXamlReader)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that RelativePanel properties can be set via XamlReader::Load.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()
    };

} } } } } }

