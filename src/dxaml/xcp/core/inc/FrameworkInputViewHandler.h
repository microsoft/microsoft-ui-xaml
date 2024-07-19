// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <fwd/windows.ui.viewmanagement.h>

#include <frameworkinputview_min.h>
#include <FrameworkUdk/FrameworkInputView.h> //IFrameworkInputViewOcculusionsChangedListener

class CFocusManager;

class CFrameworkInputViewHandler final : public CXcpObjectBase<IObject>, 
                                         public udk_::IFrameworkInputViewOcculusionsChangedListener
{
public:

    CFrameworkInputViewHandler(_In_ CFocusManager* focusManager) :
        m_focusManager(focusManager)
    {
    }

    ~CFrameworkInputViewHandler() override;

    _Check_return_ HRESULT Initialize();

    HRESULT STDMETHODCALLTYPE OnFrameworkInputViewOcclusionsChanged(
        _In_ boolean handled,
        _In_ wfc::IVectorView<wuv::Core::CoreInputViewOcclusion*> *pvOcclusions) override;

    
private:    
    CFocusManager* m_focusManager = nullptr;

    wrl::ComPtr<wuv::Core::ICoreInputView> m_spCoreInputView;
};
