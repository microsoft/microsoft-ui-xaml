// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Base interface for drawing contexts supporting PC, D2D and SW rendering.

#include "precomp.h"
#include "DrawingContext.h"

//------------------------------------------------------------------------
// 
// Initializes a new instance of the DrawingContext class.
//
//------------------------------------------------------------------------
DrawingContext::DrawingContext()
    : m_transform(TRUE)
{
}

//------------------------------------------------------------------------
// 
// Release resources associated with the DrawingContext.
//
//------------------------------------------------------------------------
DrawingContext::~DrawingContext()
{
}
