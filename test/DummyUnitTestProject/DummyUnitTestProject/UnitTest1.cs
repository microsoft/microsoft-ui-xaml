using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace DummyUnitTestProject
{
    [TestClass]
    public class UnitTest1
    {
        [TestMethod]
        public void PassingTest()
        {
            Assert.IsTrue(true);
        }

        [TestMethod]
        public void FailingTest()
        {
            Assert.IsTrue(false);
        }

        [TestMethod]
        public void UnreliableTest()
        {
            Assert.IsTrue(new System.Random().Next() % 2 == 0);
        }
    }
}
