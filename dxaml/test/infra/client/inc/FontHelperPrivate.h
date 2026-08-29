// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
// Adapted for the public SDK from TAEF's Tailored.h

#pragma once

interface IDWriteFontFileLoader;

MIDL_INTERFACE("c146c0ce-3810-4ae2-a4fd-93c5fdc1ace2")
IFontHelperNative : public IUnknown
{
public:
    virtual HRESULT GetCustomSystemFontCollection(IUnknown** fontCollection) = 0;
    virtual HRESULT SetSystemFontCollectionOverride(IUnknown* fontCollection) = 0;
    virtual HRESULT CreateCustomFontCollection(
        wchar_t const* fontFileNames[], unsigned int cFontFileNames,
        wchar_t const* fontNames[], unsigned int cFontNames,
        _Out_ IUnknown** ppFontCollection) = 0;
    
};
