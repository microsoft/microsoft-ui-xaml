// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.ComponentModel;
using System.Globalization;
using Microsoft.UI.Xaml;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.App.ViewModels;

// A frame in the Frames tab. While a session records, its latest frames still change, so a row is updated in place.
public sealed class FrameRow(SessionFrame frame) : INotifyPropertyChanged
{
    // The drawing tick's bar is full at this length; at 60 frames a second a frame has 16.7 ms.
    private const double FullBarMilliseconds = 50;
    private const double BarWidthPixels = 56;
    private const double FrameBudgetMilliseconds = 1000.0 / 60;

    public event PropertyChangedEventHandler? PropertyChanged;

    public SessionFrame Frame { get; private set; } = frame;
    public double? DrawMilliseconds => Frame.EndMilliseconds - Frame.LastTickStartMilliseconds;
    public string Number => Frame.FrameNumber.ToString("N0", CultureInfo.CurrentCulture) + (Frame.Drawn ? "" : "*");
    // When the frame's last tick ended: when it was drawn, or for a thread's latest frame, its latest tick so far.
    public string DrawnAt => Milliseconds(Frame.EndMilliseconds);
    public string Draw => Milliseconds(DrawMilliseconds);
    public double BarWidth => Math.Clamp((DrawMilliseconds ?? 0) / FullBarMilliseconds, 0, 1) * BarWidthPixels;
    public Visibility SlowVisibility => DrawMilliseconds > FrameBudgetMilliseconds ? Visibility.Visible : Visibility.Collapsed;
    public string Xaml => Frame.Xaml.ToString("N0", CultureInfo.CurrentCulture);
    public string Composition => Frame.Composition.ToString("N0", CultureInfo.CurrentCulture);
    public string Other => Frame.Other.ToString("N0", CultureInfo.CurrentCulture);

    public void Update(SessionFrame frame)
    {
        Frame = frame;
        PropertyChanged?.Invoke(this, new(string.Empty));
    }

    public override string ToString() => $"Frame {Number} | drawn at {DrawnAt} ms | draw {Draw} ms | XAML {Xaml} | Composition {Composition} | Other {Other}";

    private static string Milliseconds(double? value) => value?.ToString("N1", CultureInfo.CurrentCulture) ?? "-";
}
