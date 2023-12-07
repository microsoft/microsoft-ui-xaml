// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Method:   dtor
//
//  Synopsis:
//      Destructor for StackPanel object
//
//------------------------------------------------------------------------

CStackPanel::~CStackPanel()
{
    delete [] m_pIrregularSnapPointKeys;
}

_Check_return_
HRESULT
CStackPanel::MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize)
{
    XSIZEF stackDesiredSize = { };
    const XSIZEF combinedThickness = CBorder::HelperGetCombinedThickness(this);
    XSIZEF childConstraint = (m_orientation == DirectUI::Orientation::Vertical)
        ? XSIZEF{ availableSize.width - combinedThickness.width, XFLOAT_INF }
        : XSIZEF{ XFLOAT_INF, availableSize.height - combinedThickness.height };

    auto children = GetUnsortedChildren();
    UINT32 childrenCount = children.GetCount();

    for (XUINT32 childIndex = 0; childIndex < childrenCount; childIndex++)
    {
        CUIElement* currentChild = children[childIndex];
        ASSERT(currentChild);

        IFC_RETURN(currentChild->Measure(childConstraint));
        IFC_RETURN(currentChild->EnsureLayoutStorage());

        if (m_orientation == DirectUI::Orientation::Vertical)
        {
            stackDesiredSize.width = std::max(stackDesiredSize.width, currentChild->GetLayoutStorage()->m_desiredSize.width);
            stackDesiredSize.height += currentChild->GetLayoutStorage()->m_desiredSize.height;
        }
        else
        {
            stackDesiredSize.width += currentChild->GetLayoutStorage()->m_desiredSize.width;
            stackDesiredSize.height = std::max(stackDesiredSize.height, currentChild->GetLayoutStorage()->m_desiredSize.height);
        }
    }

    stackDesiredSize.width += combinedThickness.width;
    stackDesiredSize.height += combinedThickness.height;

    if (childrenCount > 1)
    {
        const float combinedSpacing = GetSpacing() * (childrenCount - 1);
        if (m_orientation == DirectUI::Orientation::Vertical)
        {
            stackDesiredSize.height += combinedSpacing;
        }
        else
        {
            stackDesiredSize.width += combinedSpacing;
        }
    }

    desiredSize = stackDesiredSize;

    return S_OK;
}

_Check_return_
HRESULT
CStackPanel::ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize)
{
    newFinalSize = finalSize;

    XRECTF arrangeRect = CBorder::HelperGetInnerRect(this, finalSize);
    const float spacing = GetSpacing();

    auto children = GetUnsortedChildren();
    UINT32 childrenCount = children.GetCount();

    for (XUINT32 childIndex = 0; childIndex < childrenCount; childIndex++)
    {
        CUIElement* currentChild = children[childIndex];
        ASSERT(currentChild);

        IFC_RETURN(currentChild->EnsureLayoutStorage());

        if (m_orientation == DirectUI::Orientation::Vertical)
        {
            arrangeRect.Height = currentChild->GetLayoutStorage()->m_desiredSize.height;
            arrangeRect.Width = std::max(arrangeRect.Width, currentChild->GetLayoutStorage()->m_desiredSize.width);
        }
        else
        {
            arrangeRect.Width = currentChild->GetLayoutStorage()->m_desiredSize.width;
            arrangeRect.Height = std::max(arrangeRect.Height, currentChild->GetLayoutStorage()->m_desiredSize.height);
        }

        IFC_RETURN(currentChild->Arrange(arrangeRect));

        // Offset the rect for the next child.
        if (m_orientation == DirectUI::Orientation::Vertical)
        {
            arrangeRect.Y += arrangeRect.Height + spacing;
        }
        else
        {
            arrangeRect.X += arrangeRect.Width + spacing;
        }
    }

    // UIElement_NotifySnapPointsChanged might have to be called because of
    // snap point updates.
    IFC_RETURN(NotifySnapPointsChanges(children));

    return S_OK;
}

