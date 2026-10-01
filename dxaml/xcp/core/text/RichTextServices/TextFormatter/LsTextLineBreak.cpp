// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LsTextLineBreak.h"
#include "LsTextFormatter.h"

using namespace Ptls6;
using namespace RichTextServices;
using namespace RichTextServices::Internal;

LsBreakRecordTraits::Handle LsBreakRecordTraits::Adopt(
    LsTextFormatter& origin,
    PLSBREAKRECLINE& producerSlot) noexcept
{
    if (!producerSlot)
    {
        return {};
    }
    ASSERT(origin.m_pLsContext != nullptr);
    return Handle::Adopt(origin, producerSlot);
}

void LsBreakRecordTraits::Destroy(TextFormatter& owner, PLSBREAKRECLINE record) noexcept
{
    auto& formatter = static_cast<LsTextFormatter&>(owner);
    // Do not release the context and continue if LS could not consume the record.
    FAIL_FAST_ASSERT(LsDestroyBreakRecord(formatter.m_pLsContext, record) == lserrNone);
}

LsTextLineBreak::LsTextLineBreak(LsBreakRecordTraits::Handle&& record) noexcept
    : m_breakRecord(std::move(record))
{
}

LsTextLineBreak::LsTextLineBreak() = default;
LsTextLineBreak::~LsTextLineBreak() = default;
