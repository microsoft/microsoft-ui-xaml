// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <TypeTableStructs.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { 
    namespace Metadata {

        struct ClassInfoCallbacks : public std::enable_shared_from_this<ClassInfoCallbacks>
        {
            std::function<CREATEPFN(const CClassInfo*)> GetCoreConstructor;
            std::function<HRESULT()> RunClassConstructorIfNecessary;
        };
    }

} } } }