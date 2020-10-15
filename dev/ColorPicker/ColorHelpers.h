// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CppWinRTHelpers.h"
#include "DispatcherHelper.h"
#include "ColorConversion.h"

enum class IncrementDirection
{
    Lower,
    Higher,
};

enum class IncrementAmount
{
    Small,
    Large,
};

Hsv IncrementColorChannel(
    const Hsv &originalHsv,
    winrt::ColorPickerHsvChannel channel,
    IncrementDirection direction,
    IncrementAmount amount,
    bool shouldWrap,
    double minBound,
    double maxBound);

template <typename T>
int sgn(T val);

Hsv FindNextNamedColor(
    const Hsv& originalHsv,
    winrt::ColorPickerHsvChannel channel,
    IncrementDirection direction,
    bool shouldWrap,
    double minBound,
    double maxBound);

double IncrementAlphaChannel(
    double originalAlpha,
    IncrementDirection direction,
    IncrementAmount amount,
    bool shouldWrap,
    double minBound,
    double maxBound);

void CreateCheckeredBackgroundAsync(
    int width,
    int height,
    winrt::Color checkerColor,
    std::shared_ptr<std::vector<byte>> const& bgraCheckeredPixelData,
    winrt::IAsyncAction &asyncActionToAssign,
    DispatcherHelper dispatcherHelper,
    std::function<void(winrt::WriteableBitmap)> completedFunction);

winrt::WriteableBitmap CreateBitmapFromPixelData(
    int pixelWidth,
    int pixelHeight,
    std::shared_ptr<std::vector<byte>> const& bgraPixelData);

winrt::LoadedImageSurface CreateSurfaceFromPixelData(
    int pixelWidth,
    int pixelHeight,
    std::shared_ptr<std::vector<byte>> const& bgraPixelData);

void CancelAsyncAction(winrt::IAsyncAction const& action);