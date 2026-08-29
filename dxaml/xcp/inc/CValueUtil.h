// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

enum class KnownTypeIndex : UINT16;
class CValue;
class xruntime_string_ptr;

// Extension methods for CValue.
namespace CValueUtil
{
    // For enum type index of passed CValue or UnknownType if it cannot be extracted.
    KnownTypeIndex GetTypeIndex(
        _In_ const CValue& value);

    // Performs an equality check, just like CValue::operator==, except it doesn't un-box the value if it's an IPropertyValue.
    // For example, if the two values are both IPropertyValue, both a boolean 'true', operator== will call them equal,
    // but this will only compare the IInspectable pointers.
    bool EqualsWithoutUnboxing(
        _In_ const CValue& lhs,
        _In_ const CValue& rhs);

    // Get xruntime_string_ptr if value is valueString or fail with error.
    _Check_return_ HRESULT GetRuntimeString(
        _In_ const CValue& value,
        _Out_ xruntime_string_ptr& string);

    // Get xephemeral_string_ptr if value is valueString.  If not, return NullString.
    void GetEphemeralString(
        _In_ const CValue& value,
        _Out_ xephemeral_string_ptr& string);

    // Convert valueTimeSpan to seconds represented as a double.  If type is not valueTimeSpan, return 0.
    double GetTimeSpanAsSeconds(
        _In_ const CValue& value);

    // Get the value (if convertible to UINT64) or the address of the 
    // CValue if it an object type. This is meant for the PropertyChanged 
    // ETW events, so if it is a double or string we return 0.
    UINT64 GetValueOrAddressAsUINT64(
        _In_ const CValue& value);

    // If value held is valueObject or valueIInspectable, decrement ref-count if it's owned.  For other types noop.
    void ReleaseRefAndDropObjectOwnership(
        _Inout_ CValue& value);
}