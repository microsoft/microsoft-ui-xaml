// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

namespace Private
{
    /* static */
    BOOLEAN ValueConversionHelpers::CanConvertValueToString(
        _In_ wf::PropertyType propertyType)
    {
        switch (propertyType)
        {
            // Keep this in sync with ConvertValueToString().

            case wf::PropertyType_UInt8:
            case wf::PropertyType_Int16:
            case wf::PropertyType_UInt16:
            case wf::PropertyType_Int32:
            case wf::PropertyType_UInt32:
            case wf::PropertyType_Int64:
            case wf::PropertyType_UInt64:
            case wf::PropertyType_Single:
            case wf::PropertyType_Double:
            case wf::PropertyType_Char16:
            case wf::PropertyType_Boolean:
            case wf::PropertyType_String:
            case wf::PropertyType_Guid:
                return TRUE;

            default:
                return FALSE;
        }
    }

    /* static */
    _Check_return_ HRESULT
    ValueConversionHelpers::ConvertValueToString(
        _In_ wf::IPropertyValue* pPropertyValue,
        _In_ wf::PropertyType propertyType,
        _Out_ HSTRING *pResult)
    {
        HRESULT hr = S_OK;
        std::wstringstream output;
        BOOLEAN createString = TRUE;

        *pResult = nullptr;

        switch (propertyType)
        {
           // Keep this in sync with CanConvertValueToString().

           case wf::PropertyType_UInt8:
           {
               BYTE value = 0;
               IFC(pPropertyValue->GetUInt8(&value));
               output << value;
               break;
           }

           case wf::PropertyType_Int16:
           {
               INT16 value = 0;
               IFC(pPropertyValue->GetInt16(&value));
               output << value;
               break;
           }

           case wf::PropertyType_UInt16:
           {
               UINT16 value = 0;
               IFC(pPropertyValue->GetUInt16(&value));
               output << value;
               break;
           }

           case wf::PropertyType_Int32:
           {
               INT32 value = 0;
               IFC(pPropertyValue->GetInt32(&value));
               output << value;
               break;
           }

           case wf::PropertyType_UInt32:
           {
               UINT32 value = 0;
               IFC(pPropertyValue->GetUInt32(&value));
               output << value;
               break;
           }

           case wf::PropertyType_Int64:
           {
               INT64 value = 0;
               IFC(pPropertyValue->GetInt64(&value));
               output << value;
               break;
           }

           case wf::PropertyType_UInt64:
           {
               UINT64 value = 0;
               IFC(pPropertyValue->GetUInt64(&value));
               output << value;
               break;
           }

           case wf::PropertyType_Single:
           {
               FLOAT value = 0;
               IFC(pPropertyValue->GetSingle(&value));
               output << value;
               break;
           }

           case wf::PropertyType_Double:
           {
               DOUBLE value = 0;
               IFC(pPropertyValue->GetDouble(&value));
               output << value;
               break;
           }

           case wf::PropertyType_Char16:
           {
               WCHAR value = 0;
               IFC(pPropertyValue->GetChar16(&value));
               output << value;
               break;
           }

           case wf::PropertyType_Boolean:
           {
               BOOLEAN value = FALSE;
               IFC(pPropertyValue->GetBoolean(&value));
               output << value;
               break;
           }

           case wf::PropertyType_String:
           {
               IFC(pPropertyValue->GetString(pResult));
               createString = FALSE;
               break;
           }

           case wf::PropertyType_Guid:
           {
               GUID value;
               IFC(pPropertyValue->GetGuid(&value));
               IFC(ConvertGuidToString(value, pResult));
               createString = FALSE;
               break;
           }

           default:
           {
               IFC(E_FAIL);
               break;
           }
        }

        if (createString)
        {
            std::wstring outputString;
            outputString = output.str();
            IFC(wrl_wrappers::HStringReference(outputString.c_str(), static_cast<UINT32>(outputString.size())).CopyTo(pResult));
        }

    Cleanup:
        RRETURN(hr);
    }

