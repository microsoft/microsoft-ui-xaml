// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using System.Runtime.CompilerServices;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;
using Windows.UI.ViewManagement;
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates <c>TableViewTemplateColumn</c> cell templating: a read-only
/// visual template (the department / status chips) alongside a genuinely
/// interactive one (a two-way bound CheckBox), plus picker / menu / flyout
/// placement from inside cells.
/// </summary>
public sealed partial class CellFlyoutsPage : Page, INotifyPropertyChanged
{
    private string _statusText = string.Empty;
    private Person? _watchedRow;
    private TableViewSource? _source;
    private string _mode = "flat";
    private string _groupKey = "Department";

    // Set only once GroupBy / ClearGroupBy has actually returned, so the readout
    // cannot claim a mode the source never took.
    private string _appliedMode = "Flat";

    protected override Windows.Foundation.Size MeasureOverride(Windows.Foundation.Size availableSize)
    {
        if (Content is FrameworkElement child)
        {
            child.Measure(availableSize);
            return child.DesiredSize;
        }
        return new Windows.Foundation.Size(0, 0);
    }

    protected override Windows.Foundation.Size ArrangeOverride(Windows.Foundation.Size finalSize)
    {
        if (Content is FrameworkElement child)
        {
            child.Arrange(new Windows.Foundation.Rect(0, 0, finalSize.Width, finalSize.Height));
        }
        return finalSize;
    }

    public CellFlyoutsPage()
    {
        InitializeComponent();

        foreach (var person in PersonData.Take(18))
        {
            People.Add(person);
        }

        PeopleTable.ItemsSource = _source = TableViewSource.From(People);
        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;
        ApplyGrouping();
        UpdateStatus("Ready: toggle Active, or open any picker, drop-down, or flyout from a cell.");
    }

    public ObservableCollection<Person> People { get; } = new();

    public string StatusText
    {
        get => _statusText;
        private set
        {
            if (_statusText != value)
            {
                _statusText = value;
                OnPropertyChanged();
            }
        }
    }

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        // Watch every row so the checked count stays truthful; detached in Unloaded.
        foreach (var person in People)
        {
            person.PropertyChanged += OnPersonChanged;
        }

        if (People.Count > 0)
        {
            _watchedRow = People[0];
            RefreshReadout();
        }

