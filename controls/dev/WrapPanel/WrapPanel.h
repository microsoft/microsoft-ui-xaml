// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "WrapPanel.g.h"
#include "WrapPanel.properties.h"

class WrapPanel : 
    public ReferenceTracker<WrapPanel, winrt::implementation::WrapPanelT>,
    public WrapPanelProperties
{
public:

    void OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);

    winrt::Size MeasureOverride(winrt::Size const& availableSize);
    winrt::Size ArrangeOverride(winrt::Size const& finalSize);

private:

    winrt::Size UpdateRows(winrt::Size availableSize);

    struct UvMeasure
    {
        double U = 0.0;
        double V = 0.0;

        UvMeasure() = default;
        UvMeasure(double u, double v) : U(u), V(v) {}

        UvMeasure(const winrt::Microsoft::UI::Xaml::Controls::Orientation orientation, winrt::Windows::Foundation::Size const& size)
        {
            if (orientation == winrt::Microsoft::UI::Xaml::Controls::Orientation::Horizontal)
            {
                U = size.Width;
                V = size.Height;
            }
            else
            {
                U = size.Height;
                V = size.Width;
            }
        }

        UvMeasure(winrt::Microsoft::UI::Xaml::Controls::Orientation orientation, double width, double height)
        {
            if (orientation == winrt::Microsoft::UI::Xaml::Controls::Orientation::Horizontal)
            {
                U = width;
                V = height;
            }
            else
            {
                U = height;
                V = width;
            }
        }

        UvMeasure Add(double u, double v) const
        {
            return UvMeasure(U + u, V + v);
        }

        UvMeasure Add(UvMeasure const& measure) const
        {
            return Add(measure.U, measure.V);
        }

        winrt::Windows::Foundation::Size ToSize(winrt::Microsoft::UI::Xaml::Controls::Orientation orientation) const
        {
            if (orientation == winrt::Microsoft::UI::Xaml::Controls::Orientation::Horizontal)
            {
                return winrt::Windows::Foundation::Size{ (float)U, (float)V };
            }
            else
            {
                return winrt::Windows::Foundation::Size{ (float)V, (float)U };
            }
        }
    };

    struct UvRect
    {
        UvMeasure Position;
        UvMeasure Size;

        winrt::Windows::Foundation::Rect ToRect(winrt::Microsoft::UI::Xaml::Controls::Orientation orientation) const
        {
            if (orientation == winrt::Microsoft::UI::Xaml::Controls::Orientation::Vertical)
            {
                return winrt::Windows::Foundation::Rect{ (float)Position.V, (float)Position.U, (float)Size.V, (float)Size.U };
            }
            else if (orientation == winrt::Microsoft::UI::Xaml::Controls::Orientation::Horizontal)
            {
                return winrt::Windows::Foundation::Rect{ (float)Position.U, (float)Position.V, (float)Size.U, (float)Size.V };
            }
            else
            {
                throw std::invalid_argument("The input orientation is not valid.");
            }
        }
    };

    struct Row
    {
        std::vector<UvRect> ChildrenRects;
        UvMeasure Size;

        Row() = default;
        Row(std::vector<UvRect> const& childrenRects, UvMeasure size) : ChildrenRects(childrenRects), Size(size) {}

        UvRect Rect() const
        {
            if (!ChildrenRects.empty())
            {
                return UvRect{ ChildrenRects.front().Position, Size };
            }
            else
            {
                return UvRect{ UvMeasure(), Size };
            }
        }

        void Add(UvMeasure const& position, UvMeasure const& size)
        {
            UvRect rect{ position, size };
            ChildrenRects.push_back(rect);
            double newU = position.U + size.U;
            double newV = std::max(Size.V, size.V);
            Size = UvMeasure(newU, newV);
        }
    };

    std::vector<Row> m_rows;
};
