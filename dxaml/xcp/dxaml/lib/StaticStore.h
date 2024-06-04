// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines a class for encapsulating process-global data in a thread-safe way.

#pragma once

namespace DirectUI
{
    class RoutedEvent;

    class StaticStore
    {
    private:
        // Initialize the metadata store by reading in all of the known
        // namespace, type, property, and event info from the static tables
        // in TypeInfo.g.h.
        _Check_return_ HRESULT Initialize();

        _Check_return_ HRESULT EnsureUriFactory();

        // Create thread-safe singleton instance of StaticStore if needed.
        static HRESULT EnsureStaticStore();

    public:
        static xref_ptr<StaticStore> GetInstance();

        // Ref counting for the singleton.
        unsigned int AddRef();
        unsigned int Release();

        static _Check_return_ HRESULT GetBoolean(_In_ BOOLEAN bValue, _Outptr_ IInspectable** ppValue);

        static _Check_return_ HRESULT GetDefaultValue(_In_ KnownTypeIndex nTypeIndex, _Outptr_ IInspectable** ppValue);

        static _Check_return_ HRESULT GetUnsetValue(_Outptr_ IInspectable **ppUnsetValue);

        static _Check_return_ HRESULT GetVisibilityValue(_In_ xaml::Visibility visibility, _Outptr_ IInspectable** ppValue);

        static _Check_return_ HRESULT GetTextRangeValue(_In_ xaml_docs::TextRange textRange, _Outptr_ IInspectable** ppValue);

        static wf::IPropertyValueStatics* GetPropertyValueStaticsNoRef()
        {
            return GetInstance()->m_spValueFactory.Get();
        }

        static _Check_return_ HRESULT GetUriFactory(_COM_Outptr_ wf::IUriRuntimeClassFactory** result);

        static _Check_return_ HRESULT GetKeyDownEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetPreviewKeyDownEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetKeyUpEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetPreviewKeyUpEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetPointerEnteredEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetPointerPressedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetPointerMovedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetPointerReleasedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetPointerExitedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetPointerCaptureLostEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetPointerCanceledEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetPointerWheelChangedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetTappedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetDoubleTappedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetHoldingEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetRightTappedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetRightTappedUnhandledEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetManipulationStartingEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetManipulationInertiaStartingEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetManipulationStartedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetManipulationDeltaEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetManipulationCompletedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetDragEnterEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetDragLeaveEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetDragOverEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetDropEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetGettingFocusEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetLosingFocusEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetNoFocusCandidateFoundEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetCharacterReceivedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetBringIntoViewRequestedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);
        static _Check_return_ HRESULT GetContextRequestedEvent(_Outptr_ xaml::IRoutedEvent** ppRoutedEvent);

    private:
        ctl::ComPtr<wf::IPropertyValueStatics> m_spValueFactory;
        ctl::ComPtr<wf::IUriRuntimeClassFactory> m_spUriFactory;
        ctl::ComPtr<IInspectable> m_spTrueValue;
        ctl::ComPtr<IInspectable> m_spFalseValue;
        ctl::ComPtr<IInspectable> m_spSingle;
        ctl::ComPtr<IInspectable> m_spDouble;
        ctl::ComPtr<IInspectable> m_spChar;
        ctl::ComPtr<IInspectable> m_spInt16;
        ctl::ComPtr<IInspectable> m_spUInt16;
        ctl::ComPtr<IInspectable> m_spInt32;
        ctl::ComPtr<IInspectable> m_spUInt32;
        ctl::ComPtr<IInspectable> m_spInt64;
        ctl::ComPtr<IInspectable> m_spUInt64;
        ctl::ComPtr<IInspectable> m_spTimeSpan;
        ctl::ComPtr<IInspectable> m_spPoint;
        ctl::ComPtr<IInspectable> m_spSize;
        ctl::ComPtr<IInspectable> m_spRect;
        ctl::ComPtr<IInspectable> m_spDuration;
        ctl::ComPtr<IInspectable> m_spCornerRadius;
        ctl::ComPtr<IInspectable> m_spThickness;
        ctl::ComPtr<IInspectable> m_spColor;
        ctl::ComPtr<IInspectable> m_spGridLength;
        ctl::ComPtr<IInspectable> m_spMatrix;
        ctl::ComPtr<IInspectable> m_spMatrix3D;
        ctl::ComPtr<IInspectable> m_spTextRange;
        ctl::ComPtr<IInspectable> m_spBindingMode;
        ctl::ComPtr<IInspectable> m_spUpdateSourceTrigger;
        ctl::ComPtr<IInspectable> m_spUnsetValue;
        ctl::ComPtr<IInspectable> m_spVisibilityVisibleValue;
        ctl::ComPtr<IInspectable> m_spVisibilityCollapsedValue;

        ctl::ComPtr<DirectUI::RoutedEvent> m_spKeyDownEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spKeyUpEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spPreviewKeyDownEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spPreviewKeyUpEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spCharacterReceivedEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spPointerEnteredEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spPointerPressedEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spPointerMovedEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spPointerReleasedEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spPointerExitedEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spPointerCaptureLostEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spPointerCanceledEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spPointerWheelChangedEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spTappedEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spDoubleTappedEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spHoldingEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spRightTappedEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spRightTappedUnhandledEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spManipulationStartingEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spManipulationInertiaStartingEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spManipulationStartedEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spManipulationDeltaEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spManipulationCompletedEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spDragEnterEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spDragLeaveEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spDragOverEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spDropEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spGettingFocusEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spLosingFocusEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spNoFocusCandidateFoundEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spBringIntoViewRequestedEvent;
        ctl::ComPtr<DirectUI::RoutedEvent> m_spContextRequestedEvent;

        unsigned int m_refCount = 0;
    };
}
