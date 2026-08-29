// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// UIElement representation of Hyperlink focus. This element is required
// since Hyperlink is not a UIElement and does not have visual
// representation, but can take focus, so CFocusRectangle is used to
// indicate when a Hyperlink has focus.
//
// WARNING: This class should never be made public. It sets values directly,
// bypassing the typical SetValue property system pathways, to allow instances to
// be created and used from the Render thread for the case of ListViewItemChrome.
// That's generally a fine thing to do, as long as you promise that these values can
// never, ever be a binding or anything more complex than internal jupiter types
// and primitive values.
class CFocusRectangle : public CRectangle
{
public:

    CFocusRectangle(_In_ CCoreServices *pCore);

    static _Check_return_ HRESULT Create(
        _In_ CCoreServices *pCore,
        _Outptr_ CFocusRectangle **ppFocusRectangle
        );

    _Check_return_ HRESULT SetStrokeThickness(float thickness);
    _Check_return_ HRESULT SetStrokeDashOffset(float dashOffset);

    _Check_return_ HRESULT SetBounds(
        _In_ const XRECTF &bounds,
        _In_ CBrush *pStrokeBrush
        );

    bool GetIsLayoutElement() const final { return true; }

    // See the comments in Shape.h for the justification here- we're using a method
    // based property to get around some invalid assumptions this type made before checks
    // were in place to prevent them.
    bool HasStrokeDashArray() const override;
    xref_ptr<CDoubleCollection> GetStrokeDashArray() const override;
    _Check_return_ HRESULT SetStrokeDashArray(xref_ptr<CDoubleCollection> brush) override;

    xref_ptr<CBrush> GetStroke() const override;
    _Check_return_ HRESULT SetStroke(xref_ptr<CBrush> brush) override;

private:
    XRECTF m_bounds;

    xref_ptr<CBrush> m_stroke;
    xref_ptr<CDoubleCollection> m_strokeDashArray;



};
