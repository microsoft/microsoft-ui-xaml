// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "RpcClient.h"
// This file is generated when the contract idl is built and contains
// definitions for the RPC functions.
#include "rpcinterface_c.c"

#include <RpcHandle.h>

#include "Utilities.h"

using namespace WEX::Common;
using namespace WEX::TestExecution;
using namespace Microsoft::UI::Xaml::Tests::Common;

static bool g_isConnected = false;

void RpcClientEnsureConnected()
{
    if (!g_isConnected)
    {
        RpcStringHandle rpcBindingString;

        RPC_STATUS status = RPC_S_OK;

        status = RpcStringBindingCompose(
           NULL,
           static_cast<RPC_WSTR>(const_cast<WCHAR*>(L"ncalrpc")),
           NULL,
           static_cast<RPC_WSTR>(const_cast<WCHAR*>(L"RpcInterface")),
           static_cast<RPC_WSTR>(const_cast<WCHAR*>(L"Security=anonymous dynamic true")),
           &rpcBindingString);
        Throw::IfFalse(status == RPC_S_OK, HRESULT_FROM_WIN32(status));

        status = RpcBindingFromStringBinding(rpcBindingString, &hRpcInterface);
        Throw::IfFalse(status == RPC_S_OK, HRESULT_FROM_WIN32(status));

        status = RpcEpResolveBinding(hRpcInterface, RpcInterface_v1_0_c_ifspec);
        Throw::IfFalse(status == RPC_S_OK, HRESULT_FROM_WIN32(status));

        status = RpcMgmtIsServerListening(hRpcInterface);
        Throw::IfFalse(status == RPC_S_OK, HRESULT_FROM_WIN32(status));

        g_isConnected = true;
    }
}

void __RPC_USER MIDL_user_free(_Pre_maybenull_ _Post_invalid_ void* p)
{
    delete [] (BYTE*)p;
}

_Must_inspect_result_
_Ret_maybenull_ _Post_writable_byte_size_(size)
 void* __RPC_USER MIDL_user_allocate(_In_ size_t size)
{
    return (void *)new BYTE[size];
}

