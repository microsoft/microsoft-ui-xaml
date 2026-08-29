// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    class Matrix3DHelper
    {
        friend class Matrix3DFactory;

    private:

        // Private helper methods
        static XDOUBLE GetDeterminant(_In_ xaml_media::Media3D::Matrix3D target);
        static XDOUBLE GetNormalizedAffineDeterminant(_In_ xaml_media::Media3D::Matrix3D target);
        static bool InvertCore(_In_ xaml_media::Media3D::Matrix3D target, _Out_ xaml_media::Media3D::Matrix3D* pReturnValue);
        static bool IsAffine(_In_ xaml_media::Media3D::Matrix3D target);
        static bool NormalizedAffineInvert(_In_ xaml_media::Media3D::Matrix3D target, _Out_ xaml_media::Media3D::Matrix3D* pReturnValue);

    };
};
