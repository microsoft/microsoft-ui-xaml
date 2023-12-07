// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <HWCompNode.h>
#include <NamespaceAliases.h>
#include <WinRTLocalExpressionCache.h>
#include <FacadeStorage.h>
#include <fwd/windows.ui.composition.h>
#include <microsoft.ui.composition.h>
#include <microsoft.ui.composition.experimental.h>

struct RedirectionTransformInfo;
class CTimeManager;
class WinRTExpressionConversionContext;
class WinRTLocalExpressionBuilder;
class CXamlLight;
struct DropShadowRecipe;

class HWCompTreeNodeWinRT : public HWCompTreeNode
{
    enum class PreviousClipComparison : bool
    {
        CompareAgainstPreviousClip,
        AlwaysSet
    };

    // RAII object to disable/re-enable Implicit Animations on the given Visual
    class ImplicitAnimationDisabler
    {
    public:
        explicit ImplicitAnimationDisabler(_In_ IUnknown* visualUnk, bool disableIA);
        virtual ~ImplicitAnimationDisabler();

    private:
        wrl::ComPtr<WUComp::ICompositionObjectPartner> m_visualAsCO;    // The Visual we're working with
        bool m_disableIA;                                               // If true, do the actual work of disabling/re-enabling
    };

public:
    ~HWCompTreeNodeWinRT() override;

    static _Check_return_ HRESULT Create(
        _In_ CCoreServices *coreServices,
        _In_ CompositorTreeHost *compositorTreeHost,
        _In_ DCompTreeHost* dcompTreeHost,
        bool isPlaceholderCompNode,
        _Outptr_ HWCompTreeNode **compositorTreeNode);

    _Check_return_ HRESULT SetElementData(
        _In_ CWindowRenderTarget *renderTarget,
        _In_ CUIElement *uiElement,
        bool isHitTestVisibleSubtree
        ) override;

    xref_ptr<WUComp::IVisual> GetWUCVisual() const override;

    // Returns a visual used as a reference visual during incremental rendering. The reference visual itself isn't
    // used as a parent for other visuals; new visuals will be inserted into the parent after the reference visual.
    virtual WUComp::IVisual* GetReferenceVisualForIncrementalRendering() const;

    _Check_return_ HRESULT EnsureVisual(_In_ DCompTreeHost *dompTreeHost) override;

    bool HasHandOffVisual() const override;
    bool HasHandInVisual() const override;

    WUComp::IVisual* GetHandOffVisual() override;

    // Discards the WinRT hand-in IVisual previously set by SetHandInVisual.
    void DiscardHandInVisual() override;

    inline void HidePrependVisual() {m_prependVisual->put_IsVisible(false);}

    void SetAllowReuseHandOffVisual(bool allowReuseHandOffVisual) override { m_allowReuseHandOffVisual = allowReuseHandOffVisual; }

    void SetLightsTargetingElement(_In_ std::vector<CXamlLight*> lights);
    void UpdatePrimaryVisualLights(_In_ DCompTreeHost* dcompTreeHost);
    void UntargetFromLights(_In_ DCompTreeHost* dcompTreeHost);
    void SetLightsAttachedToElement();

    WUComp::IVisual* GetPrimaryVisualNoRef() const { return m_primaryVisual.Get(); }

    WUComp::IContainerVisual* GetContainerVisualForChildren() const;

    void InsertChildSynchronous(
        _In_opt_ DCompTreeHost *dcompTreeHost,
        _In_ HWCompNode *child,
        _In_opt_ HWCompNode *referenceNode,
        _In_opt_ WUComp::IVisual* previousSiblingVisual,
        _In_opt_ CUIElement *element,
        const bool ignoreInsertVisualErrors);

    void RemoveSynchronous();

    bool HasHitTestVisibleContentInSubtree() const override;

protected:
    HWCompTreeNodeWinRT(
        _In_ CCoreServices *renderCore,
        _In_ CompositorTreeHost *compositorTreeHost,
        _In_ DCompTreeHost* dcompTreeHost,
        bool isPlaceholderCompNode
        );

