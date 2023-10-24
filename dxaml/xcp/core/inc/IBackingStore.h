// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef IBACKING_STORE_H
#define IBACKING_STORE_H

class CTextBoxBase;
struct ITextContainer;
class CTextPosition;

//
// This interface is the TextEditor's contract with the backing store for editing, undo needs.
//
struct IBackingStore
{
    // Used by ChangeBlockRecord
    virtual bool IsTextDirty() = 0;
    virtual void ClearTextDirty() = 0;
    virtual CTextBoxBase *GetTextControl() = 0;

    // Used by UndoManager/UndoUnit
    virtual _Check_return_ HRESULT GetText(
        _In_                                  XUINT32   iTextPosition1, 
        _In_                                  XUINT32   iTextPosition2, 
        _Out_                                 XUINT32  *pcCharacters,
        _Outptr_result_buffer_maybenull_(*pcCharacters) const WCHAR **ppCharacters) = 0;

    virtual _Check_return_ HRESULT DeleteText(_In_ XUINT32 iTextPosition1, _In_ XUINT32 iTextPosition2) = 0;

    virtual _Check_return_ HRESULT InsertText(
        _In_                         XUINT32  iTextPosition,
        _In_                         XUINT32  cCharacters,
        _In_reads_opt_(cCharacters) WCHAR   *pCharacters) = 0;
    
    // Used by TextEditor command handlers
    virtual XUINT32 GetStartTextPosition() = 0;
    virtual XUINT32 GetEndTextPosition() = 0;
    virtual _Check_return_ HRESULT CreateTextPositionFromOffset(
        _In_  XUINT32        offset,
        _Out_ CTextPosition *pPosition) = 0;

    // Remove when SelectionWordBreaker is extended to work with RTB/generalized CTextPosition interface. PS #73242.
    virtual ITextContainer *GetContainer() = 0; 
};

#endif
