// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef PLAIN_TEXT_POSITION_H
#define PLAIN_TEXT_POSITION_H

#include <enumdefs.h>

struct ITextView;
struct ITextContainer;
class CFrameworkElement;

/*
Summary:
    A lightweight, integer-based text position for plain-text scenarios.

Remarks:
    When a position is invalid, all calls fail except:
        * IsValid: returns FALSE
        * Equals: returns FALSE
        * LessThan: returns FALSE
*/
class CPlainTextPosition
{

    friend class CSelectionWordBreaker;

public:
    CPlainTextPosition();
    CPlainTextPosition(
        _In_ ITextContainer *pContainer,
        _In_ XUINT32         offset,
        _In_ TextGravity     gravity);
    
    // Copy constructor.
    CPlainTextPosition(const CPlainTextPosition &initializer);

    _Check_return_ HRESULT GetOffset(_Out_ XUINT32 *pOffset) const;
    _Check_return_ HRESULT GetGravity(_Out_ TextGravity *pGravity) const;
    _Check_return_ HRESULT GetCharacterRect(
        _In_ TextGravity gravity,
        _Out_ XRECTF *pRect
        ) const;
    ITextContainer *GetTextContainer() const;
    _Check_return_ HRESULT GetLogicalParent(_Outptr_ CDependencyObject **ppParent) const;
    _Check_return_ HRESULT GetVisualParent(_Outptr_ CFrameworkElement **ppParent) const;

    _Check_return_ bool IsValid() const;

    _Check_return_ bool Equals(_In_ const CPlainTextPosition &other) const;
    _Check_return_ bool LessThan(_In_ const CPlainTextPosition &other) const;

    _Check_return_ HRESULT IsAtInsertionPosition(_Out_ bool *pIsAtInsertionPosition) const;

    _Check_return_ HRESULT GetNextInsertionPosition(
        _Out_ bool              *pFoundPosition,
        _Out_ CPlainTextPosition *pPosition) const;

    _Check_return_ HRESULT GetPreviousInsertionPosition(
        _Out_ bool              *pFoundPosition,
        _Out_ CPlainTextPosition *pPosition) const;

    _Check_return_ HRESULT GetBackspacePosition(
        _Out_ bool         *pFoundPosition,
        _Out_ CPlainTextPosition *pPosition) const;

    _Check_return_ HRESULT GetPositionAtOffset(
        _In_ XINT32 offset,
        _In_ TextGravity gravity,
        _Out_ bool *pFoundPosition,
        _Out_ CPlainTextPosition *pPosition) const;

    _Check_return_ HRESULT IsInsideLineBreak(_Out_ bool *pIsInsideLineBreak) const;

    _Check_return_ HRESULT Clone(_Out_ CPlainTextPosition *pPosition) const;


protected:
    ITextView *GetTextView() const;

private:
    _Check_return_ HRESULT MoveToNextInsertionPosition(_Out_ bool *pFoundPosition);
    _Check_return_ HRESULT MoveToPreviousInsertionPosition(_Out_ bool *pFoundPosition);
    _Check_return_ HRESULT MoveToBackspacePosition(_Out_ bool *pFoundPosition);

    _Check_return_ HRESULT CheckValid() const;

    _Check_return_ HRESULT MoveByOffset(
        _In_ XINT32 offset,
        _In_ TextGravity gravity,
        _Out_ bool *pFoundPosition);
    
private:
    ITextContainer *m_pContainer;
    XUINT32         m_offset;
    TextGravity     m_gravity;
};

/*
Overloaded comparison operators for convenience. They're all defined in terms
of CPlainTextPosition::Equals and CPlainTextPosition::LessThan.
*/

inline bool operator==(const CPlainTextPosition& lhs, const CPlainTextPosition& rhs)
{
    return lhs.Equals(rhs);
}

inline bool operator!=(const CPlainTextPosition& lhs, const CPlainTextPosition& rhs)
{
    return !operator==(lhs, rhs);
}

inline bool operator<(const CPlainTextPosition& lhs, const CPlainTextPosition& rhs)
{
    return lhs.LessThan(rhs);
}

inline bool operator<=(const CPlainTextPosition& lhs, const CPlainTextPosition& rhs)
{
    return operator<(lhs, rhs) || operator==(lhs, rhs);
}

inline bool operator>(const CPlainTextPosition& lhs, const CPlainTextPosition& rhs)
{
    return !operator<=(lhs, rhs);
}

inline bool operator>=(const CPlainTextPosition& lhs, const CPlainTextPosition& rhs)
{
    return !operator<(lhs, rhs);
}

#endif // PLAIN_TEXT_POSITION_H
