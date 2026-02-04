// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ComObjectUnitTests.h"

#include <ComBase.h>
#include <ComObject.h>
#include <ComPtr.h>

using namespace ctl;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Com {
    
    
    void ComObjectUnitTests::CanInstantiateComObject()
    {
        ctl::ComPtr<ComBase> comInstance;
        THROW_IF_FAILED(ComObject<ComBase>::CreateInstance(&comInstance));
        VERIFY_IS_NOT_NULL(comInstance);

        ctl::ComPtr<SupportErrorInfo> supportErrorInfoInstance;
        THROW_IF_FAILED(ComObject<SupportErrorInfo>::CreateInstance(&supportErrorInfoInstance));
        VERIFY_IS_NOT_NULL(supportErrorInfoInstance);
    }

} } } } }