// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef ITEXT_SELECTION_H
#define ITEXT_SELECTION_H

#include "TextPosition.h"

/*
Summary:
    A logical text selection.

Remarks:
    A text selection can either represent a caret or a (non-empty) text range. A text selection
    is composed of two 'positions':
        1) Start position
        2) End position

    In the case of a caret, both positions are the same.
    
    One of the start/end positions is a 'dynamic' end, which can be moved by cursor keys or mouse 
    clicking to extend the selection, and the other's a 'fixed' end. The dynamic end is referred to
    as the 'moving' position, while the fixed end is the 'anchor' position.

    The selection exposes APIs to:
        * Get/Set the underlying text.
        * Query text range start/end/length
        * Update the selection position

    Selection start/end text positions have 'gravity'. Text gravity is used to resolve 
    certain ambiguities related to cursor positioning.

See also:
    <CTextPosition>
    <TextGravity>
    <ITextView> for examples of text-gravity-based decisions.
*/
struct IJupiterTextSelection
{
    virtual ~IJupiterTextSelection() {}

    //------------------------------------------------------------------------
    //  Summary:
    //      Retrieves the 'moving' end of the selection.
    //------------------------------------------------------------------------
    virtual _Check_return_ HRESULT GetMovingTextPosition(_Out_ CTextPosition *pPosition) const = 0;

    //------------------------------------------------------------------------
    //  Summary:
    //      Retrieves the 'anchor' (fixed) end of the selection.
    //------------------------------------------------------------------------
    virtual _Check_return_ HRESULT GetAnchorTextPosition(_Out_ CTextPosition *pPosition) const = 0;

    //------------------------------------------------------------------------
    //  Summary:
    //      Retrieves the 'start' of the selection.
    //------------------------------------------------------------------------
    virtual _Check_return_ HRESULT  GetStartTextPosition(_Out_ CTextPosition *pPosition) const = 0;

    //------------------------------------------------------------------------
    //  Summary:
    //      Retrieves the 'end' of the selection.
    //------------------------------------------------------------------------
    virtual _Check_return_ HRESULT  GetEndTextPosition(_Out_ CTextPosition *pPosition) const = 0;

    //------------------------------------------------------------------------
    //  Summary:
    //      Retrieves the text gravity of the moving position.
    //------------------------------------------------------------------------
    virtual TextGravity GetMovingGravity() const = 0;

    //------------------------------------------------------------------------
    //  Summary:
    //      Retrieves the text gravity of the cursor, set through <Select>.
    //------------------------------------------------------------------------
    virtual TextGravity GetCursorGravity() const = 0;

    //------------------------------------------------------------------------
    //  Summary:
    //      Retrieves the text gravity of the start position.
    //------------------------------------------------------------------------
    virtual TextGravity GetStartGravity() const = 0;

    //------------------------------------------------------------------------
    //  Summary:
    //      Retrieves the text gravity of the end position.
    //------------------------------------------------------------------------
    virtual TextGravity GetEndGravity() const = 0;

    //
    // Text APIs
    //

    //------------------------------------------------------------------------
    //  Summary:
    //      Retrieves the text underlying the selection. If the selection is empty,
    //      returns a reference to the static const xstring_ptr::NullString()
    //------------------------------------------------------------------------
    virtual _Check_return_ HRESULT GetText(_Out_ xstring_ptr* pstrText) const = 0;

    //------------------------------------------------------------------------
    //  Summary:
    //      Retrieves the XAML representing the selection. If the selection is empty,
    //      or if XAML format is not supported, returns a reference to the static 
    //      const xstring_ptr::NullString()
    //------------------------------------------------------------------------
    virtual _Check_return_ HRESULT GetXaml(
        _Out_ xstring_ptr* pstrXaml
    ) const = 0;

    //
    // Selection length
    //

    //------------------------------------------------------------------------
    //  Summary:
    //      Checks whether the selection is empty (i.e. represents a caret).
    //------------------------------------------------------------------------
    virtual _Check_return_ bool IsEmpty() const = 0;

    //------------------------------------------------------------------------
    //  Summary:
    //      Retrieves the length of the selection (== 0 for a caret).
    //------------------------------------------------------------------------
    virtual _Check_return_ HRESULT GetLength(_Out_ XUINT32 *pLength) const = 0;

    //
    // Point-oriented selection APIs
    //

    //------------------------------------------------------------------------
    //  Summary:
    //      Resets the selection to represent a caret at the text position indicated
    //      by the given point.
    //------------------------------------------------------------------------
    virtual _Check_return_ HRESULT SetCaretPositionFromPoint(_In_ XPOINTF point) = 0;

    //------------------------------------------------------------------------
    //  Summary:
    //      Extends the selection by moving the 'moving position' to the text
    //      position indicated by the given point.
    //------------------------------------------------------------------------
    virtual _Check_return_ HRESULT ExtendSelectionByMouse(_In_ XPOINTF point) = 0;

    //------------------------------------------------------------------------
    //  Summary:
    //      Selects the word that falls under the given point, if any.
    //------------------------------------------------------------------------
    virtual _Check_return_ HRESULT SelectWord(_In_ XPOINTF point) = 0;

    //
    // Position-oriented selection APIs
    //

    //------------------------------------------------------------------------
    //  Summary:
    //      Resets the selection to cover the given range. XUINT32-based overload.
    //
    //  Remarks:
    //      Anchor position and moving position can be the same.
    //
    //      In an ideal world, we wouldn't have to expose an XUINT32-based overload.
    //      However, most of our code currently uses XUINT32s to represent text positions,
    //      so we can't rely on having an <CTextPosition>-only interface. Moving forward,
    //      it's likely that we'll unify our codebase to use <CTextPosition>s everywhere.
    //------------------------------------------------------------------------
    virtual _Check_return_ HRESULT Select(
        _In_ XUINT32     iAnchorTextPosition,
        _In_ XUINT32     iMovingTextPosition, 
        _In_ TextGravity eCursorGravity
    ) = 0;

    //------------------------------------------------------------------------
    //  Summary:
    //      Resets the selection to cover the given range. CTextPosition-based overload.
    //
    //  Remarks:
    //      Anchor position and moving position can be the same.
    //------------------------------------------------------------------------
    virtual _Check_return_ HRESULT Select(
        _In_ const CTextPosition &anchorTextPosition,
        _In_ const CTextPosition &movingTextPosition, 
        _In_ TextGravity          eCursorGravity
    ) = 0;

    //------------------------------------------------------------------------
    //  Summary:
    //      Extends the selection by moving the 'moving position' to the text
    //      position indicated by the given point.
    //------------------------------------------------------------------------
    virtual _Check_return_ HRESULT ExtendSelectionByMouse(_In_ const CTextPosition &cursorPosition) = 0;

    //------------------------------------------------------------------------
    //  Summary:
    //      Extends the selection by moving the 'moving position' to the text
    //      position indicated by the given point.
    //------------------------------------------------------------------------
    virtual _Check_return_ HRESULT SetCaretPositionFromTextPosition(_In_ const CTextPosition &position) = 0;

    //------------------------------------------------------------------------
    //  Summary:
    //      Extends the selection by moving the 'moving position' to the text
    //      position indicated by the given point.
    //------------------------------------------------------------------------
    virtual _Check_return_ HRESULT SelectWord(_In_ const CTextPosition &position) = 0;

    virtual void ResetSelection() = 0;
};

#endif
