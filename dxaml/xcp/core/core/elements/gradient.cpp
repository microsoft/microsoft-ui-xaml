// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CColor.h>
#include <LinearGradientBrush.h>
#include <ColorUtil.h>

//------------------------------------------------------------------------
//
//  Method:   CGradientStop
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------
CGradientStop::CGradientStop(_In_ CCoreServices *pCore)
    : CDependencyObject(pCore)
    , m_uLastUpdate(pCore->m_uFrameNumber)
{
    memset(&m_stop, 0, sizeof(XcpGradientStop));
}

//------------------------------------------------------------------------
//
//  Method:   SetValue
//
//  Synopsis:
//      Sets a value in the dependency object.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGradientStop::SetValue(_In_ const SetValueParams& args)
{
    HRESULT hr = S_OK;
    CDependencyObject *pObject = nullptr;
    CValue convertedValue;
    const CValue* pValueToUse = &args.m_value;

    // It is illegal to try to add a child object to a gradient stop
    if (args.m_pDP == nullptr)
    {
        IFC(E_FAIL);
    }

    // See if this is one of the gradient stop color property.  If it is we need to
    // compute the sRGB color and store it before calling the base SetValue method.
    if (args.m_pDP->IsContentProperty())
    {
        XUINT32 rgb = 0;

        // Convert the incoming property to a 32bit color
        switch (args.m_value.GetType())
        {
            case valueColor:
                rgb = args.m_value.AsColor();
                break;

            case valueString:
                // Call the type converter on the incoming string
                {
                    CREATEPARAMETERS cp(GetContext(), args.m_value);
                    const CREATEPFN pfnCreate = args.m_pDP->GetPropertyType()->GetCoreConstructor();
                    IFC(pfnCreate(&pObject, &cp));
                    convertedValue.SetObjectAddRef(pObject);
                    pValueToUse = &convertedValue;
                    rgb = checked_cast<CColor>(convertedValue.AsObject())->m_rgb;
                }
                break;

            // and fall through to the object to type converter
            case valueObject:
                rgb = checked_cast<CColor>(args.m_value.AsObject())->m_rgb;
                break;

            default:
                IFC(E_FAIL);
        }

#pragma warning(push)
#pragma warning(disable:4244) // 'argument' : conversion from 'type1' to 'type2', possible loss of data
        // Now compute and store the sRGB color for the gradient
        m_stop.color.a = MIL_COLOR_GET_ALPHA(rgb) / 255.0f;
        m_stop.color.r = Convert_sRGB_UINT16_To_scRGB_float(MIL_COLOR_GET_RED(rgb) << 8);
        m_stop.color.g = Convert_sRGB_UINT16_To_scRGB_float(MIL_COLOR_GET_GREEN(rgb) << 8);
        m_stop.color.b = Convert_sRGB_UINT16_To_scRGB_float(MIL_COLOR_GET_BLUE(rgb) << 8);
#pragma warning(pop)
    }

    // Mark the stop as dirty
    m_uLastUpdate = GetContext()->m_uFrameNumber;

    IFC(CDependencyObject::SetValue(SetValueParams(args, *pValueToUse)));

Cleanup:
    ReleaseInterface(pObject);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CGradientStop::Clone
//
//  Synopsis:
//      Create a clone of 'this' gradient stop
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGradientStop::Clone( _Out_ CGradientStop **ppClone )
{
    HRESULT hr = S_OK;
    CGradientStop *pLocalClone = NULL;
    CREATEPARAMETERS cp(GetContext());

    IFC(CreateDO(&pLocalClone, &cp));

    pLocalClone->m_rgb = m_rgb;
    pLocalClone->m_stop = m_stop;

    IFC(pLocalClone->ClonePropertySetField(this));

    // m_uLastUpdate was taken care of by the constructor.
    *ppClone = pLocalClone;
    pLocalClone = NULL;

Cleanup:
    ReleaseInterface(pLocalClone);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CreateFromArrays
//
//  Synopsis:
//      Creates an instance of a gradient stop collection to for use by the
//  gradient brushes.
//
//  Implementation notes:
//      We create the array with two extra elements to allow extra values
//  to be added to the array without reallocation.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGradientStopCollection::CreateFromArray(
    _Outptr_result_buffer_((nCount + 2)) XcpGradientStop **ppGradientStopsTarget,
    _In_ XUINT32 nCount,
    _In_ CGradientStopCollection *pGradientStopsSource
)
{
    XcpGradientStop *pStops;
    XUINT32 iCount;

    ASSERT(nCount == pGradientStopsSource->GetCount());
    _Analysis_assume_(nCount == pGradientStopsSource->GetCount());

    // avoid potential overflow
    IFCEXPECT_RETURN(nCount <= ((XUINT32_MAX - 2) / sizeof(XcpGradientStop)));

// Now allocate the new stops collection

    pStops = new XcpGradientStop[nCount + 2];

// Initialize the flattened stop collection from the arrays.

    auto& gradientStopsSource = pGradientStopsSource->GetCollection();
    for (iCount = 0; iCount != nCount; iCount++)
    {
        pStops[iCount] = static_cast<CGradientStop*>(gradientStopsSource[iCount])->m_stop;
    }

// Return the object to the caller

    *ppGradientStopsTarget = pStops;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Append
//
//  Synopsis:
//      Overload of CDOCollection so we can redraw the frame
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGradientStopCollection::Append(CValue& value, _Out_opt_ XUINT32 *pnIndex)
{
    IFC_RETURN(CDOCollection::Append(value, pnIndex));

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   Append
//
//  Synopsis:
//      Overload of CDOCollection so we can redraw the frame
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGradientStopCollection::Append(_In_ CDependencyObject *pObject, _Out_opt_ XUINT32 *pnIndex)
{
    IFC_RETURN(CDOCollection::Append(pObject, pnIndex));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Insert
//
//  Synopsis:
//      Overload of CDOCollection so we can redraw the frame
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGradientStopCollection::Insert(_In_ XUINT32 nIndex, CValue& value)
{
    IFC_RETURN(CDOCollection::Insert(nIndex, value));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   RemoveAt
//
//  Synopsis:
//      Overload of CDOCollection so we can redraw the frame
//
//------------------------------------------------------------------------
_Check_return_ void*
CGradientStopCollection::RemoveAt(_In_ XUINT32 nIndex)
{
    void *pResult = CDOCollection::RemoveAt(nIndex);
    return pResult;
}

//------------------------------------------------------------------------
//
//  Method:   Clear
//
//  Synopsis:
//      Overload of CDOCollection so we can redraw the frame
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGradientStopCollection::Clear()
{
    IFC_RETURN(CDOCollection::Clear());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Clone
//
//  Synopsis:
//      Return a clone of 'this' collection of gradient stops.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGradientStopCollection::Clone( _Out_ CGradientStopCollection **ppClone )
{
    HRESULT hr = S_OK;
    CREATEPARAMETERS cp(GetContext());
    CGradientStopCollection *pLocalClone = NULL;
    CGradientStop *pOriginalStop = NULL;
    CGradientStop *pClonedStop = NULL;

    IFC(CreateDO(&pLocalClone, &cp));

    for( XUINT32 i = 0; i < GetCount(); i++ )
    {
        // Can't use DoPointerCast as GetItemWithAddRef() returns void*
        pOriginalStop = reinterpret_cast<CGradientStop*>(GetItemWithAddRef(i));
        IFC(pOriginalStop->Clone(&pClonedStop));
        ReleaseInterface(pOriginalStop);

        IFC(pLocalClone->Append(pClonedStop));
        ReleaseInterface(pClonedStop);
    }

    *ppClone = pLocalClone;
    pLocalClone = NULL;

Cleanup:
    ReleaseInterface(pLocalClone);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Destructor for a the gradient brush base class.
//
//------------------------------------------------------------------------
CGradientBrush::~CGradientBrush()
{
    ReleaseInterface(m_pStops);
}

//------------------------------------------------------------------------
//
//  Method:   Constructor for CNoParentShareableDependencyObject::Clone()
//
//------------------------------------------------------------------------
CGradientBrush::CGradientBrush(_In_ const CGradientBrush& original, _Out_ HRESULT& hr) : CBrush(original, hr)
{
    CGradientStopCollection *pClonedStopCollection = NULL;

    // Initialization to valid state
    m_nInterpolate = original.m_nInterpolate;
    m_nSpread      = original.m_nSpread;
    m_nMapping     = original.m_nMapping;
    m_pStops       = NULL;

    // Halt if base constructor had problems
    IFC(hr);

    // Proceed to remainder of work
    if (original.m_pStops)
    {
        IFC(original.m_pStops->Clone(&pClonedStopCollection));

        IFC(SetValueByKnownIndex(KnownPropertyIndex::GradientBrush_GradientStops, pClonedStopCollection)); // Adds ref to collection
    }

Cleanup:
    ReleaseInterface(pClonedStopCollection);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if the brush has gradient stops and none of them have alpha.
//
//------------------------------------------------------------------------
bool
CGradientBrush::IsOpaque() const
{
    XUINT32 gradientStopCount = m_pStops->GetCount();
    bool stopHasAlpha = false;
    for (XUINT32 i = 0; !stopHasAlpha && i < gradientStopCount; i++)
    {
        CGradientStop *pStop = reinterpret_cast<CGradientStop *>(m_pStops->GetItemWithAddRef(i));\
        if (!ColorUtils::IsOpaqueColor(pStop->m_rgb))
        {
            stopHasAlpha = TRUE;
        }
        ReleaseInterface(pStop);
    }

    return gradientStopCount > 0 && !stopHasAlpha;
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGradientBrush::GetAcceleratedGradientStops(
    _Outptr_ XcpGradientStop **ppGradientStops,
    _Out_ XUINT32 *pGradientStopCount,
    _Out_ InterpolationMode *pInterpolationMode,
    _Out_ GradientWrapMode *pGradientWrapMode
    )
{
    CGradientStop *pStop = NULL;
    XUINT32 gradientStopCount = m_pStops->GetCount();
    XcpGradientStop *pGradientStops = new XcpGradientStop[gradientStopCount];

    for (XUINT32 i = 0; i < gradientStopCount; i++)
    {
        pStop = reinterpret_cast<CGradientStop *>(m_pStops->GetItemWithAddRef(i));
        pGradientStops[i].rPosition = pStop->m_stop.rPosition;
        pGradientStops[i].color.a = static_cast<XFLOAT>(pStop->m_rgb >> 24 & 0xff) / 255.0f;
        pGradientStops[i].color.r = static_cast<XFLOAT>(pStop->m_rgb >> 16 & 0xff) / 255.0f;
        pGradientStops[i].color.g = static_cast<XFLOAT>(pStop->m_rgb >> 8 & 0xff) / 255.0f;
        pGradientStops[i].color.b = static_cast<XFLOAT>(pStop->m_rgb & 0xff) / 255.0f;
        ReleaseInterface(pStop);
    }

    *pGradientStopCount = gradientStopCount;
    *ppGradientStops = pGradientStops;

    *pInterpolationMode = (m_nInterpolate == XcpColorInterpolationModeScRgbLinearInterpolation)
        ? InterpolationMode::Gamma_1_0
        : InterpolationMode::Gamma_2_2;

    switch (m_nSpread)
    {
        case XcpGradientWrapModeExtend:
            *pGradientWrapMode = GradientWrapMode::Clamp;
            break;

        case XcpGradientWrapModeFlip:
            *pGradientWrapMode = GradientWrapMode::Mirror;
            break;

        case XcpGradientWrapModeTile:
            *pGradientWrapMode = GradientWrapMode::Wrap;
            break;

        default:
            IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Linear gradien brush releases its cached rendering texture on leave.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CLinearGradientBrush::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    IFC_RETURN(CGradientBrush::LeaveImpl(pNamescopeOwner, params));
    if (params.fIsLive)
    {
        ReleaseRenderingCache();
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Constructor for CNoParentShareableDependencyObject::Clone()
//
//------------------------------------------------------------------------
CLinearGradientBrush::CLinearGradientBrush( _In_ const CLinearGradientBrush& original, _Out_ HRESULT& hr ) : CGradientBrush( original, hr )
{
    // Initialization to valid state
    m_ptStart = original.m_ptStart;
    m_ptEnd   = original.m_ptEnd;

    m_requestedTexels = original.m_requestedTexels;

    m_pRenderingCache = NULL;

    // Halt if base constructor had problems
    //IFC(hr);

    // Proceed to remainder of work
    // (None)
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Ensures a PAL brush exists and is up-to-date.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CLinearGradientBrush::UpdateAcceleratedBrush(
    _In_ const D2DRenderParams &renderParams
    )
{
    HRESULT hr = S_OK;
    IPALAcceleratedBrush* pPALBrush = NULL;

    IFC(CreateAcceleratedBrush(renderParams, &pPALBrush));
    ReplaceInterface(m_pPALBrush, pPALBrush);

    IFC(CBrush::UpdateAcceleratedBrush(renderParams));

Cleanup:
    ReleaseInterface(pPALBrush);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the print brush corresponding to this brush.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CLinearGradientBrush::GetPrintBrush(
    _In_ const D2DRenderParams &printParams,
    _Outptr_ IPALAcceleratedBrush **ppBrush
    )
{
    RRETURN(CreateAcceleratedBrush(printParams, ppBrush));
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a D2D/print brush from the render params.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CLinearGradientBrush::CreateAcceleratedBrush(
    _In_ const D2DRenderParams& renderParams,
    _Outptr_ IPALAcceleratedBrush **ppBrush
    )
{
    //
    // We create a horizontal gradient so that we can reuse logic from the
    // software rasterizer when computing the brush transform. See
    // CLinearGradientBrush::GetBrushParams.
    //

    HRESULT hr = S_OK;
    IPALAcceleratedBrush *pPALBrush = NULL;
    XcpGradientStop *pGradientStops = NULL;

    // If there are no gradient stops just return a transparent brush.
    if (m_pStops == NULL || m_pStops->GetCount() == 0)
    {
        IFC(renderParams.GetD2DRenderTarget()->CreateSolidColorBrush(
            0 /* uiColor */,
            0.0f /* rOpacity */,
            &pPALBrush
            ));
    }
    else
    {
        XUINT32 gradientStopCount = 0;
        InterpolationMode interpolationMode;
        GradientWrapMode gradientWrapMode;

        //
        // The software rasterizer creates a horizontal gradient. We do the
        // same here.
        //
        XPOINTF start = {0, 0};
        XPOINTF stop = {1, 0};

        IFC(GetAcceleratedGradientStops(
            &pGradientStops,
            &gradientStopCount,
            &interpolationMode,
            &gradientWrapMode
            ));

        IFC(renderParams.GetD2DRenderTarget()->CreateLinearGradientBrush(
            start,
            stop,
            pGradientStops,
            gradientStopCount,
            interpolationMode,
            gradientWrapMode,
            GetOpacity(),
            &pPALBrush
            ));
    }

    SetInterface(*ppBrush, pPALBrush);

Cleanup:
    delete[] pGradientStops;
    ReleaseInterface(pPALBrush);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates the device independent D2D resources for the brush.
//      Called during prerender.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CLinearGradientBrush::D2DEnsureDeviceIndependentResources(
    _In_ const D2DPrecomputeParams& cp,
    _In_ const CMILMatrix *pMyAccumulatedTransform,
    _In_ const XRECTF_RB *pBrushBounds,
    _Inout_ AcceleratedBrushParams *pPALBrushParams
    )
{
    //
    // We recycle logic from the software rasterizer in order to figure out
    // the correct transform that fills the geometry with this linear gradient
    // brush. The software rasterizer realizes the gradient into a horizontal
    // texture, then computes a transform to map that back to the bounds of
    // the geometry being filled.
    //

    XPOINTF brushPoints[3];
    CMILMatrix brushTransform(TRUE);
    CMILMatrix relativeTransform(TRUE);
    CMILMatrix absoluteTransform(TRUE);
    XRECTF brushBounds(ToXRectF(*pBrushBounds));

    //
    // The source is the horizontal gradient we created in UpdateAcceleratedBrush.
    // It started at (0,0) and stopped at (1,0) -- 1 pixel wide. The height doesn't
    // matter since a horizontal gradient isn't affected by being scaled vertically.
    // We arbitrarily use 1.
    //
    XRECTF sourceBounds = {0, 0, 1, 1};

    IFC_RETURN(CBrush::D2DEnsureDeviceIndependentResources(
        cp,
        pMyAccumulatedTransform,
        pBrushBounds,
        pPALBrushParams
        ));

    // Scale the gradient up to fill the bounds if needed
    if (m_nMapping == XcpRelative)
    {
        // Start point of the target gradient, in absolute coordinates
        brushPoints[0].x = m_ptStart.x * brushBounds.Width + brushBounds.X;
        brushPoints[0].y = m_ptStart.y * brushBounds.Height + brushBounds.Y;

        // End point of the target gradient, in absolute coordinates
        brushPoints[1].x = m_ptEnd.x * brushBounds.Width + brushBounds.X;
        brushPoints[1].y = m_ptEnd.y * brushBounds.Height + brushBounds.Y;
    }
    else
    {
        brushPoints[0] = m_ptStart;
        brushPoints[1] = m_ptEnd;
    }

    //
    // Gradient direction, which is perpendicular to the direction of the gradient.
    // In the software rasterizer a skew brush transform can change this. For
    // hardware we just push the skew transform as part of the brush transform.
    //
    brushPoints[2].x = -(brushPoints[1].y - brushPoints[0].y) + brushPoints[0].x;
    brushPoints[2].y = (brushPoints[1].x - brushPoints[0].x) + brushPoints[0].y;

    //
    // Collect the brush transforms and pass the start/end/direction points
    // through them.
    //
    auto rt = GetRelativeTransform();
    if (rt != nullptr)
    {
        rt->GetTransform(&relativeTransform);
    }

    auto t = GetTransform();
    if (t != nullptr)
    {
        t->GetTransform(&absoluteTransform);
    }

    CBrushTypeUtils::GetBrushTransform(
        &relativeTransform,
        &absoluteTransform,
        &brushBounds,
        &brushTransform
        );

    if (!brushTransform.IsIdentity())
    {
        brushTransform.Transform(
            brushPoints,
            brushPoints,
            3
            );
    }

    //
    // Use the transformed points to calculate the transform that maps our 1x1
    // gradient to the start/end/direction points.
    // See CGradientTextureGenerator::CalculateTextureMapping.
    //
    pPALBrushParams->Reset();
    pPALBrushParams->m_transform.InferAffineMatrix(
        brushPoints,
        sourceBounds
        );

    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      The virtual method which does the tree walk to clean up all
//      the device related resources like brushes, textures,
//      primitive composition data etc. in this subgraph.
//
//-----------------------------------------------------------------------------
void CLinearGradientBrush::CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp)
{
    CGradientBrush::CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    ReleaseRenderingCache();
}
