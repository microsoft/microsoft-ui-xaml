// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace RichTextServices
{
    class TextParagraphProperties;
    class TextCollapsingSymbol;
}

class BlockLayoutHelpers
{
public:

    // Get TextFormatter instance.
    static _Check_return_ HRESULT GetTextFormatter(
        _In_ CDependencyObject *pLayoutOwner,
        _Outptr_ RichTextServices::TextFormatter **ppTextFormatter
        );

    // Release TextFormatter instance.
    static void ReleaseTextFormatter(
        _In_ CDependencyObject *pLayoutOwner,
        _In_ RichTextServices::TextFormatter *pTextFormatter
        );

    static _Check_return_ HRESULT GetParagraphProperties(
        _In_opt_ CDependencyObject *pParagraph,
        _In_ CDependencyObject *pLayoutOwner,
        _Outptr_ RichTextServices::TextParagraphProperties **ppTextParagraphProperties
        );

    static _Check_return_ HRESULT CreateCollapsingSymbol(
        _In_opt_ CDependencyObject *pParagraph,
        _In_ CDependencyObject *pLayoutOwner,
        _Outptr_ RichTextServices::TextCollapsingSymbol **ppCollapsingSymbol
        );

    static _Check_return_ HRESULT GetTextAlignment(
        _In_opt_ CDependencyObject *pParagraph,
        _In_ CDependencyObject *pLayoutOwner,
        _Out_ DirectUI::TextAlignment *pTextAlignment
        );

    static DirectUI::TextTrimming GetTextTrimming(
        _In_ CDependencyObject *pLayoutOwner
        );

    static bool ShowParagraphEllipsisOnPage(
        _In_ CDependencyObject *pPageOwner
        );

    static void GetPagePadding(
        _In_ CDependencyObject *pPageOwner,
        _Out_ XTHICKNESS *pPadding
        );

    static void GetLayoutRoundingHeightAdjustment(
        _In_ CDependencyObject *pPageOwner,
        _Out_ XFLOAT *pLayoutRoundingHeightAdjustment
        );

    static void GetBlockMargin(
        _In_opt_ CDependencyObject *pBlock,
        _Out_ XTHICKNESS *pMargin
        );

    static _Check_return_ HRESULT GetElementBaseline(
        _In_ CUIElement *pElement,
        _Out_ XFLOAT *pBaseline
        );

    static _Check_return_ HRESULT GetLineStackingInfo(
        _In_opt_ CDependencyObject *pParagraph,
        _In_ CDependencyObject *pLayoutOwner,
        _Out_ DirectUI::LineStackingStrategy *pLineStackingStrategy,
        _Out_ XFLOAT *pDefaultFontBaseline,
        _Out_ XFLOAT *pDefaultFontLineAdvance,
        _Out_ XFLOAT *pLineHeight
        );

    static RichTextServices::FlowDirection::Enum GetFlowDirection(
        _In_ CDependencyObject *pLayoutOwner
        );

    static CFontContext * GetFontContext(
        _In_ CDependencyObject *pLayoutOwner
        );

    static DirectUI::OpticalMarginAlignment GetOpticalMarginAlignment(
        _In_ CDependencyObject *pLayoutOwner
        );

    static bool GetIsColorFontEnabled(
        _In_ CDependencyObject *pLayoutOwner
        );

    static DirectUI::TextReadingOrder GetTextReadingOrder(
        _In_ CDependencyObject *pLayoutOwner
        );

private:

    static _Check_return_ HRESULT GetFormatting(
        _In_        CDependencyObject     *pParagraph,
        _Outptr_ TextFormatting const **ppFormatting
        );

    static _Check_return_ HRESULT GetInheritedAttachedProperties(
        _In_        CDependencyObject          *pParagraph,
        _Outptr_ InheritedProperties const **ppInheritedAttachedProperties
        );

    static DirectUI::TextWrapping GetTextWrapping(
        _In_ CDependencyObject *pLayoutOwner
        );

    static XFLOAT GetParagraphIndent(
        _In_opt_ CDependencyObject *pParagraph,
        _In_ CDependencyObject *pLayoutOwner
        );

    static _Check_return_ HRESULT ComputeCollapsingCharacterWidth(
        _In_  IFssFontFace                        *pFontFace,
        _In_  WCHAR                               *pCollapsingChar,
        _In_  RichTextServices::TextRunProperties *pTextRunProperties,
        _Out_ XFLOAT                              *pCollapsingCharWidth
        );

    static DirectUI::TextLineBounds GetTextLineBounds(
        _In_ CDependencyObject *pLayoutOwner
        );

    static DirectUI::TextAlignment GetTextAlignment(_In_ CDependencyObject* layoutOwner);

};

