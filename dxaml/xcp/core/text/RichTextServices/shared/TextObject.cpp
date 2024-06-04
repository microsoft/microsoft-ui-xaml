// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextObject.h"

using namespace RichTextServices;

//---------------------------------------------------------------------------
//
//  Member:
//      TextObject::TextObject
//
//  Synopsis:
//      Constructor.
//
//---------------------------------------------------------------------------
TextObject::TextObject()
    : m_referenceCount(1)
{
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextObject::~TextObject
//
//  Synopsis:
//      Destructor.
//
//---------------------------------------------------------------------------
TextObject::~TextObject()
{
    // TODO: this assert fires if a TextSource is held as a member (not pointer, but just instance) of a ParagraphNode - it
    // is delete in the dtor without calling release. Assert seems too aggressive in this case - that is valid behavior. 
    //ASSERT(m_referenceCount == 0);
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextObject::AddRef
//
//  Synopsis:
//      Increments the count of references to this object.
//
//  Returns:
//      The object's new reference count.
//
//---------------------------------------------------------------------------
XUINT32 TextObject::AddRef()
{
    return ++m_referenceCount;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextObject::Release
//
//  Synopsis:
//      Decrements the count of references to this object.
//
//  Returns:
//      The object's new reference count.
//
//---------------------------------------------------------------------------
XUINT32 TextObject::Release()
{
    ASSERT(m_referenceCount > 0);

    XUINT32 referenceCount = --m_referenceCount;

    if (0 == m_referenceCount)
    {
        delete this;
    }

    return referenceCount;
}