        UpdateCheckedCount();
    }

    private void OnPageUnloaded(object sender, RoutedEventArgs e)
    {
        foreach (var person in People)
        {
            person.PropertyChanged -= OnPersonChanged;
        }

        _watchedRow = null;
    }

    private void OnPersonChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (e.PropertyName == nameof(Person.IsActive))
        {
            UpdateCheckedCount();

            if (sender is Person person)
            {
                UpdateStatus($"Active = {person.IsActive} committed for {person.FullName}.");
            }
        }

        if (ReferenceEquals(sender, _watchedRow))
        {
            RefreshReadout();
            UpdateStatus($"{e.PropertyName} changed for first row.");
        }
    }

    private void UpdateCheckedCount()
    {
        if (CheckedCountText is null)
        {
            return;
        }

        var checkedCount = People.Count(p => p.IsActive);
        CheckedCountText.Text = string.Format(
            CultureInfo.CurrentCulture,
            "{0:N0} of {1:N0}",
            checkedCount,
            People.Count);
    }

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_source is null || ShapingModeSelector?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _mode = tag;
        ApplyGrouping();
    }

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_source is null || GroupKeySelector?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _groupKey = tag;
        ApplyGrouping();
    }

    private void OnExpandAllGroupsClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode == "Flat")
        {
            return;
        }

        PeopleTable.ExpandAllGroups();
    }

    private void OnCollapseAllGroupsClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode == "Flat")
        {
            return;
        }

        PeopleTable.CollapseAllGroups();
    }

    /// <summary>
    /// Reshapes the existing <see cref="TableViewSource"/> in place. GroupBy /
    /// ClearGroupBy mutate and return the same instance, so the source is never
    /// rebuilt — rebuilding would drop selection, scroll offset and expansion.
    /// </summary>
    private void ApplyGrouping()
    {
        if (_source is null)
        {
            return;
        }

        var grouped = _mode == "grouped";
        switch (_mode)
        {
            case "grouped":
            {
                var key = _groupKey;
                // The key selector receives the item; the identity selector receives the
                // group KEY produced above, so it only has to stringify it.
                _source.GroupBy(
                    item => (object)GroupValue(item, key),
                    groupKey => groupKey?.ToString() ?? "(none)");

                _appliedMode = $"Grouped by {key}";
                break;
            }

            // case "hierarchy":
            // case "groupedhierarchy":
            //     Hierarchical rows are not available in this release. TableViewSource.idl
            //     exposes only Filter / GroupBy / Sort and their Clear* counterparts, so
            //     there is no hierarchy verb to call here yet and nothing is written rather
            //     than naming an API that does not exist. When the control ships hierarchy
            //     support, apply it to THIS same source instance alongside the GroupBy stage
            //     above so the two axes compose rather than replace one another, then remove
            //     IsEnabled="False" from the two hierarchy items in the Shaping mode selector.
            //     break;

            default:
                _source.ClearGroupBy();
                _appliedMode = "Flat";
                break;
        }

        if (ExpandAllButton is not null)
        {
            GroupKeySelector.IsEnabled = grouped;
            ExpandAllButton.IsEnabled = grouped;
            CollapseAllButton.IsEnabled = grouped;
        }

        if (grouped)
        {
            DispatcherQueue.TryEnqueue(() => PeopleTable?.ExpandAllGroups());
        }

        if (GroupedByValueText is not null)
        {
            GroupedByValueText.Text = _appliedMode;
        }
    }

    private static string GroupValue(object item, string key)
    {
        if (item is not Person person)
        {
            return "(none)";
        }

        var value = key switch
        {
            "Role" => person.Role,
            _ => person.Department,
        };

        return string.IsNullOrWhiteSpace(value) ? "(none)" : value;
    }

    private void OnDatePickerLoaded(object sender, RoutedEventArgs e)
    {
        if (sender is DatePicker datePicker)
        {
            datePicker.MinYear = new DateTimeOffset(new DateTime(2010, 1, 1));
            datePicker.MaxYear = new DateTimeOffset(DateTime.Now.AddYears(2));
        }
    }

    private void OnDepartmentComboBoxLoaded(object sender, RoutedEventArgs e)
    {
        if (sender is ComboBox comboBox)
        {
            comboBox.ItemsSource = PersonData.Departments;
        }
    }

    private void OnDetailsButtonClick(object sender, RoutedEventArgs e)
    {
        if (sender is not FrameworkElement element)
        {
            return;
        }

        // FlyoutBase.ShowAttachedFlyout alone is not reliable for a Flyout declared inside a
        // DataTemplate on WinUI 3 desktop: the template-created Flyout has no XamlRoot, and
        // showing it silently no-ops (the Click handler still runs, so the failure is invisible).
        // Framework-owned pickers - DatePicker, ComboBox - are unaffected because they set the
        // XamlRoot on their own flyouts. Resolve the flyout, give it the element's XamlRoot, and
        // ShowAt the element so it anchors to this row's cell rather than the page.
        var flyout = FlyoutBase.GetAttachedFlyout(element);
        if (flyout is null)
        {
            UpdateStatus("Details button has no attached flyout.");
            return;
        }

        flyout.XamlRoot = element.XamlRoot;
        flyout.ShowAt(element);
        UpdateStatus("Details button opened its attached flyout.");
    }

    private void RefreshReadout()
    {
        if (_watchedRow is null)
        {
            LiveReadout.Text = "(no rows)";
            return;
        }

        LiveReadout.Text =
            $"Name        : {_watchedRow.FullName}\n" +
            $"JoinDate    : {_watchedRow.JoinDate:yyyy-MM-dd}\n" +
            $"ShiftStart  : {_watchedRow.ShiftStart:hh\\:mm}\n" +
            $"Department  : {_watchedRow.Department}\n" +
            $"Active      : {_watchedRow.IsActive}\n" +
            $"Role        : {_watchedRow.Role}";
    }

    private void UpdateStatus(string message)
    {
        StatusText = $"{message} Templated controls are hit-testable and tab stops.";
    }

    // ---- actions -------------------------------------------------------------------

    private void OnTableSelectionChanged(TableView sender, SelectionChangedEventArgs args)
    {
        if (RecolourChipButton is not null)
        {
            var hasSelection = PeopleTable.SelectedItem is Person;
            RecolourChipButton.IsEnabled = hasSelection;
            ToggleSelectedActiveButton.IsEnabled = hasSelection;
        }
    }

    /// <summary>
    /// Rewrites Department on the model. The chip re-tints through its converter
    /// and, when grouped by Department, the row moves to another group.
    /// </summary>
    private void OnRecolourChipClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            return;
        }

        var pool = PersonData.Departments.ToList();
        var next = pool[(pool.IndexOf(person.Department) + 1) % pool.Count];
        person.Department = next;
        UpdateStatus($"{person.FullName} is now in {next}.");
    }

    private void OnToggleSelectedActiveClick(object sender, RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person person)
        {
            return;
        }

        person.IsActive = !person.IsActive;
    }

    private void OnCheckAllClick(object sender, RoutedEventArgs e) => SetAllActive(true);

    private void OnUncheckAllClick(object sender, RoutedEventArgs e) => SetAllActive(false);

    private void SetAllActive(bool isActive)
    {
        foreach (var person in People)
        {
            person.IsActive = isActive;
        }

        UpdateCheckedCount();
        UpdateStatus(isActive ? "Checked every row." : "Unchecked every row.");
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    private void OnPropertyChanged([CallerMemberName] string? name = null) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}

/// <summary>
/// Shared brush cache for the chip templates on this page.
///
/// Two things it deliberately gets right, because the nearby Showcase
/// converters get both wrong:
/// <list type="number">
///   <item><description>Brushes are built once into <c>static readonly</c>
///     dictionaries and handed back by reference. A converter that returns
///     <c>new SolidColorBrush(...)</c> allocates per realized cell per call,
///     which under virtualization is continuous churn.</description></item>
///   <item><description>Under a Windows Contrast theme the brand tints are
///     dropped entirely — the chip falls back to transparent plus the theme's
///     own stroke and text brushes — instead of painting a brand colour over
///     the user's guaranteed contrast pair.</description></item>
/// </list>
/// </summary>
internal static class ChipBrushes
{
    private const byte TintAlpha = 0x33;