// DirectManipulation-specific implementations

//-------------------------------------------------------------------------
//
//  Function:   CStackPanel::GetIrregularSnapPoints
//
//  Synopsis:
//    Used to retrieve an array of irregular snap points.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CStackPanel::GetIrregularSnapPoints(
    _In_ bool bHorizontalOrientation,  // True when horizontal snap points are requested.
    _In_ bool bNearAligned,            // True when requested snap points will align to the left/top of the children
    _In_ bool bFarAligned,             // True when requested snap points will align to the right/bottom of the children
    _Outptr_opt_result_buffer_(*pcSnapPoints) XFLOAT** ppSnapPoints,   // Placeholder for returned array
    _Out_ XUINT32* pcSnapPoints)                                   // Number of snap points returned
{
    HRESULT hr = S_OK;
    bool bIsFirstChild = true;
    XFLOAT* pSnapPoints = NULL;
    XFLOAT* pSnapPointKeys = NULL;
    XFLOAT childDim = 0.0;
    XFLOAT cumulatedDim = 0.0;
    XFLOAT lowerMarginSnapPointKey = 0.0;
    XFLOAT upperMarginSnapPointKey = 0.0;
    XUINT32 cSnapPoints = 0;
    uint8_t cSnapPointKeys = 0;

    IFCEXPECT(ppSnapPoints);
    *ppSnapPoints = NULL;
    IFCEXPECT(pcSnapPoints);
    *pcSnapPoints = 0;

    if ((m_orientation == DirectUI::Orientation::Vertical && !bHorizontalOrientation) ||
        (m_orientation == DirectUI::Orientation::Horizontal && bHorizontalOrientation))
    {
        if (m_bAreScrollSnapPointsRegular)
        {
            // Accessing the irregular snap points while AreScrollSnapPointsRegular is True is not supported.
            IFC(E_FAIL); // TODO: Replace with custom error code. Something similar to InvalidOperationException.
        }

        IFC(ResetSnapPointKeys());

        IFC(GetCommonSnapPointKeys(&lowerMarginSnapPointKey, &upperMarginSnapPointKey));

        auto children = GetUnsortedChildren();
        auto count = children.GetCount();
        ASSERT(count <= UINT8_MAX - 1);
        count = std::min(count, static_cast<uint32_t>(UINT8_MAX - 1));

        if (count > 0)
        {
            pSnapPoints = new XFLOAT[count];

            pSnapPointKeys = new XFLOAT[count];

            for (XUINT32 childIndex = 0; childIndex < count; childIndex++)
            {
                CUIElement* pChild = children[childIndex];
                if (pChild)
                {
                    IFC(pChild->EnsureLayoutStorage());

                    if (bNearAligned)
                    {
                        // Snap points are aligned to the left/top of the children
                        pSnapPoints[cSnapPoints] = cumulatedDim;
                    }

                    if (m_orientation == DirectUI::Orientation::Vertical)
                    {
                        childDim = pChild->GetLayoutStorage()->m_desiredSize.height;
                    }
                    else
                    {
                        childDim = pChild->GetLayoutStorage()->m_desiredSize.width;
                    }

                    if (!bNearAligned && !bFarAligned)
                    {
                        // Snap points are centered on the children
                        pSnapPoints[cSnapPoints] = cumulatedDim + childDim / 2;
                    }

                    cumulatedDim += childDim;

                    if (bFarAligned)
                    {
                        // Snap points are aligned to the right/bottom of the children
                        pSnapPoints[cSnapPoints] = cumulatedDim;
                    }

                    if (!(bNearAligned && bIsFirstChild))
                    {
                        // Do not include the lower margin for the first child's snap point when the alignment is Near
                        pSnapPoints[cSnapPoints] += lowerMarginSnapPointKey;
                    }

                    bIsFirstChild = FALSE;

                    pSnapPointKeys[cSnapPointKeys] = childDim;

                    cSnapPoints++;
                    cSnapPointKeys++;
                }
            }

            *ppSnapPoints = pSnapPoints;
            *pcSnapPoints = cSnapPoints;
            pSnapPoints = NULL;
        }

        m_pIrregularSnapPointKeys = pSnapPointKeys;
        m_cIrregularSnapPointKeys = cSnapPointKeys;
        m_lowerMarginSnapPointKey = lowerMarginSnapPointKey;
        m_upperMarginSnapPointKey = upperMarginSnapPointKey;
        m_bAreSnapPointsKeysHorizontal = bHorizontalOrientation;
        pSnapPointKeys = NULL;

        // Next snap point change needs to raise a notification
        if (bHorizontalOrientation)
        {
            m_bNotifiedHorizontalSnapPointsChanges = FALSE;
        }
        else
        {
            m_bNotifiedVerticalSnapPointsChanges = FALSE;
        }
    }

Cleanup:
    delete [] pSnapPoints;
    delete [] pSnapPointKeys;
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CStackPanel::GetRegularSnapPoints
//
//  Synopsis:
//    Used to retrieve an offset and interval for regular snap points.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CStackPanel::GetRegularSnapPoints(
    _In_ bool bHorizontalOrientation,  // True when horizontal snap points are requested.
    _In_ bool bNearAligned,            // True when requested snap points will align to the left/top of the children
    _In_ bool bFarAligned,             // True when requested snap points will align to the right/bottom of the children
    _Out_ XFLOAT* pOffset,              // Placeholder for snap points offset
    _Out_ XFLOAT* pInterval)            // Placeholder for snap points interval
{
    XFLOAT childDim = 0.0;
    XFLOAT lowerMarginSnapPointKey = 0.0;
    XFLOAT upperMarginSnapPointKey = 0.0;

    IFCEXPECT_RETURN(pOffset);
    *pOffset = 0.0;
    IFCEXPECT_RETURN(pInterval);
    *pInterval = 0.0;

    if ((m_orientation == DirectUI::Orientation::Vertical && !bHorizontalOrientation) ||
        (m_orientation == DirectUI::Orientation::Horizontal && bHorizontalOrientation))
    {
        if (!m_bAreScrollSnapPointsRegular)
        {
            // Accessing the regular snap points while AreScrollSnapPointsRegular is False is not supported.
            IFC_RETURN(E_FAIL); // TODO: Replace with custom error code. Something similar to InvalidOperationException.
        }

        IFC_RETURN(ResetSnapPointKeys());

        IFC_RETURN(GetCommonSnapPointKeys(&lowerMarginSnapPointKey, &upperMarginSnapPointKey));

        auto children = GetUnsortedChildren();
        UINT32 count = children.GetCount();
        for (XUINT32 childIndex = 0; childIndex < count; childIndex++)
        {
            CUIElement* pChild = children[childIndex];
            if (pChild)
            {
                IFC_RETURN(pChild->EnsureLayoutStorage());

                if (m_orientation == DirectUI::Orientation::Vertical)
                {
                    childDim = pChild->GetLayoutStorage()->m_desiredSize.height;
                }
                else
                {
                    childDim = pChild->GetLayoutStorage()->m_desiredSize.width;
                }

                if (bNearAligned)
                {
                    *pOffset = lowerMarginSnapPointKey;
                }
                else if (bFarAligned)
                {
                    *pOffset = upperMarginSnapPointKey;
                }
                else if (!bNearAligned && !bFarAligned)
                {
                    // Snap points are centered on the children
                    *pOffset = childDim / 2 + lowerMarginSnapPointKey;
                }
                *pInterval = childDim;
                break;
            }
        }

        m_regularSnapPointKey = childDim;
        m_lowerMarginSnapPointKey = lowerMarginSnapPointKey;
        m_upperMarginSnapPointKey = upperMarginSnapPointKey;
        m_bAreSnapPointsKeysHorizontal = bHorizontalOrientation;

        // Next snap point change needs to raise a notification
        if (bHorizontalOrientation)
        {
            m_bNotifiedHorizontalSnapPointsChanges = FALSE;
        }
        else
        {
            m_bNotifiedVerticalSnapPointsChanges = FALSE;
        }
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CStackPanel::GetIrregularSnapPointKeys
//
//  Synopsis:
//    Determines the keys for irregular snap points.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CStackPanel::GetIrregularSnapPointKeys(
    const CUIElementCollectionWrapper& children,
    _Outptr_result_buffer_(*pcSnapPointKeys) XFLOAT** ppSnapPointKeys,
    _Out_ XINT32* pcSnapPointKeys,
    _Out_ XFLOAT* pLowerMarginSnapPointKey,
    _Out_ XFLOAT* pUpperMarginSnapPointKey)
{
    HRESULT hr = S_OK;
    XINT32 cSnapPointKeys = 0;
    XFLOAT* pSnapPointKeys = NULL;
    CUIElement* pChild = NULL;
    UINT32 count = children.GetCount();

    IFCEXPECT(ppSnapPointKeys);
    *ppSnapPointKeys = NULL;
    IFCEXPECT(pcSnapPointKeys);
    *pcSnapPointKeys = 0;
    IFCEXPECT(pLowerMarginSnapPointKey);
    *pLowerMarginSnapPointKey = 0.0;
    IFCEXPECT(pUpperMarginSnapPointKey);
    *pUpperMarginSnapPointKey = 0.0;

    if (count > 0)
    {
        pSnapPointKeys = new XFLOAT[count];

        for (XUINT32 childIndex = 0; childIndex < count; childIndex++)
        {
            pChild = children[childIndex];
            if (pChild)
            {
                IFC(pChild->EnsureLayoutStorage());

                if (m_orientation == DirectUI::Orientation::Vertical)
                {
                    pSnapPointKeys[cSnapPointKeys] = pChild->GetLayoutStorage()->m_desiredSize.height;
                }
                else
                {
                    pSnapPointKeys[cSnapPointKeys] = pChild->GetLayoutStorage()->m_desiredSize.width;
                }
                cSnapPointKeys++;
            }
        }

        *ppSnapPointKeys = pSnapPointKeys;
        *pcSnapPointKeys = cSnapPointKeys;
        pSnapPointKeys = NULL;
    }

    IFC(GetCommonSnapPointKeys(pLowerMarginSnapPointKey, pUpperMarginSnapPointKey));

Cleanup:
    delete [] pSnapPointKeys;
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CStackPanel::GetRegularSnapPointKeys
//
//  Synopsis:
//    Determines the keys for regular snap points.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CStackPanel::GetRegularSnapPointKeys(
    const CUIElementCollectionWrapper& children,
    _Out_ XFLOAT* pSnapPointKey,
    _Out_ XFLOAT* pLowerMarginSnapPointKey,
    _Out_ XFLOAT* pUpperMarginSnapPointKey)
{
    CUIElement* pChild = NULL;
    auto count = children.GetCount();

    IFCEXPECT_RETURN(pSnapPointKey);
    *pSnapPointKey = 0.0;
    IFCEXPECT_RETURN(pLowerMarginSnapPointKey);
    *pLowerMarginSnapPointKey = 0.0;
    IFCEXPECT_RETURN(pUpperMarginSnapPointKey);
    *pUpperMarginSnapPointKey = 0.0;

    for (XUINT32 childIndex = 0; childIndex < count; childIndex++)
    {
        pChild = children[childIndex];
        if (pChild)
        {
            IFC_RETURN(pChild->EnsureLayoutStorage());

            if (m_orientation == DirectUI::Orientation::Vertical)
            {
                *pSnapPointKey = pChild->GetLayoutStorage()->m_desiredSize.height;
            }
            else
            {
                *pSnapPointKey = pChild->GetLayoutStorage()->m_desiredSize.width;
            }
            break;
        }
    }

    IFC_RETURN(GetCommonSnapPointKeys(pLowerMarginSnapPointKey, pUpperMarginSnapPointKey));

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CStackPanel::GetCommonSnapPointKeys
//
//  Synopsis:
//    Determines the common keys for regular and irregular snap points.
//    Those keys are the left/right margins for a horizontal stackpanel,
//    or the top/bottom margins for a vertical stackpanel.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CStackPanel::GetCommonSnapPointKeys(
    _Out_ XFLOAT* pLowerMarginSnapPointKey,
    _Out_ XFLOAT* pUpperMarginSnapPointKey)
{
    IFCEXPECT_RETURN(pLowerMarginSnapPointKey);
    *pLowerMarginSnapPointKey = 0.0;
    IFCEXPECT_RETURN(pUpperMarginSnapPointKey);
    *pUpperMarginSnapPointKey = 0.0;

    if (m_pLayoutProperties)
    {
        if (m_orientation == DirectUI::Orientation::Horizontal)
        {
            *pLowerMarginSnapPointKey = m_pLayoutProperties->m_margin.left;
            *pUpperMarginSnapPointKey = m_pLayoutProperties->m_margin.right;
        }
        else
        {
            *pLowerMarginSnapPointKey = m_pLayoutProperties->m_margin.top;
            *pUpperMarginSnapPointKey = m_pLayoutProperties->m_margin.bottom;
        }
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CStackPanel::NotifySnapPointsChanges
//
//  Synopsis:
//    Called to let the peer know that snap points have changed.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CStackPanel::NotifySnapPointsChanges(_In_ bool bIsForHorizontalSnapPoints)
{
    if ((bIsForHorizontalSnapPoints && m_bNotifyHorizontalSnapPointsChanges && !m_bNotifiedHorizontalSnapPointsChanges) ||
        (!bIsForHorizontalSnapPoints && m_bNotifyVerticalSnapPointsChanges && !m_bNotifiedVerticalSnapPointsChanges))
    {
        IFC_RETURN(FxCallbacks::UIElement_NotifySnapPointsChanged(this, bIsForHorizontalSnapPoints));

        if (bIsForHorizontalSnapPoints)
        {
            m_bNotifiedHorizontalSnapPointsChanges = TRUE;
        }
        else
        {
            m_bNotifiedVerticalSnapPointsChanges = TRUE;
        }
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CStackPanel::NotifySnapPointsChanges
//
//  Synopsis:
//    Checks if the snap point keys have changed and a notification needs
//    to be raised.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CStackPanel::NotifySnapPointsChanges(const CUIElementCollectionWrapper& children)
{
    HRESULT hr = S_OK;
    XINT32 cSnapPointKeys = 0;
    XFLOAT* pSnapPointKeys = NULL;
    XFLOAT snapPointKey = 0.0;
    XFLOAT lowerMarginSnapPointKey = 0.0;
    XFLOAT upperMarginSnapPointKey = 0.0;
    bool bNotifyForHorizontalSnapPoints = false;
    bool bNotifyForVerticalSnapPoints = false;

    if (m_orientation == DirectUI::Orientation::Vertical)
    {
        if (((m_regularSnapPointKey != -1.0) || (m_cIrregularSnapPointKeys != UINT8_MAX)) &&
            m_bAreSnapPointsKeysHorizontal && m_bNotifyHorizontalSnapPointsChanges)
        {
            // Last computed snap point keys were for horizontal orientation.
            // New orientation is vertical.
            // Consumer wants notifications for horizontal snap points.
            bNotifyForHorizontalSnapPoints = TRUE;
        }
    }
    else
    {
        if (((m_regularSnapPointKey != -1.0) || (m_cIrregularSnapPointKeys != UINT8_MAX)) &&
            !m_bAreSnapPointsKeysHorizontal && m_bNotifyVerticalSnapPointsChanges)
        {
            // Last computed snap point keys were for vertical orientation.
            // New orientation is horizontal.
            // Consumer wants notifications for vertical snap points.
            bNotifyForVerticalSnapPoints = TRUE;
        }
    }

    if ((m_bNotifyHorizontalSnapPointsChanges && m_orientation == DirectUI::Orientation::Horizontal &&
         m_bAreSnapPointsKeysHorizontal && !m_bNotifiedHorizontalSnapPointsChanges) ||
         (m_bNotifyVerticalSnapPointsChanges && m_orientation == DirectUI::Orientation::Vertical &&
         !m_bAreSnapPointsKeysHorizontal && !m_bNotifiedVerticalSnapPointsChanges))
    {
        if (m_regularSnapPointKey != -1.0)
        {
            if (m_bAreScrollSnapPointsRegular)
            {
                IFC(GetRegularSnapPointKeys(children, &snapPointKey, &lowerMarginSnapPointKey, &upperMarginSnapPointKey));
                if (m_regularSnapPointKey != snapPointKey ||
                    m_lowerMarginSnapPointKey != lowerMarginSnapPointKey ||
                    m_upperMarginSnapPointKey != upperMarginSnapPointKey)
                {
                    if (m_bAreSnapPointsKeysHorizontal)
                    {
                        bNotifyForHorizontalSnapPoints = TRUE;
                    }
                    else
                    {
                        bNotifyForVerticalSnapPoints = TRUE;
                    }
                }
            }
            else
            {
                if (m_bAreSnapPointsKeysHorizontal)
                {
                    bNotifyForHorizontalSnapPoints = TRUE;
                }
                else
                {
                    bNotifyForVerticalSnapPoints = TRUE;
                }
            }
        }
        else if (m_cIrregularSnapPointKeys != UINT8_MAX)
        {
            if (!m_bAreScrollSnapPointsRegular)
            {
                IFC(GetIrregularSnapPointKeys(children, &pSnapPointKeys, &cSnapPointKeys, &lowerMarginSnapPointKey, &upperMarginSnapPointKey));
                if (static_cast<uint32_t>(m_cIrregularSnapPointKeys) != cSnapPointKeys ||
                    m_lowerMarginSnapPointKey != lowerMarginSnapPointKey ||
                    m_upperMarginSnapPointKey != upperMarginSnapPointKey)
                {
                    if (m_bAreSnapPointsKeysHorizontal)
                    {
                        bNotifyForHorizontalSnapPoints = TRUE;
                    }
                    else
                    {
                        bNotifyForVerticalSnapPoints = TRUE;
                    }
                }
                else
                {
                    for (XINT32 iSnapPointKey = 0; iSnapPointKey < cSnapPointKeys; iSnapPointKey++)
                    {
                        if (m_pIrregularSnapPointKeys[iSnapPointKey] != pSnapPointKeys[iSnapPointKey])
                        {
                            if (m_bAreSnapPointsKeysHorizontal)
                            {
                                bNotifyForHorizontalSnapPoints = TRUE;
                            }
                            else
                            {
                                bNotifyForVerticalSnapPoints = TRUE;
                            }
                            break;
                        }
                    }
                }
            }
            else
            {
                if (m_bAreSnapPointsKeysHorizontal)
                {
                    bNotifyForHorizontalSnapPoints = TRUE;
                }
                else
                {
                    bNotifyForVerticalSnapPoints = TRUE;
                }
            }
        }
    }

    if (bNotifyForHorizontalSnapPoints)
    {
        IFC(NotifySnapPointsChanges(TRUE /*bIsForHorizontalSnapPoints*/));
    }

    if (bNotifyForVerticalSnapPoints)
    {
        IFC(NotifySnapPointsChanges(FALSE /*bIsForHorizontalSnapPoints*/));
    }

Cleanup:
    delete [] pSnapPointKeys;
    RRETURN(hr);
}


//-------------------------------------------------------------------------
//
//  Function:   CStackPanel::OnAreScrollSnapPointsRegularChanged
//
//  Synopsis:
//    Called when the AreScrollSnapPointsRegular property changed.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CStackPanel::OnAreScrollSnapPointsRegularChanged()
{
    auto children = GetUnsortedChildren();
    IFC_RETURN(NotifySnapPointsChanges(children));

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CStackPanel::RefreshIrregularSnapPointKeys
//
//  Synopsis:
//    Refreshes the m_pIrregularSnapPointKeys/m_cIrregularSnapPointKeys
//    fields based on all children.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CStackPanel::RefreshIrregularSnapPointKeys()
{
    HRESULT hr = S_OK;
    XINT32 cSnapPointKeys = 0;
    XFLOAT* pSnapPointKeys = NULL;
    XFLOAT lowerMarginSnapPointKey = 0.0;
    XFLOAT upperMarginSnapPointKey = 0.0;

    ASSERT(!m_bAreScrollSnapPointsRegular);

    auto children = GetUnsortedChildren();

    IFC(ResetSnapPointKeys());

    IFC(GetIrregularSnapPointKeys(
        children,
        &pSnapPointKeys,
        &cSnapPointKeys,
        &lowerMarginSnapPointKey,
        &upperMarginSnapPointKey));

    m_pIrregularSnapPointKeys = pSnapPointKeys;
    ASSERT(cSnapPointKeys <= UINT8_MAX - 1);
    m_cIrregularSnapPointKeys = static_cast<uint8_t>(cSnapPointKeys);
    m_lowerMarginSnapPointKey = lowerMarginSnapPointKey;
    m_upperMarginSnapPointKey = upperMarginSnapPointKey;
    m_bAreSnapPointsKeysHorizontal = (m_orientation == DirectUI::Orientation::Horizontal);
    pSnapPointKeys = NULL;

Cleanup:
    delete [] pSnapPointKeys;
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CStackPanel::RefreshRegularSnapPointKeys
//
//  Synopsis:
//    Refreshes the m_regularSnapPointKey field based on a single child.
//    Refreshes also the m_lowerMarginSnapPointKey/m_upperMarginSnapPointKey fields based
//    on the current margins.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CStackPanel::RefreshRegularSnapPointKeys()
{
    XFLOAT snapPointKey = 0.0;
    XFLOAT lowerMarginSnapPointKey = 0.0;
    XFLOAT upperMarginSnapPointKey = 0.0;

    ASSERT(m_bAreScrollSnapPointsRegular);

    IFC_RETURN(ResetSnapPointKeys());

    m_regularSnapPointKey = 0.0;

    auto children = GetUnsortedChildren();

    IFC_RETURN(GetRegularSnapPointKeys(children, &snapPointKey, &lowerMarginSnapPointKey, &upperMarginSnapPointKey));

    m_bAreSnapPointsKeysHorizontal = (m_orientation == DirectUI::Orientation::Horizontal);
    m_regularSnapPointKey = snapPointKey;
    m_lowerMarginSnapPointKey = lowerMarginSnapPointKey;
    m_upperMarginSnapPointKey = upperMarginSnapPointKey;

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CStackPanel::ResetSnapPointKeys
//
//  Synopsis:
//    Resets both regular and irregular snap point keys.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CStackPanel::ResetSnapPointKeys()
{
    delete [] m_pIrregularSnapPointKeys;
    m_pIrregularSnapPointKeys = NULL;
    m_cIrregularSnapPointKeys = UINT8_MAX;
    m_regularSnapPointKey = -1.0;
    m_lowerMarginSnapPointKey = 0;
    m_upperMarginSnapPointKey = 0;

    RRETURN(S_OK);
}

//-------------------------------------------------------------------------
//
//  Function:   CStackPanel::SetSnapPointsChangeNotificationsRequirement
//
//  Synopsis:
//    Determines whether the StackPanel must call NotifySnapPointsChanged
//    when snap points change or not.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CStackPanel::SetSnapPointsChangeNotificationsRequirement(
    _In_ bool bIsForHorizontalSnapPoints,
    _In_ bool bNotifyChanges)
{
    if (bIsForHorizontalSnapPoints)
    {
        m_bNotifyHorizontalSnapPointsChanges = bNotifyChanges;
        if (m_orientation == DirectUI::Orientation::Horizontal && bNotifyChanges)
        {
            if (m_bAreScrollSnapPointsRegular)
            {
                IFC_RETURN(RefreshRegularSnapPointKeys());
            }
            else
            {
                IFC_RETURN(RefreshIrregularSnapPointKeys());
            }
            m_bNotifiedHorizontalSnapPointsChanges = FALSE;
        }
    }
    else
    {
        m_bNotifyVerticalSnapPointsChanges = bNotifyChanges;
        if (m_orientation == DirectUI::Orientation::Vertical && bNotifyChanges)
        {
            if (m_bAreScrollSnapPointsRegular)
            {
                IFC_RETURN(RefreshRegularSnapPointKeys());
            }
            else
            {
                IFC_RETURN(RefreshIrregularSnapPointKeys());
            }
            m_bNotifiedVerticalSnapPointsChanges = FALSE;
        }
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CStackPanel::SetValue
//
//  Synopsis:
//    Overridden to detect changes of the AreScrollSnapPointsRegular property.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CStackPanel::SetValue(_In_ const SetValueParams& args)
{
    ASSERT(args.m_pDP != nullptr);

    IFC_RETURN(CPanel::SetValue(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::StackPanel_AreScrollSnapPointsRegular:
        IFC_RETURN(OnAreScrollSnapPointsRegularChanged());
        break;
    }

    return S_OK;
}

xref_ptr<CBrush> CStackPanel::GetBorderBrush() const
{
    if(!IsPropertyDefaultByIndex(KnownPropertyIndex::StackPanel_BorderBrush))
    {
        CValue result;
        VERIFYHR(GetValueByIndex(KnownPropertyIndex::StackPanel_BorderBrush, &result));
        return static_sp_cast<CBrush>(result.DetachObject());
    }
    else
    {
        return CPanel::GetBorderBrush();
    }
}

XTHICKNESS CStackPanel::GetBorderThickness() const
{
    if(!IsPropertyDefaultByIndex(KnownPropertyIndex::StackPanel_BorderThickness))
    {
        CValue result;
        VERIFYHR(GetValueByIndex(KnownPropertyIndex::StackPanel_BorderThickness, &result));
        return *(result.AsThickness());
    }
    else
    {
        return CPanel::GetBorderThickness();
    }
}

XTHICKNESS CStackPanel::GetPadding() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::StackPanel_Padding, &result));
    return *(result.AsThickness());
}

XCORNERRADIUS CStackPanel::GetCornerRadius() const
{
    if(!IsPropertyDefaultByIndex(KnownPropertyIndex::StackPanel_CornerRadius))
    {
        CValue result;
        VERIFYHR(GetValueByIndex(KnownPropertyIndex::StackPanel_CornerRadius, &result));
        return *(result.AsCornerRadius());
    }
    else
    {
        return CPanel::GetCornerRadius();
    }
}

DirectUI::BackgroundSizing CStackPanel::GetBackgroundSizing() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::StackPanel_BackgroundSizing, &result));
    return static_cast<DirectUI::BackgroundSizing>(result.AsEnum());
}

float CStackPanel::GetSpacing() const
{
    CValue result;
    IFCFAILFAST(GetValueByIndex(KnownPropertyIndex::StackPanel_Spacing, &result));
    return result.As<valueFloat>();
}
