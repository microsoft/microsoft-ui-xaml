// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//      Portable definition of HRESULT codes

#include <winerror.h>

#ifdef RC_INVOKED
#define _HRESULT_TYPEDEF_(_sc) _sc
#else
#define _HRESULT_TYPEDEF_(_sc) ((HRESULT)_sc)
#endif

// MessageId: E_NOT_SUPPORTED
//
// MessageText:
//
// This operation is not supported.
//
#ifndef E_NOT_SUPPORTED
#define E_NOT_SUPPORTED                  _HRESULT_TYPEDEF_(0x80131515L)
#endif

// different than above, used widely in OS for general error reporting
#ifndef E_NOTSUPPORTED
#define E_NOTSUPPORTED HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED)
#endif

//
// Webcam/Microphone error codes.
//

//
// The capture device requested is already in use.
//
#ifndef E_CAPTURE_DEVICE_IN_USE
#define E_CAPTURE_DEVICE_IN_USE         _HRESULT_TYPEDEF_(0x800FF000L)
#endif

//
// The capture device was removed while it was in use.
//
#ifndef E_CAPTURE_DEVICE_REMOVED
#define E_CAPTURE_DEVICE_REMOVED        _HRESULT_TYPEDEF_(0x800FF001L)
#endif

//
// Access was denied to the capture device due to insufficient privelages.
//
#ifndef E_CAPTURE_DEVICE_ACCESS_DENIED
#define E_CAPTURE_DEVICE_ACCESS_DENIED  _HRESULT_TYPEDEF_(0x800FF002L)
#endif

//
// Invalid operation in current state for capture source, state must be stopped.
//
#ifndef E_CAPTURE_SOURCE_NOT_STOPPED
#define E_CAPTURE_SOURCE_NOT_STOPPED    _HRESULT_TYPEDEF_(0x800FF003L)
#endif

//
// The capture device was not available for an unspecified reason.
//
#ifndef E_CAPTURE_DEVICE_NOT_AVAILABLE
#define E_CAPTURE_DEVICE_NOT_AVAILABLE  _HRESULT_TYPEDEF_(0x800FF004L)
#endif

//
// The GPU device instance has been suspended.
//
#ifndef E_GRAPHICS_DEVICE_REMOVED
#define E_GRAPHICS_DEVICE_REMOVED        _HRESULT_TYPEDEF_(0x887A0005L)
#endif

