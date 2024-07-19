// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines imports to be used by wrapping frameworks.

#pragma once

#include <minxcptypes.h>
#include <namespacealiases.h>
#include <cstdint>

#include <sal.h>
#include <winerror.h>

class CAutomationPeer;
class CCollection;
class CContentControl;
class CContentPresenter;
class CContentRoot;
class CControl;
class CCoreServices;
class CDataTemplate;
class CDependencyObject;
class CDependencyObjectCollection;
class CDependencyProperty;
class CDispatcherTimer;
class CEasingFunctionBase;
class CEventArgs;
class CFrameworkElement;
class CGeneralTransform;
class CInline;
class CInternalTransform;
class CKeyEventArgs;
class CPanel;
class CPointer;
class CPrintDocument;
class CRichTextBlock;
class CRichTextBlockOverflow;
class CScrollContentControl;
class CStackPanel;
class CStoryboard;
class CSurfaceImageSource;
class CSwapChainPanel;
class CTextAdapter;
class CTextBlock;
class CTextBox;
class CTextBoxView;
class CTextElement;
class CTextPointerWrapper;
class CTextRangeAdapter;
class CTimeline;
class CTransform;
class CUIElement;
class CValue;
class CVirtualSurfaceImageSource;
class CVisualStateGroup;
class CWriteableBitmap;
class IVirtualSurfaceImageSourceCallbacks;
class xruntime_string_ptr;
class xstring_ptr_view;
class xstring_ptr;
enum class ComponentResourceLocation;
enum class CTextBoxView_ScrollCommand;
enum class KnownEventIndex : uint16_t;
enum class KnownPropertyIndex : uint16_t;
enum class KnownTypeIndex : uint16_t;
enum ValueType : uint32_t;
enum XDMContentType;
namespace ABI::Windows::Foundation { struct Rect; }
namespace ABI::Windows::System { enum VirtualKeyModifiers : unsigned int; }
namespace DirectUI { enum class ClockState : uint8_t; }
namespace DirectUI { enum class DataPackageOperation : uint8_t; }
namespace DirectUI { enum class DragDropMessageType : uint8_t; }
namespace DirectUI { enum class FocusState : uint8_t; }
namespace DirectUI { enum class InputPaneState : uint8_t; }
namespace DirectUI { enum class ManagedEvent : uint8_t; }
namespace DirectUI { enum class TransitionTrigger : uint8_t; }
namespace Parser { struct XamlBuffer; }
namespace UIAXcp { enum APAutomationEvents; }
namespace UIAXcp { enum APAutomationProperties; }
namespace UIAXcp { enum TextPatternRangeEndpoint; }
namespace UIAXcp { enum TextUnit; }
struct CTextBoxView_ScrollData;
struct ICustomResourceLoader;
struct IInspectable;
struct IPALMemory;
struct IUnknown;
struct SetValueParams;
struct XamlAssemblyToken;
struct XamlTypeNamespaceToken;
template <typename T> class xref_ptr;

namespace CoreImports
{
    _Check_return_ HRESULT UIElement_HitTestPoint(
        _In_opt_ CUIElement *pvUIElement,
        _In_ CCoreServices *pContext,
        _In_ XPOINTF ptHit,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_ XINT32 *pnCount,
        _Outptr_result_buffer_(*pnCount) CUIElement*** ppElements);

    _Check_return_ HRESULT UIElement_HitTestRect(
        _In_opt_ CUIElement *pvUIElement,
        _In_ CCoreServices *pContext,
        _In_ XRECTF rcHit,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_ XINT32 *pnCount,
        _Outptr_result_buffer_(*pnCount) CUIElement*** ppElements,
        _Out_ bool *isRootNull);

    _Check_return_ HRESULT UIElement_DeleteList(
        _In_reads_(nResults) void *pvUIElement,
        _In_ XINT32 nResults);

    _Check_return_ HRESULT Host_GetActualHeight(
        _In_ CCoreServices* pCore,
        _Out_ XUINT32* pnActualHeight);

    _Check_return_ HRESULT Host_GetActualWidth(
        _In_ CCoreServices* pCore,
        _Out_ XUINT32* pnActualWidth);

    _Check_return_ HRESULT Host_GetEnableFrameRateCounter(
        _In_ CCoreServices* pCore,
        _Out_ bool* pbEnableFrameRateCounter);

    _Check_return_ HRESULT Host_SetEnableFrameRateCounter(
        _In_ CCoreServices* pCore,
        _In_ bool bEnableFrameRateCounter);

    _Check_return_ HRESULT Input_GetKeyboardModifiers(
        _Out_ wsy::VirtualKeyModifiers* pnKeyboardModifiers);

    _Check_return_ HRESULT Application_SetVisualRoot(
        _In_ CCoreServices* pCore,
        _In_ CUIElement* pRootVisual);

    _Check_return_ HRESULT Application_SetRootScrollViewer(
        _In_ CCoreServices* pCore,
        _In_opt_ CScrollContentControl* pRootScrollViewer,
        _In_opt_ CContentPresenter* pContentPresenter);

    _Check_return_ HRESULT Application_GetInputPaneState(
        _In_ CUIElement* pElement,
        _Out_ DirectUI::InputPaneState* pInputPaneState,
        _Out_ XRECTF* pInputPaneBounds);

    _Check_return_ HRESULT Application_LoadComponent(
        _In_ CCoreServices* pCore,
        _In_ CDependencyObject* pComponent,
        _In_ const xstring_ptr& strUri,
        _In_ ::ComponentResourceLocation resourceLocation);

    _Check_return_ HRESULT FocusManager_CanHaveFocusableChildren(
        _In_ CDependencyObject* pElement,
        _Out_ bool *pCanHaveFocusableChildren);

    _Check_return_ HRESULT PopupRoot_CloseTopmostPopup(
        _In_ CDependencyObject *pPopupRootCDO,
        _In_ bool bLightDismissOnly);

    _Check_return_ HRESULT Popup_Close(
        _In_ CDependencyObject *pPopupCDO);

    _Check_return_ HRESULT Popup_GetIsLightDismiss(
        _In_ CDependencyObject *pPopupCDO,
        _Out_ bool *pIsLightDismiss);

