// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using MUXControlsTestApp.Utilities;
using Windows.ApplicationModel.Core;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Controls.Primitives;
using System.Threading;
using Windows.UI.Xaml.Markup;
using System.Collections.Generic;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

#if !BUILD_WINDOWS
using XamlControlsResources = Microsoft.UI.Xaml.Controls.XamlControlsResources;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    public class MainWindowLightConfiguration : IDisposable
    {
        public CoreApplicationView View = null;
        public MainWindowLightConfiguration()
        {
            RunOnUIThread.Execute(() =>
            {
                View = CoreApplication.MainView;
            });
            IdleSynchronizer.Wait();
        }

        public void Dispose()
        {
        }
    }

    public class SecondaryWindowLightConfiguration : IDisposable
    {
        public CoreApplicationView View = null;
        public SecondaryWindowLightConfiguration()
        {
            View = ViewHelper.MakeSecondaryView(() =>
            {
                Window.Current.Content = new Frame();
            });
        }

        public void Dispose()
        {
            ViewHelper.CloseSecondaryView(View);
        }
    }

    [TestClass]
    public class LightConfigurationTests : ApiTestBase
    {
        MediaPlayerElement _mpe;
        AutoResetEvent _mediaFullScreened;
        AutoResetEvent _cleanupVerified;

        DispatcherTimer _lightValidationTimer;
        TimeSpan _pollInterval = new TimeSpan(16667);      // 16ms
        int _pollMaxRetries = 60;
        int _pollRetry = 0;
        UIElement _visualRoot;
        UIElement _popupRoot;
        UIElement _fullWindowMediaRoot;
        AutoResetEvent _validationCompleted;
        
        [TestMethod]
        public void VerifyLightsOnMainWindow()
        {
            using (var config = new MainWindowLightConfiguration())
            {
                VerifyLights(config.View, false, false);
            }
        }

        //[TestMethod]
        //BUGBUG: Bug 18287798: Failure 183760047- Failed: MUXControls.ApiTests.LightConfigurationTests.VerifyLightsOnSecondaryWindow
        public void VerifyLightsOnSecondaryWindow()
        {
            using (var config = new SecondaryWindowLightConfiguration())
            {
                VerifyLights(config.View, false, false);
            }
        }

        [TestMethod]
        public void VerifyLightsAfterResettingContentOnMainWindow()
        {
            using (var config = new MainWindowLightConfiguration())
            {
                VerifyLights(config.View, false, true);
            }
        }

        // Disabled due to: Bug 17808897: Test unreliable in master: MUXControls.ApiTests.LightConfigurationTests.VerifyLightsAfterResettingContentOnSecondaryWindow
        //[TestMethod]
        public void VerifyLightsAfterResettingContentOnSecondaryWindow()
        {
            using (var config = new SecondaryWindowLightConfiguration())
            {
                VerifyLights(config.View, false, true);
            }
        }

        // Disabled due to: Bug 17808897: Test unreliable in master: MUXControls.ApiTests.LightConfigurationTests.VerifyLightsAfterResettingContentOnSecondaryWindow
        //[TestMethod]
        public void VerifyLightsAttachedDuringLayoutOnSecondaryWindow()
        {
            using (var config = new SecondaryWindowLightConfiguration())
            {
                VerifyLights(config.View, true, false);
            }
        }

        void VerifyLights(CoreApplicationView whichView, bool forceLightAttachDuringLayout, bool resetWindowContent)
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Warning("Lights don't work on RS1 and earlier, nothing to verify.");
                return;
            }

            AutoResetEvent popupOpened = null;
            Popup myPopup = null;
            StackPanel mySPRoot = null;

            RunOnUIThread.Execute(whichView, () =>
            {
                mySPRoot = new StackPanel();

                // Lights will be created when the first RevealBrush enters the tree
                if (!forceLightAttachDuringLayout)
                {
                    Button myButton = new Button();
                    myButton.Width = 75;
                    myButton.Height = 50;
                    myButton.Style = Application.Current.Resources["ButtonRevealStyle"] as Style;
                    mySPRoot.Children.Add(myButton);
                }
                else
                {
                    string popupWithGridview = TestUtilities.ProcessTestXamlForRepo(
                        @"<Popup xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' xmlns:controls='using:Microsoft.UI.Xaml.Controls' x:Name='MarkupPopup' IsOpen='False'>
                        <Popup.Resources>
                            <Style TargetType='GridViewItem' x:Key='RevealExampleGridViewItem'>
                                <Setter Property='Background' Value='Transparent' />
                                <Setter Property='HorizontalContentAlignment' Value='Center' />
                                <Setter Property='VerticalContentAlignment' Value='Center' />
                                <Setter Property='Template'>
                                    <Setter.Value>
                                        <ControlTemplate TargetType='GridViewItem'>
                                            <controls:RevealListViewItemPresenter ContentTransitions='{TemplateBinding ContentTransitions}'
                                        SelectionCheckMarkVisualEnabled='{ThemeResource GridViewItemSelectionCheckMarkVisualEnabled}'
                                        CheckBrush='Transparent'
                                        CheckBoxBrush='Transparent'
                                        DragBackground='{ThemeResource GridViewItemDragBackground}'
                                        DragForeground='{ThemeResource GridViewItemDragForeground}'
                                        FocusBorderBrush='{ThemeResource GridViewItemFocusBorderBrush}'
                                        FocusSecondaryBorderBrush='{ThemeResource GridViewItemFocusSecondaryBorderBrush}'
                                        PlaceholderBackground='Transparent'
                                        PointerOverBackground='Transparent'
                                        PointerOverForeground='{ThemeResource GridViewItemForegroundPointerOver}'
                                        SelectedBackground='Transparent'
                                        SelectedForeground='{ThemeResource GridViewItemForegroundSelected}'
                                        SelectedPointerOverBackground='Transparent'
                                        PressedBackground='Transparent'
                                        SelectedPressedBackground='Transparent'
                                        DisabledOpacity='{ThemeResource ListViewItemDisabledThemeOpacity}'
                                        DragOpacity='{ThemeResource ListViewItemDragThemeOpacity}'
                                        ReorderHintOffset='{ThemeResource GridViewItemReorderHintThemeOffset}'
                                        HorizontalContentAlignment='{TemplateBinding HorizontalContentAlignment}'
                                        VerticalContentAlignment='{TemplateBinding VerticalContentAlignment}'
                                        ContentMargin='{TemplateBinding Padding}'
                                        CheckMode='{ThemeResource GridViewItemCheckMode}' />
                                        </ControlTemplate>
                                    </Setter.Value>
                                </Setter>
                            </Style>

                            <DataTemplate x:Key='BackgroundBrushDataTemplate'>
                                <Grid Margin='5' Background='{Binding Value}' >
                                    <TextBlock
                                Margin='3'
                                FontSize='12'
                                MinWidth='200'
                                MinHeight='36'
                                MaxWidth='330'
                                TextWrapping='Wrap'
                                Text='{Binding Key}' />
                                </Grid>
                            </DataTemplate>
                            <DataTemplate x:Key='BorderBrushDataTemplate'>
                                <Border Margin='5' BorderBrush='{Binding Value}' BorderThickness='3'>
                                    <TextBlock
                                Margin='3'
                                FontSize='12'
                                MinWidth='200'
                                MinHeight='36'
                                MaxWidth='330'
                                TextWrapping='Wrap'
                                Text='{Binding Key}' />
                                </Border>
                            </DataTemplate>
                        </Popup.Resources>
                        <GridView Name='BackgroundList' ItemsSource='{Binding RevealBackgroundBrushes, Mode=OneWay}' ItemContainerStyle='{StaticResource RevealExampleGridViewItem}' ItemTemplate='{StaticResource BackgroundBrushDataTemplate}' MaxWidth='700' Margin='10' MaxHeight='200'/>
                    </Popup>");

                    myPopup = XamlReader.Load(popupWithGridview) as Popup;
                    myPopup.DataContext = this;
                    mySPRoot.Children.Add(myPopup);
                }

                if (whichView != CoreApplication.MainView)
                {
                    Window.Current.Content = mySPRoot;
                }
                else
                {
                    Content = mySPRoot;
                }
            });
            IdleSynchronizer.Wait();

            if (resetWindowContent)
            {
                RunOnUIThread.Execute(whichView, () =>
                {
                    StackPanel newSPRoot = new StackPanel();
                    Button myButton = new Button();
                    myButton.Width = 75;
                    myButton.Height = 50;
                    myButton.Style = Application.Current.Resources["ButtonRevealStyle"] as Style;
                    newSPRoot.Children.Add(myButton);

                    if (whichView != CoreApplication.MainView)
                    {
                        Window.Current.Content = newSPRoot;
                        
                    }
                    else
                    {
                        Content = newSPRoot;
                    }
                });
                IdleSynchronizer.Wait();
            }

            RunOnUIThread.Execute(whichView, () =>
            {
                if (forceLightAttachDuringLayout)
                {
                    myPopup.IsOpen = true;
                }

            // Find and store public Visual Root
            _visualRoot = GetTopParent(Window.Current.Content);

                popupOpened = new AutoResetEvent(false);
            // Make an unparented popup and open it so we can check that the popup root has lights set on it too.
            Popup popup = new Popup();
                popup.Child = new Grid();
                popup.Opened += (sender, args) =>
                {

                // Find and store Popup Root
                _popupRoot = GetTopParent(popup.Child);
                    Verify.AreNotEqual(_visualRoot, _popupRoot);
                    popup.IsOpen = false;
                    popupOpened.Set();
                };
                popup.IsOpen = true;
            });
            IdleSynchronizer.Wait();

            Verify.IsTrue(popupOpened.WaitOne(TimeSpan.FromMinutes(2)), "Waiting for popup to open ");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(whichView, () =>
            {
                _mediaFullScreened = new AutoResetEvent(false);

                Log.Comment("Creating MediaPlayerElement and going to full screen.");
                _mpe = new MediaPlayerElement();
                _mpe.AreTransportControlsEnabled = true;
                mySPRoot.Children.Add(_mpe);
                _mpe.IsFullWindow = true;
                XamlControlsResources.EnsureRevealLights(_mpe.TransportControls);
                CompositionTarget.Rendering += CompositionTarget_Rendering;
            });

            Verify.IsTrue(_mediaFullScreened.WaitOne(TimeSpan.FromMinutes(2)), "Waiting for media player to go full screen");
            IdleSynchronizer.Wait();

            // Validate each root has the expected lights
            RunOnUIThread.Execute(whichView, () =>
            {
                _pollRetry = 0;
                _validationCompleted = new AutoResetEvent(false);
                _lightValidationTimer = new DispatcherTimer();
                _lightValidationTimer.Interval = _pollInterval;
                _lightValidationTimer.Tick += PollTimer_Tick;
                _lightValidationTimer.Start();
            });

            Verify.IsTrue(_validationCompleted.WaitOne(TimeSpan.FromMinutes(2)), "Waiting for light validation to complete");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(whichView, () =>
            {
                if (whichView == CoreApplication.MainView)
                {
                    Content = null;
                }
                else
                {
                    Window.Current.Content = null;
                }
            });
            IdleSynchronizer.Wait();

            _cleanupVerified = new AutoResetEvent(false);
            RunOnUIThread.Execute(whichView, () =>
            {
            // RevealBrush cleans itself up in a CompositionTarget.Rendering callback. Put the cleanup validation in CT.R
            // as well to let the cleanup code run.
            CompositionTarget.Rendering += CTR_CheckCleanup;
            });

            Verify.IsTrue(_cleanupVerified.WaitOne(TimeSpan.FromMinutes(1)), "Waiting for cleanup validation");
        }

        private void CompositionTarget_Rendering(object sender, object e)
        {
            CompositionTarget.Rendering -= CompositionTarget_Rendering;

            // Find and store FullWindow Media Root
            _fullWindowMediaRoot = GetTopParent(_mpe.TransportControls);
            Verify.AreNotEqual(_visualRoot, _fullWindowMediaRoot);

            _mpe.IsFullWindow = false;
            _mediaFullScreened.Set();
        }

        private void CTR_CheckCleanup(object sender, object e)
        {
            CompositionTarget.Rendering -= CTR_CheckCleanup;

            Log.Comment("Check that lights are detached once all RevealBrushes in the main tree are gone");

            Verify.AreEqual(_visualRoot.Lights.Count, 0, "Window.Current.Content.Lights.Count should be 0");
            Verify.AreEqual(_popupRoot.Lights.Count, 0, "PopupRoot.Lights.Count should be 0");
            // In RS3 we made a change where WUX internally sets lights on the RootScrollViewer on the RootVisual instead. If that
            // happens, then we don't need to attach lights to the other roots. This change was made around the time that
            // Symbol.GlobalNavigationButton was added.
            bool wuxSetsRSVLightOnRootVisual = Windows.Foundation.Metadata.ApiInformation.IsEnumNamedValuePresent("Windows.UI.Xaml.Controls.Symbol", "GlobalNavigationButton");
            if (!wuxSetsRSVLightOnRootVisual)
            {
                Verify.AreEqual(_fullWindowMediaRoot.Lights.Count, 5, "FullWindowMediaRoot.Lights.Count should be 5 (because they were manually added)");
            }
            else
            {
                Verify.AreEqual(_fullWindowMediaRoot.Lights.Count, 0, "FullWindowMediaRoot.Lights.Count should be 0 (because they were never manually added)");
            }

            _cleanupVerified.Set();
        }

        private UIElement GetTopParent(UIElement current)
        {
            UIElement parent = current;
            do
            {
                current = parent;
                parent = (UIElement)VisualTreeHelper.GetParent(current);
            } while (parent != null);

            return current;
        }

        private void PollTimer_Tick(object sender, object e)
        {
            int expectedLightsOnPopupRoot = 5;
            int expectedLightsOnFWMediaRoot = 5;

            // In RS3 we made a change where WUX internally sets lights on the RootScrollViewer on the RootVisual instead. If that
            // happens, then we don't need to attach lights to the other roots. This change was made around the time that
            // Symbol.GlobalNavigationButton was added.
            bool wuxSetsRSVLightOnRootVisual = Windows.Foundation.Metadata.ApiInformation.IsEnumNamedValuePresent("Windows.UI.Xaml.Controls.Symbol", "GlobalNavigationButton");
            if (wuxSetsRSVLightOnRootVisual)
            {
                Log.Comment("WUX sets RootScrollViewer lights on root visual");
                expectedLightsOnPopupRoot = 0;
                expectedLightsOnFWMediaRoot = 0;
            }

            bool rootHaslights = _visualRoot.Lights.Count == 5;
            bool popupRootHaslights = _popupRoot.Lights.Count == expectedLightsOnPopupRoot;
            bool fullWindowMediaHaslights = _fullWindowMediaRoot.Lights.Count == expectedLightsOnFWMediaRoot;

            _pollRetry++;
            Log.Comment("Validating lights, attempt #" + _pollRetry);

            if ((rootHaslights && popupRootHaslights && fullWindowMediaHaslights) ||
                _pollRetry == _pollMaxRetries)
            {
                Verify.AreEqual(_visualRoot.Lights.Count, 5, "Window.Current.Content.Lights.Count should be 5");
                Verify.AreEqual(_popupRoot.Lights.Count, expectedLightsOnPopupRoot, "PopupRoot.Lights.Count should be " + expectedLightsOnPopupRoot);
                Verify.AreEqual(_fullWindowMediaRoot.Lights.Count, expectedLightsOnFWMediaRoot, "FullWindowMediaRoot.Lights.Count should be " + expectedLightsOnFWMediaRoot);

                ((DispatcherTimer)sender).Stop();
                _validationCompleted.Set();
            }
            else
            {
                Log.Comment("Window.Current.Content.Lights.Count is " + _visualRoot.Lights.Count + ", should be 5");
                Log.Comment("PopupRoot.Lights.Count is " + _popupRoot.Lights.Count + ", should be " + expectedLightsOnPopupRoot);
                Log.Comment("FullWindowMediaRoot.Lights.Count is " + _fullWindowMediaRoot.Lights.Count + ", should be " + expectedLightsOnFWMediaRoot);
            }
        }

        public static IEnumerable<string> BackgroundBrushNames
        {
            get
            {
                return new List<string>() {
                    "SystemControlBackgroundBaseLowRevealBackgroundBrush",
                    "SystemControlTransparentRevealBackgroundBrush",
                    "SystemControlHighlightAccentRevealBackgroundBrush",
                    "SystemControlHighlightAccent3RevealBackgroundBrush",
                    "SystemControlHighlightAccent2RevealBackgroundBrush",
                    "SystemControlHighlightListMediumRevealBackgroundBrush",
                    "SystemControlHighlightListLowRevealBackgroundBrush",
                    "SystemControlBackgroundBaseMediumLowRevealBaseLowBackgroundBrush",
                    "SystemControlHighlightBaseMediumLowRevealAccentBackgroundBrush",
                    "SystemControlHighlightListMediumRevealListLowBackgroundBrush",
                    "SystemControlHighlightAccent3RevealAccent2BackgroundBrush"
                };
            }
        }

        public static string GetBrushName(string fullName)
        {
            //SystemControlTransparentRevealBackgroundBrush
            //SystemControlBackgroundTransparentRevealBorderBrush
            if (fullName == null || fullName.Length <= 24)
                return string.Empty;

            return fullName.Substring(19, fullName.Length - 24);
        }

        public IEnumerable<KeyValuePair<string, Brush>> RevealBackgroundBrushes
        {
            get
            {
                foreach (var brushName in BackgroundBrushNames)
                {
                    var brush = Application.Current.Resources[brushName] as Brush;
                    if (brush != null)
                        yield return new KeyValuePair<string, Brush>(GetBrushName(brushName), brush);
                }
            }
        }

    }
}
