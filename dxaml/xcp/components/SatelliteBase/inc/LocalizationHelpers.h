// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private
{   
    _Check_return_ HRESULT FindStringResource(
        _In_ WORD resourceId,
        _Out_ HSTRING* value);
        
    void XamlTestHookFreeResourceLibrary();
}
