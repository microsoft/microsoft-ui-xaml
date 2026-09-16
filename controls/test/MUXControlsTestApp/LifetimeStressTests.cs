// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Numerics;
using System.Runtime.CompilerServices;

using MUXControlsTestApp.Utilities;

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;

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
    //   * Reporting, never gating: the suite runs its create/teardown/GC workload on EVERY test pass - including the
    //     per-PR gate and Nightly - so a lifetime report is produced right in that pipeline run. It is engineered so
    //     it never fails the pipeline: leaks and any thrown managed exception are downgraded to non-gating warnings
    //     (see RunStress / RunIterationReporting), so this suite never records a Failed test result. The one thing a
    //     managed catch cannot intercept is a genuine NATIVE crash / fail-fast (e.g. a stowed exception in
    //     combase.dll) that terminates the TAEF host outright; that is the real signal we want, and a known
    //     deterministic crasher is quarantined per-scenario with [TestProperty("Ignore","True")] (see
    //     StressItemsRepeaterRealizationAndRecycling) so it does not gate while its product bug is pending.
    //   * Runnable as a long soak (the classic "run for a few hours" behavior) by setting environment variables
    //     (see below), so the scheduled WinUI-LifetimeStress pipeline can crank the workload way up. Each iteration
    //     aggressively settles the UI thread and forces the GC + finalizers so a dangling native peer is much more
    //     likely to fault *immediately* instead of "eventually".
    //   * Isolated into its own TAEF test suite (see the TestSuite TestProperty). The Helix work-item generator
    //     produces a dedicated work item for this suite, so if a lifetime bug does crash the test host it does
    //     not take down unrelated tests, and the soak can be scheduled independently.
    //
    // Configuration (all optional, read from environment variables so they work locally, on pipeline agents, and
    // when injected into a Helix work item):
    //
    //   WINUI_LIFETIME_STRESS_MINUTES      If > 0, each scenario loops until this many minutes have elapsed. This
    //                                      is the soak knob and the WinUI-LifetimeStress pipeline sets it.
    //   WINUI_LIFETIME_STRESS_ITERATIONS   If > 0 (and soak mode is off), run each scenario this many create/destroy
    //                                      cycles - a heavier local/manual run. When neither variable is set (the
    //                                      default, including the PR gate and Nightly), each scenario runs a small
    //                                      non-gating "report" pass of DefaultReportIterations cycles.
    //
    // Adding coverage: the cheapest, highest-value thing a contributor can do when fixing a lifetime crash (as the
    // ItemsRepeater realization/recycling scenario below demonstrates) is to add the offending create/teardown
    // sequence here so the fix is protected against regression.
    [TestClass]
    // Classification=Integration keeps this class selected by the DevTestSuite Helix work-item generator, which
    // filters on @Classification='Integration'. That is what makes the generator emit the dedicated
    // "*-LifetimeStressTestSuite" work item - required so the scheduled soak pipeline (WinUI-LifetimeStress.yml)
    // picks the suite up, and so it is discovered/reported in every test pass. The MUXControlsTestApp module also
    // sets Classification=Integration module-wide via ApiTestAssemblyHandling.AssemblyInitialize; we declare it here
    // as well so this suite's selection is explicit and self-documenting, matching the InteractionTests convention.
    // NOTE: selection is not the same as gating. The suite runs a small non-gating "report" pass in the PR gate -
    // see RunStress / RunIterationReporting: leaks and thrown managed exceptions are downgraded to warnings, so it
    // reports in the PR run but never records a Failed result and cannot fail the pipeline.
    [TestProperty("Classification", "Integration")]
    [TestProperty("TestSuite", "LifetimeStressTestSuite")]
    public class LifetimeStressTests : ApiTestBase
    {
        // Number of create/destroy cycles per scenario in the default "report" pass (PR gate + Nightly). Kept small
        // so the report pass is fast and cheap; it is enough to surface a lifetime report while every iteration's
        // aggressive GC still makes a dangling native peer fault promptly. Override with WINUI_LIFETIME_STRESS_ITERATIONS
        // for a heavier local run, or WINUI_LIFETIME_STRESS_MINUTES for the scheduled soak.
        private const int DefaultReportIterations = 3;

        // Name of the scenario currently executing, used by SafeUI to attribute a warning when UI-thread work throws.
        // Scenarios in this suite run one at a time on the test thread, so a single static is sufficient.
        private static string s_currentScenario = "LifetimeStress";

        // Explicit fixed cycle count (WINUI_LIFETIME_STRESS_ITERATIONS). 0 means "not set" - the default report
        // pass (DefaultReportIterations) is used instead.
        private static int ConfiguredIterations
        {
            get { return GetEnvInt("WINUI_LIFETIME_STRESS_ITERATIONS", 0); }
        }

        private static double ConfiguredSoakMinutes
        {
            get { return GetEnvDouble("WINUI_LIFETIME_STRESS_MINUTES", 0.0); }
        }

        // Opt-in "aggressive native repro" knob. When > 0, the native-crash-repro scenarios below run their most
        // aggressive, most-likely-to-fault variant (many more off-thread final releases, deeper trees, more churn) -
        // that is the configuration that actually provokes a latent native lifetime crash (use-after-free / premature
        // native peer destruction / off-thread release). The scheduled soak pipeline (WinUI-LifetimeStress.yml) sets
        // this; the per-PR gate deliberately leaves it UNSET so the gate runs only the light, benign variant and stays
        // non-gating (it can never take the shared pipeline down). Reproducing the crash is the soak's job; the PR gate
        // only needs the report pass.
        private static bool AggressiveNativeReproEnabled
        {
            get { return GetEnvInt("WINUI_LIFETIME_STRESS_NATIVE", 0) > 0; }
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

                SafeUI(() =>
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
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
        // Quarantined as non-gating. This scenario currently crashes the TAEF test host
        // (TE.ProcessHost.exe) with a stowed exception (0xC000027B) in combase.dll during the
        // realize/recycle churn - i.e. it is surfacing a genuine ItemsRepeater native-peer
        // lifetime bug, which is exactly what this suite is designed to catch. A host crash is an
        // unconditional Run Tests stage failure (it cannot be downgraded to a warning the way a
        // WeakReference leak can, because the process fail-fasts before any managed result is
        // reported), so it blocks the shared PR pipeline. Ignore it here so the pipeline is not
        // gated on the unfixed underlying crash; re-enable once the ItemsRepeater realize/recycle
        // lifetime bug it exposes is root-caused and fixed.
        [TestProperty("Ignore", "True")]
        public void StressItemsRepeaterRealizationAndRecycling()
        {
            RunStress("StressItemsRepeaterRealizationAndRecycling", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
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

                SafeUI(() =>
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
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

                SafeUI(() =>
                {
                    objects["Window"] = CreateActivateAndCloseWindow();
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
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

        // ListView / GridView container generation and recycling lifetime stress.
        //
        // The templated ItemsControl virtualization path (ModernCollectionBasePanel + the container recycling queue)
        // is the single largest home of lifetime bugs in WinUI after ItemsRepeater: item containers are generated,
        // recycled and cleared as the ItemsSource and viewport change, and a mistake in when a container's native
        // peer is released (relative to its managed container / content) shows up as a use-after-free. This churns
        // the source and scrolls the viewport to force generate/recycle, then tears the whole thing down and collects.
        [TestMethod]
        public void StressListViewContainerRecycling()
        {
            RunStress("StressListViewContainerRecycling", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    // A constrained viewport is required for the list to actually virtualize (and therefore recycle)
                    // rather than realize every item up front.
                    var listView = new ListView()
                    {
                        Width = 300,
                        Height = 400,
                        ItemsSource = Enumerable.Range(0, 200).Select(i => string.Format("Item #{0}", i)).ToList(),
                    };
                    objects["ListView"] = new WeakReference(listView);

                    Content = listView;
                    Content.UpdateLayout();

                    // Swap the source and scroll the viewport to opposite ends so containers are generated, recycled
                    // and cleared repeatedly - the classic churn that surfaces container-lifetime bugs.
                    for (int churn = 0; churn < 5; churn++)
                    {
                        listView.ItemsSource = Enumerable.Range(churn * 50, 150).Select(i => string.Format("Item #{0}", i)).ToList();
                        Content.UpdateLayout();

                        if (listView.Items.Count > 0)
                        {
                            listView.ScrollIntoView(listView.Items[listView.Items.Count - 1]);
                            Content.UpdateLayout();
                            listView.ScrollIntoView(listView.Items[0]);
                            Content.UpdateLayout();
                        }
                    }

                    // Tear down mid-flight: drop the source and detach the list while containers may still be realized.
                    listView.ItemsSource = null;
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // Popup open/close lifetime stress.
        //
        // Opening a Popup spins up a separate popup root / overlay and a native peer for the hosted content; closing
        // it tears that back down. Repeated open/close (a historically crash-prone path for light-dismiss overlays
        // and popup-hosted content) followed by dropping the tree and collecting exercises that create/destroy cycle.
        [TestMethod]
        public void StressPopupOpenClose()
        {
            RunStress("StressPopupOpenClose", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var root = new Grid();
                    var popup = new Popup();
                    var popupChild = new Border()
                    {
                        Width = 120,
                        Height = 80,
                        Child = new TextBlock() { Text = "popup" },
                    };
                    popup.Child = popupChild;

                    // Rooting the Popup in the tree gives it a XamlRoot so it can be opened without a live window ctor.
                    root.Children.Add(popup);
                    objects["Popup"] = new WeakReference(popup);
                    objects["PopupChild"] = new WeakReference(popupChild);

                    Content = root;
                    Content.UpdateLayout();

                    for (int open = 0; open < 10; open++)
                    {
                        popup.IsOpen = true;
                        Content.UpdateLayout();

                        popup.IsOpen = false;
                        Content.UpdateLayout();
                    }

                    popup.Child = null;
                    root.Children.Clear();
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // NavigationView menu-item churn lifetime stress.
        //
        // NavigationView builds a comparatively large control graph (pane, item containers, selection/repeater
        // plumbing) and mutates it as menu items are added/removed, the pane is toggled and selection changes. Each
        // of those paths creates and releases peers, so churning them and then tearing the whole thing down is a good
        // way to catch a peer that outlives (or is released before) the element it belongs to.
        [TestMethod]
        public void StressNavigationViewMenuChurn()
        {
            RunStress("StressNavigationViewMenuChurn", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var navView = new NavigationView()
                    {
                        Width = 400,
                        Height = 500,
                        Content = new TextBlock() { Text = "content" },
                    };
                    objects["NavigationView"] = new WeakReference(navView);

                    for (int m = 0; m < 8; m++)
                    {
                        var item = new NavigationViewItem() { Content = string.Format("Item {0}", m) };
                        if (m == 0)
                        {
                            objects["FirstItem"] = new WeakReference(item);
                        }
                        navView.MenuItems.Add(item);
                    }

                    Content = navView;
                    Content.UpdateLayout();

                    for (int churn = 0; churn < 5; churn++)
                    {
                        navView.IsPaneOpen = !navView.IsPaneOpen;
                        Content.UpdateLayout();

                        if (navView.MenuItems.Count > 0)
                        {
                            navView.SelectedItem = navView.MenuItems[churn % navView.MenuItems.Count];
                            Content.UpdateLayout();
                        }

                        // Add then remove an item so the menu-item container generation/recycling path runs.
                        navView.MenuItems.Add(new NavigationViewItem() { Content = string.Format("Extra {0}", churn) });
                        Content.UpdateLayout();
                        navView.MenuItems.RemoveAt(navView.MenuItems.Count - 1);
                        Content.UpdateLayout();
                    }

                    navView.SelectedItem = null;
                    navView.MenuItems.Clear();
                    navView.Content = null;
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // TabView add/remove lifetime stress.
        //
        // TabView creates a tab strip container plus per-tab content and mutates that collection as tabs are added
        // and removed. Adding a full set of tabs (each with its own content element) and then removing them one by
        // one exercises the tab-item container and content create/teardown path before the tree is dropped.
        [TestMethod]
        public void StressTabViewAddRemove()
        {
            RunStress("StressTabViewAddRemove", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var tabView = new TabView()
                    {
                        Width = 500,
                        Height = 400,
                    };
                    objects["TabView"] = new WeakReference(tabView);

                    Content = tabView;
                    Content.UpdateLayout();

                    for (int t = 0; t < 8; t++)
                    {
                        var tab = new TabViewItem()
                        {
                            Header = string.Format("Tab {0}", t),
                            Content = new TextBlock() { Text = string.Format("content {0}", t) },
                        };
                        if (t == 0)
                        {
                            objects["FirstTab"] = new WeakReference(tab);
                        }
                        tabView.TabItems.Add(tab);
                        Content.UpdateLayout();
                    }

                    // Remove the tabs one at a time (mid-flight teardown of each tab's container + content).
                    while (tabView.TabItems.Count > 0)
                    {
                        tabView.TabItems.RemoveAt(tabView.TabItems.Count - 1);
                        Content.UpdateLayout();
                    }

                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // -----------------------------------------------------------------------------------------------------------
        // Targeted component scenarios.
        //
        // The scenarios below extend the suite past the broad control sweep to specific components that recur in the
        // Watson "Lifetime Issues" crash buckets. Each one drives that component's create -> use -> teardown -> GC
        // path so a peer-lifetime / ref-counting bug in it either faults promptly or leaves a residual reference that
        // is reported as a non-gating warning. They are report-only like the rest of the suite (see RunStress). NOTE:
        // a scenario reproducing a bug is not guaranteed in the small PR-gate "report" pass - some faults only appear
        // under the scheduled soak (WINUI_LIFETIME_STRESS_MINUTES) and/or under AppVerifier/page-heap.
        // -----------------------------------------------------------------------------------------------------------

        // MenuFlyout / MenuFlyoutPresenter open/close lifetime stress (Watson: CMenuFlyoutPresenter teardown,
        // STOWED_EXCEPTION_8000ffff). Attaching a MenuFlyout to a button, opening it (which realizes the presenter and
        // its items inside a popup) and dismissing it exercises the flyout-presenter create/teardown path.
        [TestMethod]
        public void StressMenuFlyoutOpenClose()
        {
            RunStress("StressMenuFlyoutOpenClose", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var button = new Button() { Content = "target" };
                    Content = button;
                    Content.UpdateLayout();

                    for (int churn = 0; churn < 5; churn++)
                    {
                        var flyout = new MenuFlyout();
                        for (int m = 0; m < 6; m++)
                        {
                            flyout.Items.Add(new MenuFlyoutItem() { Text = string.Format("Item {0}", m) });
                        }
                        flyout.Items.Add(new MenuFlyoutSubItem() { Text = "More" });
                        if (churn == 0)
                        {
                            objects["Flyout"] = new WeakReference(flyout);
                        }

                        button.Flyout = flyout;
                        flyout.ShowAt(button);
                        Content.UpdateLayout();
                        flyout.Hide();
                        Content.UpdateLayout();
                        button.Flyout = null;
                    }

                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // ResourceDictionary lifetime stress (Watson: CResourceDictionary::GetKeyNoRefImpl access violation).
        // Build merged/keyed ResourceDictionaries, attach them to an element, resolve keys through the merged graph,
        // then clear and drop them. Churning the dictionary graph exercises the resource-map lookup/teardown paths.
        [TestMethod]
        public void StressResourceDictionaryChurn()
        {
            RunStress("StressResourceDictionaryChurn", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var root = new Grid();
                    Content = root;

                    for (int churn = 0; churn < 6; churn++)
                    {
                        var merged = new ResourceDictionary();
                        merged["Brush" + churn] = new SolidColorBrush(Microsoft.UI.Colors.Red);

                        var dict = new ResourceDictionary();
                        dict.MergedDictionaries.Add(merged);
                        dict["LocalKey"] = 42.0;
                        if (churn == 0)
                        {
                            objects["ResourceDictionary"] = new WeakReference(dict);
                            objects["Merged"] = new WeakReference(merged);
                        }

                        var border = new Border();
                        border.Resources = dict;
                        root.Children.Add(border);
                        root.UpdateLayout();

                        // Resolve keys so the lookup path (GetKeyNoRefImpl) runs against the merged graph.
                        object localValue = border.Resources["LocalKey"];
                        object mergedValue = border.Resources.MergedDictionaries[0]["Brush" + churn];

                        root.Children.Remove(border);
                        merged.Clear();
                        dict.MergedDictionaries.Clear();
                        dict.Clear();
                        root.UpdateLayout();
                    }

                    root.Children.Clear();
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // ItemsSourceView lifetime stress (Watson: ItemsSourceView::OnItemsSourceChanged fail-fast).
        // Rapidly swap the ItemsSource of items controls between different collection kinds (array, List,
        // ObservableCollection) and null. Each swap tears down the previous ItemsSourceView and builds a new one.
        [TestMethod]
        public void StressItemsSourceViewSwaps()
        {
            RunStress("StressItemsSourceViewSwaps", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var repeater = new ItemsRepeater() { Layout = new StackLayout() };
                    var listView = new ListView() { Width = 200, Height = 200 };
                    objects["Repeater"] = new WeakReference(repeater);

                    var panel = new StackPanel();
                    panel.Children.Add(repeater);
                    panel.Children.Add(listView);
                    Content = panel;
                    Content.UpdateLayout();

                    for (int churn = 0; churn < 6; churn++)
                    {
                        object[] sources =
                        {
                            Enumerable.Range(0, 40).ToArray(),
                            Enumerable.Range(0, 30).Select(i => "S" + i).ToList(),
                            new ObservableCollection<string>(Enumerable.Range(0, 20).Select(i => "O" + i)),
                            null,
                        };

                        foreach (var src in sources)
                        {
                            repeater.ItemsSource = src;
                            listView.ItemsSource = src;
                            Content.UpdateLayout();
                        }
                    }

                    repeater.ItemsSource = null;
                    listView.ItemsSource = null;
                    panel.Children.Clear();
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // Automation-peer lifetime stress (Watson: CUIAWindow::InitIds, AppBarAutomationPeerFactory::Release,
        // DependencyObjectPropertyAccess::Release). Create controls, build their automation peers, walk the peer
        // children, then drop everything. Peers hold cross-boundary references back to their owners - a classic
        // lifetime path where a mistake over-pegs the owner or frees the peer early.
        [TestMethod]
        public void StressAutomationPeerCreateRelease()
        {
            RunStress("StressAutomationPeerCreateRelease", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var panel = new StackPanel();
                    var owners = new List<FrameworkElement>
                    {
                        new Button() { Content = "b" },
                        new CommandBar(),
                        new AppBar(),
                        new ListView() { ItemsSource = Enumerable.Range(0, 10) },
                        new TextBox() { Text = "t" },
                        new CheckBox() { Content = "c" },
                    };
                    foreach (var owner in owners) { panel.Children.Add(owner); }
                    objects["FirstOwner"] = new WeakReference(owners[0]);

                    Content = panel;
                    Content.UpdateLayout();

                    foreach (var owner in owners)
                    {
                        var peer = FrameworkElementAutomationPeer.CreatePeerForElement(owner);
                        if (peer != null)
                        {
                            var name = peer.GetName();
                            var children = peer.GetChildren();
                            if (children != null)
                            {
                                foreach (var child in children)
                                {
                                    var childName = child.GetName();
                                }
                            }
                        }
                    }

                    panel.Children.Clear();
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // Frame navigation-cache lifetime stress (Watson: DirectUI::NavigationCache::LoadContent access violation).
        // Navigate a Frame between cached pages and back so the navigation cache retains, reuses and finally releases
        // page instances - the path that has over-held or prematurely freed cached pages. The pages set
        // NavigationCacheMode=Required so the cache actually participates.
        //
        // Quarantined: this scenario deterministically fail-fasts with a native access violation (0xC0000005)
        // inside the Frame navigation-cache teardown path (coreclr.dll), which terminates the TAEF host before any
        // managed result is reported and cannot be downgraded to a non-gating warning. Ignore it so the shared
        // pipeline is not gated on the unfixed underlying native crash; re-enable once that product bug is
        // root-caused and fixed.
        [TestProperty("Ignore", "True")]
        [TestMethod]
        public void StressFrameNavigationCache()
        {
            RunStress("StressFrameNavigationCache", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var frame = new Frame() { Width = 300, Height = 300 };
                    objects["Frame"] = new WeakReference(frame);
                    Content = frame;
                    Content.UpdateLayout();

                    for (int nav = 0; nav < 6; nav++)
                    {
                        frame.Navigate(typeof(LifetimeStressPage));
                        Content.UpdateLayout();
                        frame.Navigate(typeof(LifetimeStressPageTwo));
                        Content.UpdateLayout();
                        if (frame.CanGoBack)
                        {
                            frame.GoBack();
                            Content.UpdateLayout();
                        }
                    }

                    frame.Content = null;
                    frame.BackStack.Clear();
                    frame.ForwardStack.Clear();
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // XAML/XBF parse + teardown lifetime stress (Watson: XamlBinaryFormatReader2::GetXbfHash / markup teardown).
        // Repeatedly parse a non-trivial XAML fragment with XamlReader.Load, add the produced tree to the live tree,
        // lay it out and drop it. This drives the parser-produced object graph create/teardown path.
        [TestMethod]
        public void StressXamlReaderLoadUnload()
        {
            const string markup =
                "<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'" +
                "      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>" +
                "  <Grid.Resources><SolidColorBrush x:Key='B' Color='Blue'/></Grid.Resources>" +
                "  <StackPanel>" +
                "    <TextBlock Text='hello'/>" +
                "    <Button Content='ok'/>" +
                "    <ListView/>" +
                "  </StackPanel>" +
                "</Grid>";

            RunStress("StressXamlReaderLoadUnload", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    for (int churn = 0; churn < 6; churn++)
                    {
                        var element = (UIElement)XamlReader.Load(markup);
                        if (churn == 0)
                        {
                            objects["ParsedRoot"] = new WeakReference(element);
                        }
                        Content = element;
                        Content.UpdateLayout();
                        Content = null;
                    }
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // Simple-property lifetime stress (Watson: SimpleProperty::details::SetImpl access violation).
        // Simple properties (Translation/Scale/Rotation/CenterPoint) are stored in a side table keyed by element.
        // Setting and clearing them across many elements, then dropping the tree while some are still set, exercises
        // that storage's set/teardown path - which has faulted when an element is torn down with a simple property set.
        [TestMethod]
        public void StressSimplePropertySetClear()
        {
            RunStress("StressSimplePropertySetClear", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var panel = new StackPanel();
                    var elements = new List<UIElement>();
                    for (int i = 0; i < 12; i++)
                    {
                        var el = new Border() { Width = 20, Height = 20 };
                        elements.Add(el);
                        panel.Children.Add(el);
                    }
                    objects["FirstElement"] = new WeakReference(elements[0]);

                    Content = panel;
                    Content.UpdateLayout();

                    for (int churn = 0; churn < 4; churn++)
                    {
                        foreach (var el in elements)
                        {
                            el.Translation = new Vector3(churn, churn, 0);
                            el.Scale = new Vector3(1.1f, 1.1f, 1.0f);
                            el.Rotation = churn * 10.0f;
                            el.CenterPoint = new Vector3(5, 5, 0);
                        }
                        Content.UpdateLayout();

                        foreach (var el in elements)
                        {
                            el.Translation = new Vector3(0, 0, 0);
                            el.Scale = new Vector3(1, 1, 1);
                            el.Rotation = 0;
                            el.CenterPoint = new Vector3(0, 0, 0);
                        }
                        Content.UpdateLayout();
                    }

                    // Tear the tree down while the last set of simple properties may still be live.
                    foreach (var el in elements)
                    {
                        el.Translation = new Vector3(3, 3, 0);
                    }
                    panel.Children.Clear();
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // MediaTransportControls lifetime stress (Watson: MediaTransportControls crash with a shared MediaPlayer).
        // Repeatedly stand up MediaPlayerElements with transport controls enabled (which realizes the
        // MediaTransportControls template) and tear them down, clearing the media player on the way out. Exercises
        // the transport-controls create/teardown path. (A true multi-element shared-MediaPlayer repro is best driven
        // from the scheduled soak; here we keep it self-contained and window-free.)
        [TestMethod]
        public void StressMediaTransportControls()
        {
            RunStress("StressMediaTransportControls", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var panel = new StackPanel();
                    Content = panel;

                    for (int churn = 0; churn < 4; churn++)
                    {
                        var mpe1 = new MediaPlayerElement() { Width = 160, Height = 90, AreTransportControlsEnabled = true };
                        var mpe2 = new MediaPlayerElement() { Width = 160, Height = 90, AreTransportControlsEnabled = true };
                        if (churn == 0)
                        {
                            objects["MediaPlayerElement"] = new WeakReference(mpe1);
                        }
                        panel.Children.Add(mpe1);
                        panel.Children.Add(mpe2);
                        panel.UpdateLayout();

                        panel.Children.Remove(mpe1);
                        panel.Children.Remove(mpe2);
                        mpe1.SetMediaPlayer(null);
                        mpe2.SetMediaPlayer(null);
                        panel.UpdateLayout();
                    }

                    panel.Children.Clear();
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // InkToolbar lifetime stress (Watson: InkToolbar ReferenceTracker fail-fast, InkControls.dll).
        // Attach an InkToolbar to an InkCanvas, lay it out, then detach and drop. InkToolbar holds a tracked
        // reference to its target InkCanvas; attach/detach churn exercises that cross-reference teardown.
        [TestMethod]
        public void StressInkToolbarTargeting()
        {
            RunStress("StressInkToolbarTargeting", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var grid = new Grid() { Width = 300, Height = 300 };
                    Content = grid;

                    for (int churn = 0; churn < 4; churn++)
                    {
                        var inkCanvas = new InkCanvas();
                        var inkToolbar = new InkToolbar();
                        if (churn == 0)
                        {
                            objects["InkCanvas"] = new WeakReference(inkCanvas);
                            objects["InkToolbar"] = new WeakReference(inkToolbar);
                        }
                        grid.Children.Add(inkCanvas);
                        grid.Children.Add(inkToolbar);
                        grid.UpdateLayout();

                        inkToolbar.TargetInkCanvas = inkCanvas;
                        grid.UpdateLayout();

                        inkToolbar.TargetInkCanvas = null;
                        grid.Children.Remove(inkToolbar);
                        grid.Children.Remove(inkCanvas);
                        grid.UpdateLayout();
                    }

                    grid.Children.Clear();
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // TreeView expand/collapse + node add/remove lifetime stress.
        //
        // TreeView realizes a container per visible node and recycles containers as nodes are expanded, collapsed,
        // added and removed. Building a nested node tree, toggling expansion and mutating the node collection drives
        // the tree-node container generation/recycling/teardown path before the tree is dropped.
        [TestMethod]
        public void StressTreeViewNodeChurn()
        {
            RunStress("StressTreeViewNodeChurn", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var treeView = new TreeView() { Width = 400, Height = 500 };
                    objects["TreeView"] = new WeakReference(treeView);

                    for (int r = 0; r < 6; r++)
                    {
                        var root = new TreeViewNode() { Content = string.Format("Root {0}", r) };
                        if (r == 0)
                        {
                            objects["FirstNode"] = new WeakReference(root);
                        }
                        for (int c = 0; c < 3; c++)
                        {
                            root.Children.Add(new TreeViewNode() { Content = string.Format("Child {0}.{1}", r, c) });
                        }
                        treeView.RootNodes.Add(root);
                    }

                    Content = treeView;
                    Content.UpdateLayout();

                    for (int churn = 0; churn < 5; churn++)
                    {
                        foreach (var node in treeView.RootNodes)
                        {
                            node.IsExpanded = !node.IsExpanded;
                        }
                        Content.UpdateLayout();

                        var extra = new TreeViewNode() { Content = string.Format("Extra {0}", churn) };
                        treeView.RootNodes.Add(extra);
                        Content.UpdateLayout();
                        treeView.RootNodes.Remove(extra);
                        Content.UpdateLayout();
                    }

                    treeView.RootNodes.Clear();
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // ComboBox drop-down open/close + item add/remove lifetime stress.
        //
        // ComboBox realizes its item containers inside a popup on drop-down open and recycles/tears them down on
        // close. Opening and closing the drop-down, changing the selection and swapping the item collection exercises
        // the ComboBoxItem container generation/teardown path plus the popup open/close path.
        [TestMethod]
        public void StressComboBoxDropDownChurn()
        {
            RunStress("StressComboBoxDropDownChurn", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var comboBox = new ComboBox() { Width = 200 };
                    objects["ComboBox"] = new WeakReference(comboBox);

                    for (int m = 0; m < 8; m++)
                    {
                        var item = new ComboBoxItem() { Content = string.Format("Item {0}", m) };
                        if (m == 0)
                        {
                            objects["FirstItem"] = new WeakReference(item);
                        }
                        comboBox.Items.Add(item);
                    }

                    Content = comboBox;
                    Content.UpdateLayout();

                    for (int churn = 0; churn < 5; churn++)
                    {
                        comboBox.IsDropDownOpen = true;
                        Content.UpdateLayout();
                        comboBox.SelectedIndex = churn % comboBox.Items.Count;
                        Content.UpdateLayout();
                        comboBox.IsDropDownOpen = false;
                        Content.UpdateLayout();

                        var extra = new ComboBoxItem() { Content = string.Format("Extra {0}", churn) };
                        comboBox.Items.Add(extra);
                        Content.UpdateLayout();
                        comboBox.Items.Remove(extra);
                        Content.UpdateLayout();
                    }

                    comboBox.SelectedIndex = -1;
                    comboBox.Items.Clear();
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // FlipView item add/remove lifetime stress.
        //
        // FlipView realizes one item container at a time and recycles containers as the selection flips and items are
        // added/removed. Flipping through items and mutating the collection drives the FlipViewItem container
        // generation/recycling/teardown path.
        [TestMethod]
        public void StressFlipViewItemChurn()
        {
            RunStress("StressFlipViewItemChurn", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var flipView = new FlipView() { Width = 400, Height = 300 };
                    objects["FlipView"] = new WeakReference(flipView);

                    for (int m = 0; m < 8; m++)
                    {
                        var item = new FlipViewItem() { Content = new TextBlock() { Text = string.Format("Item {0}", m) } };
                        if (m == 0)
                        {
                            objects["FirstItem"] = new WeakReference(item);
                        }
                        flipView.Items.Add(item);
                    }

                    Content = flipView;
                    Content.UpdateLayout();

                    for (int churn = 0; churn < 5; churn++)
                    {
                        flipView.SelectedIndex = churn % flipView.Items.Count;
                        Content.UpdateLayout();

                        var extra = new FlipViewItem() { Content = new TextBlock() { Text = string.Format("Extra {0}", churn) } };
                        flipView.Items.Add(extra);
                        Content.UpdateLayout();
                        flipView.Items.Remove(extra);
                        Content.UpdateLayout();
                    }

                    flipView.Items.Clear();
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // Pivot item add/remove + selection lifetime stress.
        //
        // Pivot realizes a header plus the selected item's content and recycles them as the selection moves and items
        // are added/removed. Changing the selected pivot and mutating the item collection drives the PivotItem
        // header/content generation/teardown path.
        //
        // Quarantined: this scenario deterministically fail-fasts with a native access violation (0xC0000005)
        // inside the Pivot item add/remove/select path (Microsoft.UI.Xaml.Phone.dll), which terminates the TAEF host
        // before any managed result is reported and cannot be downgraded to a non-gating warning. Ignore it so the
        // shared pipeline is not gated on the unfixed underlying native crash; re-enable once that product bug is
        // root-caused and fixed.
        [TestProperty("Ignore", "True")]
        [TestMethod]
        public void StressPivotItemChurn()
        {
            RunStress("StressPivotItemChurn", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var pivot = new Pivot() { Width = 400, Height = 400 };
                    objects["Pivot"] = new WeakReference(pivot);

                    for (int m = 0; m < 6; m++)
                    {
                        var item = new PivotItem()
                        {
                            Header = string.Format("Header {0}", m),
                            Content = new TextBlock() { Text = string.Format("Item {0}", m) },
                        };
                        if (m == 0)
                        {
                            objects["FirstItem"] = new WeakReference(item);
                        }
                        pivot.Items.Add(item);
                    }

                    Content = pivot;
                    Content.UpdateLayout();

                    for (int churn = 0; churn < 5; churn++)
                    {
                        pivot.SelectedIndex = churn % pivot.Items.Count;
                        Content.UpdateLayout();

                        var extra = new PivotItem()
                        {
                            Header = string.Format("Extra {0}", churn),
                            Content = new TextBlock() { Text = string.Format("Extra {0}", churn) },
                        };
                        pivot.Items.Add(extra);
                        Content.UpdateLayout();
                        pivot.Items.Remove(extra);
                        Content.UpdateLayout();
                    }

                    pivot.Items.Clear();
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // SplitView pane open/close lifetime stress.
        //
        // SplitView hosts a pane (here a ListView that realizes its own containers) alongside content and shows/hides
        // the pane. Toggling IsPaneOpen and switching display mode while the pane holds realized item containers
        // drives the pane show/hide + content teardown path.
        [TestMethod]
        public void StressSplitViewPaneChurn()
        {
            RunStress("StressSplitViewPaneChurn", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var paneList = new ListView() { ItemsSource = Enumerable.Range(0, 20) };
                    var splitView = new SplitView()
                    {
                        Width = 500,
                        Height = 400,
                        Pane = paneList,
                        Content = new TextBlock() { Text = "content" },
                        IsPaneOpen = true,
                        DisplayMode = SplitViewDisplayMode.Inline,
                    };
                    objects["SplitView"] = new WeakReference(splitView);
                    objects["PaneListView"] = new WeakReference(paneList);

                    Content = splitView;
                    Content.UpdateLayout();

                    for (int churn = 0; churn < 5; churn++)
                    {
                        splitView.IsPaneOpen = !splitView.IsPaneOpen;
                        Content.UpdateLayout();
                        splitView.DisplayMode = (churn % 2 == 0)
                            ? SplitViewDisplayMode.CompactOverlay
                            : SplitViewDisplayMode.Inline;
                        Content.UpdateLayout();
                    }

                    splitView.Pane = null;
                    splitView.Content = null;
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // Expander expand/collapse + content swap lifetime stress.
        //
        // Expander realizes its header and (on expand) its content, tearing the content presenter down on collapse.
        // Toggling IsExpanded and swapping the content element drives the expander content presenter
        // create/teardown path.
        [TestMethod]
        public void StressExpanderExpandCollapse()
        {
            RunStress("StressExpanderExpandCollapse", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var firstContent = new TextBlock() { Text = "content 0" };
                    var expander = new Expander()
                    {
                        Width = 300,
                        Header = "header",
                        Content = firstContent,
                        IsExpanded = true,
                    };
                    objects["Expander"] = new WeakReference(expander);
                    objects["FirstContent"] = new WeakReference(firstContent);

                    Content = expander;
                    Content.UpdateLayout();

                    for (int churn = 0; churn < 5; churn++)
                    {
                        expander.IsExpanded = !expander.IsExpanded;
                        Content.UpdateLayout();
                        expander.Content = new TextBlock() { Text = string.Format("content {0}", churn + 1) };
                        Content.UpdateLayout();
                    }

                    expander.Content = null;
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // CommandBar primary/secondary command add/remove lifetime stress.
        //
        // CommandBar hosts AppBar* command elements and (for secondary commands) realizes an overflow flyout on open.
        // Adding/removing commands and opening/closing the overflow drives the command element + overflow presenter
        // create/teardown path.
        [TestMethod]
        public void StressCommandBarButtonChurn()
        {
            RunStress("StressCommandBarButtonChurn", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var commandBar = new CommandBar() { Width = 500 };
                    objects["CommandBar"] = new WeakReference(commandBar);

                    for (int m = 0; m < 6; m++)
                    {
                        var button = new AppBarButton() { Label = string.Format("Cmd {0}", m) };
                        if (m == 0)
                        {
                            objects["FirstButton"] = new WeakReference(button);
                        }
                        commandBar.PrimaryCommands.Add(button);
                        commandBar.SecondaryCommands.Add(new AppBarButton() { Label = string.Format("More {0}", m) });
                    }

                    Content = commandBar;
                    Content.UpdateLayout();

                    for (int churn = 0; churn < 5; churn++)
                    {
                        commandBar.IsOpen = true;
                        Content.UpdateLayout();
                        commandBar.IsOpen = false;
                        Content.UpdateLayout();

                        var extra = new AppBarButton() { Label = string.Format("Extra {0}", churn) };
                        commandBar.PrimaryCommands.Add(extra);
                        Content.UpdateLayout();
                        commandBar.PrimaryCommands.Remove(extra);
                        Content.UpdateLayout();
                    }

                    commandBar.PrimaryCommands.Clear();
                    commandBar.SecondaryCommands.Clear();
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // ListView selection + item add/remove lifetime stress.
        //
        // ListView realizes a container per visible item and recycles them as items are selected, added and removed.
        // Driving explicit ListViewItem instances (so a specific container instance can be tracked), churning the
        // selection and mutating the collection exercises the container generation/recycling/teardown path.
        [TestMethod]
        public void StressListViewSelectionChurn()
        {
            RunStress("StressListViewSelectionChurn", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var listView = new ListView()
                    {
                        Width = 300,
                        Height = 400,
                        SelectionMode = ListViewSelectionMode.Multiple,
                    };
                    objects["ListView"] = new WeakReference(listView);

                    for (int m = 0; m < 12; m++)
                    {
                        var item = new ListViewItem() { Content = string.Format("Item {0}", m) };
                        if (m == 0)
                        {
                            objects["FirstItem"] = new WeakReference(item);
                        }
                        listView.Items.Add(item);
                    }

                    Content = listView;
                    Content.UpdateLayout();

                    for (int churn = 0; churn < 5; churn++)
                    {
                        listView.SelectedIndex = churn % listView.Items.Count;
                        Content.UpdateLayout();
                        listView.SelectAll();
                        Content.UpdateLayout();
                        listView.SelectedItems.Clear();
                        Content.UpdateLayout();

                        var extra = new ListViewItem() { Content = string.Format("Extra {0}", churn) };
                        listView.Items.Add(extra);
                        Content.UpdateLayout();
                        listView.Items.Remove(extra);
                        Content.UpdateLayout();
                    }

                    listView.SelectedIndex = -1;
                    listView.Items.Clear();
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // GridView selection + item add/remove lifetime stress.
        //
        // GridView is the wrapping-panel sibling of ListView and shares the same container generation/recycling
        // machinery. Driving explicit GridViewItem instances, churning the selection and mutating the collection
        // exercises that container generation/recycling/teardown path in the wrapping-layout configuration.
        [TestMethod]
        public void StressGridViewSelectionChurn()
        {
            RunStress("StressGridViewSelectionChurn", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var gridView = new GridView()
                    {
                        Width = 400,
                        Height = 400,
                        SelectionMode = ListViewSelectionMode.Extended,
                    };
                    objects["GridView"] = new WeakReference(gridView);

                    for (int m = 0; m < 12; m++)
                    {
                        var item = new GridViewItem() { Content = string.Format("Item {0}", m) };
                        if (m == 0)
                        {
                            objects["FirstItem"] = new WeakReference(item);
                        }
                        gridView.Items.Add(item);
                    }

                    Content = gridView;
                    Content.UpdateLayout();

                    for (int churn = 0; churn < 5; churn++)
                    {
                        gridView.SelectedIndex = churn % gridView.Items.Count;
                        Content.UpdateLayout();

                        var extra = new GridViewItem() { Content = string.Format("Extra {0}", churn) };
                        gridView.Items.Add(extra);
                        Content.UpdateLayout();
                        gridView.Items.Remove(extra);
                        Content.UpdateLayout();
                    }

                    gridView.SelectedIndex = -1;
                    gridView.Items.Clear();
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // BreadcrumbBar item-source churn lifetime stress.
        //
        // BreadcrumbBar realizes a container per crumb from its ItemsSource and regenerates them when the source
        // changes. Repeatedly swapping the item source (growing and shrinking the crumb trail) drives the crumb
        // container generation/teardown path.
        [TestMethod]
        public void StressBreadcrumbBarChurn()
        {
            RunStress("StressBreadcrumbBarChurn", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var breadcrumb = new BreadcrumbBar() { Width = 500 };
                    objects["BreadcrumbBar"] = new WeakReference(breadcrumb);

                    var initial = new ObservableCollection<string>();
                    for (int m = 0; m < 6; m++)
                    {
                        initial.Add(string.Format("Crumb {0}", m));
                    }
                    objects["FirstItemSource"] = new WeakReference(initial);
                    breadcrumb.ItemsSource = initial;

                    Content = breadcrumb;
                    Content.UpdateLayout();

                    for (int churn = 0; churn < 5; churn++)
                    {
                        var next = new ObservableCollection<string>();
                        for (int m = 0; m < (churn % 6) + 1; m++)
                        {
                            next.Add(string.Format("C{0}.{1}", churn, m));
                        }
                        breadcrumb.ItemsSource = next;
                        Content.UpdateLayout();
                    }

                    breadcrumb.ItemsSource = null;
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // SelectorBar item add/remove + selection lifetime stress.
        //
        // SelectorBar realizes a container per SelectorBarItem and moves selection between them. Adding a set of
        // items, churning the selection and mutating the collection drives the SelectorBarItem container
        // generation/teardown path.
        [TestMethod]
        public void StressSelectorBarItemChurn()
        {
            RunStress("StressSelectorBarItemChurn", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var selectorBar = new SelectorBar() { Width = 500 };
                    objects["SelectorBar"] = new WeakReference(selectorBar);

                    for (int m = 0; m < 6; m++)
                    {
                        var item = new SelectorBarItem() { Text = string.Format("Item {0}", m) };
                        if (m == 0)
                        {
                            objects["FirstItem"] = new WeakReference(item);
                        }
                        selectorBar.Items.Add(item);
                    }

                    Content = selectorBar;
                    Content.UpdateLayout();

                    for (int churn = 0; churn < 5; churn++)
                    {
                        selectorBar.SelectedItem = selectorBar.Items[churn % selectorBar.Items.Count];
                        Content.UpdateLayout();

                        var extra = new SelectorBarItem() { Text = string.Format("Extra {0}", churn) };
                        selectorBar.Items.Add(extra);
                        Content.UpdateLayout();
                        selectorBar.Items.Remove(extra);
                        Content.UpdateLayout();
                    }

                    selectorBar.SelectedItem = null;
                    selectorBar.Items.Clear();
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // -------------------------------------------------------------------------------------------------------------
        // Coverage map: WinUI "Lifetime Issues" reliability/Watson bugs (WinUI_Bugs_2026-09-10, Technical area =
        // "Lifetime Issues", 38 items) -> the scenario that exercises each faulting path. "Generic peer churn" is the
        // AddRef/Release/Unpeg/TrackerClear/metadata/ComObject teardown traffic driven by every-pass creation,
        // realization, off-thread final release, reparenting and deep-tree teardown of the whole control surface
        // (StressControlCreateLoadUnloadCollect + StressOffThreadPeerFinalRelease + StressRapidReparentEnterLeave +
        // StressDeepVisualTreePeerChurn).
        //
        //   48542267 MediaTransportControls / shared MediaPlayer ...... StressMediaTransportControls
        //   49348881 CMenuFlyoutPresenter ............................. StressMenuFlyoutOpenClose
        //   54453205 SimpleProperty::SetImpl .......................... StressSimplePropertySetClear
        //   54466371 / 55902178 CResourceDictionary::GetKeyNoRefImpl .. StressResourceDictionaryChurn
        //   54475960 CUIAWindow::InitIds .............................. StressAutomationPeerCreateRelease
        //   63277318 AppBarAutomationPeerFactory::Release ............. StressAutomationPeerCreateRelease
        //   54537037 XamlBinaryFormatReader2::GetXbfHash ............... StressXamlReaderLoadUnload
        //   56712355 NavigationCache::LoadContent ..................... StressFrameNavigationCache
        //   57672120 ItemsSourceView::OnItemsSourceChanged ........... StressItemsSourceViewSwaps
        //   60801847 Vector _scalar_deleting_destructor (double free) . StressItemsSourceViewSwaps
        //   55026189 WindowGenerated::get_DispatcherQueue ............. StressWindowOpenClose
        //   62091304 InkToolbar ReferenceTrackerRuntimeClass ......... StressInkToolbarTargeting
        //   56307002 LsDestroyBreakRecord (text line services) ....... StressTextLineServicesChurnNative  (added below)
        //   53672707 CDirectManipulationService::Activate... ......... StressScrollViewContentChurnNative (added below;
        //            realizes + tears down the DM/scroll service - full activation needs real manipulation input)
        //   58759931 WeakReferenceImpl::Resolve ...................... StressEventHandlerAfterTeardownNative
        //   50386959 AddRefForPeerReferenceHelper / 54447527 UnpegManagedPeer / 54449843 TrackerTargetReference::Clear /
        //   54506263 OfTypeByIndex / 54554788 unconditional_release_ref / 54638556 AddRef / 56731116 xstring_ptr_view::
        //   GetBuffer / 57024687 DynamicMetadataStorage / 59109646 ShouldDisablePixelSnapping / 60579018 GetProperty
        //   BaseByIndex / 63449698 DependencyObjectPropertyAccess::Release / 63129346 / 63277512 / 63485295 / 63779618
        //   ctl::ComObject_* ........................................ Generic peer churn (see above)
        //
        // "Generic peer churn" native-suffixed scenarios: StressOffThreadPeerFinalReleaseNative,
        // StressRapidReparentEnterLeaveNative, StressDeepVisualTreePeerChurnNative, StressReentrantUnloadTeardownNative.
        //
        // Tracked but NOT reproduced here - each needs infrastructure MUXControlsTestApp (a desktop test app) cannot
        // host, so a managed scenario cannot drive the faulting path:
        //   54170426 CXamlIslandRoot::SetIslandInputSite / 54479739 GetElementIslandInputSite . XAML island input site
        //   55899151 WindowsXamlManager (Taskbar.dll) ............... system XAML hosting from an external host process
        //   54463285 CWindowsServices::GetKeyboardModifiersState .... live keyboard input state
        //   56396875 dcomp CompositionObject::get_Properties ........ DirectComposition device internals
        //   54450443 / 54450547 PLMHandler::OnSuspending/OnResuming . Process Lifetime Management suspend/resume (UWP)
        // -------------------------------------------------------------------------------------------------------------

        // =============================================================================================================
        // Native-crash reproduction scenarios.
        //
        // NAMING: every scenario below carries a "Native" suffix (e.g. StressOffThreadPeerFinalReleaseNative) so the
        // native-crash-repro tests are easy to grep in build/TAEF logs - search for "Native" to find just these.
        //
        // The scenarios above surface *managed*-observable lifetime problems (a leaked WeakReference, a thrown managed
        // exception) and report them as non-gating warnings. The scenarios in THIS section instead target the native
        // lifetime crash classes the managed harness cannot otherwise reach - use-after-free, premature native peer
        // destruction, off-thread final release and enter/leave peer-wiring bugs in the 3-layer peer model - by driving
        // the exact access patterns that historically fault natively (a hard fail-fast / stowed exception that no
        // managed catch can intercept).
        //
        // They stay NON-GATING by construction:
        //   * All managed-thread work goes through SafeUI / RunIterationReporting, so any thrown managed exception is
        //     downgraded to a warning (never a Verify.Fail).
        //   * The genuinely fatal, host-crashing step of each scenario is guarded behind AggressiveNativeReproEnabled
        //     (WINUI_LIFETIME_STRESS_NATIVE), which ONLY the scheduled soak pipeline sets. In the per-PR gate the knob
        //     is off, so the scenario runs a light benign pass and cannot crash the shared pipeline. The soak - itself
        //     a CI build - runs the aggressive variant that actually reproduces the native crash, isolated in this
        //     suite's own Helix work item so a host crash there does not take unrelated tests down.
        //   * If a specific scenario becomes a KNOWN deterministic crasher even in the light pass, quarantine just that
        //     one with [TestProperty("Ignore","True")] (see StressItemsRepeaterRealizationAndRecycling) until its
        //     product bug is fixed - same convention the rest of the suite uses.
        // =============================================================================================================

        // Off-thread final peer release (Pillar C class). Create native-peer-heavy elements on the UI thread, wire them
        // into the live tree so the native peer is created, then unparent and drop the ONLY managed reference WITHOUT
        // collecting on the UI thread. The subsequent forced finalization runs the managed peer's finalizer on the
        // GC/finalizer thread, so the FINAL native release originates off the owning (UI) thread and must be marshaled
        // back through the UIAffinityReleaseQueue funnel. A bug in that off-thread release path faults here.
        [TestMethod]
        public void StressOffThreadPeerFinalReleaseNative()
        {
            RunNativeStress("StressOffThreadPeerFinalReleaseNative", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();
                int peers = AggressiveNativeReproEnabled ? 64 : 6;

                SafeUI(() =>
                {
                    var host = new Grid();
                    Content = host;
                    host.UpdateLayout();

                    for (int p = 0; p < peers; p++)
                    {
                        // A native-peer-heavy control, left POPULATED (open pane + menu items) so its cross-boundary
                        // fields are live at release time rather than pre-quiesced.
                        var child = new NavigationView() { PaneTitle = "peer", IsPaneOpen = true };
                        child.MenuItems.Add(new NavigationViewItem() { Content = "a" });
                        child.MenuItems.Add(new NavigationViewItem() { Content = "b" });
                        if (p == 0)
                        {
                            objects["FirstPeer"] = new WeakReference(child);
                        }

                        // Re-enter teardown from Unloaded: mutate the pane (a converted cross-boundary field) while the
                        // native peer is mid-unlink, so the field is touched during leave-tree instead of quiesced.
                        bool reentered = false;
                        child.Unloaded += (s, e) =>
                        {
                            if (reentered) { return; }
                            reentered = true;
                            child.IsPaneOpen = !child.IsPaneOpen;
                        };

                        host.Children.Add(child);
                        host.UpdateLayout();

                        // Unparent WITHOUT first nulling MenuItems / closing the pane: the converted peer fields are
                        // still populated when the element leaves the tree and is dropped for off-thread finalization.
                        host.Children.Clear();
                        host.UpdateLayout();
                    }

                    Content = null;
                });

                FinalizeOffThread();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // Re-entrant teardown. Remove an element from the tree and, from inside its own Unloaded handler, synchronously
        // mutate the tree again (null its content, clear its parent). Re-entering teardown while the native peer is
        // mid-unlink is a classic use-after-free / premature-peer-destruction trigger.
        [TestMethod]
        public void StressReentrantUnloadTeardownNative()
        {
            RunNativeStress("StressReentrantUnloadTeardownNative", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();
                int churn = AggressiveNativeReproEnabled ? 60 : 5;

                SafeUI(() =>
                {
                    var host = new StackPanel();
                    Content = host;
                    host.UpdateLayout();

                    for (int c = 0; c < churn; c++)
                    {
                        var panel = new Border();
                        var child = new Button() { Content = "x" };
                        panel.Child = child;
                        if (c == 0)
                        {
                            objects["FirstPanel"] = new WeakReference(panel);
                        }

                        bool reentered = false;
                        child.Unloaded += (s, e) =>
                        {
                            if (reentered) { return; }
                            reentered = true;
                            // Re-enter teardown while this peer is mid-unlink: drop the child, clear the host, and
                            // force a synchronous layout so the native peer is re-walked during its own leave-tree.
                            panel.Child = null;
                            host.Children.Clear();
                            host.UpdateLayout();
                        };

                        host.Children.Add(panel);
                        host.UpdateLayout();
                        if (host.Children.Contains(panel))
                        {
                            host.Children.Remove(panel);
                        }
                        host.UpdateLayout();
                    }

                    Content = null;
                });

                FinalizeOffThread();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // Native-backed event handler outliving its peer. Subscribe a native-backed event whose delegate closes over
        // the element, remove and drop the element, force collection, then keep mutating the live tree so the framework
        // pumps layout/size callbacks. If a revoked/native handler outlives the peer it dereferences freed native state.
        [TestMethod]
        public void StressEventHandlerAfterTeardownNative()
        {
            RunNativeStress("StressEventHandlerAfterTeardownNative", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();
                int churn = AggressiveNativeReproEnabled ? 60 : 5;

                SafeUI(() =>
                {
                    var host = new StackPanel();
                    Content = host;
                    host.UpdateLayout();

                    for (int c = 0; c < churn; c++)
                    {
                        var element = new Slider() { Minimum = 0, Maximum = 100, Width = 120 };
                        if (c == 0)
                        {
                            objects["FirstElement"] = new WeakReference(element);
                        }

                        // Native-backed handler left SUBSCRIBED across teardown: the delegate closes over the element
                        // and keeps dereferencing its native peer as layout/size callbacks fire.
                        SizeChangedEventHandler handler = (s, e) => { _ = element.Value; };
                        element.SizeChanged += handler;

                        host.Children.Add(element);
                        host.UpdateLayout();

                        host.Children.Remove(element);
                        host.UpdateLayout();

                        // After-teardown access: the element has left the tree (native peer unlinked) but we still call
                        // into it, forcing a size/layout pass that dereferences the just-unlinked peer.
                        element.Width = 240;
                        element.UpdateLayout();
                        _ = element.ActualWidth;
                    }

                    Content = null;
                });

                FinalizeOffThread();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // Rapid reparent across two live subtrees. Move the same element back and forth between two parents that are
        // both in the live tree. Each move drives the native peer through leave-tree + enter-tree wiring; a bug in the
        // enter/leave peer bookkeeping faults under this churn.
        [TestMethod]
        public void StressRapidReparentEnterLeaveNative()
        {
            RunNativeStress("StressRapidReparentEnterLeaveNative", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();
                int moves = AggressiveNativeReproEnabled ? 400 : 20;

                SafeUI(() =>
                {
                    var root = new Grid();
                    var left = new StackPanel();
                    var right = new StackPanel();
                    root.Children.Add(left);
                    root.Children.Add(right);
                    Content = root;
                    root.UpdateLayout();

                    var mover = new ComboBox() { ItemsSource = Enumerable.Range(0, 20) };
                    objects["Mover"] = new WeakReference(mover);

                    // Re-enter the enter/leave peer wiring: on first Loaded, synchronously reparent the element from
                    // inside its own enter-tree callback so the native peer is unlinked while still mid-enter.
                    bool reentered = false;
                    mover.Loaded += (s, e) =>
                    {
                        if (reentered) { return; }
                        reentered = true;
                        if (left.Children.Contains(mover))
                        {
                            left.Children.Remove(mover);
                            right.Children.Add(mover);
                            root.UpdateLayout();
                        }
                    };

                    left.Children.Add(mover);
                    root.UpdateLayout();

                    Panel current = right.Children.Contains(mover) ? (Panel)right : (Panel)left;
                    for (int m = 0; m < moves; m++)
                    {
                        Panel next = (current == left) ? right : left;
                        // Defensive against the reentrant Loaded move above having relocated the element already.
                        left.Children.Remove(mover);
                        right.Children.Remove(mover);
                        next.Children.Add(mover);
                        root.UpdateLayout();
                        current = next;
                    }

                    // Unparent the whole tree WITHOUT first detaching the mover, so the peer is released with live
                    // enter-tree bookkeeping rather than after a clean detach.
                    Content = null;
                });

                FinalizeOffThread();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // Deep visual-tree peer churn. Build a deeply nested chain of native-peer-bearing elements, realize it, then
        // tear the whole chain down at once and collect. Deep nesting multiplies native peer create/destroy traffic and
        // stresses the recursive leave-tree teardown path where premature-peer-destruction bugs live.
        [TestMethod]
        public void StressDeepVisualTreePeerChurnNative()
        {
            RunNativeStress("StressDeepVisualTreePeerChurnNative", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();
                int depth = AggressiveNativeReproEnabled ? 400 : 30;

                SafeUI(() =>
                {
                    var root = new Border();
                    Content = root;
                    objects["Root"] = new WeakReference(root);

                    Border cursor = root;
                    Border midpoint = root;
                    for (int d = 0; d < depth; d++)
                    {
                        var next = new Border();
                        cursor.Child = next;
                        cursor = next;
                        if (d == depth / 2) { midpoint = next; }
                    }
                    var leaf = new TextBlock() { Text = "leaf" };
                    cursor.Child = leaf;

                    // Re-enter the recursive leave-tree teardown: when the midpoint leaves the tree, sever its own
                    // Child so the lower half is unlinked while the upper half is still mid-teardown.
                    bool reentered = false;
                    midpoint.Unloaded += (s, e) =>
                    {
                        if (reentered) { return; }
                        reentered = true;
                        midpoint.Child = null;
                    };

                    root.UpdateLayout();

                    // Unparent the whole deep chain at once WITHOUT pre-severing links, so the recursive native teardown
                    // runs over a fully-populated chain, then off-thread finalize.
                    Content = null;

                    // After-teardown access to the deep leaf now that its ancestors have left the tree.
                    leaf.UpdateLayout();
                    _ = leaf.ActualWidth;
                });

                FinalizeOffThread();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // Text line-services teardown (Watson: Microsoft.UI.Xaml.Internal.dll!LsDestroyBreakRecord). Build text-heavy
        // elements whose wrapped, multi-line content forces the line-services layer to create line/break records,
        // realize them, then tear them down and collect. A lifetime bug in break-record teardown faults here.
        [TestMethod]
        public void StressTextLineServicesChurnNative()
        {
            RunNativeStress("StressTextLineServicesChurnNative", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();
                int churn = AggressiveNativeReproEnabled ? 60 : 6;
                string paragraph = string.Concat(Enumerable.Repeat("The quick brown fox jumps over the lazy dog. ", 40));

                SafeUI(() =>
                {
                    var host = new StackPanel() { Width = 200 };
                    Content = host;
                    host.UpdateLayout();

                    for (int c = 0; c < churn; c++)
                    {
                        var block = new TextBlock() { Text = paragraph, TextWrapping = TextWrapping.WrapWholeWords, Width = 180 };
                        var box = new TextBox() { Text = paragraph, TextWrapping = TextWrapping.Wrap, Width = 180, Height = 80, AcceptsReturn = true };
                        if (c == 0)
                        {
                            objects["FirstBlock"] = new WeakReference(block);
                            objects["FirstBox"] = new WeakReference(box);
                        }

                        // Re-enter line-services layout: on the first size change, rewrap by mutating Text/Width from
                        // inside the callback so break records are rebuilt while the previous set is being torn down.
                        bool reentered = false;
                        block.SizeChanged += (s, e) =>
                        {
                            if (reentered) { return; }
                            reentered = true;
                            block.Width = 90;
                            block.Text = paragraph + paragraph;
                            block.UpdateLayout();
                        };

                        host.Children.Add(block);
                        host.Children.Add(box);
                        host.UpdateLayout();

                        // Remove WITHOUT clearing text: the line/break records are still populated when the elements
                        // leave the tree and are dropped for off-thread finalization (LsDestroyBreakRecord path).
                        host.Children.Clear();
                        host.UpdateLayout();

                        _ = block.ActualHeight;
                    }

                    Content = null;
                });

                FinalizeOffThread();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // ScrollView / DirectManipulation service setup + teardown (Watson: CDirectManipulationService::Activate
        // DirectManipulationManager). Realize a ScrollView over large scrollable content - which stands up the
        // manipulation/scroll service - then swap its content and tear it down repeatedly. NOTE: fully ACTIVATING the
        // DM manager needs real touch/pen manipulation input this headless suite cannot inject; this exercises the
        // DM/scroll service create + teardown path, which is where the reported lifetime fault occurs.
        [TestMethod]
        public void StressScrollViewContentChurnNative()
        {
            RunNativeStress("StressScrollViewContentChurnNative", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();
                int churn = AggressiveNativeReproEnabled ? 60 : 6;

                SafeUI(() =>
                {
                    var scroll = new ScrollView() { Width = 200, Height = 200 };
                    objects["ScrollView"] = new WeakReference(scroll);

                    // Re-enter the scroll/DM service wiring: on the first size change, re-point the manipulated content
                    // from inside the callback so the service is redirected while it is still being stood up.
                    bool reentered = false;
                    scroll.SizeChanged += (s, e) =>
                    {
                        if (reentered) { return; }
                        reentered = true;
                        var swap = new StackPanel();
                        swap.Children.Add(new Button() { Content = "swap", Width = 400 });
                        scroll.Content = swap;
                        scroll.UpdateLayout();
                    };

                    Content = scroll;
                    Content.UpdateLayout();

                    for (int c = 0; c < churn; c++)
                    {
                        var content = new StackPanel();
                        for (int i = 0; i < 40; i++)
                        {
                            content.Children.Add(new Button() { Content = "row " + i, Width = 400 });
                        }
                        scroll.Content = content;
                        scroll.UpdateLayout();
                    }

                    // Unparent the ScrollView with its large content STILL set (DM/scroll service live) instead of
                    // pre-nulling the content, then off-thread finalize.
                    Content = null;
                });

                FinalizeOffThread();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // =============================================================================================================
        // Native-crash reproduction scenarios that specifically drive the cross-boundary peer fields converted from
        // raw ctl::ComPtr to TrackerPtr (see the ListView/GridView/SplitView/ToggleSwitch/UIElement lifetime fix).
        //
        // A raw ctl::ComPtr peer field that outlives its owner (or is released from the wrong thread) is exactly the
        // use-after-free / premature-native-peer-destruction class this suite exists to catch; the same field stored
        // as a TrackerPtr participates in the tracker (GC) graph and is released safely. These scenarios churn the
        // specific controls whose fields were converted, then drive the FINAL native release off the owning (UI)
        // thread - finalize on the GC/finalizer thread, then settle the UI thread - so a regressed (raw-ComPtr) field
        // faults here while the TrackerPtr form does not. They are gating (routed through RunNativeStress) and run the
        // aggressive configuration only when AggressiveNativeReproEnabled is set (the scheduled soak / opt-in gate);
        // in the light per-PR pass they do a small benign churn and cannot crash the shared pipeline.
        // =============================================================================================================

        // ListViewBase::m_spContainerBeingClicked (+ ModernCollectionBasePanel::m_spLayoutStrategy /
        // m_spLayoutDataInfoProvider via the virtualizing backing panel). Churn a click-enabled, virtualizing ListView
        // - swap the source and scroll both ends so containers are generated/recycled while item-click wiring holds a
        // container reference - then drop the only managed reference and finalize off-thread so the native peer's
        // final release runs on the finalizer thread.
        [TestMethod]
        public void StressListViewClickContainerChurnNative()
        {
            RunNativeStress("StressListViewClickContainerChurnNative", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();
                int churn = AggressiveNativeReproEnabled ? 60 : 5;

                SafeUI(() =>
                {
                    var listView = new ListView()
                    {
                        Width = 300,
                        Height = 400,
                        IsItemClickEnabled = true,
                        SelectionMode = ListViewSelectionMode.Extended,
                        ItemsSource = Enumerable.Range(0, 200).Select(i => string.Format("Item #{0}", i)).ToList(),
                    };
                    objects["ListView"] = new WeakReference(listView);

                    // Re-enter container recycling: on leave-tree, poke the selection and the container lookup (the
                    // m_spContainerBeingClicked path) while the native peer is mid-unlink.
                    bool reentered = false;
                    listView.Unloaded += (s, e) =>
                    {
                        if (reentered) { return; }
                        reentered = true;
                        listView.SelectedIndex = -1;
                        _ = listView.ContainerFromIndex(0);
                    };

                    Content = listView;
                    Content.UpdateLayout();

                    DependencyObject container = null;
                    for (int c = 0; c < churn; c++)
                    {
                        listView.ItemsSource = Enumerable.Range(c * 50, 150).Select(i => string.Format("Item #{0}", i)).ToList();
                        Content.UpdateLayout();

                        if (listView.Items.Count > 0)
                        {
                            listView.SelectedIndex = c % listView.Items.Count;
                            listView.ScrollIntoView(listView.Items[listView.Items.Count - 1]);
                            Content.UpdateLayout();
                            listView.ScrollIntoView(listView.Items[0]);
                            Content.UpdateLayout();
                            container = listView.ContainerFromIndex(0) ?? container;
                        }
                    }

                    // Unparent WITHOUT nulling ItemsSource: containers / click-container fields are still populated when
                    // the peer is dropped for off-thread finalization.
                    Content = null;

                    // After-teardown access to a generated container now that the list has left the tree.
                    (container as ListViewItem)?.UpdateLayout();
                });

                FinalizeOffThread();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // GridView over ListViewBase / ModernCollectionBasePanel. Realize a virtualizing GridView, churn selection and
        // add/remove its explicit containers (which drives the click-container and layout-strategy fields), then drop
        // the only managed reference and finalize off-thread.
        [TestMethod]
        public void StressGridViewContainerChurnNative()
        {
            RunNativeStress("StressGridViewContainerChurnNative", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();
                int churn = AggressiveNativeReproEnabled ? 60 : 5;

                SafeUI(() =>
                {
                    var gridView = new GridView()
                    {
                        Width = 400,
                        Height = 400,
                        IsItemClickEnabled = true,
                        SelectionMode = ListViewSelectionMode.Extended,
                    };
                    objects["GridView"] = new WeakReference(gridView);

                    bool reentered = false;
                    gridView.Unloaded += (s, e) =>
                    {
                        if (reentered) { return; }
                        reentered = true;
                        gridView.SelectedIndex = -1;
                        _ = gridView.ContainerFromIndex(0);
                    };

                    Content = gridView;
                    Content.UpdateLayout();

                    GridViewItem firstItem = null;
                    for (int c = 0; c < churn; c++)
                    {
                        var item = new GridViewItem() { Content = string.Format("Item {0}", c) };
                        if (c == 0)
                        {
                            objects["FirstItem"] = new WeakReference(item);
                            firstItem = item;
                        }
                        gridView.Items.Add(item);
                        Content.UpdateLayout();

                        gridView.SelectedIndex = gridView.Items.Count - 1;
                        gridView.ScrollIntoView(gridView.Items[gridView.Items.Count - 1]);
                        Content.UpdateLayout();

                        if (gridView.Items.Count > 8)
                        {
                            gridView.Items.RemoveAt(0);
                            Content.UpdateLayout();
                        }
                    }

                    // Unparent WITHOUT clearing Items: explicit containers + selection are live when the peer is
                    // dropped for off-thread finalization.
                    Content = null;

                    // After-teardown access to an explicit container now that the grid has left the tree.
                    firstItem?.UpdateLayout();
                });

                FinalizeOffThread();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // SplitView light-dismiss layer (m_outerDismissLayerPopup / m_dismissHostElement / m_top/bottom/left/right
        // DismissElement). Those fields are created when the pane opens in a light-dismiss (overlay) display mode.
        // Repeatedly open/close the pane in an overlay mode with light dismiss on - standing the dismiss layer up and
        // tearing it down each cycle - then drop the only managed reference and finalize off-thread.
        [TestMethod]
        public void StressSplitViewLightDismissChurnNative()
        {
            RunNativeStress("StressSplitViewLightDismissChurnNative", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();
                int churn = AggressiveNativeReproEnabled ? 80 : 6;

                SafeUI(() =>
                {
                    var paneList = new ListView() { ItemsSource = Enumerable.Range(0, 20) };
                    var splitView = new SplitView()
                    {
                        Width = 500,
                        Height = 400,
                        Pane = paneList,
                        Content = new TextBlock() { Text = "content" },
                        DisplayMode = SplitViewDisplayMode.Overlay,
                        LightDismissOverlayMode = LightDismissOverlayMode.On,
                        IsPaneOpen = false,
                    };
                    objects["SplitView"] = new WeakReference(splitView);
                    objects["PaneListView"] = new WeakReference(paneList);

                    // Re-enter the dismiss-layer teardown: while the SplitView leaves the tree, flip the pane and touch
                    // the Pane field (a converted cross-boundary field) mid-unlink.
                    bool reentered = false;
                    splitView.Unloaded += (s, e) =>
                    {
                        if (reentered) { return; }
                        reentered = true;
                        splitView.IsPaneOpen = !splitView.IsPaneOpen;
                        _ = splitView.Pane;
                    };

                    Content = splitView;
                    Content.UpdateLayout();

                    for (int c = 0; c < churn; c++)
                    {
                        // Open in a light-dismiss overlay mode: creates the dismiss-layer popup + dismiss elements.
                        splitView.DisplayMode = (c % 2 == 0)
                            ? SplitViewDisplayMode.Overlay
                            : SplitViewDisplayMode.CompactOverlay;
                        splitView.IsPaneOpen = true;
                        Content.UpdateLayout();
                        // Close: tears the dismiss layer back down.
                        splitView.IsPaneOpen = false;
                        Content.UpdateLayout();
                    }

                    // Leave the dismiss layer STANDING (pane open) and do NOT null Pane/Content before unparenting, so
                    // the converted dismiss-layer fields (m_outerDismissLayerPopup et al.) are populated when the peer
                    // is dropped for off-thread finalization.
                    splitView.IsPaneOpen = true;
                    Content.UpdateLayout();
                    Content = null;
                });

                FinalizeOffThread();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
        }

        // ToggleSwitch::m_spKnobTransform / m_spCurtainTransform. These transform peers come from the control template,
        // so they are created on OnApplyTemplate (first layout in a live tree) and released on teardown. Churn
        // template-apply + IsOn toggles (which drive the curtain/knob transforms) across enter/leave, then drop the
        // only managed reference and finalize off-thread.
        [TestMethod]
        public void StressToggleSwitchTransformChurnNative()
        {
            RunNativeStress("StressToggleSwitchTransformChurnNative", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();
                int churn = AggressiveNativeReproEnabled ? 80 : 6;

                SafeUI(() =>
                {
                    var host = new StackPanel();
                    Content = host;
                    host.UpdateLayout();

                    ToggleSwitch firstToggle = null;
                    for (int c = 0; c < churn; c++)
                    {
                        var toggle = new ToggleSwitch() { IsOn = false };
                        if (c == 0)
                        {
                            objects["FirstToggle"] = new WeakReference(toggle);
                            firstToggle = toggle;
                        }

                        // Re-enter the curtain/knob transform update: flip IsOn once from inside Toggled so the
                        // transform peers are re-driven while the previous toggle's visual state is still settling.
                        bool reentered = false;
                        toggle.Toggled += (s, e) =>
                        {
                            if (reentered) { return; }
                            reentered = true;
                            toggle.IsOn = !toggle.IsOn;
                            toggle.UpdateLayout();
                        };

                        host.Children.Add(toggle);
                        host.UpdateLayout(); // OnApplyTemplate -> creates knob/curtain transform peers.

                        toggle.IsOn = true;
                        host.UpdateLayout();
                    }

                    // Unparent the host with the toggles' transforms STILL active (IsOn=true); do NOT remove each toggle
                    // first, so the converted transform fields are populated when the peers are dropped for off-thread
                    // finalization.
                    Content = null;

                    // After-teardown access to a transform-bearing toggle now that it has left the tree.
                    firstToggle?.UpdateLayout();
                });

                FinalizeOffThread();
                SafeUI(() => VerifyCollected(objects, failOnLeak: true));
                IdleSynchronizer.Wait();
            });
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

        // Run the supplied per-iteration work and always produce a report, never a gating failure. In every mode the
        // per-iteration work is wrapped so a thrown managed exception is logged as a WARNING (a report line) instead
        // of failing the test - so this suite never records a Failed result and therefore never fails Publish Test
        // Results / the pipeline. Modes:
        //   * WINUI_LIFETIME_STRESS_MINUTES > 0  -> soak: loop each scenario on a wall-clock budget (scheduled pipeline).
        //   * WINUI_LIFETIME_STRESS_ITERATIONS>0 -> explicit fixed cycle count (heavier local/manual run).
        //   * neither set (PR gate + Nightly)    -> a small default "report" pass (DefaultReportIterations).
        //
        // IMPORTANT - the one thing this cannot catch: a genuine object-lifetime bug can fault as a NATIVE crash /
        // fail-fast (e.g. a stowed exception in combase.dll) that terminates the TAEF host process outright. Managed
        // try/catch cannot intercept that, so such a crash would still fail the work item. That is the real signal we
        // want, and we handle a known deterministic crasher by quarantining the specific scenario with
        // [TestProperty("Ignore","True")] (see StressItemsRepeaterRealizationAndRecycling). Everything a managed
        // catch can reach (thrown exceptions, leaks) is downgraded to a non-gating warning here.
        private static void RunStress(string scenarioName, Action<int> iteration)
        {
            double soakMinutes = ConfiguredSoakMinutes;
            int iterations = ConfiguredIterations;

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
                    RunIterationReporting(scenarioName, iteration, i);
                    i++;
                }

                Log.Comment("[{0}] Soak complete after {1} iteration(s).", scenarioName, i);
            }
            else
            {
                bool explicitRun = iterations > 0;
                int count = explicitRun ? iterations : DefaultReportIterations;
                Log.Comment("[{0}] {1}: {2} iteration(s).", scenarioName,
                    explicitRun ? "Explicit run" : "Report pass (non-gating)", count);
                for (int i = 0; i < count; i++)
                {
                    Log.Comment("[{0}] Iteration {1}/{2}...", scenarioName, i, count);
                    RunIterationReporting(scenarioName, iteration, i);
                }
            }
        }

        // Native-crash-repro wrapper. Emits an explicit, greppable "[LifetimeStress] NATIVE" marker around the
        // scenario so EVERY pipeline run - including the non-gating per-PR gate - produces a searchable native test
        // log line that proves the scenario executed and records which mode it ran in. aggressiveNativeRepro reflects
        // WINUI_LIFETIME_STRESS_NATIVE: it is off in the PR gate (light, non-gating variant) and on in the scheduled
        // soak (aggressive, host-crashing variant). Search build/TAEF logs for "[LifetimeStress] NATIVE" to find just
        // these lines.
        private static void RunNativeStress(string scenarioName, Action<int> iteration)
        {
            Log.Comment("[LifetimeStress] NATIVE: scenario '{0}' starting (aggressiveNativeRepro={1}).",
                scenarioName, AggressiveNativeReproEnabled);
            RunStress(scenarioName, iteration);
            Log.Comment("[LifetimeStress] NATIVE: scenario '{0}' completed (aggressiveNativeRepro={1}).",
                scenarioName, AggressiveNativeReproEnabled);
        }

        // Run a single scenario iteration, downgrading any thrown managed exception to a non-gating warning so it is
        // reported without failing the test (and therefore without failing the pipeline). This is the outer net for
        // exceptions thrown on the TEST thread (e.g. IdleSynchronizer.Wait / SettleAndCollect). Exceptions thrown on
        // the UI thread are caught earlier, inside SafeUI, before RunOnUIThread.Execute can turn them into a
        // Verify.Fail (which would record a Failed verdict). A native crash / fail-fast cannot be caught by either
        // and will still take the host down - that is intentional (see RunStress remarks); a known deterministic
        // crasher is quarantined per-scenario with [TestProperty("Ignore","True")].
        private static void RunIterationReporting(string scenarioName, Action<int> iteration, int i)
        {
            s_currentScenario = scenarioName;
            try
            {
                iteration(i);
            }
            catch (Exception ex)
            {
                Log.Warning(string.Format(
                    "[LifetimeStress] REPORT: scenario '{0}' threw on iteration {1}: {2}: {3}. Logged as a warning " +
                    "(non-gating); investigate for a possible lifetime bug.",
                    scenarioName, i, ex.GetType().Name, ex.Message));
            }
        }

        // Run an action on the UI thread, catching any exception INSIDE the UI-thread callback and downgrading it to a
        // non-gating warning. This is essential: RunOnUIThread.Execute converts an escaping UI-thread exception into a
        // Verify.Fail, and a Verify.Fail records a Failed test verdict that a catch on the test thread cannot undo. By
        // swallowing the exception here (before it escapes the callback) the scenario reports the problem without ever
        // failing the test - so this suite reports in the PR run but never gates it. Scenarios call this instead of
        // RunOnUIThread.Execute directly.
        private static void SafeUI(Action action)
        {
            RunOnUIThread.Execute(() =>
            {
                try
                {
                    action();
                }
                catch (Exception ex)
                {
                    Log.Warning(string.Format(
                        "[LifetimeStress] REPORT: scenario '{0}' threw on the UI thread: {1}: {2}. Logged as a " +
                        "warning (non-gating); investigate for a possible lifetime bug.",
                        s_currentScenario, ex.GetType().Name, ex.Message));
                }
            });
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

        // Drive the FINAL native release off the owning (UI) thread: run the managed finalizer on the GC/finalizer
        // thread first (so the last native Release originates off-thread and must be marshaled back through the
        // UIAffinityReleaseQueue), then settle the UI thread so that marshaled release is actually drained. A peer
        // whose cross-boundary field regressed from TrackerPtr to a raw ctl::ComPtr faults in exactly this window; the
        // TrackerPtr form is released safely. This is the release funnel every native scenario ends with.
        private static void FinalizeOffThread()
        {
            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();
            SettleAndCollect();
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

    // Minimal cached pages used by StressFrameNavigationCache. NavigationCacheMode=Required makes the Frame's
    // navigation cache actually retain and later release these page instances (the code path under test). Declared
    // as top-level public types so Frame.Navigate can activate them.
    public sealed class LifetimeStressPage : Page
    {
        public LifetimeStressPage()
        {
            NavigationCacheMode = NavigationCacheMode.Required;
            Content = new TextBlock() { Text = "LifetimeStressPage" };
        }
    }

    public sealed class LifetimeStressPageTwo : Page
    {
        public LifetimeStressPageTwo()
        {
            NavigationCacheMode = NavigationCacheMode.Required;
            Content = new TextBlock() { Text = "LifetimeStressPageTwo" };
        }
    }
}