    private static AccessibilitySettings? s_accessibilitySettings;
    private static Brush? s_highContrastForeground;

    private static readonly SolidColorBrush s_transparent = new(Colors.Transparent);

    private static readonly Dictionary<string, SolidColorBrush> s_departmentTints = BuildDepartmentBrushes(TintAlpha);
    private static readonly Dictionary<string, SolidColorBrush> s_departmentDots = BuildDepartmentBrushes(0xFF);

    private static readonly SolidColorBrush s_fallbackTint = new(ColorHelper.FromArgb(TintAlpha, 0x64, 0x74, 0x8B));
    private static readonly SolidColorBrush s_fallbackDot = new(ColorHelper.FromArgb(0xFF, 0x64, 0x74, 0x8B));

    private static readonly SolidColorBrush s_activeTint = new(ColorHelper.FromArgb(TintAlpha, 0x16, 0xA3, 0x4A));
    private static readonly SolidColorBrush s_activeDot = new(ColorHelper.FromArgb(0xFF, 0x16, 0xA3, 0x4A));
    private static readonly SolidColorBrush s_inactiveTint = new(ColorHelper.FromArgb(TintAlpha, 0x64, 0x74, 0x8B));
    private static readonly SolidColorBrush s_inactiveDot = new(ColorHelper.FromArgb(0xFF, 0x64, 0x74, 0x8B));

    /// <summary>
    /// True when Windows is running a Contrast theme. Read through a single
    /// cached <see cref="AccessibilitySettings"/>; the property itself is live,
    /// so a theme change is picked up without re-creating the object.
    /// </summary>
    internal static bool IsHighContrast
    {
        get
        {
            s_accessibilitySettings ??= new AccessibilitySettings();
            return s_accessibilitySettings.HighContrast;
        }
    }

    /// <summary>Theme-owned foreground used for the dot under a Contrast theme.</summary>
    private static Brush HighContrastForeground =>
        s_highContrastForeground ??=
            Application.Current.Resources["TextFillColorPrimaryBrush"] as Brush ?? s_transparent;

    internal static Brush DepartmentTint(string? department)
    {
        if (IsHighContrast)
        {
            return s_transparent;
        }

        return department is not null && s_departmentTints.TryGetValue(department, out var brush)
            ? brush
            : s_fallbackTint;
    }

    internal static Brush DepartmentDot(string? department)
    {
        if (IsHighContrast)
        {
            return HighContrastForeground;
        }

        return department is not null && s_departmentDots.TryGetValue(department, out var brush)
            ? brush
            : s_fallbackDot;
    }

    internal static Brush ActiveTint(bool isActive) =>
        IsHighContrast ? s_transparent : (isActive ? s_activeTint : s_inactiveTint);

    internal static Brush ActiveDot(bool isActive) =>
        IsHighContrast ? HighContrastForeground : (isActive ? s_activeDot : s_inactiveDot);

    private static Dictionary<string, SolidColorBrush> BuildDepartmentBrushes(byte alpha) => new(StringComparer.Ordinal)
    {
        ["Marketing"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0xA8, 0x55, 0xF7)),   // purple
        ["Product"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0x0E, 0xA5, 0xE9)),     // blue
        ["Finance"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0x22, 0xC5, 0x5E)),     // green
        ["Design"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0xEC, 0x48, 0x99)),      // pink
        ["Sales"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0x14, 0xB8, 0xA6)),       // teal
        ["HR"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0xF5, 0x9E, 0x0B)),          // amber
        ["Engineering"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0x63, 0x66, 0xF1)), // indigo
        ["Operations"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0xEF, 0x44, 0x44)),  // red
    };
}

/// <summary>Translucent chip background for the Department chip.</summary>
public sealed partial class DepartmentChipTintConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language) =>
        ChipBrushes.DepartmentTint(value as string);

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();
}

/// <summary>Solid accent dot for the Department chip.</summary>
public sealed partial class DepartmentChipDotConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language) =>
        ChipBrushes.DepartmentDot(value as string);

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();
}

/// <summary>Translucent chip background for the Active / Inactive status chip.</summary>
public sealed partial class ActiveChipTintConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language) =>
        ChipBrushes.ActiveTint(value is bool isActive && isActive);

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();
}

/// <summary>Solid accent dot for the Active / Inactive status chip.</summary>
public sealed partial class ActiveChipDotConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language) =>
        ChipBrushes.ActiveDot(value is bool isActive && isActive);

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();
}

/// <summary>
/// Text label for the status chip. The chip never conveys meaning by colour
/// alone — this label is both the visible and the accessible value.
/// </summary>
public sealed partial class ActiveChipTextConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language) =>
        value is bool isActive && isActive ? "Active" : "Inactive";

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();
}
