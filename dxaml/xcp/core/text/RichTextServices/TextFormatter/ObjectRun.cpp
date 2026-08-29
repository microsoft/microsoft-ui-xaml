// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      ObjectRun class implementation.

#include "precomp.h"

using namespace RichTextServices;

//---------------------------------------------------------------------------
//
//  Member:
//      ObjectRun::ObjectRun
//
//  Synopsis:
//      Initializes a new instance of the ObjectRun class.
//
//---------------------------------------------------------------------------
ObjectRun::ObjectRun(
    _In_ XUINT32 characterIndex,
        // Index of the first character of the run.
    _In_ TextRunProperties *pProperties
    ) : 
    TextRun(2, characterIndex, TextRunType::Object),
    m_pProperties(pProperties)
{
    AddRefInterface(pProperties);
}

//---------------------------------------------------------------------------
//
//  Member:
//      ObjectRun::~ObjectRun
//
//  Synopsis:
//      Releases resources owened by ObjectRun.
//
//---------------------------------------------------------------------------
ObjectRun::~ObjectRun()
{
    ReleaseInterface(m_pProperties);
}

//---------------------------------------------------------------------------
//
//  Member:
//      ObjectRun::GetProperties
//
//  Synopsis:
//      Returns the m_pProperties member.
//
//---------------------------------------------------------------------------
TextRunProperties *ObjectRun::GetProperties() const
{
    return m_pProperties;
}

