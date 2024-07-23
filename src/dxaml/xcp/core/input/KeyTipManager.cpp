// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <MUX-ETWEvents.h>
#include "KeytipAutomaticPlacementAlgorithm.h"
#include "FocusProperties.h"
#include "RichTextBlockView.h"
#include <utility>
#include "InputServices.h"
#include "KeyboardAcceleratorUtility.h"
#include "ComMacros.h"
#include "Microsoft.UI.Xaml.h"
#include "CValueBoxer.h"

#ifdef DBG_KEYTIPS
#define KEYTIPTRACE(...) \
    GetPALDebuggingServices()->XcpTrace(MonitorRaw, __WFILE__,__LINE__, 0, NULL,L"KeyTips: " WIDEN_VA(,#__VA_ARGS__),##__VA_ARGS__);
#else
#define KEYTIPTRACE(...)
#endif

constexpr XRECTF_RB infiniteBounds { -FLT_MAX, -FLT_MAX, FLT_MAX, FLT_MAX };

namespace KeyTipHelper {
    // Create a DependencyObject-based object
    template <typename T>
    xref_ptr<T> CreateObject(_In_ CCoreServices* core)
    {
        xref_ptr<T> newObjPtr;
        CDependencyObject* newObj = nullptr;
        CREATEPARAMETERS cp(core);

        IFCFAILFAST(T::Create(&newObj, &cp));
        newObjPtr.attach(static_cast<T*>(newObj));

        return newObjPtr;
    }

    _Check_return_ HRESULT SetupPopup(_In_ CPopup* popup)
    {
        // Turn off HighContrastAdjustment for the KeyTip, since we've already chosen colors for the KeyTips that work well
        // in high-contast mode.
        CValue highContrastAdjustment;
        highContrastAdjustment.SetEnum(DirectUI::ElementHighContrastAdjustment::None);
        IFC_RETURN(popup->SetValueByKnownIndex(KnownPropertyIndex::UIElement_HighContrastAdjustment, highContrastAdjustment));

        IFC_RETURN(popup->SetIsWindowed());
        IFC_RETURN(popup->SetValueByKnownIndex(KnownPropertyIndex::UIElement_IsHitTestVisible, false));
        IFC_RETURN(popup->SetValueByKnownIndex(KnownPropertyIndex::Popup_IsOpen, true));
        return S_OK;
    }

    struct VisualProperties
    {
        _Check_return_ HRESULT EnsureInitialized(_In_ CCoreServices* core);
        xref_ptr<CDependencyObject> KeyTipBackground;
        xref_ptr<CDependencyObject> KeyTipBorderBrush;
        xref_ptr<CDependencyObject> KeyTipBorderThemeThickness;
        xref_ptr<CDependencyObject> KeyTipThemePadding;
        xref_ptr<CDependencyObject> KeyTipForeground;
        xref_ptr<CDependencyObject> KeyTipFontFamily;
        xref_ptr<CDependencyObject> KeyTipContentThemeFontSize;
    };

    _Check_return_ HRESULT SetupBorder(_In_ CBorder* border, _In_ const VisualProperties& visualProperties)
    {
        IFC_RETURN(border->SetValueByKnownIndex(KnownPropertyIndex::Border_Background, visualProperties.KeyTipBackground.get()));
        IFC_RETURN(border->SetValueByKnownIndex(KnownPropertyIndex::Border_BorderBrush, visualProperties.KeyTipBorderBrush.get()));
        IFC_RETURN(border->SetValueByKnownIndex(KnownPropertyIndex::Border_BorderThickness, visualProperties.KeyTipBorderThemeThickness.get()));
        IFC_RETURN(border->SetValueByKnownIndex(KnownPropertyIndex::Border_Padding, visualProperties.KeyTipThemePadding.get()));
        return S_OK;
    }

    _Check_return_ HRESULT SetupTextBlock(_In_ CTextBlock* textBlock, _In_ const VisualProperties& visualProperties)
    {
        IFC_RETURN(textBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_Foreground, visualProperties.KeyTipForeground.get()));
        IFC_RETURN(textBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_FontFamily, visualProperties.KeyTipFontFamily.get()));
        IFC_RETURN(textBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_FontSize, visualProperties.KeyTipContentThemeFontSize.get()));
        return S_OK;
    }

    _Check_return_ HRESULT SetText(
        _In_ CTextBlock* textBlock,
        _In_opt_ CDependencyObject* elementWithAccessKey,
        _In_ const unsigned int pressedLength)
    {
        xstring_ptr accessKey;
        xstring_ptr textToShow;

        if (elementWithAccessKey)
        {
            CValue textValue = {};

            KnownPropertyIndex index = KnownPropertyIndex::UnknownType_UnknownProperty;

            if (elementWithAccessKey->OfTypeByIndex<KnownTypeIndex::UIElement>()) { index = KnownPropertyIndex::UIElement_AccessKey; }
            else if (elementWithAccessKey->OfTypeByIndex<KnownTypeIndex::TextElement>()) { index = KnownPropertyIndex::TextElement_AccessKey; }

            IFC_RETURN(elementWithAccessKey->GetValueByIndex(index, &textValue));
            IFC_RETURN(textValue.GetString(accessKey));
        }

        // trim off the front of the string as needed
        if (pressedLength == 0)
        {
            textToShow = accessKey;
        }
        else
        {
            FAIL_FAST_ASSERT(pressedLength < accessKey.GetCount());
            const WCHAR* buffer = accessKey.GetBuffer();
            IFC_RETURN(xstring_ptr::CloneBuffer(buffer + pressedLength, &textToShow));
        }

        CValue textToShowValue;
        textToShowValue.SetString(std::move(textToShow));
        IFC_RETURN(textBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_Text, textToShowValue));
        return S_OK;
    }

    _Check_return_ HRESULT SetPosition(_In_ CBorder* border, _In_ CDependencyObject* element, KeyTip* keyTip)
    {
        IFC_RETURN(border->Measure({ XFLOAT_INF, XFLOAT_INF }));

        CValue horizontalOffsetValue = {};
        CValue verticalOffsetValue = {};

        if (element->OfTypeByIndex<KnownTypeIndex::UIElement>())
        {
            IFC_RETURN(element->GetValueByIndex(KnownPropertyIndex::UIElement_KeyTipHorizontalOffset, &horizontalOffsetValue));
            IFC_RETURN(element->GetValueByIndex(KnownPropertyIndex::UIElement_KeyTipVerticalOffset, &verticalOffsetValue));
        }
        else if (element->OfTypeByIndex<KnownTypeIndex::TextElement>())
        {
            IFC_RETURN(element->GetValueByIndex(KnownPropertyIndex::TextElement_KeyTipHorizontalOffset, &horizontalOffsetValue));
            IFC_RETURN(element->GetValueByIndex(KnownPropertyIndex::TextElement_KeyTipVerticalOffset, &verticalOffsetValue));
        }

        XSIZEF keytipSize = border->DesiredSize;

        //Initially place the keytip in the top left-hand corner.
        //This will be updated by positioning algorithm.
        keyTip->KeytipBounds.right = keytipSize.width;
        keyTip->KeytipBounds.bottom = keytipSize.height;
        keyTip->KeytipBounds.top = 0;
        keyTip->KeytipBounds.left = 0;

        keyTip->HorizontalOffset = (float)horizontalOffsetValue.AsDouble();
        keyTip->VerticalOffset = (float)verticalOffsetValue.AsDouble();
        keyTip->IsRightToLeft = element->IsRightToLeft();

        return S_OK;
    }
}