    _Check_return_ HRESULT Popup_GetSavedFocusState(
        _In_ CDependencyObject* pPopupCDO,
        _Out_ DirectUI::FocusState *pFocusState);

    _Check_return_ HRESULT Popup_SetFocusStateAfterClosing(
        _In_ CDependencyObject* pPopupCDO,
        _In_ DirectUI::FocusState focusState);

    _Check_return_ HRESULT Popup_SetShouldTakeFocus(
        _In_ CDependencyObject* pPopupCDO,
        _In_ bool shouldTakeFocus);

    _Check_return_ HRESULT FocusManager_GetFirstFocusableElement(
        _In_ CDependencyObject* pSearchStart,
        _Outptr_ CDependencyObject** ppFirstFocusableElement);

    _Check_return_ HRESULT FocusManager_GetLastFocusableElement(
        _In_ CDependencyObject* pSearchStart,
        _Outptr_ CDependencyObject** ppLastFocusableElement);

    _Check_return_ HRESULT LayoutInformation_GetLayoutExceptionElement(
        _In_ CCoreServices* pCore,
        _Out_ CValue* pObject);

    _Check_return_ HRESULT LayoutInformation_SetLayoutExceptionElement(
        _In_ CCoreServices* pCore,
        _In_ CUIElement* pElement);

    _Check_return_ HRESULT DependencyObject_Freeze(
        _In_ CDependencyObject* pObject);

    _Check_return_ HRESULT DependencyObject_Unfreeze(
        _In_ CDependencyObject* pObject,
        _Out_ bool* pbWasFrozen);

    _Check_return_ HRESULT DependencyObject_GetBaseUri(
        _In_ CDependencyObject *pObject,
        _Out_ CValue *pValue);

    _Check_return_ HRESULT Host_CreateFromXaml(
        _In_ CCoreServices* pCore,
        _In_ uint32_t cXaml,
        _In_reads_(cXaml) const WCHAR* pXaml,
        _In_ bool bCreateNameScope,
        _In_ bool bRequireDefaultNamespace,
        _In_ bool bExpandTemplatesDuringParse,
        _Out_ CValue* pObject);

    _Check_return_ HRESULT CreateFromXamlBytes(
        _In_ CCoreServices* pCore,
        _In_ const Parser::XamlBuffer& buffer,
        _In_ const xstring_ptr_view& strSourceAssemblyName,
        _In_ bool bCreateNameScope,
        _In_ bool bRequireDefaultNamespace,
        _In_ const xstring_ptr_view& strXamlResourceUri,
        _Out_ CValue *pObject);

    _Check_return_ HRESULT FrameworkElement_MeasureOverride(
        _In_ CFrameworkElement* pElement,
        _In_ XFLOAT fAvailableWidth,
        _In_ XFLOAT fAvailableHeight,
        _Inout_ XFLOAT* pfDesiredWidth,
        _Inout_ XFLOAT* pfDesiredHeight);

    _Check_return_ HRESULT FrameworkElement_ArrangeOverride(
        _In_ CFrameworkElement* pElement,
        _In_ XFLOAT fFinalWidth,
        _In_ XFLOAT fFinalHeight,
        _Inout_ XFLOAT* pfActualWidth,
        _Inout_ XFLOAT* pfActualHeight);

    _Check_return_ HRESULT UIElement_SetCurrentTransitionLocation(
        _In_ CUIElement* pTarget,
        _In_ XFLOAT left,
        _In_ XFLOAT top,
        _In_ XFLOAT width,
        _In_ XFLOAT height);

    _Check_return_ HRESULT UIElement_SetIsEntering(
        _In_ CUIElement* pTarget,
        _In_ bool fValue);

    _Check_return_ HRESULT UIElement_SetIsLeaving(
        _In_ CUIElement* pTarget,
        _In_ bool fValue);

    _Check_return_ HRESULT UIElement_GetHasTransition(
        _In_ CUIElement* pTarget,
        _Out_ bool* pHasTransition,
        _Out_opt_ DirectUI::TransitionTrigger* pTransitionTrigger);

    _Check_return_ HRESULT UIElement_CancelTransition(
        _In_ CUIElement* pTarget);

    _Check_return_ HRESULT LayoutManager_GetLayoutTickForTransition(
        _In_ CUIElement* pTarget,
        _Out_ XINT16* pTick);

    _Check_return_ HRESULT Transition_SetIsStaggeringEnabled(
        _In_ void* pTransition,
        _In_ bool value);

    _Check_return_ HRESULT UIElement_SetDeferUnlinkContainer(
        _In_ CUIElement* pTarget);

    _Check_return_ HRESULT CPanel_PanelGetClosestIndexSlow(
        _In_ CPanel* pNativeInstance,
        _In_ XPOINTF location,
        _Out_ XINT32* returnValue);

    _Check_return_ HRESULT UIElement_Measure(
        _In_ CUIElement* pElement,
        _In_ XFLOAT fAvailableWidth,
        _In_ XFLOAT fAvailableHeight);

    _Check_return_ HRESULT UIElement_Arrange(
        _In_ CUIElement* pElement,
        _In_ XFLOAT fX,
        _In_ XFLOAT fY,
        _In_ XFLOAT fWidth,
        _In_ XFLOAT fHeight);

    _Check_return_ HRESULT UIElement_GetDesiredSize(
        _In_ CUIElement* pElement,
        _Inout_ XFLOAT* pfDesiredWidth,
        _Inout_ XFLOAT* pfDesiredHeight);

    _Check_return_ HRESULT UIElement_GetVisualOffset(
        _In_ CUIElement* pElement,
        _Inout_ XFLOAT* pfOffsetX,
        _Inout_ XFLOAT* pfOffsetY);

    _Check_return_ HRESULT UIElement_IsFocusable(
        _In_ CUIElement* pElement,
        _Out_ bool* pIsFocusable);

    _Check_return_ HRESULT DispatcherTimer_Start(
        _In_ CDispatcherTimer* pDispatcherTimer);

    _Check_return_ HRESULT DispatcherTimer_Stop(
        _In_ CDispatcherTimer* pDispatcherTimer);

