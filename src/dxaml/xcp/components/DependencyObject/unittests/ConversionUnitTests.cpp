// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ConversionUnitTests.h"

#include <MetadataAPI.h>
#include <TypeTableStructs.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <CDependencyObject.h>
#include <primitives.h>
#include <ThreadLocalStorage.h>
#include <MockClassInfo.h>
#include <MockDependencyProperty.h>
#include <DependencyObjectMocks.h>
#include <CString.h>
#include <Double.h>
#include <DoubleUtil.h>
#include <ValueBuffer.h>
#include "WaitForDebugger.h"

using namespace DirectUI;
using namespace Microsoft::UI::Xaml::Tests::Metadata;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework { namespace DependencyObject {

    void ConversionUnitTests::CanRepackageDoubleAsInt()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Int32);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.SetDouble(1.0);
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
        VERIFY_ARE_EQUAL(valueSigned, pValue->GetType());
        VERIFY_ARE_EQUAL(1, pValue->AsSigned());
    }

#pragma region Conversions from float
    void ConversionUnitTests::CanConvertFloatToBool()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Boolean);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.SetFloat(1.5f);
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
        VERIFY_ARE_EQUAL(valueBool, pValue->GetType());
        VERIFY_IS_TRUE(!!pValue->AsBool());

        value.SetFloat(0.0f);
        pValue = &value;
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
        VERIFY_ARE_EQUAL(valueBool, pValue->GetType());
        VERIFY_IS_FALSE(!!pValue->AsBool());
    }

    void ConversionUnitTests::CanConvertFloatToDouble()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetFlags(MetaDataPropertyInfoFlags::IsSparse);
        dp.SetPropertyTypeIndex(KnownTypeIndex::Double);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.SetFloat(1.5f);
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
        VERIFY_ARE_EQUAL(valueDouble, pValue->GetType());
        VERIFY_ARE_EQUAL(value.AsDouble(), pValue->AsDouble());
    }

    void ConversionUnitTests::CanConvertFloatToEnum()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Visibility);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.SetFloat(1.0f);
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
        VERIFY_IS_TRUE(pValue->IsEnum());
        VERIFY_ARE_EQUAL(static_cast<UINT32>(value.AsFloat()), pValue->AsEnum());
    }

    void ConversionUnitTests::CanConvertFloatToInt()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Int32);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.SetFloat(1.0f);
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
        VERIFY_ARE_EQUAL(valueSigned, pValue->GetType());
        VERIFY_ARE_EQUAL(static_cast<INT32>(value.AsFloat()), pValue->AsSigned());
    }

    void ConversionUnitTests::CanConvertFloatToRect()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Rect);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.SetFloat(3.1f);
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
        VERIFY_ARE_EQUAL(valueRect, pValue->GetType());
        VERIFY_ARE_EQUAL(value.AsFloat(), pValue->AsRect()->X);
        VERIFY_ARE_EQUAL(value.AsFloat(), pValue->AsRect()->Y);
        VERIFY_ARE_EQUAL(value.AsFloat(), pValue->AsRect()->Width);
        VERIFY_ARE_EQUAL(value.AsFloat(), pValue->AsRect()->Height);
    }

    void ConversionUnitTests::CanConvertFloatToDependencyObject()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::DependencyObject);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.SetFloat(2.1f);
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
        VERIFY_ARE_EQUAL(valueObject, pValue->GetType());
        VERIFY_ARE_EQUAL(value.AsFloat(), static_cast<CDouble*>(pValue->AsObject())->m_eValue);
    }

    void ConversionUnitTests::CannotConvertFloatToDateTime()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::DateTime);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.SetFloat(42.0f);
        VERIFY_ARE_EQUAL(E_INVALIDARG, buffer.RepackageValue(&dp, pValue));
    }

    void ConversionUnitTests::CanConvertFloatToThicknessInPlace()
    {
        ValueBuffer buffer(nullptr);
        buffer.GetValuePtr()->SetFloat(2.0);

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Thickness);
        VERIFY_SUCCEEDED(buffer.Repackage(&dp));

        auto thickness = *buffer.GetValuePtr()->AsThickness();
        VERIFY_ARE_EQUAL(thickness.bottom, 2.0);
        VERIFY_ARE_EQUAL(thickness.top, 2.0);
        VERIFY_ARE_EQUAL(thickness.left, 2.0);
        VERIFY_ARE_EQUAL(thickness.right, 2.0);
    }

    void ConversionUnitTests::CanConvertFloatToGridLengthInPlace()
    {
        ValueBuffer buffer(nullptr);
        buffer.GetValuePtr()->SetFloat(2.0);

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::GridLength);
        VERIFY_SUCCEEDED(buffer.Repackage(&dp));

        auto gridLength = *buffer.GetValuePtr()->AsGridLength();
        VERIFY_ARE_EQUAL(gridLength.type, DirectUI::GridUnitType::Pixel);
        VERIFY_ARE_EQUAL(gridLength.value, 2.0);
    }

    void ConversionUnitTests::CanConvertFloatToAutoGridLengthInPlace()
    {
        ValueBuffer buffer(nullptr);
        buffer.GetValuePtr()->SetFloat(static_cast<float>(XDOUBLE_NAN));

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::GridLength);
        VERIFY_SUCCEEDED(buffer.Repackage(&dp));

        auto gridLength = *buffer.GetValuePtr()->AsGridLength();
        VERIFY_ARE_EQUAL(gridLength.type, DirectUI::GridUnitType::Auto);

        // Since one property of NaN is that value comparisons always fail,
        // meaning NaN == NaN is always false, we have to use a utility function
        // to check for NaN.
        VERIFY_IS_TRUE(!!DoubleUtil::IsNaN(gridLength.value));
    }

    void ConversionUnitTests::CanConvertFloatToCornerRadiusInPlace()
    {
        ValueBuffer buffer(nullptr);
        buffer.GetValuePtr()->SetFloat(2.0);

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::CornerRadius);
        VERIFY_SUCCEEDED(buffer.Repackage(&dp));

        auto cornerRadius = *buffer.GetValuePtr()->AsCornerRadius();
        VERIFY_ARE_EQUAL(cornerRadius.bottomLeft, 2.0);
        VERIFY_ARE_EQUAL(cornerRadius.bottomRight, 2.0);
        VERIFY_ARE_EQUAL(cornerRadius.topLeft, 2.0);
        VERIFY_ARE_EQUAL(cornerRadius.topRight, 2.0);
    }

