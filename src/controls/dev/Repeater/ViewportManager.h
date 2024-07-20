#pragma once

class ViewportManager
{
public:
    virtual winrt::UIElement SuggestedAnchor() const = 0;

    virtual double HorizontalCacheLength() const = 0;
    virtual void HorizontalCacheLength(double value) = 0;

    virtual double VerticalCacheLength() const = 0;
    virtual void VerticalCacheLength(double value) = 0;

    virtual winrt::Rect GetLayoutVisibleWindow() const = 0;
    virtual winrt::Rect GetLayoutRealizationWindow() const = 0;

    virtual void SetLayoutExtent(winrt::Rect extent) = 0;
    virtual winrt::Rect GetLayoutExtent() const = 0;
    virtual winrt::Point GetOrigin() const = 0;

    virtual void OnLayoutChanged(bool isVirtualizing) = 0;
    virtual void OnElementPrepared(const winrt::UIElement& element) = 0;
    virtual void OnElementCleared(const winrt::UIElement& element) = 0;
    virtual void OnOwnerMeasuring() = 0;
    virtual void OnOwnerArranged() = 0;
    virtual void OnMakeAnchor(const winrt::UIElement& anchor, const bool isAnchorOutsideRealizedRange) = 0;
    virtual void OnBringIntoViewRequested(const winrt::BringIntoViewRequestedEventArgs args) = 0;

    virtual void ResetScrollers() = 0;

    virtual winrt::UIElement MadeAnchor() const = 0;
};