    _Check_return_ HRESULT Control_Raise(
        _In_ CControl* pControl,
        _In_ CEventArgs* pEventArgs,
        _In_ KnownEventIndex nDelegateIndex);

    _Check_return_ HRESULT SetAutomationPeerParent(
        _In_ CAutomationPeer* pAutomationPeer,
        _In_ CAutomationPeer* pParentAutomationPeer);

    _Check_return_ HRESULT GetTextProviderValue(
        _In_ CTextAdapter* ptextprovider,
        _In_ XUINT32 nAPIIndex,
        _In_ CValue pArg,
        _Out_ CValue* pData);

    _Check_return_ HRESULT GetTextRangeProviderValue(
        _In_ CTextRangeAdapter* ptextprovider,
        _In_ XUINT32 nAPIIndex,
        _In_ CValue pArg,
        _Out_ CValue* pData);

    _Check_return_ HRESULT GetTextRangeArray(
        _In_ CTextAdapter* pTextProvider,
        _In_ XUINT32 nAPIIndex,
        _Outptr_result_buffer_all_(*pnCount) CTextRangeAdapter*** pppChildren,
        _Out_ XINT32* pnCount);

    _Check_return_ HRESULT TextRangeAdapter_CompareEndpoints(
        _In_ CTextRangeAdapter* pTextRangeProvider,
        _In_ UIAXcp::TextPatternRangeEndpoint endPoint,
        _In_ CTextRangeAdapter* pTargetTextRangeProvider,
        _In_ UIAXcp::TextPatternRangeEndpoint targetEndPoint,
        _Out_ XINT32* pReturnValue);

    _Check_return_ HRESULT TextRangeAdapter_GetBoundingRectangles(
        _In_ CTextRangeAdapter* pTextRangeProvider,
        _Outptr_result_buffer_all_(*pnCount) double** ppRectangles,
        _Out_ XINT32* pnCount);

    _Check_return_ HRESULT TextRangeAdapter_Move(
        _In_ CTextRangeAdapter* pTextRangeProvider,
        _In_ UIAXcp::TextUnit unit,
        _In_ XINT32 count,
        _Out_ XINT32* pReturnValue);

    _Check_return_ HRESULT TextRangeAdapter_MoveEndpointByUnit(
        _In_ CTextRangeAdapter* pTextRangeProvider,
        _In_ UIAXcp::TextPatternRangeEndpoint endPoint,
        _In_ UIAXcp::TextUnit unit,
        _In_ XINT32 count,
        _Out_ XINT32* pReturnValue);

    _Check_return_ HRESULT TextRangeAdapter_MoveEndpointByRange(
        _In_ CTextRangeAdapter* pTextRangeProvider,
        _In_ UIAXcp::TextPatternRangeEndpoint endPoint,
        _In_ CTextRangeAdapter* pTargetTextRangeProvider,
        _In_ UIAXcp::TextPatternRangeEndpoint targetEndPoint);

    _Check_return_ HRESULT TextRangeAdapter_GetChildren(
        _In_ CTextRangeAdapter* pTextRangeProvider,
        _Outptr_result_buffer_all_(*pnCount) CAutomationPeer*** pppChildren,
        _Out_ XINT32* pnCount);

    _Check_return_ HRESULT AutomationListenerExists(
        _In_ CCoreServices* pCore,
        _In_ UIAXcp::APAutomationEvents nAutomationEventIndex,
        _Out_ bool* bExists);

    _Check_return_ HRESULT AutomationRaiseAutomationPropertyChanged(
        _In_ CAutomationPeer* pAutomationPeer,
        _In_ UIAXcp::APAutomationProperties nAutomationPropertyIndex,
        _In_ const CValue& oldValue,
        _In_ const CValue& newValue);

    _Check_return_ HRESULT AutomationRaiseFocusChangedOnUIAWindow(
        _In_ CCoreServices* pCore,
        _In_ CDependencyObject* sender);

    _Check_return_ HRESULT DependencyObject_GetIsAssociated(
        _In_ CDependencyObject* pObject,
        _Out_ bool* pfIsAssociated,
        _Out_ bool* pfAllowsMultipleAssociations);

    _Check_return_ HRESULT DependencyObject_ClearValue(
        _In_ CDependencyObject* pObject,
        _In_ const CDependencyProperty* pDP);

    _Check_return_ HRESULT DependencyObject_IsPropertyDefault(
        _In_ CDependencyObject* pObject,
        _In_ const CDependencyProperty* pDP,
        _Out_ bool* pbIsPropertyDefault);

    _Check_return_ HRESULT DependencyObject_ShouldCreatePeerWithStrongRef(
        _In_ CDependencyObject* pObject,
        _Out_ bool* pbShouldCreate);

    _Check_return_ HRESULT DependencyObject_GetTypeIndex(
        _In_ CDependencyObject* pObject,
        _Out_ KnownTypeIndex* piTypeIndex);

    _Check_return_ HRESULT Collection_Add(
        _In_ CCollection* pCollection,
        _In_ CValue* pItem);

    _Check_return_ HRESULT Collection_GetItem(
        _In_ CCollection* pCollection,
        _In_ XUINT32 nIndex,
        _Out_ CValue* pItem);

    _Check_return_ HRESULT Collection_Insert(
        _In_ CCollection* pCollection,
        _In_ XUINT32 nIndex,
        _In_ CValue* pItem);

    _Check_return_ HRESULT DependencyObjectCollection_SetAt(
        _In_ CDependencyObjectCollection* pCollection,
        _In_ XUINT32 nIndex,
        _In_ CValue* pItem);

    _Check_return_ HRESULT Collection_RemoveAt(
        _In_ CCollection* pCollection,
        _In_ XUINT32 nIndex);

    _Check_return_ HRESULT Collection_IndexOf(
        _In_ CCollection* pCollection,
        _In_ CValue* pItem,
        _Out_ XINT32* piIndex);

    _Check_return_ HRESULT Collection_Clear(
        _In_ CCollection* pCollection);

    _Check_return_ HRESULT Collection_Move(
        _In_ CCollection *pCollection,
        _In_ XINT32 nIndex,
        _In_ XINT32 nPosition);

    _Check_return_ HRESULT Collection_SetOwner(
        _In_ CCollection* pCollection,
        _In_ CDependencyObject* pOwner);