#pragma endregion

#pragma region Conversions from Int32
    void ConversionUnitTests::CanConvertIntToBool()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Boolean);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.SetSigned(1);
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
        VERIFY_ARE_EQUAL(valueBool, pValue->GetType());
        VERIFY_IS_TRUE(!!pValue->AsBool());

        value.SetSigned(0);
        pValue = &value;
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
        VERIFY_ARE_EQUAL(valueBool, pValue->GetType());
        VERIFY_IS_FALSE(!!pValue->AsBool());
    }

    void ConversionUnitTests::CanConvertIntToColor()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Color);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.SetSigned(0xFF00FF);
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
        VERIFY_ARE_EQUAL(valueColor, pValue->GetType());
        VERIFY_ARE_EQUAL(value.AsSigned(), pValue->AsColor());

        value.SetSigned(0);
        pValue = &value;
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
        VERIFY_ARE_EQUAL(valueColor, pValue->GetType());
        VERIFY_ARE_EQUAL(value.AsSigned(), pValue->AsColor());
    }

    void ConversionUnitTests::CanConvertIntToFloat()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Float);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.SetSigned(42);
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
        VERIFY_ARE_EQUAL(valueFloat, pValue->GetType());
        VERIFY_ARE_EQUAL(static_cast<FLOAT>(value.AsSigned()), pValue->AsFloat());
    }

    void ConversionUnitTests::CanConvertIntToDouble()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetFlags(MetaDataPropertyInfoFlags::IsSparse);
        dp.SetPropertyTypeIndex(KnownTypeIndex::Double);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.SetSigned(42);
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
        VERIFY_ARE_EQUAL(valueDouble, pValue->GetType());
        VERIFY_ARE_EQUAL(static_cast<DOUBLE>(value.AsSigned()), pValue->AsDouble());
    }

    void ConversionUnitTests::CannotConvertIntToDateTime()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::DateTime);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.SetSigned(42);
        VERIFY_ARE_EQUAL(E_INVALIDARG, buffer.RepackageValue(&dp, pValue));
    }

    void ConversionUnitTests::CannotConvertIntToDependencyObject()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::DependencyObject);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.SetSigned(42);
        VERIFY_ARE_EQUAL(E_INVALIDARG, buffer.RepackageValue(&dp, pValue));
    }
