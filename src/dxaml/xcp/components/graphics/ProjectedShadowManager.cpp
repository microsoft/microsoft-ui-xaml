// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ProjectedShadowManager.h"
#include <DOPointerCast.h>
#include <UIElement.h>
#include <Framework.h>
#include <corep.h>
#include <Border.h>
#include <Popup.h>
#include <Brush.h>
#include <DCompTreeHost.h>
#include <ThemeShadow.h>
#include <CValueBoxer.h>
#include "Value.h"
#include "DXamlServices.h"
#include "CValueBoxer.h"
#include <CDependencyObject.h>
#include <MetadataAPI.h>
#include <RuntimeEnabledFeatures.h>
#include <LayoutTransitionElement.h>
#include <RootVisual.h>
#include <ContentRoot.h>
#include <ContentRootCoordinator.h>

#include <microsoft.ui.composition.internal.h>
#include <microsoft.ui.composition.private.h>

const float ProjectedShadowManager::sc_ambientOpacityMin = 0.11f;
const float ProjectedShadowManager::sc_ambientOpacityMax = 0.18f;
const float ProjectedShadowManager::sc_ambientOpacityFalloff = 192.0f;
const float ProjectedShadowManager::sc_ambientBlurMin = 0.0f;
const float ProjectedShadowManager::sc_ambientBlurMax = 86.0f;
const float ProjectedShadowManager::sc_ambientBlurMultiplier = 0.225f;
const float ProjectedShadowManager::sc_directionalOpacityMin = 0.13f;
const float ProjectedShadowManager::sc_directionalOpacityMax = 0.22f;
const float ProjectedShadowManager::sc_directionalOpacityFalloff = 192.0f;
const float ProjectedShadowManager::sc_directionalBlurMin = 0.0f;
const float ProjectedShadowManager::sc_directionalBlurMax = 500.0f;
const float ProjectedShadowManager::sc_directionalBlurMultiplier = 0.9f;
const wfn::Vector3 sc_ambientLightDirection = { 0.0f, 0.02f, -1.0f };
const wfn::Vector3 sc_directionalLightDirection = { 0.0f, 0.4f, -1.0f };

WUComp::IVisual* ElementShadowSource::GetVisualNoRef() const
{
    WUComp::IVisual* visual = nullptr;
    CUIElement* element = m_wrElement.lock_noref();

    if (element)
    {
        visual = element->GetShadowVisualNoRef();
    }

    return visual;
}

ProjectedShadowManager::ProjectedShadowManager(DCompTreeHost* dcompTreeHost)
    : m_dcompTreeHostNoRef(dcompTreeHost) 
{
}

ProjectedShadowManager::~ProjectedShadowManager()
{
    if (m_effectPolicyHelper != nullptr)
    {
        // This object is meant to control the lifetime of the EffectPolicyHelper, except the EffectPolicyHelper might
        // be temporarily kept alive by an ExecuteOnUIThread call in response to a system setting change. Disconnect
        // the callback in the EffectPolicyHelper so it doesn't notify this deleted ProjectedShadowManager.
        m_effectPolicyHelper->ClearCallback(this);
    }
}

ThemeShadowScene::~ThemeShadowScene()
{
    ClearCasters();
    ClearReceivers();
}

ThemeShadowGlobalScene::~ThemeShadowGlobalScene()
{
    m_popupCasters.clear();
}

void ThemeShadowScene::EnsureInitialized(_In_opt_ CXamlIslandRoot* xamlIslandRoot)
{
    if (!m_isInitialized)
    {
        SetupLights(xamlIslandRoot);

        wrl::ComPtr<WUComp::ICompositorWithProjectedShadow> compositorProjectedShadow;
        IFCFAILFAST(m_dcompTreeHostNoRef->GetCompositor()->QueryInterface(IID_PPV_ARGS(&compositorProjectedShadow)));

        // Create ProjectedShadowScene 1
        IFCFAILFAST(compositorProjectedShadow->CreateProjectedShadow(&m_projectedShadowScene1));
        IFCFAILFAST(m_projectedShadowScene1->put_LightSource(m_light1.Get()));

        // Create ProjectedShadowScene 2
        IFCFAILFAST(compositorProjectedShadow->CreateProjectedShadow(&m_projectedShadowScene2));
        IFCFAILFAST(m_projectedShadowScene2->put_LightSource(m_light2.Get()));

        ProjectedShadowManager::ConfigureScenePair(m_projectedShadowScene1.Get(), m_projectedShadowScene2.Get());

        m_isInitialized = true;
    }
}

void ThemeShadowScene::SetupLights(_In_opt_ CXamlIslandRoot* xamlIslandRoot)
{
    wrl::ComPtr<WUComp::ICompositor2> compositor2;
    IFCFAILFAST(m_dcompTreeHostNoRef->GetCompositor()->QueryInterface(IID_PPV_ARGS(&compositor2)));

    // Configure Light 1
    wrl::ComPtr<WUComp::IDistantLight> distantLight1;
    IFCFAILFAST(compositor2->CreateDistantLight(&distantLight1));
    IFCFAILFAST(distantLight1.As(&m_light1));
    IFCFAILFAST(distantLight1->put_Direction(sc_ambientLightDirection));

    // Configure Light 2
    wrl::ComPtr<WUComp::IDistantLight> distantLight2;
    IFCFAILFAST(compositor2->CreateDistantLight(&distantLight2));
    IFCFAILFAST(distantLight2.As(&m_light2));
    IFCFAILFAST(distantLight2->put_Direction(sc_directionalLightDirection));

    // For normal CoreWindow hosting, use the HWND RootVisual as coordinate space.
    // For islands, m_dcompTreeHostNoRef->GetHwndRootVisual() returns null.
    // Instead, use a visual from XamlIslandRoot as coordinate space.

    wrl::ComPtr<WUComp::IVisual> referenceVisual;

    if (xamlIslandRoot)
    {
        // In test scenarios we want the mock root visual here, so get it directly from the CXamlIslandRoot rather than
        // go through the island. The island will return a real visual.
        referenceVisual = xamlIslandRoot->GetRootVisual();
    }
    else
    {
        wrl::ComPtr<WUComp::IContainerVisual> rootVisual(m_dcompTreeHostNoRef->GetHwndRootVisual());
        IFCFAILFAST(rootVisual.As(&referenceVisual));
    }

    IFCFAILFAST(distantLight1->put_CoordinateSpace(referenceVisual.Get()));
    IFCFAILFAST(distantLight2->put_CoordinateSpace(referenceVisual.Get()));
}

