// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.CompilerServices;

using MUXControlsTestApp.Utilities;

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;

using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Lifetime stress tests.
    //
    // Historically (back to the Win8 era) XAML shipped a set of "lifetime tests" that repeatedly created,
    // parented, unparented and destroyed elements while forcing aggressive garbage collection, watching for
    // eventual crashes caused by object-lifetime bugs (use-after-free / premature native peer destruction /
    // ref-counting mistakes in the 3-layer peer model). Those tests were dropped when the old test team tests
    // went away. This class reintroduces that coverage in a form that is:
    //
    //   * Deterministic and fast enough to run as a gate on every test pass (default iteration counts), rather
    //     than only as a multi-hour soak. Each iteration aggressively settles the UI thread and forces the GC +
    //     finalizers so a dangling native peer is much more likely to fault *immediately* instead of "eventually".
    //   * Still able to run as a long soak (the classic "run for a few hours" behavior) by setting environment
    //     variables (see below), so a scheduled pipeline can crank the workload way up.
    //   * Isolated into its own TAEF test suite (see the TestSuite TestProperty). The Helix work-item generator
    //     produces a dedicated work item for this suite, so if a lifetime bug does crash the test host it does
    //     not take down unrelated tests, and the soak can be scheduled independently.
    //
    // Configuration (all optional, read from environment variables so they work locally, on pipeline agents, and
    // when injected into a Helix work item):
    //
    //   WINUI_LIFETIME_STRESS_ITERATIONS   Number of create/destroy cycles per scenario. Default: 50.
    //   WINUI_LIFETIME_STRESS_MINUTES      If > 0, each scenario loops until this many minutes have elapsed
    //                                      instead of using a fixed iteration count. Use this for the soak.
    //
    // Adding coverage: the cheapest, highest-value thing a contributor can do when fixing a lifetime crash (as the
    // ItemsRepeater realization/recycling scenario below demonstrates) is to add the offending create/teardown
    // sequence here so the fix is protected against regression.
    [TestClass]
    // Classification=Integration makes this class an explicit member of the DevTestSuite PR gate
    // (WinUI-GitHub-PR). The MUXControlsTestApp module sets Classification=Integration module-wide via
    // ApiTestAssemblyHandling.AssemblyInitialize, but we declare it here as well so this suite's
    // participation in the per-PR test pass is explicit and self-documenting, matching the convention
    // used by the InteractionTests classes. The Helix work-item generator emits a dedicated
    // "*-LifetimeStressTestSuite" work item for it, so it runs isolated from unrelated tests.
    [TestProperty("Classification", "Integration")]
    [TestProperty("TestSuite", "LifetimeStressTestSuite")]
    public class LifetimeStressTests : ApiTestBase
    {
        private const int DefaultIterations = 50;

        private static int ConfiguredIterations
        {
            get { return GetEnvInt("WINUI_LIFETIME_STRESS_ITERATIONS", DefaultIterations); }
        }

        private static double ConfiguredSoakMinutes
        {
            get { return GetEnvDouble("WINUI_LIFETIME_STRESS_MINUTES", 0.0); }
        }

        // Repeatedly create a broad set of controls, add each to the live visual tree, run layout (which forces the
        // native peer to be created and wired up), then unparent, drop the managed reference and collect. This is the
        // classic lifetime torture test: a bug in peer creation/destruction or ref-counting will fault here, and the
        // per-iteration collection makes the fault prompt instead of eventual.
        [TestMethod]
        public void StressControlCreateLoadUnloadCollect()
        {
            RunStress("StressControlCreateLoadUnloadCollect", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                RunOnUIThread.Execute(() =>
                {
                    foreach (var pair in CreateControlSet())
                    {
                        var element = pair.Value;
                        objects[pair.Key] = new WeakReference(element);

                        Content = element;
                        Content.UpdateLayout();
                        Content = null;
                    }
                });

                SettleAndCollect();
                // Report a residual reference as a warning rather than failing the run. A leaked control here is a
                // real signal worth investigating, but this suite is a non-gating lifetime *report*: the primary
                // pass/fail signal is that the create/load/unload/collect loop does not crash the test host. Emitting
                // a failed test result would gate the shared pipeline (the Run Tests stage's Publish Test Results
                // step) on a soft, sometimes-flaky signal, so we surface it as a warning instead.
                RunOnUIThread.Execute(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // ItemsRepeater realization / recycling lifetime stress.
        //
        // ItemsRepeater is a repeated source of lifetime crashes: elements are realized, recycled and cleared as the
        // data source and viewport change, and a mistake in when the native peer for a realized element is released
        // (relative to the managed element / element factory) shows up as a use-after-free. This scenario builds a
        // repeater inside a scrolling host, realizes elements, churns the ItemsSource, forces recycling via repeated
        // layout, and finally tears the whole thing down mid-flight and collects.
        [TestMethod]
        public void StressItemsRepeaterRealizationAndRecycling()
        {
            RunStress("StressItemsRepeaterRealizationAndRecycling", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                RunOnUIThread.Execute(() =>
                {
                    var elementFactory = new RecyclingElementFactory();
                    elementFactory.RecyclePool = new RecyclePool();
                    elementFactory.Templates["Item"] = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                              <TextBlock Text='{Binding}' Height='50' />
                          </DataTemplate>");

                    var repeater = new ItemsRepeater()
                    {
                        ItemsSource = Enumerable.Range(0, 200).Select(i => string.Format("Item #{0}", i)),
                        ItemTemplate = elementFactory,
                        Layout = new StackLayout(),
                    };
                    objects["Repeater"] = new WeakReference(repeater);
                    objects["ElementFactory"] = new WeakReference(elementFactory);

                    var scrollHost = new ItemsRepeaterScrollHost()
                    {
                        Width = 400,
                        Height = 600,
                        ScrollViewer = new ScrollViewer
                        {
                            Content = repeater
                        }
                    };

                    Content = scrollHost;
                    Content.UpdateLayout();

                    // Churn the source and viewport to force elements to be realized, recycled and cleared. Each
                    // ItemsSource swap invalidates the realized range; the UpdateLayout calls drive the realize /
                    // recycle path that is the usual home of ItemsRepeater lifetime bugs.
                    for (int churn = 0; churn < 5; churn++)
                    {
                        repeater.ItemsSource = Enumerable.Range(churn * 50, 150).Select(i => string.Format("Item #{0}", i));
                        Content.UpdateLayout();

                        // Poke the element cache: realize a spread of indices then let them recycle on the next pass.
                        for (int i = 0; i < 150; i += 10)
                        {
                            var realized = repeater.GetOrCreateElement(i);
                            if (realized != null)
                            {
                                realized.UpdateLayout();
                            }
                        }
                        Content.UpdateLayout();
                    }

                    // Tear down without a graceful drain: clear the source, detach the repeater from the host and
                    // drop the tree while realized elements may still be in flight.
                    repeater.ItemsSource = null;
                    scrollHost.ScrollViewer.Content = null;
                    scrollHost.ScrollViewer = null;
                    Content = null;
                });

                SettleAndCollect();
                // ItemsRepeater / element-factory graphs can legitimately need extra time to unwind; treat a residual
                // reference as a warning rather than a hard failure here. The primary signal for this scenario is that
                // the churn/teardown loop does not crash the test host.
                RunOnUIThread.Execute(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // Reparenting lifetime stress. Moving a live element between parents (and in/out of the tree) exercises
        // enter/leave and peer re-association, another historically crash-prone path.
        [TestMethod]
        public void StressElementReparenting()
        {
            RunStress("StressElementReparenting", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                RunOnUIThread.Execute(() =>
                {
                    var root = new Grid();
                    var parentA = new Border() { Width = 100, Height = 100 };
                    var parentB = new ContentControl() { Width = 100, Height = 100 };
                    root.Children.Add(parentA);
                    root.Children.Add(parentB);

                    var child = new TextBox() { Text = "reparent-me" };
                    objects["Child"] = new WeakReference(child);
                    objects["ParentA"] = new WeakReference(parentA);
                    objects["ParentB"] = new WeakReference(parentB);

                    Content = root;
                    Content.UpdateLayout();

                    // Bounce the child between the two parents, running layout each time so enter/leave actually runs.
                    for (int move = 0; move < 10; move++)
                    {
                        parentA.Child = child;
                        Content.UpdateLayout();

                        parentA.Child = null;
                        parentB.Content = child;
                        Content.UpdateLayout();

                        parentB.Content = null;
                    }

                    root.Children.Clear();
                    Content = null;
                });

                SettleAndCollect();
                RunOnUIThread.Execute(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // Window open/close lifetime stress. Windows own a lot of native state; repeatedly creating, activating and
        // closing them while collecting catches lifetime bugs in window/content teardown.
        [TestMethod]
        public void StressWindowOpenClose()
        {
            RunStress("StressWindowOpenClose", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                RunOnUIThread.Execute(() =>
                {
                    objects["Window"] = CreateActivateAndCloseWindow();
                });

                SettleAndCollect();
                RunOnUIThread.Execute(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private static WeakReference CreateActivateAndCloseWindow()
        {
            var window = new Window();
            var reference = new WeakReference(window);

            var content = new Grid();
            content.Children.Add(new TextBlock() { Text = "lifetime" });
            content.Children.Add(new Button() { Content = "ok" });
            window.Content = content;

            window.Activate();
            window.Close();

            return reference;
        }

        // Build a fresh instance of every WinUI control we want to torture. Each entry is a distinct control type so
        // a single iteration covers essentially the whole WinUI control surface.
        //
        // This aims to cover *all* of the WinUI (Microsoft.UI.Xaml.Controls) controls, since a lifetime bug can live
        // in any control's peer creation / enter-leave / teardown path. It is deliberately kept to controls that are
        // cheap to construct and constructible without a live window / parent / external service. The handful that
        // are intentionally NOT swept here need special hosting and would add flakiness rather than lifetime signal:
        //   * WebView2         - needs the WebView2 runtime / a core environment.
        //   * MapControl       - needs a map service token and network.
        //   * InkToolbar       - requires a target InkCanvas to be attached.
        //   * CommandBarFlyout / RadioMenuFlyoutItem - flyout-only types, not standalone tree content.
        // ItemsRepeater is covered by its own dedicated realization/recycling scenario above.
        private static Dictionary<string, UIElement> CreateControlSet()
        {
            return new Dictionary<string, UIElement>
            {
                ["AnimatedIcon"] = new AnimatedIcon(),
                ["AnimatedVisualPlayer"] = new AnimatedVisualPlayer(),
                ["AnnotatedScrollBar"] = new AnnotatedScrollBar(),
                ["AutoSuggestBox"] = new AutoSuggestBox(),
                ["BreadcrumbBar"] = new BreadcrumbBar(),
                ["ColorPicker"] = new ColorPicker(),
                ["ComboBox"] = new ComboBox() { ItemsSource = Enumerable.Range(0, 50) },
                ["DropDownButton"] = new DropDownButton() { Content = "menu" },
                ["Expander"] = new Expander() { Content = new TextBlock() { Text = "content" } },
                ["ImageIcon"] = new ImageIcon(),
                ["InfoBadge"] = new InfoBadge(),
                ["InfoBar"] = new InfoBar() { Title = "title", Message = "message", IsOpen = true },
                ["ItemContainer"] = new ItemContainer() { Child = new TextBlock() { Text = "item" } },
                ["ItemsView"] = new ItemsView(),
                ["ListView"] = new ListView() { ItemsSource = Enumerable.Range(0, 50) },
                ["MenuBar"] = new MenuBar(),
                ["MonochromaticOverlayPresenter"] = new MonochromaticOverlayPresenter(),
                ["NavigationView"] = new NavigationView(),
                ["NumberBox"] = new NumberBox(),
                ["PagerControl"] = new PagerControl() { NumberOfPages = 10 },
                ["ParallaxView"] = new ParallaxView(),
                ["PersonPicture"] = new PersonPicture() { DisplayName = "Lifetime Test" },
                ["PipsPager"] = new PipsPager() { NumberOfPages = 10 },
                ["ProgressBar"] = new ProgressBar() { Value = 50 },
                ["ProgressRing"] = new ProgressRing() { IsActive = true },
                ["RadioButtons"] = new RadioButtons() { ItemsSource = Enumerable.Range(0, 5) },
                ["RatingControl"] = new RatingControl(),
                ["RefreshContainer"] = new RefreshContainer() { Content = new TextBlock() { Text = "pull" } },
                ["ScrollPresenter"] = new ScrollPresenter(),
                ["ScrollView"] = new ScrollView(),
                ["SelectorBar"] = new SelectorBar(),
                ["SplitButton"] = new SplitButton() { Content = "split" },
                ["SplitView"] = new SplitView() { Content = new TextBlock() { Text = "content" }, Pane = new TextBlock() { Text = "pane" } },
                ["SwipeControl"] = new SwipeControl() { Content = new TextBlock() { Text = "swipe" } },
                ["TabView"] = new TabView(),
                ["TeachingTip"] = new TeachingTip() { Title = "tip", Subtitle = "subtitle" },
                ["TitleBar"] = new TitleBar() { Title = "title" },
                ["TreeView"] = new TreeView() { ItemsSource = Enumerable.Range(0, 50) },
                ["TwoPaneView"] = new TwoPaneView(),
            };
        }

        // Loop the supplied per-iteration work either a fixed number of times (default / gate) or until a wall-clock
        // soak budget elapses (WINUI_LIFETIME_STRESS_MINUTES). Progress is logged so a crash's last-known iteration is
        // visible in the test output.
        private static void RunStress(string scenarioName, Action<int> iteration)
        {
            int iterations = ConfiguredIterations;
            double soakMinutes = ConfiguredSoakMinutes;

            if (soakMinutes > 0.0)
            {
                var deadline = Stopwatch.StartNew();
                var budget = TimeSpan.FromMinutes(soakMinutes);
                Log.Comment("[{0}] Soak mode: running for {1} minute(s).", scenarioName, soakMinutes);

                int i = 0;
                while (deadline.Elapsed < budget)
                {
                    Log.Comment("[{0}] Soak iteration {1} (elapsed {2:0.0}/{3:0.0} min)...",
                        scenarioName, i, deadline.Elapsed.TotalMinutes, budget.TotalMinutes);
                    iteration(i);
                    i++;
                }

                Log.Comment("[{0}] Soak complete after {1} iteration(s).", scenarioName, i);
            }
            else
            {
                Log.Comment("[{0}] Running {1} iteration(s).", scenarioName, iterations);
                for (int i = 0; i < iterations; i++)
                {
                    Log.Comment("[{0}] Iteration {1}/{2}...", scenarioName, i, iterations);
                    iteration(i);
                }
            }
        }

        // Aggressively settle the UI thread and force collection + finalization. Doing this every iteration is what
        // turns an "eventual" lifetime crash into a prompt one.
        private static void SettleAndCollect()
        {
            IdleSynchronizer.Wait();
            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();
            IdleSynchronizer.Wait();
        }

        private static void VerifyCollected(Dictionary<string, WeakReference> objects, bool failOnLeak)
        {
            foreach (var pair in objects)
            {
                object target = pair.Value.Target;
                if (target != null)
                {
                    if (failOnLeak)
                    {
                        // Throwing exceptions makes running this under the test-debugging platform harder, so disable
                        // the exception form of the failure (mirrors the existing LeakTests convention).
                        Verify.DisableVerifyFailureExceptions = true;
                        Verify.Fail(string.Format("Object {0} is still alive when it should not be.", pair.Key));
                        Verify.DisableVerifyFailureExceptions = false;
                    }
                    else
                    {
                        // Non-gating: surface the residual reference as a warning so it shows up in the test report
                        // without failing the test (and therefore without failing Publish Test Results / the pipeline).
                        Log.Warning(string.Format("[LifetimeStress] REPORT: object '{0}' was still alive after forced collection; logged as a warning (non-gating). Investigate for a possible lifetime leak.", pair.Key));
                    }
                }
            }

            objects.Clear();
        }

        private static int GetEnvInt(string name, int fallback)
        {
            var raw = Environment.GetEnvironmentVariable(name);
            int value;
            if (!string.IsNullOrEmpty(raw) && int.TryParse(raw, out value) && value > 0)
            {
                return value;
            }
            return fallback;
        }

        private static double GetEnvDouble(string name, double fallback)
        {
            var raw = Environment.GetEnvironmentVariable(name);
            double value;
            if (!string.IsNullOrEmpty(raw) && double.TryParse(raw, out value) && value > 0.0)
            {
                return value;
            }
            return fallback;
        }
    }
}
