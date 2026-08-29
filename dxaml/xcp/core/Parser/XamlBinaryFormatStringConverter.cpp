// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XamlBinaryFormatStringConverter.h"
#include <CValue.h>
#include <StringConversions.h>
#include <primitives.h>
#include <CColor.h>
#include <Double.h>

// Convert value from string to target type if possible.
bool XamlBinaryFormatStringConverter::TryConvertValue(_Const_ _In_ const XamlTypeToken typeToken, _Inout_ CValue& valueContainer)
{
    ASSERT(typeToken.GetHandle() != KnownTypeIndex::UnknownType);

    CREATEPARAMETERS cp(const_cast<CCoreServices*>(m_core), valueContainer.AsString());

    bool hasTypeConverted = false;

    if (valueContainer.GetType() == valueString && typeToken.GetProviderKind() == tpkNative)
    {
        auto typeIndex = typeToken.GetHandle();

        switch (typeIndex)
        {
            case KnownTypeIndex::String:
                {
                    hasTypeConverted = true;
                }
                break;

            case KnownTypeIndex::Int32:
                if (SUCCEEDED(CInt32::CreateCValue(&cp, valueContainer)))
                {
                    hasTypeConverted = true;
                }
                break;

            case KnownTypeIndex::Boolean:
                if (SUCCEEDED(CBoolean::CreateCValue(&cp, valueContainer)))
                {
                    hasTypeConverted = true;
                }
                break;

            case KnownTypeIndex::Double:
                if (SUCCEEDED(CDouble::CreateCValue(&cp, valueContainer)))
                {
                    hasTypeConverted = true;
                }
                break;

            case KnownTypeIndex::LengthConverter:
                if (SUCCEEDED(CLengthConverter::CreateCValue(&cp, valueContainer)))
                {
                    hasTypeConverted = true;
                }
                break;

            case KnownTypeIndex::KeyTime:
            {
                XDOUBLE value = 0.0;

                if (SUCCEEDED(KeyTimeFromString(valueContainer.AsString(), &value)))
                {
                    valueContainer.SetFloat(static_cast<XFLOAT>(value));
                    hasTypeConverted = true;
                }
                break;
            }

            case KnownTypeIndex::GridLength:
            {
                std::unique_ptr<XGRIDLENGTH> value(new XGRIDLENGTH);
                if (SUCCEEDED(GridLengthFromString(valueContainer.AsString(), value.get())))
                {
                    valueContainer.SetGridLength(value.release());
                    hasTypeConverted = true;
                }
                break;
            }

            case KnownTypeIndex::Thickness:
            {
                std::unique_ptr<XTHICKNESS> value(new XTHICKNESS);
                if (SUCCEEDED(ThicknessFromString(valueContainer.AsString(), value.get())))
                {
                    valueContainer.SetThickness(value.release());
                    hasTypeConverted = true;
                }
                break;
            }

            case KnownTypeIndex::Brush:
            case KnownTypeIndex::Color:
            {
                unsigned int value = 0;
                if (SUCCEEDED(CColor::ColorFromString(valueContainer.AsString(), &value)))
                {
                    valueContainer.SetColor(value);
                    hasTypeConverted = true;
                }
                break;
            }

            default:
            {
                break;
            }
        }

        // Handle enum values.
        if (!hasTypeConverted && typeIndex != KnownTypeIndex::UnknownType)
        {
            auto type = DirectUI::MetadataAPI::GetClassInfoByIndex(typeIndex);
            if (type->IsEnum())
            {
                unsigned int valueTableLength = 0;
                const XTABLE* valueTable = nullptr;
                if (SUCCEEDED(GetEnumValueTable(typeIndex, &valueTableLength, &valueTable)))
                {
                    int nValue = 0;
                    xstring_ptr valueString = valueContainer.AsString();
                    if (SUCCEEDED(FlagsEnumerateFromString(valueTableLength, valueTable, valueString.GetCount(), valueString.GetBuffer(), &nValue)))
                    {
                        if (type->IsCompactEnum())
                        {
                            ASSERT(nValue <= UINT8_MAX);
                            valueContainer.Set<valueEnum8>({ static_cast<uint8_t>(nValue), typeIndex });
                        }
                        else
                        {
                            valueContainer.Set<valueEnum>({ static_cast<uint32_t>(nValue), typeIndex });
                        }
                        hasTypeConverted = true;
                    }
                }
            }
        }
    }

    return hasTypeConverted;
}