void ThemeShadowScene::AddNewCaster(
    SourceType sourceType,
    _In_ WUComp::IVisual* visual,
    _In_ CUIElement* element,
    _In_ CPopup* containingPopup,
    _In_opt_ WUComp::ICompositionBrush* maskBrush)
{
    ASSERT(m_dcompTreeHostNoRef->GetProjectedShadowManager()->GetHasEverHadCaster());

    if (GetType() == SceneType::Global && sourceType == SourceType::Element)
    {
        // Track Popup -> Contained caster association and enforce One Caster per Popup rule.
        auto& popupCastersMap = static_cast<ThemeShadowGlobalScene*>(this)->m_popupCasters;
        auto emplaceResult = popupCastersMap.emplace(xref::get_weakref(containingPopup), xref::get_weakref(element));
        bool replacingLTE = false;
        bool replacingNonExistingCaster = false;

        if (!emplaceResult.second)
        {
            CUIElement* currentElement = static_cast<CUIElement*>(emplaceResult.first->second.lock_noref());
            if (currentElement == nullptr)
            {
                //Bug 19631750: sometimes the caster associated with the popup is deleted or in a
                //bad state, and could not be retrieved. We replace this corrupted caster with the new
                //one that comes in
                popupCastersMap[xref::get_weakref(containingPopup)] = xref::get_weakref(element);
                replacingNonExistingCaster = true;
            }
            else if (currentElement->OfTypeByIndex<KnownTypeIndex::LayoutTransitionElement>())
            {
                CLayoutTransitionElement* lte = static_cast<CLayoutTransitionElement*>(currentElement);
                CUIElement* lteTarget = lte->GetTargetElement();

                // If the Popup is associated with an LTE targeting this UIElement, allow the UIElement to take LTE's place.
                if (element == lteTarget)
                {
                    popupCastersMap[xref::get_weakref(containingPopup)] = xref::get_weakref(element);
                    replacingLTE = true;
                }
            }
        }

        ASSERT(emplaceResult.second || replacingLTE || replacingNonExistingCaster);
    }

    xref_ptr<ShadowCasterPair> newCaster;

    if (sourceType == SourceType::Visual)
    {
        ASSERT(visual);
        newCaster.attach(new ShadowCasterPair(visual, maskBrush));
    }
    else
    {
        ASSERT(element);
        newCaster.attach(new ShadowCasterPair(element, maskBrush));
    }

    m_addedShadowCasters.push_back(newCaster);
    SetCastersDirty(true);
}

void ProjectedShadowManager::ReleaseResources()
{
    // Between tests we shut down the compositor which will close any object created
    // by it, so we need to abandon all of our scenes.
    m_globalScene.reset();
    m_customScenes.clear();
    m_xamlIslandRootsGlobalScenes.clear();
    m_popupsLookingForNewCasters.clear();
    SetHasEverHadCaster(false);
}

ShadowCasterPair* ThemeShadowScene::FindShadowCasterPairByVisual(_In_ WUComp::IVisual* visual) const
{
    auto found = std::find_if(m_shadowCasters.begin(), m_shadowCasters.end(), [&](const auto& casterEntry)
    {
        return casterEntry->GetSource()->GetVisualNoRef() == visual;
    });

    return found == m_shadowCasters.end() ? nullptr : (*found).get();
}

ShadowCasterPair* ThemeShadowScene::FindFilteredShadowCasterPairByVisual(_In_ WUComp::IVisual* visual) const
{
    auto found = std::find_if(m_filteredShadowCasters.begin(), m_filteredShadowCasters.end(), [&](const auto& casterEntry)
    {
        return casterEntry->GetSource()->GetVisualNoRef() == visual;
    });

    return found == m_filteredShadowCasters.end() ? nullptr : (*found).get();
}

ShadowCasterPair* ThemeShadowScene::FindShadowCasterPairByElement(_In_ CUIElement* element) const
{
    auto found = std::find_if(m_shadowCasters.begin(), m_shadowCasters.end(), [&](const auto& casterEntry)
    {
        return casterEntry->GetSource()->GetType() == SourceType::Element &&
               casterEntry->GetSource()->GetElementNoRef() == element;
    });

    return found == m_shadowCasters.end() ? nullptr : (*found).get();
}

ShadowCasterPair* ThemeShadowScene::FindFilteredShadowCasterPairByElement(_In_ CUIElement* element) const
{
    auto found = std::find_if(m_filteredShadowCasters.begin(), m_filteredShadowCasters.end(), [&](const auto& casterEntry)
    {
        return casterEntry->GetSource()->GetType() == SourceType::Element &&
               casterEntry->GetSource()->GetElementNoRef() == element;
    });

    return found == m_filteredShadowCasters.end() ? nullptr : (*found).get();
}

bool ThemeShadowScene::RemoveCaster(_In_ WUComp::IVisual* visual)
{
    ProjectedShadowManager* projectedShadowManager = m_dcompTreeHostNoRef->GetProjectedShadowManager();
    if (!projectedShadowManager->GetHasEverHadCaster())
    {
        return false;
    }

    bool result = false;

    // Try to remove from local casters
    bool isFilteredCaster = false;
    xref_ptr<ShadowCasterPair> casterEntry(FindShadowCasterPairByVisual(visual));
    if (!casterEntry)
    {
        // Check filtered casters too.
        // For these, removal path is run with modification (since they are not in the WUC scene).
        casterEntry = FindFilteredShadowCasterPairByVisual(visual);
        isFilteredCaster = !!casterEntry;
    }

    if (casterEntry)
    {
        CUIElement* casterElement = casterEntry->GetSource()->GetType() == SourceType::Element ? casterEntry->GetSource()->GetElementNoRef() : nullptr;

        if (isFilteredCaster)
        {
            // Filtered caster does not need to be processed in UpdateCasters(), finish its cleanup here.
            m_filteredShadowCasters.remove(casterEntry);

            // TODO_Shadows: Should this be done at the time element gets filtered?
            casterElement->SetShadowVisual(nullptr);
        }
        else
        {
            m_removedShadowCasters.push_back(casterEntry);
            m_shadowCasters.remove(casterEntry);
            SetCastersDirty(true);
        }

        // If global scene, Remove Popup -> Contained caster association
        if (GetType() == SceneType::Global && casterEntry->GetSource()->GetType() == SourceType::Element)
        {
            auto& popupCastersMap = static_cast<ThemeShadowGlobalScene*>(this)->m_popupCasters;
            auto it = popupCastersMap.begin();
            while (it != popupCastersMap.end())
            {
                CUIElement* casterInMap = it->second.lock_noref();
                if ((casterElement && casterInMap && casterInMap == casterElement) ||
                    !casterInMap)
                {
                    //Task 19130750: If there is a removed caster,
                    //we dirty the whole popup subtree because there may
                    //be ignored elements with shadow property set
                    if (projectedShadowManager->HasEverHadIgnoredCasters())
                    {
                        projectedShadowManager->AddToPopupsLookingForNewCastersList(it->first.lock_noref());
                    }
                    it = popupCastersMap.erase(it);
                }
                else
                {
                    it++;
                }
            }
        }

        result  = true;
    }

    return result;
}

bool ProjectedShadowManager::RemoveCaster(_In_ WUComp::IVisual* visual)
{
    if (!GetHasEverHadCaster()) { return false; }

    bool result = false;

    // First try to remove from Global scene
    if (m_globalScene)
    {
       result = m_globalScene->RemoveCaster(visual);
    }

    for (auto& xamlIslandGlobalScenePair : m_xamlIslandRootsGlobalScenes)
    {
        xamlIslandGlobalScenePair.second->RemoveCaster(visual);
    }

    // Next try to remove from Custom scenes
    if (!result)
    {
        for (auto& customScene : m_customScenes)
        {
            if (customScene->RemoveCaster(visual))
            {
                result = true;
                break;
            }
        }
    }

    return result;
}

bool ProjectedShadowManager::AreCasterListsEmpty(_In_ ThemeShadowGlobalScene* globalScene)
{
    return globalScene->m_shadowCasters.size() == 0 &&
           globalScene->m_addedShadowCasters.size() == 0 &&
           globalScene->m_removedShadowCasters.size() == 0 &&
           globalScene->m_filteredShadowCasters.size() == 0;
}

// Used only by policy-triggered fallback
void ProjectedShadowManager::ClearGlobalScenes()
{
    ASSERT(m_globalScenesNeedClearing);
    m_globalScene = nullptr;
    m_xamlIslandRootsGlobalScenes.clear();
    m_globalScenesNeedClearing = false;
}

