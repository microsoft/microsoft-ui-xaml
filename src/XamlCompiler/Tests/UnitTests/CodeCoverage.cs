// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.IO;
using System.Text;
using Win8Xaml.CompilerProxies;

// Tests added just to get into hard to reach code coverage places.

namespace UnitTests
{
    [TestClass]
    public class CodeCoverage
    {
        TestHelper _testHelper;

        [TestInitialize]
        public void SchemaInit()
        {
            _testHelper = new TestHelper();
        }

        /// <summary>
        /// This test is here to execute the ReadXClassFromXamlFileStream
        /// method in the XAML compiler to increase code coverage.
        /// It also validates that this (internal) method works.
        /// </summary>
        [TestMethod]
        public void ReadXClassFromXamlFileStream()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    x:Class='MyStuff.MyPage'>
    <Grid>
        <Button x:Name='btn' Click='ClickHdlr' />
    </Grid>
</Page>";

            Stream str = new MemoryStream(Encoding.UTF8.GetBytes(xaml));
            StreamReader rdr = new StreamReader(str); 
            
            DirectUISchemaContext schema = _testHelper.LoadSchema(SchemaMode.ManagedRuntime);

            string className = XamlNodeStreamHelper.ReadXClassFromXamlFileStream(rdr, schema.Instance);

            Assert.AreEqual("MyStuff.MyPage", className);
        }


    }
}
