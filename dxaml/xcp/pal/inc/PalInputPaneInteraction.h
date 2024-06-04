// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Interface:  IPALInputPaneInteraction
//  Synopsis: InputPane registration, unregistration

#ifndef __PAL__INPUTPANE__INTERACTION
#define __PAL__INPUTPANE__INTERACTION

class CContentRoot;
struct IPALInputPaneInteraction : public IObject
{
    virtual _Check_return_ HRESULT RegisterInputPaneHandler(_In_ XHANDLE hWindow, _In_ CContentRoot* contentRoot) = 0;
    virtual _Check_return_ HRESULT UnregisterInputPaneHandler() = 0;
};

#endif //__PAL__INPUTPANE__INTERACTION