void ProjectedShadowManager::EnsureEffectPolicyHelper(_In_ CCoreServices* coreServices)
{
    if (m_effectPolicyHelper == nullptr)
    {
        m_effectPolicyHelper.attach(new EffectPolicyHelper(coreServices, this));
    }
}

void ProjectedShadowManager::UpdateCasterStatus(_In_ CUIElement* uielement)
{
    // Note: EffectPolicyHelper uses IAccessibilitySettings to determine whether high contrast mode is enabled. Its
    // add_HighContrastChanged API is needed to be notified when high contrast mode changes, and this API requires
    // a CoreWindow to already be initialized. So we delay initializing EffectPolicyHelper (and accessing IAccessibilitySettings)
    // to now to ensure a CoreWindow is available.
    EnsureEffectPolicyHelper(uielement->GetContext());

    if (!AreShadowsEnabled())
    {
        return;
    }

    // RS5 Bug #17622840:  While an LTE is targeting an element with a ThemeShadow set, the target element will not have
    // a CompNode, and therefore cannot cast its shadows.  The fix is to use the LTE's visual to stand in for the target.
    // When the LTE goes away the target will gain its CompNode and shadows will transfer over to the target's visual.
    CUIElement* casterElement = uielement;
    if (uielement->OfTypeByIndex<KnownTypeIndex::LayoutTransitionElement>())
    {
        CLayoutTransitionElement* lte = static_cast<CLayoutTransitionElement*>(uielement);
        casterElement = lte->GetTargetElement();
    }

    // Popup's ThemeShadow is processed in Child's UpdateCasterStatus() call
    if (casterElement->OfTypeByIndex<KnownTypeIndex::Popup>())
    {
        return;
    }

    CValue shadowValue;
    IFCFAILFAST(casterElement->GetValueByIndex(KnownPropertyIndex::UIElement_Shadow, &shadowValue));
    xref_ptr<CThemeShadow> themeShadow(do_pointer_cast<CThemeShadow>(shadowValue.AsObject()));

    CUIElement* parent = casterElement->GetUIElementAdjustedParentInternal();
    bool isPopupChild = parent && parent->OfTypeByIndex<KnownTypeIndex::Popup>();

    bool noShadowOnThisAndPreviousFrame =
        themeShadow == nullptr &&                                                 // No shadow on this frame
        (!GetHasEverHadCaster() || !casterElement->GetShadowVisualNoRef()) &&     // No shadow on previous frame
        !isPopupChild;                                                            // Exception: Popup.Child can also take shadow from Popup,
                                                                                  // so determining its shadow status is complex. Assume shadows exist for this early-out check.

    if (noShadowOnThisAndPreviousFrame)
    {
        return;
    }

    CPopup* containingPopup = nullptr;
    if (isPopupChild)
    {
        containingPopup = static_cast<CPopup*>(parent);
    }

    // If the element has ThemeShadow, determine if it goes into global scene. There are 3 rules:
    // 1. Caster cannot specify Receivers
    // 2. Caster is inside a Popup
    // 3. There is no other caster without receivers in same Popup
    bool hasShadow = themeShadow != nullptr;
    bool hasReceivers = hasShadow && themeShadow->HasCustomReceivers();
    bool addToGlobalScene = false;
    if (hasShadow && !hasReceivers)
    {
        // The element is not a direct child of Popup, but is it still a descendant?
        CUIElement* rootOfPopupSubtree = static_cast<CUIElement*>(casterElement->GetRootOfPopupSubTree());
        bool isPopupDescendant = rootOfPopupSubtree != nullptr;
        if (isPopupDescendant)
        {
            CUIElement* uielementAdjustedParent = rootOfPopupSubtree->GetUIElementAdjustedParentInternal();
            ASSERT(uielementAdjustedParent->OfTypeByIndex<KnownTypeIndex::Popup>());
            containingPopup = static_cast<CPopup*>(uielementAdjustedParent);
        }

        if (containingPopup)
        {
            // If there is already a global caster inside this Popup, ignore this one.
            if (DoesPopupHaveGlobalCaster(containingPopup, uielement))
            {
                return;
            }

            addToGlobalScene = true;
        }
    }

    // Special handling if ThemeShadow is set on Popup - when Popup's ThemeShadow is set, it is applied to the child (since Popup itself does not have a size).
    // If this is a Popup.Child, check for ThemeShadow on the Popup.
    // In case Popup and Popup.Child both set it, respect Child's ThemeShadow.
    if (!hasShadow && isPopupChild)
    {
        CValue value;
        IFCFAILFAST(parent->GetValueByIndex(KnownPropertyIndex::UIElement_Shadow, &value));
        xref_ptr<CThemeShadow> popupThemeShadow(do_pointer_cast<CThemeShadow>(value.AsObject()));

        bool popupHasShadow = do_pointer_cast<CShadow>(value.AsObject()) != nullptr;
        if (popupHasShadow)
        {
            bool popupHasReceivers = popupThemeShadow->HasCustomReceivers();
            if (!popupHasReceivers)
            {
                ASSERT(containingPopup != nullptr);

                // If there is already a global caster inside this Popup, ignore this one.
                if (DoesPopupHaveGlobalCaster(containingPopup, uielement))
                {
                    return;
                }

                addToGlobalScene = true;
            }

            // Use Popup's ThemeShadow to configure caster Child
            themeShadow = popupThemeShadow;
            hasReceivers = popupHasReceivers;
        }
    }

    // If adding to local scene, make sure receivers are set
    if (themeShadow != nullptr && !addToGlobalScene && !hasReceivers)
    {
        return;
    }

    CXamlIslandRoot* xamlIslandRoot = do_pointer_cast<CXamlIslandRoot>(uielement->GetContext()->GetRootForElement(uielement));

    UpdateCasterStatus(
        uielement->GetContext(),
        SourceType::Element,
        uielement->GetShadowVisualNoRef(),
        uielement,
        themeShadow.get(),
        addToGlobalScene,
        containingPopup,
        xamlIslandRoot);
}