#pragma endregion

#pragma region Conversions from bool/enum
    void ConversionUnitTests::CanConvertBoolToEnum()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Visibility);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.SetBool(true);
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
        VERIFY_IS_TRUE(pValue->IsEnum());
        VERIFY_ARE_EQUAL((XUINT32)value.AsBool(), pValue->AsEnum());
    }

    void ConversionUnitTests::CanConvertBoolToInt()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Int32);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.SetBool(true);
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
        VERIFY_ARE_EQUAL(valueSigned, pValue->GetType());
        VERIFY_ARE_EQUAL((XINT32)value.AsBool(), pValue->AsSigned());
    }

    void ConversionUnitTests::CanConvertEnumToBool()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Boolean);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.SetEnum(1);
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
        VERIFY_ARE_EQUAL(valueBool, pValue->GetType());
        VERIFY_ARE_EQUAL(value.AsEnum(), (XUINT32)pValue->AsBool());
    }

    void ConversionUnitTests::CanConvertEnumToInt()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Int32);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.SetEnum(1);
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
        VERIFY_ARE_EQUAL(valueSigned, pValue->GetType());
        VERIFY_ARE_EQUAL(value.AsEnum(), pValue->AsSigned());
    }

    void ConversionUnitTests::CannotConvertBoolToDependencyObject()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::DependencyObject);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.SetBool(true);
        VERIFY_ARE_EQUAL(E_INVALIDARG, buffer.RepackageValue(&dp, pValue));
    }

    void ConversionUnitTests::CannotConvertEnumToDependencyObject()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::DependencyObject);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.SetEnum(1);
        VERIFY_ARE_EQUAL(E_INVALIDARG, buffer.RepackageValue(&dp, pValue));
    }
#pragma endregion

#pragma region Conversions from string
    void ConversionUnitTests::CanConvertStringToBool()
    {
        DECLARE_CONST_STRING_IN_TEST_CODE(trueString, L"True");
        DECLARE_CONST_STRING_IN_TEST_CODE(falseString, L"False");

        auto mock = TlsProvider<ClassInfoCallbacks>::CreateWrappedObject();
        mock->GetCoreConstructor = [](const CClassInfo* classInfo) -> CREATEPFN
        {
            if (classInfo->GetIndex() == KnownTypeIndex::Boolean)
            {
                return &CBoolean::Create;
            }
            return nullptr;
        };

        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Boolean);

        ValueBuffer buffer(obj->GetContext());

        CValue value;

        CValue* pResult = nullptr;

        // Convert "True" to a boolean.
        value.SetString(trueString);
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, &value, &pResult));
        VERIFY_ARE_EQUAL(valueBool, pResult->GetType());
        VERIFY_IS_TRUE(!!pResult->AsBool());

        // Convert "False" to a boolean.
        value.SetString(falseString);
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, &value, &pResult));
        VERIFY_ARE_EQUAL(valueBool, pResult->GetType());
        VERIFY_IS_FALSE(!!pResult->AsBool());
    }

    void ConversionUnitTests::ConversionFromBadStringToBool()
    {
        WaitForDebugger();

        DECLARE_CONST_STRING_IN_TEST_CODE(helloString, L"Hello");

        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Boolean);
        dp.SetFlags(MetaDataPropertyInfoFlags::IsSparse);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        value.SetString(helloString);

        CValue* pValue = &value;

        // Verify behavior on latest Windows version: fail on bad strings.
        VERIFY_FAILED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
    }
