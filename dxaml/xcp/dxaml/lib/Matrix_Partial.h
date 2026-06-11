// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    class MatrixHelper
    {
        friend class MatrixFactory;

    public:
        // Replaces target matrix with the product of itself and the incoming CMILMatrix.
        static void Append(_Inout_ xaml_media::Matrix& target, _In_ CMILMatrix that);

        static void GetScaleDimensions(
            _In_ const xaml_media::Matrix& matrix,
            _Out_ XFLOAT *pScaleX,
            _Out_ XFLOAT *pScaleY
            );

        // Converts a Matrix to a CMILMatrix.
        static CMILMatrix ToCMILMatrix(_In_ const xaml_media::Matrix &source);

    private:

        // Private helper method
        static void MultiplyPoint(_In_ xaml_media::Matrix target, _Inout_ XFLOAT* pX, _Inout_ XFLOAT* pY);

        

        // Converts a CMILMatrix to a Matrix.
        static xaml_media::Matrix FromCMILMatrix(_In_ CMILMatrix source);
    };
};