void ProjectedShadowManager::UpdateCasterStatus(
    _In_ CCoreServices* core,
    SourceType sourceType,
    _In_ WUComp::IVisual* visual,
    _In_opt_ CUIElement* uielement,
    _In_opt_ CThemeShadow* themeShadow,
    bool addToGlobalScene,
    _In_opt_ CPopup* containingPopup,
    _In_ CXamlIslandRoot* xamlIslandRoot)
{
    if (!AreShadowsEnabled())
    {
        return;
    }

    ThemeShadowScene* scene = nullptr;

    m_core = core;

    if (themeShadow)
    {
        // If shadow is set now, make sure we have an appropriate scene.
        SetHasEverHadCaster(true);
        scene = EnsureScene(themeShadow, addToGlobalScene, xamlIslandRoot);
    }
    else if (GetHasEverHadCaster())
    {
        //TO DO: what should we do for global scenes associated with xamlIslandRoots ?
        // If shadow is not set and we've had casters, it might have just been removed, and needs to be updated.
        scene = GetSceneForVisual(visual);
    }

    // No scene associated with this element needs updating
    if (!scene)
    {
        return;
    }

    bool isFilteredCaster = false;
    xref_ptr<ShadowCasterPair> casterEntry(scene->FindShadowCasterPairByVisual(visual));
    if (!casterEntry)
    {
        // Check filtered casters too.
        // For these, skip any add/update work (which could lead to duplicate entries), but allow removal so they can get cleaned up.
        casterEntry = scene->FindFilteredShadowCasterPairByVisual(visual);
        isFilteredCaster = !!casterEntry;
    }

    if (themeShadow)
    {
        if (!isFilteredCaster)
        {
            wrl::ComPtr<WUComp::ICompositionBrush> maskBrush;
            IFCFAILFAST(themeShadow->GetMask(&maskBrush));

            if (casterEntry)
            {
                // Caster is already in list, needs update if it changed (eg Mask property)
                wrl::ComPtr<WUComp::ICompositionBrush> oldMaskBrush = casterEntry->GetMask();

                if (oldMaskBrush != maskBrush)
                {
                    scene->m_shadowCasters.remove(casterEntry);
                    scene->AddNewCaster(uielement ? SourceType::Element : SourceType::Visual,
                                        visual,
                                        uielement,
                                        containingPopup,
                                        maskBrush.Get());
                }
            }
            else
            {
                // Add new caster to map
                scene->AddNewCaster(uielement ? SourceType::Element : SourceType::Visual,
                                    visual,
                                    uielement,
                                    containingPopup,
                                    maskBrush.Get());
            }
        }
    }
    else
    {
        // If UIElement does not cast a shadow, see if it is in the caster list and if so remove it
        // TODO_Shadows: Don't do this expensive check. Instead, somehow mark the primary visual for removal when UIElement switched to not being a caster.
        if (casterEntry)
        {
            VERIFY(scene->RemoveCaster(casterEntry->GetSource()->GetVisualNoRef()));
        }
    }
}

void ThemeShadowScene::UpdateCasters()
{
    if (GetCastersDirty())
    {
        wrl::ComPtr<WUComp::ICompositionProjectedShadowCasterCollection> wucCasterCollection1, wucCasterCollection2;
        IFCFAILFAST(m_projectedShadowScene1->get_Casters(&wucCasterCollection1));
        IFCFAILFAST(m_projectedShadowScene2->get_Casters(&wucCasterCollection2));

        // Process removed casters
        for (auto& removedShadowCasterPair : m_removedShadowCasters)
        {
            CasterPair removedWucCasters = removedShadowCasterPair->GetWucCasters();
            IFCFAILFAST(wucCasterCollection1->Remove(removedWucCasters.first.Get()));
            IFCFAILFAST(wucCasterCollection2->Remove(removedWucCasters.second.Get()));

            // Caster has been removed - invalidate UIElement's ShadowVisual
            const auto& source = removedShadowCasterPair->GetSource();
            if (source->GetType() == SourceType::Element && source->GetElementNoRef())
            {
                // There is a possibility that the element left and re-entered the scene on the same frame
                // (eg TextBlockIntegrationTests::VerifySelectingTextWithTouchShowsSelectionFlyout), so the
                // same element is associated with two different visuals. Since we set the ShadowVisual early and unset it late,
                // in this scenario the UIElement is already storing the right ShadowVisual. Since it is needed in
                // UpdateReceivers that will be called later in this frame, make sure to not unset it here.
                // Note that analogous handling is not needed for shared casters, since receivers are set by the uDWM in that case.
                wrl::ComPtr<WUComp::IVisual> wucCasterVisual;
                IFCFAILFAST(removedWucCasters.first.Get()->get_CastingVisual(&wucCasterVisual));
                bool elementReentered = wucCasterVisual.Get() != source->GetElementNoRef()->GetShadowVisualNoRef();
                if (!elementReentered)
                {
                    source->GetElementNoRef()->SetShadowVisual(nullptr);
                }
            }
        }
        m_removedShadowCasters.clear();

        // Process added casters
        for (auto& addedShadowCasterPair : m_addedShadowCasters)
        {
            wrl::ComPtr<WUComp::ICompositorWithProjectedShadow> compositorProjectedShadow;
            IFCFAILFAST(m_dcompTreeHostNoRef->GetCompositor()->QueryInterface(IID_PPV_ARGS(&compositorProjectedShadow)));

            wrl::ComPtr<WUComp::ICompositionProjectedShadowCaster> wucCaster1, wucCaster2;

            IFCFAILFAST(compositorProjectedShadow->CreateProjectedShadowCaster(&wucCaster1));
            IFCFAILFAST(compositorProjectedShadow->CreateProjectedShadowCaster(&wucCaster2));

            wrl::ComPtr<WUComp::ICompositionBrush> mask = addedShadowCasterPair->GetMask();

            wrl::ComPtr<WUComp::IVisual> casterVisual = addedShadowCasterPair->GetSource()->GetVisualNoRef();
            IFCFAILFAST(wucCaster1->put_CastingVisual(casterVisual.Get()));
            IFCFAILFAST(wucCaster2->put_CastingVisual(casterVisual.Get()));
            addedShadowCasterPair->SetWucCasters(wucCaster1.Get(), wucCaster2.Get());

            // ICompositionProjectedShadowCaster2 has been renamed to ICompositionProjectedShadowCasterFuture and has
            // been stripped out of the public winmds. Luckily Xaml only references it from dead code now. Commenting
            // out the dead code to avoid the build errors.
            /*
            wrl::ComPtr<WUComp::ICompositionProjectedShadowCasterFuture> wucCaster21, wucCaster22;

            IFCFAILFAST(wucCaster1->QueryInterface(IID_PPV_ARGS(&wucCaster21)));
            IFCFAILFAST(wucCaster2->QueryInterface(IID_PPV_ARGS(&wucCaster22)));

            // Ancestor clip will be respected as long as casterVisual is not null
            IFCFAILFAST(wucCaster21->put_AncestorClip(casterVisual.Get()));
            IFCFAILFAST(wucCaster22->put_AncestorClip(casterVisual.Get()));

            if (mask)
            {
                IFCFAILFAST(wucCaster21->put_Mask(mask.Get()));
                IFCFAILFAST(wucCaster22->put_Mask(mask.Get()));
            }
            */

            IFCFAILFAST(wucCasterCollection1->InsertAtTop(wucCaster1.Get()));
            IFCFAILFAST(wucCasterCollection2->InsertAtTop(wucCaster2.Get()));

            // Add caster to running list
            m_shadowCasters.push_back(addedShadowCasterPair);
        }
        m_addedShadowCasters.clear();

        SetCastersDirty(false);
    }
}

// Given a Popup, return its containing Popup parent (if any).
CPopup* ProjectedShadowManager::GetNextAncestorPopup(_In_ CPopup* popup)
{
    CUIElement* child = static_cast<CUIElement*>(popup);
    CUIElement* parent = static_cast<CUIElement*>(popup->GetParentInternal(false /*fPublic*/));
    CPopupRoot* popupRoot = popup->GetPopupRoot();
    CPopup* parentPopup = nullptr;

    while (parent)
    {
        if (parent == popupRoot)
        {
            // We're interested in rendering as part of a subtree, and the unloading child of a popup still renders
            // in that child's subtree, so look at the unloading child too.
            parentPopup = popupRoot->GetOpenPopupWithChild(child, true /* checkUnloadingChildToo */);
            break;
        }
        else
        {
            child = parent;
            parent = static_cast<CUIElement*>(child->GetParentInternal(false /*fPublic*/));
        }
    }

    return parentPopup;
}

