// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Globalization;
using Microsoft.UI.Text;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Converters;

// The binding has no Path, so the whole row item arrives here.
public sealed partial class RowCardConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
    {
        if (value is not Person person)
        {
            return null!;
        }

        // One element cannot have two parents: build a fresh element per evaluation, never cache it.
        var panel = new StackPanel { Spacing = 4 };
        panel.Children.Add(new TextBlock { Text = person.FullName, FontWeight = FontWeights.SemiBold });
        panel.Children.Add(new TextBlock { Text = string.Format(CultureInfo.CurrentCulture, "{0}, {1}", person.Role, person.Department), Opacity = 0.75 });
        panel.Children.Add(new TextBlock { Text = person.Email, Opacity = 0.6, FontSize = 12 });

        // Rich content has no text of its own for UI Automation, so name the card explicitly.
        AutomationProperties.SetName(panel, string.Format(CultureInfo.CurrentCulture, "{0}, {1}, {2}, {3}", person.FullName, person.Role, person.Department, person.Email));

        return panel;
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotSupportedException();
}
