// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Microsoft.DirectManipulation.h>

class CDirectManipulationService;

class CDirectManipulationFrameInfoProvider : public ATL::CComObjectRootEx<ATL::CComSingleThreadModel>,
                                             public IDirectManipulationFrameInfoProvider
{
private:
    // CDirectManipulationService implementation that handles the notifications.
    CDirectManipulationService* m_pDMService;

protected:
    CDirectManipulationFrameInfoProvider()
    {
        m_pDMService = NULL;
    }

public:
    _Check_return_ HRESULT SetDMService(_In_ CDirectManipulationService* pDMService);

public:
    DECLARE_NOT_AGGREGATABLE(CDirectManipulationFrameInfoProvider)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CDirectManipulationFrameInfoProvider)
        COM_INTERFACE_ENTRY(IDirectManipulationFrameInfoProvider)
    END_COM_MAP()

    // IDirectManipulationFrameInfoProvider
    IFACEMETHOD(GetNextFrameInfo)(
        _Out_ XUINT64* pTime,
        _Out_ XUINT64* pProcessTime,
        _Out_ XUINT64* pCompositionTime);
};
