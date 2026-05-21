// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace DependencyObject {

        class PropertySystemUnitTests : public WEX::TestClass<PropertySystemUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(PropertySystemUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_CLASS_PROPERTY(L"FeatureUnderTest",
                    L"@ClassName = 'CDependencyObject' AND ("
                    L"@FunctionName = 'ValidateCValue*' OR "
                    L"@FunctionName = 'VerifyCanAssociate*' OR "
                    L"@FunctionName = 'SetBaseValueSource*' OR "
                    L"@FunctionName = 'GetDefaultValue*' OR "
                    L"@FunctionName = 'SetEffectiveValue*' OR "
                    L"@FunctionName = 'GetEffectiveValue*')")
            END_TEST_CLASS()

            TEST_METHOD(GetDefaultValue_AppBarClosedDisplayMode)
            
            TEST_METHOD(GetDefaultValue_Enum)

            TEST_METHOD(PropagateLayoutDirty)

            TEST_METHOD(RepackageValue)

            TEST_METHOD(RepackageValuePreservesNullableState)

            TEST_METHOD(CannotSetPropertyOnFrozenObject)

            TEST_METHOD(CanResetReferenceFromChildInSparseProperty)

            TEST_METHOD(DependencyPropertyStoresWeakReferences)

            TEST_METHOD(GetDefaultValue)

            TEST_METHOD(SetAnimatedValueSetsModifierFlag)

            TEST_METHOD(SetBaseValueSource)

            TEST_METHOD(SetEffectiveValue)

            TEST_METHOD(SetEffectiveValueInFieldChecksValueEquality)

            TEST_METHOD(SetEffectiveValueInObjectField)

            TEST_METHOD(SetValueOverridesAnimatedValue)
            
            TEST_METHOD(SetValueDuringAnimationOverridesAnimatedValue)

            TEST_METHOD(TypeGetsStoredAsTypeHandle)

            TEST_METHOD(UpdateObjectStateInvokesDPsWhenValueDidNotChange)

            TEST_METHOD(ValidateCValue)

            TEST_METHOD(VerifyCanAssociate)
            
            TEST_METHOD(VerifyCanAssociate_Sparse)

            TEST_METHOD(GetDefaultValue_ModernCollectionBasePanelAreStickyGroupHeadersEnabledBase)

            TEST_METHOD(GetDefaultValue_ItemsStackPanelAreStickyGroupHeadersEnabledBase)

            TEST_METHOD(GetDefaultValue_ItemsWrapGridAreStickyGroupHeadersEnabledBase)

            TEST_METHOD(RepackageValue_TypeHandle)

            TEST_METHOD(GetDefaultValue_TypeName)

            TEST_METHOD(RepackageFloatGridLengthPreservesFloatType)
            TEST_METHOD(RepackageDoubleGridLengthPreservesDoubleType)
        };
    }}
} } } }
