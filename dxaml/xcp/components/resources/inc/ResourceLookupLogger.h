// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xstring_ptr.h>
#include <XStringBuilder.h>
#include <theming\inc\theme.h>
#include "XamlTelemetry.h"

class CResourceDictionary;

namespace Diagnostics
{
    class ResourceLookupLogger
    {
    public:
        ResourceLookupLogger();
        ~ResourceLookupLogger() = default;

        bool IsLogging() const { return m_isLogging; }

        uint64_t GetNextEtwIndex();

        _Check_return_ HRESULT Start(const xstring_ptr_view& resourceKey, const xstring_ptr_view& resourceConsumingUri);
        _Check_return_ HRESULT Stop(const xstring_ptr_view& resourceKey, xstring_ptr& traceMessage);

        _Check_return_ HRESULT OnEnterDictionary(CResourceDictionary* dictionary, const xstring_ptr_view& resourceKey, uint64_t etwEventIndex);
        _Check_return_ HRESULT OnLeaveDictionary(CResourceDictionary* dictionary, const xstring_ptr_view& resourceKey, uint64_t etwEventIndex);

        _Check_return_ HRESULT OnEnterMergedDictionary(CResourceDictionary* dictionary, const std::int32_t index, const xstring_ptr_view& resourceKey, uint64_t etwEventIndex);
        _Check_return_ HRESULT OnLeaveMergedDictionary(CResourceDictionary* dictionary, const std::int32_t index, const xstring_ptr_view& resourceKey, uint64_t etwEventIndex);

        _Check_return_ HRESULT OnEnterThemeDictionary(CResourceDictionary* dictionary, Theming::Theme theme, const xstring_ptr_view& resourceKey, uint64_t etwEventIndex);
        _Check_return_ HRESULT OnLeaveThemeDictionary(CResourceDictionary* dictionary, Theming::Theme theme, const xstring_ptr_view& resourceKey, uint64_t etwEventIndex);

        _Check_return_ HRESULT OnFoundResource(CResourceDictionary* dictionary, const xstring_ptr_view& resourceKey);

        _Check_return_ HRESULT OnEnterImplicitStyle(CResourceDictionary* dictionary, const xstring_ptr_view& resourceKey, uint64_t etwEventIndex);
        _Check_return_ HRESULT OnLeaveImplicitStyle(CResourceDictionary* dictionary, const xstring_ptr_view& resourceKey, uint64_t etwEventIndex);

    private:
        void IncrementIndentationLevel();
        void DecrementIndentationLevel();

        void IncrementEtwIndentationLevel();
        void DecrementEtwIndentationLevel();

        _Check_return_ HRESULT StartNewLineWithIndentation();

        std::shared_ptr<XStringBuilder> m_messageBuilder;
        xstring_ptr m_traceMessage;
        std::uint32_t m_indentationLevel = 0;
        std::uint32_t m_etwIndentationLevel = 0;    // Etw logs more details with its own indentation
#ifdef TRACE_RESOURCELOOKUPS
        bool m_isLogging = true;
#else
        bool m_isLogging = false;
#endif
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
            , m_resourceKey(resourceKey)
        {
            if (m_logger) // For best performance, the call should pass null for the logger if logging is disabled
            {
                m_etwEventIndex = m_logger->GetNextEtwIndex();
                IGNOREHR(m_logger->OnEnterDictionary(m_dictionary, m_resourceKey, m_etwEventIndex));
            }
        }
        ~EnterLeaveDictionaryLogger()
        {
            if (m_logger)
            {
                IGNOREHR(m_logger->OnLeaveDictionary(m_dictionary, m_resourceKey, m_etwEventIndex));
            }
        }
        EnterLeaveDictionaryLogger(const EnterLeaveDictionaryLogger&) = delete;
        EnterLeaveDictionaryLogger& operator=(const EnterLeaveDictionaryLogger&) = delete;
        EnterLeaveDictionaryLogger(EnterLeaveDictionaryLogger&&) = delete;
        EnterLeaveDictionaryLogger& operator=(EnterLeaveDictionaryLogger&&) = delete;
    private:
        Diagnostics::ResourceLookupLogger* m_logger;
        CResourceDictionary* m_dictionary;
        const xstring_ptr_view& m_resourceKey;
        uint64_t m_etwEventIndex;
    };

