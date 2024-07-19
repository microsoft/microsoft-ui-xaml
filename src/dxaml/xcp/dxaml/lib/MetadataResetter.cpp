// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <MetadataResetter.h>

using namespace DirectUI;

MetadataResetter::~MetadataResetter()
{
    MetadataAPI::Reset();
}
