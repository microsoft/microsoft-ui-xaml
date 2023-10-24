// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <HWCompNodeWinRT.h>
#include <ThemeShadow.h>
#include <UIElement.h>
#include <vector_map.h>
#include <fwd/windows.ui.composition.h>
#include "EffectPolicyHelper.h"

class DCompTreeHost;
class ProjectedShadowManager;

using namespace Microsoft::WRL;

typedef std::pair<ComPtr<WUComp::ICompositionProjectedShadowCaster>, ComPtr<WUComp::ICompositionProjectedShadowCaster>> CasterPair;

enum class SceneType
{
    Global,         // Receiver list not set. Flattened root and popups are receivers.
    Custom          // Receiver list specifies receivers
};

enum class SourceType
{
    Element,        // Caster visual identified by UIElement and obtained from its CompNode. Common case.
    Visual          // Caster visual identified by Visual dierctly. Used by Connected Animations.
};

class ShadowSource
{
public:
    SourceType GetType() { return m_sourceType; }
    virtual WUComp::IVisual* GetVisualNoRef() const = 0;
    virtual CUIElement* GetElementNoRef() const = 0;
    virtual ~ShadowSource() = default;

protected:
    ShadowSource(_In_ SourceType sourceType)
        : m_sourceType(sourceType)
    {
    }

protected:
    SourceType m_sourceType;
};

class VisualShadowSource : public ShadowSource
{
public:
    VisualShadowSource(_In_ WUComp::IVisual* visual)
        : ShadowSource(SourceType::Visual)
        , m_visual(visual)
    {
    }
    WUComp::IVisual* GetVisualNoRef() const override { return m_visual; };
    CUIElement* GetElementNoRef() const override { return nullptr; };

private:
    WUComp::IVisual* m_visual;
};

class ElementShadowSource : public ShadowSource
{
public:
    ElementShadowSource(_In_ CUIElement* element)
        : ShadowSource(SourceType::Element)
    {
        m_wrElement = xref::get_weakref(element);
    }

    WUComp::IVisual* GetVisualNoRef() const override;
    CUIElement* GetElementNoRef() const override { return m_wrElement.lock_noref(); }

private:
    xref::weakref_ptr<CUIElement> m_wrElement;
};

// ThemeShadow visual design employs two shadows: an "ambient" and a "directional", which always appear together.
// This requires a pair of casters with different configurations (but wrapping the same Visual)
class ShadowCasterPair : public CXcpObjectBase<IObject>
{
public:
    ~ShadowCasterPair() override = default;

    ShadowCasterPair(_In_ WUComp::IVisual* visual, _In_ WUComp::ICompositionBrush* maskBrush)
    {
        m_source = std::unique_ptr<ShadowSource>(new VisualShadowSource(visual));
        m_mask = maskBrush;
    }

    ShadowCasterPair(_In_ CUIElement* element, _In_ WUComp::ICompositionBrush* maskBrush)
    {
        m_source = std::unique_ptr<ShadowSource>(new ElementShadowSource(element));
        m_mask = maskBrush;
    }

    ShadowSource* GetSource() const { return m_source.get(); }
    Microsoft::WRL::ComPtr<WUComp::ICompositionBrush> GetMask() const { return m_mask; }

    void SetWucCasters(_In_ WUComp::ICompositionProjectedShadowCaster* wucCaster1, _In_ WUComp::ICompositionProjectedShadowCaster* wucCaster2)
    {
        m_wucCasterPair.first = wucCaster1;
        m_wucCasterPair.second = wucCaster2;
    }
    CasterPair GetWucCasters() const { return m_wucCasterPair; }

protected:

    std::unique_ptr<ShadowSource> m_source;
    ComPtr<WUComp::ICompositionBrush> m_mask;
    CasterPair m_wucCasterPair;
};

struct PopupFilteringInfo
{
    bool isCaster{false};
    bool isFilteredCaster{false};
    CUIElement* casterElement{};      // Caster Element (valid only if isCaster is true).
    CPopup* ContainingPopup{};
    float Depth{0.0f};

    // Note: Draw order is the other piece needed for filtering decision, but it is
    //        determined implicitly from position in m_pOpenedPopups (which is in reverse draw order).
};

// A scene containing casters and receivers for ThemeShadow elements that specify a Receiver list.
// Each ThemeShadow with a Receivers list engenders its own ThemeShadowScene, and a list is maintained by ProjectedShadowManager.
class ThemeShadowScene : public CDependencyObject
{
    friend ProjectedShadowManager;

public:
    explicit ThemeShadowScene(DCompTreeHost* dcompTreeHost, _In_ CCoreServices* core)
        : CDependencyObject(core)
        , m_dcompTreeHostNoRef(dcompTreeHost) {}

    ~ThemeShadowScene() override;

    virtual SceneType GetType() { return SceneType::Custom; }

    void EnsureInitialized(_In_opt_ CXamlIslandRoot* xamlIslandRoot);