    void TextBoxView_GetScrollData(
        _In_ CTextBoxView *pTextBoxView,
        _Outptr_ const CTextBoxView_ScrollData **ppScrollData);

    void TextBoxView_SetScrollEnabled(
        _In_ CTextBoxView *pTextBoxView,
        _In_ bool canHorizontallScroll,
        _In_ bool canVerticallyScroll);

    _Check_return_ HRESULT TextBoxView_Scroll(
        _In_ CTextBoxView *pTextBoxView,
        _In_ CTextBoxView_ScrollCommand command,
        _In_ XUINT32 mouseWheelDelta,
        _Out_opt_ bool *pScrolled);

    _Check_return_ HRESULT TextBoxView_SetScrollOffsets(
        _In_ CTextBoxView *pTextBoxView,
        _In_ double horizontalOffset,
        _In_ double verticalOffset);

    void TextBoxView_DirectManipulationStarted(
        _In_ CTextBoxView *pTextBoxView);

    void TextBoxView_DirectManipulationCompleted(
        _In_ CTextBoxView *pTextBoxView);

    void TextBoxView_DirectManipulationDelta(
        _In_ CTextBoxView *pTextBoxView,
        FLOAT xCumulativeTranslation,
        FLOAT yCumulativeTranslation,
        FLOAT zCumulativeFactor);

    void TextBoxView_CaretChanged(
        _In_ CTextBoxView *pTextBoxView);

    void TextBoxView_CaretVisibilityChanged(
        _In_ CTextBoxView *pTextBoxView);

    void TextBoxView_InvalidateView(
        _In_ CTextBoxView *pTextBoxView);

    _Check_return_ HRESULT TextBox_OnDeleteButtonClick(
        _In_ CTextBox *pTextBox);

    _Check_return_ HRESULT RichEditBox_GetDocument(
        _In_ void *pRichEditBox,
        _In_ REFIID iid,
        _Outptr_ IUnknown **ppInterface);

    _Check_return_ HRESULT ConvertStringToTypedCValue(
        _In_ CCoreServices* pCore,
        _In_ const xstring_ptr& strClrTypeName,
        _In_ const xstring_ptr& strText,
        _Out_ CValue* pValue);

    _Check_return_ HRESULT DependencyObject_GetVisualRelative(
        _In_ CUIElement* pElement,
        _In_ XINT32 iRelativeLinkKind,
        _Out_ CValue* pRelative);

    _Check_return_ HRESULT ClearUIElementChildren(
        _In_ CUIElement* pElement);

    _Check_return_ HRESULT GetManagedPropertyValueFromStyle(
        _In_ bool bUseBuiltInStyle,
        _In_ CDependencyObject* pElement,
        _In_ const CDependencyProperty* pDP,
        _Out_ CValue* pValue,
        _Out_ bool* pbFoundValue);

    _Check_return_ HRESULT DependencyObject_GetValue(
        _In_ CDependencyObject* pObject,
        _In_ const CDependencyProperty* pDP,
        _Out_ CValue* pValue);

    _Check_return_ HRESULT DependencyObject_SetValue(
        _In_ CDependencyObject* pObject,
        _In_ const SetValueParams& args);

    _Check_return_ HRESULT GetAnimationBaseValue(
        _In_ CDependencyObject* pObject,
        _In_ const CDependencyProperty* pDP,
        _Out_ CValue* pValue);

    _Check_return_ HRESULT DependencyObject_AddRef(
        _In_ CDependencyObject* pObject);

    _Check_return_ HRESULT WantsEvent(
        _In_ CDependencyObject* hElement,
        _In_ DirectUI::ManagedEvent nManagedEventIndex,
        _In_ bool bWantsToHandleEvent);

    _Check_return_ HRESULT WantsEventStatic(
        _In_ CCoreServices* pCore,
        _In_ DirectUI::ManagedEvent nManagedEventIndex,
        _In_ bool bWantsToHandleEvent);

    _Check_return_ HRESULT CreateObjectByTypeIndex(
        _In_ CCoreServices* pCore,
        _In_ KnownTypeIndex iTypeIndex,
        _Out_ CDependencyObject** ppObject);

    _Check_return_ HRESULT NotifyHasManagedPeer(
        _In_ XHANDLE hObject,
        _In_ ValueType nType,
        _In_ bool bIsCustomType,
        _In_ bool bIsGCRoot);

    _Check_return_ HRESULT GetClassFullName(
        _In_ CCoreServices* pCore,
        _In_ const xstring_ptr_view& strClassName,
        _Inout_ CValue* pFullName);

    _Check_return_ HRESULT ContentControl_SetContentIsNotLogical(
        _In_ CContentControl* pContentControl);

    _Check_return_ HRESULT ContentControl_TryGetContentTemplateRoot(
        _In_ CContentControl* pContentControl,
        _Outptr_ CUIElement** ppContentTemplateRoot);

    _Check_return_ HRESULT ScrollContentControl_SetRootScrollViewerOriginalHeight(
        _In_ CScrollContentControl* pScrollContentControl,
        _In_ XFLOAT fOriginalHeight);

    _Check_return_ HRESULT ScrollContentControl_SetRootScrollViewerSetting(
        _In_ CScrollContentControl* pScrollContentControl,
        _In_ XUINT8 setting,
        _In_ bool bValue);

    _Check_return_ HRESULT ScrollContentControl_GetRootScrollViewerSetting(
        _In_ CScrollContentControl* pScrollContentControl,
        _In_ XUINT8 setting,
        _Out_ bool& bValue);

    _Check_return_ HRESULT ScrollContentControl_SetFocusOnFlyoutLightDismissPopupByPointer(
        _In_ CScrollContentControl* pScrollContentControl,
        _Out_ bool* pfIsFocusedOnLightDismissPopupOfFlyout);

    _Check_return_ HRESULT ContentControl_SetContentIsTemplateBoundManaged(
        _In_ CContentControl* pContentControl,
        _In_ bool bIsTemplateBoundByManaged);

    _Check_return_ HRESULT SetFrameworkContext(
        _In_ CCoreServices* pCore,
        _In_ XHANDLE context);

