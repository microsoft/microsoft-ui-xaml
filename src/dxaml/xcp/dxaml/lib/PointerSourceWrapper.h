// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <memory>

class PointerSourceWrapper : public std::enable_shared_from_this<PointerSourceWrapper>
{
public:
    // Normally the hoverPointerSourceElement is the core counterpart to the DXaml UIElement. The exception is if the DXaml element
    // is the root scroll viewer. In that case we get the hover pointer source from the CRootVisual instead. This lets us receive
    // input from other roots like the full window media root, which is a sibling visual to the root scroll viewer.
    _Check_return_ HRESULT Initialize(_In_ CUIElement* hoverPointerSourceElement);

    WUComp::ICompositionObject* GetPointerSourceProxy() const
    {
        return m_pointerSourceProxyCO.Get();
    }

private:
    Microsoft::WRL::ComPtr<WUComp::ICompositionPropertySet> m_pointerSourceProxy;
    Microsoft::WRL::ComPtr<WUComp::ICompositionObject> m_pointerSourceProxyCO;
    Microsoft::WRL::ComPtr<WUComp::ICompositionObject> m_realPointerSourceCO;
    Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> m_realPointerPointAnimation;
};
