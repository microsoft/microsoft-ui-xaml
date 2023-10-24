// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "BitmapImage.g.h"
#include "ImageBrush.g.h"
#include "RenderTargetBitmap.g.h"
#include "DragVisual_partial.h"
#include "DragVisualBuffer.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

ULONG DragVisual::z_ulUniqueAsyncActionId = 1;

HRESULT DragVisual::Initialize(_In_ xaml::IUIElement* pUIElement)
{
    m_spUIElement = pUIElement;
    m_dragVisualContentType = DragVisualContentType::VisualFromDraggedUIElement;
    return S_OK;
}

HRESULT DragVisual::Initialize(_In_ xaml_imaging::IBitmapImage* pBitmapImage)
{
    m_spBitmapImage = pBitmapImage;
    m_dragVisualContentType = DragVisualContentType::VisualFromBitmapImage;
    return S_OK;
}

HRESULT DragVisual::Initialize(_In_ wgri::ISoftwareBitmap* pSoftwareBitmap)
{
    m_spSoftwareBitmap = pSoftwareBitmap;
    m_dragVisualContentType = DragVisualContentType::VisualFromSoftwareBitmap;
    return S_OK;
}

_Check_return_ HRESULT
DragVisual::RenderAsync(_Outptr_ wf::IAsyncAction** ppRenderAsyncAction)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<DragVisualRenderAsyncAction> spRenderAsyncAction;

    ctl::ComPtr<DirectUI::IDispatcher> spDispatcher;
    ULONG actionId = ::InterlockedIncrement(&DragVisual::z_ulUniqueAsyncActionId);

    *ppRenderAsyncAction = nullptr;
    m_isRendered = false;

    IFC(CheckThread());
    IFC(DXamlCore::GetCurrent()->GetXamlDispatcher(&spDispatcher));

    IFC(Microsoft::WRL::MakeAndInitialize<DragVisualRenderAsyncAction>(
        &spRenderAsyncAction,
        actionId,
        spDispatcher.Get()));

    m_spRenderAsyncAction = spRenderAsyncAction;

    IFC(spRenderAsyncAction->StartOperation());

    switch (m_dragVisualContentType)
    {
    case DragVisualContentType::VisualFromDraggedUIElement:
        {
            ASSERT(m_spUIElement != nullptr);
            ctl::ComPtr<wf::IAsyncAction> spAsyncAction;

            ctl::WeakRefPtr wpThis;
            IFC(ctl::AsWeak(this, &wpThis));

            auto completionCallback = wrl::Callback<wf::IAsyncActionCompletedHandler>([wpThis](wf::IAsyncAction*, wf::AsyncStatus) mutable
            {
                HRESULT hr = S_OK;
                auto spThis = wpThis.AsOrNull<IDragVisual>().Cast<DragVisual>();
                if (spThis)
                {
                    // In order to generate a software bitmap, we wait for the bits to be there
                    auto bitsAvailableCallback = wrl::Callback<wf::IAsyncOperationCompletedHandler<wsts::IBuffer*>>
                        ([wpThis](wf::IAsyncOperation<wsts::IBuffer*>* pBufferOperation, wf::AsyncStatus) mutable
                    {
                        HRESULT hr = S_OK;
                        auto spThis = wpThis.AsOrNull<IDragVisual>().Cast<DragVisual>();
                        if (spThis)
                        {
                            IFC(pBufferOperation->GetResults(spThis->m_spBuffer.ReleaseAndGetAddressOf()));
                            IFC(spThis->OnRenderCompleted());
                        }
                    Cleanup:
                        RRETURN(hr);
                    });
                    ctl::ComPtr<wf::IAsyncOperation<wsts::IBuffer*>> spOperation;
                    IFC(spThis->m_spRenderTargetBitmap->GetPixelsAsync(spOperation.GetAddressOf()));
                    IFC(spOperation->put_Completed(bitsAvailableCallback.Get()));
                }
            Cleanup:
                RRETURN(hr);
            });

            // RenderAsync was called previously, release reference on the RenderTargetBitmap and
            // create a new one
            ctl::ComPtr<RenderTargetBitmap> spRenderTargetBitmap;
            IFC(ctl::make<RenderTargetBitmap>(&spRenderTargetBitmap));
            IFC(spRenderTargetBitmap.As(&m_spRenderTargetBitmap));

            IFC(m_spRenderTargetBitmap->RenderAsync(m_spUIElement.Get(), &spAsyncAction));
            IFC(spAsyncAction->put_Completed(completionCallback.Get()));
        }
        break;
    case DragVisualContentType::VisualFromBitmapImage:
        {
            ASSERT(m_spBitmapImage != nullptr);
            // RenderAsync was called previously, release reference on the ImageBrush and
            // create a new one
            ctl::ComPtr<ImageBrush> spImageBrush;
            IFC(ctl::make<ImageBrush>(&spImageBrush));
            IFC(spImageBrush.MoveTo(m_spImageBrush.ReleaseAndGetAddressOf()));

            auto pBitmapImageNativeNoRef = static_cast<CBitmapImage*>(m_spBitmapImage.Cast<BitmapImage>()->GetHandle());
            bool imageAvailable = false;
            if (pBitmapImageNativeNoRef->IsImageDecoded(&imageAvailable))
            {
                // We need do complete synchronously
                if (imageAvailable)
                {
                    IFC(RetrieveImageSize());
                }
                else
                {
                    spRenderAsyncAction->CoreSetError(E_FAIL);
                }
                spRenderAsyncAction->CoreFireCompletion();
                m_spRenderAsyncAction = nullptr;
            }
            else
            {
                // We'll complete once the image has been loaded
                EventRegistrationToken eventShareToken;
                ctl::WeakRefPtr wpThis;
                IFC(ctl::AsWeak(this, &wpThis));

                auto imageOpenCallback = wrl::Callback<xaml::IRoutedEventHandler>(
                    [wpThis](IInspectable*, IRoutedEventArgs*) mutable
                {
                    HRESULT hr = S_OK;
                    auto spThis = wpThis.AsOrNull<IDragVisual>().Cast<DragVisual>();
                    if (spThis)
                    {
                        IFC(spThis->OnImageOpened());
                    }
                Cleanup:
                    RRETURN(hr);
                });
                auto imageFailedCallback = wrl::Callback<xaml::IExceptionRoutedEventHandler>(
                    [wpThis](IInspectable*, IExceptionRoutedEventArgs*) mutable
                {
                    HRESULT hr = S_OK;
                    auto spThis = wpThis.AsOrNull<IDragVisual>().Cast<DragVisual>();
                    if (spThis)
                    {
                        IFC(spThis->OnImageFailed());
                    }
                Cleanup:
                    RRETURN(hr);
                });
                IFC(m_spBitmapImage->add_ImageOpened(imageOpenCallback.Get(), &eventShareToken));
                IFC(m_spBitmapImage->add_ImageFailed(imageFailedCallback.Get(), &eventShareToken));
                CImageBrush* pImageBrushNative = static_cast<CImageBrush*>(m_spImageBrush.Cast<ImageBrush>()->GetHandle());
                pImageBrushNative->SetIsDragDrop();
            }

            IFC(m_spImageBrush->put_ImageSource(m_spBitmapImage.Cast<BitmapImage>()));
        }
        break;
    case DragVisualContentType::VisualFromSoftwareBitmap:
        {
            ASSERT(m_spSoftwareBitmap != nullptr);
            spRenderAsyncAction->CoreFireCompletion();
            m_spRenderAsyncAction = nullptr;
        }
        break;
    default:
        IFC(E_FAIL);
    }

    *ppRenderAsyncAction = spRenderAsyncAction.Detach();

