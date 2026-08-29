// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CCoreServices;

// MaxTextureSizeProvider helper class abstracts the query for a "safe" max texture size
// to be used when creating DComp surfaces.
class MaxTextureSizeProvider
{
public:
    explicit MaxTextureSizeProvider(_In_ CCoreServices* core);

    uint32_t GetMaxTextureSize() const;

private:
    CCoreServices* m_pCoreNoRef = nullptr;
};
