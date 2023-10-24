// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FrameworkInputViewHandler.h"
#include "InputServices.h"
#include "RectUtil.h"
#include <DXamlServices.h>

CFrameworkInputViewHandler::~CFrameworkInputViewHandler()
{
    if (m_spCoreInputView)
    {
        VERIFYHR(FrameworkInputView_UnregisterListener(m_spCoreInputView.Get(), this));
    }
}

HRESULT CFrameworkInputViewHandler::Initialize()
{
    // If we're in the context of XAML islands, then we don't want to use GetForCurrentView -
    // that requires CoreWindow, which is not supported in islands.
    if (DirectUI::DXamlServices::GetHandle()->GetInitializationType() != InitializationType::IslandsOnly)
    {
        wrl::ComPtr<wuv::Core::ICoreInputViewStatics> spInputViewStatics;

        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_Core_CoreInputView).Get(),
            &spInputViewStatics));

        IFC_RETURN(spInputViewStatics->GetForCurrentView(&m_spCoreInputView));

        // listen to FrameworkOcclusionsChanged
        IFC_RETURN(FrameworkInputView_RegisterListener(m_spCoreInputView.Get(), this));
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CFrameworkInputViewHandler::OnFrameworkInputViewOcclusionsChanged(
    _In_ boolean handled,
    _In_ wfc::IVectorView<wuv::Core::CoreInputViewOcclusion*> *pOcclusions)
{
    // do nothing if App has already handled OccludingInputViewChanged event
    if (handled)
    {
        return S_OK;
    }

    CDependencyObject* pFocusedElement = m_focusManager->GetFocusedElementNoRef();
    if (!CInputServices::IsTextEditableControl(pFocusedElement))
    {
        return S_OK;
    }

    UINT uiOcclusionCount = 0;
    wf::Rect finalRect = DirectUI::RectUtil::CreateEmptyRect();

    IFC_RETURN(pOcclusions->get_Size(&uiOcclusionCount));

    for (UINT32 i = 0; i < uiOcclusionCount; i++)
    {
        wrl::ComPtr<wuv::Core::ICoreInputViewOcclusion> currentOcclusion;
        IFC_RETURN(pOcclusions->GetAt(i, &currentOcclusion));
        wuv::Core::CoreInputViewOcclusionKind currentOcclusionKind;
        IFC_RETURN(currentOcclusion->get_OcclusionKind(&currentOcclusionKind));
        if (currentOcclusionKind == wuv::Core::CoreInputViewOcclusionKind::CoreInputViewOcclusionKind_Overlay)
        {
            wf::Rect currentOccludingRect = DirectUI::RectUtil::CreateEmptyRect();
            IFC_RETURN(currentOcclusion->get_OccludingRect(&currentOccludingRect));
            IFC_RETURN(DirectUI::RectUtil::Union(finalRect, currentOccludingRect));
        }
    }

    // we may get empty rect at this point, but that is expected for scenario such as IME window closing
    BOOLEAN isEmpty = FALSE;
    IFC_RETURN(DirectUI::RectUtil::GetIsEmpty(finalRect, &isEmpty));

    CTextBoxBase *pTextControl = do_pointer_cast<CTextBoxBase>(pFocusedElement);
    if (isEmpty)
    {
        XRECTF dummy {};
        IFC_RETURN(pTextControl->RaiseCandidateWindowBoundsChangedEvent(dummy));
    }
    else
    {
        RECT rectConvert;
        rectConvert.left = static_cast<LONG>(finalRect.X);
        rectConvert.top = static_cast<LONG>(finalRect.Y);
        rectConvert.right = static_cast<LONG>(finalRect.X + finalRect.Width);
        rectConvert.bottom = static_cast<LONG>(finalRect.Y + finalRect.Height);
        IFC_RETURN(pTextControl->RaiseCandidateWindowBoundsChangedEventForDIPs(&rectConvert));
    }

    return S_OK;
}