void ThemeShadowScene::ClearCasters()
{
    wrl::ComPtr<WUComp::ICompositionProjectedShadowCasterCollection> wucCasterCollection1, wucCasterCollection2;
    IFCFAILFAST(m_projectedShadowScene1->get_Casters(&wucCasterCollection1));
    IFCFAILFAST(m_projectedShadowScene2->get_Casters(&wucCasterCollection2));

    // Bug 20448053: Re-enable asserts in ThemeShadowScene::ClearCasters()
    //ASSERT(m_removedShadowCasters.size() == 0);
    //ASSERT(m_addedShadowCasters.size() == 0);
    m_shadowCasters.clear();
    m_filteredShadowCasters.clear();
    wucCasterCollection1->RemoveAll();
    wucCasterCollection2->RemoveAll();
}

// Estimate shadow depth from the Translate.Z value set on the caster UIElement,
// since this is how shadow depth is set for all in-box Controls. If the popup is parented,
// we find all popups in the ancestor chain and accumulate the Translate.Z values.
float ThemeShadowGlobalScene::EstimateShadowDepth(CPopup* popup)
{
    float totalDepth = 0;
    CPopup* currentPopup = popup;

    while(currentPopup)
    {
        auto it = m_popupCasters.find(xref::get_weakref(currentPopup));

        // Get depth from caster element inside the popup if this is a caster, Popup.Child itself if it is only a receiver (this will typically be 0).
        CUIElement* elementWithDepth = it != m_popupCasters.end() ? it->second.lock_noref() : currentPopup->m_pChild;
        float localDepth = ProjectedShadowManager::GetTranslateZ(elementWithDepth);
        totalDepth += localDepth;

        currentPopup = ProjectedShadowManager::GetNextAncestorPopup(currentPopup);
    }

    return totalDepth;
}

// While processing receivers, we also identify any casters that would cause a "shadow on top of themselves" artifact.
// This analyzes caster-receiver pairs, and is convenient to do when all opened popups are walked for receiver determination.
void ThemeShadowGlobalScene::UpdateReceiversAndFilterCasters(_In_ CCoreServices* pCore, _In_opt_ CXamlIslandRoot* xamlIslandRoot)
{
    wrl::ComPtr<WUComp::ICompositorWithProjectedShadow> compositorProjectedShadow;
    IFCFAILFAST(m_dcompTreeHostNoRef->GetCompositor()->QueryInterface(IID_PPV_ARGS(&compositorProjectedShadow)));

    wrl::ComPtr<WUComp::ICompositionProjectedShadowReceiverUnorderedCollection> wucReceiverCollection1, wucReceiverCollection2;

    // At the end of tests we can hit RO_E_CLOSED when calling into shadow scenes. No-op for RO_E_CLOSED.
    HRESULT hr = m_projectedShadowScene1->get_Receivers(&wucReceiverCollection1);
    if (hr != RO_E_CLOSED)
    {
        IFCFAILFAST(hr);
        IFCFAILFAST(m_projectedShadowScene2->get_Receivers(&wucReceiverCollection2));

        std::vector<xref_ptr<ShadowCasterPair>> addedFilteredShadowCasters;   // Collects any non-shared, global casters identifed for filtering in this frame.

        bool needUpdate = false;
        //Guarantee that the list is clean
        m_receiverVisualsToUpdate.clear();
        // Only add receivers if there are actually casters in the scene
        if (m_shadowCasters.size() > 0)
        {
            // Add all opened popups to receiver list:
            // -> If popup contains a (potentially deep) caster, use that Caster Visual to ensure correct clipping of shadow on receiving end.
            // -> If popup does not have a caster, use Popup.Child.
            CPopupRoot* popupRoot = xamlIslandRoot ? xamlIslandRoot->GetPopupRootNoRef() : pCore->GetMainPopupRoot();
            if (popupRoot->m_pOpenPopups)
            {
                float minPopupDepth = std::numeric_limits<float>::max();
                for (CXcpList<CPopup>::XCPListNode *pNode = popupRoot->m_pOpenPopups->GetHead(); pNode != NULL; pNode = pNode->m_pNext)
                {
                    CPopup* openPopup = pNode->m_pData;
                    CUIElement* popupChild = openPopup->m_pChild;

                    if (popupChild &&              // We can't cast/receive on Popup without a child since its visual has no size
                        !openPopup->IsWindowed()   // Task 24600366: Lifted shadows for windowed popups
                    )
                    {
                        WUComp::IVisual* popupReceiverVisual = nullptr;
                        PopupFilteringInfo info;
                        info.ContainingPopup = openPopup;
                        info.Depth = EstimateShadowDepth(openPopup);

                        auto it = m_popupCasters.find(xref::get_weakref(openPopup));
                        if (it != m_popupCasters.end())
                        {
                            CUIElement* casterElement = it->second.lock_noref();
                            info.casterElement = casterElement;
                            info.isCaster = true;

                            // A previously filtered caster is no longer a candidate for filtering, but still needs consideration as a receiver.
                            // As a receiver it continues to use the caster element + its depth for shadow computation
                            // In case of a deep caster, this avoids an abrupt change in any shadow it is receiving when it is filtered out.
                            info.isFilteredCaster = casterElement && FindFilteredShadowCasterPairByElement(casterElement) != nullptr;

                            popupReceiverVisual = it->second.lock_noref()->GetShadowVisualNoRef();
                            //Currently comment out this Assertion to reduce test crash,
                            //Bug 19327369 created to track this problem.
                            //ASSERT(popupReceiverVisual);
                        }
                        else
                        {

                            // Bug 19172249, it is possible that the visual element
                            // has been destroyed or in process being destroyed, thus,
                            // we acquire the visual from  the CompNode held by the child element
                            // It's also possible the Popup is not a caster (receiver only)
                            HWCompTreeNode* popupCompNode = popupChild->GetCompositionPeer();
                            if (popupCompNode)
                            {
                                info.casterElement = popupChild;
                                info.isCaster = false;
                                popupReceiverVisual = (static_cast<HWCompTreeNodeWinRT*>(popupCompNode))->GetPrimaryVisualNoRef();
                            }
                        }

                        // Filter any casters that would cause a "shadow on top of themselves" artifact
                        // Compare each pair (caster, receiver) to identify any casters that need to be filtered out
                        // Using the rule: DrawOrder(C) < DrawOrder(R) and Depth(C) > Depth(R)
                        // Then C will cast a shadow on R while drawing below it, and we mitigate this by removing C from casters list.
                        // Note that, since m_openedPopups list is in reverse DrawOrder, this simplifies to checking that caster's depth
                        // is not greater than the minimum popup depth we've seen so far.
                        if  (info.isCaster && !info.isFilteredCaster && info.Depth > minPopupDepth)
                        {
                            ShadowCasterPair* casterPair = FindShadowCasterPairByElement(info.casterElement);
                            addedFilteredShadowCasters.push_back(xref_ptr<ShadowCasterPair>(casterPair));
                            m_shadowCasters.remove(xref_ptr<ShadowCasterPair>(casterPair));
                        }

                        minPopupDepth = std::min(info.Depth, minPopupDepth);
                        CheckReceiverNeedUpdate(popupReceiverVisual, needUpdate);
                    }
                }
            }

            // Add flattened Public Root primary visual to receiver list. It could be RSV-hosted content or a Canvas.
            HWCompTreeNode* rootReceiverCompNode = nullptr;
            CUIElement* publicRoot = xamlIslandRoot ?
                xamlIslandRoot->GetPublicRootVisual() :
                static_cast<CUIElement*>(pCore->getVisualRoot());
            if (publicRoot)
            {
                CScrollContentControl* rsvRoot = VisualTree::GetForElementNoRef(publicRoot)->GetRootScrollViewer();
                if (rsvRoot)
                {
                    CUIElement* borderWrapper = static_cast<CUIElement*>(publicRoot->GetParentInternal(false /*publicOnly*/));
                    rootReceiverCompNode = borderWrapper->GetCompositionPeer();
                }
                else if (publicRoot->OfTypeByIndex<KnownTypeIndex::Canvas>())
                {
                    rootReceiverCompNode = publicRoot->GetCompositionPeer();
                }

                // TODO: The public root Canvas is not guaranteed to have a comp node.
                // Bug 15141734 <Perf: Xaml ProjectedShadow: Only give Root Canvas a CompNode if there are shadows in the scene>
                if (rootReceiverCompNode != nullptr)
                {
                    WUComp::IVisual* publicRootPrimaryVisual = (static_cast<HWCompTreeNodeWinRT*>(rootReceiverCompNode))->GetPrimaryVisualNoRef();
                    CheckReceiverNeedUpdate(publicRootPrimaryVisual, needUpdate);
                }
            }
        }

        // Remove any casters filtered in this frame
        if (addedFilteredShadowCasters.size() > 0)
        {
            wrl::ComPtr<WUComp::ICompositionProjectedShadowCasterCollection> wucCasterCollection1, wucCasterCollection2;
            IFCFAILFAST(m_projectedShadowScene1->get_Casters(&wucCasterCollection1));
            IFCFAILFAST(m_projectedShadowScene2->get_Casters(&wucCasterCollection2));

            for (auto& filteredShadowCasterPair : addedFilteredShadowCasters)
            {
                CasterPair filteredWucCasters = filteredShadowCasterPair->GetWucCasters();
                IFCFAILFAST(wucCasterCollection1->Remove(filteredWucCasters.first.Get()));
                IFCFAILFAST(wucCasterCollection2->Remove(filteredWucCasters.second.Get()));

                // Caster has been filtered - invalidate UIElement's ShadowVisual
                const auto& source = filteredShadowCasterPair->GetSource();
                if (source->GetType() == SourceType::Element && source->GetElementNoRef())
                {
                    source->GetElementNoRef()->SetShadowVisual(nullptr);
                }

                m_filteredShadowCasters.push_back(xref_ptr<ShadowCasterPair>(filteredShadowCasterPair));
            }
            addedFilteredShadowCasters.clear();
        }

        // It's possible filtering removed all casters - make sure to clear receivers also
        if (m_shadowCasters.size() == 0)
        {
            m_receiverVisualsToUpdate.clear();
        }

        //Reset receiver collection if necessary
        if (needUpdate || (m_receiverVisualsToUpdate.size() != m_receiverVisualsCacheList.size()))
        {
            //We first clear the receiver collection, even if there are no receivers to update
            IFCFAILFAST(wucReceiverCollection1->RemoveAll());
            IFCFAILFAST(wucReceiverCollection2->RemoveAll());

            m_receiverVisualsCacheList.clear();

            for (WUComp::IVisual* receiverVisual : m_receiverVisualsToUpdate)
            {
                wrl::ComPtr<WUComp::ICompositionProjectedShadowReceiver> receiver;
                IFCFAILFAST(compositorProjectedShadow->CreateProjectedShadowReceiver(&receiver));
                HRESULT hr2 = receiver->put_ReceivingVisual(receiverVisual);
                if (hr2 != RO_E_CLOSED)
                {
                    IFCFAILFAST(hr2);
                    // Receivers can be shared between scenes
                    IFCFAILFAST(wucReceiverCollection1->Add(receiver.Get()));
                    IFCFAILFAST(wucReceiverCollection2->Add(receiver.Get()));
                }

                m_receiverVisualsCacheList.push_back(Microsoft::WRL::ComPtr<WUComp::IVisual>(receiverVisual));
            }
        }

        //Guarantees that there is no dangling pointer
        m_receiverVisualsToUpdate.clear();
    }
}

