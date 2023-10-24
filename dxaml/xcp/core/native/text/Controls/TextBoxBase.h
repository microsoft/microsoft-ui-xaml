// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WinUIEdit.h>
#include "Indexes.g.h"
#include <fwd/windows.ui.text.h>
#include <fwd/windows.globalization.h>
#include <fwd/windows.ui.viewmanagement.h>

class CKeyEventArgs;
class CPointerEventArgs;
class CInputPointEventArgs;

class CInputScope;
class CTextBoxView;
class ITextServices2;
class CInputScope;
class TextServicesHost;
class RichEditOleCallback;
class CTextInputPrivateSettings;
class TextBoxUIManagerEventSink;

struct ITfSource;
struct ITextDocument;
struct ITextSelection2;
struct ITextMarkContainer;
struct RE_MOUSEINPUT;

typedef HRESULT (*ShutdownTextServicesFunction)(_In_ IUnknown *pTextServices);

//---------------------------------------------------------------------------
//
//  Base class for editable DirectUI text controls.  This class is not
//  exposed publicly but consolidates implementation across public derived
//  classes.
//
//  This class builds off of Window's ITextServices, the windowless RichEdit
//  control.
//
//---------------------------------------------------------------------------
class CTextBoxBase : public CControl
{
    friend class CTextBoxView;
    friend class TextServicesHost;
    friend class CTextBoxBaseAutomationPeer;

public:

    enum class TextCompositionStage
    {
        CompositionStarted = 0,
        CompositionChanged,
        CompositionEnded,
    };

    class ScopedSuppressChangeNotifications final
    {
        friend class CTextBoxBase;

    public:
        ScopedSuppressChangeNotifications(const ScopedSuppressChangeNotifications&) = delete;
        ScopedSuppressChangeNotifications(ScopedSuppressChangeNotifications&& move);

        ~ScopedSuppressChangeNotifications();

        ScopedSuppressChangeNotifications& operator=(const ScopedSuppressChangeNotifications&) = delete;
        ScopedSuppressChangeNotifications& operator=(ScopedSuppressChangeNotifications&& move);

    private:
        ScopedSuppressChangeNotifications(_In_ CTextBoxBase* textBoxBase, _In_ ITextDocument2* document);
        void Reset();

        CTextBoxBase* m_textBoxBase;
        Microsoft::WRL::ComPtr<ITextDocument2> m_document;
    };

