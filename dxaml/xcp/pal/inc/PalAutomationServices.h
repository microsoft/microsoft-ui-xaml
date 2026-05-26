// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef __PAL__AUTOMATION_SERVICES__
#define __PAL__AUTOMATION_SERVICES__

struct IXcpHostSite;
class CDependencyObject;

struct IPALAutomationServices
{
    virtual _Check_return_ HRESULT GetAutomationProvider(
        _In_ IXcpHostSite *pSite,
        _In_ CDependencyObject *pElement,
        _Outptr_ IUnknown** ppProvider) = 0;
};

#endif //__PAL__AUTOMATION_SERVICES__
