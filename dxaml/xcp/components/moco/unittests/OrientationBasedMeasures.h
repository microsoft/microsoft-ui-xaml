// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <array>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls {

class OrientationBasedMeasures
{
public:
    OrientationBasedMeasures(bool isHorizontal)
        : _isHorizontal(isHorizontal)
    { }

    #pragma region VirtualizingOffset
    float VirtualizingOffset(wf::Rect rect) const
    {
        return _isHorizontal ? rect.X : rect.Y;
    }
    float VirtualizingOffset(wf::Point point) const
    {
        return _isHorizontal ? point.X : point.Y;
    }
    #pragma endregion

    #pragma region NonVirtualizingOffset
    float NonVirtualizingOffset(wf::Rect rect) const
    {
        return _isHorizontal ? rect.Y : rect.X;
    }
    float NonVirtualizingOffset(wf::Point point) const
    {
        return _isHorizontal ? point.Y : point.X;
    }
    #pragma endregion

    #pragma region VirtualizingSize
    float VirtualizingSize(wf::Rect rect) const
    {
        return _isHorizontal ? rect.Width : rect.Height;
    }
    float VirtualizingSize(wf::Size size) const
    {
        return _isHorizontal ? size.Width : size.Height;
    }    
    #pragma endregion

    #pragma region NonVirtualizingSize
    float NonVirtualizingSize(wf::Rect rect) const
    {
        return _isHorizontal ? rect.Height : rect.Width;
    }    
    float NonVirtualizingSize(wf::Size size) const
    {
        return _isHorizontal ? size.Height : size.Width;
    }
    #pragma endregion

    #pragma region (Non)VirtualizingOffsetPlusSize
    float VirtualizingOffsetPlusSize(wf::Rect rect) const
    {
        return _isHorizontal ? rect.X + rect.Width : rect.Y + rect.Height;
    }
    float NonVirtualizingOffsetPlusSize(wf::Rect rect) const
    {
        return _isHorizontal ? rect.Y + rect.Height : rect.X + rect.Width;
    }
    #pragma endregion

    #pragma region Padding methods

    float TotalVirtualizingPadding(xaml::Thickness thickness) const
    {
        return static_cast<float>(_isHorizontal ? thickness.Left + thickness.Right : thickness.Top + thickness.Bottom);
    }
    float TotalNonVirtualizingPadding(xaml::Thickness thickness) const
    {
        return static_cast<float>(_isHorizontal ? thickness.Top + thickness.Bottom : thickness.Left + thickness.Right);
    }
    float NonVirtualizingPaddingAtStart(xaml::Thickness thickness) const
    {
        return static_cast<float>(_isHorizontal ? thickness.Top : thickness.Left);
    }

    #pragma endregion


    xaml_controls::Orientation GetOrientation() const
    {
        return _isHorizontal ?
            xaml_controls::Orientation_Horizontal :
            xaml_controls::Orientation_Vertical;

    }

    bool IsHorizontal() const { return _isHorizontal; }

    static std::array<OrientationBasedMeasures, 2> GetOrientations()
    {
        std::array<OrientationBasedMeasures, 2> orientations = {
            OrientationBasedMeasures(true /* isHorizontal */),
            OrientationBasedMeasures(false /* isHorizontal */)
        };
        return orientations;
    }



private:
    bool _isHorizontal;
};

} } } } }