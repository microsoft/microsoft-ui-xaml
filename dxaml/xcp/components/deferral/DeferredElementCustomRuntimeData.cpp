// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <XStringBuilder.h>
#include <XbfVersioning.h>
#include "DeferredElementCustomRuntimeData.h"
#include "CustomWriterRuntimeDataTypeIndex.h"

CustomWriterRuntimeDataTypeIndex DeferredElementCustomRuntimeData::GetTypeIndexForSerialization(_In_ const TargetOSVersion& targetOS) const
{
    return Parser::Versioning::GetDeferredElementSerializationVersion(targetOS);
}

_Check_return_ HRESULT DeferredElementCustomRuntimeData::SerializeImpl(
    _In_ XamlBinaryFormatSubWriter2* writer,
    _In_ const std::vector<unsigned int>& streamOffsetTable)
{
    IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(
        *this,
        writer,
        streamOffsetTable));

    return S_OK;
}

_Check_return_ HRESULT DeferredElementCustomRuntimeData::ToString(_In_ bool verboseData, _Out_ xstring_ptr& strValue) const
{
    XStringBuilder strBuilder;
    const UINT32 strCount = 256;
    WCHAR *pstrBuffer = nullptr;

    IFC_RETURN(strBuilder.InitializeAndGetFixedBuffer(strCount, &pstrBuffer));

    IFC_RETURN(StringCchPrintf(
        pstrBuffer,
        strCount,
        L"[DeferredElement]"));

    IFC_RETURN(strBuilder.DetachString(&strValue));

    return S_OK;
}
