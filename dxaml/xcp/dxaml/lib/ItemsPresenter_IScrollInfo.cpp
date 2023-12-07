// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ItemsPresenter.g.h"
#include "ScrollViewer.g.h"
#include "ScrollContentPresenter.g.h"
#include "Control.g.h"
#include "Panel.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// IScrollInfo interface implementation

_Check_return_ HRESULT ItemsPresenter::get_CanVerticallyScrollImpl(_Out_ BOOLEAN* pValue) 
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IScrollInfo> spScrollInfo;

    IFCPTR(pValue);

    *pValue = TRUE;

    IFC(get_Panel(&spPanel));
    spScrollInfo = spPanel.AsOrNull<IScrollInfo>();

    if (spScrollInfo)
    {
        IFC(spScrollInfo->get_CanVerticallyScroll(pValue));
    }

    *pValue = *pValue && m_ScrollData.m_canVerticallyScroll;

Cleanup:
    RRETURN(hr);
}
            
_Check_return_ HRESULT ItemsPresenter::put_CanVerticallyScrollImpl(_In_ BOOLEAN value) 
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IScrollInfo> spScrollInfo;

    if (m_ScrollData.m_canVerticallyScroll != value)
    {
        m_ScrollData.m_canVerticallyScroll = value;
    }

    IFC(get_Panel(&spPanel));
    spScrollInfo = spPanel.AsOrNull<IScrollInfo>();

    if (spScrollInfo)
    {
        IFC(spScrollInfo->put_CanVerticallyScroll(value));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsPresenter::get_CanHorizontallyScrollImpl(_Out_ BOOLEAN* pValue) 
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IScrollInfo> spScrollInfo;

    IFCPTR(pValue);

    *pValue = TRUE;

    IFC(get_Panel(&spPanel));
    spScrollInfo = spPanel.AsOrNull<IScrollInfo>();

    if (spScrollInfo)
    {
        IFC(spScrollInfo->get_CanHorizontallyScroll(pValue));
    }

    *pValue = *pValue && m_ScrollData.m_canHorizontallyScroll;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsPresenter::put_CanHorizontallyScrollImpl(_In_ BOOLEAN value) 
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IScrollInfo> spScrollInfo;

    if (m_ScrollData.m_canHorizontallyScroll != value)
    {
        m_ScrollData.m_canHorizontallyScroll = value;
    }

    IFC(get_Panel(&spPanel));
    spScrollInfo = spPanel.AsOrNull<IScrollInfo>();

    if (spScrollInfo)
    {
        IFC(spScrollInfo->put_CanHorizontallyScroll(value));
    }

Cleanup:
    RRETURN(hr);
}
            
_Check_return_ HRESULT ItemsPresenter::get_ExtentWidthImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_extent.Width;

    RRETURN(hr);
}
            
_Check_return_ HRESULT ItemsPresenter::get_ExtentHeightImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_extent.Height;

    RRETURN(hr);
}
            
_Check_return_ HRESULT ItemsPresenter::get_ViewportWidthImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_viewport.Width;
    
    RRETURN(hr);
}
            
_Check_return_ HRESULT ItemsPresenter::get_ViewportHeightImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_viewport.Height;
    
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsPresenter::get_HorizontalOffsetImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_ComputedOffset.X;

    RRETURN(hr);
}
            
_Check_return_ HRESULT ItemsPresenter::get_VerticalOffsetImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_ComputedOffset.Y;

    RRETURN(hr);
}
     
_Check_return_ HRESULT ItemsPresenter::get_MinHorizontalOffsetImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_MinOffset.X;

    RRETURN(hr);
}
            
_Check_return_ HRESULT ItemsPresenter::get_MinVerticalOffsetImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_ScrollData.m_MinOffset.Y;

    RRETURN(hr);
}

