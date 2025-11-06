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
    public class SystemBackdropHostTests : ApiTestBase
    {
        [TestMethod]
        public void CanInstantiateSystemBackdropHost()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropHost = new SystemBackdropHost();
                Verify.IsNotNull(systemBackdropHost, "SystemBackdropHost should be instantiable");
            });
        }

        [TestMethod]
        public void VerifyDefaultPropertyValues()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropHost = new SystemBackdropHost();

                // Verify default SystemBackdrop is null
                Verify.IsNull(systemBackdropHost.SystemBackdrop, "Default SystemBackdrop should be null");

                // Verify default CornerRadius is 0,0,0,0
                var defaultCornerRadius = systemBackdropHost.CornerRadius;
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
                var systemBackdropHost = new SystemBackdropHost();

                // Test uniform corner radius
                var uniformRadius = new CornerRadius(8.0);
                systemBackdropHost.CornerRadius = uniformRadius;
                var retrievedUniformRadius = systemBackdropHost.CornerRadius;
                Verify.AreEqual(8.0, retrievedUniformRadius.TopLeft, "CornerRadius.TopLeft should be 8.0");
                Verify.AreEqual(8.0, retrievedUniformRadius.TopRight, "CornerRadius.TopRight should be 8.0");
                Verify.AreEqual(8.0, retrievedUniformRadius.BottomRight, "CornerRadius.BottomRight should be 8.0");
                Verify.AreEqual(8.0, retrievedUniformRadius.BottomLeft, "CornerRadius.BottomLeft should be 8.0");

                // Test non-uniform corner radius
                var nonUniformRadius = new CornerRadius(4.0, 8.0, 12.0, 16.0);
                systemBackdropHost.CornerRadius = nonUniformRadius;
                var retrievedNonUniformRadius = systemBackdropHost.CornerRadius;
                Verify.AreEqual(4.0, retrievedNonUniformRadius.TopLeft, "CornerRadius.TopLeft should be 4.0");
                Verify.AreEqual(8.0, retrievedNonUniformRadius.TopRight, "CornerRadius.TopRight should be 8.0");
                Verify.AreEqual(12.0, retrievedNonUniformRadius.BottomRight, "CornerRadius.BottomRight should be 12.0");
                Verify.AreEqual(16.0, retrievedNonUniformRadius.BottomLeft, "CornerRadius.BottomLeft should be 16.0");

                // Reset to zero
                systemBackdropHost.CornerRadius = new CornerRadius(0.0);
                var zeroRadius = systemBackdropHost.CornerRadius;
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
                var systemBackdropHost = new SystemBackdropHost();

                // Verify we can set to null explicitly
                systemBackdropHost.SystemBackdrop = null;
                Verify.IsNull(systemBackdropHost.SystemBackdrop, "SystemBackdrop should be null after setting to null");

                // Note: We cannot test with actual MicaBackdrop or DesktopAcrylicBackdrop in API tests
                // because they require composition setup. Those scenarios are tested in integration tests.
            });
        }

        [TestMethod]
        public void CanSetSystemBackdropAcrylic()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropHost = new SystemBackdropHost();
                systemBackdropHost.Width = 200;
                systemBackdropHost.Height = 200;

                // Verify initial state (null)
                Verify.IsNull(systemBackdropHost.SystemBackdrop, "SystemBackdrop should initially be null");

                // Test setting DesktopAcrylicBackdrop
                var acrylicBackdrop = new Microsoft.UI.Xaml.Media.DesktopAcrylicBackdrop();
                systemBackdropHost.SystemBackdrop = acrylicBackdrop;
                Verify.IsNotNull(systemBackdropHost.SystemBackdrop, "SystemBackdrop should be set");
                Verify.AreEqual(acrylicBackdrop, systemBackdropHost.SystemBackdrop, "SystemBackdrop should match the set value");

                // Note: This test validates property get/set behavior for DesktopAcrylicBackdrop.
                // Testing backdrop lifecycle (clearing, visual tree integration) is handled in SystemBackdropHostIntegrationTests.
            });
        }

        [TestMethod]
        public void CanSetSystemBackdropMica()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropHost = new SystemBackdropHost();
                systemBackdropHost.Width = 200;
                systemBackdropHost.Height = 200;

                // Verify initial state (null)
                Verify.IsNull(systemBackdropHost.SystemBackdrop, "SystemBackdrop should initially be null");

                // Test setting MicaBackdrop
                var micaBackdrop = new Microsoft.UI.Xaml.Media.MicaBackdrop();
                systemBackdropHost.SystemBackdrop = micaBackdrop;
                Verify.IsNotNull(systemBackdropHost.SystemBackdrop, "SystemBackdrop should be set");
                Verify.AreEqual(micaBackdrop, systemBackdropHost.SystemBackdrop, "SystemBackdrop should match the set value");

                // Note: This test validates property get/set behavior for MicaBackdrop.
                // Testing backdrop lifecycle (clearing, visual tree integration) is handled in SystemBackdropHostIntegrationTests.
            });
        }

        [TestMethod]
        public void CanSetAndGetSize()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropHost = new SystemBackdropHost();

                systemBackdropHost.Width = 200.0;
                systemBackdropHost.Height = 150.0;

                Verify.AreEqual(200.0, systemBackdropHost.Width, "Width should be 200");
                Verify.AreEqual(150.0, systemBackdropHost.Height, "Height should be 150");

                systemBackdropHost.Width = 100.0;
                systemBackdropHost.Height = 100.0;

                Verify.AreEqual(100.0, systemBackdropHost.Width, "Width should be 100");
                Verify.AreEqual(100.0, systemBackdropHost.Height, "Height should be 100");
            });
        }

        [TestMethod]
        public void CanAddToVisualTree()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropHost = new SystemBackdropHost();
                systemBackdropHost.Width = 200;
                systemBackdropHost.Height = 200;

                var stackPanel = new StackPanel();
                stackPanel.Children.Add(systemBackdropHost);

                Content = stackPanel;
                Content.UpdateLayout();

                // Verify it's in the visual tree
                Verify.IsNotNull(systemBackdropHost.XamlRoot, "SystemBackdropHost should be in the visual tree");
            });
        }

        [TestMethod]
        public void CanSetAlignment()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropHost = new SystemBackdropHost();

                systemBackdropHost.HorizontalAlignment = HorizontalAlignment.Left;
                systemBackdropHost.VerticalAlignment = VerticalAlignment.Top;

                Verify.AreEqual(HorizontalAlignment.Left, systemBackdropHost.HorizontalAlignment, "HorizontalAlignment should be Left");
                Verify.AreEqual(VerticalAlignment.Top, systemBackdropHost.VerticalAlignment, "VerticalAlignment should be Top");

                systemBackdropHost.HorizontalAlignment = HorizontalAlignment.Stretch;
                systemBackdropHost.VerticalAlignment = VerticalAlignment.Stretch;

                Verify.AreEqual(HorizontalAlignment.Stretch, systemBackdropHost.HorizontalAlignment, "HorizontalAlignment should be Stretch");
                Verify.AreEqual(VerticalAlignment.Stretch, systemBackdropHost.VerticalAlignment, "VerticalAlignment should be Stretch");
            });
        }

        [TestMethod]
        public void CanSetMargin()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropHost = new SystemBackdropHost();

                var margin = new Thickness(10, 20, 30, 40);
                systemBackdropHost.Margin = margin;

                var retrievedMargin = systemBackdropHost.Margin;
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
                var systemBackdropHost = new SystemBackdropHost();
                systemBackdropHost.Width = 200;
                systemBackdropHost.Height = 200;
                systemBackdropHost.CornerRadius = new CornerRadius(0);

                var stackPanel = new StackPanel();
                stackPanel.Children.Add(systemBackdropHost);

                Content = stackPanel;
                Content.UpdateLayout();

                // Update CornerRadius while in visual tree
                systemBackdropHost.CornerRadius = new CornerRadius(8.0);
                var currentRadius = systemBackdropHost.CornerRadius;
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
                var systemBackdropHost = new SystemBackdropHost();

                systemBackdropHost.MinWidth = 50.0;
                systemBackdropHost.MinHeight = 50.0;

                Verify.AreEqual(50.0, systemBackdropHost.MinWidth, "MinWidth should be 50");
                Verify.AreEqual(50.0, systemBackdropHost.MinHeight, "MinHeight should be 50");
            });
        }

        [TestMethod]
        public void CanSetMaxSize()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropHost = new SystemBackdropHost();

                systemBackdropHost.MaxWidth = 500.0;
                systemBackdropHost.MaxHeight = 500.0;

                Verify.AreEqual(500.0, systemBackdropHost.MaxWidth, "MaxWidth should be 500");
                Verify.AreEqual(500.0, systemBackdropHost.MaxHeight, "MaxHeight should be 500");
            });
        }

        [TestMethod]
        public void CanSetOpacity()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropHost = new SystemBackdropHost();

                systemBackdropHost.Opacity = 0.5;
                Verify.AreEqual(0.5, systemBackdropHost.Opacity, "Opacity should be 0.5");

                systemBackdropHost.Opacity = 1.0;
                Verify.AreEqual(1.0, systemBackdropHost.Opacity, "Opacity should be 1.0");

                systemBackdropHost.Opacity = 0.0;
                Verify.AreEqual(0.0, systemBackdropHost.Opacity, "Opacity should be 0.0");
            });
        }

        [TestMethod]
        public void CanSetVisibility()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropHost = new SystemBackdropHost();

                systemBackdropHost.Visibility = Visibility.Collapsed;
                Verify.AreEqual(Visibility.Collapsed, systemBackdropHost.Visibility, "Visibility should be Collapsed");

                systemBackdropHost.Visibility = Visibility.Visible;
                Verify.AreEqual(Visibility.Visible, systemBackdropHost.Visibility, "Visibility should be Visible");
            });
        }

        [TestMethod]
        public void CanCreateMultipleInstances()
        {
            RunOnUIThread.Execute(() =>
            {
                var host1 = new SystemBackdropHost();
                var host2 = new SystemBackdropHost();
                var host3 = new SystemBackdropHost();

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
                var systemBackdropHost = new SystemBackdropHost();
                systemBackdropHost.Width = 200;
                systemBackdropHost.Height = 200;

                var stackPanel = new StackPanel();
                stackPanel.Children.Add(systemBackdropHost);

                Content = stackPanel;
                Content.UpdateLayout();

                Verify.AreEqual(1, stackPanel.Children.Count, "StackPanel should have 1 child");
                Verify.IsNotNull(systemBackdropHost.XamlRoot, "SystemBackdropHost should be in the visual tree");

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
                Verify.IsNotNull(SystemBackdropHost.CornerRadiusProperty, "CornerRadiusProperty should exist");
            });
        }

        [TestMethod]
        public void VerifySystemBackdropPropertyDescriptor()
        { 
            RunOnUIThread.Execute(() =>
            {
                Verify.IsNotNull(SystemBackdropHost.SystemBackdropProperty, "SystemBackdropProperty should exist");
            });
        }

        [TestMethod]
        public void CanAccessPropertiesViaGetValue()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropHost = new SystemBackdropHost();
                systemBackdropHost.CornerRadius = new CornerRadius(10.0);

                var cornerRadiusValue = systemBackdropHost.GetValue(SystemBackdropHost.CornerRadiusProperty);
                Verify.IsNotNull(cornerRadiusValue, "CornerRadius value should not be null");

                var backdropValue = systemBackdropHost.GetValue(SystemBackdropHost.SystemBackdropProperty);
                Verify.IsNull(backdropValue, "SystemBackdrop value should be null by default");
            });
        }

        [TestMethod]
        public void CanSetPropertiesViaSetValue()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropHost = new SystemBackdropHost();

                var newCornerRadius = new CornerRadius(15.0);
                systemBackdropHost.SetValue(SystemBackdropHost.CornerRadiusProperty, newCornerRadius);

                var retrievedRadius = systemBackdropHost.CornerRadius;
                Verify.AreEqual(15.0, retrievedRadius.TopLeft, "CornerRadius.TopLeft should be 15.0 after SetValue");
                Verify.AreEqual(15.0, retrievedRadius.TopRight, "CornerRadius.TopRight should be 15.0 after SetValue");
                Verify.AreEqual(15.0, retrievedRadius.BottomRight, "CornerRadius.BottomRight should be 15.0 after SetValue");
                Verify.AreEqual(15.0, retrievedRadius.BottomLeft, "CornerRadius.BottomLeft should be 15.0 after SetValue");

                systemBackdropHost.SetValue(SystemBackdropHost.SystemBackdropProperty, null);
                Verify.IsNull(systemBackdropHost.SystemBackdrop, "SystemBackdrop should be null after SetValue(null)");
            });
        }

        [TestMethod]
        public void CanSetLargeCornerRadius()
        {
            RunOnUIThread.Execute(() =>
            {
                var systemBackdropHost = new SystemBackdropHost();
                systemBackdropHost.Width = 100;
                systemBackdropHost.Height = 100;

                // Set corner radius larger than the control size
                var largeRadius = new CornerRadius(200.0);
                systemBackdropHost.CornerRadius = largeRadius;

                var retrievedRadius = systemBackdropHost.CornerRadius;
                Verify.AreEqual(200.0, retrievedRadius.TopLeft, "CornerRadius.TopLeft should be 200.0");
                Verify.AreEqual(200.0, retrievedRadius.TopRight, "CornerRadius.TopRight should be 200.0");
                Verify.AreEqual(200.0, retrievedRadius.BottomRight, "CornerRadius.BottomRight should be 200.0");
                Verify.AreEqual(200.0, retrievedRadius.BottomLeft, "CornerRadius.BottomLeft should be 200.0");
            });
        }
    }
}
