// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

namespace DirectUI
{

// ApiEtwStart and ApiEtwStop are provided for API entry point logging to ETW and are wrapped around
// the ETW provided functions to prevent inlining which can bloat DLL size.  The use of Enabled
// checks EventEnabledApiFunctionCallStart and EventEnabledApiFunctionCallStop should be done prior
// to calling these functions, however, to avoid unnecessary function call costs when ETW is disabled
// (which is most of the time).  The added function call cost when ETW is enabled is moot in this scenario
// since this information is primarily used for debugging purposes and localized performance purposes.
void ApiEtwStart(void* id, KnownMethodIndex knownMethodIndex)
{
    TraceApiFunctionCallStart(
        reinterpret_cast<uint64_t>(id),
        static_cast<uint16_t>(knownMethodIndex));
}

void ApiEtwStop(void* id, HRESULT hr)
{
    TraceApiFunctionCallStop(
        reinterpret_cast<uint64_t>(id),
        hr);
}

// Helper functions used by codegen to enforce strictness at the public API boundary

_Check_return_ HRESULT DefaultStrictApiCheck(_In_ DependencyObject* object)
{
    CDependencyObject* cdo = object->GetHandle();
    return (cdo != nullptr) ? cdo->DefaultStrictApiCheck() : S_OK;
}

_Check_return_ HRESULT StrictOnlyApiCheck(_In_ DependencyObject* object, const WCHAR* apiName)
{
    CDependencyObject* cdo = object->GetHandle();
    return (cdo != nullptr) ? cdo->StrictOnlyApiCheck(apiName) : S_OK;
}

_Check_return_ HRESULT NonStrictOnlyApiCheck(_In_ DependencyObject* object, const WCHAR* apiName)
{
    CDependencyObject* cdo = object->GetHandle();
    return (cdo != nullptr) ? cdo->NonStrictOnlyApiCheck(apiName) : S_OK;
}

}
