// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Threading;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Tests.Common;
using System.Diagnostics;

using Microsoft.UI.Xaml.Controls.Primitives;

using Microsoft.UI.Xaml.Shapes;

namespace Microsoft.UI.Xaml.Tests.Controls
{
    [TestClass]
    public class ElementSoundPlayerIntegrationTests : XamlTestsBase
    {
        private static ElementSoundPlayerState defaultState = ElementSoundPlayerState.Auto;
        private static double defaultVolume = 0.0;

        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Ignore", "True")] // TODO: Re-enable ElementSoundPlayer tests that were disabled for Lifted Xaml.
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Recording default ElementSoundPlayer State and Volume.");
                defaultState = ElementSoundPlayer.State;
                defaultVolume = ElementSoundPlayer.Volume;
                Log.Comment("State={0}, Volume={1}", defaultState, defaultVolume);
            });
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestCleanup]
        public override void TestCleanup()
        {
            UIExecutor.Execute(() =>
            {
                Log.Comment("Restoring default ElementSoundPlayer State={0} and Volume={1}.", defaultState, defaultVolume);
                ElementSoundPlayer.State = defaultState;
                ElementSoundPlayer.Volume = defaultVolume;
            });

            TestServices.WindowHelper.SetPlayingSoundNodeCallback(null);

            base.TestCleanup();
        }

        public enum SetSpatialAudioMode
        {
            None,
            Off,
            On
        }
        
        public struct SoundData
        {
            public ElementSoundKind elementSoundKind;
            public bool isSpatialAudioSound;
            public double x;
            public double y;
            public double z;
            public double volume;

            public SoundData(
                    ElementSoundKind _elementSoundKind, 
                    bool _isSpatialAudioSound, 
                    double _x, 
                    double _y, 
                    double _z,
                    double _volume) 
            {
                elementSoundKind = _elementSoundKind;
                isSpatialAudioSound = _isSpatialAudioSound;
                x = _x;
                y = _y; 
                z = _z;
                volume = _volume;
            }
        };

        [TestMethod]
        [TestProperty(
            "Description",
            "Verify tapping headers to change pivot items plays next/previous sound depending on index position")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyPivotHeaderTapSound()
        {
            const string rootPanelXaml =
                @"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <Pivot x:Name='rootPivot' Title='PIVOT TITLE'>
                        <Pivot.RightHeader>
                            <CommandBar ClosedDisplayMode='Compact'>
                                <AppBarButton Icon='Back' Label='Previous'/>
                                <AppBarButton Icon='Forward' Label='Next'/>
                            </CommandBar>
                        </Pivot.RightHeader> 
                        <PivotItem Header='Pivot Item 1' x:Name='PivotItem1'>
                            <!--Pivot content goes here-->
                            <TextBlock Text='Content of pivot item 1.'/>
                        </PivotItem>
                        <PivotItem Header='Pivot Item 2' x:Name='PivotItem2'>
                            <!--Pivot content goes here-->
                            <TextBlock Text='Content of pivot item 2.'/>
                        </PivotItem>
                        <PivotItem Header='Pivot Item 3' x:Name='PivotItem3'>
                            <!--Pivot content goes here-->
                            <TextBlock Text='Content of pivot item 3.'/>
                        </PivotItem>
                    </Pivot>
                </Grid>";

            SoundData[] soundData = new SoundData[]
            { 
                new SoundData(ElementSoundKind.MoveNext, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.MovePrevious, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.MoveNext, false, 0, 0, 0, .8)
            };

            int currentSoundIndex = 0;

            Grid rootPanel = null;

            UIExecutor.Execute(() =>
            {
                Log.Comment("Loading UI.");
                rootPanel = XamlReader.Load(rootPanelXaml) as Grid;
                Window.Current.Content = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Enabling element sound playing.");
                ElementSoundPlayer.State = ElementSoundPlayerState.On;
                ElementSoundPlayer.Volume = 0.8;
            });

            PlayingSoundNodeCallback playingSoundCallback = null;

            UIExecutor.Execute(() =>
            {
                Log.Comment("Creating PlayingSoundNodeCallback.");
                playingSoundCallback = new PlayingSoundNodeCallback((ElementSoundKind sound, bool isSpatialAudioSound, float x, float y, float z, double volume) =>
                {
                    Log.Comment("PlayingSoundNodeCallback: ElementSoundKind: {0}, isSpatialSound: {1}, Emitter.x: {2}, Emitter.y: {3}, Emitter.z: {4}, Volume: {5}", 
                            sound, 
                            isSpatialAudioSound, 
                            (decimal)x, 
                            (decimal)y, 
                            (decimal)z,
                            (decimal)volume);

                    if (soundData != null)
                    {
                        Verify.AreEqual(sound, soundData[currentSoundIndex].elementSoundKind);
                        Verify.AreEqual(isSpatialAudioSound, soundData[currentSoundIndex].isSpatialAudioSound);
                        
                        currentSoundIndex++;
                    }
                });
            });

            Pivot rootPivot = null;
            PivotHeaderPanel pivotHeaderPanel = null;
            ContentControl pivotHeaderItem1 = null;
            ContentControl pivotHeaderItem2 = null;
            ContentControl pivotHeaderItem3 = null;

            UIExecutor.Execute(() =>
            {
                Log.Comment("Loading UI.");
                rootPanel = XamlReader.Load(rootPanelXaml) as Grid;
                Window.Current.Content = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Hooking up PlayingSoundNodeCallback.");
            TestServices.WindowHelper.SetPlayingSoundNodeCallback(playingSoundCallback);

            UIExecutor.Execute(() =>
            {
                Log.Comment("Getting rootPivot..");
                rootPivot = rootPanel.FindName("rootPivot") as Pivot;

                Log.Comment("Getting Pivot header panel..");
                pivotHeaderPanel = VisualTreeUtils.FindNameInSubtree(rootPivot, "StaticHeader") as PivotHeaderPanel;
                Verify.IsNotNull(pivotHeaderPanel);

                Log.Comment("Children via 'StaticHeader': {0}", pivotHeaderPanel.Children.Count);
                pivotHeaderItem1 = pivotHeaderPanel.Children[0] as ContentControl;
                pivotHeaderItem2 = pivotHeaderPanel.Children[1] as ContentControl;
                pivotHeaderItem3 = pivotHeaderPanel.Children[2] as ContentControl;

                Verify.IsNotNull(pivotHeaderItem1, "Pivot Header #1");
                Verify.IsNotNull(pivotHeaderItem2, "Pivot Header #2");
                Verify.IsNotNull(pivotHeaderItem3, "Pivot Header #3");
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Tapping pivot header item #3...");
                TestServices.InputHelper.Tap(pivotHeaderItem3);
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Tapping pivot header item #1...");
                TestServices.InputHelper.Tap(pivotHeaderItem1);
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Tapping pivot header item #2...");
                TestServices.InputHelper.Tap(pivotHeaderItem2);
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Verify callback was called once for each tap...");
            Verify.AreEqual(currentSoundIndex, 3);
        }

        [TestMethod]
        [TestProperty(
            "Description",
            "VerifyDefaultSpatialAudioModeSetting")]
        public void VerifyDefaultSpatialAudioModeSeting()
        {
            const string rootPanelXaml =
                @"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='btnTopCenter' Content='TopCenter' VerticalAlignment='Top' HorizontalAlignment='Center'/>
                        <Button x:Name='btnLeft' Content='Left' HorizontalAlignment='Left'/>
                        <Button x:Name='btnCenter' Content='Center' VerticalAlignment='Center' HorizontalAlignment='Center'/>
                        <Button x:Name='btnRight' Content='Right' HorizontalAlignment='Right'/>
                        <Button x:Name='btnBottom' Content='BottomCenter' VerticalAlignment='Bottom' HorizontalAlignment='Center'/>
                </Grid>";

            // ::::::::::::::::::::::::::::::::::::::::::::::::::
            //      Load the Xaml panel 
            // ::::::::::::::::::::::::::::::::::::::::::::::::::

            Grid rootPanel = null;

            UIExecutor.Execute(() =>
            {
                Log.Comment("Loading UI.");
                rootPanel = XamlReader.Load(rootPanelXaml) as Grid;
                Window.Current.Content = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Get default ElementSpatialAudioMode...");
                var spatialAudioMode = ElementSoundPlayer.SpatialAudioMode;

                Log.Comment("Verify default mode is ElementSpatialAudioMode.Auto...");
                Verify.AreEqual(spatialAudioMode, ElementSpatialAudioMode.Auto);
            });
        } 

        [TestMethod]
        [TestProperty(
            "Description",
            "Exercises the ElementSoundPlayer's State and Volume properties, and the ElementSoundMode property of input, HyperLink and Flyout controls for automatic element sound playing: Verify SpatialAudio is off by default - do not explicitly provide a setting")]
        [TestProperty("Hosting:Mode", "UAP")]    
        public void ExerciseElementSoundPlayerService()
        {
            SoundData[] soundData = new SoundData[]
            { 
                new SoundData(ElementSoundKind.Focus, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.Focus, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.Focus, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.Focus, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.Focus, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.Focus, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.Show, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.Hide, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.Show, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.Focus, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.Focus, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.Hide, false, 0, 0, 0, .8)
            };

            // Verify SpatialAudio is off by default - do not explicitly provide a setting
            ExerciseElementSoundPlayer(SetSpatialAudioMode.None, soundData);
        }

        [TestMethod]
        [TestProperty(
            "Description",
            "Exercises the ElementSoundPlayer's State and Volume properties, and the ElementSoundMode property of input, HyperLink and Flyout controls for automatic element sound playing.")]
        [TestProperty("Platform", "Desktop")]
        [TestProperty("EnabledOnOneCore", "False")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ExerciseElementSoundPlayerServiceWithSpatialAudio()
        {
            SoundData[] soundData = new SoundData[]
            {
                new SoundData(ElementSoundKind.Focus, true, -1.753425, 0.9310345, -3.724138, 0.8),
                new SoundData(ElementSoundKind.Focus, true, -1.753425, 0.8678161, -3.471264, 0.8),
                new SoundData(ElementSoundKind.Focus, true, -1.330724, 0.9676724, -3.87069, 0.8),
                new SoundData(ElementSoundKind.Focus, true, -1.330724, 0.9683908, -3.873563, 0.8),
                new SoundData(ElementSoundKind.Focus, true, -1.303327, 0.9123563, -3.649425, 0.8),
                new SoundData(ElementSoundKind.Focus, true, -1.187867, 0.8663793, -3.465517, 0.8),
                new SoundData(ElementSoundKind.Show, true, -2, 1, -4, 0.8),
                new SoundData(ElementSoundKind.Hide, true, -1.18591, 0.7744253, -3.097701, 0.8),
                new SoundData(ElementSoundKind.Show, true, -1.720157, 0.9389368, -3.755747, 0.8),
                new SoundData(ElementSoundKind.Focus, true, -1.18591, 0.7514368, -3.005747, 0.8),
                new SoundData(ElementSoundKind.Focus, true, -1.18591, 0.7974138, -3.189655, 0.8),
                new SoundData(ElementSoundKind.Hide, true, -1.18591, 0.7744253, -3.097701, 0.8)
            };

            ExerciseElementSoundPlayer(SetSpatialAudioMode.On, soundData);
        }

        [TestMethod]
        [TestProperty(
            "Description",
            "Exercises the ElementSoundPlayer's State and Volume properties, and the ElementSoundMode property of input, HyperLink and Flyout controls for automatic element sound playing.")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ExerciseElementSoundPlayerServiceWithoutSpatialAudio()
        {
            SoundData[] soundData = new SoundData[]
            { 
                new SoundData(ElementSoundKind.Focus, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.Focus, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.Focus, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.Focus, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.Focus, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.Focus, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.Show, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.Hide, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.Show, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.Focus, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.Focus, false, 0, 0, 0, .8),
                new SoundData(ElementSoundKind.Hide, false, 0, 0, 0, .8)
            };

            ExerciseElementSoundPlayer(SetSpatialAudioMode.Off, soundData);
        }

        public void ExerciseElementSoundPlayer(SetSpatialAudioMode setSpatialAudioMode, SoundData[] soundData)
        {
            const string rootPanelXaml =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Orientation='Horizontal'>
                    <StackPanel>
                        <Button x:Name='btnButton' Content='Button'/>
                        <TextBox x:Name='txtTextBox' Text='TextBox'/>
                        <TextBlock x:Name='tblTextBlock'>
                            TextBlock-beginning<LineBreak/>
                            <Hyperlink x:Name='hyperlinkBing' NavigateUri='http://www.bing.com'> www.bing.com link</Hyperlink><LineBreak/>
                            TextBlock-end
                        </TextBlock>
                    </StackPanel>
                    <StackPanel>
                        <Button x:Name='btnParentButton'>
                            <Button.Content>
                                <TextBox x:Name='txtButtonContentTextBox' Text='Content'/>
                            </Button.Content>
                        </Button>
                        <Button x:Name='btnFlyoutButton' Content='Flyout Button'>
                            <Button.Flyout>
                                <Flyout x:Name='flyoutButton'>
                                    <StackPanel>
                                        <CheckBox x:Name='chkCheckBoxInButtonFlyout' Content='CheckBox'/>
                                        <RadioButton x:Name='rdRadioButtonInButtonFlyout' Content='RadioButton'/>
                                    </StackPanel>
                                </Flyout>
                            </Button.Flyout>
                        </Button>
                        <Button x:Name='btnAttachedFlyoutButton' Content='Attached Flyout Button'>
                            <Flyout.AttachedFlyout>
                                <Flyout x:Name='flyoutAttached'>
                                    <StackPanel>
                                        <CheckBox x:Name='chkCheckBoxInAttachedFlyout' Content='CheckBox'/>
                                        <RadioButton x:Name='rdRadioButtonInAttachedFlyout' Content='RadioButton'/>
                                    </StackPanel>
                                </Flyout>
                            </Flyout.AttachedFlyout>
                        </Button>
                    </StackPanel>
                </StackPanel>";

           

            int currentSoundIndex = 0; 
        
            StackPanel rootPanel = null;
            Button btnButton = null;
            Button btnAttachedFlyoutButton = null;
            RadioButton rdRadioButtonInAttachedFlyout = null;

            PlayingSoundNodeCallback playingSoundCallback = null;

            UIExecutor.Execute(() =>
            {
                Log.Comment("Creating PlayingSoundNodeCallback.");
                playingSoundCallback = new PlayingSoundNodeCallback((ElementSoundKind sound, bool isSpatialAudioSound, float x, float y, float z, double volume) =>
                {
                    Log.Comment("PlayingSoundNodeCallback Actual: ElementSoundKind: {0}, isSpatialSound: {1}, Emitter.x: {2}, Emitter.y: {3}, Emitter.z: {4}, Volume: {5}", 
                            sound, 
                            isSpatialAudioSound, 
                            (decimal)x, 
                            (decimal)y, 
                            (decimal)z,
                            (decimal)volume);

                    Log.Comment("PlayingSoundNodeCallback Expected: ElementSoundKind: {0}, isSpatialSound: {1}, Emitter.x: {2}, Emitter.y: {3}, Emitter.z: {4}", 
                            soundData[currentSoundIndex].elementSoundKind, 
                            soundData[currentSoundIndex].isSpatialAudioSound, 
                            (decimal)soundData[currentSoundIndex].x, 
                            (decimal)soundData[currentSoundIndex].y, 
                            (decimal)soundData[currentSoundIndex].z,
                            (decimal)soundData[currentSoundIndex].volume);
  
                    if (soundData != null)
                    {
                        Verify.AreEqual(sound, soundData[currentSoundIndex].elementSoundKind);
                        Verify.AreEqual(isSpatialAudioSound, soundData[currentSoundIndex].isSpatialAudioSound);

                        // Compare only to two decimal places to accommodate minor UI changes
                        Verify.AreEqual(Math.Round(x, 2), Math.Round(soundData[currentSoundIndex].x, 2));
                        Verify.AreEqual(Math.Round(y, 2), Math.Round(soundData[currentSoundIndex].y, 2));
                        Verify.AreEqual(Math.Round(z, 2), Math.Round(soundData[currentSoundIndex].z, 2));
                        Verify.AreEqual(volume, soundData[currentSoundIndex].volume);

                        currentSoundIndex++;
                    }
                });
            });

            UIExecutor.Execute(() =>
            {
                Log.Comment("Loading UI.");
                rootPanel = XamlReader.Load(rootPanelXaml) as StackPanel;
                Window.Current.Content = rootPanel;

                btnButton = rootPanel.FindName("btnButton") as Button;
                btnAttachedFlyoutButton = rootPanel.FindName("btnAttachedFlyoutButton") as Button;
                rdRadioButtonInAttachedFlyout = rootPanel.FindName("rdRadioButtonInAttachedFlyout") as RadioButton;

                Verify.IsNotNull(btnButton);
                Verify.IsNotNull(btnAttachedFlyoutButton);
                Verify.IsNotNull(rdRadioButtonInAttachedFlyout);
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Enabling element sound playing.");
                ElementSoundPlayer.State = ElementSoundPlayerState.On;
                ElementSoundPlayer.Volume = 0.8;

                Log.Comment("Setting Spatial Audio Mode to {0}...", setSpatialAudioMode);
                switch (setSpatialAudioMode)
                {
                    case SetSpatialAudioMode.None:
                    break;

                    case SetSpatialAudioMode.Off:
                        ElementSoundPlayer.SpatialAudioMode = ElementSpatialAudioMode.Off;
                    break;

                    case SetSpatialAudioMode.On:
                        ElementSoundPlayer.SpatialAudioMode = ElementSpatialAudioMode.On;
                    break;
                }
            });

            Log.Comment("Hooking up PlayingSoundNodeCallback.");
            TestServices.WindowHelper.SetPlayingSoundNodeCallback(playingSoundCallback);

            Log.Comment("Focusing btnButton.");
            FocusHelper.EnsureFocus(btnButton, FocusState.Pointer);
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(ElementSoundPlayer.State, ElementSoundPlayerState.On);
                Verify.AreEqual(ElementSoundPlayer.Volume, 0.8);
            });

            Log.Comment("Tabbing 6 times to visit all focusable top level elements and play focus sound.");
            for (int tab = 0; tab < 6; tab++)
            {
                TestServices.KeyboardHelper.Tab();
                TestServices.WindowHelper.WaitForIdle();
            }

            UIExecutor.Execute(() =>
            {
                Log.Comment("Showing attached Flyout.");
                Flyout.ShowAttachedFlyout(btnAttachedFlyoutButton);
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Discarding attached Flyout.");
            TestServices.KeyboardHelper.Escape();
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Changing rdRadioButtonInAttachedFlyout.ElementSoundMode from Default to FocusOnly.");
                Verify.AreEqual(rdRadioButtonInAttachedFlyout.ElementSoundMode, ElementSoundMode.Default);
                rdRadioButtonInAttachedFlyout.ElementSoundMode = ElementSoundMode.FocusOnly;
                Verify.AreEqual(rdRadioButtonInAttachedFlyout.ElementSoundMode, ElementSoundMode.FocusOnly);

                Log.Comment("Changing btnAttachedFlyoutButton.ElementSoundMode from Default to Off.");
                Verify.AreEqual(btnAttachedFlyoutButton.ElementSoundMode, ElementSoundMode.Default);
                btnAttachedFlyoutButton.ElementSoundMode = ElementSoundMode.Off;
                Verify.AreEqual(btnAttachedFlyoutButton.ElementSoundMode, ElementSoundMode.Off);

                Log.Comment("Showing attached Flyout again.");
                Flyout.ShowAttachedFlyout(btnAttachedFlyoutButton);
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Tabbing among Flyout controls rdRadioButtonInAttachedFlyout and chkCheckBoxInAttachedFlyout.");
            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();
            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Discarding attached Flyout again.");
            TestServices.KeyboardHelper.Escape();
            TestServices.WindowHelper.WaitForIdle();

            TestServices.WindowHelper.SetPlayingSoundNodeCallback(null);

            new AutoResetEvent(false).WaitOne(500);

            LogAppMemoryUsage();
        }

        [TestMethod]
        [TestProperty("Description", "Validates that a request to play a sound while the audio graph is still being loaded results in delayed play.")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        public void PlaySoundAtStartup()
        {
            const string rootTextBlockXaml =
                @"<TextBlock xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Text='Testing ElementSoundPlayer.Play'/>";

            AutoResetEvent playingSoundNodeEvent = new AutoResetEvent(false);
            PlayingSoundNodeCallback playingSoundCallback = null;

            UIExecutor.Execute(() =>
            {
                Log.Comment("Loading UI.");
                Window.Current.Content = XamlReader.Load(rootTextBlockXaml) as TextBlock;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                ElementSoundPlayer.SpatialAudioMode = ElementSpatialAudioMode.On;
            });

            UIExecutor.Execute(() =>
            {
                Log.Comment("Enabling element sound playing.");
                ElementSoundPlayer.State = ElementSoundPlayerState.On;
                ElementSoundPlayer.Volume = 0.8;

                Log.Comment("Creating PlayingSoundNodeCallback.");
                playingSoundCallback = new PlayingSoundNodeCallback((ElementSoundKind sound, bool isSpatialAudioSound, float x, float y, float z, double volume) =>
                {
                    Log.Comment("PlayingSoundNodeCallback: ElementSoundKind: {0}, isSpatialSound: {1}, Emitter.x: {2}, Emitter.y: {3}, Emitter.z: {4}, Volume: {5}", 
                            sound, 
                            isSpatialAudioSound, 
                            (decimal)x, 
                            (decimal)y, 
                            (decimal)z,
                            (decimal)volume);

                    Verify.AreEqual(sound, ElementSoundKind.Focus);
                    playingSoundNodeEvent.Set();
                });
            });

            Log.Comment("Hooking up PlayingSoundNodeCallback.");
            TestServices.WindowHelper.SetPlayingSoundNodeCallback(playingSoundCallback);

            UIExecutor.Execute(() =>
            {
                Log.Comment("Requesting to play Got-Focus sound.");
                ElementSoundPlayer.Play(ElementSoundKind.Focus);
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Waiting for PlayingSoundNodeCallback.");
            Verify.IsTrue(playingSoundNodeEvent.WaitOne(TimeSpan.FromSeconds(5)), "Sound node was played.");

            Log.Comment("Resetting PlayingSoundNodeCallback.");
            TestServices.WindowHelper.SetPlayingSoundNodeCallback(null);
            
            new AutoResetEvent(false).WaitOne(500);

            LogAppMemoryUsage();
        }


        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void TestButtonPanelWithSpatialAudio() 
        {
            SoundData[] soundData = new SoundData[]
            {
                new SoundData(ElementSoundKind.Focus, true, 0, 1.016667, -3, .8),
                new SoundData(ElementSoundKind.Focus, true, -1.953125, 0, -3, .8),
                new SoundData(ElementSoundKind.Focus, true, 0, 0, -3, .8),
                new SoundData(ElementSoundKind.Focus, true, 1.941667, 0, -3, .8),
                new SoundData(ElementSoundKind.Focus, true, 0, -1.016667, -3, .8)
            };

            ButtonPanel(true /* isSpatialAudioMode */, soundData);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void TestButtonPanelWithoutSpatialAudio() 
        {
            SoundData[] soundData = new SoundData[]
            {
                new SoundData(ElementSoundKind.Focus, false, 0, 0, -3, .8),
                new SoundData(ElementSoundKind.Focus, false, 0, 0, -3, .8),
                new SoundData(ElementSoundKind.Focus, false, 0, 0, -3, .8),
                new SoundData(ElementSoundKind.Focus, false, 0, 0, -3, .8),
                new SoundData(ElementSoundKind.Focus, false, 0, 0, -3, .8),
            };

            ButtonPanel(false/* isSpatialAudioMode */, soundData);
        }

        public void ButtonPanel(bool isSpatialAudioModeEnabled, SoundData[] soundData) 
        {
            const string rootPanelXaml =
                @"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='btnTopCenter' Content='TopCenter' VerticalAlignment='Top' HorizontalAlignment='Center'/>
                        <Button x:Name='btnLeft' Content='Left' HorizontalAlignment='Left'/>
                        <Button x:Name='btnCenter' Content='Center' VerticalAlignment='Center' HorizontalAlignment='Center'/>
                        <Button x:Name='btnRight' Content='Right' HorizontalAlignment='Right'/>
                        <Button x:Name='btnBottom' Content='BottomCenter' VerticalAlignment='Bottom' HorizontalAlignment='Center'/>
                </Grid>";

            // ::::::::::::::::::::::::::::::::::::::::::::::::::
            //      Load the Xaml panel 
            // ::::::::::::::::::::::::::::::::::::::::::::::::::

            Grid rootPanel = null;

            UIExecutor.Execute(() =>
            {
                Log.Comment("Loading UI.");
                rootPanel = XamlReader.Load(rootPanelXaml) as Grid;
                Window.Current.Content = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            // ::::::::::::::::::::::::::::::::::::::::::::::::::
            //      Enable ElementSoundPlayer sound
            // ::::::::::::::::::::::::::::::::::::::::::::::::::
            
            UIExecutor.Execute(() =>
            {
                Log.Comment("Enabling element sound playing.");
                ElementSoundPlayer.State = ElementSoundPlayerState.On;
                ElementSoundPlayer.Volume = 0.8;
            });

            // ::::::::::::::::::::::::::::::::::::::::::::::::::
            //      Set up test callback 
            // ::::::::::::::::::::::::::::::::::::::::::::::::::
            
            int currentSoundIndex = 0; 
            PlayingSoundNodeCallback playingSoundCallback = null;

            UIExecutor.Execute(() =>
            {
                Log.Comment("Creating PlayingSoundNodeCallback.");
                playingSoundCallback = new PlayingSoundNodeCallback((ElementSoundKind sound, bool isSpatialAudioSound, float x, float y, float z, double volume) =>
                {
                    Log.Comment("PlayingSoundNodeCallback Actual: ElementSoundKind: {0}, isSpatialSound: {1}, Emitter.x: {2}, Emitter.y: {3}, Emitter.z: {4}, Volume: {5}", 
                            sound, 
                            isSpatialAudioSound, 
                            (decimal)x, 
                            (decimal)y, 
                            (decimal)z,
                            (decimal)volume);

                    Log.Comment("PlayingSoundNodeCallback Expected: ElementSoundKind: {0}, isSpatialSound: {1}, Emitter.x: {2}, Emitter.y: {3}, Emitter.z: {4}", 
                            soundData[currentSoundIndex].elementSoundKind, 
                            soundData[currentSoundIndex].isSpatialAudioSound, 
                            (decimal)soundData[currentSoundIndex].x, 
                            (decimal)soundData[currentSoundIndex].y, 
                            (decimal)soundData[currentSoundIndex].z,
                            (decimal)soundData[currentSoundIndex].volume);

                    if (soundData != null)
                    {
                        Verify.AreEqual(sound, soundData[currentSoundIndex].elementSoundKind);
                        Verify.AreEqual(isSpatialAudioSound, soundData[currentSoundIndex].isSpatialAudioSound);
                        
                        currentSoundIndex++;
                    }
                });
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Hooking up PlayingSoundNodeCallback.");
            TestServices.WindowHelper.SetPlayingSoundNodeCallback(playingSoundCallback);

            // ::::::::::::::::::::::::::::::::::::::::::::::::::
            //      Set SpatialAudio Mode 
            // ::::::::::::::::::::::::::::::::::::::::::::::::::
            
            UIExecutor.Execute(() =>
            {
                Log.Comment("Setting Spatial Audio Mode to {0}...", isSpatialAudioModeEnabled);
                if (isSpatialAudioModeEnabled)
                {
                    ElementSoundPlayer.SpatialAudioMode = ElementSpatialAudioMode.On;
                }
                else
                {
                    ElementSoundPlayer.SpatialAudioMode = ElementSpatialAudioMode.Off;
                }
            });

            LogAppMemoryUsage();

            new AutoResetEvent(false).WaitOne(1000);

            // ::::::::::::::::::::::::::::::::::::::::::::::::::
            //      Tab through buttons  
            // ::::::::::::::::::::::::::::::::::::::::::::::::::
            
            // This tests the round-robin focus sounds
            Log.Comment("Tabbing through buttons...");
            for (int tab = 0; tab < 5; tab++)
            {
                TestServices.KeyboardHelper.Tab();
                TestServices.WindowHelper.WaitForIdle();
            }

            new AutoResetEvent(false).WaitOne(500);

            LogAppMemoryUsage();

            new AutoResetEvent(false).WaitOne(500);
        }

        private static void LogAppMemoryUsage()
        {   
            var memoryReport = global::Windows.System.MemoryManager.GetProcessMemoryReport();
            Log.Comment("PrivateWorkingSetUsage: {0}", memoryReport.PrivateWorkingSetUsage);
            Log.Comment("TotalWorkingSetUsage: {0}", memoryReport.TotalWorkingSetUsage);
            Log.Comment("AppMemoryUsage: {0}", global::Windows.System.MemoryManager.AppMemoryUsage);
            Log.Comment("AppMemoryUsageLimit: {0}", global::Windows.System.MemoryManager.AppMemoryUsageLimit);
        }

    }

}
