// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using XmlValidation;

namespace UnitTests
{
    [TestClass]
    public class TreeComparatorTests
    {
        [TestInitialize]
        public void Initialize()
        {
            tc = new TreeStructureComparator();
        }

        [TestMethod]
        public void TrivialElement()
        {
            string xml = @"<Element/>";
            Assert.IsTrue(tc.Compare(xml, xml));
        }

        [TestMethod]
        public void ElementWithChild()
        {
            string xml =
                @"<Element>
                    <Element/>
                  </Element>";

            Assert.IsTrue(tc.Compare(xml, xml));
        }

        [TestMethod]
        public void ElementWithChildren()
        {
            string xml =
                @"<Element>
                    <Element/>
                    <Element>
                      <Element/>
                    </Element>
                  </Element>";

            Assert.IsTrue(tc.Compare(xml, xml));
        }

        private void GeneralizedOneChangeScenario(
            string xml1,
            string xml2,
            TreeModificationType ct,
            params uint[] differences)
        {
            Assert.IsTrue(ct == TreeModificationType.Deletion || ct == TreeModificationType.Insertion);

            if (ct == TreeModificationType.Insertion)
            {
                string temp = xml1;
                xml1 = xml2;
                xml2 = temp;
            }

            Assert.IsFalse(tc.Compare(xml1, xml2));

            var res = tc.GetChanges();
            Assert.AreEqual(res.Count, differences.Length);

            for (int i = 0; i < res.Count; ++i)
            {
                Assert.AreEqual(res[i].ChangeType, ct);

                if (ct == TreeModificationType.Deletion)
                {
                    Assert.AreEqual(res[i].ReferenceLineNumber, differences[i]);
                }
                else
                {
                    Assert.AreEqual(res[i].ValidatedLineNumber, differences[i]);
                }
            }
        }

        private void Scenario0(bool del)
        {
            GeneralizedOneChangeScenario(
                @"<Element>
                    <Element/>
                  </Element>",
                @"<Element/>",
                (del) ? TreeModificationType.Deletion : TreeModificationType.Insertion,
                2);
        }

        [TestMethod]
        public void Deletion0()
        {
            Scenario0(true);
        }

        [TestMethod]
        public void Insertion0()
        {
            Scenario0(false);
        }

        private void Scenario1(bool del)
        {
            GeneralizedOneChangeScenario(
                @"<Element>
                    <Element/>
                    <Element/>
                  </Element>",
                @"<Element>
                    <Element/>
                  </Element>",
                (del) ? TreeModificationType.Deletion : TreeModificationType.Insertion,
                3);
        }

        [TestMethod]
        public void Deletion1()
        {
            Scenario1(true);
        }

        [TestMethod]
        public void Insertion1()
        {
            Scenario1(false);
        }

        private void Scenario2(bool del)
        {
            GeneralizedOneChangeScenario(
                @"<Element>
                    <Element/>
                    <Element/>
                    <Element/>
                    <Element/>
                  </Element>",
                @"<Element>
                    <Element/>
                    <Element/>
                  </Element>",
                (del) ? TreeModificationType.Deletion : TreeModificationType.Insertion,
                4, 5);
        }

        [TestMethod]
        public void Deletion2()
        {
            Scenario2(true);
        }

        [TestMethod]
        public void Insertion2()
        {
            Scenario2(false);
        }

        private void Scenario3(bool del)
        {
            GeneralizedOneChangeScenario(
                @"<Element>
                    <Element/>
                    <Element/>
                    <Element/>
                    <Element/>
                  </Element>",
                @"<Element/>",
                (del) ? TreeModificationType.Deletion : TreeModificationType.Insertion,
                2, 3, 4, 5);
        }

        [TestMethod]
        public void Deletion3()
        {
            Scenario3(true);
        }

        [TestMethod]
        public void Insertion3()
        {
            Scenario3(false);
        }

        private void Scenario4(bool del)
        {
            GeneralizedOneChangeScenario(
                @"<Element>
                    <Element>
                      <Element/>
                      <Element/>
                    </Element>
                  </Element>",
                @"<Element>
                    <Element>
                      <Element/>
                    </Element>
                  </Element>",
                (del) ? TreeModificationType.Deletion : TreeModificationType.Insertion,
                4);
        }

        [TestMethod]
        public void Deletion4()
        {
            Scenario4(true);
        }

        [TestMethod]
        public void Insertion4()
        {
            Scenario4(false);
        }

        private void Scenario5(bool del)
        {
            GeneralizedOneChangeScenario(
                @"<Element>
                    <Element/>
                    <Element>
                      <Element/>
                    </Element>
                    <Element/>
                  </Element>",
                @"<Element>
                    <Element>
                      <Element/>
                    </Element>
                  </Element>",
                (del) ? TreeModificationType.Deletion : TreeModificationType.Insertion,
                2, 6);
        }

        [TestMethod]
        public void Deletion5()
        {
            Scenario5(true);
        }

        [TestMethod]
        public void Insertion5()
        {
            Scenario5(false);
        }

        private void Scenario6(bool del)
        {
            GeneralizedOneChangeScenario(
                @"<Element>
                    <Element/>
                    <Element>
                      <Element/>
                    </Element>
                    <Element/>
                  </Element>",
                @"<Element>
                    <Element/>
                    <Element/>
                  </Element>",
                (del) ? TreeModificationType.Deletion : TreeModificationType.Insertion,
                3);
        }

        [TestMethod]
        public void Deletion6()
        {
            Scenario6(true);
        }

        [TestMethod]
        public void Insertion6()
        {
            Scenario6(false);
        }

        [TestMethod]
        public void Move0()
        {
            Assert.IsFalse(tc.Compare(
                @"<Element>
                    <Element/>
                    <Element>
                      <Element/>
                    </Element>
                    <Element/>
                  </Element>",
                @"<Element>
                    <Element>
                      <Element/>
                    </Element>
                    <Element/>
                    <Element/>
                  </Element>"));

            var res = tc.GetChanges();

            Assert.AreEqual(res.Count, 1);
            Assert.AreEqual(res[0].ChangeType, TreeModificationType.Move);
            Assert.AreEqual(res[0].ReferenceLineNumber, 2U);
            Assert.AreEqual(res[0].ValidatedLineNumber, 5U);
        }

        [TestMethod]
        public void Move1()
        {
            Assert.IsFalse(tc.Compare(
                @"<Element>
                    <Element>
                      <Element/>
                    </Element>
                    <Element/>
                    <Element/>
                  </Element>",
                @"<Element>
                    <Element/>
                    <Element/>
                    <Element>
                      <Element/>
                    </Element>
                  </Element>"));

            var res = tc.GetChanges();

            Assert.AreEqual(res.Count, 1);
            Assert.AreEqual(res[0].ChangeType, TreeModificationType.Move);
            Assert.AreEqual(res[0].ReferenceLineNumber, 2U);
            Assert.AreEqual(res[0].ValidatedLineNumber, 4U);
        }

        private TreeStructureComparator tc;
    }
}