_Check_return_ HRESULT ItemsPresenter::get_ScrollOwnerImpl(_Outptr_ IInspectable** pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spOwner;
    
    *pValue = NULL;

    IFC(m_ScrollData.get_ScrollOwner(&spOwner));

    IFC(spOwner.CopyTo(pValue));
        
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsPresenter::put_ScrollOwnerImpl(_In_opt_ IInspectable* value)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spOwner;
    ctl::ComPtr<IScrollInfo> spScrollInfo;
    ctl::ComPtr<IScrollOwner> spNewOwner;
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IInspectable> spInnerOwner;

    IFC(ctl::do_query_interface(spNewOwner, value));    
    IFC(m_ScrollData.get_ScrollOwner(&spOwner));

    if (spOwner != spNewOwner)
    {
        m_ScrollData.ClearLayout(); 
        IFC(m_ScrollData.put_ScrollOwner(spNewOwner.Get()));
    }
    
    IFC(get_Panel(&spPanel));
    spScrollInfo = spPanel.AsOrNull<IScrollInfo>();

    if (spScrollInfo)
    {
        IFC(spScrollInfo->get_ScrollOwner(&spInnerOwner));
        if (spInnerOwner.Get()!= ctl::as_iinspectable(this))
        {
            IFC(spScrollInfo->put_ScrollOwner(ctl::as_iinspectable(this)));
        }
    }

Cleanup:
    RRETURN(hr);
}
            
_Check_return_ HRESULT ItemsPresenter::LineUpImpl()
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    DOUBLE pixelDelta = -ScrollViewerLineDelta;
    IFC(TranslateVerticalPixelDeltaToOffset(m_ScrollData.get_OffsetY(), pixelDelta, offset));
    IFC(SetVerticalOffset(offset));
    
Cleanup:
    RRETURN(hr);
}
            
_Check_return_ HRESULT ItemsPresenter::LineDownImpl()
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    DOUBLE pixelDelta = ScrollViewerLineDelta;
    IFC(TranslateVerticalPixelDeltaToOffset(m_ScrollData.get_OffsetY(), pixelDelta, offset));
    IFC(SetVerticalOffset(offset));
    
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsPresenter::LineLeftImpl()
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    DOUBLE pixelDelta = -ScrollViewerLineDelta;
    IFC(TranslateHorizontalPixelDeltaToOffset(m_ScrollData.get_OffsetX(), pixelDelta, offset));
    IFC(SetHorizontalOffset(offset));
    
Cleanup:
    RRETURN(hr);
}
            
_Check_return_ HRESULT ItemsPresenter::LineRightImpl()
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    DOUBLE pixelDelta = ScrollViewerLineDelta;
    IFC(TranslateHorizontalPixelDeltaToOffset(m_ScrollData.get_OffsetX(), pixelDelta, offset));
    IFC(SetHorizontalOffset(offset));
    
Cleanup:
    RRETURN(hr);
}
            
_Check_return_ HRESULT ItemsPresenter::PageUpImpl()
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    wf::Size desiredSize = {};
    DOUBLE pixelDelta = 0;

    IFC(get_DesiredSize(&desiredSize));
    pixelDelta = -desiredSize.Height;

    IFC(TranslateVerticalPixelDeltaToOffset(m_ScrollData.get_OffsetY(), pixelDelta, offset));
    IFC(SetVerticalOffset(offset));

Cleanup:
    RRETURN(hr);
}
            
_Check_return_ HRESULT ItemsPresenter::PageDownImpl()
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    wf::Size desiredSize = {};
    DOUBLE pixelDelta = 0;

    IFC(get_DesiredSize(&desiredSize));
    pixelDelta = desiredSize.Height;

    IFC(TranslateVerticalPixelDeltaToOffset(m_ScrollData.get_OffsetY(), pixelDelta, offset));
    IFC(SetVerticalOffset(offset));

Cleanup:
    RRETURN(hr);
}
            
