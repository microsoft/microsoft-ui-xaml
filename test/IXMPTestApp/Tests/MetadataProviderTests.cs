// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.ApplicationModel.Core;
using Windows.UI.Xaml.Markup;
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

namespace IXMPTestApp.Tests
{
    [TestClass]
    public class MetadataProviderTests
    {
        [TestMethod]
        public void CanLoadXamlFragments()
        {
            var dispatcher = CoreApplication.MainView.Dispatcher;
            dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
            {
                Log.Comment("Loading RefreshContainer...");
                XamlReader.Load(@"
                    <controls:RefreshContainer
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                        xmlns:controls='using:Microsoft.UI.Xaml.Controls'
                        PullDirection='TopToBottom'>
                            <ListView x:Name='lv' />
                    </controls:RefreshContainer>");

                Log.Comment("Loading ColorPicker...");
                XamlReader.Load(@"
                    <controls:ColorPicker
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                        xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                    </controls:ColorPicker>");

                Log.Comment("Loading ParallaxView...");
                XamlReader.Load(@"
                    <controls:ParallaxView
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                        xmlns:controls='using:Microsoft.UI.Xaml.Controls'
                        Name='ParallaxView'>
                        <!-- Comment -->
                        <Rectangle Width='100' Height='100' Fill='Red' />
                    </controls:ParallaxView>");


                // This test case is disabled in Debug configuration due to:
                // Bug #1725: RecyclingElementFactory.Templates cannot be set from Xaml on RS4 and below in debug configuration (using reflection provider) 
                if (!PlatformConfiguration.IsDebugBuildConfiguration())
                {
                    Log.Comment("Loading ItemsRepeater...");
                    XamlReader.Load(@"
                        <controls:ItemsRepeaterScrollHost
                            xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                            xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                            <ScrollViewer>
                                <controls:ItemsRepeater Name='repeater'>
                                    <controls:ItemsRepeater.Layout>
                                        <controls:StackLayout Orientation='Vertical' Spacing='10' />
                                    </controls:ItemsRepeater.Layout>
                                    <controls:ItemsRepeater.ItemTemplate>
                                        <controls:RecyclingElementFactory>
                                            <controls:RecyclingElementFactory.RecyclePool>
                                                <controls:RecyclePool />
                                            </controls:RecyclingElementFactory.RecyclePool>
                                            <DataTemplate x:Key='Primary'>
                                                <TextBlock Text='{Binding}' />
                                            </DataTemplate>
                                        </controls:RecyclingElementFactory>
                                    </controls:ItemsRepeater.ItemTemplate>
                                </controls:ItemsRepeater>
                            </ScrollViewer>
                        </controls:ItemsRepeaterScrollHost>");
                }

                Log.Comment("Loading SwipeControl...");
                XamlReader.Load(@"
                    <controls:SwipeControl
                        Background='Yellow'
                        Width='300'
                        Height='100'
                        Margin='10'
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                        xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                        <controls:SwipeControl.Resources>
                            <controls:SymbolIconSource x:Key='SymbolIcon' Symbol='Camera'/>
                            <controls:FontIconSource x:Key='FontIcon' Glyph='&#xE115;' FontWeight='Normal'/>
                            <controls:PathIconSource x:Key='PathIcon' Data='F1 M 16,12 20,2L 20,16 1,16'/>
                            <controls:BitmapIconSource x:Key='BitmapIcon' UriSource='ms-appx:///Assets/StoreLogo.png' ShowAsMonochrome='False' />
                        </controls:SwipeControl.Resources>
                        <controls:SwipeControl.LeftItems>
                            <controls:SwipeItems Mode='Reveal'>
                                <controls:SwipeItem Text='Settings' IconSource='{StaticResource SymbolIcon}' Background='Gray' />
                                <controls:SwipeItem Text='Video' IconSource='{StaticResource FontIcon}' Background='Green' />
                                <controls:SwipeItem Text='Picture' IconSource='{StaticResource BitmapIcon}' Background='Yellow' />
                            </controls:SwipeItems>
                        </controls:SwipeControl.LeftItems>
                        <controls:SwipeControl.RightItems>
                            <controls:SwipeItems Mode='Reveal'>
                                <controls:SwipeItem Text='Delete' IconSource='{StaticResource PathIcon}' Background='Red' />
                            </controls:SwipeItems>
                        </controls:SwipeControl.RightItems>
                        <TextBlock x:Name='tb' Text='Swipe Me (but only left or right)'/>
                    </controls:SwipeControl>");

                Log.Comment("Loading TreeView...");
                XamlReader.Load(@"
                    <controls:TreeView
                        x:Name='myTreeView'
                        AllowDrop='True'
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                        xmlns:controls='using:Microsoft.UI.Xaml.Controls' />");

                Log.Comment("Loading RatingControl...");
                XamlReader.Load(@"
                    <controls:RatingControl
                        Caption='A B C D'
                        MaxRating='12'
                        xmlns ='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                        xmlns:controls='using:Microsoft.UI.Xaml.Controls' />");

                Log.Comment("Loading NavigationView...");
                XamlReader.Load(@"
                    <controls:NavigationView
                        x:Name='NavView'
                        AutomationProperties.Name='NavView'
                        AutomationProperties.AutomationId='NavView'
                        HorizontalAlignment='Left'
                        VerticalAlignment='Top'
                        Header='Home'
                        CompactModeThresholdWidth='480'
                        xmlns ='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                        xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                        <controls:NavigationView.MenuItems>
                            <controls:NavigationViewItem x:Name='HomeItem' Content='Home' Icon='Home' />
                            <controls:NavigationViewItem x:Name='AppsItem' Content='Apps' Icon='Shop' />
                            <controls:NavigationViewItem x:Name='GamesItem' Content='Games' Icon='Emoji' />
                            <controls:NavigationViewItemSeparator />
                            <controls:NavigationViewItemHeader Content='Header Text' />
                            <controls:NavigationViewItem x:Name='MusicItem' Content='Music' Icon='Audio' />
                            <controls:NavigationViewItem x:Name='MoviesItem' Content='Movies' Icon='Video' />
                            <controls:NavigationViewItem x:Name='TVItem' Content='TV' Icon='Slideshow' />
                        </controls:NavigationView.MenuItems>
                        <TextBlock HorizontalAlignment='Center' VerticalAlignment='Center' Text='Content'/>
                    </controls:NavigationView>");

                Log.Comment("Loading PersonPicture...");
                XamlReader.Load(@"<controls:PersonPicture 
                                    x:Name='personPicture'
                                    BadgeNumber='1234'
                                    xmlns ='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                    xmlns:controls='using:Microsoft.UI.Xaml.Controls'
                                    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'/>");

            }).AsTask().Wait();
        }
    }
}
