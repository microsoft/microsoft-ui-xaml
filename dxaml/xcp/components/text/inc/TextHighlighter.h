// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <depends.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>

class CBrush;
class CTextRangeCollection;

class CTextHighlighter final : public CDependencyObject
{
public:
    DECLARE_CREATE(CTextHighlighter);
    ~CTextHighlighter() override;

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTextHighlighter>::Index;
    }
    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    void InvalidateTextRanges();

    const CTextRangeCollection* GetRanges() const { return m_ranges; }

    // Public properties
    CBrush* m_foreground = nullptr;
    CBrush* m_background = nullptr;
    CTextRangeCollection* m_ranges = nullptr;

private:
    explicit CTextHighlighter(_In_ CCoreServices* core)
        : CDependencyObject(core)
    {}

    _Check_return_ HRESULT InitInstance() override;
};