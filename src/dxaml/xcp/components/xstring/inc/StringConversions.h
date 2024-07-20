// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <TrimWhitespace.h>
#include <xtable.h>
#include <stack_vector.h>

namespace DirectUI
{
    enum class DurationType : uint8_t;
    enum class RepeatBehaviorType : uint8_t;
}

_Check_return_ HRESULT SignedFromDecimalString(
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ UINT32* pcSuffix,
    _Outptr_result_buffer_(*pcSuffix) const WCHAR** ppSuffix,
    _Out_ INT32 *pnValue
    );

_Check_return_ HRESULT UnsignedFromDecimalString(
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ UINT32* pcSuffix,
    _Outptr_result_buffer_(*pcSuffix) const WCHAR** ppSuffix,
    _Out_ UINT32 *pnValue
    );

_Check_return_ HRESULT UnsignedFromHexString(
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ UINT32* pcSuffix,
    _Outptr_result_buffer_(*pcSuffix) const WCHAR** ppSuffix,
    _Out_ UINT32 *pnResult
    );

_Check_return_ HRESULT FloatFromString(
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ UINT32* pcSuffix,
    _Outptr_result_buffer_(*pcSuffix) const WCHAR** ppSuffix,
    _Out_ FLOAT *peResult,
    _In_ BOOL bAllowSpecialValues = FALSE);

_Check_return_ HRESULT NonSignedFromString(
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ UINT32* pcSuffix,
    _Outptr_result_buffer_(*pcSuffix) const WCHAR** ppSuffix,
    _Out_ FLOAT *peResult
    );

_Check_return_ HRESULT ArrayFromString(
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ UINT32* pcSuffix,
    _Outptr_result_buffer_(*pcSuffix) const WCHAR** ppSuffix,
    _In_ UINT32 cElements,
    _Out_writes_(cElements) FLOAT *peResult,
    _In_opt_ UINT32 bConsumeString = TRUE);

template <std::size_t count>
_Check_return_ HRESULT ArrayFromString(
    const xstring_ptr_view& str,
    std::array<float, count>& array)
{
    UINT32 cString = 0;
    const WCHAR* pString = str.GetBufferAndCount(&cString);
    UINT32 cScratch;
    const WCHAR* pScratch;
    
    IFC_RETURN(ArrayFromString(cString, pString, &cScratch, &pScratch, count, array.data()));
    return S_OK;
}

_Check_return_ HRESULT EnumerateFromString(
    _In_ UINT32 cTable,
    _In_reads_(cTable) const XTABLE *pTable,
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ INT32 *pnResult);

_Check_return_ HRESULT FlagsEnumerateFromString(
    _In_ UINT32 cTable,
    _In_reads_(cTable) const XTABLE *pTable,
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ INT32 *pnResult);

_Check_return_ HRESULT NameFromString(
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ UINT32* pcSuffix,
    _Outptr_result_buffer_(*pcSuffix) const WCHAR** ppSuffix,
    _Out_ XNAME *pName);

_Check_return_ HRESULT TimeSpanFromString(
    _In_ const xstring_ptr_view& inString,
    _Out_ DOUBLE *peValue);

_Check_return_ HRESULT TimeSpanToString(
    _In_ DOUBLE timeSpanInSec,
    _Out_ xstring_ptr& outString);

_Check_return_ HRESULT KeyTimeFromString(
    _In_ const xstring_ptr_view& inString,
    _Out_ DOUBLE *peValue);

_Check_return_ HRESULT KeyTimeToString(
    _In_ DOUBLE timeSpanInSec,
    _Out_ xstring_ptr& outString);

_Check_return_ HRESULT DurationFromString(
    _In_ const xstring_ptr_view& inString,
    _Out_ DirectUI::DurationType *pDurationType,
    _Out_ DOUBLE *peValue);

_Check_return_ HRESULT DurationToString(
    _In_ DirectUI::DurationType durationType,
    _In_ DOUBLE timeSpanInSec,
    _Out_ xstring_ptr& outString);

_Check_return_ HRESULT RepeatBehaviorFromString(
    _In_ const xstring_ptr_view& inString,
    _Out_ DirectUI::RepeatBehaviorType *repeatBehaviorType,
    _Out_ DOUBLE* durationInSec,
    _Out_ FLOAT* count);

_Check_return_ HRESULT RepeatBehaviorToString(
    _In_ DirectUI::RepeatBehaviorType repeatBehaviorType,
    _In_ DOUBLE durationInSec,
    _In_ FLOAT count,
    _Out_ xstring_ptr& outString);

_Check_return_ HRESULT GridLengthFromString(
    _In_ const xstring_ptr_view& inString,
    _Out_ XGRIDLENGTH *peValue);

_Check_return_ HRESULT ParseGridDefinitionCollectionInitializationString(
    _In_ const xstring_ptr& inString,
    _In_ Jupiter::stack_vector<xstring_ptr, 8>& defCollection);

// exists for the parser, do not use in other code
_Check_return_ HRESULT ThicknessFromString(
    _In_ const xstring_ptr_view& inString,
    _Out_ XTHICKNESS *peValue);

_Check_return_ HRESULT MatrixFromString(
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ UINT32* pcSuffix,
    _Outptr_result_buffer_(*pcSuffix) const WCHAR** ppSuffix,
    _Out_ CMILMatrix *pmatResult);

_Check_return_ HRESULT Matrix4x4FromString(
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ UINT32* pcSuffix,
    _Outptr_result_buffer_(*pcSuffix) const WCHAR** ppSuffix,
    _Out_ CMILMatrix4x4 *pmatResult);

_Check_return_ HRESULT Vector2FromString(
    const xstring_ptr_view& value,
    wfn::Vector2& convertedValue);

_Check_return_ HRESULT Vector3FromString(
    const xstring_ptr_view& value,
    wfn::Vector3& convertedValue);

_Check_return_ HRESULT QuaternionFromString(
    const xstring_ptr_view& value,
    wfn::Quaternion& convertedValue);

_Check_return_ HRESULT Matrix3x2FromString(
    const xstring_ptr_view& value,
    wfn::Matrix3x2& convertedValue);

_Check_return_ HRESULT Matrix4x4FromString(
    const xstring_ptr_view& value,
    wfn::Matrix4x4& convertedValue);

_Check_return_ HRESULT TimeSpanFromString(
    const xstring_ptr_view& value,
    wf::TimeSpan& convertedValue);