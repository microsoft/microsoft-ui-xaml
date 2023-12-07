// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Method:   CLineBreak::GetRun
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CLineBreak::GetRun(
    _In_                              XUINT32               characterPosition,
    _Out_opt_                   const TextFormatting      **ppTextFormatting,
    _Out_opt_                   const InheritedProperties **ppInheritedProperties,
    _Out_opt_                         TextNestingType      *pNestingType,
    _Out_opt_                         CTextElement        **ppNestedElement,
    _Outptr_result_buffer_(*pcCharacters) const WCHAR         **ppCharacters,
    _Out_                             XUINT32              *pcCharacters
)
{
    // LineBreak only has 2 positions - Open/Close.
    ASSERT(characterPosition <= 2);
    if (characterPosition >= 2)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    if (characterPosition == 0)
    {
        *ppCharacters = NULL;
    }
    else
    {
        *ppCharacters = const_cast<WCHAR*>(L"\x2028"); //Warning! Casting constness away!
    }
    *pcCharacters = 1;

    // LineBreak only has 2 positions. The first position is treated as a hidden run with no characters and a count of 1.
    if (ppTextFormatting)
    {
        IFC_RETURN(GetTextFormatting(ppTextFormatting));
    }

    if (ppInheritedProperties)
    {
        IFC_RETURN(GetInheritedProperties(ppInheritedProperties));
    }

    if (ppNestedElement)
    {
        *ppNestedElement = this;
    }

    if (pNestingType)
    {
        *pNestingType = ((characterPosition == 0) ? OpenNesting : NestedContent);
    }

    return S_OK;
}
