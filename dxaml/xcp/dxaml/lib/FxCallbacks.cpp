// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "AutomationPeer.g.h"
#include "AutoSuggestBox.g.h"
#include "BitmapSource.g.h"
#include "CommandBar.g.h"
#include "CommandBarElementCollection.g.h"
#include "ContentControl.g.h"
#include "ContentPresenter.g.h"
#include "ContentPresenter.g.h"
#include "DefaultStyles.h"
#include "DragDropInternal.h"
#include "DragEventArgs.g.h"
#include "FlyoutBase.g.h"
#include "FrameworkApplication_partial.h"
#include "FrameworkElement.g.h"
#include "Hub.g.h"
#include "HubSectionCollection.g.h"
#include "Hyperlink.g.h"
#include "Image.g.h"
#include "ISupportInitialize.g.h"
#include "ItemContainerGenerator.g.h"
#include "ItemsPresenter.g.h"
#include "JupiterControl.h"
#include "JupiterWindow.h"
#include "MenuFlyout.g.h"
#include "MenuFlyoutItem.g.h"
#include "MenuFlyoutItemBaseCollection.g.h"
#include "MenuFlyoutPresenter.g.h"
#include "Page.g.h"
#include "PasswordBox.g.h"
#include "Popup.g.h"
#include "ResourceDictionary_partial.h"
#include "RichEditBox.g.h"
#include "StaggerFunctionBase.g.h"
#include "Style.g.h"
#include "SwapChainPanel.g.h"
#include "TemplateBindingExpression.h"
#include "TextBox.g.h"
#include "TextBoxView.g.h"
#include "TextBlock.g.h"
#include "RichTextBlock.g.h"
#include "Timeline.g.h"
#include "TimelineCollection.g.h"
#include "ToolTipService.g.h"
#include "Transition.g.h"
#include "UserControl.g.h"
#include "Window.g.h"
#include "XamlCompositionBrushBase.g.h"
#include "XamlParserCallbacks.h"
#include "KeyboardAccelerator.g.h"
#include "KeyboardAcceleratorCollection.g.h"
#include "KeyboardAcceleratorInvokedEventArgs.g.h"
#include "InteractionCollection.h"
#include "IApplicationBarService.h"
#include "BuildTreeService.g.h"
#include "BudgetManager.g.h"
#include "XamlIslandRoot.g.h"
#include "CaretBrowsingDialog.h"
#include "TextControlFlyoutHelper.h"
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include "XamlRoot_Partial.h"
#include "Button.g.h"
#include "FlyoutPresenter_Partial.h"
#include "ElementSoundPlayerService_Partial.h"
#include <XamlTypeTokens.h>

#include <Microsoft.Windows.ApplicationModel.Resources.h>

using namespace DirectUI;
using namespace DirectUISynonyms;

class AgCoreCallbacks
{
public:
    AgCoreCallbacks() = delete;

    static _Check_return_ HRESULT FireEvent(
        _In_ CDependencyObject* pListener,
        _In_ KnownEventIndex eventId,
        _In_ CDependencyObject* pSender,
        _In_ CEventArgs* pArgs,
        _In_ XUINT32 flags)
    {
        return DXamlCore::GetCurrent()->FireEvent(pListener, eventId, pSender, pArgs, flags);
    }

    static _Check_return_ HRESULT RaiseEvent(
        _In_ CDependencyObject* target,
        _In_ DirectUI::ManagedEvent eventId,
        _In_ CEventArgs* coreEventArgs)
    {
        return DXamlCore::GetCurrent()->RaiseEvent(
            target,
            coreEventArgs,
            eventId);
    }

    // Create a framework peer for a native object.
    static _Check_return_ HRESULT CreateManagedPeer(
        _In_ CDependencyObject* element,
        _In_ KnownTypeIndex typeIndex,
        _In_ bool fPeggedNoRef,
        _In_ bool fPeggedRef,
        _In_ bool isShutdownException)
    {
        HRESULT hr = S_OK;
        DependencyObject* pObject = NULL;
        KnownTypeIndex hClass = typeIndex;

        // Validate element since we're ignoring failures (and hence validation) in
        // the GetPeer call below
        IFCPTR(element);

        // We're not checking the GetPeer call with IFC because there are
        // still a few types we don't wrap and can't create peers for (and
        // failing in this method would tear everything down)
        if (SUCCEEDED(DXamlCore::GetCurrent()->GetPeer(element, hClass, &pObject)))
        {
            if (fPeggedNoRef)
            {
                pObject->PegNoRef();
            }

            if (fPeggedRef)
            {
                pObject->UpdatePegWithPossibleShutdownException(true, !!isShutdownException);
            }
        }

    Cleanup:
        ctl::release_interface(pObject);
        RRETURN(hr);
    }

    static void PegManagedPeerNoRef(
        _In_ CDependencyObject* element)
    {
        DependencyObject* pDO = NULL;

        if (SUCCEEDED(DXamlCore::GetCurrent()->TryGetPeer(element, &pDO)))
        {
            if (pDO)
            {
                pDO->PegNoRef();
            }
        }

        ctl::release_interface(pDO);
    }

    static void UnpegManagedPeerNoRef(
        _In_ CDependencyObject* element)
    {
        DependencyObject* pDO = NULL;

        if (SUCCEEDED(DXamlCore::GetCurrent()->TryGetPeer(element, &pDO)))
        {
            if (pDO)
            {
                pDO->UnpegNoRef();
            }
        }

        ctl::release_interface(pDO);
    }

    static void TryPegPeer(
        _In_ CDependencyObject* element,
        _Out_ bool *pPegged,
        _Out_ bool *pIsPendingDelete)
    {
        DependencyObject* pDO = NULL;

        *pPegged = false;
        *pIsPendingDelete = false;

        if (SUCCEEDED(DXamlCore::GetCurrent()->TryGetPeer(element, pIsPendingDelete, &pDO)))
        {
            if (pDO)
            {
                pDO->UpdatePegWithPossibleShutdownException(true, false);
                *pPegged = true;
            }
        }

        ctl::release_interface(pDO);
    }

    static _Check_return_ HRESULT AreObjectsOfSameType(
        _In_ CDependencyObject* nativeObject1,
        _In_ CDependencyObject* nativeObject2,
        _Out_ bool* areEqual)
    {
        HRESULT hr = S_OK;
        DependencyObject* pDO1 = NULL;
        DependencyObject* pDO2 = NULL;

        IInspectable *pValue1 = NULL;
        IInspectable *pValue2 = NULL;

        const CClassInfo* pType1 = NULL;
        const CClassInfo* pType2 = NULL;

        DXamlCore* pCore = DXamlCore::GetCurrent();
        IFC(pCore->GetPeer(nativeObject1, &pDO1));
        IFC(pCore->GetPeer(nativeObject2, &pDO2));

        CValueBoxer::UnwrapExternalObjectReferenceIfPresent(ctl::as_iinspectable(pDO1), &pValue1);
        CValueBoxer::UnwrapExternalObjectReferenceIfPresent(ctl::as_iinspectable(pDO2), &pValue2);

        IFC(DirectUI::MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(pValue1, &pType1));
        IFC(DirectUI::MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(pValue2, &pType2));

        *areEqual = pType1 == pType2;

    Cleanup:
        ctl::release_interface(pDO1);
        ctl::release_interface(pDO2);
        ReleaseInterface(pValue1);
        ReleaseInterface(pValue2);
        RRETURN(hr);
    }

    // Load Theme Resources
    static _Check_return_ HRESULT LoadThemeResources()
    {
        DXamlCore* pCore = DXamlCore::GetCurrent();
        IFCCATASTROPHIC_RETURN(pCore);

        auto defaultStyles = pCore->m_pDefaultStyles;
        if (defaultStyles) // This can be null during XamlCore shutdown
        {
            IFC_RETURN(defaultStyles->GetStyleCache()->LoadThemeResources());
        }

        return S_OK;
    }

    // Gets a Style corresponding to the type of the provided object.
    static _Check_return_ HRESULT GetBuiltInStyle(
        _In_ CDependencyObject* nativeTarget,
        _Out_ CStyle** nativeStyle)
    {
        ctl::ComPtr<DependencyObject> dobj;
        ctl::ComPtr<Style> style;

        IFCPTR_RETURN(nativeStyle);
        *nativeStyle = nullptr;

        // Get the framework peer
        DXamlCore* pCore = DXamlCore::GetCurrent();
        if (SUCCEEDED(pCore->GetPeer(nativeTarget, &dobj)))
        {
            IFC_RETURN(pCore->m_pDefaultStyles->GetDefaultStyleByKey(dobj.Get(), &style));

            if (style)
            {
                *nativeStyle = static_cast<CStyle*>(style->GetHandle());
                IFC_RETURN(CoreImports::DependencyObject_AddRef(*nativeStyle));
            }
        }

        return S_OK;
    }

    static _Check_return_ HRESULT FrameworkCallbacks_SetDataContext(
        _In_ CFrameworkElement* pElement,
        _In_ CValue* pValue)

    {
        HRESULT hr = S_OK;
        DependencyObject *pDO = NULL;
        IFrameworkElement *pFE = NULL;
        IInspectable *pUnboxedValue = NULL;

        IFC(CValueBoxer::UnboxObjectValue(pValue, NULL, &pUnboxedValue));

        IFC(DXamlCore::GetCurrent()->GetPeer(pElement, &pDO));

        IFC(ctl::do_query_interface(pFE, pDO));

        IFC(pFE->put_DataContext(pUnboxedValue));

    Cleanup:
        ReleaseInterface(pUnboxedValue);
        ReleaseInterface(pFE);
        ctl::release_interface(pDO);

        RRETURN(hr);
    }

    static _Check_return_ HRESULT FrameworkCallbacks_ClearDataContext(
        _In_ CFrameworkElement* pElement)
    {
        HRESULT hr = S_OK;
        DependencyObject *pDO = NULL;

        IFC(DXamlCore::GetCurrent()->GetPeer(pElement, &pDO));
        IFC(pDO->ClearValue(
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_DataContext)));