namespace BoundsHelper {

    // Returns a rectangle that represents the bounds of the XAML root.  The coordinate space returned is ClientLogical.
    // For win32/islands, it's the bounds of the island.
    // For UWP, this is ApplicationView.VisibleBounds (on desktop this is usually identical to CoreWindow.Bounds)
    _Check_return_ HRESULT GetVisibleContentBounds(_In_opt_ CDependencyObject* const object, _Out_ XRECTF_RB& visibleBounds)
    {
        if(object == nullptr)
        {
            visibleBounds = infiniteBounds;
        }
        else
        {
            wf::Rect visibleBoundsAsRect;
            IFC_RETURN(FxCallbacks::DXamlCore_GetVisibleContentBoundsForElement(object, &visibleBoundsAsRect));
            visibleBounds = { visibleBoundsAsRect.X,
                visibleBoundsAsRect.Y,
                visibleBoundsAsRect.X + visibleBoundsAsRect.Width,
                visibleBoundsAsRect.Y + visibleBoundsAsRect.Height };
        }

        return S_OK;
    }

    XRECTF_RB OffsetBounds(
        _In_ const XRECTF_RB& bounds,
        _In_ const XRECTF_RB& containerBounds)
    {
        //If the top-left of the container is at a mainimal value, we were unable to get container Bounds,
        //In this situation, use an offset of 0,0 for scaling.

        float xOffset = (containerBounds.left == (-FLT_MAX)) ? 0 : containerBounds.left;
        float yOffset = (containerBounds.top == (-FLT_MAX)) ? 0 : containerBounds.top;

        return {
                bounds.left + xOffset,
                bounds.top + yOffset,
                bounds.right + xOffset,
                bounds.bottom + yOffset };
    }