Cleanup:
    if (FAILED(hr) && (m_spRenderAsyncAction != nullptr))
    {
        m_spRenderAsyncAction->CoreSetError(hr);
        m_spRenderAsyncAction->CoreFireCompletion();
        m_spRenderAsyncAction = nullptr;
    }
    return hr;
}

_Check_return_ HRESULT
DragVisual::RetrieveImageSize()
{
    HRESULT hr = S_OK;
    INT height;
    INT width;

    IFC(m_spBitmapImage.Cast<BitmapImage>()->get_PixelWidth(&width));
    IFC(m_spBitmapImage.Cast<BitmapImage>()->get_PixelHeight(&height));

    m_size.Width = static_cast<float>(width);
    m_size.Height = static_cast<float>(height);

    m_isRendered = true;

Cleanup:
    return hr;
}

_Check_return_ HRESULT
DragVisual::OnImageOpened()
{
    HRESULT hr = S_OK;
    IFC(RetrieveImageSize());

Cleanup:
    if (m_spRenderAsyncAction != nullptr)
    {
        m_spRenderAsyncAction->CoreFireCompletion();
        m_spRenderAsyncAction = nullptr;
    }

    return hr;
}

_Check_return_ HRESULT
DragVisual::OnImageFailed()
{
    if (m_spRenderAsyncAction != nullptr)
    {
        m_spRenderAsyncAction->CoreSetError(E_FAIL);
        m_spRenderAsyncAction->CoreFireCompletion();
        m_spRenderAsyncAction = nullptr;
    }

    return S_OK;
}