    _Check_return_ HRESULT Property_GetDefaultValue(
        _In_ CCoreServices* pCore,
        _In_ CDependencyObject* pObject,
        _In_ KnownTypeIndex nObjectCoreTypeIndex,
        _In_ const CDependencyProperty* pDP,
        _Out_ CValue* pDefaultValue);

    _Check_return_ HRESULT RefreshXamlSchemaContext(
        _In_ CCoreServices* pCore);

    _Check_return_ HRESULT VisualStateGroup_GetCurrentState(
        _In_ CVisualStateGroup* pVisualStateGroup,
        _Out_ CValue* pCurrentState);

    _Check_return_ HRESULT XamlSchemaContext_AddAssemblyXmlnsDefinition(
        _In_ CCoreServices* pCore,
        _In_ XamlAssemblyToken tAssembly,
        _In_ const xstring_ptr& strXmlNamespace,
        _In_ XamlTypeNamespaceToken tTypeNamespace,
        _In_ const xstring_ptr& strTypeNamespace);

    _Check_return_ HRESULT EasingFunctionBase_Ease(
        _In_ CEasingFunctionBase* pEasingFunction,
        _In_ XFLOAT fNormalizedTime,
        _Out_ XFLOAT* pfAlpha);

    _Check_return_ HRESULT WriteableBitmap_Invalidate(
        _In_ CWriteableBitmap* pWriteableBitmap);

    _Check_return_ HRESULT WriteableBitmap_CopyPixels(
        _In_ CWriteableBitmap* pWriteableBitmap,
        _In_ XHANDLE hPixels);

    _Check_return_ HRESULT WriteableBitmap_Create(
        _In_ CWriteableBitmap* pWriteableBitmap,
        _In_ XHANDLE hPixels,
        _In_ XINT32 iWidth,
        _In_ XINT32 iHeight);

    _Check_return_ HRESULT SurfaceImageSource_Initialize(
        _In_ CSurfaceImageSource* pSurfaceImageSource,
        _In_ XINT32 iWidth,
        _In_ XINT32 iHeight,
        _In_ bool isOpaque);

    _Check_return_ HRESULT SurfaceImageSource_SetDevice(
        _In_ CSurfaceImageSource* pSurfaceImageSource,
        _In_ IUnknown *pDevice);

    _Check_return_ HRESULT SurfaceImageSource_BeginDraw(
        _In_ CSurfaceImageSource* pSurfaceImageSource,
        _In_ const XRECT *pUpdateRect,
        _In_ REFIID iid,
        _Out_ IUnknown **ppSurface,
        _Out_ XPOINT *pPoint);

    _Check_return_ HRESULT SurfaceImageSource_EndDraw(
        _In_ CSurfaceImageSource* pSurfaceImageSource);

    _Check_return_ HRESULT SurfaceImageSource_SetDeviceWithD2D(
        _In_ CSurfaceImageSource* pSurfaceImageSource,
        _In_ IUnknown *pDevice);

    _Check_return_ HRESULT SurfaceImageSource_BeginDrawWithD2D(
        _In_ CSurfaceImageSource* pSurfaceImageSource,
        _In_ const XRECT *pUpdateRect,
        _In_ REFIID iid,
        _In_ bool calledFromUIThread,
        _Outptr_ IUnknown **ppUpdateObject,
        _Out_ XPOINT *pPoint);

    _Check_return_ HRESULT SurfaceImageSource_EndDrawWithD2D(
        _In_ CSurfaceImageSource* pSurfaceImageSource);

    _Check_return_ HRESULT SurfaceImageSource_SuspendDraw(
        _In_ CSurfaceImageSource* pSurfaceImageSource,
        _In_ bool calledFromUIThread);

    _Check_return_ HRESULT SurfaceImageSource_ResumeDraw(
        _In_ CSurfaceImageSource* pSurfaceImageSource,
        _In_ bool calledFromUIThread);

    _Check_return_ HRESULT VirtualSurfaceImageSource_Initialize(
        _In_ CVirtualSurfaceImageSource* pVirtualSurfaceImageSource,
        _In_ XINT32 iWidth,
        _In_ XINT32 iHeight,
        _In_ bool isOpaque);

    _Check_return_ HRESULT VirtualSurfaceImageSource_Invalidate(
        _In_ CVirtualSurfaceImageSource* pVirtualSurfaceImageSource,
        _In_ XRECT updateRect);

    _Check_return_ HRESULT VirtualSurfaceImageSource_GetUpdateRects(
        _In_ CVirtualSurfaceImageSource* pVirtualSurfaceImageSource,
        _Out_writes_(count) XRECT *pUpdates,
        _In_ XDWORD count);

    _Check_return_ HRESULT VirtualSurfaceImageSource_GetUpdateRectCount(
        _In_ CVirtualSurfaceImageSource* pVirtualSurfaceImageSource,
        _Out_ DWORD* pNumOfUpdates);

    _Check_return_ HRESULT VirtualSurfaceImageSource_GetVisibleBounds(
        _In_ CVirtualSurfaceImageSource* pVirtualSurfaceImageSource,
        _Out_ XRECT_RB* pBounds);

    _Check_return_ HRESULT VirtualSurfaceImageSource_Resize(
        _In_ CVirtualSurfaceImageSource* pVirtualSurfaceImageSource,
        _In_ XINT32 newWidth,
        _In_ XINT32 newHeight);

    _Check_return_ HRESULT VirtualSurfaceImageSource_RegisterCallbacks(
        _In_ CVirtualSurfaceImageSource* pVirtualSurfaceImageSource,
        _In_ IVirtualSurfaceImageSourceCallbacks *pCallback);

    _Check_return_ HRESULT SwapChainPanel_SetSwapChain(
        _In_ CSwapChainPanel* pSwapChainElement,
        _In_opt_ IUnknown* pSwapChain);

    _Check_return_ HRESULT Transform_Inverse(
        _In_ CTransform* pTransform,
        _Out_ CGeneralTransform **ppResult);

    _Check_return_ HRESULT Transform_Transform(
        _In_ void *pvTransform,
        _In_ XPOINTF ptHit,
        _Inout_ XPOINTF *pResult);