    _Check_return_ HRESULT AddBoundsForTextElement(
        _In_ CTextElement* pTextElement,
        _Inout_ std::vector<XRECTF_RB>& elementBounds)
    {
        ITextView* textView = nullptr;
        CFrameworkElement* container = pTextElement->GetContainingFrameworkElement();
        XUINT32 boundsCount;
        XRECTF* hyperlinkBounds = nullptr;
        xref_ptr<CGeneralTransform> transform;

        XRECTF_RB visibleBounds;
        IFC_RETURN(GetVisibleContentBounds(container, visibleBounds));

        auto scopeGuard = wil::scope_exit([&]
        {
            delete[] hyperlinkBounds;
        });

        if (container->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
        {
            CRichTextBlock* richTextBlock = static_cast<CRichTextBlock*>(container);
            textView = richTextBlock->GetSingleElementTextView();
            IFC_RETURN(richTextBlock->TransformToVisual(nullptr, &transform));
        }
        else if (container->OfTypeByIndex<KnownTypeIndex::TextBlock>())
        {
            CTextBlock* textBlock = static_cast<CTextBlock*>(container);
            textView = textBlock->GetTextView();
            IFC_RETURN(textBlock->TransformToVisual(nullptr, &transform));
        }
        else
        {
            IFCFAILFAST(E_FAIL);
        }

        if (textView)
        {
            IFC_RETURN(RichTextBlockView::GetBoundsCollectionForElement(textView, pTextElement, &boundsCount, &hyperlinkBounds));

            for (unsigned int i = 0; i < boundsCount; i++)
            {
                XRECTF hyperlinkInContainer = {};
                IFC_RETURN(transform->TransformRect(hyperlinkBounds[i], &hyperlinkInContainer));

                const XRECTF_RB transformedBounds = {
                    hyperlinkInContainer.X,
                    hyperlinkInContainer.Y,
                    hyperlinkInContainer.X + hyperlinkInContainer.Width,
                    hyperlinkInContainer.Y + hyperlinkInContainer.Height
                };

                elementBounds.push_back(OffsetBounds(transformedBounds, visibleBounds));
            }
        }

        return S_OK;
    }

    _Check_return_ HRESULT GetFocusableElementBounds(
        _In_ CDependencyObject* const startRoot,
        _Inout_ std::vector<XRECTF_RB>& elementBounds,
        _In_ const XRECTF_RB& visibleBounds)
    {

        const auto& collection = FocusProperties::GetFocusChildren<CDOCollection>(startRoot);
        if (!collection || collection->IsLeaving())
        {
            return S_OK;
        }

        const unsigned int numFocusableChildren = collection->GetCount();

        //For every child in the tree that is focusable
        for (unsigned int i = 0; i < numFocusableChildren; i++)
        {
            xref_ptr<CDependencyObject> child;
            child.attach(static_cast<CDependencyObject*>(collection->GetItemWithAddRef(i)));

            CUIElement* childAsUIElement = do_pointer_cast<CUIElement>(child.get());
            CTextElement* childAsTextElement = do_pointer_cast<CTextElement>(child.get());

            if (childAsUIElement &&
                FocusProperties::IsFocusable(child.get(), false /*ignoreOffScreenPosition*/) &&
                !child->OfTypeByIndex<KnownTypeIndex::RootScrollViewer>())
            {
                XRECTF_RB childBounds;
                IFC_RETURN(childAsUIElement->GetGlobalBoundsLogical(&childBounds, false /*ignoreClipping*/));
                XRECTF_RB childBoundsinDips = OffsetBounds(childBounds, visibleBounds);
                elementBounds.push_back(childBoundsinDips);

            }
            else if (childAsTextElement &&
                FocusProperties::IsFocusable(child.get(), false /*ignoreOffScreenPosition*/))
            {
                IFC_RETURN(AddBoundsForTextElement(
                    childAsTextElement,
                    elementBounds));
            }

            if (!childAsUIElement || childAsUIElement->IsVisible())
            {
                IFC_RETURN(GetFocusableElementBounds(child.get(), elementBounds, visibleBounds));
            }
        }

        return S_OK;
    }

    // For this object, find the object we should use for positioning the KeyTip.  This function
    // may return object itself. It will not return nullptr.
    CDependencyObject* GetKeyTipTargetNoRef(_In_ CDependencyObject* const object)
    {
        // Check if object is UIElement and KeyTipTarget is defined on it.
        // KeyTipTarget can be used with non template controls e.g. Pivot.
        // If both are defined, KeyTipTarget will override the Template KeyTip target.
        CUIElement* pElement = do_pointer_cast<CUIElement>(object);
        if (pElement != nullptr)
        {
            CDependencyObject* pDO = nullptr;
            CValue value;
            VERIFYHR(pElement->GetValueByIndex(KnownPropertyIndex::UIElement_KeyTipTarget, &value));
            VERIFYHR(DirectUI::CValueBoxer::UnwrapWeakRef(&value, DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::UIElement_KeyTipTarget), &pDO));

            if (pDO != nullptr)
            {
                return pDO;
            }
        }

        CControl* const control = do_pointer_cast<CControl>(object);
        if (control)
        {
            CValue targetValue;
            IFCFAILFAST(control->GetValueByIndex(KnownPropertyIndex::Control_TemplateKeyTipTarget, &targetValue));
            xref_ptr<CDependencyObject> target =  targetValue.DetachObject();
            if (target)
            {
                return target.get(); // drops ref
            }
        }

        ASSERT(object != nullptr);
        return object;
    }

