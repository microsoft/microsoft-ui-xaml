// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      A wrapper around UIElements used by the Drag and Drop system to draw drag icons on the TransitionRoot layer.
//      It's used by the singleton DragDrop class, which is in turn used by ListViewBase.

#include "precomp.h"
#include "DragDropVisual.h"
#include "UIElement.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Construct a new DragDropVisual that displays the given UIElement.
// pDragVisual - the UIElement to display in this DragDropVisual.
// offsetPosition - a relative offset that will be applied to positions given to SetPosition.
// useSystemDefaultVisual - the system's visual is displayed instead of the XAML's
_Check_return_ HRESULT DragDropVisual::CreateInstance(
    _In_opt_ xaml::IUIElement* pDragVisual,
    _In_ wf::Point offsetPosition,
    _Outptr_ DragDropVisual** ppInstance)
{
    HRESULT hr = S_OK;
    DragDropVisual* pVisual = NULL;

    IFCPTR(ppInstance);

    *ppInstance = NULL;

    IFC(ctl::ComObject<DragDropVisual>::CreateInstance(&pVisual));
    IFC(pVisual->Init(pDragVisual, offsetPosition));

    *ppInstance = pVisual;
    pVisual = NULL;

Cleanup:
    delete pVisual;
    RRETURN(hr);
}

// Construct a new DragDropVisual that displays the given UIElement.
_Check_return_ HRESULT  DragDropVisual::Init(_In_opt_ xaml::IUIElement* pDragVisual, _In_ wf::Point offsetPosition)
{
    m_refCount = 0;
    m_isShowing = false;
    m_opacity = 1.0f;

    if (pDragVisual)
    {
        SetPtrValue(m_tpDragVisual, pDragVisual);
        m_useSystemDefaultVisual = false;
    }
    else
    {
        m_useSystemDefaultVisual = true;
    }

    m_offsetPosition = offsetPosition;

    m_currentPosition.X = 0;
    m_currentPosition.Y = 0;
    m_offsetPositionInRoot.X = 0;
    m_offsetPositionInRoot.Y = 0;

    return S_OK;
}

// Releases all resources held by this DragDropVisual.
DragDropVisual::~DragDropVisual()
{
    auto spDragVisual = m_tpDragVisual.GetSafeReference();
    auto spDragVisualContainer = m_tpDragVisualContainer.GetSafeReference();
    if (spDragVisual && spDragVisualContainer)
    {
        VERIFYHR(HideCore(spDragVisual.Get(), spDragVisualContainer.Get()));
    }
}

// Obtains the top-leftmost corner of any visual elements drawn by this DragDropVisual.
// Will result in error if the DragDropVisual isn't shown.
_Check_return_ HRESULT DragDropVisual::GetTopLeftPosition(_Out_ wf::Point* pPosition)
{
    HRESULT hr = S_OK;
    xaml_media::Matrix matrix;

    IFCPTR(pPosition);
    IFCEXPECT(m_tpDragVisualContainerTransform);

    pPosition->X = 0;
    pPosition->Y = 0;

    IFC(m_tpDragVisualContainerTransform->get_Matrix(&matrix));

    pPosition->X = static_cast<FLOAT>(matrix.OffsetX);
    pPosition->Y = static_cast<FLOAT>(matrix.OffsetY);

Cleanup:
    RRETURN(hr);
}

// Moves this visual so it is positioned at the given point on the TransitionRoot layer.
// Will result in error if the DragDropVisual isn't shown.
_Check_return_ HRESULT DragDropVisual::SetPosition(_In_ wf::Point position)
{
    HRESULT hr = S_OK;

    m_currentPosition = position;

    IFC(UpdateTransform());

Cleanup:
    RRETURN(hr);
}

bool DragDropVisual::IsShowing()
{
    RRETURN(IsShowingCore(m_tpDragVisual.Get(), m_tpDragVisualContainer.Get()));
}


// Whether or not the DragDropVisual is active. This can become false through a call to Hide(), or when the
// target visual is removed from the tree.
bool DragDropVisual::IsShowingCore(
    _In_ xaml::IUIElement *pDragVisual,
    _In_ xaml::IUIElement *pDragVisualContainer)
{
    bool result = false;

    if (m_isShowing)
    {
        if (pDragVisualContainer)
        {
            result = !!IsRendererActive(pDragVisual);
        }
        else if (m_useSystemDefaultVisual)
        {
            result = true;
        }
    }

    return result;
}

// Returns TRUE if our renderer is active.
bool DragDropVisual::IsRendererActive(_In_ xaml::IUIElement *pDragVisual)
{
    CUIElement* pDragVisualCore = static_cast<CUIElement*>(static_cast<UIElement *>(pDragVisual)->GetHandle());
    return pDragVisualCore ? CoreImports::DragDrop_IsVisualActive(pDragVisualCore) : FALSE;
}

