// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Static methods declared by TextFormatter as inline object handlers
//      for LS.

#pragma once

using namespace Ptls6;

namespace RichTextServices
{
    // Callback used by line services to create inline object context.
    LSERR LineServicesInlineCreateILSObj(
        _In_ POLS pols,
        _In_ PLSC plsc,
        _In_ PCLSCBK pclscbk,
        _In_ LONG idObj,
        _Out_ PILSOBJ *ppilsobj
    );

    // Object context for embedded UIElements.
    class LineServicesEmbeddedObjectContext : public Ptls6::CLsObjectContext
    {
    public:
        void LSAPI Destroy();

        LSERR LSAPI CreateLNObj(
            _In_ PLSPARACLIENT plsparaclient,
            _In_ PCLSDEVINFO plsdevinf,
            _Out_ BOOL& fSublinesInside,
            _Out_ BOOL& fProhibitHyphenation,
            _Out_ BOOL& fTransparentForSpans,
            _Out_ BOOL& fRestrictBreakByIdObj,
            _Outptr_ CLsObjectLineContext** pplnobj);

        POLS pols;
        PLSC plsc;
    };

    // Line format context for embedded UIElement.
    class LineServicesEmbeddedObjectLineContext : public CLsObjectLineContext
    {
    public:
        void LSAPI Destroy();

        LSERR LSAPI FormatSimple(
            _In_ const FMTIN& fmtinput);

        LineServicesEmbeddedObjectContext *pObjectContext;
        PLSPARACLIENT plsparaclient;
    };

    // Display context for embedded UIElements.
    class LineServicesEmbeddedObject : public CLsObject
    {
    public:
        LSERR LSAPI Display(
            _In_ PCDISPIN pdispin);

        void LSAPI Destroy();

        LSERR LSAPI ProposeBreakAfter(
            _Out_ BRKCOND* pbrkcond);

        LSERR LSAPI ProposeBreakBefore(
            _Out_ BRKCOND* pbrkcond);

        LineServicesEmbeddedObjectContext *pObjectContext;
        OBJDIM objdim;
        PLSPARACLIENT plsparaclient;
        BRKCOND breakBefore;
        BRKCOND breakAfter;
        LONG penPositionX{};
        LONG penPositionY{};
        bool isPenPositionUsed;
    };
}
