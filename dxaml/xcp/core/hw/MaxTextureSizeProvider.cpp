// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MaxTextureSizeProvider.h"

MaxTextureSizeProvider::MaxTextureSizeProvider(_In_ CCoreServices* core)
: m_pCoreNoRef(core)
{
}

uint32_t MaxTextureSizeProvider::GetMaxTextureSize() const
{
    return m_pCoreNoRef->GetMaxTextureSize();
}