    _Check_return_ HRESULT Transform_TransformBounds(
        _In_ void *pvTransform,
        _In_ XRECTF rectHit,
        _Inout_ XRECTF *pResult);

    _Check_return_ HRESULT InternalTransform_Inverse(
        _In_ CInternalTransform* pTransform,
        _Out_ CGeneralTransform **ppResult);

    _Check_return_ HRESULT InternalTransform_Transform(
        _In_ void *pvTransform,
        _In_ XPOINTF ptHit,
        _Inout_ XPOINTF *pResult);

    _Check_return_ HRESULT InternalTransform_TransformBounds(
        _In_ void *pvTransform,
        _In_ XRECTF rectHit,
        _Inout_ XRECTF *pResult);

    _Check_return_ HRESULT LayoutManager_ClearErrorOccurredDuringLayout(
        _In_ CCoreServices* pCore);

    _Check_return_ HRESULT LayoutManager_DidErrorOccurDuringLayout(
        _In_ CCoreServices* pCore,
        _Out_ bool* bErrorOccurred);

    _Check_return_ HRESULT Text_GetBaselineOffset(
        _In_ CDependencyObject* pObject,
        _Out_ XFLOAT* pfBaselineOffset);

    _Check_return_ HRESULT TextElement_GetEdge(
        _In_ CTextElement* pTextElement,
        _In_ XINT32 iEdgeIndex,
        _Out_ CValue* pTextPointer);

    _Check_return_ HRESULT TextPointer_GetLogicalDirection(
        _In_ CTextPointerWrapper* pTextPointer,
        _Out_ CValue* pLogicalDirection);

    _Check_return_ HRESULT TextPointer_GetOffset(
        _In_ CTextPointerWrapper* pTextPointer,
        _Out_ XINT32* piOffset);

    _Check_return_ HRESULT TextPointer_GetParent(
        _In_ CTextPointerWrapper* pTextPointer,
        _Out_ CValue* pParent);

    _Check_return_ HRESULT TextPointer_GetVisualParent(
        _In_ CTextPointerWrapper* pTextPointer,
        _Out_ CValue* pParent);

    _Check_return_ HRESULT TextPointer_GetPositionAtOffset(
        _In_ CTextPointerWrapper* pTextPointer,
        _In_ XINT32 iOffset,
        _In_ XINT32 iLogicalDirection,
        _Out_ CValue* pOffsetTextPointer);

    _Check_return_ HRESULT TextPointer_GetCharacterRect(
        _In_ CTextPointerWrapper* pTextPointer,
        _In_ XINT32 iLogicalDirection,
        _Out_ wf::Rect* pRect);

    _Check_return_ HRESULT TextBlock_GetEdge(
        _In_ CTextBlock* pTextBlock,
        _In_ XINT32 iEdgeIndex,
        _Out_ CValue* pTextPointer);

    _Check_return_ HRESULT TextBlock_GetSelectionEdge(
        _In_ CTextBlock* pTextBlock,
        _In_ XINT32 iEdgeIndex,
        _Out_ CValue* pTextPointer);

    _Check_return_ HRESULT TextBlock_Select(
        _In_ CTextBlock* pTextBlock,
        _In_ CValue* pAnchorPositionTextPointer,
        _In_ CValue* pMovingPositionTextPointer);

    _Check_return_ HRESULT CRichTextBlock_GetPlainText(
        _In_ CFrameworkElement* pElement,
        _Out_ xstring_ptr* pstrPlainText);

    _Check_return_ HRESULT CRichTextBlock_GetEdge(
        _In_ CFrameworkElement* pElement,
        _In_ XINT32 iEdgeIndex,
        _Out_ CValue* pTextPointer);

    _Check_return_ HRESULT CRichTextBlock_GetSelectionEdge(
        _In_ CRichTextBlock* pRichTextBlock,
        _In_ XINT32 iEdgeIndex,
        _Out_ CValue* pTextPointer);

    _Check_return_ HRESULT CRichTextBlock_Select(
        _In_ CRichTextBlock* pRichTextBlock,
        _In_ CValue* pAnchorPositionTextPointer,
        _In_ CValue* pMovingPositionTextPointer);

    _Check_return_ HRESULT CRichTextBlock_GetTextPositionFromPoint(
        _In_ CFrameworkElement* pElement,
        _In_ XPOINTF point,
        _Out_ CValue* pTextPointer);

    _Check_return_ HRESULT CRichTextBlockOverflow_GetMaster(
        _In_ CRichTextBlockOverflow* pObject,
        _Inout_ CRichTextBlock** ppMaster);

    _Check_return_ HRESULT GetLinkHostFrameworkElement(
        _In_ CInline* pLink,
        _Out_ CValue* pHostElement);

    _Check_return_ HRESULT PrintDocument_MakeDocument(
        CPrintDocument* pPrintDocument,
        void* pDocPackageTarget,
        void* pDocSettings);

    _Check_return_ HRESULT PrintDocument_BeginPreview(
        CPrintDocument* pPrintDocument,
        void* pPreviewPackageTarget);

    _Check_return_ HRESULT PrintDocument_EndPreview(
        CPrintDocument* pPrintDocument);

    _Check_return_ HRESULT PrintDocument_Paginate(
        CPrintDocument* pPrintDocument,
        void* pDocumentSettings);

    _Check_return_ HRESULT PrintDocument_GetPreviewPage(
        CPrintDocument* pPrintDocument,
        XUINT32 desiredJobPage,
        XFLOAT width,
        XFLOAT height);

    _Check_return_ HRESULT ManipulationHandler_NotifyCanManipulateElements(
        _In_ XHANDLE hManipulationHandler,
        _In_ bool fCanManipulateElementsByTouch,
        _In_ bool fCanManipulateElementsNonTouch,
        _In_ bool fCanManipulateElementsWithBringIntoViewport);

    _Check_return_ HRESULT ManipulationHandler_NotifyManipulatableElementChanged(
        _In_ XHANDLE hManipulationHandler,
        _In_opt_ CUIElement* pOldManipulatableElement,
        _In_opt_ CUIElement* pNewManipulatableElement);

    _Check_return_ HRESULT ManipulationHandler_NotifySecondaryContentAdded(
        _In_ XHANDLE hManipulationHandler,
        _In_opt_ CUIElement* pManipulatableElement,
        _In_ CUIElement* pContentElement,
        _In_ XDMContentType contentType);

