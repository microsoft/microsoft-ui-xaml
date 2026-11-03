// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.ComponentModel;
using System.Globalization;
using Microsoft.UI.Xaml;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.App.ViewModels;

// A UI-thread tick in the Frames tab. While a session records, its latest ticks still change, so a row is updated in place.
public sealed class FrameRow(SessionFrame frame) : INotifyPropertyChanged
{
    // A tick this long fills the bar.
    private const double FullBarMilliseconds = 50;
    private const double BarWidthPixels = 72;
    // At 60 frames a second, a frame has 16.7 ms.
    private const double FrameBudgetMilliseconds = 1000.0 / 60;

    public event PropertyChangedEventHandler? PropertyChanged;

    public SessionFrame Frame { get; private set; } = frame;
    // The region of interest that ends with this tick, if any.
    public InterestingRowItem? Region { get; set; }
    public bool IsSlow => Frame.TickMilliseconds > FrameBudgetMilliseconds;
    public string Number => Frame.FrameNumber.ToString("N0", CultureInfo.CurrentCulture);
    public string Start => Milliseconds(Frame.StartMilliseconds);
    public string End => Milliseconds(Frame.EndMilliseconds);
    public string Tick => Milliseconds(Frame.TickMilliseconds);
    public double BarWidth => Math.Clamp((Frame.TickMilliseconds ?? 0) / FullBarMilliseconds, 0, 1) * BarWidthPixels;
    public Visibility SlowVisibility => IsSlow ? Visibility.Visible : Visibility.Collapsed;
    public Visibility DrewVisibility => Frame.Drawn == true ? Visibility.Visible : Visibility.Collapsed;
    public string Xaml => Frame.Xaml.ToString("N0", CultureInfo.CurrentCulture);
    public string Composition => Frame.Composition.ToString("N0", CultureInfo.CurrentCulture);
    public string Other => Frame.Other.ToString("N0", CultureInfo.CurrentCulture);

    public void Update(SessionFrame frame)
    {
        Frame = frame;
        PropertyChanged?.Invoke(this, new(string.Empty));
    }

    public override string ToString() =>
        $"Frame {Number} | starts at {Start} ms | tick {Tick} ms | {Frame.Drawn switch { true => "drew", false => "did not draw", null => "latest tick" }} | " +
        $"XAML {Xaml} | Composition {Composition} | Other {Other}";

    private static string Milliseconds(double? value) => value?.ToString("N1", CultureInfo.CurrentCulture) ?? "-";
}
