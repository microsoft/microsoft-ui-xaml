#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <winrt/Microsoft.UI.Xaml.Automation.h>
#include <winrt/Microsoft.UI.Xaml.Hosting.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Windows.Globalization.DateTimeFormatting.h>
#include <winrt/Windows.Globalization.NumberFormatting.h>
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
        Windows::UI::Color{ 0xFF, 0x10, 0x7C, 0x10 },
        Windows::UI::Color{ 0xFF, 0xD8, 0x3B, 0x01 }
    };
    constexpr std::array<uint8_t, 3> AreaFillAlphas{ 0x60, 0xFF, 0x00 };

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
        // Inspect the public editor so invalid text is not mistaken for an automatic bound.
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
        text << (error.code() == E_INVALIDARG
            ? L"The value is not valid. Check the range, spacing, point index or date format. "
            : L"The chart operation could not be completed. Check the sample's SDK and runtime setup. ");
        text << L"(HRESULT 0x" << std::uppercase << std::hex << std::setw(8) << std::setfill(L'0')
             << static_cast<uint32_t>(error.code().value) << L")";
        return hstring{ text.str() };
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

    hstring DataRows(IObservableVector<double> const& values)
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

namespace winrt::ChartAppCppUnpackaged::implementation
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
                chart.Resources(Markup::XamlReader::Load(resourceMarkup).as<ResourceDictionary>());
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
                AutomationProperties::SetName(data, L"Current secondary chart values");
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

    MainWindow::MainWindow()
    {
        InitializeComponent();
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
        m_yAxis = LinearAxis{};
        MarkupChart().Axes().Append(m_xAxis);
        MarkupChart().Axes().Append(m_yAxis);
        ProfitSeries().XAxis(m_xAxis);
        ProfitSeries().YAxis(m_yAxis);
        auto barX = CategoryAxis{};
        auto barY = LinearAxis{};
        BarMarkupChart().Axes().Append(barX);
        BarMarkupChart().Axes().Append(barY);
        MarkupBarSeries().XAxis(barX);
        MarkupBarSeries().YAxis(barY);
        CreateCodeChart();
        CreateDateTimeCharts();
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
        m_codeChart = Chart{};
        m_codeChart.FontSize(14);
        m_codeChart.Series().Append(line);
        CodeChartHost().Child(m_codeChart);

        // Dimensional handles can be connected before their source collections exist.
        auto xValues = Samples{};
        line.XValues(xValues);
        m_codeChart.Data().Append(xValues);
        m_codeChart.Data().Append(yValues);
        xValues.ItemsSource(single_threaded_observable_vector<hstring>({ L"Alpha", L"Beta", L"Gamma", L"Delta", L"Epsilon" }));
        m_codeChartValues = single_threaded_observable_vector<double>({ 12, 38, 21, 47, 34 });
        yValues.ItemsSource(m_codeChartValues);
        line.Title(L"Initialized out of order");
        line.StrokeDashStyle(StrokeDashStyle::DashDot);
        line.StrokeThickness(3);
        line.Stroke(SolidColorBrush{ Windows::UI::Color{ 255, 0, 120, 212 } });
        line.DataLabelOverrides().Insert(1, DataLabelOverride{ L"Beta", SolidColorBrush{ LabelGreen } });
        line.DataMarkerOverrides().Insert(3, DataMarkerOverride{ MarkerShape::Diamond, SolidColorBrush{ MarkerPurple } });
        m_codeChart.ShowLegend(true);
        AutomationProperties::SetName(m_codeChart, L"Code-created line chart: Alpha, Beta, Gamma, Delta, Epsilon");
        AutomationProperties::SetHelpText(m_codeChart, L"Exact values are listed under Current data.");
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
        constexpr wchar_t const* months[]{ L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun" };
        std::wostringstream text;
        text << L"Month   Profit   Expenses";
        for (uint32_t i = 0; i < 6; ++i)
        {
            text << L'\n' << months[i] << std::setw(9) << m_markupProfitValues.GetAt(i)
                 << std::setw(11) << m_markupExpenseValues.GetAt(i);
        }
        text << L"\n\nCode line (Alpha, Beta, Gamma, Delta, Epsilon):\n" << DataRows(m_codeChartValues).c_str();
        DataText().Text(text.str());
    }

    void MainWindow::ReportError(hresult_error const& error)
    {
        auto text = ErrorText(error);
        StatusText().Text(text);
        PresentationKnobStatusText().Text(text);
    }

    void MainWindow::ApplyEdit(std::function<void()> const& edit)
    {
        if (!m_ready || m_syncing || m_closing) return;
        try
        {
            edit();
            StatusText().Text(L"Settings applied.");
            PresentationKnobStatusText().Text(L"Editors show the selected series and point's current settings.");
        }
        catch (hresult_error const& error)
        {
            SyncAxisControls();
            SyncPresentationKnobs();
            ReportError(error);
        }
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
            }
            synchronize();
            status.Text(message);
        }
        catch (hresult_error const& error)
        {
            synchronize();
            status.Text(error.code() == E_INVALIDARG
                ? hstring{ area ? L"The area option is not valid. Choose an available option." : L"The bar option is not valid. Choose an available option." }
                : hstring{ area ? L"Area options: " : L"Bar options: " } + ErrorText(error));
        }
    }

    void MainWindow::SyncAreaOptions()
    {
        SyncGuard guard{ m_syncing };
        auto series = MarkupAreaSeries();
        AreaColorChoice().SelectedIndex(SeriesColorIndex(series.Fill(), series.Stroke(), true));
        AreaFillChoice().SelectedIndex(AreaFillIndex(series.Fill()));
        AreaVisibleCheckBox().IsChecked(series.IsVisible());
        AreaValuesCheckBox().IsChecked(series.ShowDataLabels());
        AreaMarkersCheckBox().IsChecked(series.ShowDataMarkers());
        AreaLegendCheckBox().IsChecked(AreaMarkupChart().ShowLegend());
    }

    void MainWindow::SyncBarOptions()
    {
        SyncGuard guard{ m_syncing };
        auto series = MarkupBarSeries();
        BarColorChoice().SelectedIndex(SeriesColorIndex(series.Fill(), series.Stroke(), false));
        BarVisibleCheckBox().IsChecked(series.IsVisible());
        BarValuesCheckBox().IsChecked(series.ShowDataLabels());
        BarLegendCheckBox().IsChecked(BarMarkupChart().ShowLegend());
        BarOrientationText().Text(series.Orientation() == BarOrientation::Horizontal
            ? L"Orientation: Horizontal" : L"Orientation: Vertical");
    }

    void MainWindow::SetAreaAppearance(int32_t colorIndex, int32_t fillIndex)
    {
        auto series = MarkupAreaSeries();
        auto fill = colorIndex == 0 ? AreaOriginalFill : SeriesColors[colorIndex - 1];
        auto stroke = colorIndex == 0 ? AreaOriginalStroke : SeriesColors[colorIndex - 1];
        fill.A = AreaFillAlphas[fillIndex];
        // A transparent brush preserves outline-only mode; null would restore a palette fill.
        series.Fill(SolidColorBrush{ fill });
        series.Stroke(SolidColorBrush{ stroke });
        if (series.ShowDataMarkers()) SetAreaMarkers(true);
    }

    void MainWindow::SetAreaMarkers(bool visible)
    {
        auto series = MarkupAreaSeries();
        series.ShowDataMarkers(visible);
        series.MarkerShape(Charts::MarkerShape::Circle);
        series.DataMarkerBrush(visible ? series.Stroke() : nullptr);
    }

    void MainWindow::SetBarColor(int32_t colorIndex)
    {
        auto series = MarkupBarSeries();
        series.Fill(SolidColorBrush{ colorIndex == 0 ? BarOriginalFill : SeriesColors[colorIndex - 1] });
        series.Stroke(SolidColorBrush{ colorIndex == 0 ? BarOriginalStroke : SeriesColors[colorIndex - 1] });
    }

    LineSeries MainWindow::SelectedPresentationSeries()
    {
        return Selection(PresentationSeriesComboBox(), 2) == 1 ? ExpensesSeries() : ProfitSeries();
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
        return static_cast<uint32_t>(value);
    }

    void MainWindow::SyncPresentationKnobs()
    {
        m_syncing = true;
        auto series = SelectedPresentationSeries();
        LegendVisibilityCheckBox().IsChecked(MarkupChart().ShowLegend());
        LegendTitleTextBox().Text(MarkupChart().LegendTitle());
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
            SyncNumber(OverrideIndexNumberBox(), IReference<double>{ 0.0 });
            ReportError(error);
        }
        auto labels = series.DataLabelOverrides();
        auto label = labels.HasKey(index) ? labels.Lookup(index) : nullptr;
        LabelOverrideTextBox().Text(label ? label.Text() : L"");
        LabelOverrideBrushCheckBox().IsChecked(label && label.Brush() != nullptr);
        auto markers = series.DataMarkerOverrides();
        auto marker = markers.HasKey(index) ? markers.Lookup(index) : nullptr;
        MarkerShapeComboBox().SelectedIndex(marker ? static_cast<int32_t>(marker.Shape()) : 8);
        MarkerOverrideBrushCheckBox().IsChecked(marker && marker.Brush() != nullptr);
        m_syncing = false;
    }

    void MainWindow::SyncAxisControls()
    {
        m_syncing = true;
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
        DtIntervalTypeBoxA().SelectedIndex(static_cast<int32_t>(m_dtAxisA.IntervalType()));
        DtIntervalTypeBoxB().SelectedIndex(static_cast<int32_t>(m_dtAxisB.IntervalType()));
        DtLabelFormatBoxA().Text(m_dtAxisA.LabelFormat());
        DtLabelFormatBoxB().Text(m_dtAxisB.LabelFormat());
        m_syncing = false;
    }

    void MainWindow::OnLayoutSizeChanged(IInspectable const&, SizeChangedEventArgs const& args)
    {
        if (!m_ready || m_closing) return;
        auto width = args.NewSize().Width;
        bool compact = width < 1000;
        Grid::SetRow(ThemeChoice(), compact ? 2 : 0);
        Grid::SetColumn(ThemeChoice(), compact ? 0 : 1);
        ThemeChoice().HorizontalAlignment(compact ? HorizontalAlignment::Left : HorizontalAlignment::Right);
        ScenarioHeader().ColumnSpacing(compact ? 0 : 24);
        ScenarioContent().Padding(Thickness{ 24, width < 840 ? 56.0 : 24.0, 24, 24 });
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
        AxesEditorPanel().Visibility(visible(tag == L"axes"));
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
        DateTimeScenario().ColumnDefinitions().GetAt(1).Width(GridLength{ wide ? 1.0 : 0.0, GridUnitType::Star });
        Grid::SetColumn(MonthlyDateCard(), wide ? 1 : 0);
        Grid::SetRow(MonthlyDateCard(), wide ? 0 : 1);
        Grid::SetColumnSpan(DateFormatHelp(), wide ? 2 : 1);
        Grid::SetRow(DateFormatHelp(), wide ? 1 : 2);
    }

    void MainWindow::OnThemeChoiceChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        if (!m_ready || m_closing) return;
        switch (ThemeChoice().SelectedIndex())
        {
        case 1: OnLightClick(nullptr, nullptr); break;
        case 2: OnDarkClick(nullptr, nullptr); break;
        default: OnSystemClick(nullptr, nullptr); break;
        }
    }

    void MainWindow::OnLightClick(IInspectable const&, RoutedEventArgs const&) { RootGrid().RequestedTheme(ElementTheme::Light); }
    void MainWindow::OnDarkClick(IInspectable const&, RoutedEventArgs const&) { RootGrid().RequestedTheme(ElementTheme::Dark); }
    void MainWindow::OnSystemClick(IInspectable const&, RoutedEventArgs const&) { RootGrid().RequestedTheme(ElementTheme::Default); }

    void MainWindow::OnToggleUpdatesClick(IInspectable const&, RoutedEventArgs const&)
    {
        if (!m_codeChartTimer) return;
        if (m_codeChartTimer.IsEnabled()) m_codeChartTimer.Stop();
        else m_codeChartTimer.Start();
        UpdatesButton().Content(box_value(m_codeChartTimer.IsEnabled() ? L"Pause updates" : L"Resume updates"));
        StatusText().Text(m_codeChartTimer.IsEnabled() ? L"Primary data updates once per second." : L"Primary updates paused. The secondary chart is independent.");
    }

    void MainWindow::OnToggleSecondaryClick(IInspectable const&, RoutedEventArgs const&)
    {
        if (m_closing) return;
        if (m_secondaryThread.joinable())
        {
            m_secondaryState->stop.store(true);
            SecondaryButton().IsEnabled(false);
            StatusText().Text(L"Closing the secondary UI thread...");
            return;
        }
        m_secondaryState = std::make_shared<SecondaryChartState>();
        m_secondaryLifetime = get_strong();
        try
        {
            m_secondaryThread = std::thread{ RunSecondaryWindow, m_secondaryState };
            m_secondaryPoll.Start();
            StatusText().Text(L"Opening an independently updating chart on a secondary UI thread.");
        }
        catch (std::system_error const&)
        {
            m_secondaryState.reset();
            m_secondaryLifetime = nullptr;
            StatusText().Text(L"Unable to start the secondary UI thread. Close other sample windows and try again.");
        }
    }

    void MainWindow::PollSecondaryWindow()
    {
        if (!m_secondaryState || !m_secondaryState->done.load()) return;
        // The completion flag is set after apartment teardown; joining cannot wait on XAML.
        m_secondaryThread.join();
        m_secondaryPoll.Stop();
        StatusText().Text(m_secondaryState->error.empty() ? L"Secondary UI thread closed. Toggle to open it again." : m_secondaryState->error);
        m_secondaryState.reset();
        m_secondaryLifetime = nullptr;
        SecondaryButton().IsEnabled(true);
        if (m_closing) Close();
    }

    void MainWindow::OnToggleBarOrientationClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(false, [&]
        {
            auto series = MarkupBarSeries();
            series.Orientation(series.Orientation() == BarOrientation::Horizontal ? BarOrientation::Vertical : BarOrientation::Horizontal);
        }, L"Bar orientation updated.");
    }

    void MainWindow::OnAreaChoiceChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyExampleEdit(true, [&]
        {
            SetAreaAppearance(Selection(AreaColorChoice(), 4), Selection(AreaFillChoice(), 3));
        }, L"Area color and fill updated.");
    }
    void MainWindow::OnAreaVisibleClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(true, [&] { MarkupAreaSeries().IsVisible(Checked(AreaVisibleCheckBox())); }, L"Area series visibility updated.");
    }
    void MainWindow::OnAreaValuesClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(true, [&] { MarkupAreaSeries().ShowDataLabels(Checked(AreaValuesCheckBox())); }, L"Area value labels updated.");
    }
    void MainWindow::OnAreaMarkersClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(true, [&] { SetAreaMarkers(Checked(AreaMarkersCheckBox())); }, L"Area point markers updated.");
    }
    void MainWindow::OnAreaLegendClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(true, [&] { AreaMarkupChart().ShowLegend(Checked(AreaLegendCheckBox())); }, L"Area legend updated.");
    }
    void MainWindow::OnAreaResetClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(true, [&]
        {
            auto series = MarkupAreaSeries();
            SetAreaMarkers(false);
            SetAreaAppearance(0, 0);
            series.IsVisible(true);
            series.ShowDataLabels(false);
            AreaMarkupChart().ShowLegend(true);
        }, L"Area example reset.");
    }
    void MainWindow::OnBarColorChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyExampleEdit(false, [&] { SetBarColor(Selection(BarColorChoice(), 4)); }, L"Bar color updated.");
    }
    void MainWindow::OnBarVisibleClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(false, [&] { MarkupBarSeries().IsVisible(Checked(BarVisibleCheckBox())); }, L"Bar series visibility updated.");
    }
    void MainWindow::OnBarValuesClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(false, [&] { MarkupBarSeries().ShowDataLabels(Checked(BarValuesCheckBox())); }, L"Bar value labels updated.");
    }
    void MainWindow::OnBarLegendClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(false, [&] { BarMarkupChart().ShowLegend(Checked(BarLegendCheckBox())); }, L"Bar legend updated.");
    }
    void MainWindow::OnBarResetClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyExampleEdit(false, [&]
        {
            auto series = MarkupBarSeries();
            SetBarColor(0);
            series.IsVisible(true);
            series.ShowDataLabels(false);
            BarMarkupChart().ShowLegend(true);
            series.Orientation(BarOrientation::Horizontal);
        }, L"Bar example reset.");
    }

    void MainWindow::OnLegendVisibilityClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyEdit([&] { MarkupChart().ShowLegend(Checked(LegendVisibilityCheckBox())); });
    }
    void MainWindow::OnApplyLegendTitleClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyEdit([&] { MarkupChart().LegendTitle(LegendTitleTextBox().Text()); });
    }
    void MainWindow::OnPresentationSeriesChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyEdit([&] { SyncPresentationKnobs(); });
    }
    void MainWindow::OnShowDataLabelsClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyEdit([&] { SelectedPresentationSeries().ShowDataLabels(Checked(ShowDataLabelsCheckBox())); });
    }
    void MainWindow::OnShowDataMarkersClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyEdit([&] { SelectedPresentationSeries().ShowDataMarkers(Checked(ShowDataMarkersCheckBox())); });
    }
    void MainWindow::OnDataLabelBrushClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyEdit([&] { SelectedPresentationSeries().DataLabelBrush(SelectedBrush(DataLabelBrushCheckBox(), LabelBlue)); });
    }
    void MainWindow::OnDataMarkerBrushClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyEdit([&] { SelectedPresentationSeries().DataMarkerBrush(SelectedBrush(DataMarkerBrushCheckBox(), MarkerOrange)); });
    }

    void MainWindow::OnOverrideIndexChanged(IInspectable const&, NumberBoxValueChangedEventArgs const& args)
    {
        QueueOverrideIndexEdit(args.OldValue(), true);
    }

    void MainWindow::OnOverrideIndexLostFocus(IInspectable const&, RoutedEventArgs const&)
    {
        QueueOverrideIndexEdit(0, false);
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
                        std::trunc(previousValue) == previousValue ? previousValue : 0.0;
                    self->m_syncing = true;
                    SyncNumber(self->OverrideIndexNumberBox(), IReference<double>{ previous });
                    self->m_syncing = false;
                    self->SyncPresentationKnobs();
                    self->ReportError(error);
                }
            }
        }))
        {
            ReportError(hresult_error{ E_FAIL, L"The UI dispatcher could not apply the point index." });
        }
    }

    void MainWindow::OnApplyLabelOverrideClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyEdit([&]
        {
            SelectedPresentationSeries().DataLabelOverrides().Insert(SelectedOverrideIndex(),
                DataLabelOverride{ LabelOverrideTextBox().Text(), SelectedBrush(LabelOverrideBrushCheckBox(), LabelGreen) });
            SyncPresentationKnobs();
        });
    }
    void MainWindow::OnRemoveLabelOverrideClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyEdit([&]
        {
            auto labels = SelectedPresentationSeries().DataLabelOverrides();
            auto index = SelectedOverrideIndex();
            if (labels.HasKey(index)) labels.Remove(index);
            SyncPresentationKnobs();
        });
    }
    void MainWindow::OnApplyMarkerOverrideClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyEdit([&]
        {
            SelectedPresentationSeries().DataMarkerOverrides().Insert(SelectedOverrideIndex(),
                DataMarkerOverride{ MarkerShapes[Selection(MarkerShapeComboBox(), 10)], SelectedBrush(MarkerOverrideBrushCheckBox(), MarkerPurple) });
            SyncPresentationKnobs();
        });
    }
    void MainWindow::OnRemoveMarkerOverrideClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyEdit([&]
        {
            auto markers = SelectedPresentationSeries().DataMarkerOverrides();
            auto index = SelectedOverrideIndex();
            if (markers.HasKey(index)) markers.Remove(index);
            SyncPresentationKnobs();
        });
    }
    void MainWindow::OnClearOverridesClick(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyEdit([&]
        {
            auto series = SelectedPresentationSeries();
            series.DataLabelOverrides().Clear();
            series.DataMarkerOverrides().Clear();
            SyncPresentationKnobs();
        });
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
        if (!m_ready || m_syncing || m_closing) return;
        // Let NumberBox finish committing its text before validating or restoring the editor.
        if (!DispatcherQueue().TryEnqueue([weak = get_weak(), box, property]
        {
            if (auto self = weak.get(); self && !self->m_closing)
            {
                try
                {
                    auto value = AutoNumber(box);
                    auto current = property == 0 ? self->m_yAxis.Minimum() :
                        property == 1 ? self->m_yAxis.Maximum() : self->m_yAxis.Spacing();
                    if ((!value && !current) || (value && current && value.Value() == current.Value())) return;
                    self->ApplyEdit([&]
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
                    self->SyncAxisControls();
                    self->ReportError(error);
                }
            }
        }))
        {
            ReportError(hresult_error{ E_FAIL, L"The UI dispatcher could not apply the axis edit." });
        }
    }
    void MainWindow::OnSortKeyChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyEdit([&] { m_xAxis.SortKey(static_cast<CategorySortKey>(Selection(SortKeyBox(), 2))); });
    }
    void MainWindow::OnSortOrderChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyEdit([&] { m_xAxis.SortOrder(static_cast<Charts::SortOrder>(Selection(SortOrderBox(), 2))); });
    }
    void MainWindow::OnTickLabelsChanged(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyEdit([&] { m_yAxis.ShowTickLabels(Checked(TickLabelsCheck())); m_xAxis.ShowTickLabels(Checked(TickLabelsCheck())); });
    }
    void MainWindow::OnTickMarksChanged(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyEdit([&] { m_yAxis.ShowTickMarks(Checked(TickMarksCheck())); m_xAxis.ShowTickMarks(Checked(TickMarksCheck())); });
    }
    void MainWindow::OnAxisVisibleChanged(IInspectable const&, RoutedEventArgs const&)
    {
        ApplyEdit([&] { m_yAxis.IsVisible(Checked(AxisVisibleCheck())); m_xAxis.IsVisible(Checked(AxisVisibleCheck())); });
    }
    void MainWindow::OnGridLinesChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyEdit([&]
        {
            auto value = static_cast<Charts::GridLines>(Selection(GridLinesBox(), 3));
            m_yAxis.GridLines(value);
            m_xAxis.GridLines(value);
        });
    }
    void MainWindow::OnGridLineBrushChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyEdit([&] { auto brush = AxisBrush(GridLineBrushBox()); m_yAxis.GridLineMajorBrush(brush); m_xAxis.GridLineMajorBrush(brush); });
    }
    void MainWindow::OnTickBrushChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyEdit([&] { auto brush = AxisBrush(TickBrushBox()); m_yAxis.TickBrush(brush); m_xAxis.TickBrush(brush); });
    }
    void MainWindow::OnTickLabelBrushChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyEdit([&] { auto brush = AxisBrush(TickLabelBrushBox()); m_yAxis.TickLabelBrush(brush); m_xAxis.TickLabelBrush(brush); });
    }
    void MainWindow::OnAxisLineBrushChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        ApplyEdit([&] { auto brush = AxisBrush(AxisLineBrushBox()); m_yAxis.AxisLineBrush(brush); m_xAxis.AxisLineBrush(brush); });
    }

    void MainWindow::ApplyDateInterval(bool monthly)
    {
        ApplyEdit([&]
        {
            auto box = monthly ? DtIntervalTypeBoxB() : DtIntervalTypeBoxA();
            auto axis = monthly ? m_dtAxisB : m_dtAxisA;
            auto type = IntervalTypes[Selection(box, 5)];
            axis.IntervalType(type);
            auto warning = monthly ? DtWarningB() : DtWarningA();
            bool dense = monthly && (type == DateTimeIntervalType::Day || type == DateTimeIntervalType::Week);
            bool sparse = !monthly && type == DateTimeIntervalType::Year;
            warning.Text(dense ? L"Day/Week intervals are too dense for this three-year monthly range."
                : sparse ? L"Year intervals are not useful for a 75-day range." : L"");
            warning.Visibility(dense || sparse ? Visibility::Visible : Visibility::Collapsed);
        });
    }

    void MainWindow::ApplyDateFormat(bool monthly)
    {
        ApplyEdit([&]
        {
            auto box = monthly ? DtLabelFormatBoxB() : DtLabelFormatBoxA();
            auto axis = monthly ? m_dtAxisB : m_dtAxisA;
            auto format = box.Text();
            // Validate with the same public formatter before applying the template.
            if (!format.empty())
            {
                Windows::Globalization::DateTimeFormatting::DateTimeFormatter formatter{ format };
                formatter.Format(CalendarDate(2024, 1, 1));
            }
            axis.LabelFormat(format);
        });
    }
    void MainWindow::OnDtIntervalTypeAChanged(IInspectable const&, SelectionChangedEventArgs const&) { ApplyDateInterval(false); }
    void MainWindow::OnDtLabelFormatAApply(IInspectable const&, RoutedEventArgs const&) { ApplyDateFormat(false); }
    void MainWindow::OnDtIntervalTypeBChanged(IInspectable const&, SelectionChangedEventArgs const&) { ApplyDateInterval(true); }
    void MainWindow::OnDtLabelFormatBApply(IInspectable const&, RoutedEventArgs const&) { ApplyDateFormat(true); }
}
