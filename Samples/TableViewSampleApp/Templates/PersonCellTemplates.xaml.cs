// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Globalization;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Templates;

/// <summary>
/// Code-behind for the shared Person cell templates. A code-behind ResourceDictionary is what
/// allows {x:Bind} and event handlers inside the templates. Merged once in App.xaml.
/// </summary>
public sealed partial class PersonCellTemplates : ResourceDictionary
{
    public PersonCellTemplates()
    {
        InitializeComponent();
    }

    /// <summary>Earliest selectable join date for the date editors (1 Jan 2010).</summary>
    public static DateTimeOffset MinJoinYear { get; } = new(new DateTime(2010, 1, 1));

    /// <summary>Latest selectable join date for the date editors (two years from launch).</summary>
    public static DateTimeOffset MaxJoinYear { get; } = new(DateTime.Today.AddYears(2));

    /// <summary>
    /// Raised after a Details button (DetailsButtonTemplate, or <see cref="TryOpenDetails"/>)
    /// opened its flyout. Static: subscribe in the page's Loaded and unsubscribe in Unloaded,
    /// or the page leaks.
    /// </summary>
    public static event EventHandler<Person>? DetailsOpened;

    /// <summary>"Engineering · Seattle" for the details flyout.</summary>
    public static string DepartmentAndOffice(string department, string office) =>
        string.IsNullOrEmpty(office) ? department : string.Format(CultureInfo.CurrentCulture, "{0} · {1}", department, office);

    /// <summary>"Joined 12 March 2021" in the current culture.</summary>
    public static string JoinedText(DateTimeOffset joinDate) =>
        string.Format(CultureInfo.CurrentCulture, "Joined {0:D}", joinDate);

    private void OnDetailsButtonClick(object sender, RoutedEventArgs e)
    {
        if (sender is FrameworkElement element)
        {
            ShowAttached(element);
        }
    }

    /// <summary>
    /// Opens the Details flyout of <paramref name="person"/>'s realized row in
    /// <paramref name="table"/>. Returns false when that row is not realized (scrolled away),
    /// so the caller can say so in its Last action readout.
    /// </summary>
    public static bool TryOpenDetails(DependencyObject table, Person person)
    {
        var button = FindDetailsButton(table, person);
        return button is not null && ShowAttached(button);
    }

    private static bool ShowAttached(FrameworkElement element)
    {
        // FlyoutBase.ShowAttachedFlyout alone is not reliable for a Flyout declared inside a
        // DataTemplate on WinUI 3 desktop: the template-created Flyout has no XamlRoot, so it
        // silently does not open. Give it the element's XamlRoot and ShowAt the element so it
        // anchors to this row's cell.
        var flyout = FlyoutBase.GetAttachedFlyout(element);
        if (flyout is null || element.XamlRoot is null)
        {
            return false;
        }

        flyout.XamlRoot = element.XamlRoot;
        flyout.ShowAt(element);
        if (element.DataContext is Person person)
        {
            DetailsOpened?.Invoke(null, person);
        }

        return true;
    }

    private static Button? FindDetailsButton(DependencyObject root, Person person)
    {
        var count = VisualTreeHelper.GetChildrenCount(root);
        for (var i = 0; i < count; i++)
        {
            var child = VisualTreeHelper.GetChild(root, i);
            if (child is Button button
                && ReferenceEquals(button.DataContext, person)
                && FlyoutBase.GetAttachedFlyout(button) is not null)
            {
                return button;
            }

            if (FindDetailsButton(child, person) is { } found)
            {
                return found;
            }
        }

        return null;
    }
}
