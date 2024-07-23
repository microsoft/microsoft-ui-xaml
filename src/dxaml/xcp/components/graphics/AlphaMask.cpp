// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <DCompTreeHost.h>
#include <core.h> // Remove in the future when possible
#include <corep.h> // Remove in the future when possible
#include <WindowRenderTarget.h>
#include <DCompSurface.h>
#include <UIElement.h>
#include <GraphicsUtility.h>
#include <d3d11device.h>
#include "D3D11SharedDeviceGuard.h"
#include <D2DAccelerated.h>
#include <D2DPrintTarget.h>
#include <TransformToRoot.h>
#include "AlphaMask.h"
#include <WindowsGraphicsDeviceManager.h>

//
// Provides functionality to the CShape/CImage/CTextBlock::GetAlphaMask method and to the render walk. GetAlphaMask
// provides a CompositionBrush that can be used for drop shadows (and potentially other composition based effects).
//
// Both consumers require the ability to turn a CUIElement into an 8-bit mask texture, with some small differences:
//
// Local properties:
//    - GetAlphaMask respects the local properties of the element.
//    - The render walk wants to ignore local properties on the element. Instead, it uses a rasterization scale that
//      it computed while walking down the tree.
//    - In both cases, we want to scale down the element if it won't fit in the max texture size.
//
// Output:
//    - GetAlphaMask ultimately returns a CompositionBrush. It connects the mask texture to the brush.
//    - The render walk just wants the alpha mask.
//    - The render walk wants stroke and fill separately.
//
class AlphaMask::Impl
{
public:
    _Check_return_ HRESULT Ensure(_In_ CUIElement* pUIElement);

    _Check_return_ HRESULT UpdateIfAvailable(_In_ CUIElement* pUIElement);

    void Hide();

    bool IsPresent() const
    {
        return (m_spCompositionSurfaceBrush != nullptr);
    }

    wrl::ComPtr<WUComp::ICompositionBrush> GetCompositionBrush();

    static _Check_return_ HRESULT RasterizeElement(
        _In_ CUIElement* pUIElement,
        const float additionalScaleFactor,
        _In_opt_ const CMILMatrix* realizationScale,
        const bool includeElementLocalProperties,
        const bool renderFill,
        const bool renderStroke,
        const XPOINTF* shapeMaskOffset,
        _In_ DCompSurface* surface,
        const bool renderCollapsedMask,
        _In_opt_ WUComp::ICompositionSurfaceBrush* compositionBrush);

private:
    _Check_return_ HRESULT EnsureBackingSurface(
        DCompTreeHost* pDCompTreeHostNoRef,
        uint32_t width,
        uint32_t height,
        _Out_ float* adjustedScaleFactor);

    void ConnectBackingSurface(_In_ DCompTreeHost* pDCompTreeHostNoRef);

    static _Ret_notnull_ DCompTreeHost* GetDCompTreeHostNoRef(_In_ CDependencyObject* pDO);
    static _Ret_notnull_ CD2DFactory* GetSharedD2DFactoryNoRef(_In_ CDependencyObject* pDO);

    // Indicates whether the backing surface is connected to the composition brush
    bool m_connected = false;

    // Backing surface to draw the alpha mask onto.
    wrl::ComPtr<DCompSurface> m_spBackingSurface;

    // In RS1, this holds onto a strong reference to the brush so that it can be updated.  If, in the future, Composition
    // supports a weak reference on their objects (or this object in particular), then the weak reference can be used for
    // updates and a strong reference can be handed out during creation.
    wrl::ComPtr<WUComp::ICompositionSurfaceBrush> m_spCompositionSurfaceBrush;
};

AlphaMask::AlphaMask()
    : m_pImpl(new Impl)
{
}

AlphaMask::~AlphaMask()
{
    // Disconnect the backing surface from the brush before releasing since the brush
    // is shared with the app which might still be holding onto a reference to the
    // brush.
    m_pImpl->Hide();
}

_Check_return_ HRESULT AlphaMask::Ensure(_In_ CUIElement* pUIElement)
{
    return m_pImpl->Ensure(pUIElement);
}

_Check_return_ HRESULT AlphaMask::UpdateIfAvailable(_In_ CUIElement* pUIElement)
{
    return m_pImpl->UpdateIfAvailable(pUIElement);
}

void AlphaMask::Hide()
{
    m_pImpl->Hide();
}

bool AlphaMask::IsPresent() const
{
    return m_pImpl->IsPresent();
}

