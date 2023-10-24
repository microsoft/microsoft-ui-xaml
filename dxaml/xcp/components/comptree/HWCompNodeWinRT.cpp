// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "HWCompNodeWinRT.h"
#include <HWRedirectedCompTreeNodeWinRT.h>
#include <HWWindowedPopupCompTreeNodeWinRT.h>
#include <DManipData.h>
#include <DCompTreeHost.h>
#include <UIElement.h>
#include <DOPointerCast.h>
#include <Popup.h>
#include <MultiParentShareableDependencyObject.h>
#include <CompositorTree.h>
#include <TransitionTarget.h>
#include <Projection.h>
#include <Transform.h>
#include <CompositeTransform.h>
#include <Transform3D.h>
#include <DOCollection.h>
#include <Geometry.h>
#include <Brush.h>
#include <Shape.h>
#include <Rectangle.h>
#include <XamlLocalTransformBuilder.h>
#include <WinRTLocalExpressionBuilder.h>
#include <WinRTExpressionConversionContext.h>
#include <Windows.UI.Composition.h>
#include <MUX-ETWEvents.h>
#include <ExpressionHelper.h>
#include <TimeMgr.h>
#include <FloatUtil.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <XamlLightTargetMap.h>
#include <XamlLightCollection.h>
#include <XamlLight.h>
#include "corep.h"
#include <FrameworkTheming.h>
#include "hwwalk.h"
#include <RootVisual.h>
#include <ProjectedShadowManager.h>
#include <ThemeShadow.h>
#include <WindowRenderTarget.h>
#include <ColorUtil.h>
#include <LayoutTransitionElement.h>
#include <SimplePropertiesHelpers.h>
#include <microsoft.ui.composition.private.h>
#include <Microsoft.UI.Content.h>
#include "D2DAcceleratedPrimitives.h"
#include "DropShadowRecipe.h"
#include <FxCallbacks.h>
#include <FrameworkUdk/Containment.h>

// Bug 45792810: Explorer first frame - DesktopWindowXamlSource spends 30ms on RoGetActivationFactory
// Bug 46468883: [1.4 servicing] Explorer first frame - DesktopWindowXamlSource spends 30ms on RoGetActivationFactory
#define WINAPPSDK_CHANGEID_46468883 46468883

using namespace DirectUI;
using namespace RuntimeFeatureBehavior;

// These constants need to be kept in sync with test code, see WinRTMockDComp.cpp
static const wchar_t* s_prependVisualTag = L"_XAML_DEBUG_TAG_PrependVisual";
static const wchar_t* s_primaryVisualTag = L"_XAML_DEBUG_TAG_PrimaryVisual";
static const wchar_t* s_transitionClipVisualTag = L"_XAML_DEBUG_TAG_TransitionClipVisual";
static const wchar_t* s_roundedCornerClipVisualTag = L"_XAML_DEBUG_TAG_RoundedCornerClipVisual";
static const wchar_t* s_componentsVisualTag = L"_XAML_DEBUG_TAG_ComponentsVisual";
static const wchar_t* s_dmanipHitTestVisualTag = L"_XAML_DEBUG_TAG_DManipHitTestVisual";
static const wchar_t* s_dmanipHitTestVisualTagOmitFromDump = L"_XAML_DEBUG_TAG_DManipHitTestVisual_OmitFromDump";
static const wchar_t* s_contentVisualTag = L"_XAML_DEBUG_TAG_ContentVisual";
static const wchar_t* s_dropShadowVisualTag = L"_XAML_DEBUG_TAG_DropShadowVisual";
static const wchar_t* s_nameTagPrefix = L"_XAML_DEBUG_TAG_Name_";

// Translation strings
// Notes on expressions that support Translation:  There are 2 separate Translation properties supported by XAML:
// -ElementCompositionPreview
// -Translation facade
// Both properties are allowed to be used simultaneously.  This leads to the expressions below which incorporate them.
// For ECP Translation, this is stored in the PrimaryVisual's property set, thus you'll see this below as "primaryVisual.Translation"
// For Translation facade, this may be stored in the backing PropertySet, or in the PrependVisual's PropertySet in cases where
// the backing PropertySet isn't needed.  The Translation value shows up below as "PS.Translation" when the backing PropertySet is used.
// Otherwise, it shows up below as "prependVisual.Translation" when it's stored in the PrependVisual's PropertySet.
// See additional notes in EnsureTranslationExpressions().
const wchar_t* const s_offsetString = L"Offset";
const wchar_t* const s_transformMatrixString = L"TransformMatrix";
const wchar_t* const s_offsetTranslationExpressionString = L"primaryVisual.Translation";
const wchar_t* const s_matrixTranslationExpressionStringECPOnly = L"Matrix4x4.CreateFromTranslation(primaryVisual.Translation) * prependVisual.PrependTransform * Matrix4x4.CreateFromTranslation(-1 * primaryVisual.Translation)";
const wchar_t* const s_matrixTranslationExpressionStringFacadeOnly = L"Matrix4x4.CreateFromTranslation(PS.Translation) * prependVisual.PrependTransform";
const wchar_t* const s_matrixTranslationExpressionStringECPAndStaticFacade = L"Matrix4x4.CreateFromTranslation(primaryVisual.Translation + prependVisual.Translation) * prependVisual.PrependTransform * Matrix4x4.CreateFromTranslation(-1 * (primaryVisual.Translation))";
const wchar_t* const s_matrixTranslationExpressionStringECPAndAnimatedFacade = L"Matrix4x4.CreateFromTranslation(primaryVisual.Translation + PS.Translation) * prependVisual.PrependTransform * Matrix4x4.CreateFromTranslation(-1 * (primaryVisual.Translation))";
const wchar_t* const s_prependClipTranslationExpressionStringECPOnly = L"Matrix3x2.CreateFromTranslation(-vector2(primaryVisual.Translation.X, primaryVisual.Translation.Y))";
const wchar_t* const s_prependClipTranslationExpressionStringFacadeOnly = L"Matrix3x2.CreateFromTranslation(-vector2(PS.Translation.X, PS.Translation.Y))";
const wchar_t* const s_prependClipTranslationExpressionStringECPAndStaticFacade = L"Matrix3x2.CreateFromTranslation(-vector2(primaryVisual.Translation.X + prependVisual.Translation.X, primaryVisual.Translation.Y + prependVisual.Translation.Y))";
const wchar_t* const s_prependClipTranslationExpressionStringECPAndAnimatedFacade = L"Matrix3x2.CreateFromTranslation(-vector2(primaryVisual.Translation.X + PS.Translation.X, primaryVisual.Translation.Y + PS.Translation.Y))";
const wchar_t* const s_prependVisualString = L"prependVisual";
const wchar_t* const s_primaryVisualString = L"primaryVisual";

// Facade related strings:  PropertySet names first
const wchar_t* s_PS = L"PS";
const wchar_t* s_TranslationFacade = L"Translation";
const wchar_t* s_PrependTransform = L"PrependTransform";

// Facade related strings:  "Glue" expression strings that tie the backing PropertySet to the Visual
const wchar_t* s_RotationFacadeExpressionString = L"PS.Rotation";
const wchar_t* s_ScaleFacadeExpressionString = L"PS.Scale";
const wchar_t* s_TransformMatrixFacadeExpressionString = L"PS.TransformMatrix";
const wchar_t* s_CenterPointFacadeExpressionString = L"PS.CenterPoint";
const wchar_t* s_RotationAxisFacadeExpressionString = L"PS.RotationAxis";
const wchar_t* s_OpacityFacadeExpressionString = L"PS.Opacity";

// Drop shadow related strings:
const wchar_t* const s_PrimaryOpacityExpressionString = L"primaryVisual.Opacity";

HWCompTreeNodeWinRT::ImplicitAnimationDisabler::ImplicitAnimationDisabler(_In_ IUnknown* visualUnk, bool disableIA)
 : m_disableIA(disableIA)
{
    if (m_disableIA)
    {
        VERIFYHR(visualUnk->QueryInterface(IID_PPV_ARGS(&m_visualAsCO)));
        IFCFAILFAST(m_visualAsCO->EnableImplicitAnimations(FALSE));
    }
}

HWCompTreeNodeWinRT::ImplicitAnimationDisabler::~ImplicitAnimationDisabler()
{
    if (m_disableIA)
    {
        IFCFAILFAST(m_visualAsCO->EnableImplicitAnimations(TRUE));
    }
}

HWCompTreeNodeWinRT::HWCompTreeNodeWinRT(
    _In_ CCoreServices *renderCore,
    _In_ CompositorTreeHost *compositorTreeHost,
    _In_ DCompTreeHost* dcompTreeHost,
    bool isPlaceholderCompNode
    )
    : HWCompTreeNode(renderCore, compositorTreeHost, isPlaceholderCompNode)
    // The cached previous values are initialized to WUC visual defaults. That way we can skip setting them at all if not needed,
    // which will preserve whatever values, animations, and expressions that the app set directly on the hand off visual before
    // we got a chance to render.
    , m_previousWUCOpacity(1.0f)
    , m_previousWUCOffset({0.0f, 0.0f, 0.0f})
    , m_previousWUCCompositeMode(WUComp::CompositionCompositeMode::CompositionCompositeMode_Inherit)
    , m_previousWUCTransformMatrix({
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f })
    , m_previousWUCInsetClip({0.0f, 0.0f, 0.0f, 0.0f})
    , m_previousWUCClipTransformMatrix({
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 0.0f })
    , m_previousHasWUCInsetClip(false)
    , m_hasEverHadPropertiesPushed(false)
    , m_allowReuseHandOffVisual(true)
    , m_shouldUpdateLightsTargetingElement(false)
    , m_shouldUpdateLightsAttachedToElement(false)
    , m_isInConnectedAnimation(false)
    , m_isPlayingDropShadowOpacityAnimation(false)
{
    //
    // The content visual must be created ahead of time. When a UIElement creates a comp node, it needs to
    // update HWWalk with the rendering context, one of which is the ContainerVisual to insert new SpriteVisuals
    // into. That ContainerVisual is the content visual, and we need it before we connect the comp node with
    // the rest of the comp tree.
    //
    // The content visual must be a ContainerVisual. It will contain a mix of InteropVisuals (for comp nodes)
    // and SpriteVisuals (for element render data). The SpriteVisuals must be able to walk up to the parent and
    // remove themselves, for when the content is dirtied and needs to be removed. A SpriteVisual cannot walk
    // up to a parent InteropVisual, so we must use a ContainerVisual for the parent.
    //
    // Revealing the structure of Xaml's WUC visual tree is not an issue. The hand off visual and hand in visual
    // are both parented under an InteropVisual (although not the same one) in the comp node visual chain, and
    // cannot walk up to their parents.
    //
    CreateContentVisual(dcompTreeHost);
}

HWCompTreeNodeWinRT::~HWCompTreeNodeWinRT()
{
    if (m_contentVisual != nullptr)
    {
        // When hand off visuals are used, the primary visual is preserved even after the comp node is deleted.
        // In that case, make sure we remove the content visual from the hand off visual, otherwise it will be
        // inserted again when a new comp node is created using the same hand off visual.
        WUComp::IVisual* contentParent = GetBottomMostNonContentVisual();
        RemoveWUCSpineVisual(contentParent, m_contentVisual.Get());
    }

    // Explicitly check for nullptr.  It's possible for a CompNode to be created, then deleted, without having called PushProperties().
    // In this case all the visuals will be nullptr, and we would crash trying to access them while cleaning up.
    if (m_prependVisual != nullptr)
    {
        CleanupPrependVisualListener();
        CleanupHandOffVisual();
        CleanupHandInVisual();
        CleanupDManipHitTestVisual();
        CleanupChildLinks();
        CleanupContentVisual();
    }
}

_Check_return_ HRESULT HWCompTreeNodeWinRT::Create(
    _In_ CCoreServices *coreServices,
    _In_ CompositorTreeHost *compositorTreeHost,
    _In_ DCompTreeHost* dcompTreeHost,
    bool isPlaceholderCompNode,
    _Outptr_ HWCompTreeNode **compositorTreeNode)
{
    HRESULT hr = S_OK;
    HWCompTreeNodeWinRT *node = new HWCompTreeNodeWinRT(coreServices, compositorTreeHost, dcompTreeHost, isPlaceholderCompNode);

    TraceCompTreeCreateTreeNodeInfo(
        reinterpret_cast<XUINT64>(node),
        0 /* emulatedVisual */,
        static_cast<XUINT32>(isPlaceholderCompNode)
        );

    *compositorTreeNode = node;

    RRETURN(hr);//RRETURN_REMOVAL
}

_Check_return_ HRESULT HWCompTreeNodeWinRT::SetElementData(
    _In_ CWindowRenderTarget *renderTarget,
    _In_ CUIElement *uiElement,
    bool isHitTestVisibleSubtree    // Note:  only valid if SpriteVisuals is enabled
    )
{
    m_pUIElementNoRef = uiElement;
    m_pUIElementPeerWeakRef = xref::get_weakref(uiElement);

    if (m_pUIElementNoRef->OfTypeByIndex<KnownTypeIndex::SwapChainElement>())
    {
        // SwapChainPanel itself doesn't generate a CompNode, and its child SwapChainElement doesn't
        // generate any SpriteVisuals, which are the "normal" carrier of hit-test-visibility information.
        // In this special case, use the isHitTestVisibleSubtree flag provided by the RenderWalk to propagate
        // hit-test-visibility down to the primary visual.
        m_isHitTestVisible = isHitTestVisibleSubtree;
    }
    else
    {
        m_isHitTestVisible = !m_pUIElementNoRef->RequiresHitTestInvisibleCompNode();
    }

    if (uiElement->IsManipulatable())
    {
        IFC_RETURN(UpdateDManipData(uiElement));
    }
    else
    {
        m_spDManipData.reset();
    }

    CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);

    return S_OK;
}

xref_ptr<WUComp::IVisual> HWCompTreeNodeWinRT::GetWUCVisual() const
{
    xref_ptr<WUComp::IVisual> visual(m_prependVisual.Get());
    return visual;
}

WUComp::IVisual* HWCompTreeNodeWinRT::GetReferenceVisualForIncrementalRendering() const
{
    return m_prependVisual.Get();
}

// Get the child-most visual of the visuals for the "Spine" of this CompNode.
WUComp::IVisual* HWCompTreeNodeWinRT::GetBottomMostVisual() const
{
    if (m_contentVisual != nullptr)
    {
        return m_contentVisual.Get();
    }
    else
    {
        return GetBottomMostNonContentVisual();
    }
}

WUComp::IVisual* HWCompTreeNodeWinRT::GetBottomMostNonContentVisual() const
{
    ASSERT(m_primaryVisual != nullptr);

    // Start searching upwards from the bottom end of the optional clip visuals which are always at the very bottom.
    for (int i = BOTTOMCLIP_Capacity - 1; i >= 0; i--)
    {
        if (m_bottomClipVisuals[i] != nullptr)
        {
            return m_bottomClipVisuals[i].Get();
        }
    }

    return m_primaryVisual.Get();
}

bool HWCompTreeNodeWinRT::IsUsingHandOffVisual(CUIElement &element) const
{
    return m_allowReuseHandOffVisual && element.IsUsingHandOffVisual();
}

_Check_return_ HRESULT HWCompTreeNodeWinRT::EnsureVisual(_In_ DCompTreeHost *dcompTreeHost)
{
    if (m_prependVisual == nullptr)
    {
        ASSERT(dcompTreeHost != nullptr);
        CreateWUCSpineVisual(dcompTreeHost, &m_prependVisual, s_prependVisualTag);
        DCompTreeHost::SetTagIfEnabled(m_prependVisual.Get(), VisualDebugTags::CompNode_PrependVisual);

        ASSERT(m_primaryVisual == nullptr);

        // If the app created a Hand-Off visual before this CompNode was created, we'll use that
        // visual for the Primary visual instead of creating one ourselves.
        if (IsUsingHandOffVisual(*m_pUIElementNoRef))
        {
            xref_ptr<WUComp::IVisual> handoffVisual;
            m_pUIElementNoRef->GetStoredHandOffVisual(handoffVisual.ReleaseAndGetAddressOf());
            VERIFYHR(handoffVisual->QueryInterface(IID_PPV_ARGS(&m_primaryVisual)));
            ASSERT(m_primaryVisual != nullptr);
            SetDebugTag(m_primaryVisual.Get(), s_primaryVisualTag);
            DCompTreeHost::SetTagIfEnabled(m_primaryVisual.Get(), VisualDebugTags::CompNode_PrimaryVisual);

            // If the UIElement gets recycled, and the HandOff visual is in use, care must be taken to cache/restore the
            // property values we're tracking for the purposes of knowing when XAML can and cannot stomp over
            // a visual property.
            // If we find a cache of these property values, restore that cache now.  We'll then use these values the next
            // time PushProperties is called.
            auto& map = m_pUIElementNoRef->GetDCompTreeHost()->GetHandOffVisualDataMap();
            auto handOffPropertyIter = map.find(m_pUIElementNoRef);
            ASSERT(handOffPropertyIter != map.end());
            HandOffVisualData& cache = handOffPropertyIter->second;
            if (cache.cachedPropertiesInUse)
            {
                m_previousWUCOpacity = cache.previousWUCOpacity;
                m_previousWUCOffset = cache.previousWUCOffset;
                m_previousWUCCompositeMode = cache.previousWUCCompositeMode;
                m_previousWUCTransformMatrix = cache.previousWUCTransformMatrix;
                m_previousWUCInsetClip = cache.previousWUCInsetClip;
                m_previousWUCClipTransformMatrix = cache.previousWUCClipTransformMatrix;
                m_previousHasWUCInsetClip = cache.previousHasWUCInsetClip;
            }
        }
        else
        {
            CreateWUCSpineVisual(dcompTreeHost, &m_primaryVisual, s_primaryVisualTag);
            DCompTreeHost::SetTagIfEnabled(m_primaryVisual.Get(), VisualDebugTags::CompNode_PrimaryVisual);
        }
        InsertWUCSpineVisual(m_prependVisual.Get(), m_primaryVisual.Get());

        // TODO_Shadows: Remove this when Feature_SynchronousCompTreeUpdates is enabled.
        if (!m_contentVisual && NeedsContentVisualForShadows(dcompTreeHost, m_pUIElementNoRef))
        {
            CreateContentVisual(dcompTreeHost);
        }

        if (m_contentVisual != nullptr)
        {
            InsertWUCSpineVisual(m_primaryVisual.Get(), m_contentVisual.Get());
        }

        SetNameDebugTag();
    }

    return S_OK;
}

// Create a visual that will be used as part of the "spine" of our tree.  See comments in PushProperties().
void HWCompTreeNodeWinRT::CreateWUCSpineVisual(_In_ DCompTreeHost* dcompTreeHost, _Out_ ixp::IVisual** visual, _In_ const wchar_t* debugTag)
{
    wrl::ComPtr<ixp::IContainerVisual> containerVisual;
    IFCFAILFAST(dcompTreeHost->GetCompositor()->CreateContainerVisual(containerVisual.ReleaseAndGetAddressOf()));
    IFCFAILFAST(containerVisual->QueryInterface(IID_PPV_ARGS(visual)));

    SetDebugTag(*visual, debugTag);
}

void HWCompTreeNodeWinRT::SetDebugTag(_In_ WUComp::IVisual* visual, _In_ const wchar_t* debugTag)
{
    if (DCompTreeHost::VisualDebugTagsEnabled())
    {
        DCompTreeHost::SetTag(visual, debugTag, 0.0f);
    }
}

// Set the x:Name attribute from the visual's corresponding UIElement as a debug tag, if x:Name is present
void HWCompTreeNodeWinRT::SetNameDebugTag()
{
    if (DCompTreeHost::VisualDebugTagsEnabled())
    {
        CValue result;
        VERIFYHR(m_pUIElementNoRef->GetValueByIndex(KnownPropertyIndex::DependencyObject_Name, &result));

        // The convention used is to set a debug tag that begins with s_nameTagPrefix, followed by x:Name
        UINT32 count = 0;
        const wchar_t* name = result.AsEncodedString().GetBufferAndCount(&count);
        if (name != nullptr)
        {
            std::wstring debugTag;
            debugTag.append(s_nameTagPrefix);
            debugTag.append(name);
            SetDebugTag(m_primaryVisual.Get(), debugTag.c_str());
        }
    }
}

bool HWCompTreeNodeWinRT::HasIndependentTransformManipulation() const
{
    return (m_spDManipData != nullptr) && (m_spDManipData->GetManipulationContent() != nullptr);
}

void HWCompTreeNodeWinRT::InsertWUCSpineVisual(_In_ WUComp::IVisual* parentVisual, _In_ WUComp::IVisual* childVisual)
{
    xref_ptr<WUComp::IVisualCollection> childCollection;
    GetVisualCollectionFromWUCSpineVisual(parentVisual, childCollection.ReleaseAndGetAddressOf());
    // LTEs can render one UIElement in multiple places, which causes us to insert the same WUC hand off visual in multiple
    // places in the tree. DComp will return E_INVALIDARG in this case, which we'll ignore. The subsequent inserts will no-op.
    IFCFAILFAST_ALLOW_INVALIDARG(childCollection->InsertAtBottom(childVisual));
}

void HWCompTreeNodeWinRT::RemoveWUCSpineVisual(_In_ WUComp::IVisual* parentVisual, _In_ WUComp::IVisual* childVisual)
{
    xref_ptr<WUComp::IVisualCollection> childCollection;
    GetVisualCollectionFromWUCSpineVisual(parentVisual, childCollection.ReleaseAndGetAddressOf());
    // During shutdown we might find no child collection due to the CompositionContent already closing down the entire tree.
    if (childCollection != nullptr)
    {
        IFCFAILFAST(childCollection->Remove(childVisual));
    }
}

void HWCompTreeNodeWinRT::GetVisualCollectionFromWUCSpineVisual(_In_ WUComp::IVisual* visual, _Out_ WUComp::IVisualCollection** visualCollection)
{
    xref_ptr<ixp::IContainerVisual> containerVisual;
    IFCFAILFAST(visual->QueryInterface(IID_PPV_ARGS(containerVisual.ReleaseAndGetAddressOf())));

    // For CoreWindow scenarios, the CompositionContent is also listening for the CoreWindow's closed event.
    // CompositionContent will get the notification first and close the entire visual tree, then Xaml will
    // exit its message loop and tear down the tree. Since CompositionContent already closed everything,
    // Xaml will get lots of RO_E_CLOSED errors. These are all safe to ignore. So tolerate RO_E_CLOSED if
    // we're also in the middle of tearing down the tree.
    HRESULT hr = containerVisual->get_Children(visualCollection);
    if (FAILED(hr)
        && (hr != RO_E_CLOSED || !GetContext()->IsTearingDownTree()))
    {
        IFCFAILFAST(hr);
    }
}

// Given the visual ID, return true if this visual is the bottom-most of the optional clip visuals.
bool HWCompTreeNodeWinRT::IsBottomMostClipVisual(int visualID) const
{
    ASSERT(visualID >= 0 && visualID < BOTTOMCLIP_Capacity);

    // Start searching down from one past the given visual.
    for (int i = visualID + 1; i < BOTTOMCLIP_Capacity; i++)
    {
        if (m_bottomClipVisuals[i] != nullptr)
        {
            return false;
        }
    }

    return true;
}

// Given the visual ID, return this visual's appropriate parent visual.
WUComp::IVisual* HWCompTreeNodeWinRT::GetParentForBottomClipVisual(int visualID) const
{
    ASSERT(visualID >= 0 && visualID < BOTTOMCLIP_Capacity);

    for (int i = visualID - 1; i >= 0; i--)
    {
        if (m_bottomClipVisuals[i] != nullptr)
        {
            return m_bottomClipVisuals[i].Get();
        }
    }

    return m_primaryVisual.Get();
}

// Given the visual ID, return this visual's appropriate child visual.
WUComp::IVisual* HWCompTreeNodeWinRT::GetChildForBottomClipVisual(int visualID) const
{
    ASSERT(visualID >= 0 && visualID < BOTTOMCLIP_Capacity);

    for (int i = visualID + 1; i < BOTTOMCLIP_Capacity; i++)
    {
        if (m_bottomClipVisuals[i] != nullptr)
        {
            return m_bottomClipVisuals[i].Get();
        }
    }

    return nullptr;
}

// Given a visual ID for the desired clip visual, create the visual if necessary and return it.
WUComp::IVisual* HWCompTreeNodeWinRT::EnsureBottomClipVisual(_In_ DCompTreeHost* dcompTreeHost, int visualID, const wchar_t* visualTag)
{
    if (m_bottomClipVisuals[visualID] == nullptr)
    {
        CreateWUCSpineVisual(dcompTreeHost, &m_bottomClipVisuals[visualID], visualTag);

        WUComp::IVisual* parentVisual = GetParentForBottomClipVisual(visualID);
        if (IsBottomMostClipVisual(visualID))
        {
            // This visual is the new bottom-most visual.  Reparent the children of previous bottom-most visual to this visual.
            ReparentVisualChildrenHelper(parentVisual /* oldParent */, m_bottomClipVisuals[visualID].Get() /* newParent */);
        }
        else
        {
            // There is a visual below the new visual.  Re-parent it to the new visual.
            WUComp::IVisual* childVisual = GetChildForBottomClipVisual(visualID);
            ASSERT(childVisual != nullptr);
            RemoveWUCSpineVisual(parentVisual, childVisual);
            InsertWUCSpineVisual(m_bottomClipVisuals[visualID].Get(), childVisual);
        }

        InsertWUCSpineVisual(parentVisual, m_bottomClipVisuals[visualID].Get() /* childVisual */);
    }

    return m_bottomClipVisuals[visualID].Get();
}