    Cleanup:
        ctl::release_interface(pDO);
        RRETURN(hr);
    }

    static _Check_return_ HRESULT FrameworkCallbacks_SupportInitializeEndInit(
        _In_ CDependencyObject* nativeTarget,
        _In_ const std::shared_ptr<::XamlServiceProviderContext>& context)
    {
        ctl::ComPtr<DependencyObject> spDO;
        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &spDO));

        ctl::ComPtr<IInspectable> spObject;
        CValueBoxer::UnwrapExternalObjectReferenceIfPresent(ctl::as_iinspectable(spDO.Get()), &spObject);

        auto spInitialize = spObject.AsOrNull<ISupportInitialize>();
        if (spInitialize)
        {
            IFC_RETURN(spInitialize->EndInit(context.get()));
        }

        return S_OK;
    }

    static _Check_return_ HRESULT FrameworkCallbacks_GetCustomTypeIDFromObject(
        _In_ CDependencyObject* target,
        _Out_ KnownTypeIndex* typeID)
    {
        HRESULT hr = S_OK;
        ctl::ComPtr<DependencyObject> spDO;
        const CClassInfo* pType = nullptr;

        IFC(DXamlCore::GetCurrent()->GetPeer(target, &spDO));

        IFC(MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(ctl::as_iinspectable(spDO.Get()), &pType));
        IFCPTR(pType);

        *typeID = pType->GetIndex();

    Cleanup:
        RRETURN(hr);
    }

    static _Check_return_ HRESULT FrameworkCallbacks_GetCustomTypeFullName(
        _In_ CDependencyObject* target,
        _Out_ xstring_ptr* pstrFullName)
    {
        HRESULT hr = S_OK;
        ctl::ComPtr<DependencyObject> spDO;
        const CClassInfo* pType = nullptr;

        pstrFullName->Reset();

        IFCPTR(target);

        IFC(DXamlCore::GetCurrent()->GetPeer(target, &spDO));

        // Only retrieve the type name for custom types.
        if (spDO->IsComposed())
        {
            IFC(MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(ctl::as_iinspectable(spDO.Get()), &pType));
            *pstrFullName = pType->GetFullName();
        }

    Cleanup:
        RRETURN(hr);
    }

    static _Check_return_ HRESULT FrameworkCallbacks_GetCustomPropertyValue(
        _In_ CDependencyObject* pNativeTarget,
        _In_ const CDependencyProperty* pDP,
        _Out_ CValue* pPropertyValue)
    {
        HRESULT hr = S_OK;
        ctl::ComPtr<DependencyObject> spDO;
        ctl::ComPtr<IInspectable> spValue;
        ctl::ComPtr<DependencyObject> spMOR;
        BoxerBuffer buffer;

        IFC(DXamlCore::GetCurrent()->GetPeer(pNativeTarget, &spDO));

        IFC(spDO->GetValue(pDP, &spValue));
        IFC(CValueBoxer::BoxObjectValue(pPropertyValue, pDP->GetPropertyType(), spValue.Get(), &buffer, &spMOR));

        // When a ExternalObjectReference is created, it gets addref'ed and returned to the caller
        // which needs to release it once it is done using it.
        if (spMOR != nullptr && ctl::iinspectable_cast(spMOR.Get()) != spValue.Get())
        {
            ASSERT(pPropertyValue->AsObject() == spMOR->GetHandle());
            spMOR.Detach();
        }

    Cleanup:
        RRETURN(hr);
    }

    static _Check_return_ HRESULT SetCustomPropertyValue(
            _In_ CDependencyObject* pNativeTarget,
            _In_ const CDependencyProperty* pDP,
            _In_ CValue* pPropertyValue,
            _In_ bool fAnimatedValue)
    {
        HRESULT hr = S_OK;
        ctl::ComPtr<DependencyObject> spDO;
        ctl::ComPtr<IInspectable > spValue;

        IFC(DXamlCore::GetCurrent()->GetPeer(pNativeTarget, &spDO));

        IFC(CValueBoxer::UnboxObjectValue(pPropertyValue, pDP->GetPropertyType(), &spValue));
        if (fAnimatedValue)
        {
            ASSERT(false);
            IFC(E_NOTIMPL);
        }
        else
        {
            IFC(spDO->SetValue(pDP, spValue.Get()));
        }

    Cleanup:
        RRETURN(hr);
    }

    static _Check_return_ HRESULT FrameworkCallbacks_SetCustomPropertyValue(
        _In_ CDependencyObject* pNativeTarget,
        _In_ const CDependencyProperty* pDP,
        _In_ CValue* pPropertyValue,
        _In_ KnownTypeIndex typeIndex)
    {
        RRETURN(SetCustomPropertyValue(
            pNativeTarget,
            pDP,
            pPropertyValue,
            false /*fAnimatedValue*/));
    }

    static _Check_return_ HRESULT FrameworkCallbacks_Hyperlink_OnClick(
        _In_ CDependencyObject* nativeHost
        )
    {
        HRESULT hr = S_OK;
        DependencyObject *pDO = nullptr;
        IHyperlink *pHyperlink = nullptr;
        Hyperlink *pHyperlinkPrivate = nullptr;

        IFC(DXamlCore::GetCurrent()->GetPeer(nativeHost, &pDO));
        IFC(ctl::do_query_interface(pHyperlink, pDO));
        pHyperlinkPrivate = static_cast<Hyperlink*>(pHyperlink);

        IFC(pHyperlinkPrivate->OnClick());

    Cleanup:
        ReleaseInterface(pHyperlink);
        ctl::release_interface(pDO);
        return hr;
    }

    // redirect to instance
    static _Check_return_ HRESULT FrameworkCallbacks_BudgetService_StoreFrameTime(_In_ bool isStartOfTick)
    {
        HRESULT hr = S_OK;

        ctl::ComPtr<BudgetManager> spBudgetManager;
        IFC(DXamlCore::GetCurrent()->GetBudgetManager(spBudgetManager));

        IFC(spBudgetManager->StoreFrameTime(isStartOfTick));

    Cleanup:
        RRETURN(hr);
    }

    static _Check_return_ HRESULT FrameworkCallbacks_IsAnimationEnabled(
        _Out_ bool* pIsAnimationEnabled)
    {
        *pIsAnimationEnabled = false;
        bool result = false;

        IFC_RETURN(DXamlCore::GetCurrent()->IsAnimationEnabled(&result));

        *pIsAnimationEnabled = result;

        return S_OK;
    }

    // redirect to instance
    static _Check_return_ HRESULT FrameworkCallbacks_PhasedWorkDistributor_PerformWork(_Out_ bool* pWorkleft)
    {
        HRESULT hr = S_OK;
        bool workLeft = false;
        ctl::ComPtr<BuildTreeService> spBuildTreeService;
        IFC(DXamlCore::GetCurrent()->GetBuildTreeService(spBuildTreeService));

        IFC(spBuildTreeService->BuildTrees(&workLeft));

        *pWorkleft = workLeft; // The core will always call us, but it needs to schedule a new tick if we still have work to do

    Cleanup:
        RRETURN(hr);
    }

    static _Check_return_ HRESULT FrameworkCallbacks_IsDXamlCoreShuttingDown(
        _Out_ bool* pIsDXamlCoreShuttingDown)
    {
        *pIsDXamlCoreShuttingDown = DXamlCore::IsShuttingDownStatic();

        return S_OK;
    }

    static _Check_return_ HRESULT ReportUnhandledError(
        _In_ XUINT32 errorCode)
    {
        HRESULT hr = S_OK;

        IFC(ErrorHelper::ReportUnhandledError((HRESULT)errorCode));

    Cleanup:
        RRETURN(hr);
    }

    static _Check_return_ HRESULT GetExceptionText(_Outptr_result_buffer_(*textLength + 1) WCHAR** text, _Out_ XUINT32* textLength)
    {
        HRESULT hr = S_OK;
        *textLength = 0;
        *text = NULL;

        RRETURN(hr);
    }


    //------------------------------------------------------------------------
    //
    //  Synopsis:
    //     Only ThemeAnimations have DynamicTimelines as peer. All others return S_OK;
    //
    // Allows the peer to add timelines to a collection.
    //
    //------------------------------------------------------------------------
    static _Check_return_ HRESULT GetDynamicTimelines(_In_ CDependencyObject* pNativeDynamicTimeline, _In_ bool bGenerateSteadyStateOnly, _In_ CValue* pNativeTimelineCollection)
    {
        HRESULT hr = S_OK;

        DependencyObject* pPeer = NULL;
        Timeline* pThemeAnimation = NULL;     // currently only ThemeAnimation is supported
        wfc::IVector<xaml_animation::Timeline*>* pTimelines = NULL;

        // peer
        IFC(DXamlCore::GetCurrent()->GetPeer(pNativeDynamicTimeline, &pPeer));
        pThemeAnimation = static_cast<Timeline*>(pPeer);

        // children collection
        IFC(CValueBoxer::UnboxObjectValue(pNativeTimelineCollection, MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::TimelineCollection), __uuidof(wfc::IVector<xaml_animation::Timeline*>), reinterpret_cast<void**>(&pTimelines)));

        // execute
        IFC(pThemeAnimation->CreateTimelines(!!bGenerateSteadyStateOnly, pTimelines));

        Cleanup:
        ctl::release_interface(pThemeAnimation);
        ReleaseInterface(pTimelines);
        RRETURN(hr);
    }

    // This callback is here for Jupiter to process things once per
    // pass through the message pump.  Not to be confused with the CompositionTarget
    // draw callback to user code.
    // This was introduced so we had a place on the UI thread to release
    // our thread affinitized UI objects, but the name and callback semantic is more general
    // on the assumption it will be generally useful.
    static void PerFrameCallback()
    {
        DXamlCore* pCore = DXamlCore::GetCurrentNoCreate();
        if (pCore && pCore->IsInitialized())
        {
            pCore->ReleaseQueuedObjects( /*bSync*/ false );
        }

        #if DBG
        IGNOREHR(static_cast<ReferenceTrackerManager*>(ReferenceTrackerManager::GetNoRef())->RunValidation());
        #endif
    }
};


//
// FxCallback implementations
//

namespace FxCallbacks
{
    _Check_return_ HRESULT DependencyObject_NotifyPropertyChanged(_In_ CDependencyObject* pDO, _In_ const PropertyChangedParams& args)
        { return DirectUI::DependencyObject::NotifyPropertyChanged(pDO, args); }

    _Check_return_ HRESULT DependencyObject_EnterImpl(_In_ CDependencyObject* nativeDO, _In_ CDependencyObject* nativeNamescopeOwner, _In_ bool bLive, _In_ bool bSkipNameRegistration, _In_ bool bCoercedIsEnabled, _In_ bool bUseLayoutRounding)
        { return DirectUI::DependencyObject::EnterImpl(nativeDO, nativeNamescopeOwner, bLive, bSkipNameRegistration, bCoercedIsEnabled, bUseLayoutRounding); }

    _Check_return_ HRESULT DependencyObject_LeaveImpl(_In_ CDependencyObject* nativeDO, _In_ CDependencyObject* nativeNamescopeOwner, _In_ bool bLive, _In_ bool bSkipNameRegistration, _In_ bool bCoercedIsEnabled, _In_ bool bVisualTreeBeingReset)
        { return DirectUI::DependencyObject::LeaveImpl(nativeDO, nativeNamescopeOwner, bLive, bSkipNameRegistration, bCoercedIsEnabled, bVisualTreeBeingReset); }

    _Check_return_ HRESULT DependencyObject_SetBinding(_In_ CDependencyObject* nativeTarget, _In_ KnownPropertyIndex propertyId, _In_ CDependencyObject* pBinding)
        { return DirectUI::DependencyObject::SetBindingCallback(nativeTarget, propertyId, pBinding); }

    _Check_return_ HRESULT DependencyObject_SetPeerReferenceToProperty(_In_ CDependencyObject* nativeTarget, _In_ const CDependencyProperty* pDP, _In_ const CValue& value, _In_ bool bPreservePegNoRef, _In_opt_ IInspectable* pNewValueOuter, _Outptr_opt_result_maybenull_ IInspectable** ppOldValueOuter)
        { return DependencyObject::SetPeerReferenceToPropertyCallback(nativeTarget, pDP, value, bPreservePegNoRef, pNewValueOuter, ppOldValueOuter); }

    _Check_return_ HRESULT DependencyObject_AddPeerReferenceToItem(_In_ CDependencyObject* nativeOwner, _In_ CDependencyObject* nativeTarget)
        { return DependencyObject::AddPeerReferenceToItemCallback(nativeOwner, nativeTarget); }

    _Check_return_ HRESULT DependencyObject_RemovePeerReferenceToItem(_In_ CDependencyObject* nativeOwner, _In_ CDependencyObject* nativeTarget)
        { return DependencyObject::RemovePeerReferenceToItemCallback(nativeOwner, nativeTarget); }

    _Check_return_ HRESULT DependencyObject_OnCollectionChanged(_In_ CDependencyObject* nativeOwner, _In_ XUINT32 nChangeType, _In_ XUINT32 nIndex)
        { return DependencyObject::OnCollectionChangedCallback(nativeOwner, nChangeType, nIndex); }

    _Check_return_ HRESULT DependencyObject_GetDefaultValue(_In_ CDependencyObject* pReferenceObject, _In_ const CDependencyProperty* pDP, _Out_ CValue* pValue)
        { return DirectUI::DependencyObject::GetDefaultValueCallback(pReferenceObject, pDP, pValue); }

    _Check_return_ HRESULT DependencyObject_NotifyDeferredElementStateChanged(_In_ CDependencyObject* parent, _In_ KnownPropertyIndex propertyIndex, _In_ DeferredElementStateChange state, _In_ UINT32 collectionIndex, _In_ CDependencyObject* realizedElement)
        { return DirectUI::DependencyObject::NotifyDeferredElementStateChangedStatic(parent, propertyIndex, state, collectionIndex, realizedElement); }

    _Check_return_ HRESULT DependencyObject_RefreshExpressionsOnThemeChange(_In_ CDependencyObject* nativeDO, _In_ Theming::Theme theme, _In_ bool forceRefresh)
        { return DirectUI::DependencyObject::RefreshExpressionsOnThemeChange(nativeDO, theme, forceRefresh); }

    _Check_return_ HRESULT FlyoutPresenter_GetParentMenuFlyoutSubItem(_In_ CDependencyObject* nativeDO, _Outptr_ CDependencyObject** ppMenuFlyoutSubItem)
        { return DirectUI::MenuFlyoutPresenter::GetParentMenuFlyoutSubItem(nativeDO, ppMenuFlyoutSubItem); }