    // CDependencyObject overrides.
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTextBoxBase>::Index;
    }

    _Check_return_ HRESULT GetValue(
        _In_  const CDependencyProperty *pdp,
        _Out_ CValue *pValue
        ) final;

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    // CFrameworkElement overrides.
    _Check_return_ HRESULT OnApplyTemplate() override;

    // ITextHost2
    void TxSetCapture(_In_ BOOL takeCapture);
    void TxSetFocus();

    _Check_return_ HRESULT TextBoxToScreen(_Inout_ XPOINT* point);
    _Check_return_ HRESULT TextBoxToClient(_Inout_ XPOINT* point, bool applyRasterizationScale = false);
    _Check_return_ HRESULT ScreenToTextBox(_Inout_ XPOINT* point, bool roundUp = false);
    _Check_return_ HRESULT ClientToTextBox(_Inout_ XPOINTF* point);

    virtual _Check_return_ HRESULT TxGetMaxLength(_Out_ XUINT32 *pLength);
    virtual _Check_return_ HRESULT TxGetPasswordChar(_Out_ WCHAR *pChar);
    _Check_return_ HRESULT TxGetPropertyBits(_In_ XUINT32 mask, _Out_ XUINT32 *pFlags);
    virtual _Check_return_ HRESULT TxNotify(_In_ UINT32 notification, _In_ void *pData);
    _Check_return_ HRESULT TxGetSelectionBarWidth(_Out_ XINT32 *pSelectionBarWidth);

    _Check_return_ HRESULT GetVisibleRect(XRECT_RB* prc);
    _Check_return_ HRESULT EnsureRectVisible(_In_ const RECT& rect);
    _Check_return_ HRESULT EnsureRectVisibleWithPadding(_In_ const RECT& rect);
    _Check_return_ HRESULT BringLastVisibleRectIntoView(bool scrollTextBoxIntoView);
    _Check_return_ HRESULT RaisePendingBringLastVisibleRectIntoView(_In_ bool forceIntoView, _In_ bool focusChanged);
    CTextBoxView *GetView() const;

    virtual DirectUI::TextWrapping GetTextWrapping() const;
    virtual _Check_return_ HRESULT GetAlignment(_Out_ DirectUI::TextAlignment *pAlignment);

    // Property values, overridden by derived classes.
    virtual bool IsReadOnly() const;
    virtual bool IsPassword() const;
    virtual bool AcceptsReturn() const; // TODO: make this protected again once RichEdit GetNaturalSize EMU bug 152434 is fixed.
    virtual bool IsPasswordRevealed() const;

    _Check_return_ HRESULT Cut();
    _Check_return_ HRESULT Copy();
    _Check_return_ HRESULT Undo();
    _Check_return_ HRESULT Redo();
    _Check_return_ HRESULT SelectAll();
    _Check_return_ HRESULT Paste();
    _Check_return_ HRESULT ClearUndoRedoHistory();

    bool CanPasteClipboardContent() const;
    bool CanUndo() const;
    bool CanRedo() const;

    // Raises the event and then returns whether or not it was handled by the app.
    bool RaiseCutEventAndCheckIfHandled();
    bool RaiseCopyEventAndCheckIfHandled();
    bool RaisePasteEventAndCheckIfHandled();

    void InvalidateView();
    void InvalidateViewAndForceRedraw();

    _Check_return_ HRESULT GetBaselineOffset(_Out_ XFLOAT *pBaselineOffset); // TODO: not referenced, remove or update Text_GetBaselineOffset.

    // ITextDocument helpers.
    _Check_return_ HRESULT GetDocument(_Outptr_ ITextDocument2 **ppDocument) const;
    _Check_return_ HRESULT GetSelection(_Outptr_ ITextSelection2 **ppSelection);

    _Check_return_ HRESULT GetRichEditRawElementProviderSimple(_Out_ void** ppProvider);

    _Check_return_ HRESULT GetUnwrappedPattern(_In_ XINT32 patternID, _In_ bool isRichEdit, _Out_ void** ppProvider);

    virtual bool IsEmpty() = 0;

    virtual _Check_return_ HRESULT GetInputScope(_Out_ ::InputScopeNameValue *inputScope) = 0;

    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;

    void SetHandleRightTappedEvent(_In_ bool handleRightTappedEvent);

    void SetLastMessage(_In_ XUINT32 lastMessage);

    _Check_return_ HRESULT SendRichEditFocus();

    ITextServices2* GetTextServices() const;

    bool ShouldRaiseEvent(_In_ EventHandle hEvent, _In_ bool fInputEvent = false, _In_opt_ CEventArgs *pArgs = NULL) override
    {
        // TODO: Only return TRUE for the events we implicitly handle (OnGotFocus, etc.).
        return true;
    }

    void EnableEnsureRectVisible();
    void DisableEnsureRectVisible();

    bool HasSelection();
    bool IsHandlingSelectionChangingEvent() const { return m_handlingSelectionChangingEvent; }

    void IncomingFocusFromGamepad()
    {
        m_canEnableManualInputPane = true;
    }

    _Check_return_ HRESULT NotifyEditFocusLost();
    _Check_return_ HRESULT NotifyEditControlInputPaneHiding();
    _Check_return_ HRESULT ForceEditFocusLoss();
    _Check_return_ HRESULT ForceFocusLoss();
    _Check_return_ HRESULT DismissAllFlyouts();

    _Check_return_ HRESULT SetGripperRects(XRECT_WH rcBegin, XRECT_WH rcEnd);
    _Check_return_ HRESULT SetGripperBeingManipulated(_In_ bool isBeingManipulated);
    _Check_return_ HRESULT OnGripperTapped(_In_ CEventArgs* pEventArgs, bool doubleTap = false);
    _Check_return_ HRESULT OnGripperDoubleTapped(_In_ CEventArgs* pEventArgs);
    _Check_return_ HRESULT OnGripperRightTapped(_In_ CEventArgs* pEventArgs);
    _Check_return_ HRESULT OnGripperPressedWithBarrelButtonDown(_In_ CEventArgs* pEventArgs);
    DirectUI::TextReadingOrder GetTextReadingOrder() { return m_textReadingOrder; }

    _Check_return_ HRESULT EnableCandidateWindowBoundsTracking(EventHandle event);
    _Check_return_ HRESULT OnInputLanguageChange(_In_ HKL inputLanguage);
    _Check_return_ HRESULT OnWindowMoved();
     _Check_return_ HRESULT UpdateFocusState(_In_ DirectUI::FocusState focusState) override;
     virtual _Check_return_ HRESULT GetSpellCheckEnabled(_Out_ bool *pIsSpellCheckEnabled, _Out_ bool *pIsDefault) const;
     virtual _Check_return_ HRESULT GetTextPredictionEnabled(_Out_ bool *pIsTextPredictionEnabled, _Out_ bool *pIsDefault) const;

    _Check_return_ HRESULT GetCompositionString(_Outptr_ HSTRING* composition);
    _Check_return_ HRESULT GetCompositionPrefixAndPostfixStrings(_Outptr_ HSTRING* prefix, _Outptr_ HSTRING* postfix);

    _Check_return_ HRESULT RaiseCandidateWindowBoundsChangedEventForRoot(_In_ const RECT* rect);
    _Check_return_ HRESULT RaiseCandidateWindowBoundsChangedEventForDIPs(_In_ const RECT* rect);
    _Check_return_ HRESULT RaiseCandidateWindowBoundsChangedEvent(_In_ XRECTF &xRect);
    _Check_return_ HRESULT OnGripperHeld(_In_ CEventArgs* pEventArgs);

    void GetActualSize(_Out_ float& actualWidth, _Out_ float& actualHeight);
    void GetOriginalSize(_Out_ float& actualWidth, _Out_ float& actualHeight);

    ScopedSuppressChangeNotifications SuppressChangeNotifications();

    // This function is temporary and should be removed in 19H1 in favor of always using windowed popups to host the EIC.
    bool IsInXamlIsland();

    bool HasContextFlyout() const;
    bool IsProofingMenuValid() const { return m_proofingMenuIsValid; }
    _Check_return_ HRESULT FireContextMenuOpeningEventSynchronously(_Out_ bool& handled, const wf::Point& point);

    static _Check_return_ HRESULT GetProofingMenuFlyout(
        _In_ CDependencyObject* pObject,
        _In_ uint32_t cArgs,
        _Inout_updates_(cArgs) CValue* ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue* pResult);

    _Check_return_ HRESULT GetProofingMenuFlyoutNoRef(_Out_ CFlyoutBase*& proofingMenu);
    _Check_return_ HRESULT UpdateRichEditContextMenu(bool useLegacyContextMenu);
    bool UseLegacyContextMenu() { return m_useLegacyContextMenu; }

    _Check_return_ HRESULT GetContextMenuShowPosition(_Out_ XPOINTF *position);
    _Check_return_ HRESULT GetSelectionBoundingRect(_Out_ wf::Rect *selectionBoundingRect);

    bool ShouldUseVisualPixels();
    _Check_return_ HRESULT InjectCharaterReceivedTSF(_In_ CEventArgs* pEventArgs);

