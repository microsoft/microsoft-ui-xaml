// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Tests.Common;
using Microsoft.UI.Xaml;

namespace Microsoft.UI.Xaml.Tests.Framework.ComponentConnector
{
    public class ComponentConnectorTestHelper : IDisposable
    {
        #region IDisposable Support

        private bool disposedValue = false; // To detect redundant calls
        private bool isDisposing = false;
        public ComponentConnectorTestHelper()
        {
            TestServices.WindowHelper.InitializeXaml();
        }

        private void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                this.isDisposing = disposing;
                if (disposing)
                {
                    UIExecutor.Execute(() =>
                    {
                        TestServices.WindowHelper.WindowContent = null;
                    });
                    TestServices.WindowHelper.WaitForIdle();

                    TestServices.WindowHelper.ShutdownXaml();
                    TestServices.WindowHelper.VerifyTestCleanup();
                }
                disposedValue = true;
            }
        }

        public uint ControlTemplateConnectedCount{get; private set;}
        public bool ControlTemplateConnected { get { return ControlTemplateConnectedCount > 0;}}
        public uint GetBindingConnectorCount{get; set;}
        public bool GetBindingConnectorCalled { get { return GetBindingConnectorCount > 0;}}
        public bool TemplatedChildrenConnected {get; private set;}

        public void SetupTest(bool testingInvalidScenario = false)
        {
            Action<int, object> bindingsConnect = (id, target) => {
                Log.Comment($"Connect: {id}");
                switch (id)
                {
                    case 1:
                        Verify.IsNotNull(target as ControlTemplate);
                        ControlTemplateConnectedCount++;
                        break;
                    case 4:
                        TemplatedChildrenConnected = true;
                        break;
                    case 2:
                        TemplatedChildrenConnected = true;
                        break;
                    case 6:
                        Log.Error("Template children in ControlTemplate without x:ConnectionId should not get Connect");
                        break;
                    case 7:
                        Log.Error("Template children in ControlTemplate without x:ConnectionId should not get Connect");
                        break;
                    default:
                        break;
                }

            };

            Func<int, object, IComponentConnector> getBindingConnector = (id, target) => {
                Log.Comment($"GetBindingConnector: {id}");
                IComponentConnector returnValue = null;
                switch (id)
                {
                    case 1:
                        if (target is Button buttonTarget)
                        {
                            Log.Comment("Templated parent connected");
                            GetBindingConnectorCount++;
                            CustomTypes.MainPageBindings bindings = null;
                            if (!testingInvalidScenario)
                            {
                                bindings = new CustomTypes.MainPageBindings( bindingsConnect);
                            }
                            returnValue = bindings;
                            Microsoft.UI.Xaml.Markup.XamlBindingHelper.SetDataTemplateComponent(buttonTarget, bindings);
                        }
                        break;
                    case 5:
                        {
                            var bindings = new CustomTypes.MainPageBindings(bindingsConnect);
                            returnValue = bindings;
                            Microsoft.UI.Xaml.Markup.XamlBindingHelper.SetDataTemplateComponent((DependencyObject)target, bindings);
                        }
                        break;
                    default:
                        break;
                }
                return returnValue;
            };

            UIExecutor.Execute(() =>
            {
                var page = new CustomTypes.MainPage(getBindingConnector);
                Application.LoadComponent(
                    page,
                    new Uri("ms-appx:///resources/managed/framework/componentconnector/MainPage.xaml"),
                    Microsoft.UI.Xaml.Controls.Primitives.ComponentResourceLocation.Application);
                TestServices.WindowHelper.WindowContent = page;
            });

            TestServices.WindowHelper.WaitForIdle();
        }

        // This code added to correctly implement the disposable pattern.
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(true);
            // TODO: uncomment the following line if the finalizer is overridden above.
            // GC.SuppressFinalize(this);
        }

        #endregion
    }
    [TestClass]
    public class ComponentConnectorIntegrationTests : XamlTestsBase
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
        [TestProperty("Hosting:Mode", "UAP")]
        public void ValidateConnectCallsOnControlTemplates()
        {
            using (var helper = new ComponentConnectorTestHelper())
            {
                // Leaky test, leak detection disabled
                TestServices.ErrorHandlingHelper.IgnoreLeaksForTest();
                helper.SetupTest();
                Verify.IsTrue(helper.GetBindingConnectorCalled);
                Verify.IsTrue(helper.ControlTemplateConnected);
                Verify.IsTrue(helper.TemplatedChildrenConnected);
            }
        }

        [TestMethod]
        public void ValidateReuseOfExistingBindingConnector()
        {
            using (var helper = new ComponentConnectorTestHelper())
            {
                // Leaky test, leak detection disabled
                TestServices.ErrorHandlingHelper.IgnoreLeaksForTest();
                helper.SetupTest();
                Verify.IsTrue(helper.GetBindingConnectorCalled);
                Verify.IsTrue(helper.ControlTemplateConnected);

                Verify.IsTrue(1 == helper.GetBindingConnectorCount);
                Verify.IsTrue(1 == helper.ControlTemplateConnectedCount);

                ControlTemplate template = null;
                UIExecutor.Execute(() =>
                {
                    var page = (Page)TestServices.WindowHelper.WindowContent;
                    var button = (Button)page.FindName("button");
                    
                    template = button.Template;
                    button.Template = null;
                });
                TestServices.WindowHelper.WaitForIdle();
                Verify.IsTrue(1 == helper.GetBindingConnectorCount);
                Verify.IsTrue(1 == helper.ControlTemplateConnectedCount);

                UIExecutor.Execute(() =>
                {
                    var page = (Page)TestServices.WindowHelper.WindowContent;
                    var button = (Button)page.FindName("button");
                    
                    button.Template = template;
                    template = null;
                });
                TestServices.WindowHelper.WaitForIdle();
                Verify.IsTrue(1 == helper.GetBindingConnectorCount);
                Verify.IsTrue(2 == helper.ControlTemplateConnectedCount);
            }
        }

        [TestMethod]
        public void DontCrashOnInvalidDataTemplateComponent()
        {
            using (var helper = new ComponentConnectorTestHelper())
            {
                // Leaky test, leak detection disabled
                TestServices.ErrorHandlingHelper.IgnoreLeaksForTest();
                helper.SetupTest(testingInvalidScenario: true);

                ControlTemplate template = null;
                UIExecutor.Execute(() =>
                {
                    var page = (Page)TestServices.WindowHelper.WindowContent;
                    var button = (Button)page.FindName("button");
                    
                    template = button.Template;
                    button.Template = null;
                });
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    var page = (Page)TestServices.WindowHelper.WindowContent;
                    var button = (Button)page.FindName("button");
                    button.Template = template;
                    template = null;
                });

                TestServices.WindowHelper.WaitForIdle();
                Log.Comment("Verify dont crash");
            }
        }
    }
}