    /* static */
    _Check_return_ HRESULT
    ValueConversionHelpers::ConvertGuidToString(
        _In_ GUID guid,
        _Out_ HSTRING *pResult)
    {
        HRESULT hr = S_OK;
        const INT CLSID_STRING_SIZE = 39; // size of a guid string including null character {########-####-####-####-############}\0
        WCHAR guidString[CLSID_STRING_SIZE];

        if (StringFromGUID2(guid, guidString, CLSID_STRING_SIZE) == 0)
        {
            IFC(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
        }
        IFC(WindowsCreateString(guidString, CLSID_STRING_SIZE, pResult));

    Cleanup:
        RRETURN(hr);
    }

    //---------------------------------------------------------------------------------------------
    // Static Function: ValueComparisonHelpers::AreEqual
    //
    // Does a deep comparizon of two IInspectables. WinRT objects object typically have multiple
    // IInspectable implementations with different addresses (one for each WinRT interface the object
    // implements. This method will return true if two different IInpsectables representing the
    // same WinRT object are specified.
    // If both objects specified implement IReference, this method will compare the boxed values
    // and return true if they are equal.
    //---------------------------------------------------------------------------------------------
    _Check_return_ HRESULT
    ValueComparisonHelpers::AreEqual(
        _In_ IInspectable* pValue1,
        _In_ IInspectable* pValue2,
        _Out_ BOOLEAN* pAreEqual)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<IInspectable> spValue1;
        wrl::ComPtr<IInspectable> spValue2;
        wf::PropertyType value1Type = wf::PropertyType_Empty;
        wf::PropertyType value2Type = wf::PropertyType_Empty;
        wrl::ComPtr<wf::IPropertyValue> spValue1AsPV;
        wrl::ComPtr<wf::IPropertyValue> spValue2AsPV;

        *pAreEqual = FALSE;

        if (pValue1 == nullptr || pValue2 == nullptr)
        {
            *pAreEqual = pValue1 == pValue2;
            goto Cleanup;
        }

        IFC(pValue1->QueryInterface(__uuidof(IInspectable), &spValue1));
        IFC(pValue2->QueryInterface(__uuidof(IInspectable), &spValue2));
        if (spValue1 == spValue2)
        {
            // identity IInspectable pointers are equal, so the two values represent the same object.
            *pAreEqual = TRUE;
            goto Cleanup;
        }

        IFC(spValue1.As(&spValue1AsPV));
        IFC(spValue2.As(&spValue2AsPV));

        // If we're not dealing with PropertyValues, then
        // the objects aren't equal, because we already
        // did a reference check.
        if (!spValue1AsPV || !spValue2AsPV)
        {
            goto Cleanup;
        }

        // If the types don't match, then they're not equal
        IFC(spValue1AsPV->get_Type(&value1Type));
        IFC(spValue2AsPV->get_Type(&value2Type));
        if (value1Type != value2Type)
        {
            goto Cleanup;
        }

        switch (value1Type)
        {
            case wf::PropertyType_String:
            {
                wrl_wrappers::HString strOldString;
                wrl_wrappers::HString strNewString;

                IFC(spValue1AsPV->GetString(strOldString.GetAddressOf()));
                IFC(spValue2AsPV->GetString(strNewString.GetAddressOf()));

                *pAreEqual = strOldString == strNewString;
                break;
            }
            case wf::PropertyType_Char16:
            {
                WCHAR oldChar = 0;
                WCHAR newChar = 0;

                IFC(spValue1AsPV->GetChar16(&oldChar));
                IFC(spValue2AsPV->GetChar16(&newChar));

                *pAreEqual = oldChar == newChar;
                break;
            }
            case wf::PropertyType_Boolean:
            {
                BOOLEAN oldValueBool = FALSE;
                BOOLEAN newValueBool = FALSE;

                IFC(spValue1AsPV->GetBoolean(&oldValueBool));
                IFC(spValue2AsPV->GetBoolean(&newValueBool));

                *pAreEqual = oldValueBool == newValueBool;
                break;
            }
            case wf::PropertyType_UInt8:
            {
                UINT8 oldValueUint8 = 0;
                UINT8 newValueUint8 = 0;

                IFC(spValue1AsPV->GetUInt8(&oldValueUint8));
                IFC(spValue2AsPV->GetUInt8(&newValueUint8));

                *pAreEqual = oldValueUint8 == newValueUint8;
                break;
            }
            case wf::PropertyType_Int16:
            {
                INT16 oldValueInt16 = 0;
                INT16 newValueInt16 = 0;

                IFC(spValue1AsPV->GetInt16(&oldValueInt16));
                IFC(spValue2AsPV->GetInt16(&newValueInt16));

                *pAreEqual = oldValueInt16 == newValueInt16;
                break;
            }
            case wf::PropertyType_UInt16:
            {
                UINT16 oldValueUint16 = 0;
                UINT16 newValueUint16 = 0;

                IFC(spValue1AsPV->GetUInt16(&oldValueUint16));
                IFC(spValue2AsPV->GetUInt16(&newValueUint16));

                *pAreEqual = oldValueUint16 == newValueUint16;
                break;
            }
            case wf::PropertyType_Int32:
            {
                INT32 oldValueInt32 = 0;
                INT32 newValueInt32 = 0;

                IFC(spValue1AsPV->GetInt32(&oldValueInt32));
                IFC(spValue2AsPV->GetInt32(&newValueInt32));

                *pAreEqual = oldValueInt32 == newValueInt32;
                break;
            }
            case wf::PropertyType_UInt32:
            {
                UINT32 oldValueUint32 = 0;
                UINT32 newValueUint32 = 0;

                IFC(spValue1AsPV->GetUInt32(&oldValueUint32));
                IFC(spValue2AsPV->GetUInt32(&newValueUint32));

                *pAreEqual = oldValueUint32 == newValueUint32;
                break;
            }
            case wf::PropertyType_Int64:
            {
                INT64 oldValueInt64 = 0;
                INT64 newValueInt64 = 0;

                IFC(spValue1AsPV->GetInt64(&oldValueInt64));
                IFC(spValue2AsPV->GetInt64(&newValueInt64));

                *pAreEqual = oldValueInt64 == newValueInt64;
                break;
            }
            case wf::PropertyType_UInt64:
            {
                UINT64 oldValueUint64 = 0;
                UINT64 newValueUint64 = 0;

                IFC(spValue1AsPV->GetUInt64(&oldValueUint64));
                IFC(spValue2AsPV->GetUInt64(&newValueUint64));

                *pAreEqual = oldValueUint64 == newValueUint64;
                break;
            }
            case wf::PropertyType_Single:
            {
                FLOAT oldValueFloat = 0.0f;
                FLOAT newValueFloat = 0.0f;

                IFC(spValue1AsPV->GetSingle(&oldValueFloat));
                IFC(spValue2AsPV->GetSingle(&newValueFloat));

                *pAreEqual = oldValueFloat == newValueFloat;
                break;
            }
            case wf::PropertyType_Double:
            {
                DOUBLE oldValueDouble = 0;
                DOUBLE newValueDouble = 0;

                IFC(spValue1AsPV->GetDouble(&oldValueDouble));
                IFC(spValue2AsPV->GetDouble(&newValueDouble));

                *pAreEqual = oldValueDouble == newValueDouble;
                break;
            }
            case wf::PropertyType_Guid:
            {
                GUID oldValueGuid = {};
                GUID newValueGuid = {};

                IFC(spValue1AsPV->GetGuid(&oldValueGuid));
                IFC(spValue2AsPV->GetGuid(&newValueGuid));

                *pAreEqual = oldValueGuid == newValueGuid;
                break;
            }
            case wf::PropertyType_DateTime:
            {
                wf::DateTime oldValueDateTime = {};
                wf::DateTime newValueDateTime = {};

                IFC(spValue1AsPV->GetDateTime(&oldValueDateTime));
                IFC(spValue2AsPV->GetDateTime(&newValueDateTime));

                *pAreEqual = oldValueDateTime.UniversalTime == newValueDateTime.UniversalTime;
                break;
            }
            case wf::PropertyType_TimeSpan:
            {
                wrl::ComPtr<wf::IReference<wf::TimeSpan>> spTimeSpanRef;
                wf::TimeSpan value1TimeSpan = {};
                wf::TimeSpan value2TimeSpan = {};

                IFC(spValue1AsPV.As(&spTimeSpanRef));
                IFC(spTimeSpanRef->get_Value(&value1TimeSpan));

                IFC(spValue2AsPV.As(&spTimeSpanRef));
                IFC(spTimeSpanRef->get_Value(&value2TimeSpan));

                *pAreEqual = value1TimeSpan.Duration == value2TimeSpan.Duration;
                break;
            }
            case wf::PropertyType_Point:
            {
                wrl::ComPtr<wf::IReference<wf::Point>> spPointRef;
                wf::Point value1Point = {};
                wf::Point value2Point = {};

                IFC(spValue1AsPV.As(&spPointRef));
                IFC(spPointRef->get_Value(&value1Point));

                IFC(spValue2AsPV.As(&spPointRef));
                IFC(spPointRef->get_Value(&value2Point));

                *pAreEqual =
                    value1Point.X == value2Point.X &&
                    value1Point.Y == value2Point.Y;
                break;
            }
            case wf::PropertyType_Size:
            {
                wrl::ComPtr<wf::IReference<wf::Size>> spSizeRef;
                wf::Size value1Size = {};
                wf::Size value2Size = {};

                IFC(spValue1AsPV.As(&spSizeRef));
                IFC(spSizeRef->get_Value(&value1Size));

                IFC(spValue2AsPV.As(&spSizeRef));
                IFC(spSizeRef->get_Value(&value2Size));

                *pAreEqual =
                    value1Size.Width == value2Size.Width &&
                    value1Size.Height == value2Size.Height;
                break;
            }
            case wf::PropertyType_Rect:
            {
                wrl::ComPtr<wf::IReference<wf::Rect>> spRectRef;
                wf::Rect value1Rect = {};
                wf::Rect value2Rect = {};

                IFC(spValue1AsPV.As(&spRectRef));
                IFC(spRectRef->get_Value(&value1Rect));

                IFC(spValue2AsPV.As(&spRectRef));
                IFC(spRectRef->get_Value(&value2Rect));

                *pAreEqual =
                    value1Rect.X == value2Rect.X &&
                    value1Rect.Y == value2Rect.Y &&
                    value1Rect.Width == value2Rect.Width &&
                    value1Rect.Height == value2Rect.Height;
                break;
            }
        }

    Cleanup:
        RRETURN(hr);
    }
}
