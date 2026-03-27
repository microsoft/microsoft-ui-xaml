// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.IO;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Controls.SampleIntegrationTests
{
    [TestClass]    
    public class SampleIntegrationTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("__ExecutionUnit", "b914a3e3-0d78-4274-88b9-46a4773bb21b;a69ddfa4-5142-4bed-887d-6d0ca14a3473")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
            TestDeploymentDir = context.TestDeploymentDir;
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]        
        [TestProperty("Description", "Validates that we can create a Grid instance.")]
        public void CanCreateAGrid()
        {
            UIExecutor.Execute(() =>
            {
                var grid = new Grid();
                Verify.IsNotNull(grid);

                var gridFromParser = XamlReader.Load(@"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'/>"); ;
                Verify.IsNotNull(gridFromParser);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Validates that we can load a xaml file.")]
        public void CanLoadAXamlFile()
        {
            UIExecutor.Execute(() =>
            {
                string xamlText = File.ReadAllText(TestDeploymentDir + @"\resources\managed\controls\sample\sample.xaml");
                Grid objFromFile = (Grid)XamlReader.Load(xamlText);
                Verify.IsNotNull(objFromFile);
                TestServices.WindowHelper.WindowContent = objFromFile;
            });

            TestServices.WindowHelper.WaitForIdle();
        }

        private static string TestDeploymentDir = string.Empty;

    }
}