#pragma endregion

#pragma region Conversions from double
    void ConversionUnitTests::CanConvertDoubleToGridLength()
    {
        ValueBuffer buffer(nullptr);
        buffer.GetValuePtr()->SetDouble(3.0);

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::GridLength);
        VERIFY_SUCCEEDED(buffer.Repackage(&dp));

        auto gridLength = *buffer.GetValuePtr()->AsGridLength();
        VERIFY_ARE_EQUAL(gridLength.type, DirectUI::GridUnitType::Pixel);
        VERIFY_ARE_EQUAL(gridLength.value, 3.0f);
    }

    void ConversionUnitTests::CanConvertDoubleToAutoGridLength()
    {
        ValueBuffer buffer(nullptr);
        buffer.GetValuePtr()->SetDouble(XDOUBLE_NAN);

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::GridLength);
        VERIFY_SUCCEEDED(buffer.Repackage(&dp));

        auto gridLength = *buffer.GetValuePtr()->AsGridLength();
        VERIFY_ARE_EQUAL(gridLength.type, DirectUI::GridUnitType::Auto);

        // Since one property of NaN is that value comparisons always fail,
        // meaning NaN == NaN is always false, we have to use a utility function
        // to check for NaN.
        VERIFY_IS_TRUE(!!DoubleUtil::IsNaN(gridLength.value));
    }

#pragma endregion
    void ConversionUnitTests::CannotConvertObjectToString()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        xref_ptr<CDependencyObject> reference;
        reference.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::String);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.WrapObjectNoRef(reference.get());
        VERIFY_ARE_EQUAL(E_FAIL, buffer.RepackageValue(&dp, pValue));

        // However... we do accept a null object, which simply converts into a null string
        value.WrapObjectNoRef(nullptr);
        VERIFY_SUCCEEDED(buffer.RepackageValue(&dp, pValue));
        VERIFY_ARE_EQUAL(valueString, buffer.GetValuePtr()->GetType());
        VERIFY_IS_TRUE(!!buffer.GetValuePtr()->AsString().IsNull());
    }

    void ConversionUnitTests::DoubleGetsConvertedToFloat()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Double);
        dp.SetFlags(MetaDataPropertyInfoFlags::StoreDoubleAsFloat);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.SetSigned(42);
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
        VERIFY_ARE_EQUAL(valueFloat, pValue->GetType());
        VERIFY_ARE_EQUAL(static_cast<FLOAT>(value.AsSigned()), pValue->AsFloat());
    }

    void ConversionUnitTests::IntObjectGetsConvertedToIntCValue()
    {
        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        xref_ptr<CInt32> intValue;
        intValue.attach(new CInt32());
        intValue->m_iValue = 42;

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::Int32);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.WrapObjectNoRef(intValue.get());
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
        VERIFY_ARE_EQUAL(valueSigned, pValue->GetType());
        VERIFY_ARE_EQUAL(intValue->m_iValue, pValue->AsSigned());
    }

    void ConversionUnitTests::StringObjectGetsConvertedToStringCValue()
    {
        DECLARE_CONST_STRING_IN_TEST_CODE(myString, L"myString");

        xref_ptr<CDependencyObject> obj;
        obj.attach(new CDependencyObject());

        xref_ptr<CString> stringValue;
        stringValue.attach(new CString(/* core */ nullptr, myString));

        MockDependencyProperty dp;
        dp.SetPropertyTypeIndex(KnownTypeIndex::String);

        ValueBuffer buffer(obj->GetContext());

        CValue value;
        CValue* pValue = &value;

        value.WrapObjectNoRef(stringValue.get());
        VERIFY_SUCCEEDED(buffer.RepackageValueAndSetPtr(&dp, pValue, &pValue));
        VERIFY_ARE_EQUAL(valueString, pValue->GetType());
        VERIFY_ARE_EQUAL(stringValue->m_strString, pValue->AsString());
    }

} } } } } }