protected:
    CTextBoxBase(_In_ CCoreServices *pCore);

    ~CTextBoxBase() override;

    virtual _Check_return_ HRESULT Initialize();
    void Destroy();

    // CUIElement overrides.
    _Check_return_ HRESULT OnPointerEntered(_In_ CEventArgs* pEventArgs) final;
    _Check_return_ HRESULT OnPointerExited(_In_ CEventArgs* pEventArgs) final;
    _Check_return_ HRESULT OnPointerPressed(_In_ CEventArgs* pEventArgs) final;
    _Check_return_ HRESULT OnPointerMoved(_In_ CEventArgs* pEventArgs) final;
    _Check_return_ HRESULT OnPointerReleased(_In_ CEventArgs* pEventArgs) final;
    _Check_return_ HRESULT OnPointerCaptureLost(_In_ CEventArgs* pEventArgs) final;
    _Check_return_ HRESULT OnDoubleTapped(_In_ CEventArgs* pEventArgs) final;
    _Check_return_ HRESULT OnHolding(_In_ CEventArgs* pEventArgs) final;
    _Check_return_ HRESULT OnKeyUp(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnKeyDown(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnGotFocus(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnLostFocus(_In_ CEventArgs* pEventArgs) final;
    _Check_return_ HRESULT OnIsEnabledChanged(_In_ CEventArgs* pEventArgs) final;
    _Check_return_ HRESULT OnCharacterReceived(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnInheritedPropertyChanged(_In_ CEventArgs* pEventArgs) final;
    _Check_return_ HRESULT OnRightTapped(_In_ CEventArgs* pEventArgs) final;
    _Check_return_ HRESULT OnTapped(_In_ CEventArgs* pEventArgs) final;
    _Check_return_ HRESULT ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize) override;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) override;

    _Check_return_ HRESULT NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh = false) override;

    void EnableSetValueReentrancyGuard();
    void ClearSetValueReentrancyGuard();
    bool IsInSetValue() const;

    _Check_return_ HRESULT CoercePropertyValue(
        _In_ const CDependencyProperty* pProperty,
        _Inout_ xstring_ptr& strSource,
        _Inout_ xstring_ptr& strTarget);

    _Check_return_ HRESULT CoercePropertyValue(
        _In_ const CDependencyProperty* pProperty,
        _In_ XINT32* pSource,
        _In_ XINT32 target);

    // Notifications.
    virtual _Check_return_ HRESULT OnContentChanged(_In_ const bool fTextChanged) = 0;
    virtual _Check_return_ HRESULT OnSelectionChanged();
    _Check_return_ HRESULT ProcessSelectionChangingEvent(_Inout_ long& selectionStart, _Inout_ long& selectionLength, _Out_ BOOLEAN& SelectionChangingCanceled);

    // State or behavior getters, overridden by derived classes.
    virtual bool AcceptsRichText() const;

    // Property change handlers.
    virtual _Check_return_ HRESULT OnAcceptsReturnChanged();
    virtual _Check_return_ HRESULT OnTextWrappingChanged();
    _Check_return_ HRESULT OnIsReadOnlyChanged();
    _Check_return_ HRESULT OnTextAlignmentChanged();
    _Check_return_ HRESULT OnBidiOptionsChanged();
    _Check_return_ HRESULT OnMaxLengthChanged();
    _Check_return_ HRESULT OnIsSpellCheckEnabledChanged(_Inout_ bool& isSpellCheckEnabled);
    _Check_return_ HRESULT OnIsTextPredictionEnabledChanged(_Inout_ bool& isTextPredictionEnabled);
    _Check_return_ HRESULT OnDesiredCandidateWindowAlignmentChanged(_In_ DirectUI::CandidateWindowAlignment alignment);
    _Check_return_ HRESULT OnInputScopeChanged(_In_ CInputScope *pInputScope);
    void OnSelectionHighlightColorChanged();
    _Check_return_ HRESULT OnSelectionHighlightColorWhenNotFocusedChanged();
    _Check_return_ HRESULT OnPasswordRevealedChanged(_In_ bool passwordRevealed);

    virtual _Check_return_ HRESULT UpdateVisualState();

    static bool IsPositiveInteger(_In_ const CValue& value);
    static bool IsWrapWholeWords(_In_ const CValue& value);

    _Check_return_ HRESULT GetTextServicesBuffer(_Out_ xstring_ptr* pstrText);
    virtual _Check_return_ HRESULT SetTextServicesBuffer(_In_ const xstring_ptr& strText);
    virtual KnownEventIndex GetPastePropertyID() const = 0;
    virtual KnownEventIndex GetCopyPropertyID() const { return KnownEventIndex::UnknownType_UnknownEvent;  };
    virtual KnownEventIndex GetCutPropertyID() const { return KnownEventIndex::UnknownType_UnknownEvent; };
    virtual KnownPropertyIndex GetSelectionHighlightColorPropertyID() const = 0;
    virtual KnownPropertyIndex GetSelectionHighlightColorWhenNotFocusedPropertyID() const = 0;
    virtual KnownPropertyIndex GetSelectionFlyoutPropertyID() const = 0;
#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
    virtual KnownPropertyIndex GetHeaderPlacementPropertyID() const = 0;
#endif

    CSolidColorBrush* GetSelectionHighlightColorNoRef();
#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
    DirectUI::ControlHeaderPlacement GetHeaderPlacement() const;
#endif

private:
    _Check_return_ HRESULT CreateSelectionHighlightColor();
    _Check_return_ HRESULT CreateSelectionHighlightColorWhenNotFocused();
    _Check_return_ HRESULT HideSelection(_In_ bool hideSelection);

protected:
    virtual KnownEventIndex GetTextCompositionEventPropertyID(TextCompositionStage stage) const;
    virtual _Check_return_ HRESULT UpdateTextForCompositionStartedEvent();

    bool ShouldGenerateLinguisticAlternatives();

    void ResetLinguisticAlternativeState();

    _Check_return_ HRESULT ForceRemoveFocus();

    _Check_return_ HRESULT InitializeView();
    _Check_return_ HRESULT RemovePeerReferenceToView();
    _Check_return_ HRESULT AddPeerReferenceToView();

    _Check_return_ HRESULT SetLanguageOption(_In_ XUINT32 option, _In_ bool value);
    _Check_return_ HRESULT SetInputPaneDisplayPolicyForTSF1(_In_ bool isManualDisplayPolic);
    _Check_return_ HRESULT EnsureDocument();
    void ReleaseDocumentIfNotFocused();
    _Check_return_ HRESULT AttachToHost(_In_ CFrameworkElement* pElement);
    _Check_return_ HRESULT DetachFromHost();

    bool IsSecondaryInputScopeDefault() const;

    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *namescopeOwner, EnterParams params) override;

