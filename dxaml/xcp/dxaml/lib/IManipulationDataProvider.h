// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    /// <summary>
    ///     Interface through which a ScrollViewer 
    ///     communicates with its manipulation data provider to get manupulation related data.
    /// </summary>
    MIDL_INTERFACE("90e6cebf-7807-44ad-8541-3ef234730ec8")
    IManipulationDataProvider : public IInspectable
    {
        // Provides physical orientation of content
        IFACEMETHOD(get_PhysicalOrientation)(
            _Out_ xaml_controls::Orientation* value) = 0;

        // Tells whether the element is participating in a manipulation or not.
        virtual _Check_return_ HRESULT UpdateInManipulation(
            _In_ BOOLEAN isInManipulation,
            _In_ BOOLEAN isInLiveTree,
            _In_ DOUBLE nonVirtualizingOffset) = 0;

        // Updates the zoom factor on manipulatable element
        virtual _Check_return_ HRESULT SetZoomFactor(
            _In_ FLOAT newZoomFactor) = 0; 

        // Gets the extent in pixels even for logical scrolling scenarios.
        virtual _Check_return_ HRESULT ComputePixelExtent(
            _In_ bool ignoreZoomFactor,
            _Out_ DOUBLE& extent) = 0;

        // Gets the offset in pixels even for logical scrolling scenarios.
        virtual _Check_return_ HRESULT ComputePixelOffset(
            _In_ BOOLEAN isForHorizontalOrientation,
            _Out_ DOUBLE& offset) = 0;

        // Gets the logical offset given a pixel delta.
        virtual _Check_return_ HRESULT ComputeLogicalOffset(
            _In_ BOOLEAN isForHorizontalOrientation,
            _Inout_ DOUBLE& pixelDelta,
            _Out_ DOUBLE& logicalOffset) = 0;

        // Gets the size of first Visible child
        virtual _Check_return_ HRESULT GetSizeOfFirstVisibleChild(
            _Out_ wf::Size& size) = 0; 
    };
}