    _Check_return_ HRESULT ManipulationHandler_NotifySecondaryContentRemoved(
        _In_ XHANDLE hManipulationHandler,
        _In_opt_ CUIElement* pManipulatableElement,
        _In_ CUIElement* pContentElement);

    _Check_return_ HRESULT ManipulationHandler_NotifyViewportChanged(
        _In_ XHANDLE hManipulationHandler,
        _In_ CUIElement* pManipulatedElement,
        _In_ bool fInManipulation,
        _In_ bool fBoundsChanged,
        _In_ bool fTouchConfigurationChanged,
        _In_ bool fNonTouchConfigurationChanged,
        _In_ bool fConfigurationsChanged,
        _In_ bool fChainedMotionTypesChanged,
        _In_ bool fHorizontalOverpanModeChanged,
        _In_ bool fVerticalOverpanModeChanged,
        _Out_ bool* pfConfigurationsUpdated);

    _Check_return_ HRESULT ManipulationHandler_NotifyPrimaryContentChanged(
        _In_ XHANDLE hManipulationHandler,
        _In_ CUIElement* pManipulatedElement,
        _In_ bool fInManipulation,
        _In_ bool fLayoutRefreshed,
        _In_ bool fBoundsChanged,
        _In_ bool fHorizontalChanged,
        _In_ bool fVerticalChanged,
        _In_ bool fZoomFactorBoundaryChanged);

    _Check_return_ HRESULT ManipulationHandler_NotifyPrimaryContentTransformChanged(
        _In_ XHANDLE hManipulationHandler,
        _In_ CUIElement* pManipulatedElement,
        _In_ bool fInManipulation,
        _In_ bool fTranslationXChanged,
        _In_ bool fTranslationYChanged,
        _In_ bool fZoomFactorChanged);

    _Check_return_ HRESULT ManipulationHandler_NotifySnapPointsChanged(
        _In_ XHANDLE hManipulationHandler,
        _In_ CUIElement* pManipulatedElement,
        _In_ XUINT8 motionType);

    _Check_return_ HRESULT ManipulationHandler_GetPrimaryContentTransform(
        _In_ XHANDLE hManipulationHandler,
        _In_ CUIElement* pManipulatedElement,
        _In_ bool fForBringIntoViewport,
        _Out_ XFLOAT& translationX,
        _Out_ XFLOAT& translationY,
        _Out_ XFLOAT& uncompressedZoomFactor,
        _Out_ XFLOAT& zoomFactorX,
        _Out_ XFLOAT& zoomFactorY);

    _Check_return_ HRESULT ManipulationHandler_BringIntoViewport(
        _In_ XHANDLE hManipulationHandler,
        _In_ CUIElement* pManipulatedElement,
        _In_ XRECTF& bounds,
        _In_ XFLOAT translateX,
        _In_ XFLOAT translateY,
        _In_ XFLOAT zoomFactor,
        _In_ bool fTransformIsValid,
        _In_ bool fSkipDuringTouchContact,
        _In_ bool fSkipAnimationWhileRunning,
        _In_ bool fAnimate,
        _In_ bool fApplyAsManip,
        _In_ bool fIsForMakeVisible,
        _Out_ bool* pfHandled);

    _Check_return_ HRESULT ManipulationHandler_SetConstantVelocities(
        _In_ XHANDLE hManipulationHandler,
        _In_ CUIElement* pManipulatedElement,
        _In_ XFLOAT panXVelocity,
        _In_ XFLOAT panYVelocity);

    _Check_return_ HRESULT ManipulationHandler_ProcessInputMessage(
        _In_ XHANDLE hManipulationHandler,
        _In_ CUIElement* pManipulatedElement,
        _In_ bool ignoreFlowDirection,
        _Out_ bool& fHandled);

    _Check_return_ HRESULT StackPanel_GetIrregularSnapPoints(
        _In_ CStackPanel* pStackPanel,
        _In_ bool bHorizontalOrientation,
        _In_ bool bNearAligned,
        _In_ bool bFarAligned,
        _Outptr_opt_result_buffer_(*pcSnapPoints) FLOAT** ppSnapPoints,
        _Out_ uint32_t* pcSnapPoints);

    _Check_return_ HRESULT StackPanel_GetRegularSnapPoints(
        _In_ CStackPanel* pStackPanel,
        _In_ bool bHorizontalOrientation,
        _In_ bool bNearAligned,
        _In_ bool bFarAligned,
        _Out_ XFLOAT* pOffset,
        _Out_ XFLOAT* pInterval);

    _Check_return_ HRESULT StackPanel_SetSnapPointsChangeNotificationsRequirement(
        _In_ CStackPanel* pStackPanel,
        _In_ bool bIsForHorizontalSnapPoints,
        _In_ bool bNotifyChanges);

    _Check_return_ HRESULT Application_JupiterComplete(_In_ CDependencyObject* pDo);

    _Check_return_ HRESULT Storyboard_SetTarget(
        _In_ CTimeline* pTimeline,
        _In_ CDependencyObject* pTarget);

    DirectUI::ClockState Timeline_GetClockState(
        _In_ CTimeline *timeline);

    double Timeline_GetCurrentTime(
        _In_ CTimeline *timeline);

    xref_ptr<CDependencyObject> Timeline_ResolveName(
        _In_ CTimeline *timeline,
        _In_ const xstring_ptr& strName);

    // Raises a drag and drop event of the specified type. See InputManager.ProcessDragDrop.
    _Check_return_ HRESULT DragDrop_RaiseEvent(
        _In_ CContentRoot* contentRoot,
        _In_ CCoreServices* pCore,
        _In_ DirectUI::DragDropMessageType msgType,
        _In_ XPOINTF dropPoint,
        _In_ bool raiseDragEventsSync = false,
        _In_opt_ IInspectable* pWinRtDragInfo = nullptr,
        _In_opt_ IInspectable* pDragDropAsyncOperation = nullptr,
        _Inout_opt_ DirectUI::DataPackageOperation* pAcceptedOperation = nullptr,
        _In_opt_ CDependencyObject* hitTestRoot = nullptr);