void HWCompTreeNodeWinRT::CleanupBottomClipVisual(int visualID)
{
    if (m_bottomClipVisuals[visualID] != nullptr)
    {
        WUComp::IVisual* parentVisual = GetParentForBottomClipVisual(visualID);
        RemoveWUCSpineVisual(parentVisual, m_bottomClipVisuals[visualID].Get());

        if (IsBottomMostClipVisual(visualID))
        {
            // Reparent our visuals to the new bottom-most visual
            ReparentVisualChildrenHelper(m_bottomClipVisuals[visualID].Get() /* oldParent */, parentVisual /* newParent */);
        }
        else
        {
            // There is a visual below the visual we're removing.  Reparent it to this visual's parent.
            WUComp::IVisual* childVisual = GetChildForBottomClipVisual(visualID);
            ASSERT(childVisual != nullptr);
            RemoveWUCSpineVisual(m_bottomClipVisuals[visualID].Get(), childVisual);
            InsertWUCSpineVisual(parentVisual, childVisual);
        }

        // We're finished with all reparenting, release the visual.
        m_bottomClipVisuals[visualID] = nullptr;
    }
}

void HWCompTreeNodeWinRT::ReparentVisualChildrenHelper(
    _In_ WUComp::IVisual* oldParent,
    _In_ WUComp::IVisual* newParent
    )
{
    xref_ptr<WUComp::IVisualCollection> originalChildrenCollection;
    GetVisualCollectionFromWUCSpineVisual(oldParent, originalChildrenCollection.ReleaseAndGetAddressOf());

    xref_ptr<WUComp::IVisualCollection> newChildrenCollection;
    GetVisualCollectionFromWUCSpineVisual(newParent, newChildrenCollection.ReleaseAndGetAddressOf());

    xref_ptr<wfc::IIterable<WUComp::Visual*>> iterable;
    IFCFAILFAST(originalChildrenCollection->QueryInterface(IID_PPV_ARGS(iterable.ReleaseAndGetAddressOf())));
    xref_ptr<wfc::IIterator<WUComp::Visual*>> iterator;
    IFCFAILFAST(iterable->First(iterator.ReleaseAndGetAddressOf()));
    boolean hasCurrent = false;
    IFCFAILFAST(iterator->get_HasCurrent(&hasCurrent));

    while (hasCurrent)
    {
        xref_ptr<WUComp::IVisual> visual;
        IFCFAILFAST(iterator->get_Current(visual.ReleaseAndGetAddressOf()));

        // Must move the iterator past the current item before removing it.
        IFCFAILFAST(iterator->MoveNext(&hasCurrent));

        // Transfer the current visual over to the end of the new container's children
        IFCFAILFAST(originalChildrenCollection->Remove(visual));
        IFCFAILFAST(newChildrenCollection->InsertAtTop(visual));
    }
}

bool HWCompTreeNodeWinRT::HasHandOffVisual() const
{
    return (m_primaryVisual != nullptr);
}

bool HWCompTreeNodeWinRT::HasHandInVisual() const
{
    return (m_handInVisual != nullptr);
}

WUComp::IVisual* HWCompTreeNodeWinRT::GetHandOffVisual()
{
    return m_primaryVisual.Get();
}

void HWCompTreeNodeWinRT::DiscardHandInVisual()
{
    // For this implementation, no-op, PushProperties() is responsible for actually discarding the visual.
}

_Check_return_ HRESULT HWCompTreeNodeWinRT::EnsureDManipData()
{
    HRESULT hr = S_OK;

    if (m_spDManipData == nullptr)
    {
        m_spDManipData.reset(new DManipDataWinRT());
    }

    RRETURN(hr);//RRETURN_REMOVAL
}

// Update the properties on the visuals that make up the "spine" of our visual tree.
// These visuals carry properties only.  Here's a list of the visuals and their purposes, in parent-child order:
//
// Prepend Visual
//      This visual acts as an accumulator of properties to the nearest ancestor CompNode.
//      This visual is always required.
//
// Primary Visual
//      This visual's properties are an almost direct mapping to the UIElement peer's properties.
//      This visual is always required.
//      This is also the same visual as the HandOff visual, which is app-visible
//
// Components Visual
//      This visual is only used when facades are in use and Primary has TransformMatrix in use
//      This visual receives the UIElement facade properties (with the exception of Translation),
//      and also receives the Clip that would normally get pushed into Primary
//
// BottomClip Visual(s)
//      There are multiple optional visuals for various types of clipping at the bottom of the CompNode:
//      1) BOTTOMCLIP_TransitionTarget:
//      This visual is only used when a ThemeAnimation is animating the element's clip transform.
//      This visual is typically short-lived - it's created at the beginning of the animation and destroyed at the end.
//      Note:  This visual is also used for applying a layout-clip, when applying it as a "self-clip".
//      2) BOTTOMCLIP_RoundedCorners:
//      This visual is only used when the element has CornerRadius in use with at least one non-zero corner radius.
//
// In addition, there are some "special case" visuals we deal with:
//
// HandIn Visual
//      This visual is created by the app for Composition Islands.
//      This visual is optional and provided to us by the app, we'll insert it at the top of the z-order.
//
// DropShadow Visual
//      This visual draws a drop shadow.
//      This visual is used only when UIElement.Shadow is set to a ThemeShadow and we are in drop shadow mode.
//
_Check_return_ HRESULT HWCompTreeNodeWinRT::PushProperties(
    _In_opt_ DCompTreeHost *dcompTreeHost,
    bool useDCompAnimations,
    bool disablePixelSnapping
    ) noexcept
{
    if (m_isDCompVisualsDirty || m_disablePixelSnapping != disablePixelSnapping)
    {
        m_disablePixelSnapping = disablePixelSnapping;

        UpdatePrependVisual(dcompTreeHost, disablePixelSnapping);

        // Ensure the "Components" visual is created/destroyed, depending on if it's needed
        UpdateComponentsVisualConfiguration(dcompTreeHost);

        // Depending on whether or not we have a Components visual, we'll use this visual as the recipient for
        // facades, as well as the Clip that we would normally push into the Primary visual.
        WUComp::IVisual* visualForFacades = HasComponentsVisual() ? GetComponentsVisual() : m_primaryVisual.Get();

        UpdatePrimaryVisual(dcompTreeHost, disablePixelSnapping);

        // Update UIElement clip + transform. The transform can be set by Xaml (UIElement.Clip.Transform)
        // or driven by DManip (used in sticky headers implementation)
        UpdateVisualClipAndTransform(dcompTreeHost, visualForFacades);

        // Update all facade properties
        if (m_pUIElementNoRef->IsStrictOnly())
        {
            UpdateRotationFacade(dcompTreeHost, visualForFacades);
            UpdateScaleFacade(dcompTreeHost, visualForFacades);
            UpdateTransformMatrixFacade(dcompTreeHost, visualForFacades);
            UpdateCenterPointFacade(dcompTreeHost, visualForFacades);
            UpdateRotationAxisFacade(dcompTreeHost, visualForFacades);
        }

        UpdateTransitionClipVisual(dcompTreeHost);
        UpdateRoundedCornerClipVisual(dcompTreeHost);
        UpdateHandInVisual();

        // The drop shadow visual must be updated after updating these visuals:
        // Primary Visual
        // TransitionClip Visual
        UpdateDropShadowVisual(dcompTreeHost);

        UpdateContentVisual(dcompTreeHost);

        if (m_pUIElementNoRef->OfTypeByIndex<KnownTypeIndex::Popup>())
        {
            // 19H1 Bug #19618232:  If the position or size of a windowed popup changes after it's already open,
            // we must update the bounds of the popup's HWND.
            CPopup* popup = static_cast<CPopup*>(m_pUIElementNoRef);
            IFCFAILFAST(popup->Reposition());
        }

        m_isDCompVisualsDirty = false;
        m_hasEverHadPropertiesPushed = true;
    }

    // Kick off any pending implicit Show/Hide animations.
    // It's important that we do this after updating properties, to avoid property stomping.
    m_pUIElementNoRef->TriggerImplicitShowHideAnimations();

    return S_OK;
}

void HWCompTreeNodeWinRT::UpdatePrependVisual(
    _In_ DCompTreeHost *dcompTreeHost,
    bool disablePixelSnapping
    )
{
    ASSERT(m_prependVisual != nullptr);

    UpdatePrependTransform();
    UpdatePrependClip(dcompTreeHost);
    UpdatePrependOpacity(dcompTreeHost);
    UpdatePrependPixelSnapping(disablePixelSnapping);
}

void HWCompTreeNodeWinRT::UpdatePrependTransform()
{
    wfn::Matrix4x4 prependTransform;
    m_prependTransform.ToMatrix4x4(&prependTransform);

    // 19H1 Bug #19358663:  While an LTE is targeting an element with Translation set (typically for ThemeShadow),
    // the target will not generate a CompNode, or apply its transform related properties and therefore doesn't
    // incorporate Translation, which makes shadows not render.  The fix is to use the LTE's CompNode as a stand-in
    // for the target and apply Translation to this CompNode instead.  If/when the CompNode goes away, Translation will
    // transfer back to the target.
    CUIElement* elementForTranslation = m_pUIElementNoRef;
    if (m_pUIElementNoRef->OfTypeByIndex<KnownTypeIndex::LayoutTransitionElement>())
    {
        CLayoutTransitionElement* lte = static_cast<CLayoutTransitionElement*>(m_pUIElementNoRef);
        elementForTranslation = lte->GetTargetElement();
    }

    // Compute the current set of Translation "scenarios" in use, which can be a combination of:
    // -ECP.Translation
    // -Translation facade (either static or animating)
    // Based on the scenarios in use, we create the minimal set of ExpressionAnimations that represent them.
    // Care is taken to avoid creating unnecessary animations.  Further, we avoid putting the Translation facade
    // into the Offset property as we can incorporate this property without the help of the "hit-testing stash".
    // Note: The HandOff visual is not required to be in use for an app to target Translation,
    //       implicit Show/Hide animations can target Translation without ever using the HandOff visual.
    int currentTranslationScenariosInUse = GetCurrentTranslationScenariosInUse(elementForTranslation);
    if (m_translationScenariosInUse != currentTranslationScenariosInUse)
    {
        // One of the combinations of Translation in use has changed.  We may need to take away, create, or change the form of
        // an existing ExpressionAnimation.  Simply stop the animations, release them and re-create as necessary.
        Microsoft::WRL::ComPtr<WUComp::ICompositionObject> prependAsCompositionObject;
        IFCFAILFAST(m_prependVisual.As(&prependAsCompositionObject));
        if (m_offsetTranslationExpressionCache)
        {
            IFCFAILFAST(prependAsCompositionObject->StopAnimation(wrl::Wrappers::HStringReference(s_offsetString).Get()));
        }

        if (m_matrixTranslationExpressionCache)
        {
            IFCFAILFAST(prependAsCompositionObject->StopAnimation(wrl::Wrappers::HStringReference(s_transformMatrixString).Get()));
        }

        wrl::ComPtr<WUComp::ICompositionClip> compositionClip;
        IFCFAILFAST(m_prependVisual->get_Clip(&compositionClip));
        if (compositionClip)
        {
            wrl::ComPtr<WUComp::ICompositionObject> compositionClipAsCO;
            VERIFYHR(compositionClip.As(&compositionClipAsCO));
            if (m_prependClipTranslationExpressionCache)
            {
                IFCFAILFAST(compositionClipAsCO->StopAnimation(wrl::Wrappers::HStringReference(ExpressionHelper::sc_propertyName_TransformMatrix).Get()));
            }
        }

        m_offsetTranslationExpressionCache.Reset();
        m_matrixTranslationExpressionCache.Reset();
        m_prependClipTranslationExpressionCache.Reset();
        m_translationScenariosInUse = currentTranslationScenariosInUse;
    }

    if (IsTranslationExpressionRequired())
    {
        // Some form of ExpressionAnimations are required.  Create those now.
        EnsureTranslationExpressions(elementForTranslation);
    }
    else
    {
        // No ExpressionAnimations are required as all that's present are static values.  Compute/set those now.
        if (IsTranslationInUse())
        {
            // Static Translation facade is the only form of Translation in use, compute a static value
            CMILMatrix4x4 prepend4x4(prependTransform);
            CMILMatrix4x4 translation4x4(true);
            wfn::Vector3 translation = elementForTranslation->GetTranslation();
            translation4x4.SetToTranslation(translation.X, translation.Y, translation.Z);
            prepend4x4.Prepend(translation4x4);
            prepend4x4.ToMatrix4x4(&prependTransform);
        }

        // Update our static prepend transform
        IFCFAILFAST(m_prependVisual->put_TransformMatrix(prependTransform));
    }

    if (!IsECPTranslationInUse())
    {
        // Set the Offset back to [0,0,0] as we're no longer using ECP Translation
        wfn::Vector3 offsetValue = {};
        IFCFAILFAST(m_prependVisual->put_Offset(offsetValue));
    }
}

// Helper function to compute the bit-mask of Translation scenarios that are currently in use.
int HWCompTreeNodeWinRT::GetCurrentTranslationScenariosInUse(_In_ CUIElement* elementForTranslation)
{
    FacadeStorage& facadeStorage = GetContext()->GetFacadeStorage();
    bool translationECPExpressionInUse = elementForTranslation->GetIsTranslationEnabled();
    bool translationAnimatedFacadeInUse = elementForTranslation->HasFacadeAnimation() && facadeStorage.HasAnimation(elementForTranslation, KnownPropertyIndex::UIElement_Translation);
    bool translationStaticFacadeInUse = !translationAnimatedFacadeInUse && !SimpleProperties::IsSimplePropertySetToDefault(KnownPropertyIndex::UIElement_Translation, elementForTranslation);

    int currentTranslationScenariosInUse = 0;
    if (translationECPExpressionInUse)
    {
        currentTranslationScenariosInUse |= static_cast<int>(TranslationScenarios::ECP);
    }
    if (translationAnimatedFacadeInUse)
    {
        currentTranslationScenariosInUse |= static_cast<int>(TranslationScenarios::AnimatedFacade);
    }
    if (translationStaticFacadeInUse)
    {
        currentTranslationScenariosInUse |= static_cast<int>(TranslationScenarios::StaticFacade);
    }

    return currentTranslationScenariosInUse;
}

bool HWCompTreeNodeWinRT::IsTranslationInUse() const
{
    return m_translationScenariosInUse != 0;
}

bool HWCompTreeNodeWinRT::IsTranslationExpressionRequired() const
{
    return m_translationScenariosInUse > 1;
}

bool HWCompTreeNodeWinRT::IsECPTranslationInUse() const
{
    return (m_translationScenariosInUse & static_cast<int>(TranslationScenarios::ECP)) != 0;
}

bool HWCompTreeNodeWinRT::IsStaticTranslationFacadeInUse() const
{
    return (m_translationScenariosInUse & static_cast<int>(TranslationScenarios::StaticFacade)) != 0;
}

bool HWCompTreeNodeWinRT::IsAnimatedTranslationFacadeInUse() const
{
    return (m_translationScenariosInUse & static_cast<int>(TranslationScenarios::AnimatedFacade)) != 0;
}

void HWCompTreeNodeWinRT::EnsureTranslationExpressions(_In_ CUIElement* elementForTranslation)
{
    // This sets up the expression animations that incorporate ElementCompositionPreview Translation and the Translation facade.
    // We avoid applying these properties to the Primary visual because the app has access to them via ECP's hand off visual.
    // ECP.Translation (once enabled via ECP.SetIsTranslationEnabled) is stored in the primary visual's property set for the app to access,
    // and is mapped into the prepend visual's transform matrix.
    // Tha Translation facade is stored in a backing property set that's hidden from the app (the app accesses Translation via the UIElement facade property),
    // and is mapped into the prepend visual's transform matrix.
    if (!m_offsetTranslationExpressionCache)
    {
        FacadeStorage& facadeStorage = GetContext()->GetFacadeStorage();

        // Push latest static values into various PropertySets
        UpdateTranslationExpressionParameters(elementForTranslation);

        // Get our visuals as CompositionObjects for use in the expression
        Microsoft::WRL::ComPtr<WUComp::ICompositionObject> prependAsCompositionObject;
        Microsoft::WRL::ComPtr<WUComp::ICompositionObject> primaryAsCompositionObject;
        IFCFAILFAST(m_prependVisual.As(&prependAsCompositionObject));
        IFCFAILFAST(m_primaryVisual.As(&primaryAsCompositionObject));

        // Get the Compositor
        Microsoft::WRL::ComPtr<WUComp::ICompositor> compositor;
        IFCFAILFAST(prependAsCompositionObject->get_Compositor(compositor.GetAddressOf()));

        // Create our offset expression
        // This expression is primarily needed for hit-testing.  We'll listen for changes to this property and put them into the hit-testing stash.
        if (IsECPTranslationInUse())
        {
            // Ensure we insert Translation into Primary visual's PropertySet
            CUIElement::EnsureTranslationInitialized(m_primaryVisual.Get());

            // Start listening to Prepend visual's Offset property for hit-testing purposes
            // Note: Detaching the listener when translate is disabled is done in CUIElement::SetIsTranslationEnabled(), so that we can
            // immediately update the local transform to reflect the no-longer-used Translation property.
            m_pUIElementNoRef->AttachListenerToPrependVisual(m_prependVisual.Get());

            Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> offsetExpression;
            IFCFAILFAST(compositor->CreateExpressionAnimation(offsetExpression.GetAddressOf()));

            IFCFAILFAST(offsetExpression->put_Expression(wrl_wrappers::HStringReference(s_offsetTranslationExpressionString).Get()));
            Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> offsetExpressionAsCompositionAnimation;
            IFCFAILFAST(offsetExpression.As(&offsetExpressionAsCompositionAnimation));
            IFCFAILFAST(offsetExpressionAsCompositionAnimation->SetReferenceParameter(wrl_wrappers::HStringReference(s_primaryVisualString).Get(), primaryAsCompositionObject.Get()));
            IFCFAILFAST(prependAsCompositionObject->StartAnimation(wrl_wrappers::HStringReference(s_offsetString).Get(), offsetExpressionAsCompositionAnimation.Get()));

            m_offsetTranslationExpressionCache = offsetExpression;
        }

        // Create our matrix expression
        // This expression is needed because the prepend transform may have a scale in it, which needs to be applied "above" Translation.
        // Thus the expression first undoes the effect of Translation (applied to Offset), then re-applies Translation below the prepend transform.
        // Here we take care to create the minimal ExpressionAnimation that incorporates only the Translation scenarios that are in use.
        Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> matrixExpression;
        IFCFAILFAST(compositor->CreateExpressionAnimation(matrixExpression.GetAddressOf()));
        const wchar_t* matrixTranslationExpressionString;
        switch (static_cast<TranslationScenarios>(m_translationScenariosInUse))
        {
        case TranslationScenarios::AnimatedFacade:
            matrixTranslationExpressionString = s_matrixTranslationExpressionStringFacadeOnly;
            break;
        case TranslationScenarios::ECP:
            matrixTranslationExpressionString = s_matrixTranslationExpressionStringECPOnly;
            break;
        case TranslationScenarios::StaticFacadePlusECP:
            matrixTranslationExpressionString = s_matrixTranslationExpressionStringECPAndStaticFacade;
            break;
        case TranslationScenarios::AnimatedFacadePlusECP:
            matrixTranslationExpressionString = s_matrixTranslationExpressionStringECPAndAnimatedFacade;
            break;
        case TranslationScenarios::StaticFacade:
        default:
            FAIL_FAST_ASSERT(FALSE);
            break;
        }
        IFCFAILFAST(matrixExpression->put_Expression(wrl_wrappers::HStringReference(matrixTranslationExpressionString).Get()));
        Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> matrixExpressionAsCompositionAnimation;
        IFCFAILFAST(matrixExpression.As(&matrixExpressionAsCompositionAnimation));

        // Set reference parameters as appropriate for the given animations.
        if (IsECPTranslationInUse())
        {
            IFCFAILFAST(matrixExpressionAsCompositionAnimation->SetReferenceParameter(wrl_wrappers::HStringReference(s_primaryVisualString).Get(), primaryAsCompositionObject.Get()));
        }
        if (IsAnimatedTranslationFacadeInUse())
        {
            IFCFAILFAST(matrixExpressionAsCompositionAnimation->SetReferenceParameter(wrl_wrappers::HStringReference(s_PS).Get(), facadeStorage.GetBackingCompositionObject(elementForTranslation).Get()));
        }
        IFCFAILFAST(matrixExpressionAsCompositionAnimation->SetReferenceParameter(wrl_wrappers::HStringReference(s_prependVisualString).Get(), prependAsCompositionObject.Get()));

        // Assign our expressions to our cache
        m_matrixTranslationExpressionCache = matrixExpression;

        IFCFAILFAST(prependAsCompositionObject->StartAnimation(wrl_wrappers::HStringReference(s_transformMatrixString).Get(), matrixExpressionAsCompositionAnimation.Get()));
    }
    else
    {
        // In this case we aren't creating new animations, but we still need to push new static values into PropertySets.
        UpdateTranslationExpressionParameters(elementForTranslation);
    }
}

void HWCompTreeNodeWinRT::UpdateTranslationExpressionParameters(_In_ CUIElement* elementForTranslation)
{
    wfn::Matrix4x4 prependTransform;
    m_prependTransform.ToMatrix4x4(&prependTransform);

    wrl::ComPtr<WUComp::ICompositionObject> prependAsCompositionObject;
    IFCFAILFAST(m_prependVisual.As(&prependAsCompositionObject));
    wrl::ComPtr<WUComp::ICompositionPropertySet> prependPropertySet;
    IFCFAILFAST(prependAsCompositionObject->get_Properties(&prependPropertySet));

    // The PrependTransform is needed by all forms of the TransformMatrix animation.
    IFCFAILFAST(prependPropertySet->InsertMatrix4x4(wrl::Wrappers::HStringReference(s_PrependTransform).Get(), prependTransform));

    if (IsStaticTranslationFacadeInUse())
    {
        // We have a static Translation property.  In this case we don't necessarily have a backing PropertySet for the facade,
        // so we store this in the PrependVisual's PropertySet.
        IFCFAILFAST(prependPropertySet->InsertVector3(wrl::Wrappers::HStringReference(s_TranslationFacade).Get(), elementForTranslation->GetTranslation()));
    }
}

