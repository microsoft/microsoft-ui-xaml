// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Windows.Foundation;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Controls.Primitives;
using WEX.TestExecution;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.Common
{
    public static class VisualTreeUtils
    {
        public static UIElement GetRoot(this UIElement element)
        {
            if (element == null)
                return null;

            var parent = VisualTreeHelper.GetParent(element) as UIElement;
            if (parent == null)
            {
                return element;
            }

            return parent.GetRoot();
        }

        public static T GetParentOfType<T>(this DependencyObject element)
            where T : DependencyObject
        {
            if(element == null)
            {
                return null;
            }

            var parent = VisualTreeHelper.GetParent(element);

            return parent as T ?? parent.GetParentOfType<T>();
        }

        public static Point GetCenterPoint(this FrameworkElement element)
        {
            return element.GetElementPoint(0.5f, 0.5f);
        }

        public static Point GetElementPoint(this FrameworkElement element, float fractionOfWidth, float fractionOfHeight)
        {
            Point point = new Point();
            Verify.IsNotNull(element, "Element cannot be null.");
            UIExecutor.Execute(() => {

                double height = element.ActualHeight;
                double width = element.ActualWidth;

                Verify.AreNotEqual(height, 0.0, "Element has no height.");
                Verify.AreNotEqual(width, 0.0, "Element has no width.");

                // Start with the point at the specified fraction into the element, and then
                // transform that to global coordinates.
                point.X = width * fractionOfWidth;
                point.Y = height * fractionOfHeight;

                GeneralTransform transform = element.TransformToVisual(null);
                point = transform.TransformPoint(point);
            });

            return point;
        }

        public static T FindName<T>(this FrameworkElement element, string descendantName)
            where T: class
        {
            var result = element.FindName(descendantName) as T;
            Verify.IsNotNull(result);
            return result;
        }

        public static FrameworkElement FindNameInSubtree(this DependencyObject element, string descendantName, bool dumpTreeIfNotFound = false)
        {
            return FindNameInSubtreeWorker(element, descendantName, dumpTreeIfNotFound);
        }

        private static FrameworkElement FindNameInSubtreeWorker(DependencyObject element, string descendantName, bool dumpTreeIfNotFound = false)
        {
            if (element == null)
                return null;

            FrameworkElement frameworkElement = element as FrameworkElement;
            string elementName = (frameworkElement != null) ? frameworkElement.Name : "";

            if (elementName == descendantName)
                return frameworkElement;

            int childrenCount = VisualTreeHelper.GetChildrenCount(element);
            for (int i = 0; i < childrenCount; i++)
            {
                var result = FindNameInSubtreeWorker(VisualTreeHelper.GetChild(element, i), descendantName);
                if (result != null)
                    return result;
            }

            if (dumpTreeIfNotFound)
            {
                Log.Comment("FindNameInSubtree dump (failed to find '{0}':", descendantName);
                LogDumpTree(element);
            }

            return null;
        }

        public static void LogDumpTree(DependencyObject element, string indent = "")
        {
            int childrenCount = VisualTreeHelper.GetChildrenCount(element);
            string name = (element is FrameworkElement) ? ((FrameworkElement)element).Name : "";
            Log.Comment("{0}{1} ('{2}')", indent, element.GetType().Name, name);
            indent += "  ";
            for (int i = 0; i < childrenCount; i++)
            {
                LogDumpTree(VisualTreeHelper.GetChild(element, i), indent);
            }
        }

        public static T FindElementOfTypeInSubtree<T>(this DependencyObject element)
            where T : DependencyObject
        {
            if (element == null)
                return null;

            if (element is T)
                return (T)element;

            int childrenCount = VisualTreeHelper.GetChildrenCount(element);
            for (int i = 0; i < childrenCount; i++)
            {
                var result = FindElementOfTypeInSubtree<T>(VisualTreeHelper.GetChild(element, i));
                if (result != null)
                    return result;
            }

            return null;
        }

        public static FrameworkElement GetVisualChildByNameFromOpenPopups(string childName, UIElement element)
        {
            foreach (Popup popup in VisualTreeHelper.GetOpenPopupsForXamlRoot(element.XamlRoot))
            {
                FrameworkElement popupChild = (FrameworkElement)popup.Child;
                if (popupChild.Name == childName)
                {
                    return popupChild;
                }
                else
                {
                    var result = popupChild.FindNameInSubtree(childName);
                    if (result != null)
                        return result;
                }
            }
            return null;
        }
    }
}
