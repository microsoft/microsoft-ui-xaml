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

    // Helper classes for logging resource lookups. These will call the appropriate enter/leave methods on the
    // logger, ensuring that the logger is correctly notified on scope exit. These are fully implemented in the
    // header so they can be inlined for best performance in the hot resource lookup paths.

    class EnterLeaveDictionaryLogger
    {
    public:
        EnterLeaveDictionaryLogger(Diagnostics::ResourceLookupLogger* logger, CResourceDictionary* dictionary, const xstring_ptr_view& resourceKey)
            : m_logger(logger->IsLogging() ? logger : nullptr)
            , m_dictionary(dictionary)
        {
            if (m_logger) // For best performance, the call should pass null for the logger if logging is disabled
            {
                IGNOREHR(m_logger->OnEnterDictionary(dictionary, resourceKey));
            }
        }
        ~EnterLeaveDictionaryLogger()
        {
            if (m_logger)
            {
                IGNOREHR(m_logger->OnLeaveDictionary(m_dictionary));
            }
        }
    private:
        Diagnostics::ResourceLookupLogger* m_logger;
        CResourceDictionary* m_dictionary;
    };

    class EnterLeaveMergedDictionaryLogger
    {
    public:
        EnterLeaveMergedDictionaryLogger(Diagnostics::ResourceLookupLogger* logger, const std::int32_t index, const xstring_ptr_view& resourceKey)
            : m_logger(logger->IsLogging() ? logger : nullptr)
            , m_index(index)
        {
            if (m_logger) // For best performance, the call should pass null for the logger if logging is disabled
            {
                IGNOREHR(m_logger->OnEnterMergedDictionary(index, resourceKey));
            }
        }
        ~EnterLeaveMergedDictionaryLogger()
        {
            if (m_logger)
            {
                IGNOREHR(m_logger->OnLeaveMergedDictionary(m_index));
            }
        }
    private:
        Diagnostics::ResourceLookupLogger* m_logger;
        std::int32_t m_index;
    };

    class EnterLeaveThemeDictionaryLogger
    {
    public:
        EnterLeaveThemeDictionaryLogger(Diagnostics::ResourceLookupLogger* logger, Theming::Theme theme, const xstring_ptr_view& resourceKey)
            : m_logger(logger->IsLogging() ? logger : nullptr)
            , m_theme(theme)
        {
            if (m_logger) // For best performance, the call should pass null for the logger if logging is disabled
            {
                IGNOREHR(m_logger->OnEnterThemeDictionary(theme, resourceKey));
            }
        }
        ~EnterLeaveThemeDictionaryLogger()
        {
            if (m_logger)
            {
                IGNOREHR(m_logger->OnLeaveThemeDictionary(m_theme));
            }
        }
    private:
        Diagnostics::ResourceLookupLogger* m_logger;
        Theming::Theme m_theme;
    };
}