    XRECTF_RB GetGlobalBoundsForKeyTips(_In_ CDependencyObject* const object)
    {
        XRECTF_RB bounds;

        CDependencyObject* target = GetKeyTipTargetNoRef(object);

        CUIElement* element = do_pointer_cast<CUIElement>(target);
        CTextElement* textElement = do_pointer_cast<CTextElement>(target);

        if (element != nullptr)
        {
            XRECTF_RB visibleBounds;

            IFCFAILFAST(GetVisibleContentBounds(element, visibleBounds));

            // elementBounds is relative to the app,
            // while screenBounds is the visibleBounds of the windows with respect to the screen.

            //As a result, we offset keyTip.ObjectBounds appropriately for this shift to make sense.

            VERIFYHR(element->GetGlobalBoundsLogical(&bounds));

            bounds = OffsetBounds(bounds, visibleBounds);
        }
        else if (textElement != nullptr)
        {
            std::vector<XRECTF_RB> boundsList;
            IFCFAILFAST(AddBoundsForTextElement(textElement, boundsList));

            if (boundsList.empty()) { IFCFAILFAST(E_FAIL); } // We should always get the bounds for the textelement

            // If a hyperlink has spans multiple lines, we want to always get the bounds of the first line
            bounds = boundsList[0];
        }
        else
        {
            XAML_FAIL_FAST();
        }

        return bounds;
    }

}

using namespace KeyTipHelper;
using namespace BoundsHelper;

KeyTipManager::~KeyTipManager()
{
    Reset();
}

// Show a key tip asynchronously
void KeyTipManager::ShowAutoKeyTipForElement(
    _In_ CDependencyObject* obj,
    _In_z_ const wchar_t* keysPressed)
{
    if(!m_areKeyTipsEnabled) { return; }

    KEYTIPTRACE(L"ShowAutoKeyTipForElement %x", obj);

    DirectUI::KeyTipPlacementMode oldPlacementMode = DirectUI::KeyTipPlacementMode::Auto;
    bool placementModeFromFiltering = false;

    // If we have any KeyTips already bound to obj, hide its popup as we show the new one.
    // This is the case for multiple-character access keys, when we want to update the KeyTip to show
    // the remaining characters (e.g., the AccesKey is "ABC" and the user presses "A").
    for (auto& it : m_keyTips)
    {
        if (it.Object.get() == obj)
        {
            it.State = KeyTip::NeedsHidePopup;
            //Remember previous placement mode for consistent placement
            //after filtering
            if (it.Popup && !placementModeFromFiltering)
            {
                oldPlacementMode = it.PlacementMode;
                placementModeFromFiltering = true;
            }
        }
    }

    m_keyTips.emplace_back(obj, oldPlacementMode, placementModeFromFiltering);
    m_keysPressed = static_cast<decltype(m_keysPressed)>(wcslen(keysPressed));

    if (m_state == State::Idle)
    {
        GoToState(obj->GetContext(), State::WaitingToUpdateKeyTips);
    }
}

// Remove all the stale keytips from m_keyTips
void KeyTipManager::RemoveNullKeyTips()
{
    // Remove all the null elements
    m_keyTips.erase(
        std::remove_if(
            m_keyTips.begin(),
            m_keyTips.end(),
            [](const KeyTip& entry)
            {
                return entry.Object == nullptr;
            }),
        m_keyTips.end());

#if DBG
    for (auto& it : m_keyTips)
    {
        ASSERT(it.Object);
    }
#endif
}


