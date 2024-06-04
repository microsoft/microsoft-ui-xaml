// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef DXAML_INSTANCE_STORAGE_H_
#define DXAML_INSTANCE_STORAGE_H_

namespace DXamlInstanceStorage
{

    typedef LPVOID Handle;

    _Check_return_ HRESULT Initialize();

    _Check_return_ HRESULT Deinitialize();

    _Check_return_ HRESULT GetValue(_Outptr_result_maybenull_ Handle* phValue);

    _Check_return_ HRESULT SetValue(_In_opt_ Handle hValue);
    
};

#endif // DXAML_INSTANCE_STORAGE_H_
