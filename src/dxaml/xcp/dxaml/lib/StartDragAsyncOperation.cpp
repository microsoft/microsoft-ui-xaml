// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DragVisual_partial.h"
#include "StartDragAsyncOperation.h"
#include "UIElement.g.h"
#include <DragDropInternal.h>
#include <FrameworkUdk/CoreDragOperationFunctions.h>

using namespace DirectUI;
using namespace DirectUISynonyms;

// Work around disruptive max/min macros
#undef max
#undef min

ULONG StartDragAsyncOperation::z_ulUniqueAsyncActionId = 1;

_Check_return_ HRESULT DelayRelease(IInspectable* inspectable);

StartDragAsyncOperation::~StartDragAsyncOperation()
{
    // We need to ensure that the final release of the winrt drag info instance is delayed.
    // If not, the final release could also initiate another drag operation in the
    // same UI thread, but because Xaml is draining the clean up queue,
    // then OnReentrancyProtectedWindowMessage could be trigger and the app would crash.
    IFCFAILFAST(DelayRelease(m_spCoreDragOperationAsyncInfo.Get()));
}

_Check_return_ HRESULT
StartDragAsyncOperation::SetVisual(_In_ bool hasAnchorPoint, _In_ wf::Point anchorPoint, _In_opt_ DragVisual* dragVisual)
{
    m_hasAnchorPoint = hasAnchorPoint;
    m_anchorPoint = anchorPoint;
    m_spDragVisual = dragVisual;

    if ((m_spDragVisual != nullptr) && !m_spDragVisual->IsRendered())
    {
        wrl::ComPtr<wf::IAsyncAction> spAction;
        ctl::ComPtr<wf::IAsyncOperation<wadt::DataPackageOperation>> spThis(this);
        ctl::WeakRefPtr wpThis;
        IFC_RETURN(ctl::AsWeak(spThis.Get(), &wpThis));
        auto completionCallback = wrl::Callback<wf::IAsyncActionCompletedHandler>([wpThis](wf::IAsyncAction*asyncOp, wf::AsyncStatus status) mutable
        {
            auto spThis = wpThis.AsOrNull<wf::IAsyncOperation<wadt::DataPackageOperation>>().Cast<StartDragAsyncOperation>();
            if (spThis)
            {
                IFC_RETURN(spThis->OnRenderCompleted(asyncOp, status));
            }
            return S_OK;
        });

        IFC_RETURN(m_spDragVisual->RenderAsync(spAction.ReleaseAndGetAddressOf()));
        IFC_RETURN(spAction->put_Completed(completionCallback.Get()));

        m_deferVisual = true;
    }
    else
    {
        m_deferVisual = false;
        // call OnRenderCompleted synchronously
        IFC_RETURN(OnRenderCompleted(nullptr, wf::AsyncStatus::Completed));
    }
    return S_OK;
}

_Check_return_ HRESULT
StartDragAsyncOperation::StartIfNeededImpl(_In_ CDependencyObject* anchorElement, _In_ ixp::IPointerPoint* pointerPoint)
{
    if (!m_started)
    {
        IFC_RETURN(StartOperation());

        ctl::ComPtr<wf::IAsyncOperation<wadt::DataPackageOperation>> spAsyncCoreOperation;

        if (m_deferVisual)
        {
            IFC_RETURN(m_spDragOperation->put_DragUIContentMode(mui::DragDrop::DragUIContentMode::DragUIContentMode_Deferred));
        }

        wrl::ComPtr<mui::DragDrop::IDragDropManager> dragDropManager;
        CXamlIslandRoot* xamlIslandRoot = VisualTree::GetXamlIslandRootForElement(anchorElement);

        if (xamlIslandRoot)
        {
            // If we're inside an island, the DragDropManager should already be registered
            IFC_RETURN(xamlIslandRoot->get_DragDropManager(&dragDropManager));
        }
        else
        {
            IFC_RETURN(E_FAIL);
        }

        {
            IFC_RETURN(m_spDragOperation->StartAsync(dragDropManager.Get(), pointerPoint, &spAsyncCoreOperation));
            if (!spAsyncCoreOperation)
            {
                // There is currently an issue with starting an AsyncDrag operation when elevated.  It returns S_OK, but does not return
                // return an async operation.  This needs to be fixed in the drag/drop code, but won't be for a while, so we are going
                // detect this and raise our own error rather than getting an access violation later.  See Bug 38743518: Xaml drag crashes
                // when running elevated
                IFC_RETURN(ErrorHelper::OriginateError(E_UNEXPECTED,
                    wrl_wrappers::HStringReference(L"Drag start failed. If this is an elevated process, drag/drop is currently not supported there.").Get()));
            }
        }

        // Pass strong ref to lambda, so object is guaranteed to be alive when it's called.
        // In turn lambda's lifetime is controlled by CoreDragOperation (which is also a field in this class).
        // Reset spThis in lambda to break reference cycle.

        ctl::ComPtr<wf::IAsyncOperation<wadt::DataPackageOperation>> spThis(this);

        auto callback = Microsoft::WRL::Callback<wf::IAsyncOperationCompletedHandler<wadt::DataPackageOperation>>(
                           [spThis, this](wf::IAsyncOperation<wadt::DataPackageOperation>* asyncOp, wf::AsyncStatus status) mutable -> HRESULT
        {
            auto scopeGuard = wil::scope_exit([&]
            {
                // Ensure we have a core since this async action may come in after we have shutdown.
                auto dxamlCore = DXamlCore::GetCurrent();
                if (dxamlCore)
                {
                    dxamlCore->SetIsWinRTDndOperationInProgress(false);
                }
                spThis = nullptr;
            });

            switch (status)
            {
                case wf::AsyncStatus::Completed:
                    {
                        ctl::ComPtr<UIElement> spUIElement = m_wrUIElement.AsOrNull<UIElement>();

                        IFC_RETURN(asyncOp->GetResults(&m_result));

                        if (spUIElement)
                        {
                            IFC_RETURN(spUIElement->OnDropCompleted(m_result));
                        }
                    }
                    break;

                case wf::AsyncStatus::Canceled:
                    Cancel();
                    break;

                case wf::AsyncStatus::Error:
                    TryTransitionToError(E_FAIL);
                    break;
            }

            IFC_RETURN(FireCompletion());

            return S_OK;
        });

        IFC_RETURN(spAsyncCoreOperation->put_Completed(callback.Get()));
        IFC_RETURN(spAsyncCoreOperation.As(&m_spCoreDragOperationAsyncInfo));
    }
    return S_OK;
}