void HWCompTreeNodeWinRT::UpdatePrependClipTranslationExpression()
{
    // 19H1 Bug #19358663:  While an LTE is targeting an element with Translation set (typically for ThemeShadow),
    // the target will not generate a CompNode, or apply its transform related properties and therefore doesn't
    // incorporate Translation, which makes shadows not render.  The fix is to use the LTE's CompNode as a stand-in
    // for the target and apply Translation to this CompNode instead.  If/when the CompNode goes away, Translation will
    // transfer back to the target.
    CUIElement* elementForTranslation = m_pUIElementNoRef;
    if (m_pUIElementNoRef->OfTypeByIndex<KnownTypeIndex::LayoutTransitionElement>())
    {
        CLayoutTransitionElement* lte = static_cast<CLayoutTransitionElement*>(m_pUIElementNoRef);
        elementForTranslation = lte->GetTargetElement();
    }

    // Since the Translation expression is being set on the Prepend visual, we need to undo its effect
    // on the Prepend clip.  We accomplish this be creating an ExpressionAnimation that targets the
    // clip transform.
    wrl::ComPtr<WUComp::ICompositionClip> compositionClip;
    IFCFAILFAST(m_prependVisual->get_Clip(&compositionClip));

    if (IsTranslationInUse())
    {
        if (compositionClip == nullptr)
        {
            // The prepend clip is not in use.  Release any in-use expression.
            m_prependClipTranslationExpressionCache.Reset();
        }
        else
        {
            if (IsTranslationExpressionRequired())
            {
                if (m_prependClipTranslationExpressionCache == nullptr)
                {
                    FacadeStorage& facadeStorage = GetContext()->GetFacadeStorage();

                    // The prepend clip is in use, create our ExpressionAnimation and start it now.
                    wrl::ComPtr<WUComp::ICompositionObject> prependAsCO;
                    wrl::ComPtr<WUComp::ICompositionObject> primaryAsCO;
                    VERIFYHR(m_prependVisual.As(&prependAsCO));
                    VERIFYHR(m_primaryVisual.As(&primaryAsCO));

                    wrl::ComPtr<WUComp::ICompositor> compositor;
                    IFCFAILFAST(prependAsCO->get_Compositor(&compositor));

                    // Create the expression - it takes the inverse of Translation and applies this to the clip's TransformMatrix.
                    IFCFAILFAST(compositor->CreateExpressionAnimation(m_prependClipTranslationExpressionCache.GetAddressOf()));

                    // Here we take care to create the minimal ExpressionAnimation that incorporates only the Translation scenarios that are in use.
                    const wchar_t* clipExpressionString;
                    switch (static_cast<TranslationScenarios>(m_translationScenariosInUse))
                    {
                    case TranslationScenarios::AnimatedFacade:
                        clipExpressionString = s_prependClipTranslationExpressionStringFacadeOnly;
                        break;
                    case TranslationScenarios::ECP:
                        clipExpressionString = s_prependClipTranslationExpressionStringECPOnly;
                        break;
                    case TranslationScenarios::StaticFacadePlusECP:
                        clipExpressionString = s_prependClipTranslationExpressionStringECPAndStaticFacade;
                        break;
                    case TranslationScenarios::AnimatedFacadePlusECP:
                        clipExpressionString = s_prependClipTranslationExpressionStringECPAndAnimatedFacade;
                        break;
                    case TranslationScenarios::StaticFacade:
                    default:
                        FAIL_FAST_ASSERT(FALSE);
                        break;
                    }
                    IFCFAILFAST(m_prependClipTranslationExpressionCache->put_Expression(wrl_wrappers::HStringReference(clipExpressionString).Get()));

                    wrl::ComPtr<WUComp::ICompositionAnimation> prependClipTranslationExpressionAsCA;
                    IFCFAILFAST(m_prependClipTranslationExpressionCache.As(&prependClipTranslationExpressionAsCA));

                    // Set reference parameters as appropriate for the given animations.
                    if (IsECPTranslationInUse())
                    {
                        IFCFAILFAST(prependClipTranslationExpressionAsCA->SetReferenceParameter(wrl_wrappers::HStringReference(s_primaryVisualString).Get(), primaryAsCO.Get()));
                    }
                    if (IsStaticTranslationFacadeInUse())
                    {
                        IFCFAILFAST(prependClipTranslationExpressionAsCA->SetReferenceParameter(wrl_wrappers::HStringReference(s_prependVisualString).Get(), prependAsCO.Get()));
                    }
                    else if (IsAnimatedTranslationFacadeInUse())
                    {
                        IFCFAILFAST(prependClipTranslationExpressionAsCA->SetReferenceParameter(wrl_wrappers::HStringReference(s_PS).Get(), facadeStorage.GetBackingCompositionObject(elementForTranslation).Get()));
                    }

                    wrl::ComPtr<WUComp::ICompositionObject> compositionClipAsCO;
                    VERIFYHR(compositionClip.As(&compositionClipAsCO));
                    IFCFAILFAST(compositionClipAsCO->StartAnimation(wrl_wrappers::HStringReference(ExpressionHelper::sc_propertyName_TransformMatrix).Get(), prependClipTranslationExpressionAsCA.Get()));
                }
            }
            else
            {
                // push static value of -1 * Translation facade into TransformMatrix
                wfn::Vector3 translation = elementForTranslation->GetTranslation();
                wrl::ComPtr<WUComp::ICompositionClip2> compositionClip2;
                VERIFYHR(compositionClip.As(&compositionClip2));

                CMILMatrix translationMatrix(true);
                translationMatrix.AppendTranslation(-translation.X, -translation.Y);
                C_ASSERT(sizeof(wfn::Matrix3x2) == sizeof(translationMatrix));
                IFCFAILFAST(compositionClip2->put_TransformMatrix(*reinterpret_cast<wfn::Matrix3x2*>(&translationMatrix)));

                m_prependClipTranslationExpressionCache.Reset();
            }
        }
    }
    else
    {
        if (compositionClip != nullptr)
        {
            // Translation is not in use, set the clip transform to identity and release our expression if we have one.
            wrl::ComPtr<WUComp::ICompositionClip2> compositionClip2;
            VERIFYHR(compositionClip.As(&compositionClip2));

            CMILMatrix identityMatrix(true);
            C_ASSERT(sizeof(wfn::Matrix3x2) == sizeof(identityMatrix));
            IFCFAILFAST(compositionClip2->put_TransformMatrix(*reinterpret_cast<wfn::Matrix3x2*>(&identityMatrix)));

            m_prependClipTranslationExpressionCache.Reset();
        }
    }
}

void HWCompTreeNodeWinRT::UpdatePrependClip(_In_ DCompTreeHost *dcompTreeHost)
{
    SetPrependClipRect(dcompTreeHost, GetOverallLocalPrependClipRect());
    UpdatePrependClipTranslationExpression();
}

// Helper function, returns the prepend clip intersected with the layout clip if present
XRECTF HWCompTreeNodeWinRT::GetOverallLocalPrependClipRect()
{
    XRECTF overallClip = m_prependClip;

    // The LayoutClip is in the parent coordinate space and is applied above any RenderTransform.
    // This is a behavior change starting with Redstone, the previous behavior was to apply the LayoutClip below RenderTransforms.
    // We think this behavior makes more sense - the LayoutClip should ideally act as an ancestor clip, not a self-clip.
    if (m_pUIElementNoRef->HasLayoutClip() && m_pUIElementNoRef->ShouldApplyLayoutClipAsAncestorClip())
    {
        IntersectRect(&overallClip, &m_pUIElementNoRef->LayoutClipGeometry->m_rc);
    }

    return overallClip;
}

void HWCompTreeNodeWinRT::SetPrependClipRect(_In_ DCompTreeHost *dcompTreeHost, _In_ const XRECTF& prependClipRect)
{
    if (!IsInfiniteRectF(prependClipRect))
    {
        // We have a non-empty clip.  Create an InsetClip if necessary and update it.

        // Update the Size as InsetClip is dependent on Size.
        // Note that Size is really only necessary when we have a non-empty prepend clip.
        // REVIEW:  Would it make more sense to always set Size?
        wfn::Vector2 size;
        size.X = m_pUIElementNoRef->GetActualWidth();
        size.Y = m_pUIElementNoRef->GetActualHeight();
        IFCFAILFAST(m_prependVisual->put_Size(size));

        xref_ptr<WUComp::ICompositionClip> compositionClip;
        xref_ptr<WUComp::IInsetClip> insetClip;
        IFCFAILFAST(m_prependVisual->get_Clip(compositionClip.ReleaseAndGetAddressOf()));
        if (compositionClip == nullptr)
        {
            IFCFAILFAST(dcompTreeHost->GetCompositor()->CreateInsetClip(insetClip.ReleaseAndGetAddressOf()));
            VERIFYHR(insetClip->QueryInterface(IID_PPV_ARGS(compositionClip.ReleaseAndGetAddressOf())));
            IFCFAILFAST(m_prependVisual->put_Clip(compositionClip));
        }
        else
        {
            VERIFYHR(compositionClip->QueryInterface(IID_PPV_ARGS(insetClip.ReleaseAndGetAddressOf())));
        }
        IFCFAILFAST(insetClip->put_LeftInset(prependClipRect.X));
        IFCFAILFAST(insetClip->put_TopInset(prependClipRect.Y));
        float rightInset = m_pUIElementNoRef->GetActualWidth() - (prependClipRect.X + prependClipRect.Width);
        float bottomInset = m_pUIElementNoRef->GetActualHeight() - (prependClipRect.Y + prependClipRect.Height);
        IFCFAILFAST(insetClip->put_RightInset(rightInset));
        IFCFAILFAST(insetClip->put_BottomInset(bottomInset));
    }
    else
    {
        // We have an empty clip.  Clear out any InsetClip we might have set.
        IFCFAILFAST(m_prependVisual->put_Clip(nullptr));
    }
}

void HWCompTreeNodeWinRT::UpdatePrependOpacity(_In_ DCompTreeHost *dcompTreeHost)
{
    const bool mustUseExpression = m_pUIElementNoRef->MustAttachWUCExpression(KnownPropertyIndex::TransitionTarget_Opacity);
    const bool canReleaseExpression = m_pUIElementNoRef->CanReleaseWUCExpression(KnownPropertyIndex::TransitionTarget_Opacity);

    // Update the Opacity
    if (mustUseExpression)
    {
        WinRTExpressionConversionContext context(dcompTreeHost->GetCompositor());
        m_pUIElementNoRef->GetTransitionTarget()->EnsureWUCOpacityExpression(&context, m_prependOpacity);

        const auto& opacityExpression = m_pUIElementNoRef->GetTransitionTarget()->GetWUCOpacityExpression();
        if (m_transitionTargetOpacityExpressionCache.Get() != opacityExpression)
        {
            wrl::ComPtr<WUComp::ICompositionObject> visualICO;
            IFCFAILFAST(m_prependVisual.As(&visualICO));
            wrl::ComPtr<WUComp::ICompositionAnimation> opacityExpressionCA;
            IFCFAILFAST(opacityExpression->QueryInterface(IID_PPV_ARGS(&opacityExpressionCA)));
            IFCFAILFAST(visualICO->StartAnimation(wrl_wrappers::HStringReference(ExpressionHelper::sc_OpacityPropertyName).Get(), opacityExpressionCA.Get()));

            m_transitionTargetOpacityExpressionCache = opacityExpression;
        }
    }
    else
    {
        float staticOpacity = m_prependOpacity;

        if (m_pUIElementNoRef->GetTransitionTarget() != nullptr)
        {
            if (canReleaseExpression)
            {
                m_pUIElementNoRef->GetTransitionTarget()->ClearWUCOpacityExpression();
            }
            staticOpacity *= m_pUIElementNoRef->GetTransitionTarget()->m_opacity;
        }

        IFCFAILFAST(m_prependVisual->put_Opacity(staticOpacity));

        m_transitionTargetOpacityExpressionCache.Reset();
    }
}

void HWCompTreeNodeWinRT::UpdatePrependPixelSnapping(bool disablePixelSnapping)
{
    // Pixel Snapping
    // Manipulatable CompNodes for primary content get pixel snapping on their PrependVisual.
    // This is primarily done in order to ensure Sticky Headers don't "jiggle" as you pan.
    // The problem with Sticky Headers is that the entire ListView control may land on a sub-pixel boundary,
    // particularly when used within a Semantic Zoom control which applies multiple render transforms, and
    // doesn't guarantee the ListView will land on a pixel boundary, even when LayoutRounding is in effect.
    // In this scenario jiggling is likely to occur due to how pixel snapping math works out in this configuration.
    // Consider the following example visual sub-tree:
    //
    // GrandParent (translateY = 0.3)   => PrependVisual for ItemsPresenter
    //   |
    // Parent (translateY = -100.5)     => TransformVisual for ItemsPresenter
    //   |
    // <Visuals omitted>
    //   |
    // Child  (translateY = 100.5)      => TransformVisual for StickyHeader
    //
    // GrandParent does not have pixel snapping, but Parent and Child both have pixel snapping turned on.
    // When DComp renders this sub-tree, and it reaches Parent, it will pixel snap Parent's world transform:
    // Parent translateY = round(0.3 + (-100.5)) = -100
    // When DComp moves on to compute the world transform for Child, it starts with Parent's world transform,
    // which has already been pixel snapped, and we get this:
    // Child translateY = round(-100 + 100.5) = 1
    // We should have ended up with 0 for Child.  As you pan, what ends up happening is Parent and Child pixel
    // snap at different times and Child appears to jiggle up and down.
    // By turning on pixel snapping for the PrependVisual, we help guarantee the primary and secondary content
    // stay aligned with each other and we avoid the jiggling.
    // We scope this only to the primary manipulatable content of ItemsPresenters as this is the only known problematic scenario.
    // To prevent jittering, don't pixel snap if a transform is being animated.

    bool hasIndependentTransformManipulation = HasIndependentTransformManipulation();
    bool isPrimaryManipulatableContent = hasIndependentTransformManipulation && (m_spDManipData->GetManipulationContentType() == XcpDMContentTypePrimary);

    const bool isPixelSnappingEnabled =
        hasIndependentTransformManipulation &&
        isPrimaryManipulatableContent &&
        !disablePixelSnapping &&
        m_pUIElementNoRef->OfTypeByIndex<KnownTypeIndex::ItemsPresenter>();

    wrl::ComPtr<ixp::IVisual4> visual4;
    VERIFYHR(m_prependVisual.As(&visual4));
    IFCFAILFAST(visual4->put_IsPixelSnappingEnabled(isPixelSnappingEnabled));
}

// DropShadow visual design notes
// Structure:
//
// Prepend
// |                    \
// DropShadowParent      Primary
// |                     |
// DropShadow            etc
//
// There will be two new visuals:
//   -DropShadowParent:  Must be an interop visual, needed for legacy DComp features
//   -DropShadow:  A SpriteVisual which draws the drop shadow
// These visuals are parented under the PrependVisual, to escape clips in the main spine.
//
// Escaping clips:  The drop shadow will escape most clips, but not all:
//
//  -Prepend Clip:  No.  This is the clip that comes from an ancestor
//  -Layout Clip:  Only when applied as a self clip, not when applied as ancestor clip
//  -UIElement.Clip:  Yes.
//  -ThemeTransition Clip:  No.  This is an “entrance animation”.  We may need to expand the bounds
//   of the animating clip to include shadow if it’s noticeable
//  -RoundedCorner Clip:  Yes – we’re drawing rounded corners ourselves
//
// Visual properties:
//
// -Opacity:  Will duplicate PrimaryVisual.Opacity
// -Clip: Will dupliacte ThemeTransition.Clip
// -Transform:  Will have TransformParent => Primary visual.  The TransformParnet is needed
//  because the drop shadow is parented to the PrependVisual, but needs the transform from Primary visual
// -Bounds: For the most part, receives the bounds of the UIElement.
//  For any clip that is escaped, will clip the bounds down by that clip.

void HWCompTreeNodeWinRT::UpdateDropShadowVisual(_In_ DCompTreeHost *dcompTreeHost)
{
    if (NeedsDropShadowVisual())
    {
        // Determine the drop shadow recipe and corner radius
        CValue value;
        IFCFAILFAST(m_pUIElementNoRef->GetValueByIndex(KnownPropertyIndex::FrameworkElement_ActualTheme, &value));
        auto theme = static_cast<DirectUI::ElementTheme>(value.AsEnum());

        // Assume a static translation Z. We don't support animating Z translation + shadows.
        CUIElement* elementWithThemeShadow = GetElementForLTEProperties();
        CUIElement* parent = elementWithThemeShadow->GetUIElementAdjustedParentInternal();
        if (parent && parent->OfTypeByIndex<KnownTypeIndex::Popup>() && parent->IsShadowCaster())
        {
            elementWithThemeShadow = parent;
        }

        wfn::Vector3 translation = elementWithThemeShadow->GetTranslation();
        float translationZ = translation.Z;

        DropShadowRecipe recipe = GetDropShadowRecipe(translationZ, theme);
        EnsureDropShadowVisual(dcompTreeHost);
        UpdateDropShadowRecipeCornerRadius(recipe);
        UpdateDropShadowVisualBrush(dcompTreeHost, recipe);
        UpdateDropShadowVisualBounds(dcompTreeHost, recipe);
        UpdateDropShadowVisualTransform(dcompTreeHost);
        UpdateDropShadowVisualOpacity(dcompTreeHost);
        UpdateDropShadowVisualForThemeTransitionClip(dcompTreeHost);

        wrl::ComPtr<ixp::IVisual3> visual3;
        VERIFYHR(m_dropShadowParentVisual.As(&visual3));
        IFCFAILFAST(visual3->put_IsHitTestVisible(false));
    }
    else
    {
        CleanupDropShadowVisual();
    }
}

void HWCompTreeNodeWinRT::UpdateDropShadowRecipeCornerRadius(_Inout_ DropShadowRecipe& recipe)
{
    for (CUIElement* pElement = GetElementForLTEProperties();
            pElement != nullptr;
            pElement = pElement->GetUIElementAdjustedParentInternal(false))
    {
        if (pElement->OfTypeByIndex<KnownTypeIndex::Rectangle>())
        {
            CRectangle* rect = static_cast<CRectangle*>(pElement);
            if (rect->m_eRadiusX != 0 || rect->m_eRadiusY != 0)
            {
                recipe.RadiusX = rect->m_eRadiusX;
                recipe.RadiusY = rect->m_eRadiusY;
                break;
            }
        }
        else
        {
            XCORNERRADIUS cornerRadius = {0, 0, 0, 0};
            // Since Control doesn't override FrameworkElement::GetCornerRadius, we'll have to detect
            // if the element is a Control, and if so, get the corner radius from Control_CornerRadius instead.
            if (pElement->OfTypeByIndex<KnownTypeIndex::Control>())
            {
                CControl* control = static_cast<CControl*>(pElement);
                CValue value;
                IFCFAILFAST(control->GetValueByIndex(KnownPropertyIndex::Control_CornerRadius, &value));
                cornerRadius = *value.AsCornerRadius();
            }
            else if (pElement->OfTypeByIndex<KnownTypeIndex::FrameworkElement>())
            {
                CFrameworkElement* frameworkElement = static_cast<CFrameworkElement*>(pElement);
                cornerRadius = frameworkElement->GetCornerRadius();
            }

            const bool hasRoundedCorner =
            cornerRadius.topLeft != 0
            || cornerRadius.topRight != 0
            || cornerRadius.bottomLeft != 0
            || cornerRadius.bottomRight != 0;

            if (hasRoundedCorner)
            {
                // Since we're creating our DropShadow with rectangles, we'll want to
                // use radiusX and radiusY values. So if the element gives us an XCORNERRADIUS,
                // fill in the recipe's equivalent radiusX and radiusY values.
                XFLOAT cr = std::max(std::max(cornerRadius.topLeft, cornerRadius.topRight), std::max(cornerRadius.bottomLeft, cornerRadius.bottomRight));
                recipe.RadiusX = cr;
                recipe.RadiusY = cr;
                break;
            }
        }
    }
}

D2D1_RECT_F GetRect(wfn_::float2 offset, wfn_::float2 size)
{
    D2D1_RECT_F rect;
    rect.left = offset.x;
    rect.top = offset.y;
    rect.right = offset.x + size.x;
    rect.bottom = offset.y + size.y;
    return rect;
}

// Update the size and offset using the NineGridBrush's insets.
void HWCompTreeNodeWinRT::UpdateDropShadowVisualBounds(_In_ DCompTreeHost *dcompTreeHost, _In_ const DropShadowRecipe& recipe)
{
    // Update the offset
    wfn::Vector3 offset;
    offset.X = -recipe.Insets.left;
    offset.Y = -recipe.Insets.top;
    offset.Z = 0;
    IFCFAILFAST(m_dropShadowSpriteVisual->put_Offset(offset));

    // Update the Size
    wfn::Vector2 size;
    size.X = m_pUIElementNoRef->GetActualWidth() + recipe.Insets.left + recipe.Insets.right;
    size.Y = m_pUIElementNoRef->GetActualHeight() + recipe.Insets.top + recipe.Insets.bottom;
    IFCFAILFAST(m_dropShadowSpriteVisual->put_Size(size));
}

void HWCompTreeNodeWinRT::UpdateDropShadowVisualBrush(_In_ DCompTreeHost *dcompTreeHost, _In_ const DropShadowRecipe& recipe)
{
    wrl::ComPtr<ixp::ISpriteVisual> dropShadowSV;
    IFCFAILFAST(m_dropShadowSpriteVisual.As(&dropShadowSV));

    const auto& wucBrushManager = dcompTreeHost->GetWUCBrushManager();
    if (auto cachedBrush = wucBrushManager->GetDropShadowBrushFromCache(recipe))
    {
        dropShadowSV->put_Brush(cachedBrush.Get());
    }
    else
    {
        wrl::ComPtr<ixp::IVisual> dropShadowVisual;
        CreateDropShadowVisual(dcompTreeHost, &dropShadowVisual, recipe);

        // We'll need to put the dropShadowVisual inside of a surface to be the source of the NineGrid brush.
        wrl::ComPtr<ixp::ICompositionVisualSurface> dropShadowVS;
        wrl::ComPtr<ixp::ICompositorWithVisualSurface> compositorVisualSurface;
        IFCFAILFAST(dcompTreeHost->GetCompositor()->QueryInterface(IID_PPV_ARGS(&compositorVisualSurface)));
        IFCFAILFAST(compositorVisualSurface->CreateVisualSurface(&dropShadowVS));
        dropShadowVS->put_SourceVisual(dropShadowVisual.Get());

        wfn::Vector2 shadowVisualSize;
        dropShadowVisual->get_Size(&shadowVisualSize);
        dropShadowVS->put_SourceSize(shadowVisualSize);

        wrl::ComPtr<ixp::ICompositionVisualSurfacePartner> visualSurfacePartner;
        IFCFAILFAST(dropShadowVS.As(&visualSurfacePartner));
        // TODO: Make sure to test in high DPI
        visualSurfacePartner->put_RealizationSize(shadowVisualSize); // Required to avoid dwm.exe picking an arbitrary size that will cause bluriness.

        wrl::ComPtr<ixp::ICompositionSurfaceBrush> dropShadowSurfaceBrush;
        wrl::ComPtr<ixp::ICompositionSurface> dropShadowSurface;
        IFCFAILFAST(dropShadowVS.As(&dropShadowSurface));
        IFCFAILFAST(dcompTreeHost->GetCompositor()->CreateSurfaceBrushWithSurface(dropShadowSurface.Get(), &dropShadowSurfaceBrush));
        dropShadowSurfaceBrush->put_Stretch(ixp::CompositionStretch_Fill); // required for use as CompositionNineGridBrush source
        wrl::ComPtr<ixp::ICompositionBrush> dropShadowBrush;
        IFCFAILFAST(dropShadowSurfaceBrush.As(&dropShadowBrush));

        // Since the shape used to generate the shadow is rounded, the insets must extend to include its corner radius.
        // Otherwise the nine grid won't extend into the rounded corners of the content, and there will be no shadow behind
        // the rounded corners.
        // See picture in GetDropShadowRecipe.
        const XFLOAT maxCR = std::max(recipe.RadiusX, recipe.RadiusY);
        const XTHICKNESS nineGrid = {
            recipe.Insets.left + maxCR + 1, // We're extending the insets by a pixel to overlap the real visual.
            recipe.Insets.top + maxCR + 1,  // See diagram and calculations in CreateDropShadowVisual below.
            recipe.Insets.right + maxCR + 1,
            recipe.Insets.bottom + maxCR + 1 };
        wrl::ComPtr<ixp::ICompositionNineGridBrush> nineGridBrush = wucBrushManager->GetNineGridBrush(nullptr, &nineGrid, 1.0, 1.0, true);
        IFCFAILFAST(nineGridBrush->put_Source(dropShadowBrush.Get()));

        // Update the visual with the NineGrid brush and cache it.
        wrl::ComPtr<ixp::ICompositionBrush> nineGridCompBrush;
        nineGridBrush.As(&nineGridCompBrush);
        IFCFAILFAST(dropShadowSV->put_Brush(nineGridCompBrush.Get()));

        wucBrushManager->PutDropShadowBrushInCache(recipe, nineGridCompBrush);
    }
}

