// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Interface:  IDirectManipulationContainer interface
//
//  Synopsis:
//    Interface implemented by UI elements that allow some inner element(s)
//    to be manipulated with DirectManipulation. The ScrollViewer is one
//    example.

#pragma once

// Keep this enum in sync with the DMManipulationState enum in DirectManipulationTypes.h
enum DirectManipulationState
{
    ManipulationNone      = 0,
    ManipulationStarting  = 1,
    ManipulationStarted   = 2,
    ManipulationDelta     = 3,
    ManipulationLastDelta = 4,
    ManipulationCompleted = 5,
    ConstantVelocityScrollStarted = 6,
    ConstantVelocityScrollStopped = 7    
};