// Called on UI thread, this is where we actually create the KeyTip visuals and place them
_Check_return_ HRESULT KeyTipManager::Execute()
{
    if (m_state == State::WaitingToUpdateKeyTips)
    {
        return S_OK;
    }

    // Loop through the entries and hide or show whatever popups need to be hidden/shown
    for (auto& it : m_keyTips)
    {
        if (it.State == KeyTip::NeedsHidePopup)
        {
            KEYTIPTRACE(L"Hiding KeyTip %x", it.Object);
            it.Reset(); // Just null out for now, we'll remove this next
        }
    }
    RemoveNullKeyTips();

    if (m_keyTips.empty())
    {
        // There's nothing left to do
        GoToState(nullptr /*core*/, State::Idle);
        return S_OK;
    }

    CDependencyObject* const root = VisualTree::GetRootOrIslandForElement(m_keyTips[0].Object);
    CCoreServices* const core = root->GetContext();

    // If we're suppressing KeyTips due to animations, we can't show them yet.  Instead
    // we'll wait for animations to finish and kick off a timer to wait for a timeout.
    if (m_finiteAnimationIsRunning && m_state != State::ForciblyUpdatingKeyTips)
    {
        GoToState(core, State::WaitingForAnimationsToComplete);
        return S_OK;
    }

    VisualProperties visualProperties;

    // Create popups for KeyTips that need them.  This must be done before the autopositioning
    // algorithm runs, since the algorithm depends on the size of the UI elements created for
    // the KeyTip popup.
    for (auto& keyTip : m_keyTips)
    {
        if (keyTip.State == KeyTip::NeedsPopup)
        {
            KEYTIPTRACE(L"Create Popup for KeyTip %x", keyTip.Object);
            IFC_RETURN(CreatePopup(keyTip, visualProperties, m_keysPressed));
        }
    }

    XRECTF_RB visibleBounds;
    IFC_RETURN(GetVisibleContentBounds(root, visibleBounds));

    std::vector<XRECTF_RB> focusableElementBounds;
    IFC_RETURN(GetFocusableElementBounds(root, focusableElementBounds, visibleBounds));

    // For screenBounds, when we're using windowed popups use infiniteBounds so that the KeyTips can display anywhere.
    // Previously, we constrained the KeyTip bounds to visibleBounds in all cases, which is the bounds of the Window or
    // DesktopWindowXamlSource/island.
    const bool usingWindowedPopups = CPopup::DoesPlatformSupportWindowedPopup(core);
    const XRECTF_RB screenBounds = usingWindowedPopups ? infiniteBounds : visibleBounds;

    // IslandsOnly means we're running in a non-UWP / win32 scenario.  To reduce compat risk for this
    // monitor-detection logic, we only do monitor detection in this case -- to avoid breaking existing UWP and
    // XamlPresenter scenarios.
    const bool enableMonitorDetection = (core->GetInitializationType() == InitializationType::IslandsOnly);

    KeytipAutomaticPlacementAlgorithm::PositionKeyTips(
        m_keyTips,
        screenBounds,
        focusableElementBounds,
        do_pointer_cast<CXamlIslandRoot>(root),
        enableMonitorDetection);

    for (auto& keyTip : m_keyTips)
    {
        if (keyTip.PlacementMode == DirectUI::KeyTipPlacementMode::Hidden)
        {
           keyTip.Reset();
        }
        else if (keyTip.Popup)
        {
            //Here, we subtract visibleBounds offsets as Popup Horizontal anmd Vertical Offsets are in app coordinates.
            //If screenBound offsets are minimal in the top left, we could not get visual bounds. In this situation,
            // scale by 0
            const float xOffset = (visibleBounds.left == (-FLT_MAX)) ? 0 : visibleBounds.left;
            const float yOffset = (visibleBounds.top == (-FLT_MAX)) ? 0 : visibleBounds.top;

            const float xPosition = keyTip.KeytipBounds.left - xOffset;
            const float yPosition = keyTip.KeytipBounds.top - yOffset;

            //Update keytip popup positions
            IFC_RETURN(keyTip.Popup->SetValueByKnownIndex(KnownPropertyIndex::Popup_HorizontalOffset, xPosition));
            IFC_RETURN(keyTip.Popup->SetValueByKnownIndex(KnownPropertyIndex::Popup_VerticalOffset, yPosition));
        }
    }

    // Remove all the null elements,
    // since new ones may have been introduced by
    // autopositioning
    RemoveNullKeyTips();
    GoToState(core, State::Idle);

    TraceKeyTipsVisualChangingInfo();

    return S_OK;
}




// Create a timer with specified timeout in ms.  The state transitions ensure that we only have one timer going
// at a time.
void KeyTipManager::StartStateTimer(_In_ CCoreServices* core, int milliseconds)
{
    FAIL_FAST_ASSERT(!m_stateTimer);

    m_stateTimer = CreateObject<CDispatcherTimer>(core);

    CValue handler;
    handler.SetInternalHandler(&KeyTipManager::OnStateTimerFiredStatic);
    IFCFAILFAST(m_stateTimer->AddEventListener(
        EventHandle(KnownEventIndex::DispatcherTimer_Tick),
        &handler,
        EVENTLISTENER_INTERNAL,
        nullptr /*pResult*/,
        false /*fHandledEventsToo*/));

    xref_ptr<CTimeSpan> timeoutTimeSpan;

    CValue resourceTimeout;
    resourceTimeout.SetFloat(0.001f * milliseconds);
    CREATEPARAMETERS cpTimeout(core, resourceTimeout);
    IFCFAILFAST(CTimeSpan::Create(reinterpret_cast<CDependencyObject**>(timeoutTimeSpan.ReleaseAndGetAddressOf()), &cpTimeout));

    resourceTimeout.WrapObjectNoRef(timeoutTimeSpan.get());
    IFCFAILFAST(m_stateTimer->SetValueByIndex(KnownPropertyIndex::DispatcherTimer_Interval, resourceTimeout));

    IFCFAILFAST(m_stateTimer->Start());
}

