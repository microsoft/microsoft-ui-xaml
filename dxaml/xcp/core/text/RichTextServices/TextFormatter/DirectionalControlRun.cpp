// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      DirectionalControlRun class implementation.

#include "precomp.h"

using namespace RichTextServices;

//---------------------------------------------------------------------------
//
//  Member:
//      DirectionalControlRun::DirectionalControlRun
//
//  Synopsis:
//      Initializes a new instance of the DirectionalControlRun class.
//
//---------------------------------------------------------------------------
DirectionalControlRun::DirectionalControlRun(
    _In_ XUINT32 characterIndex,
        // Index of the first character of the run.
    _In_ DirectionalControl::Enum control
        // DirectionalControl type specified by this run.
    ) :
    TextRun(1, characterIndex, TextRunType::DirectionalControl),
    m_control(control)
{
}

//---------------------------------------------------------------------------
//
//  Member:
//      DirectionalControlRun::~DirectionalControlRun
//
//  Synopsis:
//      Release resources associated with this instance of DirectionalControlRun.
//
//---------------------------------------------------------------------------
DirectionalControlRun::~DirectionalControlRun()
{
}
