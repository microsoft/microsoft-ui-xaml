// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CDependencyObject.h>
#include <PCRenderDataList.h>

#include <microsoft.ui.composition.h>

class CCoreServices;
class CUIElement;

class CXamlLight : public CDependencyObject
{
private:
    CXamlLight(_In_ CCoreServices* core)
        : CDependencyObject(core)
        , m_needsSubtreeRewalk(false)
        , m_wasWUCLightClosed(false)
    {}

    _Check_return_ HRESULT HandleROEClosed(HRESULT hr);

public:
    DECLARE_CREATE(CXamlLight);

    KnownTypeIndex GetTypeIndex() const override;

    // Adds the specified visual to the list of WUC visuals targeted by the WUC light. Also bookkeep that the visual came from
    // the specified element.
    void AddTargetVisual(_In_ CUIElement* target, _In_ WUComp::IVisual* visual);

    // Removes one visual associated with the specified element from the light target list. It's possible that the visual isn't
    // actually targeted by this light, in which case we no-op. Returns whether a visual was actually removed.
    bool RemoveTargetVisual(_In_ CUIElement* target, _In_ WUComp::IVisual* visual);

    // Removes all visuals associated with the specified element from the light target list.
    void RemoveTargetElement(_In_ CUIElement* target);

    void RemoveAllTargetElements();

    bool TargetsElement(_In_ CUIElement* element);

    void SetCoordinateSpace(_In_opt_ WUComp::IVisual* visual);

    bool HasWUCLight() const;
    wrl::ComPtr<WUComp::ICompositionLight> GetWUCLight();
    void SetWUCLight(WUComp::ICompositionLight* wucLight);

    const xstring_ptr& GetLightId();

    bool IsEnabledInXamlIsland();

protected:
    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) override;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) override;

public:
    // If a XamlLight just entered the tree or got a new CompositionLight, it needs to trigger a full rewalk of its subtree on
    // the next frame. That walk will pick up all targets of the light. Note that this is just a walk, and does not force any
    // of the contents dirty.
    bool m_needsSubtreeRewalk : 1;

    // Debug info flag - if the app closed the WUC CompositionLight while the XamlLight was still using it, we'll hit RO_E_CLOSED
    // when we call into the WUC light. In that case we ignore the error, release the WUC light, and mark down this flag. This
    // flag gets reset when a new WUC light is set.
    bool m_wasWUCLightClosed : 1;

    // A list of WUC visuals that are targeted by the WUC light inside this CXamlLight, keyed by the UIElements that created the
    // visual. Updated by the render walk when it connects the lights with their targets. When this light leaves the tree, all
    // CUIElements inside are notified that they're no longer being targeted by this light. Also updated as we recycle visuals
    // inside the visual content renderer.
    containers::vector_map<
        CUIElement*,
        std::unordered_set<WUComp::IVisual*>
        > m_targetedVisuals;

private:
    // The list of WUC visuals targeted by the CompositionLight. We cache it here to handle the Leave scenario. When a XamlLight
    // leaves the tree, we want to go into the corresponding WUC CompositionLight and clear out its target visual list. However,
    // when we actually get the LeaveImpl call, sparse storage has already been cleaned out, which means the CompositionBrush
    // property value is no longer accessible, so we cache the target list explicitly in CXamlLight. This field gets updated
    // whenever the CompositionBrush changes.
    wrl::ComPtr<WUComp::IVisualUnorderedCollection> m_wucLightTargets;

    wrl::ComPtr<WUComp::ICompositionLight> m_wucLight;

    // The cached ID of the light. The cache is reset when a new WUC light is set.
    xstring_ptr m_lightId;
};
