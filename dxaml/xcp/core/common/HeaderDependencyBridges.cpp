// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "HeaderDependencyBridges.h"

namespace HeaderDependencyBridges
{
    xstring_ptr StringFromDependencyObject(_In_ CDependencyObject* pDO)
    {
        return static_cast<CString*>(pDO)->m_strString;
    }
}