    _Check_return_ HRESULT EnsureDManipData() override;

    _Check_return_ HRESULT PushProperties(
        _In_opt_ DCompTreeHost *dcompTreeHost,
        bool useDCompAnimations,
        bool disablePixelSnapping
        ) noexcept override;

    void CreateWUCSpineVisual(_In_ DCompTreeHost* dcompTreeHost, _Out_ WUComp::IVisual** visual, _In_ const wchar_t* debugTag);
    void InsertWUCSpineVisual(_In_ WUComp::IVisual* parentVisual, _In_ WUComp::IVisual* childVisual);
    void RemoveWUCSpineVisual(_In_ WUComp::IVisual* parentVisual, _In_ WUComp::IVisual* childVisual);
    void GetVisualCollectionFromWUCSpineVisual(_In_ WUComp::IVisual* visual, _Out_ WUComp::IVisualCollection** visualCollection);
    WUComp::IVisual* GetBottomMostVisual() const;
    WUComp::IVisual* GetBottomMostNonContentVisual() const;

    bool IsBottomMostClipVisual(int visualID) const;
    WUComp::IVisual* GetParentForBottomClipVisual(int visualID) const;
    WUComp::IVisual* GetChildForBottomClipVisual(int visualID) const;

    WUComp::IVisual* EnsureBottomClipVisual(_In_ DCompTreeHost* dcompTreeHost, int visualID, const wchar_t* visualTag);
    void CleanupBottomClipVisual(int visualID);

    void ReparentVisualChildrenHelper(_In_ WUComp::IVisual* oldParent, _In_ WUComp::IVisual* newParent);

    void SetDebugTag(_In_ WUComp::IVisual* visual, _In_ const wchar_t* debugTag);
    void SetNameDebugTag();

    bool HasIndependentTransformManipulation() const;

    virtual void UpdatePrependVisual(
        _In_ DCompTreeHost *dcompTreeHost,
        bool disablePixelSnapping
        );

    virtual void UpdatePrependTransform();

    virtual void UpdatePrependClip(_In_ DCompTreeHost *dcompTreeHost);
    XRECTF GetOverallLocalPrependClipRect();
    void SetPrependClipRect(_In_ DCompTreeHost *dcompTreeHost, _In_ const XRECTF& prependClipRect);

    void UpdatePrependOpacity(_In_ DCompTreeHost *dcompTreeHost);

    void UpdatePrependPixelSnapping(bool disablePixelSnapping);

    CUIElement* GetElementForLTEProperties() const;
    CThemeShadow* GetThemeShadowFromDropShadowCaster() const;
    bool NeedsDropShadowVisual() const;
    void UpdateDropShadowVisual(_In_ DCompTreeHost *dcompTreeHost);
    void UpdateDropShadowVisualBounds(_In_ DCompTreeHost *dcompTreeHost, _In_ const DropShadowRecipe& recipe);
    void UpdateDropShadowVisualTransform(_In_ DCompTreeHost *dcompTreeHost);
    void UpdateDropShadowVisualOpacity(_In_ DCompTreeHost *dcompTreeHost);
    void UpdateDropShadowVisualBrush(_In_ DCompTreeHost *dcompTreeHost, _In_ const DropShadowRecipe& recipe);
    void CreateDropShadowVisual(_In_ DCompTreeHost *dcompTreeHost, _Out_ ixp::IVisual** visual, _In_ const DropShadowRecipe& recipe);
    void UpdateDropShadowRecipeCornerRadius(_Inout_ DropShadowRecipe& recipe);
    void UpdateDropShadowVisualForThemeTransitionClip(_In_ DCompTreeHost *dcompTreeHost);