    class EnterLeaveMergedDictionaryLogger
    {
    public:
        EnterLeaveMergedDictionaryLogger(Diagnostics::ResourceLookupLogger* logger, CResourceDictionary* dictionary, const std::int32_t index, const xstring_ptr_view& resourceKey)
            : m_logger(logger->IsLogging() ? logger : nullptr)
            , m_dictionary(dictionary)
            , m_index(index)
            , m_resourceKey(resourceKey)
        {
            if (m_logger) // For best performance, the call should pass null for the logger if logging is disabled
            {
                m_etwEventIndex = m_logger->GetNextEtwIndex();
                IGNOREHR(m_logger->OnEnterMergedDictionary(m_dictionary, m_index, m_resourceKey, m_etwEventIndex));
            }
        }
        ~EnterLeaveMergedDictionaryLogger()
        {
            if (m_logger)
            {
                IGNOREHR(m_logger->OnLeaveMergedDictionary(m_dictionary, m_index, m_resourceKey, m_etwEventIndex));
            }
        }
        EnterLeaveMergedDictionaryLogger(const EnterLeaveMergedDictionaryLogger&) = delete;
        EnterLeaveMergedDictionaryLogger& operator=(const EnterLeaveMergedDictionaryLogger&) = delete;
        EnterLeaveMergedDictionaryLogger(EnterLeaveMergedDictionaryLogger&&) = delete;
        EnterLeaveMergedDictionaryLogger& operator=(EnterLeaveMergedDictionaryLogger&&) = delete;
    private:
        Diagnostics::ResourceLookupLogger* m_logger;
        CResourceDictionary* m_dictionary;
        std::int32_t m_index;
        const xstring_ptr_view& m_resourceKey;
        uint64_t m_etwEventIndex;
    };

    class EnterLeaveThemeDictionaryLogger
    {
    public:
        EnterLeaveThemeDictionaryLogger(Diagnostics::ResourceLookupLogger* logger, CResourceDictionary* dictionary, Theming::Theme theme, const xstring_ptr_view& resourceKey)
            : m_logger(logger->IsLogging() ? logger : nullptr)
            , m_dictionary(dictionary)
            , m_theme(theme)
            , m_resourceKey(resourceKey)
        {
            if (m_logger) // For best performance, the call should pass null for the logger if logging is disabled
            {
                m_etwEventIndex = m_logger->GetNextEtwIndex();
                IGNOREHR(m_logger->OnEnterThemeDictionary(m_dictionary, m_theme, m_resourceKey, m_etwEventIndex));
            }
        }
        ~EnterLeaveThemeDictionaryLogger()
        {
            if (m_logger)
            {
                IGNOREHR(m_logger->OnLeaveThemeDictionary(m_dictionary, m_theme, m_resourceKey, m_etwEventIndex));
            }
        }
        EnterLeaveThemeDictionaryLogger(const EnterLeaveThemeDictionaryLogger&) = delete;
        EnterLeaveThemeDictionaryLogger& operator=(const EnterLeaveThemeDictionaryLogger&) = delete;
        EnterLeaveThemeDictionaryLogger(EnterLeaveThemeDictionaryLogger&&) = delete;
        EnterLeaveThemeDictionaryLogger& operator=(EnterLeaveThemeDictionaryLogger&&) = delete;
    private:
        Diagnostics::ResourceLookupLogger* m_logger;
        CResourceDictionary* m_dictionary;
        Theming::Theme m_theme;
        const xstring_ptr_view& m_resourceKey;
        uint64_t m_etwEventIndex;
    };

    class SearchImplicitStyleLogger
    {
    public:
        SearchImplicitStyleLogger(Diagnostics::ResourceLookupLogger* logger, CResourceDictionary* dictionary, const xstring_ptr_view& resourceKey)
            : m_logger(logger->IsLogging() ? logger : nullptr)
            , m_dictionary(dictionary)
            , m_resourceKey(resourceKey)
        {
            if (m_logger) // For best performance, the call should pass null for the logger if logging is disabled
            {
                m_etwEventIndex = m_logger->GetNextEtwIndex();
                IGNOREHR(m_logger->OnEnterImplicitStyle(m_dictionary, m_resourceKey, m_etwEventIndex));
            }
        }
        ~SearchImplicitStyleLogger()
        {
            if (m_logger)
            {
                IGNOREHR(m_logger->OnLeaveImplicitStyle(m_dictionary, m_resourceKey, m_etwEventIndex));
            }
        }
        SearchImplicitStyleLogger(const SearchImplicitStyleLogger&) = delete;
        SearchImplicitStyleLogger& operator=(const SearchImplicitStyleLogger&) = delete;
        SearchImplicitStyleLogger(SearchImplicitStyleLogger&&) = delete;
        SearchImplicitStyleLogger& operator=(SearchImplicitStyleLogger&&) = delete;
    private:
        Diagnostics::ResourceLookupLogger* m_logger;
        CResourceDictionary* m_dictionary;
        const xstring_ptr_view& m_resourceKey;
        uint64_t m_etwEventIndex;
    };
}