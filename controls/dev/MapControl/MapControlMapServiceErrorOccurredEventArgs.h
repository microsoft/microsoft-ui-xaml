// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"
#include "MapControlMapServiceErrorOccurredEventArgs.g.h"


class MapControlMapServiceErrorOccurredEventArgs :
    public winrt::implementation::MapControlMapServiceErrorOccurredEventArgsT<MapControlMapServiceErrorOccurredEventArgs>
{
public:

    MapControlMapServiceErrorOccurredEventArgs(winrt::hstring diagnosticMessage) :
        m_diagnosticMessage(diagnosticMessage) {};

    winrt::hstring DiagnosticMessage() { return m_diagnosticMessage; };

private:
    winrt::hstring m_diagnosticMessage;
};
