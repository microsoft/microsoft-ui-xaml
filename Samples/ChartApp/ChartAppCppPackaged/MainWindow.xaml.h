#pragma once

#include "MainWindow.g.h"
#include <winrt/Microsoft.UI.Xaml.Controls.Charts.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <functional>
#include <memory>
#include <thread>

namespace winrt::ChartAppCppPackaged::implementation
{
    struct SecondaryChartState;

    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        void OnLightClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnDarkClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnSystemClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnToggleUpdatesClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnToggleSecondaryClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnToggleBarOrientationClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnLegendVisibilityClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnApplyLegendTitleClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnPresentationSeriesChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
        void OnShowDataLabelsClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnShowDataMarkersClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnDataLabelBrushClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnDataMarkerBrushClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnOverrideIndexChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::NumberBoxValueChangedEventArgs const&);
        void OnOverrideIndexLostFocus(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnApplyLabelOverrideClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnRemoveLabelOverrideClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnApplyMarkerOverrideClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnRemoveMarkerOverrideClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnClearOverridesClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnLinearMinChanged(Microsoft::UI::Xaml::Controls::NumberBox const&, Microsoft::UI::Xaml::Controls::NumberBoxValueChangedEventArgs const&);
        void OnLinearMaxChanged(Microsoft::UI::Xaml::Controls::NumberBox const&, Microsoft::UI::Xaml::Controls::NumberBoxValueChangedEventArgs const&);
        void OnLinearSpacingChanged(Microsoft::UI::Xaml::Controls::NumberBox const&, Microsoft::UI::Xaml::Controls::NumberBoxValueChangedEventArgs const&);
        void OnAxisNumberLostFocus(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnSortKeyChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
        void OnSortOrderChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
        void OnTickLabelsChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnTickMarksChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnAxisVisibleChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnGridLinesChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
        void OnGridLineBrushChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
        void OnTickBrushChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
        void OnTickLabelBrushChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
        void OnAxisLineBrushChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
        void OnDtIntervalTypeAChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
        void OnDtLabelFormatAApply(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnDtIntervalTypeBChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
        void OnDtLabelFormatBApply(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);

    private:
        void ApplyEdit(std::function<void()> const& edit);
        void ReportError(winrt::hresult_error const& error);
        void SyncPresentationKnobs();
        void SyncAxisControls();
        void QueueLinearAxisEdit(Microsoft::UI::Xaml::Controls::NumberBox const& box, int property);
        void QueueOverrideIndexEdit(double previousValue, bool synchronize);
        Microsoft::UI::Xaml::Controls::Charts::LineSeries SelectedPresentationSeries();
        uint32_t SelectedOverrideIndex();
        void CreateCodeChart();
        void CreateDateTimeCharts();
        void UpdateCodeChartData();
        void UpdateDataText();
        void ApplyDateInterval(bool monthly);
        void ApplyDateFormat(bool monthly);
        void PollSecondaryWindow();
        void StopPrimaryTimer();

        Microsoft::UI::Xaml::Controls::Charts::Chart m_codeChart{ nullptr };
        Microsoft::UI::Xaml::Controls::Charts::LinearAxis m_yAxis{ nullptr };
        Microsoft::UI::Xaml::Controls::Charts::CategoryAxis m_xAxis{ nullptr };
        Microsoft::UI::Xaml::Controls::Charts::DateTimeAxis m_dtAxisA{ nullptr };
        Microsoft::UI::Xaml::Controls::Charts::DateTimeAxis m_dtAxisB{ nullptr };
        Windows::Foundation::Collections::IObservableVector<double> m_codeChartValues{ nullptr };
        Windows::Foundation::Collections::IObservableVector<double> m_markupProfitValues{ nullptr };
        Windows::Foundation::Collections::IObservableVector<double> m_markupExpenseValues{ nullptr };
        Microsoft::UI::Xaml::DispatcherTimer m_codeChartTimer{ nullptr };
        Microsoft::UI::Xaml::DispatcherTimer m_secondaryPoll{ nullptr };
        winrt::event_token m_codeTick{};
        winrt::event_token m_pollTick{};
        uint32_t m_codeChartUpdateIndex{};
        bool m_ready{};
        bool m_syncing{};
        bool m_closing{};
        std::shared_ptr<SecondaryChartState> m_secondaryState;
        winrt::com_ptr<MainWindow> m_secondaryLifetime;
        std::thread m_secondaryThread;
    };
}

namespace winrt::ChartAppCppPackaged::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
