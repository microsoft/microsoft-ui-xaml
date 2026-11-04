#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <winrt/Microsoft.UI.Xaml.Automation.h>
#include <winrt/Microsoft.UI.Xaml.Automation.Peers.h>
#include <winrt/Microsoft.UI.Xaml.Hosting.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Windows.Globalization.DateTimeFormatting.h>
#include <winrt/Windows.Globalization.NumberFormatting.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <vector>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Charts;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Automation;
using namespace Microsoft::UI::Dispatching;

namespace
{
    constexpr Windows::UI::Color LabelBlue{ 255, 0, 99, 177 };
    constexpr Windows::UI::Color MarkerOrange{ 255, 216, 59, 1 };
    constexpr Windows::UI::Color LabelGreen{ 255, 16, 124, 16 };
    constexpr Windows::UI::Color MarkerPurple{ 255, 136, 23, 152 };
    constexpr std::array MarkerShapes{
        MarkerShape::None, MarkerShape::Square, MarkerShape::Diamond, MarkerShape::Triangle,
        MarkerShape::X, MarkerShape::Asterisk, MarkerShape::ShortDash, MarkerShape::LongDash,
        MarkerShape::Circle, MarkerShape::Plus
    };
    constexpr std::array IntervalTypes{
        DateTimeIntervalType::Auto, DateTimeIntervalType::Day, DateTimeIntervalType::Week,
        DateTimeIntervalType::Month, DateTimeIntervalType::Year
    };
    constexpr std::array AxisColors{
        Windows::UI::Color{ 255, 220, 40, 40 },
        Windows::UI::Color{ 255, 40, 80, 220 },
        Windows::UI::Color{ 255, 40, 180, 60 }
    };
    constexpr Windows::UI::Color AreaOriginalFill{ 0x60, 0x4F, 0x6B, 0xED };
    constexpr Windows::UI::Color AreaOriginalStroke{ 0xFF, 0x30, 0x47, 0xB8 };
    constexpr Windows::UI::Color BarOriginalFill{ 0xFF, 0x0F, 0x6C, 0xBD };
    constexpr Windows::UI::Color BarOriginalStroke{ 0xFF, 0x07, 0x3B, 0x66 };
    constexpr std::array SeriesColors{
        Windows::UI::Color{ 0xFF, 0x0F, 0x6C, 0xBD },
        Windows::UI::Color{ 0xFF, 0x00, 0x82, 0x72 },
        Windows::UI::Color{ 0xFF, 0xFF, 0xB9, 0x00 },
        Windows::UI::Color{ 0xFF, 0x87, 0x64, 0xB8 },
        Windows::UI::Color{ 0xFF, 0xE3, 0x00, 0x8C },
        Windows::UI::Color{ 0xFF, 0x69, 0x79, 0x7E },
        Windows::UI::Color{ 0xFF, 0x10, 0x7C, 0x10 },
        Windows::UI::Color{ 0xFF, 0xD8, 0x3B, 0x01 }
    };
    constexpr std::array<uint8_t, 3> AreaFillAlphas{ 0x60, 0xFF, 0x00 };
    constexpr std::array<double, 4> LineWeights{ 1, 2, 3, 5 };
    constexpr std::array LineStyles{
        StrokeDashStyle::Solid, StrokeDashStyle::Dash, StrokeDashStyle::Dot,
        StrokeDashStyle::DashDot, StrokeDashStyle::DashDotDot
    };
    constexpr std::array<double, 6> ProfitSeed{ 18, 27, 22, 41, 36, 52 };
    constexpr std::array<double, 6> ExpensesSeed{ 31, 25, 29, 24, 32, 28 };
    constexpr std::array<double, 6> AreaSeed{ 8, 18, 14, 29, 24, 37 };
    constexpr std::array<double, 6> BarSeed{ 12, 20, 17, 31, 26, 39 };

    struct SyncGuard
    {
        explicit SyncGuard(bool& flag) : m_flag(flag), m_previous(flag) { m_flag = true; }
        ~SyncGuard() { m_flag = m_previous; }
        SyncGuard(SyncGuard const&) = delete;
        SyncGuard& operator=(SyncGuard const&) = delete;
    private:
        bool& m_flag;
        bool m_previous;
    };

    bool SameRgb(Windows::UI::Color const& left, Windows::UI::Color const& right)
    {
        return left.R == right.R && left.G == right.G && left.B == right.B;
    }

    int32_t SeriesColorIndex(Brush const& fill, Brush const& stroke, bool area)
    {
        auto solidFill = fill.try_as<SolidColorBrush>();
        auto solidStroke = stroke.try_as<SolidColorBrush>();
        if (!solidFill || !solidStroke) return -1;
        auto fillColor = solidFill.Color();
        auto strokeColor = solidStroke.Color();
        if (strokeColor.A != 0xFF || (!area && fillColor.A != 0xFF)) return -1;
        if (SameRgb(fillColor, area ? AreaOriginalFill : BarOriginalFill) &&
            SameRgb(strokeColor, area ? AreaOriginalStroke : BarOriginalStroke)) return 0;
        for (size_t i = 0; i < SeriesColors.size(); ++i)
        {
            if (SameRgb(fillColor, SeriesColors[i]) && SameRgb(strokeColor, SeriesColors[i]))
                return static_cast<int32_t>(i + 1);
        }
        return -1;
    }

    int32_t AreaFillIndex(Brush const& fill)
    {
        if (auto solid = fill.try_as<SolidColorBrush>())
        {
            for (size_t i = 0; i < AreaFillAlphas.size(); ++i)
            {
                if (solid.Color().A == AreaFillAlphas[i]) return static_cast<int32_t>(i);
            }
        }
        return -1;
    }

    int32_t LineColorIndex(Brush const& stroke)
    {
        if (!stroke) return 0;
        if (auto solid = stroke.try_as<SolidColorBrush>())
        {
            for (size_t i = 0; i < SeriesColors.size(); ++i)
                if (solid.Color().A == 0xFF && SameRgb(solid.Color(), SeriesColors[i]))
                    return static_cast<int32_t>(i + 1);
        }
        return -1;
    }

    template<typename Collection, typename Value>
    void RemoveItem(Collection const& collection, Value const& value)
    {
        uint32_t index{};
        if (value && collection.IndexOf(value, index)) collection.RemoveAt(index);
    }

    void ResetSeriesDefaults(CartesianSeries const& series, double thickness, bool markers, bool labels)
    {
        series.IsVisible(true);
        series.Stroke(nullptr);
        series.StrokeThickness(thickness);
        series.StrokeDashStyle(StrokeDashStyle::Solid);
        series.MarkerShape(MarkerShape::Circle);
        series.ShowDataMarkers(markers);
        series.ShowDataLabels(labels);
        series.DataMarkerBrush(nullptr);
        series.DataLabelBrush(nullptr);
        series.DataMarkerOverrides().Clear();
        series.DataLabelOverrides().Clear();
    }

    void ResetAxisAppearance(CartesianAxis const& axis, CartesianAxis const& defaults)
    {
        axis.IsVisible(defaults.IsVisible());
        axis.ShowTickLabels(defaults.ShowTickLabels());
        axis.ShowTickMarks(defaults.ShowTickMarks());
        axis.GridLines(defaults.GridLines());
        axis.GridLineMajorBrush(defaults.GridLineMajorBrush());
        axis.TickBrush(defaults.TickBrush());
        axis.TickLabelBrush(defaults.TickLabelBrush());
        axis.AxisLineBrush(defaults.AxisLineBrush());
    }

    bool Checked(CheckBox const& box)
    {
        return box.IsChecked() && box.IsChecked().Value();
    }

    Brush SelectedBrush(CheckBox const& box, Windows::UI::Color const& color)
    {
        return Checked(box) ? SolidColorBrush{ color }.as<Brush>() : nullptr;
    }

    int32_t Selection(ComboBox const& box, int32_t count)
    {
        auto index = box.SelectedIndex();
        if (index < 0 || index >= count)
        {
            throw hresult_invalid_argument(L"Choose an available option.");
        }
        return index;
    }

    Brush AxisBrush(ComboBox const& box)
    {
        auto index = Selection(box, 4);
        return index == 0 ? nullptr : SolidColorBrush{ AxisColors[index - 1] }.as<Brush>();
    }

    int32_t BrushIndex(Brush const& brush)
    {
        if (!brush) return 0;
        if (auto solid = brush.try_as<SolidColorBrush>())
        {
            auto color = solid.Color();
            for (size_t i = 0; i < AxisColors.size(); ++i)
            {
                auto expected = AxisColors[i];
                if (color.A == expected.A && color.R == expected.R &&
                    color.G == expected.G && color.B == expected.B)
                {
                    return static_cast<int32_t>(i + 1);
                }
            }
        }
        return -1;
    }

    TextBox NumberInput(DependencyObject const& root)
    {
        if (auto input = root.try_as<TextBox>()) return input;
        for (int32_t i = 0; i < VisualTreeHelper::GetChildrenCount(root); ++i)
        {
            if (auto input = NumberInput(VisualTreeHelper::GetChild(root, i))) return input;
        }
        return nullptr;
    }

    IReference<double> AutoNumber(NumberBox const& box)
    {
        // With ValidationMode="Disabled", Value is NaN for both empty and invalid text, so read the text.
        auto input = NumberInput(box);
        auto text = input ? input.Text() : box.Text();
        if (std::wstring_view{ text }.find_first_not_of(L" \t\r\n") == std::wstring_view::npos) return nullptr;
        auto value = box.NumberFormatter().as<Windows::Globalization::NumberFormatting::INumberParser>().ParseDouble(text);
        if (!value || !std::isfinite(value.Value()))
        {
            throw hresult_invalid_argument(L"Enter a finite number, or clear the axis field for Auto.");
        }
        return value;
    }

