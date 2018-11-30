// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.Foundation;
using Windows.UI.Xaml.Controls;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common
{
    public class OrientationBasedMeasures
    {
        public ScrollOrientation ScrollOrientation { get; set; }

        public bool IsVerical
        {
            get { return ScrollOrientation == ScrollOrientation.Vertical; }
        }

        public OrientationBasedMeasures(ScrollOrientation o)
        {
            ScrollOrientation = o;
        }

        public double Major(Size size)
        {
            return IsVerical ? size.Height : size.Width;
        }

        public double Minor(Size size)
        {
            return IsVerical ? size.Width : size.Height;
        }

        public double MajorSize(Rect rect)
        {
            return IsVerical ? rect.Height : rect.Width;
        }

        public double MinorSize(Rect rect)
        {
            return IsVerical ? rect.Width : rect.Height;
        }

        public double MajorStart(Rect rect)
        {
            return IsVerical ? rect.Top : rect.Left;
        }

        public double MajorEnd(Rect rect)
        {
            return IsVerical ? rect.Bottom : rect.Right;
        }

        public double MinorStart(Rect rect)
        {
            return IsVerical ? rect.Left : rect.Top;
        }

        public void SetMajorSize(ref Rect rect, double value)
        {
            if (IsVerical)
            {
                rect.Height = value;
            }
            else
            {
                rect.Width = value;
            }
        }

        public void SetMajorStart(ref Rect rect, double value)
        {
            if (IsVerical)
            {
                rect.Y = value;
            }
            else
            {
                rect.X = value;
            }
        }

        public void SetMinorStart(ref Rect rect, double value)
        {
            if (IsVerical)
            {
                rect.X = value;
            }
            else
            {
                rect.Y = value;
            }
        }

        public Rect MinorMajorRect(double minor, double major, double minorSize, double majorSize)
        {
            return
                IsVerical ?
                new Rect(minor, major, minorSize, majorSize) :
                new Rect(major, minor, majorSize, minorSize);
        }

        public Point MinorMajorPoint(double minor, double major)
        {
            return
                IsVerical ?
                new Point(minor, major) :
                new Point(major, minor);
        }

        public Size MinorMajorSize(double minor, double major)
        {
            return
                IsVerical ?
                new Size(minor, major) :
                new Size(major, minor);
        }
    }
}