bool ThemeShadowGlobalScene::DoesPopupHaveGlobalCaster(_In_ CPopup* popup, _In_ CUIElement* uielement)
{
    auto it = m_popupCasters.find(xref::get_weakref(popup));
    if (it != m_popupCasters.end())
    {
        const auto& caster = it->second.lock_noref();

        // TODO_Shadows:  While an LTE targets a UIElement with ThemeShadow, we use the LTE itself as the caster since the element has no compnode.
        //                The idea was that its possible to be some (small) overlapping period where both the original UIE and the LTE are active as casters,
        //                so we relax the "one caster per popup" rule for this case.
        //                In practice, this is not happening, and also if it did we would likely get visual artifacts from this transient double-shadowing.
        //                This check and associated logic in ThemeShadowScene::AddNewCaster can likely be removed.
        if (caster && !caster->OfTypeByIndex<KnownTypeIndex::LayoutTransitionElement>())
        {
            if (uielement != caster)
            {
                m_dcompTreeHostNoRef->GetProjectedShadowManager()->SetHasEverHadIgnoredCasters(true);
            }
            return true;
        }
    }

    return false;
}

void ThemeShadowScene::UpdateReceivers(_In_ CCoreServices* pCore, _In_opt_ CXamlIslandRoot* xamlIslandRoot)
{
    wrl::ComPtr<WUComp::ICompositorWithProjectedShadow> compositorProjectedShadow;
    IFCFAILFAST(m_dcompTreeHostNoRef->GetCompositor()->QueryInterface(IID_PPV_ARGS(&compositorProjectedShadow)));

    bool needUpdate = false;
    //Guarantee that the list is clean
    m_receiverVisualsToUpdate.clear();
    // Only add custom receivers if there are actually casters in the scene
    if (m_shadowCasters.size() > 0 && m_themeShadow->HasCustomReceivers())
    {
        CUIElementWeakCollection* receivers = m_themeShadow->GetReceiversNoRef();
        if (receivers != nullptr)
        {
            for (const xref::weakref_ptr<CUIElement>& receiverWeak : *receivers)
            {
                CUIElement* customReceiverElement = receiverWeak.lock_noref();
                if (customReceiverElement != nullptr)
                {
                    // An explicit receiver that's not in the tree will not have a comp node.
                    HWCompTreeNode* customReceiverCompNode = customReceiverElement->GetCompositionPeer();
                    if (customReceiverCompNode != nullptr)
                    {
                        WUComp::IVisual* customReceiverPrimaryVisual = (static_cast<HWCompTreeNodeWinRT*>(customReceiverCompNode))->GetPrimaryVisualNoRef();
                        CheckReceiverNeedUpdate(customReceiverPrimaryVisual, needUpdate);
                    }
                }
            }
        }
    }

    //Reset receiver collections only if necessary
    if (needUpdate || (m_receiverVisualsToUpdate.size() != m_receiverVisualsCacheList.size()))
    {
        //We only clear the collections if no receivers to add to the collection
        wrl::ComPtr<WUComp::ICompositionProjectedShadowReceiverUnorderedCollection> wucReceiverCollection1, wucReceiverCollection2;

        IFCFAILFAST(m_projectedShadowScene1->get_Receivers(&wucReceiverCollection1));
        IFCFAILFAST(m_projectedShadowScene2->get_Receivers(&wucReceiverCollection2));

        IFCFAILFAST(wucReceiverCollection1->RemoveAll());
        IFCFAILFAST(wucReceiverCollection2->RemoveAll());

        m_receiverVisualsCacheList.clear();

        for (WUComp::IVisual* customReceiverPrimaryVisual : m_receiverVisualsToUpdate)
        {
            wrl::ComPtr<WUComp::ICompositionProjectedShadowReceiver> customReceiver;
            IFCFAILFAST(compositorProjectedShadow->CreateProjectedShadowReceiver(&customReceiver));
            IFCFAILFAST(customReceiver->put_ReceivingVisual(customReceiverPrimaryVisual));

            // Receivers can be shared between scenes
            IFCFAILFAST(wucReceiverCollection1->Add(customReceiver.Get()));
            IFCFAILFAST(wucReceiverCollection2->Add(customReceiver.Get()));

            m_receiverVisualsCacheList.push_back(Microsoft::WRL::ComPtr<WUComp::IVisual>(customReceiverPrimaryVisual));
        }
    }

    //Guarantees that there is no dangling pointer
    m_receiverVisualsToUpdate.clear();
}

