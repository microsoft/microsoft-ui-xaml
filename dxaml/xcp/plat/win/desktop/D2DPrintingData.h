// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "documenttarget.h"
#include "printpreview.h"
#include "windows.graphics.printing.h"

class CD2DPrintingData final : public CXcpObjectBase <IPALD2DPrintingData>
{
public:
    CD2DPrintingData();

    _Check_return_ HRESULT SetPreviewPackageTarget(_In_ void* pPreviewPackageTarget);
    _Ret_notnull_ void* GetPreviewPackageTargetNoRef();

    _Check_return_ HRESULT SetDocumentPackageTarget(_In_ void* pDocumentPackageTarget);
    _Ret_notnull_ void* GetDocumentPackageTargetNoRef();

    _Check_return_ HRESULT SetDocumentSettings(_In_ void* pDocumentSettings);
    _Ret_notnull_ void* GetDocumentSettingsNoRef();

    _Check_return_ HRESULT SetPreviewPaneSize(XUINT32 pageNumber, XSIZEF size);
    _Check_return_ HRESULT GetPreviewPaneSize(XUINT32 pageNumber, XSIZEF& size);

    _Check_return_ HRESULT GetPageSize(XINT32 pageNumber, _Out_ XSIZEF& pageSize);

    void SetDPIScale(XFLOAT value);
    XFLOAT GetDPIScale();

protected:
    virtual ~CD2DPrintingData();

private:
    IPrintPreviewDxgiPackageTarget* m_pPreviewPackageTarget;
    IPrintDocumentPackageTarget* m_pDocumentPackageTarget;
    IInspectable* m_pDocumentSettings;
    xchainedmap<XUINT32, XSIZEF> m_previewPaneSizeMap;
    XFLOAT m_dpiScale;
};