    _Check_return_ HRESULT UIElement_OnCreateAutomationPeer(_In_ CDependencyObject* nativeTarget, _Out_ CAutomationPeer** returnAP)
        { return DirectUI::UIElement::OnCreateAutomationPeer(nativeTarget, returnAP); }

    _Check_return_ HRESULT UIElement_GetCanManipulateElements(_In_ CUIElement* nativeTarget, _Out_ bool* fCanManipulateElementsByTouch, _Out_ bool* fCanManipulateElementsNonTouch, _Out_ bool* fCanManipulateElementsWithBringIntoViewport)
        { return DirectUI::UIElement::GetCanManipulateElements(nativeTarget, fCanManipulateElementsByTouch, fCanManipulateElementsNonTouch, fCanManipulateElementsWithBringIntoViewport); }

    _Check_return_ HRESULT UIElement_SetManipulationHandler(_In_ CUIElement* nativeTarget, _In_opt_ void* nativeManipulationHandler)
        { return DirectUI::UIElement::SetManipulationHandler(nativeTarget, nativeManipulationHandler); }

    _Check_return_ HRESULT UIElement_SetManipulationHandlerWantsNotifications(_In_ CUIElement* nativeTarget, _In_ CUIElement* nativeManipulatedElement, _In_ bool fWantsNotifications)
        { return DirectUI::UIElement::SetManipulationHandlerWantsNotifications(nativeTarget, nativeManipulatedElement, fWantsNotifications); }

    _Check_return_ HRESULT UIElement_SetPointedElement(_In_ CUIElement* nativeTarget, _In_ CDependencyObject* nativePointedElement)
        { return DirectUI::UIElement::SetPointedElement(nativeTarget, nativePointedElement); }

    _Check_return_ HRESULT UIElement_GetManipulatedElement(_In_ CUIElement* nativeTarget, _In_ CDependencyObject* nativePointedElement, _In_opt_ CUIElement* nativeChildElement, _Out_ CUIElement** nativeManipulatedElement)
        { return DirectUI::UIElement::GetManipulatedElement(nativeTarget, nativePointedElement, nativeChildElement, nativeManipulatedElement); }

    _Check_return_ HRESULT UIElement_GetManipulationViewport(_In_ CUIElement* nativeTarget, _In_ CUIElement* nativeManipulatedElement, _Out_opt_ XRECTF* pBounds, _Out_opt_ CMILMatrix* pInputTransform, _Out_opt_ XUINT32* pTouchConfiguration, _Out_opt_ XUINT32* pNonTouchConfiguration, _Out_opt_ XUINT32* pBringIntoViewportConfiguration, _Out_opt_ XUINT32* pHorizontalOverpanMode, _Out_opt_ XUINT32* pVerticalOverpanMode, _Out_opt_ XUINT8* pcConfigurations, _Outptr_result_buffer_maybenull_(* pcConfigurations) XUINT32** ppConfigurations, _Out_opt_ XUINT32* pChainedMotionTypes)
        { return DirectUI::UIElement::GetManipulationViewport(nativeTarget, nativeManipulatedElement, pBounds, pInputTransform, pTouchConfiguration, pNonTouchConfiguration, pBringIntoViewportConfiguration, pHorizontalOverpanMode, pVerticalOverpanMode, pcConfigurations, ppConfigurations, pChainedMotionTypes); }

    _Check_return_ HRESULT UIElement_GetManipulationPrimaryContent(_In_ CUIElement* nativeTarget, _In_ CUIElement* nativeManipulatedElement, _Out_opt_ XSIZEF* pOffsets, _Out_opt_ XRECTF* pBounds, _Out_opt_ XUINT32* pHorizontalAlignment, _Out_opt_ XUINT32* pVerticalAlignment, _Out_opt_ XFLOAT* pMinZoomFactor, _Out_opt_ XFLOAT* pMaxZoomFactor, _Out_opt_ bool* pfIsHorizontalStretchAlignmentTreatedAsNear, _Out_opt_ bool* pfIsVerticalStretchAlignmentTreatedAsNear, _Out_opt_ bool* pfIsLayoutRefreshed)
        { return DirectUI::UIElement::GetManipulationPrimaryContent(nativeTarget, nativeManipulatedElement, pOffsets, pBounds, pHorizontalAlignment, pVerticalAlignment, pMinZoomFactor, pMaxZoomFactor, pfIsHorizontalStretchAlignmentTreatedAsNear, pfIsVerticalStretchAlignmentTreatedAsNear, pfIsLayoutRefreshed); }

    _Check_return_ HRESULT UIElement_GetManipulationSecondaryContent(_In_ CUIElement* nativeTarget, _In_ CUIElement* nativeContentElement, _Out_ XSIZEF* pOffsets)
        { return DirectUI::UIElement::GetManipulationSecondaryContent(nativeTarget, nativeContentElement, pOffsets); }

    _Check_return_ HRESULT UIElement_GetManipulationPrimaryContentTransform(_In_ CUIElement* nativeTarget, _In_ CUIElement* nativeManipulatedElement, _In_ bool fInManipulation, _In_ bool fForInitialTransformationAdjustment, _In_ bool fForMargins, _Out_opt_ XFLOAT* pTranslationX, _Out_opt_ XFLOAT* pTranslationY, _Out_opt_ XFLOAT* pZoomFactor)
        { return DirectUI::UIElement::GetManipulationPrimaryContentTransform(nativeTarget, nativeManipulatedElement, fInManipulation, fForInitialTransformationAdjustment, fForMargins, pTranslationX, pTranslationY, pZoomFactor); }

    _Check_return_ HRESULT UIElement_GetManipulationSecondaryContentTransform(_In_ CUIElement* nativeTarget, _In_ CUIElement* nativeContentElement, _Out_ XFLOAT* pTranslationX, _Out_ XFLOAT* pTranslationY, _Out_ XFLOAT* pZoomFactor)
        { return DirectUI::UIElement::GetManipulationSecondaryContentTransform(nativeTarget, nativeContentElement, pTranslationX, pTranslationY, pZoomFactor); }

    _Check_return_ HRESULT UIElement_GetManipulationSnapPoints(_In_ CUIElement* nativeTarget, _In_ CUIElement* nativeManipulatedElement, _In_ XUINT32 motionType, _Out_ bool* pfAreSnapPointsOptional, _Out_ bool* pfAreSnapPointsSingle, _Out_ bool* pfAreSnapPointsRegular, _Out_ XFLOAT* pRegularOffset, _Out_ XFLOAT* pRegularInterval, _Out_ XUINT32* pcIrregularSnapPoints, _Outptr_result_buffer_(* pcIrregularSnapPoints) XFLOAT** ppIrregularSnapPoints, _Out_ XUINT32* pSnapCoordinate)
        { return DirectUI::UIElement::GetManipulationSnapPoints(nativeTarget, nativeManipulatedElement, motionType, pfAreSnapPointsOptional, pfAreSnapPointsSingle, pfAreSnapPointsRegular, pRegularOffset, pRegularInterval, pcIrregularSnapPoints, ppIrregularSnapPoints, pSnapCoordinate); }

    _Check_return_ HRESULT UIElement_NotifyManipulatabilityAffectingPropertyChanged(_In_ CUIElement* nativeTarget, _In_ bool fIsInLiveTree)
        { return DirectUI::UIElement::NotifyManipulatabilityAffectingPropertyChanged(nativeTarget, fIsInLiveTree); }

    _Check_return_ HRESULT UIElement_NotifyContentAlignmentAffectingPropertyChanged(_In_ CUIElement* nativeTarget, _In_ CUIElement* nativeManipulatedElement, _In_ bool fIsForHorizontalAlignment, _In_ bool fIsForStretchAlignment, _In_ bool fIsStretchAlignmentTreatedAsNear)
        { return DirectUI::UIElement::NotifyContentAlignmentAffectingPropertyChanged(nativeTarget, nativeManipulatedElement, fIsForHorizontalAlignment, fIsForStretchAlignment, fIsStretchAlignmentTreatedAsNear); }

    _Check_return_ HRESULT UIElement_NotifyManipulationProgress(_In_ CUIElement* nativeTarget, _In_ CUIElement* nativeManipulatedElement, _In_ XUINT32 state, _In_ XFLOAT xCumulativeTranslation, _In_ XFLOAT yCumulativeTranslation, _In_ XFLOAT zCumulativeFactor, _In_ XFLOAT xInertiaEndTranslation, _In_ XFLOAT yInertiaEndTranslation, _In_ XFLOAT zInertiaEndFactor, _In_ XFLOAT xCenter, _In_ XFLOAT yCenter, _In_ bool fIsInertiaEndTransformValid, _In_ bool fIsInertial, _In_ bool fIsTouchConfigurationActivated, _In_ bool fIsBringIntoViewportConfigurationActivated)
        { return DirectUI::UIElement::NotifyManipulationProgress(nativeTarget, nativeManipulatedElement, state, xCumulativeTranslation, yCumulativeTranslation, zCumulativeFactor, xInertiaEndTranslation, yInertiaEndTranslation, zInertiaEndFactor, xCenter, yCenter, fIsInertiaEndTransformValid, fIsInertial, fIsTouchConfigurationActivated, fIsBringIntoViewportConfigurationActivated); }

    _Check_return_ HRESULT UIElement_NotifyManipulationStateChanged(_In_ CUIElement* nativeTarget, _In_ XUINT32 state)
        { return DirectUI::UIElement::NotifyManipulationStateChanged(nativeTarget, state); }

    _Check_return_ HRESULT UIElement_IsBringIntoViewportNeeded(_In_ CUIElement* nativeTarget, _Out_ bool * bringIntoViewportNeeded)
        { return DirectUI::UIElement::IsBringIntoViewportNeeded(nativeTarget, bringIntoViewportNeeded); }

    _Check_return_ HRESULT UIElement_NotifyBringIntoViewportNeeded(_In_ CUIElement* nativeTarget, _In_ CUIElement* nativeManipulatedElement, _In_ XFLOAT translationX, _In_ XFLOAT translationY, _In_ XFLOAT zoomFactor, _In_ bool fTransformIsValid, _In_ bool fTransformIsInertiaEnd)
        { return DirectUI::UIElement::NotifyBringIntoViewportNeeded(nativeTarget, nativeManipulatedElement, translationX, translationY, zoomFactor, fTransformIsValid, fTransformIsInertiaEnd); }

    _Check_return_ HRESULT UIElement_NotifySnapPointsChanged(_In_ CUIElement* nativeTarget, _In_ bool fHorizontalSnapPoints)
        { return DirectUI::UIElement::NotifySnapPointsChanged(nativeTarget, fHorizontalSnapPoints); }

    _Check_return_ HRESULT UIElement_NotifyCanDragChanged(_In_ CUIElement* nativeTarget, _In_ bool fCanDrag)
        { return DirectUI::UIElement::NotifyCanDragChanged(nativeTarget, fCanDrag); }

    _Check_return_ HRESULT UIElement_OnDirectManipulationDraggingStarted(_In_ CUIElement* nativeTarget)
        { return DirectUI::UIElement::OnDirectManipulationDraggingStarted(nativeTarget); }

    _Check_return_ HRESULT UIElement_NotifyInputPaneStateChange(_In_ CUIElement* nativeTarget, _In_ DirectUI::InputPaneState inputPaneState, _In_ XRECTF inputPaneBounds)
        { return DirectUI::UIElement::NotifyInputPaneStateChange(nativeTarget, inputPaneState, inputPaneBounds); }

    _Check_return_ HRESULT UIElement_ApplyInputPaneTransition(_In_ CUIElement* nativeTarget, _In_ bool fEnableThemeTransition)
        { return DirectUI::UIElement::ApplyInputPaneTransition(nativeTarget, fEnableThemeTransition); }

    _Check_return_ HRESULT UIElement_ApplyElevationEffect(_In_ CUIElement* target, unsigned int depth)
        { return DirectUI::UIElement::ApplyElevationEffectProxy(target, depth); }

    _Check_return_ HRESULT UIElement_GetScrollContentPresenterViewportRatios(_In_ CUIElement* nativeTarget, _In_ CDependencyObject* nativeChild, _Out_ XSIZEF* ratios)
        { return DirectUI::UIElement::GetScrollContentPresenterViewportRatios(nativeTarget, nativeChild, ratios); }

