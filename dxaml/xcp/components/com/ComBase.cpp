// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ComBase.h"
#include "Indexes.g.h"

KnownTypeIndex ctl::ComBase::GetTypeIndex() const
{
    return KnownTypeIndex::UnknownType;
}
