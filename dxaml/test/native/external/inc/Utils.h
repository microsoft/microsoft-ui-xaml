// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {

    inline bool operator== (const ::Windows::UI::Color& firstColor, const ::Windows::UI::Color& secondColor)
    {
        return ((firstColor.A == secondColor.A) && 
                (firstColor.R == secondColor.R) &&
                (firstColor.G == secondColor.G) && 
                (firstColor.B == secondColor.B));
    }

    inline bool operator== (const Microsoft::UI::Xaml::Thickness& firstThickness, const Microsoft::UI::Xaml::Thickness& secondThickness)
    {
        return ((firstThickness.Left == secondThickness.Left) &&
            (firstThickness.Top == secondThickness.Top) &&
            (firstThickness.Right == secondThickness.Right) &&
            (firstThickness.Bottom == secondThickness.Bottom));
    }

    inline bool operator== (const Microsoft::UI::Xaml::CornerRadius& firstCornerRadius, const Microsoft::UI::Xaml::CornerRadius& secondCornerRadius)
    {
        return ((firstCornerRadius.TopLeft == secondCornerRadius.TopLeft) &&
            (firstCornerRadius.TopRight == secondCornerRadius.TopRight) &&
            (firstCornerRadius.BottomRight == secondCornerRadius.BottomRight) &&
            (firstCornerRadius.BottomLeft == secondCornerRadius.BottomLeft));
    }

    inline bool operator== (const Microsoft::UI::Xaml::Duration& firstDuration, const Microsoft::UI::Xaml::Duration& secondDuration)
    {
        return Microsoft::UI::Xaml::DurationHelper::Equals(firstDuration, secondDuration);
    }

    inline bool operator== (const Microsoft::UI::Xaml::GridLength& firstGridLength, const Microsoft::UI::Xaml::GridLength& secondGridLength)
    {
        return Microsoft::UI::Xaml::GridLengthHelper::Equals(firstGridLength, secondGridLength);
    }

    inline bool operator== (const Microsoft::UI::Xaml::Media::Animation::KeyTime& firstKeyTime, const Microsoft::UI::Xaml::Media::Animation::KeyTime& secondKeyTime)
    {
        return firstKeyTime.TimeSpan.Duration == secondKeyTime.TimeSpan.Duration;
    }

    inline bool operator== (const Microsoft::UI::Xaml::Media::Animation::RepeatBehavior& firstRepeatBehavior, const Microsoft::UI::Xaml::Media::Animation::RepeatBehavior& secondRepeatBehavior)
    {
        return Microsoft::UI::Xaml::Media::Animation::RepeatBehaviorHelper::Equals(firstRepeatBehavior, secondRepeatBehavior);
    }


    inline bool operator== (const Microsoft::UI::Xaml::Media::Matrix& firstMatrix, const Microsoft::UI::Xaml::Media::Matrix& secondMatrix)
    {
        return ((firstMatrix.M11 == secondMatrix.M11) &&
            (firstMatrix.M12 == secondMatrix.M12) &&
            (firstMatrix.M21 == secondMatrix.M21) &&
            (firstMatrix.M22 == secondMatrix.M22) &&
            (firstMatrix.OffsetX == secondMatrix.OffsetX) &&
            (firstMatrix.OffsetY == secondMatrix.OffsetY));
    }

    inline bool operator== (const Microsoft::UI::Xaml::Media::Media3D::Matrix3D& firstMatrix3D, const Microsoft::UI::Xaml::Media::Media3D::Matrix3D& secondMatrix3D)
    {
        return ((firstMatrix3D.M11 == secondMatrix3D.M11) &&
                (firstMatrix3D.M12 == secondMatrix3D.M12) &&
                (firstMatrix3D.M13 == secondMatrix3D.M13) &&
                (firstMatrix3D.M14 == secondMatrix3D.M14) &&
                (firstMatrix3D.M21 == secondMatrix3D.M21) &&
                (firstMatrix3D.M22 == secondMatrix3D.M22) &&
                (firstMatrix3D.M23 == secondMatrix3D.M23) &&
                (firstMatrix3D.M24 == secondMatrix3D.M24) &&
                (firstMatrix3D.M31 == secondMatrix3D.M31) &&
                (firstMatrix3D.M32 == secondMatrix3D.M32) &&
                (firstMatrix3D.M33 == secondMatrix3D.M33) &&
                (firstMatrix3D.M34 == secondMatrix3D.M34) &&
                (firstMatrix3D.OffsetX == secondMatrix3D.OffsetX) &&
                (firstMatrix3D.OffsetY == secondMatrix3D.OffsetY) &&
                (firstMatrix3D.OffsetZ == secondMatrix3D.OffsetZ) &&
                (firstMatrix3D.M44 == secondMatrix3D.M44));
    }

    inline bool operator== (const ::Windows::UI::Xaml::Interop::TypeName& lhs, const ::Windows::UI::Xaml::Interop::TypeName& rhs)
    {
        return (lhs.Kind == rhs.Kind) &&
            (lhs.Name == rhs.Name);
    }

} } } } }