    _Check_return_ HRESULT UIElement_IsScrollViewerContentScrollable(_In_ CUIElement* nativeTarget, _Out_ bool* isContentHorizontallyScrollable, _Out_ bool* isContentVerticallyScrollable)
        { return DirectUI::UIElement::IsScrollViewerContentScrollable(nativeTarget, isContentHorizontallyScrollable, isContentVerticallyScrollable); }

    _Check_return_ HRESULT UIElement_ProcessTabStop(_In_ CContentRoot* contentRoot, _In_opt_ CDependencyObject* pFocusedElement, _In_opt_ CDependencyObject* pCandidateTabStopElement, const bool isShiftPressed, const bool didCycleFocusAtRootVisualScope, _Outptr_ CDependencyObject** ppNewTabStop, _Out_ bool* pIsTabStopOverrided)
        { return DirectUI::UIElement::ProcessTabStop(contentRoot, pFocusedElement, pCandidateTabStopElement, isShiftPressed, didCycleFocusAtRootVisualScope, ppNewTabStop, pIsTabStopOverrided); }

    _Check_return_ HRESULT UIElement_GetNextTabStop(_In_ CDependencyObject* pFocusedElement, _Outptr_ CDependencyObject** ppNextTabStop)
        { return DirectUI::UIElement::GetNextTabStop(pFocusedElement, ppNextTabStop); }

    _Check_return_ HRESULT UIElement_GetPreviousTabStop(_In_ CDependencyObject* pFocusedElement, _Outptr_ CDependencyObject** ppPreviousTabStop)
        { return DirectUI::UIElement::GetPreviousTabStop(pFocusedElement, ppPreviousTabStop); }

    _Check_return_ HRESULT UIElement_GetFirstFocusableElement(_In_ CDependencyObject* pSearchStart, _Outptr_ CDependencyObject** ppFirstFocusable)
        { return DirectUI::UIElement::GetFirstFocusableElement(pSearchStart, ppFirstFocusable); }

    _Check_return_ HRESULT UIElement_GetLastFocusableElement(_In_ CDependencyObject* pSearchStart, _Outptr_ CDependencyObject** ppLastFocusable)
        { return DirectUI::UIElement::GetLastFocusableElement(pSearchStart, ppLastFocusable); }

    _Check_return_ HRESULT UIElement_GetDManipElementAndProperty(_In_ CUIElement* nativeTarget, _In_ KnownPropertyIndex targetProperty, _Outptr_ CDependencyObject** ppDManipElement, _Out_ XUINT32* pDManipProperty)
    {
        return DirectUI::UIElement::GetDManipElementAndProperty(nativeTarget, targetProperty, ppDManipElement, pDManipProperty);
    }

    _Check_return_ HRESULT UIElement_GetDManipElement(_In_ CUIElement* nativeTarget, _Outptr_ CDependencyObject** ppDManipElement)
    {
        return DirectUI::UIElement::GetDManipElement(nativeTarget, ppDManipElement);
    }

    bool UIElement_ShouldPlayImplicitShowHideAnimation(_In_ CUIElement* nativeTarget)
        { return DirectUI::UIElement::ShouldPlayImplicitShowHideAnimation(nativeTarget); }

    _Check_return_ HRESULT BitmapSource_SetSourceAsync(_In_ CBitmapSource* pNative, _In_ wsts::IRandomAccessStream* pStreamSource,
                                                                    _Outptr_ wf::IAsyncAction** ppReturnValue)
        { return DirectUI::BitmapSource::SetSourceAsync(pNative, pStreamSource, ppReturnValue); }

    _Check_return_ HRESULT FrameworkElement_MeasureOverride(_In_ CFrameworkElement* nativeTarget, _In_ XFLOAT inWidth, _In_ XFLOAT inHeight, _Out_ XFLOAT* outWidth, _Out_ XFLOAT* outHeight)
        { return DirectUI::FrameworkElement::MeasureOverrideFromCore(nativeTarget, inWidth, inHeight, outWidth, outHeight); }

    _Check_return_ HRESULT FrameworkElement_ArrangeOverride(_In_ CFrameworkElement* nativeTarget, _In_ XFLOAT inWidth, _In_ XFLOAT inHeight, _Out_ XFLOAT* outWidth, _Out_ XFLOAT* outHeight)
        { return DirectUI::FrameworkElement::ArrangeOverrideFromCore(nativeTarget, inWidth, inHeight, outWidth, outHeight); }

    _Check_return_ HRESULT FrameworkElement_OnApplyTemplate(_In_ CFrameworkElement* nativeTarget)
        { return DirectUI::FrameworkElement::OnApplyTemplateFromCore(nativeTarget); }

    _Check_return_ HRESULT FrameworkElement_GetLogicalParentForAP(_In_ CDependencyObject* nativeTarget, _Outptr_ CDependencyObject** ppLogicalParentForAP)
        { return DirectUI::FrameworkElement::GetLogicalParentForAPCore(nativeTarget, ppLogicalParentForAP); }

    _Check_return_ HRESULT UserControl_RegisterAppBars(_In_ CDependencyObject* nativeDO)
        { return DirectUI::UserControl::RegisterAppBarsCallback(nativeDO); }

    _Check_return_ HRESULT UserControl_UnregisterAppBars(_In_ CDependencyObject* nativeDO)
        { return DirectUI::UserControl::UnregisterAppBarsCallback(nativeDO); }

    _Check_return_ HRESULT Control_UpdateVisualState(_In_ CDependencyObject* nativeTarget, _In_ bool fUseTransitions)
        { return DirectUI::Control::UpdateVisualState(nativeTarget, fUseTransitions); }

    _Check_return_ HRESULT Control_UpdateEngagementState(_In_ CControl* nativeTarget, _In_ bool fEngage)
        { return DirectUI::Control::UpdateEngagementState(nativeTarget, fEngage); }

    _Check_return_ HRESULT Control_GetBuiltInStyle(_In_ CDependencyObject* nativeTarget, _Out_ CStyle** nativeStyle)
        { return AgCoreCallbacks::GetBuiltInStyle(nativeTarget, nativeStyle); }

    _Check_return_ HRESULT TextBox_NotifyOffsetsChanging(_In_ CDependencyObject* nativeTextBoxView, XDOUBLE oldHorizontalOffset, XDOUBLE newHorizontalOffset, XDOUBLE oldVerticalOffset, XDOUBLE newVerticalOffset)
        { return DirectUI::TextBoxView::NotifyOffsetsChanging(nativeTextBoxView, oldHorizontalOffset, newHorizontalOffset, oldVerticalOffset, newVerticalOffset); }

    _Check_return_ HRESULT TextBox_InvalidateScrollInfo(_In_ CDependencyObject* nativeTextBoxView)
        { return DirectUI::TextBoxView::InvalidateScrollInfo(nativeTextBoxView); }

    _Check_return_ HRESULT TextBox_OnApplyTemplateHandler(_In_ CDependencyObject* nativeTextBox)
        { return DirectUI::TextBox::OnApplyTemplateHandler(nativeTextBox); }

    _Check_return_ HRESULT TextBox_ShowPlaceholderTextHandler(_In_ CDependencyObject* nativeTextBox, _In_ bool isEnabled)
        { return DirectUI::TextBox::ShowPlaceholderTextHandler(nativeTextBox, isEnabled); }

    _Check_return_ HRESULT TextBox_GetBringIntoViewOnFocusChange(_In_ CDependencyObject* nativeTextBox, _Out_ bool* pBringIntoViewOnFocusChange)
        { return DirectUI::TextBox::GetBringIntoViewOnFocusChange(nativeTextBox, pBringIntoViewOnFocusChange); }

    _Check_return_ HRESULT TextBox_OnTextChangingHandler(_In_ CTextBox* const nativeTextBox, _In_ bool fTextChanged)
        { return DirectUI::TextBox::OnTextChangingHandler(nativeTextBox, fTextChanged); }

    _Check_return_ HRESULT TextBox_AddMenuFlyoutItemClickHandler(_In_ CDependencyObject* pMenuFlyoutItem, _In_ INTERNAL_EVENT_HANDLER eventHandler)
        { return DirectUI::MenuFlyoutItem::AddProofingItemHandlerStatic(pMenuFlyoutItem, eventHandler); }

    _Check_return_ HRESULT RichEditBox_HandleHyperlinkNavigation(_In_reads_( cLinkText) WCHAR* pLinkText, XUINT32 cLinkText)
        { return DirectUI::RichEditBox::HandleHyperlinkNavigation(pLinkText, cLinkText); }

    _Check_return_ HRESULT TextBoxView_CaretChanged(_In_ CDependencyObject* pNativeTextBoxView)
        { return DirectUI::TextBoxView::CaretChanged(pNativeTextBoxView); }

    _Check_return_ HRESULT TextBoxView_CaretVisibilityChanged(_In_ CDependencyObject* pNativeTextBoxView)
        { return DirectUI::TextBoxView::CaretVisibilityChanged(pNativeTextBoxView); }

    _Check_return_ HRESULT TextBoxView_InvalidateView(_In_ CDependencyObject* pNativeTextBoxView)
        { return DirectUI::TextBoxView::InvalidateView(pNativeTextBoxView); }

    _Check_return_ HRESULT TextBox_OnContextMenuOpeningHandler(_In_ CDependencyObject* nativeTextBox, _In_ XFLOAT cursorLeft, _In_ XFLOAT cursorTop,  _Out_ bool& handled)
        { return DirectUI::TextBox::OnContextMenuOpeningHandler(nativeTextBox, cursorLeft, cursorTop, handled); }

    _Check_return_ HRESULT PasswordBox_OnApplyTemplateHandler(_In_ CPasswordBox* nativePasswordBox)
        { return DirectUI::PasswordBox::OnApplyTemplateHandler(nativePasswordBox); }

    _Check_return_ HRESULT PasswordBox_ShowPlaceholderTextHandler(_In_ CDependencyObject* nativePasswordBox, _In_ bool isEnabled)
        { return DirectUI::PasswordBox::ShowPlaceholderTextHandler(nativePasswordBox, isEnabled); }

    _Check_return_ HRESULT PasswordBox_OnContextMenuOpeningHandler(_In_ CDependencyObject* nativePasswordBox, _In_ XFLOAT cursorLeft, _In_ XFLOAT cursorTop, _Out_ bool& handled)
        { return DirectUI::PasswordBox::OnContextMenuOpeningHandler(nativePasswordBox, cursorLeft, cursorTop, handled); }

    _Check_return_ HRESULT RichEditBox_OnApplyTemplateHandler(_In_ CDependencyObject* nativeRichEditBox)
        { return DirectUI::RichEditBox::OnApplyTemplateHandler(nativeRichEditBox); }

    _Check_return_ HRESULT RichEditBox_OnTextChangingHandler(_In_ CDependencyObject* nativeRichEditBox, _In_ bool fTextChanged)
        { return DirectUI::RichEditBox::OnTextChangingHandler(nativeRichEditBox, fTextChanged); }

    _Check_return_ HRESULT RichEditBox_ShowPlaceholderTextHandler(_In_ CDependencyObject* nativeRichEditBox, _In_ bool isEnabled)
        { return DirectUI::RichEditBox::ShowPlaceholderTextHandler(nativeRichEditBox, isEnabled); }

    _Check_return_ HRESULT RichEditBox_OnContextMenuOpeningHandler(_In_ CDependencyObject* nativeRichEditBox, _In_ XFLOAT cursorLeft, _In_ XFLOAT cursorTop, _Out_ bool& handled)
        { return DirectUI::RichEditBox::OnContextMenuOpeningHandler(nativeRichEditBox, cursorLeft, cursorTop, handled); }

    _Check_return_ HRESULT TextBlock_OnContextMenuOpeningHandler(_In_ CDependencyObject* nativeTextBlock, _In_ XFLOAT cursorLeft, _In_ XFLOAT cursorTop, _Out_ bool& handled)
        { return DirectUI::TextBlock::OnContextMenuOpeningHandler(nativeTextBlock, cursorLeft, cursorTop, handled); }

