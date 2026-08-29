// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      A wrapper around UIElements used by the Drag and Drop system to draw drag icons on the TransitionRoot layer.
//      It's used by the singleton DragDrop class, which is in turn used by ListViewBase.

#pragma once

namespace DirectUI
{
    // A wrapper around UIElements used by the Drag and Drop system to draw drag icons on the TransitionRoot layer.
    class DragDropVisual : public ctl::WeakReferenceSource
    {
        private:
            // The container for the drag visual, responsible for drawing the drag visual on the TransitionRoot layer.
            // This is provided via a pinvoke to DragDrop_CreateDragVisual.
            TrackerPtr<xaml::IUIElement> m_tpDragVisualContainer; 
            
            // The user-provided drag visual.
            TrackerPtr<xaml::IUIElement> m_tpDragVisual;
            
            // The transform applied to the drag visual container.
            TrackerPtr<xaml_media::IMatrixTransform> m_tpDragVisualContainerTransform;
            
            // Whether or not the visual is being requested to display via a call to Show() or Hide().
            bool m_isShowing;

            // When WinRt Drag and Drop API is called, system default visual is displayed during drag operations
            // so XAML does not need to handle drag visual.
            bool m_useSystemDefaultVisual;
            
            // Our ref count.
            ULONG m_refCount{};
            
            // The offset we apply to points given to SetPosition.
            wf::Point m_offsetPosition;
            wf::Point m_offsetPositionInRoot;
            
            // The last position passed to SetPosition().
            wf::Point m_currentPosition;
            
            // The opacity of this DragDropVisual. Usually this is not needed,
            // as the inner content takes care of its own opacity. However, chromed controls
            // require this as they don't have the required number of UIElement layers.
            XFLOAT m_opacity;
            
        public:
            // Construct a new DragDropVisual that displays the given UIElement.
            // pDragVisual - the UIElement to display in this DragDropVisual.
            // offsetPosition - a relative offset that will be applied to positions given to SetPosition.
            // useSystemDefaultVisual - the system's visual is displayed instead of the XAML's
            static _Check_return_ HRESULT CreateInstance(
                _In_opt_ xaml::IUIElement* pDragVisual,
                _In_ wf::Point offsetPosition,
                _Outptr_ DragDropVisual** ppInstance);
            
            // Whether or not the DragDropVisual is active. This can become false through a call to Hide(), or when the
            // target visual is removed from the tree.
            bool IsShowing();
            
            // Moves this visual so it is positioned at the given point on the TransitionRoot layer.
            // Will result in error if the DragDropVisual isn't shown.
            _Check_return_ HRESULT SetPosition(_In_ wf::Point position);
            
            // Obtains the top-leftmost corner of any visual elements drawn by this DragDropVisual.
            // Will result in error if the DragDropVisual isn't shown.
            _Check_return_ HRESULT GetTopLeftPosition(_Out_ wf::Point* pPosition);
            
            // Start drawing the drag visual on the TransitionRoot.
            _Check_return_ HRESULT Show();
            
            // Stops drawing this visual on the TransitionRoot layer.
            _Check_return_ HRESULT Hide();
            
            // Sets the opacity of this DragDropVisual. Usually this is not needed,
            // as the inner content takes care of its own opacity. However, chromed controls
            // require this as they don't have the required number of UIElement layers.
            _Check_return_ HRESULT SetOpacity(_In_ XFLOAT opacity);
            
        protected:
            
            // Releases all resources held by this DragDropVisual.
            ~DragDropVisual() override;

            // Construct a new DragDropVisual that displays the given UIElement.
            virtual _Check_return_ HRESULT Init(_In_opt_ xaml::IUIElement* pDragVisual, _In_ wf::Point offsetPosition);
            
        private:

            bool IsShowingCore(
                _In_ xaml::IUIElement *pDragVisual, 
                _In_ xaml::IUIElement *pDragVisualContainer);
            
            // Returns TRUE if our renderer is active.
            bool IsRendererActive(_In_ xaml::IUIElement *pDragVisual);

            _Check_return_ HRESULT HideCore(
                _In_ xaml::IUIElement *pDragVisual, 
                _In_ xaml::IUIElement *pDragVisualContainer);
            
            // Updates the transform on our container. Applies m_transformedOffsetPosition as necessary.
            _Check_return_ HRESULT UpdateTransform();
    };
}
