// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;

namespace TableViewSampleApp.Controls;

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

    // WinUI reports an unset AutomationId as the Name, so clear the x:Name (already assigned to the field).
    private void HideNameFromAutomation()
    {
        if (!string.IsNullOrEmpty(Name) && string.IsNullOrEmpty(AutomationProperties.GetAutomationId(this)))
        {
            Name = string.Empty;
        }
    }

    public string? Header
    {
        get => (string?)GetValue(HeaderProperty);
        set => SetValue(HeaderProperty, value);
    }

    public static readonly DependencyProperty HeaderProperty =
        DependencyProperty.Register(nameof(Header), typeof(string), typeof(RailSection),
            new PropertyMetadata(null, (d, e) => ((RailSection)d).OnHeaderChanged((string?)e.NewValue)));

    public string? Description
    {
        get => (string?)GetValue(DescriptionProperty);
        set => SetValue(DescriptionProperty, value);
    }

    public static readonly DependencyProperty DescriptionProperty =
        DependencyProperty.Register(nameof(Description), typeof(string), typeof(RailSection),
            new PropertyMetadata(null, (d, e) => ((RailSection)d).OnDescriptionChanged((string?)e.NewValue)));

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