    _Check_return_ HRESULT RichTextBlock_OnContextMenuOpeningHandler(_In_ CDependencyObject* nativeRichTextBlock, _In_ XFLOAT cursorLeft, _In_ XFLOAT cursorTop, _Out_ bool& handled)
        { return DirectUI::RichTextBlock::OnContextMenuOpeningHandler(nativeRichTextBlock, cursorLeft, cursorTop, handled); }

    _Check_return_ HRESULT VisualStateManager_CustomVSMGoToState(_In_ CDependencyObject* pControl, _In_ VisualStateToken token, _In_ int groupIndex, _In_ bool useTransitions, _Out_ bool* bSucceeded)
        { return DirectUI::VisualStateManager::CustomVSMGoToState(pControl, token, groupIndex, useTransitions, bSucceeded); }

    _Check_return_ HRESULT ItemsControl_SetItemCollection(_In_ CItemCollection* nativeItemCollection, _In_ CItemsControl* nativeItemsControl)
        { return DirectUI::ItemsControl::SetItemCollectionStatic(nativeItemCollection, nativeItemsControl); }

    _Check_return_ HRESULT ItemsControl_ClearVisualChildren(_In_ CItemsControl* nativeItemsControl, _In_ bool bHostIsReplaced)
        { return DirectUI::ItemsControl::ClearVisualChildren(nativeItemsControl, bHostIsReplaced); }

    _Check_return_ HRESULT ItemsControl_DisplayMemberPathChanged(_In_ CItemsControl* nativeItemsControl)
        { return DirectUI::ItemsControl::DisplayMemberPathChanged(nativeItemsControl); }

    _Check_return_ HRESULT ItemsControl_RecreateVisualChildren(_In_ CItemsControl* nativeItemsControl)
        { return DirectUI::ItemsControl::RecreateVisualChildren(nativeItemsControl); }

    _Check_return_ HRESULT ItemsControl_NotifyAllItemsAdded(_In_ CItemsControl* nativeItemsControl)
        { return DirectUI::ItemsControl::NotifyAllItemsAdded(nativeItemsControl); }

    _Check_return_ HRESULT ItemsPresenter_Dispose(_In_ CItemsPresenter* pNativeItemsPresenter)
        { return DirectUI::ItemsPresenter::Dispose(pNativeItemsPresenter); }

    _Check_return_ HRESULT ContentControl_OnContentChanged(_In_ CDependencyObject* nativeTarget, _In_ CValue* oldContentValue, _In_ CValue* newContentValue, _In_opt_ IInspectable* pValueOuter)
        { return DirectUI::ContentControl::OnContentChangedCallback(nativeTarget, oldContentValue, newContentValue, pValueOuter); }

    _Check_return_ HRESULT ContentPresenter_BindDefaultTextBlock(_In_ CTextBlock* pTextBlock, _In_opt_ const xstring_ptr* pstrBindingPath)
        { return DirectUI::ContentPresenter::BindDefaultTextBlock(pTextBlock, pstrBindingPath); }

    _Check_return_ HRESULT ContentPresenter_OnChildrenCleared(_In_ CDependencyObject* nativeTarget)
        { return DirectUI::ContentPresenter::OnChildrenCleared(nativeTarget); }

    _Check_return_ HRESULT ContentPresenter_OnContentTemplateChanged(_In_ CDependencyObject* target, _In_ const PropertyChangedParams& args)
        { return DirectUI::ContentPresenter::OnContentTemplateChangedCallback(target, args); }

    _Check_return_ HRESULT ContentPresenter_OnContentTemplateSelectorChanged(_In_ CDependencyObject* target, _In_ const PropertyChangedParams& args)
        { return DirectUI::ContentPresenter::OnContentTemplateSelectorChangedCallback(target, args); }

    _Check_return_ HRESULT Popup_OnClosed(_In_ CDependencyObject* nativePopup)
        { return DirectUI::Popup::OnClosed(nativePopup); }

    void Popup_OnIslandLostFocus(_In_ CDependencyObject* nativePopup)
        { DirectUI::Popup::OnIslandLostFocus(nativePopup); }

    bool TextBlock_HasDataboundText(_In_ CTextBlock* nativeTextBlock)
        { return DirectUI::TextBlock::HasDataboundText(nativeTextBlock); }

    _Check_return_ HRESULT JoltHelper_FireEvent(_In_ CDependencyObject* pListener, _In_ KnownEventIndex eventId, _In_ CDependencyObject* pSender, _In_ CEventArgs* pArgs, _In_ XUINT32 flags)
        { return AgCoreCallbacks::FireEvent(pListener, eventId, pSender, pArgs, flags); }

    _Check_return_ HRESULT JoltHelper_RaiseEvent(_In_ CDependencyObject* target, _In_ DirectUI::ManagedEvent eventId, _In_ CEventArgs* coreEventArgs)
        { return AgCoreCallbacks::RaiseEvent(target, eventId, coreEventArgs); }

    _Check_return_ HRESULT DragDrop_PopulateDragEventArgs(_In_ CDragEventArgs* pArgs)
        { return DirectUI::DragDrop::PopulateDragEventArgs(pArgs); }

    _Check_return_ HRESULT DragDrop_CheckIfCustomVisualShouldBeCleared(_In_ CDependencyObject* pSource)
        { return HasDragDrop_CheckIfCustomVisualShouldBeCleared() ? DirectUI::DragDrop::CheckIfCustomVisualShouldBeCleared(pSource) : E_NOTIMPL; }

    _Check_return_ HRESULT RaiseDragDropEventAsyncOperation_CheckIfAcceptedOperationShouldBeReset(_In_ IInspectable* pOperation, _In_ CDependencyObject* pSource)
        { return HasRaiseDragDropEventAsyncOperation_CheckIfAcceptedOperationShouldBeReset() ? DirectUI::RaiseDragDropEventAsyncOperation::CheckIfAcceptedOperationShouldBeCleared(pOperation, pSource) : E_NOTIMPL; }

    _Check_return_ HRESULT RaiseDragDropEventAsyncOperation_SetAcceptedOperationSetterUIElement(_In_ IInspectable* pOperation, _In_opt_ CDependencyObject* pSource)
        { return HasRaiseDragDropEventAsyncOperation_SetAcceptedOperationSetterUIElement() ? DirectUI::RaiseDragDropEventAsyncOperation::SetAcceptedOperationSetterUIElement(pOperation, pSource) : E_NOTIMPL; }

    _Check_return_ HRESULT DragEventArgs_GetIsDeferred(_In_ CDragEventArgs* args, _Out_ bool* isDeferred)
        { return HasDragEventArgs_GetIsDeferred() ? DirectUI::DragEventArgs::GetIsDeferred(args, isDeferred) : E_NOTIMPL; }

    _Check_return_ HRESULT JoltHelper_TriggerGCCollect()
        { return DirectUI::ReferenceTrackerManager::TriggerCollection(); }

    _Check_return_ HRESULT Error_ReportUnhandledError(_In_ XUINT32 hr)
        { return AgCoreCallbacks::ReportUnhandledError(hr); }

    DirectUI::FocusVisualKind FrameworkApplication_GetFocusVisualKind()
        { return DirectUI::FrameworkApplication::GetFocusVisualKind(); }

    Theming::Theme FrameworkApplication_GetApplicationRequestedTheme()
        { return DirectUI::FrameworkApplication::GetApplicationRequestedTheme(); }

    _Check_return_ HRESULT FrameworkApplication_GetApplicationHighContrastAdjustment(_Out_ DirectUI::ApplicationHighContrastAdjustment* pApplicationHighContrastAdjustment)
        { return DirectUI::FrameworkApplication::GetApplicationHighContrastAdjustment(pApplicationHighContrastAdjustment); }

    _Check_return_ HRESULT FrameworkCallbacks_SetTemplateBinding(_In_ CDependencyObject* source, _In_ const CDependencyProperty* sourceProperty, _In_ CDependencyObject* target, _In_ const CDependencyProperty* targetProperty)
        { return DirectUI::TemplateBindingExpression::SetTemplateBinding(source, sourceProperty, target, targetProperty); }

    _Check_return_ HRESULT FrameworkCallbacks_GetCustomTypeIDFromObject(_In_ CDependencyObject* target, _Out_ KnownTypeIndex* typeID)
        { return AgCoreCallbacks::FrameworkCallbacks_GetCustomTypeIDFromObject(target, typeID); }

    _Check_return_ HRESULT FrameworkCallbacks_GetCustomTypeFullName(_In_ CDependencyObject* target, _Out_ xstring_ptr* pstrFullName)
        { return AgCoreCallbacks::FrameworkCallbacks_GetCustomTypeFullName(target, pstrFullName); }

    _Check_return_ HRESULT FrameworkCallbacks_AreObjectsOfSameType(_In_ CDependencyObject* nativeObject1, _In_ CDependencyObject* nativeObject2, _Out_ bool* areEqual)
        { return AgCoreCallbacks::AreObjectsOfSameType(nativeObject1, nativeObject2, areEqual); }

    _Check_return_ HRESULT FrameworkCallbacks_SetDataContext(_In_ CFrameworkElement* pElement, _In_ CValue* pValue)
        { return AgCoreCallbacks::FrameworkCallbacks_SetDataContext(pElement, pValue); }

    _Check_return_ HRESULT FrameworkCallbacks_ClearDataContext(_In_ CFrameworkElement* pElement)
        { return AgCoreCallbacks::FrameworkCallbacks_ClearDataContext(pElement); }

    _Check_return_ HRESULT FrameworkCallbacks_SupportInitializeEndInit(_In_ CDependencyObject* nativeTarget, _In_ const std::shared_ptr< ::XamlServiceProviderContext>& context)
        { return AgCoreCallbacks::FrameworkCallbacks_SupportInitializeEndInit(nativeTarget, context); }

    _Check_return_ HRESULT FrameworkCallbacks_CheckPeerType(_In_ CDependencyObject* nativeRoot, _In_ const xstring_ptr& strPeerType, _In_ XINT32 bCheckExact)
        { return DirectUI::XamlParserCallbacks::FrameworkCallbacks_CheckPeerType(nativeRoot, strPeerType, bCheckExact); }

    _Check_return_ HRESULT FrameworkCallbacks_OnParentUpdated(_In_ CDependencyObject* childElement, _In_opt_ CDependencyObject* oldParentElement, _In_opt_ CDependencyObject* newParentElement, _In_ bool isNewParentAlive)
        { return DirectUI::DependencyObject::OnParentUpdated(childElement, oldParentElement, newParentElement, isNewParentAlive); }

    _Check_return_ HRESULT FrameworkCallbacks_PropagateDataContextChange(_In_ CFrameworkElement* element)
        { return DirectUI::FrameworkElement::PropagateDataContextChange(element); }

    _Check_return_ HRESULT FrameworkCallbacks_CreateManagedPeer(_In_ CDependencyObject* element, _In_ KnownTypeIndex typeIndex, _In_ bool fPeggedNoRef, _In_ bool fPeggedRef, _In_ bool isShutdownException)
        { return AgCoreCallbacks::CreateManagedPeer(element, typeIndex, fPeggedNoRef, fPeggedRef, isShutdownException); }

    void FrameworkCallbacks_UnpegManagedPeerNoRef(_In_ CDependencyObject* element)
        { AgCoreCallbacks::UnpegManagedPeerNoRef(element); }

    void FrameworkCallbacks_PegManagedPeerNoRef(_In_ CDependencyObject* element)
        { AgCoreCallbacks::PegManagedPeerNoRef(element); }

    void FrameworkCallbacks_TryPegPeer(_In_ CDependencyObject* element, _Out_ bool* pPegged, _Out_ bool* pIsPendingDelete)
        { AgCoreCallbacks::TryPegPeer(element, pPegged, pIsPendingDelete); }

    _Check_return_ HRESULT FrameworkCallbacks_ReferenceTrackerWalk(_In_ CDependencyObject* element, _In_ DirectUI::EReferenceTrackerWalkType walkType, _In_ bool isRoot, _Out_ bool* pIsPeerAlive, _Out_ bool* pWalked)
        { return DirectUI::DependencyObject::ReferenceTrackerWalk(element, walkType, isRoot, pIsPeerAlive, pWalked); }

    _Check_return_ HRESULT FrameworkCallbacks_SetExpectedReferenceOnPeer(_In_ CDependencyObject* element)
        { return DirectUI::DependencyObject::SetExpectedReferenceOnPeer(element); }

