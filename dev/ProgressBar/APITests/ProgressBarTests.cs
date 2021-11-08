// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;
using Microsoft.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Automation.Provider;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Automation;
using Windows.UI.Xaml.Shapes;
using Windows.UI.Xaml.Markup;
using Windows.UI.Xaml.Controls;
#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class ProgressBarTests : ApiTestBase
    {
        [TestMethod]
        public void ResourceOverridablity()
        {
            RunOnUIThread.Execute(() =>
            {
                var root = (StackPanel)XamlReader.Load(
    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' 
                             xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                             xmlns:primitives='using:Microsoft.UI.Xaml.Controls.Primitives'
                             xmlns:controls='using:Microsoft.UI.Xaml.Controls'> 
                             <Grid>
                                <Grid.Resources>
                                    <x:Double x:Key='ProgressBarTrackHeight'>3</x:Double>
                                </Grid.Resources>
                                <controls:ProgressBar/>
                            </Grid>
                            <controls:ProgressBar/>
                       </StackPanel>");

                Content = root;
                Content.UpdateLayout();

                var grid = VisualTreeHelper.GetChild(root, 0);
                var progressBar1 = VisualTreeHelper.GetChild(grid, 0);
                var templateRoot1 = VisualTreeHelper.GetChild(progressBar1, 0);
                var Border11 = VisualTreeHelper.GetChild(templateRoot1, 0);
                var Border12 = VisualTreeHelper.GetChild(Border11, 0);
                var grid1 = VisualTreeHelper.GetChild(Border12, 0);
                var rect1 = VisualTreeHelper.GetChild(grid1, 0) as Rectangle;
                Verify.AreEqual(3, rect1.Height);

                var progressBar2 = VisualTreeHelper.GetChild(root, 1);
                var templateRoot2 = VisualTreeHelper.GetChild(progressBar2, 0);
                var Border21 = VisualTreeHelper.GetChild(templateRoot2, 0);
                var Border22 = VisualTreeHelper.GetChild(Border21, 0);
                var grid2 = VisualTreeHelper.GetChild(Border22, 0);
                var rect2 = VisualTreeHelper.GetChild(grid2, 0) as Rectangle;
                Verify.AreEqual(1, rect2.Height);
            });
        }
    }
}
