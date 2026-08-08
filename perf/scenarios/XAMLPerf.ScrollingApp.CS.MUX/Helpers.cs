using Microsoft.UI.Xaml;
using System.Collections.Generic;
using System.Numerics;
using Windows.Foundation;
using Windows.Graphics.Display;

namespace Interactions
{
    public static class Geometry
    {
        public static Point Add(this Point _this, Point other) => new Point(_this.X + other.X, _this.Y + other.Y);
        public static Point Add(this Point _this, Vector2 other) => new Point(_this.X + other.X, _this.Y + other.Y);
        public static Point Sub(this Point _this, Point other) => new Point(_this.X - other.X, _this.Y - other.Y);
        public static Point Sub(this Point _this, Vector2 other) => new Point(_this.X - other.X, _this.Y - other.Y);
        public static Point Mul(this Point _this, double scalar) => new Point(_this.X * scalar, _this.Y * scalar);
        public static Vector2 Mul(this Vector2 _this, float scalar) => new Vector2(_this.X * scalar, _this.Y * scalar);
        public static Point Normalize(this Point _this, Size size) => new Point(_this.X / size.Width, _this.Y / size.Height);
        public static Vector2 Normalize(this Vector2 _this) => Mul(_this, 1.0f / _this.Length());
    }

    public static class Coord
    {
        public static Size ScreenInDips()
        {
            var displayInformation = Windows.Graphics.Display.DisplayInformation.GetForCurrentView();

            return new Size(
                displayInformation.ScreenWidthInRawPixels / displayInformation.RawPixelsPerViewPixel,
                displayInformation.ScreenHeightInRawPixels / displayInformation.RawPixelsPerViewPixel);
        }

        public static Point DipsToRaw(Point pointInDips)
        {
            return pointInDips.Mul(DisplayInformation.GetForCurrentView().RawPixelsPerViewPixel);
        }

        public static Point AppToScreen(Point pointInApp)
        {
            Rect appBounds = Windows.UI.ViewManagement.ApplicationView.GetForCurrentView().VisibleBounds;
            return new Point(appBounds.Left, appBounds.Top).Add(pointInApp);
        }

        public static Point ElementToScreen(this UIElement element, Point pointInElement)
        {
            return AppToScreen(element.TransformToVisual(element.XamlRoot.Content).TransformPoint(pointInElement));
        }

        public static Point ActualSize(this UIElement element)
        {
            return new Point(element.ActualSize.X, element.ActualSize.Y);
        }

        public static Point Top(this Point _this) => new Point(_this.X, 0.0);

        public static Point VCenter(this Point _this) => new Point(_this.X, _this.Y * 0.5);

        public static Point Bottom(this Point _this) => new Point(_this.X, _this.Y);

        public static Point Left(this Point _this) => new Point(0.0, _this.Y);

        public static Point HCenter(this Point _this) => new Point(_this.X * 0.5, _this.Y);

        public static Point Right(this Point _this) => new Point(_this.X, _this.Y);

        public static Point HOffset(this Point _this, double offset) => new Point(_this.X + offset, _this.Y);

        public static Point VOffset(this Point _this, double offset) => new Point(_this.X, _this.Y + offset);

        public static Point MouseScreenToNormalized(Point screenInDips)
        {
            return screenInDips.Normalize(ScreenInDips()).Mul(65536);
        }
    }

    public static class Motion
    {
        public static List<PointT> GenerateLinear(Point startPoint, Vector2 velocityUnitsPerSec, uint durationMs)
        {
            Vector2 delta = velocityUnitsPerSec.Mul(0.001f * durationMs);
            return GenerateLinear(startPoint, startPoint.Add(delta), 0.001 * delta.Length() / durationMs);
        }

        public static List<PointT> GenerateLinear(Point startPoint, Point endPoint, double speedUnitsPerSec)
        {
            var result = new List<PointT>();

            if (startPoint == endPoint)
            {
                return result;
            }

            Vector2 delta = endPoint.Sub(startPoint).ToVector2();
            uint timeInMSec = (uint)(1000.0f * delta.Length() / speedUnitsPerSec);
            Vector2 direction = delta.Normalize().Mul(0.001f * (float)speedUnitsPerSec);

            for (uint i = 0; i < timeInMSec; i += stepMs)
            {
                result.Add(new PointT { Position = startPoint.Add(direction.Mul(i)), TimeDeltaMs = stepMs });
            }

            result.Add(new PointT { Position = endPoint, TimeDeltaMs = timeInMSec % stepMs });

            return result;
        }

        private const uint stepMs = 100;
    }
}