private:
    _Check_return_ HRESULT InitializeTextServices();

    _Check_return_ HRESULT SendPointerMessage(
        _In_ XUINT32 message,
        _In_ CEventArgs* pEventArgs
        );

    XUINT32 PointerMessageToMouseMessage(_In_ XUINT32 pointerMessage, _In_ CPointer* pPointer);

    // Converts a CInputPointEventArgs into wparam/lparam for ITextService messages.
    _Check_return_ HRESULT PackageMouseEventParams(
        _In_ XUINT32 mouseMessage,
        _In_ CInputPointEventArgs *pInputPointEventArgs,
        _In_ DirectUI::VirtualKeyModifiers keyModifiers,
        _In_ DirectUI::PointerDeviceType pointerDeviceType,
        _Out_ RE_MOUSEINPUT *pReMouseInput
        ) const;

    _Check_return_ HRESULT SendMouseInput(
        _In_ XUINT32 message,
        _In_ CInputPointEventArgs *pInputPointEventArgs);

    // Sends a keyboard message to ITextServices.
    _Check_return_ HRESULT SendKeyMessage(
        _In_ XUINT32 message,
        _In_ CEventArgs* pEventArgs
        );

    _Check_return_ HRESULT EnsureWindowlessHost();

    bool FocusOnPointerReleased(_In_ CPointerEventArgs* pPointerEventArgs);

    bool UseMultiLineMode() const;

    bool ShouldProcessKeyMessage(_In_ CEventArgs* pEventArgs);

    _Check_return_ HRESULT IsFrozen(_Out_ bool& isFrozen);

    _Check_return_ HRESULT UpdateLastSelectedTextElement();

    _Check_return_ HRESULT IsAutoGrowing(_Out_ bool& isAutoGrowing);

    _Check_return_ HRESULT SetFocusFromPointer(_In_ CPointerEventArgs *pPointerEventArgs, _Out_ bool& sendMessage);

    _Check_return_ HRESULT SendTextCompositionEvent(TextCompositionStage compositionStage);

    _Check_return_ HRESULT InitializeTextServiceManager();

    _Check_return_ HRESULT OnCurrentInputLanguageChanged(_In_ wut::Core::ICoreTextServicesManager *, _In_ IInspectable * pArgs);

    _Check_return_ HRESULT UpdateKeyboardLCID(LCID lcid);

    _Check_return_ HRESULT UpdateCaretParagraphDirection();

    _Check_return_ HRESULT SetParagraphDirectionIfEmpty(_In_ long position);

    _Check_return_ HRESULT UpdateSIPSettings(_In_ DirectUI::FocusState state);

    _Check_return_ HRESULT EnsureTextInputSettings();

    bool ShouldFowardMoveMessage(_In_ const CPointerEventArgs* pPointerEventArgs) const;

    static _Check_return_ HRESULT OnShowGrippersTimeout(
        _In_ CDependencyObject *pSender,
        _In_ CEventArgs* pEventArgs
    );

    _Check_return_ HRESULT OnShowGrippersTimeout();

    _Check_return_ HRESULT PostAsyncEnsureRectVisibleCall();

    static _Check_return_ HRESULT OnDelayEnsureRectVisible(
        _In_ CDependencyObject *pSender,
        _In_ CEventArgs* pEventArgs
    );

    _Check_return_ HRESULT OnDelayEnsureRectVisible();


    _Check_return_ HRESULT SendGripperHostTapMessage(_In_ CPointerEventArgs* pPointerEventArgs, int tapCount = 1);

    bool ShouldHideGrippersOnFlyoutOpening();
    bool ShouldForceFocusedVisualState();
    bool HasSelectionFlyout() const;

    CFlyoutBase* GetSelectionFlyoutNoRef() const;
    _Check_return_ HRESULT QueueUpdateSelectionFlyoutVisibility();

    _Check_return_ HRESULT ForceNotifyFocusEnterIfNeeded();

