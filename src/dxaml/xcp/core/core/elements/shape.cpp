// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <MetadataAPI.h>
#include "VisualContentRenderer.h"

using namespace DirectUI;

CShape::~CShape()
{
    ReleaseInterface(m_pRender);
    ReleaseInterface(m_pFill);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Init the pen from the shape properties
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CShape::InitPenFromShape(_Out_ CPlainPen *pPen)
{
    pPen->SetStartCap(GetStrokeStartLineCap());
    pPen->SetEndCap(GetStrokeEndLineCap());
    pPen->SetMiterLimit(GetStrokeMiterLimit());
    pPen->SetWidth(GetStrokeThickness());
    pPen->SetHeight(GetStrokeThickness());
    pPen->SetJoin(GetStrokeLineJoin());
    pPen->SetDashOffset(GetStrokeDashOffset());
    pPen->SetDashCap(GetStrokeDashCap());

    if (HasStrokeDashArray())
    {
        auto strokeDashArray = GetStrokeDashArray();
        // Note: HasStrokeDashArray checks sparse storage, but it's possible that an explicit null value is placed
        // in sparse storage, so we have to check for null here as well. We don't want to call GetStrokeDashArray
        // directly because the StrokeDashArray property is marked as created on demand, so a call to that getter
        // will create a StrokeDashArray if we didn't have one.
        if (strokeDashArray != nullptr && strokeDashArray->GetCount() > 0)
        {
            IFC_RETURN(pPen->SetDashArray(strokeDashArray->GetCollection()));
        }
    }
    // Technically there's a bug here, where a blank stroke dash array will not clear out the array stored in pPen.
    // In practice, Xaml never caches the pen and will always populate a new one, so this bug has no symptoms.

    return S_OK;
}

_Check_return_ HRESULT CShape::Stroke(
    _In_ CDependencyObject* obj,
    _In_ UINT32 numberOfArgs,
    _Inout_updates_(numberOfArgs) CValue* args,
    _In_opt_ IInspectable* valueOuter,
    _Out_ CValue* result)
{
    if (numberOfArgs == 1)
    {
        IFC_RETURN(static_cast<CShape*>(obj)->SetStroke(xref_ptr<CBrush>(static_cast<CBrush*>(args->AsObject()))));
        return S_OK;
    }
    else
    {
        result->SetObjectAddRef(static_cast<CShape*>(obj)->GetStroke().get());
        return S_OK;
    }
}

_Check_return_ HRESULT CShape::StrokeDashArray(
    _In_ CDependencyObject* obj,
    _In_ UINT32 numberOfArgs,
    _Inout_updates_(numberOfArgs) CValue* args,
    _In_opt_ IInspectable* valueOuter,
    _Out_ CValue* result)
{
    if (numberOfArgs == 1)
    {
        IFC_RETURN(static_cast<CShape*>(obj)->SetStrokeDashArray(xref_ptr<CDoubleCollection>(static_cast<CDoubleCollection*>(args->AsObject()))));
        return S_OK;
    }
    else
    {
        result->SetObjectAddRef(static_cast<CShape*>(obj)->GetStrokeDashArray().get());
        return S_OK;
    }
}

xref_ptr<CBrush> CShape::GetStroke() const
{
    CValue result;
    VERIFYHR(const_cast<CShape*>(this)->GetEffectiveValueInSparseStorage(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Shape_Stroke), &result));
    return static_sp_cast<CBrush>(result.DetachObject());
}

_Check_return_ HRESULT CShape::SetStroke(xref_ptr<CBrush> stroke)
{
    CValue v;

    if (stroke) v.SetObjectAddRef(stroke.get()); else v.SetNull();
    IFC_RETURN(SetEffectiveValueInSparseStorage(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Shape_Stroke), v));
    return S_OK;
}

float CShape::GetStrokeMiterLimit() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::Shape_StrokeMiterLimit, &result));
    return static_cast<float>(result.AsDouble());
}

float CShape::GetStrokeThickness() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::Shape_StrokeThickness, &result));

    // Only return a non-zero StrokeThickness if Stroke brush has been defined. Otherwise the fill region of the shape will be
    // deflated, and the edge will be antialiased.
    if(!GetStroke())
    {
        return 0.0f;
    }

    return static_cast<float>(abs(result.AsDouble()));
}

float CShape::GetAbsStrokeThickness() const
{
    return abs(GetStrokeThickness());
}

XcpPenCap CShape::GetStrokeStartLineCap() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::Shape_StrokeStartLineCap, &result));
    return static_cast<XcpPenCap>(result.AsEnum());
}

XcpPenCap CShape::GetStrokeEndLineCap() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::Shape_StrokeEndLineCap, &result));
    return static_cast<XcpPenCap>(result.AsEnum());
}

XcpLineJoin CShape::GetStrokeLineJoin() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::Shape_StrokeLineJoin, &result));
    return static_cast<XcpLineJoin>(result.AsEnum());
}

float CShape::GetStrokeDashOffset() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::Shape_StrokeDashOffset, &result));
    return static_cast<float>(result.AsDouble());
}

bool CShape::HasStrokeDashArray() const
{
    return IsEffectiveValueInSparseStorage(KnownPropertyIndex::Shape_StrokeDashArray);
}

xref_ptr<CDoubleCollection> CShape::GetStrokeDashArray() const
{
    CValue result;
    //VERIFYHR(GetValueByIndex(KnownPropertyIndex::Shape_StrokeDashArray, &result));
    VERIFYHR(const_cast<CShape*>(this)->GetEffectiveValueInSparseStorage(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Shape_StrokeDashArray), &result));
    return static_sp_cast<CDoubleCollection>(result.DetachObject());
}

_Check_return_ HRESULT CShape::SetStrokeDashArray(xref_ptr<CDoubleCollection> strokeDashArray)
{
    CValue v;
    if (strokeDashArray) v.SetObjectAddRef(strokeDashArray.get()); else v.SetNull();
    IFC_RETURN(SetEffectiveValueInSparseStorage(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Shape_StrokeDashArray), v));
    return S_OK;
}

