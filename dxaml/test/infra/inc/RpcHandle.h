// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Handle.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {
        class RpcStringPolicy
        {
        public:
            typedef RPC_WSTR THandleType;
            
            static void Close(RPC_WSTR& rpcString)
            {
                ::RpcStringFree(&rpcString);
            }

            static bool IsValid(const RPC_WSTR& rpcString)
            {
                return rpcString != nullptr;
            }

            static RPC_WSTR Invalidate(RPC_WSTR& rpcString)
            {
                rpcString = nullptr;
                return rpcString;
            }
        };

        DECLARE_AUTOHANDLE_CLASS(RpcStringHandle, RpcStringPolicy);
    }
} } } }