    wrl::ComPtr<ixp::ICompositionShadow> MakeDropShadow(_In_ DCompTreeHost *dcompTreeHost, const float blurRadius, const wu::Color color, const float offsetY);
    wrl::ComPtr<ixp::IVisual> MakeShapeVisual(_In_ DCompTreeHost *dcompTreeHost, _In_ ixp::ICompositionShape* compositionShape, const wfn_::float2 size, const wfn_::float3 offset);
    wrl::ComPtr<ixp::IVisual> MakeLayerVisual(_In_ DCompTreeHost *dcompTreeHost, _In_ ixp::ICompositionShadow* shadow, _In_ ixp::IVisual* child, const wfn_::float2 size);

    void UpdatePrimaryVisual(
        _In_ DCompTreeHost *dcompTreeHost,
        bool disablePixelSnapping
        );

    bool IsWUCTransformMatrixDifferentFromPrevious(const wfn::Matrix4x4 &matrix);
    void UpdatePrimaryVisualTransformMatrix(_In_ DCompTreeHost *dcompTreeHost);

    virtual void UpdatePrimaryVisualTransformParent(_In_ DCompTreeHost *dcompTreeHost);

    virtual bool GetRedirectionTransformInfo(_In_ RedirectionTransformInfo* rto);

    void UpdatePrimaryVisualViewportInteraction(_In_ DCompTreeHost *dcompTreeHost);
    bool IsViewportInteractionRequired(_In_ DCompTreeHost *dcompTreeHost) const;
    void UpdateDManipHitTestVisual(_In_ DCompTreeHost* dcompTreeHost) override;
    void EnsureDManipHitTestVisual(_In_ DCompTreeHost *dcompTreeHost);
    void CleanupDManipHitTestVisual();

    void UpdatePrimaryVisualCompositeMode();

    void UpdatePrimaryVisualPixelSnapping(bool disablePixelSnapping);

    void UpdatePrimaryVisualHitTestVisibility();

    wrl::ComPtr<WUComp::IExpressionAnimation> EnsureFacadeGlueExpression(_In_ WUComp::ICompositor* compositor, KnownPropertyIndex facadeID, _In_ const wchar_t* expressionString);
    void StartFacadeGlueExpression(_In_ WUComp::IVisual* visual, _In_ const wchar_t* propertyName, _In_ WUComp::IExpressionAnimation* expressionAnimation);

    bool RequiresComponentsVisual();
    bool HasComponentsVisual() const;
    WUComp::IVisual* GetComponentsVisual() const;
    void UpdateComponentsVisualConfiguration(_In_ DCompTreeHost *dcompTreeHost);

    void UpdateRotationFacade(_In_ DCompTreeHost *dcompTreeHost, _In_ WUComp::IVisual* visual);
    void UpdateScaleFacade(_In_ DCompTreeHost *dcompTreeHost, _In_ WUComp::IVisual* visual);
    void UpdateTransformMatrixFacade(_In_ DCompTreeHost *dcompTreeHost, _In_ WUComp::IVisual* visual);
    void UpdateCenterPointFacade(_In_ DCompTreeHost *dcompTreeHost, _In_ WUComp::IVisual* visual);
    void UpdateRotationAxisFacade(_In_ DCompTreeHost *dcompTreeHost, _In_ WUComp::IVisual* visual);

    void UpdateTransitionClipVisual(_In_ DCompTreeHost *dcompTreeHost);

    void UpdateRoundedCornerClipVisual(_In_ DCompTreeHost *dcompTreeHost);

    void UpdateHandInVisual();
    void CleanupHandInVisual();
    void CleanupHandOffVisual();
    void CleanupPrependVisualListener();

    void UpdateContentVisual(_In_ DCompTreeHost *dcompTreeHost);
    void CleanupContentVisual();

    void CleanupChildLinks();

    _Check_return_ HRESULT InsertChildInternal(
        _In_opt_ DCompTreeHost *dcompTreeHost,
        _In_ HWCompNode *child,
        _In_opt_ HWCompNode *referenceNode,
        _In_opt_ CUIElement *element
        ) override;

