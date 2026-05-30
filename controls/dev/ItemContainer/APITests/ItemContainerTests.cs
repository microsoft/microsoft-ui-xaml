// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;


namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class ItemContainerTests : ApiTestBase
    {
        [TestMethod]
        public void VerifyDefaultPropertyValues()
        {
            RunOnUIThread.Execute(() =>
            {
                ItemContainer itemContainer = new ItemContainer();
                Verify.IsNotNull(itemContainer);
                Verify.IsNull(itemContainer.Child);
                Verify.IsFalse(itemContainer.IsSelected);

#if MUX_PRERELEASE
                Verify.AreEqual(itemContainer.MultiSelectMode, ItemContainerMultiSelectMode.Auto);
                Log.Comment("ItemContainer MultiSelectMode is Auto by default");
                Verify.AreEqual(itemContainer.CanUserSelect, ItemContainerUserSelectMode.Auto);
                Log.Comment("ItemContainer CanUserSelect is Auto by default");
                Verify.AreEqual(itemContainer.CanUserInvoke, ItemContainerUserInvokeMode.Auto);
                Log.Comment("ItemContainer CanUserInvoke is Auto by default");
#endif
            });
        }

        [TestMethod]
        public void VerifySettableAPIValues()
        {
            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Instantiate and verify default ItemContainer values");
                ItemContainer itemContainer = new ItemContainer();
                Verify.IsNotNull(itemContainer);
                Verify.IsNull(itemContainer.Child);
                Verify.IsFalse(itemContainer.IsSelected);

                itemContainer.IsSelected = true;
                Verify.IsTrue(itemContainer.IsSelected, "Can set ItemContainer IsSelected to true");

                itemContainer.Child = new Grid();
                Verify.IsNotNull(itemContainer.Child, "Can set ItemContainer's Child");

#if MUX_PRERELEASE
                Verify.AreEqual(itemContainer.MultiSelectMode, ItemContainerMultiSelectMode.Auto);
                Log.Comment("ItemContainer MultiSelectMode is Auto by default");
                itemContainer.MultiSelectMode = ItemContainerMultiSelectMode.Single;
                Verify.AreEqual(itemContainer.MultiSelectMode, ItemContainerMultiSelectMode.Single);
                Log.Comment("Can set ItemContainer MultiSelectMode to Single");
                itemContainer.MultiSelectMode = ItemContainerMultiSelectMode.Multiple;
                Verify.AreEqual(itemContainer.MultiSelectMode, ItemContainerMultiSelectMode.Multiple);
                Log.Comment("Can set ItemContainer MultiSelectMode to Multiple");

                Verify.AreEqual(itemContainer.CanUserSelect, ItemContainerUserSelectMode.Auto);
                Log.Comment("ItemContainer CanUserSelect is Auto by default");
                itemContainer.CanUserSelect = ItemContainerUserSelectMode.UserCanSelect;
                Verify.AreEqual(itemContainer.CanUserSelect, ItemContainerUserSelectMode.UserCanSelect);
                Log.Comment("Can set ItemContainer CanUserSelect to UserCanSelect");
                itemContainer.CanUserSelect = ItemContainerUserSelectMode.UserCannotSelect;
                Verify.AreEqual(itemContainer.CanUserSelect, ItemContainerUserSelectMode.UserCannotSelect);
                Log.Comment("Can set ItemContainer CanUserSelect to UserCannotSelect");

                Verify.AreEqual(itemContainer.CanUserInvoke, ItemContainerUserInvokeMode.Auto);
                Log.Comment("ItemContainer CanUserInvoke is Auto by default");
                itemContainer.CanUserInvoke = ItemContainerUserInvokeMode.UserCanInvoke;
                Verify.AreEqual(itemContainer.CanUserInvoke, ItemContainerUserInvokeMode.UserCanInvoke);
                Log.Comment("Can set ItemContainer CanUserInvoke to UserCanInvoke");
                itemContainer.CanUserInvoke = ItemContainerUserInvokeMode.UserCannotInvoke;
                Verify.AreEqual(itemContainer.CanUserInvoke, ItemContainerUserInvokeMode.UserCannotInvoke);
                Log.Comment("Can set ItemContainer CanUserInvoke to UserCannotInvoke");
#endif

                Verify.IsTrue(itemContainer.IsEnabled, "ItemContainer is enabled by default");
                itemContainer.IsEnabled = false;
                Verify.IsFalse(itemContainer.IsEnabled, "Can set ItemContainer IsEnabled to false");
            });
        }

        [TestMethod]
        public void VerifyItemContainerUIASelectionBehavior()
        {
            ISelectionItemProvider selectionItemProvider = null;
            ItemContainer itemContainer = null;

            RunOnUIThread.Execute(() =>
            {
                itemContainer = new ItemContainer();
                Content = itemContainer;
                Content.UpdateLayout();

                var itemContainerPeer = FrameworkElementAutomationPeer.CreatePeerForElement(itemContainer);
                Verify.IsNotNull(itemContainerPeer);
                var itemContainerSelectionPattern = itemContainerPeer.GetPattern(PatternInterface.SelectionItem);
                Verify.IsNotNull(itemContainerSelectionPattern);
                selectionItemProvider = itemContainerSelectionPattern as ISelectionItemProvider;

                Verify.IsNull(selectionItemProvider.SelectionContainer);
                Log.Comment("ItemContainer SelectionContainer is Null if not child of ItemsView");

                // ItemContainer IsSelected is false by default.
                Verify.IsFalse(selectionItemProvider.IsSelected);

#if MUX_PRERELEASE
                Verify.AreEqual(itemContainer.CanUserSelect, ItemContainerUserSelectMode.Auto);
                Log.Comment("ItemContainer CanUserSelect is Auto by default");
                Verify.AreEqual(itemContainer.CanUserInvoke, ItemContainerUserInvokeMode.Auto);
                Log.Comment("ItemContainer CanUserInvoke is Auto by default");
#endif

                // Change selection through automationPeer.
                selectionItemProvider.Select();
                Verify.IsTrue(selectionItemProvider.IsSelected, "ItemContainer should be selected");

                // Setting selection through itemContainer API.
                itemContainer.IsSelected = false;
                Verify.IsFalse(selectionItemProvider.IsSelected, "ItemContainer should not be selected");

#if MUX_PRERELEASE
                itemContainer.CanUserSelect = ItemContainerUserSelectMode.UserCannotSelect;
                Verify.AreEqual(itemContainer.CanUserSelect, ItemContainerUserSelectMode.UserCannotSelect);
                Log.Comment("Set ItemContainer CanUserSelect to UserCannotSelect");
#endif
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
#if MUX_PRERELEASE
                // Try change selection through automationPeer. 
                selectionItemProvider.Select();
                Verify.IsFalse(selectionItemProvider.IsSelected, "ItemContainer should not be selected since UserCannotSelect");
#endif

                // Try setting selection through itemContainer API.
                itemContainer.IsSelected = true;
                Verify.IsTrue(selectionItemProvider.IsSelected, "ItemContainer should be selected since IsSelected bypasses SelectionItemProvider.Select()");
            });
        }

