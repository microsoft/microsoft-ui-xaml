// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "InputServices.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------
CParametricCurveSegment::~CParametricCurveSegment()
{
}

//------------------------------------------------------------------------
//
//    Synopsis:
//        Populates a ParametricCurveSegmentDefinition object with this object's data.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CParametricCurveSegment::GetParametricCurveSegmentDefinition(
    _Inout_ CParametricCurveSegmentDefinition *pDefinition)
{
    HRESULT hr = S_OK;

    pDefinition->m_beginOffset = m_beginOffset;
    pDefinition->m_constantCoefficient = m_constantCoefficient;
    pDefinition->m_linearCoefficient = m_linearCoefficient;
    pDefinition->m_quadraticCoefficient = m_quadraticCoefficient;
    pDefinition->m_cubicCoefficient = m_cubicCoefficient;

//Cleanup:
    RRETURN(hr); // RRETURN_REMOVAL
}

void
CParametricCurveSegmentCollection::SortCollection()
{
// With the change from block/flatten to vectors the item list is always
// available as a flattened array so we can just use it. The cast here
// is ugly but currently we don't support "parametric curve" collections
// just DO collections. And rather that doing the expensive derived type
// covert we just know these are parametric curves.

    auto& curveSegments = GetCollection();

// TODO: A bubble sort? Are these curves trivial (5-7 segments) or hundreds of segments?

    if (!m_bIsSorted)
    {
        XUINT32 min = 0;
        for (XUINT32 i = 0; i < GetCount(); i++)
        {
            min = i;
            for (XUINT32 j = i + 1; j < GetCount(); j++)
            {
                // Sort on the source values
                XFLOAT leftValue = static_cast<CParametricCurveSegment*>(curveSegments[min])->m_beginOffset;
                XFLOAT rightValue = static_cast<CParametricCurveSegment*>(curveSegments[j])->m_beginOffset;

                if (leftValue > rightValue)
                {
                    min = j;
                }
            }
            if (min != i)
            {
                swap(min, i);
            }
        }

        m_bIsSorted = TRUE;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------
CParametricCurve::~CParametricCurve()
{
    ReleaseInterface(m_pCurveSegments);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates an instance of a the object
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CParametricCurve::Create(
    _Outptr_result_maybenull_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
    )
{
    HRESULT hr = S_OK;
    CParametricCurve *pParametricCurve = nullptr;
    CParametricCurveSegmentCollection *pCurveSegments = nullptr;
    *ppObject = nullptr;

    // Create type
    pParametricCurve = new CParametricCurve(pCreate->m_pCore);

    // Create internal members
    IFC(CParametricCurveSegmentCollection::Create(reinterpret_cast<CDependencyObject **>(&pCurveSegments), pCreate));
    IFC(pParametricCurve->SetValueByKnownIndex(KnownPropertyIndex::ParametricCurve_CurveSegments, pCurveSegments));

    //do base-class initialization and return new object
    IFC(ValidateAndInit(pParametricCurve, ppObject));

    // Cleanup
    pParametricCurve = NULL;

Cleanup:
    ReleaseInterface(pCurveSegments);
    ReleaseInterface(pParametricCurve);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the primary content property.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CParametricCurve::SetPrimaryContentProperty(
    _In_ const xstring_ptr_view& strPrimaryContentProperty)
{
    IFC_RETURN(strPrimaryContentProperty.Promote(&m_strPrimaryContentPropertyName));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the secondary content property.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CParametricCurve::SetSecondaryContentProperty(
    _In_ DirectUI::DirectManipulationProperty secondaryContentProperty,
    _In_ const xstring_ptr_view& strSecondaryContentProperty)
{
    m_secondaryContentProperty = (XDMProperty)secondaryContentProperty;
    IFC_RETURN(strSecondaryContentProperty.Promote(&m_strAssociatedDependencyPropertyName));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Resolves property indexes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CParametricCurve::ResolveProperties(
    _In_ CDependencyObject *pPrimaryContent,
    _In_ CDependencyObject *pDependencyPropertyHolder)
{
    auto core = GetContext();

    IFC_RETURN(core->ParsePropertyPath(
            &pPrimaryContent,
            &m_pPrimaryContentProperty,
            m_strPrimaryContentPropertyName));

    IFC_RETURN(core->ParsePropertyPath(
            &pDependencyPropertyHolder,
            &m_pAssociatedDependencyProperty,
            m_strAssociatedDependencyPropertyName));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the associated dependency property.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CParametricCurve::UpdateDependencyProperty(
    _In_ CDependencyObject *pPrimaryContent,
    _In_ CDependencyObject *pDependencyPropertyHolder)
{
    DOUBLE doubleValue;
    FLOAT floatValue;
    CValue value;

    IFC_RETURN(pPrimaryContent->GetValueByIndex(m_pPrimaryContentProperty->GetIndex(), &value));

    IFC_RETURN(value.GetDouble(doubleValue));
    floatValue = static_cast<FLOAT>(doubleValue);
    IFC_RETURN(EvaluateCurve(floatValue, &floatValue));

    // DManip transform values need to be negated when used for dependency property values.
    // For example, if a sticky header needs to be moved down 100 pixels,
    // the DManip value will be -100, while the dependency property (in this case,
    // the TranslateY property on a CompositeTransform) needs to be 100.
    value.SetFloat(floatValue * (m_secondaryContentProperty == XcpDMPropertyTranslationX || m_secondaryContentProperty == XcpDMPropertyTranslationY ? -1 : 1));

    IFC_RETURN(pDependencyPropertyHolder->SetValueByIndex(m_pAssociatedDependencyProperty->GetIndex(), value));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Evaluates a curve based on a given input value.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CParametricCurve::EvaluateCurve(
    _In_ XFLOAT input,
    _Out_ XFLOAT *pOutput)
{
    IFCPTR_RETURN(pOutput);

    switch (m_secondaryContentProperty)
    {
    case XcpDMPropertyTranslationX:
    case XcpDMPropertyTranslationY:
        *pOutput = 0;
        break;

    case XcpDMPropertyZoom:
        *pOutput = 1;
        break;

    default:
        *pOutput = 0;
        ASSERT(FALSE, "Invalid secondary content property");
        break;
    }

    if (m_pCurveSegments->GetCount() > 0)
    {
        m_pCurveSegments->SortCollection();
        auto& curveSegments = m_pCurveSegments->GetCollection();

        XUINT32 cCurveSegmentToUse = 0;
        for ( ; cCurveSegmentToUse + 1 < curveSegments.size(); cCurveSegmentToUse++)
        {
            // Go until the input is sandwiched between two begin offsets - that's the segment to use.
            if (input >= static_cast<CParametricCurveSegment*>(curveSegments[cCurveSegmentToUse])->m_beginOffset &&
                input < static_cast<CParametricCurveSegment*>(curveSegments[cCurveSegmentToUse + 1])->m_beginOffset)
            {
                break;
            }
        }

        CParametricCurveSegment *pCurveSegmentToUse = static_cast<CParametricCurveSegment*>(curveSegments[cCurveSegmentToUse]);

        // We need to first subtract off the begin offset - the input value in each curve segment
        // is expected to begin at zero.
        input -= pCurveSegmentToUse->m_beginOffset;

        XFLOAT inputSquared = input * input;

        *pOutput =
            pCurveSegmentToUse->m_constantCoefficient +
            input * pCurveSegmentToUse->m_linearCoefficient +
            inputSquared * pCurveSegmentToUse->m_quadraticCoefficient +
            input * inputSquared * pCurveSegmentToUse->m_cubicCoefficient;
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//    Synopsis:
//        Populates a ParametricCurveDefinition object with this object's data.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CParametricCurve::GetParametricCurveDefinition(
    _In_ CUIElement *pPrimaryContent,
    _Inout_ CParametricCurveDefinition *pDefinition)
{
    HRESULT hr = S_OK;
    CDependencyObject *pDManipElement = nullptr;
    XDMProperty primaryDMProperty = XcpDMPropertyNone;

    IFC(FxCallbacks::UIElement_GetDManipElementAndProperty(
        pPrimaryContent,
        m_pPrimaryContentProperty->GetIndex(),
        &pDManipElement,
        reinterpret_cast<XUINT32 *>(&primaryDMProperty)));

    pDefinition->m_primaryDMProperty = primaryDMProperty;
    pDefinition->m_secondaryDMProperty = m_secondaryContentProperty;

    ASSERT(!pDefinition->m_pSegments);
    delete[] pDefinition->m_pSegments;
    pDefinition->m_segments = 0;

    if (m_pCurveSegments->GetCount() > 0)
    {
        m_pCurveSegments->SortCollection();
        auto& curveSegments = m_pCurveSegments->GetCollection();
        pDefinition->m_segments = static_cast<XUINT32>(curveSegments.size());
        pDefinition->m_pSegments = new CParametricCurveSegmentDefinition[pDefinition->m_segments];

        for (XUINT32 i = 0; i < pDefinition->m_segments; i++)
        {
            IFC(static_cast<CParametricCurveSegment*>(curveSegments[i])->GetParametricCurveSegmentDefinition(&pDefinition->m_pSegments[i]));
        }
    }
    else
    {
        pDefinition->m_pSegments = nullptr;
        pDefinition->m_segments = 0;
    }

Cleanup:
    ReleaseInterface(pDManipElement);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------
CSecondaryContentRelationship::~CSecondaryContentRelationship()
{
    // Release the tracked reference to the primary content if we have one.
    VERIFYHR(ReleaseRefOnPrimaryContent());

    ReleaseInterface(m_pCurves);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates an instance of a the object
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSecondaryContentRelationship::Create(
    _Outptr_result_maybenull_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
    )
{
    HRESULT hr = S_OK;
    CSecondaryContentRelationship *pSecondaryContentRelationship = nullptr;
    CParametricCurveCollection *pCurves = nullptr;
    *ppObject = nullptr;

    // Create type
    pSecondaryContentRelationship = new CSecondaryContentRelationship(pCreate->m_pCore);

    // Create internal members
    IFC(CParametricCurveCollection::Create(reinterpret_cast<CDependencyObject **>(&pCurves), pCreate));
    IFC(pSecondaryContentRelationship->SetValueByKnownIndex(KnownPropertyIndex::SecondaryContentRelationship_Curves, pCurves));

    //do base-class initialization and return new object
    IFC(ValidateAndInit(pSecondaryContentRelationship, ppObject));

    // Cleanup
    pSecondaryContentRelationship = NULL;

Cleanup:
    ReleaseInterface(pCurves);
    ReleaseInterface(pSecondaryContentRelationship);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the primary content.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSecondaryContentRelationship::SetPrimaryContent(
_In_ CUIElement* pPrimaryContent)
{
    m_primaryContentWeakRef = xref::get_weakref(pPrimaryContent);
    return S_OK; // RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the secondary content.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSecondaryContentRelationship::SetSecondaryContent(
_In_ CUIElement* pSecondaryContent,
    _In_ CDependencyObject* pDependencyPropertyHolder)
{
    m_secondaryContentWeakRef = xref::get_weakref(pSecondaryContent);
    m_dependencyPropertyHolderWeakRef = xref::get_weakref(pDependencyPropertyHolder);
    return S_OK; // RRETURN_REMOVAL
}

// Sets an "auxiliary" dependency property.  This property basically shadows the main dependency property
// associated with secondary content.  It is currently only used to keep the sticky header render transform
// in sync with its LayoutTransitionElement's render transform.
_Check_return_ HRESULT
CSecondaryContentRelationship::SetAuxiliaryDependencyPropertyHolder(_In_ CDependencyObject* pAuxiliaryDependencyPropertyHolder)
{
    m_auxiliaryDependencyPropertyHolderWeakRef = xref::get_weakref(pAuxiliaryDependencyPropertyHolder);
    return S_OK; // RRETURN_REMOVAL
}

// Helper method to begin the process of synchronizing a change to the secondary curve.
// See more details on deferred DM Release in CInputServices::PrepareSecondaryContentRelationshipForCurveUpdate().
_Check_return_ HRESULT
CSecondaryContentRelationship::PrepareForCurveUpdate()
{
    return GetContext()->GetInputServices()->PrepareSecondaryContentRelationshipForCurveUpdate(this);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Applies the relationship in the input manager.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSecondaryContentRelationship::Apply()
{
    xref_ptr<CParametricCurve> spCurve;
    IFC_RETURN(EnsureRefOnPrimaryContent());

    // First we need to resolve all of the properties in the curve collection.
    for (XUINT32 i = 0; i < m_pCurves->GetCount(); i++)
    {
        spCurve.attach(do_pointer_cast<CParametricCurve>(m_pCurves->GetItemDOWithAddRef(i)));
        if (spCurve)
        {
            IFC_RETURN(spCurve->ResolveProperties(GetPrimaryContent(), GetDependencyPropertyHolder()));
        }
    }

    // We'll first attempt to apply all of the content relationships in the queue, in order to ensure
    // that relationships are always applied in order, and will then apply this relationship.
    auto mainInputManager = GetContext()->GetInputServices();
    IFC_RETURN(mainInputManager->ApplySecondaryContentRelationships());
    IFC_RETURN(mainInputManager->ApplySecondaryContentRelationship(this));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Applies the relationship in the input manager.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSecondaryContentRelationship::Remove()
{
    IFC_RETURN(GetContext()->GetInputServices()->RemoveSecondaryContentRelationship(this));

    // Release the tracked reference to the primary content if we have one.
    IFC_RETURN(ReleaseRefOnPrimaryContent());

    return S_OK;
}

//------------------------------------------------------------------------
//
//    Synopsis:
//        Updates the associated dependency properties involved in this relationship.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSecondaryContentRelationship::UpdateDependencyProperties(_In_ bool bManipulationCompleting)
{
    UNREFERENCED_PARAMETER(bManipulationCompleting);

    HRESULT hr = S_OK;
    CParametricCurve *pCurve = nullptr;

    // With DManip-on-DComp, there is some special case handling of when to actually apply
    // parametric curves to DP's:
    // The "main" dependency property held in m_pDependencyPropertyHolderWeakRef is controlled exclusively by DManip
    // and we must avoid updating it otherwise we'll end up double-applying this transform.  A couple things worth noting here:
    // 1) The CompNode updating code has the smarts to insert a shared DComp Transform driven directly by DManip.  This Transform
    //    stays set permanently on the DComp visual.
    // 2) The UI thread can retrieve this transform directly from DManip via CUIElement::GetLocalTransform() - it has the smarts to
    // recognize the element is manipulated and can get this transform via CInputServices::GetDirectManipulationCompositorTransform().
    //
    // There are two exceptional cases where we still update the DP's even in DManip-on-DComp mode:
    // 1) If this relationship is targeting a clip, the DP is the clip transform.  Two things of interest in this case:
    //    a) The CompNode updating code has the smarts to insert a shared DComp Transform so the output side does not need this DP, however...
    //    b) The UI thread still needs to know about the clip transform to perform proper culling for various purposes (eg hit-testing).
    //       Unfortunately this clip transform cannot be easily known as being driven by DManip - GetLocalTransform is not responsible for
    //       retrieving this!  What is more, there are many places that directly access the clip and use the clip transform.  Any such place
    //       that uses the clip transform would have to be made smarter to get this transform from DManip, this would be a lot of code churn
    //       and performance impact so it's a lot simpler (at least for now) to go ahead and set the clip transform DP.
    //
    // 2) Sticky headers follow an unusual pattern that needs special handling.  Because of the need for the sticky header to escape clipping,
    //    a LayoutTransitionElement is used.  The LTE's render transform is driven by DManip, not the ListViewItem it is targeting.  However the header item
    //    also needs its render transform to reflect the sticky header curve to keep things like TransformToVisual and hit-testing working.
    //    Rather than have DManip drive both the ListViewItem and the LTE (which would require putting the ListViewItem in yet another CompNode),
    //    it is simpler to just apply the sticky header curve to the ListViewItem's render transform.  Note importantly that this render transform
    //    is ignored by the render walk, due to the way LTE's work - they compute a redirection transform that skips over the render transform of
    //    the target (see CUIElement::GetRedirectionTransformsAndParentCompNode()).  So this render transform is only updated for "hit-testing" type scenarios.
    //
    if (m_shouldTargetClip)
    {
        TraceUpdateDependencyPropertiesForSCRInfo();
        for (XUINT32 i = 0; i < m_pCurves->GetCount(); i++)
        {
            pCurve = do_pointer_cast<CParametricCurve>(m_pCurves->GetItemDOWithAddRef(i));
            if (pCurve)
            {
                IFC(pCurve->UpdateDependencyProperty(GetPrimaryContent(), GetDependencyPropertyHolder()));
            }
            ReleaseInterface(pCurve);
        }
    }

    CDependencyObject* pAuxiliaryDependencyPropertyHolder = GetAuxiliaryDependencyPropertyHolder();
    if (pAuxiliaryDependencyPropertyHolder != nullptr)
    {
        for (XUINT32 i = 0; i < m_pCurves->GetCount(); i++)
        {
            pCurve = do_pointer_cast<CParametricCurve>(m_pCurves->GetItemDOWithAddRef(i));
            if (pCurve)
            {
                IFC(pCurve->UpdateDependencyProperty(GetPrimaryContent(), pAuxiliaryDependencyPropertyHolder));
            }
            ReleaseInterface(pCurve);
        }
    }

Cleanup:
    ReleaseInterface(pCurve);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the parametric curves converted to CParametricCurveDefinition objects.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSecondaryContentRelationship::GetParametricCurveDefinitions(
    _Out_ XUINT32 *pcDefinitions,
    _Outptr_result_buffer_maybenull_(*pcDefinitions) CParametricCurveDefinition **ppDefinitions)
{
    HRESULT hr = S_OK;
    CParametricCurve *pCurve = nullptr;
    *ppDefinitions = nullptr;
    *pcDefinitions = 0;

    XUINT32 cDefinitions = m_pCurves->GetCount();
    CParametricCurveDefinition *pDefinitions = new CParametricCurveDefinition[cDefinitions];

    // First we need to resolve all of the properties in the curve collection.
    for (XUINT32 i = 0; i < cDefinitions; i++)
    {
        pCurve = do_pointer_cast<CParametricCurve>(m_pCurves->GetItemDOWithAddRef(i));
        IFC(pCurve->GetParametricCurveDefinition(m_primaryContentWeakRef.lock(), &pDefinitions[i]));
        ReleaseInterface(pCurve);
    }

    *pcDefinitions = cDefinitions;
    *ppDefinitions = pDefinitions;

Cleanup:
    ReleaseInterface(pCurve);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the primary content, if it exists.
//
//------------------------------------------------------------------------
xref_ptr<CUIElement>
CSecondaryContentRelationship::GetPrimaryContent() const
{
    return m_primaryContentWeakRef.lock();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the secondary content, if it exists.
//
//------------------------------------------------------------------------
xref_ptr<CUIElement>
CSecondaryContentRelationship::GetSecondaryContent() const
{
    return m_secondaryContentWeakRef.lock();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the dependency property holder, if it exists.
//
//------------------------------------------------------------------------
xref_ptr<CDependencyObject>
CSecondaryContentRelationship::GetDependencyPropertyHolder() const
{
    return m_dependencyPropertyHolderWeakRef.lock();
}

// Return the "auxiliary" DP (see notes in UpdateDependencyProperties() for its purpose)
xref_ptr<CDependencyObject>
CSecondaryContentRelationship::GetAuxiliaryDependencyPropertyHolder() const
{
    return m_auxiliaryDependencyPropertyHolderWeakRef.lock();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds a tracked reference on the primary content.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CSecondaryContentRelationship::EnsureRefOnPrimaryContent()
{
    if (!m_holdingTrackedRefOnPrimaryContent)
    {
        CDependencyObject* pPrimaryContent = GetPrimaryContent();

        // We need to hold a reference on the primary content for as long the secondary content relationship is applied,
        // since it needs to use the managed peer in order to get the manipulated element for DManip (e.g., a ScrollViewer's content).
        if (pPrimaryContent != nullptr)
        {
            IFC_RETURN(AddPeerReferenceToItem(pPrimaryContent));
            m_holdingTrackedRefOnPrimaryContent = true;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Releases a tracked reference on the primary content.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CSecondaryContentRelationship::ReleaseRefOnPrimaryContent()
{
    if (m_holdingTrackedRefOnPrimaryContent)
    {
        CDependencyObject* pPrimaryContent = GetPrimaryContent();

        // Could be null if the managed peer got disconnected first.
        if (pPrimaryContent != nullptr)
        {
            IFC_RETURN(RemovePeerReferenceToItem(pPrimaryContent));
        }

        m_holdingTrackedRefOnPrimaryContent = false;
    }

    return S_OK;
}