    _Check_return_ HRESULT FrameworkCallbacks_ClearExpectedReferenceOnPeer(_In_ CDependencyObject* element)
        { return DirectUI::DependencyObject::ClearExpectedReferenceOnPeer(element); }

    _Check_return_ HRESULT FrameworkCallbacks_Hyperlink_OnClick(_In_ CDependencyObject* nativeHost)
        { return AgCoreCallbacks::FrameworkCallbacks_Hyperlink_OnClick(nativeHost); }

    _Check_return_ HRESULT FrameworkCallbacks_LoadThemeResources()
        { return AgCoreCallbacks::LoadThemeResources(); }

    _Check_return_ HRESULT FrameworkCallbacks_BudgetService_StoreFrameTime(_In_ bool isBeginningOfTick)
        { return AgCoreCallbacks::FrameworkCallbacks_BudgetService_StoreFrameTime(isBeginningOfTick); }

    _Check_return_ HRESULT FrameworkCallbacks_IsAnimationEnabled(_Out_ bool* pIsAnimationEnabled)
        { return AgCoreCallbacks::FrameworkCallbacks_IsAnimationEnabled(pIsAnimationEnabled); }

    _Check_return_ HRESULT FrameworkCallbacks_PhasedWorkDistributor_PerformWork(_Out_ bool* pWorkRemaining)
        { return AgCoreCallbacks::FrameworkCallbacks_PhasedWorkDistributor_PerformWork(pWorkRemaining); }

    _Check_return_ HRESULT FrameworkCallbacks_IsDXamlCoreShuttingDown(_Out_ bool* pIsDXamlCoreShuttingDown)
        { return AgCoreCallbacks::FrameworkCallbacks_IsDXamlCoreShuttingDown(pIsDXamlCoreShuttingDown); }

    _Check_return_ HRESULT XcpImports_ParticipateInTransition(_In_ CDependencyObject* nativeTransition, _In_ CDependencyObject* nativeUIElement, _In_ XINT32 transitionTrigger, _Out_ bool* DoesParticipate)
        { return DirectUI::Transition::ParticipateInTransition(nativeTransition, nativeUIElement, transitionTrigger, DoesParticipate); }

    _Check_return_ HRESULT XcpImports_CreateStoryboardsForTransition(_In_ CDependencyObject* nativeTransition, _In_ CDependencyObject* nativeUIElement, _In_ XRECTF startBounds, _In_ XRECTF destinationBounds, _In_ XINT32 transitionTrigger, _Out_ XINT32* cStoryboards, _Outptr_result_buffer_(* cStoryboards) CStoryboard*** pppTransitionStoryboard, _Out_ DirectUI::TransitionParent* parentToTransitionEnum)
        { return DirectUI::Transition::CreateStoryboardsForTransition(nativeTransition, nativeUIElement, startBounds, destinationBounds, transitionTrigger, cStoryboards, pppTransitionStoryboard, parentToTransitionEnum); }

    _Check_return_ HRESULT XcpImports_NotifyLayoutTransitionStart(_In_ CDependencyObject* nativeUIElement)
        { return DirectUI::Transition::NotifyLayoutTransitionStart(nativeUIElement); }

    _Check_return_ HRESULT XcpImports_NotifyLayoutTransitionEnd(_In_ CDependencyObject* nativeUIElement)
        { return DirectUI::Transition::NotifyLayoutTransitionEnd(nativeUIElement); }

    _Check_return_ HRESULT XcpImports_StaggerManaged(_In_ CDependencyObject* nativeStaggerFunction, _In_ XINT32 cElements, _In_reads_(cElements) CUIElement** ppElements, _In_reads_(cElements) XRECTF* pBounds, _Out_writes_(cElements) XFLOAT* pDelays)
        { return DirectUI::StaggerFunctionBase::GetTransitionDelayValues(nativeStaggerFunction, cElements, ppElements, pBounds, pDelays); }

    _Check_return_ HRESULT XcpImports_GetDynamicTimelines(_In_ CDependencyObject* nativeDynamicTimeline, _In_ bool bGenerateSteadyStateOnly, _In_ CValue* timelineCollection)
        { return AgCoreCallbacks::GetDynamicTimelines(nativeDynamicTimeline, bGenerateSteadyStateOnly, timelineCollection); }

    void XcpImports_PerFrameCallback()
        { AgCoreCallbacks::PerFrameCallback(); }

    _Check_return_ HRESULT AutomationPeer_GetAutomationPeerStringValue(_In_ CDependencyObject* nativeTarget, _In_ UIAXcp::APAutomationProperties eProperty, _Out_writes_z_(*cString) WCHAR* pString, _Inout_ XINT32* cString)
        { return DirectUI::AutomationPeer::GetAutomationPeerStringValue(nativeTarget, eProperty, pString, cString); }

    _Check_return_ HRESULT AutomationPeer_GetAutomationPeerIntValue(_In_ CDependencyObject* nativeTarget, _In_ UIAXcp::APAutomationProperties eProperty, _Inout_ XINT32* nRetVal)
        { return DirectUI::AutomationPeer::GetAutomationPeerIntValue(nativeTarget, eProperty, nRetVal); }

    _Check_return_ HRESULT AutomationPeer_GetAutomationPeerPointValue(_In_ CDependencyObject* nativeTarget, _In_ UIAXcp::APAutomationProperties eProperty, _Out_ XPOINTF* pPointF)
        { return DirectUI::AutomationPeer::GetAutomationPeerPointValue(nativeTarget, eProperty, pPointF); }

    _Check_return_ HRESULT AutomationPeer_GetAutomationPeerRectValue(_In_ CDependencyObject* nativeTarget, _In_ UIAXcp::APAutomationProperties eProperty, _Inout_ XRECTF* pRectF)
        { return DirectUI::AutomationPeer::GetAutomationPeerRectValue(nativeTarget, eProperty, pRectF); }

    _Check_return_ HRESULT AutomationPeer_GetAutomationPeerAPValue(_In_ CDependencyObject* nativeTarget, _In_ UIAXcp::APAutomationProperties eProperty, _Inout_ CDependencyObject** returnAP)
        { return DirectUI::AutomationPeer::GetAutomationPeerAPValue(nativeTarget, eProperty, returnAP); }

    _Check_return_ HRESULT AutomationPeer_GetAutomationPeerDOValue(_In_ CDependencyObject* nativeTarget, _In_ UIAXcp::APAutomationProperties eProperty, _Inout_ CDependencyObject** returnDO)
        { return DirectUI::AutomationPeer::GetAutomationPeerDOValue(nativeTarget, eProperty, returnDO); }

    _Check_return_ HRESULT AutomationPeer_CallAutomationPeerMethod(_In_ CDependencyObject* nativeTarget, _In_ XINT32 methodIndex)
        { return DirectUI::AutomationPeer::CallAutomationPeerMethod(nativeTarget, methodIndex); }

    _Check_return_ HRESULT AutomationPeer_GetAutomationPeerChildren(_In_ CDependencyObject* nativeTarget, _In_ XUINT32 methodIndex, _Inout_ XINT32* returnCount, __deref_inout_ecount(* returnCount) CDependencyObject*** returnAP)
        { return DirectUI::AutomationPeer::GetAutomationPeerChildren(nativeTarget, methodIndex, returnCount, returnAP); }

    _Check_return_ HRESULT AutomationPeer_Navigate(_In_ CDependencyObject* nativeTarget, _In_ UIAXcp::AutomationNavigationDirection direction, _Outptr_result_maybenull_ CDependencyObject** ppReturnAPAsDO, _Outptr_result_maybenull_ IUnknown** ppReturnIREPFAsUnk)
        { return DirectUI::AutomationPeer::Navigate(nativeTarget, direction, ppReturnAPAsDO, ppReturnIREPFAsUnk); }

    _Check_return_ HRESULT AutomationPeer_GetElementFromPoint(_In_ CDependencyObject* nativeTarget, _In_ CValue param, _Outptr_result_maybenull_ CDependencyObject** ppReturnAPAsDO, _Outptr_result_maybenull_ IUnknown** ppReturnIREPFAsUnk)
        { return DirectUI::AutomationPeer::GetElementFromPoint(nativeTarget, param, ppReturnAPAsDO, ppReturnIREPFAsUnk); }

    _Check_return_ HRESULT AutomationPeer_GetFocusedElement(_In_ CDependencyObject* nativeTarget, _Outptr_result_maybenull_ CDependencyObject** ppReturnAPAsDO, _Outptr_result_maybenull_ IUnknown** ppReturnIREPFAsUnk)
        { return DirectUI::AutomationPeer::GetFocusedElement(nativeTarget, ppReturnAPAsDO, ppReturnIREPFAsUnk); }

    _Check_return_ HRESULT AutomationPeer_GetPattern(_In_ CDependencyObject* nativeTarget, _In_ CDependencyObject** nativeInterface, _In_ UIAXcp::APPatternInterface eInterface)
        { return DirectUI::AutomationPeer::GetPattern(nativeTarget, nativeInterface, eInterface); }

    _Check_return_ HRESULT AutomationPeer_UIATextRangeInvoke(_In_ CDependencyObject* nativeTarget, _In_ XINT32 eFunction, _In_ XINT32 cParams, _In_opt_ void* pvParams, _In_opt_ Automation::CValue* pRetVal)
        { return DirectUI::AutomationPeer::UIATextRangeInvoke(nativeTarget, eFunction, cParams, pvParams, pRetVal); }

    _Check_return_ HRESULT AutomationPeer_UIAPatternInvoke(_In_ CDependencyObject* nativeTarget, _In_ UIAXcp::APPatternInterface eInterface, _In_ XINT32 eFunction, _In_ XINT32 cParams, _In_opt_ void* pvParams, _In_opt_ Automation::CValue* pRetVal)
        { return DirectUI::AutomationPeer::UIAPatternInvoke(nativeTarget, eInterface, eFunction, cParams, pvParams, pRetVal); }

    _Check_return_ HRESULT AutomationPeer_NotifyNoUIAClientObjectForAP(_In_ CDependencyObject* nativeTarget)
        { return DirectUI::AutomationPeer::NotifyNoUIAClientObjectForAP(nativeTarget); }

    _Check_return_ HRESULT AutomationPeer_GenerateAutomationPeerEventsSource(_In_ CDependencyObject* nativeTarget, _In_ CDependencyObject* nativeTargetParent)
        { return DirectUI::AutomationPeer::GenerateAutomationPeerEventsSource(nativeTarget, nativeTargetParent); }

    _Check_return_ HRESULT Virtualization_ExecuteDeferredUnlinkAction(_In_ CUIElement* nativeTarget)
        { return DirectUI::ItemContainerGenerator::ExecuteDeferredUnlinkAction(nativeTarget); }

    _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_CreateInstance(_In_ XamlTypeToken inXamlType, _Out_ ::XamlQualifiedObject* newObject)
        { return DirectUI::XamlParserCallbacks::XamlManagedRuntimeRPInvokes_CreateInstance(inXamlType, newObject); }

    _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_CreateFromValue(_In_ void* inServiceContext, _In_ XamlTypeToken inTs, _In_ ::XamlQualifiedObject* qoValue, _In_ XamlPropertyToken inProperty, _In_ ::XamlQualifiedObject* qoRootInstance, _Out_ ::XamlQualifiedObject* qo)
        { return DirectUI::XamlParserCallbacks::XamlManagedRuntimeRPInvokes_CreateFromValue(inServiceContext, inTs, qoValue, inProperty, qoRootInstance, qo); }

    _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_GetValue(_In_ const ::XamlQualifiedObject* qoObj, _In_ XamlPropertyToken inProperty, _Out_ ::XamlQualifiedObject* outValue)
        { return DirectUI::XamlParserCallbacks::XamlManagedRuntimeRPInvokes_GetValue(qoObj, inProperty, outValue); }

    _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_GetAmbientValue(_In_ const ::XamlQualifiedObject* qoObj, _In_ XamlPropertyToken inProperty, _Out_ ::XamlQualifiedObject* outValue)
        { return DirectUI::XamlParserCallbacks::XamlManagedRuntimeRPInvokes_GetAmbientValue(qoObj, inProperty, outValue); }

