// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using Win8Xaml.CompilerProxies;

namespace UnitTests
{
    [TestClass]
    public class MultipleViewTests
    {
        TestHelper _testHelper;

        [TestInitialize]
        public void SchemaInit()
        {
            _testHelper = new TestHelper();
        }

        private void TestMultipleViewPathLogic(string[] apparentRelativePaths, string expectedBaseApparentRelativeFolder, string expectedBaseFileName)
        {
            XamlClassCodeInfo classCodeInfo = new XamlClassCodeInfo("MyNamespace.DifferentClassName", false);

            foreach (string apparentRelativePath in apparentRelativePaths)
            {
                XamlFileCodeInfo fileCodeInfo = new XamlFileCodeInfo();
                fileCodeInfo.ApparentRelativePath = apparentRelativePath;
                classCodeInfo.AddXamlFileInfo(fileCodeInfo);
            }

            Assert.AreEqual(classCodeInfo.BaseApparentRelativeFolder, expectedBaseApparentRelativeFolder, "BaseApparentRelativeFolder mismatch");
            Assert.AreEqual(classCodeInfo.BaseFileName, expectedBaseFileName, "BaseFileName mismatch");
        }

        private void TestMultipleViewPathLogicBaseError(string[] apparentRelativePaths)
        {
            string errorMessage = null;

            try
            {
                this.TestMultipleViewPathLogic(apparentRelativePaths, "", "");
            }
            catch (Exception ex)
            {
                errorMessage = ex.Message;
            }
            Assert.IsNotNull(errorMessage, "Mismatched base filenames should have caused an error");
        }

        [TestMethod]
        public void MultipleView_BaseCase()
        {
            this.TestMultipleViewPathLogic(new string[] { "MainPage.xaml" }, "", "MainPage");
        }

        [TestMethod]
        public void MultipleView_BaseCaseInFolder()
        {
            this.TestMultipleViewPathLogic(new string[] { "Views\\MainPage.xaml" }, "Views", "MainPage");
        }

        [TestMethod]
        public void MultipleView_DottedQualifiers()
        {
            this.TestMultipleViewPathLogic(new string[] { "MainPage.xaml", "MainPage.Platform-Mobile.xaml" }, "", "MainPage");
        }

        [TestMethod]
        public void MultipleView_DottedQualifiersRootAbsent()
        {
            this.TestMultipleViewPathLogic(new string[] { "MainPage.Platform-Desktop.xaml", "MainPage.Platform-Mobile.xaml" }, "", "MainPage");
        }

        [TestMethod]
        public void MultipleView_FolderQualifiers()
        {
            this.TestMultipleViewPathLogic(new string[] { "MainPage.xaml", "Platform-Mobile\\MainPage.xaml" }, "", "MainPage");
        }

        [TestMethod]
        public void MultipleView_FolderQualifiersRootAbsent()
        {
            this.TestMultipleViewPathLogic(new string[] { "Platform-Desktop\\MainPage.xaml", "Platform-Mobile\\MainPage.xaml" }, "", "MainPage");
        }

        [TestMethod]
        public void MultipleView_DottedQualifiersInSubfolder()
        {
            this.TestMultipleViewPathLogic(new string[] { "Views\\MainPage.xaml", "Views\\MainPage.Platform-Mobile.xaml" }, "Views", "MainPage");
        }

        [TestMethod]
        public void MultipleView_DottedQualifiersInSubfolderRootAbsent()
        {
            this.TestMultipleViewPathLogic(new string[] { "Views\\MainPage.Platform-Desktop.xaml", "Views\\MainPage.Platform-Mobile.xaml" }, "Views", "MainPage");
        }

        [TestMethod]
        public void MultipleView_FolderQualifiersInSubfolder()
        {
            this.TestMultipleViewPathLogic(new string[] { "Views\\MainPage.xaml", "Views\\Platform-Mobile\\MainPage.xaml" }, "Views", "MainPage");
        }

        [TestMethod]
        public void MultipleView_FolderQualifiersInSubfolderRootAbsent()
        {
            this.TestMultipleViewPathLogic(new string[] { "Views\\Platform-Desktop\\MainPage.xaml", "Views\\Platform-Mobile\\MainPage.xaml" }, "Views", "MainPage");
        }

        [TestMethod]
        public void MultipleView_ItemFileNameWithDots()
        {
            this.TestMultipleViewPathLogic(new string[] { "Microsoft.VisualStudio.MainPage.xaml", "Microsoft.VisualStudio.MainPage.Portrait.xaml" }, "", "Microsoft.VisualStudio.MainPage");
        }

        [TestMethod]
        public void MultipleView_NonmatchingBaseRootFolder()
        {
            this.TestMultipleViewPathLogicBaseError(new string[] { "MainPage.xaml", "BlankPage.xaml" });
        }

        [TestMethod]
        public void MultipleView_NonmatchingBaseDifferentFolders()
        {
            this.TestMultipleViewPathLogicBaseError(new string[] { "Views\\Platform-Desktop\\MainPage.xaml", "Views\\Platform-Mobile\\BlankPage.xaml" });
        }

        [TestMethod]
        public void MultipleView_NonmatchingBaseSubfolder()
        {
            this.TestMultipleViewPathLogicBaseError(new string[] { "Views\\MainPage.Platform-Desktop.xaml", "Views\\BlankPage.Platform-Mobile.xaml" });
        }
    }
}
