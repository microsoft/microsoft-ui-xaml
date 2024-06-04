// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Indexes.g.h>

template<class T>
struct DependencyObjectTraits
{
    static const KnownTypeIndex Index = KnownTypeIndex::UnknownType;
};

class CType;
template<>
struct DependencyObjectTraits<CType>
{
    static const KnownTypeIndex Index = KnownTypeIndex::TypeName;
};
