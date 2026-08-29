// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "AnimationTestHelper.h"

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Controls;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

DoubleAnimation^ AnimationTestHelper::MakeDoubleAnimation(Canvas^ rootCanvas, String^ targetName, String^ targetProperty, ULONGLONG duration, double from, double to)
{
    auto target = safe_cast<DependencyObject^>(rootCanvas->FindName(targetName));

    auto da = ref new DoubleAnimation();
    da->From = from;
    da->To = to;
    TimeSpan span;
    span.Duration = duration;
    da->Duration = DurationHelper::FromTimeSpan(span);
    Storyboard::SetTarget(da, target);
    Storyboard::SetTargetProperty(da, targetProperty);

    return da;
}

DoubleAnimation^ AnimationTestHelper::MakeDoubleAnimation(Canvas^ rootCanvas, String^ targetName, ULONGLONG duration, double from, double to)
{
    return MakeDoubleAnimation(rootCanvas, targetName, L"X", duration, from, to);
}

DoubleAnimation^ AnimationTestHelper::MakeDoubleAnimation(DependencyObject^ target, String^ targetProperty)
{
    auto da = ref new DoubleAnimation();
    da->From = 0.0;
    da->To = 100.0;
    TimeSpan span; span.Duration = 10000000L;
    da->Duration = DurationHelper::FromTimeSpan(span);
    Storyboard::SetTarget(da, target);
    Storyboard::SetTargetProperty(da, targetProperty);

    return da;
}

ColorAnimation^ AnimationTestHelper::MakeColorAnimation(Canvas^ rootCanvas, String^ targetName, ULONGLONG duration, unsigned int fromArgb, unsigned int toArgb)
{
    auto target = safe_cast<SolidColorBrush^>(rootCanvas->FindName(targetName));

    auto da = ref new ColorAnimation();
    da->From = mu::ColorHelper::FromArgb(
        (fromArgb >> 24) & 0xff,
        (fromArgb >> 16) & 0xff,
        (fromArgb >> 8) & 0xff,
        (fromArgb >> 0) & 0xff);
    da->To = mu::ColorHelper::FromArgb(
        (toArgb >> 24) & 0xff,
        (toArgb >> 16) & 0xff,
        (toArgb >> 8) & 0xff,
        (toArgb >> 0) & 0xff);
    TimeSpan span;
    span.Duration = duration;
    da->Duration = DurationHelper::FromTimeSpan(span);
    Storyboard::SetTarget(da, target);
    Storyboard::SetTargetProperty(da, L"Color");

    return da;
}

Storyboard^ AnimationTestHelper::MakeStoryboard(Timeline^ timeline)
{
    auto sb = ref new Storyboard();
    sb->Children->Append(timeline);
    return sb;
}

Storyboard^ AnimationTestHelper::MakeStoryboard(Timeline^ timeline, ULONGLONG duration)
{
    auto sb = ref new Storyboard();
    TimeSpan span; span.Duration = duration; sb->Duration = DurationHelper::FromTimeSpan(span);
    sb->Children->Append(timeline);
    return sb;
}

Storyboard^ AnimationTestHelper::MakeBlankStoryboard(ULONGLONG duration)
{
    auto sb = ref new Storyboard();
    TimeSpan span; span.Duration = duration; sb->Duration = DurationHelper::FromTimeSpan(span);
    return sb;
}

DoubleAnimation^ AnimationTestHelper::MakeOpacityAnimation(Canvas^ target)
{
    DoubleAnimation^ da = ref new DoubleAnimation();
    da->From = 1.0;
    da->To = 1.0;
    TimeSpan span; span.Duration = 10000000L; da->Duration = DurationHelper::FromTimeSpan(span);
    Storyboard::SetTarget(da, target);
    Storyboard::SetTargetProperty(da, L"Opacity");
    return da;
}

} } } } } }