#if MUX_PRERELEASE
        [TestMethod]
        public void VerifyItemContainerUIAInvokeBehavior()
        {
            IInvokeProvider invokeProvider = null;
            ItemContainer itemContainer = null;
            bool itemInvoked = false;

            RunOnUIThread.Execute(() =>
            {
                itemContainer = new ItemContainer();
                Content = itemContainer;

                itemContainer.ItemInvoked += (ItemContainer sender, ItemContainerInvokedEventArgs args) =>
                {
                    itemInvoked = true;
                };

                Content.UpdateLayout();

                var itemContainerPeer = ItemContainerAutomationPeer.CreatePeerForElement(itemContainer);
                Verify.IsNotNull(itemContainerPeer);
                var itemContainerInvokePattern = itemContainerPeer.GetPattern(PatternInterface.Invoke);

                Verify.AreEqual(itemContainer.CanUserSelect, ItemContainerUserSelectMode.Auto);
                Log.Comment("ItemContainer CanUserSelect is Auto by default");
                Verify.AreEqual(itemContainer.CanUserInvoke, ItemContainerUserInvokeMode.Auto);
                Log.Comment("ItemContainer CanUserInvoke is Auto by default");

                Verify.IsNull(itemContainerInvokePattern, "InvokePattern is not implemented since CanUserInvoke != UserCanInvoke");

                Log.Comment("Set ItemContainer CanUserInvoke to UserCanInvoke");
                itemContainer.CanUserInvoke = ItemContainerUserInvokeMode.UserCanInvoke;
                Verify.AreEqual(itemContainer.CanUserInvoke, ItemContainerUserInvokeMode.UserCanInvoke);

                itemContainerInvokePattern = itemContainerPeer.GetPattern(PatternInterface.Invoke);
                invokeProvider = itemContainerInvokePattern as IInvokeProvider;
                Verify.IsNotNull(itemContainerInvokePattern, "InvokePattern is implemented since CanUserInvoke == UserCanInvoke");
                
                // Invoke through automationPeer.
                invokeProvider.Invoke();

                Verify.IsTrue(itemInvoked, "ItemContainer ItemInvoked event should be raised");
            });
        }
#endif
    }
}
