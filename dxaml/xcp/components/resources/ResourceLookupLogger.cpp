// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <corep.h>
#include <DebugOutput.h>
#include <DependencyLocator.h>
#include <DXamlServices.h>
#include <ErrorHelper.h>
#include <Resources.h>
#include <XamlTraceLogging.h>
#include <XStringUtils.h>
#include "XamlTelemetry.h"
#include "ResourceLookupLogger.h"
#include "template.h"
#include "MetadataAPI.h"

namespace
{
    _Check_return_ HRESULT
    ConvertThemeToString(Theming::Theme theme, _Out_ xstring_ptr& themeAsString)
    {
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_highConstrastWhite, L"HighContrastWhite");
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_highConstrastBlack, L"HighContrastBlack");
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_highConstrastCustom, L"HighContrastCustom");
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_highConstrast, L"HighContrast");
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_light, L"Light");
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_dark, L"Dark");

        XStringBuilder builder;

        switch (Theming::GetHighContrastValue(theme))
        {
            case Theming::Theme::HighContrastWhite:
                IFC_RETURN(builder.Append(c_highConstrastWhite));
                break;
            case Theming::Theme::HighContrastBlack:
                IFC_RETURN(builder.Append(c_highConstrastBlack));
                break;
            case Theming::Theme::HighContrastCustom:
                IFC_RETURN(builder.Append(c_highConstrastCustom));
                break;
        }

        if (builder.GetCount() > 0)
        {
            IFC_RETURN(builder.Append(XSTRING_PTR_EPHEMERAL(L" | ")));
        }

        switch (Theming::GetBaseValue(theme))
        {
            case Theming::Theme::Light:
                IFC_RETURN(builder.Append(c_light));
                break;
            case Theming::Theme::Dark:
                IFC_RETURN(builder.Append(c_dark));
                break;
        }

        IFC_RETURN(builder.DetachString(&themeAsString));

        return S_OK;
    }

    // Generate an identifier for the resource dictionary using the following methods:
    // 1) Use base URI if present (general case)
    // 2) Check if it is one of the known framework resource dictionaries that feeds
    //    the StaticResource and ThemeResource markup extensions
    //    * themeresources.xbf
    //    * system colors
    //    * Application.Resources (default "App.xaml" may have been overridden in code-behind)
    // 3) hard-coded string (this should be rare)
    xstring_ptr GenerateIdentifierForResourceDictionary(CResourceDictionary* dictionary)
    {
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_anonymousDictionaryId, L"<anonymous dictionary>");
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_applicationResourcesId, L"Application.Resources");
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_globalThemeResourcesId, L"Framework ThemeResources.xbf");
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_systemColorsId, L"Framework-defined colors");

        xstring_ptr identifier;

        // GetBaseUri() addrefs before returning
        xref_ptr<IPALUri> baseUri;
        baseUri.attach(dictionary->GetBaseUri());

        if (!dictionary->m_strSource.IsNullOrEmpty())
        {
            identifier = dictionary->m_strSource;
        }
        else if (baseUri != nullptr)
        {
            TRACE_HR_NORETURN(baseUri->GetCanonical(&identifier));
        }
        else if (dictionary == DirectUI::DXamlServices::GetHandle()->GetThemeResources_NoLazyLoad())
        {
            identifier = c_globalThemeResourcesId;
        }
        else if (dictionary->IsSystemColorsDictionary())
        {
            identifier = c_systemColorsId;
        }
        else if (dictionary == DirectUI::DXamlServices::GetHandle()->GetApplicationResourceDictionary())
        {
            identifier = c_applicationResourcesId;
        }
        else
        {
            // The dictionary has no base URI and is not one of the known special dictionaries
            // (perhaps it was created in code-behind) so it's anonymous
            identifier = c_anonymousDictionaryId;
        }

        return identifier;
    }

    xstring_ptr IndentString(uint32_t level, const xstring_ptr_view& stringToIndent, const xstring_ptr_view& suffix = xstring_ptr::EmptyString(), const xstring_ptr_view& suffix2 = xstring_ptr::EmptyString())
    {
        XStringBuilder builder;

        for (uint32_t i = 0; i < level; ++i)
        {
            IFCFAILFAST(builder.Append(STR_LEN_PAIR(L"  ")));
        }

        IFCFAILFAST(builder.Append(stringToIndent));
        IFCFAILFAST(builder.Append(suffix));
        IFCFAILFAST(builder.Append(suffix2));

        xstring_ptr indentedString;
        IFCFAILFAST(builder.DetachString(&indentedString));
        return indentedString;
    }
}

