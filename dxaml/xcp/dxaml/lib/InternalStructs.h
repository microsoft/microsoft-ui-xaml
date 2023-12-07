// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Working around codegen's lack of support for internal structs
// Someday, we'll be able to tackle this and put it somewhere better.
XAML_ABI_NAMESPACE_BEGIN namespace Microsoft
{
    namespace UI
    {
        namespace Xaml
        {
            namespace Controls
            {
                namespace Primitives
                {
                    // uses in IItemLookupPanel to return GetClosestElementInfo from point.
                    // element can be item container or group header.
                    struct ElementInfo
                    {
                        INT32 m_childIndex;
                        BOOLEAN m_childIsHeader;
                    };
                }
            }
        }
    }
} XAML_ABI_NAMESPACE_END

namespace DirectUI
{
    // Codegen insists on putting internal types in the DirectUI namespace.
    // Temporary workaround to typedef that type to the real backing type
    typedef ::XamlServiceProviderContext XamlServiceProviderContext;
}
