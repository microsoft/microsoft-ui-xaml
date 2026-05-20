// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <CustomMetadataRegistrar.h>
#include <memory>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace ControlTemplates {

    class ControlTemplateIntegrationTests
    {

    public:
        ControlTemplateIntegrationTests() {}

        BEGIN_TEST_CLASS(ControlTemplateIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"f62323d4-fd46-4c98-aa85-334db95ba8f6;27f34780-4ef6-4102-929f-29737dfda1b9;0a9cdf5f-1e1b-4b1d-9659-b354bf5f4ca6")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)

        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanApplyTemplateToTargetType)
            TEST_METHOD_PROPERTY(L"Description",
                L"Create ControlTemplates that target various types, and verify that they can be applied to compatible controls")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanObserveOnApplyTemplateCall)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that OnApplyTemplate is invoked")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CantApplyControlTemplateToUserControl)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that applying a ControlTemplate to a UserControl fails with an exception")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanApplyControlTemplateWithRootElementNamedAsExistingElementInTree)
            TEST_METHOD_PROPERTY(L"Description",
                L"Create a control with a name, and apply to it a ControlTemplate that has root element with the same name. Verify no name collision occurs")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanGetTemplateChildFindControlTemplateChildren)
            TEST_METHOD_PROPERTY(L"Description",
                L"Apply a ControlTemplate with nested, named elements, and verify that each are findable via GetTemplateChild")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanGetTemplateChildIgnoreUnnamedChildrenAndOutOfScopeNames)
            TEST_METHOD_PROPERTY(L"Description",
                L"Apply a ControlTemplate with a nested, unnamed child, and verify that GetTemplateChild won't find it, nor the targetType control")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TargetTypeInXmlNamespacesTest)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that a ControlTemplate's target type can be resolved in various XML namespaces")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DefaultControlTemplateTargetTypeIsControlTest)
            TEST_METHOD_PROPERTY(L"Description", L"Check that the default ControlTemplate TargetType is Control.")
        END_TEST_METHOD()

    private:
    };
} } } } } }
