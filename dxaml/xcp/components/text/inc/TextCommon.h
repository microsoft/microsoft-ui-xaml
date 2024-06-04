// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Link {

    static bool IsLink(_In_ const CDependencyObject* const link)
    {
        return link->OfTypeByIndex<KnownTypeIndex::Hyperlink>();
    }
}