    void SyncNumber(NumberBox const& box, IReference<double> const& value)
    {
        box.Value(value ? value.Value() : std::numeric_limits<double>::quiet_NaN());
        auto text = value ? box.NumberFormatter().FormatDouble(value.Value()) : L"";
        box.Text(text);
        if (auto input = NumberInput(box)) input.Text(text);
    }

    hstring ErrorText(hresult_error const& error)
    {
        std::wostringstream text;
        auto message = error.message();
        text << (message.empty() ? L"The chart operation could not be completed." : message.c_str());
        text << L" (HRESULT 0x" << std::uppercase << std::hex << std::setw(8) << std::setfill(L'0')
             << static_cast<uint32_t>(error.code().value) << L")";
        return hstring{ text.str() };
    }

    hstring DateIntervalNote(DateTimeIntervalType interval, bool monthly)
    {
        if (interval == DateTimeIntervalType::Auto)
            return L"Known issue: after switching back to Auto, points can keep their previous positions. Select Day, then Auto, to restore them.";
        if (!monthly && interval == DateTimeIntervalType::Year)
            return L"Year is not meaningful for a 75-day range.";
        if (monthly && (interval == DateTimeIntervalType::Day || interval == DateTimeIntervalType::Week))
            return L"Day and Week ticks are too dense for a three-year range.";
        return {};
    }

    DateTime CalendarDate(WORD year, WORD month, WORD day)
    {
        SYSTEMTIME time{};
        time.wYear = year;
        time.wMonth = month;
        time.wDay = day;
        FILETIME fileTime{};
        check_bool(SystemTimeToFileTime(&time, &fileTime));
        return clock::from_file_time(fileTime);
    }

    hstring DataRows(IVector<double> const& values)
    {
        std::wostringstream text;
        for (uint32_t i = 0; i < values.Size(); ++i)
        {
            if (i) text << L", ";
            text << values.GetAt(i);
        }
        return hstring{ text.str() };
    }

}

namespace winrt::ChartsSample::implementation
{
    struct SecondaryChartState
    {
        std::atomic<bool> stop{};
        std::atomic<bool> done{};
        hstring error;
    };

    namespace
    {
        void RunSecondaryWindow(std::shared_ptr<SecondaryChartState> const& state)
        {
            DispatcherQueueController controller{ nullptr };
            Microsoft::UI::Xaml::Hosting::WindowsXamlManager manager{ nullptr };
            Window window{ nullptr };
            DispatcherTimer timer{ nullptr };
            event_token tick{};
            event_token closed{};
            bool apartmentInitialized{};
            bool windowClosed{};
            hstring operation{ L"Initialize the secondary UI thread" };
            auto rememberError = [&](hresult_error const& error)
            {
                if (state->error.empty()) state->error = operation + L": " + ErrorText(error);
            };
            try
            {
                init_apartment(apartment_type::single_threaded);
                apartmentInitialized = true;
                controller = DispatcherQueueController::CreateOnCurrentThread();
                auto dispatcher = controller.DispatcherQueue();
                operation = L"Initialize XAML on the secondary UI thread";
                manager = Microsoft::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();

                // Every XAML object, resource and observable collection belongs to this STA.
                auto root = StackPanel{};
                root.Padding(Thickness{ 24 });
                root.Spacing(12);
                operation = L"Create secondary chart resources";
                constexpr wchar_t resourceMarkup[] = LR"(
                    <ResourceDictionary xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                                        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <ResourceDictionary.MergedDictionaries>
                            <XamlControlsResources xmlns="using:Microsoft.UI.Xaml.Controls" />
                            <XamlChartsResources xmlns="using:Microsoft.UI.Xaml.Controls.Charts" />
                            <ResourceDictionary>
                                <ResourceDictionary.ThemeDictionaries>
                                    <ResourceDictionary x:Key="Light">
                                        <SolidColorBrush x:Key="ChartsControlForegroundBrush" Color="#FF1B1B1B" />
                                    </ResourceDictionary>
                                    <ResourceDictionary x:Key="Dark">
                                        <SolidColorBrush x:Key="ChartsControlBackgroundBrush" Color="#FF0B1A2A" />
                                        <SolidColorBrush x:Key="ChartsControlForegroundBrush" Color="#FFE8F1FF" />
                                    </ResourceDictionary>
                                </ResourceDictionary.ThemeDictionaries>
                            </ResourceDictionary>
                        </ResourceDictionary.MergedDictionaries>
                    </ResourceDictionary>)";
                auto resources = Markup::XamlReader::Load(resourceMarkup).as<ResourceDictionary>();
                root.Resources(resources);
                operation = L"Create secondary chart content";
                auto heading = TextBlock{};
                heading.Text(L"Chart on a secondary UI thread");
                heading.FontSize(24);
                heading.TextWrapping(TextWrapping::Wrap);
                root.Children().Append(heading);
                auto description = TextBlock{};
                description.Text(L"This chart owns its data and updates independently every 120 ms. Close this window or use the primary window's toggle to stop it.");
                description.TextWrapping(TextWrapping::Wrap);
                root.Children().Append(description);

                auto values = single_threaded_observable_vector<double>({ 14, 28, 19, 43, 31 });
                auto samples = Samples{};
                samples.ItemsSource(values);
                auto series = LineSeries{};
                series.Title(L"Secondary UI thread");
                series.YValues(samples);
                series.StrokeThickness(3);
                operation = L"Create the secondary chart";
                auto chart = Chart{};
                chart.Height(280);
                chart.ShowLegend(true);
                chart.Data().Append(samples);
                chart.Series().Append(series);
                AutomationProperties::SetName(chart, L"Five independently updating values on a secondary UI thread");
                root.Children().Append(chart);
                auto data = TextBlock{};
                data.IsTextSelectionEnabled(true);
                data.TextWrapping(TextWrapping::Wrap);
                data.Text(DataRows(values));
                root.Children().Append(data);
                auto scroller = ScrollViewer{};
                scroller.Content(root);
                operation = L"Create the secondary window";
                window = Window{};
                window.Title(L"Charts secondary UI thread");
                window.Content(scroller);
                timer = DispatcherTimer{};
                timer.Interval(std::chrono::milliseconds{ 120 });
                closed = window.Closed([&windowClosed, dispatcher, timer](auto&&, auto&&)
                {
                    windowClosed = true;
                    timer.Stop();
                    dispatcher.EnqueueEventLoopExit();
                });
                tick = timer.Tick([state, values, data, window, index = uint32_t{}](auto&&, auto&&) mutable
                {
                    if (state->stop.load())
                    {
                        window.Close();
                        return;
                    }
                    auto current = index++ % values.Size();
                    auto value = values.GetAt(current);
                    values.SetAt(current, value >= 48 ? value - 31 : value + 9);
                    data.Text(DataRows(values));
                });
                timer.Start();
                operation = L"Activate the secondary window";
                window.Activate();
                operation = L"Run the secondary window";
                dispatcher.RunEventLoop();
            }
            catch (hresult_error const& error)
            {
                rememberError(error);
            }
            catch (std::exception const&)
            {
                state->error = L"The secondary chart window could not be opened. Close it and try again.";
            }