wrl::ComPtr<WUComp::ICompositionBrush> AlphaMask::GetCompositionBrush()
{
    return m_pImpl->GetCompositionBrush();
}

_Check_return_ HRESULT AlphaMask::Impl::Ensure(_In_ CUIElement* pUIElement)
{
    if (!IsPresent())
    {
        wrl::ComPtr<WUComp::ICompositor> spCompositor = GetDCompTreeHostNoRef(pUIElement)->GetCompositor();
        IFCFAILFAST(spCompositor->CreateSurfaceBrush(m_spCompositionSurfaceBrush.ReleaseAndGetAddressOf()));

        // It is not safe to assume we can create a DComp surface at this time as it's possible the D3D device
        // and main SurfaceFactory were released.  This can happen if the app is using the ReleaseGraphicsDeviceOnSuspend
        // feature, or if the app lost its device and was ticking while the window was hidden.
        bool deviceLost = false;
        IFC_RETURN(pUIElement->GetContext()->DetermineDeviceLost(&deviceLost));

        // If the DComp device has been put into the offered state, we must avoid attempting to create a DComp surface.
        bool isSuspended = !!pUIElement->GetContext()->IsSuspended();

        if (isSuspended || deviceLost)
        {
            // If either of the above are true, just dirty the element and bail out, we'll hand the
            // app back an empty CompositionBrush and create/draw the AlphaMask surface on the next RenderWalk.
            CUIElement::NWSetContentDirty(pUIElement, DirtyFlags::Render);
        }
        else
        {
            // Rasterize only on the first call if the brush isn't available, otherwise updates will be done when there
            // is a render walk for the element.
            HRESULT hr = UpdateIfAvailable(pUIElement);

            // We cannot allow device lost errors to bubble back to the app as the app is not responsible for recovering,
            // and will likely crash.  Give the core a chance to inspect the HRESULT and begin the recovery process.
            // If the device was lost we'll swallow the error and hand the app back an empty CompositionBrush,
            // then as soon as we can recover our device we'll re-walk the tree and create/draw the AlphaMask surface.
            pUIElement->GetContext()->HandleDeviceLost(&hr);
            IFCFAILFAST(hr);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT AlphaMask::Impl::UpdateIfAvailable(_In_ CUIElement* pUIElement)
{
    if (IsPresent())
    {
        // Compute the element size with realization scale
        CTransformToRoot realizationTransform;
        float realizationScaleX;
        float realizationScaleY;
        IFC_RETURN(pUIElement->ComputeRealizationTransform(&realizationTransform));
        IFC_RETURN(realizationTransform.GetScaleDimensions(pUIElement, &realizationScaleX, &realizationScaleY));

        auto scaledElementWidth = static_cast<uint32_t>(realizationScaleX * pUIElement->GetActualWidth());
        auto scaledElementHeight = static_cast<uint32_t>(realizationScaleY * pUIElement->GetActualHeight());

        auto hasSize = (scaledElementWidth != 0) && (scaledElementHeight != 0);

        // Allocate and connect the backing surface
        if (hasSize && pUIElement->IsActive())
        {
            auto pDCompTreeHostNoRef = GetDCompTreeHostNoRef(pUIElement);

            float adjustedScaleFactor;
            IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(
                EnsureBackingSurface(pDCompTreeHostNoRef, scaledElementWidth, scaledElementHeight, &adjustedScaleFactor));

            ConnectBackingSurface(pDCompTreeHostNoRef);

            IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(RasterizeElement(
                pUIElement,
                adjustedScaleFactor,
                nullptr /* rasterizationScale - used only for the render walk */,
                true /* includeElementLocalProperties - AlphaMask respects local properties */,
                true /* renderFill */,
                true /* renderStroke */,
                nullptr /* shapeMaskOffset */,
                m_spBackingSurface.Get(),
                false /* renderCollapsedMask */,
                m_spCompositionSurfaceBrush.Get()));
        }
        else if (m_connected)
        {
            // Content is collapsed or not visible, so remove the backing surface and
            // update it on the brush.
            Hide();
        }
    }

    return S_OK;
}

void AlphaMask::Impl::Hide()
{
    if (IsPresent())
    {
        HRESULT hr = m_spCompositionSurfaceBrush->put_Surface(nullptr);

        // It's possible the app closed the brush out from under XAML. Ignore RO_E_CLOSED.
        if (hr != RO_E_CLOSED)
        {
            IFCFAILFAST(hr);
        }

        m_spBackingSurface = nullptr;
        m_connected = false;
    }
}

_Check_return_ HRESULT AlphaMask::Impl::EnsureBackingSurface(
    _In_ DCompTreeHost* pDCompTreeHostNoRef,
    uint32_t width,
    uint32_t height,
    _Out_ float* adjustedScaleFactor)
{
    ASSERT(width > 0);
    ASSERT(height > 0);

    *adjustedScaleFactor = 1.0f;

    if ((m_spBackingSurface == nullptr) ||
        m_spBackingSurface->IsDiscarded() ||
        (width != m_spBackingSurface->GetWidthWithoutGutters()) ||
        (height != m_spBackingSurface->GetHeightWithoutGutters()))
    {
        uint32_t maxTextureSize = pDCompTreeHostNoRef->GetMaxTextureSize();
        uint32_t maxDimension = MAX(width, height);
        if (maxDimension > maxTextureSize)
        {
            // The surface is over maxTextureSize.
            // Scale down to maxTextureSize, clamping if necessary to account for floating point error.
            *adjustedScaleFactor = static_cast<float>(maxTextureSize)/static_cast<float>(maxDimension);
            width = MIN(static_cast<uint32_t>(width*(*adjustedScaleFactor)), maxTextureSize);
            height = MIN(static_cast<uint32_t>(height*(*adjustedScaleFactor)), maxTextureSize);
        }

        IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(pDCompTreeHostNoRef->CreateSurface(
            width,
            height,
            false, // isOpaque
            true,  // isAlphaMask
            false, // isVirtual
            false, // isHDR
            true,  // requestAtlas
            m_spBackingSurface.ReleaseAndGetAddressOf()));

        m_connected = false;
    }

    return S_OK;
}

void AlphaMask::Impl::ConnectBackingSurface(_In_ DCompTreeHost* pDCompTreeHostNoRef)
{
    if (!m_connected)
    {
        FAIL_FAST_ASSERT(m_spBackingSurface != nullptr);
        FAIL_FAST_ASSERT(m_spCompositionSurfaceBrush != nullptr);

        wrl::ComPtr<WUComp::ICompositionSurface> spCompositionSurface;
        IFCFAILFAST(pDCompTreeHostNoRef->GetCompositorPrivate()->CreateCompositionSurfaceForDCompositionSurface(
            m_spBackingSurface->GetIDCompSurface(),
            spCompositionSurface.ReleaseAndGetAddressOf()));

        HRESULT hr = m_spCompositionSurfaceBrush->put_Surface(spCompositionSurface.Get());

        // It's possible the app closed the brush out from under XAML. Ignore RO_E_CLOSED and limp along without a surface inside the brush.
        if (hr != RO_E_CLOSED)
        {
            IFCFAILFAST(hr);
        }

        m_connected = true;
    }
}

/* static */ _Check_return_ HRESULT AlphaMask::RasterizeFill(_In_ CUIElement* pUIElement, const CMILMatrix& realizationScale, _In_ const XPOINTF* shapeMaskOffset, const bool renderCollapsedMask, _In_ DCompSurface* surface)
{
    return AlphaMask::Impl::RasterizeElement(
        pUIElement,
        1.0f /* additionalScaleFactor */,
        &realizationScale,
        false /* includeElementLocalProperties */,
        true /* renderFill */,
        false /* renderStroke */,
        shapeMaskOffset,
        surface,
        renderCollapsedMask,
        nullptr /* compositionBrush */);
}

/* static */ _Check_return_ HRESULT AlphaMask::RasterizeStroke(_In_ CUIElement* pUIElement, const CMILMatrix& realizationScale, _In_ const XPOINTF* shapeMaskOffset, const bool renderCollapsedMask, _In_ DCompSurface* surface)
{
    return AlphaMask::Impl::RasterizeElement(
        pUIElement,
        1.0f /* additionalScaleFactor */,
        &realizationScale,
        false /* includeElementLocalProperties */,
        false /* renderFill */,
        true /* renderStroke */,
        shapeMaskOffset,
        surface,
        renderCollapsedMask,
        nullptr /* compositionBrush */);
}

/* static */ _Check_return_ HRESULT AlphaMask::Impl::RasterizeElement(
    _In_ CUIElement* pUIElement,
    const float additionalScaleFactor,
    _In_opt_ const CMILMatrix* realizationScale,
    const bool includeElementLocalProperties,
    const bool renderFill,
    const bool renderStroke,
    _In_opt_ const XPOINTF* shapeMaskOffset,
    _In_ DCompSurface* surface,
    const bool renderCollapsedMask,
    _In_opt_ WUComp::ICompositionSurfaceBrush* compositionBrush)
{
    XRECT updateRect =
    {
        0,
        0,
        surface->GetWidthWithoutGutters(),
        surface->GetHeightWithoutGutters(),
    };

    XPOINT beginDrawOffset = {};

    wrl::ComPtr<IUnknown> spUnknown;
    IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(surface->BeginDraw(
        &updateRect,
        __uuidof(IDXGISurface),
        spUnknown.ReleaseAndGetAddressOf(),
        &beginDrawOffset));

    wrl::ComPtr<IDXGISurface> spDXGISurface;
    IFCFAILFAST(spUnknown.As(&spDXGISurface));

    // D2D rasterization code
    CD2DFactory* pD2DFactory;

    xref_ptr<CD2DRenderTarget<IPALAcceleratedRenderTarget>> spD2DRenderTarget;
    {
        CWindowRenderTarget* pRenderTargetNoRef = pUIElement->GetContext()->NWGetWindowRenderTarget();
        FAIL_FAST_ASSERT(pRenderTargetNoRef != nullptr);

        CD3D11Device *pDeviceNoRef = pRenderTargetNoRef->GetGraphicsDeviceManager()->GetGraphicsDevice();
        FAIL_FAST_ASSERT(pDeviceNoRef != nullptr);

        CD3D11SharedDeviceGuard guard;
        IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(pDeviceNoRef->TakeLockAndCheckDeviceLost(&guard));

        pD2DFactory = GetSharedD2DFactoryNoRef(pUIElement);

        ID2D1DeviceContext* pSharedD2DDeviceContextNoRef = pDeviceNoRef->GetD2DDeviceContext(&guard);
        FAIL_FAST_ASSERT(pSharedD2DDeviceContextNoRef != nullptr);

        spD2DRenderTarget = make_xref<CD2DRenderTarget<IPALAcceleratedRenderTarget>>(pSharedD2DDeviceContextNoRef);

        IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(
            spD2DRenderTarget->SetDxgiTarget(spDXGISurface.Get()));

        IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(
            spD2DRenderTarget->BeginDraw());

        pSharedD2DDeviceContextNoRef->Clear(D2D1::ColorF(D2D1::ColorF::Black, 0.0f));
    }


    CMILMatrix rootOffsetTransform(true /* initialize to identity */);
    if (includeElementLocalProperties)
    {
        // Preserve only the scale of the element and apply surface offset so it draws into the
        // appropriate spot in the atlas.
        // The scale of the element is preserved so that maximum quality is achieved on the texture.  Other
        // transforms should be applied via the app directly on the visual (or parent of the visual).
        // RightToLeft flow direction must adjust the matrices to render from right to left.
        // Refer to XamlLocalTransformBuilder::ApplyFlowDirection or WinRTLocalExpressionBuilder::ApplyFlowDirection
        // for the matrix stack associated with RTL.
        CTransformToRoot realizationTransform;
        float realizationScaleX;
        float realizationScaleY;
        IFC_RETURN(pUIElement->ComputeRealizationTransform(&realizationTransform));
        IFC_RETURN(realizationTransform.GetScaleDimensions(pUIElement, &realizationScaleX, &realizationScaleY));
        realizationScaleX *= additionalScaleFactor;
        realizationScaleY *= additionalScaleFactor;
        
        bool shouldFlipRTL;
        bool shouldFlipRTLInPlace;
        pUIElement->GetShouldFlipRTL(&shouldFlipRTL, &shouldFlipRTLInPlace);
        auto rtlScaleX = shouldFlipRTL ? -1.f : 1.f;
        auto rtlOffset = (shouldFlipRTL && shouldFlipRTLInPlace) ? pUIElement->GetActualWidth() : 0.f;

        rootOffsetTransform.Scale(realizationScaleX * rtlScaleX, realizationScaleY);
        rootOffsetTransform.SetDx(static_cast<float>(beginDrawOffset.x) + rtlOffset);
        rootOffsetTransform.SetDy(static_cast<float>(beginDrawOffset.y));
    }
    else
    {
        rootOffsetTransform = *realizationScale;

        // Note: subtract the shapeMaskOffset. This is the top-left corner of the shape, which we want to move to (0, 0).
        // A path that has the top-left corner at (50, 20) needs to have (50, 20) subtracted. A path that has the top-left
        // corner at (-100, -40) needs to have (100, 40) added.
        rootOffsetTransform.SetDx(static_cast<float>(beginDrawOffset.x - shapeMaskOffset->x));
        rootOffsetTransform.SetDy(static_cast<float>(beginDrawOffset.y - shapeMaskOffset->y));
    }

    SharedRenderParams sharedParams;
    sharedParams.overrideTransform = true;
    sharedParams.pCurrentTransform = &rootOffsetTransform;
    sharedParams.renderCollapsedMask = renderCollapsedMask;

    // Print the print visual.
    D2DPrecomputeParams cp(pD2DFactory, nullptr);
    D2DRenderParams printParams(spD2DRenderTarget.get(), nullptr, TRUE);
    printParams.m_fForceVector = FALSE;
    printParams.m_renderFill = renderFill;
    printParams.m_renderStroke = renderStroke;

    if (includeElementLocalProperties)
    {
        IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(pUIElement->Print(sharedParams, cp, printParams));
    }
    else
    {
        // We're rendering the element while ignoring its properties. Set the root transform here before calling into the element.
        IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(spD2DRenderTarget->SetTransform(&rootOffsetTransform));

        // We don't care about the element's brushes since we just want an alpha mask. Use a simple SolidColorBrush.
        wrl::ComPtr<IPALAcceleratedBrush> overrideBrush;
        IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(spD2DRenderTarget->CreateSolidColorBrush(0xffffffff, 1.0f, overrideBrush.ReleaseAndGetAddressOf()));
        printParams.SetOverrideBrush(overrideBrush.Get());

        // This is the branch used by the render walk to generate an alpha mask for the element. We don't want to bake
        // any of the element's local properties into the mask, which CUIElement::Print incorporates, so call
        // CUIElement::PreChildrenPrintVirtual directly.
        IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(pUIElement->PreChildrenPrintVirtual(sharedParams, cp, printParams));
    }

    IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(spD2DRenderTarget->EndDraw());

    IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(surface->EndDraw());

    if (compositionBrush != nullptr && additionalScaleFactor != 1.0f)
    {
        // If we're here it indicates we scaled the surface down to fit in maxTextureSize.
        // Apply the inverse of this scale factor so that it renders at the intended size.
        wrl::ComPtr<WUComp::ICompositionSurfaceBrush2> brush2;
        VERIFYHR(compositionBrush->QueryInterface(IID_PPV_ARGS(brush2.ReleaseAndGetAddressOf())));

        wfn::Vector2 inverseScale = {1.0f/additionalScaleFactor, 1.0f/additionalScaleFactor};
        IFCFAILFAST(brush2->put_Scale(inverseScale));
    }

    return S_OK;
}

wrl::ComPtr<WUComp::ICompositionBrush> AlphaMask::Impl::GetCompositionBrush()
{
    wrl::ComPtr<WUComp::ICompositionBrush> spCompositionBrush;
    IFCFAILFAST(m_spCompositionSurfaceBrush.As(&spCompositionBrush));
    return spCompositionBrush;
}

_Ret_notnull_ DCompTreeHost* AlphaMask::Impl::GetDCompTreeHostNoRef(
    _In_ CDependencyObject* pDO
    )
{
    CWindowRenderTarget* pRenderTargetNoRef = pDO->GetContext()->NWGetWindowRenderTarget();
    FAIL_FAST_ASSERT(pRenderTargetNoRef != nullptr);

    DCompTreeHost* pDCompTreeHostNoRef = pRenderTargetNoRef->GetDCompTreeHost();
    FAIL_FAST_ASSERT(pDCompTreeHostNoRef != nullptr);

    return pDCompTreeHostNoRef;
}

_Ret_notnull_ CD2DFactory* AlphaMask::Impl::GetSharedD2DFactoryNoRef(
    _In_ CDependencyObject* pDO
    )
{
    CWindowRenderTarget* pRenderTargetNoRef = pDO->GetContext()->NWGetWindowRenderTarget();
    FAIL_FAST_ASSERT(pRenderTargetNoRef != nullptr);

    CD3D11Device *pDeviceNoRef = pRenderTargetNoRef->GetGraphicsDeviceManager()->GetGraphicsDevice();
    FAIL_FAST_ASSERT(pDeviceNoRef != nullptr);

    IFCFAILFAST(pDeviceNoRef->EnsureD2DResources());

    CD2DFactory *pSharedD2DFactoryNoRef = pDeviceNoRef->GetD2DFactory();
    FAIL_FAST_ASSERT(pSharedD2DFactoryNoRef != nullptr);

    return pSharedD2DFactoryNoRef;
}
