// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ResourceDictionaryKey.h"
#include <objectWriter\inc\StreamOffsetToken.h>
#include <ankerl\unordered_dense.h>
#include <vector>

class CDependencyObject;

using ResourceMap = ankerl::unordered_dense::map<
    ResourceKeyStorage,
    CDependencyObject*,
    ResourceKeyStorage_transparent_hash,
    ResourceKeyStorage_transparent_equal>;

using DeferredResourceMap = ankerl::unordered_dense::map<
    ResourceKeyStorage,
    StreamOffsetToken,
    ResourceKeyStorage_transparent_hash,
    ResourceKeyStorage_transparent_equal>;

using ConditionalDeferredResourceMap = ankerl::unordered_dense::map<
    ResourceKeyStorage,
    std::vector<StreamOffsetToken>,
    ResourceKeyStorage_transparent_hash,
    ResourceKeyStorage_transparent_equal>;