    _Check_return_ HRESULT InsertChildAtBeginningInternal(
        _In_opt_ DCompTreeHost *dompTreeHost,
        _In_ HWCompNode *child
        ) override;

    void InsertChildSynchronousInternal(
        _In_opt_ DCompTreeHost *dcompTreeHost,
        _In_ HWCompNode *child,
        _In_opt_ HWCompNode *referenceNode,
        // When we're doing synchronous comp tree updates, there are no render data comp nodes, and sprite visuals
        // are added directly to the comp node children. That means the reference WUC visual might be a sprite visual
        // from a UIElement, or it might be an interop visual from a comp node. It's specified here, and it's used
        // instead of pPreviousSibling if it's provided.
        _In_opt_ WUComp::IVisual* previousSiblingVisual,
        _In_opt_ CUIElement *element,
        const bool ignoreInsertVisualErrors);

    _Check_return_ HRESULT RemoveChildInternal(
        _In_ HWCompNode *child
        ) override;

    _Check_return_ HRESULT SetConnectedAnimationRunning(_In_ bool isRunning, _Outptr_opt_ WUComp::IVisual ** visual = nullptr) override;

    // Returns whether the clip was updated
    bool UpdateInsetClip(
        _In_ XRECTF& regularClip,
        _Inout_ WUComp::IInsetClip* insetClip,
        PreviousClipComparison previousClipComparison);

    bool IsWUCClipTransformMatrixDifferentFromPrevious(const wfn::Matrix3x2 &matrix);
    void UpdateVisualClipAndTransform(_In_ DCompTreeHost *dcompTreeHost, _In_ WUComp::IVisual* visual);

    void UpdatePrimaryVisualOffset(_In_ WUComp::ICompositor* compositor);

    // Helper enum, represents UIElement.Opacity scenarios
    enum class OpacityScenario
    {
        StaticValue = 0,            // Opacity is a static value
        AnimatedFacade = 1,         // Opacity is being animated via Facade animation
        AnimatedStoryboard = 2,     // Opacity is being animated via Storyboard
    };

    OpacityScenario GetPrimaryOpacityScenario();
    void UpdatePrimaryVisualOpacity(_In_ WUComp::ICompositor* compositor);

    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> GetTransform3DWinRTExpression(_In_ WinRTExpressionConversionContext* context);
    void ApplyTransform3DToLocalTransformExpression(
        _In_ DCompTreeHost* dcompTreeHost,
        _In_ WinRTLocalExpressionBuilder* builder
        );

    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> GetProjectionWinRTExpression(_In_ WinRTExpressionConversionContext* context);
    void ApplyProjectionToLocalTransformExpression(
        _In_ DCompTreeHost* dcompTreeHost,
        _In_ WinRTLocalExpressionBuilder* builder
        );

    bool IsUsingHandOffVisual(CUIElement &element) const;

    int GetCurrentTranslationScenariosInUse(_In_ CUIElement* elementForTranslation);
    bool IsTranslationInUse() const;
    bool IsTranslationExpressionRequired() const;
    bool IsECPTranslationInUse() const;
    bool IsStaticTranslationFacadeInUse() const;
    bool IsAnimatedTranslationFacadeInUse() const;
    void EnsureTranslationExpressions(_In_ CUIElement* elementForTranslation);
    void UpdateTranslationExpressionParameters(_In_ CUIElement* elementForTranslation);
    void UpdatePrependClipTranslationExpression();

    bool NeedsContentVisualForShadows(_In_ DCompTreeHost* dcompTreeHost, _In_ CUIElement* element) const;

    void CreateContentVisual(_In_ DCompTreeHost* dcompTreeHost);

    void EnsureDropShadowVisual(_In_ DCompTreeHost* dcompTreeHost);
    void CleanupDropShadowVisual();

    bool HasTransitionClipAnimation() const;

    Microsoft::WRL::ComPtr<WUComp::IVisual> m_prependVisual;
    Microsoft::WRL::ComPtr<WUComp::IVisual> m_primaryVisual;
    Microsoft::WRL::ComPtr<WUComp::IVisual> m_handInVisual;

