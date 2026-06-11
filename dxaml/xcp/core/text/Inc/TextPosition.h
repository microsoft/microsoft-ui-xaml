// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef TEXT_POSITION_H
#define TEXT_POSITION_H

#ifndef NO_HEADER_INCLUDES

#include "PlainTextPosition.h"

#endif

class CTextPosition
{
public:

    CTextPosition();

    CTextPosition(const CPlainTextPosition& position);

    _Check_return_ HRESULT GetOffset(_Out_ XUINT32 *pOffset) const;

    _Check_return_ bool IsValid() const;

    _Check_return_ bool Equals(_In_ const CTextPosition &other) const;
    _Check_return_ bool LessThan(_In_ const CTextPosition &other) const;

    _Check_return_ HRESULT IsAtInsertionPosition(
        _Out_ bool *pIsAtInsertionPosition) const;

    _Check_return_ HRESULT GetNextInsertionPosition(
        _Out_ bool         *pFoundPosition,
        _Out_ CTextPosition *pPosition) const;

    _Check_return_ HRESULT GetPreviousInsertionPosition(
        _Out_ bool         *pFoundPosition,
        _Out_ CTextPosition *pPosition) const;

    _Check_return_ HRESULT GetBackspacePosition(
        _Out_ bool         *pFoundPosition,
        _Out_ CTextPosition *pPosition) const;

    _Check_return_ const CPlainTextPosition& GetPlainPosition() const;

    _Check_return_ HRESULT IsAfterLineBreak(_Out_ bool *pIsAfterLineBreak) const;
    _Check_return_ HRESULT IsInsideLineBreak(_Out_ bool *pIsInsideLineBreak) const;

private:
    CPlainTextPosition m_plainPosition;
};

/*
Overloaded comparison operators for convenience. They're all defined in terms
of CTextPosition::Equals and CTextPosition::LessThan.
*/

inline bool operator==(const CTextPosition& lhs, const CTextPosition& rhs)
{
    return lhs.Equals(rhs);
}

inline bool operator!=(const CTextPosition& lhs, const CTextPosition& rhs)
{
    return !operator==(lhs, rhs);
}

inline bool operator<(const CTextPosition& lhs, const CTextPosition& rhs)
{
    return lhs.LessThan(rhs);
}

inline bool operator<=(const CTextPosition& lhs, const CTextPosition& rhs)
{
    return operator<(lhs, rhs) || operator==(lhs, rhs);
}

inline bool operator>(const CTextPosition& lhs, const CTextPosition& rhs)
{
    return !operator<=(lhs, rhs);
}

inline bool operator>=(const CTextPosition& lhs, const CTextPosition& rhs)
{
    return !operator<(lhs, rhs);
}

#endif // NO_HEADER_INCLUDES
