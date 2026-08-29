// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace RichTextServices
{
    //---------------------------------------------------------------------------
    //
    //  TextObject
    //
    //  TextObject is the common base class for text stack components.
    //  It provides ref counting services for acquiring/releasing
    //  references to objects.
    //
    //---------------------------------------------------------------------------
    class TextObject
    {
    public:
        
        // Increments the count of references to this object.
        XUINT32 AddRef();

        // Decrements the count of references to this object.
        XUINT32 Release();

    protected:

        // Constructor.
        TextObject();

        // Destructor.
        virtual ~TextObject();

        // Gets the current reference count.
        XUINT32 GetReferenceCount() const;

    private:

        // Count of references to this instance.
        XUINT32 m_referenceCount;
    };

    //---------------------------------------------------------------------------
    //
    //  Member:
    //      TextObject::GetReferenceCount
    //
    //  Returns:
    //      The current reference count.
    //
    //---------------------------------------------------------------------------
    inline
    XUINT32 TextObject::GetReferenceCount() const
    {
        return m_referenceCount;
    }
}
