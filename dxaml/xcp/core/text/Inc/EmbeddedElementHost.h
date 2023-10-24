// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef EMBEDDED_ELEMENT_HOST
#define EMBEDDED_ELEMENT_HOST

class CInlineUIContainer;

//------------------------------------------------------------------------
//  Summary:
//     This interface specifies the contract a text element must implement to host
//     embedded UIElements.
//
//------------------------------------------------------------------------
struct IEmbeddedElementHost
{
    //------------------------------------------------------------------------
    //  Summary:
    //      Adds the given embedded element to the host. Fails if the element already
    //      exists in the host.
    //
    //  Remarks:
    //      Adding an element to a host twice is an indication of a bug, since
    //      there's currently no compelling scenario for it. So we 0fail instead of silently
    //      ignoring it.
    // 
    //------------------------------------------------------------------------
    virtual _Check_return_ HRESULT AddElement(_In_ CInlineUIContainer *pContainer) = 0;

    //------------------------------------------------------------------------
    //  Summary:
    //        Gets a value indicating whether the host allows adding
    //        an element in its current state.
    // 
    //------------------------------------------------------------------------
    virtual bool CanAddElement() const = 0;

    //------------------------------------------------------------------------
    //  Summary:
    //      Removes the given embedded element from the host. Fails if the element 
    //      does not exist in the host.
    //
    //  Remarks:
    //      Removing a non-existent element from a host is an indication of a bug. So
    //      we fail instead of silently ignoring it.
    // 
    //------------------------------------------------------------------------
    virtual _Check_return_ HRESULT RemoveElement(_In_ CInlineUIContainer *pContainer) = 0;

    //------------------------------------------------------------------------
    //  Summary:
    //      Updates the given embedded element position. Fails if the element does 
    //      not exist in the host.
    //  
    //  Parameters:
    //      position - The position in host element space.
    // 
    //------------------------------------------------------------------------
    virtual _Check_return_ HRESULT UpdateElementPosition(
        _In_       CInlineUIContainer *pContainer,
        _In_ const XPOINTF    &position) = 0;   

    //------------------------------------------------------------------------
    //  Summary:
    //      Retrieves the embedded element position in host space. Fails if the element
    //      does not exist in the host.
    //
    //------------------------------------------------------------------------
    virtual _Check_return_ HRESULT GetElementPosition(
        _In_  CInlineUIContainer *pContainer,
        _Out_ XPOINTF    *pPosition) = 0;

    //------------------------------------------------------------------------
    //  Summary:
    //      Retrieves the available size of the host. The size is used for measuring
    //      embedded elements.
    //------------------------------------------------------------------------
    virtual const XSIZEF & GetAvailableMeasureSize() const = 0;
};

#endif
