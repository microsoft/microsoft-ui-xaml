// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SmartStackLogger.h"
#include <WexDebug.h>
#include <ErrorContext.h>
#include <ErrorHandlerSettings.h>
#include <XamlLogging.h>
#include <LogController.h>
#include <ErrorContextStructure.h>

using namespace WEX::Common;

namespace Private { namespace Infrastructure {

void PrintContext(CONTEXT context)
{
    NoThrowString stackText;
    auto stackLines = Debug::GetStack(&context, stackText);
    LOG_WARNING(stackText);
}

bool SmartStackLogger::IsEnabled() const
{
    return m_enabled;
}

void SmartStackLogger::SetIsEnabled(bool value)
{
    LOG_OUTPUT(L"%s SmartStackLogger",value ? L"Enabling" : L"Disabling");
    m_enabled = value;
}

void SmartStackLogger::LogStackIfNovel(const ErrorHandling::XamlFailureInfo& failure)
{
    if (nullptr!=failure.pFailureInfo)
    {
        LOG_WARNING(L"IFC([HRESULT 0x%08X]):", failure.pFailureInfo->hr);
    }

    if (IsEnabled())
    {
        if (nullptr!=failure.pErrorContext)
        {
            CONTEXT context = failure.pErrorContext->contextRecord;
            PrintContext(context);
        }
    }
}

void SmartStackLogger::LogStackFromContext(CONTEXT context)
{
    LOG_WARNING(L"WIL Failure detected, callstack:");
    PrintContext(context);
}

} }