// Start drawing the drag visual on the TransitionRoot.
_Check_return_ HRESULT DragDropVisual::Show()
{
    HRESULT hr = S_OK;

    DependencyObject *pDragIconContainerAsDO = NULL;
    CUIElement* pDragVisualContainer = NULL;

    if (!m_isShowing)
    {
        if (!m_useSystemDefaultVisual)
        {
            DXamlCore* pCore = DXamlCore::GetCurrent();
            ctl::ComPtr<IGeneralTransform> spToRoot = nullptr;
            ctl::ComPtr<ITransform> spToRootAsITransform = nullptr;
            wf::Point offsetPositionInRoot = { -m_offsetPosition.X, -m_offsetPosition.Y };
            wf::Point originInRoot = { 0, 0 };

            ASSERT(m_tpDragVisual.Get());

            m_tpDragVisualContainerTransform.Clear();

            IFC(CoreImports::LayoutTransitionElement_Create(
                pCore->GetHandle(),
                m_tpDragVisual.Cast<UIElement>()->GetHandle(),
                nullptr /* pParentElement */,
                true /* isAbsolutelyPositioned */,
                &pDragVisualContainer));

            IFC(pCore->GetPeer(pDragVisualContainer, KnownTypeIndex::UIElement, &pDragIconContainerAsDO));
            IFC(SetPtrValueWithQI(m_tpDragVisualContainer, pDragIconContainerAsDO));

            // get the transform from the core
            {
                CUIElement* dragVisualCore = static_cast<CUIElement*>(m_tpDragVisual.Cast<UIElement>()->GetHandle());
                xref_ptr<CGeneralTransform> generalTransformCore;

                IFC(dragVisualCore->TransformToVisual(nullptr, true /*ignore3D*/, &generalTransformCore));
                IFC(CValueBoxer::ConvertToFramework(generalTransformCore.get(), spToRoot.ReleaseAndGetAddressOf(), /* fReleaseCoreValue */ FALSE));
            }

            IFC(SetPtrValueWithQI(m_tpDragVisualContainerTransform, spToRoot.Get()));
            IFC(spToRoot.As<ITransform>(&spToRootAsITransform));

            IFC(m_tpDragVisualContainer->put_RenderTransform(spToRootAsITransform.Get()));
            IFC(m_tpDragVisualContainer.Cast<UIElement>()->put_IsHitTestVisible(FALSE));

            // Transform to the window's coordinate system.
            IFC(spToRoot->TransformPoint(offsetPositionInRoot, &offsetPositionInRoot));
            IFC(spToRoot->TransformPoint(originInRoot, &originInRoot));

            m_offsetPositionInRoot.X = originInRoot.X - offsetPositionInRoot.X;
            m_offsetPositionInRoot.Y = originInRoot.Y - offsetPositionInRoot.Y;

            IFC(UpdateTransform());

            // Update the opacity (someone may have called SetOpacity before we were shown).
            IFC(SetOpacity(m_opacity));
        }
        m_isShowing = true;
    }

Cleanup:
    ReleaseInterface(pDragVisualContainer);
    ctl::release_interface(pDragIconContainerAsDO);
    RRETURN(hr);
}

// Stops drawing this visual on the TransitionRoot layer.
// NOTE: This method is not safe to be called from the destructor, or from
// any destructor path
_Check_return_ HRESULT DragDropVisual::Hide()
{
    RRETURN(HideCore(m_tpDragVisual.Get(), m_tpDragVisualContainer.Get()));
}

_Check_return_ HRESULT DragDropVisual::HideCore(
    _In_ xaml::IUIElement *pDragVisual,
    _In_ xaml::IUIElement *pDragVisualContainer)
{
    HRESULT hr = S_OK;

    if (!m_useSystemDefaultVisual && IsShowingCore(pDragVisual, pDragVisualContainer))
    {
        ASSERT(pDragVisual);
        auto *pCoreDragVisual = static_cast<UIElement*>(pDragVisual)->GetHandle();
        auto *pCoreDragVisualContainer = static_cast<UIElement*>(pDragVisualContainer)->GetHandle();

        IFC(CoreImports::LayoutTransitionElement_Destroy(
            DXamlCore::GetCurrent()->GetHandle(),
            pCoreDragVisual,
            nullptr /*pParentElement */,
            pCoreDragVisualContainer));
    }

Cleanup:
    m_isShowing = false;
    m_tpDragVisualContainerTransform.Clear();
    m_tpDragVisualContainer.Clear();

    RRETURN(hr);
}

// Sets the opacity of this DragDropVisual. Usually this is not needed,
// as the inner content takes care of its own opacity. However, chromed controls
// require this as they don't have the required number of UIElement layers.
_Check_return_ HRESULT DragDropVisual::SetOpacity(_In_ XFLOAT opacity)
{
    HRESULT hr = S_OK;

    m_opacity = opacity;

    if (m_tpDragVisualContainer)
    {
        IFC(m_tpDragVisualContainer->put_Opacity(opacity));
    }

Cleanup:
    RRETURN(hr);
}

// Updates the transform on our container. Applies m_transformedOffsetPosition as necessary.
_Check_return_ HRESULT DragDropVisual::UpdateTransform()
{
    HRESULT hr = S_OK;
    xaml_media::Matrix matrix;

    if (m_tpDragVisualContainerTransform)
    {
        ASSERT(!m_useSystemDefaultVisual);
        IFC(m_tpDragVisualContainerTransform->get_Matrix(&matrix));

        matrix.OffsetX = m_offsetPositionInRoot.X + m_currentPosition.X;
        matrix.OffsetY = m_offsetPositionInRoot.Y + m_currentPosition.Y;

        IFC(m_tpDragVisualContainerTransform->put_Matrix(matrix));
    }

Cleanup:
    RRETURN(hr);
}
