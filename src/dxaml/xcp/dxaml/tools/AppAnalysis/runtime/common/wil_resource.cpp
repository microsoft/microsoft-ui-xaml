// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "wil_resource.h"

void CoDeallocSourceInfo(_Inout_ appanalysis::SourceInfo* info)
{
    WindowsDeleteString(info->FileName);
    WindowsDeleteString(info->FileHash);
}

HRESULT CoAllocSourceInfo(_In_ const appanalysis::SourceInfo& lhs, _Out_ appanalysis::SourceInfo* rhs)
{
    appanalysis::SourceInfo info = {};
    *rhs = { 0 };
    
    IFC_RETURN(WindowsDuplicateString(lhs.FileName, &info.FileName));
    IFC_RETURN(WindowsDuplicateString(lhs.FileHash, &info.FileHash));
    info.LineNumber = lhs.LineNumber;
    info.ColumnNumber = lhs.ColumnNumber;

    *rhs = info;

    return S_OK;
}