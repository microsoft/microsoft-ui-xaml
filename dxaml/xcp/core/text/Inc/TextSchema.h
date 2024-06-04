// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef TEXT_SCHEMA_H
#define TEXT_SCHEMA_H

//
// Forward declarations
//
class CTextElement;

//------------------------------------------------------------------------
//  Summary:
//      Acts as a centralized location for text content structure information.
// 
//------------------------------------------------------------------------
class CTextSchema
{
public:
    static _Check_return_ bool IsInlineUIContainer(_In_ CTextElement *pElement);
    static _Check_return_ bool IsSpan(_In_ CTextElement *pElement);
    static _Check_return_ bool IsRun(_In_ CTextElement *pElement);
    static _Check_return_ bool IsHyperlink(_In_ CTextElement *pElement);

    static _Check_return_ HRESULT TextBlockSupportsElement(
        _In_  CTextElement *pTextElement,
        _Out_ bool        *pResult);

    static _Check_return_ HRESULT HyperlinkSupportsElement(
        _In_  CTextElement *pTextElement,
        _Out_ bool        *pResult);

    static _Check_return_ HRESULT InlineCollectionSupportsElement(
        _In_  CInlineCollection *pCollection,
        _In_  CTextElement      *pTextElement,
        _Out_ bool             *pResult);

private:
    static _Check_return_ bool IsInlineCollectionInElement(
        _In_ CInlineCollection *pCollection,
        _In_ KnownTypeIndex     elementTypeIndex);
};

#endif
