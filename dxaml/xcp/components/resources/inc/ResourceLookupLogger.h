// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xstring_ptr.h>
#include <XStringBuilder.h>
#include <theming\inc\theme.h>
#include <stack_vector.h>
#include "XamlTelemetry.h"

class CResourceDictionary;
class CControlTemplate;

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

#ifdef TRACE_RESOURCELOOKUPS
        //
        // ResourceLookupContext
        //
        // Xaml does lots of resource lookups as it loads the tree during startup, and it's helpful to know why we're doing the look
        // up. Most of the resources tend to be in control templates in styles. Controls also tend to be nested. These fields help us
        // track what controls/templates are currently being loaded, so we can see where the expensive templates are.
        //
        // The first place that a template contributes to resource lookups is when it's being loaded by the parser. Templates are
        // typically declared as part of a Style, in a Setter for the Control.Template property. Property setters are defer loaded,
        // meaning they're only loaded when the Control.Template valus is needed, rather than when the parent Style is loaded.
        //
        // Other property Setters in a Style can also contribute to resource lookups. We'll log those too for convenience.
        //
        // Templates can also contribute to resource lookups when they are being applied to a control. Applying a template isn't
        // straightforward. CFrameworkElement::ApplyTemplate is the function that applies a template, but when nested templates are
        // involved it isn't a recursive function. For example, a ListView's template might include a ScrollViewer, which has its own
        // template. CFE::ApplyTemplate will expand the ListView template down to the ScrollViewer and enter it into the tree, but it
        // will not apply the ScrollViewer's template recursively. Instead, ListView's ApplyTemplate completes and returns up to Measure,
        // then we run Measure on the expanded template. When Measure gets to the ScrollViewer we'll do another ApplyTemplate call for
        // the ScrollViewer to apply its template, and at this point ListView's ApplyTemplate has already completed and is no longer on
        // the stack.
        //
        // Things get trickier because there are also controls that explicitly call ApplyTemplate on their template parts (e.g. NavigationView
        // calls ApplyTemplate on its NavigationViewItem containers), and this breaks the Measure-ApplyTemplate pattern and instead makes
        // ApplyTemplate recursive. We need to work for both these scenarios.
        //
        // Controls that explicitly call ApplyTemplate can also result in multiple ApplyTemplate calls for the same element. For example,
        // NavigationViewItem::GetPresenter explicitly calls ApplyTemplate on its NavigationViewItemPresenter. We later apply a built-in
        // template on the same NVIP via a VisualState, like MUX_NavigationViewItemPresenterStyleWhenOnLeftPane.
        //
        // CFrameworkElement::InvokeApplyTemplate also calls out to the subclass after applying a template. Controls like ScrollViewer will
        // do more resource lookups at this point. We capture that as part of the resource context as well.
        //
    public:
        enum ContextType
        {
            Context_ParseTemplate,      // Setter.Value for the Control.Template property
            Context_ParseStyle,         // Setter.Value for any other property
            Context_LoadContent,        // CFrameworkElement::ApplyTemplate calling CControlTemplate::LoadContent and entering the expanded template into the tree
            Context_PostApplyTemplate,  // CFrameworkElement::InvokeApplyTemplate calling out to the subclass after applying a template
            Context_ReturnToMeasure,    // Returning to Measure after InvokeApplyTemplate, used to capture non-recursive nested templates
        };

        void PushResourceLookupContext(ContextType contextType, _In_ void* pointer, _In_ const xstring_ptr& resourceKey);
        void PushResourceLookupContext(ContextType contextType, _In_ void* pointer, _In_ CControlTemplate* pTemplate);
        void PopResourceLookupContext(_In_ void* pointer);

    private:

        struct ResourceLookupContext
        {
            ContextType m_contextType;
            void* m_pointer;
            const xstring_ptr m_resourceContextString;
        };

        bool ShouldLogResourceLookupContext() const;

        Jupiter::stack_vector<ResourceLookupContext, 8> m_resourceLookupContextStack;
        xstring_ptr m_fullResourceLookupContextString;
        std::unique_ptr<XStringBuilder> m_resourceLookupContextBuilder;
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