    // Checks whether or not the given visual has an active associated drag visual.
    // pVisual: A pointer to the visual (previously passed to DragDrop_CreateDragVisual as pVisual).
    // Returns: TRUE if the visual has an active associated drag visual.
    bool DragDrop_IsVisualActive(
        _In_ CUIElement* pVisual);

    _Check_return_ HRESULT CoreServices_SetCustomResourceLoader(
        _In_ CCoreServices *pCore,
        _In_opt_ ICustomResourceLoader *pLoader);

    _Check_return_ HRESULT CoreServices_GetCustomResourceLoader(
        _In_ CCoreServices *pCore,
        _Outptr_ ICustomResourceLoader **ppLoader);

    _Check_return_ HRESULT CoreServices_TryGetApplicationResource(
        _In_ CCoreServices *pCore,
        _In_ const xstring_ptr& strUri,
        _Outptr_result_maybenull_ IPALMemory **ppMemory,
        _Out_ bool *pfIsBinaryXaml);

    _Check_return_ HRESULT CoreServices_CombineResourceUri(
        _In_ CCoreServices *pCore,
        _In_ const xstring_ptr_view& strBaseUri,
        _In_ const xstring_ptr_view& strUri,
        _Out_ xruntime_string_ptr* pstrCombinedUri);

    _Check_return_ HRESULT CoreServices_GetBaseUri(
        _In_ CCoreServices *pCore,
        _Out_ xstring_ptr* pstrBaseUri);

    _Check_return_ HRESULT Parser_GenerateBinaryXaml(
        _In_                                        CCoreServices  *pCore,
        _In_reads_bytes_(cXamlTextBufferSize)            XBYTE          *pXamlTextBuffer,
        _In_                                        XUINT32         cXamlTextBufferSize,
        _Outptr_result_bytebuffer_(*pcXamlBinaryBufferSize) XBYTE         **ppXamlBinaryBuffer,
        _Out_                                       XUINT32        *pcXamlBinaryBufferSize);

    _Check_return_ HRESULT Timeline_SetAllowDependentAnimations(
        _In_ bool allowDependentAnimations);

    _Check_return_ HRESULT Timeline_GetAllowDependentAnimations(
        _Out_ bool *pAllowDependentAnimations);

    _Check_return_ HRESULT HostProperties_SetInvisibleHitTestMode(
        _In_ CCoreServices* pCore,
        _In_ bool bInvisibleHitTestMode);

    _Check_return_ HRESULT CFrameworkElement_GetLayoutInformation(
        _In_ CFrameworkElement* pElement,
        _Out_ XUINT32* pcFloats,
        _Out_writes_(*pcFloats) XFLOAT** ppFloats);

    _Check_return_ HRESULT CStoryboard_SeekPublic(
        _In_ CStoryboard* pNativeInstance,
        _In_ const CValue& seekTime);

    _Check_return_ HRESULT CStoryboard_SeekAlignedToLastTickPublic(
        _In_ CStoryboard* pNativeInstance,
        _In_ const CValue& seekTime);

    _Check_return_ HRESULT CStoryboard_SetStoryboardStartedCallback(
        _In_ std::function<HRESULT(CDependencyObject* /* storyboard */, CDependencyObject* /* target */)> callback);

    _Check_return_ HRESULT Page_ValidatePropertyIfSwapChainBackgroundPanelChild(
        _In_ CCoreServices* pCore,
        _In_ CUIElement *pUIElement,
        KnownPropertyIndex propertyIndex);

    _Check_return_ HRESULT Page_EnsureIsRootVisualIfSwapChainBackgroundPanelChild(
        _In_ CCoreServices* pCore,
        _In_ CUIElement *pUIElement);

    _Check_return_ HRESULT Page_ValidatePropertiesIfSwapChainBackgroundPanelChild(
        _In_ CCoreServices* pCore,
        _In_ CUIElement *pUIElement);

    _Check_return_ HRESULT CUIElement_CapturePointer(
        _In_ CUIElement* pNativeInstance,
        _In_ CPointer* pValue,
        _Out_ bool* pReturnValue);

    _Check_return_ HRESULT CDataTemplate_LoadContent(
        _In_ CDataTemplate* pNativeInstance,
        _Outptr_ CDependencyObject** returnValue);

    // Build a LayoutTransition visual
    // pTargetUIElement: A pointer to the visual content to be displayed on the parent layer. This visual must be in the visual tree.
    //                   It will be hidden from view until LayoutTransitionElement_Destroy is called. However, the visual will continue to
    //                   take up space in the original layout.
    // pParentElement: An optional pointer to the parent of the LayoutTransition. If null, the transition will be displayed
    //                 in the TransitionRoot layer
    // ppTransitionElement: This buffer will be filled with the address of the element on the parent layer. Internally, this is
    //               a LayoutTransitionElement.
    _Check_return_ HRESULT LayoutTransitionElement_Create(
        _In_ CCoreServices* pCore,
        _In_ CUIElement* pTargetUIElement,
        _In_opt_ CUIElement* pParentElement,
        _In_ bool isAbsolutelyPositioned,
        _Outptr_ CUIElement** ppTransitionElement);

    // Destroys a LayoutTransition visual, and makes the visual content visible again in its original position.
    // pTargetUIElement: A pointer to the visual that is being displayed in the parent layer (previously passed to LayoutTransitionElement_Create as pTargetUIElement).
    // pTransitionElement: The visual that was returned from a previous call to LayoutTransitionElement_Create.
    _Check_return_ HRESULT LayoutTransitionElement_Destroy(
        _In_ CCoreServices* pCore,
        _In_opt_ CUIElement* pTargetUIElement,
        _In_opt_ CUIElement* pParentElement,
        _In_opt_ CUIElement* pTransitionElement);

    _Check_return_ HRESULT LayoutTransitionElement_SetDestinationOffset(
        _In_ CUIElement* pTransitionElement,
        _In_ XFLOAT x,
        _In_ XFLOAT y);

    _Check_return_ HRESULT RootVisual_TransformToVisual(
        _In_ CCoreServices* pCore,
        _In_opt_ CUIElement* pRelativeElement,
        _Outptr_ CGeneralTransform** returnValue);
}
