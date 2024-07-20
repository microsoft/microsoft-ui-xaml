// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CValueUtil.h>
#include <CValue.h>
#include <CDependencyObject.h>
#include <xstring_ptr.h>
#include <TypeTableStructs.h>
#include <Primitives.h>
#include <Clock.h>
#include <Indexes.g.h>

KnownTypeIndex CValueUtil::GetTypeIndex(
    _In_ const CValue& value)
{
    switch (value.GetType())
    {
        case valueObject:
            {
                CDependencyObject* dependencyObject = value.AsObject();

                if (dependencyObject)
                {
                    if (dependencyObject->GetClassInformation()->IsEnum())
                    {
                        return static_cast<CEnumerated *>(dependencyObject)->GetEnumTypeIndex();
                    }

                    return dependencyObject->GetTypeIndex();
                }
            }
            break;

        case valueEnum:
        case valueEnum8:
            {
                uint32_t enumValue = 0;
                KnownTypeIndex enumTypeIndex = KnownTypeIndex::UnknownType;
                value.GetEnum(enumValue, enumTypeIndex);
                return enumTypeIndex;
            }
    }

    return KnownTypeIndex::UnknownType;
}

bool CValueUtil::EqualsWithoutUnboxing(
    _In_ const CValue& lhs,
    _In_ const CValue& rhs)
{
    if (lhs.GetType() == valueIInspectable && rhs.GetType() == valueIInspectable)
    {
        return lhs.As<valueIInspectable>() == rhs.As<valueIInspectable>();
    }

    return lhs == rhs;
}

_Check_return_ HRESULT CValueUtil::GetRuntimeString(
    _In_ const CValue& value,
    _Out_ xruntime_string_ptr& string)
{
    if (value.GetType() == valueString)
    {
        return xruntime_string_ptr::DecodeAndPromote(
            value.AsEncodedString(),
            &string);
    }
    else
    {
        string.Reset();
        return CORE_E_INVALIDTYPE;
    }
}

void CValueUtil::GetEphemeralString(
    _In_ const CValue& value,
    _Out_ xephemeral_string_ptr& string)
{
    xephemeral_string_ptr::Decode(
        (value.GetType() == valueString) ? value.AsEncodedString() : xencoded_string_ptr::NullString(),
        &string);
}

double CValueUtil::GetTimeSpanAsSeconds(
    _In_ const CValue& value)
{
    ASSERT(value.GetType() == valueTimeSpan);
    return TimeSpanUtil::ToSeconds(value.As<valueTimeSpan>());
}

UINT64 CValueUtil::GetValueOrAddressAsUINT64(
    _In_ const CValue& value)
{
    switch (value.GetType())
    {
        case valueBool:
            return static_cast<UINT64>(value.As<valueBool>());

        case valueColor:
            return static_cast<UINT64>(value.As<valueColor>());

        case valueInt64:
            return static_cast<UINT64>(value.As<valueInt64>());

        case valueEnum:
        case valueEnum8:
            return static_cast<UINT64>(value.AsEnum());

        case valueSigned:
            return static_cast<UINT64>(value.As<valueSigned>());

        case valueDateTime:
            {
                wf::DateTime time = { 0 };
                VERIFYHR(value.Get<valueDateTime>(time));
                return static_cast<UINT64>(time.UniversalTime);
            }

        case valueTimeSpan:
            {
                wf::TimeSpan timeSpan = { 0 };
                VERIFYHR(value.Get<valueTimeSpan>(timeSpan));
                return static_cast<UINT64>(timeSpan.Duration);
            }

        // all other value types return the pointer to the value
        // since we can't represent the value in a 64 bit integer.
        case valuePoint:
            return reinterpret_cast<UINT64>(value.As<valuePoint>());

        case valueRect:
            return reinterpret_cast<UINT64>(value.As<valueRect>());

        case valueFloatArray:
            return reinterpret_cast<UINT64>(value.As<valueFloatArray>());

        case valueObject:
            return reinterpret_cast<UINT64>(value.As<valueObject>());

        case valueThickness:
            return reinterpret_cast<UINT64>(value.As<valueThickness>());

        case valueCornerRadius:
            return reinterpret_cast<UINT64>(value.As<valueCornerRadius>());

        case valueInternalHandler:
            return reinterpret_cast<UINT64>(value.As<valueInternalHandler>());

        case valueSize:
            return reinterpret_cast<UINT64>(value.As<valueSize>());

        case valueGridLength:
            return reinterpret_cast<UINT64>(value.As<valueGridLength>());

        case valuePointArray:
            return reinterpret_cast<UINT64>(value.As<valuePointArray>());

        case valueIUnknown:
            return reinterpret_cast<UINT64>(value.As<valueIUnknown>());

        case valuePointer:
            return reinterpret_cast<UINT64>(value.As<valuePointer>());

        case valueSignedArray:
            return reinterpret_cast<UINT64>(value.As<valueSignedArray>());

        case valueDoubleArray:
            return reinterpret_cast<UINT64>(value.As<valueDoubleArray>());

        // for the other types that we put into the etw payload,
        // we return 0 for this value
        case valueAny:
        case valueNull:
        case valueDouble:
        case valueFloat:
        case valueString:
        default:
            return 0;
    }
}

void CValueUtil::ReleaseRefAndDropObjectOwnership(
    _Inout_ CValue& value)
{
    switch (value.GetType())
    {
        case valueObject:
            {
                CValueCustomData saveBits = value.GetCustomData();
                value.WrapObjectNoRef(value.AsObject());
                value.SetCustomData(saveBits);
            }
            break;

        case valueIInspectable:
            {
                CValueCustomData saveBits = value.GetCustomData();
                value.WrapIInspectableNoRef(value.AsIInspectable());
                value.SetCustomData(saveBits);
            }
            break;

        default:
            ASSERT(false);
            break;
    }
}