// Stop and delete the state timer
void KeyTipManager::StopStateTimer()
{
    if (m_stateTimer)
    {
        VERIFYHR(m_stateTimer->Stop());

        CValue handler;
        handler.SetInternalHandler(&KeyTipManager::OnStateTimerFiredStatic);
        VERIFYHR(m_stateTimer->RemoveEventListener(EventHandle(KnownEventIndex::DispatcherTimer_Tick), &handler));

        m_stateTimer = nullptr;
    }
}

// Hide a key tip asynchronously
void KeyTipManager::HideAutoKeyTipForElement(_In_ const CDependencyObject* const obj)
{
    KEYTIPTRACE(L"HideAutoKeyTipForElement %x", obj);

    // Hide even if !m_areKeyTipsEnabled
    for (auto& it : m_keyTips)
    {
        if (it.Object.get() == obj)
        {
            if (it.Popup)
            {
                it.State = KeyTip::NeedsHidePopup;
                if (m_state == State::Idle || m_state == State::WaitingForAnimationsToComplete)
                {
                    // When we're in WaitingForAnimationsToComplete, we want to kick a quicker update here so we can
                    // hide the KeyTips that are going away (when hiding KeyTips, we shouldn't want for animations
                    // to finish).  After hiding the KeyTips, we'll see that there's an animation running and go back
                    // to the WaitingForAnimationsToComplete state.
                    GoToState(obj->GetContext(), State::WaitingToUpdateKeyTips);
                }
            }
            else
            {
                it.Reset();
            }
        }
    }

    RemoveNullKeyTips();

    if (m_keyTips.empty() && m_state == State::WaitingForAnimationsToComplete)
    {
         // This will happen when we're changing scope and there are no visible KeyTips because we're waiting for
         // an animation to complete.  Let's go back to the idle state to reset the state timer.
        GoToState(obj->GetContext(), State::Idle);
    }
}

bool KeyTipManager::AreKeyTipsEnabled() const
{
    return m_areKeyTipsEnabled;
}

// This can be called via public API
void KeyTipManager::SetAreKeyTipsEnabled(bool enabled)
{
    if (m_areKeyTipsEnabled != enabled)
    {
        m_areKeyTipsEnabled = enabled;
        if (!m_areKeyTipsEnabled)
        {
            // Clear out all the KeyTips when the app switches off KeyTips
            Reset();
        }
    }
}

// Called when IsDisplayEnabled changes
void KeyTipManager::AccessKeyModeChanged()
{
    // Clear out all the KeyTip popups when entering and exiting AK mode
    Reset();
}

// Remove all the keytips from the live tree and forget about them
void KeyTipManager::Reset()
{
    // Remove all the active popups and clear out the popup map
    for (auto& it : m_keyTips)
    {
        if (it.Popup)
        {
            IFCFAILFAST(it.Popup->SetValueByKnownIndex(KnownPropertyIndex::Popup_IsOpen, false));
        }
    }
    m_keyTips.clear();
    m_keyTips.shrink_to_fit();
    m_keysPressed = 0;

    GoToState(nullptr /*core*/, State::Idle);
}

