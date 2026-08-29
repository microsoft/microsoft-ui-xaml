// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "Result.h"

namespace RichTextServices
{
    class TextBreak; 

    //---------------------------------------------------------------------------
    //
    //  ILinkedTextContainer
    //
    //  An interface used by text controls to implement linking behavior.
    //
    //---------------------------------------------------------------------------
    struct ILinkedTextContainer
    {
        //---------------------------------------------------------------------------
        //
        //  Member:
        //      ILinkedTextContainer::GetPrevious
        //
        //  Synopsis:
        //      Gets the ILinkedTextContainer preceding this one.
        //
        //---------------------------------------------------------------------------
        virtual
        ILinkedTextContainer *GetPrevious() const = 0;

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      ILinkedTextContainer::GetNext
        //
        //  Synopsis:
        //      Gets the ILinkedTextContainer following this one.
        //
        //---------------------------------------------------------------------------
        virtual
        ILinkedTextContainer *GetNext() const = 0;

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      ILinkedTextContainer::GetBreak
        //
        //  Synopsis:
        //      Gets the breaking information for this container.
        //
        //---------------------------------------------------------------------------
        virtual
        TextBreak *GetBreak() const = 0;

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      ILinkedTextContainer::IsBreakValid
        //
        //  Synopsis:
        //      Gets a value indicating whether the break information for this
        //      container is valid.
        //
        //---------------------------------------------------------------------------
        virtual
        bool IsBreakValid() const = 0;

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      ILinkedTextContainer::PreviousBreakUpdated
        //
        //  Synopsis:
        //      Used by a linked container to handle updates to the previous 
        //      container's break record.
        //
        //---------------------------------------------------------------------------
        virtual
        Result::Enum PreviousBreakUpdated(
            _In_ ILinkedTextContainer *pPrevious
                // Container fulfilling the break request.
            ) = 0;

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      ILinkedTextContainer::PreviousLinkAttached
        //
        //  Synopsis:
        //      Notifies a linked container that it has been linked as the next 
        //      link for another container.
        //
        //---------------------------------------------------------------------------
        virtual
        Result::Enum PreviousLinkAttached(
            _In_ ILinkedTextContainer *pPrevious
                // Container that has set this one as its next link.
            ) = 0;

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      ILinkedTextContainer::NextLinkDetached
        //
        //  Synopsis:
        //      Notifies a linked container that it has been removed as the previous 
        //      link for another container.
        //
        //---------------------------------------------------------------------------
        virtual
        Result::Enum NextLinkDetached(
            _In_ ILinkedTextContainer *pNext
                // Container that formerly had this one as its previous link.
            ) = 0;

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      ILinkedTextContainer::PreviousLinkDetached
        //
        //  Synopsis:
        //      Notifies a linked container that it has been removed as the next 
        //      link for another container.
        //
        //---------------------------------------------------------------------------
        virtual
        Result::Enum PreviousLinkDetached(
            _In_ ILinkedTextContainer *pPrevious
                // Container that formerly had this one as its next link.
            ) = 0;

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      ILinkedTextContainer::IsMaster
        //
        //  Synopsis:
        //      Gets a value indicating whether this is the master control in a linked
        //      text scenario, i.e. whether it is the original owner of content.
        //
        //---------------------------------------------------------------------------
        virtual
        bool IsMaster() const = 0;
    };
}
