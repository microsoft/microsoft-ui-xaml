// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

CD2DPrintingData::CD2DPrintingData()
{
    m_pDocumentPackageTarget = NULL;
    m_pDocumentSettings = NULL;
    m_pPreviewPackageTarget = NULL;
    m_dpiScale = 1.0f;
}

CD2DPrintingData::~CD2DPrintingData()
{
    ReleaseInterface(m_pDocumentPackageTarget);
    ReleaseInterface(m_pPreviewPackageTarget);
    ReleaseInterface(m_pDocumentSettings);
    m_previewPaneSizeMap.Clear();
}

_Check_return_ HRESULT CD2DPrintingData::SetDocumentPackageTarget(_In_ void* pDocumentPackageTarget)
{
    ReplaceInterface(m_pDocumentPackageTarget, static_cast<IPrintDocumentPackageTarget*>(pDocumentPackageTarget));
    RRETURN(S_OK);
}

_Ret_notnull_ void* CD2DPrintingData::GetDocumentPackageTargetNoRef()
{
    return m_pDocumentPackageTarget;
}

_Check_return_ HRESULT CD2DPrintingData::SetPreviewPackageTarget(_In_ void* pPreviewPackageTarget)
{
    ReplaceInterface(m_pPreviewPackageTarget, static_cast<IPrintPreviewDxgiPackageTarget*>(pPreviewPackageTarget));
    RRETURN(S_OK);
}

_Ret_notnull_ void* CD2DPrintingData::GetPreviewPackageTargetNoRef()
{
    return m_pPreviewPackageTarget;
}

_Check_return_ HRESULT CD2DPrintingData::SetDocumentSettings(_In_ void* pDocumentSettings)
{
    ReplaceInterface(m_pDocumentSettings, static_cast<IInspectable*>(pDocumentSettings));
    m_previewPaneSizeMap.Clear(); // Clear the cached preview pane sizes since page size may have changed
    RRETURN(S_OK);
}

_Ret_notnull_ void* CD2DPrintingData::GetDocumentSettingsNoRef()
{
    return m_pDocumentSettings;
}

_Check_return_ HRESULT CD2DPrintingData::SetPreviewPaneSize(XUINT32 pageNumber, XSIZEF size)
{
    HRESULT hr = S_OK;
    XSIZEF unused;

    IFC(m_previewPaneSizeMap.Remove(pageNumber, unused));
    IFC(m_previewPaneSizeMap.Add(pageNumber, size));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT CD2DPrintingData::GetPreviewPaneSize(XUINT32 pageNumber, XSIZEF& size)
{
    RRETURN(m_previewPaneSizeMap.Get(pageNumber, size));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the size of a page from IPrintTaskOptionsCore.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CD2DPrintingData::GetPageSize(XINT32 pageNumber, _Out_ XSIZEF& pageSize)
{
    HRESULT hr = S_OK;
    wgr::Printing::IPrintTaskOptionsCore* pPrintTaskOptionsCore = NULL;
    wgr::Printing::PrintPageDescription description;
    IInspectable* pPrintTaskOptionsAsIInspectable = static_cast<IInspectable*>(GetDocumentSettingsNoRef());

    pageSize.width = pageSize.height = 800;
    if (pPrintTaskOptionsAsIInspectable)
    {
        IFC(pPrintTaskOptionsAsIInspectable->QueryInterface(
             __uuidof(wgr::Printing::IPrintTaskOptionsCore),
            reinterpret_cast<void**>(&pPrintTaskOptionsCore)));
        IFC(pPrintTaskOptionsCore->GetPageDescription(pageNumber, &description));
        pageSize.width = description.PageSize.Width;
        pageSize.height = description.PageSize.Height;
    }

Cleanup:
    ReleaseInterface(pPrintTaskOptionsCore);
    RRETURN(hr);
}

void CD2DPrintingData::SetDPIScale(XFLOAT value)
{
    m_dpiScale = value;
}

XFLOAT CD2DPrintingData::GetDPIScale()
{
    return m_dpiScale;
}

