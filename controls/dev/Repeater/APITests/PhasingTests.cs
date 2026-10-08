// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Private.Controls;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests
{
    using ElementFactory = Microsoft.UI.Xaml.Controls.ElementFactory;

    // Bug 17377723: crash in CControlTemplate::CreateXBindConnector in RS5.
    [TestClass]
    public partial class PhasingTests : ApiTestBase
    {
        const int expectedLastRealizedIndex = 8;

        [TestMethod]
        public void ValidatePhaseInvokeAndOrdering()
        {
            ItemsRepeater repeater = null;
            int numPhases = 6; // 0 to 5
            ManualResetEvent buildTreeCompleted = new ManualResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                var itemTemplate = (DataTemplate)XamlReader.Load(
                       @"<DataTemplate  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <Button Width='100' Height='100'/>
                        </DataTemplate>");
                repeater = new ItemsRepeater()
                {
                    ItemsSource = Enumerable.Range(0, 10),
                    ItemTemplate = new CustomElementFactory(numPhases),
                    Layout = new StackLayout(),
                };
                
                repeater.ElementPrepared += (sender, args) =>
                {
                    if (args.Index == expectedLastRealizedIndex)
                    {
                        Log.Comment("Item 8 Created!" );
                        RepeaterTestHooks.BuildTreeCompleted += (sender1, args1) =>
                        {
                            buildTreeCompleted.Set();
                        };
                    }
                };

                Content = new ItemsRepeaterScrollHost()
                {
                    Width = 400,
                    Height = 400,
                    ScrollViewer = new ScrollViewer
                    {
                        Content = repeater
                    }
                };

                // CompositionTarget.Rendering += (sender, args) => { Log.Comment("Rendering"); }; // debugging aid
            });

            if(buildTreeCompleted.WaitOne(TimeSpan.FromMilliseconds(2000)))
            {
                RunOnUIThread.Execute(() =>
                {
                    var calls = ElementPhasingManager.ProcessedCalls;

                    Verify.AreEqual(9, calls.Count);
                    calls[0].RemoveAt(0); // Remove the create we did for first measure.
                    foreach (var index in calls.Keys)
                    {
                        var phases = calls[index];
                        Verify.AreEqual(6, phases.Count);
                        for (int i = 0; i < phases.Count; i++)
                        {
                            Verify.AreEqual(i, phases[i]);
                        }
                    }

                    ElementPhasingManager.ProcessedCalls.Clear();
                });
            }
            else
            {
                Verify.Fail("Failed on waiting on build tree.");
            }
        }

        [TestMethod]
        public void ValidateXBindWithoutPhasing()
        {
            ItemsRepeater repeater = null;
            int numPhases = 1; // Just Phase 0 for x:Bind
            ManualResetEvent ElementLoadedCompleted = new ManualResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                var itemTemplate = (DataTemplate)XamlReader.Load(
                       @"<DataTemplate  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <Button Width='100' Height='100'/>
                        </DataTemplate>");
                repeater = new ItemsRepeater()
                {
                    ItemsSource = Enumerable.Range(0, 10),
                    ItemTemplate = new CustomElementFactory(numPhases),
                    Layout = new StackLayout(),
                };

                repeater.ElementPrepared += (sender, args) =>
                {
                    if (args.Index == expectedLastRealizedIndex)
                    {
                        Log.Comment("Item 8 Created!");
                        ElementLoadedCompleted.Set();
                    }
                };

                Content = new ItemsRepeaterScrollHost()
                {
                    Width = 400,
                    Height = 400,
                    ScrollViewer = new ScrollViewer
                    {
                        Content = repeater
                    }
                };
            });

            if(ElementLoadedCompleted.WaitOne(TimeSpan.FromMilliseconds(2000)))
            {
                RunOnUIThread.Execute(() =>
                {
                    var calls = ElementPhasingManager.ProcessedCalls;

                    Verify.AreEqual(calls.Count, 9);
                    calls[0].RemoveAt(0); // Remove the create we did for first measure.
                    foreach (var index in calls.Keys)
                    {
                        var phases = calls[index];
                        Verify.AreEqual(1, phases.Count); // Just phase 0
                    }

                    ElementPhasingManager.ProcessedCalls.Clear();
                });
            }
            else
            {
                Verify.Fail("Failed on waiting on build tree.");
            }
        }

        // Scenario: realize items whose phased bindings (four phases each) take about 15 ms per phase, so the total phase
        //           work is far larger than one frame's budget.
        // Expected: every realized item still receives phases 0 to 3 in increasing order, and the work is spread over
        //           several frames instead of blocking one.
        // A failure means: slow x:Phase bindings would freeze the UI thread or leave items partially bound.
        [TestMethod]
        [TestProperty("Description", "Verifies phased binding work that exceeds a frame budget is spread over several frames and still completes every phase in order.")]
        public void ValidateSlowPhasesAreSpreadOverFrames()
        {
            const int phaseCount = 4;
            const int itemCount = 6;
            var calls = new Dictionary<int, List<int>>();
            var frames = new HashSet<int>();
            int frame = 0;
            var allPhasesDone = new ManualResetEvent(false);
            EventHandler<object> onRendering = (s, e) => frame++;

            RunOnUIThread.Execute(() =>
            {
                CompositionTarget.Rendering += onRendering;
                Action<int, int> record = (item, phase) =>
                {
                    if (!calls.ContainsKey(item))
                    {
                        calls[item] = new List<int>();
                    }
                    calls[item].Add(phase);
                    if (phase > 0)
                    {
                        frames.Add(frame);
                    }
                    if (calls.Count == itemCount && calls.Values.All(phases => phases.Count(p => p == phaseCount - 1) > 0))
                    {
                        allPhasesDone.Set();
                    }
                };

                var repeater = new ItemsRepeater()
                {
                    ItemsSource = Enumerable.Range(0, itemCount),
                    ItemTemplate = new SlowPhasedElementFactory(phaseCount, record),
                    Layout = new StackLayout(),
                };
                Content = new ItemsRepeaterScrollHost()
                {
                    Width = 400,
                    Height = 400,
                    ScrollViewer = new ScrollViewer { Content = repeater }
                };
            });

            bool completed = allPhasesDone.WaitOne(TimeSpan.FromSeconds(10));
            RunOnUIThread.Execute(() =>
            {
                CompositionTarget.Rendering -= onRendering;
                foreach (var entry in calls)
                {
                    Log.Comment($"Item {entry.Key}: phases {string.Join(",", entry.Value)}");
                }
                Log.Comment("Frames with phase work: " + frames.Count);
                Verify.IsTrue(completed, "Every item reached its last phase.");
                foreach (var entry in calls)
                {
                    var distinct = entry.Value.Distinct().ToList();
                    Verify.AreEqual(phaseCount, distinct.Count, $"Item {entry.Key} received every phase.");
                    for (int i = 1; i < distinct.Count; i++)
                    {
                        Verify.IsGreaterThan(distinct[i], distinct[i - 1], $"Item {entry.Key} phases increase.");
                    }
                }
                Verify.IsGreaterThan(frames.Count, 1, "The slow phase work was spread over several frames.");
                Content = null;
            });
        }

        private partial class SlowPhasedElementFactory : ElementFactory
        {
            private readonly int _phaseCount;
            private readonly Action<int, int> _record;

            public SlowPhasedElementFactory(int phaseCount, Action<int, int> record)
            {
                _phaseCount = phaseCount;
                _record = record;
            }

            protected override UIElement GetElementCore(ElementFactoryGetArgs args)
            {
                var element = new Button() { Width = 100, Height = 50 };
                XamlBindingHelper.SetDataTemplateComponent(element, new SlowPhasingComponent(_phaseCount, _record));
                return element;
            }

            protected override void RecycleElementCore(ElementFactoryRecycleArgs args)
            {
            }
        }

        private partial class SlowPhasingComponent : IDataTemplateComponent
        {
            private readonly int _phaseCount;
            private readonly Action<int, int> _record;
            private int _item = -1;

            public SlowPhasingComponent(int phaseCount, Action<int, int> record)
            {
                _phaseCount = phaseCount;
                _record = record;
            }

            public void Recycle()
            {
            }

            public void ProcessBindings(object item, int itemIndex, int phase, out int nextPhase)
            {
                if (phase == 0)
                {
                    _item = (int)item;
                }
                else
                {
                    Thread.Sleep(15);
                }

                _record(_item, phase);
                nextPhase = phase >= _phaseCount - 1 ? -1 : phase + 1;
            }
        }

        private partial class CustomElementFactory : ElementFactory
        {
            private int _numPhases;
            private RecyclePool _recyclePool = new RecyclePool();
            private string key = "foobar";

            public CustomElementFactory(int numPhases)
            {
                _numPhases = numPhases;
            }

            protected override UIElement GetElementCore(ElementFactoryGetArgs args)
            {
                var element = _recyclePool.TryGetElement(key, args.Parent);
                if (element == null)
                {
                    element = new Button() { Width = 100, Height = 100 };
                }

                var elementManager = new ElementPhasingManager(_numPhases);
                XamlBindingHelper.SetDataTemplateComponent(element, elementManager);
                return element;
            }

            protected override void RecycleElementCore(ElementFactoryRecycleArgs args)
            {
                XamlBindingHelper.GetDataTemplateComponent(args.Element).Recycle();
                _recyclePool.PutElement(args.Element, key, args.Parent);
            }
        }

        private partial class ElementPhasingManager : IDataTemplateComponent
        {
            private int _numPhases = 1; // Default is just phase 0
            private int _data = -1;

            public List<int> Data { get; private set; }
            public bool IsCleared { get; private set; }

            // data index -> list<phases>
            public static Dictionary<int, List<int>> ProcessedCalls { get; set; }

            public ElementPhasingManager(int numPhases)
            {
                _numPhases = numPhases;
            }

            public void Recycle()
            {
                IsCleared = true;
                Log.Comment(string.Format("Recycle Index:{0}", _data));
            }

            public void ProcessBindings(object item, int itemIndex, int phase, out int nextPhase)
            {
                if (Data == null)
                {
                    Data = new List<int>();
                }

                if (ProcessedCalls == null)
                {
                    ProcessedCalls = new Dictionary<int, List<int>>();
                }

                _data = (int)item;
                Data.Add(_data);
                if (!ProcessedCalls.ContainsKey(_data))
                {
                    ProcessedCalls.Add(_data, new List<int>());
                }

                ProcessedCalls[_data].Add(phase);

                nextPhase = phase >= _numPhases -1 ? -1 : phase + 1;
                Log.Comment(string.Format("Index:{0}  Phase:{1}  NextPhase:{2}", item.ToString(), phase, nextPhase));
            }
        }
    }
}