int ThemeShadowScene::GetNumOfReceivers()
{
    wrl::ComPtr<WUComp::ICompositionProjectedShadowReceiverUnorderedCollection> wucReceiverCollection;
    IFCFAILFAST(m_projectedShadowScene1->get_Receivers(&wucReceiverCollection));
    int receiverCount = 0;
    IFCFAILFAST(wucReceiverCollection->get_Count(&receiverCount));

    return receiverCount;
}

void ThemeShadowScene::ClearReceivers()
{
    wrl::ComPtr<WUComp::ICompositionProjectedShadowReceiverUnorderedCollection> wucReceiverCollection1, wucReceiverCollection2;

    // At the end of tests we can hit RO_E_CLOSED when calling into shadow scenes. No-op for RO_E_CLOSED.
    HRESULT hr = m_projectedShadowScene1->get_Receivers(&wucReceiverCollection1);
    if (hr != RO_E_CLOSED)
    {
        IFCFAILFAST(hr);
        IFCFAILFAST(m_projectedShadowScene2->get_Receivers(&wucReceiverCollection2));

        IFCFAILFAST(wucReceiverCollection1->RemoveAll());
        IFCFAILFAST(wucReceiverCollection2->RemoveAll());
    }
    m_receiverVisualsCacheList.clear();
}

void ThemeShadowScene::CheckReceiverNeedUpdate(WUComp::IVisual* receiverVisual, bool& needUpdate)
{
    if (!receiverVisual)
    {
        return;
    }

    m_receiverVisualsToUpdate.push_back(receiverVisual);

    if (!needUpdate)
    {
        for (auto& cachedReceiverVisual : m_receiverVisualsCacheList)
        {
            if (cachedReceiverVisual.Get() == receiverVisual)
            {
                //Since the visual is already in the list,
                //no update is needed
                return;
            }
        }
    }

    needUpdate = true;
}