// Creates this visual tree:
// <ContainerVisual>         <!-- Sized to largest shadow's visible bounds.  -->
//   <ContainerVisual.Clip>
//     <GeometricClip/>      <!-- Removes the pixels rendered by the ShapeVisuals (making the center transparent) but keeps the pixels rendered by the shadows. -->
//   </ContainerVisual.Clip>
//   <LayerVisual>           <!-- Renders ambient shadow -->
//     <LayerVisual.Shadow>
//       <DropShadow>
//     </LayerVisual.Shadow>
//     <ShapeVisual/>        <!-- Renders solid rounded rect, horizontally and vertically centered -->
//   </LayerVisual>
//   <LayerVisual>           <!-- Renders directional shadow -->
//     <LayerVisual.Shadow>
//       <DropShadow>
//     </LayerVisual.Shadow>
//     <ShapeVisual/>        <!-- Renders solid rounded rect, horizontally and vertically centered -->
//   </LayerVisual>
// </ContainerVisual>
void HWCompTreeNodeWinRT::CreateDropShadowVisual(_In_ DCompTreeHost *dcompTreeHost, _Out_ ixp::IVisual** visual, _In_ const DropShadowRecipe& recipe)
{
    // Use whole numbers to avoid alignment problems between ShapeVisual used to generate the shadow and geometric clip
    // applied to remove the ShapeVisual from the final result

    //
    // There's some subtle trickiness we need to be careful of regarding the hollow element. The drop shadow recipe
    // creates a dummy rounded corner shape, put a drop shadow on it, then punches out a hole over the dummy shape
    // with a rounded corner clip matching it. This leaves us with an image of just the drop shadow, which we draw
    // around the real rounded corner content.
    //
    // In practice, this recipe of punching out the center and fitting it exactly over the real visual creates
    // rendering artifacts around the rounded corners of the real visual. Consider this situation, which is a
    // zoomed-in view of the top-left corner of a rounded corner border with a shadow:
    //    - The pixels labeled with "d s" are filled with the drop shadow.
    //    - The pixels labeled with " V " are filled with the real visual. These correspond to the hole that have
    //      been punched out.
    //    - The pixels labeled with " * " are the antialiased rounded corners of the real visual.
    //
    //      +---+---+---+---+---+---+
    //      |d s|d s|d s|d s|d s|d s|
    //      +---+---+---+---+---+---+
    //      |d s|d s|d s|d s| * | V | <- real visual goes here
    //      +---+---+---+---+---+---+
    //      |d s|d s| * | * | V | V | <- real visual
    //      +---+---+---+---+---+---+
    //      |d s|d s| * | V | V | V | <- real visual
    //      +---+---+---+---+---+---+
    //      |d s| * | V | V | V | V | <- real visual
    //      +---+---+---+---+---+---+
    //      |d s| V | V | V | V | V | <- real visual
    //      +---+---+---+---+---+---+
    //            ^   ^   ^   ^   ^
    //          real visual goes here
    //
    // The antialiased rounded corners have transparency, so they will blend with whatever content is behind them.
    // In this case, the content behind them could very different in color from the drop shadow. That will make the
    // blended result stand out against the the color of the drop shadow. This effect becomes especially visible for
    // pixels that are barely part of the antialiased rounded corner, because they'll have high transparency and
    // pick up most of whatever color is under them.
    //
    // To avoid this ugliness, we size the dummy shape slightly smaller than normal, which results in us punching out
    // a smaller hole. But we don't change how the real visual fits over this image of the drop shadow. This way,
    // the outside of the real visual fits over a ring of pixels from the drop shadow. The previous picture becomes:
    //
    //      +---+---+---+---+---+---+
    //      |d s|d s|d s|d s|d s|d s|
    //      +---+---+---+---+---+---+
    //      |d s|d s|d s|d s|d*s|dVs| <- real visual goes here
    //      +---+---+---+---+---+---+
    //      |d s|d s|d*s|d*s| V | V | <- real visual
    //      +---+---+---+---+---+---+
    //      |d s|d s|d*s| V | V | V | <- real visual
    //      +---+---+---+---+---+---+
    //      |d s|d*s| V | V | V | V | <- real visual
    //      +---+---+---+---+---+---+
    //      |d s|dVs| V | V | V | V | <- real visual
    //      +---+---+---+---+---+---+
    //            ^   ^   ^   ^   ^
    //          real visual goes here
    //
    // Now there's some overlap between the drop shadow and the real visual. Those antialiased corners of the real
    // visual will blend on top of the inner edge of the drop shadow, which creates a smooth transition out to the
    // rest of the drop shadow.
    //
    // The catch is that we've effectively pushed the entire drop shadow into the real visual by a pixel on all sides,
    // where it will be covered up. To compensate for that, we increase the blur radius on the drop shadow slightly
    // to make up for the lost pixel.
    //
    // As for the math for how we're doing this, the drop shadow image that we draw consists of insets and the dummy
    // visual. Each inset contains the drop shadow, plus some potential extra space. There is no overlap between the
    // shadows in the image and the real visual.
    //
    //      |  left inset |  <-- dummy width -->  | right inset |
    //      | <--------------- container size  ---------------> |
    //         |  shadow  |         dummy         |  shadow  |
    //                    |    real visual size   |
    //
    // We're reducing the dummy visual by 1px on each side, and bumping up the insets to match. We're also bumping
    // up the shadow radius to compensate for the reduced dummy visual size. The shadow moves toward the center of
    // the image due to the reduced dummy visual size. The real visual size stays the same, which gives us a pixel
    // of overlap with the shadows in the image:
    //
    //      |  left inset' | <-- dummy width'--> | right inset' |
    //      | <--------------- container size' ---------------> |
    //         |  shadow'  |        dummy        |  shadow'  |
    //                    |    real visual size   |
    //
    // So the calculations are:
    //      left inset' = left inset + 1
    //      right inset' = right inset + 1
    //      dummy width' = dummy width - 2
    //      dummy offset' = left inset' = left inset + 1
    //      container size' = container size = dummy width + left inset + right inset
    //      shadow' = shadow + 1
    //

    const wfn_::float2 roundedRectSize{
        // For simplicity we make the content square.
        std::max(
            // The content must be large enough to accommodate rounded corners on both sides.
            std::roundf(2 * std::max(recipe.RadiusX, recipe.RadiusY)),
            // A rect that's too small will cast a very light shadow at large shadow radiuses. Clamp to a min value.
            64.0f
        )
    };
    const wfn_::float2 roundedRectSizeSmaller { roundedRectSize.x - 2, roundedRectSize.y - 2 };

    // (0, 0) is the top-left corner of the shadow region. The content rounded rect renders at an offset.
    // Note: We add 1 here to both dimensions to keep the smaller dummy shape centered in the area that the real visual
    // is going to cover.
    const wfn_::float3 roundedRectOffset{ recipe.Insets.left + 1, recipe.Insets.top + 1, 0 };

    //
    // This is the bounding box of the shadow being cast by the content rounded rect. This also defines the size of
    // the drop shadow image that we'll draw using a hollow nine grid brush.
    //
    // Note: recipe.Insets is the amount of space allotted to the shadow, but from empirical measurements the shadow
    // won't actually fill all of this space. There will be a few fully transparent pixels along the edges. This doesn't
    // cause any problems because we're doing a 1:1 mapping from the shadow surface to the visual - the visual that we
    // produce will be large enough to accommodate these empty pixels.
    //
    const wfn_::float2 containerSize{
        roundedRectSize.x + recipe.Insets.left + recipe.Insets.right,
        roundedRectSize.y + recipe.Insets.top + recipe.Insets.bottom
    };

    // First we'll create the RoundedRectangleShape that will be put inside
    // both the AmbientShapeVisual and DirectionalShapeVisual.
    wrl::ComPtr<ixp::ICompositionRoundedRectangleGeometry> roundedRectangleGeometry;
    IFCFAILFAST(dcompTreeHost->GetCompositor5()->CreateRoundedRectangleGeometry(&roundedRectangleGeometry));
    roundedRectangleGeometry->put_Size(roundedRectSizeSmaller);
    roundedRectangleGeometry->put_CornerRadius({recipe.RadiusX, recipe.RadiusY});

    wrl::ComPtr<ixp::ICompositionSpriteShape> roundedRectangleSS;
    IFCFAILFAST(dcompTreeHost->GetCompositor5()->CreateSpriteShape(&roundedRectangleSS));
    wrl::ComPtr<ixp::ICompositionGeometry> roundedRectangleCG;
    IFCFAILFAST(roundedRectangleGeometry.As(&roundedRectangleCG));
    roundedRectangleSS->put_Geometry(roundedRectangleCG.Get());

    // Give the rectangle shape an opaque color - the color will be removed by a clip, but it must be opaque
    // since the DropShadow will inherit from the rectangle's opacity.
    wrl::ComPtr<ixp::ICompositionColorBrush> rectFillColor;
    IFCFAILFAST(dcompTreeHost->GetCompositor()->CreateColorBrushWithColor({ 255, 0, 0, 0 }, &rectFillColor));
    wrl::ComPtr<ixp::ICompositionBrush> fillBrush;
    IFCFAILFAST(rectFillColor.As(&fillBrush));
    roundedRectangleSS->put_FillBrush(fillBrush.Get());

    wrl::ComPtr<ixp::ICompositionShape> roundedRectangleShape;
    IFCFAILFAST(roundedRectangleSS.As(&roundedRectangleShape));

    // The AmbientLayerVisual has a rounded rectangle shape visual inside as the dummy shape that casts the shadow.
    // The layer visual is sized to include the shadow. We set the shadow's source policy to InheritFromVisualContent
    // so that only the shape inside that draws pixels casts the shadow.
    //
    // At low elevations, there should not be an ambient shadow. This is marked by setting the ambient opacity in the
    // recipe to 0.
    wrl::ComPtr<ixp::IVisual> ambientLayerVisual { nullptr };
    if (recipe.AmbientOpacity > 0)
    {
        // Bump up the blur radius to compensate for pushing the shadow to overlap with the real visual.
        wrl::ComPtr<ixp::ICompositionShadow> ambientShadow = MakeDropShadow(dcompTreeHost, recipe.AmbientBlurRadius + 1, recipe.AmbientColor, recipe.AmbientYOffset);

        wrl::ComPtr<ixp::IVisual> ambientShapeVisual = MakeShapeVisual(dcompTreeHost, roundedRectangleShape.Get(), roundedRectSizeSmaller, roundedRectOffset);

        ambientLayerVisual = MakeLayerVisual(dcompTreeHost, ambientShadow.Get(), ambientShapeVisual.Get(), containerSize);
    }

    // The DirectionalLayerVisual also has a rounded rectangle shape visual inside as the dummy shape that casts the
    // shadow. The layer visual is sized to include the shadow, and again we set the shadow's source policy to
    // InheritFromVisualContent so that only the dummy shape casts the shadow and not the entire layer visual. This
    // shadow exists at all elevations.
    // Bump up the blur radius to compensate for pushing the shadow to overlap with the real visual.
    wrl::ComPtr<ixp::ICompositionShadow> directionalShadow = MakeDropShadow(dcompTreeHost, recipe.DirectionalBlurRadius + 1, recipe.DirectionalColor, recipe.DirectionalYOffset);

    wrl::ComPtr<ixp::IVisual> directionalShapeVisual = MakeShapeVisual(dcompTreeHost, roundedRectangleShape.Get(), roundedRectSizeSmaller, roundedRectOffset);

    wrl::ComPtr<ixp::IVisual> directionalLayerVisual = MakeLayerVisual(dcompTreeHost, directionalShadow.Get(), directionalShapeVisual.Get(), containerSize);

    // Now to combine the Ambient and Directional visuals, we'll put them in a container visual.
    wrl::ComPtr<ixp::IContainerVisual> shadowVisualContainer;
    IFCFAILFAST(dcompTreeHost->GetCompositor()->CreateContainerVisual(&shadowVisualContainer));
    wrl::ComPtr<ixp::IVisualCollection> shadowVisualCollection;
    shadowVisualContainer->get_Children(&shadowVisualCollection);
    if (ambientLayerVisual)
    {
        shadowVisualCollection->InsertAtTop(ambientLayerVisual.Get());
    }
    shadowVisualCollection->InsertAtTop(directionalLayerVisual.Get());
    wrl::ComPtr<ixp::IVisual> shadowVisual;
    IFCFAILFAST(shadowVisualContainer.As(&shadowVisual));
    shadowVisual->put_Size(containerSize);

    // Now we'll clip out the rounded rectangle from our visual.
    // We'll have to create the clip using D2D since it's able to combine geometries
    // TODO: Cache this factory as it may be a bit expensive
    const auto& wucBrushManager = dcompTreeHost->GetWUCBrushManager();
    wrl::ComPtr<ID2D1Factory1> d2d1Factory = wucBrushManager->EnsureDropShadowD2DFactory();

    wrl::ComPtr<ID2D1RectangleGeometry> d2OuterRectGeometry;
    VERIFYHR(d2d1Factory->CreateRectangleGeometry(GetRect({}, containerSize), d2OuterRectGeometry.ReleaseAndGetAddressOf()));

    wrl::ComPtr<ID2D1RoundedRectangleGeometry> d2RoundedRectGeometry;
    D2D1_ROUNDED_RECT d2RoundedRect;
    d2RoundedRect.rect = GetRect({ roundedRectOffset.x, roundedRectOffset.y }, { roundedRectSizeSmaller.x, roundedRectSizeSmaller.y });
    d2RoundedRect.radiusX = recipe.RadiusX;
    d2RoundedRect.radiusY = recipe.RadiusY;
    VERIFYHR(d2d1Factory->CreateRoundedRectangleGeometry(d2RoundedRect, d2RoundedRectGeometry.ReleaseAndGetAddressOf()));

    wrl::ComPtr<ID2D1PathGeometry> d2PathGeometry;
    VERIFYHR(d2d1Factory->CreatePathGeometry(d2PathGeometry.ReleaseAndGetAddressOf()));
    wrl::ComPtr<ID2D1GeometrySink> sink;
    VERIFYHR(d2PathGeometry->Open(sink.ReleaseAndGetAddressOf()));
    VERIFYHR(d2OuterRectGeometry->CombineWithGeometry(d2RoundedRectGeometry.Get(), D2D1_COMBINE_MODE_EXCLUDE, nullptr, sink.Get()));
    VERIFYHR(sink->Close());

    wrl::ComPtr<CGeometrySource> cgeoSource;
    cgeoSource.Attach(new CGeometrySource());
    cgeoSource->UpdateD2DGeometry(d2PathGeometry.Get());

    wrl::ComPtr<ixp::ICompositionPath> shadowPath;
    ixp::ICompositionPathFactory* pathFactoryNoRef = nullptr;
    if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_46468883>())
    {
        pathFactoryNoRef = ActivationFactoryCache::GetActivationFactoryCache()->GetPathFactory();
    }
    else
    {
        pathFactoryNoRef = dcompTreeHost->GetPathFactory();
    }
    IFCFAILFAST(pathFactoryNoRef->Create(cgeoSource.Get(), &shadowPath));

    wrl::ComPtr<ixp::ICompositionPathGeometry> compPG;
    IFCFAILFAST(dcompTreeHost->GetCompositor5()->CreatePathGeometry(&compPG));
    IFCFAILFAST(compPG->put_Path(shadowPath.Get()));
    wrl::ComPtr<ixp::ICompositionGeometry> compPgBase;
    VERIFYHR(compPG.As(&compPgBase));
    wrl::ComPtr<ixp::ICompositionGeometricClip> geoClip;
    IFCFAILFAST(dcompTreeHost->GetCompositor6()->CreateGeometricClipWithGeometry(compPgBase.Get(), geoClip.ReleaseAndGetAddressOf()));
    wrl::ComPtr<ixp::ICompositionClip> clip;
    VERIFYHR(geoClip.As(&clip));
    shadowVisual->put_Clip(clip.Get());

    *visual = shadowVisual.Detach();
}

wrl::ComPtr<ixp::ICompositionShadow> HWCompTreeNodeWinRT::MakeDropShadow(_In_ DCompTreeHost *dcompTreeHost, const float blurRadius, const wu::Color color, const float offsetY)
{
    wrl::ComPtr<ixp::IDropShadow> dropShadow;
    IFCFAILFAST(dcompTreeHost->GetCompositor2()->CreateDropShadow(&dropShadow));
    dropShadow->put_BlurRadius(blurRadius);
    dropShadow->put_Color(color);
    dropShadow->put_Offset({ 0, offsetY, 0 });

    wrl::ComPtr<ixp::IDropShadow2> dropShadow2;
    IFCFAILFAST(dropShadow.As(&dropShadow2));
    dropShadow2->put_SourcePolicy(ixp::CompositionDropShadowSourcePolicy_InheritFromVisualContent);

    wrl::ComPtr<ixp::ICompositionShadow> compositionShadow;
    IFCFAILFAST(dropShadow.As(&compositionShadow));
    return compositionShadow;
}

wrl::ComPtr<ixp::IVisual> HWCompTreeNodeWinRT::MakeShapeVisual(_In_ DCompTreeHost *dcompTreeHost, _In_ ixp::ICompositionShape* compositionShape, const wfn_::float2 size, const wfn_::float3 offset)
{
    wrl::ComPtr<ixp::IShapeVisual> shapeVisual;
    IFCFAILFAST(dcompTreeHost->GetCompositor5()->CreateShapeVisual(&shapeVisual));

    wrl::ComPtr<wfc::IVector<ixp::CompositionShape*>> shapeCollection;
    IFCFAILFAST(shapeVisual->get_Shapes(&shapeCollection));
    shapeCollection->Append(compositionShape);

    wrl::ComPtr<ixp::IVisual> visual;
    IFCFAILFAST(shapeVisual.As(&visual));
    visual->put_Size(size);
    visual->put_Offset(offset);
    return visual;
}

wrl::ComPtr<ixp::IVisual> HWCompTreeNodeWinRT::MakeLayerVisual(_In_ DCompTreeHost *dcompTreeHost, _In_ ixp::ICompositionShadow* shadow, _In_ ixp::IVisual* child, const wfn_::float2 size)
{
    wrl::ComPtr<ixp::ILayerVisual> layerVisual;
    IFCFAILFAST(dcompTreeHost->GetCompositor2()->CreateLayerVisual(&layerVisual));

    wrl::ComPtr<ixp::ILayerVisual2> layerVisual2;
    VERIFYHR(layerVisual.As(&layerVisual2));
    layerVisual2->put_Shadow(shadow);

    wrl::ComPtr<ixp::IContainerVisual> containerVisual;
    IFCFAILFAST(layerVisual.As(&containerVisual));
    wrl::ComPtr<ixp::IVisualCollection> children;
    containerVisual->get_Children(&children);
    children->InsertAtTop(child);

    wrl::ComPtr<ixp::IVisual> visual;
    IFCFAILFAST(layerVisual.As(&visual));
    visual->put_Size(size);
    return visual;
}

void HWCompTreeNodeWinRT::UpdateDropShadowVisualTransform(_In_ DCompTreeHost *dcompTreeHost)
{
    // The drop shadow is parented to the PrependVisual, but needs the transform from Primary visual as well.
    ComPtr<ixp::IVisual2> visual2;
    VERIFYHR(m_dropShadowParentVisual.As(&visual2));
    IFCFAILFAST(visual2->put_ParentForTransform(m_primaryVisual.Get()));
}

void HWCompTreeNodeWinRT::UpdateDropShadowVisualOpacity(_In_ DCompTreeHost *dcompTreeHost)
{
    // Notes:
    // It is better for performance if we can set a static Opacity value, as ExpressionAnimations
    // carry per-DWM-frame overhead to evaluate the expression/reference parameter.
    // The criteria for setting a static Opacity value are:
    // 1) There must not be any animation targeting UIElement.Opacity.  Animations are by definition no static.
    // 2) The hand-off visual must not be in use.  In this case, the app might be animating Visual.Opacity.
    OpacityScenario scenario = GetPrimaryOpacityScenario();
    if (scenario == OpacityScenario::StaticValue &&
        !IsUsingHandOffVisual(*m_pUIElementNoRef))
    {
        float opacity;
        IFCFAILFAST(m_primaryVisual->get_Opacity(&opacity));
        IFCFAILFAST(m_dropShadowParentVisual->put_Opacity(opacity));
        m_dropShadowOpacityExpressionCache.Reset();
    }
    else
    {
        if (m_dropShadowOpacityExpressionCache == nullptr)
        {
            wrl::ComPtr<ixp::ICompositionAnimation> opacityExpressionCA;

            IFCFAILFAST(dcompTreeHost->GetCompositor()->CreateExpressionAnimationWithExpression(
                wrl_wrappers::HStringReference(s_PrimaryOpacityExpressionString).Get(),
                m_dropShadowOpacityExpressionCache.ReleaseAndGetAddressOf()
                ));
            VERIFYHR(m_dropShadowOpacityExpressionCache.As(&opacityExpressionCA));

            wrl::ComPtr<ixp::ICompositionObject> primaryVisualCO;
            VERIFYHR(m_primaryVisual.As(&primaryVisualCO));
            opacityExpressionCA->SetReferenceParameter(
                wrl_wrappers::HStringReference(s_primaryVisualString).Get(),
                primaryVisualCO.Get()
                );

            wrl::ComPtr<ixp::ICompositionObject> dropShadowVisualCO;
            VERIFYHR(m_dropShadowParentVisual.As(&dropShadowVisualCO));
            IFCFAILFAST(dropShadowVisualCO->StartAnimation(
                wrl_wrappers::HStringReference(ExpressionHelper::sc_OpacityPropertyName).Get(),
                opacityExpressionCA.Get()
                ));
        }
    }
}

void HWCompTreeNodeWinRT::UpdateDropShadowVisualForThemeTransitionClip(_In_ DCompTreeHost *dcompTreeHost)
{
    if (HasTransitionClipAnimation() || m_pUIElementNoRef->ShouldFadeInDropShadow())
    {
        //
        // If we have a TransitionClip animation, the shadow will escape the bounds of this clip and will render incorrectly
        // To workaround this, we'll play an opacity animation that fades the shadow in with a delay of 250 ms.
        // Both TransitionClip animations that also have shadows animate for 250 ms so this will start exposing the shadow after
        // the TransitionClip animation is complete.
        //
        // If animations are disabled, then the TransitionClip animation is disabled as well, so we can skip all of this.
        //
        bool isAnimationEnabled = true;
        IGNOREHR(FxCallbacks::FrameworkCallbacks_IsAnimationEnabled(&isAnimationEnabled));
        if (isAnimationEnabled && !m_isPlayingDropShadowOpacityAnimation)
        {
            auto compositor = dcompTreeHost->GetCompositor();
            wrl::ComPtr<ixp::IScalarKeyFrameAnimation> opacityAnimation;
            wrl::ComPtr<ixp::IKeyFrameAnimation> opacityAnimation_kfa;
            wrl::ComPtr<ixp::ICompositionAnimation> opacityAnimationCA;
            IFCFAILFAST(compositor->CreateScalarKeyFrameAnimation(&opacityAnimation));
            VERIFYHR(opacityAnimation.As(&opacityAnimation_kfa));
            VERIFYHR(opacityAnimation.As(&opacityAnimationCA));

            wrl::ComPtr<ixp::ICubicBezierEasingFunction> cubicBezier;
            IFCFAILFAST(dcompTreeHost->GetEasingFunctionStatics()->CreateCubicBezierEasingFunction(compositor, wfn_::float2{0.17f, 0.8f}, wfn_::float2{0.33f, 0.99f}, &cubicBezier));
            wrl::ComPtr<ixp::ICompositionEasingFunction> easingFunction;
            IFCFAILFAST(cubicBezier.As(&easingFunction));

            IFCFAILFAST(opacityAnimation->InsertKeyFrame(0.0f, 0.0f));
            IFCFAILFAST(opacityAnimation->InsertKeyFrameWithEasingFunction(1.0f, 1.0f, easingFunction.Get()));

            IFCFAILFAST(opacityAnimation_kfa->put_DelayTime({ static_cast<INT64>(2500000) }));
            IFCFAILFAST(opacityAnimation_kfa->put_Duration({ static_cast<INT64>(5000000) }));
            IFCFAILFAST(opacityAnimation_kfa->put_StopBehavior(ixp::AnimationStopBehavior_SetToFinalValue));

            wrl::ComPtr<ixp::ICompositionObject> visualCO;
            VERIFYHR(m_dropShadowSpriteVisual.As(&visualCO));
            IFCFAILFAST(m_dropShadowSpriteVisual->put_Opacity(0.0f));   // Static 0 to apply the value while we wait for the DelayTime on the animation
            IFCFAILFAST(visualCO->StartAnimation(wrl::Wrappers::HStringReference(ExpressionHelper::sc_OpacityPropertyName).Get(), opacityAnimationCA.Get()));

            m_isPlayingDropShadowOpacityAnimation = true;
        }
    }
}

void HWCompTreeNodeWinRT::UpdatePrimaryVisual(
    _In_ DCompTreeHost *dcompTreeHost,
    bool disablePixelSnapping
    )
{
    ASSERT(m_primaryVisual != nullptr);

    // Using ImplicitAnimationDisabler here implements a basic policy to not play Implicit Animations in certain situations:
    // -When the HandOff visual is getting its properties set for the very first time after being created
    // -When the HandOff visual is changing where it is in the visual tree due to the XAML element it's attached to moving around in the XAML tree
    // These scenarios are currently easy to detect today as we always create a new CompNode in these situations.
    // The policy is therefore easy to implement:  On the first PushProperties(), temporarily disable implicit animations while we set properties.
    // Note:  If and when we change the CompNode creation policies (specifically, when we go 1:1)  this will need to be revisited,
    // particularly if we start keeping CompNodes alive while moving their corresponding UIElement around in the XAML tree.
    ImplicitAnimationDisabler disabler(m_primaryVisual.Get(), !m_hasEverHadPropertiesPushed);

    // Update the Size
    wfn::Vector2 size;
    size.X = m_pUIElementNoRef->GetActualWidth();
    size.Y = m_pUIElementNoRef->GetActualHeight();
    IFCFAILFAST(m_primaryVisual->put_Size(size));

    UpdatePrimaryVisualOffset(dcompTreeHost->GetCompositor());

    UpdatePrimaryVisualOpacity(dcompTreeHost->GetCompositor());

    UpdatePrimaryVisualTransformMatrix(dcompTreeHost);
    UpdatePrimaryVisualTransformParent(dcompTreeHost);

    // No need to check for viewport interactions. It's on a partner interface and the app cannot set it.
    UpdatePrimaryVisualViewportInteraction(dcompTreeHost);

    UpdatePrimaryVisualCompositeMode();

    // No need to check for default pixel snapping. It's on a partner interface and the app cannot set it.
    UpdatePrimaryVisualPixelSnapping(disablePixelSnapping);

    // No need to check for hit test visibility. It's on a partner interface and the app cannot set it.
    UpdatePrimaryVisualHitTestVisibility();

    UpdatePrimaryVisualLights(dcompTreeHost);
}

void HWCompTreeNodeWinRT::UpdatePrimaryVisualOffset(_In_ WUComp::ICompositor* compositor)
{
    if (m_pUIElementNoRef->IsOffsetIndependentlyAnimating())
    {
        WinRTExpressionConversionContext context(compositor);
        m_pUIElementNoRef->EnsureWUCOffsetExpression(&context);

        const auto& offsetExpression = m_pUIElementNoRef->GetWUCCanvasOffsetExpression();

        wrl::ComPtr<WUComp::ICompositionObject> visualICO;
        IFCFAILFAST(m_primaryVisual.As(&visualICO));
        wrl::ComPtr<WUComp::ICompositionAnimation> offsetExpressionCA;
        IFCFAILFAST(offsetExpression->QueryInterface(IID_PPV_ARGS(&offsetExpressionCA)));
        IFCFAILFAST(visualICO->StartAnimation(wrl::Wrappers::HStringReference(ExpressionHelper::sc_OffsetPropertyName).Get(), offsetExpressionCA.Get()));

        m_previousWUCOffset.X = FloatUtil::NaN; // No need to set the other components, we'll check for NaN in this one.
    }
    else
    {
        if (!m_pUIElementNoRef->NeedsWUCOffsetExpression())
        {
            m_pUIElementNoRef->ClearWUCOffsetExpression();
        }

        // Update the static Offset
        // This is the combination of LayoutOffset + static Translation
        wfn::Vector3 offset;
        offset.X = m_pUIElementNoRef->GetActualOffsetX();
        offset.Y = m_pUIElementNoRef->GetActualOffsetY();
        offset.Z = 0;

        if (FloatUtil::IsNaN(m_previousWUCOffset.X)
            || offset.X != m_previousWUCOffset.X
            || offset.Y != m_previousWUCOffset.Y
            || offset.Z != m_previousWUCOffset.Z)
        {
            IFCFAILFAST(m_primaryVisual->put_Offset(offset));
            m_previousWUCOffset = offset;
        }
    }
}

