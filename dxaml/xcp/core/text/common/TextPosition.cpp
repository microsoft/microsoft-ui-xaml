// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

CTextPosition::CTextPosition()
{
}

CTextPosition::CTextPosition(const CPlainTextPosition& position)
{
    m_plainPosition = position;
}

_Check_return_ HRESULT CTextPosition::GetOffset(_Out_ XUINT32 *pOffset) const
{
    RRETURN(m_plainPosition.GetOffset(pOffset));
}

_Check_return_ bool CTextPosition::IsValid() const
{
    return m_plainPosition.IsValid();
}

_Check_return_ bool CTextPosition::Equals(_In_ const CTextPosition &other) const
{
    return m_plainPosition.Equals(other.m_plainPosition);
}

_Check_return_ bool CTextPosition::LessThan(_In_ const CTextPosition &other) const
{
    return m_plainPosition.LessThan(other.m_plainPosition);
}

_Check_return_ HRESULT CTextPosition::IsAtInsertionPosition(
    _Out_ bool *pIsAtInsertionPosition) const
{
    return m_plainPosition.IsAtInsertionPosition(pIsAtInsertionPosition);
}

_Check_return_ HRESULT CTextPosition::GetNextInsertionPosition(
    _Out_ bool         *pFoundPosition,
    _Out_ CTextPosition *pPosition) const
{
    CPlainTextPosition position;
    IFC_RETURN(m_plainPosition.GetNextInsertionPosition(pFoundPosition, &position));
    *pPosition = position;

    return S_OK;
}

_Check_return_ HRESULT CTextPosition::GetPreviousInsertionPosition(
    _Out_ bool         *pFoundPosition,
    _Out_ CTextPosition *pPosition) const
{
    CPlainTextPosition position;
    IFC_RETURN(m_plainPosition.GetPreviousInsertionPosition(pFoundPosition, &position));
    *pPosition = position;

    return S_OK;
}

_Check_return_ HRESULT CTextPosition::GetBackspacePosition(
    _Out_ bool         *pFoundPosition,
    _Out_ CTextPosition *pPosition) const
{
    CPlainTextPosition position;
    IFC_RETURN(m_plainPosition.GetBackspacePosition(pFoundPosition, &position));
    *pPosition = position;

    return S_OK;
}

_Check_return_ const CPlainTextPosition& CTextPosition::GetPlainPosition() const
{
    return m_plainPosition;
}

_Check_return_ HRESULT CTextPosition::IsAfterLineBreak(_Out_ bool *pIsAfterLineBreak) const
{
    // TODO: May be needed in RichTextBlock/TextBlock selection scenarios.
    *pIsAfterLineBreak = FALSE;
    RRETURN(S_OK);
}

_Check_return_ HRESULT CTextPosition::IsInsideLineBreak(_Out_ bool *pIsInsideLineBreak) const
{
    IFC_RETURN(m_plainPosition.IsInsideLineBreak(pIsInsideLineBreak));

    return S_OK;
}