_Check_return_ HRESULT ItemsPresenter::PageLeftImpl() 
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    wf::Size desiredSize = {};
    DOUBLE pixelDelta = 0;

    IFC(get_DesiredSize(&desiredSize));
    pixelDelta = -desiredSize.Width;

    IFC(TranslateHorizontalPixelDeltaToOffset(m_ScrollData.get_OffsetX(), pixelDelta, offset));
    IFC(SetHorizontalOffset(offset));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsPresenter::PageRightImpl()
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    wf::Size desiredSize = {};
    DOUBLE pixelDelta = 0;

    IFC(get_DesiredSize(&desiredSize));
    pixelDelta = desiredSize.Width;

    IFC(TranslateHorizontalPixelDeltaToOffset(m_ScrollData.get_OffsetX(), pixelDelta, offset));
    IFC(SetHorizontalOffset(offset));

Cleanup:
    RRETURN(hr);
}
            
IFACEMETHODIMP ItemsPresenter::MouseWheelUp(_In_ UINT mouseWheelDelta)
{
    HRESULT hr = S_OK;
    BOOLEAN passToScrollOwner = FALSE;
    
    IFC(get_ShouldPassWheelMessageToScrollOwner(passToScrollOwner));
    
    if (passToScrollOwner)
    {
        IFC(PassWheelMessageToScrollOwner(ZoomDirection_In));
    }
    else
    {
        xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
        DOUBLE offset = 0.0;
        wf::Size size;
        
        IFC(get_DesiredSize(&size));
        IFC(get_PhysicalOrientation(&orientation));
        if (orientation == xaml_controls::Orientation_Vertical)
        {
            DOUBLE pixelDelta = -GetVerticalScrollWheelDelta(size, mouseWheelDelta);
            IFC(TranslateVerticalPixelDeltaToOffset(m_ScrollData.get_OffsetY(), pixelDelta, offset));
            IFC(SetVerticalOffset(offset));
        }
        else
        {
            DOUBLE pixelDelta = -GetHorizontalScrollWheelDelta(size, mouseWheelDelta);
            IFC(TranslateHorizontalPixelDeltaToOffset(m_ScrollData.get_OffsetX(), pixelDelta, offset));
            IFC(SetHorizontalOffset(offset));
        }
    }
    
Cleanup:
    RRETURN(hr);
}
            
IFACEMETHODIMP ItemsPresenter::MouseWheelDown(_In_ UINT mouseWheelDelta)
{
    HRESULT hr = S_OK;
    BOOLEAN passToScrollOwner = FALSE;
    
    IFC(get_ShouldPassWheelMessageToScrollOwner(passToScrollOwner));
    
    if (passToScrollOwner)
    {
        IFC(PassWheelMessageToScrollOwner(ZoomDirection_Out));
    }
    else
    {
        xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
        DOUBLE offset = 0.0;
        wf::Size size;
        
        IFC(get_DesiredSize(&size));
        IFC(get_PhysicalOrientation(&orientation));
        if (orientation == xaml_controls::Orientation_Vertical)
        {
            DOUBLE pixelDelta = GetVerticalScrollWheelDelta(size, mouseWheelDelta);
            IFC(TranslateVerticalPixelDeltaToOffset(m_ScrollData.get_OffsetY(), pixelDelta, offset));
            IFC(SetVerticalOffset(offset));
        }
        else
        {
            DOUBLE pixelDelta = GetHorizontalScrollWheelDelta(size, mouseWheelDelta);
            IFC(TranslateHorizontalPixelDeltaToOffset(m_ScrollData.get_OffsetX(), pixelDelta, offset));
            IFC(SetHorizontalOffset(offset));
        }
    }
    
Cleanup:
    RRETURN(hr);
}
      
