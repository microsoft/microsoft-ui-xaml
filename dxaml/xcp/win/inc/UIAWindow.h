// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Various identifiers that have to be looked up.
#pragma once

#include "UIAStructs.h"

#include "UIAHostEnvironmentInfo.h"
#include <map>
#include <fwd/windows.foundation.h>

#ifndef UIA_TRACE
#if defined(DBG) && defined(TRACE_UIA)
#define UIA_TRACE(...) \
        { \
            GetPALDebuggingServices()->XcpTrace(MonitorRaw, __WFILE__,__LINE__, 0, NULL,WIDEN_VA(,#__VA_ARGS__),##__VA_ARGS__); \
        }
#else
#define UIA_TRACE(...)
#endif
#endif

class CAutomationPeerCollection; // Forward declaration for ConvertToVariant(CAutomationPeerCollection, ...)
class CAutomationPeerAnnotationCollection;

DEFINE_GUID(GUID_PreventKeyboardDisplayOnProgrammaticFocus, 0xd48814c, 0x1e01, 0x49b1, 0x91, 0xe8, 0x61, 0xab, 0x2f, 0x24, 0x9f, 0x4f);

struct UIAIdentifiers
{
    PROPERTYID IsControlElement_Property;
    PROPERTYID ControlType_Property;
    PROPERTYID IsContentElement_Property;
    PROPERTYID LabeledBy_Property;
    PROPERTYID AutomationId_Property;
    PROPERTYID ItemType_Property;
    PROPERTYID IsPassword_Property;
    PROPERTYID LocalizedControlType_Property;
    PROPERTYID Name_Property;
    PROPERTYID AcceleratorKey_Property;
    PROPERTYID AccessKey_Property;
    PROPERTYID HasKeyboardFocus_Property;
    PROPERTYID IsKeyboardFocusable_Property;
    PROPERTYID IsEnabled_Property;
    PROPERTYID BoundingRectangle_Property;
    PROPERTYID ProcessId_Property;
    PROPERTYID RuntimeId_Property;
    PROPERTYID ClassName_Property;
    PROPERTYID HelpText_Property;
    PROPERTYID ClickablePoint_Property;
    PROPERTYID Culture_Property;
    PROPERTYID IsOffscreen_Property;
    PROPERTYID Orientation_Property;
    PROPERTYID FrameworkId_Property;
    PROPERTYID IsRequiredForForm_Property;
    PROPERTYID ItemStatus_Property;
    PROPERTYID LiveSetting_Property;
    PROPERTYID ControlledPeers_Property;                    // Was UIA_ControllerForPropertyId name improved for WinRT
    PROPERTYID FlowsFrom_Property;
    PROPERTYID FlowsTo_Property;
    PROPERTYID PositionInSet_Property;
    PROPERTYID SizeOfSet_Property;
    PROPERTYID Level_Property;
    PROPERTYID AnnotationTypes_Property;
    PROPERTYID AnnotationObjects_Property;
    PROPERTYID LandmarkType_Property;
    PROPERTYID LocalizedLandmarkType_Property;
    PROPERTYID IsPeripheral_Property;
    PROPERTYID IsDataValidForForm_Property;
    PROPERTYID FullDescription_Property;
    PROPERTYID DescribedBy_Property;
    PROPERTYID HeadingLevel_Property;
    PROPERTYID IsDialog_Property;

    // Pattern properties start here
    PROPERTYID Annotation_AnnotationTypeId_Property;
    PROPERTYID Annotation_AnnotationTypeName_Property;
    PROPERTYID Annotation_Author_Property;
    PROPERTYID Annotation_DateTime_Property;
    PROPERTYID Annotation_Target_Property;
    PROPERTYID Dock_DockPosition_Property;
    PROPERTYID Drag_DropEffect_Property;
    PROPERTYID Drag_DropEffects_Property;
    PROPERTYID Drag_GrabbedItems_Property;
    PROPERTYID Drag_IsGrabbed_Property;
    PROPERTYID DropTarget_DropTargetEffect_Property;
    PROPERTYID DropTarget_DropTargetEffects_Property;
    PROPERTYID ExpandCollapse_ExpandCollapseState_Property;
    PROPERTYID GridItem_Column_Property;
    PROPERTYID GridItem_ColumnSpan_Property;
    PROPERTYID GridItem_Parent_Property;
    PROPERTYID GridItem_Row_Property;
    PROPERTYID GridItem_RowSpan_Property;
    PROPERTYID Grid_ColumnCount_Property;
    PROPERTYID Grid_RowCount_Property;
    PROPERTYID MultipleView_CurrentView_Property;
    PROPERTYID MultipleView_SupportedViews_Property;
    PROPERTYID RangeValue_IsReadOnly_Property;
    PROPERTYID RangeValue_LargeChange_Property;
    PROPERTYID RangeValue_Maximum_Property;
    PROPERTYID RangeValue_Minimum_Property;
    PROPERTYID RangeValue_SmallChange_Property;
    PROPERTYID RangeValue_Value_Property;
    PROPERTYID Scroll_HorizontallyScrollable_Property;
    PROPERTYID Scroll_HorizontalScrollPercent_Property;
    PROPERTYID Scroll_HorizontalViewSize_Property;
    PROPERTYID Scroll_VerticallyScrollable_Property;
    PROPERTYID Scroll_VerticalScrollPercent_Property;
    PROPERTYID Scroll_VerticalViewSize_Property;
    PROPERTYID SelectionItem_IsSelected_Property;
    PROPERTYID SelectionItem_SelectionContainer_Property;
    PROPERTYID Selection_CanSelectMultiple_Property;
    PROPERTYID Selection_IsSelectionRequired_Property;
    PROPERTYID Selection_Selection_Property;
    PROPERTYID TableItem_ColumnHeaderItems_Property;
    PROPERTYID TableItem_RowHeaderItems_Property;
    PROPERTYID Table_ColumnHeaders_Property;
    PROPERTYID Table_RowHeaders_Property;
    PROPERTYID Table_RowOrColumn_Property;
    PROPERTYID Toggle_ToggleState_Property;
    PROPERTYID Transform_CanMove_Property;
    PROPERTYID Transform_CanResize_Property;
    PROPERTYID Transform_CanRotate_Property;
    PROPERTYID Value_IsReadOnly_Property;
    PROPERTYID Value_Value_Property;
    PROPERTYID Window_CanMaximize_Property;
    PROPERTYID Window_CanMinimize_Property;
    PROPERTYID Window_IsModal_Property;
    PROPERTYID Window_IsTopmost_Property;
    PROPERTYID Window_WindowInteractionState_Property;
    PROPERTYID Window_WindowVisualState_Property;
    PROPERTYID Transform2_CanZoom_Property;
    PROPERTYID Transform2_ZoomLevel_Property;
    PROPERTYID Transform2_MaxZoom_Property;
    PROPERTYID Transform2_MinZoom_Property;
    PROPERTYID SpreadsheetItem_Formula_Property;
    PROPERTYID Styles_ExtendedProperties_Property;
    PROPERTYID Styles_FillColor_Property;
    PROPERTYID Styles_FillPatternColor_Property;
    PROPERTYID Styles_FillPatternStyle_Property;
    PROPERTYID Styles_Shape_Property;
    PROPERTYID Styles_StyleId_Property;
    PROPERTYID Styles_StyleName_Property;
    PROPERTYID PreventKeyboardDisplayOnProgrammaticFocus_Property;

    PATTERNID Invoke_Pattern;
    PATTERNID Dock_Pattern;
    PATTERNID Drag_Pattern;
    PATTERNID DropTarget_Pattern;
    PATTERNID ExpandCollapse_Pattern;
    PATTERNID GridItem_Pattern;
    PATTERNID Grid_Pattern;
    PATTERNID MultipleView_Pattern;
    PATTERNID RangeValue_Pattern;
    PATTERNID ScrollItem_Pattern;
    PATTERNID Scroll_Pattern;
    PATTERNID SelectionItem_Pattern;
    PATTERNID Selection_Pattern;
    PATTERNID TableItem_Pattern;
    PATTERNID Table_Pattern;
    PATTERNID Toggle_Pattern;
    PATTERNID Transform_Pattern;
    PATTERNID Value_Pattern;
    PATTERNID Window_Pattern;
    PATTERNID Text_Pattern;
    PATTERNID ItemContainer_Pattern;
    PATTERNID VirtualizedItem_Pattern;
    PATTERNID Text_Pattern2;
    PATTERNID TextChild_Pattern;
    PATTERNID Annotation_Pattern;
    PATTERNID ObjectModel_Pattern;
    PATTERNID Spreadsheet_Pattern;
    PATTERNID SpreadsheetItem_Pattern;
    PATTERNID Styles_Pattern;
    PATTERNID Transform_Pattern2;
    PATTERNID SynchronizedInput_Pattern;
    PATTERNID TextEdit_Pattern;
    PATTERNID CustomNavigation_Pattern;

    CONTROLTYPEID Button_ControlType;
    CONTROLTYPEID Calendar_ControlType;
    CONTROLTYPEID CheckBox_ControlType;
    CONTROLTYPEID ComboBox_ControlType;
    CONTROLTYPEID Custom_ControlType;
    CONTROLTYPEID DataGrid_ControlType;
    CONTROLTYPEID DataItem_ControlType;
    CONTROLTYPEID Document_ControlType;
    CONTROLTYPEID Edit_ControlType;
    CONTROLTYPEID Group_ControlType;
    CONTROLTYPEID Header_ControlType;
    CONTROLTYPEID HeaderItem_ControlType;
    CONTROLTYPEID Hyperlink_ControlType;
    CONTROLTYPEID Image_ControlType;
    CONTROLTYPEID List_ControlType;
    CONTROLTYPEID ListItem_ControlType;
    CONTROLTYPEID Menu_ControlType;
    CONTROLTYPEID MenuBar_ControlType;
    CONTROLTYPEID MenuItem_ControlType;
    CONTROLTYPEID Pane_ControlType;
    CONTROLTYPEID ProgressBar_ControlType;
    CONTROLTYPEID RadioButton_ControlType;
    CONTROLTYPEID ScrollBar_ControlType;
    CONTROLTYPEID Separator_ControlType;
    CONTROLTYPEID Slider_ControlType;
    CONTROLTYPEID Spinner_ControlType;
    CONTROLTYPEID SplitButton_ControlType;
    CONTROLTYPEID StatusBar_ControlType;
    CONTROLTYPEID SemanticZoom_ControlType;
    CONTROLTYPEID Tab_ControlType;
    CONTROLTYPEID TabItem_ControlType;
    CONTROLTYPEID Table_ControlType;
    CONTROLTYPEID Text_ControlType;
    CONTROLTYPEID Thumb_ControlType;
    CONTROLTYPEID TitleBar_ControlType;
    CONTROLTYPEID ToolBar_ControlType;
    CONTROLTYPEID ToolTip_ControlType;
    CONTROLTYPEID Tree_ControlType;
    CONTROLTYPEID TreeItem_ControlType;
    CONTROLTYPEID Window_ControlType;
    CONTROLTYPEID AppBar_ControlType;

    EVENTID AsyncContentLoaded_Event;
    EVENTID AutomationFocusChanged_Event;
    EVENTID AutomationPropertyChanged_Event;
    EVENTID Drag_DragCancel_Event;
    EVENTID Drag_DragComplete_Event;
    EVENTID Drag_DragStart_Event;
    EVENTID DropTarget_DragEnter_Event;
    EVENTID DropTarget_DragLeave_Event;
    EVENTID DropTarget_Dropped_Event;
    EVENTID Invoke_Invoked_Event;
    EVENTID MenuClosed_Event;
    EVENTID MenuOpened_Event;
    EVENTID Selection_InvalidatedEvent_Event;
    EVENTID SelectionItem_ElementAddedToSelectionEvent_Event;
    EVENTID SelectionItem_ElementRemovedFromSelectionEvent_Event;
    EVENTID SelectionItem_ElementSelectedEvent_Event;
    EVENTID StructureChanged_Event;
    EVENTID Text_TextChangedEvent_Event;
    EVENTID Text_TextSelectionChangedEvent_Event;
    EVENTID ToolTipClosed_Event;
    EVENTID ToolTipOpened_Event;
    EVENTID LiveRegionChanged_Event;
    EVENTID InputReachedTarget_Event;
    EVENTID InputReachedOtherElement_Event;
    EVENTID InputDiscarded_Event;
    EVENTID WindowClosed_Event;
    EVENTID WindowOpened_Event;
    EVENTID TextEdit_TextChanged_Event;
    EVENTID TextEdit_ConversionTargetChanged_Event;
    EVENTID LayoutInvalidated_Event;
    EVENTID Notification_Event;

    TEXTATTRIBUTEID Text_AnimationStyle_Attribute;
    TEXTATTRIBUTEID Text_BackgroundColor_Attribute;
    TEXTATTRIBUTEID Text_BulletStyle_Attribute;
    TEXTATTRIBUTEID Text_CapStyle_Attribute;
    TEXTATTRIBUTEID Text_Culture_Attribute;
    TEXTATTRIBUTEID Text_FontName_Attribute;
    TEXTATTRIBUTEID Text_FontSize_Attribute;
    TEXTATTRIBUTEID Text_FontWeight_Attribute;
    TEXTATTRIBUTEID Text_ForegroundColor_Attribute;
    TEXTATTRIBUTEID Text_HorizontalTextAlignment_Attribute;
    TEXTATTRIBUTEID Text_IndentationFirstLine_Attribute;
    TEXTATTRIBUTEID Text_IndentationLeading_Attribute;
    TEXTATTRIBUTEID Text_IndentationTrailing_Attribute;
    TEXTATTRIBUTEID Text_IsHidden_Attribute;
    TEXTATTRIBUTEID Text_IsItalic_Attribute;
    TEXTATTRIBUTEID Text_IsReadOnly_Attribute;
    TEXTATTRIBUTEID Text_IsSubscript_Attribute;
    TEXTATTRIBUTEID Text_IsSuperscript_Attribute;
    TEXTATTRIBUTEID Text_MarginBottom_Attribute;
    TEXTATTRIBUTEID Text_MarginLeading_Attribute;
    TEXTATTRIBUTEID Text_MarginTop_Attribute;
    TEXTATTRIBUTEID Text_MarginTrailing_Attribute;
    TEXTATTRIBUTEID Text_OutlineStyles_Attribute;
    TEXTATTRIBUTEID Text_OverlineColor_Attribute;
    TEXTATTRIBUTEID Text_OverlineStyle_Attribute;
    TEXTATTRIBUTEID Text_StrikethroughColor_Attribute;
    TEXTATTRIBUTEID Text_StrikethroughStyle_Attribute;
    TEXTATTRIBUTEID Text_Tabs_Attribute;
    TEXTATTRIBUTEID Text_TextFlowDirections_Attribute;
    TEXTATTRIBUTEID Text_UnderlineColor_Attribute;
    TEXTATTRIBUTEID Text_UnderlineStyle_Attribute;
};

class CUIAWrapper;
class CUIAWindow;
class CUIAInvokeProviderWrapper;
class CUIAWindowValidator;

// Definitions for Advise Event bits
#define FLAG_AEToolTipOpened                                        0x00000001
#define FLAG_AEToolTipClosed                                        0x00000002
#define FLAG_AEMenuOpened                                           0x00000004
#define FLAG_AEMenuClosed                                           0x00000008
#define FLAG_AEAutomationFocusChanged                               0x00000010
#define FLAG_AEInvokePatternOnInvoked                               0x00000020
#define FLAG_AESelectionItemPatternOnElementAddedToSelection        0x00000040
#define FLAG_AESelectionItemPatternOnElementRemovedFromSelection    0x00000080
#define FLAG_AESelectionItemPatternOnElementSelected                0x00000100
#define FLAG_AESelectionPatternOnInvalidated                        0x00000200
#define FLAG_AETextPatternOnTextSelectionChanged                    0x00000400
#define FLAG_AETextPatternOnTextChanged                             0x00000800
#define FLAG_AEAsyncContentLoaded                                   0x00001000
#define FLAG_AEPropertyChanged                                      0x00002000
#define FLAG_AEStructureChanged                                     0x00004000
#define FLAG_AEInputReachedTarget                                   0x00008000
#define FLAG_AEInputReachedOtherElement                             0x00010000
#define FLAG_AEInputDiscarded                                       0x00020000
#define FLAG_AEWindowClosed                                         0x00040000
#define FLAG_AEWindowOpened                                         0x00080000

typedef HRESULT (__stdcall *PATTERNPFN)(_In_ void* pWrapper, _In_ XUINT32 cValue, _In_ CValue* pValue, _Out_ CValue* pRetVal);

struct IXcpHostSite;

// Represents a UIA provider for XAML content.
// When a XAML app is running, CUIAWindow can be thought of as the provider for the CoreWindow's content.
// This is also used to host XAML content for popups and XamlIslandRoots.  (the name CUIAWindow may be misleading in this case).
class CUIAWindow : public IRawElementProviderSimple,
    public IRawElementProviderFragment,
    public IRawElementProviderFragmentRoot,
    public IRawElementProviderAdviseEvents,
    public IRawElementProviderHwndOverride,
    public CInterlockedReferenceCount,
    public IInspectable
{
protected:
    CUIAWindow(_In_ const UIAHostEnvironmentInfo& uiaHostEnvironmentInfo, _In_ IXcpHostSite *pHost);
    ~CUIAWindow() override;

    HRESULT InitIds();
public:
    HRESULT Init();
    void    Deinit();

    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Outptr_ void**) final;
    ULONG STDMETHODCALLTYPE AddRef() final;
    ULONG STDMETHODCALLTYPE Release() final;

    // IInspectable
    HRESULT STDMETHODCALLTYPE GetIids(ULONG* iidCount, IID** iids) final;
    HRESULT STDMETHODCALLTYPE GetRuntimeClassName(HSTRING *className) final;
    HRESULT STDMETHODCALLTYPE GetTrustLevel(TrustLevel *trustLevel) final;

    // IRawElementProviderSimple methods
    HRESULT STDMETHODCALLTYPE get_ProviderOptions(_Out_ ProviderOptions * pRetVal) final;
    HRESULT STDMETHODCALLTYPE GetPatternProvider(_In_ PATTERNID patternId, _Out_ IUnknown ** pRetVal) final;
    HRESULT STDMETHODCALLTYPE GetPropertyValue(_In_ PROPERTYID propertyId, _Out_ VARIANT * pRetVal) final;
    HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(_Out_ IRawElementProviderSimple ** pRetVal) final;

    // IRawElementProviderFragment methods
    HRESULT STDMETHODCALLTYPE get_BoundingRectangle(_Out_ UiaRect * pRetVal) final;
    HRESULT STDMETHODCALLTYPE get_FragmentRoot(_Out_ IRawElementProviderFragmentRoot** pRetVal) final;
    HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(_Outptr_result_maybenull_ SAFEARRAY** retVal) final;
    HRESULT STDMETHODCALLTYPE GetRuntimeId(_Out_ SAFEARRAY ** pRetVal) final;
    HRESULT STDMETHODCALLTYPE Navigate(_In_ NavigateDirection direction, _Out_ IRawElementProviderFragment ** pRetVal) final;
    HRESULT STDMETHODCALLTYPE NavigateImpl(_In_ NavigateDirection direction, _Out_ IRawElementProviderFragment ** pRetVal);
    HRESULT STDMETHODCALLTYPE SetFocus() final;

    // IRawElementProviderFragmentRoot methods
    HRESULT STDMETHODCALLTYPE ElementProviderFromPoint(_In_ double x, _In_ double y, _Out_ IRawElementProviderFragment** pRetVal) final;
    HRESULT STDMETHODCALLTYPE ElementProviderFromPointImpl(_In_ double x, _In_ double y, _Out_ IRawElementProviderFragment** pRetVal);
    HRESULT STDMETHODCALLTYPE GetFocus(_Out_ IRawElementProviderFragment** pRetVal) final;
    HRESULT STDMETHODCALLTYPE GetFocusImpl(_Out_ IRawElementProviderFragment ** pRetVal);

    // IRawElementProviderAdviseEvents methods
    HRESULT STDMETHODCALLTYPE AdviseEventAdded(_In_ EVENTID eventId, _Out_ SAFEARRAY *propertyIDs) final;
    HRESULT STDMETHODCALLTYPE AdviseEventRemoved(_In_ EVENTID eventId, _Out_ SAFEARRAY *propertyIDs) final;

    // IRawElementProviderHwndOverride
    HRESULT STDMETHODCALLTYPE GetOverrideProviderForHwnd(_In_ HWND hwnd, _Outptr_result_maybenull_ IRawElementProviderSimple **pRetVal) final;
    HRESULT STDMETHODCALLTYPE GetOverrideProviderForHwndImpl(_In_ HWND hwnd, _Outptr_result_maybenull_ IRawElementProviderSimple **pRetVal);

    HRESULT ReleaseInterfaceOffThread(_In_ IObject *pObject);

    _Check_return_ HRESULT UIAClientsAreListening(_In_ UIAXcp::APAutomationEvents eAutomationEvent) const;
    _Check_return_ HRESULT UIARaiseAutomationEvent(_In_ CAutomationPeer *pAP,
        _In_ UIAXcp::APAutomationEvents eAutomationEvent);
    _Check_return_ HRESULT UIARaiseAutomationPropertyChangedEvent(_In_ CAutomationPeer *pAP,
        _In_ UIAXcp::APAutomationProperties eAutomationProperty,
        _In_ const CValue& oldValue,
        _In_ const CValue& newValue);
    _Check_return_ HRESULT UIARaiseFocusChangedEventOnUIAWindow();

    _Check_return_ HRESULT UIARaiseTextEditTextChangedEvent(_In_ CAutomationPeer *pAP,
        _In_ UIAXcp::AutomationTextEditChangeType eAutomationProperty,
        _In_ CValue *pChangedData);

    _Check_return_ HRESULT UIARaiseNotificationEvent(
        _In_ CAutomationPeer* ap,
        UIAXcp::AutomationNotificationKind notificationKind,
        UIAXcp::AutomationNotificationProcessing notificationProcessing,
        const xstring_ptr& displayString,
        const xstring_ptr& activityId);

    void UIADisconnectAllProviders();

    EVENTID ConvertEnumToId(_In_ UIAXcp::APAutomationEvents eEvent);
    CONTROLTYPEID ConvertEnumToId(_In_ UIAXcp::APAutomationControlType eControlType);
    PROPERTYID ConvertEnumToId(_In_ UIAXcp::APAutomationProperties ePropertyType);
    LANDMARKTYPEID ConvertEnumToId(_In_ UIAXcp::AutomationLandmarkType eLandmarkType);
    UIAXcp::APAutomationProperties ConvertIdToAPAutomationProperties(_In_ PROPERTYID ePropertyId);
    UIAXcp::APAutomationControlType ConvertIdToAPAutomationControlType(_In_ CONTROLTYPEID eControlTypeId);
    UIAXcp::APAutomationProperties GetAutomationComponentProperty(_In_ PROPERTYID ePropertyId);
    HEADINGLEVELID ConvertEnumToId(_In_ UIAXcp::AutomationHeadingLevel eHeadingLevel);

    HRESULT CValueToVariant(
        _In_ const CValue& value,
        _Out_ VARIANT* pVAR);

    HRESULT VariantToCValue(
        _In_ const VARIANT* pVAR,
        _Out_ CValue *value);

    HRESULT AutomationPeerAnnotationCollectionToVariant(
        _In_opt_ CAutomationPeerAnnotationCollection* pAnnotations,
        _In_ PROPERTYID propertyId,
        _Out_ VARIANT* pResult);

    HRESULT ConvertToVariant(_In_ CAutomationPeerCollection* pAutomationPeerCollection, _Out_ VARIANT* pResult);

    IXcpHostSite* GetHost()
    {
        return m_pHost;
    }

    UIAHostEnvironmentInfo& GetHostEnvironmentInfo()
    {
        return m_uiaHostEnvironmentInfo;
    }

    UIAIdentifiers* GetUIAds()
    {
        return m_pUIAIds;
    }

    void InvalidateUIAWindowValidator()
    {
        m_pUIAWindowValidator = NULL;
    }

    IUIAWindowValidator* GetUIAWindowValidator();
    HRESULT CreateProviderForAP(CAutomationPeer* pAP, CUIAWrapper** ppRet);

    BOOL TransformClientToScreen(_In_ UIAHostEnvironmentInfo& uiaHostEnvironmentInfo, _Inout_ POINT* pPoint);
    BOOL TransformClientToScreen(_In_ UIAHostEnvironmentInfo& uiaHostEnvironmentInfo, _Inout_ XRECTF* pRect);
    BOOL TransformScreenToClient(_In_ const UIAHostEnvironmentInfo& uiaHostEnvironmentInfo, _Inout_ POINT* pPoint);

    // The ContentSite sets the AutomationOption, and we read it from the IContentIsland.
    // When we're in FragmentBased mode:
    //  * We are guaranteed to be ContentIsland-based (not UWP, which is no longer supported).
    //  * We don't own the fragment root, we're part of a fragment tree rooted by our host.
    //  * So, our get_FragmentRoot must return a root we get from the host, via the ContentIsland.
    //  * Our get_HostRawElementProvider must return null.
    //  * For our host to support ProviderFromPoint, it will need to QI us for 
    //      IRawElementProviderFragmentRoot and call get_ElementProviderFromPoint on us.
    // We have more work to do for fragment-based UIA for Popup:
    // https://task.ms/55483046 Ensure UIA works correctly in windowed popups within windowless islands
    bool IsAutomationOptionFragmentBased() const
    {
        return m_automationOption == ixp::ContentAutomationOptions_FragmentBased;
    }

    static bool ShouldFrameworkProvideWindowProperties();

    void SetMockUIAClientsListening(bool isEnabledMockUIAClientsListening);

    // Given a root element, return a list of automation peers for that root.
    // The list will contain any open unparented popups as well.
    static XUINT32 GetAutomationPeersForRoot(
        CUIElement* root,
        _Outptr_result_buffer_(return) CAutomationPeer*** rootPeersOut);

    virtual xref_ptr<ixp::IContentIsland> GetContentIsland() = 0;
    xref_ptr<ixp::IContentIslandAutomation> GetContentIslandAutomation();

protected:
    virtual HRESULT GetRootVisual(_Out_ xref_ptr<CDependencyObject>& rootVisual) = 0;
    virtual HRESULT GetPropertyValueOverride(_In_ PROPERTYID propertyId, _Out_ VARIANT * pRetVal) = 0;

private:
    void FlushUiaBridgeEventTable();
    HRESULT GetAPForHwnd(HWND hwnd, CAutomationPeer* pAPRoot, CAutomationPeer** pHwndAP);
    _Check_return_ HRESULT GetPreventKeyboardDisplayOnProgrammaticFocusId();

protected:
    UIAHostEnvironmentInfo m_uiaHostEnvironmentInfo;
    IXcpHostSite* m_pHost {nullptr};
    UIAIdentifiers* m_pUIAIds {nullptr};

private:
    XUINT32 m_cAdviseEventToolTipOpened {0};
    XUINT32 m_cAdviseEventToolTipClosed {0};
    XUINT32 m_cAdviseEventMenuOpened {0};
    XUINT32 m_cAdviseEventMenuClosed {0};
    XUINT32 m_cAdviseEventAutomationFocusChanged {0};
    XUINT32 m_cAdviseEventInvokePatternOnInvoked {0};
    XUINT32 m_cAdviseEventSelectionItemPatternOnElementAddedToSelection {0};
    XUINT32 m_cAdviseEventSelectionItemPatternOnElementRemovedFromSelection {0};
    XUINT32 m_cAdviseEventSelectionItemPatternOnElementSelected {0};
    XUINT32 m_cAdviseEventSelectionPatternOnInvalidated {0};
    XUINT32 m_cAdviseEventTextPatternOnTextSelectionChanged {0};
    XUINT32 m_cAdviseEventTextPatternOnTextChanged {0};
    XUINT32 m_cAdviseEventAsyncContentLoaded {0};
    XUINT32 m_cAdviseEventPropertyChanged {0};
    XUINT32 m_cAdviseEventStructureChanged {0};
    XUINT32 m_cAdviseEventDragStart {0};
    XUINT32 m_cAdviseEventDragCancel {0};
    XUINT32 m_cAdviseEventDragComplete {0};
    XUINT32 m_cAdviseEventDragEnter {0};
    XUINT32 m_cAdviseEventDragLeave {0};
    XUINT32 m_cAdviseEventDropped {0};
    XUINT32 m_cAdviseEventLiveRegionChanged {0};
    XUINT32 m_cAdviseEventInputReachedTarget {0};
    XUINT32 m_cAdviseEventInputReachedOtherElement {0};
    XUINT32 m_cAdviseEventInputDiscarded {0};
    XUINT32 m_cAdviseEventWindowClosed {0};
    XUINT32 m_cAdviseEventWindowOpened {0};
    XUINT32 m_cAdviseEventTextEditTextChanged {0};
    XUINT32 m_cAdviseEventConversionTargetChanged {0};
    XUINT32 m_cLayoutInvalidated {0};
    XUINT32 m_cAdviseNotificationEvent {0};

    IUIAWindowValidator* m_pUIAWindowValidator {nullptr};

    bool m_isEnabledMockUIAClientsListening {false};
    bool m_fInitialized {false};
    ixp::ContentAutomationOptions m_automationOption { ixp::ContentAutomationOptions_FrameworkBased };
};

class CUIAHostWindow final : public CUIAWindow
{
private:
    CUIAHostWindow(
        _In_ const UIAHostEnvironmentInfo& uiaHostEnvironmentInfo,
        _In_ IXcpHostSite *pHost,
        _In_opt_ CDependencyObject *pPopup);

public:
    static HRESULT Create(
        _In_ const UIAHostEnvironmentInfo& uiaHostEnvironmentInfo,
        _In_ IXcpHostSite *pHost,
        _In_opt_ CDependencyObject *pPopup,
        _Outptr_ CUIAHostWindow** ppUIAWindow);

    xref_ptr<ixp::IContentIsland> GetContentIsland() override;

protected:
    HRESULT GetRootVisual(_Out_ xref_ptr<CDependencyObject>& rootVisual) override;
    HRESULT GetPropertyValueOverride(_In_ PROPERTYID propertyId, _Out_ VARIANT * pRetVal) override;    

private:
    xref::weakref_ptr<CDependencyObject> m_popupWeakRef;
};

//------------------------------------------------------------------------
//
//  Class:   CUIAWindowValidator
//
//  Synopsis:  UIAWindowValidator Class to keep track of UIAWindow validation
//
//
//------------------------------------------------------------------------
class CUIAWindowValidator : public IUIAWindowValidator
{
public:
    CUIAWindowValidator(_In_ CUIAWindow *pUIAWindow) : m_cRef(0)
    {
        ASSERT(pUIAWindow);
        m_pUIAWindow = pUIAWindow;
    }

    ~CUIAWindowValidator()
    {
        if (m_pUIAWindow)
        {
            m_pUIAWindow->InvalidateUIAWindowValidator();
        }
    }

    XUINT32 AddRef() override { return ++m_cRef; }

    XUINT32 Release() override
    {
        if (--m_cRef <= 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }

    void Invalidate() override
    {
        m_pUIAWindow = NULL;
    }

    XINT32 IsValid() override
    {
        return m_pUIAWindow ? TRUE/* valid UIA Window */ : FALSE/* invalid */;
    }

private:
    CUIAWindow *m_pUIAWindow;
    XUINT32 m_cRef;
};
