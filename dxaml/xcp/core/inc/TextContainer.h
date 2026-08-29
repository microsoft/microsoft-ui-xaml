// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Definition of ITextContainer and ITextEditableContainer interfaces.
//      They define the contract for the backing store of text controls.
//
//      ITextContainer defines APIs for a read-only TextContainer - TextBlock.
//      ITextEditableContainer extends ITextContainer, adding edit APIs
//      for an editable TextContainer - TextBox.

#pragma once
#ifndef TEXT_CONTAINER_H
#define TEXT_CONTAINER_H

#include "TextElement.h"
#include "IBackingStore.h"

#ifndef ENUM_LOGICAL_DIRECTION
#define ENUM_LOGICAL_DIRECTION
enum LogicalDirection
{
    Forward = 0,
    Backward,
};
#endif

//------------------------------------------------------------------------
//
//  Interface:  ITextContainer
//
//  Synopsis:
//      APIs that provide access to character content
//
//------------------------------------------------------------------------

struct ITextContainer
{
    //------------------------------------------------------------------------
    //
    //  Method:   ITextContainer::GetCore
    //
    //  Synopsis: Returns the CoreServices object.
    //
    //------------------------------------------------------------------------

    virtual CCoreServices *GetCore() = 0;

    virtual CDependencyObject *GetAsDependencyObject() = 0;

    //------------------------------------------------------------------------
    //
    //  Method:   ITextContainer::GetPositionCount
    //
    //  Synopsis: Returns the number of character positions covered by the
    //            text content.
    //
    //  Each UTF-16 code unit takes a character position. Additionally, every
    //  Inline or InlineCollection reserves 2 positions corresponding
    //  to its start and end.
    //
    //------------------------------------------------------------------------

    virtual void GetPositionCount(_Out_ XUINT32 *pcPositions) = 0;


    //------------------------------------------------------------------------
    //
    //  Method:   ITextContainer::GetRun
    //
    //  Synopsis: Returns a run of characters all of the same format starting
    //            at the given position.
    //
    //  If the startPosition corresponds to the (reserved) start or end
    //  position of an Inline, GetRun returns
    //    1) *ppCharacters == NULL,
    //    2) *pTextFormatting == NULL,
    //    3) *pcCharacters == number of reserved positions (1 for start or end of Inline).
    //
    //  If the startPosition corresponds to a (UTF-16) code unit, GetRun
    //  returns
    //    1) *ppCharacters points to that code unit,
    //    2) *pcCharacters is set to the length of the longest character run
    //       that is contiguous in memory, and for which all code units share
    //       the same formatting,
    //    3) *pFormatting points to the format sheared by the code units at
    //       *ppCharacters. Inheritance of formatting properties is already
    //       resolved.
    //
    //------------------------------------------------------------------------

    virtual _Check_return_ HRESULT GetRun(
        _In_                              XUINT32               characterPosition,
        _Out_opt_                   const TextFormatting      **ppTextFormatting,
        _Out_opt_                   const InheritedProperties **ppInheritedProperties,
        _Out_opt_                         TextNestingType      *pNestingType,
        _Outptr_result_maybenull_                   CTextElement        **ppNestedElement,
        _Outptr_result_buffer_(*pcCharacters) const WCHAR         **ppCharacters,
        _Out_                             XUINT32              *pcCharacters
    ) = 0;


    //------------------------------------------------------------------------
    //
    //  Method:   ITextContainer::GetText
    //
    //  Synopsis: Concatenates all text content into a single flat string.
    //
    //------------------------------------------------------------------------

    virtual _Check_return_ HRESULT GetText(
        _In_                                  bool     insertNewlines,
        _Out_                                 XUINT32  *pcCharacters,
        _Outptr_result_buffer_maybenull_(*pcCharacters) const WCHAR **ppCharacters,
        _Out_                                 bool    *pIsTakeOwnership
    ) = 0;

    //------------------------------------------------------------------------
    //
    //  Method:   ITextContainer::GetText
    //
    //  Synopsis: Concatenates all text content between 2 offsets into a
    //            flat string.
    //
    //------------------------------------------------------------------------
    virtual _Check_return_ HRESULT GetText(
        _In_                                  XUINT32   iTextPosition1,
        _In_                                  XUINT32   iTextPosition2,
        _In_                                  bool     insertNewlines,
        _Out_                                 XUINT32  *pcCharacters,
        _Outptr_result_buffer_maybenull_(*pcCharacters) const WCHAR **ppCharacters,
        _Out_                                 bool    *pIsTakeOwnership
    ) = 0;

    //------------------------------------------------------------------------
    //
    //  Method:   ITextContainer::GetContainingElement
    //
    //  Synopsis: Returns the CTextElement that contains a text position.
    //
    //  If the startPosition corresponds to the (reserved) start or end
    //  position of an element, that element will be returned.
    //  If the position is within the content (i.e. non-start or end position)
    //  of an element, that element returned.
    //  The containing element is always the closest contatining element,
    //  or the immediate parent of a position, not any other ancestor.
    //  For collections that reserve positions e.g. InlineCollection reserves
    //  its start/end positions, the containing element is NULL, because
    //  a collection can't be considered an element. It is the responsibility
    //  of parents of InlineCollection to handle this case.
    //
    //------------------------------------------------------------------------
    virtual _Check_return_ HRESULT GetContainingElement(
        _In_                              XUINT32               characterPosition,
        _Outptr_                       CTextElement        **ppContainingElement
    ) = 0;

    //------------------------------------------------------------------------
    //
    //  Method:   ITextContainer::GetElementEdgeOffset
    //
    //  Synopsis: Returns the offset of the TextElement at the specified edge
    //            in the container. If the element is not found, *pFound is 
    //            FALSE.
    //
    //------------------------------------------------------------------------
    virtual _Check_return_ HRESULT GetElementEdgeOffset(
        _In_ CTextElement *pElement,
        _In_ ElementEdge edge, 
        _Out_ XUINT32 *pOffset,
        _Out_ bool *pFound
    ) = 0;

    //------------------------------------------------------------------------
    //
    //  Method:   ITextContainer::GetOwnerUIElement
    //
    //  Synopsis: Get the owner UIElement so that we can trigger a re-layout when a font download complete
    //
    //------------------------------------------------------------------------

    virtual CUIElement *GetOwnerUIElement() = 0;

    virtual ~ITextContainer()
    {
    }
};

#endif
