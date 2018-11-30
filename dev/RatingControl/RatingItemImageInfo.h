// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "RatingItemInfo.h"

#include "RatingItemImageInfo.g.h"
#include "RatingItemImageInfo.properties.h"

class RatingItemImageInfo :
    public winrt::implementation::RatingItemImageInfoT<RatingItemImageInfo, RatingItemInfo>,
    public RatingItemImageInfoProperties
{
public:
    ForwardRefToBaseReferenceTracker(RatingItemInfo)
};
