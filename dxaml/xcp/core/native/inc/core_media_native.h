// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//      Contains the Jupiter media interfaces

#ifndef CORE_MEDIA_NATIVE_H
#define CORE_MEDIA_NATIVE_H

#include <Windows.Media.Casting.h>
#include <Windows.Media.Playback.h>
#include <MFSharingEngine.h>
#include <NamespaceAliases.h>

struct IPALGraphicDevice;
struct IUnknown;

//------------------------------------------------------------------------
//
//  Synopsis:
//    Interface to represent the Image's method of working with the
//    IMFImageSharingEngine for PlayTo
//
//------------------------------------------------------------------------
struct IXcpImageSharingEngine : public IObject
{
    virtual _Check_return_ HRESULT SetSource( _In_ IPALSurface *pSoftwareSurface) = 0;
    virtual void Shutdown() = 0;
    virtual _Check_return_ HRESULT Connect(_In_opt_ IMFSharingEngineClassFactory *pFactory, bool createSharingEngine) = 0;
    virtual _Check_return_ HRESULT Disconnect() = 0;
    virtual _Check_return_ HRESULT InitializeCasting(_In_ IInspectable *pCastingEngine, _In_ IUnknown *pMediaEngineCallback, _In_ wm::Casting::ICastingSource *pSource) = 0;
};

//--------------------------------------------------------------------------
//
//  Synopsis:
//      Interface for an Async callback mechanism for IXcpImageSharingEngine
//      to talk back to the UI.
//
//--------------------------------------------------------------------------
struct IXcpImageSharingEngineNotify
{
   virtual _Check_return_ HRESULT OnConnected() = 0;
   virtual _Check_return_ HRESULT OnDisconnected() = 0;
};

#endif
