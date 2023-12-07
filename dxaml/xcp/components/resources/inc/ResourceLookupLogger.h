// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xstring_ptr.h>
#include <XStringBuilder.h>
#include <theming\inc\theme.h>

class CResourceDictionary;

namespace Diagnostics
{
    class ResourceLookupLogger
    {
    public:
        ResourceLookupLogger();
        ~ResourceLookupLogger() = default;

        bool IsLogging() const { return m_isLogging; }

        _Check_return_ HRESULT Start(const xstring_ptr_view& resourceKey, const xstring_ptr_view& resourceConsumingUri);
        _Check_return_ HRESULT Stop(const xstring_ptr_view& resourceKey, xstring_ptr& traceMessage);

        _Check_return_ HRESULT OnEnterDictionary(CResourceDictionary* dictionary, const xstring_ptr_view& resourceKey);
        _Check_return_ HRESULT OnLeaveDictionary(CResourceDictionary* dictionary);

        _Check_return_ HRESULT OnEnterMergedDictionary(const std::int32_t index, const xstring_ptr_view& resourceKey);
        _Check_return_ HRESULT OnLeaveMergedDictionary(const std::int32_t index);

        _Check_return_ HRESULT OnEnterThemeDictionary(Theming::Theme theme, const xstring_ptr_view& resourceKey);
        _Check_return_ HRESULT OnLeaveThemeDictionary(Theming::Theme theme);

    private:
        void IncrementIndentationLevel();
        void DecrementIndentationLevel();

        _Check_return_ HRESULT StartNewLineWithIndentation();

        std::shared_ptr<XStringBuilder> m_messageBuilder;
        xstring_ptr m_traceMessage;
        std::uint32_t m_indentationLevel = 0;
        bool m_isLogging = false;
    };
}