// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System;
using System.Collections.ObjectModel;
using System.Text;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests
{
    [TestClass]
    public class SelectionModelTests : ApiTestBase
    {
        [TestMethod]
        public void ValidateOneLevelSingleSelectionNoSource()
        {
            RunOnUIThread.Execute(() =>
            {
                SelectionModel selectionModel = new SelectionModel() { SingleSelect = true };
                Log.Comment("No source set.");
                Select(selectionModel, 4, true);
                ValidateSelection(selectionModel, new List<IndexPath>() { Path(4) });
                Select(selectionModel, 4, false);
                ValidateSelection(selectionModel, new List<IndexPath>() { });
            });
        }

        [TestMethod]
        public void ValidateOneLevelSingleSelection()
        {
            RunOnUIThread.Execute(() =>
            {
                SelectionModel selectionModel = new SelectionModel() { SingleSelect = true };
                Log.Comment("Set the source to 10 items");
                selectionModel.Source = Enumerable.Range(0, 10).ToList();

                // Check index selection
                Select(selectionModel, 3, true);
                ValidateSelection(selectionModel, new List<IndexPath>() { Path(3) }, new List<IndexPath>() { Path() });
                Select(selectionModel, 3, false);
                ValidateSelection(selectionModel, new List<IndexPath>() { });

                // Check index path selection
                Select(selectionModel, Path(4), true);
                ValidateSelection(selectionModel, new List<IndexPath>() { Path(4) }, new List<IndexPath>() { Path() });
                Select(selectionModel, Path(4), false);
                ValidateSelection(selectionModel, new List<IndexPath>() { });
            });
        }

        [TestMethod]
        public void ValidateSelectionChangedEventSingleSelection()
        {
            RunOnUIThread.Execute(() =>
            {
                SelectionModel selectionModel = new SelectionModel() { SingleSelect = true };
                selectionModel.Source = Enumerable.Range(0, 10).ToList();

                bool select = true;
                int selectionChangedFiredCount = 0;
                selectionModel.SelectionChanged += delegate (SelectionModel sender, SelectionModelSelectionChangedEventArgs args) {
                    selectionChangedFiredCount++;

                    // Verify SelectionChanged was raised after selection state was changed in the SelectionModel
                    if (select)
                    {
                        ValidateSelection(selectionModel, new List<IndexPath>() { Path(4) }, new List<IndexPath>() { Path() });
                    }
                    else
                    {
                        ValidateSelection(selectionModel, new List<IndexPath>() { });
                    }
                };

                Select(selectionModel, Path(4), select);
                Verify.AreEqual(1, selectionChangedFiredCount);

                select = false;
                Select(selectionModel, Path(4), select);
                Verify.AreEqual(2, selectionChangedFiredCount);
            });
        }

        [TestMethod]
        public void ValidateSelectionChangedEventMultipleSelection()
        {
            RunOnUIThread.Execute(() =>
            {
                SelectionModel selectionModel = new SelectionModel();
                selectionModel.Source = Enumerable.Range(0, 10).ToList();

                int selectionChangedFiredCount = 0;
                selectionModel.SelectionChanged += delegate (SelectionModel sender, SelectionModelSelectionChangedEventArgs args) 
                {
                    selectionChangedFiredCount++;

                    // Verify SelectionChanged was raised after selection state was changed in the SelectionModel
                    ValidateSelection(selectionModel, new List<IndexPath>() { Path(4) }, new List<IndexPath>() { Path() });
                };

                Select(selectionModel, 4, true);
                Verify.AreEqual(1, selectionChangedFiredCount);
            });
        }

        [TestMethod]
        public void ValidateCanSetSelectedIndex()
        {
            RunOnUIThread.Execute(() =>
            {
                var model = new SelectionModel();
                var ip = IndexPath.CreateFrom(34);
                model.SelectedIndex = ip;
                Verify.AreEqual(0, ip.CompareTo(model.SelectedIndex));
            });
        }

        [TestMethod]
        public void ValidateOneLevelMultipleSelection()
        {
            RunOnUIThread.Execute(() =>
            {
                SelectionModel selectionModel = new SelectionModel();
                selectionModel.Source = Enumerable.Range(0, 10).ToList();

                Select(selectionModel, 4, true);
                ValidateSelection(selectionModel, new List<IndexPath>() { Path(4) }, new List<IndexPath>() { Path() });
                SelectRangeFromAnchor(selectionModel, 8, true /* select */);
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(4),
                        Path(5),
                        Path(6),
                        Path(7),
                        Path(8)
                    },
                    new List<IndexPath>() { Path() });

                ClearSelection(selectionModel);
                SetAnchorIndex(selectionModel, 6);
                SelectRangeFromAnchor(selectionModel, 3, true /* select */);
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(3),
                        Path(4),
                        Path(5),
                        Path(6)
                    },
                    new List<IndexPath>() { Path() });

                SetAnchorIndex(selectionModel, 4);
                SelectRangeFromAnchor(selectionModel, 5, false /* select */);
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(3),
                        Path(6)
                    },
                    new List<IndexPath>() { Path() });
            });
        }

        [TestMethod]
        public void ValidateTwoLevelSingleSelection()
        {
            RunOnUIThread.Execute(() =>
            {
                SelectionModel selectionModel = new SelectionModel();
                Log.Comment("Setting the source");
                selectionModel.Source = CreateNestedData(1 /* levels */ , 2 /* groupsAtLevel */, 2 /* countAtLeaf */);
                Select(selectionModel, 1, 1, true);
                ValidateSelection(selectionModel,
                    new List<IndexPath>() { Path(1, 1) }, new List<IndexPath>() { Path(), Path(1) });
                Select(selectionModel, 1, 1, false);
                ValidateSelection(selectionModel, new List<IndexPath>() { });
            });
        }

        [TestMethod]
        public void ValidateTwoLevelMultipleSelection()
        {
            RunOnUIThread.Execute(() =>
            {
                SelectionModel selectionModel = new SelectionModel();
                Log.Comment("Setting the source");
                selectionModel.Source = CreateNestedData(1 /* levels */ , 3 /* groupsAtLevel */, 3 /* countAtLeaf */);

                Select(selectionModel, 1, 2, true);
                ValidateSelection(selectionModel, new List<IndexPath>() { Path(1, 2) }, new List<IndexPath>() { Path(), Path(1) });
                SelectRangeFromAnchor(selectionModel, 2, 2, true /* select */);
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(1, 2),
                        Path(2), // Inner node should be selected since everything 2.* is selected
                        Path(2, 0),
                        Path(2, 1),
                        Path(2, 2)
                    },
                    new List<IndexPath>()
                    {
                        Path(),
                        Path(1)
                    },
                    1 /* selectedInnerNodes */);

                ClearSelection(selectionModel);
                SetAnchorIndex(selectionModel, 2, 1);
                SelectRangeFromAnchor(selectionModel, 0, 1, true /* select */);
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(0, 1),
                        Path(0, 2),
                        Path(1, 0),
                        Path(1, 1),
                        Path(1, 2),
                        Path(1),
                        Path(2, 0),
                        Path(2, 1)
                    },
                    new List<IndexPath>()
                    {
                        Path(),
                        Path(0),
                        Path(2),
                    },
                    1 /* selectedInnerNodes */);

                SetAnchorIndex(selectionModel, 1, 1);
                SelectRangeFromAnchor(selectionModel, 2, 0, false /* select */);
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(0, 1),
                        Path(0, 2),
                        Path(1, 0),
                        Path(2, 1)
                    },
                    new List<IndexPath>()
                    {
                        Path(),
                        Path(1),
                        Path(0),
                        Path(2),
                    },
                    0 /* selectedInnerNodes */);

                ClearSelection(selectionModel);
                ValidateSelection(selectionModel, new List<IndexPath>() { });
            });
        }

        [TestMethod]
        public void ValidateNestedSingleSelection()
        {
            RunOnUIThread.Execute(() =>
            {
                SelectionModel selectionModel = new SelectionModel() { SingleSelect = true };
                Log.Comment("Setting the source");
                selectionModel.Source = CreateNestedData(3 /* levels */ , 2 /* groupsAtLevel */, 2 /* countAtLeaf */);
                var path = Path(1, 0, 1, 1);
                Select(selectionModel, path, true);
                ValidateSelection(selectionModel,
                    new List<IndexPath>() { path },
                    new List<IndexPath>()
                    {
                        Path(),
                        Path(1),
                        Path(1, 0),
                        Path(1, 0, 1),
                    });
                Select(selectionModel, Path(0, 0, 1, 0), true);
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(0, 0, 1, 0)
                    },
                    new List<IndexPath>()
                    {
                        Path(),
                        Path(0),
                        Path(0, 0),
                        Path(0, 0, 1)
                    });
                Select(selectionModel, Path(0, 0, 1, 0), false);
                ValidateSelection(selectionModel, new List<IndexPath>() { });
            });
        }

        [TestMethod]
        public void ValidateNestedMultipleSelection()
        {
            ValidateNestedMultipleSelection(true /* handleChildrenRequested */);
            ValidateNestedMultipleSelection(false /* handleChildrenRequested */);
        }

        private void ValidateNestedMultipleSelection(bool handleChildrenRequested)
        {
            RunOnUIThread.Execute(() =>
            {
                SelectionModel selectionModel = new SelectionModel();
                List<IndexPath> sourcePaths = new List<IndexPath>();

                Log.Comment("Setting the source");
                selectionModel.Source = CreateNestedData(3 /* levels */ , 2 /* groupsAtLevel */, 4 /* countAtLeaf */);
                if (handleChildrenRequested)
                {
                    selectionModel.ChildrenRequested += (SelectionModel sender, SelectionModelChildrenRequestedEventArgs args) =>
                    {
                        Log.Comment("ChildrenRequestedIndexPath:" + args.SourceIndex);
                        sourcePaths.Add(args.SourceIndex);
                        args.Children = args.Source is IEnumerable ? args.Source : null;
                    };
                }

                var startPath = Path(1, 0, 1, 0);
                Select(selectionModel, startPath, true);
                ValidateSelection(selectionModel,
                    new List<IndexPath>() { startPath },
                    new List<IndexPath>()
                    {
                        Path(),
                        Path(1),
                        Path(1, 0),
                        Path(1, 0, 1)
                    });

                var endPath = Path(1, 1, 1, 0);
                SelectRangeFromAnchor(selectionModel, endPath, true /* select */);

                if (handleChildrenRequested)
                {
                    // Validate SourceIndices.
                    var expectedSourceIndices = new List<IndexPath>()
                    {
                        Path(1),
                        Path(1, 0),
                        Path(1, 0, 1),
                        Path(1, 1),
                        Path(1, 0, 1, 3),
                        Path(1, 0, 1, 2),
                        Path(1, 0, 1, 1),
                        Path(1, 0, 1, 0),
                        Path(1, 1, 1),
                        Path(1, 1, 0),
                        Path(1, 1, 0, 3),
                        Path(1, 1, 0, 2),
                        Path(1, 1, 0, 1),
                        Path(1, 1, 0, 0),
                        Path(1, 1, 1, 0)
                    };

                    Verify.AreEqual(expectedSourceIndices.Count, sourcePaths.Count);
                    for (int i = 0; i < expectedSourceIndices.Count; i++)
                    {
                        Verify.IsTrue(AreEqual(expectedSourceIndices[i], sourcePaths[i]));
                    }
                }

                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(1, 0, 1, 0),
                        Path(1, 0, 1, 1),
                        Path(1, 0, 1, 2),
                        Path(1, 0, 1, 3),
                        Path(1, 0, 1),
                        Path(1, 1, 0, 0),
                        Path(1, 1, 0, 1),
                        Path(1, 1, 0, 2),
                        Path(1, 1, 0, 3),
                        Path(1, 1, 0),
                        Path(1, 1, 1, 0),
                    },
                    new List<IndexPath>()
                    {
                        Path(),
                        Path(1),
                        Path(1, 0),
                        Path(1, 1),
                        Path(1, 1, 1),
                    },
                    2 /* selectedInnerNodes */);

                ClearSelection(selectionModel);
                ValidateSelection(selectionModel, new List<IndexPath>() { });

                startPath = Path(0, 1, 0, 2);
                SetAnchorIndex(selectionModel, startPath);
                endPath = Path(0, 0, 0, 2);
                SelectRangeFromAnchor(selectionModel, endPath, true /* select */);
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                         Path(0, 0, 0, 2),
                         Path(0, 0, 0, 3),
                         Path(0, 0, 1, 0),
                         Path(0, 0, 1, 1),
                         Path(0, 0, 1, 2),
                         Path(0, 0, 1, 3),
                         Path(0, 0, 1),
                         Path(0, 1, 0, 0),
                         Path(0, 1, 0, 1),
                         Path(0, 1, 0, 2),
                    },
                    new List<IndexPath>()
                    {
                        Path(),
                        Path(0),
                        Path(0, 0),
                        Path(0, 0, 0),
                        Path(0, 1),
                        Path(0, 1, 0),
                    },
                    1 /* selectedInnerNodes */);

                startPath = Path(0, 1, 0, 2);
                SetAnchorIndex(selectionModel, startPath);
                endPath = Path(0, 0, 0, 2);
                SelectRangeFromAnchor(selectionModel, endPath, false /* select */);
                ValidateSelection(selectionModel, new List<IndexPath>() { });
            });
        }

        [TestMethod]
        public void ValidateInserts()
        {
            RunOnUIThread.Execute(() =>
            {
                var data = new ObservableCollection<int>(Enumerable.Range(0, 10));
                var selectionModel = new SelectionModel();
                selectionModel.Source = data;

                selectionModel.Select(3);
                selectionModel.Select(4);
                selectionModel.Select(5);
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(3),
                        Path(4),
                        Path(5),
                    },
                    new List<IndexPath>()
                    {
                        Path()
                    });

                Log.Comment("Insert in selected range: Inserting 3 items at index 4");
                data.Insert(4, 41);
                data.Insert(4, 42);
                data.Insert(4, 43);
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(3),
                        Path(7),
                        Path(8),
                    },
                    new List<IndexPath>()
                    {
                        Path()
                    });

                Log.Comment("Insert before selected range: Inserting 3 items at index 0");
                data.Insert(0, 100);
                data.Insert(0, 101);
                data.Insert(0, 102);
                ValidateSelection(selectionModel,
                   new List<IndexPath>()
                   {
                        Path(6),
                        Path(10),
                        Path(11),
                   },
                   new List<IndexPath>()
                   {
                       Path()
                   });

                Log.Comment("Insert after selected range: Inserting 3 items at index 12");
                data.Insert(12, 1000);
                data.Insert(12, 1001);
                data.Insert(12, 1002);
                ValidateSelection(selectionModel,
                  new List<IndexPath>()
                  {
                        Path(6),
                        Path(10),
                        Path(11),
                  },
                    new List<IndexPath>()
                    {
                        Path()
                    });
            });
        }

        [TestMethod]
        public void ValidateGroupInserts()
        {
            RunOnUIThread.Execute(() =>
            {
                var data = CreateNestedData(1 /* levels */ , 3 /* groupsAtLevel */, 3 /* countAtLeaf */);
                var selectionModel = new SelectionModel();
                selectionModel.Source = data;

                selectionModel.Select(1, 1);
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(1, 1),
                    },
                    new List<IndexPath>()
                    {
                        Path(),
                        Path(1),
                    });

                Log.Comment("Insert before selected range: Inserting item at group index 0");
                data.Insert(0, 100);
                ValidateSelection(selectionModel,
                   new List<IndexPath>()
                   {
                        Path(2, 1)
                   },
                   new List<IndexPath>()
                   {
                       Path(),
                       Path(2),
                   });

                Log.Comment("Insert after selected range: Inserting item at group index 3");
                data.Insert(3, 1000);
                ValidateSelection(selectionModel,
                  new List<IndexPath>()
                  {
                      Path(2, 1)
                  },
                  new List<IndexPath>()
                  {
                      Path(),
                      Path(2),
                  });
            });
        }

        [TestMethod]
        public void ValidateRemoves()
        {
            RunOnUIThread.Execute(() =>
            {
                var data = new ObservableCollection<int>(Enumerable.Range(0, 10));
                var selectionModel = new SelectionModel();
                selectionModel.Source = data;

                selectionModel.Select(6);
                selectionModel.Select(7);
                selectionModel.Select(8);
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(6),
                        Path(7),
                        Path(8)
                    },
                    new List<IndexPath>()
                    {
                        Path()
                    });

                Log.Comment("Remove before selected range: Removing item at index 0");
                data.RemoveAt(0);
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(5),
                        Path(6),
                        Path(7)
                    },
                    new List<IndexPath>()
                    {
                        Path()
                    });

                Log.Comment("Remove from before to middle of selected range: Removing items at index 3, 4, 5");
                data.RemoveAt(3);
                data.RemoveAt(3);
                data.RemoveAt(3);
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(3),
                        Path(4)
                    },
                    new List<IndexPath>()
                    {
                        Path()
                    });

                Log.Comment("Remove after selected range: Removing item at index 5");
                data.RemoveAt(5);
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(3),
                        Path(4)
                    },
                    new List<IndexPath>()
                    {
                        Path()
                    });
            });
        }

        [TestMethod]
        public void ValidateGroupRemoves()
        {
            RunOnUIThread.Execute(() =>
            {
                var data = CreateNestedData(1 /* levels */ , 3 /* groupsAtLevel */, 3 /* countAtLeaf */);
                var selectionModel = new SelectionModel();
                selectionModel.Source = data;

                selectionModel.Select(1, 1);
                selectionModel.Select(1, 2);
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(1, 1),
                        Path(1, 2)
                    },
                    new List<IndexPath>()
                    {
                       Path(),
                       Path(1),
                    });

                Log.Comment("Remove before selected range: Removing item at group index 0");
                data.RemoveAt(0);
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(0, 1),
                        Path(0, 2)
                    },
                    new List<IndexPath>()
                    {
                        Path(),
                        Path(0),
                    });

                Log.Comment("Remove after selected range: Removing item at group index 1");
                data.RemoveAt(1);
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(0, 1),
                        Path(0, 2)
                    },
                    new List<IndexPath>()
                    {
                        Path(),
                        Path(0),
                    });

                Log.Comment("Remove group containing selected items");
                data.RemoveAt(0);
                ValidateSelection(selectionModel, new List<IndexPath>());
            });
        }

        [TestMethod]
        public void CanReplaceItem()
        {
            RunOnUIThread.Execute(() =>
            {
                var data = new ObservableCollection<int>(Enumerable.Range(0, 10));
                var selectionModel = new SelectionModel();
                selectionModel.Source = data;

                selectionModel.Select(3);
                selectionModel.Select(4);
                selectionModel.Select(5);
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(3),
                        Path(4),
                        Path(5),
                    },
                    new List<IndexPath>()
                    {
                        Path()
                    });

                data[3] = 300;
                data[4] = 400;
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(5),
                    },
                    new List<IndexPath>()
                    {
                        Path()
                    });
            });
        }

        [TestMethod]
        public void ValidateGroupReplaceLosesSelection()
        {
            RunOnUIThread.Execute(() =>
            {
                var data = CreateNestedData(1 /* levels */ , 3 /* groupsAtLevel */, 3 /* countAtLeaf */);
                var selectionModel = new SelectionModel();
                selectionModel.Source = data;

                selectionModel.Select(1, 1);
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(1, 1)
                    },
                    new List<IndexPath>()
                    {
                        Path(),
                        Path(1)
                    });

                data[1] = new ObservableCollection<int>(Enumerable.Range(0, 5));
                ValidateSelection(selectionModel, new List<IndexPath>());
            });
        }

        [TestMethod]
        public void ValidateClear()
        {
            RunOnUIThread.Execute(() =>
            {
                var data = new ObservableCollection<int>(Enumerable.Range(0, 10));
                var selectionModel = new SelectionModel();
                selectionModel.Source = data;

                selectionModel.Select(3);
                selectionModel.Select(4);
                selectionModel.Select(5);
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(3),
                        Path(4),
                        Path(5),
                    },
                    new List<IndexPath>()
                    {
                        Path()
                    });

                data.Clear();
                ValidateSelection(selectionModel, new List<IndexPath>());
            });
        }

        [TestMethod]
        public void ValidateGroupClear()
        {
            RunOnUIThread.Execute(() =>
            {
                var data = CreateNestedData(1 /* levels */ , 3 /* groupsAtLevel */, 3 /* countAtLeaf */);
                var selectionModel = new SelectionModel();
                selectionModel.Source = data;

                selectionModel.Select(1, 1);
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(1, 1)
                    },
                    new List<IndexPath>()
                    {
                        Path(),
                        Path(1)
                    });

                (data[1] as IList).Clear();
                ValidateSelection(selectionModel, new List<IndexPath>());
            });
        }

        // In some cases the leaf node might get a collection change that affects an ancestors selection
        // state. In this case we were not raising selection changed event. For example, if all elements 
        // in a group are selected and a new item gets inserted - the parent goes from selected to partially 
        // selected. In that case we need to raise the selection changed event so that the header containers 
        // can show the correct visual.
        [TestMethod]
        public void ValidateEventWhenInnerNodeChangesSelectionState()
        {
            RunOnUIThread.Execute(() =>
            {
                bool selectionChangedRaised = false;
                var data = CreateNestedData(1 /* levels */ , 3 /* groupsAtLevel */, 3 /* countAtLeaf */);
                var selectionModel = new SelectionModel();
                selectionModel.Source = data;
                selectionModel.SelectionChanged += (sender, args) => { selectionChangedRaised = true; };

                selectionModel.Select(1, 0);
                selectionModel.Select(1, 1);
                selectionModel.Select(1, 2);
                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(1, 0),
                        Path(1, 1),
                        Path(1, 2),
                        Path(1)
                    },
                    new List<IndexPath>()
                    {
                        Path(),
                    },
                    1 /* selectedInnerNodes */);

                Log.Comment("Inserting 1.0");
                selectionChangedRaised = false;
                (data[1] as ObservableCollection<object>).Insert(0, 100);
                Verify.IsTrue(selectionChangedRaised, "SelectionChanged event was not raised");
                ValidateSelection(selectionModel,
                   new List<IndexPath>()
                   {
                        Path(1, 1),
                        Path(1, 2),
                        Path(1, 3),
                   },
                   new List<IndexPath>()
                   {
                       Path(),
                       Path(1),
                   });

                Log.Comment("Removing 1.0");
                selectionChangedRaised = false;
                (data[1] as ObservableCollection<object>).RemoveAt(0);
                Verify.IsTrue(selectionChangedRaised, "SelectionChanged event was not raised");
                ValidateSelection(selectionModel,
                   new List<IndexPath>()
                   {
                       Path(1, 0),
                       Path(1, 1),
                       Path(1, 2),
                       Path(1)
                   },
                   new List<IndexPath>()
                   {
                        Path(),
                   },
                   1 /* selectedInnerNodes */);
            });
        }

        [TestMethod]
        public void ValidatePropertyChangedEventIsRaised()
        {
            RunOnUIThread.Execute(() =>
            {
                var selectionModel = new SelectionModel();
                Log.Comment("Set the source to 10 items");
                selectionModel.Source = Enumerable.Range(0, 10).ToList();

                bool selectedIndexChanged = false;
                bool selectedIndicesChanged = false;
                bool SelectedItemChanged = false;
                bool SelectedItemsChanged = false;
                bool AnchorIndexChanged = false;
                selectionModel.PropertyChanged += (sender, args) =>
                {
                    switch (args.PropertyName)
                    {
                        case "SelectedIndex":
                            selectedIndexChanged = true;
                            break;
                        case "SelectedIndices":
                            selectedIndicesChanged = true;
                            break;
                        case "SelectedItem":
                            SelectedItemChanged = true;
                            break;
                        case "SelectedItems":
                            SelectedItemsChanged = true;
                            break;
                        case "AnchorIndex":
                            AnchorIndexChanged = true;
                            break;

                        default:
                            throw new InvalidOperationException();
                    }
                };

                Select(selectionModel, 3, true);

                Verify.IsTrue(selectedIndexChanged);
                Verify.IsTrue(selectedIndicesChanged);
                Verify.IsTrue(SelectedItemChanged);
                Verify.IsTrue(SelectedItemsChanged);
                Verify.IsTrue(AnchorIndexChanged);
            });
        }

        [TestMethod]
        public void CanExtendSelectionModelINPC()
        {
            RunOnUIThread.Execute(() =>
            {
                var selectionModel = new CustomSelectionModel();
                bool intPropertyChanged = false;
                selectionModel.PropertyChanged += (sender, args) =>
                {
                    if (args.PropertyName == "IntProperty")
                    {
                        intPropertyChanged = true;
                    }
                };

                selectionModel.IntProperty = 5;
                Verify.IsTrue(intPropertyChanged);
            });
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // TODO 33512004: SelectionModel - GetCustomProperty("SelectedItem") throws invalid cast to ICustomProperty
        public void CanReadSelectedItemViaICustomPropertyProvider()
        {
            RunOnUIThread.Execute(() =>
            {
                var selectionModel = new SelectionModel();
                Log.Comment("Set the source to 10 items");
                selectionModel.Source = Enumerable.Range(0, 10).ToList();

                selectionModel.Select(3);

                var icpp = (ICustomPropertyProvider)selectionModel;
                var selectedItemProperty = icpp.GetCustomProperty("SelectedItem");
                Verify.IsTrue(selectedItemProperty.CanRead);
                Verify.AreEqual(3, selectedItemProperty.GetValue(selectionModel));
            });
        }

        [TestMethod]
        public void SelectRangeRegressionTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var selectionModel = new SelectionModel() {
                    Source = CreateNestedData(1, 2, 3)
                };

                // length of start smaller than end used to cause an out of range error.
                selectionModel.SelectRange(IndexPath.CreateFrom(0), IndexPath.CreateFrom(1, 1));

                ValidateSelection(selectionModel,
                    new List<IndexPath>()
                    {
                        Path(0, 0),
                        Path(0, 1),
                        Path(0, 2),
                        Path(0),
                        Path(1, 0),
                        Path(1, 1)
                    },
                    new List<IndexPath>()
                    {
                        Path(),
                        Path(1)
                    },
                    1 /* selectedInnerNodes */);

                selectionModel = new SelectionModel() { Source = CreateNestedData(2, 2, 1) };

                selectionModel.SelectRange(
                    Path(1), Path(2));

                ValidateSelection(
                    selectionModel,
                    new List<IndexPath> {
                        Path(1,0,0),
                        Path(1), Path(2),
                        Path(1,0),Path(1,1),
                        Path(2,0),Path(2,1),
                        Path(1,0,1),
                        Path(1,1,0),Path(1,1,1),
                        Path(2,0,0),Path(2,0,1),
                        Path(2,1,0),Path(2,1,1),

                    },
                    new List<IndexPath> { IndexPath.CreateFromIndices(new List<int> { }) },
                    12);

            });
        }

        [TestMethod]
        public void AlreadySelectedDoesNotRaiseEvent()
        {
            var testName = "Select(int32 index), single select";

            RunOnUIThread.Execute(() =>
            {
                var list = Enumerable.Range(0, 10).ToList();

                var selectionModel = new SelectionModel() {
                    Source = list,
                    SingleSelect = true
                };

                // Single select index
                selectionModel.Select(0);
                selectionModel.SelectionChanged += ThrowIfRaisedSelectionChanged;
                selectionModel.Select(0);

                selectionModel = new SelectionModel() {
                    Source = list,
                    SingleSelect = true
                };
                // Single select indexpath
                testName = "SelectAt(IndexPath index), single select";
                selectionModel.SelectAt(IndexPath.CreateFrom(1));
                selectionModel.SelectionChanged += ThrowIfRaisedSelectionChanged;
                selectionModel.SelectAt(IndexPath.CreateFrom(1));

                // multi select index
                selectionModel = new SelectionModel() {
                    Source = list
                };
                selectionModel.Select(1);
                selectionModel.Select(2);
                testName = "Select(int32 index), multiselect";
                selectionModel.SelectionChanged += ThrowIfRaisedSelectionChanged;
                selectionModel.Select(1);
                selectionModel.Select(2);

                selectionModel = new SelectionModel() {
                    Source = list
                };

                // multi select indexpath
                selectionModel.SelectAt(IndexPath.CreateFrom(1));
                selectionModel.SelectAt(IndexPath.CreateFrom(2));
                testName = "SelectAt(IndexPath index), multiselect";
                selectionModel.SelectionChanged += ThrowIfRaisedSelectionChanged;
                selectionModel.SelectAt(IndexPath.CreateFrom(1));
                selectionModel.SelectAt(IndexPath.CreateFrom(2));
            });

            void ThrowIfRaisedSelectionChanged(SelectionModel sender, SelectionModelSelectionChangedEventArgs args)
            {
                throw new Exception("SelectionChangedEvent was raised, but shouldn't have been raised as selection did not change. Tested method: " + testName);
            }
        }

        [TestMethod]
        public void AlreadyDeselectedDoesNotRaiseEvent()
        {
            var testName = "Deselect(int32 index), single select";

            RunOnUIThread.Execute(() =>
            {
                var list = Enumerable.Range(0, 10).ToList();

                var selectionModel = new SelectionModel() {
                    Source = list,
                    SingleSelect = true
                };

                // Single select index
                selectionModel.SelectionChanged += ThrowIfRaisedSelectionChanged;
                selectionModel.Deselect(0);

                selectionModel = new SelectionModel() {
                    Source = list,
                    SingleSelect = true
                };
                // Single select indexpath
                testName = "DeselectAt(IndexPath index), single select";
                selectionModel.SelectionChanged += ThrowIfRaisedSelectionChanged;
                selectionModel.DeselectAt(IndexPath.CreateFrom(1));

                // multi select index
                selectionModel = new SelectionModel() {
                    Source = list
                };
                testName = "Deselect(int32 index), multiselect";
                selectionModel.SelectionChanged += ThrowIfRaisedSelectionChanged;
                selectionModel.Deselect(1);
                selectionModel.Deselect(2);

                selectionModel = new SelectionModel() {
                    Source = list
                };

                // multi select indexpath
                testName = "DeselectAt(IndexPath index), multiselect";
                selectionModel.SelectionChanged += ThrowIfRaisedSelectionChanged;
                selectionModel.DeselectAt(IndexPath.CreateFrom(1));
                selectionModel.DeselectAt(IndexPath.CreateFrom(2));
            });

            void ThrowIfRaisedSelectionChanged(SelectionModel sender, SelectionModelSelectionChangedEventArgs args)
            {
                throw new Exception("SelectionChangedEvent was raised, but shouldn't have been raised as selection did not change. Tested method: " + testName);
            }
        }

        [TestMethod] 
        public void ValidateSelectionModeChangeFromMultipleToSingle()
        {
            RunOnUIThread.Execute(() =>
            {
                SelectionModel selectionModel = new SelectionModel();
                selectionModel.Source = Enumerable.Range(0, 10).ToList();

                // First test: switching from multiple to single selection mode with just one selected item
                selectionModel.Select(4);

                selectionModel.SingleSelect = true;

                // Verify that the item at index 4 is still selected 
                Verify.IsTrue(selectionModel.SelectedIndex.CompareTo(Path(4)) == 0, "Item at index 4 should have still been selected");

                // Second test: this time switching from multiple to single selection mode with more than one selected item
                selectionModel.SingleSelect = false;
                selectionModel.Select(5);
                selectionModel.Select(6);

                // Now switch to single selection mode
                selectionModel.SingleSelect = true;

                // Verify that 
                // - only one item is currently selected
                // - the currently selected item is the item with the lowest index in the Multiple selection list
                Verify.AreEqual(1, selectionModel.SelectedIndices.Count,
                    "Exactly one item should have been selected now after we switched from Multiple to Single selection mode");
                Verify.IsTrue(selectionModel.SelectedIndices[0].CompareTo(selectionModel.SelectedIndex) == 0,
                    "SelectedIndex and SelectedIndices should have been identical");
                Verify.IsTrue(selectionModel.SelectedIndex.CompareTo(Path(4)) == 0, "The currently selected item should have been the first item in the Multiple selection list");
            });
        }

        [TestMethod]
        public void ValidateSelectionModeChangeFromMultipleToSingleSelectionChangedEvent()
        {
            RunOnUIThread.Execute(() =>
            {
                SelectionModel selectionModel = new SelectionModel();
                selectionModel.Source = Enumerable.Range(0, 10).ToList();

                // First test: switching from multiple to single selection mode with just one selected item
                selectionModel.Select(4);

                int selectionChangedFiredCount = 0;
                selectionModel.SelectionChanged += IncreaseCountIfRaisedSelectionChanged;

                // Now switch to single selection mode
                selectionModel.SingleSelect = true;

                // Verify that no SelectionChanged event was raised
                Verify.AreEqual(0, selectionChangedFiredCount, "SelectionChanged event should have not been raised as only one item was selected");

                // Second test: this time switching from multiple to single selection mode with more than one selected item
                selectionModel.SelectionChanged -= IncreaseCountIfRaisedSelectionChanged;
                selectionModel.SingleSelect = false;
                selectionModel.Select(5);
                selectionModel.SelectionChanged += IncreaseCountIfRaisedSelectionChanged;

                // Now switch to single selection mode
                selectionModel.SingleSelect = true;

                // Verify that the SelectionChanged event was raised 
                Verify.AreEqual(1, selectionChangedFiredCount, "SelectionChanged event should have been raised as the selection changed");

                void IncreaseCountIfRaisedSelectionChanged(SelectionModel sender, SelectionModelSelectionChangedEventArgs args)
                {
                    selectionChangedFiredCount++;
                }
            });
        }

        private const int E_FAIL = unchecked((int)0x80004005);

        [TestMethod]
        [TestProperty("Description", "Verifies SelectAll and SelectAllFlat select every item of a flat source and raise SelectionChanged once each.")]
        public void VerifySelectAllOnFlatSource()
        {
            RunOnUIThread.Execute(() =>
            {
                var source = Enumerable.Range(0, 10).ToList();
                var selectionModel = new SelectionModel() { Source = source };
                int selectionChangedCount = 0;
                selectionModel.SelectionChanged += delegate { selectionChangedCount++; };

                selectionModel.SelectAll();
                Verify.AreEqual(1, selectionChangedCount);
                VerifyAllFlatItemsSelected(selectionModel, source);

                selectionModel.ClearSelection();
                Verify.AreEqual(0, selectionModel.SelectedIndices.Count);
                selectionChangedCount = 0;

                selectionModel.SelectAllFlat();
                Verify.AreEqual(1, selectionChangedCount);
                VerifyAllFlatItemsSelected(selectionModel, source);

                Log.Comment("SelectAll on an empty source selects nothing.");
                selectionModel.Source = new List<int>();
                selectionModel.SelectAll();
                Verify.AreEqual(0, selectionModel.SelectedIndices.Count);
                Verify.IsNull(selectionModel.SelectedIndex);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies SelectAll realizes and selects every leaf and group of a nested source.")]
        public void VerifySelectAllOnNestedSource()
        {
            RunOnUIThread.Execute(() =>
            {
                var source = CreateNestedData(levels: 1, groupsAtLevel: 3, countAtLeaf: 4);
                var selectionModel = new SelectionModel() { Source = source };

                selectionModel.SelectAll();

                for (int group = 0; group < 3; group++)
                {
                    Verify.IsTrue(selectionModel.IsSelectedAt(Path(group)).Value, $"Group {group} is selected.");
                    for (int item = 0; item < 4; item++)
                    {
                        Verify.IsTrue(selectionModel.IsSelected(group, item).Value, $"Item {group}.{item} is selected.");
                        Verify.IsTrue(selectionModel.IsSelectedAt(Path(group, item)).Value);
                    }
                }

                var selectedLeaves = selectionModel.SelectedIndices.Where(p => p.GetSize() == 2).ToList();
                Verify.AreEqual(12, selectedLeaves.Count, "All 12 leaves are reported as selected.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies IsSelected(groupIndex, itemIndex) for selected, unselected and never-realized groups.")]
        public void VerifyIsSelectedWithGroupIndex()
        {
            RunOnUIThread.Execute(() =>
            {
                var source = CreateNestedData(levels: 1, groupsAtLevel: 3, countAtLeaf: 4);
                var selectionModel = new SelectionModel() { Source = source };

                selectionModel.Select(1, 2);

                Verify.IsTrue(selectionModel.IsSelected(1, 2).Value);
                Verify.IsFalse(selectionModel.IsSelected(1, 1).Value);
                Verify.IsFalse(selectionModel.IsSelected(1, 3).Value);
                Verify.IsFalse(selectionModel.IsSelected(0, 0).Value);
                Verify.IsFalse(selectionModel.IsSelected(2, 0).Value, "Items of a group that was never realized are not selected.");
                Verify.AreEqual(selectionModel.IsSelectedAt(Path(1, 2)).Value, selectionModel.IsSelected(1, 2).Value);

                selectionModel.Deselect(1, 2);
                Verify.IsFalse(selectionModel.IsSelected(1, 2).Value);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies SingleSelect reports its value and that selecting by group and item index in single selection mode replaces the previous selection.")]
        public void VerifySingleSelectWithGroupIndexReplacesSelection()
        {
            RunOnUIThread.Execute(() =>
            {
                var source = CreateNestedData(levels: 1, groupsAtLevel: 3, countAtLeaf: 4);
                var selectionModel = new SelectionModel() { Source = source };
                Verify.IsFalse(selectionModel.SingleSelect);

                selectionModel.SingleSelect = true;
                Verify.IsTrue(selectionModel.SingleSelect);

                selectionModel.Select(0, 1);
                Verify.IsTrue(selectionModel.IsSelected(0, 1).Value);

                selectionModel.Select(2, 3);
                Verify.IsFalse(selectionModel.IsSelected(0, 1).Value, "The previous selection is cleared.");
                Verify.IsTrue(selectionModel.IsSelected(2, 3).Value);
                Verify.AreEqual(1, selectionModel.SelectedIndices.Count);
                Verify.AreEqual(0, selectionModel.SelectedIndex.CompareTo(Path(2, 3)));

                selectionModel.SingleSelect = false;
                Verify.IsFalse(selectionModel.SingleSelect);
                selectionModel.Select(0, 0);
                Verify.IsTrue(selectionModel.IsSelected(0, 0).Value);
                Verify.IsTrue(selectionModel.IsSelected(2, 3).Value, "Multiple selection keeps the previous selection.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies DeselectRange removes a sub-range from a selected range, accepts a reversed range, and handles a single-item range.")]
        public void VerifyDeselectRange()
        {
            RunOnUIThread.Execute(() =>
            {
                var selectionModel = new SelectionModel() { Source = Enumerable.Range(0, 10).ToList() };
                selectionModel.SelectRange(Path(0), Path(9));
                Verify.AreEqual(10, selectionModel.SelectedIndices.Count);

                int selectionChangedCount = 0;
                selectionModel.SelectionChanged += delegate { selectionChangedCount++; };

                Log.Comment("Deselect the middle of the selected range.");
                selectionModel.DeselectRange(Path(3), Path(5));
                VerifyFlatSelection(selectionModel, 10, new int[] { 0, 1, 2, 6, 7, 8, 9 });
                Verify.AreEqual(1, selectionChangedCount);

                Log.Comment("Deselect with start after end.");
                selectionModel.DeselectRange(Path(8), Path(7));
                VerifyFlatSelection(selectionModel, 10, new int[] { 0, 1, 2, 6, 9 });
                Verify.AreEqual(2, selectionChangedCount);

                Log.Comment("Deselect a single-item range.");
                selectionModel.DeselectRange(Path(0), Path(0));
                VerifyFlatSelection(selectionModel, 10, new int[] { 1, 2, 6, 9 });
                Verify.IsGreaterThan(selectionChangedCount, 2, "SelectionChanged is raised (event count for single-item ranges is covered by VerifySingleItemRangeRaisesSelectionChangedOnce).");
            });
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Product concern PC-SELECTION-DOUBLE-EVENT (bug pending): SelectionModel.SelectRangeImpl raises SelectionChanged twice when start equals end (SelectAt/DeselectAt plus the unconditional OnSelectionChanged).
        [TestProperty("Description", "Verifies SelectRange and DeselectRange with identical start and end raise SelectionChanged exactly once, like a multi-item range.")]
        public void VerifySingleItemRangeRaisesSelectionChangedOnce()
        {
            RunOnUIThread.Execute(() =>
            {
                var selectionModel = new SelectionModel() { Source = Enumerable.Range(0, 10).ToList() };
                int selectionChangedCount = 0;
                selectionModel.SelectionChanged += delegate { selectionChangedCount++; };

                selectionModel.SelectRange(Path(4), Path(4));
                Verify.IsTrue(selectionModel.IsSelected(4).Value);
                Verify.AreEqual(1, selectionChangedCount, "SelectRange(4, 4) raises SelectionChanged once.");

                selectionChangedCount = 0;
                selectionModel.DeselectRange(Path(4), Path(4));
                Verify.IsFalse(selectionModel.IsSelected(4).Value);
                Verify.AreEqual(1, selectionChangedCount, "DeselectRange(4, 4) raises SelectionChanged once.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the ICustomPropertyProvider members of SelectionModel other than the SelectedItem property (see CanReadSelectedItemViaICustomPropertyProvider).")]
        public void VerifyCustomPropertyProviderMembers()
        {
            RunOnUIThread.Execute(() =>
            {
                var selectionModel = new SelectionModel() { Source = Enumerable.Range(0, 3).ToList() };
                var provider = (ICustomPropertyProvider)selectionModel;

                Verify.AreEqual("SelectionModel", provider.GetStringRepresentation());
                Log.Comment($"Type: {provider.Type?.FullName}");
                Verify.IsNotNull(provider.Type);
                Verify.AreEqual(typeof(SelectionModel).FullName, provider.Type.FullName);
                Verify.IsNull(provider.GetCustomProperty("NotAProperty"));
                Verify.IsNull(provider.GetIndexedProperty("SelectedItem", typeof(int)));
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies PropertyChanged handlers stop receiving notifications once removed.")]
        public void VerifyPropertyChangedHandlerRemoval()
        {
            RunOnUIThread.Execute(() =>
            {
                var selectionModel = new SelectionModel() { Source = Enumerable.Range(0, 5).ToList() };
                var changedProperties = new List<string>();
                global::System.ComponentModel.PropertyChangedEventHandler handler = (sender, args) => changedProperties.Add(args.PropertyName);

                selectionModel.PropertyChanged += handler;
                selectionModel.Select(1);
                Log.Comment("Properties changed: " + string.Join(",", changedProperties));
                Verify.IsTrue(changedProperties.Contains("SelectedIndex"));

                changedProperties.Clear();
                selectionModel.PropertyChanged -= handler;
                selectionModel.Select(2);
                Verify.AreEqual(0, changedProperties.Count, "No notification after the handler is removed.");
                Verify.IsTrue(selectionModel.IsSelected(2).Value);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies SelectionModelChildrenRequestedEventArgs Source and SourceIndex are only accessible during the ChildrenRequested handler.")]
        public void VerifyChildrenRequestedArgsOnlyValidDuringHandler()
        {
            RunOnUIThread.Execute(() =>
            {
                var source = CreateNestedData(levels: 1, groupsAtLevel: 2, countAtLeaf: 3);
                var selectionModel = new SelectionModel() { Source = source };
                SelectionModelChildrenRequestedEventArgs captured = null;
                var requestedIndexes = new List<string>();

                selectionModel.ChildrenRequested += (sender, args) =>
                {
                    captured = args;
                    requestedIndexes.Add(args.SourceIndex.ToString());
                    args.Children = args.Source is IList ? args.Source : null;
                };

                selectionModel.Select(1, 2);
                Verify.IsTrue(selectionModel.IsSelected(1, 2).Value);
                Verify.IsNotNull(captured, "ChildrenRequested was raised.");
                Log.Comment("ChildrenRequested for: " + string.Join(",", requestedIndexes));

                Verify.AreEqual(E_FAIL, CaptureHResult(() => { var x = captured.Source; }), "Source outside the handler");
                Verify.AreEqual(E_FAIL, CaptureHResult(() => { var x = captured.SourceIndex; }), "SourceIndex outside the handler");
            });
        }

        private void VerifyAllFlatItemsSelected(SelectionModel selectionModel, List<int> source)
        {
            Verify.AreEqual(source.Count, selectionModel.SelectedIndices.Count);
            Verify.AreEqual(source.Count, selectionModel.SelectedItems.Count);
            for (int i = 0; i < source.Count; i++)
            {
                Verify.IsTrue(selectionModel.IsSelected(i).Value, $"Item {i} is selected.");
                Verify.AreEqual(source[i], selectionModel.SelectedItems[i]);
            }
        }

        private void VerifyFlatSelection(SelectionModel selectionModel, int count, int[] expectedSelected)
        {
            var actual = Enumerable.Range(0, count).Where(i => selectionModel.IsSelected(i).Value).ToArray();
            Log.Comment("Selected: " + string.Join(",", actual));
            Verify.AreEqual(string.Join(",", expectedSelected), string.Join(",", actual));
            Verify.AreEqual(expectedSelected.Length, selectionModel.SelectedIndices.Count);
        }

        private static int CaptureHResult(Action action)
        {
            try
            {
                action();
            }
            catch (Exception e)
            {
                Log.Comment($"Caught {e.GetType().Name}: HResult=0x{e.HResult:X8} '{e.Message}'");
                return e.HResult;
            }
            return 0;
        }

        private void Select(SelectionModel manager, int index, bool select)
        {
            Log.Comment((select ? "Selecting " : "DeSelecting ") + index);
            if (select)
            {
                manager.Select(index);
            }
            else
            {
                manager.Deselect(index);
            }
        }

        private void Select(SelectionModel manager, int groupIndex, int itemIndex, bool select)
        {
            Log.Comment((select ? "Selecting " : "DeSelecting ") + groupIndex + "." + itemIndex);
            if (select)
            {
                manager.Select(groupIndex, itemIndex);
            }
            else
            {
                manager.Deselect(groupIndex, itemIndex);
            }
        }

        private void Select(SelectionModel manager, IndexPath index, bool select)
        {
            Log.Comment((select ? "Selecting " : "DeSelecting ") + index);
            if (select)
            {
                manager.SelectAt(index);
            }
            else
            {
                manager.DeselectAt(index);
            }
        }

        private void SelectRangeFromAnchor(SelectionModel manager, int index, bool select)
        {
            Log.Comment("SelectRangeFromAnchor " + index + " select: " + select.ToString());
            if (select)
            {
                manager.SelectRangeFromAnchor(index);
            }
            else
            {
                manager.DeselectRangeFromAnchor(index);
            }
        }

        private void SelectRangeFromAnchor(SelectionModel manager, int groupIndex, int itemIndex, bool select)
        {
            Log.Comment("SelectRangeFromAnchor " + groupIndex + "." + itemIndex + " select:" + select.ToString());
            if (select)
            {
                manager.SelectRangeFromAnchor(groupIndex, itemIndex);
            }
            else
            {
                manager.DeselectRangeFromAnchor(groupIndex, itemIndex);
            }
        }

        private void SelectRangeFromAnchor(SelectionModel manager, IndexPath index, bool select)
        {
            Log.Comment("SelectRangeFromAnchor " + index + " select: " + select.ToString());
            if (select)
            {
                manager.SelectRangeFromAnchorTo(index);
            }
            else
            {
                manager.DeselectRangeFromAnchorTo(index);
            }
        }

        private void ClearSelection(SelectionModel manager)
        {
            Log.Comment("ClearSelection");
            manager.ClearSelection();
        }

        private void SetAnchorIndex(SelectionModel manager, int index)
        {
            Log.Comment("SetAnchorIndex " + index);
            manager.SetAnchorIndex(index);
        }

        private void SetAnchorIndex(SelectionModel manager, int groupIndex, int itemIndex)
        {
            Log.Comment("SetAnchor " + groupIndex + "." + itemIndex);
            manager.SetAnchorIndex(groupIndex, itemIndex);
        }

        private void SetAnchorIndex(SelectionModel manager, IndexPath index)
        {
            Log.Comment("SetAnchor " + index);
            manager.AnchorIndex = index;
        }

        private void ValidateSelection(
            SelectionModel selectionModel,
            List<IndexPath> expectedSelected,
            List<IndexPath> expectedPartialSelected = null,
            int selectedInnerNodes = 0)
        {
            Log.Comment("Validating Selection...");

            Log.Comment("Selection contains indices:");
            foreach (var index in selectionModel.SelectedIndices)
            {
                Log.Comment(" " + index.ToString());
            }

            Log.Comment("Selection contains items:");
            foreach (var item in selectionModel.SelectedItems)
            {
                Log.Comment(" " + item.ToString());
            }

            if (selectionModel.Source != null)
            {
                List<IndexPath> allIndices = GetIndexPathsInSource(selectionModel.Source);
                foreach (var index in allIndices)
                {
                    bool? isSelected = selectionModel.IsSelectedAt(index);
                    if (Contains(expectedSelected, index))
                    {
                        Verify.IsTrue(isSelected.Value, index + " is Selected");
                    }
                    else if (expectedPartialSelected != null && Contains(expectedPartialSelected, index))
                    {
                        Verify.IsNull(isSelected, index + " is partially Selected");
                    }
                    else
                    {
                        if (isSelected == null)
                        {
                            Log.Comment("*************" + index + " is null");
                            Verify.Fail("Expected false but got null");
                        }
                        else
                        {
                            Verify.IsFalse(isSelected.Value, index + " is not Selected");
                        }
                    }
                }
            }
            else
            {
                foreach (var index in expectedSelected)
                {
                    Verify.IsTrue(selectionModel.IsSelectedAt(index).Value, index + " is Selected");
                }
            }
            if (expectedSelected.Count > 0)
            {
                Log.Comment("SelectedIndex is " + selectionModel.SelectedIndex);
                Verify.AreEqual(0, selectionModel.SelectedIndex.CompareTo(expectedSelected[0]));
                if (selectionModel.Source != null)
                {
                    Verify.AreEqual(selectionModel.SelectedItem, GetData(selectionModel, expectedSelected[0]));
                }

                int itemsCount = selectionModel.SelectedItems.Count();
                Verify.AreEqual(selectionModel.Source != null ? expectedSelected.Count - selectedInnerNodes : 0, itemsCount);
                int indicesCount = selectionModel.SelectedIndices.Count();
                Verify.AreEqual(expectedSelected.Count - selectedInnerNodes, indicesCount);
            }

            Log.Comment("Validating Selection... done");
        }

        private object GetData(SelectionModel selectionModel, IndexPath indexPath)
        {
            var data = selectionModel.Source;
            for (int i = 0; i < indexPath.GetSize(); i++)
            {
                var listData = data as IList;
                data = listData[indexPath.GetAt(i)];
            }

            return data;
        }

        private bool AreEqual(IndexPath a, IndexPath b)
        {
            if (a.GetSize() != b.GetSize())
            {
                return false;
            }

            for (int i = 0; i < a.GetSize(); i++)
            {
                if (a.GetAt(i) != b.GetAt(i))
                {
                    return false;
                }
            }

            return true;
        }

        private List<IndexPath> GetIndexPathsInSource(object source)
        {
            List<IndexPath> paths = new List<IndexPath>();
            Traverse(source, (TreeWalkNodeInfo node) =>
            {
                if (!paths.Contains(node.Path))
                {
                    paths.Add(node.Path);
                }
            });

            Log.Comment("All Paths in source..");
            foreach (var path in paths)
            {
                Log.Comment(path.ToString());
            }
            Log.Comment("done.");

            return paths;
        }

        private static void Traverse(object root, Action<TreeWalkNodeInfo> nodeAction)
        {
            var pendingNodes = new Stack<TreeWalkNodeInfo>();
            IndexPath current = Path(null);
            pendingNodes.Push(new TreeWalkNodeInfo() { Current = root, Path = current });

            while (pendingNodes.Count > 0)
            {
                var currentNode = pendingNodes.Pop();
                var currentObject = currentNode.Current as IList;

                if (currentObject != null)
                {
                    for (int i = currentObject.Count - 1; i >= 0; i--)
                    {
                        var child = currentObject[i];
                        List<int> path = new List<int>();
                        for (int idx = 0; idx < currentNode.Path.GetSize(); idx++)
                        {
                            path.Add(currentNode.Path.GetAt(idx));
                        }

                        path.Add(i);
                        var childPath = IndexPath.CreateFromIndices(path);
                        if (child != null)
                        {
                            pendingNodes.Push(new TreeWalkNodeInfo() { Current = child, Path = childPath });
                        }
                    }
                }

                nodeAction(currentNode);
            }
        }

        private bool Contains(List<IndexPath> list, IndexPath index)
        {
            bool contains = false;
            foreach (var item in list)
            {
                if (item.CompareTo(index) == 0)
                {
                    contains = true;
                    break;
                }
            }

            return contains;
        }

        public static ObservableCollection<object> CreateNestedData(int levels = 3, int groupsAtLevel = 5, int countAtLeaf = 10)
        {
            var data = new ObservableCollection<object>();
            if (levels != 0)
            {
                for (int i = 0; i < groupsAtLevel; i++)
                {
                    data.Add(CreateNestedData(levels - 1, groupsAtLevel, countAtLeaf));
                }
            }
            else
            {
                for (int i = 0; i < countAtLeaf; i++)
                {
                    data.Add(_nextData++);
                }
            }

            return data;
        }

        public static IndexPath Path(params int[] path)
        {
            return IndexPath.CreateFromIndices(path);
        }

        private static int _nextData = 0;
        private struct TreeWalkNodeInfo
        {
            public object Current { get; set; }

            public IndexPath Path { get; set; }
        }
    }

    class CustomSelectionModel : SelectionModel
    {
        public int IntProperty
        {
            get { return _intProperty; }
            set
            {
                _intProperty = value;
                OnPropertyChanged("IntProperty");
            }
        }

        private int _intProperty;
    }
}
