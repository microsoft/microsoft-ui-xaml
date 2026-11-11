// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Controls
{
    // Tests for the experimental Trigger and MultiTrigger conditional setters.
    [TestClass]
    public class TriggerTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Hosting:Mode", "WPF")]
        [TestProperty("Classification", "Integration")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        private const string TriggerXaml =
            "<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>" +
            "  <Grid.Triggers>" +
            "    <Trigger Property='Tag' Value='on'>" +
            "      <Setter Target='Part.Visibility' Value='Visible' />" +
            "    </Trigger>" +
            "    <MultiTrigger>" +
            "      <MultiTrigger.Conditions>" +
            "        <Condition Property='Tag' Value='on' />" +
            "        <Condition Property='Width' Value='100' />" +
            "      </MultiTrigger.Conditions>" +
            "      <Setter Target='Part.Opacity' Value='0.5' />" +
            "      <Setter Target='Part.Height' Value='20' />" +
            "    </MultiTrigger>" +
            "  </Grid.Triggers>" +
            "  <Border x:Name='Part' Visibility='Collapsed' />" +
            "</Grid>";

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void TriggersParseAndCanEnterTheTree()
        {
            Grid root = null;

            UIExecutor.Execute(() =>
            {
                root = (Grid)XamlReader.Load(TriggerXaml);
                Verify.AreEqual(2, root.Triggers.Count, "Both triggers should be parsed.");

                var trigger = root.Triggers[0] as Trigger;
                Verify.IsNotNull(trigger, "First trigger is a Trigger.");
                Verify.AreEqual("Tag", trigger.Property);
                Verify.AreEqual("on", trigger.Value as string);
                Verify.AreEqual(1, trigger.Setters.Count);

                var multi = root.Triggers[1] as MultiTrigger;
                Verify.IsNotNull(multi, "Second trigger is a MultiTrigger.");
                Verify.AreEqual(2, multi.Conditions.Count);
                Verify.AreEqual("Width", multi.Conditions[1].Property);
                Verify.AreEqual("100", multi.Conditions[1].Value as string);
                Verify.AreEqual(2, multi.Setters.Count);

                TestServices.WindowHelper.WindowContent = root;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(root.IsLoaded, "The element with triggers should be in the live tree.");
            });
        }

        private const string Namespaces =
            "xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'";

        private static T LoadIntoWindow<T>(string xaml) where T : FrameworkElement
        {
            T element = null;
            UIExecutor.Execute(() =>
            {
                element = (T)XamlReader.Load(xaml);
                TestServices.WindowHelper.WindowContent = element;
            });
            TestServices.WindowHelper.WaitForIdle();
            return element;
        }

            private static Border PartAt(Grid root)
            {
                Border part = null;
                UIExecutor.Execute(() => part = (Border)root.Children[0]);
                return part;
            }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void LegacyEventTriggerStillRuns()
        {
            var root = LoadIntoWindow<Grid>(
                "<Grid " + Namespaces + ">" +
                "  <Grid.Triggers>" +
                "    <EventTrigger RoutedEvent='Grid.Loaded'>" +
                "      <BeginStoryboard>" +
                "        <Storyboard>" +
                "          <DoubleAnimation Storyboard.TargetName='Part' Storyboard.TargetProperty='Opacity'" +
                "                           To='0.5' Duration='0:0:0.01' FillBehavior='HoldEnd' />" +
                "        </Storyboard>" +
                "      </BeginStoryboard>" +
                "    </EventTrigger>" +
                "  </Grid.Triggers>" +
                "  <Border x:Name='Part' />" +
                "</Grid>");
            var part = PartAt(root);

            TestServices.WindowHelper.WaitForIdle();
            global::System.Threading.Thread.Sleep(500);
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(0.5, part.Opacity, "The EventTrigger's storyboard should have run.");
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void TriggerAppliesWhileConditionHolds()
        {
            var root = LoadIntoWindow<Grid>(
                "<Grid " + Namespaces + ">" +
                "  <Grid.Triggers>" +
                "    <Trigger Property='IsHitTestVisible' Value='False'>" +
                "      <Setter Target='Part.Opacity' Value='0.25' />" +
                "    </Trigger>" +
                "  </Grid.Triggers>" +
                "  <Border x:Name='Part' />" +
                "</Grid>");
            var part = PartAt(root);

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(1.0, part.Opacity, "The condition does not hold yet.");

                root.IsHitTestVisible = false;
                Verify.AreEqual(0.25, part.Opacity, "The condition holds.");

                root.IsHitTestVisible = true;
                Verify.AreEqual(1.0, part.Opacity, "The original value shows through again.");
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void MultiTriggerRequiresAllConditions()
        {
            var root = LoadIntoWindow<Grid>(
                "<Grid " + Namespaces + " Width='100'>" +
                "  <Grid.Triggers>" +
                "    <MultiTrigger>" +
                "      <MultiTrigger.Conditions>" +
                "        <Condition Property='IsHitTestVisible' Value='False' />" +
                "        <Condition Property='Width' Value='100' />" +
                "      </MultiTrigger.Conditions>" +
                "      <Setter Target='Part.Opacity' Value='0.5' />" +
                "    </MultiTrigger>" +
                "  </Grid.Triggers>" +
                "  <Border x:Name='Part' />" +
                "</Grid>");
            var part = PartAt(root);

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(1.0, part.Opacity, "Neither condition holds.");

                root.IsHitTestVisible = false;
                Verify.AreEqual(0.5, part.Opacity, "Both conditions hold.");

                root.Width = 80;
                Verify.AreEqual(1.0, part.Opacity, "Only one condition holds.");

                root.Width = 100;
                Verify.AreEqual(0.5, part.Opacity, "Both conditions hold again.");
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void LastActiveSetterWinsAndEarlierOneIsRevealed()
        {
            var root = LoadIntoWindow<Grid>(
                "<Grid " + Namespaces + " Width='100'>" +
                "  <Grid.Triggers>" +
                "    <Trigger Property='IsHitTestVisible' Value='False'>" +
                "      <Setter Target='Part.Opacity' Value='0.25' />" +
                "    </Trigger>" +
                "    <Trigger Property='Width' Value='100'>" +
                "      <Setter Target='Part.Opacity' Value='0.75' />" +
                "    </Trigger>" +
                "  </Grid.Triggers>" +
                "  <Border x:Name='Part' />" +
                "</Grid>");
            var part = PartAt(root);

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(0.75, part.Opacity, "Only the second trigger applies.");

                root.IsHitTestVisible = false;
                Verify.AreEqual(0.75, part.Opacity, "Both apply; the last declared wins.");

                root.Width = 80;
                Verify.AreEqual(0.25, part.Opacity, "The earlier trigger shows through when the later one deactivates.");

                root.IsHitTestVisible = true;
                Verify.AreEqual(1.0, part.Opacity, "No trigger applies, so the original value returns.");
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void ConditionHoldingBeforeLoadIsApplied()
        {
            var button = LoadIntoWindow<Button>(
                "<Button " + Namespaces + " IsEnabled='False'>" +
                "  <Button.Template>" +
                "    <ControlTemplate TargetType='Button'>" +
                "      <Grid>" +
                "        <Grid.Triggers>" +
                "          <Trigger Property='IsEnabled' Value='False'>" +
                "            <Setter Target='Part.Opacity' Value='0.5' />" +
                "          </Trigger>" +
                "        </Grid.Triggers>" +
                "        <Border x:Name='Part' />" +
                "      </Grid>" +
                "    </ControlTemplate>" +
                "  </Button.Template>" +
                "</Button>");

            UIExecutor.Execute(() =>
            {
                var part = FindByName(button, "Part");
                Verify.AreEqual(0.5, part.Opacity, "The condition held when the template was created.");

                button.IsEnabled = true;
                Verify.AreEqual(1.0, part.Opacity, "The original value shows through.");
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void StringConditionMatches()
        {
            var root = LoadIntoWindow<Grid>(
                "<Grid " + Namespaces + ">" +
                "  <Grid.Triggers>" +
                "    <Trigger Property='Language' Value='fr-FR'>" +
                "      <Setter Target='Part.Opacity' Value='0.5' />" +
                "    </Trigger>" +
                "  </Grid.Triggers>" +
                "  <Border x:Name='Part' />" +
                "</Grid>");
            var part = PartAt(root);

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(1.0, part.Opacity);

                root.Language = "fr-FR";
                Verify.AreEqual(0.5, part.Opacity, "The string condition holds.");

                root.Language = "de-DE";
                Verify.AreEqual(1.0, part.Opacity, "A different string does not match.");
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void ChangesToUnrelatedPropertiesKeepAppliedValues()
        {
            var root = LoadIntoWindow<Grid>(
                "<Grid " + Namespaces + ">" +
                "  <Grid.Triggers>" +
                "    <Trigger Property='IsHitTestVisible' Value='False'>" +
                "      <Setter Target='Part.Opacity' Value='0.25' />" +
                "    </Trigger>" +
                "  </Grid.Triggers>" +
                "  <Border x:Name='Part' />" +
                "</Grid>");
            var part = PartAt(root);

            UIExecutor.Execute(() =>
            {
                root.IsHitTestVisible = false;
                Verify.AreEqual(0.25, part.Opacity);

                for (int i = 0; i < 50; i++)
                {
                    root.Width = 100 + i;
                    root.Tag = i;
                }
                Verify.AreEqual(0.25, part.Opacity, "Unrelated changes do not disturb the applied value.");

                root.IsHitTestVisible = true;
                Verify.AreEqual(1.0, part.Opacity, "The condition still reacts to its own property.");
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void SetterThatBreaksItsOwnConditionDoesNotHang()
        {
            var root = LoadIntoWindow<Grid>(
                "<Grid " + Namespaces + " x:Name='Root' Width='100'>" +
                "  <Grid.Triggers>" +
                "    <Trigger Property='Width' Value='100'>" +
                "      <Setter Target='Root.Width' Value='50' />" +
                "    </Trigger>" +
                "  </Grid.Triggers>" +
                "</Grid>");

            // Reaching the end of the test is the check: the evaluation passes are bounded.
            UIExecutor.Execute(() =>
            {
                root.Width = 80;
                root.Width = 100;
                Verify.IsTrue(root.Width == 50 || root.Width == 100);
            });
        }

        // The Precedence tests record how trigger setters interact with other value sources today.
        // They log what they observe so the behavior can be quoted; they do not assert a contract.
        private static string Op(FrameworkElement element) => element.Opacity.ToString("0.###");

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void PrecedenceAgainstLocalValue()
        {
            var root = LoadIntoWindow<Grid>(
                "<Grid " + Namespaces + ">" +
                "  <Grid.Triggers>" +
                "    <Trigger Property='IsHitTestVisible' Value='False'>" +
                "      <Setter Target='Part.Opacity' Value='0.25' />" +
                "    </Trigger>" +
                "  </Grid.Triggers>" +
                "  <Border x:Name='Part' Opacity='0.9' />" +
                "</Grid>");
            var part = PartAt(root);

            UIExecutor.Execute(() =>
            {
                Log.Comment("Local value 0.9, trigger inactive: " + Op(part));
                root.IsHitTestVisible = false;
                Log.Comment("Trigger active (setter 0.25): " + Op(part));
                part.Opacity = 0.8;
                Log.Comment("App sets local 0.8 while trigger active: " + Op(part));
                root.IsHitTestVisible = true;
                Log.Comment("Trigger inactive again: " + Op(part));
            });
        }

        private const string TriggerAndVisualStateTemplate =
            "<ContentControl " + Namespaces + " Width='100'>" +
            "  <ContentControl.Template>" +
            "    <ControlTemplate TargetType='ContentControl'>" +
            "      <Grid>" +
            "        <Grid.Triggers>" +
            "          <Trigger Property='Width' Value='100'>" +
            "            <Setter Target='Part.Opacity' Value='0.25' />" +
            "          </Trigger>" +
            "        </Grid.Triggers>" +
            "        <Border x:Name='Part' />" +
            "        <VisualStateManager.VisualStateGroups>" +
            "          <VisualStateGroup x:Name='TestStates'>" +
            "            <VisualState x:Name='Normal' />" +
            "            <VisualState x:Name='Dim'>" +
            "              <VisualState.Setters><Setter Target='Part.Opacity' Value='0.5' /></VisualState.Setters>" +
            "            </VisualState>" +
            "            <VisualState x:Name='Animated'>" +
            "              <Storyboard>" +
            "                <DoubleAnimation Storyboard.TargetName='Part' Storyboard.TargetProperty='Opacity'" +
            "                                 To='0.6' Duration='0:0:0.01' FillBehavior='HoldEnd' />" +
            "              </Storyboard>" +
            "            </VisualState>" +
            "          </VisualStateGroup>" +
            "        </VisualStateManager.VisualStateGroups>" +
            "      </Grid>" +
            "    </ControlTemplate>" +
            "  </ContentControl.Template>" +
            "</ContentControl>";

        private static void GoToState(ContentControl control, string state)
        {
            UIExecutor.Execute(() => VisualStateManager.GoToState(control, state, false));
            TestServices.WindowHelper.WaitForIdle();
            global::System.Threading.Thread.Sleep(300);
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void PrecedenceAgainstVisualStateSetter()
        {
            var control = LoadIntoWindow<ContentControl>(TriggerAndVisualStateTemplate);
            FrameworkElement part = null;
            UIExecutor.Execute(() => part = FindByName(control, "Part"));

            UIExecutor.Execute(() => Log.Comment("Trigger active (0.25), no visual state: " + Op(part)));

            Log.Comment("--- Order 1: trigger active first, then the visual state ---");
            GoToState(control, "Dim");
            UIExecutor.Execute(() => Log.Comment("Visual state Dim (0.5) entered while trigger active: " + Op(part)));
            GoToState(control, "Normal");
            UIExecutor.Execute(() => Log.Comment("Back to Normal (Dim leaves): " + Op(part)));

            Log.Comment("--- Order 2: visual state active first, then the trigger turns on and off ---");
            UIExecutor.Execute(() => control.Width = 80);
            UIExecutor.Execute(() => Log.Comment("Trigger inactive, no visual state: " + Op(part)));
            GoToState(control, "Dim");
            UIExecutor.Execute(() => Log.Comment("Visual state Dim active, trigger inactive: " + Op(part)));
            UIExecutor.Execute(() => control.Width = 100);
            UIExecutor.Execute(() => Log.Comment("Trigger turns on while Dim is active: " + Op(part)));
            UIExecutor.Execute(() => control.Width = 80);
            UIExecutor.Execute(() => Log.Comment("Trigger turns off while Dim is still active: " + Op(part)));
            GoToState(control, "Normal");
            UIExecutor.Execute(() => Log.Comment("Dim leaves: " + Op(part)));
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void PrecedenceAgainstVisualStateStoryboard()
        {
            var control = LoadIntoWindow<ContentControl>(TriggerAndVisualStateTemplate);
            FrameworkElement part = null;
            UIExecutor.Execute(() => part = FindByName(control, "Part"));

            UIExecutor.Execute(() => Log.Comment("Trigger active (0.25), no visual state: " + Op(part)));
            GoToState(control, "Animated");
            UIExecutor.Execute(() => Log.Comment("Storyboard state (0.6) entered while trigger active: " + Op(part)));
            UIExecutor.Execute(() => control.Width = 80);
            UIExecutor.Execute(() => Log.Comment("Trigger turns off while storyboard state is active: " + Op(part)));
            UIExecutor.Execute(() => control.Width = 100);
            UIExecutor.Execute(() => Log.Comment("Trigger turns on again while storyboard state is active: " + Op(part)));
            GoToState(control, "Normal");
            UIExecutor.Execute(() => Log.Comment("Storyboard state leaves: " + Op(part)));
        }

        private static FrameworkElement FindByName(DependencyObject parent, string name)
        {
            int count = Microsoft.UI.Xaml.Media.VisualTreeHelper.GetChildrenCount(parent);
            for (int i = 0; i < count; i++)
            {
                var child = Microsoft.UI.Xaml.Media.VisualTreeHelper.GetChild(parent, i);
                if (child is FrameworkElement element && element.Name == name)
                {
                    return element;
                }

                var found = FindByName(child, name);
                if (found != null)
                {
                    return found;
                }
            }
            return null;
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void TriggersInItemTemplateReactToSelectionAndRowContext()
        {
            const string itemStyle =
                "<Style " + Namespaces + " TargetType='ListViewItem'>" +
                "  <Setter Property='Template'>" +
                "    <Setter.Value>" +
                "      <ControlTemplate TargetType='ListViewItem'>" +
                "        <Grid MinHeight='40'>" +
                "          <Grid.Triggers>" +
                "            <Trigger Property='IsSelected' Value='True'>" +
                "              <Setter Target='Selection.Visibility' Value='Visible' />" +
                "            </Trigger>" +
                "            <MultiTrigger>" +
                "              <MultiTrigger.Conditions>" +
                "                <Condition Property='IsSelected' Value='True' />" +
                "                <Condition Property='RowContext.IsNextSelected' Value='True' />" +
                "              </MultiTrigger.Conditions>" +
                "              <Setter Target='Selection.Opacity' Value='0.5' />" +
                "            </MultiTrigger>" +
                "            <Trigger Property='RowContext.NextRelation' Value='None'>" +
                "              <Setter Target='Separator.Visibility' Value='Collapsed' />" +
                "            </Trigger>" +
                "          </Grid.Triggers>" +
                "          <Border x:Name='Selection' Visibility='Collapsed' />" +
                "          <ContentPresenter Content='{TemplateBinding Content}' />" +
                "          <Border x:Name='Separator' Height='1' VerticalAlignment='Bottom' />" +
                "        </Grid>" +
                "      </ControlTemplate>" +
                "    </Setter.Value>" +
                "  </Setter>" +
                "</Style>";

            ListView list = null;
            UIExecutor.Execute(() =>
            {
                list = new ListView
                {
                    Height = 300,
                    SelectionMode = ListViewSelectionMode.Multiple,
                    ItemContainerStyle = (Style)XamlReader.Load(itemStyle),
                    ItemsSource = new[] { "A", "B", "C", "D" }
                };
                TestServices.WindowHelper.WindowContent = list;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                list.SelectedItems.Add(list.Items[1]);
                list.SelectedItems.Add(list.Items[2]);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                FrameworkElement Part(int index, string name) =>
                    FindByName((DependencyObject)list.ContainerFromIndex(index), name);

                // Row 0 is not selected.
                Verify.AreEqual(Visibility.Collapsed, Part(0, "Selection").Visibility);
                Verify.AreEqual(Visibility.Visible, Part(0, "Separator").Visibility);

                // Rows 1 and 2 are selected: both show the selection; only row 1 has a selected next neighbor.
                Verify.AreEqual(Visibility.Visible, Part(1, "Selection").Visibility);
                Verify.AreEqual(0.5, Part(1, "Selection").Opacity);
                Verify.AreEqual(Visibility.Visible, Part(2, "Selection").Visibility);
                Verify.AreEqual(1.0, Part(2, "Selection").Opacity);

                // The last row has no next neighbor, so its separator is hidden.
                Verify.AreEqual(Visibility.Collapsed, Part(3, "Separator").Visibility);
                Verify.AreEqual(Visibility.Visible, Part(2, "Separator").Visibility);
            });

            UIExecutor.Execute(() =>
            {
                list.SelectedItems.Remove(list.Items[2]);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                FrameworkElement Part(int index, string name) =>
                    FindByName((DependencyObject)list.ContainerFromIndex(index), name);

                Verify.AreEqual(Visibility.Collapsed, Part(2, "Selection").Visibility, "Deselected row hides the selection.");
                Verify.AreEqual(1.0, Part(1, "Selection").Opacity, "Row 1 no longer has a selected next neighbor.");
            });
        }
    }
}
