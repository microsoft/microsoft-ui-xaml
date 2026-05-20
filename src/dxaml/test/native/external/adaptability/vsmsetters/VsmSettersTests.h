// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Adaptability {
        // Tuple describing what we expect layout for a given VisualState to be
        // <state name, expected fadeout animation, expected width of rect1, expected width of rect2, expected color of rect2's Fill Brush>
        using ExpectedLayoutSpec = std::tuple<Platform::String^, bool, double, double, ::Windows::UI::Color>;

        class VsmSetterIntegrationTests : public WEX::TestClass<VsmSetterIntegrationTests>
        {
        public:
            BEGIN_TEST_CLASS(VsmSetterIntegrationTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"c228d235-833c-46b2-858c-130c2cd3da83;f6edc090-2624-4657-9dee-b5437970e657")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(VerifyTargetPropertyPathConstructor)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that when a TargetPropertyPath is created using a DependencyProperty,"
                L"the resulting object has the expected values in the Target and Path properties.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyTargetPropertyPathTypeConverter)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that TargetPropertyPath objects are properly type converted "
                    L"from strings. Specifically, the Target property should have the correct object that was referenced by "
                    L"name in the original string and the Path property should have the correct property path string.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyTargetPropertyPathTypeConverterExceptions)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that TargetPropertyPath's type converter properly throws "
                    L"an exception if either the target or the property path are missing from the input string.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyVisualStateSettersExceptions)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the expected exceptions are thrown if various parts of "
                    L"a destination VisualState's setter are missing.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyVisualStateSettersInVsm_WithTransitions_Deferred_Templated)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that VisualState setters are properly applied when in the "
                    L"deferred VSM code path, transitions ARE used, and the VSM IS inside a template.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyVisualStateSettersInVsm_WithoutTransitions_Deferred_Templated)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that VisualState setters are properly applied when in the "
                    L"deferred VSM code path, transitions ARE NOT used, and the VSM IS inside a template.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyVisualStateSettersInVsm_WithTransitions_Legacy_Templated)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that VisualState setters are properly applied when in the "
                    L"legacy VSM code path, transitions ARE used, and the VSM IS inside a template.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyVisualStateSettersInVsm_WithoutTransitions_Legacy_Templated)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that VisualState setters are properly applied when in the "
                    L"legacy VSM code path, transitions ARE NOT not used, and the VSM IS inside a template.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyVisualStateSettersInVsm_WithTransitions_Deferred)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that VisualState setters are properly applied when in the "
                    L"deferred VSM code path, transitions ARE used, and the VSM IS NOT inside a template.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyVisualStateSettersInVsm_WithoutTransitions_Deferred)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that VisualState setters are properly applied when in the "
                    L"deferred VSM code path, transitions ARE NOT used, and the VSM IS NOT inside a template.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyVisualStateSettersInVsm_WithTransitions_Legacy)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that VisualState setters are properly applied when in the "
                    L"legacy VSM code path, transitions ARE used, and the VSM IS NOT inside a template.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyVisualStateSettersInVsm_WithoutTransitions_Legacy)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that VisualState setters are properly applied when in the "
                    L"legacy VSM code path, transitions ARE NOT not used, and the VSM IS NOT inside a template.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyVisualStateSettersDComp)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the DComp tree when VSM setters are used to ensure that primitives are rendered as expected.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyVisualStateSettersBinding)
                TEST_METHOD_PROPERTY(L"Description", L"Validates a simple binding on a VSM setter in a UserControl.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyVisualStateSetterCanUseXNull)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that VisualState setters are able to use {x:Null} as the Value.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyVisualStateSettersCanHaveUnsetValue)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that VisualState setters are allowed to not have a Value (this is a no-op).")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyVisualStateSetterDoesNotIncorrectlyClearValueDuringTransition)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that VisualState setters don't incorrectly clear property value during a VisualTransition.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyVisualStateSetterCanUseThemeResource)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that VisualState setters are able to use {ThemeResource} as the Value and that it updates on theme change when the base value is a {TemplateBinding}.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyVisualStateSetterCanReevaluateBinding)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that VisualState setters reevaluate bindings if GoToState(useTransitions=false) is called on the current state.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyAsyncSetterDoesnotClobberLaterSyncSetter)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that if asynchronous setter A is followed within the same tick by synchronous setter B, B is not clobbered by A.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyVsmSettersCanApplyStyle)
                TEST_METHOD_PROPERTY(L"Description",
                    L"Verifies that a VSM Setter can be used to change an element's applied Style. There is a check during XBF generation to "
                    L"forbid use of Setter.Property in a VSM Setter, so this is to verify that the check isn't overly broad, "
                    L"i.e. Setter.Property as part of a Style is still valid.")
            END_TEST_METHOD()

        private:
            void VerifyVisualStateSettersInVsm_Helper(
                bool useTemplatedVsm,
                bool usePessimalCodePath,
                bool useTransitions,
                const std::vector<ExpectedLayoutSpec>& expectedVisualStateLayouts);

            const std::vector<ExpectedLayoutSpec> InitializeExpectedVisualStateLayouts(bool usingTransitions);
        };
    }
} } } }

