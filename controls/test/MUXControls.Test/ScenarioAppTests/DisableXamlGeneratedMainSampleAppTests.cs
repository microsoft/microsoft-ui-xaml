using Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Microsoft.Windows.Apps.Test.Foundation;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    /// <summary>
    /// Shared plumbing for the sample apps under Samples\DisableXamlGeneratedMain. Those apps
    /// define DISABLE_XAML_GENERATED_MAIN and supply their own entry point; the tests below
    /// verify that such apps not only compile, but also launch.
    /// </summary>
    public static class DisableXamlGeneratedMainAppTestsUtils
    {
        // Every one of these sample apps is signed with the WinUITest certificate, whose
        // publisher hash is 6f07fta6qpts2, and pins AppxPackageName to the package name, so
        // the package, executable, window title and installer name are all the same string.
        public static TestApplicationInfo CreateTestApplicationInfo(string appName)
        {
            return new TestApplicationInfo(
                appName,
                $"{appName}_6f07fta6qpts2!App",
                $"{appName}_6f07fta6qpts2",
                appName,
                $"{appName}.exe",
                appName,
                isUwpApp: false,
                TestApplicationInfo.MUXCertSerialNumber,
                TestApplicationInfo.MUXBaseAppxDir);
        }

        /// <summary>
        /// Clicks an element in the app's main window so that it has focus.
        /// </summary>
        private static void FocusMainWindow(string appName)
        {
            Log.Comment($"Looking for the top-level window of {appName}");
            UIObject root = FindElement.GetDesktopTopLevelWindow(appName);
            Verify.IsNotNull(root, "Top-level window");

            UIObject safeElementToClick = FindElement.GetDescendantByName(root, "MainWindowTextBlock");
            Verify.IsNotNull(safeElementToClick, "MainWindowTextBlock");
            InputHelper.LeftClick(safeElementToClick);
            Wait.ForIdle();
        }

        /// <summary>
        /// Verifies that the app's MainWindow was created and that it was reached through the
        /// developer-supplied entry point, which records <paramref name="expectedEntryPointMarker"/>.
        /// </summary>
        public static void VerifyMainWindowLaunched(string appName, string expectedEntryPointMarker)
        {
            FocusMainWindow(appName);

            WinUISampleAppTestsUtils.VerifyText("MainWindowTextBlock", "MainWindow");
            WinUISampleAppTestsUtils.VerifyText("EntryPointTextBlock", expectedEntryPointMarker);
        }
    }

    /// <summary>
    /// Verifies that Samples\DisableXamlGeneratedMain\Cs launches. It defines
    /// DISABLE_XAML_GENERATED_MAIN, declares its own Main and delegates to the
    /// XamlCompiler-generated XamlGeneratedMain helper.
    /// </summary>
    [TestClass]
    public class DisableXamlGeneratedMainCsAppTests
    {
        private const string AppName = "DisableXamlGeneratedMainCs";

        public static TestApplicationInfo TestApplication
        {
            get { return DisableXamlGeneratedMainAppTestsUtils.CreateTestApplicationInfo(AppName); }
        }

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "ScenarioTestSuite")]
        [TestProperty("IgnoreForValidateWindowsAppSDK", "True")]
        [TestProperty("Platform", "Any")]
        [TestProperty("IsolationLevel", "Test")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext, TestApplication);
        }

        [ClassCleanup]
        public static void ClassCleanup()
        {
            TestEnvironment.AssemblyCleanupWorker(TestApplication);
        }

        [TestMethod]
        [Description("Launch an app that defines DISABLE_XAML_GENERATED_MAIN and verify its MainWindow appears, having been reached through the app's own Main rather than a generated one.")]
        public void LaunchAndVerifyCustomEntryPointTest()
        {
            DisableXamlGeneratedMainAppTestsUtils.VerifyMainWindowLaunched(AppName, "CustomMain");
        }
    }

    /// <summary>
    /// Verifies that Samples\DisableXamlGeneratedMain\CsNoCtor launches. In addition to
    /// defining DISABLE_XAML_GENERATED_MAIN, it has no parameterless App constructor: its
    /// Main constructs App with a parameterized constructor instead.
    /// </summary>
    [TestClass]
    public class DisableXamlGeneratedMainNoCtorCsAppTests
    {
        private const string AppName = "DisableXamlGeneratedMainNoCtorCs";

        public static TestApplicationInfo TestApplication
        {
            get { return DisableXamlGeneratedMainAppTestsUtils.CreateTestApplicationInfo(AppName); }
        }

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "ScenarioTestSuite")]
        [TestProperty("IgnoreForValidateWindowsAppSDK", "True")]
        [TestProperty("Platform", "Any")]
        [TestProperty("IsolationLevel", "Test")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext, TestApplication);
        }

        [ClassCleanup]
        public static void ClassCleanup()
        {
            TestEnvironment.AssemblyCleanupWorker(TestApplication);
        }

        [TestMethod]
        [Description("Launch an app that defines DISABLE_XAML_GENERATED_MAIN and has no parameterless App constructor, and verify its MainWindow appears, having been reached through the app's own Main using App's parameterized constructor.")]
        public void LaunchAndVerifyCustomEntryPointTest()
        {
            DisableXamlGeneratedMainAppTestsUtils.VerifyMainWindowLaunched(AppName, "CustomMain:42");
        }
    }

    /// <summary>
    /// Verifies that Samples\DisableXamlGeneratedMain\Cpp launches. It defines
    /// DISABLE_XAML_GENERATED_MAIN, declares its own wWinMain and delegates to the
    /// XamlCompiler-generated wXamlGeneratedMain helper.
    /// </summary>
    [TestClass]
    public class DisableXamlGeneratedMainCppAppTests
    {
        private const string AppName = "DisableXamlGeneratedMainCpp";

        public static TestApplicationInfo TestApplication
        {
            get { return DisableXamlGeneratedMainAppTestsUtils.CreateTestApplicationInfo(AppName); }
        }

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "ScenarioTestSuite")]
        [TestProperty("IgnoreForValidateWindowsAppSDK", "True")]
        [TestProperty("Platform", "Any")]
        [TestProperty("IsolationLevel", "Test")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext, TestApplication);
        }

        [ClassCleanup]
        public static void ClassCleanup()
        {
            TestEnvironment.AssemblyCleanupWorker(TestApplication);
        }

        [TestMethod]
        [Description("Launch an app that defines DISABLE_XAML_GENERATED_MAIN and verify its MainWindow appears, having been reached through the app's own wWinMain rather than a generated one.")]
        public void LaunchAndVerifyCustomEntryPointTest()
        {
            DisableXamlGeneratedMainAppTestsUtils.VerifyMainWindowLaunched(AppName, "CustomMain");
        }
    }

    /// <summary>
    /// Verifies that Samples\DisableXamlGeneratedMain\CppNoCtor launches. In addition to
    /// defining DISABLE_XAML_GENERATED_MAIN, it has no default App constructor: its wWinMain
    /// constructs App with a parameterized constructor instead.
    /// </summary>
    [TestClass]
    public class DisableXamlGeneratedMainNoCtorCppAppTests
    {
        private const string AppName = "DisableXamlGeneratedMainNoCtorCpp";

        public static TestApplicationInfo TestApplication
        {
            get { return DisableXamlGeneratedMainAppTestsUtils.CreateTestApplicationInfo(AppName); }
        }

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "ScenarioTestSuite")]
        [TestProperty("IgnoreForValidateWindowsAppSDK", "True")]
        [TestProperty("Platform", "Any")]
        [TestProperty("IsolationLevel", "Test")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext, TestApplication);
        }

        [ClassCleanup]
        public static void ClassCleanup()
        {
            TestEnvironment.AssemblyCleanupWorker(TestApplication);
        }

        [TestMethod]
        [Description("Launch an app that defines DISABLE_XAML_GENERATED_MAIN and has no default App constructor, and verify its MainWindow appears, having been reached through the app's own wWinMain using App's parameterized constructor.")]
        public void LaunchAndVerifyCustomEntryPointTest()
        {
            DisableXamlGeneratedMainAppTestsUtils.VerifyMainWindowLaunched(AppName, "CustomMain:42");
        }
    }
}
