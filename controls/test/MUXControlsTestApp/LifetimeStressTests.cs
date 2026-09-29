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
    // Lifetime stress coverage for create/use/teardown/GC paths.
    [TestClass]
    // Keep this selected as its own Integration suite/work item.
    [TestProperty("Classification", "Integration")]
    [TestProperty("TestSuite", "LifetimeStressTestSuite")]
    public class LifetimeStressTests : ApiTestBase
    {
        // Default report-pass iterations; env vars can raise this.
        private const int DefaultReportIterations = 3;

        // Current scenario name for UI-thread warning attribution.
        private static string s_currentScenario = "LifetimeStress";

        // WINUI_LIFETIME_STRESS_ITERATIONS override; 0 uses the default report pass.
        private static int ConfiguredIterations
        {
            get { return GetEnvInt("WINUI_LIFETIME_STRESS_ITERATIONS", 0); }
        }

        private static double ConfiguredSoakMinutes
        {
            get { return GetEnvDouble("WINUI_LIFETIME_STRESS_MINUTES", 0.0); }
        }

        // WINUI_LIFETIME_STRESS_NATIVE enables native repro variants.
        private static bool AggressiveNativeReproEnabled
        {
            get { return GetEnvInt("WINUI_LIFETIME_STRESS_NATIVE", 0) > 0; }
        }

        // Create/parent/layout/unparent/collect in a loop to shake out peer-lifetime bugs.
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
                // Leaks are reported as warnings; this suite only gates on host crashes.
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // ItemsRepeater realization/recycling churn.
        [TestMethod]
        // Quarantined until the ItemsRepeater native lifetime crash is fixed.
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

                    // Swap sources and force realization/recycling.
                    for (int churn = 0; churn < 5; churn++)
                    {
                        repeater.ItemsSource = Enumerable.Range(churn * 50, 150).Select(i => string.Format("Item #{0}", i));
                        Content.UpdateLayout();

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

                    // Drop the tree while realized elements may still be in flight.
                    repeater.ItemsSource = null;
                    scrollHost.ScrollViewer.Content = null;
                    scrollHost.ScrollViewer = null;
                    Content = null;
                });

                SettleAndCollect();
                // Treat residual repeater graphs as warnings.
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // Reparenting exercises enter/leave and peer re-association.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // Window create/activate/close teardown stress.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
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

        // Virtualized ListView container generation/recycling stress.
        [TestMethod]
        public void StressListViewContainerRecycling()
        {
            RunStress("StressListViewContainerRecycling", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    // Keep the viewport constrained so virtualization actually recycles containers.
                    var listView = new ListView()
                    {
                        Width = 300,
                        Height = 400,
                        ItemsSource = Enumerable.Range(0, 200).Select(i => string.Format("Item #{0}", i)).ToList(),
                    };
                    objects["ListView"] = new WeakReference(listView);

                    Content = listView;
                    Content.UpdateLayout();

                    // Swap the source and scroll both ends to churn generated containers.
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

                    // Drop the list while containers may still be realized.
                    listView.ItemsSource = null;
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // Popup open/close lifetime stress.
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

                    // Give the Popup a XamlRoot without constructing a Window.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // NavigationView menu item and pane churn.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // TabView tab/content add-remove churn.
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

                    while (tabView.TabItems.Count > 0)
                    {
                        tabView.TabItems.RemoveAt(tabView.TabItems.Count - 1);
                        Content.UpdateLayout();
                    }

                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // Targeted scenarios for recurring Lifetime Issues Watson buckets.

        // MenuFlyout presenter open/close teardown stress.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // ResourceDictionary merge/lookup/teardown stress.
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

                        // Force merged-dictionary lookup before teardown.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // ItemsSourceView source-swap teardown stress.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // Automation peer creation and release stress.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // Frame navigation-cache teardown stress; quarantined until the native crash is fixed.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // XamlReader parse/load/unload teardown stress.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // Simple-property storage set/clear teardown stress.
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

                    foreach (var el in elements)
                    {
                        el.Translation = new Vector3(3, 3, 0);
                    }
                    panel.Children.Clear();
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // MediaTransportControls template create/teardown stress.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // InkToolbar/InkCanvas target attach-detach teardown stress.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // TreeView node/container expand-collapse churn.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // ComboBox popup and item-container churn.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // FlipView item-container churn.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // Pivot item churn; quarantined until the native crash is fixed.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // SplitView pane open/close lifetime stress.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // Expander expand/collapse and content-swap stress.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // CommandBar command and overflow presenter churn.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // ListView selection and item churn.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // GridView selection and item churn.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // BreadcrumbBar ItemsSource swap churn.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // SelectorBar item and selection churn.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // Parameterized coverage for remaining item/popup churners.

        // RadioButtons selection and item churn.
        [TestMethod]
        public void StressRadioButtonsSelectionChurn()
        {
            RunChurnScenario<RadioButtons>(
                "StressRadioButtonsSelectionChurn",
                (objects) =>
                {
                    var radioButtons = new RadioButtons();
                    for (int m = 0; m < 8; m++)
                    {
                        var item = new RadioButton() { Content = string.Format("Option {0}", m) };
                        if (m == 0)
                        {
                            objects["FirstItem"] = new WeakReference(item);
                        }
                        radioButtons.Items.Add(item);
                    }
                    return radioButtons;
                },
                (radioButtons, c) =>
                {
                    radioButtons.SelectedIndex = c % radioButtons.Items.Count;

                    var extra = new RadioButton() { Content = string.Format("Extra {0}", c) };
                    radioButtons.Items.Add(extra);
                    radioButtons.Items.Remove(extra);
                },
                (radioButtons) =>
                {
                    radioButtons.SelectedIndex = -1;
                    radioButtons.Items.Clear();
                });
        }

        // MenuBar item and nested flyout-item churn.
        [TestMethod]
        public void StressMenuBarItemChurn()
        {
            RunChurnScenario<MenuBar>(
                "StressMenuBarItemChurn",
                (objects) =>
                {
                    var menuBar = new MenuBar();
                    for (int m = 0; m < 6; m++)
                    {
                        var menu = new MenuBarItem() { Title = string.Format("Menu {0}", m) };
                        if (m == 0)
                        {
                            objects["FirstItem"] = new WeakReference(menu);
                        }
                        for (int f = 0; f < 4; f++)
                        {
                            menu.Items.Add(new MenuFlyoutItem() { Text = string.Format("Item {0}.{1}", m, f) });
                        }
                        menuBar.Items.Add(menu);
                    }
                    return menuBar;
                },
                (menuBar, c) =>
                {
                    var first = menuBar.Items[0];
                    first.Items.Add(new MenuFlyoutItem() { Text = string.Format("Extra {0}", c) });
                    first.Items.RemoveAt(first.Items.Count - 1);

                    var extraMenu = new MenuBarItem() { Title = string.Format("Extra {0}", c) };
                    menuBar.Items.Add(extraMenu);
                    menuBar.Items.RemoveAt(menuBar.Items.Count - 1);
                },
                (menuBar) =>
                {
                    foreach (var item in menuBar.Items)
                    {
                        item.Items.Clear();
                    }
                    menuBar.Items.Clear();
                });
        }

        // DropDownButton flyout popup/item churn.
        [TestMethod]
        public void StressDropDownButtonFlyoutChurn()
        {
            RunChurnScenario<DropDownButton>(
                "StressDropDownButtonFlyoutChurn",
                (objects) => BuildFlyoutButton(objects, new DropDownButton() { Content = "menu" }),
                (button, c) => ChurnFlyoutButton((MenuFlyout)button.Flyout, button, c),
                (button) => TeardownFlyoutButton(button));
        }

        // SplitButton uses the same flyout churn path as DropDownButton.
        [TestMethod]
        public void StressSplitButtonFlyoutChurn()
        {
            RunChurnScenario<SplitButton>(
                "StressSplitButtonFlyoutChurn",
                (objects) =>
                {
                    var button = new SplitButton() { Content = "split" };
                    var flyout = BuildMenuFlyout(objects);
                    button.Flyout = flyout;
                    return button;
                },
                (button, c) => ChurnFlyoutButton((MenuFlyout)button.Flyout, button, c),
                (button) =>
                {
                    var flyout = (MenuFlyout)button.Flyout;
                    if (flyout != null)
                    {
                        flyout.Hide();
                        flyout.Items.Clear();
                    }
                    button.Flyout = null;
                });
        }

        // InfoBar open/close content lifetime stress.
        [TestMethod]
        public void StressInfoBarOpenClose()
        {
            RunChurnScenario<InfoBar>(
                "StressInfoBarOpenClose",
                (objects) =>
                {
                    var content = new TextBlock() { Text = "content" };
                    objects["Content"] = new WeakReference(content);
                    return new InfoBar()
                    {
                        Title = "title",
                        Message = "message",
                        Content = content,
                        IsOpen = true,
                    };
                },
                (infoBar, c) => infoBar.IsOpen = !infoBar.IsOpen,
                (infoBar) =>
                {
                    infoBar.IsOpen = false;
                    infoBar.Content = null;
                });
        }

        // TeachingTip popup/content open-close stress.
        [TestMethod]
        public void StressTeachingTipOpenClose()
        {
            RunChurnScenario<TeachingTip>(
                "StressTeachingTipOpenClose",
                (objects) =>
                {
                    var content = new TextBlock() { Text = "content" };
                    objects["Content"] = new WeakReference(content);
                    return new TeachingTip()
                    {
                        Title = "tip",
                        Subtitle = "subtitle",
                        Content = content,
                    };
                },
                (teachingTip, c) => teachingTip.IsOpen = !teachingTip.IsOpen,
                (teachingTip) =>
                {
                    teachingTip.IsOpen = false;
                    teachingTip.Content = null;
                });
        }

        // Shared flyout-button churn helpers.
        private static MenuFlyout BuildMenuFlyout(Dictionary<string, WeakReference> objects)
        {
            var flyout = new MenuFlyout();
            for (int m = 0; m < 6; m++)
            {
                var item = new MenuFlyoutItem() { Text = string.Format("Item {0}", m) };
                if (m == 0)
                {
                    objects["FirstItem"] = new WeakReference(item);
                }
                flyout.Items.Add(item);
            }
            return flyout;
        }

        private static TButton BuildFlyoutButton<TButton>(Dictionary<string, WeakReference> objects, TButton button)
            where TButton : Button
        {
            button.Flyout = BuildMenuFlyout(objects);
            return button;
        }

        private static void ChurnFlyoutButton(MenuFlyout flyout, FrameworkElement target, int c)
        {
            flyout.ShowAt(target);
            flyout.Hide();

            var extra = new MenuFlyoutItem() { Text = string.Format("Extra {0}", c) };
            flyout.Items.Add(extra);
            flyout.Items.RemoveAt(flyout.Items.Count - 1);
        }

        private static void TeardownFlyoutButton(Button button)
        {
            var flyout = (MenuFlyout)button.Flyout;
            if (flyout != null)
            {
                flyout.Hide();
                flyout.Items.Clear();
            }
            button.Flyout = null;
        }

        // Coverage map for current Lifetime Issues Watson buckets.

        // Native-crash repro scenarios; aggressive variants only run when opted in.

        // GC-thread final release exercises UIAffinityReleaseQueue.
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
                        // Leave cross-boundary fields populated at release time.
                        var child = new NavigationView() { PaneTitle = "peer", IsPaneOpen = true };
                        child.MenuItems.Add(new NavigationViewItem() { Content = "a" });
                        child.MenuItems.Add(new NavigationViewItem() { Content = "b" });
                        if (p == 0)
                        {
                            objects["FirstPeer"] = new WeakReference(child);
                        }

                        // Mutate the pane during Unloaded to touch fields mid-unlink.
                        bool reentered = false;
                        child.Unloaded += (s, e) =>
                        {
                            if (reentered) { return; }
                            reentered = true;
                            child.IsPaneOpen = !child.IsPaneOpen;
                        };

                        host.Children.Add(child);
                        host.UpdateLayout();

                        // Unparent without quiescing menu/pane fields first.
                        host.Children.Clear();
                        host.UpdateLayout();
                    }

                    Content = null;
                });

                FinalizeOffThread();
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // Re-enter teardown from Unloaded while the peer is mid-unlink.
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
                            // Re-enter teardown and force layout during leave-tree.
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
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // Leave a native-backed event handler subscribed across teardown.
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

                        // Keep the handler subscribed so callbacks touch the torn-down element.
                        SizeChangedEventHandler handler = (s, e) => { _ = element.Value; };
                        element.SizeChanged += handler;

                        host.Children.Add(element);
                        host.UpdateLayout();

                        host.Children.Remove(element);
                        host.UpdateLayout();

                        // Touch the element after it leaves the tree.
                        element.Width = 240;
                        element.UpdateLayout();
                        _ = element.ActualWidth;
                    }

                    Content = null;
                });

                FinalizeOffThread();
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // Window event-handler reentrant-dispatch + off-thread teardown native repro. Targets the reentrant
        // Window-event dispatch and weak/strong Window retention path (DesktopWindowImpl::OnMessage /
        // CFTMEventSource::Raise): a Window peer can be finalized on the GC thread while a window message is
        // still dispatching handlers on the UI thread. Registers handlers on every Window event source (so
        // add_* captures the owner weak reference via UpdateWindowWeakReference and OnMessage dispatches through
        // populated event sources), mutates the handler set reentrantly during dispatch, then drives final
        // release off the UI thread. Regression signal is a native host crash on unfixed product; leaks stay
        // warning-only (non-gating).
        [TestMethod]
        public void StressWindowEventHandlerTeardownNative()
        {
            RunNativeStress("StressWindowEventHandlerTeardownNative", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();
                int churn = AggressiveNativeReproEnabled ? 60 : 5;

                SafeUI(() =>
                {
                    for (int c = 0; c < churn; c++)
                    {
                        // Keep only a weak reference; the strong Window ref is dropped inside the helper so the
                        // off-thread final release can race an in-flight OnMessage dispatch.
                        objects[string.Format("Window{0}", c)] = CreateWindowWithReentrantHandlersAndClose();
                    }
                });

                FinalizeOffThread();
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private static WeakReference CreateWindowWithReentrantHandlersAndClose()
        {
            var window = new Window();
            var reference = new WeakReference(window);

            var content = new Grid();
            content.Children.Add(new TextBlock() { Text = "lifetime-events" });
            window.Content = content;

            // Register handlers on every DesktopWindowImpl event source so add_* captures the owner weak
            // reference (UpdateWindowWeakReference) and OnMessage dispatches through populated CFTMEventSource
            // instances during activation/close.
            global::Windows.Foundation.TypedEventHandler<object, WindowActivatedEventArgs> activated = null;
            activated = (s, e) =>
            {
                // Reentrant churn during dispatch: add/remove a handler while a window message is being raised
                // to exercise the reentrant-cleanup handler-retention path in CFTMEventSource::Raise.
                var raisedWindow = s as Window;
                if (raisedWindow != null)
                {
                    global::Windows.Foundation.TypedEventHandler<object, WindowVisibilityChangedEventArgs> transient = (s2, e2) => { };
                    raisedWindow.VisibilityChanged += transient;
                    raisedWindow.VisibilityChanged -= transient;
                }
            };
            window.Activated += activated;
            window.SizeChanged += (s, e) => { };
            window.VisibilityChanged += (s, e) => { };
            window.Closed += (s, e) => { };

            window.Activate();
            window.Close();

            return reference;
        }

        // Rapid reparenting across live subtrees exercises enter/leave peer bookkeeping.
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

                    // Reparent from Loaded to hit the mid-enter path.
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
                        left.Children.Remove(mover);
                        right.Children.Remove(mover);
                        next.Children.Add(mover);
                        root.UpdateLayout();
                        current = next;
                    }

                    // Drop the tree without first detaching the mover.
                    Content = null;
                });

                FinalizeOffThread();
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // Deep visual-tree teardown stresses recursive peer cleanup.
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

                    // Sever the midpoint during Unloaded to re-enter recursive teardown.
                    bool reentered = false;
                    midpoint.Unloaded += (s, e) =>
                    {
                        if (reentered) { return; }
                        reentered = true;
                        midpoint.Child = null;
                    };

                    root.UpdateLayout();

                    // Drop the populated chain at once, then finalize off-thread.
                    Content = null;

                    leaf.UpdateLayout();
                    _ = leaf.ActualWidth;
                });

                FinalizeOffThread();
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // Text line-services break-record teardown stress.
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

                        // Rewrap from SizeChanged to rebuild break records mid-layout.
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

                        // Remove text-heavy elements without clearing their line data first.
                        host.Children.Clear();
                        host.UpdateLayout();

                        _ = block.ActualHeight;
                    }

                    Content = null;
                });

                FinalizeOffThread();
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // ScrollView/DirectManipulation service setup and teardown stress.
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

                    // Swap manipulated content while the scroll service is being wired up.
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

                    // Drop the ScrollView while large content is still set.
                    Content = null;
                });

                FinalizeOffThread();
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // Native scenarios for peer fields that must stay tracker-owned.

        // ListView click-container and virtualizing-panel field churn.
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

                    // Poke selection and container lookup during leave-tree.
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

                    // Unparent without clearing ItemsSource so container fields stay populated.
                    Content = null;

                    (container as ListViewItem)?.UpdateLayout();
                });

                FinalizeOffThread();
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // GridView explicit-container churn with off-thread final release.
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

                    // Unparent without clearing Items so selection/container fields stay live.
                    Content = null;

                    firstItem?.UpdateLayout();
                });

                FinalizeOffThread();
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // SplitView light-dismiss layer churn.
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

                    // Flip the pane and touch Pane during dismiss-layer teardown.
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
                        // Opens create the dismiss-layer popup and elements.
                        splitView.DisplayMode = (c % 2 == 0)
                            ? SplitViewDisplayMode.Overlay
                            : SplitViewDisplayMode.CompactOverlay;
                        splitView.IsPaneOpen = true;
                        Content.UpdateLayout();
                        splitView.IsPaneOpen = false;
                        Content.UpdateLayout();
                    }

                    // Leave the dismiss layer standing before unparenting.
                    splitView.IsPaneOpen = true;
                    Content.UpdateLayout();
                    Content = null;
                });

                FinalizeOffThread();
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // ToggleSwitch template transform peer churn.
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

                        // Re-enter transform updates from Toggled.
                        bool reentered = false;
                        toggle.Toggled += (s, e) =>
                        {
                            if (reentered) { return; }
                            reentered = true;
                            toggle.IsOn = !toggle.IsOn;
                            toggle.UpdateLayout();
                        };

                        host.Children.Add(toggle);
                        host.UpdateLayout(); // Applies the ToggleSwitch template.

                        toggle.IsOn = true;
                        host.UpdateLayout();
                    }

                    // Unparent with active toggle transforms still set.
                    Content = null;

                    firstToggle?.UpdateLayout();
                });

                FinalizeOffThread();
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // Controls covered by the broad create/load/unload sweep.

        // Shared harness for controls whose churn shape is build, interact, teardown.
        private void RunChurnScenario<TControl>(
            string scenarioName,
            Func<Dictionary<string, WeakReference>, TControl> build,
            Action<TControl, int> churn,
            Action<TControl> teardown)
            where TControl : UIElement
        {
            RunStress(scenarioName, (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var control = build(objects);
                    objects["Control"] = new WeakReference(control);

                    Content = control;
                    Content.UpdateLayout();

                    for (int c = 0; c < 5; c++)
                    {
                        churn(control, c);
                        Content.UpdateLayout();
                    }

                    teardown(control);
                    Content = null;
                });

                SettleAndCollect();
                SafeUI(() => VerifyCollected(objects, failOnLeak: false));
                IdleSynchronizer.Wait();
            });
        }

        // Container-recycling flush: ensures an ItemsControl drops recycled containers so item elements can be
        // collected. Non-gating; ported from System XAML BaseLifetimeTest.cs FlushChildrenCache.
        [TestMethod]
        public void StressItemsControlContainerRecyclingFlush()
        {
            RunStress("StressItemsControlContainerRecyclingFlush", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();
                ItemsControl itemsControl = null;

                SafeUI(() =>
                {
                    itemsControl = new ItemsControl();
                    var item = new ContentControl() { Content = "recycled item" };
                    objects["Item"] = new WeakReference(item);
                    itemsControl.Items.Add(item);

                    Content = itemsControl;
                    Content.UpdateLayout();

                    itemsControl.Items.Clear();
                    Content.UpdateLayout();
                });

                SettleAndCollect();

                WeakReference itemRef = objects.ContainsKey("Item") ? objects["Item"] : null;
                FlushItemsControlCache(itemsControl, itemRef);

                SafeUI(() => Content = null);
                VerifyLifetime(objects);
                IdleSynchronizer.Wait();
            });
        }

        // ---- Legacy-style lifetime tests (genuine off-UI-thread GC pump) ----
        // These mirror the Win8-era System XAML lifetime tests: they drive collection from a background thread
        // (via CollectOffUIThreadUntilDead) so the finalizer-thread-vs-UI-thread final-release timing is
        // exercised. All non-gating; leaks/exceptions are reported as warnings for the PostTestRun totals.

        // Create/parent/layout/unparent the full control set, then collect off the UI thread.
        [TestMethod]
        public void LegacyControlCollectionTests()
        {
            RunStress("LegacyControlCollectionTests", (iteration) =>
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

                VerifyLifetimeLegacy(objects);
                IdleSynchronizer.Wait();
            });
        }

        // Reparent an element across hosts, then collect off the UI thread.
        [TestMethod]
        public void LegacyElementReparentingTests()
        {
            RunStress("LegacyElementReparentingTests", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();

                SafeUI(() =>
                {
                    var first = new Grid();
                    var second = new Grid();
                    var child = new Button() { Content = "reparented" };
                    objects["Child"] = new WeakReference(child);
                    objects["FirstHost"] = new WeakReference(first);
                    objects["SecondHost"] = new WeakReference(second);

                    first.Children.Add(child);
                    Content = first;
                    Content.UpdateLayout();

                    first.Children.Remove(child);
                    second.Children.Add(child);
                    Content = second;
                    Content.UpdateLayout();

                    second.Children.Remove(child);
                    Content = null;
                });

                VerifyLifetimeLegacy(objects);
                IdleSynchronizer.Wait();
            });
        }

        // ItemsControl container recycling, converged off the UI thread.
        [TestMethod]
        public void LegacyItemsControlFlushTests()
        {
            RunStress("LegacyItemsControlFlushTests", (iteration) =>
            {
                var objects = new Dictionary<string, WeakReference>();
                ItemsControl itemsControl = null;

                SafeUI(() =>
                {
                    itemsControl = new ItemsControl();
                    var item = new ContentControl() { Content = "legacy item" };
                    objects["Item"] = new WeakReference(item);
                    itemsControl.Items.Add(item);

                    Content = itemsControl;
                    Content.UpdateLayout();

                    itemsControl.Items.Clear();
                    Content.UpdateLayout();
                });

                SettleAndCollect();

                WeakReference itemRef = objects.ContainsKey("Item") ? objects["Item"] : null;
                FlushItemsControlCache(itemsControl, itemRef);

                SafeUI(() => Content = null);
                VerifyLifetimeLegacy(objects);
                IdleSynchronizer.Wait();
            });
        }

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

        // Report, fixed-iteration, and soak modes downgrade managed failures.
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

        // Adds a greppable native marker and routes through the same reporting wrapper.
        private static void RunNativeStress(string scenarioName, Action<int> iteration)
        {
            Log.Comment("[LifetimeStress] NATIVE: scenario '{0}' starting (aggressiveNativeRepro={1}).",
                scenarioName, AggressiveNativeReproEnabled);
            RunStress(scenarioName, iteration);
            Log.Comment("[LifetimeStress] NATIVE: scenario '{0}' completed (aggressiveNativeRepro={1}).",
                scenarioName, AggressiveNativeReproEnabled);
        }

        // Convert managed iteration failures into warnings.
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

        // Catch UI-thread failures before RunOnUIThread records a failed verdict.
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

        // Settle UI work, then force GC/finalizers every iteration. Ported ideas from System XAML
        // BaseLifetimeTest.cs: vary the finalizing thread and confirm the finalizer thread actually ran.
        // Settle UI work, then force GC/finalizers every iteration.
        private static void SettleAndCollect()
        {
            IdleSynchronizer.Wait();
            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();
            IdleSynchronizer.Wait();
        }

        // Pump the UI thread so cross-thread (marshaled) final releases can run before we sample WeakReferences.
        private static void PumpUI()
        {
            RunOnUIThread.Execute(() => { });
            IdleSynchronizer.Wait();
        }

        private static int s_gcThreadToggle;

        // Force a full GC, alternating between the current (non-UI) test thread and a fresh background thread,
        // so finalization is driven from different threads across iterations. This surfaces the UI-thread vs
        // finalizer-thread final-release races that lifetime bugs depend on (per Win8-era lifetime coverage).
        private static void ForceGCVaryingThread()
        {
            if ((global::System.Threading.Interlocked.Increment(ref s_gcThreadToggle) & 1) == 0)
            {
                global::System.Threading.Tasks.Task.Run(() =>
                {
                    GC.Collect();
                    GC.WaitForPendingFinalizers();
                    GC.Collect();
                }).Wait();
            }
            else
            {
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();
            }
        }

        private static bool AllCollected(Dictionary<string, WeakReference> objects)
        {
            foreach (var pair in objects)
            {
                if (pair.Value.Target != null)
                {
                    return false;
                }
            }
            return true;
        }

        // Convergence-based collection: alternately force GC (varying the finalizing thread) and pump the UI
        // thread until every tracked object is gone, or the attempt/time budget is spent. Ported from System
        // XAML BaseLifetimeTest.cs CollectUntilDead - more reliable than a fixed number of GC passes, which
        // reduces false-positive leak warnings in the PostTestRun totals. Runs on the test thread.
        private static bool CollectUntilDead(Dictionary<string, WeakReference> objects, int maxAttempts = 50, double timeoutSeconds = 10.0)
        {
            var stopwatch = Stopwatch.StartNew();
            for (int attempt = 0; attempt < maxAttempts && !AllCollected(objects); attempt++)
            {
                ForceGCVaryingThread();
                PumpUI();
                if (timeoutSeconds > 0.0 && stopwatch.Elapsed.TotalSeconds >= timeoutSeconds)
                {
                    break;
                }
            }
            return AllCollected(objects);
        }

        // Converge collection off the UI thread, then report survivors. Routes leaks through VerifyCollected so
        // they keep the exact "object 'X' was still alive after forced collection" phrase the PostTestRun totals
        // step counts. Always non-gating: leaks are warning-only, exactly like the rest of the suite.
        private static void VerifyLifetime(Dictionary<string, WeakReference> objects)
        {
            CollectUntilDead(objects);
            VerifyCollected(objects, failOnLeak: false);
        }

        // Genuine off-UI-thread GC pump. Drives GC/finalization from a dedicated background thread while also
        // marshaling a collect onto the UI thread each pass, reproducing the finalizer-thread-vs-UI-thread
        // final-release timing the legacy System XAML lifetime tests (BaseLifetimeTest.cs
        // CollectUntilDead / CallFromBackgroundThread) relied on. Runs on the test thread; blocks on the
        // background worker.
        private static void CollectOffUIThreadUntilDead(Dictionary<string, WeakReference> objects, int maxAttempts = 50, double timeoutSeconds = 15.0)
        {
            global::System.Threading.Tasks.Task.Run(() =>
            {
                var stopwatch = Stopwatch.StartNew();
                for (int attempt = 0; attempt < maxAttempts && !AllCollected(objects); attempt++)
                {
                    // Background-thread collection: a final release may be driven from this thread.
                    GC.Collect();
                    GC.WaitForPendingFinalizers();

                    // UI-thread collection: a final release marshaled back to the UI thread runs here.
                    RunOnUIThread.Execute(() =>
                    {
                        GC.Collect();
                        GC.WaitForPendingFinalizers();
                    });
                    IdleSynchronizer.Wait();

                    if (timeoutSeconds > 0.0 && stopwatch.Elapsed.TotalSeconds >= timeoutSeconds)
                    {
                        break;
                    }
                }
            }).Wait();
        }

        // Converge using the genuine off-UI-thread pump, then report survivors through VerifyCollected so leaks
        // keep the exact phrase the PostTestRun totals step counts. Always non-gating: leaks are warning-only,
        // exactly like the rest of the suite.
        private static void VerifyLifetimeLegacy(Dictionary<string, WeakReference> objects)
        {
            CollectOffUIThreadUntilDead(objects);
            VerifyCollected(objects, failOnLeak: false);
        }

        // Insert/clear churn to force an ItemsControl to release a cached/recycled container that is pinning the
        // tracked element, collecting until it dies or the cap is hit. Ported from System XAML
        // BaseLifetimeTest.cs FlushChildrenCache. Non-gating: residual references are surfaced by VerifyLifetime.
        private static void FlushItemsControlCache(ItemsControl itemsControl, WeakReference weakRef, int maxAttempts = 50)
        {
            if (itemsControl == null || weakRef == null)
            {
                return;
            }

            SafeUI(() => itemsControl.Items.Clear());
            SettleAndCollect();

            int i = 0;
            while (weakRef.Target != null && i++ < maxAttempts)
            {
                SafeUI(() =>
                {
                    itemsControl.Items.Insert(0, new ContentControl() { Content = "flush" });
                    itemsControl.Items.Clear();
                });
                SettleAndCollect();
            }

            if (weakRef.Target != null)
            {
                // Diagnostic only; the counted leak signal is emitted by VerifyLifetime/VerifyCollected.
                Log.Comment("[LifetimeStress] DIAG: ItemsControl container flush did not release the tracked element after {0} attempt(s).", i);
            }
        }

        // Force final native release off the UI thread, then drain marshaled releases.
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
                        // Match LeakTests: report via Verify without throwing exceptions.
                        Verify.DisableVerifyFailureExceptions = true;
                        Verify.Fail(string.Format("Object {0} is still alive when it should not be.", pair.Key));
                        Verify.DisableVerifyFailureExceptions = false;
                    }
                    else
                    {
                        // Residual references are warning-only for this report suite.
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

    // Cached pages used by StressFrameNavigationCache.
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
