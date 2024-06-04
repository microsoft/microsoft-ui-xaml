// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines the Logger class to log binding 

#pragma once

namespace DirectUI
{
    class DebugOutput
    {
    public:
        static DebugOutput& GetInstance();

        static bool IsLoggingForBindingEnabled();
        static bool IsLoggingForXamlResourceReferenceEnabled();

        static void LogBindingErrorMessage(_In_ xstring_ptr message);
        static void LogXamlResourceReferenceErrorMessage(_In_ xstring_ptr message);

        static void SetForceDebugSettingsTracingEvents(bool value)
        {
            GetInstance().m_forceDebugSettingsTracingEvents = value;
        }

    private:
        DebugOutput()
            : m_isLoggingForBindingEnabled(false)
            , m_isLoggingForXamlResourceReferenceEnabled(false)
            , m_forceDebugSettingsTracingEvents(false)
        {
        }
        ~DebugOutput() = default;

        DebugOutput(const DebugOutput&) = delete;
        DebugOutput& operator=(const DebugOutput&) = delete;
        DebugOutput(DebugOutput&&) = delete;
        DebugOutput& operator=(DebugOutput&&) = delete;

        static BOOL CALLBACK InitOnceCallback(
            PINIT_ONCE initOnce,
            PVOID Parameter,
            PVOID *Context);

        void InitializeDebugSettings();

        bool m_isLoggingForBindingEnabled : 1;
        bool m_isLoggingForXamlResourceReferenceEnabled : 1;
        bool m_forceDebugSettingsTracingEvents : 1;
    };

}