    _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_SetValue(_In_ ::XamlQualifiedObject* inObj, _In_ XamlPropertyToken inProperty, _In_ ::XamlQualifiedObject* inValue)
        { return DirectUI::XamlParserCallbacks::XamlManagedRuntimeRPInvokes_SetValue(inObj, inProperty, inValue); }

    _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_Add(_In_ ::XamlQualifiedObject* qoCollection, _In_ ::XamlQualifiedObject* inValue)
        { return DirectUI::XamlParserCallbacks::XamlManagedRuntimeRPInvokes_Add(qoCollection, inValue); }

    _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_AddToDictionary(_In_ ::XamlQualifiedObject* dictionary, _In_ ::XamlQualifiedObject* inKey, _In_ ::XamlQualifiedObject* inValue)
        { return DirectUI::XamlParserCallbacks::XamlManagedRuntimeRPInvokes_AddToDictionary(dictionary, inKey, inValue); }

    _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_CallProvideValue(_In_ ::XamlQualifiedObject* markupExtension, _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext, _Out_ ::XamlQualifiedObject* outValue)
        { return DirectUI::XamlParserCallbacks::XamlManagedRuntimeRPInvokes_CallProvideValue(markupExtension, spServiceProviderContext, outValue); }

    _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_SetConnectionId(_In_ ::XamlQualifiedObject* qoComponentConnector, _In_ ::XamlQualifiedObject* qoConnectionId, _In_ ::XamlQualifiedObject* qoTarget)
        { return DirectUI::XamlParserCallbacks::XamlManagedRuntimeRPInvokes_SetConnectionId(qoComponentConnector, qoConnectionId, qoTarget); }

    _Check_return_ HRESULT XamlManagedRuntimeRPInvokes_GetXBindConnector(_In_ ::XamlQualifiedObject* qoComponentConnector, _In_ ::XamlQualifiedObject* qoConnectionId, _In_ ::XamlQualifiedObject* qoTarget, _In_ ::XamlQualifiedObject* qoReturnConnector)
        { return DirectUI::XamlParserCallbacks::XamlManagedRuntimeRPInvokes_GetXBindConnector(qoComponentConnector, qoConnectionId, qoTarget, qoReturnConnector); }

    _Check_return_ HRESULT ApplicationBarService_GetAppBarStatus(_In_ CDependencyObject* object, _Out_ bool* pbIsOpenTop, _Out_ bool* pbIsStickyTop, _Out_ XFLOAT* pWidthTop, _Out_ XFLOAT* pHeightTop, _Out_ bool* pbIsOpenBottom, _Out_ bool* pbIsStickyBottom, _Out_ XFLOAT* pWidthBottom, _Out_ XFLOAT* pHeightBottom)
        { return DirectUI::ApplicationBarServiceStatics::GetAppBarStatus(object, pbIsOpenTop, pbIsStickyTop, pWidthTop, pHeightTop, pbIsOpenBottom, pbIsStickyBottom, pWidthBottom, pHeightBottom); }

    _Check_return_ HRESULT ApplicationBarService_ProcessToggleApplicationBarsFromMouseRightTapped(_In_ IInspectable* xamlRootInspectable)
        { return DirectUI::ApplicationBarServiceStatics::ProcessToggleApplicationBarsFromMouseRightTapped(xamlRootInspectable); }

    _Check_return_ HRESULT Window_GetContentRootBounds(_In_ CDependencyObject* pObject, _Out_ XRECTF* pContentRootBounds)
        { return DirectUI::Window::GetContentRootBounds(pObject, pContentRootBounds); }

    _Check_return_ HRESULT Window_GetContentRootLayoutBounds(_In_ CDependencyObject* pObject, _Out_ XRECTF* pContentRootBounds)
        { return DirectUI::Window::GetContentRootLayoutBounds(pObject, pContentRootBounds); }

    _Check_return_ HRESULT Window_GetRootScrollViewer(_Outptr_ CDependencyObject** ppRootScrollViewer)
        { return DirectUI::Window::GetRootScrollViewer(ppRootScrollViewer); }

    bool Window_AtlasRequest(uint32_t width, uint32_t height, PixelFormat pixelFormat)
        { return DirectUI::Window::AtlasRequest(width, height, pixelFormat); }

    _Check_return_ HRESULT TextElement_OnCreateAutomationPeer(_In_ CDependencyObject* nativeTarget, _Out_ CAutomationPeer** returnAP)
        { return DirectUI::TextElement::OnCreateAutomationPeer(nativeTarget, returnAP); }

    _Check_return_ HRESULT CommandBarElementCollection_ValidateItem(_In_ CDependencyObject* pObject)
        { return DirectUI::CommandBarElementCollection::ValidateItem(pObject); }

    _Check_return_ HRESULT HubSectionCollection_ValidateItem(_In_ CDependencyObject* pObject)
        { return DirectUI::HubSectionCollection::ValidateItem(pObject); }

    _Check_return_ HRESULT MenuFlyoutItemBaseCollection_ValidateItem(_In_ CDependencyObject* pObject)
        { return DirectUI::MenuFlyoutItemBaseCollection::ValidateItem(pObject); }

    _Check_return_ HRESULT FlyoutBase_IsOpen(_In_ CFlyoutBase* flyoutBase, _Out_ bool& isOpen)
        { return DirectUI::FlyoutBase::IsOpen(flyoutBase, isOpen); }

    _Check_return_ HRESULT FlyoutBase_ShowAt(_In_ CFlyoutBase* pFlyoutBase, _In_ CFrameworkElement* pTarget)
        { return DirectUI::FlyoutBase::ShowAtStatic(pFlyoutBase, pTarget); }

    _Check_return_ HRESULT FlyoutBase_ShowAt(_In_ CFlyoutBase* pFlyoutBase, _In_ CFrameworkElement* pTarget, _In_ wf::Point point, _In_ wf::Rect exclusionRect, _In_ xaml_primitives::FlyoutShowMode flyoutShowMode)
        { return DirectUI::FlyoutBase::ShowAtStatic(pFlyoutBase, pTarget, point, exclusionRect, flyoutShowMode); }

    _Check_return_ HRESULT MenuFlyout_ShowAt(_In_ CMenuFlyout* pMenuFlyout, _In_ CUIElement* pTarget, _In_ wf::Point point)
        { return DirectUI::MenuFlyout::ShowAtStatic(pMenuFlyout, pTarget, point); }

    _Check_return_ HRESULT UIElement_IsDraggableOrPannable(_In_ CUIElement* pUIElement, _Out_ bool* pIsDraggableOrPannable)
        { return DirectUI::UIElement::IsDraggableOrPannable(pUIElement, pIsDraggableOrPannable); }

    bool DXamlCore_IsWinRTDndOperationInProgress()
        { return DirectUI::DXamlCore::IsWinRTDndOperationInProgress(); }

    _Check_return_ HRESULT FlyoutBase_CloseOpenFlyout(_In_opt_ CFlyoutBase* parentFlyout)
        { return DirectUI::FlyoutBase::CloseOpenFlyout(parentFlyout); }

    _Check_return_ HRESULT FlyoutBase_OnClosing(_In_ CFlyoutBase* object, _Out_ bool* cancel)
        { return DirectUI::FlyoutBase::OnClosingStatic(object, cancel); }

    _Check_return_ HRESULT FlyoutBase_Hide(_In_opt_ CFlyoutBase* flyout)
        { return DirectUI::FlyoutBase::HideFlyout(flyout); }

    _Check_return_ HRESULT FlyoutBase_GetPlacementTargetNoRef(_In_ CFlyoutBase* flyout, _Outptr_ CFrameworkElement** placementTarget)
        { return DirectUI::FlyoutBase::GetPlacementTargetNoRef(flyout, placementTarget); }

    _Check_return_ HRESULT Button_SuppressFlyoutOpening(_In_ CButton* button)
        { return DirectUI::Button::SuppressFlyoutOpening(button); }

    _Check_return_ HRESULT ElementSoundPlayerService_RequestInteractionSoundForElement(_In_ DirectUI::ElementSoundKind sound, _In_ CDependencyObject* pControl)
        { return DirectUI::ElementSoundPlayerService::RequestInteractionSoundForElementStatic(sound, pControl); }

    _Check_return_ HRESULT ElementSoundPlayerService_PlayInteractionSound()
        { return DirectUI::ElementSoundPlayerService::PlayInteractionSoundStatic(); }

    _Check_return_ HRESULT ExternalObjectReference_GetTarget(_In_ CDependencyObject* pDO, _Outptr_opt_result_maybenull_ IInspectable** ppTarget)
        { return DirectUI::ExternalObjectReference::GetTarget(pDO, ppTarget); }

    _Check_return_ HRESULT ToolTipService_RegisterToolTip(_In_ CDependencyObject* owner, _In_ CFrameworkElement* container)
        { return ToolTipService::RegisterToolTipFromCore(owner, container); }

    _Check_return_ HRESULT ToolTipService_UnregisterToolTip(_In_ CDependencyObject* owner, _In_ CFrameworkElement* container)
        { return ToolTipService::UnregisterToolTipFromCore(owner, container); }

    _Check_return_ HRESULT ToolTipService_OnOwnerPointerEntered(_In_ CDependencyObject* sender, _In_ CPointerEventArgs* args)
        { return ToolTipService::OnOwnerPointerEnteredFromCore(sender, args); }

    _Check_return_ HRESULT ToolTipService_OnOwnerPointerExitedOrLostOrCanceled(_In_ CDependencyObject* sender, _In_ CPointerEventArgs* args)
        { return ToolTipService::OnOwnerPointerExitedOrLostOrCanceledFromCore(sender, args); }

    _Check_return_ HRESULT ToolTipService_OnOwnerGotFocus(_In_ CDependencyObject* sender, _In_ CRoutedEventArgs* args)
        { return ToolTipService::OnOwnerGotFocusFromCore(sender, args); }

    _Check_return_ HRESULT ToolTipService_OnOwnerLostFocus(_In_ CDependencyObject* sender, _In_ CRoutedEventArgs* args)
        { return ToolTipService::OnOwnerLostFocusFromCore(sender, args); }

    _Check_return_ HRESULT XamlCompositionBrushBase_OnConnected(_In_ CDependencyObject* object)
        { return DirectUI::XamlCompositionBrushBase::OnConnectedFromCore(object); }

    _Check_return_ HRESULT XamlCompositionBrushBase_OnDisconnected(_In_ CDependencyObject* object)
        { return DirectUI::XamlCompositionBrushBase::OnDisconnectedFromCore(object); }

    _Check_return_ HRESULT XamlCompositionBrushBase_OnElementConnected(_In_ CDependencyObject* object, _In_ CDependencyObject* connectedElement)
        { return DirectUI::XamlCompositionBrushBase::OnElementConnectedFromCore(object, connectedElement); }

    bool XamlCompositionBrushBase_HasPrivateOverrides(_In_ CDependencyObject* object)
        { return DirectUI::XamlCompositionBrushBase::HasPrivateOverrides(object); }

    _Check_return_ HRESULT UIElement_OnBringIntoViewRequested(_In_ CUIElement* pUIElement, _In_ CRoutedEventArgs* args)
        { return DirectUI::UIElement::OnBringIntoViewRequestedFromCore(pUIElement, args); }

    _Check_return_ HRESULT UIElement_RaiseProcessKeyboardAccelerators(
        _In_ CUIElement* pUIElement,
        _In_ wsy::VirtualKey key,
        _In_ wsy::VirtualKeyModifiers keyModifiers,
        _Out_ BOOLEAN *pHandled,
        _Out_ BOOLEAN *pHandledShouldNotImpedeTextInput)
        { return DirectUI::UIElement::RaiseProcessKeyboardAcceleratorsStatic(pUIElement, key, keyModifiers, pHandled, pHandledShouldNotImpedeTextInput); }

    _Check_return_ HRESULT KeyboardAccelerator_RaiseKeyboardAcceleratorInvoked(
        _In_ CKeyboardAccelerator* pNativeAccelerator,
        _In_ CDependencyObject* pElement,
        _Out_ BOOLEAN *pIsHandled)
        { return DirectUI::KeyboardAccelerator::RaiseKeyboardAcceleratorInvoked(pNativeAccelerator, pElement, pIsHandled); }

    _Check_return_ HRESULT KeyboardAccelerator_SetToolTip(
        _In_ CKeyboardAccelerator* pNativeAccelerator,
        _In_ CDependencyObject* pParentControl)
        { return DirectUI::KeyboardAccelerator::SetToolTip(pNativeAccelerator, pParentControl); }

