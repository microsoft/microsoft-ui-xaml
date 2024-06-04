// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Defines an AsyncAction interface that can be called in the core and translate up to a
// WinRT IAsyncAction in the wrapping framework.  Revisit when we consider refactoring
// Core and the DXAML layer

#pragma once

#include <FocusMovement.h>

class ICoreAsyncAction
{
public:
    virtual bool CoreContinueAsyncAction() = 0;
    virtual void CoreFireCompletion() = 0;
    virtual void CoreReleaseRef() = 0;
    virtual void CoreSetError(HRESULT hr) = 0;
};

template< typename TRESULT >
class ICoreAsyncOperation : public ICoreAsyncAction
{
public:
    virtual _Check_return_ HRESULT CoreSetResults(TRESULT results) = 0;
};

class ICoreAsyncFocusOperation : public ICoreAsyncOperation<Focus::FocusMovementResult>
{
public:
    virtual const GUID GetCorrelationId() const = 0;
};