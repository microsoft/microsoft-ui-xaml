// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <windows.web.h>
#include "ValueBoxerTests.h"
#include <valueboxer\inc\ValueBoxer.h>
#include <EnumDefs.g.h>
#include "CStaticLock.h"
#include "MetadataAPI.h"
#include "PropertyValueWrapper.h"
#include "DurationVO.h"
#include "RepeatBehaviorVO.h"

using namespace WEX::Common;
using namespace DirectUI;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace ValueBoxer {

    bool BasicValueBoxerUnitTests::ClassSetup()
    {
        THROW_IF_FAILED(DirectUI::StaticLockGlobalInit());
        return true;
    }

    bool operator==(const xaml::Duration& lhs, const xaml::Duration& rhs)
    {
        return !(lhs.Type != rhs.Type ||
                (lhs.Type == xaml::DurationType::DurationType_TimeSpan && lhs.TimeSpan.Duration != rhs.TimeSpan.Duration));
    }

    bool operator==(const xaml::CornerRadius& lhs, const xaml::CornerRadius& rhs)
    {
        return (lhs.BottomLeft == rhs.BottomLeft) &&
               (lhs.BottomRight == rhs.BottomRight) &&
               (lhs.TopLeft == rhs.TopLeft) &&
               (lhs.TopRight == rhs.TopRight);
    }

    bool operator==(const  wf::DateTime& lhs, const wf::DateTime& rhs)
    {
        return lhs.UniversalTime == rhs.UniversalTime;
    }

    bool operator==(const xaml_animation::RepeatBehavior& lhs, const xaml_animation::RepeatBehavior& rhs)
    {
        if (lhs.Type != rhs.Type)
        {
            return false;
        }

        switch (lhs.Type)
        {
            case xaml_animation::RepeatBehaviorType::RepeatBehaviorType_Count:
                return lhs.Count == rhs.Count;

            case xaml_animation::RepeatBehaviorType::RepeatBehaviorType_Duration:
                return lhs.Duration.Duration == rhs.Duration.Duration;
        }

        return false;
    }

    bool operator==(const  xaml_media::Media3D::Matrix3D& lhs, const xaml_media::Media3D::Matrix3D& rhs)
    {
        return lhs.M11 == rhs.M11 &&
               lhs.M12 == rhs.M12 &&
               lhs.M13 == rhs.M13 &&
               lhs.M14 == rhs.M14 &&
               lhs.M21 == rhs.M21 &&
               lhs.M22 == rhs.M22 &&
               lhs.M23 == rhs.M23 &&
               lhs.M24 == rhs.M24 &&
               lhs.M31 == rhs.M31 &&
               lhs.M32 == rhs.M32 &&
               lhs.M33 == rhs.M33 &&
               lhs.M34 == rhs.M34 &&
               lhs.OffsetX == rhs.OffsetX &&
               lhs.OffsetY == rhs.OffsetY &&
               lhs.OffsetZ == rhs.OffsetZ &&
               lhs.M44 == rhs.M44;
    }

    template <typename T>
    static void Validate_CopyValue(const T& in)
    {
        T out;
        VERIFY_ARE_EQUAL(CValueBoxer::CopyValue(in, &out), S_OK);
        VERIFY_ARE_EQUAL(memcmp(&in, &out, sizeof(T)), 0);
    }

    template<typename T>
    static void ValidateBox(const T& value, std::function<void(const CValue&)> validateFn)
    {
        CValue box;
        BoxerBuffer buf;

        THROW_IF_FAILED(CValueBoxer::BoxValue(&box, value, &buf));

        validateFn(box);
    }

    template<typename T>
    static void ValidateUnbox(const T& value, std::function<CValue()> setupFn)
    {
        CValue box = setupFn();
        T unboxedValue;

        THROW_IF_FAILED(CValueBoxer::UnboxValue(&box, &unboxedValue));

        VERIFY_IS_TRUE(value == unboxedValue);
    }

    template<typename T>
    static void ValidateBoxViaIReference(const T& value, std::function<void(const CValue&)> validateFn)
    {
        CValue box;
        BoxerBuffer buf;
        ctl::ComPtr<IInspectable> spValue;

        THROW_IF_FAILED(PropertyValue::CreateReference(value, &spValue));
        THROW_IF_FAILED(CValueBoxer::BoxValue<T>(&box, spValue.Get(), &buf));

        validateFn(box);
    }

    template<typename T>
    static void ValidateUnboxViaIReference(const T& value, std::function<CValue()> setupFn)
    {
        CValue box = setupFn();
        ctl::ComPtr<wf::IReference<T>> spOutValue;

        THROW_IF_FAILED(CValueBoxer::UnboxValue<T>(&box, spOutValue.ReleaseAndGetAddressOf()));

        T unboxedValue;
        THROW_IF_FAILED(spOutValue->get_Value(&unboxedValue));

        VERIFY_IS_TRUE(value == unboxedValue);
    }

    void BasicValueBoxerUnitTests::Validate_CValueBoxer_CopyValue()
    {
        Validate_CopyValue<BOOLEAN>(TRUE);
        Validate_CopyValue<INT>(-55);
        Validate_CopyValue<UINT>(65530);
        Validate_CopyValue<ABI::Windows::Web::WebErrorStatus>(ABI::Windows::Web::WebErrorStatus_Timeout);
    }

    void BasicValueBoxerUnitTests::Validate_CValueBoxer_BoxUnboxSimple()
    {
        {
            // BOOLEAN

            CValue box;
            BoxerBuffer buf;
            const BOOLEAN inValue = TRUE;
            bool boxCheck = false;

            THROW_IF_FAILED(CValueBoxer::BoxValue(&box, inValue, &buf));
            THROW_IF_FAILED(box.GetBool(boxCheck));
            VERIFY_ARE_EQUAL(boxCheck, !!inValue);

            BOOLEAN boxCheck2 = FALSE;
            THROW_IF_FAILED(CValueBoxer::UnboxValue(&box, &boxCheck2));
            VERIFY_ARE_EQUAL(boxCheck2, inValue);
        }

        {
            // INT64

            CValue box;
            BoxerBuffer buf;
            const int64_t inValue = 99;
            int64_t boxCheck = 0;

            THROW_IF_FAILED(CValueBoxer::BoxValue(&box, inValue, &buf));
            THROW_IF_FAILED(box.GetInt64(boxCheck));
            VERIFY_ARE_EQUAL(boxCheck, inValue);

            boxCheck = 0;
            THROW_IF_FAILED(CValueBoxer::UnboxValue(&box, &boxCheck));
            VERIFY_ARE_EQUAL(boxCheck, inValue);
        }
    }

    void BasicValueBoxerUnitTests::Validate_CValueBoxer_BoxUnboxTextRange()
    {
        {
            // TextRangeData
            CValue box;
            BoxerBuffer buf;
            const xaml_docs::TextRange inValue = { 10, 20 };
            TextRangeData boxCheck = {};

            THROW_IF_FAILED(CValueBoxer::BoxValue(&box, inValue, &buf));
            THROW_IF_FAILED(box.Get<valueTextRange>(boxCheck));
            VERIFY_ARE_EQUAL(boxCheck.startIndex, inValue.StartIndex);
            VERIFY_ARE_EQUAL(boxCheck.length, inValue.Length);

            xaml_docs::TextRange outValue = {};
            THROW_IF_FAILED(CValueBoxer::UnboxValue(&box, &outValue));
            VERIFY_ARE_EQUAL(outValue.StartIndex, inValue.StartIndex);
            VERIFY_ARE_EQUAL(outValue.Length, inValue.Length);
        }
    }

    void BasicValueBoxerUnitTests::Validate_CValueBoxer_BoxUnboxEnum()
    {
        {
            // Box/unbox numeric enum

            CValue box;
            uint32_t boxCheck = 0;

            THROW_IF_FAILED(CValueBoxer::BoxEnumValue(&box, 99));
            THROW_IF_FAILED(box.GetEnum(boxCheck));
            VERIFY_ARE_EQUAL(boxCheck, 99);

            boxCheck = 0;
            THROW_IF_FAILED(CValueBoxer::UnboxEnumValue(&box, nullptr, &boxCheck));
            VERIFY_ARE_EQUAL(boxCheck, 99);
        }

        {
            // Box enum with resolution.

            CValue box;
            wrl_wrappers::HStringReference strName(L"Collapsed");
            uint32_t boxCheck = 0;
            uint32_t length;
            auto strNameBuf = strName.GetRawBuffer(&length);
            THROW_IF_FAILED(CValueBoxer::BoxEnumValue(
                &box,
                strNameBuf,
                length,
                MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Visibility)));

            THROW_IF_FAILED(box.GetEnum(boxCheck));
            VERIFY_ARE_EQUAL(boxCheck, 1);
        }

        {
            // Unbox enum with resolution.

            CValue box;
            wrl_wrappers::HStringReference strName(L"Collapsed");
            uint32_t boxCheck = 0;

            THROW_IF_FAILED(box.SetString(strName.Get()));

            THROW_IF_FAILED(CValueBoxer::UnboxEnumValue(
                &box,
                MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Visibility),
                &boxCheck));

            VERIFY_ARE_EQUAL(boxCheck, 1);
        }
    }

    void BasicValueBoxerUnitTests::Validate_CValueBoxer_BoxUnboxDuration()
    {
        xaml::Duration value = {};

        // Box Duration

        value.Type = xaml::DurationType::DurationType_Automatic;
        ValidateBox(
            value,
            [](const CValue& box)
        {
            DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(strAutomatic, L"Automatic");
            xstring_ptr result;
            THROW_IF_FAILED(box.GetString(result));
            VERIFY_IS_TRUE(!!result.Equals(strAutomatic));
        });

        value.Type = xaml::DurationType::DurationType_Forever;
        ValidateBox(
            value,
            [](const CValue& box)
        {
            DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(strForever, L"Forever");
            xstring_ptr result;
            THROW_IF_FAILED(box.GetString(result));
            VERIFY_IS_TRUE(!!result.Equals(strForever));
        });

        value.Type = xaml::DurationType::DurationType_TimeSpan;
        value.TimeSpan.Duration = 999;
        ValidateBox(
            value,
            [](const CValue& box)
        {
            wf::TimeSpan result;
            THROW_IF_FAILED(box.GetTimeSpan(result));
            VERIFY_ARE_EQUAL(result.Duration, 999);
        });

        // Unbox Duration

        value.Type = xaml::DurationType::DurationType_Automatic;
        ValidateUnbox(
            value,
            []()
        {
            CValue result;
            result.SetNull();
            return result;
        });

        value.Type = xaml::DurationType::DurationType_Automatic;
        ValidateUnbox(
            value,
            []()
        {
            DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(strAutomatic, L"Automatic");
            CValue result;
            result.SetString(strAutomatic);
            return result;
        });

        value.Type = xaml::DurationType::DurationType_Forever;
        ValidateUnbox(
            value,
            []()
        {
            DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(strForever, L"Forever");
            CValue result;
            result.SetString(strForever);
            return result;
        });
    }

    void BasicValueBoxerUnitTests::Validate_CValueBoxer_BoxIFontFamily()
    {
        struct DummyFontFamily : public xaml_media::IFontFamily
        {
            IFACEMETHOD(get_Source)(_Out_ HSTRING* pValue) override
            {
                wrl_wrappers::HString value;
                value.Set(L"Some_value");
                *pValue = value.Detach();
                return S_OK;
            }

            STDMETHOD(QueryInterface)(THIS_ IN REFIID, OUT PVOID *) { return 0; }
            STDMETHOD_(ULONG, AddRef)(THIS) { return 0; }
            STDMETHOD_(ULONG, Release)(THIS) { return 0; }
            STDMETHOD(GetIids)(_Out_ ULONG *iidCount, _Outptr_result_buffer_maybenull_(*iidCount) IID **iids) { return 0; }
            STDMETHOD(GetTrustLevel)(_Out_ TrustLevel *trustLvl) 
            {
                *trustLvl = TrustLevel::BaseTrust;
                return 0;
            }
            STDMETHOD(GetRuntimeClassName)(_Out_ HSTRING *pClassName) 
            {
                *pClassName = nullptr;
                return 0;
            }
        };

        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(strSource, L"Some_value");
        std::unique_ptr<xaml_media::IFontFamily> fontFamily(new DummyFontFamily());
        BoxerBuffer buf;
        CValue box;

        THROW_IF_FAILED(CValueBoxer::BoxValue(&box, fontFamily.get(), &buf));

        xstring_ptr result;
        THROW_IF_FAILED(box.GetString(result));
        VERIFY_IS_TRUE(!!result.Equals(strSource));
    }

    void BasicValueBoxerUnitTests::Validate_CValueBoxer_BoxUnboxViaIReference()
    {
        {
            // CornerRadius

            xaml::CornerRadius value;

            value.BottomLeft = 1;
            value.BottomRight = 2;
            value.TopLeft = 3;
            value.TopRight = 4;
            ValidateBoxViaIReference(
                value,
                [value](const CValue& box)
                {
                    XCORNERRADIUS* coreResult = nullptr;
                    THROW_IF_FAILED(box.GetCornerRadius(coreResult));

                    xaml::CornerRadius result;
                    result.BottomLeft = coreResult->bottomLeft;
                    result.BottomRight = coreResult->bottomRight;
                    result.TopLeft = coreResult->topLeft;
                    result.TopRight = coreResult->topRight;

                    VERIFY_IS_TRUE(value == result);
                });

            XCORNERRADIUS coreValue;
            coreValue.bottomLeft = (XFLOAT)value.BottomLeft;
            coreValue.bottomRight = (XFLOAT)value.BottomRight;
            coreValue.topLeft = (XFLOAT)value.TopLeft;
            coreValue.topRight = (XFLOAT)value.TopRight;

            ValidateUnboxViaIReference(
                value,
                [&coreValue]()
                {
                    CValue result;
                    result.WrapCornerRadius(&coreValue);
                    return result;
                });
        }

        {
            // Duration

            xaml::Duration value;

            value.Type = xaml::DurationType::DurationType_Automatic;
            ValidateBoxViaIReference(
                value,
                [](const CValue& box)
            {
                DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(strAutomatic, L"Automatic");
                xstring_ptr result;
                THROW_IF_FAILED(box.GetString(result));
                VERIFY_IS_TRUE(!!result.Equals(strAutomatic));
            });

            ValidateUnboxViaIReference(
                value,
                []()
            {
                DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(strAutomatic, L"Automatic");
                CValue result;
                result.SetString(strAutomatic);
                return result;
            });
        }

        {
            // RepeatBehavior

            xaml_animation::RepeatBehavior value;
            value.Type = xaml_animation::RepeatBehaviorType::RepeatBehaviorType_Count;
            value.Count = 22;

            ValidateBoxViaIReference(
                value,
                [](const CValue& box)
            {
                DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(strCount, L"22.000000x");
                xstring_ptr result;
                THROW_IF_FAILED(box.GetString(result));
                VERIFY_IS_TRUE(!!result.Equals(strCount));
            });

            ValidateUnboxViaIReference(
                value,
                []()
            {
                DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(strCount, L"22x");
                CValue result;
                result.SetString(strCount);
                return result;
            });
        }

        {
            // Matrix3D

            xaml_media::Media3D::Matrix3D value;

            value.M11 = 11.0;
            value.M12 = 12.0;
            value.M13 = 13.0;
            value.M14 = 14.0;
            value.M21 = 21.0;
            value.M22 = 22.0;
            value.M23 = 23.0;
            value.M24 = 24.0;
            value.M31 = 31.0;
            value.M32 = 32.0;
            value.M33 = 33.0;
            value.M34 = 34.0;
            value.OffsetX = 41.0;
            value.OffsetY = 42.0;
            value.OffsetZ = 43.0;
            value.M44 = 44.0;

            ValidateBoxViaIReference(
                value,
                [value](const CValue& box)
                {
                    XMATRIX3D* coreValue = nullptr;
                    unsigned arrayLength = 0;
                    THROW_IF_FAILED(box.GetFloatArray(
                        reinterpret_cast<XFLOAT*&>(coreValue),
                        &arrayLength));
                    VERIFY_IS_TRUE(arrayLength == 16);

                    xaml_media::Media3D::Matrix3D temp;

                    temp.M11 = coreValue->m11;
                    temp.M12 = coreValue->m12;
                    temp.M13 = coreValue->m13;
                    temp.M14 = coreValue->m14;
                    temp.M21 = coreValue->m21;
                    temp.M22 = coreValue->m22;
                    temp.M23 = coreValue->m23;
                    temp.M24 = coreValue->m24;
                    temp.M31 = coreValue->m31;
                    temp.M32 = coreValue->m32;
                    temp.M33 = coreValue->m33;
                    temp.M34 = coreValue->m34;
                    temp.OffsetX = coreValue->offsetX;
                    temp.OffsetY = coreValue->offsetY;
                    temp.OffsetZ = coreValue->offsetZ;
                    temp.M44 = coreValue->m44;

                    VERIFY_IS_TRUE(temp == value);
                });

            XMATRIX3D coreValue;

            coreValue.m11 = (XFLOAT)value.M11;
            coreValue.m12 = (XFLOAT)value.M12;
            coreValue.m13 = (XFLOAT)value.M13;
            coreValue.m14 = (XFLOAT)value.M14;
            coreValue.m21 = (XFLOAT)value.M21;
            coreValue.m22 = (XFLOAT)value.M22;
            coreValue.m23 = (XFLOAT)value.M23;
            coreValue.m24 = (XFLOAT)value.M24;
            coreValue.m31 = (XFLOAT)value.M31;
            coreValue.m32 = (XFLOAT)value.M32;
            coreValue.m33 = (XFLOAT)value.M33;
            coreValue.m34 = (XFLOAT)value.M34;
            coreValue.offsetX = (XFLOAT)value.OffsetX;
            coreValue.offsetY = (XFLOAT)value.OffsetY;
            coreValue.offsetZ = (XFLOAT)value.OffsetZ;
            coreValue.m44 = (XFLOAT)value.M44;

            ValidateUnboxViaIReference(
                value,
                [&coreValue]()
                {
                    CValue result;
                    result.WrapFloatArray(16, reinterpret_cast<XFLOAT*>(&coreValue));
                    return result;
                });
        }
    }

    void BasicValueBoxerUnitTests::Validate_CValueBoxer_ConvertToFramework()
    {
        {
            // VirtualKeyModifiers

            DirectUI::VirtualKeyModifiers coreValue = DirectUI::VirtualKeyModifiers::Menu;
            wsy::VirtualKeyModifiers fxValue = static_cast<wsy::VirtualKeyModifiers>(9999);
            THROW_IF_FAILED(CValueBoxer::ConvertToFramework(coreValue, &fxValue, FALSE));
            VERIFY_ARE_EQUAL(static_cast<int>(coreValue), static_cast<int>(fxValue));
        }
    }

    void BasicValueBoxerUnitTests::Validate_IValueBoxer_BoxUnboxSimple()
    {
        PropertyValueWrapper init;

        {
            // BOOLEAN

            ctl::ComPtr<IInspectable> spOut;
            BOOLEAN value = TRUE;

            THROW_IF_FAILED(IValueBoxer::BoxValue(spOut.ReleaseAndGetAddressOf(), value));

            BOOLEAN unboxedValue = FALSE;
            THROW_IF_FAILED(IValueBoxer::UnboxValue(spOut.Get(), &unboxedValue));

            VERIFY_ARE_EQUAL(value, unboxedValue);
        }

        {
            // DateTime

            ctl::ComPtr<IInspectable> spOut;
            wf::DateTime value;

            value.UniversalTime = 9939;

            THROW_IF_FAILED(IValueBoxer::BoxValue(spOut.ReleaseAndGetAddressOf(), value));

            wf::DateTime unboxedValue;
            THROW_IF_FAILED(IValueBoxer::UnboxValue(spOut.Get(), &unboxedValue));

            VERIFY_IS_TRUE(value == unboxedValue);
        }
    }
} } } } }