void ProjectedShadowManager::UpdateShadowScenes(_In_ CCoreServices* pCore)
{
    if (!GetHasEverHadCaster())
    {
        return;
    }

    if (!AreShadowsEnabled())
    {
        if (m_customScenesNeedClearing)
        {
            ClearCustomScenes();
        }

        if (m_globalScenesNeedClearing)
        {
            ClearGlobalScenes();

            // Clear visuals list now that all scenes are cleaned up.
            m_shadowVisuals.clear();
        }

        return;
    }

    // Update global scene.
    if (m_globalScene)
    {
        m_globalScene->UpdateCasters();
        m_globalScene->UpdateReceiversAndFilterCasters(pCore, nullptr);

        if (AreCasterListsEmpty(m_globalScene.get()))
        {
            m_globalScene = nullptr;
        }
    }

    if (m_xamlIslandRootsGlobalScenes.size() > 0)
    {
        auto it = m_xamlIslandRootsGlobalScenes.begin();
        while (it != m_xamlIslandRootsGlobalScenes.end())
        {
            CXamlIslandRoot* xamlIslandRoot = it->first.lock_noref();
            xref_ptr<ThemeShadowGlobalScene> xamlIslandRootsGlobalScene = it->second;

            xamlIslandRootsGlobalScene->UpdateCasters();
            xamlIslandRootsGlobalScene->UpdateReceiversAndFilterCasters(pCore, xamlIslandRoot);

            if (AreCasterListsEmpty(xamlIslandRootsGlobalScene.get()))
            {
                it = m_xamlIslandRootsGlobalScenes.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    // Update custom scenes. Clean up any scene that no longer has casters.
    auto it = std::begin(m_customScenes);
    while (it != std::end(m_customScenes))
    {
        (*it)->UpdateCasters();
        (*it)->UpdateReceivers(pCore, nullptr);

        if ((*it)->m_shadowCasters.size() == 0 || (*it)->GetNumOfReceivers() == 0)
        {
            it = m_customScenes.erase(it);
        }
        else
        {
            ++it;
        }
    }

    //dirty the subtree of popups with ignored casters
    for (auto& popupWeakRef : m_popupsLookingForNewCasters)
    {
        CPopup* popup = popupWeakRef.lock_noref();
        if (popup != nullptr)
        {
            popup->SetEntireSubtreeDirty();
        }
    }
    m_popupsLookingForNewCasters.clear();
}

// Used only by policy-triggered fallback
void ProjectedShadowManager::ClearCustomScenes()
{
    ASSERT(m_customScenesNeedClearing);
    m_customScenes.clear();
    m_customScenesNeedClearing = false;
}

void ProjectedShadowManager::ConfigureScenePair(_In_ WUComp::ICompositionProjectedShadow* scene1, _In_ WUComp::ICompositionProjectedShadow* scene2)
{
    // ICompositionProjectedShadow2 has been renamed to ICompositionProjectedShadowFuture and has been stripped
    // out of the public winmds. Luckily Xaml only references it from dead code now. Commenting out the dead code
    // to avoid the build errors.
    /*
    wrl::ComPtr<WUComp::ICompositionProjectedShadowFuture> scene1Private;
    scene1->QueryInterface(IID_PPV_ARGS(&scene1Private));

    wrl::ComPtr<WUComp::ICompositionProjectedShadow2> scene2Private;
    scene2->QueryInterface(IID_PPV_ARGS(&scene2Private));

    IFCFAILFAST(scene1Private->put_MinOpacity(sc_ambientOpacityMin));
    IFCFAILFAST(scene1Private->put_MaxOpacity(sc_ambientOpacityMax));
    IFCFAILFAST(scene1Private->put_OpacityFalloff(sc_ambientOpacityFalloff));
    */
    IFCFAILFAST(scene1->put_MinBlurRadius(sc_ambientBlurMin));
    IFCFAILFAST(scene1->put_MaxBlurRadius(sc_ambientBlurMax));
    IFCFAILFAST(scene1->put_BlurRadiusMultiplier(sc_ambientBlurMultiplier));

    // ICompositionProjectedShadow2 has been renamed to ICompositionProjectedShadowFuture and has been stripped
    // out of the public winmds. Luckily Xaml only references it from dead code now. Commenting out the dead code
    // to avoid the build errors.
    /*
    IFCFAILFAST(scene2Private->put_MinOpacity(sc_directionalOpacityMin));
    IFCFAILFAST(scene2Private->put_MaxOpacity(sc_directionalOpacityMax));
    IFCFAILFAST(scene2Private->put_OpacityFalloff(sc_directionalOpacityFalloff));
    */
    IFCFAILFAST(scene2->put_MinBlurRadius(sc_directionalBlurMin));
    IFCFAILFAST(scene2->put_MaxBlurRadius(sc_directionalBlurMax));
    IFCFAILFAST(scene2->put_BlurRadiusMultiplier(sc_directionalBlurMultiplier));
}

void ProjectedShadowManager::AddToPopupsLookingForNewCastersList(CPopup* popup)
{
    m_popupsLookingForNewCasters.push_back(xref::get_weakref(popup));
}

ThemeShadowScene* ProjectedShadowManager::EnsureScene(_In_opt_ CThemeShadow* shadow, bool addToGlobalScene, _In_opt_ CXamlIslandRoot* xamlIslandRoot)
{
    ThemeShadowScene* result = nullptr;

    if (shadow != nullptr)
    {
        if (addToGlobalScene)
        {
            if (xamlIslandRoot != nullptr)
            {
                xref::weakref_ptr<CXamlIslandRoot> xamlIslandRootWeakRef = xref::get_weakref(xamlIslandRoot);
                if (!m_xamlIslandRootsGlobalScenes[xamlIslandRootWeakRef])
                {
                    xref_ptr<ThemeShadowGlobalScene> xamlIslandRootsGlobalScene = nullptr;
                    xamlIslandRootsGlobalScene.attach(new ThemeShadowGlobalScene(m_dcompTreeHostNoRef, m_core));
                    xamlIslandRootsGlobalScene->EnsureInitialized(xamlIslandRoot);
                    m_xamlIslandRootsGlobalScenes[xamlIslandRootWeakRef] = xamlIslandRootsGlobalScene;
                }
                result = m_xamlIslandRootsGlobalScenes[xamlIslandRootWeakRef];
            }
            else
            {
                if (!m_globalScene)
                {
                    m_globalScene.attach(new ThemeShadowGlobalScene(m_dcompTreeHostNoRef, m_core));
                    m_globalScene->EnsureInitialized(xamlIslandRoot);
                }
                result = m_globalScene;
            }
        }
        else
        {
            if (shadow->HasCustomReceivers())
            {
                CUIElementWeakCollection* receivers = shadow->GetReceiversNoRef();
                for (const xref::weakref_ptr<CUIElement>& receiverWeak : *receivers)
                {
                    const CUIElement* customReceiver = receiverWeak.lock_noref();
                    if (customReceiver != nullptr)
                    {
                        for (auto& customScene : m_customScenes)
                        {
                            if (customScene->m_themeShadow.get() == shadow)
                            {
                                result = customScene.get();
                            }
                        }

                        if (result == nullptr)
                        {
                            xref_ptr<ThemeShadowScene> newScene;
                            newScene.attach(new ThemeShadowScene(m_dcompTreeHostNoRef, m_core));
                            newScene->m_themeShadow = shadow;
                            newScene->EnsureInitialized(xamlIslandRoot);
                            m_customScenes.push_back(xref_ptr<ThemeShadowScene>(newScene));
                            result = newScene;
                        }

                        // Scenes can contain multiple receivers. As soon as we've found one receiver and have a scene, we can
                        // return it.
                        ASSERT(result != nullptr);
                        break;
                    }
                }
            }
        }
    }

    return result;
}

ThemeShadowScene* ProjectedShadowManager::GetGlobalSceneForVisual(
    _In_ WUComp::IVisual* visual,
    _In_ xref_ptr<ThemeShadowGlobalScene> globalScene
)
{
    const auto& casterEntry = globalScene->FindShadowCasterPairByVisual(visual);
    if (casterEntry)
    {
        return static_cast<ThemeShadowScene*>(globalScene.get());
    }

    return nullptr;
}

ThemeShadowScene* ProjectedShadowManager::GetSceneForVisual(_In_ WUComp::IVisual* visual)
{
    ThemeShadowScene* result = nullptr;

    // First look in the global scene, where we can have local or shared casters
    if (m_globalScene)
    {
        result = GetGlobalSceneForVisual(visual, m_globalScene);
        if (result != nullptr)
        {
            return result;
        }
    }

    if (m_xamlIslandRootsGlobalScenes.size() > 0)
    {
        for (auto& xamlIslandGlobalScenePair : m_xamlIslandRootsGlobalScenes)
        {
            result = GetGlobalSceneForVisual(visual, xamlIslandGlobalScenePair.second);
            if (result != nullptr)
            {
                return result;
            }
        }
    }

    // Next try the custom receiver scenes
    if (m_customScenes.size() > 0)
    {
        for (const auto& customScene : m_customScenes)
        {
            const auto& casterEntry = customScene->FindShadowCasterPairByVisual(visual);
            if (casterEntry)
            {
                return customScene.get();
            }
        }
    }

    return nullptr;
}

bool ProjectedShadowManager::DoesPopupHaveGlobalCaster(_In_ CPopup* popup, _In_ CUIElement* uielement)
{
    if (m_globalScene)
    {
        if (m_globalScene->DoesPopupHaveGlobalCaster(popup, uielement))
        {
            return true;
        }
    }

    if (m_xamlIslandRootsGlobalScenes.size() > 0)
    {
        for (const auto& xamlIslandGlobalScenePair : m_xamlIslandRootsGlobalScenes)
        {
            const auto& islandGlobalScene = xamlIslandGlobalScenePair.second;
            if (islandGlobalScene && islandGlobalScene->DoesPopupHaveGlobalCaster(popup, uielement))
            {
                return true;
            }
        }
    }

    return false;
}

WUComp::IVisual* ProjectedShadowManager::GetShadowVisualNoRef(_In_ CUIElement* element)
{
    auto it = m_shadowVisuals.find(element);
    if (it != m_shadowVisuals.end())
    {
        return it->second;
    }

    return nullptr;
}

void ProjectedShadowManager::SetShadowVisual(_In_ CUIElement* element, _In_ WUComp::IVisual* visual)
{
    if (!visual)
    {
        m_shadowVisuals.erase(element);
    }
    else
    {
        m_shadowVisuals[element] = visual;
    }
}

float ProjectedShadowManager::GetTranslateZ(_In_opt_ CUIElement* element)
{
    return element ? element->GetTranslation(false /* preferAnimatingValue */).Z : 0;
}

void ProjectedShadowManager::OnPolicyChanged()
{
    if (m_effectPolicyHelper != nullptr)
    {
        const bool areShadowsEnabled = m_effectPolicyHelper->AreShadowsEnabled();

        if (areShadowsEnabled != m_areShadowsEnabled)
        {
            m_areShadowsEnabled = areShadowsEnabled;
            if (!m_areShadowsEnabled)
            {
                m_customScenesNeedClearing = true;
                m_globalScenesNeedClearing = true;    // Note this takes place later than custom scene cleanup
            }

            if (m_core)
            {
                // Dirty all content roots to reflect new shadow policy
                ContentRootCoordinator* contentRootCoordinator = m_core->GetContentRootCoordinator();
                const auto& contentRoots = contentRootCoordinator->GetContentRoots();

                for (const auto& contentRoot : contentRoots)
                {
                    VisualTree* visualTree = contentRoot->GetVisualTreeNoRef();
                    if (visualTree)
                    {
                        CUIElement* rootElement = visualTree->GetRootElementNoRef();
                        if (rootElement)
                        {
                            rootElement->SetEntireSubtreeDirty();
                        }
                    }
                }
            }
        }
    }
}

void ProjectedShadowManager::ForceShadowsPolicy(_In_ CCoreServices* coreServices, const bool forceShadowsOn)
{
    EnsureEffectPolicyHelper(coreServices);
    m_effectPolicyHelper->ForceShadowsPolicy(forceShadowsOn);
}

void ProjectedShadowManager::ClearShadowPolicyOverrides()
{
    // Tests don't call this without forcing the shadow policy first, so the effect policy helper should already exist.
    m_effectPolicyHelper->ClearShadowPolicyOverrides();
}
