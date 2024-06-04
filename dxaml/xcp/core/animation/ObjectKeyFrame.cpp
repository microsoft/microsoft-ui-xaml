// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ObjectKeyFrame.h"

CObjectKeyFrame::~CObjectKeyFrame()
{
    // CDependencyObject::ResetReferencesFromChildren() does not clean up this member, so we clean up here
    // if the parent of the ValuePointer is the object itself, we clear the parent on that object to prevent having a dangling pointer
    CDependencyObject* valueAsObject = m_vValue.AsObject();

    if (valueAsObject)
    {
        IGNOREHR(ResetReferenceFromChild(valueAsObject));
    }
}