public:
    _Check_return_ HRESULT UpdateSelectionFlyoutVisibility();

private:
    _Check_return_ HRESULT GetSelectionLength(_Out_ uint32_t& selectionLength);

    // Proofing menu helpers
    _Check_return_ HRESULT EnsureProofingMenu();
    _Check_return_ HRESULT UpdateProofingMenu();
    _Check_return_ HRESULT GetSpellingInfoForCP(_Out_ GETCONTEXTMENUEX& gcmex);
    _Check_return_ HRESULT AddSpellingSuggestions(const GETCONTEXTMENUEX& gcmex, _Out_ CMenuFlyoutItemBaseCollection*& items);
    _Check_return_ HRESULT CreateProofingMenuItem(_In_ INTERNAL_EVENT_HANDLER hEvent, const xstring_ptr& itemText, _Out_ CMenuFlyoutItemBaseCollection*& items);

    // Start of class members
private:
    xref_ptr<CMenuFlyout> m_proofingMenu;
    EventRegistrationToken m_inputLanguageChangedToken = { };
    xref_ptr<CDispatcherTimer> m_spEnsureRectVisibleTimer;

public:
    CSolidColorBrush* m_pSelectionHighlightColor = nullptr;
    CSolidColorBrush* m_pSelectionHighlightColorWhenNotFocused = nullptr;
    CInputScope* m_pInputScope = nullptr;

    DirectUI::TextWrapping m_textWrapping = DirectUI::TextWrapping::NoWrap;
    DirectUI::TextAlignment m_textAlignment = DirectUI::TextAlignment::DetectFromContent;
    DirectUI::TextReadingOrder m_textReadingOrder = DirectUI::TextReadingOrder::DetectFromContent;

    XINT32 m_iMaxLength = 0;
    XINT32 m_iSelectionStart = 0;
    XINT32 m_iSelectionLength = 0;

