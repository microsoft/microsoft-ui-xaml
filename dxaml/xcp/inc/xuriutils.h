// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Description:
//      Portable URI utilities

#ifndef X_URI_UTILS_H
#define X_URI_UTILS_H

HRESULT PercentEncodeString(_In_ XUINT32 cOriginal, _In_reads_(cOriginal) WCHAR* pszOriginal, _Out_ XUINT32* pcEncoded, _Outptr_result_z_ WCHAR** ppszEncoded);
bool ContainsCharactersToEncode(_In_ XUINT32 cOriginal, _In_reads_(cOriginal) WCHAR* pszOriginal);
  
#endif