    // This visual is created to support ThemeShadow as well as synchronous comp tree updates.
    // It's created for convenience so that this comp node has a consistent WUC visual for adding children.
    wrl::ComPtr<WUComp::IVisual> m_contentVisual;
    wrl::ComPtr<WUComp::IVisual> m_dropShadowParentVisual;  // Used for ThemeShadow in drop shadow mode
    wrl::ComPtr<WUComp::IVisual> m_dropShadowSpriteVisual;  // Used for ThemeShadow in drop shadow mode

    // Facades_TODO:  Rename BOTTOMCLIP to no longer use the word "clip"
    #define BOTTOMCLIP_Components           0
    #define BOTTOMCLIP_TransitionTarget     1
    #define BOTTOMCLIP_RoundedCorners       2
    // Always keep Capacity last
    #define BOTTOMCLIP_Capacity             3

    Microsoft::WRL::ComPtr<WUComp::IVisual> m_bottomClipVisuals[BOTTOMCLIP_Capacity];

    Microsoft::WRL::ComPtr<WUComp::IVisual> m_dmanipHitTestVisual;

    WinRTLocalExpressionCache m_primaryVisualExpressionCache;
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_transitionTargetOpacityExpressionCache;
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_opacityExpressionCache;
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_offsetTranslationExpressionCache;
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_matrixTranslationExpressionCache;
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_prependClipTranslationExpressionCache;
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_transitionClipTransformExpressionCache;
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_dropShadowOpacityExpressionCache;

    // Sometimes the app gets the hand off visual right after loading the UIElement, before Xaml gets a chance to render anything.
    // The app then sets a custom expression on the WUC visual's opacity right away, then Xaml does its render walk and overwrites
    // that expression with a static value. In order to preserve custom values/animations/expressions that the app has set, we'll
    // keep track of the last value that Xaml set on the WUC visual, then no-op if the next set has the same static value. If a
    // property is animated by a Xaml animation, then we do not make any checks.
    float m_previousWUCOpacity;
    wfn::Vector3 m_previousWUCOffset;
    WUComp::CompositionCompositeMode m_previousWUCCompositeMode;
    wfn::Matrix4x4 m_previousWUCTransformMatrix;
    XRECTF_RB m_previousWUCInsetClip;
    wfn::Matrix3x2 m_previousWUCClipTransformMatrix;
    bool m_previousHasWUCInsetClip : 1;
    bool m_hasEverHadPropertiesPushed : 1;  // false until PushProperties is called for the first time, then stays true
    bool m_allowReuseHandOffVisual : 1;
    bool m_isInConnectedAnimation : 1;
    bool m_isPlayingDropShadowOpacityAnimation : 1;

    // When set to true, causes us to target every light in m_lightsTargetingElement at the primary visual.
    bool m_shouldUpdateLightsTargetingElement : 1;
    // When set to true, causes us to go to every WUC CompositionLight in the UIElement's XamlLight collection and set its
    // CoordinateSpace to the primary visual.
    bool m_shouldUpdateLightsAttachedToElement : 1;

    // Defines bit-flags for Translation "scenarios" currently in use
    enum class TranslationScenarios
    {
        None = 0,
        StaticFacade = 1,           // Translation facade is set to static value
        AnimatedFacade = 2,         // Translation facade is being animated
        ECP = 4,                    // ElementCompositionPreview.SetIsTranslationEnabled is turned on
        StaticFacadePlusECP = 5,    // Shorthand combination value for StaticFacade | ECP
        AnimatedFacadePlusECP = 6,  // Shorthand combination value for AnimatedFacade | ECP
    };
    int m_translationScenariosInUse = 0;    // Stores a combination of TranslationScenarios bit-flags

    xref::weakref_ptr<CUIElement> m_pUIElementPeerWeakRef;

    std::vector<CXamlLight*> m_lightsTargetingElement;
};