IFACEMETHODIMP ItemsPresenter::MouseWheelLeft(_In_ UINT mouseWheelDelta)
{
        HRESULT hr = S_OK;
        BOOLEAN passToScrollOwner = FALSE;
        
        IFC(get_ShouldPassWheelMessageToScrollOwner(passToScrollOwner));
        
        if (passToScrollOwner)
        {
            IFC(PassWheelMessageToScrollOwner(ZoomDirection_In));
        }
        else
        {
            xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
            DOUBLE offset = 0.0;
            wf::Size size;
            BOOLEAN invertForRTL = FALSE;
            xaml::FlowDirection flowDirection = xaml::FlowDirection_LeftToRight;
        
            IFC(get_FlowDirection(&flowDirection));
            invertForRTL = (flowDirection == xaml::FlowDirection_RightToLeft);
            
            IFC(get_DesiredSize(&size));
            IFC(get_PhysicalOrientation(&orientation));
            if (orientation == xaml_controls::Orientation_Vertical)
            {
                DOUBLE pixelDelta = -GetVerticalScrollWheelDelta(size, mouseWheelDelta);
                IFC(TranslateVerticalPixelDeltaToOffset(m_ScrollData.get_OffsetY(), pixelDelta, offset));
                IFC(SetVerticalOffset(offset));
            }
            else
            {
                DOUBLE pixelDelta = (invertForRTL ? GetHorizontalScrollWheelDelta(size, mouseWheelDelta) : -GetHorizontalScrollWheelDelta(size, mouseWheelDelta));
                IFC(TranslateHorizontalPixelDeltaToOffset(m_ScrollData.get_OffsetX(), pixelDelta, offset));
                IFC(SetHorizontalOffset(offset));
            }
        }
        
Cleanup:
    RRETURN(hr);
}
            
