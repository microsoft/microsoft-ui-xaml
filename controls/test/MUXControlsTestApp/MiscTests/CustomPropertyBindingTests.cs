// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Common;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Markup;
using MUXControlsTestApp.Utilities;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WinRT;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class CustomPropertyBindingTests : ApiTestBase
    {
        [TestMethod]
        public void DescriptorCapturingTargetDoesNotLeak()
        {
            var source = new CustomPropertySource();
            WeakReference target = null;
            RunOnUIThread.Execute(() => target = CreateTargetCycle(source));
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Collect();
                Verify.IsTrue(source.Descriptor.IsAlive, "The live target protects its descriptor.");
                Content = null;
            });

            for (int i = 0; i < 10 && (target.IsAlive || source.Descriptor.IsAlive); i++)
            {
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(Collect);
            }

            Verify.IsFalse(target.IsAlive, "The descriptor must not root its binding target.");
            Verify.IsFalse(source.Descriptor.IsAlive, "The descriptor/target cycle must be collectible.");
            GC.KeepAlive(source);
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private WeakReference CreateTargetCycle(CustomPropertySource source)
        {
            var target = new TextBlock();
            source.DescriptorTarget = target;
            target.SetBinding(TextBlock.TextProperty, new Binding
            {
                Source = source,
                Path = new PropertyPath("Value"),
                Mode = BindingMode.OneWay,
            });
            Content = target;
            Verify.AreEqual("first", target.Text);
            Verify.IsNotNull(source.Descriptor);
            source.DescriptorTarget = null;
            return new WeakReference(target);
        }

        [TestMethod]
        public void LiveBindingSurvivesGCAndSameTypeReconnect()
        {
            RunOnUIThread.Execute(() =>
            {
                var first = new CustomPropertySource();
                var source = new CustomPropertySource { Child = first };
                var target = new TextBox();
                target.SetBinding(TextBox.TextProperty, new Binding
                {
                    Source = source,
                    Path = new PropertyPath("Child.Value"),
                    Mode = BindingMode.TwoWay,
                    UpdateSourceTrigger = UpdateSourceTrigger.PropertyChanged,
                });
                Content = target;
                Verify.AreEqual("first", target.Text);

                Collect();
                first.Value = "changed";
                Verify.AreEqual("changed", target.Text);

                var second = new CustomPropertySource { Value = "second" };
                source.Child = second;
                Verify.AreEqual("second", target.Text);
                Collect();
                target.Text = "written";
                Verify.AreEqual("written", second.Value);
                Verify.AreEqual("changed", first.Value);

                first.Value = "old source";
                Verify.AreEqual("written", target.Text, "The previous source must be disconnected.");
            });
        }

        [TestMethod]
        public void NotificationAfterDescriptorCollectionDoesNotCallDeadCCW()
        {
            VerifyCollectedDescriptorNotification(reconnect: false);
        }

        [TestMethod]
        public void ReconnectAfterDescriptorCollectionDoesNotCallDeadCCW()
        {
            VerifyCollectedDescriptorNotification(reconnect: true);
        }

        [TestMethod]
        public void DescriptorLookupCanReconnectBinding()
        {
            VerifyCollectedDescriptorNotification(reconnect: false, reenter: true);
        }

        private void VerifyCollectedDescriptorNotification(bool reconnect, bool reenter = false)
        {
            var child = new CustomPropertySource();
            var source = new CustomPropertySource { Child = child };
            var items = new ObservableCollection<object> { source };
            ItemsControl host = null;

            RunOnUIThread.Execute(() =>
            {
                host = new ItemsControl
                {
                    ItemsSource = items,
                    ItemTemplate = (DataTemplate)XamlReader.Load(
                        "<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>" +
                        "<TextBlock Text='{Binding Child.Value.Length, Mode=OneWay}'/>" +
                        "</DataTemplate>"),
                };
                Content = host;
                host.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNotNull(child.Descriptor, "The binding must use ICustomPropertyProvider.");
                AbandonUniqueWrapper(host);
                Collect();
                Verify.IsTrue(child.Descriptor.IsAlive);

                DiscardItem(host, items);
                Collect();
                Verify.IsFalse(child.Descriptor.IsAlive, "Exercise the collected-descriptor path.");

                if (reconnect)
                {
                    source.Child = new CustomPropertySource { Value = "replacement" };
                }
                else
                {
                    if (reenter)
                    {
                        child.OnGetProperty = () => source.Child = new CustomPropertySource();
                    }
                    child.Value = "updated";
                    if (reenter)
                    {
                        Verify.IsNull(child.OnGetProperty, "The replacement lookup must run.");
                    }
                }
            });

            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => Content = null);
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private static void DiscardItem(ItemsControl host, ObservableCollection<object> items)
        {
            var container = host.ContainerFromIndex(0);
            items.Clear();
            GC.KeepAlive(container);
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private static void AbandonUniqueWrapper(ItemsControl host)
        {
            // Leave a stale tracker-source connection so the native binding outlives its descriptor.
            var wrappers = (ComWrappers)typeof(ComWrappersSupport)
                .GetProperty("ComWrappers", BindingFlags.Static | BindingFlags.NonPublic)
                .GetValue(null);
            var container = host.ContainerFromIndex(0);
            var pointer = MarshalInspectable<object>.FromManaged(container);
            try
            {
                wrappers.GetOrCreateObjectForComInstance(pointer,
                    CreateObjectFlags.TrackerObject | CreateObjectFlags.UniqueInstance);
            }
            finally
            {
                Marshal.Release(pointer);
            }
        }

        private static void Collect()
        {
            GC.Collect(GC.MaxGeneration, GCCollectionMode.Forced, blocking: true, compacting: true);
            GC.WaitForPendingFinalizers();
            GC.Collect(GC.MaxGeneration, GCCollectionMode.Forced, blocking: true, compacting: true);
        }

        private sealed class CustomPropertySource : INotifyPropertyChanged, IBindableCustomPropertyImplementation
        {
            private string value = "first";
            private CustomPropertySource child;

            public event PropertyChangedEventHandler PropertyChanged;
            public FrameworkElement DescriptorTarget { get; set; }
            public WeakReference Descriptor { get; private set; }
            public Action OnGetProperty { get; set; }

            public string Value
            {
                get => value;
                set
                {
                    this.value = value;
                    PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Value)));
                }
            }

            public CustomPropertySource Child
            {
                get => child;
                set
                {
                    child = value;
                    PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Child)));
                }
            }

            public BindableCustomProperty GetProperty(string name)
            {
                if (name == nameof(Child))
                {
                    return new BindableCustomProperty(true, false, name, typeof(CustomPropertySource),
                        target => ((CustomPropertySource)target).Child, null, null, null);
                }
                if (name != nameof(Value))
                {
                    return null;
                }

                var callback = OnGetProperty;
                OnGetProperty = null;
                callback?.Invoke();

                var capturedTarget = DescriptorTarget;
                var property = new BindableCustomProperty(true, true, name, typeof(string),
                    target =>
                    {
                        GC.KeepAlive(capturedTarget);
                        return ((CustomPropertySource)target).Value;
                    },
                    (target, value) => ((CustomPropertySource)target).Value = (string)value,
                    null, null);
                Descriptor = new WeakReference(property);
                return property;
            }

            public BindableCustomProperty GetProperty(Type indexParameterType) => null;
        }
    }
}
