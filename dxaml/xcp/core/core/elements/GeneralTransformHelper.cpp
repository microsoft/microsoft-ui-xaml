// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "GeneralTransformHelper.h"

// Invert a transform returned from TransformToVisual. Is there a better way?
xref_ptr<CGeneralTransform> GetInverseTransform(_In_ CGeneralTransform* generalTransform)
{
    xref_ptr<CGeneralTransform> returnValue;
    CGeneralTransform* inverseTransform = nullptr;

    CTransform* transform = do_pointer_cast<CTransform>(generalTransform);
    if (transform)
    {
        if (FAILED(transform->Inverse(&inverseTransform)))
        {
            return nullptr;
        }
        returnValue.attach(inverseTransform);
        return returnValue;
    }

    CInternalTransform* internalTransform = do_pointer_cast<CInternalTransform>(generalTransform);
    if (internalTransform)
    {
        if(FAILED(internalTransform->Inverse(&inverseTransform)))
        {
            return nullptr;
        }
        returnValue.attach(inverseTransform);
        return returnValue;
    }

    XAML_FAIL_FAST();
    return returnValue;
}