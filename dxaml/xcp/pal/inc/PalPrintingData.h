// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//      Contains the basic printing data for the core platform 
//      abstraction layer

#ifndef __PAL__PRINTING__DATA__
#define __PAL__PRINTING__DATA__

//------------------------------------------------------------------------
//
//  Interface:  IPALPrintingData
//
//  Synopsis:
//      PAL Printing Data
//
//------------------------------------------------------------------------

struct IPALPrintingData : public IObject
{
};

struct IPALPrintingData2 : public IPALPrintingData
{
public:
    virtual void  SetDidPrintingStart(bool fPrintingStarted) = 0;
    virtual _Check_return_ bool GetDidPrintingStart() = 0;

    virtual _Check_return_ HRESULT SetScaleX(XFLOAT fScaleX) = 0;
    virtual _Check_return_ XFLOAT GetScaleX() = 0;

    virtual _Check_return_ HRESULT SetScaleY(XFLOAT fScaleY) = 0;
    virtual _Check_return_ XFLOAT GetScaleY() = 0;

    virtual _Check_return_ HRESULT SetPageRangeFrom(XINT32 nPageRangeFrom) = 0;
    virtual _Check_return_ XINT32 GetPageRangeFrom() = 0;

    virtual _Check_return_ HRESULT SetPageRangeTo(XINT32 nPageRangeTo) = 0;
    virtual _Check_return_ XINT32 GetPageRangeTo() = 0;

    virtual _Check_return_ HRESULT SetPrintableAreaWidth(XUINT32 nPrintableAreaWidth) = 0;
    virtual _Check_return_ XUINT32 GetPrintableAreaWidth() = 0;

    virtual _Check_return_ HRESULT SetPrintableAreaHeight(XUINT32 nPrintableAreaHeight) = 0;
    virtual _Check_return_ XUINT32 GetPrintableAreaHeight() = 0;

    virtual _Check_return_ HRESULT SetPageMargins(XTHICKNESS tPageMargins) = 0;
    virtual _Check_return_ XTHICKNESS GetPageMargins() = 0;

    virtual _Check_return_ HRESULT SetDocumentName(
        _In_ const XINT32  cLength,
        _In_reads_((cLength+1)) const WCHAR *pDocumentName) = 0;
    virtual _Check_return_ WCHAR* GetDocumentName() = 0;

    virtual _Check_return_ bool GetSupportsPostscript() = 0;   
    virtual void SetSupportsPostscript(bool value) = 0;

    virtual _Check_return_ bool GetIsPrintFormatPostscript() = 0;   
    virtual void SetPrintFormatToPostscript(bool value) = 0;
};


struct IPALD2DPrintingData : public IPALPrintingData
{
public:
    virtual HRESULT SetPreviewPackageTarget(_In_ void* pPreviewPackageTarget) = 0;
    virtual _Ret_notnull_ void* GetPreviewPackageTargetNoRef() = 0;

    virtual HRESULT SetDocumentPackageTarget(_In_ void* pDocumentPackageTarget) = 0;
    virtual _Ret_notnull_ void* GetDocumentPackageTargetNoRef() = 0;

    virtual HRESULT SetDocumentSettings(_In_ void* pDocumentSettings) = 0;
    virtual _Ret_notnull_ void* GetDocumentSettingsNoRef() = 0;

    virtual HRESULT SetPreviewPaneSize(XUINT32 pageNumber, XSIZEF size) = 0;
    virtual HRESULT GetPreviewPaneSize(XUINT32 pageNumber, XSIZEF& size) = 0;

    virtual HRESULT GetPageSize(XINT32 pageNumber, _Out_ XSIZEF& pageSize) = 0;

    virtual void SetDPIScale(XFLOAT value) = 0;
    virtual XFLOAT GetDPIScale() = 0;
};

#endif //#ifndef __PAL__PRINTING__DATA__
