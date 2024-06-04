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

#include "ResourceLookupLogger.h"

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
                builder.Append(c_highConstrastWhite);
                break;
            case Theming::Theme::HighContrastBlack:
                builder.Append(c_highConstrastBlack);
                break;
            case Theming::Theme::HighContrastCustom:
                builder.Append(c_highConstrastCustom);
                break;
        }

        if (builder.GetCount() > 0)
        {
            builder.Append(XSTRING_PTR_EPHEMERAL(L" | "));
        }

        switch (Theming::GetBaseValue(theme))
        {
            case Theming::Theme::Light:
                builder.Append(c_light);
                break;
            case Theming::Theme::Dark:
                builder.Append(c_dark);
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
        else if (dictionary == DirectUI::DXamlServices::GetHandle()->GetThemeResources())
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
}

namespace Diagnostics
{
    ResourceLookupLogger::ResourceLookupLogger()
    {
        m_messageBuilder = std::make_shared<XStringBuilder>();
    }

    _Check_return_ HRESULT ResourceLookupLogger::Start(const xstring_ptr_view& resourceKey, const xstring_ptr_view& resourceConsumingUri)
    {
        ASSERT(!m_isLogging);

        m_isLogging = true;
        m_indentationLevel = 0;

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
        ASSERT(m_isLogging);

        m_isLogging = false;
        DecrementIndentationLevel();

        TraceLoggingWrite(
            g_hTraceProvider,
            "ResourceLookup_Stop",
            TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TraceLoggingValue(resourceKey.GetBuffer(), "ResourceKey"));

        StartNewLineWithIndentation();
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

        return S_OK;
    }

    _Check_return_ HRESULT ResourceLookupLogger::OnEnterDictionary(CResourceDictionary* dictionary, const xstring_ptr_view& resourceKey)
    {
        if (IsLogging())
        {
            auto id = GenerateIdentifierForResourceDictionary(dictionary);

            TraceLoggingWrite(
                g_hTraceProvider,
                "ResourceLookup_EnterDictionary",
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
                TraceLoggingValue(id.GetBuffer(), "ID"),
                TraceLoggingValue(resourceKey.GetBuffer(), "ResourceKey"));

            IFC_RETURN(StartNewLineWithIndentation());
            IFC_RETURN(m_messageBuilder->Append(StringCchPrintfWWrapper(
                L"Searching dictionary '%s' for resource with key '%s'.",
                id.GetBuffer(), resourceKey.GetBuffer())));

            IncrementIndentationLevel();
        }

        return S_OK;
    }

    _Check_return_ HRESULT ResourceLookupLogger::OnLeaveDictionary(CResourceDictionary* dictionary)
    {
        if (IsLogging())
        {
            auto id = GenerateIdentifierForResourceDictionary(dictionary);

            TraceLoggingWrite(
                g_hTraceProvider,
                "ResourceLookup_LeaveDictionary",
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
                TraceLoggingValue(id.GetBuffer(), "ID"));

            DecrementIndentationLevel();

            IFC_RETURN(StartNewLineWithIndentation());
            IFC_RETURN(m_messageBuilder->Append(StringCchPrintfWWrapper(
                L"Finished searching dictionary '%s'.",
                id.GetBuffer())));
        }

        return S_OK;
    }

    _Check_return_ HRESULT ResourceLookupLogger::OnEnterMergedDictionary(const std::int32_t index, const xstring_ptr_view& resourceKey)
    {
        if (IsLogging())
        {
            xstring_ptr id;
            {
                wchar_t buffer[_MAX_ITOSTR_BASE10_COUNT];
                if (_itow_s(index, buffer, 10))
                {
                    IFC_RETURN(HRESULT_FROM_WIN32(_doserrno))
                }
                IFC_RETURN(xstring_ptr::CloneBuffer(buffer, &id));
            }

            TraceLoggingWrite(
                g_hTraceProvider,
                "ResourceLookup_EnterMergedDictionary",
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
                TraceLoggingValue(id.GetBuffer(), "ID"),
                TraceLoggingValue(resourceKey.GetBuffer(), "ResourceKey"));

            IFC_RETURN(StartNewLineWithIndentation());
            IFC_RETURN(m_messageBuilder->Append(StringCchPrintfWWrapper(
                L"Searching merged dictionary with index '%s' for resource with key '%s'.",
                id.GetBuffer(), resourceKey.GetBuffer())));

            IncrementIndentationLevel();
        }

        return S_OK;
    }