_Check_return_ HRESULT
StartDragAsyncOperation::StartIfNeeded(_In_ CDependencyObject* anchorElement, _In_ ixp::IPointerPoint* pointerPoint)
{
    return StartIfNeededImpl(anchorElement, pointerPoint);
}

_Check_return_ HRESULT
StartDragAsyncOperation::DoCancel()
{
    // If the operation hasn't been started yet, we need to do it
    if (!m_started)
    {
        IFC_RETURN(StartOperation());
        // If we are cancelling prior to actually starting, then we won't
        // actually be invoked, so ensure the core knows that the operation
        // is no longer in progress.
        DXamlCore::GetCurrent()->SetIsWinRTDndOperationInProgress(false);
    }

    IFC_RETURN(Cancel());
    IFC_RETURN(AsyncBase::FireCompletion());
    return S_OK;
}

STDMETHODIMP
StartDragAsyncOperation::Cancel(void)
{
    if (m_spCoreDragOperationAsyncInfo)
    {
        IFC_RETURN(m_spCoreDragOperationAsyncInfo->Cancel());
    }
    return __super::Cancel();
}

STDMETHODIMP
StartDragAsyncOperation::GetResults(_Inout_ wadt::DataPackageOperation* result)
{
    *result = m_result;
    return S_OK;
}

_Check_return_ HRESULT
StartDragAsyncOperation::OnRenderCompleted(_In_opt_ wf::IAsyncAction*, wf::AsyncStatus status)
{
    if (status == wf::AsyncStatus::Completed)
    {
        // Set custom visual for the drag operation
        if (m_spDragVisual != nullptr)
        {
            ctl::ComPtr<wgri::ISoftwareBitmap> spSoftwareBitmap;
            // In some scenarios, the application might mess with the UIElement being dragged,
            // resulting with a failure to create the SoftwareBitmap.
            // However, there is no way for the application to catch this error because everything is done
            // asynchronously AND without the application having a IAsyncAction/IAsyncOperation to check.
            // Therefore, we will just ignore the error and not display any visual in this case, which is
            // better than an unexpected exit of the application
            // Note also that if we are here, we have told WinRT API that we would provide a custom visual
            // and there is no way to change this anymore...
            IFC_RETURN(m_spDragVisual->GetSoftwareBitmap(spSoftwareBitmap.GetAddressOf()));
            if (spSoftwareBitmap)
            {
                if (m_hasAnchorPoint)
                {
                    int bitmapWidth = 0;
                    int bitmapHeight = 0;
                    IFC_RETURN(spSoftwareBitmap->get_PixelWidth(&bitmapWidth));
                    IFC_RETURN(spSoftwareBitmap->get_PixelHeight(&bitmapHeight));
                    // In the case the visual is too small and the anchor point is outside of it,
                    // or in some cases when the user moves the mouse really fast before D&D starts
                    // Anchor point might have coordinates either negative or larger than size and
                    // SetDragUIContentWithAnchorPoint will fail, which we cannot afford as we are in a callback
                    // and therefore we coerce the anchor point in accepted bounds
                    wf::Point coercedAnchor =
                        {
                            std::max(std::min(m_anchorPoint.X, static_cast<float>(bitmapWidth)), 0.0f),
                            std::max(std::min(m_anchorPoint.Y, static_cast<float>(bitmapHeight)), 0.0f)};
                    IFC_RETURN(m_spDragOperation->SetDragUIContentFromSoftwareBitmap2(spSoftwareBitmap.Get(), coercedAnchor));
                }
                else
                {
                    IFC_RETURN(m_spDragOperation->SetDragUIContentFromSoftwareBitmap(spSoftwareBitmap.Get()));
                }
            }
        }

        // Used to hide the original dragged item in case this is a ListViewBaseItem element
        IFC_RETURN(DXamlCore::GetCurrent()->GetDragDrop()->HidePrimaryDraggedItem());
    }
    else
    {
        switch (status)
        {
        case wf::AsyncStatus::Canceled:
            Cancel();
            break;
        case wf::AsyncStatus::Error:
            TryTransitionToError(E_FAIL);
            break;
        }

        IFC_RETURN(AsyncBase::FireCompletion());
    }

    return S_OK;
}
