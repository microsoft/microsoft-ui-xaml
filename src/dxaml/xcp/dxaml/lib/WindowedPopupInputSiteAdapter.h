// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <fwd/windows.ui.viewmanagement.h>
#include <fwd/windows.ui.core.h>
#include <fwd/microsoft.ui.xaml.h>
#include <microsoft.ui.input.experimental.h>
#include "NamespaceAliases.h"

class CJupiterWindow;
class CContentRoot;
class InputSiteAdapter;
class CPopup;

namespace DirectUI {class PointerPointTransform;}

class WindowedPopupInputSiteAdapter : public InputSiteAdapter
{
public:
    void Initialize(_In_ CPopup* popup, _In_ ixp::IContentIsland* contentIsland, _In_ CContentRoot* contentRoot, _In_ CJupiterWindow* jupiterWindow);

    _Check_return_ HRESULT SetTransformFromContentRoot(_In_ xaml_media::IGeneralTransform* transform, _In_ wf::Point* offset);

    bool ReplayPointerUpdate() override;

protected:
    _Check_return_ HRESULT OnDirectManipulationHitTest(_In_ ixp::IPointerEventArgs* args) override;
    _Check_return_ HRESULT OnPointerMessage(const UINT uMsg, _In_ ixp::IPointerEventArgs* args) override;

private:
    _Check_return_ HRESULT GetTransformedPointerPoint(_In_ ixp::IPointerEventArgs* args, _Out_ ixp::IPointerPoint** transformedPointerPoint);

    ctl::ComPtr<DirectUI::PointerPointTransform> m_pointerPointTransformFromContentRoot;

    // This popup controls the lifetime of this WindowedPopupInputSiteAdapter, and we'll never outlive the popup.
    CPopup* m_windowedPopupNoRef;
};