    /* static */ _Check_return_ HRESULT
    ResourceLookupLogger::OnLeaveMergedDictionary(const std::int32_t index)
    {
        if (IsLogging())
        {
            xstring_ptr id;
            {
                wchar_t buffer[_MAX_ITOSTR_BASE10_COUNT];
                if (_itow_s(index, buffer, 10))
                {
                    IFC_RETURN(HRESULT_FROM_WIN32(_doserrno))
                }
                IFC_RETURN(xstring_ptr::CloneBuffer(buffer, &id));
            }

            TraceLoggingWrite(
                g_hTraceProvider,
                "ResourceLookup_LeaveMergedDictionary",
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
                TraceLoggingValue(id.GetBuffer(), "ID"));

            DecrementIndentationLevel();

            IFC_RETURN(StartNewLineWithIndentation());
            IFC_RETURN(m_messageBuilder->Append(StringCchPrintfWWrapper(
                L"Finished searching merged dictionary with index '%s'.",
                id.GetBuffer())));
        }

        return S_OK;
    }

    /* static */ _Check_return_ HRESULT
    ResourceLookupLogger::OnEnterThemeDictionary(Theming::Theme theme, const xstring_ptr_view& resourceKey)
    {
        if (IsLogging())
        {
            xstring_ptr themeAsString;
            IFC_RETURN(ConvertThemeToString(theme, themeAsString));

            TraceLoggingWrite(
                g_hTraceProvider,
                "ResourceLookup_EnterThemeDictionary",
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
                TraceLoggingValue(themeAsString.GetBuffer(), "ActiveTheme"),
                TraceLoggingValue(resourceKey.GetBuffer(), "ResourceKey"));

            IFC_RETURN(StartNewLineWithIndentation());
            IFC_RETURN(m_messageBuilder->Append(StringCchPrintfWWrapper(
                L"Searching theme dictionary (active theme: '%s') for resource with key '%s'.",
                themeAsString.GetBuffer(), resourceKey.GetBuffer())));

            IncrementIndentationLevel();
        }

        return S_OK;
    }

    /* static */ _Check_return_ HRESULT
    ResourceLookupLogger::OnLeaveThemeDictionary(Theming::Theme theme)
    {
        if (IsLogging())
        {
            xstring_ptr themeAsString;
            IFC_RETURN(ConvertThemeToString(theme, themeAsString));

            TraceLoggingWrite(
                g_hTraceProvider,
                "ResourceLookup_EnterThemeDictionary",
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
                TraceLoggingValue(themeAsString.GetBuffer(), "ActiveTheme"));

            DecrementIndentationLevel();

            IFC_RETURN(StartNewLineWithIndentation());
            IFC_RETURN(m_messageBuilder->Append(StringCchPrintfWWrapper(
                L"Finished searching theme dictionary (active theme: '%s').",
                themeAsString.GetBuffer())));
        }

        return S_OK;
    }

    void ResourceLookupLogger::IncrementIndentationLevel()
    {
        ASSERT(m_indentationLevel <= (std::numeric_limits<decltype(m_indentationLevel)>::max() - 1));

        ++m_indentationLevel;
    }

    void ResourceLookupLogger::DecrementIndentationLevel()
    {
        ASSERT(m_indentationLevel >= (std::numeric_limits<decltype(m_indentationLevel)>::min() + 1));

        --m_indentationLevel;
    }

    _Check_return_ HRESULT ResourceLookupLogger::StartNewLineWithIndentation()
    {
        IFC_RETURN(m_messageBuilder->AppendChar(L'\n'));
        for (std::uint32_t i = 0; i < m_indentationLevel; ++i)
        {
            IFC_RETURN(m_messageBuilder->Append(STR_LEN_PAIR(L"  ")));
        }

        return S_OK;
    }
}