namespace Diagnostics
{
    ResourceLookupLogger::ResourceLookupLogger()
    {
    }

    static std::atomic<uint64_t> s_ResourceEtwIndex{0};

    uint64_t ResourceLookupLogger::GetNextEtwIndex()
    {
        return s_ResourceEtwIndex.fetch_add(1, std::memory_order_relaxed);
    }

    _Check_return_ HRESULT ResourceLookupLogger::Start(const xstring_ptr_view& resourceKey, const xstring_ptr_view& resourceConsumingUri)
    {
#ifdef TRACE_RESOURCELOOKUPS
        ASSERT(IsLogging());
#else
        ASSERT(!IsLogging());
        m_isLogging = true;
#endif
        m_messageBuilder = std::make_shared<XStringBuilder>();
        m_indentationLevel = 0;
        m_etwIndentationLevel = 0;

        // If the caller provides a Uri for the document that referenced this resource then output that first.  This allows app
        // developers to tell which .xaml file contained the missing resource.
        if (!resourceConsumingUri.IsNullOrEmpty())
        {
            IFC_RETURN(m_messageBuilder->Append(resourceConsumingUri));
            IFC_RETURN(StartNewLineWithIndentation());
        }

        TraceLoggingWrite(
            g_hTraceProvider,
            "ResourceLookup_Start",
            TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TraceLoggingValue(resourceKey.GetBuffer(), "ResourceKey"));

        IFC_RETURN(m_messageBuilder->Append(StringCchPrintfWWrapper(
            L"Beginning search for resource with key '%s'.",
            resourceKey.GetBuffer())));

        IncrementIndentationLevel();

        return S_OK;
    }

    _Check_return_ HRESULT ResourceLookupLogger::Stop(const xstring_ptr_view& resourceKey, xstring_ptr& traceMessage)
    {
        ASSERT(IsLogging());

#ifdef TRACE_RESOURCELOOKUPS
#else
        m_isLogging = false;
#endif
        DecrementIndentationLevel();

        TraceLoggingWrite(
            g_hTraceProvider,
            "ResourceLookup_Stop",
            TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TraceLoggingValue(resourceKey.GetBuffer(), "ResourceKey"));

        IFC_RETURN(StartNewLineWithIndentation());
        IFC_RETURN(m_messageBuilder->Append(StringCchPrintfWWrapper(
            L"Finished search for resource with key '%s'.",
            resourceKey.GetBuffer())));

        // Leave a copy of the trace log in memory for the dump
        IFC_RETURN(m_messageBuilder->DetachString(&(m_traceMessage)));
        traceMessage = m_traceMessage;

        if (DirectUI::DebugOutput::IsLoggingForXamlResourceReferenceEnabled())
        {
            DirectUI::DebugOutput::LogXamlResourceReferenceErrorMessage(m_traceMessage);
        }

        m_messageBuilder.reset();

        return S_OK;
    }