// Enter a new state.  "core" can be nullptr if newState is Idle.
// The state machine is mostly to deal with the problem that we need to wait for animations to complete before showing KeyTips.
// Here's the design:
// 1. When a KeyTip is requested (ALT key press or scope change), if there are no finite animations running the KeyTip is shown after a 1ms timer
// 2. If there are finite animations running, the KeyTip is shown after all finite animations have stopped (with 1s timeout).
// 3. If the platform requests a KeyTip be hidden while we're waiting for an animation, we hide the KeyTip after a 1ms timer.
//      (It's possible a KeyTip was waiting for an animation, but is never shown because a hide request came in.)
// 4. If a new KeyTip is requested while waiting for an animation to finish, it's shown along with the KeyTips already waiting to be
//      displayed -- we don't start a new timer for that KeyTip.  Exception: if a scope change happens while waiting for an animation, we
//      do start a new timer for the new incoming KeyTips.  Note KeyTipManager itself doesn't know about scopes, we see that as a set of
//      AK dismissal requests, and then AK display requests.
void KeyTipManager::GoToState(_In_opt_ CCoreServices* core, State newState)
{
    const State oldState = m_state;

    switch (oldState)
    {
        case State::WaitingToUpdateKeyTips:
        case State::WaitingForAnimationsToComplete:
            // Whenever we're *leaving* these states, stop the state timer.
            StopStateTimer();
            break;
    };

    switch (newState)
    {
        case State::Idle:
            KEYTIPTRACE(L"GoToState Idle");
            break;

        case State::WaitingToUpdateKeyTips:
            KEYTIPTRACE(L"GoToState WaitingToUpdateKeyTips");
            FAIL_FAST_ASSERT(oldState == State::Idle || oldState == State::WaitingForAnimationsToComplete);

            // Wait 1ms to let other event handlers run first, they might kick off an animation
            StartStateTimer(core, 1 /*timeout in ms*/);
            break;

        case State::UpdatingKeyTips:
            KEYTIPTRACE(L"GoToState UpdatingKeyTips");
            FAIL_FAST_ASSERT(oldState == State::WaitingToUpdateKeyTips);

            // Call us back on the UI thread, we'll do this work in Execute function
            IFCFAILFAST(core->ExecuteOnUIThread(this, ReentrancyBehavior::CrashOnReentrancy));
            break;

        case State::WaitingForAnimationsToComplete:
            KEYTIPTRACE(L"GoToState WaitingForAnimationsToComplete");
            FAIL_FAST_ASSERT(oldState == State::UpdatingKeyTips);

            // We'll wait for animtations to finish -- use a 1s timeout though.
            StartStateTimer(core, 1000 /*timeout in ms*/);
            break;

        case State::ForciblyUpdatingKeyTips:
            KEYTIPTRACE(L"GoToState ForciblyUpdatingKeyTips");
            FAIL_FAST_ASSERT(oldState == State::WaitingForAnimationsToComplete);

            // Call us back on the UI thread, we'll do this work in Execute function
            IFCFAILFAST(core->ExecuteOnUIThread(this, ReentrancyBehavior::CrashOnReentrancy));
            break;

        default:
            XAML_FAIL_FAST();
    }
    m_state = newState;
}

_Check_return_ HRESULT
VisualProperties::EnsureInitialized(_In_ CCoreServices* core)
{
    if (KeyTipBackground)
    {
        // already initialized
        return S_OK;
    }
    IFC_RETURN(core->LookupThemeResource(XSTRING_PTR_EPHEMERAL(L"KeyTipBackground"), KeyTipBackground.ReleaseAndGetAddressOf()));
    IFC_RETURN(core->LookupThemeResource(XSTRING_PTR_EPHEMERAL(L"KeyTipBorderBrush"), KeyTipBorderBrush.ReleaseAndGetAddressOf()));
    IFC_RETURN(core->LookupThemeResource(XSTRING_PTR_EPHEMERAL(L"KeyTipBorderThemeThickness"), KeyTipBorderThemeThickness.ReleaseAndGetAddressOf()));
    IFC_RETURN(core->LookupThemeResource(XSTRING_PTR_EPHEMERAL(L"KeyTipThemePadding"), KeyTipThemePadding.ReleaseAndGetAddressOf()));
    IFC_RETURN(core->LookupThemeResource(XSTRING_PTR_EPHEMERAL(L"KeyTipForeground"), KeyTipForeground.ReleaseAndGetAddressOf()));
    IFC_RETURN(core->LookupThemeResource(XSTRING_PTR_EPHEMERAL(L"KeyTipFontFamily"), KeyTipFontFamily.ReleaseAndGetAddressOf()));
    IFC_RETURN(core->LookupThemeResource(XSTRING_PTR_EPHEMERAL(L"KeyTipContentThemeFontSize"), KeyTipContentThemeFontSize.ReleaseAndGetAddressOf()));
    return S_OK;
}