HWCompTreeNodeWinRT::OpacityScenario HWCompTreeNodeWinRT::GetPrimaryOpacityScenario()
{
    if (m_pUIElementNoRef->IsLocalOpacityIndependentlyAnimating())
    {
        return OpacityScenario::AnimatedStoryboard;
    }

    FacadeStorage& facadeStorage = GetContext()->GetFacadeStorage();
    if (m_pUIElementNoRef->HasFacadeAnimation() && facadeStorage.HasAnimation(m_pUIElementNoRef, KnownPropertyIndex::UIElement_Opacity))
    {
        return OpacityScenario::AnimatedFacade;
    }

    return OpacityScenario::StaticValue;
}

void HWCompTreeNodeWinRT::UpdatePrimaryVisualOpacity(_In_ WUComp::ICompositor* compositor)
{
    OpacityScenario scenario = GetPrimaryOpacityScenario();
    if (scenario == OpacityScenario::AnimatedStoryboard)
    {
        WinRTExpressionConversionContext context(compositor);
        m_pUIElementNoRef->EnsureWUCOpacityExpression(&context);

        const auto& opacityExpression = m_pUIElementNoRef->GetWUCOpacityExpression();

        if (m_opacityExpressionCache.Get() != opacityExpression)
        {
            wrl::ComPtr<WUComp::ICompositionObject> visualICO;
            IFCFAILFAST(m_primaryVisual.As(&visualICO));
            wrl::ComPtr<WUComp::ICompositionAnimation> opacityExpressionCA;
            IFCFAILFAST(opacityExpression->QueryInterface(IID_PPV_ARGS(&opacityExpressionCA)));
            IFCFAILFAST(visualICO->StartAnimation(wrl::Wrappers::HStringReference(ExpressionHelper::sc_OpacityPropertyName).Get(), opacityExpressionCA.Get()));

            m_previousWUCOpacity = FloatUtil::NaN;

            m_opacityExpressionCache = opacityExpression;
        }
    }
    else if (scenario == OpacityScenario::AnimatedFacade)
    {
        wrl::ComPtr<WUComp::IExpressionAnimation> opacityExpression = EnsureFacadeGlueExpression(compositor, KnownPropertyIndex::UIElement_Opacity, s_OpacityFacadeExpressionString);
        StartFacadeGlueExpression(m_primaryVisual.Get(), ExpressionHelper::sc_OpacityPropertyName, opacityExpression.Get());
        m_previousWUCOpacity = FloatUtil::NaN;
    }
    else
    {
        ASSERT(scenario == OpacityScenario::StaticValue);

        if (!m_pUIElementNoRef->NeedsWUCOpacityExpression())
        {
            m_pUIElementNoRef->ClearWUCOpacityExpression();
        }
        m_opacityExpressionCache.Reset();

        // Update the static Opacity
        float opacity = m_pUIElementNoRef->GetOpacityLocal();

        // Opacity on elements affect the readability for visual impaired users when using HighContrast. This overrides the opacity for elements
        // that are not fully transparent to provide a better experience in HighContrast mode.
        if (ShouldOverrideRenderOpacity(opacity, m_pUIElementNoRef))
        {
            opacity = 1;
        }

        if (FloatUtil::IsNaN(m_previousWUCOpacity)
            || opacity != m_previousWUCOpacity)
        {
            IFCFAILFAST(m_primaryVisual->put_Opacity(opacity));
            m_previousWUCOpacity = opacity;
        }
    }
}

void HWCompTreeNodeWinRT::UpdateVisualClipAndTransform(_In_ DCompTreeHost *dcompTreeHost, _In_ WUComp::IVisual* visual)
{
    xref_ptr<CGeometry> uieClip = m_pUIElementNoRef->GetClip();
    const bool hasUIEClip = uieClip != nullptr;
    const bool hasUIEClipTransform = hasUIEClip && (uieClip->m_pTransform != nullptr);
    const bool hasUIEClipAnimation = m_pUIElementNoRef->IsLocalClipIndependentlyAnimating();
    const bool isManipulatable = ((m_spDManipData != nullptr) && (m_spDManipData->GetDMService() != nullptr));
    const bool hasIndependentClipManipulation = isManipulatable && (m_spDManipData->GetClipContent() != nullptr);
    const bool isClipRequiredOnPrimaryVisual = hasUIEClip || hasIndependentClipManipulation;
    bool wasClipUpdated = false;

    if (m_isInConnectedAnimation)
    {
        // If we are in a connected animation we will want to snapshot the entire element so
        // make sure there is no clip.
        IFCFAILFAST(visual->put_Clip(nullptr));
    }
    else if (isClipRequiredOnPrimaryVisual)
    {
        wrl::ComPtr<WUComp::ICompositionClip> clip;
        wrl::ComPtr<WUComp::IInsetClip> insetClip;
        IFCFAILFAST(visual->get_Clip(&clip));

        // Update the clip
        // TODO_WinRT: We always reset the clip + clip transform here. Consider having a dirty flag for these updates.
        if (clip == nullptr)
        {
            IFCFAILFAST(dcompTreeHost->GetCompositor()->CreateInsetClip(insetClip.ReleaseAndGetAddressOf()));
            IFCFAILFAST(insetClip.As(&clip));
            IFCFAILFAST(visual->put_Clip(clip.Get()));
            // No need to manually attach a property change listener to the clip. We already have one attached to the WUC visual,
            // and the visual will send a property update notification when the clip changes. We'll attach the clip listener then.
        }
        else
        {
            IFCFAILFAST(clip.As(&insetClip));
        }

        wrl::ComPtr<WUComp::ICompositionClip2> clipCC2;
        wrl::ComPtr<WUComp::ICompositionObject> clipCO;
        IFCFAILFAST(clip.As(&clipCC2));
        IFCFAILFAST(clip.As(&clipCO));

        if (hasUIEClip)
        {
            ASSERT(uieClip->OfTypeByIndex<KnownTypeIndex::RectangleGeometry>());
            CRectangleGeometry *pRectangleGeometry = static_cast<CRectangleGeometry*>(uieClip.get());

            // If the element is in strict mode, there is no reason to deal with "stomping" as we won't support the HandOff visual in this case.
            // What is more, we need to take care to push the latest value into the visual to handle transitions to/from having
            // the "Components" visual in the tree.  For simplicity, we simply always update the Clip if facades are in use.
            PreviousClipComparison comparisonType = m_pUIElementNoRef->IsStrictOnly() ? PreviousClipComparison::AlwaysSet : PreviousClipComparison::CompareAgainstPreviousClip;
            wasClipUpdated = UpdateInsetClip(pRectangleGeometry->m_rc, insetClip.Get(), comparisonType);
            m_previousHasWUCInsetClip = true;
        }

        // There are three cases for Clip Transform animations:
        // 1. Xaml app is animating UIElement.Clip.Transform directly. Used by apps to implement things like reveal animations.
        // 2. DManip-on-DComp is independently animating UIElement.Clip.Transform. Used by Xaml framework to implement sticky headers.
        // 3. There is a clip but no animation.
        // Note We do not expect a case where there is both a Xaml animated and a DManip-driven UIElement.Clip transform.
        ASSERT(!(hasUIEClipAnimation && hasIndependentClipManipulation));

        bool hasStaticTransform = false;
        CMILMatrix clipTransform(true);

        // Case 1: Xaml-based UIElement.Clip.Transform
        if (hasUIEClipTransform)
        {
            // Generate the expression
            CTransform* clipTransformNoRef = uieClip->m_pTransform;

            if (hasUIEClipAnimation)
            {
                WinRTExpressionConversionContext context(dcompTreeHost->GetCompositor());
                clipTransformNoRef->MakeWinRTExpression(&context);

                wrl::ComPtr<WUComp::IExpressionAnimation> clipTransformEA;
                wrl::ComPtr<WUComp::ICompositionAnimation> clipTransformCA;
                clipTransformEA = clipTransformNoRef->GetWinRTExpression();
                IFCFAILFAST(clipTransformEA.As(&clipTransformCA));

                IFCFAILFAST(clipCO->StartAnimation(
                    wrl_wrappers::HStringReference(ExpressionHelper::sc_propertyName_TransformMatrix).Get(),
                    clipTransformCA.Get()
                    ));

                m_previousWUCClipTransformMatrix.M11 = FloatUtil::NaN;  // No need to set the other components, we'll check for NaN in this one.
            }
            else
            {
                // Use UIElement.Clip's static transform
                clipTransformNoRef->GetTransform(&clipTransform);
                hasStaticTransform = true;
            }
        }
        else
        {
            // Make sure to unset any existing static transform
            clipTransform.SetToIdentity();
            hasStaticTransform = true;
        }

        if (hasStaticTransform)
        {
            // The two transform types have the same internal layout.
            ASSERT(sizeof(wfn::Matrix3x2) == sizeof(clipTransform));
            wfn::Matrix3x2* pwfnTransform = reinterpret_cast<wfn::Matrix3x2*>(&clipTransform);

            // If the clip rect itself was overwritten with Xaml's geometry, then overwrite the clip transform as well. The app
            // could do this:
            //   1. Set a Xaml clip geometry with no transform
            //   2. Then set a clip directly on the WUC visual along with a WUC clip transform
            //   3. Then set a Xaml clip geometry with no transform again
            //
            // We would cache the identity transform set by Xaml in step 1. When it comes time to do step 3, we would see the new
            // Xaml transform as identity, see the previous transform set by Xaml was also identity, and skip setting the clip
            // transform. The app then gets stuck with the old clip transform set in step 2 but the new clip rect set in step 3.
            //
            // To prevent this situation, whenever Xaml updates the clip rect (like it does on step 3), we also force it to update
            // the clip transform as well. This keeps the picture consistent - if Xaml applied a clip rect, then we'll use Xaml's
            // clip transform as well.

            if (wasClipUpdated || IsWUCClipTransformMatrixDifferentFromPrevious(*pwfnTransform))
            {
                IFCFAILFAST(clipCC2->put_TransformMatrix(*pwfnTransform));
                m_previousWUCClipTransformMatrix = *pwfnTransform;
            }
        }

        // Case 2: DManip-driven clip transform animation.
        if (hasIndependentClipManipulation)
        {
            DManipDataWinRT* dmanipData = static_cast<DManipDataWinRT*>(m_spDManipData.get());

            // Here, following the pattern in legacy path and recreate the shared clip transform only if
            // its been reset, but update the expression whenever we visit this comp node.
            // If we don't do the latter, we'll see incorrect clipping introduced in the middle of
            // some manipulations (eg holding down and scrolling Apps content in Start menu),
            // due to clip transform not being updated.
            if (!dmanipData->GetClipPrimaryTransformCO())
            {
                IObject* pClipContent = dmanipData->GetClipContent();
                ASSERT(pClipContent != nullptr);

                // Note that although EnsureSharedContentTransform requires two shared transform out params,
                // the secondary transform is always NULL for clip transform animations.  This is because
                // only one reflex curve is actually needed to describe how to animate the clip.
                // See additional details in DirectManipulationService::ShouldUseOverpanReflexesForContentType().
                xref_ptr<IUnknown> sharedDManipPrimaryTransform;
                xref_ptr<IUnknown> sharedDManipSecondaryTransform;
                IFCFAILFAST(
                    dmanipData->GetDMService()->EnsureSharedContentTransform(
                    dcompTreeHost->GetCompositor(),
                    pClipContent,
                    XcpDMContentTypeDescendant,
                    sharedDManipPrimaryTransform.ReleaseAndGetAddressOf(),
                    sharedDManipSecondaryTransform.ReleaseAndGetAddressOf())
                    );

                IFCFAILFAST(dmanipData->SetSharedClipTransform(sharedDManipPrimaryTransform, dcompTreeHost->GetCompositor()));
            }

            wrl::ComPtr<WUComp::IExpressionAnimation> clipOverallTransformEA;
            wrl::ComPtr<WUComp::ICompositionAnimation> clipOverallTransformCA;

            // Create overall clip transform expression. This maps the 4x4 PrimaryTransform matrix we get from DManip to
            // a 3x2 matrix expected by ICompositionClip2::TransformMatrix.
            IFCFAILFAST(dcompTreeHost->GetCompositor()->CreateExpressionAnimationWithExpression(
                wrl_wrappers::HStringReference(ExpressionHelper::sc_Expression_DManipClipTransform).Get(),
                &clipOverallTransformEA
                ));
            IFCFAILFAST(clipOverallTransformEA.As(&clipOverallTransformCA));

            // Note that the result of DManip's expression is in "Matrix" property of the PS wrapped in clipPrimaryTransformCO
            clipOverallTransformCA->SetReferenceParameter(
                wrl_wrappers::HStringReference(ExpressionHelper::sc_ClipPrimaryTransformPropertySet).Get(),
                dmanipData->GetClipPrimaryTransformCO()
                );

            IFCFAILFAST(clipCO->StartAnimation(
                wrl_wrappers::HStringReference(ExpressionHelper::sc_propertyName_TransformMatrix).Get(),
                clipOverallTransformCA.Get()
                ));
        }
    }
    // If the element is in strict mode, there is no reason to deal with "stomping" as we won't support the HandOff visual in this case.
    // What is more, we need to take care to push the latest value into the visual to handle transitions to/from having
    // the "Components" visual in the tree.  For simplicity, we simply always update the Clip if facades are in use.
    else if (m_previousHasWUCInsetClip || m_pUIElementNoRef->IsStrictOnly())
    {
        IFCFAILFAST(visual->put_Clip(nullptr));
        m_previousHasWUCInsetClip = false;

        if (IsUsingHandOffVisual(*m_pUIElementNoRef))
        {
            m_pUIElementNoRef->DetachWUCPropertyListenerFromWUCClip();
        }
    }
}

bool HWCompTreeNodeWinRT::UpdateInsetClip(
    _In_ XRECTF& regularClip,
    _Inout_ WUComp::IInsetClip* insetClip,
    PreviousClipComparison previousClipComparison)
{
    ASSERT(m_pUIElementNoRef != nullptr);
    const float rightInset = m_pUIElementNoRef->GetActualWidth() - (regularClip.X + regularClip.Width);
    const float bottomInset = m_pUIElementNoRef->GetActualHeight() - (regularClip.Y + regularClip.Height);

    const bool clipNeedsUpdate = previousClipComparison == PreviousClipComparison::AlwaysSet
        || regularClip.X != m_previousWUCInsetClip.left
        || regularClip.Y != m_previousWUCInsetClip.top
        || rightInset != m_previousWUCInsetClip.right
        || bottomInset != m_previousWUCInsetClip.bottom;

    if (clipNeedsUpdate)
    {
        IFCFAILFAST(insetClip->put_LeftInset(regularClip.X));
        IFCFAILFAST(insetClip->put_TopInset(regularClip.Y));
        IFCFAILFAST(insetClip->put_RightInset(rightInset));
        IFCFAILFAST(insetClip->put_BottomInset(bottomInset));
    }

    if (previousClipComparison == PreviousClipComparison::CompareAgainstPreviousClip)
    {
        m_previousWUCInsetClip.left = regularClip.X;
        m_previousWUCInsetClip.top = regularClip.Y;
        m_previousWUCInsetClip.right = rightInset;
        m_previousWUCInsetClip.bottom = bottomInset;
    }

    return clipNeedsUpdate;
}

bool HWCompTreeNodeWinRT::IsWUCTransformMatrixDifferentFromPrevious(const wfn::Matrix4x4 &matrix)
{
    return FloatUtil::IsNaN(m_previousWUCTransformMatrix.M11)
        || matrix.M11 != m_previousWUCTransformMatrix.M11 || matrix.M12 != m_previousWUCTransformMatrix.M12 || matrix.M13 != m_previousWUCTransformMatrix.M13 || matrix.M14 != m_previousWUCTransformMatrix.M14
        || matrix.M21 != m_previousWUCTransformMatrix.M21 || matrix.M22 != m_previousWUCTransformMatrix.M22 || matrix.M23 != m_previousWUCTransformMatrix.M23 || matrix.M24 != m_previousWUCTransformMatrix.M24
        || matrix.M31 != m_previousWUCTransformMatrix.M31 || matrix.M32 != m_previousWUCTransformMatrix.M32 || matrix.M33 != m_previousWUCTransformMatrix.M33 || matrix.M34 != m_previousWUCTransformMatrix.M34
        || matrix.M41 != m_previousWUCTransformMatrix.M41 || matrix.M42 != m_previousWUCTransformMatrix.M42 || matrix.M43 != m_previousWUCTransformMatrix.M43 || matrix.M44 != m_previousWUCTransformMatrix.M44;
}

bool HWCompTreeNodeWinRT::IsWUCClipTransformMatrixDifferentFromPrevious(const wfn::Matrix3x2 &matrix)
{
    return FloatUtil::IsNaN(m_previousWUCClipTransformMatrix.M11)
        || matrix.M11 != m_previousWUCClipTransformMatrix.M11 || matrix.M12 != m_previousWUCClipTransformMatrix.M12
        || matrix.M21 != m_previousWUCClipTransformMatrix.M21 || matrix.M22 != m_previousWUCClipTransformMatrix.M22
        || matrix.M31 != m_previousWUCClipTransformMatrix.M31 || matrix.M32 != m_previousWUCClipTransformMatrix.M32;
}

void HWCompTreeNodeWinRT::UpdatePrimaryVisualTransformMatrix(_In_ DCompTreeHost *dcompTreeHost)
{
    const bool hasIndependentTransformManipulation = HasIndependentTransformManipulation();
    const bool hasIndependentTransformAnimation = m_pUIElementNoRef->IsTransformAffectingPropertyIndependentlyAnimating();

    bool shouldFlipRTL, shouldFlipRTLInPlace;
    m_pUIElementNoRef->GetShouldFlipRTL(&shouldFlipRTL, &shouldFlipRTLInPlace);

    XFLOAT elementWidth = m_pUIElementNoRef->GetActualWidth();
    XFLOAT elementHeight = m_pUIElementNoRef->GetActualHeight();

    RedirectionTransformInfo rto;
    bool hasRedirectionTransformInfo = GetRedirectionTransformInfo(&rto);

    // In WUC, the visual.Offset goes above visual.TransformMatrix.
    //
    // In Xaml redirected rendering, we have a bunch of collected "redirected transforms" that we need to apply above the
    // visual.Offset. There's no place to put them (we don't want to make another visual), so we compensate by baking an
    // inverse offset into the TransformMatrix (to cancel the visual.Offset that WUC applies), then applying the redirected
    // transforms under that, then manually applying the Offset as part of the TransformMatrix. The inverse offset is done
    // with a "visual.Offset" term, which requires an expression for TransformMatrix.
    //
    // We'll only do this if the redirection transform contains something other than a translation. If the redirection
    // transform has only a translation, then it doesn't matter whether it gets applied above or below the offset.
    bool redirectionIsTranslationOnly = hasRedirectionTransformInfo
        ? rto.redirectionTransform->IsTranslationOnly()
        : true;

    bool requiresTransformExpression =
        hasIndependentTransformManipulation     // DManip-driven animation
        || hasIndependentTransformAnimation     // Keyframe-driven animation of RenderTransform, Transform3D, or Projection, but not Canvas.Left/Top.
        || !redirectionIsTranslationOnly;       // See block comment above - a translate only redirection transform can be correctly applied below the offset.

    if (requiresTransformExpression)
    {
        // Some part of the TransformMatrix is animating.  Here we need the more expensive Expression.
        xref_ptr<WUComp::ICompositionPropertySet> dmanipPS;
        if (hasIndependentTransformManipulation)
        {
            // Insert DManip shared transform for element's content if necessary.
            // There are two cases:
            // 1) We don't have a shared transform for this visual yet.
            // 2) The content offsets have changed.  In this case the overall property set will be NULL.
            IPALDirectManipulationService* dmService = m_spDManipData->GetDMService();
            IObject* manipulationContent = m_spDManipData->GetManipulationContent();

            DManipDataWinRT* dmanipData = static_cast<DManipDataWinRT*>(m_spDManipData.get());
            if (m_spDManipData->GetSharedPrimaryContentTransform() == nullptr ||
                dmanipData->GetOverallContentPropertySet() == nullptr)
            {
                XDMContentType contentType = m_spDManipData->GetManipulationContentType();

                if (EventEnabledContentOffsetsInfo())
                {
                    if (contentType == XcpDMContentTypePrimary || contentType == XcpDMContentTypeTopHeader || contentType == XcpDMContentTypeLeftHeader)
                    {
                        TraceContentOffsetsInfo(contentType, m_spDManipData->GetDirectManipulationContentOffsetX(), m_spDManipData->GetDirectManipulationContentOffsetY());
                    }
                }

                // Retrieve primary and secondary ManipulationTransforms from the DManip Service.
                // Typically the secondary transform will be NULL but there are some rare situations where it will be non-NULL.
                // These situations happen when the ScrollViewer sets an overpan mode to something other than "default" (DManip does over-panning).
                // Here's a breakdown of the various overpan scenarios:
                // - Overpan Suppression: This essentially turns off overpan by negating it with secondary transform.  Exposed via IScrollViewerPrivate.
                xref_ptr<IUnknown> sharedDManipPrimaryTransform;
                xref_ptr<IUnknown> sharedDManipSecondaryTransform;
                IFCFAILFAST(dmService->EnsureSharedContentTransform(
                    dcompTreeHost->GetCompositor(),
                    manipulationContent,
                    contentType,
                    sharedDManipPrimaryTransform.ReleaseAndGetAddressOf(),
                    sharedDManipSecondaryTransform.ReleaseAndGetAddressOf()));

                IFCFAILFAST(m_spDManipData->SetSharedContentTransforms(sharedDManipPrimaryTransform, sharedDManipSecondaryTransform, dcompTreeHost->GetCompositor()));
                IFCFAILFAST(dmanipData->EnsureOverallContentPropertySet(dcompTreeHost->GetCompositor()));
            }

            dmanipPS = dmanipData->GetOverallContentPropertySet();
        }

        WinRTLocalExpressionBuilder builder(dcompTreeHost->GetCompositor(),  m_primaryVisual.Get(), &m_primaryVisualExpressionCache);

        // The task of building the local expression is split into phases:
        // Call into the builder directly to handle 3D and Projection.  GetLocalTransform doesn't know how to deal with 3D (yet).
        // Call into the UIElement to fill in the properties relating to the 2D transform via GetLocalTransform.
        // Call into the builder to take the results from previous steps and build/apply a final ExpressionAnimation, which is stored in the WinRTLocalExpressionCache.
        ApplyProjectionToLocalTransformExpression(dcompTreeHost, &builder);
        ApplyTransform3DToLocalTransformExpression(dcompTreeHost, &builder);

        m_pUIElementNoRef->GetLocalTransformHelper(
            &builder,
            0.0f,       // offsetX - unused
            0.0f,       // offsetY - unused
            0.0f,       // dmOffsetX - unused
            0.0f,       // dmOffsetY - unused
            1.0f,       // dmZoomFactorX - unused
            1.0f,       // dmZoomFactorY - unused
            shouldFlipRTL,
            shouldFlipRTLInPlace,
            elementWidth,
            elementHeight,
            m_pUIElementNoRef->GetRenderTransformLocal(),
            m_pUIElementNoRef->GetRenderTransformOrigin(),
            m_pUIElementNoRef->GetTransitionTarget().get(),
            true,               // applyDMZoomToOffset - always true for Redstone+
            dmanipPS,           // pDManipTransform
            hasRedirectionTransformInfo ? &rto : nullptr,
            nullptr             // facadeInfo
            );

        builder.EnsureLocalExpression();

        m_previousWUCTransformMatrix.M11 = FloatUtil::NaN;  // No need to set the other components, we'll check for NaN in this one.
    }
    else
    {
        // Update the static TransformMatrix
        CMILMatrix4x4 overallTransform;
        CMILMatrix localTransform2D(true);

        XamlLocalTransformBuilder builder(&localTransform2D);

        // Note: We're passing in 0 for the offsets. They'll be applied in the visual's Offset instead.
        //
        // In the case of a redirection transform, this is technically incorrect. The redirection transform should apply
        // above the offset for this visual. The way we do this is to bake an inverse offset, the redirection, and the
        // offset again into the TransformMatrix. We handle this in the expression code path above.
        //
        // We can safely ignore offsets here because we've already checked that the redirection transform (if it exists)
        // contains only translations. In that case it doesn't matter whether it's applied above or below the offset, so
        // we don't do any trickery with undoing and redoing.
        ASSERT(!hasRedirectionTransformInfo || rto.redirectionTransform->IsTranslationOnly());

        m_pUIElementNoRef->GetLocalTransformHelper(
            &builder,
            0.0f,   // offsetX
            0.0f,   // offsetY
            0.0f,
            0.0f,
            1.0f,
            1.0f,
            shouldFlipRTL,
            shouldFlipRTLInPlace,
            elementWidth,
            elementHeight,
            m_pUIElementNoRef->GetRenderTransformLocal(),
            m_pUIElementNoRef->GetRenderTransformOrigin(),
            m_pUIElementNoRef->GetTransitionTarget().get(),
            false,
            nullptr,
            hasRedirectionTransformInfo ? &rto : nullptr,
            nullptr     // facadeInfo
            );

        overallTransform.SetTo2DTransform(&localTransform2D);

        if (m_pUIElementNoRef->GetRenderTransformLocal())
        {
            m_pUIElementNoRef->GetRenderTransformLocal()->ClearWUCExpression();
        }

        CTransform3D* transform3D = m_pUIElementNoRef->GetTransform3D();
        if (transform3D != nullptr)
        {
            transform3D->UpdateTransformMatrix(m_pUIElementNoRef->GetActualWidth(), m_pUIElementNoRef->GetActualHeight());
            overallTransform.Prepend(transform3D->GetTransformMatrix());

            transform3D->ClearWUCExpression();
        }

        if (m_pUIElementNoRef->GetProjection() != nullptr)
        {
            XSIZEF elementSize;
            IFCFAILFAST(m_pUIElementNoRef->GetElementSizeForProjection(&elementSize));
            CMILMatrix4x4 localProjection = m_pUIElementNoRef->GetProjection()->GetOverallProjectionMatrix(elementSize);
            overallTransform.Prepend(localProjection);

            m_pUIElementNoRef->GetProjection()->ClearWUCExpression();
        }

        wfn::Matrix4x4 finalTransform4x4;
        overallTransform.ToMatrix4x4(&finalTransform4x4);

        if (IsWUCTransformMatrixDifferentFromPrevious(finalTransform4x4))
        {
            IFCFAILFAST(m_primaryVisual->put_TransformMatrix(finalTransform4x4));
            m_previousWUCTransformMatrix = finalTransform4x4;
        }

        m_primaryVisualExpressionCache.Reset();
    }
}

