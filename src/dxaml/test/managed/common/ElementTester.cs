// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Threading.Tasks;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Windows.Foundation;
using Windows.Graphics.Imaging;
using Windows.UI.Composition;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Private.Infrastructure;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.Common
{
    public abstract class ElementTester<TElement>
        where TElement: class
    {
        protected ElementTester()
        {
            this.Background = "Red";
            this.GridHeight = "Auto";
            this.GridWidth = "Auto";
            this.DCompRendering = DCompRendering.WUCCompleteSynchronousCompTree;
            this.AsyncTestActions = new List<Func<ElementTester<TElement>, Task>>();
        }

        public DCompRendering DCompRendering { get; set; }

        public Func<IDisposable> PreSetupAction { get; set; }
        public Func<TElement, IDisposable> SetupAction { get; set; }
        public List<Func<ElementTester<TElement>, Task>> AsyncTestActions { get; private set; }
        public Action<TElement> TearDownAction { get; set; }

        public TElement Element { get; set; }

        public string GridHeight { get; set; }
        public string GridWidth { get; set; }
        public string Background { get; set; }

        private TElement CreateElement(string elementXaml, string name="theElement")
        {
            var element = default(TElement);
            Grid grid = null;

            UIExecutor.Execute(() =>
            {
                StringBuilder sb = new StringBuilder();
                sb.AppendLine($"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'");
                sb.AppendLine($"  Background='{Background}'");
                sb.AppendLine($"  >");
                sb.AppendLine($"  <Grid.ColumnDefinitions>");
                sb.AppendLine($"    <ColumnDefinition Width='{GridWidth}'/>");
                sb.AppendLine($"  </Grid.ColumnDefinitions>");
                sb.AppendLine($"  <Grid.RowDefinitions>");
                sb.AppendLine($"    <RowDefinition Height='{GridHeight}'/>");
                sb.AppendLine($"  </Grid.RowDefinitions>");
                sb.AppendLine(elementXaml);
                sb.AppendLine("</Grid>");
                grid = (Grid)Markup.XamlReader.Load(sb.ToString());

                TestServices.WindowHelper.WindowContent = grid;
                element = grid.FindName(name) as TElement;
                Verify.IsNotNull(element);
            });

            TestServices.WindowHelper.WaitForIdle();

            return element;
        }

        protected virtual IEnumerable<Func<ElementTester<TElement>, Task>> DefaultActions
        {
            get
            {
                return Enumerable.Empty<Func<ElementTester<TElement>, Task>>();
            }
        }

        protected virtual void ConfigureElement(StringBuilder sb)
        {
        }

        private TElement CreateElement()
        {
            var sb = new StringBuilder();
            sb.AppendLine($"<{typeof(TElement).Name} x:Name='theElement' ");

            this.ConfigureElement(sb);

            sb.AppendLine("/>");

            return CreateElement(sb.ToString());
        }

        protected virtual TElement Setup()
        {
            var element = this.CreateElement();

            TestServices.WindowHelper.WaitForIdle();

            return element;
        }

        protected virtual Task BeforeTestActions()
        {
            return Task.FromResult(0);
        }

        protected virtual Task AfterTestActions()
        {
            return Task.FromResult(0);
        }

        public async Task DoTest()
        {
            var preSetupResource = default(IDisposable);
            if (this.PreSetupAction != null)
            {
                preSetupResource = this.PreSetupAction();
            }

            using (preSetupResource)
            using (TestServices.Utilities.CreateRenderingScopeGuard(
                this.DCompRendering,
                resizeWindow: false,
                injectMockDComp: false,
                resetDevice: false /* If we reset device the next test after WUCRedenring will assert with a DComp leak */ ))
            using (new TestCleanupWrapper())
            {
                this.Element = this.Setup();

                var tearDownObject = default(IDisposable);
                if (this.SetupAction != null)
                {
                    tearDownObject = this.SetupAction(this.Element);
                }

                using(tearDownObject)
                {
                    await this.BeforeTestActions();

                    if (!this.AsyncTestActions.Any())
                    {
                        this.AsyncTestActions.AddRange(this.DefaultActions);
                    }
                    foreach(var asyncAction in this.AsyncTestActions)
                    {
                        await asyncAction(this);
                    }

                    await this.AfterTestActions();

                    if (this.TearDownAction != null)
                    {
                        this.TearDownAction(this.Element);
                    }
                }

                this.Element = null;
            }
        }
    }
}