// Create a popup and visual content to represent a KeyTip
_Check_return_ HRESULT
KeyTipManager::CreatePopup(
    _Inout_ KeyTip& keyTip,
    _Inout_ VisualProperties& visualProperties,
    _In_ unsigned int keysPressed)
{
    CCoreServices* core = keyTip.Object->GetContext();

    {
        CUIElement* element = do_pointer_cast<CUIElement>(keyTip.Object.get());
        CTextElement* textElement = do_pointer_cast<CTextElement>(keyTip.Object.get());
        FAIL_FAST_ASSERT(element || textElement);
        FAIL_FAST_ASSERT(!(element && textElement));

        // Determine placementMode, unless it has been 'inherited' during filtering.
        if(!keyTip.PlacementModeInitialized)
        {
            CValue keyTipPlacementModeValue = {};
            KnownPropertyIndex index = KnownPropertyIndex::UnknownType_UnknownProperty;

            if (element != nullptr) { index = KnownPropertyIndex::UIElement_KeyTipPlacementMode; }
            else if (textElement != nullptr) { index = KnownPropertyIndex::TextElement_KeyTipPlacementMode; }
            else { IFCFAILFAST(E_FAIL) }

            IFC_RETURN(keyTip.Object->GetValueByIndex(index, &keyTipPlacementModeValue));
            keyTip.PlacementMode = static_cast<DirectUI::KeyTipPlacementMode>(keyTipPlacementModeValue.AsEnum());
        }
        if (keyTip.PlacementMode == DirectUI::KeyTipPlacementMode::Hidden)
        {
            // Don't show a KeyTip, no need to make the popup
            return S_OK;
        }
    }

    const XRECTF_RB elementBounds = GetGlobalBoundsForKeyTips(keyTip.Object);

    if (elementBounds.right <= elementBounds.left || elementBounds.bottom <= elementBounds.top)
    {
        // The element is clipped out, don't show a KeyTip
        return S_OK;
    }

    IFC_RETURN(visualProperties.EnsureInitialized(core));

    xref_ptr<CBorder> border = CreateObject<CBorder>(core);
    IFC_RETURN(SetupBorder(border, visualProperties));

    xref_ptr<CTextBlock> textBlock = CreateObject<CTextBlock>(core);
    IFC_RETURN(SetupTextBlock(textBlock, visualProperties));

    auto visualTree = VisualTree::GetForElementNoRef(keyTip.Object);

    xref_ptr<CPopup> popup = CreateObject<CPopup>(core);
    popup->SetAssociatedVisualTree(visualTree); // we already have the right visual tree to attach popup to, attaching it here then

    // Peg the popup while in this function. SetupPopup() will create a DXaml peer via CPopup::SetIsWindowed, however since the popup is not yet parented
    // the peer it is not protected by SetExpectedReferenceOnPeer and gets destroyed once the DXaml callback completes.
    // Popup has stateful peers, so peg the peer to keep alive until popup child is set. That will in turn peg the peer until
    // the popup is closed (CPopup::SetValue(Popup_Child, foo) -> CPopup::Open -> CPopupRoot::AddToOpenPopupList).
    CorePeggedPtr<CPopup> peggedPopup;
    IFC_RETURN(peggedPopup.Set(popup));

    IFC_RETURN(SetupPopup(popup));

    IFC_RETURN(popup->SetValueByKnownIndex(KnownPropertyIndex::Popup_Child, border.get()));
    IFC_RETURN(border->SetValueByKnownIndex(KnownPropertyIndex::Border_Child, textBlock.get()));

    IFC_RETURN(SetText(textBlock, keyTip.Object, keysPressed));

    //Getting information relating to keytip size and position
    IFC_RETURN(SetPosition(border, keyTip.Object, &keyTip));

    keyTip.ObjectBounds = elementBounds;
    keyTip.Popup = popup;
    keyTip.State = KeyTip::Normal;

    return S_OK;
}

/*static*/ _Check_return_ HRESULT
KeyTipManager::OnStateTimerFiredStatic(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs*)
{
    KEYTIPTRACE(L"OnStateTimerFiredStatic");

    CCoreServices* core = pSender->GetContext();
    CInputServices* inputServices = core->GetInputServices();

    if (inputServices)
    {
        inputServices->GetKeyTipManager().OnStateTimerFired(core);
    }
    return S_OK;
}

void KeyTipManager::OnStateTimerFired(_In_ CCoreServices* core)
{
    if (!m_stateTimer)
    {
        // Someone cancelled the timer, we must not need this work anymore.
        return;
    }

    StopStateTimer(); // We only ever use the state timer for one tick at a time
    switch (m_state)
    {
        case State::Idle:
            break;
        case State::WaitingForAnimationsToComplete:
            GoToState(core, State::ForciblyUpdatingKeyTips);
            break;
        case State::WaitingToUpdateKeyTips:
            GoToState(core, State::UpdatingKeyTips);
            break;
        default:
            XAML_FAIL_FAST();
    }
}

// Called by core every frame to notify us about animations.  Avoid doing a lot of work here.
void KeyTipManager::NotifyFiniteAnimationIsRunning(_In_ CCoreServices* core, bool finiteAnimationIsRunning)
{
    if (m_finiteAnimationIsRunning != finiteAnimationIsRunning)
    {
        m_finiteAnimationIsRunning = finiteAnimationIsRunning;
        KEYTIPTRACE(L"m_finiteAnimationIsRunning is now %s", m_finiteAnimationIsRunning ? L"true" : L"false");

        if (!m_finiteAnimationIsRunning && m_state == State::WaitingForAnimationsToComplete)
        {
            GoToState(core, State::ForciblyUpdatingKeyTips);
        }
    }
}