    bool HasDragDrop_CheckIfCustomVisualShouldBeCleared() { return true; /* DEAD_CODE_REMOVAL */}

    bool HasRaiseDragDropEventAsyncOperation_CheckIfAcceptedOperationShouldBeReset() { return true; /* DEAD_CODE_REMOVAL */ }

    bool HasRaiseDragDropEventAsyncOperation_SetAcceptedOperationSetterUIElement() { return true; /* DEAD_CODE_REMOVAL */ }

    bool HasDragEventArgs_GetIsDeferred() { return true; /* DEAD_CODE_REMOVAL */ }

    _Check_return_ HRESULT PasswordBox_OnPasswordChangingHandler(_In_ CPasswordBox* const passwordBox, _In_ bool passwordChanged)
        { return DirectUI::PasswordBox::OnPasswordChangingHandler(passwordBox, passwordChanged); }

    _Check_return_ HRESULT TextBox_OnBeforeTextChangingHandler(_In_ CTextBox* const nativeTextBox, _In_ xstring_ptr* newString, _Out_ BOOLEAN* wasCanceled)
        { return DirectUI::TextBox::OnBeforeTextChangingHandler(nativeTextBox, newString, wasCanceled); }

    _Check_return_ HRESULT TextBox_OnSelectionChangingHandler(
        _In_ CDependencyObject* const nativeTextBox,
        _In_ long selectionStart,
        _In_ long selectionLength,
        _Out_ BOOLEAN* wasCanceled)
    {
        return DirectUI::TextBox::OnSelectionChangingHandler(nativeTextBox, selectionStart, selectionLength, wasCanceled);
    }

    _Check_return_ HRESULT RichEditBox_OnSelectionChangingHandler(
        _In_ CDependencyObject* const nativeRichEditBox,
        _In_ long selectionStart,
        _In_ long selectionLength,
        _Out_ BOOLEAN* wasCanceled)
    {
        return DirectUI::RichEditBox::OnSelectionChangingHandler(nativeRichEditBox, selectionStart, selectionLength, wasCanceled);
    }

    bool CompositionTarget_HasHandlers() { return DXamlCore::CompositionTarget_HasHandlers(); }

    void JupiterWindow_SetPointerCapture() { DXamlCore::GetCurrent()->GetControl()->GetJupiterWindow()->SetPointerCapture(); }
    bool JupiterWindow_HasPointerCapture() { return DXamlCore::GetCurrent()->GetControl()->GetJupiterWindow()->HasPointerCapture(); }
    void JupiterWindow_ReleasePointerCapture() { DXamlCore::GetCurrent()->GetControl()->GetJupiterWindow()->ReleasePointerCapture(); }
    void JupiterWindow_SetFocus() { DXamlCore::GetCurrent()->GetControl()->GetJupiterWindow()->SetFocus(); }
    bool JupiterWindow_IsWindowDestroyed() { return DXamlCore::GetCurrent()->GetControl()->GetJupiterWindow()->IsWindowDestroyed(); }

    void PopCaretBrowsingDialog(_In_ IInspectable* xamlRoot) { DirectUI::PopCaretBrowsingContentDialog(xamlRoot); }

    #if WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
    _Check_return_ HRESULT InteractionCollection_HasInteractionForEvent(KnownEventIndex eventId, _In_ CUIElement* sender, _Out_ bool& hasInteraction)
        { return DirectUI::InteractionCollection::HasInteractionForEvent(eventId, sender, hasInteraction); }

    _Check_return_ HRESULT InteractionCollection_DispatchInteraction(KnownEventIndex eventId, _In_ CUIElement* sender, _In_ CEventArgs* args)
        { return DirectUI::InteractionCollection::DispatchInteraction(eventId, sender, args); }
    #endif // WI_IS_FEATURE_PRESENT(Feature_Xaml2018)

    _Check_return_ HRESULT DxamlCore_OnCompositionContentStateChangedForUWP() { return DXamlCore::GetCurrent()->OnCompositionContentStateChangedForUWP(); }

    _Check_return_ HRESULT DXamlCore_SetBinding(_In_ CDependencyObject* source, _In_ HSTRING path, _In_ CDependencyObject* target, KnownPropertyIndex targetPropertyIndex) { return DXamlCore::SetBindingCore(source, path, target, targetPropertyIndex); }

    _Check_return_ HRESULT DXamlCore_GetVisibleContentBoundsForElement(_In_opt_ CDependencyObject* element, _Out_ wf::Rect* value)
        { return DXamlCore::GetCurrent()->GetVisibleContentBoundsForElement(element, value); }

    _Check_return_ HRESULT DXamlCore_GetContentBoundsForElement(_In_opt_ CDependencyObject* element, _Out_ wf::Rect* value)
        { return DXamlCore::GetCurrent()->GetContentBoundsForElement(element, value); }

    _Check_return_ HRESULT DXamlCore_CalculateAvailableMonitorRect(_In_ CUIElement* pTargetElement,
            _In_ wf::Point targetPointClientLogical,
            _Out_ wf::Rect* availableMonitorRectClientLogicalResult)
    {
        return DXamlCore::GetCurrent()->CalculateAvailableMonitorRect(pTargetElement,
            targetPointClientLogical, availableMonitorRectClientLogicalResult);
    }

    _Check_return_ HRESULT DXamlCore_GetUISettings(_Out_ ctl::ComPtr<wuv::IUISettings>& spUISettings)
    {
        return DirectUI::DXamlCore::GetCurrent()->GetUISettings(spUISettings);
    }

    void XamlIslandRoot_OnSizeChanged(_In_ CXamlIslandRoot* xamlIslandRoot)
        { DirectUI::XamlIslandRoot::OnSizeChangedStatic(xamlIslandRoot); }

    _Check_return_ HRESULT FrameworkApplication_RemoveIsland(_In_ xaml_hosting::IXamlIslandRoot* island)
    {
        return FrameworkApplication::GetCurrentNoRef()->RemoveIsland(island);
    }

    _Check_return_ HRESULT TextControlFlyout_ShowAt(_In_ CFlyoutBase* flyoutBase, _In_ CFrameworkElement* target, wf::Point point, wf::Rect exclusionRect, xaml_primitives::FlyoutShowMode flyoutShowMode)
        { return DirectUI::TextControlFlyoutHelper::ShowAt(flyoutBase, target, point, exclusionRect, flyoutShowMode); }

    _Check_return_ HRESULT TextControlFlyout_AddProofingFlyout(_In_ CFlyoutBase* flyoutBase, _In_ CFrameworkElement* target)
        { return DirectUI::TextControlFlyoutHelper::AddProofingFlyout(flyoutBase, target); }

    _Check_return_ HRESULT TextControlFlyout_CloseIfOpen(_In_opt_ CFlyoutBase* flyoutBase)
        { return DirectUI::TextControlFlyoutHelper::CloseIfOpen(flyoutBase); }

    bool TextControlFlyout_IsGettingFocus(_In_opt_ CFlyoutBase* flyoutBase, _In_ CFrameworkElement* owner)
        { return DirectUI::TextControlFlyoutHelper::IsGettingFocus(flyoutBase, owner); }

    bool TextControlFlyout_IsElementChildOfOpenedFlyout(_In_opt_ CUIElement* element)
        { return DirectUI::TextControlFlyoutHelper::IsElementChildOfOpenedFlyout(element); }

    bool TextControlFlyout_IsElementChildOfTransientOpenedFlyout(_In_opt_ CUIElement* element)
        { return DirectUI::TextControlFlyoutHelper::IsElementChildOfTransientOpenedFlyout(element); }

    bool TextControlFlyout_IsElementChildOfProofingFlyout(_In_opt_ CUIElement* element)
        { return DirectUI::TextControlFlyoutHelper::IsElementChildOfProofingFlyout(element); }

    _Check_return_ HRESULT TextControlFlyout_DismissAllFlyoutsForOwner(_In_opt_ CUIElement* element)
        { return DirectUI::TextControlFlyoutHelper::DismissAllFlyoutsForOwner(element); }

    bool  TextControlFlyout_IsOpen(_In_ CFlyoutBase* flyoutBase)
        { return DirectUI::TextControlFlyoutHelper::IsOpen(flyoutBase); }

    _Check_return_ HRESULT TextBox_QueueUpdateSelectionFlyoutVisibility(_In_ CDependencyObject* nativeControl)
        { return DirectUI::TextBox::QueueUpdateSelectionFlyoutVisibility(nativeControl); }

    _Check_return_ HRESULT TextBlock_QueueUpdateSelectionFlyoutVisibility(_In_ CDependencyObject* nativeControl)
        { return DirectUI::TextBlock::QueueUpdateSelectionFlyoutVisibility(nativeControl); }

    _Check_return_ HRESULT RichEditBox_QueueUpdateSelectionFlyoutVisibility(_In_ CDependencyObject* nativeControl)
        { return DirectUI::RichEditBox::QueueUpdateSelectionFlyoutVisibility(nativeControl); }

    _Check_return_ HRESULT RichTextBlock_QueueUpdateSelectionFlyoutVisibility(_In_ CDependencyObject* nativeControl)
        { return DirectUI::RichTextBlock::QueueUpdateSelectionFlyoutVisibility(nativeControl); }

    _Check_return_ HRESULT PasswordBox_QueueUpdateSelectionFlyoutVisibility(_In_ CDependencyObject* nativeControl)
        { return DirectUI::PasswordBox::QueueUpdateSelectionFlyoutVisibility(nativeControl); }

    IInspectable* XamlRoot_Create(_In_ VisualTree* visualTree)
        { return DirectUI::XamlRoot::Create(visualTree); }

    void XamlRoot_RaiseChanged(_In_ IInspectable* xamlRootInsp)
    {
        ctl::ComPtr<DirectUI::XamlRoot> xamlRoot;
        IGNOREHR(ctl::do_query_interface(xamlRoot, xamlRootInsp));
        VERIFYHR(xamlRoot->RaiseChangedEvent());
    }

    void XamlRoot_RaiseInputActivationChanged(_In_ IInspectable* xamlRootInsp)
    {
        ctl::ComPtr<DirectUI::XamlRoot> xamlRoot;
        IGNOREHR(ctl::do_query_interface(xamlRoot, xamlRootInsp));
        IGNOREHR(xamlRoot->RaiseInputActivationChangedEvent());
    }

    void XamlRoot_UpdatePeg(_In_ IInspectable* xamlRootInsp, bool peg)
    {
        ctl::ComPtr<DirectUI::XamlRoot> xamlRoot;
        IGNOREHR(ctl::do_query_interface(xamlRoot, xamlRootInsp));
        xamlRoot->UpdatePeg(peg);
    }

    _Check_return_ HRESULT FrameworkApplication_GetRequiresPointerMode(_Out_ xaml::ApplicationRequiresPointerMode* value)
    {
        if (auto framework = FrameworkApplication::GetCurrentNoRef())
        {
            return framework->get_RequiresPointerMode(value);
        }

        return S_OK;
    }

    CXamlIslandRoot* GetXamlIslandRootFromXamlIsland(_In_ xaml_hosting::IXamlIslandRoot* xamlIslandRoot)
    {
        ctl::ComPtr<XamlIslandRoot> spXamlIsland;
        IGNOREHR(ctl::do_query_interface(spXamlIsland, xamlIslandRoot));

        return static_cast<CXamlIslandRoot*>(spXamlIsland->GetHandle());
    }

    _Check_return_ HRESULT FlyoutPresenter_GetTargetIfOpenedAsTransient(_In_ CDependencyObject* nativeControl, _Outptr_ CDependencyObject** nativeTarget)
    {
        return DirectUI::FlyoutPresenter::GetTargetIfOpenedAsTransientStatic(nativeControl, nativeTarget);
    }

    _Check_return_ HRESULT
    FrameworkApplication_GetResourceManagerOverrideFromApp(_Out_ wrl::ComPtr<mwar::IResourceManager>& resourceManager)
    {
        if (auto frameworkApp = FrameworkApplication::GetCurrentNoRef())
        {
            IFC_RETURN(frameworkApp->RaiseResourceManagerRequestedEvent(resourceManager));
        }

        return S_OK;
    }
}
