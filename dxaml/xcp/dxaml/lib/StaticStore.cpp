// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines a class for encapsulating process-global data in a thread-safe way.

#include "precomp.h"
#include "StaticStore.h"
#include "RoutedEvent.g.h"
#include <CStaticLock.h>

using namespace ::Windows::Internal;
using namespace DirectUI;
using namespace DirectUISynonyms;

StaticStore* g_pStaticStore = nullptr;

// Create threadsafe singleton instance of StaticStore if needed.
HRESULT StaticStore::EnsureStaticStore()
{
    CStaticLock lock;

    if (!g_pStaticStore)
    {
        std::unique_ptr<StaticStore> temp(new StaticStore());
        IFC_RETURN(temp->Initialize());
        g_pStaticStore = temp.release();
    }

    return S_OK;
}

// Returns a threadsafe singleton instance of StaticStore.
xref_ptr<StaticStore> StaticStore::GetInstance()
{
    if (!g_pStaticStore)
    {
        IFCFAILFAST(EnsureStaticStore());
    }

    return xref_ptr<StaticStore>(g_pStaticStore);
}

unsigned int StaticStore::AddRef()
{
    CStaticLock lock;
    return InterlockedIncrement(&m_refCount);
}

unsigned int StaticStore::Release()
{
    CStaticLock lock;

    ASSERT(m_refCount != 0);
    unsigned int refCount = InterlockedDecrement(&m_refCount);

    if (refCount == 0)
    {
        g_pStaticStore = nullptr;
        delete this;
    }

    return refCount;
}