protected:
    XUINT32 m_lastMessage = WM_NULL;
    XRECT_RB m_requestResize = { };

private:
    // SetValue reentrancy guard, incremented inside each call to SetValue.
    uint32_t m_setValueReentrancyCount = 0;

protected:
    // The RichEdit backing store and editor.
    ITextServices2* m_pTextServices = nullptr;
    Microsoft::WRL::ComPtr<ITextDocument2> m_spDocument;

    // The UIElement responsible for measuring and rendering content.
    CTextBoxView* m_pView = nullptr;

private:
    // Function pointer to the text services shutdown method.
    ShutdownTextServicesFunction m_pFnShutdownTextServices = nullptr;

    // The primary Jupiter ITextServices host implementation.
    TextServicesHost* m_pHost = nullptr;

    // Handler for context menu callbacks.
    RichEditOleCallback* m_pRichEditOleCallback = nullptr;

    // Host for RichEdit's UI Accessibility exports.
    IPALWindowlessHost* m_pWindowlessHost = nullptr;

    xref::weakref_ptr<CFrameworkElement> m_pHostRef;
    CPointer* m_pLastPointer = nullptr;

    XINT32 m_cpLastSelBegin = -1;
    XINT32 m_cpLastSelEnd = -1;
    XRECTF m_rectVisibleRegion = { -1, -1, -1, -1 };

    // Bring into view can be delayed for height animation in progress.
    // For example: new line created in a textbox, both size change and caret
    // position update will happen. Bringing caret into view will be delayed, so
    // save the state (RECT, pending state and padding requirement) in class members
    RECT m_rectLastVisibleRectInView = { -1, -1, -1, -1 };

    XRECT_WH m_rectBeginGripperLast = { -1, -1, -1, -1 };
    XRECT_WH m_rectEndGripperLast = { -1, -1, -1, -1 };
    XRECT_WH m_rectBeginGripperManipStart = { -1, -1, -1, -1 };
    XRECT_WH m_rectEndGripperManipStart = { -1, -1, -1, -1 };

    Microsoft::WRL::ComPtr<TextBoxUIManagerEventSink> m_uiManagerEventSink;
    Microsoft::WRL::ComPtr<ITfSource> m_tfSource;
    DWORD m_uiManagerSinkCookie = 0;
    LCID m_lcid;

    // Phone, OneCore: connects private TSF3 edit settings for updating TextPrediction and Spell Check properties
    CTextInputPrivateSettings* m_pPrivateTextInputSettings = nullptr;

    // Text Service manager for input language change notification
    Microsoft::WRL::ComPtr<wut::Core::ICoreTextServicesManager> m_spTextServiceManager;

    INT m_compositionStartIndex = -1;
    INT m_compositionLength = -1;

    xref_ptr<CDispatcherTimer> m_spShowGrippersTimer;
    EventHandle m_CandidateWindowevent;

    int  m_compositionEventReentrancyCount = 0;

    unsigned int m_suppressChangeNotificationsRequests = 0;

    DirectUI::InputDeviceType m_lastInputDeviceType{ DirectUI::InputDeviceType::None };
    XPOINTF m_lastPointerPosition{};

