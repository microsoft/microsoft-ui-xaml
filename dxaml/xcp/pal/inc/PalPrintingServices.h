// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//      Contains the basic printing support for the core platform 
//      abstraction layer

#ifndef __PAL__PRINTING__SERVICES__
#define __PAL__PRINTING__SERVICES__

//------------------------------------------------------------------------
//
//  Interface:  IPALPrintingServices
//
//  Synopsis:
//      Provides an abstract way to get to access the print dialog.
//
//------------------------------------------------------------------------
struct IPALPrintingServices
{
    virtual _Check_return_ HRESULT CreateD2DPrintFactoryAndTarget(
        _Outptr_ IPALAcceleratedGraphicsFactory **ppPrintFactory,
        _Outptr_ IPALPrintTarget** ppPALPrintTarget) = 0;
    virtual _Check_return_ HRESULT CreateD2DPrintingData(
        _Outptr_ IPALD2DPrintingData** ppPrintingData) = 0;
};

#endif //__PAL__PRINTING__SERVICES__
