// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RelativePanel.h"

#include "XamlTraceLogging.h"

CRelativePanel::CRelativePanel(_In_ CCoreServices *pCore)
    : CPanel(pCore)
{
}

CRelativePanel::~CRelativePanel()
{
}

_Check_return_ HRESULT CRelativePanel::MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize)
{
    XSIZEF availableSizeForChildren;
    XSIZEF borderSize = CBorder::HelperGetCombinedThickness(this);

    // The available size for the children is equal to the available
    // size for the panel minus the size of the border.
    availableSizeForChildren.width = availableSize.width - borderSize.width;
    availableSizeForChildren.height = availableSize.height - borderSize.height;

    HRESULT hr = S_OK;
    IFC(GenerateGraph());
    IFC(m_graph.MeasureNodes(availableSizeForChildren));

    // Now that the children have been measured, we can calculate
    // the desired size of the panel, which corresponds to the 
    // desired size of the children as a whole plus the size of
    // the border.
    XSIZEF desiredSizeOfChildren = m_graph.CalculateDesiredSize();

    desiredSize.width = desiredSizeOfChildren.width + borderSize.width;
    desiredSize.height = desiredSizeOfChildren.height + borderSize.height;
    return hr;

Cleanup:
    // If this is a known error, we must throw the appropriate
    // exception based on the AgCode set by the helper class.
    // Otherwise we just fail normally.
    if (m_graph.m_knownErrorPending)
    {
        m_graph.m_knownErrorPending = false;

        if (m_graph.m_agErrorCode == AG_E_RELATIVEPANEL_NAME_NOT_FOUND)
        {
            xephemeral_string_ptr parameters[1];
            m_graph.m_errorParameter.Demote(&parameters[0]);

            return SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, m_graph.m_agErrorCode, 1, parameters);
        }
        else
        {
            return SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, m_graph.m_agErrorCode);
        }
    }
    else
    {
        return hr;
    }
}

_Check_return_ HRESULT CRelativePanel::ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize)
{
    XRECTF childRect = CBorder::HelperGetInnerRect(this, finalSize);

    IFC_RETURN(m_graph.ArrangeNodes(childRect));
    newFinalSize = finalSize;

    return S_OK;
}

_Check_return_ HRESULT CRelativePanel::GenerateGraph()
{
    CUIElementCollection* children = static_cast<CUIElementCollection*>(GetChildren());

    m_graph.GetNodes().clear();

    if (children)
    {
        auto core = GetContext();

        auto it = m_graph.GetNodes().before_begin();

        // Create a node for each child and add it to the graph.
        for (auto child : (*children))
        {
            it = m_graph.GetNodes().emplace_after(it, static_cast<CUIElement*>(child));
        }

        auto namescopeInfo = core->GetAdjustedReferenceObjectAndNamescopeType(this);

        // Now that we have all the nodes, we can build an adjacency list
        // based on the dependencies that each child has on its siblings, 
        // if any.
        IFC_RETURN(m_graph.ResolveConstraints(this, core, std::get<0>(namescopeInfo), std::get<1>(namescopeInfo)));
    }

    return S_OK;
}

xref_ptr<CBrush> CRelativePanel::GetBorderBrush() const
{
    if(!IsPropertyDefaultByIndex(KnownPropertyIndex::RelativePanel_BorderBrush))
    {
        CValue result;
        VERIFYHR(GetValueByIndex(KnownPropertyIndex::RelativePanel_BorderBrush, &result));
        return static_sp_cast<CBrush>(result.DetachObject());
    }
    else
    {
        return CPanel::GetBorderBrush();
    }    
}

XTHICKNESS CRelativePanel::GetBorderThickness() const
{
    if(!IsPropertyDefaultByIndex(KnownPropertyIndex::RelativePanel_BorderThickness))
    {
        CValue result;
        VERIFYHR(GetValueByIndex(KnownPropertyIndex::RelativePanel_BorderThickness, &result));
        return *(result.AsThickness());
    }
    else
    {
        return CPanel::GetBorderThickness();
    }
    
}

XTHICKNESS CRelativePanel::GetPadding() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::RelativePanel_Padding, &result));
    return *(result.AsThickness());
}

XCORNERRADIUS CRelativePanel::GetCornerRadius() const
{
    if(!IsPropertyDefaultByIndex(KnownPropertyIndex::RelativePanel_CornerRadius))
    {
        CValue result;
        VERIFYHR(GetValueByIndex(KnownPropertyIndex::RelativePanel_CornerRadius, &result));
        return *(result.AsCornerRadius());
    }
    else
    {
        return CPanel::GetCornerRadius();
    }
}

DirectUI::BackgroundSizing CRelativePanel::GetBackgroundSizing() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::RelativePanel_BackgroundSizing, &result));
    return static_cast<DirectUI::BackgroundSizing>(result.AsEnum());
}