_Check_return_ HRESULT StaticStore::Initialize()
{
    IFC_RETURN(ctl::GetActivationFactory(wrl_wrappers::HStringReference(STR_LEN_PAIR(RuntimeClass_Windows_Foundation_PropertyValue)).Get(), &m_spValueFactory));

    // Create commonly used values.
    IFC_RETURN(m_spValueFactory->CreateBoolean(TRUE, &m_spTrueValue));
    IFC_RETURN(m_spValueFactory->CreateBoolean(FALSE, &m_spFalseValue));
    IFC_RETURN(m_spValueFactory->CreateSingle(0.0f, &m_spSingle));
    IFC_RETURN(m_spValueFactory->CreateDouble(0.0, &m_spDouble));
    IFC_RETURN(m_spValueFactory->CreateChar16(L'\0', &m_spChar));
    IFC_RETURN(m_spValueFactory->CreateInt16(0, &m_spInt16));
    IFC_RETURN(m_spValueFactory->CreateUInt16(0, &m_spUInt16));
    IFC_RETURN(m_spValueFactory->CreateInt32(0, &m_spInt32));
    IFC_RETURN(m_spValueFactory->CreateUInt32(0, &m_spUInt32));
    IFC_RETURN(m_spValueFactory->CreateInt64(0, &m_spInt64));
    IFC_RETURN(m_spValueFactory->CreateUInt64(0, &m_spUInt64));
    IFC_RETURN(m_spValueFactory->CreateTimeSpan({}, &m_spTimeSpan));
    IFC_RETURN(m_spValueFactory->CreatePoint({}, &m_spPoint));
    IFC_RETURN(m_spValueFactory->CreateSize({}, &m_spSize));
    IFC_RETURN(m_spValueFactory->CreateRect({}, &m_spRect));
    IFC_RETURN(PropertyValue::CreateReference<xaml::Duration>({}, &m_spDuration));
    IFC_RETURN(PropertyValue::CreateReference<xaml::CornerRadius>({}, &m_spCornerRadius));
    IFC_RETURN(PropertyValue::CreateReference<xaml::Thickness>({}, &m_spThickness));
    IFC_RETURN(PropertyValue::CreateReference<wu::Color>({}, &m_spColor));
    IFC_RETURN(PropertyValue::CreateReference<xaml::GridLength>({}, &m_spGridLength));
    IFC_RETURN(PropertyValue::CreateReference<xaml_media::Matrix>({}, &m_spMatrix));
    IFC_RETURN(PropertyValue::CreateReference<xaml_media::Media3D::Matrix3D>({}, &m_spMatrix3D));
    IFC_RETURN(PropertyValue::CreateReference<xaml_docs::TextRange>({}, &m_spTextRange));
    IFC_RETURN(PropertyValue::CreateEnumReference<xaml::Visibility>(xaml::Visibility::Visibility_Visible, &m_spVisibilityVisibleValue));
    IFC_RETURN(PropertyValue::CreateEnumReference<xaml::Visibility>(xaml::Visibility::Visibility_Collapsed, &m_spVisibilityCollapsedValue));
    IFC_RETURN(PropertyValue::CreateReference<xaml_data::BindingMode>(xaml_data::BindingMode_OneWay, &m_spBindingMode));
    IFC_RETURN(PropertyValue::CreateReference<xaml_data::UpdateSourceTrigger>(xaml_data::UpdateSourceTrigger_Default, &m_spUpdateSourceTrigger));
    IFC_RETURN(ctl::ComObject<ctl::ComBase>::CreateInstance(m_spUnsetValue.GetAddressOf(), TRUE /*fDisableLeakCheck */));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::EnsureUriFactory()
{
    CStaticLock lock;

    if (!m_spUriFactory)
    {
        IFC_RETURN(ctl::GetActivationFactory(wrl_wrappers::HStringReference(STR_LEN_PAIR(RuntimeClass_Windows_Foundation_Uri)).Get(), &m_spUriFactory));
    }

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetUriFactory(_COM_Outptr_ wf::IUriRuntimeClassFactory** result)
{
    *result = nullptr;

    xref_ptr<StaticStore> instance = GetInstance();

    IFC_RETURN(instance->EnsureUriFactory());
    IFC_RETURN(instance->m_spUriFactory.CopyTo(result));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetBoolean(_In_ BOOLEAN bValue, _Outptr_ IInspectable** ppValue)
{
    if (bValue)
    {
        IFC_RETURN(GetInstance()->m_spTrueValue.CopyTo(ppValue));
    }
    else
    {
        IFC_RETURN(GetInstance()->m_spFalseValue.CopyTo(ppValue));
    }

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetUnsetValue(_Outptr_ IInspectable **ppUnsetValue)
{
    IFC_RETURN(GetInstance()->m_spUnsetValue.CopyTo(ppUnsetValue));
    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetVisibilityValue(_In_ xaml::Visibility visibility, _Outptr_ IInspectable** ppValue)
{
    if (visibility == xaml::Visibility::Visibility_Visible)
    {
        IFC_RETURN(GetInstance()->m_spVisibilityVisibleValue.CopyTo(ppValue));
    }
    else if (visibility == xaml::Visibility::Visibility_Collapsed)
    {
        IFC_RETURN(GetInstance()->m_spVisibilityCollapsedValue.CopyTo(ppValue));
    }
    else
    {
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetTextRangeValue(_In_ xaml_docs::TextRange textRange, _Outptr_ IInspectable** ppValue)
{
    IFC_RETURN(GetInstance()->m_spTextRange.CopyTo(ppValue));
    return S_OK;
}

_Check_return_ HRESULT GetEventHelper(
    ctl::ComPtr<DirectUI::RoutedEvent> StaticStore::*field,
    KnownEventIndex index,
    const wchar_t* name,
    _Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFCPTR_RETURN(ppRoutedEvent);

    auto store = StaticStore::GetInstance();
    auto& instField = (store.get())->*field;

    if (!instField)
    {
        IFC_RETURN(ctl::make_threadsafe<DirectUI::RoutedEvent>(&instField, index, wrl_wrappers::HStringReference(name).Get()));
    }

    IFC_RETURN(instField.CopyTo(ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetKeyDownEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spKeyDownEvent,
        KnownEventIndex::UIElement_KeyDown,
        L"UIElement.KeyDown",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetPreviewKeyDownEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spPreviewKeyDownEvent,
        KnownEventIndex::UIElement_PreviewKeyDown,
        L"UIElement.PreviewKeyDown",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetPreviewKeyUpEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spPreviewKeyUpEvent,
        KnownEventIndex::UIElement_PreviewKeyUp,
        L"UIElement.PreviewKeyUp",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetKeyUpEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spKeyUpEvent,
        KnownEventIndex::UIElement_KeyUp,
        L"UIElement.KeyUp",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetCharacterReceivedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spCharacterReceivedEvent,
        KnownEventIndex::UIElement_CharacterReceived,
        L"UIElement.CharacterReceived",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetPointerEnteredEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spPointerEnteredEvent,
        KnownEventIndex::UIElement_PointerEntered,
        L"UIElement.PointerEntered",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetPointerPressedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spPointerPressedEvent,
        KnownEventIndex::UIElement_PointerPressed,
        L"UIElement.PointerPressed",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetPointerMovedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spPointerMovedEvent,
        KnownEventIndex::UIElement_PointerMoved,
        L"UIElement.PointerMoved",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetPointerReleasedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spPointerReleasedEvent,
        KnownEventIndex::UIElement_PointerReleased,
        L"UIElement.PointerReleased",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetPointerExitedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spPointerExitedEvent,
        KnownEventIndex::UIElement_PointerExited,
        L"UIElement.PointerExited",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetPointerCaptureLostEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spPointerCaptureLostEvent,
        KnownEventIndex::UIElement_PointerCaptureLost,
        L"UIElement.PointerCaptureLost",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetPointerCanceledEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spPointerCanceledEvent,
        KnownEventIndex::UIElement_PointerCanceled,
        L"UIElement.PointerCanceled",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetPointerWheelChangedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spPointerWheelChangedEvent,
        KnownEventIndex::UIElement_PointerWheelChanged,
        L"UIElement.PointerWheelChanged",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetTappedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spTappedEvent,
        KnownEventIndex::UIElement_Tapped,
        L"UIElement.Tapped",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetDoubleTappedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spDoubleTappedEvent,
        KnownEventIndex::UIElement_DoubleTapped,
        L"UIElement.DoubleTapped",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetHoldingEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spHoldingEvent,
        KnownEventIndex::UIElement_Holding,
        L"UIElement.Holding",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetRightTappedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spRightTappedEvent,
        KnownEventIndex::UIElement_RightTapped,
        L"UIElement.RightTapped",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetRightTappedUnhandledEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spRightTappedUnhandledEvent,
        KnownEventIndex::UIElement_RightTappedUnhandled,
        L"UIElement.RightTappedUnhandled",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetManipulationStartingEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spManipulationStartingEvent,
        KnownEventIndex::UIElement_ManipulationStarting,
        L"UIElement.ManipulationStarting",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetManipulationInertiaStartingEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spManipulationInertiaStartingEvent,
        KnownEventIndex::UIElement_ManipulationInertiaStarting,
        L"UIElement.ManipulationInertiaStarting",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetManipulationStartedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spManipulationStartedEvent,
        KnownEventIndex::UIElement_ManipulationStarted,
        L"UIElement.ManipulationStarted",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetManipulationDeltaEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spManipulationDeltaEvent,
        KnownEventIndex::UIElement_ManipulationDelta,
        L"UIElement.ManipulationDelta",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetManipulationCompletedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spManipulationCompletedEvent,
        KnownEventIndex::UIElement_ManipulationCompleted,
        L"UIElement.ManipulationCompleted",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetDragEnterEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spDragEnterEvent,
        KnownEventIndex::UIElement_DragEnter,
        L"UIElement.DragEnter",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetDragLeaveEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spDragLeaveEvent,
        KnownEventIndex::UIElement_DragLeave,
        L"UIElement.DragLeave",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetDragOverEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spDragOverEvent,
        KnownEventIndex::UIElement_DragOver,
        L"UIElement.DragOver",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetDropEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spDropEvent,
        KnownEventIndex::UIElement_Drop,
        L"UIElement.Drop",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetGettingFocusEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spGettingFocusEvent,
        KnownEventIndex::UIElement_GettingFocus,
        L"UIElement.GettingFocus",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetLosingFocusEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spLosingFocusEvent,
        KnownEventIndex::UIElement_LosingFocus,
        L"UIElement.LosingFocus",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetNoFocusCandidateFoundEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spNoFocusCandidateFoundEvent,
        KnownEventIndex::UIElement_NoFocusCandidateFound,
        L"UIElement.NoFocusCandidateFound",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetBringIntoViewRequestedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spBringIntoViewRequestedEvent,
        KnownEventIndex::UIElement_BringIntoViewRequested,
        L"UIElement.BringIntoViewRequested",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetContextRequestedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent)
{
    IFC_RETURN(GetEventHelper(
        &StaticStore::m_spContextRequestedEvent,
        KnownEventIndex::UIElement_ContextRequested,
        L"UIElement.ContextRequested",
        ppRoutedEvent));

    return S_OK;
}

_Check_return_ HRESULT StaticStore::GetDefaultValue(_In_ KnownTypeIndex nTypeIndex, _Outptr_ IInspectable** ppValue)
{
    auto store = GetInstance();

    switch (nTypeIndex)
    {
        case KnownTypeIndex::Float:
            IFC_RETURN(store->m_spSingle.CopyTo(ppValue));
            break;
        case KnownTypeIndex::Double:
            IFC_RETURN(store->m_spDouble.CopyTo(ppValue));
            break;
        case KnownTypeIndex::Char16:
            IFC_RETURN(store->m_spChar.CopyTo(ppValue));
            break;
        case KnownTypeIndex::Int16:
            IFC_RETURN(store->m_spInt16.CopyTo(ppValue));
            break;
        case KnownTypeIndex::UInt16:
            IFC_RETURN(store->m_spUInt16.CopyTo(ppValue));
            break;
        case KnownTypeIndex::Int32:
            IFC_RETURN(store->m_spInt32.CopyTo(ppValue));
            break;
        case KnownTypeIndex::UInt32:
            IFC_RETURN(store->m_spUInt32.CopyTo(ppValue));
            break;
        case KnownTypeIndex::Int64:
            IFC_RETURN(store->m_spInt64.CopyTo(ppValue));
            break;
        case KnownTypeIndex::UInt64:
            IFC_RETURN(store->m_spUInt64.CopyTo(ppValue));
            break;
        case KnownTypeIndex::Boolean:
            IFC_RETURN(store->m_spFalseValue.CopyTo(ppValue));
            break;
        case KnownTypeIndex::TimeSpan:
            IFC_RETURN(store->m_spTimeSpan.CopyTo(ppValue));
            break;
        case KnownTypeIndex::Point:
            IFC_RETURN(store->m_spPoint.CopyTo(ppValue));
            break;
        case KnownTypeIndex::Size:
            IFC_RETURN(store->m_spSize.CopyTo(ppValue));
            break;
        case KnownTypeIndex::Rect:
            IFC_RETURN(store->m_spRect.CopyTo(ppValue));
            break;
        case KnownTypeIndex::Duration:
            IFC_RETURN(store->m_spDuration.CopyTo(ppValue));
            break;
        case KnownTypeIndex::CornerRadius:
            IFC_RETURN(store->m_spCornerRadius.CopyTo(ppValue));
            break;
        case KnownTypeIndex::Thickness:
            IFC_RETURN(store->m_spThickness.CopyTo(ppValue));
            break;
        case KnownTypeIndex::Color:
            IFC_RETURN(store->m_spColor.CopyTo(ppValue));
            break;
        case KnownTypeIndex::GridLength:
            IFC_RETURN(store->m_spGridLength.CopyTo(ppValue));
            break;
        case KnownTypeIndex::Matrix:
            IFC_RETURN(store->m_spMatrix.CopyTo(ppValue));
            break;
        case KnownTypeIndex::Matrix3D:
            IFC_RETURN(store->m_spMatrix3D.CopyTo(ppValue));
            break;
        case KnownTypeIndex::TextRange:
            IFC_RETURN(store->m_spTextRange.CopyTo(ppValue));
            break;
        case KnownTypeIndex::BindingMode:
            IFC_RETURN(store->m_spBindingMode.CopyTo(ppValue));
            break;
        case KnownTypeIndex::UpdateSourceTrigger:
            IFC_RETURN(store->m_spUpdateSourceTrigger.CopyTo(ppValue));
            break;
        case KnownTypeIndex::String:
            *ppValue = nullptr;
            break;
        default:
        {
            // unexpected value type
            IFCEXPECT_RETURN(FALSE);
            break;
        }
    }

    return S_OK;
}