void HWCompTreeNodeWinRT::UpdatePrimaryVisualTransformParent(_In_ DCompTreeHost *dcompTreeHost)
{
    // Only redirection CompNodes implement this virtual function
}

bool HWCompTreeNodeWinRT::GetRedirectionTransformInfo(_In_ RedirectionTransformInfo* rto)
{
    // Only redirection CompNodes implement this virtual function
    return false;
}

// Update the ViewportInteraction object, used for cross-process chaining of DManip
void HWCompTreeNodeWinRT::UpdatePrimaryVisualViewportInteraction(_In_ DCompTreeHost *dcompTreeHost)
{
    if (IsViewportInteractionRequired(dcompTreeHost))
    {
        if (!m_isViewportInteractionAssigned)
        {
            wrl::ComPtr<IInspectable> interaction;
            m_pUIElementNoRef->GetViewportInteraction(dcompTreeHost, &interaction);

            wrl::ComPtr<ixp::IExpVisual> expVisual;
            VERIFYHR(m_primaryVisual.As(&expVisual));
            IFCFAILFAST(expVisual->SetInteraction(interaction.Get()));

            m_isViewportInteractionAssigned = true;
            TraceSetViewportInteraction(m_pUIElementNoRef, interaction.Get());
        }
    }
    else if (m_isViewportInteractionAssigned)
    {
        wrl::ComPtr<ixp::IExpVisual> expVisual;
        VERIFYHR(m_primaryVisual.As(&expVisual));
        IFCFAILFAST(expVisual->SetInteraction(nullptr));

        m_isViewportInteractionAssigned = false;
        TraceSetViewportInteraction(m_pUIElementNoRef, nullptr);
    }
}

// Helper function to determine if this CompNode requires a ViewportInteraction
bool HWCompTreeNodeWinRT::IsViewportInteractionRequired(_In_ DCompTreeHost *dcompTreeHost) const
{
    return HasIndependentTransformManipulation() &&
            m_spDManipData->GetManipulationContentType() == XcpDMContentTypePrimary &&
            m_pUIElementNoRef->RequiresViewportInteraction(); // The root ScrollViewer only requires a viewport interaction object when it is touch-manipulatable.
}

// This comp node has a viewport interaction. Decide whether it needs DManip hit testing visual.
// The comp node needs a DManip hit testing visual iff its ancestor ScrollViewer has at least one piece
// of hit-test-visible content under it.
//
// Rather than walking the entire UIElement tree under the ScrollViewer, we'll walk the parent chain
// from this comp node's element up to the ScrollViewer, and walk the comp node tree below this comp
// node. Note that we do miss some UIElements this way. The ScrollViewer could have children that aren't
// on this comp node's element's parent chain, and we would fail to find them. We're okay with this,
// because in the default ScrollViewer template the only such elements are scroll bars.
void HWCompTreeNodeWinRT::UpdateDManipHitTestVisual(_In_ DCompTreeHost* dcompTreeHost)
{
    ASSERT(m_isViewportInteractionAssigned);

    bool needsDManipVisual = false;

    // Walk up from the target element to the nearest ScrollViewer (or up to the root). If we find
    // an element that has hit-test-visible visuals, we're done.
    for (CUIElement* element = m_pUIElementNoRef;
        !needsDManipVisual && element != nullptr && !element->OfTypeByIndex<KnownTypeIndex::ScrollViewer>();
        element = element->GetUIElementParentInternal(false /* publicParentOnly */))
    {
        needsDManipVisual = element->HasHitTestVisibleVisuals();
    }

    // If we haven't found any hit-test-visible visuals on the way up to the ScrollViewer, then look
    // for them in the content of this scrollable comp node. We walk the comp node tree because it's
    // much more compact than the UIElement tree.
    if (!needsDManipVisual)
    {
        needsDManipVisual = HasHitTestVisibleContent();
    }

    if (needsDManipVisual)
    {
        EnsureDManipHitTestVisual(dcompTreeHost);
    }
    else
    {
        CleanupDManipHitTestVisual();
    }
}

// Task #13325003: Create a special hit-test-only SpriteVisual when ScrollViewer is in use.
// There are scenarios where ScrollViewer.Content doesn't completely fill the DManip
// viewport (eg the element is narrow and center aligned).  In this case XAML's hit-testing
// mismatches DWM hit-testing due to the "gutter areas":
// For example:
//
//             -------------------
//             |                 |
//             |                 |   <= Content (scrolled vertically)
//             |                 |
//             |                 |
// -----------------------------------------------
// |           |                 |               |
// |           |                 |               |
// |           |                 |               |
// | gutter    |                 | gutter        |   <= Viewport rect
// |           |                 |               |
// |           |                 |               |
// -----------------------------------------------
//             |                 |
//             |                 |
//             |                 |
//             |                 |
//             -------------------
//
// XAML hit-tests input that lands in the gutter area to within ScrollViewer's viewport
// by virtue of a Grid in ScrollViewer's template which carries a Background.  So XAML is able
// to start manipulations when input hits in the gutter area.
// However this background produces a cousin visual of the visual for ScrollViewer.Content.
// DWM requires a visual to be the child of the visual with the ViewportInteraction attached to it for
// gesture targeting to understand the hit-test is "destined" for a DManip interaction.
// When manipulations start in the gutter area, the DWM hit-test hits a visual not under the
// ViewportInteraction visual and this mismatch causes manipulations to be canceled shortly after starting.
// The fix is to create a very large transparent SpriteVisual as the child of the manipulated visual,
// centered so that it's guaranteed to cover any gutters that might be present.
//
void HWCompTreeNodeWinRT::EnsureDManipHitTestVisual(_In_ DCompTreeHost *dcompTreeHost)
{
    static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
    if (m_dmanipHitTestVisual == nullptr
        && runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::InsertDManipHitTestVisual))
    {
        WUComp::ICompositor* compositor = dcompTreeHost->GetCompositor();
        wrl::ComPtr<WUComp::ISpriteVisual> spriteVisual;
        IFCFAILFAST(compositor->CreateSpriteVisual(&spriteVisual));
        VERIFYHR(spriteVisual.As(&m_dmanipHitTestVisual));

        if (runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::DumpMockDManipHitTestVisual))
        {
            SetDebugTag(m_dmanipHitTestVisual.Get(), s_dmanipHitTestVisualTag);
        }
        else
        {
            // To minimize the number of test masters needing updating, add a special tag to tell MockDComp
            // to omit this visual from the mock dump if present.
            SetDebugTag(m_dmanipHitTestVisual.Get(), s_dmanipHitTestVisualTagOmitFromDump);
        }

        // The guidance from Composition is to limit the size of all visuals to 2^21 to avoid clip clamping
        // and floating point accuracy issues.  We'll go up one power of 2, to 2^22 to ensure it covers
        // the gutters at the extreme edges.
        static const float largeDimension = 4194304.0f;

        wfn::Vector2 size;
        size.X = largeDimension;
        size.Y = largeDimension;
        IFCFAILFAST(m_dmanipHitTestVisual->put_Size(size));

        wfn::Vector3 offset;
        offset.X = -largeDimension/2;
        offset.Y = -largeDimension/2;
        offset.Z = 0.0f;
        IFCFAILFAST(m_dmanipHitTestVisual->put_Offset(offset));

        wrl::ComPtr<WUComp::ICompositionColorBrush> transparentBrush;
        IFCFAILFAST(compositor->CreateColorBrush(&transparentBrush));
        IFCFAILFAST(transparentBrush->put_Color(ColorUtils::GetWUColor(0x00000000)));
        wrl::ComPtr<WUComp::ICompositionBrush> brush;
        VERIFYHR(transparentBrush.As(&brush));
        IFCFAILFAST(spriteVisual->put_Brush(brush.Get()));

        // We want to insert the hit test visual at the bottom of this comp node's child collection. For that, we use
        // the bottommost non-content visual. The content visual contains the UIElement's children, and is updated during
        // incremental rendering as children come and go. If the DManip hit test visual was inserted as a child of the
        // content visual, we'll have to constantly update it to put it on the bottom. So instead, we use the bottommost
        // non-content visual, so that the DManip hit test visual will be a sibling of the content visual that sits
        // underneath it, and underneath all the CUIElement's children.
        xref_ptr<WUComp::IVisualCollection> childCollection;
        GetVisualCollectionFromWUCSpineVisual(GetBottomMostNonContentVisual(), childCollection.ReleaseAndGetAddressOf());
        IFCFAILFAST(childCollection->InsertAtBottom(m_dmanipHitTestVisual.Get()));
    }
}

void HWCompTreeNodeWinRT::CleanupDManipHitTestVisual()
{
    if (m_dmanipHitTestVisual != nullptr)
    {
        xref_ptr<WUComp::IVisualCollection> childCollection;
        GetVisualCollectionFromWUCSpineVisual(GetBottomMostNonContentVisual(), childCollection.ReleaseAndGetAddressOf());
        IFCFAILFAST(childCollection->Remove(m_dmanipHitTestVisual.Get()));
        m_dmanipHitTestVisual = nullptr;
    }
}

