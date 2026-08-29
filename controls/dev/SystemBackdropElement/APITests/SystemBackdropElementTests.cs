// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using MUXControlsTestApp.Utilities;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class SystemBackdropElementTests : ApiTestBase
    {
        [TestMethod]
        public void CanInstantiateSystemBackdropElement()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropElement = new SystemBackdropElement();
                Verify.IsNotNull(systemBackdropElement, "SystemBackdropElement should be instantiable");
            });
        }

        [TestMethod]
        public void VerifyDefaultPropertyValues()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropElement = new SystemBackdropElement();

                // Verify default SystemBackdrop is null
                Verify.IsNull(systemBackdropElement.SystemBackdrop, "Default SystemBackdrop should be null");

                // Verify default CornerRadius is 0,0,0,0
                var defaultCornerRadius = systemBackdropElement.CornerRadius;
                Verify.AreEqual(0.0, defaultCornerRadius.TopLeft, "Default CornerRadius.TopLeft should be 0");
                Verify.AreEqual(0.0, defaultCornerRadius.TopRight, "Default CornerRadius.TopRight should be 0");
                Verify.AreEqual(0.0, defaultCornerRadius.BottomRight, "Default CornerRadius.BottomRight should be 0");
                Verify.AreEqual(0.0, defaultCornerRadius.BottomLeft, "Default CornerRadius.BottomLeft should be 0");
            });
        }

        [TestMethod]
        public void CanSetAndGetCornerRadius()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropElement = new SystemBackdropElement();

                // Test uniform corner radius
                var uniformRadius = new CornerRadius(8.0);
                systemBackdropElement.CornerRadius = uniformRadius;
                var retrievedUniformRadius = systemBackdropElement.CornerRadius;
                Verify.AreEqual(8.0, retrievedUniformRadius.TopLeft, "CornerRadius.TopLeft should be 8.0");
                Verify.AreEqual(8.0, retrievedUniformRadius.TopRight, "CornerRadius.TopRight should be 8.0");
                Verify.AreEqual(8.0, retrievedUniformRadius.BottomRight, "CornerRadius.BottomRight should be 8.0");
                Verify.AreEqual(8.0, retrievedUniformRadius.BottomLeft, "CornerRadius.BottomLeft should be 8.0");

                // Test non-uniform corner radius
                var nonUniformRadius = new CornerRadius(4.0, 8.0, 12.0, 16.0);
                systemBackdropElement.CornerRadius = nonUniformRadius;
                var retrievedNonUniformRadius = systemBackdropElement.CornerRadius;
                Verify.AreEqual(4.0, retrievedNonUniformRadius.TopLeft, "CornerRadius.TopLeft should be 4.0");
                Verify.AreEqual(8.0, retrievedNonUniformRadius.TopRight, "CornerRadius.TopRight should be 8.0");
                Verify.AreEqual(12.0, retrievedNonUniformRadius.BottomRight, "CornerRadius.BottomRight should be 12.0");
                Verify.AreEqual(16.0, retrievedNonUniformRadius.BottomLeft, "CornerRadius.BottomLeft should be 16.0");

                // Reset to zero
                systemBackdropElement.CornerRadius = new CornerRadius(0.0);
                var zeroRadius = systemBackdropElement.CornerRadius;
                Verify.AreEqual(0.0, zeroRadius.TopLeft, "CornerRadius.TopLeft should be 0");
                Verify.AreEqual(0.0, zeroRadius.TopRight, "CornerRadius.TopRight should be 0");
                Verify.AreEqual(0.0, zeroRadius.BottomRight, "CornerRadius.BottomRight should be 0");
                Verify.AreEqual(0.0, zeroRadius.BottomLeft, "CornerRadius.BottomLeft should be 0");
            });
        }

        [TestMethod]
        public void CanSetAndGetSystemBackdrop()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropElement = new SystemBackdropElement();

                // Verify we can set to null explicitly
                systemBackdropElement.SystemBackdrop = null;
                Verify.IsNull(systemBackdropElement.SystemBackdrop, "SystemBackdrop should be null after setting to null");

                // Note: We cannot test with actual MicaBackdrop or DesktopAcrylicBackdrop in API tests
                // because they require composition setup. Those scenarios are tested in integration tests.
            });
        }

        [TestMethod]
        public void CanSetSystemBackdropAcrylic()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropElement = new SystemBackdropElement();
                systemBackdropElement.Width = 200;
                systemBackdropElement.Height = 200;

                // Verify initial state (null)
                Verify.IsNull(systemBackdropElement.SystemBackdrop, "SystemBackdrop should initially be null");

                // Test setting DesktopAcrylicBackdrop
                var acrylicBackdrop = new Microsoft.UI.Xaml.Media.DesktopAcrylicBackdrop();
                systemBackdropElement.SystemBackdrop = acrylicBackdrop;
                Verify.IsNotNull(systemBackdropElement.SystemBackdrop, "SystemBackdrop should be set");
                Verify.AreEqual(acrylicBackdrop, systemBackdropElement.SystemBackdrop, "SystemBackdrop should match the set value");

                // Note: This test validates property get/set behavior for DesktopAcrylicBackdrop.
                // Testing backdrop lifecycle (clearing, visual tree integration) is handled in SystemBackdropElementIntegrationTests.
            });
        }

        [TestMethod]
        public void CanSetSystemBackdropMica()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropElement = new SystemBackdropElement();
                systemBackdropElement.Width = 200;
                systemBackdropElement.Height = 200;

                // Verify initial state (null)
                Verify.IsNull(systemBackdropElement.SystemBackdrop, "SystemBackdrop should initially be null");

                // Test setting MicaBackdrop
                var micaBackdrop = new Microsoft.UI.Xaml.Media.MicaBackdrop();
                systemBackdropElement.SystemBackdrop = micaBackdrop;
                Verify.IsNotNull(systemBackdropElement.SystemBackdrop, "SystemBackdrop should be set");
                Verify.AreEqual(micaBackdrop, systemBackdropElement.SystemBackdrop, "SystemBackdrop should match the set value");

                // Note: This test validates property get/set behavior for MicaBackdrop.
                // Testing backdrop lifecycle (clearing, visual tree integration) is handled in SystemBackdropElementIntegrationTests.
            });
        }

        [TestMethod]
        public void CanSetAndGetSize()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropElement = new SystemBackdropElement();

                systemBackdropElement.Width = 200.0;
                systemBackdropElement.Height = 150.0;

                Verify.AreEqual(200.0, systemBackdropElement.Width, "Width should be 200");
                Verify.AreEqual(150.0, systemBackdropElement.Height, "Height should be 150");

                systemBackdropElement.Width = 100.0;
                systemBackdropElement.Height = 100.0;

                Verify.AreEqual(100.0, systemBackdropElement.Width, "Width should be 100");
                Verify.AreEqual(100.0, systemBackdropElement.Height, "Height should be 100");
            });
        }

        [TestMethod]
        public void CanAddToVisualTree()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropElement = new SystemBackdropElement();
                systemBackdropElement.Width = 200;
                systemBackdropElement.Height = 200;

                var stackPanel = new StackPanel();
                stackPanel.Children.Add(systemBackdropElement);

                Content = stackPanel;
                Content.UpdateLayout();

                // Verify it's in the visual tree
                Verify.IsNotNull(systemBackdropElement.XamlRoot, "SystemBackdropElement should be in the visual tree");
            });
        }

        [TestMethod]
        public void CanSetAlignment()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropElement = new SystemBackdropElement();

                systemBackdropElement.HorizontalAlignment = HorizontalAlignment.Left;
                systemBackdropElement.VerticalAlignment = VerticalAlignment.Top;

                Verify.AreEqual(HorizontalAlignment.Left, systemBackdropElement.HorizontalAlignment, "HorizontalAlignment should be Left");
                Verify.AreEqual(VerticalAlignment.Top, systemBackdropElement.VerticalAlignment, "VerticalAlignment should be Top");

                systemBackdropElement.HorizontalAlignment = HorizontalAlignment.Stretch;
                systemBackdropElement.VerticalAlignment = VerticalAlignment.Stretch;

                Verify.AreEqual(HorizontalAlignment.Stretch, systemBackdropElement.HorizontalAlignment, "HorizontalAlignment should be Stretch");
                Verify.AreEqual(VerticalAlignment.Stretch, systemBackdropElement.VerticalAlignment, "VerticalAlignment should be Stretch");
            });
        }

        [TestMethod]
        public void CanSetMargin()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropElement = new SystemBackdropElement();

                var margin = new Thickness(10, 20, 30, 40);
                systemBackdropElement.Margin = margin;

                var retrievedMargin = systemBackdropElement.Margin;
                Verify.AreEqual(10.0, retrievedMargin.Left, "Margin.Left should be 10");
                Verify.AreEqual(20.0, retrievedMargin.Top, "Margin.Top should be 20");
                Verify.AreEqual(30.0, retrievedMargin.Right, "Margin.Right should be 30");
                Verify.AreEqual(40.0, retrievedMargin.Bottom, "Margin.Bottom should be 40");
            });
        }

        [TestMethod]
        public void CanUpdateCornerRadiusInVisualTree()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropElement = new SystemBackdropElement();
                systemBackdropElement.Width = 200;
                systemBackdropElement.Height = 200;
                systemBackdropElement.CornerRadius = new CornerRadius(0);

                var stackPanel = new StackPanel();
                stackPanel.Children.Add(systemBackdropElement);

                Content = stackPanel;
                Content.UpdateLayout();

                // Update CornerRadius while in visual tree
                systemBackdropElement.CornerRadius = new CornerRadius(8.0);
                var currentRadius = systemBackdropElement.CornerRadius;
                Verify.AreEqual(8.0, currentRadius.TopLeft, "CornerRadius.TopLeft should be 8.0 after update");
                Verify.AreEqual(8.0, currentRadius.TopRight, "CornerRadius.TopRight should be 8.0 after update");
                Verify.AreEqual(8.0, currentRadius.BottomRight, "CornerRadius.BottomRight should be 8.0 after update");
                Verify.AreEqual(8.0, currentRadius.BottomLeft, "CornerRadius.BottomLeft should be 8.0 after update");
            });
        }


        [TestMethod]
        public void CanSetMinSize()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropElement = new SystemBackdropElement();

                systemBackdropElement.MinWidth = 50.0;
                systemBackdropElement.MinHeight = 50.0;

                Verify.AreEqual(50.0, systemBackdropElement.MinWidth, "MinWidth should be 50");
                Verify.AreEqual(50.0, systemBackdropElement.MinHeight, "MinHeight should be 50");
            });
        }

        [TestMethod]
        public void CanSetMaxSize()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropElement = new SystemBackdropElement();

                systemBackdropElement.MaxWidth = 500.0;
                systemBackdropElement.MaxHeight = 500.0;

                Verify.AreEqual(500.0, systemBackdropElement.MaxWidth, "MaxWidth should be 500");
                Verify.AreEqual(500.0, systemBackdropElement.MaxHeight, "MaxHeight should be 500");
            });
        }

        [TestMethod]
        public void CanSetOpacity()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropElement = new SystemBackdropElement();

                systemBackdropElement.Opacity = 0.5;
                Verify.AreEqual(0.5, systemBackdropElement.Opacity, "Opacity should be 0.5");

                systemBackdropElement.Opacity = 1.0;
                Verify.AreEqual(1.0, systemBackdropElement.Opacity, "Opacity should be 1.0");

                systemBackdropElement.Opacity = 0.0;
                Verify.AreEqual(0.0, systemBackdropElement.Opacity, "Opacity should be 0.0");
            });
        }

        [TestMethod]
        public void CanSetVisibility()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropElement = new SystemBackdropElement();

                systemBackdropElement.Visibility = Visibility.Collapsed;
                Verify.AreEqual(Visibility.Collapsed, systemBackdropElement.Visibility, "Visibility should be Collapsed");

                systemBackdropElement.Visibility = Visibility.Visible;
                Verify.AreEqual(Visibility.Visible, systemBackdropElement.Visibility, "Visibility should be Visible");
            });
        }

        [TestMethod]
        public void CanCreateMultipleInstances()
        {
            RunOnUIThread.Execute(() =>
            {
                var host1 = new SystemBackdropElement();
                var host2 = new SystemBackdropElement();
                var host3 = new SystemBackdropElement();

                host1.Width = 100;
                host2.Width = 200;
                host3.Width = 300;

                var stackPanel = new StackPanel();
                stackPanel.Children.Add(host1);
                stackPanel.Children.Add(host2);
                stackPanel.Children.Add(host3);

                Content = stackPanel;
                Content.UpdateLayout();

                Verify.AreEqual(100.0, host1.ActualWidth, "Host1 ActualWidth should be 100");
                Verify.AreEqual(200.0, host2.ActualWidth, "Host2 ActualWidth should be 200");
                Verify.AreEqual(300.0, host3.ActualWidth, "Host3 ActualWidth should be 300");
            });
        }

        [TestMethod]
        public void CanRemoveFromVisualTree()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropElement = new SystemBackdropElement();
                systemBackdropElement.Width = 200;
                systemBackdropElement.Height = 200;

                var stackPanel = new StackPanel();
                stackPanel.Children.Add(systemBackdropElement);

                Content = stackPanel;
                Content.UpdateLayout();

                Verify.AreEqual(1, stackPanel.Children.Count, "StackPanel should have 1 child");
                Verify.IsNotNull(systemBackdropElement.XamlRoot, "SystemBackdropElement should be in the visual tree");

                // Remove from visual tree
                stackPanel.Children.RemoveAt(0);
                Content.UpdateLayout();

                Verify.AreEqual(0, stackPanel.Children.Count, "StackPanel should have 0 children after removal");
                // Note: XamlRoot may not immediately become null after removal from parent.
                // The important verification is that the child was removed from the parent's Children collection.
            });
        }

        [TestMethod]
        public void VerifyCornerRadiusPropertyDescriptor()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropElement = new SystemBackdropElement();
                Verify.IsNotNull(SystemBackdropElement.CornerRadiusProperty, "CornerRadiusProperty should exist");
            });
        }

        [TestMethod]
        public void VerifySystemBackdropPropertyDescriptor()
        { 
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropElement = new SystemBackdropElement();
                Verify.IsNotNull(SystemBackdropElement.SystemBackdropProperty, "SystemBackdropProperty should exist");
            });
        }

        [TestMethod]
        public void CanAccessPropertiesViaGetValue()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropElement = new SystemBackdropElement();
                systemBackdropElement.CornerRadius = new CornerRadius(10.0);

                var cornerRadiusValue = systemBackdropElement.GetValue(SystemBackdropElement.CornerRadiusProperty);
                Verify.IsNotNull(cornerRadiusValue, "CornerRadius value should not be null");

                var backdropValue = systemBackdropElement.GetValue(SystemBackdropElement.SystemBackdropProperty);
                Verify.IsNull(backdropValue, "SystemBackdrop value should be null by default");
            });
        }

        [TestMethod]
        public void CanSetPropertiesViaSetValue()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropElement = new SystemBackdropElement();

                var newCornerRadius = new CornerRadius(15.0);
                systemBackdropElement.SetValue(SystemBackdropElement.CornerRadiusProperty, newCornerRadius);

                var retrievedRadius = systemBackdropElement.CornerRadius;
                Verify.AreEqual(15.0, retrievedRadius.TopLeft, "CornerRadius.TopLeft should be 15.0 after SetValue");
                Verify.AreEqual(15.0, retrievedRadius.TopRight, "CornerRadius.TopRight should be 15.0 after SetValue");
                Verify.AreEqual(15.0, retrievedRadius.BottomRight, "CornerRadius.BottomRight should be 15.0 after SetValue");
                Verify.AreEqual(15.0, retrievedRadius.BottomLeft, "CornerRadius.BottomLeft should be 15.0 after SetValue");

                systemBackdropElement.SetValue(SystemBackdropElement.SystemBackdropProperty, null);
                Verify.IsNull(systemBackdropElement.SystemBackdrop, "SystemBackdrop should be null after SetValue(null)");
            });
        }

        [TestMethod]
        public void CanSetLargeCornerRadius()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropElement = new SystemBackdropElement();
                systemBackdropElement.Width = 100;
                systemBackdropElement.Height = 100;

                // Set corner radius larger than the control size
                var largeRadius = new CornerRadius(200.0);
                systemBackdropElement.CornerRadius = largeRadius;

                var retrievedRadius = systemBackdropElement.CornerRadius;
                Verify.AreEqual(200.0, retrievedRadius.TopLeft, "CornerRadius.TopLeft should be 200.0");
                Verify.AreEqual(200.0, retrievedRadius.TopRight, "CornerRadius.TopRight should be 200.0");
                Verify.AreEqual(200.0, retrievedRadius.BottomRight, "CornerRadius.BottomRight should be 200.0");
                Verify.AreEqual(200.0, retrievedRadius.BottomLeft, "CornerRadius.BottomLeft should be 200.0");
            });
        }
    }
}
