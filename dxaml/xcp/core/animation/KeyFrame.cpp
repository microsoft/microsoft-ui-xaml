// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "KeyFrame.h"

KnownTypeIndex CKeyFrame::GetTypeIndex() const
{
    return DependencyObjectTraits<CKeyFrame>::Index;
}
