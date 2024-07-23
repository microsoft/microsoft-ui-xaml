using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using System;
using System.Numerics;
using Common;
using System.Threading.Tasks;
using Microsoft.Windows.Apps.Test.Foundation;
using System.Threading;
using System.Runtime.InteropServices;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class WinUIGalleryDesktopTests
    {
        public static TestApplicationInfo WinUIGalleryDesktopApp
        {
            get
            {
                return new TestApplicationInfo(
#if DEBUG                    
                    "Microsoft.WinUI3ControlsGallery.Debug", 
                    "Microsoft.WinUI3ControlsGallery.Debug_6f07fta6qpts2!App", 
                    "Microsoft.WinUI3ControlsGallery.Debug_6f07fta6qpts2",
                    "WinUI 3 Gallery Dev",
#else
                    "Microsoft.WinUI3ControlsGallery", 
                    "Microsoft.WinUI3ControlsGallery_6f07fta6qpts2!App", 
                    "Microsoft.WinUI3ControlsGallery_6f07fta6qpts2",
                    "WinUI 3 Gallery",
#endif
                    "WinUIGallery.exe",
                    "WinUIGallery.Desktop", // Installer name
                    isUwpApp: false,
                    TestApplicationInfo.MUXCertSerialNumber,
                    TestApplicationInfo.MUXBaseAppxDir
                );
            }
        }

        public static TestSetupHelper.TestSetupHelperOptions TestSetupHelperOptions
        {
            get
            {
                return new TestSetupHelper.TestSetupHelperOptions() 
                {
                    AutomationIdOfSafeItemToClick = "__ClickableAreaTextBlock",
                    ClassNameOfNavigationItemToInvoke = "", // To navigate we invoke a mixed set of items, so the ClassName is unknown.
                };
            }
        }

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "ScenarioTestSuite")]
        [TestProperty("Platform", "Any")]
        [TestProperty("IsolationLevel", "Class")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext, WinUIGalleryDesktopApp);
        }

        public TestContext TestContext { get; set; }

        [TestCleanup]
        public void TestCleanup()
        {
            
        }

        [ClassCleanupAttribute]
        public static void ClassCleanup()
        {
            TestEnvironment.AssemblyCleanupWorker(WinUIGalleryDesktopApp);
        }

        [TestMethod]
        [DataSource("Table:WinUIGalleryTestData.xml#MenusAndToolbars")]
        [TestProperty("TestSuite", "MenusAndToolbars")]
        public void SimplePageNavigateTest_MenusAndToolbars()
        {
            DoSimplePageNavigateTest();
        }

        [TestMethod]
        [DataSource("Table:WinUIGalleryTestData.xml#Collections")]
        [TestProperty("TestSuite", "Collections")]
        public void SimplePageNavigateTest_Collections()
        {
            DoSimplePageNavigateTest();
        }

        [TestMethod]
        [DataSource("Table:WinUIGalleryTestData.xml#DateAndTime")]
        [TestProperty("TestSuite", "DateAndTime")]
        public void SimplePageNavigateTest_DateAndTime()
        {
            DoSimplePageNavigateTest();
        }

        [TestMethod]
        [DataSource("Table:WinUIGalleryTestData.xml#BasicInput")]
        [TestProperty("TestSuite", "BasicInput")]
        public void SimplePageNavigateTest_BasicInput()
        {
            DoSimplePageNavigateTest();
        }

        [TestMethod]
        [DataSource("Table:WinUIGalleryTestData.xml#StatusAndInfo")]
        [TestProperty("TestSuite", "StatusAndInfo")]
        public void SimplePageNavigateTest_StatusAndInfo()
        {
            DoSimplePageNavigateTest();
        }

        [TestMethod]
        [DataSource("Table:WinUIGalleryTestData.xml#DialogsAndFlyouts")]
        [TestProperty("TestSuite", "DialogsAndFlyouts")]
        public void SimplePageNavigateTest_DialogsAndFlyouts()
        {
            DoSimplePageNavigateTest();
        }

        [TestMethod]
        [DataSource("Table:WinUIGalleryTestData.xml#Scrolling")]
        [TestProperty("TestSuite", "Scrolling")]
        public void SimplePageNavigateTest_Scrolling()
        {
            DoSimplePageNavigateTest();
        }

        [TestMethod]
        [DataSource("Table:WinUIGalleryTestData.xml#Layout")]
        [TestProperty("TestSuite", "Layout")]
        public void SimplePageNavigateTest_Layout()
        {
            DoSimplePageNavigateTest();
        }

        [TestMethod]
        [DataSource("Table:WinUIGalleryTestData.xml#Navigation")]
        [TestProperty("TestSuite", "Navigation")]
        public void SimplePageNavigateTest_Navigation()
        {
            DoSimplePageNavigateTest();
        }

        [TestMethod]
        [DataSource("Table:WinUIGalleryTestData.xml#Media")]
        [TestProperty("TestSuite", "Media")]
        public void SimplePageNavigateTest_Media()
        {
            DoSimplePageNavigateTest();
        }

        [TestMethod]
        [DataSource("Table:WinUIGalleryTestData.xml#Styles")]
        [TestProperty("TestSuite", "Styles")]
        public void SimplePageNavigateTest_Styles()
        {
            DoSimplePageNavigateTest();
        }

        [TestMethod]
        [DataSource("Table:WinUIGalleryTestData.xml#Text")]
        [TestProperty("TestSuite", "Text")]
        public void SimplePageNavigateTest_Text()
        {
            DoSimplePageNavigateTest();
        }

        [TestMethod]
        [DataSource("Table:WinUIGalleryTestData.xml#Motion")]
        [TestProperty("TestSuite", "Motion")]
        public void SimplePageNavigateTest_Motion()
        {
            DoSimplePageNavigateTest();
        }

        public void DoSimplePageNavigateTest()
        {
            string sectionName =  this.TestContext.DataRow["SectionName"].ToString();
            string pageName =  this.TestContext.DataRow["PageName"].ToString();
            string textOnPage =  this.TestContext.DataRow["TextOnPage"].ToString();

            // TODO 29960508: Re-enable these tests when the bug is fixed where the returned UIA tree is incomplete when a popup is showing.
            string disabledTest = null;

            if (sectionName == "Motion" && pageName == "Animation interop")
            {
                disabledTest = "SimplePageNavigateTest_Motion#XamlCompInterop";
            }
            else if (sectionName == "Navigation" && pageName == "TabView")
            {
                disabledTest = "SimplePageNavigateTest_Navigation#TabView";
            }

            if (disabledTest != null)
            {
                Log.Warning(disabledTest + " is currently disabled due to the complete UIA tree not being properly returned.");
                return;
            }

            // TODO 46819062: Re-enable these tests when the bugs are fixed, where going to the SemanticZoom page crashes the app.
            if (sectionName == "Scrolling" && pageName == "SemanticZoom")
            {
                disabledTest = "SimplePageNavigateTest_Scrolling#SemanticZoom";
            }

            if (disabledTest != null)
            {
                Log.Warning(disabledTest + " is currently disabled due to a crash in the SemanticZoom page.");
                return;
            }

            // TODO 46878098: Re-enable these tests when the bugs are fixed.
            if (sectionName == "Collections" && pageName == "ItemsView")
            {
                disabledTest = "SimplePageNavigateTest_Collections#ItemsView";
            }
            else if (sectionName == "Collections" && pageName == "ListBox")
            {
                disabledTest = "SimplePageNavigateTest_Collections#ListBox";
            }
            else if (sectionName == "Collections" && pageName == "ListView")
            {
                disabledTest = "SimplePageNavigateTest_Collections#ListView";
            }
            else if (sectionName == "Scrolling" && pageName == "AnnotatedScrollBar")
            {
                disabledTest = "SimplePageNavigateTest_Scrolling#AnnotatedScrollBar";
            }
            else if (sectionName == "Scrolling" && pageName == "PipsPager")
            {
                disabledTest = "SimplePageNavigateTest_Scrolling#PipsPager";
            }
            else if (sectionName == "Scrolling" && pageName == "ScrollView")
            {
                disabledTest = "SimplePageNavigateTest_Scrolling#ScrollView";
            }
            else if (sectionName == "Scrolling" && pageName == "ScrollViewer")
            {
                disabledTest = "SimplePageNavigateTest_Scrolling#ScrollViewer";
            }
            else if (sectionName == "Motion" && pageName == "ParallaxView")
            {
                disabledTest = "SimplePageNavigateTest_Motion#ParallaxView";
            }
            else if (sectionName == "Navigation" && pageName == "SelectorBar")
            {
                disabledTest = "SimplePageNavigateTest_Navigation#SelectorBar";
            }
            else if (sectionName == "Media" && pageName == "MapControl")
            {
                disabledTest = "SimplePageNavigateTest_Media#MapControl";
            }
            else if (sectionName == "Media" && pageName == "MediaPlayerElement")
            {
                disabledTest = "SimplePageNavigateTest_Media#MediaPlayerElement";
            }

            if (disabledTest != null)
            {
                Log.Warning(disabledTest + " is currently disabled due to reliability issues in the pipeline.");
                return;
            }

            using (var setup = new TestSetupHelper(new string[] { sectionName, pageName}, TestSetupHelperOptions))
            {
                if (!string.IsNullOrWhiteSpace(textOnPage))
                {
                    // TODO 29960508: Remove this handling to close popups when the bug is fixed where the returned UIA tree is incomplete when a popup is showing.
                    var popup = TryFindElement.ByClassName("Popup");

                    if (popup != null)
                    {
                        Log.Comment("A popup is open. Moving focus using shift+tab to close it.");
                        KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                        Log.Comment("Waiting for popup to close...");
                    }

                    for (int i = 0; i < 20 && TryFindElement.ByClassName("Popup") != null; i++)
                    {
                        Thread.Sleep(100);
                    }
                    
                    Log.Comment("Find textBlock ({0})", textOnPage);
                    var textBlock = new TextBlock(TryFindElement.ByName(textOnPage));
                    Verify.IsNotNull(textBlock);
                    Verify.AreEqual(textBlock.DocumentText, textOnPage);
                }
            }
        }
    }
}