public:
    bool m_bIsReadOnly = false;
    bool m_bAcceptsReturn = false;
    bool m_isColorFontEnabled = true;
    bool m_ensureRectVisibleEnabled = true;
    bool m_bPreventKeyboardDisplayOnProgrammaticFocus = false;
    bool m_isSpellCheckEnabled = true;
    bool m_isTextPredictionEnabled = true;
    bool m_preventEditFocusLoss = false;
    bool m_bIsTelemetryCollectionEnabled = true;

private:
    // ********************************************************************
    // Bitfield group 1 (8 bits)
    // ********************************************************************
    bool m_handleRightTappedEvent               : 1;
    bool m_barrelButtonPressed                  : 1;
    bool m_rightButtonPressed                   : 1;
    bool m_middleButtonPressed                  : 1;
    bool m_leftButtonPressed                    : 1;
    bool m_isPointerOver                        : 1;
    bool m_isKeyboardRTL                        : 1;
    bool m_canEnableManualInputPane             : 1;

    // ********************************************************************
    // Bitfield group 2 (8 bits)
    // ********************************************************************
    bool m_ignoreGripperTouchEvents             : 1;
    bool m_ignoreKillFocus                      : 1;
    bool m_isViewRegisteredForGripperUpdates    : 1;
    bool m_ignoreScrollIntoView                 : 1;
    bool m_viewHasPeerRef                       : 1;
    bool m_gripperBeingManipulated              : 1;
    bool m_manualInputPaneEnabled               : 1;
    bool m_gamepadEngaged                       : 1;

    // ********************************************************************
    // Bitfield group 3 (8 bits)
    // ********************************************************************
    bool m_hideSelection                        : 1;
    bool m_shouldScrollHeaderIntoView           : 1;
    bool m_previouslyGeneratedAlternatives      : 1;
    bool m_firedCandidateWindowEventAfterFocus  : 1;
    bool m_forceNotifyFocusEnter                : 1;

    // Bring into view can be delayed for height animation in progress.
    // For example: new line created in a textbox, both size change and caret
    // position update will happen. Bringing caret into view will be delayed, so
    // save the state (RECT, pending state and padding requirement) in the class members below
    bool m_ensureRectVisibleWithPadding         : 1;
    bool m_ensureRectVisiblePending             : 1;

