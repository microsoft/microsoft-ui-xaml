// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "DataContextEventRecorder.h"

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Data;

using namespace Microsoft::UI::Xaml::Tests::Controls::ContentPresenter;

Microsoft::UI::Xaml::DependencyProperty^ DataContextEventRecorder::s_listenerDataContextProperty = nullptr;

void DataContextEventRecorder::AttachTo(Microsoft::UI::Xaml::Controls::ContentPresenter^ presenter)
{
    MonitoredPresenter entry;
    entry.Presenter = presenter;
    entry.DataContextChangedToken =
        presenter->DataContextChanged += ref new TypedEventHandler<FrameworkElement ^, DataContextChangedEventArgs ^>(this, &DataContextEventRecorder::OnDataContextChanged);
    
    presenter->Tag = this;

    Binding^ contentBinding = ref new Binding();
    contentBinding->Source = presenter;
    contentBinding->Path = ref new PropertyPath(L"DataContext");

    if (s_listenerDataContextProperty == nullptr)
    {
        s_listenerDataContextProperty =
        Microsoft::UI::Xaml::DependencyProperty::RegisterAttached(
            L"ListenerDataContext",
            Platform::Object::typeid,
            DataContextEventRecorder::typeid,
            ref new Microsoft::UI::Xaml::PropertyMetadata(nullptr, ref new Microsoft::UI::Xaml::PropertyChangedCallback(DataContextEventRecorder::OnListenerDataContextPropertyChanged)));
    }

    BindingOperations::SetBinding(presenter, s_listenerDataContextProperty, contentBinding);

    m_monitoredPresenters.emplace_back(std::move(entry));
}

void DataContextEventRecorder::ClearEntries()
{
    m_recordedChanges.clear();
    m_handleDataContextChangedEvent = false;
}

void DataContextEventRecorder::DetachFrom(Microsoft::UI::Xaml::Controls::ContentPresenter^ presenter)
{
    auto it = std::find_if(
        m_monitoredPresenters.begin(),
        m_monitoredPresenters.end(),
        [presenter](const MonitoredPresenter& other) { return other.Presenter == presenter; });

    if (it != m_monitoredPresenters.end())
    {
        presenter->DataContextChanged -= it->DataContextChangedToken;
        presenter->Tag = nullptr;
        m_monitoredPresenters.erase(it);
    }
}

void DataContextEventRecorder::HandleDataContextChangedEvent()
{
    m_handleDataContextChangedEvent = true;
}

void DataContextEventRecorder::Reset()
{
    for (auto& entry : m_monitoredPresenters)
    {
        DetachFrom(entry.Presenter);
    }
}

void DataContextEventRecorder::OnDataContextChanged(FrameworkElement^ sender, DataContextChangedEventArgs^ args)
{
    RecordedChange change;
    change.Type = RecordedChangeType::FrameworkElement_DataContextChanged;
    change.Presenter = safe_cast<xaml_controls::ContentPresenter^>(sender);
    change.NewValue = safe_cast<Platform::String^>(args->NewValue);
    change.OldValue = nullptr;

    m_recordedChanges.emplace_back(std::move(change));

    args->Handled = m_handleDataContextChangedEvent;

    VERIFY_ARE_EQUAL(change.Presenter->DataContext, args->NewValue);
}

/*static*/
void DataContextEventRecorder::OnListenerDataContextPropertyChanged(DependencyObject^ sender, DependencyPropertyChangedEventArgs^ args)
{
    DataContextEventRecorder::RecordedChange change;
    change.Type = DataContextEventRecorder::RecordedChangeType::DataContextPropertyChanged;
    change.Presenter = safe_cast<xaml_controls::ContentPresenter^>(sender);
    change.NewValue = safe_cast<Platform::String^>(args->NewValue);
    change.OldValue = safe_cast<Platform::String^>(args->OldValue);

    auto self = safe_cast<DataContextEventRecorder^>(change.Presenter->Tag);
    self->m_recordedChanges.emplace_back(std::move(change));

    VERIFY_ARE_EQUAL(change.Presenter->DataContext, args->NewValue);
}