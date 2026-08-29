// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <depends.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <minxcptypes.h>

class CTextRange final : public CDependencyObject
{
public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject** objectOut,
        _In_ CREATEPARAMETERS* create
        );

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTextRange>::Index;
    }

    const TextRangeData& GetRange() const { return m_range; }
    void SetRange(const TextRangeData& range) { m_range = range; }

    // Publically accessible data
    TextRangeData m_range = {};

private:
    explicit CTextRange(_In_ CCoreServices* core)
        : CDependencyObject(core)
    {}
};