void HWCompTreeNodeWinRT::UpdatePrimaryVisualCompositeMode()
{
    WUComp::CompositionCompositeMode compositeMode = WUComp::CompositionCompositeMode::CompositionCompositeMode_Inherit;
    switch (m_pUIElementNoRef->GetCompositeMode())
    {
        case DirectUI::ElementCompositeMode::MinBlend:
            compositeMode = WUComp::CompositionCompositeMode::CompositionCompositeMode_MinBlend;
            break;

        case DirectUI::ElementCompositeMode::SourceOver:
            compositeMode = WUComp::CompositionCompositeMode::CompositionCompositeMode_SourceOver;
            break;

        case DirectUI::ElementCompositeMode::Inherit:
            compositeMode = WUComp::CompositionCompositeMode::CompositionCompositeMode_Inherit;
            break;

        case DirectUI::ElementCompositeMode::DestInvert:
            compositeMode = WUComp::CompositionCompositeMode::CompositionCompositeMode_DestinationInvert;
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    if (compositeMode != m_previousWUCCompositeMode)
    {
        IFCFAILFAST(m_primaryVisual->put_CompositeMode(compositeMode));
        m_previousWUCCompositeMode = compositeMode;
    }
}

void HWCompTreeNodeWinRT::UpdatePrimaryVisualPixelSnapping(bool disablePixelSnapping)
{
    // If this UIElement had a LayerVisual created as its hand off visual, then don't try to apply pixel snapping.
    // Pixel snapping requires an interop visual, which we can't QI to.
    wrl::ComPtr<WUComp::ILayerVisual> layerHandOffVisual;
    if (FAILED(m_primaryVisual.As(&layerHandOffVisual)))
    {
        // We turn on pixel snapping for all manipulatable CompNodes to keep content crisp
        // even when DManip computes sub-pixel offsets for the manipulation transform.

        bool hasIndependentTransformManipulation = HasIndependentTransformManipulation();

        const bool isPixelSnappingEnabled =
            hasIndependentTransformManipulation &&
            !disablePixelSnapping;

        wrl::ComPtr<ixp::IVisual4> visual4;
        VERIFYHR(m_primaryVisual.As(&visual4));
        IFCFAILFAST(visual4->put_IsPixelSnappingEnabled(isPixelSnappingEnabled));
    }
}

void HWCompTreeNodeWinRT::UpdatePrimaryVisualHitTestVisibility()
{
    // If this UIElement had a LayerVisual created as its hand off visual, then don't try to apply hit test visibility.
    // Hit test visibility requires an interop visual, which we can't QI to.
    wrl::ComPtr<WUComp::ILayerVisual> layerHandOffVisual;
    if (FAILED(m_primaryVisual.As(&layerHandOffVisual)))
    {
        wrl::ComPtr<WUComp::IVisual3> visual3;
        VERIFYHR(m_primaryVisual.As(&visual3));
        IFCFAILFAST(visual3->put_IsHitTestVisible(m_isHitTestVisible));
    }
}

void HWCompTreeNodeWinRT::UpdatePrimaryVisualLights(_In_ DCompTreeHost* dcompTreeHost)
{
    if (m_shouldUpdateLightsTargetingElement)
    {
        auto& targetMap = dcompTreeHost->GetXamlLightTargetMap();

        // Unregister from all lights targeting this element on the previous frame first. We'll re-register with those that
        // are still targeting this element.
        targetMap.RemoveTargetVisual(m_pUIElementNoRef, m_primaryVisual.Get());

        for (const auto& light : m_lightsTargetingElement)
        {
            targetMap.AddTargetVisual(m_pUIElementNoRef, m_primaryVisual.Get(), light);
        }
        m_lightsTargetingElement.clear();
        m_shouldUpdateLightsTargetingElement = false;
    }

    // Also set the CoordinateSpace of all lights that are attached to this element.
    if (m_shouldUpdateLightsAttachedToElement)
    {
        CUIElement* uiElementWithLightsNoRef = m_pUIElementNoRef;
        const bool liftLights = m_pUIElementNoRef->OfTypeByIndex<KnownTypeIndex::RootVisual>();

        if (liftLights)
        {
            // If there are lights set on the root scroll viewer, we assume that the app put them there with the intention of
            // applying them to the entire tree, and only placed them there because that's as high as the app can reach in the
            // public UIElement tree. So when attaching lights to the root visual, get them from the root scroll viewer. Note
            // that if Window.Content is a canvas, then there is no root scroll viewer, so we get them from the canvas instead.
            CRootVisual* rootVisualNoRef = static_cast<CRootVisual*>(m_pUIElementNoRef);
            uiElementWithLightsNoRef = rootVisualNoRef->GetRootScrollViewerOrCanvas();
        }

        if (uiElementWithLightsNoRef != nullptr)
        {
            const auto& xamlLights = uiElementWithLightsNoRef->GetXamlLightCollection();
            ASSERT(xamlLights != nullptr);

            for (const auto& xamlLightDO : *xamlLights)
            {
                CXamlLight* xamlLight = static_cast<CXamlLight*>(xamlLightDO);
                if (xamlLight->HasWUCLight())
                {
                    xamlLight->SetCoordinateSpace(m_primaryVisual.Get());
                }
            }
        }
        else
        {
            // Bug 14127907
            // It's possible that we prepared to lift lights from the root scroll viewer to the root visual, then reset the
            // root scroll viewer before we got around to attaching the lights. In that case the element with lights will be
            // null because we failed to find the root scroll viewer here. No-op in that case. In all other cases we should
            // have an element with lights on it. Also clear the "should update lights" flag. When the root scroll viewer
            // enters the tree again and renders, we'll set the flag again.
            ASSERT(liftLights);
        }

        m_shouldUpdateLightsAttachedToElement = false;
    }
}

// Returns true if we determine the "Components" visual is required
bool HWCompTreeNodeWinRT::RequiresComponentsVisual()
{
    if (m_pUIElementNoRef->IsStrictOnly())
    {
        // The following checks determine if the TransformMatrix is in use by a property we need to
        // support in combination with facades.  This set is limited to:
        // DManip
        // RTL
        // Redirection
        // Transition animations
        // Note that we don't need to worry about RenderTransform or its cousins as we won't allow
        // these properties to be used when strict mode is turned on.

        if (HasIndependentTransformManipulation())
        {
            return true;
        }

        bool shouldFlipRTL, shouldFlipRTLInPlace;
        m_pUIElementNoRef->GetShouldFlipRTL(&shouldFlipRTL, &shouldFlipRTLInPlace);
        if (shouldFlipRTL)
        {
            return true;
        }

        RedirectionTransformInfo rto;
        if (GetRedirectionTransformInfo(&rto))
        {
            return true;
        }

        if (m_pUIElementNoRef->GetTransitionTarget() != nullptr)
        {
            return true;
        }
    }

    return false;
}

bool HWCompTreeNodeWinRT::HasComponentsVisual() const
{
    return m_bottomClipVisuals[BOTTOMCLIP_Components] != nullptr;
}

WUComp::IVisual* HWCompTreeNodeWinRT::GetComponentsVisual() const
{
    return m_bottomClipVisuals[BOTTOMCLIP_Components].Get();
}

// Determine if the "Components" visual is needed or not, creating/initializing if needed, destroying if not needed.
void HWCompTreeNodeWinRT::UpdateComponentsVisualConfiguration(_In_ DCompTreeHost *dcompTreeHost)
{
    bool requiresComponentsVisual = RequiresComponentsVisual();

    if (requiresComponentsVisual)
    {
        if (!HasComponentsVisual())
        {
            EnsureBottomClipVisual(dcompTreeHost, BOTTOMCLIP_Components, s_componentsVisualTag);

            // Clear out properties that might have been previously set on Primary visual, these are moving to Components Visual
            m_primaryVisual->put_Clip(nullptr);
            m_primaryVisual->put_RotationAngleInDegrees(static_cast<float>(SimpleProperty::Property::Default<KnownPropertyIndex::UIElement_Rotation>()));
            m_primaryVisual->put_Scale(SimpleProperty::Property::Default<KnownPropertyIndex::UIElement_Scale>());
            m_primaryVisual->put_CenterPoint(SimpleProperty::Property::Default<KnownPropertyIndex::UIElement_CenterPoint>());
            m_primaryVisual->put_RotationAxis(SimpleProperty::Property::Default<KnownPropertyIndex::UIElement_RotationAxis>());

            // Facades_TODO:  This can probably be removed
            m_previousWUCTransformMatrix.M11 = FloatUtil::NaN;
        }
        wfn::Vector2 size;
        size.X = m_pUIElementNoRef->GetActualWidth();
        size.Y = m_pUIElementNoRef->GetActualHeight();
        IFCFAILFAST(GetComponentsVisual()->put_Size(size));

        // Facades_TODO - what about pixel snapping?
    }
    else
    {
        if (HasComponentsVisual())
        {
            CleanupBottomClipVisual(BOTTOMCLIP_Components);
        }
    }
}

// Create the "Glue" ExpressionAnimation, for each facade, this ties the appropriate property in backing PropertySet to the Visual
wrl::ComPtr<WUComp::IExpressionAnimation> HWCompTreeNodeWinRT::EnsureFacadeGlueExpression(
    _In_ WUComp::ICompositor* compositor,
    KnownPropertyIndex facadeID,
    _In_ const wchar_t* expressionString)
{
    FacadeStorage& facadeStorage = GetContext()->GetFacadeStorage();

    // Retrieve the expression from cache, or create and store in the cache.
    wrl::ComPtr<WUComp::IExpressionAnimation> expressionAnimation = facadeStorage.GetFacadeGlueExpression(facadeID);
    if (expressionAnimation == nullptr)
    {
        IFCFAILFAST(compositor->CreateExpressionAnimationWithExpression(wrl::Wrappers::HStringReference(expressionString).Get(), &expressionAnimation));
        facadeStorage.SetFacadeGlueExpression(facadeID, expressionAnimation.Get());
    }

    return expressionAnimation;
}

// Helper function to start the "Glue" ExpressionAnimation
void HWCompTreeNodeWinRT::StartFacadeGlueExpression(
    _In_ WUComp::IVisual* visual,
    _In_ const wchar_t* propertyName,
    _In_ WUComp::IExpressionAnimation* expressionAnimation)
{
    FacadeStorage& facadeStorage = GetContext()->GetFacadeStorage();

    // Set the backing PropertySet as a reference parameter
    wrl::ComPtr<WUComp::ICompositionAnimation> ca;
    VERIFYHR(expressionAnimation->QueryInterface(IID_PPV_ARGS(&ca)));
    IFCFAILFAST(ca->SetReferenceParameter(wrl::Wrappers::HStringReference(s_PS).Get(), facadeStorage.GetBackingCompositionObject(m_pUIElementNoRef).Get()));

    // Start the animation on the appropriate property name
    wrl::ComPtr<WUComp::ICompositionObject> visualCO;
    VERIFYHR(visual->QueryInterface(IID_PPV_ARGS(&visualCO)));
    IFCFAILFAST(visualCO->StartAnimation(wrl::Wrappers::HStringReference(propertyName).Get(), ca.Get()));
}

void HWCompTreeNodeWinRT::UpdateRotationFacade(_In_ DCompTreeHost *dcompTreeHost, _In_ WUComp::IVisual* visual)
{
    FacadeStorage& facadeStorage = GetContext()->GetFacadeStorage();

    if (m_pUIElementNoRef->HasFacadeAnimation() && facadeStorage.HasAnimation(m_pUIElementNoRef, KnownPropertyIndex::UIElement_Rotation))
    {
        wrl::ComPtr<WUComp::IExpressionAnimation> rotationExpression = EnsureFacadeGlueExpression(dcompTreeHost->GetCompositor(), KnownPropertyIndex::UIElement_Rotation, s_RotationFacadeExpressionString);
        StartFacadeGlueExpression(visual, ExpressionHelper::sc_RotationPropertyName, rotationExpression.Get());
    }
    else
    {
        IFCFAILFAST(visual->put_RotationAngleInDegrees(m_pUIElementNoRef->GetRotation()));
    }
}

void HWCompTreeNodeWinRT::UpdateScaleFacade(_In_ DCompTreeHost *dcompTreeHost, _In_ WUComp::IVisual* visual)
{
    FacadeStorage& facadeStorage = GetContext()->GetFacadeStorage();

    if (m_pUIElementNoRef->HasFacadeAnimation() && facadeStorage.HasAnimation(m_pUIElementNoRef, KnownPropertyIndex::UIElement_Scale))
    {
        wrl::ComPtr<WUComp::IExpressionAnimation> scaleExpression = EnsureFacadeGlueExpression(dcompTreeHost->GetCompositor(), KnownPropertyIndex::UIElement_Scale, s_ScaleFacadeExpressionString);
        StartFacadeGlueExpression(visual, ExpressionHelper::sc_ScalePropertyName, scaleExpression.Get());
    }
    else
    {
        IFCFAILFAST(visual->put_Scale(m_pUIElementNoRef->GetScale()));
    }
}

void HWCompTreeNodeWinRT::UpdateTransformMatrixFacade(_In_ DCompTreeHost *dcompTreeHost, _In_ WUComp::IVisual* visual)
{
    FacadeStorage& facadeStorage = GetContext()->GetFacadeStorage();

    if (m_pUIElementNoRef->HasFacadeAnimation() && facadeStorage.HasAnimation(m_pUIElementNoRef, KnownPropertyIndex::UIElement_TransformMatrix))
    {
        wrl::ComPtr<WUComp::IExpressionAnimation> transformMatrixExpression = EnsureFacadeGlueExpression(dcompTreeHost->GetCompositor(), KnownPropertyIndex::UIElement_TransformMatrix, s_TransformMatrixFacadeExpressionString);
        StartFacadeGlueExpression(visual, ExpressionHelper::sc_TransformMatrixPropertyName, transformMatrixExpression.Get());
    }
    else
    {
        IFCFAILFAST(visual->put_TransformMatrix(m_pUIElementNoRef->GetTransformMatrix()));
    }
}

void HWCompTreeNodeWinRT::UpdateCenterPointFacade(_In_ DCompTreeHost *dcompTreeHost, _In_ WUComp::IVisual* visual)
{
    FacadeStorage& facadeStorage = GetContext()->GetFacadeStorage();

    if (m_pUIElementNoRef->HasFacadeAnimation() && facadeStorage.HasAnimation(m_pUIElementNoRef,KnownPropertyIndex::UIElement_CenterPoint))
    {
        wrl::ComPtr<WUComp::IExpressionAnimation> centerPointExpression = EnsureFacadeGlueExpression(dcompTreeHost->GetCompositor(), KnownPropertyIndex::UIElement_CenterPoint, s_CenterPointFacadeExpressionString);
        StartFacadeGlueExpression(visual, ExpressionHelper::sc_CenterPointPropertyName, centerPointExpression.Get());
    }
    else
    {
        IFCFAILFAST(visual->put_CenterPoint(m_pUIElementNoRef->GetCenterPoint()));
    }
}

void HWCompTreeNodeWinRT::UpdateRotationAxisFacade(_In_ DCompTreeHost *dcompTreeHost, _In_ WUComp::IVisual* visual)
{
    FacadeStorage& facadeStorage = GetContext()->GetFacadeStorage();

    if (m_pUIElementNoRef->HasFacadeAnimation() && facadeStorage.HasAnimation(m_pUIElementNoRef, KnownPropertyIndex::UIElement_RotationAxis))
    {
        wrl::ComPtr<WUComp::IExpressionAnimation> rotationAxisExpression = EnsureFacadeGlueExpression(dcompTreeHost->GetCompositor(), KnownPropertyIndex::UIElement_RotationAxis, s_RotationAxisFacadeExpressionString);
        StartFacadeGlueExpression(visual, ExpressionHelper::sc_RotationAxisPropertyName, rotationAxisExpression.Get());
    }
    else
    {
        IFCFAILFAST(visual->put_RotationAxis(m_pUIElementNoRef->GetRotationAxis()));
    }
}

bool HWCompTreeNodeWinRT::HasTransitionClipAnimation() const
{
    return
        m_pUIElementNoRef->IsTransitionClipIndependentlyAnimating() ||
        (m_pUIElementNoRef->HasTransitionTarget() && m_pUIElementNoRef->GetTransitionTarget()->HasClipAnimation());
}

void HWCompTreeNodeWinRT::UpdateTransitionClipVisual(
    _In_ DCompTreeHost *dcompTreeHost
    )
{
    // There is a subtlety to whether or not a transition clip is required:
    // -Normally, during the independent animation, the UIElement knows it is mid-animation
    // -Even while not doing an independent animation, the TransitionTarget can continue to apply a static clip
    // -Additionally, we'll use this visual to apply a LayoutClip if it's supposed to be applied as a "self" clip
    const bool mustApplyLayoutClip = m_pUIElementNoRef->HasLayoutClip() && !m_pUIElementNoRef->ShouldApplyLayoutClipAsAncestorClip();
    bool isTransitionClipVisualRequired = HasTransitionClipAnimation() || mustApplyLayoutClip;

    if (isTransitionClipVisualRequired)
    {
        WUComp::IVisual* transitionClipVisual = EnsureBottomClipVisual(dcompTreeHost, BOTTOMCLIP_TransitionTarget, s_transitionClipVisualTag);

        // Update the Size as InsetClip is dependent on Size.
        wfn::Vector2 size;
        size.X = m_pUIElementNoRef->GetActualWidth();
        size.Y = m_pUIElementNoRef->GetActualHeight();
        IFCFAILFAST(transitionClipVisual->put_Size(size));

        wrl::ComPtr<WUComp::ICompositionClip> clip;
        wrl::ComPtr<WUComp::IInsetClip> insetClip;
        IFCFAILFAST(transitionClipVisual->get_Clip(&clip));

        // Update the clip
        // TODO_WinRT: We always reset the clip + clip transform here. Consider having a dirty flag for these updates.
        if (clip == nullptr)
        {
            IFCFAILFAST(dcompTreeHost->GetCompositor()->CreateInsetClip(insetClip.ReleaseAndGetAddressOf()));
            IFCFAILFAST(insetClip.As(&clip));
            IFCFAILFAST(transitionClipVisual->put_Clip(clip.Get()));
        }
        else
        {
            IFCFAILFAST(clip.As(&insetClip));
        }

        wrl::ComPtr<WUComp::ICompositionClip2> clipCC2;
        wrl::ComPtr<WUComp::ICompositionObject> clipCO;
        IFCFAILFAST(clip.As(&clipCC2));
        IFCFAILFAST(clip.As(&clipCO));

        XRECTF overallClip = { 0, 0, size.X, size.Y };
        if (mustApplyLayoutClip && !m_isInConnectedAnimation)
        {
            ASSERT(m_pUIElementNoRef->LayoutClipGeometry->OfTypeByIndex<KnownTypeIndex::RectangleGeometry>());
            IntersectRect(&overallClip, &m_pUIElementNoRef->LayoutClipGeometry->m_rc);
        }
        UpdateInsetClip(overallClip, insetClip.Get(), PreviousClipComparison::AlwaysSet);

        // Update the clip transform
        if (m_pUIElementNoRef->HasTransitionTarget() && !m_isInConnectedAnimation)
        {
            wrl::ComPtr<WUComp::ICompositionAnimation> transitionClipTransformCA;
            WinRTExpressionConversionContext context(dcompTreeHost->GetCompositor());

            auto transitionTarget = m_pUIElementNoRef->GetTransitionTarget();
            transitionTarget->m_pClipTransform->MakeWinRTExpressionWithOrigin(
                overallClip.Width * transitionTarget->m_ptClipTransformOrigin.x,
                overallClip.Height * transitionTarget->m_ptClipTransformOrigin.y,
                &context,
                &m_transitionClipTransformExpressionCache);
            IFCFAILFAST(m_transitionClipTransformExpressionCache.As(&transitionClipTransformCA));

            IFCFAILFAST(clipCO->StartAnimation(
                wrl_wrappers::HStringReference(ExpressionHelper::sc_propertyName_TransformMatrix).Get(),
                transitionClipTransformCA.Get()));
        }
        else
        {
            m_transitionClipTransformExpressionCache.Reset();
        }
    }
    else
    {
        CleanupBottomClipVisual(BOTTOMCLIP_TransitionTarget);
        m_transitionClipTransformExpressionCache.Reset();
    }
}

// Helper function to clamp corner radii according to same policy as CGeometryBuilder::CalculateRoundedCornersRectangle()
static void ClampRadii(float r1, float r2, float edgeLength, _Out_ float* r1Clamped, _Out_ float* r2Clamped)
{
    float total = r1 + r2;
    if (total > 0 && total > edgeLength)
    {
        // If the total of both radii exceed the edge length, distribute the radii according to the percentages taken.
        *r1Clamped = (r1 / total) * edgeLength;
        *r2Clamped = (r2 / total) * edgeLength;
    }
    else
    {
        *r1Clamped = r1;
        *r2Clamped = r2;
    }
}

void HWCompTreeNodeWinRT::UpdateRoundedCornerClipVisual(_In_ DCompTreeHost* dcompTreeHost)
{
    CUIElement* targetElement = GetElementForLTEProperties();
    if (targetElement->RequiresCompNodeForRoundedCorners())
    {
        ixp::IVisual* roundedCornerClipVisual = EnsureBottomClipVisual(dcompTreeHost, BOTTOMCLIP_RoundedCorners, s_roundedCornerClipVisualTag);
        DCompTreeHost::SetTagIfEnabled(roundedCornerClipVisual, VisualDebugTags::CompNode_RoundedCornerClipVisual);

        wfn::Vector2 size;
        size.X = m_pUIElementNoRef->GetActualWidth();
        size.Y = m_pUIElementNoRef->GetActualHeight();

        // Get a RectangleClip for this visual, creating if necessary.
        wrl::ComPtr<ixp::ICompositionClip> clip;
        wrl::ComPtr<ixp::IRectangleClip> rectangleClip;
        IFCFAILFAST(roundedCornerClipVisual->get_Clip(&clip));

        if (clip == nullptr)
        {
            wrl::ComPtr<ixp::ICompositor7> compositor7;
            VERIFYHR(dcompTreeHost->GetCompositor()->QueryInterface(IID_PPV_ARGS(&compositor7)));
            IFCFAILFAST(compositor7->CreateRectangleClip(rectangleClip.ReleaseAndGetAddressOf()));
            IFCFAILFAST(rectangleClip.As(&clip));
            IFCFAILFAST(roundedCornerClipVisual->put_Clip(clip.Get()));
        }
        else
        {
            VERIFYHR(clip.As(&rectangleClip));
        }

        // Propagate the rounded corner clip information into the RectangleClip.
        // Note carefully that RectangleClip is not an InsetClip, its size is "absolute".
        IFCFAILFAST(rectangleClip->put_Left(0));
        IFCFAILFAST(rectangleClip->put_Top(0));
        IFCFAILFAST(rectangleClip->put_Right(size.X));
        IFCFAILFAST(rectangleClip->put_Bottom(size.Y));

        ASSERT(targetElement->OfTypeByIndex<KnownTypeIndex::FrameworkElement>());
        CFrameworkElement* frameworkElement = static_cast<CFrameworkElement*>(targetElement);
        XCORNERRADIUS cornerRadius = frameworkElement->GetCornerRadius();
        wfn::Vector2 radiusTopLeft;
        wfn::Vector2 radiusTopRight;
        wfn::Vector2 radiusBottomLeft;
        wfn::Vector2 radiusBottomRight;

        // DComp does not enforce any clamping of radii, unlike XAML, which would lead to incorrect clipping in cases
        // where the radii exceed edge length.  Clamp the radii down ourselves before setting on the visual.
        ClampRadii(cornerRadius.topLeft, cornerRadius.topRight, size.X, &radiusTopLeft.X, &radiusTopRight.X);
        ClampRadii(cornerRadius.bottomLeft, cornerRadius.bottomRight, size.X, &radiusBottomLeft.X, &radiusBottomRight.X);
        ClampRadii(cornerRadius.topLeft, cornerRadius.bottomLeft, size.Y, &radiusTopLeft.Y, &radiusBottomLeft.Y);
        ClampRadii(cornerRadius.topRight, cornerRadius.bottomRight, size.Y, &radiusTopRight.Y, &radiusBottomRight.Y);

        IFCFAILFAST(rectangleClip->put_TopLeftRadius(radiusTopLeft));
        IFCFAILFAST(rectangleClip->put_TopRightRadius(radiusTopRight));
        IFCFAILFAST(rectangleClip->put_BottomLeftRadius(radiusBottomLeft));
        IFCFAILFAST(rectangleClip->put_BottomRightRadius(radiusBottomRight));
    }
    else
    {
        CleanupBottomClipVisual(BOTTOMCLIP_RoundedCorners);
    }
}

// The app can call can Dispose (C#: IDisposable::Dispose or using / C++/CX : delete / WRL C++ : IClosable::Close)
// on the hand-in visual while we are still managing it in our UIElement and associated compnode. From this point on, any operations
// on this visual will fail. For Xaml, these are limited to tree operations when the visual is being added / removed / replaced,
// or when the compnode gets destroyed and we are cleaning it up. We need to be resilient to RO_E_CLOSED in these cases.
// (Note that app cannot Dispose of Handoff visuals or InteropCompositor - we throw in those cases).
//
// There are two possible approaches to fixing this:
//     (1)Ignore RO_E_CLOSED and treat the disposed visual as any other, or
//     (2)Handle RO_E_CLOSED by releasing Xaml's references to the visual and effectively removing it.
//
// We tried approach (2) first, as it at first appears cleaner in that it would release all resources (not just DComp) as soon as a
// disposed hand-in is detected. But we hit some complexity, mostly owing to the fact that hand-ins (and handoffs) are managed jointly
// by 2 entities: UIElement(which stores them and hands out to compnodes as needed) and CompNode (which actually inserts it in the
// right place in the DComp tree). The transfer logic between these is also somewhat different  in WinRT and legacy DComp case,
// and we would have to get both right to handle a spontaneous release of the hand-in.In more detail:
//      (i) RO_E_CLOSED is detected in compnode's code, typically in destructor/cleanup. Releasing the visual here would end up
//          calling back into the compnode's cleanup via the UIElement.Some of this happens via deferred compositor command, but some
//          cleanup happens immediately (in HWCompNodeDComp::DiscardHandInVisual, we will update state HWCompnodeDComp::m_spHandInVisual /
//          m_ishandInVisualValid inconsistently with the operation already in flight).
//     (ii) HWCompNodeDComp has a more complex method of transferring hand-ins from UIElement to Compnode, using temporary  m_spHandInVisual
//          and final m_spParentedHandInVisual, requiring extra care to manage if we are resetting the visual on the UIElement.
//    (iii) CompNode lifetime may be changing if we release the visual spontaneously when detecting RO_E_CLOSED, since having
//          a hand-in visual could be the only reason why the UIE needed a comp node; this makes the risk / scope of change greater.
//
// Upon further analysis, holding on to the disposed visual may actually be the cleaner approach. We do not have a notification from
// DComp that a visual has been disposed, so we don't find out about it until we try a tree operation on it. Thus having closed visuals
// managed by UIElement/compnode is a state that we need to support. It's better, then, to extend that state in some cases, rather than
// also introduce a new one associated with spontaneous release. Also, typically we will detect RO_E_CLOSED at either comp node destruction,
// or when replacing a disposed visual. In both cases, we will be releasing the visual shortly regardless, and immediate release would buy
// little here. The other case is for explicitly setting a disposed visual on a UIElement, which is an unnatural scenario,and the app itself
// will typically know not to do that.
//
// Thus this fix takes approach(1) and manages disposed hand-in visuals until they are naturally released. Since the visuals are disconnected
// from the tree by the disposal, they cannot mess up rendering.Since in both Legacy and WinRT, they are attached to a specific
// placeholder location, and cannot have children added by Xaml, it should be safe to disconnect them spontaneously without needing
// further fix up for the tree. Thus, here and in HWCompNodeDComp we implement approach(1) as the simplest, least bug-prone solution,
// which should effectively address the specific scenarios where we hit this.
void HWCompTreeNodeWinRT::UpdateHandInVisual()
{
    HRESULT hr = S_OK;

    // Update by comparing our current HandIn Visual state to what's stored on the CUIElement
    xref_ptr<WUComp::IVisual> currentHandInVisual;
    if (m_pUIElementNoRef->IsUsingHandInVisual())
    {
        IFCFAILFAST(m_pUIElementNoRef->GetHandInVisual(currentHandInVisual.ReleaseAndGetAddressOf()));
    }
    if (m_handInVisual.Get() != currentHandInVisual)
    {
        // OK we know the state has changed.
        // There are 3 cases:
        // 1) We don't have a HandIn visual yet.  We'll insert it.
        // 2) The HandIn visual was discarded.  We'll remove it.
        // 3) The HandIn visual was discarded, then re-created.  We'll remove then insert.
        xref_ptr<WUComp::IVisualCollection> childCollection;
        GetVisualCollectionFromWUCSpineVisual(GetBottomMostNonContentVisual(), childCollection.ReleaseAndGetAddressOf());

        if (m_handInVisual != nullptr)
        {
            hr = childCollection->Remove(m_handInVisual.Get());

            // It's possible the current hand-in has been disposed by the app, and we're finding out now when the app is replacing it with a new one.
            // For simplicity, ignore RO_E_CLOSED and allow UIElement + compnode to manage it as a valid hand-in. The overhead here is minimal since disposal released the DComp resources.
            // In this case, the replace operation will completely clean up the old hand-in.
            if (hr == RO_E_CLOSED)
            {
                TRACE(TraceAlways, L"HWCompTreeNodeWinRT::UpdateHandInVisual: Got RO_E_CLOSED removing existing hand-in. m_handInVisual: 0x%08p, this: 0x%08p", m_handInVisual.Get(), this);
                hr = S_OK;
            }

            // LTEs can render one UIElement in multiple places, which causes us to insert the same WUC hand in visual in multiple
            // places in the tree. The subsequent inserts will no-op. When we go to remove those LTEs, we'll try to remove the same
            // hand in visual from multiple places in the tree when it's actually in only one place. Ignore the E_INVALIDARGs that
            // we get from DComp.
            IFCFAILFAST_ALLOW_INVALIDARG(hr);
        }
        if (currentHandInVisual != nullptr)
        {
            // Note: This will put the hand in visual on top of all the children as well.
            hr = childCollection->InsertAtTop(currentHandInVisual);

            // It's possible the new hand-in has been disposed by the app, and we're finding out now when the app is using it to replace the previous one.
            // For simplicity, ignore RO_E_CLOSED and allow UIElement + compnode to manage it as a valid hand-in. The overhead here is minimal since disposal released the DComp resources.
            // In this case, the closed hand-in will persist until it released by app and the associated UIElement is destroyed.
            if (hr == RO_E_CLOSED)
            {
                TRACE(TraceAlways, L"HWCompTreeNodeWinRT::UpdateHandInVisual: Got RO_E_CLOSED inserting new handin. currentHandInVisual: 0x%08p, this: 0x%08p", currentHandInVisual.get(), this);
                hr = S_OK;
            }

            // LTEs can render one UIElement in multiple places, which causes us to insert the same WUC hand in visual in multiple
            // places in the tree. DComp will return E_INVALIDARG in this case, which we'll ignore.
            IFCFAILFAST_ALLOW_INVALIDARG(hr);
        }
        m_handInVisual = currentHandInVisual;
    }
}

void HWCompTreeNodeWinRT::CleanupHandInVisual()
{
    HRESULT hr = S_OK;

    // The Hand-In visual must be explicitly unparented as it may migrate to another CompNode
    if (m_handInVisual != nullptr)
    {
        xref_ptr<WUComp::IVisualCollection> childCollection;
        GetVisualCollectionFromWUCSpineVisual(GetBottomMostNonContentVisual(), childCollection.ReleaseAndGetAddressOf());

        hr = childCollection->Remove(m_handInVisual.Get());

        // It's possible the hand-in has been disposed by the app, and we're finding out now when dissociating it from the comp node.
        // For simplicity, ignore RO_E_CLOSED and allow UIElement to manage it as a valid hand-in. The overhead here is minimal since disposal released the DComp resources.
        // In this case, the closed hand-in will persist until it is released by app and the associated UIElement is destroyed (it could still migrate to another compnode).
        // For more info on the design, see comment on HWCompTreeNodeWinRT::UpdateHandInVisual.
        //
        // For more info: see comment on HWCompTreeNodeWinRT::UpdateHandInVisual.
        if (hr == RO_E_CLOSED)
        {
            TRACE(TraceAlways, L"HWCompTreeNodeWinRT::CleanupHandInVisual: Got RO_E_CLOSED removing handin; m_handInVisual: 0x%08p, this: 0x%08p", m_handInVisual.Get(), this);
            hr = S_OK;
        }

        // LTEs can render one UIElement in multiple places, which causes us to insert the same WUC hand in visual in multiple
        // places in the tree. The subsequent inserts will no-op. When we go to remove those LTEs, we'll try to remove the same
        // hand in visual from multiple places in the tree when it's actually in only one place. Ignore the E_INVALIDARGs that
        // we get from DComp.
        IFCFAILFAST_ALLOW_INVALIDARG(hr);
    }
}

void HWCompTreeNodeWinRT::CleanupPrependVisualListener()
{
    auto uiElement = m_pUIElementPeerWeakRef.lock();
    if (uiElement)
    {
        uiElement->DetachListenerFromPrependVisual();
    }
}

void HWCompTreeNodeWinRT::CleanupHandOffVisual()
{
    auto pUIElementPeer = m_pUIElementPeerWeakRef.lock();

    // The Hand-Off visual must be unparented as it may migrate to another CompNode
    xref_ptr<WUComp::IVisualCollection> childCollection;
    GetVisualCollectionFromWUCSpineVisual(m_prependVisual.Get(), childCollection.ReleaseAndGetAddressOf());
    // During shutdown we might find no child collection due to the CompositionContent already closing down the entire tree.
    if (childCollection != nullptr)
    {
        // LTEs can render one UIElement in multiple places, which causes us to insert the same WUC hand off visual in multiple
        // places in the tree. The subsequent inserts will no-op. When we go to remove those LTEs, we'll try to remove the same
        // hand off visual from multiple places in the tree when it's actually in only one place. Ignore the E_INVALIDARGs that
        // we get from DComp.
        IFCFAILFAST_ALLOW_INVALIDARG(childCollection->Remove(m_primaryVisual.Get()));
    }

    if (pUIElementPeer && IsUsingHandOffVisual(*pUIElementPeer))
    {
        // If the UIElement gets recycled, and the HandOff visual is in use, care must be taken to cache/restore the
        // property values we're tracking for the purposes of knowing when XAML can and cannot stomp over
        // a visual property.
        // As we're about to destroy this CompNode, save a cache of these values, if a CompNode is re-created
        // for the same UIElement and the HandOff visual is still in use we will restore this cache in EnsureVisual().
        auto dCompTreeHost = pUIElementPeer->GetDCompTreeHost();
        if (dCompTreeHost)
        {
            auto& map = dCompTreeHost->GetHandOffVisualDataMap();
            auto handOffPropertyIter = map.find(m_pUIElementNoRef);
            ASSERT(handOffPropertyIter != map.end());
            HandOffVisualData& cache = handOffPropertyIter->second;
            cache.cachedPropertiesInUse = true;
            cache.previousWUCOpacity = m_previousWUCOpacity;
            cache.previousWUCOffset = m_previousWUCOffset;
            cache.previousWUCCompositeMode = m_previousWUCCompositeMode;
            cache.previousWUCTransformMatrix = m_previousWUCTransformMatrix;
            cache.previousWUCInsetClip = m_previousWUCInsetClip;
            cache.previousWUCClipTransformMatrix = m_previousWUCClipTransformMatrix;
            cache.previousHasWUCInsetClip = m_previousHasWUCInsetClip;
        }
    }
}

void HWCompTreeNodeWinRT::CleanupChildLinks()
{
    xref_ptr<WUComp::IContainerVisual> containerVisual;
    VERIFYHR(GetBottomMostVisual()->QueryInterface(IID_PPV_ARGS(containerVisual.ReleaseAndGetAddressOf())));

    // For CoreWindow scenarios, the CompositionContent is also listening for the CoreWindow's closed event.
    // CompositionContent will get the notification first and close the entire visual tree, then Xaml will
    // exit its message loop and tear down the tree. Since CompositionContent already closed everything,
    // Xaml will get lots of RO_E_CLOSED errors. These are all safe to ignore. So tolerate RO_E_CLOSED if
    // we're also in the middle of tearing down the tree.
    xref_ptr<WUComp::IVisualCollection> childCollection;
    HRESULT hr = containerVisual->get_Children(childCollection.ReleaseAndGetAddressOf());
    if (SUCCEEDED(hr))
    {
        IFCFAILFAST(childCollection->RemoveAll());
    }
    else if (hr != RO_E_CLOSED || !GetContext()->IsTearingDownTree())
    {
        IFCFAILFAST(hr);
    }
}

_Check_return_ HRESULT HWCompTreeNodeWinRT::InsertChildInternal(
    _In_opt_ DCompTreeHost *dcompTreeHost,
    _In_ HWCompNode *child,
    _In_opt_ HWCompNode *referenceNode,
    _In_opt_ CUIElement *element
    )
{
    InsertChildSynchronousInternal(dcompTreeHost, child, referenceNode, nullptr /*previousSiblingVisual */, element, false /* ignoreInsertVisualErrors */);
    return S_OK;
}

void HWCompTreeNodeWinRT::InsertChildSynchronous(
    _In_opt_ DCompTreeHost *dcompTreeHost,
    _In_ HWCompNode* child,
    _In_opt_ HWCompNode* referenceNode,
    _In_opt_ WUComp::IVisual* previousSiblingVisual,
    _In_opt_ CUIElement *element,
    const bool ignoreInsertVisualErrors)
{
    ASSERT(child != this);

    // Update the compositor tree.
    XUINT32 index = 0;
    if (referenceNode != NULL)
    {
        for (; index < m_children.size(); index++)
        {
            HWCompNode *pNodeNoRef = m_children.unsafe_get_item(index);
            if (pNodeNoRef == referenceNode)
            {
                // Insert after the reference node.
                index += 1;
                break;
            }
        }
    }

    IFCFAILFAST(m_children.insert(index, child));
    AddRefInterface(child);

    IFCFAILFAST(child->AddParent(
        this,
        FALSE /* fPublic */,
        RENDERCHANGEDPFN(CDependencyObject::NWSetRenderDirty)
        ));

    InsertChildSynchronousInternal(dcompTreeHost, child, referenceNode, previousSiblingVisual, element, ignoreInsertVisualErrors);
}

void HWCompTreeNodeWinRT::InsertChildSynchronousInternal(
    _In_opt_ DCompTreeHost *dcompTreeHost,
    _In_ HWCompNode *child,
    _In_opt_ HWCompNode *referenceNode,
    _In_opt_ WUComp::IVisual* previousSiblingVisual,
    _In_opt_ CUIElement *element,
    const bool ignoreInsertVisualErrors)
{
    // Update the DComp tree.
    IFCFAILFAST(child->EnsureVisual(dcompTreeHost));

    // If the reference node exists, it should already be a child, so it should already have a visual.
    ASSERT(referenceNode == nullptr || referenceNode->GetWUCVisual() != nullptr);

    EnsureVisual(dcompTreeHost);

    wrl::ComPtr<WUComp::IVisual> wucChild;

    // Figure out which visual from the child to actually insert.
    // Typically this is the "Visual" (ie Prepend Visual), but for CompNodes that use placeholder visuals, it's the placeholder visual.
    if (child->UsesPlaceholderVisual())
    {
        // Currently only popups use a placeholder visual. All windowed popups use a placeholder visual in the main
        // tree while the content lives in a separate tree under the windowed popup island. Non-windowed popups nested
        // inside windowed popups also use placeholder visuals in the main tree while the content lives in the same
        // island as the windowed popup that they're nested under.
        ASSERT(child->OfTypeByIndex<KnownTypeIndex::HWRedirectedCompTreeNodeWinRT>());

        HWRedirectedCompTreeNodeWinRT* childPopupNode = static_cast<HWRedirectedCompTreeNodeWinRT*>(child);
        wucChild = childPopupNode->EnsurePlaceholderVisual(dcompTreeHost);

        // An HWCompNode which uses a placeholder visual must provide an element
        // to which the DComp visual can be provided, and that element must be a Popup
        ASSERT(element != nullptr);
        ASSERT(element->OfTypeByIndex<KnownTypeIndex::Popup>());

        // Provide DComp visual to windowed popup, so it can be set into its window target
        CPopup *popup = static_cast<CPopup*>(element);
        if (child->OfTypeByIndex<KnownTypeIndex::HWWindowedPopupCompTreeNodeWinRT>())
        {
            IFCFAILFAST(popup->SetRootVisualForWindowedPopupWindow(child->GetWUCVisual().get()));
        }
        else
        {
            FAIL_FAST_ASSERT(!popup->IsWindowed());
            CPopup* ancestorWindowedPopup = popup->GetFirstAncestorPopup(true /* windowedOnly */);
            FAIL_FAST_ASSERT(ancestorWindowedPopup);

            ancestorWindowedPopup->AddAdditionalVisualForWindowedPopupWindow(child->GetWUCVisual().get());
        }
    }
    else
    {
        wucChild = child->GetWUCVisual();
    }

    // Figure out which visual to insert the new child above, this is typically either the reference Node's visual, or nothing.
    xref_ptr<WUComp::IVisual> wucPreviousSibling;

    if (previousSiblingVisual != nullptr)
    {
        wucPreviousSibling = previousSiblingVisual;
    }
    else if (referenceNode != nullptr)
    {
        wucPreviousSibling = referenceNode->GetWUCVisualInMainTree();
    }

    xref_ptr<WUComp::IVisualCollection> childCollection;
    GetVisualCollectionFromWUCSpineVisual(GetBottomMostVisual(), childCollection.ReleaseAndGetAddressOf());

    if (wucPreviousSibling == nullptr)
    {
        HRESULT insertHR = childCollection->InsertAtBottom(wucChild.Get());

        //
        // LTEs target elements and render their target elements at another place in the tree. Multiple LTEs can target
        // the same element, which causes it to appear multiple times. For something like a Border or a TextBlock this
        // doesn't cause a problem - the UIElement knows how to make a copy of its content SpriteVisual for each LTE.
        // For SwapChainPanel this is a problem. The app provided a swap chain to the SCP, and the SCP puts it in a
        // single SpriteVisual, and we can't put this SpriteVisual in multiple places in the tree. When the second LTE
        // goes to render the SCP, it's going to get an E_INVALIDARG when inserting the swap chain SpriteVisual because
        // the it's already parented in the tree (by the first LTE). Silently ignore these errors.
        //
        // Note that SwapChainPanel is unique as the only element that hits this problem. Normally when an LTE targets a
        // UIElement, CUIElement::AddLayoutTransitionRenderer will call LeavePCSceneRecursive on the target element,
        // which releases its comp node and Composition Visuals. When the LTE renders, it creates a new comp node and
        // new Visuals. This happens for each LTE, so the multiple branches never share the same Visuals. SCP internally
        // holds a SpriteVisual that doesn't get released by LeavePCSceneRecursive, so it gets reused by multiple
        // branches and we run into this problem. SCP must hold this SpriteVisual because it's partly exposed to the
        // app. The app can create an IInputPointerSource off the UI thread, and that object requires a Visual to back
        // it. Once we hand out an IInputPointerSource, we have to keep the corresponding Visual alive, so we can't be
        // releasing it when an LTE targets the SCP.
        //
        if (!ignoreInsertVisualErrors)
        {
            IFCFAILFAST(insertHR);
        }
        else if (!SUCCEEDED(insertHR) && insertHR != E_INVALIDARG)
        {
            IFCFAILFAST(insertHR);
        }
    }
    else
    {
        IFCFAILFAST(childCollection->InsertAbove(wucChild.Get(), wucPreviousSibling));
    }
}

_Check_return_ HRESULT HWCompTreeNodeWinRT::InsertChildAtBeginningInternal(
    _In_opt_ DCompTreeHost *dcompTreeHost,
    _In_ HWCompNode *child)
{
    // Update the DComp tree.
    IFCFAILFAST(child->EnsureVisual(dcompTreeHost));

    EnsureVisual(dcompTreeHost);

    xref_ptr<WUComp::IVisual> wucChild = child->GetWUCVisual();

    xref_ptr<WUComp::IVisualCollection> childCollection;
    GetVisualCollectionFromWUCSpineVisual(GetBottomMostVisual(), childCollection.ReleaseAndGetAddressOf());

    IFCFAILFAST(childCollection->InsertAtBottom(wucChild));

    return S_OK;
}

_Check_return_ HRESULT HWCompTreeNodeWinRT::RemoveChildInternal(_In_ HWCompNode *child)
{
    ASSERT(m_prependVisual != nullptr);

    xref_ptr<WUComp::IVisualCollection> childCollection;
    GetVisualCollectionFromWUCSpineVisual(GetBottomMostVisual(), childCollection.ReleaseAndGetAddressOf());

    // During shutdown we might find no child collection due to the CompositionContent already closing down the entire tree.
    if (childCollection)
    {
        xref_ptr<WUComp::IVisual> visualToRemove = child->GetWUCVisualInMainTree();

        IFCFAILFAST(childCollection->Remove(visualToRemove));

        // If this is an inline popup nested inside a windowed popup, we just removed the inline popup's placeholder
        // visual from the main tree. We also need to remove the inline popup real visual from the windowed popup's
        // island.
        if (child->UsesPlaceholderVisual()
            && !child->OfTypeByIndex<KnownTypeIndex::HWWindowedPopupCompTreeNodeWinRT>())
        {
            ASSERT(child->OfTypeByIndex<KnownTypeIndex::HWRedirectedCompTreeNodeWinRT>());
            HWRedirectedCompTreeNodeWinRT* childRedirected = static_cast<HWRedirectedCompTreeNodeWinRT*>(child);

            CUIElement* element = childRedirected->GetUIElementPeer();
            ASSERT(element->OfTypeByIndex<KnownTypeIndex::Popup>());
            CPopup* popup = static_cast<CPopup*>(element);
            FAIL_FAST_ASSERT(!popup->IsWindowed());

            CPopup* ancestorWindowedPopup = popup->GetFirstAncestorPopup(true /* windowedOnly */);
            FAIL_FAST_ASSERT(ancestorWindowedPopup);

            ancestorWindowedPopup->RemoveAdditionalVisualForWindowedPopupWindow(child->GetWUCVisual().get());
        }
    }

    IFCFAILFAST(child->RemoveParent(this));

    return S_OK;
}

void HWCompTreeNodeWinRT::RemoveSynchronous()
{
    CDependencyObject* parent = GetParentInternal(false /*isPublic*/);

    TraceCompTreeRemoveFromParentBegin(
        reinterpret_cast<XUINT64>(parent),
        reinterpret_cast<XUINT64>(this),
        0 /* removeForReparenting */,
        0 /* shouldMergePrimitiveGroups */);

    //
    // Suppose we have this WUC visual tree:
    //
    //  <A Container>
    //      <B Container>
    //          <C Sprite />
    //          <D Container>
    //              <E Sprite />
    //              <F Sprite />
    //              <G Sprite />
    //          </D>
    //          <H Sprite />
    //      </B>
    //      <I Sprite />
    //  </A>
    //
    // Now the comp node corresponding to B gets removed from the comp node tree. We need to deal with B's children.
    //
    // The sprite visuals C and H can be removed. Since their ancestor comp node changed, they must be re-rendered anyway.
    //
    // The container visual D can be preserved. D itself needs to be updated with new transforms since its ancestor comp node
    // changed, but D's children aren't made dirty by this operation.
    //
    // So all we need to do here is to walk the child comp nodes of B and reparent them to A. Then B's children can be cleared.
    //

    // 1. Move all the comp node children up to the parent.
    if (parent != nullptr)
    {
        ASSERT(parent->OfTypeByIndex<KnownTypeIndex::HWCompTreeNode>());
        HWCompTreeNodeWinRT* parentCompNode = static_cast<HWCompTreeNodeWinRT*>(parent);

        for (CompNodeCollection::reverse_iterator it = m_children.rbegin(); it != m_children.rend(); ++it)
        {
            HWCompNode* child = *it;

            IFCFAILFAST(RemoveChildInternal(child));

            // Only move comp tree node children. There can be other children like swap chain comp nodes. They'll be regenerated
            // by the render walk if needed.
            if (child->OfTypeByIndex<KnownTypeIndex::HWCompTreeNode>())
            {
                // If there are child comp nodes that represent open windowed popups, we need to pass the popup
                // along when we reparent the child comp node. The new parent will ensure the windowed popup is
                // attached correctly when inserting the child comp node.
                CUIElement* childUIElement = static_cast<HWCompTreeNode*>(child)->GetUIElementPeer();

                parentCompNode->InsertChildSynchronous(
                    NULL /*pDCompTreeHost*/,
                    child,
                    this, /*pReferenceNode*/
                    nullptr /* previousSiblingVisual */,
                    childUIElement,
                    false /* ignoreInsertVisualErrors */);
            }

            ReleaseInterface(child);
            IFCFAILFAST(m_children.erase(it));
        }

        IFCFAILFAST(RemoveCompNode(parentCompNode));
    }

    // 2. After reparenting the comp node children, reset the children collection.
    // For CoreWindow scenarios, the CompositionContent is also listening for the CoreWindow's closed event.
    // CompositionContent will get the notification first and close the entire visual tree, then Xaml will
    // exit its message loop and tear down the tree. Since CompositionContent already closed everything,
    // Xaml will get lots of RO_E_CLOSED errors. These are all safe to ignore. So tolerate RO_E_CLOSED if
    // we're also in the middle of tearing down the tree.
    xref_ptr<WUComp::IVisualCollection> childrenCollection;
    HRESULT hr = GetContainerVisualForChildren()->get_Children(childrenCollection.ReleaseAndGetAddressOf());
    if (SUCCEEDED(hr))
    {
        IFCFAILFAST(childrenCollection->RemoveAll());
    }
    else if (hr != RO_E_CLOSED || !GetContext()->IsTearingDownTree())
    {
        IFCFAILFAST(hr);
    }

    TraceCompTreeRemoveFromParentEnd(
        reinterpret_cast<XUINT64>(parent),
        reinterpret_cast<XUINT64>(this),
        0 /* removeForReparenting */,
        0 /* shouldMergePrimitiveGroups */);
}

Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> HWCompTreeNodeWinRT::GetTransform3DWinRTExpression(
    _In_ WinRTExpressionConversionContext* context
    )
{
    auto transform3D = m_pUIElementNoRef->GetTransform3D();
    if (transform3D != nullptr)
    {
        XSIZEF elementSize;
        IFCFAILFAST(m_pUIElementNoRef->GetElementSizeForProjection(&elementSize));  // This needs to be componentized first

        transform3D->MakeWinRTExpression(context, elementSize.width, elementSize.height);

        return transform3D->GetWinRTExpression();
    }
    else
    {
        return nullptr;
    }
}

// Add Transform3D contribution to LocalTransform Expression. Note that the expression already
// incorporates a term "LOCAL.Transform3D", so we only need to update the referenced PS.
void HWCompTreeNodeWinRT::ApplyTransform3DToLocalTransformExpression(
    _In_ DCompTreeHost* dcompTreeHost,
    _In_ WinRTLocalExpressionBuilder* builder
    )
{
    WinRTExpressionConversionContext context(dcompTreeHost->GetCompositor());
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> transform3D_EA = GetTransform3DWinRTExpression(&context);

    if (transform3D_EA != nullptr)
    {
        builder->ApplyTransform3DExpression(transform3D_EA.Get());
    }
}

Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> HWCompTreeNodeWinRT::GetProjectionWinRTExpression(
    _In_ WinRTExpressionConversionContext* context
    )
{
    auto projection = m_pUIElementNoRef->GetProjection();
    if (projection != nullptr)
    {
        XSIZEF elementSize;
        IFCFAILFAST(m_pUIElementNoRef->GetElementSizeForProjection(&elementSize));  // This needs to be componentized first

        projection->MakeWinRTExpression(context, elementSize.width, elementSize.height);

        return projection->GetWinRTExpression();
    }
    else
    {
        return nullptr;
    }
}

// Add Projection contribution to LocalTransform Expression. Note that the expression already
// incorporates a term "LOCAL.Projection", so we only need to update the referenced PS.
void HWCompTreeNodeWinRT::ApplyProjectionToLocalTransformExpression(
    _In_ DCompTreeHost* dcompTreeHost,
    _In_ WinRTLocalExpressionBuilder* builder
    )
{
    WinRTExpressionConversionContext context(dcompTreeHost->GetCompositor());
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> projectionEA = GetProjectionWinRTExpression(&context);

    if (projectionEA != nullptr)
    {
        builder->ApplyProjectionExpression(projectionEA.Get());
    }
}

_Check_return_ HRESULT HWCompTreeNodeWinRT::SetConnectedAnimationRunning(_In_ bool isRunning, _Outptr_opt_ WUComp::IVisual ** visual)
{
    if (!m_prependVisual && isRunning)
    {
        IFC_RETURN(E_UNEXPECTED);
    }

    if (m_prependVisual)
    {
        m_prependVisual->put_IsVisible(!isRunning);
        m_isInConnectedAnimation = isRunning;
        UpdateVisualClipAndTransform(m_pUIElementNoRef->GetDCompTreeHost(), HasComponentsVisual() ? GetComponentsVisual() : m_primaryVisual.Get());
        UpdateTransitionClipVisual(m_pUIElementNoRef->GetDCompTreeHost());
    }

    if (visual)
    {
        ReplaceInterface(*visual, m_primaryVisual.Get());
    }

    return S_OK;
}

void HWCompTreeNodeWinRT::SetLightsTargetingElement(_In_ std::vector<CXamlLight*> lights)
{
    ASSERT(m_lightsTargetingElement.size() == 0);

    m_lightsTargetingElement = std::move(lights);
    m_shouldUpdateLightsTargetingElement = true;
    CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);
}

