// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Controls.FlipView
{
    [TestClass]
    public partial class FlipViewIntegrationTests : XamlTestsBase
    {
        
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        public void CanResetIncrementalLoadingFlipView()
        {
            Xaml.Controls.FlipView flipView = null;

            UIExecutor.Execute(() =>
            {
                var data = new InfiniteData();
                data.LoadMoreItemsAsync(20).AsTask().Wait();

                flipView = new Xaml.Controls.FlipView();
                flipView.ItemsSource = data;
                flipView.SelectedIndex = 1;
                TestServices.WindowHelper.WindowContent = flipView;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var data = new InfiniteData();
                Verify.AreEqual(1, flipView.SelectedIndex);
                flipView.ItemsSource = data = new InfiniteData();
                Verify.AreEqual(-1, flipView.SelectedIndex);
                flipView.UpdateLayout();
                data.LoadMoreItemsAsync(18).AsTask().Wait();
                Verify.AreEqual(0, flipView.SelectedIndex);
            });
        }

        private partial class InfiniteData : IncrementalLoadingBase
        {
            protected override bool HasMoreItemsOverride()
            {
                return true;
            }

            protected override Task<IList<object>> LoadMoreItemsOverrideAsync(CancellationToken c, uint count)
            {
                return Task.FromResult((IList<object>)new List<object>(Enumerable.Range(0, 100).Select(i => "Item #" + i.ToString())));
            }
        }
    }
}
