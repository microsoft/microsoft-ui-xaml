// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.ComponentModel;
using System.Globalization;
using Microsoft.Windows.ApplicationModel.Resources;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.App.ViewModels;

// A UI element or visual in the Objects tab. While a session records, an object's event count and last time change, so a
// row is updated in place.
public sealed class ObjectRow(SessionObject item) : INotifyPropertyChanged
{
    private static readonly ResourceLoader Strings = new();

    public event PropertyChangedEventHandler? PropertyChanged;

    public SessionObject Item { get; private set; } = item;
    public bool IsElement => Item.Kind == ObjectKind.Element;
    public bool IsVisual => Item.Kind == ObjectKind.Visual;
    public string KindText => KindName(Item.Kind);
    public string Type => ShortType(Item.Type) ?? Strings.GetString("ElementTypeNotSeen");
    public string FullType => Item.Type ?? Strings.GetString("ElementTypeNotSeen");
    public string Name => NameText(Item.Kind, Item.Name);
    public string Address => $"0x{Item.Address:X}";
    public string First => Item.FirstMilliseconds.ToString("N1", CultureInfo.CurrentCulture);
    public string Last => Item.LastMilliseconds.ToString("N1", CultureInfo.CurrentCulture);
    public string Events => Item.Events.ToString("N0", CultureInfo.CurrentCulture);

    // The type without its namespace, and an element's name or the label of the element that owns a visual, when known.
    public static string Describe(ObjectKind kind, string? type, string? name) =>
        (ShortType(type) ?? Strings.GetString("ElementTypeNotSeen")) + (string.IsNullOrEmpty(name) ? "" : " " + NameText(kind, name));

    public void Update(SessionObject item)
    {
        Item = item;
        PropertyChanged?.Invoke(this, new(string.Empty));
    }

    // Whether the full type, name or address contains the text.
    public bool Matches(string text) =>
        Item.Type?.Contains(text, StringComparison.OrdinalIgnoreCase) == true ||
        Item.Name?.Contains(text, StringComparison.OrdinalIgnoreCase) == true ||
        Address.Contains(text, StringComparison.OrdinalIgnoreCase);

    // "Element" or "Visual".
    public static string KindName(ObjectKind kind) => Strings.GetString(kind == ObjectKind.Visual ? "VisualKind" : "ElementKind");

    public override string ToString() =>
        $"{KindText} | {Describe(Item.Kind, Item.Type, Item.Name)} | {Address} | first {First} ms | last {Last} ms | {Events} events";

    private static string NameText(ObjectKind kind, string? name) => string.IsNullOrEmpty(name) ? ""
        : kind == ObjectKind.Visual ? string.Format(CultureInfo.CurrentCulture, Strings.GetString("VisualOwnerFormat"), name)
        : $"\"{name}\"";

    private static string? ShortType(string? type) => type?[(type.LastIndexOf('.') + 1)..];
}