XcpPenCap CShape::GetStrokeDashCap() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::Shape_StrokeDashCap, &result));
    return static_cast<XcpPenCap>(result.AsEnum());
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the stretch matrix for this shape.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CShape::GetShapeGeometryTransform(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _In_reads_(cArgs) CValue *pArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
    )
{
    HRESULT hr = S_OK;
    CMatrix *pMatrix = nullptr;

    if (pObject)
    {
        CMatrixTransform *pTransform = nullptr;

        CShape* pShape = static_cast<CShape*>(pObject);
        CREATEPARAMETERS cp(pShape->GetContext());

        IFC(CMatrix::Create(reinterpret_cast<CDependencyObject **>(&pMatrix), &cp));
        IFC(pShape->GetStretchTransform(&pMatrix->m_matrix));

        IFC(CMatrixTransform::Create(reinterpret_cast<CDependencyObject **>(&pTransform), &cp));
        pTransform->m_pMatrix = pMatrix;
        pMatrix = nullptr;

        pResult->SetObjectNoRef(pTransform);
    }
    else
    {
        IFC(E_FAIL);
    }

Cleanup:
    ReleaseInterface(pMatrix);
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the stretch matrix for this shape.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CShape::GetStretchTransform(
    _Out_ CMILMatrix* pMatrix
    )
{
    CMILMatrix stretchMatrix(TRUE);

    // Create geometry if it has not yet been initialized.
    if (m_pRender == nullptr)
    {
        IFC_RETURN(UpdateRenderGeometry());
    }

    // If geometry exists and it has a stretch mode applied
    // then there will be a potentially non-identity transformation
    // matrix.
    if (m_pRender != nullptr && m_Stretch != DirectUI::Stretch::None && m_pRender->m_fNeedsStretching)
    {
        // NOTE: The following code does not calculate the stroke thickness contribution
        //       correctly for transformed geometry but is consistent with rendering.
        //       The correct method would be to use the transformed widened geometry
        //       which will include the correct contribution from stroke thickness and
        //       dashing.
        XRECTF rcNatural;
        XRECTF rcClip;
        XRECTF_RB naturalBounds;
        XFLOAT strokeThickness = GetAbsStrokeThickness();

        // Get the natural bounds of the geometry.
        IFC_RETURN(GetGeometryBounds(&naturalBounds));

        rcNatural = ToXRectF(naturalBounds);

        if ((!m_fIsDefaultWidth && !m_fIsDefaultHeight) || (HasLayoutStorage()))
        {
            //
            // The bounds of the clipped/stretched shape need to account for the
            //  expansion given by the stroke.
            //
            XFLOAT eWidth = GetActualWidth();
            XFLOAT eHeight = GetActualHeight();

            rcClip.Height = MAX(eHeight - strokeThickness, 0.0f);
            rcClip.Width = MAX(eWidth - strokeThickness, 0.0f);
            rcClip.X = strokeThickness / 2.0f;
            rcClip.Y = strokeThickness / 2.0f;

            //
            // Ensure the stretch container has a non-zero area.
            //
            if (rcClip.Width <= REAL_EPSILON || rcClip.Height <= REAL_EPSILON)
            {
                rcClip.Width = rcClip.Width > 0.0f ? rcClip.Width : strokeThickness;
                rcClip.Height = rcClip.Height > 0.0f ? rcClip.Height : strokeThickness;
            }
        }
        else
        {
            ASSERT(strokeThickness >= 0);

            rcClip = rcNatural;
            rcClip.X = strokeThickness / 2.0f;
            rcClip.Y = strokeThickness / 2.0f;
        }

        //
        // Send the natural bounds and the constraint bounds to CGeometry's helper function
        //  that computes the stretch matrix
        //
        IFC_RETURN(m_pRender->ComputeStretchMatrix(m_Stretch, &rcClip, &rcNatural, &stretchMatrix));
    }

    *pMatrix = stretchMatrix;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  CShape::PrintPreChildrenPrintVirtual
//
//  Synopsis:
//      Prints the shape with fill and stroke
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CShape::PreChildrenPrintVirtual(
    _In_ const SharedRenderParams& sharedPrintParams,
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams& printParams
    )
{
    HRESULT hr = S_OK;
    IPALAcceleratedGeometry* pPrintGeometry = nullptr;
    IPALStrokeStyle* pStrokeStyle = nullptr;

    xref_ptr<IPALAcceleratedBrush> pPALFillBrush = nullptr;
    AcceleratedBrushParams *pPALFillBrushParams = nullptr;
    xref_ptr<IPALAcceleratedBrush> pPALStrokeBrush = nullptr;
    AcceleratedBrushParams *pPALStrokeBrushParams = nullptr;

    IPALAcceleratedGeometry *pElementBoundsClip = nullptr;

    bool hasElementBounds = false;
    XRECTF elementBounds = {};
    bool hasNonZeroSize = ShouldRender(&hasElementBounds, &elementBounds);

    if (!m_pFill && !GetStroke())
    {
        goto Cleanup;
    }

    IFC(UpdateRenderGeometry());

    if (m_pRender != nullptr && hasNonZeroSize)
    {
        CUIElement *pParent = do_pointer_cast<CUIElement>(GetParentInternal(false));
        XFLOAT rStrokeThickness = GetAbsStrokeThickness();
        bool shouldClipToBounds = hasElementBounds
            && (m_Stretch == DirectUI::Stretch::None || m_Stretch == DirectUI::Stretch::UniformToFill)
            && !pParent->GetIsLayoutElement();

        if (CanStretchGeometry())
        {
            if (hasElementBounds && (elementBounds.Height <= REAL_EPSILON || elementBounds.Width <= REAL_EPSILON))
            {
                // Stretching to zero; no need to render
                goto Cleanup;
            }

            // Stretch needed, get the stretched and transformed geometry.
            IFC(m_pRender->GetPrintGeometry(
                cp,
                printParams,
                hasElementBounds ? &elementBounds : nullptr,
                m_Stretch,
                rStrokeThickness,
                &pPrintGeometry
                ));
        }
        else
        {
            // No stretch needed, get the transformed unstretched geometry
            IFC(m_pRender->GetPrintGeometry(
                cp,
                printParams,
                &pPrintGeometry
                ));
        }

        // Update shape clip. The shape clips to layout size for stretch None
        // or UniformToFill.
        if (shouldClipToBounds)
        {
            IFC(cp.GetFactory()->CreateRectangleGeometry(
                elementBounds,
                &pElementBoundsClip
                ));
        }

        if (pPrintGeometry != nullptr)
        {
            bool pushedShapeClipLayer = false;
            bool pushedAxisAlignedShapeClip = false;
            bool isClipEmpty = false;

            // Clip to layout size for stretch None or UniformToFill
            IFC(CUIElement::D2DSetUpClipHelper(
                printParams.GetD2DRenderTarget(),
                FALSE /* pushAxisAlignedClip */,
                pElementBoundsClip,
                nullptr /* pContentBounds - the bounds are the same as the bounds of the clip geometry */,
                &pushedShapeClipLayer,
                &pushedAxisAlignedShapeClip,
                &isClipEmpty
                ));

            if (!isClipEmpty)
            {
                // Print with Fill
                if (printParams.m_renderFill && m_pFill != nullptr)
                {
                    pPALFillBrushParams = new AcceleratedBrushParams();

                    if (printParams.GetOverrideBrush() != nullptr)
                    {
                        pPALFillBrush = printParams.GetOverrideBrush().Get();
                    }
                    else
                    {
                        XRECTF_RB fillBrushBounds;

                        IFC(pPrintGeometry->GetBounds(&fillBrushBounds));

                        IFC(m_pFill->D2DEnsureDeviceIndependentResources(
                            cp,
                            sharedPrintParams.pCurrentTransform,
                            &fillBrushBounds,
                            pPALFillBrushParams
                            ));

                        IFC(m_pFill->GetPrintBrush(printParams, pPALFillBrush.ReleaseAndGetAddressOf()));
                    }

                    IFC(CGeometry::DrawAccelerated(
                        pPrintGeometry,
                        printParams,
                        sharedPrintParams.pCurrentTransform,
                        0.0f,       // rStrokeThickness
                        nullptr,
                        1.0f,       // opacity
                        pPALFillBrush,
                        pPALFillBrushParams
                        ));
                }

                // Print with Stroke
                auto stroke = GetStroke();
                if (printParams.m_renderStroke && stroke && rStrokeThickness > 0.0f)
                {
                    pPALStrokeBrushParams = new AcceleratedBrushParams();

                    if (printParams.GetOverrideBrush() != nullptr)
                    {
                        pPALStrokeBrush = printParams.GetOverrideBrush().Get();
                    }
                    else
                    {
                        XRECTF_RB strokeBrushBounds;

                        IFC(pPrintGeometry->GetWidenedBounds(rStrokeThickness, &strokeBrushBounds));

                        IFC(stroke->D2DEnsureDeviceIndependentResources(
                            cp,
                            sharedPrintParams.pCurrentTransform,
                            &strokeBrushBounds,
                            pPALStrokeBrushParams
                            ));

                        IFC(stroke->GetPrintBrush(printParams, pPALStrokeBrush.ReleaseAndGetAddressOf()));
                    }

                    IFC(CreateStrokeStyle(cp.GetFactory(), &pStrokeStyle));

                    IFC(CGeometry::DrawAccelerated(
                        pPrintGeometry,
                        printParams,
                        sharedPrintParams.pCurrentTransform,
                        rStrokeThickness,
                        pStrokeStyle,
                        1.0f,               // opacity
                        pPALStrokeBrush,
                        pPALStrokeBrushParams
                        ));
                }
            }

            IFC(CUIElement::D2DPopClipHelper(
                printParams.GetD2DRenderTarget(),
                pushedShapeClipLayer,
                pushedAxisAlignedShapeClip
                ));
        }
    }


Cleanup:
    ReleaseInterface(pPrintGeometry);
    ReleaseInterface(pStrokeStyle);
    ReleaseInterface(pElementBoundsClip);
    delete pPALFillBrushParams;
    delete pPALStrokeBrushParams;
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Override to base SetValue for some Shape specific property settings
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CShape::SetValue(_In_ const SetValueParams& args)
{
    if (args.m_pDP != nullptr)
    {
        switch (args.m_pDP->GetIndex())
        {
            // If we set the path data then we need to make sure that the dependant render
            // geometry is updated as well. If not then the m_pRender pointer for the shape
            // will be pointing to the freed geometry data and bad things will happen.
        case KnownPropertyIndex::Path_Data:
            ReplaceRenderGeometry(nullptr);
            break;
        case KnownPropertyIndex::FrameworkElement_Width:
        case KnownPropertyIndex::FrameworkElement_MinWidth:
        case KnownPropertyIndex::FrameworkElement_MaxWidth:
            m_fIsDefaultWidth = false;
            break;
        case KnownPropertyIndex::FrameworkElement_Height:
        case KnownPropertyIndex::FrameworkElement_MinHeight:
        case KnownPropertyIndex::FrameworkElement_MaxHeight:
            m_fIsDefaultHeight = false;
            break;
        }
    }

    IFC_RETURN(CFrameworkElement::SetValue(args));

    if (args.m_pDP != nullptr)
    {
        switch (args.m_pDP->GetIndex())
        {
            // If we set the path data then we need to make sure that the dependant render
            // geometry is updated as well. If not then the m_pRender pointer for the shape
            // will be pointing to the freed geometry data and bad things will happen.
        case KnownPropertyIndex::Path_Data:
            ReplaceRenderGeometry(do_pointer_cast<CGeometry>(args.m_value.AsObject()));
            break;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This will be called by the layout system. The result is based
//      on the stretched or unstretched bounds of the shape.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CShape::MeasureOverride(_In_ XSIZEF availableSize, _Inout_ XSIZEF &desiredSize)
{
    HRESULT hr = S_OK;
    XRECTF_RB geometryBounds = { };

    // Give the render geometry a chance to update itself.  This will happen if
    // layout or any properties that affect layout have changed.
    //
    // Note: The render geometry depends on this element's actual size, which most
    //       of the time at this point will be the size the element was last laid out at.
    //       If the current layout pass changes the size of this element, the bounds
    //       cached on UIElement will be invalidated, causing us to correctly update
    //       the render geometry in GenerateContentBounds on-demand when this element's
    //       bounds are requested. So we effectively do not lose the effects of this
    //       layout pass on the render geometry.
    if (GetIsMeasureDirty())
    {
        IFC(UpdateRenderGeometry());
    }

    IFC(GenerateGeometryBounds(&availableSize, m_Stretch, &geometryBounds));

    desiredSize.width = geometryBounds.right;
    desiredSize.height = geometryBounds.bottom;

Cleanup:
    if (FAILED(hr))
    {
        desiredSize.height = desiredSize.width = 0.0f;
        hr = S_OK;
    }

    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This will be called by the layout system. The result is based
//      on the stretched or unstretched bounds of the shape.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CShape::ArrangeOverride(_In_ XSIZEF finalSize, _Inout_ XSIZEF& newFinalSize)
{
    HRESULT hr = S_OK;
    XRECTF_RB geometryBounds = { };

    // NOTE: The bounds here do not support negative offsets. In the case where
    //       a stretch mode of none is used and the geometry goes in to negative
    //       space it will not trigger layout clipping to occur. Only if the geometry
    //       extends past the layout bounds in positive space will the final size
    //       be larger than the available size and trigger layout clipping to occur.
    //
    //       Layout clipping will be applied as needed during the rendering stage
    //       based on geometry rendering bounds and the layout bounds.

    if (m_Stretch == DirectUI::Stretch::None)
    {
        newFinalSize = finalSize;
    }
    else
    {
        IFC(GenerateGeometryBounds(&finalSize, m_Stretch, &geometryBounds));

        newFinalSize.width = geometryBounds.right;
        newFinalSize.height = geometryBounds.bottom;
    }

Cleanup:
    if (FAILED(hr))
    {
        newFinalSize.height = newFinalSize.width = 0.0f;
        hr = S_OK;
    }

    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This RENDERCHANGEDPFN marks this Shape's fill brush as dirty for rendering.
//
//------------------------------------------------------------------------
/* static */ void
CShape::NWSetFillBrushDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::Shape>());
    CShape *pShape = static_cast<CShape *>(pTarget);

    if (!flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        //
        // Fill brush - Dirties: (Render | Bounds)
        //
        pShape->NWSetDirtyFlagsAndPropagate(flags | DirtyFlags::Render | DirtyFlags::Bounds, pShape->m_fFillBrushDirty);
        pShape->m_fFillBrushDirty = true;
    }
    else
    {
        // Independent changes only dirty bounds.
        pShape->NWSetDirtyFlagsAndPropagate(DirtyFlags::Independent | DirtyFlags::Bounds, FALSE);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This RENDERCHANGEDPFN marks this Shape's stroke brush as dirty for rendering.
//
//------------------------------------------------------------------------
/* static */ void
CShape::NWSetStrokeBrushDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::Shape>());
    CShape *pShape = static_cast<CShape *>(pTarget);

    if (!flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        //
        // Stroke brush - Dirties: (Render | Bounds)
        //
        pShape->NWSetDirtyFlagsAndPropagate(flags | DirtyFlags::Render | DirtyFlags::Bounds, pShape->m_fStrokeBrushDirty);
        pShape->m_fStrokeBrushDirty = true;
    }
    else
    {
        // Independent changes only dirty bounds.
        pShape->NWSetDirtyFlagsAndPropagate(DirtyFlags::Independent | DirtyFlags::Bounds, FALSE);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether the stretch mode of the shape will stretch the
//      geometry inside.
//
//------------------------------------------------------------------------
bool CShape::CanStretchGeometry() const
{
    return true;
}

void CShape::OnContentDirty()
{
    if (GetRenderGeometry())
    {
        GetRenderGeometry()->SetWUCGeometryDirty(true);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether we should render this shape. Optionally returns
//      the bounds of the shape, including the entire stroke.
//
//------------------------------------------------------------------------
bool
CShape::ShouldRender(
    _Out_opt_ bool *pHasElementBounds,
    _Out_opt_ XRECTF *pElementBounds
    )
{
    bool areBothDimensionsSet = !m_fIsDefaultWidth && !m_fIsDefaultHeight;
    bool isNeitherDimensionSet = m_fIsDefaultWidth && m_fIsDefaultHeight;

    //
    // The element bounds are used for stretching and clipping. If the bounds aren't available
    // (i.e. the parent is a Canvas that doesn't do layout and there's no explicit size set),
    // then we fall back to the natural bounds of the geometry.
    //
    bool hasElementBounds = areBothDimensionsSet || HasLayoutStorage();
    XRECTF elementBounds = {};
    bool areDimensionsNonZero = true;
    if (hasElementBounds)
    {
        elementBounds.Width = GetActualWidth();
        elementBounds.Height = GetActualHeight();
        areDimensionsNonZero = elementBounds.Height > REAL_EPSILON && elementBounds.Width > REAL_EPSILON;
    }

    if (pHasElementBounds != nullptr)
    {
        *pHasElementBounds = hasElementBounds;
    }

    if (pElementBounds != nullptr)
    {
        *pElementBounds = elementBounds;
    }

    // In order to render this shape, it must satisfy one of the following:
    //    - Has a non-zero layout size set by the parent.
    //    - Or, if the parent is a Canvas that doesn't do layout, has both an explicit
    //      non-zero width and an explicit non-zero height set.
    //    - Or, if the parent is a Canvas that doesn't do layout, has neither an explicit
    //      width nor an explicit height set.
    return (HasLayoutStorage() && areDimensionsNonZero)     // Has non-zero layout size
        || (areBothDimensionsSet && areDimensionsNonZero)   // Has non-zero explicit size
        || (!HasLayoutStorage() && isNeitherDimensionSet);  // Has no layout size and no explicit size
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a IPALStrokeStyle implementation object aggregating all stroke properties.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CShape::CreateStrokeStyle(
    _In_ IPALAcceleratedGraphicsFactory *pD2DFactory,
    _Inout_ IPALStrokeStyle **ppStrokeStyle
    )
{
    HRESULT hr = S_OK;
    IPALStrokeStyle *pStrokeStyle = nullptr;

    DashStyle dashStyle = DashStyle::Solid;

    // StrokeDashArray is needed during rendering and is a create-on-demand property, so accessing it might create one.
    // This is problematic because setting a StrokeDashArray will dirty this Shape, and we aren't allowed to mess with
    // dirty flags while rendering. Check whether we have one first.
    const std::vector<float>* dashes = nullptr;
    xref_ptr<CDoubleCollection> dashArray;
    if (HasStrokeDashArray())
    {
        dashArray = GetStrokeDashArray();
        if (dashArray != nullptr && dashArray->GetCount() > 0)
        {
            dashes = &dashArray->GetCollection();
            dashStyle = DashStyle::Custom;
        }
    }

    IFC(pD2DFactory->CreateStrokeStyle(
        static_cast<LineCapStyle>(GetStrokeStartLineCap()),
        static_cast<LineCapStyle>(GetStrokeEndLineCap()),
        static_cast<LineCapStyle>(GetStrokeDashCap()),
        static_cast<LineJoin>(GetStrokeLineJoin()),
        GetStrokeMiterLimit(),
        dashStyle,
        GetStrokeDashOffset(),
        dashes,
        &pStrokeStyle
        ));

    ReplaceInterface(*ppStrokeStyle, pStrokeStyle);

Cleanup:
    ReleaseInterface(pStrokeStyle);
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a XRECTF that represents the rectangular geometry outline.
//      Used by CRectangle and CEllipse. Accounts for element stretch.
//
//------------------------------------------------------------------------
XRECTF
CShape::GetOutlineRect(
    bool fCheckDegenerateStroke
    )
{
    XRECTF rectBounds;
    XFLOAT rWidth = GetActualWidth();
    XFLOAT rHeight = GetActualHeight();
    XFLOAT rStrokeThickness = GetAbsStrokeThickness();
    XFLOAT stroke = 0;
    XINT32 fDegenerateRectangleStroke = FALSE;

    auto strokeObj = GetStroke();
    // Check for degenerate strokes
    if (   fCheckDegenerateStroke
        && strokeObj
        && (rStrokeThickness >= rWidth || rStrokeThickness >= rHeight))
    {
        fDegenerateRectangleStroke = TRUE;
    }

    // If we have a stroke, shrink the rect so that the stroked shape fits within width, height
    if (strokeObj && !fDegenerateRectangleStroke)
    {
        stroke = rStrokeThickness;
    }

    rectBounds.X = stroke * 0.5f;
    rectBounds.Y = stroke * 0.5f;
    rectBounds.Height = MAX(0, rHeight - stroke);
    rectBounds.Width = MAX(0, rWidth - stroke);

    // Auto layout defaults will cause GetActual* to return XFLOAT_INF.
    // In this case, that is a BAD THING and so we need to reset width and height to 0.
    if (IsInfiniteF(rectBounds.Height))
    {
        rectBounds.Height = 0;
    }
    if (IsInfiniteF(rectBounds.Width))
    {
        rectBounds.Width = 0;
    }

    UpdateRectangleBoundsForStretchMode(m_Stretch, rectBounds);

    return rectBounds;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Modifies a XRECTF based on the shape's stretch mode. Used by
//      CRectangle and CEllipse.
//
//------------------------------------------------------------------------
/* static */ void
CShape::UpdateRectangleBoundsForStretchMode(
    DirectUI::Stretch stretchMode,
    _Inout_ XRECTF& rectBounds
    )
{
    switch (stretchMode)
    {
        case DirectUI::Stretch::Fill:
            //no need for this as fill case is already been taken care of
            break;

        case DirectUI::Stretch::Uniform:
            if (rectBounds.Width > rectBounds.Height)
            {
                rectBounds.Width = rectBounds.Height;
            }
            else
            {
                rectBounds.Height = rectBounds.Width;
            }
            break;

        case DirectUI::Stretch::UniformToFill:
            if (rectBounds.Width < rectBounds.Height)
            {
                rectBounds.Width = rectBounds.Height;
            }
            else
            {
                rectBounds.Height = rectBounds.Width;
            }
            break;

        case DirectUI::Stretch::None:
            rectBounds.Height = rectBounds.Width = 0;
            break;

        default:
            // we should never hit this.
            ASSERT(false);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Virtual to clean up render data when the type of render walk changes.
//
//------------------------------------------------------------------------
void
CShape::EnsureContentRenderDataVirtual(
    RenderWalkType oldType,
    RenderWalkType newType
    )
{
    CFrameworkElement::EnsureContentRenderDataVirtual(oldType, newType);

    // Shape data is used by all walks but only updated during rendering
    // by some of them.  It will be re-created on demand if needed for
    // rendering.
    ReleaseInterface(m_pRender);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Overridden implementation of GetBoundsForImageBrushVirtual.  This returns
//      the bounds used to compute the brush transform for the specified image brush.
//      This differentiates between the fill and stroke bounds.  This matches
//      the implementations of ShapeStrokePart and ShapeFillPart.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CShape::GetBoundsForImageBrushVirtual(
    _In_ const CImageBrush *pImageBrush,
    _Out_ XRECTF *pBounds
    )
{
    XRECTF_RB boundsRB;

    if (pImageBrush == m_pFill)
    {
        IFC_RETURN(GetFillBounds(&boundsRB));
    }
    else
    {
        ASSERT(pImageBrush == GetStroke().get());

        IFC_RETURN(GetStrokeBounds(&boundsRB));
    }

    *pBounds = ToXRectF(boundsRB);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Generate bounds for the content of the element.  Content bounds
//      differ from geometry bounds in that the fill and stroke are only
//      included if there are brushes specified for them.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CShape::GenerateContentBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    XRECTF_RB bounds = { };

    auto stroke = GetStroke();
    if (m_pFill != nullptr || stroke)
    {
        XRECTF_RB fillBounds = { };
        XRECTF_RB strokeBounds = { };
        XRECTF_RB combinedBounds = { };

        if (m_pFill != nullptr || stroke)
        {
            // The fact that we're in this function implies this UIElement's cached bounds have been
            // invalidated. Since our render geometry depends on this element's actual size, we need
            // to update it here to ensure the geometry bounds we calculate are up to date.
            IFC_RETURN(UpdateRenderGeometry());
        }

        if (m_pFill != nullptr)
        {
            IFC_RETURN(GetGeometryBounds(&fillBounds));
        }

        if (stroke)
        {
            IFC_RETURN(GetWidenedGeometryBounds(&strokeBounds));
        }

        if (m_Stretch != DirectUI::Stretch::None)
        {
            CMILMatrix stretchTransform;

            IFC_RETURN(GetStretchTransform(&stretchTransform));

            stretchTransform.TransformBounds(&fillBounds, &fillBounds);
            stretchTransform.TransformBounds(&strokeBounds, &strokeBounds);
        }

        m_innerFillBounds = fillBounds;
        m_innerStrokeBounds = strokeBounds;

        combinedBounds = fillBounds;
        UnionRectF(&combinedBounds, &strokeBounds);

        if (m_Stretch == DirectUI::Stretch::UniformToFill)
        {
            // The UniformToFill stretch mode typically causes a shape to render at a smaller size
            // than the natural bounds of its geometry. We need to account for that here by clipping
            // to the layout bounds so hit testing behavior remains in sync with rendering.

            XRECTF_RB layoutBounds = {0.0f, 0.0f, GetActualWidth(), GetActualHeight()};
            IntersectRect(&combinedBounds, &layoutBounds);
        }

        bounds = combinedBounds;
    }
    else
    {
        EmptyRectF(&bounds);
        EmptyRectF(&m_innerFillBounds);
        EmptyRectF(&m_innerStrokeBounds);
    }

    *pBounds = bounds;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Generate bounds for the geometry taking in to account the strech
//      mode and constraining bounds. Called during layout.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CShape::GenerateGeometryBounds(
    _In_ const XSIZEF* pConstraintBounds,
    DirectUI::Stretch stretchMode,
    _Out_ XRECTF_RB* pBounds
    )
{
    XRECTF_RB bounds = { };
    XSIZEF stretchBounds = { };

    if (m_pRender == nullptr)
    {
        IFC_RETURN(UpdateRenderGeometry());
    }

    if (m_pRender != nullptr)
    {
        XRECTF_RB geometryBounds = { };
        XFLOAT strokeThickness = GetAbsStrokeThickness();

        IFC_RETURN(GetGeometryBounds(&geometryBounds));

        IFC_RETURN(ApplyContentStretch(strokeThickness, stretchMode, geometryBounds, *pConstraintBounds, &stretchBounds));

        //
        // NOTE: The stretched bounds only take in to account the positive area of the shape. For stretch mode
        //       none the geometry can extend in to the negative area around the shape. For other stretch modes
        //       the geometry is always shifted so that the top left point of the bounds is rendered at 0,0 in
        //       local space.
        //

        bounds.right = stretchBounds.width;
        bounds.bottom = stretchBounds.height;

        if (stretchMode != DirectUI::Stretch::None)
        {
            bounds.left = 0;
            bounds.top = 0;
        }
        else
        {
            bounds.left = geometryBounds.left - (strokeThickness / 2.0f);
            bounds.top = geometryBounds.top - (strokeThickness / 2.0f);
        }
    }
    else
    {
        EmptyRectF(&bounds);
    }

    *pBounds = bounds;

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the geometry bounds representing the filled shape.
//
//  NOTE:
//      The geometry bounds are cached to prevent expensive geometry
//      bounding for paths.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CShape::GetGeometryBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    // Create geometry if it has not yet been initialized.
    if (m_pRender == nullptr)
    {
        IFC_RETURN(UpdateRenderGeometry());
    }

    if (m_geometryBoundsDirty)
    {
        if (m_pRender != nullptr)
        {
            IFC_RETURN(m_pRender->GetBounds(&m_geometryBounds));
        }
        else
        {
            EmptyRectF(&m_geometryBounds);
        }

        m_geometryBoundsDirty = false;
    }

    *pBounds = m_geometryBounds;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the widened geometry bounds representing the stroked shape.
//
//  NOTE:
//      The geometry bounds are cached to prevent expensive geometry
//      bounding for paths.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CShape::GetWidenedGeometryBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    // Create geometry if it has not yet been initialized.
    if (m_pRender == nullptr)
    {
        IFC_RETURN(UpdateRenderGeometry());
    }

    if (m_geometryWidenedBoundsDirty)
    {
        if (m_pRender != nullptr)
        {
            CPlainPen pen;

            // The widened bounds should only be needed if the shape is actually stroked.
            // InitPenFromShape will inflate the geometry bounds based on the other stroke
            // properties (StrokeThickness, dashing, etc), but it doesn't check whether
            // the stroke exists or not.
            ASSERT(GetStroke());

            IFC_RETURN(InitPenFromShape(&pen));

            IFC_RETURN(m_pRender->GetWidenedBounds(pen, &m_geometryWidenedBounds));
        }
        else
        {
            EmptyRectF(&m_geometryWidenedBounds);
        }

        m_geometryWidenedBoundsDirty = FALSE;
    }

    *pBounds = m_geometryWidenedBounds;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Invalidate the cached geometry bounds.
//
//------------------------------------------------------------------------
void
CShape::InvalidateGeometryBounds()
{
    m_geometryBoundsDirty = true;
    m_geometryWidenedBoundsDirty = true;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the inner fill bounds of the shape.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CShape::GetFillBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    IFC_RETURN(EnsureInnerBounds(nullptr /*hitTestParams*/));
    *pBounds = m_innerFillBounds;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the inner stroke bounds of the shape.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CShape::GetStrokeBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    IFC_RETURN(EnsureInnerBounds(nullptr /*hitTestParams*/));
    *pBounds = m_innerStrokeBounds;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Apply stretch modes to geometry bounds.
//
//------------------------------------------------------------------------
/* static */ _Check_return_ HRESULT
CShape::ApplyContentStretch(
    XFLOAT strokeThickness,
    DirectUI::Stretch stretchMode,
    _In_ const XRECTF_RB& geometryNaturalBounds,
    _In_ const XSIZEF& renderConstraintBounds,
    _Out_ XSIZEF* pStretchedBounds
    )
{
    HRESULT hr = S_OK;
    XRECTF naturalBounds = { };
    XFLOAT scaleX = 1.0f;
    XFLOAT scaleY = 1.0f;

    naturalBounds.Height = MAX(geometryNaturalBounds.bottom - geometryNaturalBounds.top, 0.0f);
    naturalBounds.Width = MAX(geometryNaturalBounds.right - geometryNaturalBounds.left, 0.0f);

    if (stretchMode == DirectUI::Stretch::None)
    {
        pStretchedBounds->width = geometryNaturalBounds.right + (strokeThickness / 2.0f);
        pStretchedBounds->height = geometryNaturalBounds.bottom + (strokeThickness / 2.0f);
    }
    else
    {
        bool isXBounded = false;
        bool isYBounded = false;

        if (!IsInfiniteF(renderConstraintBounds.height))
        {
            isYBounded = TRUE;
        }

        if (!IsInfiniteF(renderConstraintBounds.width))
        {
            isXBounded = TRUE;
        }

        if (isXBounded && naturalBounds.Width > REAL_EPSILON)
        {
            scaleX = MAX(renderConstraintBounds.width - strokeThickness, 0.0f) / naturalBounds.Width;
        }

        if (isYBounded && naturalBounds.Height > REAL_EPSILON)
        {
            scaleY = MAX(renderConstraintBounds.height - strokeThickness, 0.0f) / naturalBounds.Height;
        }

        if (isXBounded && isYBounded)
        {
            switch (stretchMode)
            {
                case DirectUI::Stretch::Fill:
                    {
                        break;
                    }

                //
                // Stretch to fill the smaller dimension and keep aspect ratio.
                // All of the shape is visible, part of the bounds may not be filled.
                //
                case DirectUI::Stretch::Uniform:
                    {
                        if (scaleX > scaleY)
                        {
                            scaleX = scaleY;
                        }
                        else
                        {
                            scaleY = scaleX;
                        }

                        break;
                    }

                //
                // Stretch to fill the larger dimension and keep aspect ratio.
                // Portions of the shape may be clipped off, all of the bounds is filled.
                //
                case DirectUI::Stretch::UniformToFill:
                    {
                        if (scaleX > scaleY)
                        {
                            scaleY = scaleX;
                        }
                        else
                        {
                            scaleX = scaleY;
                        }

                        break;
                    }

                //
                // Stretch mode none is already accounted for.
                //
                case DirectUI::Stretch::None:
                default:
                    {
                        XCP_FAULT_ON_FAILURE(FALSE);
                        break;
                    }
            }
        }
        else if (stretchMode == DirectUI::Stretch::Uniform || stretchMode == DirectUI::Stretch::UniformToFill)
        {
            if (isXBounded)
            {
                scaleY = scaleX;
            }
            else if (isYBounded)
            {
                scaleX = scaleY;
            }
        }

        pStretchedBounds->width = (naturalBounds.Width * scaleX) + strokeThickness;
        pStretchedBounds->height = (naturalBounds.Height * scaleY) + strokeThickness;
    }

    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if a point intersects with the element in local space.
//
//  NOTE:
//      Overridden in derived classes to provide more detailed hit testing.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CShape::HitTestLocalInternal(
    _In_ const XPOINTF& target,
    _Out_ bool* pHit
    )
{
    return HitTestLocalInternalImpl(target, pHit);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if a polygon intersects with the element in local space.
//
//  NOTE:
//      Overridden in derived classes to provide more detailed hit testing.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CShape::HitTestLocalInternal(
    _In_ const HitTestPolygon& target,
    _Out_ bool* pHit
    )
{
    return HitTestLocalInternalImpl(target, pHit);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if a point/polygon intersects with the element in local space.
//
//  NOTE:
//      Templated to share the implementation for both points and rects.
//
//------------------------------------------------------------------------
template <typename HitType>
_Check_return_ HRESULT
CShape::HitTestLocalInternalImpl(
    _In_ const HitType& target,
    _Out_ bool* pHit
    )
{
    bool hitShape = false;

    if (m_pRender == nullptr)
    {
        IFC_RETURN(UpdateRenderGeometry());
    }

    if (m_pRender != nullptr)
    {
        CMILMatrix stretchTransform;
        CMILMatrix* pTransform = nullptr;

        if (m_Stretch != DirectUI::Stretch::None)
        {
            IFC_RETURN(GetStretchTransform(&stretchTransform));

            pTransform = &stretchTransform;
        }

        //
        // If InvisibleHitTestMode is set and the fill is NULL, hittest this shape as if it
        // has a solid color fill instead.
        //
        if (m_pFill != nullptr || GetContext()->InvisibleHitTestMode())
        {
            IFC_RETURN(m_pRender->HitTestFill(target, pTransform, &hitShape));

            if (hitShape && m_pFill != nullptr)
            {
                XRECTF geometryRenderBounds = ToXRectF(m_geometryBounds);
                IFC_RETURN(m_pFill->HitTestBrushClipInLocalSpace(
                    &geometryRenderBounds,
                    target,
                    &hitShape));
            }
        }

        //
        // Note: we don't do any additional work for InvisibleHitTestMode here - if the stroke brush is null, we treat hit
        // testing as if the thickness is zero.
        //
        auto stroke = GetStroke();
        if (!hitShape && stroke)
        {
            CPlainPen pen;

            IFC_RETURN(InitPenFromShape(&pen));

            IFC_RETURN(m_pRender->HitTestStroke(target, pen, pTransform, &hitShape));

            if (hitShape)
            {
                XRECTF geometryRenderBounds = ToXRectF(m_geometryBounds);
                IFC_RETURN(stroke->HitTestBrushClipInLocalSpace(
                    &geometryRenderBounds,
                    target,
                    &hitShape));
            }
        }
    }

    *pHit = hitShape;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This RENDERCHANGEDPFN marks this Shape's geometry as dirty.
//
//------------------------------------------------------------------------
/* static */ void
CShape::NWSetGeometryDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::Shape>());

    // A shape's geometry cannot change independently.
    ASSERT(!flags_enum::is_set(flags, DirtyFlags::Independent));

    CShape* pShape = static_cast<CShape*>(pTarget);

    // Invalidate the geometry bounds.
    pShape->m_geometryBoundsDirty = true;

    //WUCShapes_TODO: We're going to want a better / more complete / maybe less rendundant dirty tracker
    if (pShape->GetRenderGeometry() != nullptr)
    {
        pShape->GetRenderGeometry()->SetWUCGeometryDirty(true);
    }

    NWSetContentAndBoundsDirty(pShape, DirtyFlags::None);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns independently animated brushes for rendering with PC.
//
//------------------------------------------------------------------------
void
CShape::GetIndependentlyAnimatedBrushes(
    _Outptr_ CSolidColorBrush **ppFillBrush,
    _Outptr_ CSolidColorBrush **ppStrokeBrush
    )
{
    if (m_pFill != nullptr && m_pFill->OfTypeByIndex<KnownTypeIndex::SolidColorBrush>())
    {
        SetInterface(*ppFillBrush, static_cast<CSolidColorBrush *>(m_pFill));
    }

    auto stroke = GetStroke();
    if (stroke && stroke->OfTypeByIndex<KnownTypeIndex::SolidColorBrush>())
    {
        SetInterface(*ppStrokeBrush, static_cast<CSolidColorBrush *>(stroke.get()));
    }
}

//----------------------------------------------------------------------------
//  Synopsis:
//      Determines if a new mask part of the element is needed.  The element
//      will only have a mask if both the brush and texture are non-null
//-----------------------------------------------------------------------------
bool CShape::IsMaskDirty(
    _In_ HWShapeRealization *pHwShapeRealization,
    const bool renderCollapsedMask,
    bool isFillBrushAnimated,
    bool isStrokeBrushAnimated,
    _Inout_ bool* pIsFillForHitTestOnly,
    _Inout_ bool* pIsStrokeForHitTestOnly
    )
{
    return CFrameworkElement::NWIsContentDirty()
        || pHwShapeRealization->MaskNeedsUpdate(
            renderCollapsedMask,
            m_pFill, m_fFillBrushDirty, isFillBrushAnimated, pIsFillForHitTestOnly,
            GetStroke().get(), m_fStrokeBrushDirty, isStrokeBrushAnimated, pIsStrokeForHitTestOnly);

}

void CShape::CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp)
{
    CFrameworkElement::CleanupDeviceRelatedResourcesRecursive(cleanupDComp);

    m_alphaMask.Hide();
}

void CShape::ClearPCRenderData()
{
    ASSERT(IsInPCScene_IncludingDeviceLost());

    __super::ClearPCRenderData();

    // Hide the alpha mask so that the composition surface is released.
    m_alphaMask.Hide();
}

_Check_return_ HRESULT CShape::NotifyRenderContent(
    HWRenderVisibility visibility
    )
{
    if (visibility == HWRenderVisibility::Invisible)
    {
        m_alphaMask.Hide();
    }
    else
    {
        // Update the alpha mask.  This will check if there is one created and only update it if necessary.
        IFC_RETURN(m_alphaMask.UpdateIfAvailable(this));
    }

    return S_OK;
}

_Check_return_ HRESULT CShape::GetAlphaMask(
    _Outptr_ WUComp::ICompositionBrush** ppReturnValue)
{
    IFC_RETURN(m_alphaMask.Ensure(this));
    *ppReturnValue = m_alphaMask.GetCompositionBrush().Detach();

    return S_OK;
}

wrl::ComPtr<WUComp::ICompositionSpriteShape> CShape::InitializeSprite(
    _In_ VisualContentRenderer* renderer)
{
    wrl::ComPtr<WUComp::ICompositionSpriteShape> sprite;
    IFCFAILFAST(renderer->GetCompositor5()->CreateSpriteShape(&sprite));

    return sprite;
}

// Populates the CompositionSpriteShape with the data that every shape has.
// WUCShapes_TODO: stretch attribute
void CShape::PopulateCompositionSpriteShape(
    _In_ VisualContentRenderer* renderer,
    _Inout_ WUComp::ICompositionSpriteShape* sprite)
{
    IFCFAILFAST(sprite->put_StrokeMiterLimit(GetStrokeMiterLimit()));
    IFCFAILFAST(sprite->put_StrokeThickness(GetStrokeThickness()));

    XcpPenCap strokeStartCap = GetStrokeStartLineCap();
    IFCFAILFAST(sprite->put_StrokeStartCap(GetCompCapFromPenCap(strokeStartCap)));

    XcpPenCap strokeEndCap = GetStrokeEndLineCap();
    IFCFAILFAST(sprite->put_StrokeEndCap(GetCompCapFromPenCap(strokeEndCap)));

    XcpLineJoin join = GetStrokeLineJoin();
    IFCFAILFAST(sprite->put_StrokeLineJoin(GetCompLineJoinFromXcpLineJoin(join)));

    XcpPenCap strokeDashCap = GetStrokeDashCap();
    IFCFAILFAST(sprite->put_StrokeDashCap(GetCompCapFromPenCap(strokeDashCap)));

    IFCFAILFAST(sprite->put_StrokeDashOffset(GetStrokeDashOffset()));

    xref_ptr<CDoubleCollection> strokeDashArray = GetStrokeDashArray();
    if (strokeDashArray && strokeDashArray->GetCount() > 0)
    {
        wrl::ComPtr<wfc::IVector<float>> compDashArray;
        IFCFAILFAST(sprite->get_StrokeDashArray(&compDashArray));
        compDashArray->Clear();

        const std::vector<float>& collection = strokeDashArray->GetCollection();

        for (float dashLength : collection)
        {
            compDashArray->Append(dashLength * GetStrokeThickness());
        }
    }

    auto strokeBrush = GetStroke();
    if (strokeBrush)
    {
        auto strokeCompBrush = renderer->GetCompositionBrush(strokeBrush, this);
        IFCFAILFAST(sprite->put_StrokeBrush(strokeCompBrush.Get()));
    }

    if (m_pFill)
    {
        auto fillCompBrush = renderer->GetCompositionBrush(m_pFill, this);
        IFCFAILFAST(sprite->put_FillBrush(fillCompBrush.Get()));
    }
}

_Check_return_ HRESULT CShape::Render(
    _In_ VisualContentRenderer* renderer,
    _Outptr_ WUComp::ICompositionSpriteShape** ppSprite)
{
    //Create a sprite
    wrl::ComPtr<WUComp::ICompositionSpriteShape> sprite = InitializeSprite(renderer);

    // UpdateRenderGeometry guarantees that the geometry object is not null
    IFC_RETURN(UpdateRenderGeometry());
    auto geometry = m_pRender->GetCompositionGeometry(renderer);
    IFC_RETURN(sprite->put_Geometry(geometry));

    PopulateCompositionSpriteShape(renderer, sprite.Get());

    *ppSprite = sprite.Detach();

    return S_OK;
}

WUComp::CompositionStrokeCap GetCompCapFromPenCap(XcpPenCap xcpCap)
{
    switch (xcpCap)
    {
    case XcpPenCapFlat:
        return WUComp::CompositionStrokeCap_Flat;
    case XcpPenCapSquare:
        return WUComp::CompositionStrokeCap_Square;
    case XcpPenCapRound:
        return WUComp::CompositionStrokeCap_Round;
    case XcpPenCapTriangle:
        return WUComp::CompositionStrokeCap_Triangle;
    default:
        IFCFAILFAST(E_FAIL);
    }
    return WUComp::CompositionStrokeCap_Flat;
}

WUComp::CompositionStrokeLineJoin GetCompLineJoinFromXcpLineJoin(XcpLineJoin xcpLineJoin)
{
    switch (xcpLineJoin)
    {
    case XcpLineJoinMiter:
    case XcpLineJoinMiterClipped:
        return WUComp::CompositionStrokeLineJoin_Miter;
    case XcpLineJoinBevel:
        return WUComp::CompositionStrokeLineJoin_Bevel;
    case XcpLineJoinRound:
        return WUComp::CompositionStrokeLineJoin_Round;
    default:
        IFCFAILFAST(E_FAIL);
    }
    return WUComp::CompositionStrokeLineJoin_Miter;
}