_Check_return_ HRESULT
DragVisual::OnRenderCompleted()
{
    CRenderTargetBitmap *pRTB = static_cast<CRenderTargetBitmap*>(m_spRenderTargetBitmap.Cast<RenderTargetBitmap>()->GetHandle());

    m_size.Width = static_cast<float>(pRTB->GetPixelWidth());
    m_size.Height = static_cast<float>(pRTB->GetPixelHeight());

    m_isRendered = true;

    if (m_spRenderAsyncAction != nullptr)
    {
        m_spRenderAsyncAction->CoreFireCompletion();
        m_spRenderAsyncAction = nullptr;
    }

    return S_OK;
}

_Check_return_ HRESULT
DragVisual::GetSoftwareBitmap(_COM_Outptr_ wgri::ISoftwareBitmap** ppSoftwareBitmap)
{
    if (!m_spSoftwareBitmap)
    {
        IFC_RETURN(CreateSoftwareBitmap());
    }
    return m_spSoftwareBitmap.CopyTo(ppSoftwareBitmap);
}

_Check_return_ HRESULT DragVisual::CreateSoftwareBitmap()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wgri::ISoftwareBitmapStatics> spSoftwareBitmapStatics;
    bool surfaceLocked = false;
    // In some cases, the application might have messed with the UIElement which could result in 0 dimension here
    // and we cannot create the SoftwareBitmap: we just ignore it as it is impossible for the application to catch
    // the exception at this point
    if ((m_size.Width > 0) && (m_size.Height > 0))
    {
        // In the case of a bitmap image, we don't yet have the IBuffer interface
        if (m_dragVisualContentType == DragVisualContentType::VisualFromBitmapImage)
        {
            BYTE *pSWBits = nullptr;
            INT32 stride;
            XUINT32 width;
            XUINT32 height;
            IFC(static_cast<CImageBrush*>(m_spImageBrush.Cast<ImageBrush>()->GetHandle())->Lock(
                        reinterpret_cast<void**>(&pSWBits),
                        &stride,
                        &width,
                        &height));
            surfaceLocked = true;
            const int bufferSize = height * stride;
            auto buffer = ::Microsoft::WRL::Make<DragVisualBuffer>(pSWBits, bufferSize);
            buffer.CopyTo(m_spBuffer.ReleaseAndGetAddressOf());
            // Make sure the size used to call CreateCopyWithAlphaFromBuffer() is no bigger than the source buffer size.
            m_size.Width = MIN(m_size.Width, width);
            m_size.Height = MIN(m_size.Height, height);
        }
        IFC(wf::GetActivationFactory(
                    wrl_wrappers::HStringReference(RuntimeClass_Windows_Graphics_Imaging_SoftwareBitmap).Get(),
                    spSoftwareBitmapStatics.GetAddressOf()));

        IFC(spSoftwareBitmapStatics->CreateCopyWithAlphaFromBuffer(m_spBuffer.Get(),
                wgri::BitmapPixelFormat_Bgra8,
                static_cast<int>(m_size.Width),
                static_cast<int>(m_size.Height),
                wgri::BitmapAlphaMode_Premultiplied,
                m_spSoftwareBitmap.ReleaseAndGetAddressOf()));
    }
    // If everything is ok, we can release a few smartpointers
    m_spRenderTargetBitmap.Reset();
    m_spBuffer.Reset();

Cleanup:
    if (surfaceLocked)
    {
        VERIFYHR(static_cast<CImageBrush*>(m_spImageBrush.Cast<ImageBrush>()->GetHandle())->UnLock());
    }
    // If we used an ImageBrush, releasing it will also release the Software Surface
    // of the BitmapImage
    m_spImageBrush.Reset();
    return hr;
}