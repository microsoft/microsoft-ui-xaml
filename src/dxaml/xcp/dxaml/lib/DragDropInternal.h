// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DragDropVisual.h"
#include "fwd/microsoft.ui.input.dragdrop.h"

namespace DirectUI
{
    class DragDrop
    {
        private:
            // The container for the drag visual, responsible for drawing the drag visual on the TransitionRoot layer.
            ctl::ComPtr<DragDropVisual> m_spDragDropVisual;

            // Current drag-drop position, relative to the root visual.
            wf::Point m_userDragPosition{};

            // A reference to the UIElement that is the source of the current drag and drop operation. Only valid if m_isDragDropInProgress is true.
            ctl::WeakRefPtr m_wrCurrentDragSourceWeakRef;

            // A reference to the UIElement within the source that originated the drag operation.
            ctl::WeakRefPtr m_wrPrimaryDraggedItemWeakRef;

            // When WinRT DnD API is used, we can no longer track the drag position outside of the ListViewBase because we lose the ability to
            // capture mouse events. This is so that the drag position is correctly forwarded to the ListViewBase that drag is originated from.
            ctl::ComPtr<xaml_controls::IListViewBase> m_spCapturedDragSource;

            // A reference to the data associated with the current drag and drop operation. Only valid if m_isDragDropInProgress is true.
            ctl::ComPtr<wadt::IDataPackage> m_spCurrentDragData;

            // Set to true when there is a drag and drop operation in progress.
            BOOLEAN m_isDragDropInProgress;
            
            // Set to true when a WinRT drag and drop operaiton is in progress.
            bool m_isWinRTDragDropInProgress;

            // The number of items currently being dragged.
            INT32 m_cItemCount;

            // When this is set to true, the custom visual will be cleared
            bool m_shouldClearCustomVisual = false;

            // Cache the UIElement which sets the custom visual
            ctl::ComPtr<IInspectable> m_spCustomVisualSetterIInspectable;

        public:
            DragDrop();

            ~DragDrop();

            // Initiates a drag drop. In case of failure in this method, the drag drop will be automatically canceled.
            //  dragSource - the UIElement that is the source of the drag. Positions given in DragMove should be relative to this element.
            //  dragItem - the UIElement that originated the drag operation. Often a ListViewItem.
            //  data - The data to supply to drop targets.
            //  dragVisual - The visual that will serve as the drag icon (the graphics that follow the input device around).
            //  dragStartPoint - the point at which the drag originated.
            //  shouldFireEvent - when WinRT DnD API is used, we don't want to raise drag and drop from here but get the drag/drop callback on DropOperationTarget.
            _Check_return_ HRESULT DragStart(
                _In_ xaml::IUIElement* dragSource,
                _In_ xaml::IUIElement* dragItem,
                _In_ wadt::IDataPackage* data,
                _In_ DragDropVisual* dragVisual,
                _In_ wf::Point dragStartPoint,
                _In_ INT32 cItemCount,
                _In_ BOOLEAN shouldFireEvent);

            // Terminates the current drag and drop operation. The Drop event will be raised on the elements currently under the drag that have AllowDrop set.
            _Check_return_ HRESULT Drop(_In_ BOOLEAN shouldFireEvent);

            // Aborts the current drag and drop operation. A final DragLeave event will be raised on the elements currently under the drag that have AllowDrop set.
            _Check_return_ HRESULT DragCancel(_In_ BOOLEAN shouldFireEvent);

            // Notifies the drag and drop system of a user moving the drag position. This will raise the appropriate drag events and move the drag visual
            // to be centered over the new drag position.
            //  newDragPosition - the new drag position, relative to the drag source element passed into the last DragStart.
            //  pCalculatedIconTopLeft - The computed location of the top-left corner of the drag visual. Optional.
            _Check_return_ HRESULT DragMove(_In_ wf::Point newDragPosition, _Out_opt_ wf::Point* pCalculatedIconTopLeft, _In_ BOOLEAN shouldFireEvent);

            // This RPInvoke prepares an IDragEventArgs instance by supplying
            // the current data package as its Data.
            static _Check_return_ HRESULT PopulateDragEventArgs(
                _In_ CDragEventArgs* pEventArgs);