    void AddNewCaster(
        SourceType sourceType,
        _In_opt_ WUComp::IVisual* visual,
        _In_opt_ CUIElement* element,
        _In_opt_ CPopup* containingPopup,
        _In_opt_ WUComp::ICompositionBrush* maskBrush);

    // Remove caster entry from casters map if present
    bool RemoveCaster(_In_ WUComp::IVisual* visual);

    void UpdateCasters();
    void ClearCasters();
    void UpdateReceivers(_In_ CCoreServices* pCore, _In_opt_ CXamlIslandRoot* xamlIslandRoot);
    void ClearReceivers();

    void SetCastersDirty(bool areCastersDirty) { m_areCastersDirty = areCastersDirty; }
    bool GetCastersDirty() { return m_areCastersDirty; }

    int GetNumOfReceivers();

    ShadowCasterPair* FindShadowCasterPairByVisual(_In_ WUComp::IVisual* visual) const;
    ShadowCasterPair* FindShadowCasterPairByElement(_In_ CUIElement* element) const;
    ShadowCasterPair* FindFilteredShadowCasterPairByVisual(_In_ WUComp::IVisual* visual) const;
    ShadowCasterPair* FindFilteredShadowCasterPairByElement(_In_ CUIElement* element) const;

protected:
    void SetupLights(_In_opt_ CXamlIslandRoot* xamlIslandRoot);
    void CheckReceiverNeedUpdate(WUComp::IVisual* receiverVisual, bool& needUpdate);

protected:
    // Contains bundle of information that helps configure assiciated WUC caster
    std::list<xref_ptr<ShadowCasterPair>> m_shadowCasters;            // All local casters on last update
    std::vector<xref_ptr<ShadowCasterPair>> m_addedShadowCasters;     // Local casters to be added in this frame
    std::vector<xref_ptr<ShadowCasterPair>> m_removedShadowCasters;   // Local casters to be removed in this frame

    // Persistent list of filtered global casters. A caster is filtered when it would cast a shadow on itself.
    // These casters will be effectively ignored and not passed to Composition.
    std::list<xref_ptr<ShadowCasterPair>> m_filteredShadowCasters;

    std::vector<Microsoft::WRL::ComPtr<WUComp::IVisual>> m_receiverVisualsCacheList;  //Cached receivers to be compared in the next frame
    std::vector<WUComp::IVisual*> m_receiverVisualsToUpdate; //Temporary list to store the receiver visuals to update in the current frame

    // ThemeShadow visual design employs two shadows: an "ambient" and a "directional", which always appear together.
    // This requires two scenes and two lights (with slightly different direction)
    Microsoft::WRL::ComPtr<WUComp::ICompositionProjectedShadow> m_projectedShadowScene1, m_projectedShadowScene2;
    Microsoft::WRL::ComPtr<WUComp::ICompositionLight> m_light1, m_light2;

    bool m_isInitialized {false};
    bool m_areCastersDirty {false};

    // ThemeShadow element that is associated with this scene if it's a custom scene. Stays nullptr if it's the global scene.
    xref_ptr<CThemeShadow> m_themeShadow;

   _Notnull_ DCompTreeHost* m_dcompTreeHostNoRef {nullptr};
};

// The scene containing casters and receivers for ThemeShadow elements that do not specify a Receiver list.
// There is only one such scene for each XamlRoot.
class ThemeShadowGlobalScene final : public ThemeShadowScene
{
    friend ProjectedShadowManager;

public:
    explicit ThemeShadowGlobalScene(DCompTreeHost* dcompTreeHost, _In_ CCoreServices* core)
        : ThemeShadowScene(dcompTreeHost, core) {}

    ~ThemeShadowGlobalScene() override;

    SceneType GetType() override { return SceneType::Global; };

    void UpdateReceiversAndFilterCasters(_In_ CCoreServices* pCore, _In_opt_ CXamlIslandRoot* xamlIslandRoot);

private:
    bool DoesPopupHaveGlobalCaster(_In_ CPopup* popup, _In_ CUIElement* uielement);
    float EstimateShadowDepth(_In_ CPopup* popupWithCaster);

public:
    // Map of caster UIElements for a given Popup. Used to enforce the requirement that
    // only one element inside a Popup contributes to global scene.
    containers::vector_map<xref::weakref_ptr<CPopup>, xref::weakref_ptr<CUIElement>> m_popupCasters;
};