void HWCompTreeNodeWinRT::UntargetFromLights(_In_ DCompTreeHost* dcompTreeHost)
{
    auto& targetMap = dcompTreeHost->GetXamlLightTargetMap();
    targetMap.RemoveTargetVisual(m_pUIElementNoRef, m_primaryVisual.Get());
}

void HWCompTreeNodeWinRT::SetLightsAttachedToElement()
{
    m_shouldUpdateLightsAttachedToElement = true;
    CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);
}

WUComp::IContainerVisual* HWCompTreeNodeWinRT::GetContainerVisualForChildren() const
{
    wrl::ComPtr<WUComp::IContainerVisual> containerVisual;
    m_contentVisual->QueryInterface(IID_PPV_ARGS(containerVisual.ReleaseAndGetAddressOf()));
    return containerVisual.Get();   // return NoRef pointer. m_contentVisual will keep the object alive.
}

void HWCompTreeNodeWinRT::UpdateContentVisual(_In_ DCompTreeHost *dcompTreeHost)
{
    if(NeedsContentVisualForShadows(dcompTreeHost, m_pUIElementNoRef))
    {
        // In most cases, it should have been created by EnsureVisual().
        // One exception is if element had a compnode for reason other than shadow, and is now getting a shadow.
        // In this case, insert the content at the spine bottom and reparent the children.
        // TODO_Shadows: Remove this when Feature_SynchronousCompTreeUpdates is enabled.
        if (!m_contentVisual)
        {
            CreateContentVisual(dcompTreeHost);

            WUComp::IVisual* contentParent = GetBottomMostNonContentVisual();
            ReparentVisualChildrenHelper(contentParent  /* oldParent */, m_contentVisual.Get() /* newParent */);
            InsertWUCSpineVisual(contentParent, m_contentVisual.Get());
        }

        m_pUIElementNoRef->SetShadowVisual(m_contentVisual.Get());

        // Set ContentVisual Size (needed by ThemeShadow) on Casters and Custom Receivers
        // NOTE: default receivers (flattened Root + opened popups) are marked as such regardless
        // of whether or not there are shadows. To avoid major churn in MockDComp masters,
        // defer setting their size until they are actually added to WUC ProjectedShadowScene.
        wfn::Vector2 size = { m_pUIElementNoRef->GetActualWidth(), m_pUIElementNoRef->GetActualHeight() };
        IFCFAILFAST(m_contentVisual->put_Size(size));
    }

    if (!CThemeShadow::IsDropShadowMode())
    {
        dcompTreeHost->GetProjectedShadowManager()->UpdateCasterStatus(m_pUIElementNoRef);
    }
}

void HWCompTreeNodeWinRT::CleanupContentVisual()
{
    if (!CThemeShadow::IsDropShadowMode())
    {
        auto pUIElementPeer = m_pUIElementPeerWeakRef.lock();

        if (pUIElementPeer)
        {
            // If there is no peer, this must be a transient compnode for Portaling or Redirection
            // It will be cleaned up in HWCompTreeNode destructor

            // In test-only CCoreServices::ShutdownToIdle path, CRootVisual's compnode gets deleted late enough that
            // we can't get a DCompTreeHost as usual via NWGetWindowRenderTarget()->GetDCompTreeHost()
            // CRootVisual's compnode is never a caster or receiver so we'll just skip shadow cleanup for this case.
            CWindowRenderTarget* windowRenderTarget = pUIElementPeer->GetContext()->NWGetWindowRenderTarget();
            if (windowRenderTarget)
            {
                DCompTreeHost* dcompTreeHost = windowRenderTarget->GetDCompTreeHost();

                // Normally caster and receiver visuals are marked for removal from shadow scene in CUIElement::RemoveCompositionPeer.
                // In case of portal-ing or redirection, the compnode is not referenced by a UIElement, so check now if this is a caster or receiver that needs to be marked for removal.
                // Remove from caster list if needed
                dcompTreeHost->GetProjectedShadowManager()->RemoveCaster(m_contentVisual.Get());

                // Remove from receiver list if needed
                // TODO_Shadows: Currently, receiver list is fully rebuilt on every frame so we don't need any action here
                //               When we have incremental receiver list updates, dirtying - and possibly updating - the list here will be necessary
                // dcompTreeHost->GetProjectedShadowManager()->SetReceiversDirty(true);
            }
        }
    }
}

bool HWCompTreeNodeWinRT::NeedsContentVisualForShadows(_In_ DCompTreeHost* dcompTreeHost, _In_ CUIElement* element) const
{
    if (CThemeShadow::IsDropShadowMode())
    {
        return false;
    }

    if (!dcompTreeHost->GetProjectedShadowManager()->AreShadowsEnabled())
    {
        return false;
    }

    CUIElement* referenceElement = element;
    // RS5 Bug #17622840:  While an LTE is targeting an element with a ThemeShadow set, the target element will not have
    // a CompNode, and therefore cannot cast its shadows.  The fix is to use the LTE's visual to stand in for the target.
    // When the LTE goes away the target will gain its CompNode and shadows will transfer over to the target's visual.
    if (element->OfTypeByIndex<KnownTypeIndex::LayoutTransitionElement>())
    {
        CLayoutTransitionElement* lte = static_cast<CLayoutTransitionElement*>(element);
        referenceElement = lte->GetTargetElement();
    }

    // Casters and custom receivers use ContentVisual (default Receivers use primary)
    if (referenceElement->IsShadowCaster() || referenceElement->IsProjectedShadowCustomReceiver())
    {
        return true;
    }

    // Child of Popup with Shadow is implicitly a shadow caster.
    CUIElement* parent = referenceElement->GetUIElementAdjustedParentInternal();
    if (parent && parent->OfTypeByIndex<KnownTypeIndex::Popup>() && parent->IsShadowCaster())
    {
        return true;
    }

    return false;
}

void HWCompTreeNodeWinRT::CreateContentVisual(_In_ DCompTreeHost* dcompTreeHost)
{
    ASSERT(!m_contentVisual);

    wrl::ComPtr<WUComp::IContainerVisual> container;
    IFCFAILFAST(dcompTreeHost->CreateContainerVisual(container.ReleaseAndGetAddressOf()));
    container.As(&m_contentVisual);

    SetDebugTag(m_contentVisual.Get(), s_contentVisualTag);
    DCompTreeHost::SetTagIfEnabled(m_contentVisual.Get(), VisualDebugTags::CompNode_ContentVisual);
}

CThemeShadow* HWCompTreeNodeWinRT::GetThemeShadowFromDropShadowCaster() const
{
    CUIElement* elementWithThemeShadow = GetElementForLTEProperties();
    CUIElement* parent = elementWithThemeShadow->GetUIElementAdjustedParentInternal();
    if (parent && parent->OfTypeByIndex<KnownTypeIndex::Popup>() && parent->IsShadowCaster())
    {
        elementWithThemeShadow = parent;
    }

    CValue shadowValue;
    IFCFAILFAST(elementWithThemeShadow->GetValueByIndex(KnownPropertyIndex::UIElement_Shadow, &shadowValue));
    CThemeShadow* themeShadow = static_cast<CThemeShadow*>(shadowValue.AsObject());
    ASSERT(themeShadow != nullptr);

    return themeShadow;
}

CUIElement* HWCompTreeNodeWinRT::GetElementForLTEProperties() const
{
    // When an element has certain properties and is the target of an LTE, the element can't
    // have a CompNode, so we transfer any of those properties to the LTE.
    // Currently the set of properties is limited to the Shadow and CornerRadius properties.
    CUIElement* targetElement = m_pUIElementNoRef;
    if (m_pUIElementNoRef->OfTypeByIndex<KnownTypeIndex::LayoutTransitionElement>())
    {
        CLayoutTransitionElement* lte = static_cast<CLayoutTransitionElement*>(m_pUIElementNoRef);
        targetElement = lte->GetTargetElement();
    }

    return targetElement;
}

bool HWCompTreeNodeWinRT::NeedsDropShadowVisual() const
{
    if (CThemeShadow::IsDropShadowMode())
    {
        bool isHighContrast = GetContext()->GetFrameworkTheming()->HasHighContrastTheme();
        if (isHighContrast)
        {
            return false;
        }

        CUIElement* elementForDropShadowProperties = GetElementForLTEProperties();
        if (elementForDropShadowProperties->OfTypeByIndex<KnownTypeIndex::Popup>())
        {
            // Popup with Shadow delegates its shadow to Popup.Child (see below).
            return false;
        }

        if (elementForDropShadowProperties->IsShadowCaster())
        {
            return true;
        }

        // Child of Popup with Shadow is implicitly a shadow caster.
        CUIElement* parent = elementForDropShadowProperties->GetUIElementAdjustedParentInternal();
        if (parent && parent->OfTypeByIndex<KnownTypeIndex::Popup>() && parent->IsShadowCaster())
        {
            return true;
        }
    }

    return false;
}

void HWCompTreeNodeWinRT::EnsureDropShadowVisual(_In_ DCompTreeHost* dcompTreeHost)
{
    if (!m_dropShadowParentVisual)
    {
        // First create a parent interop visual.
        // We need this visual to carry legacy properties (eg TransformParent)
        // Note that we're relying on this visual being lower in the z-order than the Content visual,
        // as the drop shadow must render beneath all content.
        CreateWUCSpineVisual(dcompTreeHost, m_dropShadowParentVisual.ReleaseAndGetAddressOf(), s_dropShadowVisualTag);
        DCompTreeHost::SetTagIfEnabled(m_dropShadowParentVisual.Get(), VisualDebugTags::CompNode_DropShadowVisual);
        InsertWUCSpineVisual(m_prependVisual.Get(), m_dropShadowParentVisual.Get());

        // Create a SpriteVisual to hold most properties and draw the drop shadow.
        wrl::ComPtr<ixp::ISpriteVisual> dropShadowSpriteVisual;
        IFCFAILFAST(dcompTreeHost->GetCompositor()->CreateSpriteVisual(dropShadowSpriteVisual.ReleaseAndGetAddressOf()));
        dropShadowSpriteVisual.As(&m_dropShadowSpriteVisual);
        InsertWUCSpineVisual(m_dropShadowParentVisual.Get(), m_dropShadowSpriteVisual.Get());
    }
}

void HWCompTreeNodeWinRT::CleanupDropShadowVisual()
{
    if (m_dropShadowParentVisual)
    {
        RemoveWUCSpineVisual(m_prependVisual.Get(), m_dropShadowParentVisual.Get());
        m_dropShadowParentVisual = nullptr;
        m_dropShadowSpriteVisual = nullptr;

        // Resetting this flag makes us restart the animation the next time we need a drop shadow. This prevents the
        // old drop shadow from staying on screen while the menu animates in a second time.
        m_isPlayingDropShadowOpacityAnimation = false;
    }
}

bool HWCompTreeNodeWinRT::HasHitTestVisibleContentInSubtree() const
{
    // When we're doing synchronous comp tree updates, the tree nodes themselves can have content directly, rather than
    // having a child render data node with content. Check this tree node itself first.
    if (m_contentVisual)
    {
        wrl::ComPtr<WUComp::IContainerVisual> containerVisual;
        IFCFAILFAST(m_contentVisual.As(&containerVisual));

        wrl::ComPtr<ixp::IVisual3> visual3;
        IFCFAILFAST(containerVisual.As(&visual3));

        boolean isHitTestVisible = true;
        IFCFAILFAST(visual3->get_IsHitTestVisible(&isHitTestVisible));

        if (HWCompWinRTVisualRenderDataNode::HasHitTestVisibleSpriteVisuals(containerVisual.Get()))
        {
            return true;
        }
        else if (HasHandInVisual() && isHitTestVisible)
        {
            // Hand-in visuals are attached as a sibling of the ContentVisual, not as a child. Check for it explicitly.
            return true;
        }
    }

    return __super::HasHitTestVisibleContentInSubtree();
}
