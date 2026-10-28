// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.ComponentModel;
using System.Globalization;
using Microsoft.Windows.ApplicationModel.Resources;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.App.ViewModels;

// A UI element in the Elements tab. While a session records, an element's event count and last time change, so a row is
// updated in place.
public sealed class ElementRow(SessionObject element) : INotifyPropertyChanged
{
    private static readonly ResourceLoader Strings = new();

    public event PropertyChangedEventHandler? PropertyChanged;

    public SessionObject Element { get; private set; } = element;
    public string Type => ShortType(Element.Type) ?? Strings.GetString("ElementTypeNotSeen");
    public string FullType => Element.Type ?? Strings.GetString("ElementTypeNotSeen");
    public string Name => Element.Name ?? "";
    public string Address => $"0x{Element.Address:X}";
    public string First => Element.FirstMilliseconds.ToString("N1", CultureInfo.CurrentCulture);
    public string Last => Element.LastMilliseconds.ToString("N1", CultureInfo.CurrentCulture);
    public string Events => Element.Events.ToString("N0", CultureInfo.CurrentCulture);

    // The element's type without its namespace, and its name when it has one.
    public static string Describe(string? type, string? name) =>
        (ShortType(type) ?? Strings.GetString("ElementTypeNotSeen")) + (string.IsNullOrEmpty(name) ? "" : $" \"{name}\"");

    public void Update(SessionObject element)
    {
        Element = element;
        PropertyChanged?.Invoke(this, new(string.Empty));
    }

    // Whether the full type, name or address contains the text.
    public bool Matches(string text) =>
        Element.Type?.Contains(text, StringComparison.OrdinalIgnoreCase) == true ||
        Element.Name?.Contains(text, StringComparison.OrdinalIgnoreCase) == true ||
        Address.Contains(text, StringComparison.OrdinalIgnoreCase);

    public override string ToString() =>
        $"{Describe(Element.Type, Element.Name)} | {Address} | first {First} ms | last {Last} ms | {Events} events";

    private static string? ShortType(string? type) => type?[(type.LastIndexOf('.') + 1)..];
}