    _Check_return_ HRESULT ResourceLookupLogger::OnEnterDictionary(CResourceDictionary* dictionary, const xstring_ptr_view& resourceKey, uint64_t etwEventIndex)
    {
        if (IsLogging())
        {
            auto id = GenerateIdentifierForResourceDictionary(dictionary);
            auto indentedDictionary = IndentString(m_etwIndentationLevel, id);

            TraceLoggingProviderWrite(
                XamlTelemetry, "ResourceLookup_Dictionary",
                TraceLoggingWideString(resourceKey.GetBuffer(), "ResourceKey"),
                TraceLoggingUInt64(reinterpret_cast<uint64_t>(dictionary), "DictionaryPointer"),
                TraceLoggingWideString(indentedDictionary.GetBuffer(), "Dictionary"),
                TraceLoggingUInt32(dictionary->GetCount(), "Count"),
                TraceLoggingUInt64(etwEventIndex, "ETWEventIndex"),
                TraceLoggingBoolean(true, "IsEnter"),
                TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

            if (m_messageBuilder)
            {
                IFC_RETURN(StartNewLineWithIndentation());
                IFC_RETURN(m_messageBuilder->Append(StringCchPrintfWWrapper(
                    L"Searching dictionary '%s' for resource with key '%s'.",
                    id.GetBuffer(), resourceKey.GetBuffer())));
            }

            IncrementIndentationLevel();
        }

        return S_OK;
    }

    _Check_return_ HRESULT ResourceLookupLogger::OnLeaveDictionary(CResourceDictionary* dictionary, const xstring_ptr_view& resourceKey, uint64_t etwEventIndex)
    {
        if (IsLogging())
        {
            DecrementIndentationLevel();

            auto id = GenerateIdentifierForResourceDictionary(dictionary);
            auto indentedDictionary = IndentString(m_etwIndentationLevel, id);

            TraceLoggingProviderWrite(
                XamlTelemetry, "ResourceLookup_Dictionary",
                TraceLoggingWideString(resourceKey.GetBuffer(), "ResourceKey"),
                TraceLoggingUInt64(reinterpret_cast<uint64_t>(dictionary), "DictionaryPointer"),
                TraceLoggingWideString(indentedDictionary.GetBuffer(), "Dictionary"),
                TraceLoggingUInt32(dictionary->GetCount(), "Count"),
                TraceLoggingUInt64(etwEventIndex, "ETWEventIndex"),
                TraceLoggingBoolean(false, "IsEnter"),
                TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

            if (m_messageBuilder)
            {
                IFC_RETURN(StartNewLineWithIndentation());
                IFC_RETURN(m_messageBuilder->Append(StringCchPrintfWWrapper(
                    L"Finished searching dictionary '%s'.",
                    id.GetBuffer())));
            }
        }

        return S_OK;
    }

    _Check_return_ HRESULT ResourceLookupLogger::OnEnterMergedDictionary(CResourceDictionary* dictionary, const std::int32_t index, const xstring_ptr_view& resourceKey, uint64_t etwEventIndex)
    {
        if (IsLogging())
        {
            xstring_ptr indexAsString;
            {
                wchar_t buffer[_MAX_ITOSTR_BASE10_COUNT];
                if (_itow_s(index, buffer, 10))
                {
                    IFC_RETURN(HRESULT_FROM_WIN32(_doserrno));
                }
                IFC_RETURN(xstring_ptr::CloneBuffer(buffer, &indexAsString));
            }

            auto id = GenerateIdentifierForResourceDictionary(dictionary);
            auto indentedDictionary = IndentString(m_etwIndentationLevel, id, XSTRING_PTR_EPHEMERAL(L" Merged "), indexAsString);

            TraceLoggingProviderWrite(
                XamlTelemetry, "ResourceLookup_MergedDictionary",
                TraceLoggingWideString(resourceKey.GetBuffer(), "ResourceKey"),
                TraceLoggingUInt64(reinterpret_cast<uint64_t>(dictionary), "ParentDictionaryPointer"),
                TraceLoggingWideString(indentedDictionary.GetBuffer(), "Dictionary"),
                TraceLoggingUInt64(etwEventIndex, "ETWEventIndex"),
                TraceLoggingBoolean(true, "IsEnter"),
                TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

            if (m_messageBuilder)
            {
                IFC_RETURN(StartNewLineWithIndentation());
                IFC_RETURN(m_messageBuilder->Append(StringCchPrintfWWrapper(
                    L"Searching merged dictionary with index '%s' for resource with key '%s'.",
                    indexAsString.GetBuffer(), resourceKey.GetBuffer())));
            }

            IncrementIndentationLevel();
        }

        return S_OK;
    }

    /* static */ _Check_return_ HRESULT
    ResourceLookupLogger::OnLeaveMergedDictionary(CResourceDictionary* dictionary, const std::int32_t index, const xstring_ptr_view& resourceKey, uint64_t etwEventIndex)
    {
        if (IsLogging())
        {
            DecrementIndentationLevel();

            xstring_ptr indexAsString;
            {
                wchar_t buffer[_MAX_ITOSTR_BASE10_COUNT];
                if (_itow_s(index, buffer, 10))
                {
                    IFC_RETURN(HRESULT_FROM_WIN32(_doserrno));
                }
                IFC_RETURN(xstring_ptr::CloneBuffer(buffer, &indexAsString));
            }

            auto id = GenerateIdentifierForResourceDictionary(dictionary);
            auto indentedDictionary = IndentString(m_etwIndentationLevel, id, XSTRING_PTR_EPHEMERAL(L" Merged "), indexAsString);

            TraceLoggingProviderWrite(
                XamlTelemetry, "ResourceLookup_MergedDictionary",
                TraceLoggingWideString(resourceKey.GetBuffer(), "ResourceKey"),
                TraceLoggingUInt64(reinterpret_cast<uint64_t>(dictionary), "ParentDictionaryPointer"),
                TraceLoggingWideString(indentedDictionary.GetBuffer(), "Dictionary"),
                TraceLoggingUInt64(etwEventIndex, "ETWEventIndex"),
                TraceLoggingBoolean(false, "IsEnter"),
                TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

            if (m_messageBuilder)
            {
                IFC_RETURN(StartNewLineWithIndentation());
                IFC_RETURN(m_messageBuilder->Append(StringCchPrintfWWrapper(
                    L"Finished searching merged dictionary with index '%s'.",
                    indexAsString.GetBuffer())));
            }
        }

        return S_OK;
    }

    /* static */ _Check_return_ HRESULT
    ResourceLookupLogger::OnEnterThemeDictionary(CResourceDictionary* dictionary, Theming::Theme theme, const xstring_ptr_view& resourceKey, uint64_t etwEventIndex)
    {
        if (IsLogging())
        {
            xstring_ptr themeAsString;
            IFC_RETURN(ConvertThemeToString(theme, themeAsString));

            auto id = GenerateIdentifierForResourceDictionary(dictionary);
            auto indentedDictionary = IndentString(m_etwIndentationLevel, id, XSTRING_PTR_EPHEMERAL(L" Theme "), themeAsString);

            TraceLoggingProviderWrite(
                XamlTelemetry, "ResourceLookup_ThemeDictionary",
                TraceLoggingWideString(resourceKey.GetBuffer(), "ResourceKey"),
                TraceLoggingUInt64(reinterpret_cast<uint64_t>(dictionary), "ParentDictionaryPointer"),
                TraceLoggingWideString(indentedDictionary.GetBuffer(), "Dictionary"),
                TraceLoggingUInt64(etwEventIndex, "ETWEventIndex"),
                TraceLoggingBoolean(true, "IsEnter"),
                TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

            if (m_messageBuilder)
            {
                IFC_RETURN(StartNewLineWithIndentation());
                IFC_RETURN(m_messageBuilder->Append(StringCchPrintfWWrapper(
                    L"Searching theme dictionary (active theme: '%s') for resource with key '%s'.",
                    themeAsString.GetBuffer(), resourceKey.GetBuffer())));
            }

            IncrementIndentationLevel();
        }

        return S_OK;
    }

    /* static */ _Check_return_ HRESULT
    ResourceLookupLogger::OnLeaveThemeDictionary(CResourceDictionary* dictionary, Theming::Theme theme, const xstring_ptr_view& resourceKey, uint64_t etwEventIndex)
    {
        if (IsLogging())
        {
            DecrementIndentationLevel();

            xstring_ptr themeAsString;
            IFC_RETURN(ConvertThemeToString(theme, themeAsString));

            auto id = GenerateIdentifierForResourceDictionary(dictionary);
            auto indentedDictionary = IndentString(m_etwIndentationLevel, id, XSTRING_PTR_EPHEMERAL(L" Theme "), themeAsString);

            TraceLoggingProviderWrite(
                XamlTelemetry, "ResourceLookup_ThemeDictionary",
                TraceLoggingWideString(resourceKey.GetBuffer(), "ResourceKey"),
                TraceLoggingUInt64(reinterpret_cast<uint64_t>(dictionary), "ParentDictionaryPointer"),
                TraceLoggingWideString(indentedDictionary.GetBuffer(), "Dictionary"),
                TraceLoggingUInt64(etwEventIndex, "ETWEventIndex"),
                TraceLoggingBoolean(false, "IsEnter"),
                TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

            if (m_messageBuilder)
            {
                IFC_RETURN(StartNewLineWithIndentation());
                IFC_RETURN(m_messageBuilder->Append(StringCchPrintfWWrapper(
                    L"Finished searching theme dictionary (active theme: '%s').",
                    themeAsString.GetBuffer())));
            }
        }

        return S_OK;
    }

    _Check_return_ HRESULT ResourceLookupLogger::OnFoundResource(CResourceDictionary* dictionary, const xstring_ptr_view& resourceKey)
    {
        if (IsLogging())
        {
            auto id = GenerateIdentifierForResourceDictionary(dictionary);
            auto indentedDictionary = IndentString(m_etwIndentationLevel, id);
            uint64_t etwEventIndex = GetNextEtwIndex();

#ifdef TRACE_RESOURCELOOKUPS
            TraceLoggingProviderWrite(
                XamlTelemetry, "ResourceLookup_ResourceFound",
                TraceLoggingWideString(resourceKey.GetBuffer(), "ResourceKey"),
                TraceLoggingWideString(ShouldLogResourceLookupContext() ? m_resourceLookupContextStack.m_vector.back().m_resourceContextString.GetBuffer() : nullptr, "ShortContext"),
                TraceLoggingWideString(ShouldLogResourceLookupContext() ? m_fullResourceLookupContextString.GetBuffer() : nullptr, "FullContext"),
                TraceLoggingUInt64(reinterpret_cast<uint64_t>(dictionary), "ParentDictionaryPointer"),
                TraceLoggingWideString(indentedDictionary.GetBuffer(), "Dictionary"),
                TraceLoggingUInt64(etwEventIndex, "ETWEventIndex"),
                TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
#else
            TraceLoggingProviderWrite(
                XamlTelemetry, "ResourceLookup_ResourceFound",
                TraceLoggingWideString(resourceKey.GetBuffer(), "ResourceKey"),
                TraceLoggingUInt64(reinterpret_cast<uint64_t>(dictionary), "ParentDictionaryPointer"),
                TraceLoggingWideString(indentedDictionary.GetBuffer(), "Dictionary"),
                TraceLoggingUInt64(etwEventIndex, "ETWEventIndex"),
                TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
#endif
        }

        return S_OK;
    }

    _Check_return_ HRESULT ResourceLookupLogger::OnEnterImplicitStyle(CResourceDictionary* dictionary, const xstring_ptr_view& resourceKey, uint64_t etwEventIndex)
    {
        if (IsLogging())
        {
            auto id = GenerateIdentifierForResourceDictionary(dictionary);
            auto indentedDictionary = IndentString(m_etwIndentationLevel, id);

            TraceLoggingProviderWrite(
                XamlTelemetry, "ResourceLookup_ImplicitStyle",
                TraceLoggingWideString(resourceKey.GetBuffer(), "ResourceKey"),
                TraceLoggingUInt64(reinterpret_cast<uint64_t>(dictionary), "DictionaryPointer"),
                TraceLoggingWideString(indentedDictionary.GetBuffer(), "Dictionary"),
                TraceLoggingUInt32(dictionary->GetCount(), "Count"),
                TraceLoggingUInt64(etwEventIndex, "ETWEventIndex"),
                TraceLoggingBoolean(true, "IsEnter"),
                TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

            IncrementEtwIndentationLevel();
        }

        return S_OK;
    }

    _Check_return_ HRESULT ResourceLookupLogger::OnLeaveImplicitStyle(CResourceDictionary* dictionary, const xstring_ptr_view& resourceKey, uint64_t etwEventIndex)
    {
        if (IsLogging())
        {
            DecrementEtwIndentationLevel();

            auto id = GenerateIdentifierForResourceDictionary(dictionary);
            auto indentedDictionary = IndentString(m_etwIndentationLevel, id);

            TraceLoggingProviderWrite(
                XamlTelemetry, "ResourceLookup_ImplicitStyle",
                TraceLoggingWideString(resourceKey.GetBuffer(), "ResourceKey"),
                TraceLoggingUInt64(reinterpret_cast<uint64_t>(dictionary), "DictionaryPointer"),
                TraceLoggingWideString(indentedDictionary.GetBuffer(), "Dictionary"),
                TraceLoggingUInt32(dictionary->GetCount(), "Count"),
                TraceLoggingUInt64(etwEventIndex, "ETWEventIndex"),
                TraceLoggingBoolean(false, "IsEnter"),
                TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
        }

        return S_OK;
    }

    void ResourceLookupLogger::IncrementIndentationLevel()
    {
        ASSERT(m_indentationLevel <= (std::numeric_limits<decltype(m_indentationLevel)>::max() - 1));
        ++m_indentationLevel;
        IncrementEtwIndentationLevel();
    }

    void ResourceLookupLogger::DecrementIndentationLevel()
    {
        ASSERT(m_indentationLevel >= (std::numeric_limits<decltype(m_indentationLevel)>::min() + 1));
        --m_indentationLevel;
        DecrementEtwIndentationLevel();
    }

    void ResourceLookupLogger::IncrementEtwIndentationLevel()
    {
        ASSERT(m_etwIndentationLevel <= (std::numeric_limits<decltype(m_etwIndentationLevel)>::max() - 1));
        ++m_etwIndentationLevel;
    }

    void ResourceLookupLogger::DecrementEtwIndentationLevel()
    {
        ASSERT(m_etwIndentationLevel >= (std::numeric_limits<decltype(m_etwIndentationLevel)>::min() + 1));
        --m_etwIndentationLevel;
    }

    _Check_return_ HRESULT ResourceLookupLogger::StartNewLineWithIndentation()
    {
        ASSERT(m_messageBuilder);

        IFC_RETURN(m_messageBuilder->AppendChar(L'\n'));
        for (std::uint32_t i = 0; i < m_indentationLevel; ++i)
        {
            IFC_RETURN(m_messageBuilder->Append(STR_LEN_PAIR(L"  ")));
        }

        return S_OK;
    }

#ifdef TRACE_RESOURCELOOKUPS
    bool ResourceLookupLogger::ShouldLogResourceLookupContext() const
    {
        return !m_resourceLookupContextStack.m_vector.empty()
            // We only log resource lookups that are associated with templates - loading them via the parser, LoadContent
            // stamping out elements, entering the expanded template into the tree, and post-ApplyTemplate work. If we're
            // doing a lookup outside of ApplyTemplate (and outside of the parser loading a Setter.Valye), we don't associate
            // the resource lookup with a template. We can get into these cases when doing Measure inside an expanded template,
            // while not expanding a nested template.
            && m_resourceLookupContextStack.m_vector.back().m_contextType != ContextType::Context_ReturnToMeasure;
    }

    void ResourceLookupLogger::PushResourceLookupContext(ContextType contextType, _In_ void* pointer, _In_ const xstring_ptr& resourceKey)
    {
        if (!m_resourceLookupContextBuilder)
        {
            m_resourceLookupContextBuilder = std::make_unique<XStringBuilder>();
        }

        xstring_ptr resourceLookupContextString;
        IFCFAILFAST(m_resourceLookupContextBuilder->Append(resourceKey));
        switch (contextType)
        {
            case ContextType::Context_ParseStyle:
                IFCFAILFAST(m_resourceLookupContextBuilder->Append(XSTRING_PTR_EPHEMERAL(L" (P_Style)")));
                break;

            case ContextType::Context_ParseTemplate:
                IFCFAILFAST(m_resourceLookupContextBuilder->Append(XSTRING_PTR_EPHEMERAL(L" (P_Template)")));
                break;

            case ContextType::Context_LoadContent:
                IFCFAILFAST(m_resourceLookupContextBuilder->Append(XSTRING_PTR_EPHEMERAL(L" (LoadC)")));
                break;

            case ContextType::Context_PostApplyTemplate:
                IFCFAILFAST(m_resourceLookupContextBuilder->Append(XSTRING_PTR_EPHEMERAL(L" (PostAT)")));
                break;

            case ContextType::Context_ReturnToMeasure:
                IFCFAILFAST(m_resourceLookupContextBuilder->Append(XSTRING_PTR_EPHEMERAL(L" (Measure)")));
                break;
        }
        IFCFAILFAST(m_resourceLookupContextBuilder->DetachString(&resourceLookupContextString));

        m_resourceLookupContextStack.m_vector.push_back({ contextType, pointer, resourceLookupContextString });

        IFCFAILFAST(m_resourceLookupContextBuilder->Append(m_fullResourceLookupContextString));
        if (m_resourceLookupContextBuilder->GetCount() > 0)
        {
            IFCFAILFAST(m_resourceLookupContextBuilder->Append(XSTRING_PTR_EPHEMERAL(L", ")));
        }
        IFCFAILFAST(m_resourceLookupContextBuilder->Append(resourceLookupContextString));
        IFCFAILFAST(m_resourceLookupContextBuilder->DetachString(&m_fullResourceLookupContextString));
    }

    void ResourceLookupLogger::PushResourceLookupContext(ContextType contextType, _In_ void* pointer, _In_ CControlTemplate* pTemplate)
    {
        auto resourceKey = pTemplate->GetResourceKey();
        if (resourceKey.IsNullOrEmpty())
        {
            if (!m_resourceLookupContextBuilder)
            {
                m_resourceLookupContextBuilder = std::make_unique<XStringBuilder>();
            }

            if (pTemplate->OfTypeByIndex(KnownTypeIndex::DisplayMemberTemplate))
            {
                // Xaml's default template for ContentPresenters
                IFCFAILFAST(m_resourceLookupContextBuilder->Append(XSTRING_PTR_EPHEMERAL(L"[DMT]")));
            }
            else
            {
                IFCFAILFAST(m_resourceLookupContextBuilder->Append(XSTRING_PTR_EPHEMERAL(L"[TT: ")));
                IFCFAILFAST(m_resourceLookupContextBuilder->Append(DirectUI::MetadataAPI::GetClassInfoByIndex(pTemplate->GetTargetTypeIndex())->GetFullName()));
                IFCFAILFAST(m_resourceLookupContextBuilder->Append(XSTRING_PTR_EPHEMERAL(L"]")));
            }
            IFCFAILFAST(m_resourceLookupContextBuilder->DetachString(&resourceKey));
        }
        PushResourceLookupContext(contextType, pointer, resourceKey);
    }

    void ResourceLookupLogger::PopResourceLookupContext(_In_ void* pointer)
    {
        if (!m_resourceLookupContextStack.m_vector.empty())
        {
            if (m_resourceLookupContextStack.m_vector.back().m_pointer == pointer)
            {
                m_resourceLookupContextStack.m_vector.pop_back();

                if (!m_resourceLookupContextBuilder)
                {
                    m_resourceLookupContextBuilder = std::make_unique<XStringBuilder>();
                }

                for (const auto& resourceLookupContext : m_resourceLookupContextStack.m_vector)
                {
                    if (m_resourceLookupContextBuilder->GetCount() > 0)
                    {
                        IFCFAILFAST(m_resourceLookupContextBuilder->Append(XSTRING_PTR_EPHEMERAL(L", ")));
                    }
                    IFCFAILFAST(m_resourceLookupContextBuilder->Append(resourceLookupContext.m_resourceContextString));
                }

                IFCFAILFAST(m_resourceLookupContextBuilder->DetachString(&m_fullResourceLookupContextString));
            }
        }
    }
#endif
}