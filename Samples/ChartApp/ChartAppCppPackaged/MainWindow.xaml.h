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

        void OnLayoutSizeChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::SizeChangedEventArgs const&);
        void OnWorkspaceSizeChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::SizeChangedEventArgs const&);
        void OnViewportSizeChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::SizeChangedEventArgs const&);
        void OnEditorSizeChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::SizeChangedEventArgs const&);
        void OnDateLayoutSizeChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::SizeChangedEventArgs const&);
        void OnScenarioSelectionChanged(Microsoft::UI::Xaml::Controls::NavigationView const&, Microsoft::UI::Xaml::Controls::NavigationViewSelectionChangedEventArgs const&);
        void OnThemeChoiceChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
        void OnLightClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnDarkClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnSystemClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnToggleUpdatesClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnToggleSecondaryClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnLineChoiceChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
        void OnLineVisibleClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnLineAddSeriesClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnLineRemoveSeriesClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnLineResetClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnAreaSeriesChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
        void OnAreaAddSeriesClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnAreaRemoveSeriesClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnAreaMarkerChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
        void OnAreaChoiceChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
        void OnAreaVisibleClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnAreaValuesClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnAreaMarkersClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnAreaLegendClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnAreaResetClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnBarSeriesChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
        void OnBarAddSeriesClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnBarRemoveSeriesClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnBarMarkerChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
        void OnBarMarkersClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnBarOrientationChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
        void OnBarColorChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
        void OnBarVisibleClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnBarValuesClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnBarLegendClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnBarResetClick(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
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
        void UpdateWorkspaceLayout(Microsoft::UI::Xaml::Controls::Grid const& workspace);
        winrt::hstring SeriesDataText(Microsoft::UI::Xaml::Controls::Charts::CartesianSeries const& series);
        void ApplyEdit(std::function<void()> const& edit,
            Microsoft::UI::Xaml::Controls::Charts::Chart const& chart = nullptr);
        void ApplyExampleEdit(bool area, std::function<void()> const& edit, winrt::hstring const& message);
        void ApplyLineEdit(std::function<void()> const& edit, winrt::hstring const& message);
        void RebuildSeriesSelector(Microsoft::UI::Xaml::Controls::Charts::Chart const& chart,
            Microsoft::UI::Xaml::Controls::ComboBox const& selector, int32_t selectedIndex);
        void AddExampleSeries(int32_t example);
        void RemoveSelectedSeries(Microsoft::UI::Xaml::Controls::Charts::Chart const& chart,
            Microsoft::UI::Xaml::Controls::ComboBox const& selector);
        bool HasProfitSeries();
        void SyncAxisAvailability();
        void SyncAreaOptions();
        void SyncBarOptions();
        void SetAreaAppearance(int32_t colorIndex, int32_t fillIndex);
        void SetAreaMarkers(bool visible);
        void SetBarColor(int32_t colorIndex);
        void ReportError(winrt::hresult_error const& error);
        void SyncPresentationKnobs();
        void SyncAxisControls(bool includeDateTime = true);
        void QueueLinearAxisEdit(Microsoft::UI::Xaml::Controls::NumberBox const& box, int property);
        void QueueOverrideIndexEdit(double previousValue, bool synchronize);
        Microsoft::UI::Xaml::Controls::Charts::LineSeries SelectedPresentationSeries();
        Microsoft::UI::Xaml::Controls::Charts::AreaSeries SelectedAreaSeries();
        Microsoft::UI::Xaml::Controls::Charts::BarSeries SelectedBarSeries();
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
        Microsoft::UI::Xaml::Controls::Charts::CategoryAxis m_barXAxis{ nullptr };
        Microsoft::UI::Xaml::Controls::Charts::LinearAxis m_barYAxis{ nullptr };
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
        uint32_t m_nextLineSeries{ 3 };
        uint32_t m_nextAreaSeries{ 2 };
        uint32_t m_nextBarSeries{ 2 };
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
