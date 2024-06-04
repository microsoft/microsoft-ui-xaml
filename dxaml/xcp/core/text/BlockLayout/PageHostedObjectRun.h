// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CInlineUIContainer;
class ParagraphNode;

//------------------------------------------------------------------------
//  Summary:
//      An implementation of ObjectRun that contains an embedded UIElement.
//
//------------------------------------------------------------------------
class PageHostedObjectRun : public RichTextServices::ObjectRun
{
public:
    PageHostedObjectRun(
        _In_ CInlineUIContainer *pContainer,
        _In_ XUINT32 characterIndex,
        _In_ RichTextServices::TextRunProperties *pProperties
    );
    virtual ~PageHostedObjectRun();

    // ObjectRun overrides.
    virtual bool HasFixedSize() const;
    virtual RichTextServices::Result::Enum Format(
        _In_ RichTextServices::TextSource *pTextSource,
        _In_ XFLOAT paragraphWidth,
        _In_ const XPOINTF &currentPosition,
        _Out_ RichTextServices::ObjectRunMetrics *pMetrics);
    virtual RichTextServices::Result::Enum ComputeBoundingBox(
        _In_  bool   rightToLeft,
        _In_  bool   sideways,
        _Out_ XRECTF *pBounds);
    virtual RichTextServices::Result::Enum Arrange(
        _In_ const XPOINTF &position);

private:
    CInlineUIContainer *m_pContainer;

    _Check_return_ HRESULT GetEmbeddedElementParent(_Outptr_ CDependencyObject **ppParent);
};