protected:
    bool m_textPredictionIsDefault              : 1;
    // ********************************************************************
    // Bitfield group 4 (8 bits)
    // ********************************************************************
    bool m_compositionInProgress                : 1;
    bool m_spellCheckIsDefault                  : 1;
    bool m_handlingSelectionChangingEvent       : 1;
    bool m_forceFocusedVisualState              : 1;
    bool m_proofingMenuIsValid                  : 1;
    bool m_updatingExtent                       : 1;
    bool m_useLegacyContextMenu                 : 1;
    bool m_isSelectionFlyoutUpdateQueued        : 1;
    // ********************************************************************
    // Bitfield group 5 (8 bits)
    // ********************************************************************
    bool m_shouldHideGrippersOnFlyoutOpening    : 1;
};

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the RichEdit backing store and editor.
//
//---------------------------------------------------------------------------
inline
ITextServices2 *CTextBoxBase::GetTextServices() const
{
    return m_pTextServices;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the UIElement responsible for measuring and rendering content.
//
//---------------------------------------------------------------------------
inline
CTextBoxView *CTextBoxBase::GetView() const
{
    return m_pView;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the current TextWrappingProperty value.
//
//---------------------------------------------------------------------------
inline
DirectUI::TextWrapping CTextBoxBase::GetTextWrapping() const
{
    return m_textWrapping;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Sets a flag to indicate whether TextBox should mark the RightTapped
//      event as handled or not.
//
//---------------------------------------------------------------------------
inline
void CTextBoxBase::SetHandleRightTappedEvent(_In_ bool handleRightTappedEvent)
{
    m_handleRightTappedEvent = handleRightTappedEvent;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      CTextBoxBase is hidden from the public, so the stock do_pointer_cast
//      in core.h won't work, as they rely on OfTypeByIndex. We provide our
//      own version here.
//
//---------------------------------------------------------------------------

template<>
constexpr CTextBoxBase* do_pointer_cast<CTextBoxBase>(_In_opt_ CDependencyObject* pDO)
{
    if (pDO &&
        (pDO->OfTypeByIndex<KnownTypeIndex::TextBox>() ||
         pDO->OfTypeByIndex<KnownTypeIndex::PasswordBox>() ||
         pDO->OfTypeByIndex<KnownTypeIndex::RichEditBox>()))
    {
        return static_cast<CTextBoxBase*>(pDO);
    }
    else
    {
        return nullptr;
    }
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the last message sent to RichEdit.
//
//---------------------------------------------------------------------------
inline
void CTextBoxBase::SetLastMessage(_In_ XUINT32 lastMessage)
{
    m_lastMessage = lastMessage;
}