            // This RPInvoke is called in DragLeave when the drag leaves the UIElement that sets the
            // override visual.
            static _Check_return_ HRESULT CheckIfCustomVisualShouldBeCleared(
                _In_ CDependencyObject* pSource);

            // A reference to the UIElement that is the source of the current drag and drop operation. Returns NULL if there is no active drag and drop operation.
            IWeakReference* GetCurrentDragSource()
            {
                return m_wrCurrentDragSourceWeakRef.Get();
            }

            // A reference to the UIElement within the source that originated the drag operation.
            IWeakReference* GetCurrentDragItem()
            {
                return m_wrPrimaryDraggedItemWeakRef.Get();
            }

            // A reference to the data associated with the current drag and drop operation. Returns NULL if there is no active drag and drop operation.
            _Check_return_ HRESULT GetCurrentDragData(wadt::IDataPackage** ppData)
            {
                RRETURN(m_spCurrentDragData.CopyTo(ppData));
            }

            // This returns a captured uielement which receives Drag info on actions performed
            // outside the UIElement bounds
            _Check_return_ HRESULT GetCapturedDragSource(
                _Outptr_ xaml_controls::IListViewBase** ppTarget)
            {
                RRETURN(m_spCapturedDragSource.CopyTo(ppTarget));
            }

            void SetCapturedDragSource(_In_ xaml_controls::IListViewBase* pTarget)
            {
                m_spCapturedDragSource = pTarget;
            }

            // Returns TRUE if there is an active drag and drop operation, FALSE otherwise.
            BOOLEAN GetIsDragDropInProgress()
            {
                return m_isDragDropInProgress;
            }

            INT32 GetItemCount() const
            {
                return m_cItemCount;
            }

            _Check_return_ HRESULT CleanupDragDrop();


            // This is called when 
            // - App sets the custom drag visual on the drop site,
            // we cache the setter UIElement and we no longer need to clear the override visual.
            // - Input leaves setterUIElement, shouldClear=true, if the setterUIElement is the previous
            // custom drag visual setter, we should clear the setter and clear the visual.
            // - Core Drop/LeaveAsync is called, setterUIElement is null, to reset the states.
            void SetCustomVisualSetterUIElement(_In_opt_ IInspectable* setterUIElement);

            bool ShouldClearCustomVisual() const
            {
                return m_shouldClearCustomVisual;
            }

            // Indicates if we need to use Core APIs for Drag and Drop
            bool GetUseCoreDragDrop() const { return m_useCoreDragDrop; }
            void SetUseCoreDragDrop(_In_ bool useCoreDragDrop) { m_useCoreDragDrop = useCoreDragDrop; }

            _Check_return_ HRESULT HidePrimaryDraggedItem();

            bool GetUseSystemDefaultVisual() const
            {
                // System Visual is only provided for "real" Core Drag and Drop Implemenation
                return m_useCoreDragDrop;
            }
            HRESULT CreateDragOperation(_Outptr_ mui::DragDrop::IDragOperation **ppOperation);

            void SetIsWinRTDndOperationInProgress(bool inProgress)
            {
                m_isWinRTDragDropInProgress = inProgress;
            }

            bool IsWinRTDndOperationInProgress()
            {
                return m_isWinRTDragDropInProgress;
            }

           _Check_return_ HRESULT SetAllowedOperations(_In_ wadt::DataPackageOperation allowedOperations)
            {
                m_allowedOperations = allowedOperations;
                return S_OK;
            }

           _Check_return_ HRESULT GetAllowedOperations(_Out_ wadt::DataPackageOperation* allowedOperations)
            {
                (*allowedOperations) = m_allowedOperations;
                return S_OK;
            }

        private:
            // Updates m_userDragPosition from a position relative to m_pCurrentDragSourceWeakRef.
            _Check_return_ HRESULT UpdateUserDragPosition(_In_ wf::Point newDragPosition);

            XPOINTF GetUserDragPositionAsXPOINTF();

            void CheckIfCustomVisualShouldBeCleared(_In_ IInspectable* sourceAsInspectable);

            bool m_useCoreDragDrop = false;

            wadt::DataPackageOperation m_allowedOperations;
    };
}