IFACEMETHODIMP ItemsPresenter::MouseWheelRight(_In_ UINT mouseWheelDelta)
{
    HRESULT hr = S_OK;
    BOOLEAN passToScrollOwner = FALSE;
    
    IFC(get_ShouldPassWheelMessageToScrollOwner(passToScrollOwner));
    
    if (passToScrollOwner)
    {
        IFC(PassWheelMessageToScrollOwner(ZoomDirection_Out));
    }
    else
    {
        xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
        DOUBLE offset = 0.0;
        wf::Size size;
        BOOLEAN invertForRTL = FALSE;
        xaml::FlowDirection flowDirection = xaml::FlowDirection_LeftToRight;
        
        IFC(get_FlowDirection(&flowDirection));
        invertForRTL = (flowDirection == xaml::FlowDirection_RightToLeft);
        
        IFC(get_DesiredSize(&size));
        IFC(get_PhysicalOrientation(&orientation));
        if (orientation == xaml_controls::Orientation_Vertical)
        {
            DOUBLE pixelDelta = GetVerticalScrollWheelDelta(size, mouseWheelDelta);
            IFC(TranslateVerticalPixelDeltaToOffset(m_ScrollData.get_OffsetY(), pixelDelta, offset));
            IFC(SetVerticalOffset(offset));
        }
        else
        {
            DOUBLE pixelDelta = (invertForRTL ? -GetHorizontalScrollWheelDelta(size, mouseWheelDelta) : GetHorizontalScrollWheelDelta(size, mouseWheelDelta));
            IFC(TranslateHorizontalPixelDeltaToOffset(m_ScrollData.get_OffsetX(), pixelDelta, offset));
            IFC(SetHorizontalOffset(offset));
        }
    }
    
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsPresenter::SetHorizontalOffsetImpl(_In_ DOUBLE offset)
{
    HRESULT hr = S_OK;
    BOOLEAN isHorizontal = FALSE;
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IScrollInfo> spScrollInfo;
    DOUBLE extentWidth = 0.0;
    DOUBLE viewportWidth = 0.0;
    DOUBLE scrollX = 0.0;

    IFC(IsHorizontal(isHorizontal));
    IFC(get_ExtentWidth(&extentWidth));
    IFC(get_ViewportWidth(&viewportWidth));

    IFC(ScrollContentPresenter::ValidateInputOffset(offset, m_ScrollData.m_MinOffset.X, isHorizontal ? DoubleUtil::MaxValue : extentWidth - viewportWidth, &scrollX));

    if (!DoubleUtil::AreClose(scrollX, m_ScrollData.get_OffsetX()))
    {
        IFC(m_ScrollData.put_OffsetX(scrollX));

        IFC(get_Panel(&spPanel));
        spScrollInfo = spPanel.AsOrNull<IScrollInfo>();

        if (isHorizontal)
        {
            scrollX -= 2;
        }

        if (spScrollInfo)
        {
            IFC(spScrollInfo->SetHorizontalOffset(scrollX));
        }

        BOOLEAN bIsInDMZoom = FALSE;
        IFC(IsInDirectManipulationZoom(bIsInDMZoom));
        if (!bIsInDMZoom)
        {
            IFC(InvalidateMeasure());
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsPresenter::SetVerticalOffsetImpl(_In_ DOUBLE offset)
{
    HRESULT hr = S_OK;
    BOOLEAN isHorizontal = FALSE;
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IScrollInfo> spScrollInfo;
    DOUBLE extentHeight = 0.0;
    DOUBLE viewportHeight = 0.0;
    DOUBLE scrollY = 0.0;

    IFC(IsHorizontal(isHorizontal));
    IFC(get_ExtentHeight(&extentHeight));
    IFC(get_ViewportHeight(&viewportHeight));

    IFC(ScrollContentPresenter::ValidateInputOffset(offset, m_ScrollData.m_MinOffset.Y, !isHorizontal ? DoubleUtil::MaxValue : extentHeight - viewportHeight, &scrollY));

    if (!DoubleUtil::AreClose(scrollY, m_ScrollData.get_OffsetY()))
    {
        IFC(m_ScrollData.put_OffsetY(scrollY));

        IFC(get_Panel(&spPanel));
        spScrollInfo = spPanel.AsOrNull<IScrollInfo>();

        if (!isHorizontal)
        {
            scrollY -= 2;
        }

        if (spScrollInfo)
        {
            IFC(spScrollInfo->SetVerticalOffset(scrollY));
        }

        BOOLEAN bIsInDMZoom = FALSE;
        IFC(IsInDirectManipulationZoom(bIsInDMZoom));
        if (!bIsInDMZoom)
        {
            IFC(InvalidateMeasure());
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsPresenter::MakeVisibleImpl(
    // The element that should become visible.
    _In_ xaml::IUIElement* visual,
    // A rectangle representing in the visual's coordinate space to
    // make visible.
    wf::Rect rectangle,
    // When set to True, the DManip ZoomToRect method is invoked.
    BOOLEAN useAnimation,
    DOUBLE horizontalAlignmentRatio,
    DOUBLE verticalAlignmentRatio,
    DOUBLE offsetX,
    DOUBLE offsetY,
    _Out_ wf::Rect* resultRectangle,
    _Out_opt_ DOUBLE* appliedOffsetX,
    _Out_opt_ DOUBLE* appliedOffsetY)
{
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IScrollInfo> spScrollInfo;

    IFC_RETURN(get_Panel(&spPanel));
    spScrollInfo = spPanel.AsOrNull<IScrollInfo>();

    if (spScrollInfo)
    {
        IFC_RETURN(spScrollInfo->MakeVisible(
            visual,
            rectangle,
            useAnimation,
            horizontalAlignmentRatio,
            verticalAlignmentRatio,
            offsetX,
            offsetY,
            resultRectangle,
            appliedOffsetX,
            appliedOffsetY));
    }

    return S_OK;
}

// End of IScrollInfo interface implementation

_Check_return_ HRESULT ItemsPresenter::get_ShouldPassWheelMessageToScrollOwner(_Out_ BOOLEAN &shouldPass)
{
    HRESULT hr = S_OK;
    wsy::VirtualKeyModifiers modifiers = wsy::VirtualKeyModifiers_None;
    
    IFC(Control::GetKeyboardModifiers(&modifiers));
    
Cleanup:
    shouldPass = IsFlagSet(modifiers, wsy::VirtualKeyModifiers_Control);
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsPresenter::PassWheelMessageToScrollOwner(_In_ ZoomDirection zoomDirection)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spOwner;
    
    IFC(m_ScrollData.get_ScrollOwner(&spOwner));
    if (spOwner)
    {
        IFC(spOwner->ProcessPureInertiaInputMessage(zoomDirection));
    }
    
Cleanup:
    RRETURN(hr);
}
