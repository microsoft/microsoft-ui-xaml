// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;

namespace TableViewSampleApp.Controls;

/// <summary>
/// One section of a sample page's options rail: a Level 2 heading, an optional description, then
/// the page's own children (buttons, selectors). A plain StackPanel with no template, so the UIA
/// tree is exactly the inline pattern it replaces (FIX-PLAN §1.2): a named group, the heading,
/// the description, the controls. Children declared in XAML stay in the page's namescope, so
/// their x:Name fields and Click handlers stay in the page.
/// </summary>
public partial class RailSection : StackPanel
{
    private readonly TextBlock _heading;
    private TextBlock? _description;
    private bool _nameFromHeader;

    public RailSection()
    {
        if (Application.Current.Resources.TryGetValue("RailSectionStyle", out var style))
        {
            Style = (Style)style;
        }

        _heading = new TextBlock();
        AutomationProperties.SetHeadingLevel(_heading, AutomationHeadingLevel.Level2);
        _heading.Style = (Style)Application.Current.Resources["SectionHeaderTextStyle"];
        ApplyHeadingMargin();
        Children.Add(_heading);
        Loading += (_, _) => HideNameFromAutomation();
    }

    // WinUI reports an unset AutomationId as the element's Name, so a page's x:Name (needed to
    // reach the section from code, e.g. x:Name="Status") would add an AutomationId the inline
    // section never had. The x:Name field is already assigned by InitializeComponent, so the
    // Name itself is no longer needed; FindName on it will not work.
    private void HideNameFromAutomation()
    {
        if (!string.IsNullOrEmpty(Name) && string.IsNullOrEmpty(AutomationProperties.GetAutomationId(this)))
        {
            Name = string.Empty;
        }
    }

    /// <summary>Section heading. Also the section's accessible name unless one is set explicitly.</summary>
    public string? Header
    {
        get => (string?)GetValue(HeaderProperty);
        set => SetValue(HeaderProperty, value);
    }

    public static readonly DependencyProperty HeaderProperty =
        DependencyProperty.Register(nameof(Header), typeof(string), typeof(RailSection),
            new PropertyMetadata(null, (d, e) => ((RailSection)d).OnHeaderChanged((string?)e.NewValue)));

    /// <summary>One or two sentences under the heading. No TextBlock is created while empty.</summary>
    public string? Description
    {
        get => (string?)GetValue(DescriptionProperty);
        set => SetValue(DescriptionProperty, value);
    }

    public static readonly DependencyProperty DescriptionProperty =
        DependencyProperty.Register(nameof(Description), typeof(string), typeof(RailSection),
            new PropertyMetadata(null, (d, e) => ((RailSection)d).OnDescriptionChanged((string?)e.NewValue)));

    /// <summary>
    /// True for the first section in the rail: its heading keeps the style's top margin instead of
    /// the 8 px that separates later sections.
    /// </summary>
    public bool IsFirst
    {
        get => (bool)GetValue(IsFirstProperty);
        set => SetValue(IsFirstProperty, value);
    }

    public static readonly DependencyProperty IsFirstProperty =
        DependencyProperty.Register(nameof(IsFirst), typeof(bool), typeof(RailSection),
            new PropertyMetadata(false, (d, e) => ((RailSection)d).ApplyHeadingMargin()));

    private void OnHeaderChanged(string? header)
    {
        _heading.Text = header ?? string.Empty;
        var current = AutomationProperties.GetName(this);
        if (string.IsNullOrEmpty(current) || _nameFromHeader)
        {
            AutomationProperties.SetName(this, header ?? string.Empty);
            _nameFromHeader = true;
        }
    }

    private void OnDescriptionChanged(string? description)
    {
        if (string.IsNullOrEmpty(description))
        {
            if (_description is not null)
            {
                Children.Remove(_description);
                _description = null;
            }

            return;
        }

        if (_description is null)
        {
            _description = new TextBlock
            {
                Style = (Style)Application.Current.Resources["SectionDescriptionTextStyle"],
                TextWrapping = TextWrapping.Wrap,
            };
            Children.Insert(Children.IndexOf(_heading) + 1, _description);
        }

        _description.Text = description;
    }

    private void ApplyHeadingMargin()
    {
        if (IsFirst)
        {
            _heading.ClearValue(MarginProperty);
        }
        else
        {
            _heading.Margin = new Thickness(0, 8, 0, 0);
        }
    }
}