            // Revoke callbacks and release UI objects before shutting down the queue.
            auto cleanup = [&](auto&& action)
            {
                try { action(); }
                catch (hresult_error const& error) { rememberError(error); }
            };
            cleanup([&]
            {
                if (timer)
                {
                    timer.Stop();
                    timer.Tick(tick);
                }
            });
            cleanup([&]
            {
                if (window)
                {
                    window.Closed(closed);
                    if (!windowClosed)
                    {
                        window.Content(nullptr);
                        window.Close();
                    }
                }
            });
            timer = nullptr;
            window = nullptr;
            cleanup([&] { if (manager) manager.Close(); });
            manager = nullptr;
            cleanup([&] { if (controller) controller.ShutdownQueue(); });
            controller = nullptr;
            if (apartmentInitialized) uninit_apartment();
            state->done.store(true);
        }
    }

    MainWindow::MainWindow(hstring const& variant)
    {
        InitializeComponent();
        VariantText().Text(variant);
        Month().ItemsSource(single_threaded_observable_vector<hstring>({ L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun" }));
        m_markupProfitValues = single_threaded_observable_vector<double>({ 18, 27, 22, 41, 36, 52 });
        m_markupExpenseValues = single_threaded_observable_vector<double>({ 31, 25, 29, 24, 32, 28 });
        Profit().ItemsSource(m_markupProfitValues);
        Expenses().ItemsSource(m_markupExpenseValues);
        AreaMonth().ItemsSource(single_threaded_observable_vector<hstring>({ L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun" }));
        AreaValues().ItemsSource(single_threaded_observable_vector<double>({ 8, 18, 14, 29, 24, 37 }));
        BarMonth().ItemsSource(single_threaded_observable_vector<hstring>({ L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun" }));
        BarValues().ItemsSource(single_threaded_observable_vector<double>({ 12, 20, 17, 31, 26, 39 }));

        m_xAxis = CategoryAxis{};
        m_xAxis.Label(L"Month");
        m_yAxis = LinearAxis{};
        m_yAxis.Label(L"Profit");
        MarkupChart().Axes().Append(m_xAxis);
        MarkupChart().Axes().Append(m_yAxis);
        ProfitSeries().XAxis(m_xAxis);
        ProfitSeries().YAxis(m_yAxis);
        m_barXAxis = CategoryAxis{};
        m_barXAxis.Label(L"Month");
        m_barYAxis = LinearAxis{};
        m_barYAxis.Label(L"Value");
        BarMarkupChart().Axes().Append(m_barXAxis);
        BarMarkupChart().Axes().Append(m_barYAxis);
        MarkupBarSeries().XAxis(m_barXAxis);
        MarkupBarSeries().YAxis(m_barYAxis);
        CreateCodeChart();
        CreateDateTimeCharts();
        RebuildSeriesSelector(MarkupChart(), PresentationSeriesComboBox(), 0);
        RebuildSeriesSelector(AreaMarkupChart(), AreaSeriesChoice(), 0);
        RebuildSeriesSelector(BarMarkupChart(), BarSeriesChoice(), 0);
        m_ready = true;
        SyncPresentationKnobs();
        SyncAxisControls();
        SyncAreaOptions();
        SyncBarOptions();
        UpdateDataText();
        ScenarioNavigation().SelectedItem(NavLine());
        AppWindow().Resize(Windows::Graphics::SizeInt32{ 1280, 900 });
        ScenarioNavigation().IsPaneOpen(true);

        m_codeChartTimer = DispatcherTimer{};
        m_codeChartTimer.Interval(std::chrono::seconds{ 1 });
        m_codeTick = m_codeChartTimer.Tick([weak = get_weak()](auto&&, auto&&)
        {
            if (auto self = weak.get(); self && !self->m_closing) self->UpdateCodeChartData();
        });
        m_codeChartTimer.Start();
        m_secondaryPoll = DispatcherTimer{};
        m_secondaryPoll.Interval(std::chrono::milliseconds{ 100 });
        m_pollTick = m_secondaryPoll.Tick([weak = get_weak()](auto&&, auto&&)
        {
            if (auto self = weak.get()) self->PollSecondaryWindow();
        });
        AppWindow().Closing([weak = get_weak()](auto&&, Microsoft::UI::Windowing::AppWindowClosingEventArgs const& args)
        {
            if (auto self = weak.get())
            {
                self->m_closing = true;
                self->StopPrimaryTimer();
                if (self->m_secondaryThread.joinable())
                {
                    // Keep the primary dispatcher pumping until the other STA has shut down.
                    args.Cancel(true);
                    self->m_secondaryState->stop.store(true);
                    self->SecondaryButton().IsEnabled(false);
                    self->m_secondaryPoll.Start();
                }
            }
        });
        Closed([weak = get_weak()](auto&&, auto&&)
        {
            if (auto self = weak.get())
            {
                self->StopPrimaryTimer();
                self->m_secondaryPoll.Stop();
                self->m_secondaryPoll.Tick(self->m_pollTick);
            }
        });
    }

    void MainWindow::StopPrimaryTimer()
    {
        if (m_codeChartTimer)
        {
            m_codeChartTimer.Stop();
            m_codeChartTimer.Tick(m_codeTick);
            m_codeChartTimer = nullptr;
        }
    }

    void MainWindow::CreateCodeChart()
    {
        auto yValues = Samples{};
        auto line = LineSeries{};
        line.YValues(yValues);
        auto chart = Chart{};
        chart.FontSize(14);
        chart.Series().Append(line);
        CodeChartHost().Child(chart);

        // Dimensional handles can be connected before their source collections exist.
        auto xValues = Samples{};
        line.XValues(xValues);
        chart.Data().Append(xValues);
        chart.Data().Append(yValues);
        xValues.ItemsSource(single_threaded_observable_vector<hstring>({ L"Alpha", L"Beta", L"Gamma", L"Delta", L"Epsilon" }));
        m_codeChartValues = single_threaded_observable_vector<double>({ 12, 38, 21, 47, 34 });
        yValues.ItemsSource(m_codeChartValues);
        line.Title(L"Initialized out of order");
        line.StrokeDashStyle(StrokeDashStyle::DashDot);
        line.StrokeThickness(3);
        line.Stroke(SolidColorBrush{ Windows::UI::Color{ 255, 0, 120, 212 } });
        line.DataLabelOverrides().Insert(1, DataLabelOverride{ L"Beta", SolidColorBrush{ LabelGreen } });
        line.DataMarkerOverrides().Insert(3, DataMarkerOverride{ MarkerShape::Diamond, SolidColorBrush{ MarkerPurple } });
        chart.ShowLegend(true);
        AutomationProperties::SetName(chart, L"Code-created line chart: Alpha, Beta, Gamma, Delta, Epsilon");
        AutomationProperties::SetHelpText(chart, L"Exact values are listed under Current data.");
    }

    void MainWindow::CreateDateTimeCharts()
    {
        constexpr double revenues[]{
            42, 38, 47, 53, 61, 58, 72, 68, 55, 49, 63, 70,
            45, 41, 52, 60, 67, 64, 79, 74, 61, 54, 68, 76,
            48, 44, 55, 63, 71, 68, 83, 78, 65, 58, 72, 80
        };
        for (bool monthly : { false, true })
        {
            std::vector<DateTime> dates;
            std::vector<double> actual, targets;
            std::wostringstream table;
            table << L"Date          Actual   Target" << std::fixed << std::setprecision(2);
            auto formatter = Windows::Globalization::DateTimeFormatting::DateTimeFormatter(L"shortdate");
            for (int i = 0; i < (monthly ? 36 : 75); ++i)
            {
                DateTime date;
                double value;
                double target;
                if (monthly)
                {
                    date = CalendarDate(static_cast<WORD>(2022 + i / 12), static_cast<WORD>(i % 12 + 1), 1);
                    value = revenues[i];
                    target = 60.0 + (i / 12) * 5.0;
                }
                else
                {
                    date = CalendarDate(2024, 1, 1) + std::chrono::hours{ 24 * i };
                    double t = i / 74.0;
                    value = 50 + 20 * std::sin(t * 6.283185) + 4 * std::sin(t * 25.13274);
                    target = 50;
                }
                dates.push_back(date);
                actual.push_back(value);
                targets.push_back(target);
                table << L'\n' << formatter.Format(date).c_str() << L"   " << value << L"   " << target;
            }
            auto x = Samples{};
            auto y = Samples{};
            auto target = Samples{};
            x.ItemsSource(single_threaded_observable_vector<DateTime>(std::move(dates)));
            y.ItemsSource(single_threaded_observable_vector<double>(std::move(actual)));
            target.ItemsSource(single_threaded_observable_vector<double>(std::move(targets)));
            auto xAxis = DateTimeAxis{};
            auto yAxis = LinearAxis{};
            auto line = LineSeries{};
            line.Title(monthly ? L"Monthly value" : L"Daily value");
            line.StrokeThickness(3);
            line.XValues(x);
            line.YValues(y);
            line.XAxis(xAxis);
            line.YAxis(yAxis);
            auto targetLine = LineSeries{};
            targetLine.Title(L"Target");
            targetLine.StrokeDashStyle(StrokeDashStyle::Dash);
            targetLine.XValues(x);
            targetLine.YValues(target);
            targetLine.XAxis(xAxis);
            targetLine.YAxis(yAxis);
            auto chart = Chart{};
            chart.FontSize(14);
            chart.ShowLegend(true);
            chart.Data().Append(x);
            chart.Data().Append(y);
            chart.Data().Append(target);
            chart.Axes().Append(xAxis);
            chart.Axes().Append(yAxis);
            chart.Series().Append(line);
            chart.Series().Append(targetLine);
            AutomationProperties::SetName(chart, monthly
                ? L"36 monthly actual and target values, 2022 through 2024"
                : L"75 daily actual and target values, starting January 1, 2024");
            AutomationProperties::SetHelpText(chart, L"Open the data section below to read each value.");
            if (monthly)
            {
                m_dtAxisB = xAxis;
                DtChartHostB().Child(chart);
                DtDataTextB().Text(table.str());
            }
            else
            {
                m_dtAxisA = xAxis;
                DtChartHostA().Child(chart);
                DtDataTextA().Text(table.str());
            }
        }
    }

    void MainWindow::UpdateCodeChartData()
    {
        auto update = [&](IObservableVector<double> const& values, uint32_t bias)
        {
            auto index = (m_codeChartUpdateIndex + bias) % values.Size();
            auto value = values.GetAt(index);
            values.SetAt(index, value >= 48 ? value - 29 : value + 7 + bias);
        };
        update(m_codeChartValues, 0);
        update(m_markupProfitValues, 1);
        update(m_markupExpenseValues, 2);
        ++m_codeChartUpdateIndex;
        UpdateDataText();
    }

    void MainWindow::UpdateDataText()
    {
        std::wostringstream text;
        text << L"Current line series (" << MarkupChart().Series().Size() << L")";
        for (auto const& series : MarkupChart().Series())
        {
            text << L"\n\n" << SeriesDataText(series).c_str();
        }
        text << L"\n\nCode line (Alpha, Beta, Gamma, Delta, Epsilon):\n" << DataRows(m_codeChartValues).c_str();
        DataText().Text(text.str());
    }

    hstring MainWindow::SeriesDataText(CartesianSeries const& series)
    {
        // Named sources are available before the markup's one-time bindings connect.
        auto samples = series.YValues();
        if (series == ProfitSeries()) samples = Profit();
        else if (series == ExpensesSeries()) samples = Expenses();
        else if (series == MarkupAreaSeries()) samples = AreaValues();
        else if (series == MarkupBarSeries()) samples = BarValues();
        if (!samples) throw hresult_illegal_method_call(L"The series needs a values source.");
        return series.Title() + (series.IsVisible() ? L" (visible)" : L" (hidden)") +
            L"\nJan, Feb, Mar, Apr, May, Jun:\n" +
            DataRows(samples.ItemsSource().as<IVector<double>>());
    }

    void MainWindow::ReportError(hresult_error const& error, TextBlock const& status)
    {
        auto text = ErrorText(error);
        status.Text(text);
        AnnounceStatus(text);
    }

    void MainWindow::AnnounceStatus(hstring const& message)
    {
        if (message.empty()) return;
        // A live-region update is dropped when focus moves at the same time (leaving an edited field,
        // or a button disabled by the edit), so raise a notification after focus has settled.
        DispatcherQueue().TryEnqueue(DispatcherQueuePriority::Low, [weak = get_weak(), message]
        {
            auto self = weak.get();
            if (!self) return;
            if (auto peer = Automation::Peers::FrameworkElementAutomationPeer::CreatePeerForElement(self->StatusText()))
            {
                peer.RaiseNotificationEvent(
                    Automation::Peers::AutomationNotificationKind::Other,
                    Automation::Peers::AutomationNotificationProcessing::MostRecent,
                    message,
                    L"ChartsSampleStatus");
            }
        });
    }

    void MainWindow::ApplyEdit(std::function<void()> const& edit, TextBlock const& status, hstring const& message,
        Chart const& chart, std::function<void()> const& restore, hstring const& announcement)
    {
        if (!m_ready || m_syncing || m_closing) return;
        try
        {
            edit();
            // Brush changes alone can leave the rendered plot at its previous appearance.
            if (chart) chart.InvalidateArrange();
            status.Text(message);
            AnnounceStatus(announcement.empty() ? message : announcement);
        }
        catch (hresult_error const& error)
        {
            if (restore)
            {
                // Restore only the editor that failed so unrelated drafts are left alone.
                SyncGuard guard{ m_syncing };
                restore();
            }
            else
            {
                // Leave the date-time label drafts intact; only resync the shared axis controls.
                SyncAxisControls(false);
                SyncPresentationKnobs();
            }
            ReportError(error, status);
        }
    }

    void MainWindow::ApplyAxisEdit(std::function<void()> const& edit)
    {
        if (!m_ready || m_syncing || m_closing || !HasProfitSeries()) return;
        ApplyEdit(edit, PresentationKnobStatusText(), L"Profit axes updated.", MarkupChart());
    }

    void MainWindow::ApplyExampleEdit(bool area, std::function<void()> const& edit, hstring const& message)
    {
        if (!m_ready || m_syncing || m_closing) return;
        auto status = area ? AreaStatusText() : BarStatusText();
        auto synchronize = [&] { if (area) SyncAreaOptions(); else SyncBarOptions(); };
        try
        {
            {
                SyncGuard guard{ m_syncing };
                edit();
                // Brush changes alone can leave the rendered plot at its previous appearance.
                (area ? AreaMarkupChart() : BarMarkupChart()).InvalidateArrange();
            }
            synchronize();
            status.Text(message);
            AnnounceStatus(message);
        }
        catch (hresult_error const& error)
        {
            synchronize();
            auto const text = error.code() == E_INVALIDARG
                ? hstring{ area ? L"The area option is not valid. Choose an available option." : L"The bar option is not valid. Choose an available option." }
                : hstring{ area ? L"Area options: " : L"Bar options: " } + ErrorText(error);
            status.Text(text);
            AnnounceStatus(text);
        }
    }

    void MainWindow::ApplyLineEdit(std::function<void()> const& edit, hstring const& message)
    {
        if (!m_ready || m_syncing || m_closing) return;
        try
        {
            {
                SyncGuard guard{ m_syncing };
                edit();
                MarkupChart().InvalidateArrange();
            }
            SyncPresentationKnobs();
            SyncAxisAvailability();
            UpdateDataText();
            PresentationKnobStatusText().Text(message);
            AnnounceStatus(message);
        }
        catch (hresult_error const& error)
        {
            SyncPresentationKnobs();
            SyncAxisAvailability();
            auto const text = ErrorText(error);
            PresentationKnobStatusText().Text(text);
            AnnounceStatus(text);
        }
    }

    void MainWindow::RebuildSeriesSelector(Chart const& chart, ComboBox const& selector, int32_t selectedIndex)
    {
        SyncGuard guard{ m_syncing };
        selector.Items().Clear();
        for (auto const& series : chart.Series()) selector.Items().Append(box_value(series.Title()));
        selector.SelectedIndex((std::max)(0, (std::min)(selectedIndex, static_cast<int32_t>(chart.Series().Size()) - 1)));
    }

    void MainWindow::AddExampleSeries(int32_t example)
    {
        auto chart = example == 0 ? MarkupChart() : example == 1 ? AreaMarkupChart() : BarMarkupChart();
        auto selector = example == 0 ? PresentationSeriesComboBox() : example == 1 ? AreaSeriesChoice() : BarSeriesChoice();
        auto& next = example == 0 ? m_nextLineSeries : example == 1 ? m_nextAreaSeries : m_nextBarSeries;
        auto number = next++;
        auto const& seed = example == 0 ? ProfitSeed : example == 1 ? AreaSeed : BarSeed;
        std::vector<double> values;
        for (auto value : seed) values.push_back(value * (0.65 + 0.05 * (number % 4)) + number * 3);
        auto samples = Samples{};
        samples.ItemsSource(single_threaded_observable_vector<double>(std::move(values)));
        auto color = SeriesColors[(number - 1) % 6];
        CartesianSeries series{ nullptr };
        if (example == 0)
        {
            series = LineSeries{};
            ResetSeriesDefaults(series, 3, true, false);
        }
        else if (example == 1)
        {
            auto area = AreaSeries{};
            ResetSeriesDefaults(area, 2, false, false);
            auto fill = color;
            fill.A = AreaFillAlphas[0];
            area.Fill(SolidColorBrush{ fill });
            series = area;
        }
        else
        {
            auto bar = BarSeries{};
            ResetSeriesDefaults(bar, 1.5, false, false);
            bar.Fill(SolidColorBrush{ color });
            bar.Orientation(SelectedBarSeries().Orientation());
            auto xAxis = CategoryAxis{};
            xAxis.Label(L"Month");
            auto yAxis = LinearAxis{};
            yAxis.Label(L"Value");
            chart.Axes().Append(xAxis);
            chart.Axes().Append(yAxis);
            bar.XAxis(xAxis);
            bar.YAxis(yAxis);
            series = bar;
        }
        series.Title(L"Series " + to_hstring(number));
        series.XValues(example == 0 ? Month() : example == 1 ? AreaMonth() : BarMonth());
        series.YValues(samples);
        series.Stroke(SolidColorBrush{ color });
        chart.Data().Append(samples);
        chart.Series().Append(series);
        chart.ShowLegend(true);
        RebuildSeriesSelector(chart, selector, static_cast<int32_t>(chart.Series().Size()) - 1);
    }

    void MainWindow::RemoveSelectedSeries(Chart const& chart, ComboBox const& selector)
    {
        if (chart.Series().Size() <= 1) return;
        auto index = Selection(selector, static_cast<int32_t>(chart.Series().Size()));
        auto series = chart.Series().GetAt(index);
        chart.Series().RemoveAt(index);
        RemoveItem(chart.Data(), series.YValues());
        for (auto const& axis : { series.XAxis(), series.YAxis() })
        {
            if (!axis) continue;
            bool used = false;
            for (auto const& remaining : chart.Series())
                if (remaining.XAxis() == axis || remaining.YAxis() == axis) used = true;
            if (!used) RemoveItem(chart.Axes(), axis);
        }
        RebuildSeriesSelector(chart, selector, index);
    }

    bool MainWindow::HasProfitSeries()
    {
        uint32_t index{};
        return MarkupChart().Series().IndexOf(ProfitSeries(), index);
    }

    void MainWindow::SyncAxisAvailability()
    {
        bool present = HasProfitSeries();
        AxisControls().IsEnabled(present);
        AxisSeriesWarning().Visibility(!present && AxesEditorPanel().Visibility() == Visibility::Visible
            ? Visibility::Visible : Visibility::Collapsed);
    }

    void MainWindow::SyncAreaOptions()
    {
        SyncGuard guard{ m_syncing };
        auto series = SelectedAreaSeries();
        AreaColorChoice().SelectedIndex(SeriesColorIndex(series.Fill(), series.Stroke(), true));
        AreaFillChoice().SelectedIndex(AreaFillIndex(series.Fill()));
        AreaMarkerChoice().SelectedIndex(static_cast<int32_t>(series.MarkerShape()));
        AreaVisibleCheckBox().IsChecked(series.IsVisible());
        AreaValuesCheckBox().IsChecked(series.ShowDataLabels());
        AreaMarkersCheckBox().IsChecked(series.ShowDataMarkers());
        AreaLegendCheckBox().IsChecked(AreaMarkupChart().ShowLegend());
        AreaRemoveSeriesButton().IsEnabled(AreaMarkupChart().Series().Size() > 1);
        AreaDataText().Text(SeriesDataText(series));
    }

    void MainWindow::SyncBarOptions()
    {
        SyncGuard guard{ m_syncing };
        auto series = SelectedBarSeries();
        BarColorChoice().SelectedIndex(SeriesColorIndex(series.Fill(), series.Stroke(), false));
        BarMarkerChoice().SelectedIndex(static_cast<int32_t>(series.MarkerShape()));
        BarVisibleCheckBox().IsChecked(series.IsVisible());
        BarValuesCheckBox().IsChecked(series.ShowDataLabels());
        BarMarkersCheckBox().IsChecked(series.ShowDataMarkers());
        BarLegendCheckBox().IsChecked(BarMarkupChart().ShowLegend());
        BarOrientationChoice().SelectedIndex(series.Orientation() == BarOrientation::Horizontal ? 0 : 1);
        BarOrientationText().Text(series.Orientation() == BarOrientation::Horizontal
            ? L"Orientation: Horizontal" : L"Orientation: Vertical");
        BarRemoveSeriesButton().IsEnabled(BarMarkupChart().Series().Size() > 1);
        BarDataText().Text(SeriesDataText(series));
    }

    void MainWindow::SetAreaAppearance(int32_t colorIndex, int32_t fillIndex)
    {
        auto series = SelectedAreaSeries();
        auto fill = colorIndex == 0 ? AreaOriginalFill : SeriesColors[colorIndex - 1];
        auto stroke = colorIndex == 0 ? AreaOriginalStroke : SeriesColors[colorIndex - 1];
        fill.A = AreaFillAlphas[fillIndex];
        // A transparent brush preserves outline-only mode; null would restore a palette fill.
        series.Fill(SolidColorBrush{ fill });
        series.Stroke(SolidColorBrush{ stroke });
    }

    void MainWindow::SetBarColor(int32_t colorIndex)
    {
        auto series = SelectedBarSeries();
        series.Fill(SolidColorBrush{ colorIndex == 0 ? BarOriginalFill : SeriesColors[colorIndex - 1] });
        series.Stroke(SolidColorBrush{ colorIndex == 0 ? BarOriginalStroke : SeriesColors[colorIndex - 1] });
    }

    LineSeries MainWindow::SelectedPresentationSeries()
    {
        return MarkupChart().Series().GetAt(Selection(PresentationSeriesComboBox(),
            static_cast<int32_t>(MarkupChart().Series().Size()))).as<LineSeries>();
    }

    AreaSeries MainWindow::SelectedAreaSeries()
    {
        return AreaMarkupChart().Series().GetAt(Selection(AreaSeriesChoice(),
            static_cast<int32_t>(AreaMarkupChart().Series().Size()))).as<AreaSeries>();
    }

    BarSeries MainWindow::SelectedBarSeries()
    {
        return BarMarkupChart().Series().GetAt(Selection(BarSeriesChoice(),
            static_cast<int32_t>(BarMarkupChart().Series().Size()))).as<BarSeries>();
    }

    uint32_t MainWindow::SelectedOverrideIndex()
    {
        IReference<double> parsed{ nullptr };
        try
        {
            parsed = AutoNumber(OverrideIndexNumberBox());
        }
        catch (hresult_invalid_argument const&)
        {
            throw hresult_invalid_argument(L"Point index must be an integer from 0 through 5.");
        }
        auto value = parsed ? parsed.Value() : std::numeric_limits<double>::quiet_NaN();
        if (!std::isfinite(value) || value < 0 || value > 5 || std::trunc(value) != value)
        {
            throw hresult_invalid_argument(L"Point index must be an integer from 0 through 5.");
        }
        m_lastValidOverrideIndex = value;
        return static_cast<uint32_t>(value);
    }

    void MainWindow::SyncPresentationKnobs()
    {
        SyncGuard guard{ m_syncing };
        auto series = SelectedPresentationSeries();
        auto weight = std::find(LineWeights.begin(), LineWeights.end(), series.StrokeThickness());
        LineWeightChoice().SelectedIndex(weight == LineWeights.end() ? -1 : static_cast<int32_t>(weight - LineWeights.begin()));
        LineStyleChoice().SelectedIndex(static_cast<int32_t>(series.StrokeDashStyle()));
        LineColorChoice().SelectedIndex(LineColorIndex(series.Stroke()));
        LineMarkerChoice().SelectedIndex(static_cast<int32_t>(series.MarkerShape()));
        LineVisibleCheckBox().IsChecked(series.IsVisible());
        LineRemoveSeriesButton().IsEnabled(MarkupChart().Series().Size() > 1);
        LegendVisibilityCheckBox().IsChecked(MarkupChart().ShowLegend());
        ShowDataLabelsCheckBox().IsChecked(series.ShowDataLabels());
        ShowDataMarkersCheckBox().IsChecked(series.ShowDataMarkers());
        DataLabelBrushCheckBox().IsChecked(series.DataLabelBrush() != nullptr);
        DataMarkerBrushCheckBox().IsChecked(series.DataMarkerBrush() != nullptr);
        uint32_t index{};
        try
        {
            auto value = OverrideIndexNumberBox().Value();
            if (!std::isfinite(value) || value < 0 || value > 5 || std::trunc(value) != value)
            {
                throw hresult_invalid_argument(L"Point index must be an integer from 0 through 5.");
            }
            index = static_cast<uint32_t>(value);
        }
        catch (hresult_error const& error)
        {
            SyncNumber(OverrideIndexNumberBox(), IReference<double>{ m_lastValidOverrideIndex });
            ReportError(error, PresentationKnobStatusText());
        }
        auto labels = series.DataLabelOverrides();
        auto label = labels.HasKey(index) ? labels.Lookup(index) : nullptr;
        LabelOverrideTextBox().Text(label ? label.Text() : L"");
        LabelOverrideBrushCheckBox().IsChecked(label && label.Brush() != nullptr);
        auto markers = series.DataMarkerOverrides();
        auto marker = markers.HasKey(index) ? markers.Lookup(index) : nullptr;
        MarkerShapeComboBox().SelectedIndex(marker ? static_cast<int32_t>(marker.Shape()) : 8);
        MarkerOverrideBrushCheckBox().IsChecked(marker && marker.Brush() != nullptr);
    }

    void MainWindow::SyncAxisControls(bool includeDateTime)
    {
        SyncGuard guard{ m_syncing };
        SyncAxisAvailability();
        SyncNumber(LinearMinBox(), m_yAxis.Minimum());
        SyncNumber(LinearMaxBox(), m_yAxis.Maximum());
        SyncNumber(LinearSpacingBox(), m_yAxis.Spacing());
        SortKeyBox().SelectedIndex(static_cast<int32_t>(m_xAxis.SortKey()));
        SortOrderBox().SelectedIndex(static_cast<int32_t>(m_xAxis.SortOrder()));
        AxisVisibleCheck().IsChecked(m_yAxis.IsVisible());
        TickLabelsCheck().IsChecked(m_yAxis.ShowTickLabels());
        TickMarksCheck().IsChecked(m_yAxis.ShowTickMarks());
        GridLinesBox().SelectedIndex(static_cast<int32_t>(m_yAxis.GridLines()));
        GridLineBrushBox().SelectedIndex(BrushIndex(m_yAxis.GridLineMajorBrush()));
        TickBrushBox().SelectedIndex(BrushIndex(m_yAxis.TickBrush()));
        TickLabelBrushBox().SelectedIndex(BrushIndex(m_yAxis.TickLabelBrush()));
        AxisLineBrushBox().SelectedIndex(BrushIndex(m_yAxis.AxisLineBrush()));
        if (includeDateTime)
        {
            DtIntervalTypeBoxA().SelectedIndex(static_cast<int32_t>(m_dtAxisA.IntervalType()));
            DtIntervalTypeBoxB().SelectedIndex(static_cast<int32_t>(m_dtAxisB.IntervalType()));
            DtLabelFormatBoxA().Text(m_dtAxisA.LabelFormat());
            DtLabelFormatBoxB().Text(m_dtAxisB.LabelFormat());
        }
    }

    void MainWindow::OnLayoutSizeChanged(IInspectable const&, SizeChangedEventArgs const& args)
    {
        if (!m_ready || m_closing) return;
        auto width = args.NewSize().Width;
        bool compact = width < 1000;
        Grid::SetRow(ThemeChoice(), compact ? 2 : 0);
        Grid::SetColumn(ThemeChoice(), compact ? 0 : 1);
        ThemeChoice().HorizontalAlignment(compact ? HorizontalAlignment::Left : HorizontalAlignment::Right);
        ScenarioHeader().ColumnSpacing(compact ? 0 : 16);
        ScenarioHeader().Margin(Thickness{ 16, width < 840 ? 48.0 : 16.0, 16, 12 });
    }

    void MainWindow::UpdateWorkspaceLayout(Grid const& workspace)
    {
        if (!m_ready || m_closing) return;
        bool wide = workspace.ActualWidth() >= 900;
        double viewportHeight = ScenarioScroller().ActualHeight();
        auto preview = workspace.Children().GetAt(0).as<Border>();
        auto editor = workspace.Children().GetAt(1).as<ScrollViewer>();
        workspace.ColumnDefinitions().GetAt(1).Width(GridLength{ wide ? 440.0 : 0.0, GridUnitType::Pixel });
        workspace.ColumnSpacing(wide ? 16 : 0);
        workspace.RowSpacing(wide ? 0 : 12);
        Grid::SetColumn(editor, wide ? 1 : 0);
        Grid::SetRow(editor, wide ? 0 : 1);
        preview.Child().as<FrameworkElement>().Height(wide
            ? (std::min)(420.0, (std::max)(240.0, viewportHeight - 50.0))
            : (std::min)(240.0, (std::max)(180.0, viewportHeight * 0.35)));
        editor.MaxHeight(wide ? (std::max)(1.0, viewportHeight - 16.0) : std::numeric_limits<double>::infinity());
        editor.VerticalScrollMode(wide ? ScrollMode::Auto : ScrollMode::Disabled);
        editor.VerticalScrollBarVisibility(wide ? ScrollBarVisibility::Auto : ScrollBarVisibility::Disabled);
    }

    void MainWindow::OnWorkspaceSizeChanged(IInspectable const& sender, SizeChangedEventArgs const&)
    {
        UpdateWorkspaceLayout(sender.as<Grid>());
    }

    void MainWindow::OnViewportSizeChanged(IInspectable const&, SizeChangedEventArgs const&)
    {
        if (!m_ready || m_closing) return;
        for (auto const& workspace : { LineScenario(), AreaScenario(), BarScenario(), LiveScenario() })
            UpdateWorkspaceLayout(workspace);
    }

    void MainWindow::OnEditorSizeChanged(IInspectable const& sender, SizeChangedEventArgs const& args)
    {
        if (!m_ready || m_closing) return;
        auto grid = sender.as<Grid>();
        auto tag = unbox_value_or<hstring>(grid.Tag(), hstring{});
        uint32_t columns = args.NewSize().Width >= (tag == L"Choices" ? 300 : 360)
            ? (tag == L"Bounds" ? 3u : 2u) : 1u;
        uint32_t rows = (grid.Children().Size() + columns - 1) / columns;
        if (grid.ColumnDefinitions().Size() != columns)
        {
            grid.ColumnDefinitions().Clear();
            for (uint32_t i = 0; i < columns; ++i)
            {
                ColumnDefinition column;
                column.Width(GridLength{ 1, GridUnitType::Star });
                grid.ColumnDefinitions().Append(column);
            }
        }
        if (grid.RowDefinitions().Size() != rows)
        {
            grid.RowDefinitions().Clear();
            for (uint32_t i = 0; i < rows; ++i)
            {
                RowDefinition row;
                row.Height(GridLength{ 1, GridUnitType::Auto });
                grid.RowDefinitions().Append(row);
            }
        }
        for (uint32_t i = 0; i < grid.Children().Size(); ++i)
        {
            auto child = grid.Children().GetAt(i).as<FrameworkElement>();
            Grid::SetColumn(child, static_cast<int32_t>(i % columns));
            Grid::SetRow(child, static_cast<int32_t>(i / columns));
        }
    }

    void MainWindow::OnScenarioSelectionChanged(NavigationView const&, NavigationViewSelectionChangedEventArgs const& args)
    {
        if (!m_ready || m_closing) return;
        auto item = args.SelectedItem().try_as<NavigationViewItem>();
        if (!item) return;
        auto tag = unbox_value<hstring>(item.Tag());
        auto visible = [](bool selected) { return selected ? Visibility::Visible : Visibility::Collapsed; };
        LineScenario().Visibility(visible(tag == L"line" || tag == L"axes" || tag == L"presentation"));
        SeriesEditorPanel().Visibility(visible(tag == L"line" || tag == L"presentation"));
        LineSeriesActions().Visibility(visible(tag == L"line"));
        LineStyleEditors().Visibility(visible(tag == L"line"));
        LineVisibleCheckBox().Visibility(visible(tag == L"line"));
        LineResetButton().Visibility(visible(tag == L"line"));
        AxesEditorPanel().Visibility(visible(tag == L"axes"));
        SyncAxisAvailability();
        PresentationEditorPanel().Visibility(visible(tag == L"presentation"));
        AreaScenario().Visibility(visible(tag == L"area"));
        BarScenario().Visibility(visible(tag == L"bar"));
        DateTimeScenario().Visibility(visible(tag == L"datetime"));
        LiveScenario().Visibility(visible(tag == L"live"));

        hstring heading{ L"Line charts" };
        hstring description{ L"Compare monthly profit and expenses. Explore the legend and series defaults." };
        if (tag == L"area")
        {
            heading = L"Area charts";
            description = L"Explore a filled monthly trend created in markup.";
        }
        else if (tag == L"bar")
        {
            heading = L"Bar charts";
            description = L"Compare monthly values and switch the connected series orientation.";
        }
        else if (tag == L"datetime")
        {
            heading = L"Date & time";
            description = L"Explore daily and monthly timelines with date intervals and label formatting.";
        }
        else if (tag == L"axes")
        {
            heading = L"Axes & ordering";
            description = L"Adjust bounds, category order, ticks and grid lines on the same monthly line chart.";
        }
        else if (tag == L"presentation")
        {
            heading = L"Labels & markers";
            description = L"Make points stand out with series defaults, brushes and indexed overrides.";
        }
        else if (tag == L"live")
        {
            heading = L"Live data";
            description = L"Watch observable data update and open an independent secondary UI thread.";
        }
        ScenarioHeading().Text(heading);
        ScenarioDescription().Text(description);
        ScenarioScroller().ChangeView(nullptr, 0.0, nullptr, true);
        if (ScenarioNavigation().DisplayMode() != NavigationViewDisplayMode::Expanded)
            ScenarioNavigation().IsPaneOpen(false);
    }

    void MainWindow::OnDateLayoutSizeChanged(IInspectable const&, SizeChangedEventArgs const& args)
    {
        if (!m_ready || m_closing) return;
        bool wide = args.NewSize().Width >= 900;
        DateTimeScenario().ColumnSpacing(wide ? 16 : 0);
        DateTimeScenario().ColumnDefinitions().GetAt(1).Width(GridLength{ wide ? 1.0 : 0.0, GridUnitType::Star });
        Grid::SetColumn(MonthlyDateCard(), wide ? 1 : 0);
        Grid::SetRow(MonthlyDateCard(), wide ? 0 : 1);
        Grid::SetColumnSpan(DateFormatHelp(), wide ? 2 : 1);
        Grid::SetRow(DateFormatHelp(), wide ? 1 : 2);
    }

    void MainWindow::OnThemeChoiceChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        if (!m_ready || m_closing) return;
        auto index = ThemeChoice().SelectedIndex();
        RootGrid().RequestedTheme(index == 1 ? ElementTheme::Light : index == 2 ? ElementTheme::Dark : ElementTheme::Default);
    }

    void MainWindow::OnToggleUpdatesClick(IInspectable const&, RoutedEventArgs const&)
    {
        if (!m_codeChartTimer) return;
        if (m_codeChartTimer.IsEnabled()) m_codeChartTimer.Stop();
        else m_codeChartTimer.Start();
        UpdatesButton().Content(box_value(m_codeChartTimer.IsEnabled() ? L"Pause updates" : L"Resume updates"));
    }

    void MainWindow::OnToggleSecondaryClick(IInspectable const&, RoutedEventArgs const&)
    {
        if (m_closing) return;
        if (m_secondaryThread.joinable())
        {
            m_secondaryState->stop.store(true);
            SecondaryButton().IsEnabled(false);
            StatusText().Text(L"Closing the secondary UI thread...");
            AnnounceStatus(L"Closing the secondary UI thread...");
            return;
        }
        m_secondaryState = std::make_shared<SecondaryChartState>();
        m_secondaryLifetime = get_strong();
        try
        {
            m_secondaryThread = std::thread{ RunSecondaryWindow, m_secondaryState };
            m_secondaryPoll.Start();
            StatusText().Text(L"Opening an independently updating chart on a secondary UI thread.");
            AnnounceStatus(L"Opening an independently updating chart on a secondary UI thread.");
        }
        catch (std::system_error const&)
        {
            m_secondaryState.reset();
            m_secondaryLifetime = nullptr;
            StatusText().Text(L"Unable to start the secondary UI thread. Close other sample windows and try again.");
            AnnounceStatus(L"Unable to start the secondary UI thread. Close other sample windows and try again.");
        }
    }

    void MainWindow::PollSecondaryWindow()
    {
        if (!m_secondaryState || !m_secondaryState->done.load()) return;
        // The completion flag is set after apartment teardown; joining cannot wait on XAML.
        m_secondaryThread.join();
        m_secondaryPoll.Stop();
        auto const secondaryMessage = m_secondaryState->error.empty() ? hstring{ L"Secondary UI thread closed. Toggle to open it again." } : m_secondaryState->error;
        StatusText().Text(secondaryMessage);
        AnnounceStatus(secondaryMessage);
        m_secondaryState.reset();
        m_secondaryLifetime = nullptr;
        SecondaryButton().IsEnabled(true);
        if (m_closing) Close();
    }

    void MainWindow::OnLineChoiceChanged(IInspectable const& sender, SelectionChangedEventArgs const&)
    {
        ApplyLineEdit([&]
        {
            auto series = SelectedPresentationSeries();
            auto choice = sender.as<ComboBox>();
            if (choice == LineWeightChoice()) series.StrokeThickness(LineWeights[Selection(choice, 4)]);
            else if (choice == LineStyleChoice()) series.StrokeDashStyle(LineStyles[Selection(choice, 5)]);
            else if (choice == LineMarkerChoice()) series.MarkerShape(MarkerShapes[Selection(choice, 10)]);
            else if (choice == LineColorChoice())
            {
                auto index = Selection(choice, 9);
                series.Stroke(index == 0 ? nullptr : SolidColorBrush{ SeriesColors[index - 1] }.as<Brush>());
            }
        }, L"Selected line style updated.");
    }

    void MainWindow::OnLineVisibleClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyLineEdit([&] { SelectedPresentationSeries().IsVisible(Checked(LineVisibleCheckBox())); }, L"Line series visibility updated.");
    }

    void MainWindow::OnLineAddSeriesClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyLineEdit([&] { AddExampleSeries(0); }, L"Added and selected a line series.");
    }

    void MainWindow::OnLineRemoveSeriesClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyLineEdit([&] { RemoveSelectedSeries(MarkupChart(), PresentationSeriesComboBox()); }, L"Removed the selected line series.");
    }

    void MainWindow::OnLineResetClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyLineEdit([&]
        {
            auto chart = MarkupChart();
            chart.Series().Clear();
            chart.Data().Clear();
            chart.Axes().Clear();
            m_markupProfitValues.ReplaceAll(ProfitSeed);
            m_markupExpenseValues.ReplaceAll(ExpensesSeed);
            Profit().ItemsSource(m_markupProfitValues);
            Expenses().ItemsSource(m_markupExpenseValues);
            chart.Data().Append(Month());
            chart.Data().Append(Profit());
            chart.Data().Append(Expenses());
            m_yAxis.Minimum(nullptr);
            m_yAxis.Maximum(nullptr);
            m_yAxis.Spacing(nullptr);
            m_xAxis.SortKey(CategorySortKey::Index);
            m_xAxis.SortOrder(Charts::SortOrder::Ascending);
            ResetAxisAppearance(m_xAxis, CategoryAxis{});
            ResetAxisAppearance(m_yAxis, LinearAxis{});
            chart.Axes().Append(m_xAxis);
            chart.Axes().Append(m_yAxis);
            auto profit = ProfitSeries();
            auto expenses = ExpensesSeries();
            ResetSeriesDefaults(profit, 3, true, false);
            ResetSeriesDefaults(expenses, 2, false, true);
            profit.Title(L"Monthly profit");
            expenses.Title(L"Monthly expenses");
            profit.XValues(Month());
            profit.YValues(Profit());
            profit.XAxis(m_xAxis);
            profit.YAxis(m_yAxis);
            expenses.XValues(Month());
            expenses.YValues(Expenses());
            expenses.XAxis(nullptr);
            expenses.YAxis(nullptr);
            expenses.StrokeDashStyle(StrokeDashStyle::Dash);
            chart.Series().Append(profit);
            chart.Series().Append(expenses);
            chart.ShowLegend(true);
            chart.LegendTitle(L"Monthly totals");
            LegendTitleTextBox().Text(chart.LegendTitle());
            m_nextLineSeries = 3;
            RebuildSeriesSelector(chart, PresentationSeriesComboBox(), 0);
            SyncNumber(OverrideIndexNumberBox(), IReference<double>{ 0.0 });
            m_lastValidOverrideIndex = 0;
            SyncAxisControls(false);
        }, L"Line example reset.");
    }

    void MainWindow::OnAreaSeriesChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyExampleEdit(true, [] {}, L"Editing the selected area series.");
    }
    void MainWindow::OnAreaAddSeriesClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(true, [&] { AddExampleSeries(1); }, L"Added and selected an area series.");
    }
    void MainWindow::OnAreaRemoveSeriesClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(true, [&] { RemoveSelectedSeries(AreaMarkupChart(), AreaSeriesChoice()); }, L"Removed the selected area series.");
    }
    void MainWindow::OnAreaMarkerChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyExampleEdit(true, [&] { SelectedAreaSeries().MarkerShape(MarkerShapes[Selection(AreaMarkerChoice(), 10)]); }, L"Area marker shape updated.");
    }

    void MainWindow::OnBarSeriesChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyExampleEdit(false, [] {}, L"Editing the selected bar series.");
    }
    void MainWindow::OnBarAddSeriesClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(false, [&] { AddExampleSeries(2); }, L"Added and selected a bar series.");
    }
    void MainWindow::OnBarRemoveSeriesClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(false, [&] { RemoveSelectedSeries(BarMarkupChart(), BarSeriesChoice()); }, L"Removed the selected bar series.");
    }
    void MainWindow::OnBarMarkerChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyExampleEdit(false, [&] { SelectedBarSeries().MarkerShape(MarkerShapes[Selection(BarMarkerChoice(), 10)]); }, L"Bar marker shape updated.");
    }
    void MainWindow::OnBarMarkersClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(false, [&] { SelectedBarSeries().ShowDataMarkers(Checked(BarMarkersCheckBox())); }, L"Bar point markers updated.");
    }
    void MainWindow::OnBarOrientationChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyExampleEdit(false, [&]
        {
            SelectedBarSeries().Orientation(Selection(BarOrientationChoice(), 2) == 0
                ? BarOrientation::Horizontal : BarOrientation::Vertical);
        }, L"Bar orientation updated.");
    }

    void MainWindow::OnToggleBarOrientationClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(false, [&]
        {
            auto series = SelectedBarSeries();
            series.Orientation(series.Orientation() == BarOrientation::Horizontal ? BarOrientation::Vertical : BarOrientation::Horizontal);
        }, L"Bar orientation updated.");
    }

    void MainWindow::OnAreaChoiceChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyExampleEdit(true, [&]
        {
            SetAreaAppearance(Selection(AreaColorChoice(), 9), Selection(AreaFillChoice(), 3));
        }, L"Area color and fill updated.");
    }
    void MainWindow::OnAreaVisibleClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(true, [&] { SelectedAreaSeries().IsVisible(Checked(AreaVisibleCheckBox())); }, L"Area series visibility updated.");
    }
    void MainWindow::OnAreaValuesClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(true, [&] { SelectedAreaSeries().ShowDataLabels(Checked(AreaValuesCheckBox())); }, L"Area value labels updated.");
    }
    void MainWindow::OnAreaMarkersClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(true, [&] { SelectedAreaSeries().ShowDataMarkers(Checked(AreaMarkersCheckBox())); }, L"Area point markers updated.");
    }
    void MainWindow::OnAreaLegendClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(true, [&] { AreaMarkupChart().ShowLegend(Checked(AreaLegendCheckBox())); }, L"Area legend updated.");
    }
    void MainWindow::OnAreaResetClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(true, [&]
        {
            auto chart = AreaMarkupChart();
            chart.Series().Clear();
            chart.Data().Clear();
            chart.Axes().Clear();
            AreaValues().ItemsSource().as<IVector<double>>().ReplaceAll(AreaSeed);
            chart.Data().Append(AreaMonth());
            chart.Data().Append(AreaValues());
            auto series = MarkupAreaSeries();
            ResetSeriesDefaults(series, 2, false, false);
            series.Title(L"Monthly area");
            series.XValues(AreaMonth());
            series.YValues(AreaValues());
            series.XAxis(nullptr);
            series.YAxis(nullptr);
            chart.Series().Append(series);
            RebuildSeriesSelector(chart, AreaSeriesChoice(), 0);
            SetAreaAppearance(0, 0);
            chart.ShowLegend(true);
            m_nextAreaSeries = 2;
        }, L"Area example reset.");
    }
    void MainWindow::OnBarColorChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyExampleEdit(false, [&] { SetBarColor(Selection(BarColorChoice(), 9)); }, L"Bar color updated.");
    }
    void MainWindow::OnBarVisibleClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(false, [&] { SelectedBarSeries().IsVisible(Checked(BarVisibleCheckBox())); }, L"Bar series visibility updated.");
    }
    void MainWindow::OnBarValuesClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(false, [&] { SelectedBarSeries().ShowDataLabels(Checked(BarValuesCheckBox())); }, L"Bar value labels updated.");
    }
    void MainWindow::OnBarLegendClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(false, [&] { BarMarkupChart().ShowLegend(Checked(BarLegendCheckBox())); }, L"Bar legend updated.");
    }
    void MainWindow::OnBarResetClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(false, [&]
        {
            auto chart = BarMarkupChart();
            chart.Series().Clear();
            chart.Data().Clear();
            chart.Axes().Clear();
            BarValues().ItemsSource().as<IVector<double>>().ReplaceAll(BarSeed);
            chart.Data().Append(BarMonth());
            chart.Data().Append(BarValues());
            chart.Axes().Append(m_barXAxis);
            chart.Axes().Append(m_barYAxis);
            auto series = MarkupBarSeries();
            ResetSeriesDefaults(series, 1.5, false, false);
            series.Title(L"Monthly bars");
            series.XValues(BarMonth());
            series.YValues(BarValues());
            series.XAxis(m_barXAxis);
            series.YAxis(m_barYAxis);
            series.Orientation(BarOrientation::Horizontal);
            chart.Series().Append(series);
            RebuildSeriesSelector(chart, BarSeriesChoice(), 0);
            SetBarColor(0);
            chart.ShowLegend(true);
            m_nextBarSeries = 2;
        }, L"Bar example reset.");
    }

    void MainWindow::OnLegendVisibilityClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyLineEdit([&] { MarkupChart().ShowLegend(Checked(LegendVisibilityCheckBox())); }, L"Line legend updated.");
    }
    void MainWindow::OnApplyLegendTitleClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyLineEdit([&] { MarkupChart().LegendTitle(LegendTitleTextBox().Text()); }, L"Line legend title updated.");
    }
    void MainWindow::OnPresentationSeriesChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyLineEdit([] {}, L"Editing the selected line series.");
    }
    void MainWindow::OnShowDataLabelsClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyLineEdit([&] { SelectedPresentationSeries().ShowDataLabels(Checked(ShowDataLabelsCheckBox())); }, L"Line value labels updated.");
    }
    void MainWindow::OnShowDataMarkersClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyLineEdit([&] { SelectedPresentationSeries().ShowDataMarkers(Checked(ShowDataMarkersCheckBox())); }, L"Line point markers updated.");
    }
    void MainWindow::OnDataLabelBrushClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyEdit([&] { SelectedPresentationSeries().DataLabelBrush(SelectedBrush(DataLabelBrushCheckBox(), LabelBlue)); },
            PresentationKnobStatusText(), L"Series label brush updated.", MarkupChart());
    }
    void MainWindow::OnDataMarkerBrushClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyEdit([&] { SelectedPresentationSeries().DataMarkerBrush(SelectedBrush(DataMarkerBrushCheckBox(), MarkerOrange)); },
            PresentationKnobStatusText(), L"Series marker brush updated.", MarkupChart());
    }

    void MainWindow::OnOverrideIndexChanged(IInspectable const&, NumberBoxValueChangedEventArgs const& args)
    {
        QueueOverrideIndexEdit(args.OldValue(), true);
    }

    void MainWindow::OnOverrideIndexLostFocus(IInspectable const&, RoutedEventArgs const&)
    {
        QueueOverrideIndexEdit(m_lastValidOverrideIndex, false);
    }

    void MainWindow::QueueOverrideIndexEdit(double previousValue, bool synchronize)
    {
        if (!m_ready || m_syncing || m_closing) return;
        if (!DispatcherQueue().TryEnqueue([weak = get_weak(), previousValue, synchronize]
        {
            if (auto self = weak.get(); self && !self->m_closing)
            {
                try
                {
                    self->SelectedOverrideIndex();
                    if (synchronize) self->SyncPresentationKnobs();
                }
                catch (hresult_error const& error)
                {
                    auto previous = std::isfinite(previousValue) && previousValue >= 0 && previousValue <= 5 &&
                        std::trunc(previousValue) == previousValue ? previousValue : self->m_lastValidOverrideIndex;
                    {
                        SyncGuard guard{ self->m_syncing };
                        SyncNumber(self->OverrideIndexNumberBox(), IReference<double>{ previous });
                    }
                    self->SyncPresentationKnobs();
                    self->ReportError(error, self->PresentationKnobStatusText());
                }
            }
        }))
        {
            ReportError(hresult_error{ E_FAIL, L"The UI dispatcher could not apply the point index." }, PresentationKnobStatusText());
        }
    }

    void MainWindow::OnApplyLabelOverrideClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyEdit([&]
        {
            SelectedPresentationSeries().DataLabelOverrides().Insert(SelectedOverrideIndex(),
                DataLabelOverride{ LabelOverrideTextBox().Text(), SelectedBrush(LabelOverrideBrushCheckBox(), LabelGreen) });
            SyncPresentationKnobs();
        }, PresentationKnobStatusText(), L"Label override applied.", MarkupChart());
    }
    void MainWindow::OnRemoveLabelOverrideClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyEdit([&]
        {
            auto labels = SelectedPresentationSeries().DataLabelOverrides();
            auto index = SelectedOverrideIndex();
            if (labels.HasKey(index)) labels.Remove(index);
            SyncPresentationKnobs();
        }, PresentationKnobStatusText(), L"Label override removed, if present.", MarkupChart());
    }
    void MainWindow::OnApplyMarkerOverrideClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyEdit([&]
        {
            SelectedPresentationSeries().DataMarkerOverrides().Insert(SelectedOverrideIndex(),
                DataMarkerOverride{ MarkerShapes[Selection(MarkerShapeComboBox(), 10)], SelectedBrush(MarkerOverrideBrushCheckBox(), MarkerPurple) });
            SyncPresentationKnobs();
        }, PresentationKnobStatusText(), L"Marker override applied.", MarkupChart());
    }
    void MainWindow::OnRemoveMarkerOverrideClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyEdit([&]
        {
            auto markers = SelectedPresentationSeries().DataMarkerOverrides();
            auto index = SelectedOverrideIndex();
            if (markers.HasKey(index)) markers.Remove(index);
            SyncPresentationKnobs();
        }, PresentationKnobStatusText(), L"Marker override removed, if present.", MarkupChart());
    }
    void MainWindow::OnClearOverridesClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyEdit([&]
        {
            auto series = SelectedPresentationSeries();
            series.DataLabelOverrides().Clear();
            series.DataMarkerOverrides().Clear();
            SyncPresentationKnobs();
        }, PresentationKnobStatusText(), L"All overrides cleared for the selected series.", MarkupChart());
    }

    void MainWindow::OnLinearMinChanged(NumberBox const& sender, NumberBoxValueChangedEventArgs const&)
    {
        QueueLinearAxisEdit(sender, 0);
    }
    void MainWindow::OnLinearMaxChanged(NumberBox const& sender, NumberBoxValueChangedEventArgs const&)
    {
        QueueLinearAxisEdit(sender, 1);
    }
    void MainWindow::OnLinearSpacingChanged(NumberBox const& sender, NumberBoxValueChangedEventArgs const&)
    {
        QueueLinearAxisEdit(sender, 2);
    }
    void MainWindow::OnAxisNumberLostFocus(IInspectable const& sender, RoutedEventArgs const&)
    {
        auto box = sender.as<NumberBox>();
        QueueLinearAxisEdit(box, box == LinearMinBox() ? 0 : box == LinearMaxBox() ? 1 : 2);
    }

    void MainWindow::QueueLinearAxisEdit(NumberBox const& box, int property)
    {
        if (!m_ready || m_syncing || m_closing || !HasProfitSeries()) return;
        // Let NumberBox finish committing its text before validating or restoring the editor.
        if (!DispatcherQueue().TryEnqueue([weak = get_weak(), box, property]
        {
            if (auto self = weak.get(); self && !self->m_closing && self->HasProfitSeries())
            {
                try
                {
                    auto value = AutoNumber(box);
                    auto current = property == 0 ? self->m_yAxis.Minimum() :
                        property == 1 ? self->m_yAxis.Maximum() : self->m_yAxis.Spacing();
                    if ((!value && !current) || (value && current && value.Value() == current.Value())) return;
                    self->ApplyAxisEdit([&]
                    {
                        if (property == 0)
                        {
                            if (value && self->m_yAxis.Maximum() && value.Value() >= self->m_yAxis.Maximum().Value())
                                throw hresult_invalid_argument(L"Minimum must be less than maximum.");
                            self->m_yAxis.Minimum(value);
                        }
                        else if (property == 1)
                        {
                            if (value && self->m_yAxis.Minimum() && value.Value() <= self->m_yAxis.Minimum().Value())
                                throw hresult_invalid_argument(L"Maximum must be greater than minimum.");
                            self->m_yAxis.Maximum(value);
                        }
                        else
                        {
                            if (value && value.Value() <= 0)
                                throw hresult_invalid_argument(L"Spacing must be positive, or empty for Auto.");
                            self->m_yAxis.Spacing(value);
                        }
                    });
                }
                catch (hresult_error const& error)
                {
                    self->SyncAxisControls(false);
                    self->ReportError(error, self->PresentationKnobStatusText());
                }
            }
        }))
        {
            ReportError(hresult_error{ E_FAIL, L"The UI dispatcher could not apply the axis edit." }, PresentationKnobStatusText());
        }
    }
    void MainWindow::OnSortKeyChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyAxisEdit([&] { m_xAxis.SortKey(static_cast<CategorySortKey>(Selection(SortKeyBox(), 2))); });
    }
    void MainWindow::OnSortOrderChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyAxisEdit([&] { m_xAxis.SortOrder(static_cast<Charts::SortOrder>(Selection(SortOrderBox(), 2))); });
    }
    void MainWindow::OnTickLabelsChanged(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyAxisEdit([&] { m_yAxis.ShowTickLabels(Checked(TickLabelsCheck())); m_xAxis.ShowTickLabels(Checked(TickLabelsCheck())); });
    }
    void MainWindow::OnTickMarksChanged(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyAxisEdit([&] { m_yAxis.ShowTickMarks(Checked(TickMarksCheck())); m_xAxis.ShowTickMarks(Checked(TickMarksCheck())); });
    }
    void MainWindow::OnAxisVisibleChanged(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyAxisEdit([&] { m_yAxis.IsVisible(Checked(AxisVisibleCheck())); m_xAxis.IsVisible(Checked(AxisVisibleCheck())); });
    }
    void MainWindow::OnGridLinesChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyAxisEdit([&]
        {
            auto value = static_cast<Charts::GridLines>(Selection(GridLinesBox(), 3));
            m_yAxis.GridLines(value);
            m_xAxis.GridLines(value);
        });
    }
    void MainWindow::OnGridLineBrushChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyAxisEdit([&] { auto brush = AxisBrush(GridLineBrushBox()); m_yAxis.GridLineMajorBrush(brush); m_xAxis.GridLineMajorBrush(brush); });
    }
    void MainWindow::OnTickBrushChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyAxisEdit([&] { auto brush = AxisBrush(TickBrushBox()); m_yAxis.TickBrush(brush); m_xAxis.TickBrush(brush); });
    }
    void MainWindow::OnTickLabelBrushChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyAxisEdit([&] { auto brush = AxisBrush(TickLabelBrushBox()); m_yAxis.TickLabelBrush(brush); m_xAxis.TickLabelBrush(brush); });
    }
    void MainWindow::OnAxisLineBrushChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyAxisEdit([&] { auto brush = AxisBrush(AxisLineBrushBox()); m_yAxis.AxisLineBrush(brush); m_xAxis.AxisLineBrush(brush); });
    }

    void MainWindow::ApplyDateInterval(bool monthly)
    {
        if (!m_ready || m_syncing || m_closing) return;
        auto box = monthly ? DtIntervalTypeBoxB() : DtIntervalTypeBoxA();
        auto axis = monthly ? m_dtAxisB : m_dtAxisA;
        auto warning = monthly ? DtWarningB() : DtWarningA();
        auto index = box.SelectedIndex();
        if (index < 0 || index >= static_cast<int32_t>(IntervalTypes.size())) return;
        auto note = DateIntervalNote(IntervalTypes[index], monthly);
        ApplyEdit([&]
        {
            axis.IntervalType(IntervalTypes[index]);
            warning.Text(note);
            warning.Visibility(note.empty() ? Visibility::Collapsed : Visibility::Visible);
        }, StatusText(), L"Date interval updated.", nullptr, [&] { box.SelectedIndex(static_cast<int32_t>(axis.IntervalType())); },
            note.empty() ? hstring{} : L"Date interval updated. " + note);
    }

    void MainWindow::ApplyDateFormat(bool monthly)
    {
        auto box = monthly ? DtLabelFormatBoxB() : DtLabelFormatBoxA();
        auto axis = monthly ? m_dtAxisB : m_dtAxisA;
        ApplyEdit([&]
        {
            auto format = box.Text();
            // Validate with the same public formatter before applying the template.
            if (!format.empty())
            {
                try
                {
                    Windows::Globalization::DateTimeFormatting::DateTimeFormatter formatter{ format };
                    formatter.Format(CalendarDate(2024, 1, 1));
                }
                catch (hresult_error const&)
                {
                    throw hresult_invalid_argument(L"Invalid date template. Try shortdate or month year; leave it empty for the default.");
                }
            }
            axis.LabelFormat(format);
        }, StatusText(), L"Label format applied.", nullptr, [&] { box.Text(axis.LabelFormat()); });
    }
    void MainWindow::OnDtIntervalTypeAChanged(IInspectable const&, SelectionChangedEventArgs const&) { ApplyDateInterval(false); }
    void MainWindow::OnDtLabelFormatAApply(IInspectable const&, RoutedEventArgs const&) { ApplyDateFormat(false); }
    void MainWindow::OnDtIntervalTypeBChanged(IInspectable const&, SelectionChangedEventArgs const&) { ApplyDateInterval(true); }
    void MainWindow::OnDtLabelFormatBApply(IInspectable const&, RoutedEventArgs const&) { ApplyDateFormat(true); }
}
