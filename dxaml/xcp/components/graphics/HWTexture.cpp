// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "HWTextureMgr.h"
#include <DCompSurface.h>

void HWTexture::AddTextureSpaceToLocalSpaceTransform(_Inout_ CMILMatrix *pTextureSpaceToLocalSpaceTransform)
{
    //
    // This will be used to texture from a DComp surface, so we want to exclude the gutters here.
    // The DComp surface will have its top-left at (0, 0), so there is no need to offset anything.
    // Just scale by the width and height of the DComp surface, ignoring gutters.
    //

    CMILMatrix scaleTransform(TRUE);
    scaleTransform.Scale(
        1 / static_cast<XFLOAT>(m_pDeviceSurface->GetWidthWithoutGutters()),
        1 / static_cast<XFLOAT>(m_pDeviceSurface->GetHeightWithoutGutters())
        );

    pTextureSpaceToLocalSpaceTransform->Prepend(scaleTransform);
}