class ProjectedShadowManager final
    : public IEffectPolicyHelperCallback    // For notifications when material policy changes
{
public:
    explicit ProjectedShadowManager(DCompTreeHost* dcompTreeHost);
    ~ProjectedShadowManager();

    void ReleaseResources();

    // Add, replace, or remove a caster based on the current and previous UIElement.Shadow states.
    // Apply rules to determine if caster is entering a global scene.
    void UpdateCasterStatus(_In_ CUIElement* uielement);

    // Add, replace or remove caster entry based on caster Visual
    void UpdateCasterStatus(
        _In_ CCoreServices* core,
        SourceType sourceType,
        _In_ WUComp::IVisual* visual,
        _In_ CUIElement* uielement,
        _In_opt_ CThemeShadow* themeShadow,
        bool addToGlobalScene,
        _In_opt_ CPopup* containingPopup,
        _In_ CXamlIslandRoot* xamlIslandRoot
    );

    bool RemoveCaster(_In_ WUComp::IVisual* visual);

    void UpdateShadowScenes(_In_ CCoreServices* pCore);

    // Used for fallback only
    void ClearCustomScenes();
    void ClearGlobalScenes();

    static void ConfigureScenePair(_In_ WUComp::ICompositionProjectedShadow* scene1, _In_ WUComp::ICompositionProjectedShadow* scene2);

    bool GetHasEverHadCaster()
    {
        return m_hasEverHadCaster;
    }

    void SetHasEverHadCaster(bool value)
    {
        m_hasEverHadCaster = value;
    }

    bool DoesPopupHaveGlobalCaster(_In_ CPopup* popup, _In_ CUIElement* uielement);

    WUComp::IVisual* GetShadowVisualNoRef(_In_ CUIElement* element);
    void SetShadowVisual(_In_ CUIElement* element, _In_ WUComp::IVisual* visual);

    void SetHasEverHadIgnoredCasters(_In_ bool value) { m_hasEverHadIgnoredCasters = value; }
    bool HasEverHadIgnoredCasters() { return m_hasEverHadIgnoredCasters; }

    void AddToPopupsLookingForNewCastersList(CPopup* popup);

    static CPopup* GetNextAncestorPopup(_In_ CPopup* popup);
    static float GetTranslateZ(_In_opt_ CUIElement* element);

    const bool AreShadowsEnabled() { return m_areShadowsEnabled; }
    void OnPolicyChanged() override;
    void ForceShadowsPolicy(_In_ CCoreServices* coreServices, const bool forceShadowsOn);
    void ClearShadowPolicyOverrides();

private:
    void EnsureEffectPolicyHelper(_In_ CCoreServices* coreServices);

    ThemeShadowScene* EnsureScene(_In_opt_ CThemeShadow* shadow, bool addToGlobalScene, _In_opt_ CXamlIslandRoot* xamlIslandRoot);
    ThemeShadowScene* GetSceneForVisual(_In_ WUComp::IVisual* visual);
    ThemeShadowScene* GetGlobalSceneForVisual(_In_ WUComp::IVisual* visual, _In_ xref_ptr<ThemeShadowGlobalScene> globalScene);
    bool AreCasterListsEmpty(_In_ ThemeShadowGlobalScene* globalScene);


private:
    static const float sc_ambientOpacityMin;
    static const float sc_ambientOpacityMax;
    static const float sc_ambientOpacityFalloff;
    static const float sc_ambientBlurMin;
    static const float sc_ambientBlurMax;
    static const float sc_ambientBlurMultiplier;
    static const float sc_directionalOpacityMin;
    static const float sc_directionalOpacityMax;
    static const float sc_directionalOpacityFalloff;
    static const float sc_directionalBlurMin;
    static const float sc_directionalBlurMax;
    static const float sc_directionalBlurMultiplier;

    bool m_hasEverHadCaster {false};
    bool m_areShadowsEnabled {true};
    bool m_customScenesNeedClearing {false};
    bool m_globalScenesNeedClearing {false};
    bool m_hasEverHadIgnoredCasters {false};

    xref_ptr<EffectPolicyHelper> m_effectPolicyHelper{nullptr};

    xref_ptr<ThemeShadowGlobalScene> m_globalScene;
    containers::vector_map<xref::weakref_ptr<CXamlIslandRoot>, xref_ptr<ThemeShadowGlobalScene>> m_xamlIslandsGlobalScenes;
    std::list<xref_ptr<ThemeShadowScene>> m_customScenes;
    _Notnull_ DCompTreeHost* m_dcompTreeHostNoRef;

    // Map of ShadowVisual's associated with a UIElement
    // TODO: This is a temporary mechanism in 19H1 to support UIElement::Get/SetShadowVisual() in a space-effecient way.
    //       When ancestor targets are supported the implementation will move fully into UIElement
    //       See Task 18895128: [ShadowVisual] Implement UIElement::GetShadowVisual(ShadowVisualType) that can return either a SpriteVisual or a ContainerVisual as needed by ThemeShadow
    containers::vector_map<CUIElement*, WUComp::IVisual*> m_shadowVisuals;

    CCoreServices* m_core {nullptr};   // Needed to enforce fallback policy

    //Task 19130750: We dirty the popup subtree when caster in use is removed
    //from the live tree and there is ignored casters. We could not dirty the
    //tree during the render walk, thus, we use a collection to store the
    //popups and dirty them right before commit
    std::vector<xref::weakref_ptr<CPopup>> m_popupsLookingForNewCasters;
};
