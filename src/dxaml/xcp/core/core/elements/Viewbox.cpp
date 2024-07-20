// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Method:   CViewbox::InitInstance
//
//  Synopsis:
//      Initialize internal members
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CViewbox::InitInstance()
{
    CREATEPARAMETERS cp(GetContext());

    IFC_RETURN(CBorder::Create((CDependencyObject**)(&m_pContainerVisual), &cp));
    IFC_RETURN(CScaleTransform::Create((CDependencyObject**)(&m_pScaleTransform), &cp));

    IFC_RETURN(m_pContainerVisual->SetValueByKnownIndex(KnownPropertyIndex::UIElement_RenderTransform, m_pScaleTransform));

    IFC_RETURN(AddChild(m_pContainerVisual));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CViewbox destructor
//
//  Synopsis:
//
//------------------------------------------------------------------------

CViewbox::~CViewbox()
{
    CUIElement* pExistingLogicalChild = NULL;

    VERIFYHR(GetChild(&pExistingLogicalChild));
    if (NULL != pExistingLogicalChild)
    {
        RemoveLogicalChild(pExistingLogicalChild);
    }

    ReleaseInterface(pExistingLogicalChild);
    ReleaseInterface(m_pScaleTransform);
    ReleaseInterface(m_pContainerVisual);
}

//-------------------------------------------------------------------------
//
//  Synopsis:   This is overridden so that we can return an error if the
//              parser adds more than one child to the Viewbox.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CViewbox::AddChild(_In_ CUIElement * pChild)
{
    CUIElement* pExistingChild = NULL;

    IFCEXPECT_RETURN(pChild != NULL);

    // Can only have one child!
    pExistingChild = GetFirstChildNoAddRef();
    IFCEXPECT_RETURN(pExistingChild == NULL);

    IFC_RETURN(CFrameworkElement::AddChild(pChild));

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CViewbox::ComputeScaleFactor()
//
//  Synopsis:   Compute the scale factor of the Child content.
//
//-------------------------------------------------------------------------
_Check_return_
XSIZEF
CViewbox::ComputeScaleFactor(XSIZEF availableSize, XSIZEF contentSize)
{
    XSIZEF desiredSize;
    XFLOAT scaleX = 1.0;
    XFLOAT scaleY = 1.0;

    XUINT32 isConstrainedWidth = !IsInfiniteF(availableSize.width);
    XUINT32 isConstrainedHeight = !IsInfiniteF(availableSize.height);

    // Don't scale if we shouldn't stretch or the scaleX and scaleY are both infinity.
    if ((m_stretch != DirectUI::Stretch::None) && (isConstrainedWidth || isConstrainedHeight))
    {
        // Compute the individual scaleX and scaleY scale factors
        scaleX = IsCloseReal(contentSize.width, 0.0f) ? 0.0f : (availableSize.width / contentSize.width);
        scaleY = IsCloseReal(contentSize.height, 0.0f) ? 0.0f : (availableSize.height / contentSize.height);

        // Make the scale factors uniform by setting them both equal to
        // the larger or smaller (depending on infinite lengths and the
        // Stretch value)
        if (!isConstrainedWidth)
        {
            scaleX = scaleY;
        }
        else if (!isConstrainedHeight)
        {
            scaleY = scaleX;
        }
        else
        {
            switch (m_stretch)
            {
            case DirectUI::Stretch::Uniform:
                // Use the smaller factor for both
                scaleX = scaleY = MIN(scaleX, scaleY);
                break;
            case DirectUI::Stretch::UniformToFill:
                // Use the larger factor for both
                scaleX = scaleY = MAX(scaleX, scaleY);
                break;
            case DirectUI::Stretch::Fill:
            default:
                break;
            }
        }

        // Prevent scaling in an undesired direction
        switch (m_stretchDirection)
        {
        case DirectUI::StretchDirection::UpOnly:
            scaleX = MAX(1.0f, scaleX);
            scaleY = MAX(1.0f, scaleY);
            break;
        case DirectUI::StretchDirection::DownOnly:
            scaleX = MIN(1.0f, scaleX);
            scaleY = MIN(1.0f, scaleY);
            break;
        case DirectUI::StretchDirection::Both:
        default:
            break;
        }
    }

    desiredSize.width = scaleX;
    desiredSize.height = scaleY;

    return desiredSize;
}


//-------------------------------------------------------------------------
//
//  Function:   CViewbox::Child()
//
//  Synopsis:   This is the child property getter and setter method. Note
//              that the storage for this property is actually this children
//              collection.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CViewbox::Child(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult)
{
    HRESULT hr = S_OK;
    CViewbox* pViewbox = NULL;

    IFC(DoPointerCast(pViewbox, pObject));

    if (cArgs == 0)
    {
        // Getting the child
        CUIElement* pChild = NULL;
        hr = pViewbox->GetChild(&pChild);
        if (SUCCEEDED(hr))
        {
            pResult->SetObjectNoRef(pChild);
        }
        else
        {
            pResult->SetNull();
            IFC(hr);
        }
    }
    else if (cArgs == 1 && ppArgs->GetType() == valueObject)
    {
        // Setting the child
        CUIElement* pChild;
        IFC(DoPointerCast(pChild, ppArgs->AsObject()));
        IFC(pViewbox->SetChild(pChild));
    }
    else if (cArgs == 1 && ppArgs->GetType() == valueNull)
    {
        IFC(pViewbox->SetChild(NULL));
    }
    else
    {
        IFC(E_INVALIDARG);
    }
Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CViewbox::SetChild()
//
//  Synopsis:   Remove any existing child and set the new child tree.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CViewbox::SetChild(_In_opt_ CUIElement* pChild)
{
    HRESULT hr = S_OK;
    CUIElement* pExistingLogicalChild = NULL;

    IFC(GetChild(&pExistingLogicalChild));

    if (NULL != pExistingLogicalChild)
    {
        RemoveLogicalChild(pExistingLogicalChild);
    }

    IFC(m_pContainerVisual->SetChild(pChild));

    if (NULL != pChild)
    {
        IFC(AddLogicalChild(pChild));
    }
Cleanup:
    ReleaseInterface(pExistingLogicalChild);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CViewbox::GetChild()
//
//  Synopsis:   This will return the first (and only) child, or NULL if
//              the there is no Child yet.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CViewbox::GetChild(_Outptr_result_maybenull_ CUIElement** ppChild)
{
    IFCPTR_RETURN(ppChild);

    IFC_RETURN(m_pContainerVisual->GetChild(ppChild));
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CViewbox::MeasureOverride
//
//  Synopsis: Returns the desired size for layout purposes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CViewbox::MeasureOverride(
    XSIZEF availableSize,
    XSIZEF& desiredSize)
{
    XSIZEF childDesiredSize, scale, infiniteSize;

    IFCEXPECT_RETURN(m_pContainerVisual);

    infiniteSize.width = XFLOAT_INF;
    infiniteSize.height = XFLOAT_INF;

    IFC_RETURN(m_pContainerVisual->Measure(infiniteSize));
    IFC_RETURN(m_pContainerVisual->EnsureLayoutStorage());

    // Desired size would be my child's desired size plus the border
    childDesiredSize.width   = m_pContainerVisual->DesiredSize.width;
    childDesiredSize.height  = m_pContainerVisual->DesiredSize.height;

    scale = ComputeScaleFactor(availableSize, childDesiredSize);
    IFCEXPECT_ASSERT_RETURN(!IsInfiniteF(scale.width));
    IFCEXPECT_ASSERT_RETURN(!IsInfiniteF(scale.height));

    desiredSize.width = scale.width * childDesiredSize.width;
    desiredSize.height = scale.height * childDesiredSize.height;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CViewbox::ArrangeOverride
//
//  Synopsis: Returns the final render size for layout purposes.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CViewbox::ArrangeOverride(
    XSIZEF finalSize,
    XSIZEF& newFinalSize)
{
    HRESULT hr = S_OK;
    XSIZEF desiredSize, scale;
    XRECTF originalPosition;
    CValue val;

    IFCEXPECT(m_pContainerVisual);

    // Determine the scale factor given the final size
    IFC(m_pContainerVisual->EnsureLayoutStorage());
    desiredSize = m_pContainerVisual->DesiredSize;
    scale = ComputeScaleFactor(finalSize, desiredSize);

    // Scale the ChildElement by the necessary factor
    val.SetFloat(scale.width);
    IFC(m_pScaleTransform->SetValueByKnownIndex(KnownPropertyIndex::ScaleTransform_ScaleX, val));

    val.SetFloat(scale.height);
    IFC(m_pScaleTransform->SetValueByKnownIndex(KnownPropertyIndex::ScaleTransform_ScaleY, val));

    originalPosition.X = 0;
    originalPosition.Y = 0;
    originalPosition.Width = desiredSize.width;
    originalPosition.Height = desiredSize.height;

    // Position the Child to fill the Viewbox
    IFC(m_pContainerVisual->Arrange(originalPosition));

    finalSize.width = scale.width * desiredSize.width;
    finalSize.height = scale.height * desiredSize.height;

Cleanup:
    newFinalSize = finalSize;
    RRETURN(hr);
}
