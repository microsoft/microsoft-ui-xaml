// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ContentPresenter {

    ref class DataContextEventRecorder
    {
    internal:
        enum class RecordedChangeType
        {
            DataContextPropertyChanged, // use data binding
            FrameworkElement_DataContextChanged
        };

        struct RecordedChange
        {
            RecordedChangeType Type;
            Microsoft::UI::Xaml::Controls::ContentPresenter^ Presenter;
            Platform::String^ OldValue;
            Platform::String^ NewValue;
        };

        void AttachTo(Microsoft::UI::Xaml::Controls::ContentPresenter^ presenter);
        void ClearEntries();
        void DetachFrom(Microsoft::UI::Xaml::Controls::ContentPresenter^ presenter);
        void HandleDataContextChangedEvent();
        void Reset();

        const std::vector<RecordedChange>& GetRecordedChanges() const { return m_recordedChanges; }

    private:
        void OnDataContextChanged(Microsoft::UI::Xaml::FrameworkElement ^sender, Microsoft::UI::Xaml::DataContextChangedEventArgs ^args);
        static void OnListenerDataContextPropertyChanged(Microsoft::UI::Xaml::DependencyObject^ sender, Microsoft::UI::Xaml::DependencyPropertyChangedEventArgs^ args);

    private:
        struct MonitoredPresenter
        {
            Microsoft::UI::Xaml::Controls::ContentPresenter^ Presenter;
            ::Windows::Foundation::EventRegistrationToken DataContextChangedToken;
        };
        
        std::vector<RecordedChange> m_recordedChanges;
        std::vector<MonitoredPresenter> m_monitoredPresenters;
        bool m_handleDataContextChangedEvent;
        static Microsoft::UI::Xaml::DependencyProperty^ s_listenerDataContextProperty;
    };

} } } } } }