// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "PropertySystemUnitTests.h"

#include <MetadataAPI.h>
#include <TypeTableStructs.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <CDependencyObject.h>
#include <Type.h>
#include <primitives.h>
#include <ModifiedValue.h>
#include <MockDependencyProperty.h>
#include <DependencyObjectMocks.h>
#include <CustomDependencyProperty.h>
#include <ValueBuffer.h>
#include <DoubleUtil.h>

using namespace DirectUI;
using namespace Microsoft::UI::Xaml::Tests::Metadata;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework { namespace DependencyObject {

    void PropertySystemUnitTests::CannotSetPropertyOnFrozenObject()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Double);
        dp.SetFlags(MetaDataPropertyInfoFlags::IsSparse);

        CValue value;
        value.SetSigned(42);

        obj->SimulateFreeze();
        VERIFY_ARE_EQUAL(E_ACCESSDENIED, obj->SetEffectiveValue(CDependencyObject::EffectiveValueParams(&dp, value, BaseValueSourceLocal)));

        obj->SimulateUnfreeze();
        VERIFY_SUCCEEDED(obj->SetEffectiveValue(CDependencyObject::EffectiveValueParams(&dp, value, BaseValueSourceLocal)));
    }

    void PropertySystemUnitTests::CanResetReferenceFromChildInSparseProperty()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        xref_ptr<CDependencyObject> reference;
        reference.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetFlags(MetaDataPropertyInfoFlags::IsSparse | MetaDataPropertyInfoFlags::IsVisualTreeProperty);
        dp.SetPropertyTypeIndex(KnownTypeIndex::Object);

        CValue value;
        value.WrapObjectNoRef(reference.get());

        VERIFY_SUCCEEDED(obj->SetValue(&dp, value));

        VERIFY_IS_NOT_NULL(reference->GetParent());

        // Let our object destroy itself. This should lead to a call to ResetReferencesFromChildren.
        obj = nullptr;

        VERIFY_IS_NULL(reference->GetParent());
    }

    void PropertySystemUnitTests::DependencyPropertyStoresWeakReferences()
    {
        VERIFY_IS_TRUE(CDependencyObject::IsDependencyPropertyBackReference(KnownPropertyIndex::Page_Frame));
        VERIFY_IS_TRUE(CDependencyObject::IsDependencyPropertyWeakRef(KnownPropertyIndex::Page_Frame));

        VERIFY_IS_FALSE(CDependencyObject::IsDependencyPropertyBackReference(KnownPropertyIndex::FrameworkElement_Tag));
        VERIFY_IS_FALSE(CDependencyObject::IsDependencyPropertyWeakRef(KnownPropertyIndex::FrameworkElement_Tag));
    }

    void PropertySystemUnitTests::GetDefaultValue()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp(KnownPropertyIndex::ScrollViewer_ZoomFactor);

        CValue value;
        VERIFY_SUCCEEDED(obj->GetDefaultValue(&dp, &value));
        VERIFY_ARE_EQUAL(1.0, value.AsDouble());
    }

    void PropertySystemUnitTests::GetDefaultValue_TypeName()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp(KnownPropertyIndex::PageStackEntry_SourcePageType);

        CValue value;
        VERIFY_SUCCEEDED(obj->GetDefaultValue(&dp, &value));
        VERIFY_ARE_EQUAL(KnownTypeIndex::UnknownType, value.AsTypeHandle());
    }

    void PropertySystemUnitTests::GetDefaultValue_AppBarClosedDisplayMode()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp(KnownPropertyIndex::AppBar_ClosedDisplayMode);

        {
            CValue value;
            VERIFY_SUCCEEDED(obj->GetDefaultValue(&dp, &value));
            VERIFY_ARE_EQUAL(static_cast<UINT32>(xaml_controls::AppBarClosedDisplayMode_Compact), value.AsEnum());
        }

    }

    void PropertySystemUnitTests::GetDefaultValue_Enum()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty fieldDP;
        fieldDP.SetPropertyTypeIndex(KnownTypeIndex::Visibility);

        CCustomDependencyProperty customDP;
        customDP.SetFlags(MetaDataPropertyInfoFlags::IsCustomDependencyProperty);
        customDP.SetPropertyTypeIndex(KnownTypeIndex::Visibility);

        {
            CValue value;
            VERIFY_SUCCEEDED(obj->GetDefaultValue(&fieldDP, &value));
            VERIFY_IS_TRUE(value.IsEnum());
            VERIFY_ARE_EQUAL(static_cast<UINT32>(xaml::Visibility_Visible), value.AsEnum());

            VERIFY_SUCCEEDED(obj->GetDefaultValue(&customDP, &value));
            VERIFY_IS_TRUE(value.IsEnum());
            VERIFY_ARE_EQUAL(static_cast<UINT32>(xaml::Visibility_Visible), value.AsEnum());
        }
    }

    void PropertySystemUnitTests::PropagateLayoutDirty()
    {
        xref_ptr<MockDependencyObjectTrackingLayoutFlag> obj;
        obj.attach(new MockDependencyObjectTrackingLayoutFlag());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Int32);

        CValue value;
        value.SetSigned(10);

        dp.SetFlags(MetaDataPropertyInfoFlags::IsSparse | MetaDataPropertyInfoFlags::AffectMeasure | MetaDataPropertyInfoFlags::AffectArrange);
        VERIFY_SUCCEEDED(obj->SetEffectiveValue(CDependencyObject::EffectiveValueParams(&dp, value, BaseValueSourceLocal)));
        VERIFY_IS_TRUE(obj->affectedMeasure);
        VERIFY_IS_TRUE(obj->affectedArrange);

        obj->Reset();

        dp.SetFlags(MetaDataPropertyInfoFlags::IsSparse | MetaDataPropertyInfoFlags::AffectMeasure);

        value.SetSigned(11);
        VERIFY_SUCCEEDED(obj->SetEffectiveValue(CDependencyObject::EffectiveValueParams(&dp, value, BaseValueSourceLocal)));
        VERIFY_IS_TRUE(obj->affectedMeasure);
        VERIFY_IS_FALSE(obj->affectedArrange);
    }

    void PropertySystemUnitTests::RepackageValue()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Int32);

        xref_ptr<CInt32> boxedInt;
        boxedInt.attach(new CInt32());
        boxedInt->m_iValue = 42;

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        value.WrapObjectNoRef(boxedInt.get());

        CValue* pValue = &value;

        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
        VERIFY_ARE_EQUAL(valueSigned, pValue->GetType());
    }

    void PropertySystemUnitTests::RepackageValue_TypeHandle()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::TypeName);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        value.SetNull();

        CValue* pValue = &value;

        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
        VERIFY_ARE_EQUAL(valueTypeHandle, pValue->GetType());
    }

    void PropertySystemUnitTests::RepackageValuePreservesNullableState()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        value.SetNull();

        CValue* pValue = &value;

        MockDependencyProperty nullableDP(KnownPropertyIndex::ToggleButton_IsChecked);
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&nullableDP, pValue, &pValue));
        VERIFY_ARE_EQUAL(valueNull, pValue->GetType());

        MockDependencyProperty nonNullableDP(KnownPropertyIndex::FrameworkElement_Width);
        VERIFY_ARE_EQUAL(E_INVALIDARG, buffer.RepackageValue(&nonNullableDP, pValue));
    }

    void PropertySystemUnitTests::RepackageFloatGridLengthPreservesFloatType()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        value.SetDouble(XDOUBLE_NAN);

        CValue* pValue = &value;

        // Since our property system stores FrameworkElement_Width and FrameworkElement_Height as floats
        // under the hood for compat reasons, let's verify this repackaging reflects that and doesn't actually repackage
        // as a valueGridLength.
        MockDependencyProperty floatLengthDP(KnownPropertyIndex::FrameworkElement_Width);
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&floatLengthDP, pValue, &pValue));
        VERIFY_IS_TRUE(floatLengthDP.StoreDoubleAsFloat());
        VERIFY_ARE_EQUAL(valueFloat, pValue->GetType());
        VERIFY_IS_TRUE(!!DoubleUtil::IsNaN(pValue->AsFloat()));
    }

    void PropertySystemUnitTests::RepackageDoubleGridLengthPreservesDoubleType()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        value.SetDouble(3.0);

        CValue* pValue = &value;

        // Since our property system stores SplitView_OpenPaneLength as a double floats, let's verify this repackaging reflects that and doesn't actually repackage
        // as a valueGridLength.
        MockDependencyProperty doubleLengthDP(KnownPropertyIndex::SplitView_OpenPaneLength);
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&doubleLengthDP, pValue, &pValue));
        VERIFY_IS_TRUE(!doubleLengthDP.StoreDoubleAsFloat());
        VERIFY_ARE_EQUAL(valueDouble, pValue->GetType());
        VERIFY_ARE_EQUAL(pValue->AsDouble(), 3.0);
    }
    void PropertySystemUnitTests::SetAnimatedValueSetsModifierFlag()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Double);
        dp.SetFlags(MetaDataPropertyInfoFlags::IsSparse);

        CValue value;
        value.SetDouble(6.0);

        VERIFY_SUCCEEDED(obj->SetEffectiveValue(CDependencyObject::EffectiveValueParams(&dp, value, BaseValueSourceLocal)));

        // Verify there is no modified value yet.
        auto modifiedValue = obj->GetModifiedValue(&dp);
        VERIFY_IS_NULL(modifiedValue);

        // Set the animated value.
        value.SetDouble(42.0);
        VERIFY_SUCCEEDED(obj->SetAnimatedValue(&dp, value));

        // Verify there is now a modified value and it's in the right state.
        modifiedValue = obj->GetModifiedValue(&dp);
        VERIFY_IS_NOT_NULL(modifiedValue);
        VERIFY_IS_TRUE(modifiedValue->IsAnimated());
    }

    void PropertySystemUnitTests::SetBaseValueSource()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty nameDP(KnownPropertyIndex::DependencyObject_Name);
        VERIFY_IS_TRUE(obj->IsPropertyDefault(&nameDP));
        VERIFY_SUCCEEDED(obj->SetBaseValueSource(&nameDP, BaseValueSourceLocal));
        VERIFY_ARE_EQUAL(BaseValueSourceLocal, obj->GetBaseValueSource(&nameDP));
        VERIFY_IS_FALSE(obj->IsPropertyDefault(&nameDP));
    }

    void PropertySystemUnitTests::SetEffectiveValue()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Double);
        dp.SetFlags(MetaDataPropertyInfoFlags::IsSparse);

        CValue value;
        value.SetSigned(42);

        VERIFY_SUCCEEDED(obj->SetEffectiveValue(CDependencyObject::EffectiveValueParams(&dp, value, BaseValueSourceLocal)));
        VERIFY_ARE_EQUAL(BaseValueSourceLocal, obj->GetBaseValueSource(&dp));
        VERIFY_IS_FALSE(obj->IsPropertyDefault(&dp));

        CValue result;
        VERIFY_SUCCEEDED(obj->GetEffectiveValueInSparseStorage(&dp, &result));
        VERIFY_ARE_EQUAL(valueDouble, result.GetType());
        VERIFY_ARE_EQUAL(42.0, result.AsDouble());
    }

    void PropertySystemUnitTests::SetEffectiveValueInFieldChecksValueEquality()
    {
        DECLARE_CONST_STRING_IN_TEST_CODE(myStringValue, L"Hello");

        xref_ptr<MockDependencyObjectWithStringProperty> obj = make_xref<MockDependencyObjectWithStringProperty>();

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::String);
        dp.offset = OFFSET(MockDependencyObjectWithStringProperty, value);

        CValue value;
        value.SetString(myStringValue);

        CValue oldValue;
        bool propertyChangedValue = true;

        // First time we set it, the property value should've changed.
        VERIFY_SUCCEEDED(obj->SetEffectiveValueInField(
            CDependencyObject::EffectiveValueParams(&dp, value, BaseValueSourceLocal),
            oldValue,
            /* oldValueOuter */ nullptr,
            propertyChangedValue));

        VERIFY_IS_TRUE(propertyChangedValue);

        // The second time we set it, nothing should've changed.
        VERIFY_SUCCEEDED(obj->SetEffectiveValueInField(
            CDependencyObject::EffectiveValueParams(&dp, value, BaseValueSourceLocal),
            oldValue,
            /* oldValueOuter */ nullptr,
            propertyChangedValue));

        VERIFY_IS_FALSE(propertyChangedValue);
    }

    void PropertySystemUnitTests::SetEffectiveValueInObjectField()
    {
        xref_ptr<MockDependencyObjectWithObjectProperty> obj;
        obj.attach(new MockDependencyObjectWithObjectProperty());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::DependencyObject);
        dp.offset = OFFSET(MockDependencyObjectWithObjectProperty, value);

        xref_ptr<CDependencyObject> reference;
        reference.attach(new CDependencyObject());

        CValue value;
        value.WrapObjectNoRef(reference.get());

        VERIFY_SUCCEEDED(obj->SetEffectiveValue(CDependencyObject::EffectiveValueParams(&dp, value, BaseValueSourceLocal)));

        VERIFY_ARE_EQUAL(reference.get(), obj->value);
    }

    void PropertySystemUnitTests::SetValueOverridesAnimatedValue()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Double);
        dp.SetFlags(MetaDataPropertyInfoFlags::IsSparse);

        CValue value;
        value.SetDouble(6.0);

        VERIFY_SUCCEEDED(obj->SetAnimatedValue(&dp, value));

        // Verify there is a modified value.
        auto modifiedValue = obj->GetModifiedValue(&dp);
        VERIFY_IS_NOT_NULL(modifiedValue);
        VERIFY_IS_TRUE(modifiedValue->IsAnimated());

        // Set the local value.
        value.SetDouble(42.0);
        VERIFY_SUCCEEDED(obj->SetValue(&dp, value));

        // Verify the local value is the effective value.
        CValue result;
        VERIFY_SUCCEEDED(obj->GetValue(&dp, &result));
        VERIFY_ARE_EQUAL(value, result);
    }

    void PropertySystemUnitTests::SetValueDuringAnimationOverridesAnimatedValue()
    {
        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Double);
        dp.SetFlags(MetaDataPropertyInfoFlags::IsSparse);

        auto obj = make_xref<MockDependencyObjectWithPropertyChangedHandler>();
        obj->propertyChangedCallback = [&obj, &dp](const PropertyChangedParams& args) -> HRESULT
        {
            CValue value;
            value.SetDouble(10.0);
            return obj->SetValue(&dp, value);
        };

        CValue value;
        value.SetDouble(6.0);

        VERIFY_SUCCEEDED(obj->SetAnimatedValue(&dp, value));

        // Verify there is a modified value.
        auto modifiedValue = obj->GetModifiedValue(&dp);
        VERIFY_IS_NOT_NULL(modifiedValue);

        // Verify that the value is still considered animated.
        VERIFY_IS_TRUE(modifiedValue->IsAnimated());

        // Verify GetValue() returns the value we set in the property change handler.
        CValue result;
        VERIFY_SUCCEEDED(obj->GetValue(&dp, &result));
        VERIFY_ARE_EQUAL(10.0, result.AsDouble());

        // Verify that ClearAnimatedValue simply clears the ModifiedValue, but doesn't change any
        // values.
        VERIFY_SUCCEEDED(obj->ClearAnimatedValue(&dp, /* holdEndValue */ CValue::Empty()));
        VERIFY_IS_NULL(obj->GetModifiedValue(&dp));
        VERIFY_SUCCEEDED(obj->GetValue(&dp, &result));
        VERIFY_ARE_EQUAL(10.0, result.AsDouble());
    }

    void PropertySystemUnitTests::TypeGetsStoredAsTypeHandle()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetFlags(MetaDataPropertyInfoFlags::IsSparse);
        dp.SetPropertyTypeIndex(KnownTypeIndex::TypeName);

        xref_ptr<CType> type;
        type.attach(new CType());
        type->m_nTypeIndex = KnownTypeIndex::Button;

        CValue value;
        value.WrapObjectNoRef(type.get());

        VERIFY_SUCCEEDED(obj->SetEffectiveValue(CDependencyObject::EffectiveValueParams(&dp, value, BaseValueSourceLocal)));
        VERIFY_SUCCEEDED(obj->GetValueInternal(&dp, &value));

        VERIFY_ARE_EQUAL(valueTypeHandle, value.GetType());
        VERIFY_ARE_EQUAL(type->m_nTypeIndex, value.AsTypeHandle());
    }

    void PropertySystemUnitTests::UpdateObjectStateInvokesDPsWhenValueDidNotChange()
    {
        xref_ptr<MockDependencyObjectTrackingInvoke> obj;
        obj.attach(new MockDependencyObjectTrackingInvoke());

        // Pretend to be in the live tree.
        obj->ActivateImpl();

        MockDependencyProperty dp;
        dp.SetFlags(MetaDataPropertyInfoFlags::NeedsInvoke);
        CValue value;
        CValue oldValue;

        VERIFY_SUCCEEDED(obj->OnPropertySet(&dp, oldValue, value, /* propertyChangedValue */ false));
        VERIFY_IS_TRUE(obj->calledInvoke);
    }

    void PropertySystemUnitTests::ValidateCValue()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp(KnownPropertyIndex::FrameworkElement_Width);
        CValue value;

        value.SetFloat(0);
        VERIFY_SUCCEEDED(obj->ValidateCValue(&dp, value, dp.GetStorageType()));

        value.SetFloat(-1);
        VERIFY_ARE_EQUAL(E_INVALIDARG, obj->ValidateCValue(&dp, value, dp.GetStorageType()));
    }

    void PropertySystemUnitTests::VerifyCanAssociate()
    {
        xref_ptr<MockDependencyObjectWithObjectProperty> target;
        target.attach(new MockDependencyObjectWithObjectProperty());

        xref_ptr<CDependencyObject> reference;
        reference.attach(new CDependencyObject());

        CValue value;
        value.WrapObjectNoRef(reference.get());

        MockDependencyProperty dp;
        dp.SetFlags(MetaDataPropertyInfoFlags::None);
        dp.offset = OFFSET(MockDependencyObjectWithObjectProperty, value);

        // Properties that don't require an association check should never fail.
        reference->SetAssociated(true, nullptr /*Association owner needed only CShareableDependencyObject */);
        VERIFY_SUCCEEDED(target->VerifyCanAssociate(&dp, value));

        reference->SetAssociated(false, nullptr);
        VERIFY_SUCCEEDED(target->VerifyCanAssociate(&dp, value));

        // If an object is already associated, and the property does an association check, expect a failure.
        dp.SetFlags(MetaDataPropertyInfoFlags::RequiresMultipleAssociationCheck);
        reference->SetAssociated(true, nullptr /*Association owner needed only for CShareableDependencyObject */);
        VERIFY_ARE_EQUAL(E_INVALIDARG, target->VerifyCanAssociate(&dp, value));

        // If an object is not already associated, expect the check to pass.
        reference->SetAssociated(false, nullptr);
        VERIFY_SUCCEEDED(target->VerifyCanAssociate(&dp, value));

        // If we're re-assigning the same value, the check should pass.
        reference.CopyTo(&target->value);
        reference->SetAssociated(true, nullptr /*Association owner needed only for CShareableDependencyObject */);
        VERIFY_SUCCEEDED(target->VerifyCanAssociate(&dp, value));
    }

    void PropertySystemUnitTests::VerifyCanAssociate_Sparse()
    {
        xref_ptr<MockDependencyObjectWithObjectProperty> target;
        target.attach(new MockDependencyObjectWithObjectProperty());

        xref_ptr<CDependencyObject> reference;
        reference.attach(new CDependencyObject());

        CValue value;
        value.WrapObjectNoRef(reference.get());

        MockDependencyProperty dp;
        dp.SetFlags(MetaDataPropertyInfoFlags::IsSparse);
        dp.SetPropertyTypeIndex(KnownTypeIndex::Object);

        // Properties that don't require an association check should never fail.
        reference->SetAssociated(true, nullptr /*Association owner needed only for CShareableDependencyObject */);
        VERIFY_SUCCEEDED(target->VerifyCanAssociate(&dp, value));

        reference->SetAssociated(false, nullptr);
        VERIFY_SUCCEEDED(target->VerifyCanAssociate(&dp, value));

        // If an object is already associated, and the property does an association check, expect a failure.
        dp.SetFlags(MetaDataPropertyInfoFlags::IsSparse | MetaDataPropertyInfoFlags::RequiresMultipleAssociationCheck);
        reference->SetAssociated(true, nullptr /*Association owner needed only for CShareableDependencyObject */);
        VERIFY_ARE_EQUAL(E_INVALIDARG, target->VerifyCanAssociate(&dp, value));

        // If an object is not already associated, expect the check to pass.
        reference->SetAssociated(false, nullptr);
        VERIFY_SUCCEEDED(target->VerifyCanAssociate(&dp, value));

        // If we're re-assigning the same value, the check should pass.
        //VERIFY_SUCCEEDED(target->SetEffectiveValueInSparseStorage(&dp, value));
        CValue oldValue;
        bool propertyChangedValue = false;
        VERIFY_SUCCEEDED(target->SetEffectiveValueInSparseStorage(CDependencyObject::EffectiveValueParams(&dp, value, BaseValueSourceUnknown), oldValue, nullptr, propertyChangedValue));
        // TODO:  Replace above three lines with the commented-out line above them.
        reference->SetAssociated(true, nullptr /*Association owner needed only for CShareableDependencyObject */);
        VERIFY_SUCCEEDED(target->VerifyCanAssociate(&dp, value));
    }

    void PropertySystemUnitTests::GetDefaultValue_ModernCollectionBasePanelAreStickyGroupHeadersEnabledBase()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp(KnownPropertyIndex::ModernCollectionBasePanel_AreStickyGroupHeadersEnabledBase);

        {
            CValue value;
            VERIFY_SUCCEEDED(obj->GetDefaultValue(&dp, &value));
            VERIFY_ARE_EQUAL(true, !!value.AsBool());
        }
    }

    void PropertySystemUnitTests::GetDefaultValue_ItemsStackPanelAreStickyGroupHeadersEnabledBase()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp(KnownPropertyIndex::ItemsStackPanel_AreStickyGroupHeadersEnabled);

        {
            CValue value;
            VERIFY_SUCCEEDED(obj->GetDefaultValue(&dp, &value));
            VERIFY_ARE_EQUAL(true, !!value.AsBool());
        }
    }

    void PropertySystemUnitTests::GetDefaultValue_ItemsWrapGridAreStickyGroupHeadersEnabledBase()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp(KnownPropertyIndex::ItemsWrapGrid_AreStickyGroupHeadersEnabled);

        {
            CValue value;
            VERIFY_SUCCEEDED(obj->GetDefaultValue(&dp, &value));
            VERIFY_ARE_EQUAL(true, !!value.AsBool());
        }
    }

} } } } } }