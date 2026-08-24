// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Linq;
using System.Collections.Generic;
using System.Reflection;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using BindTestbedModel;

namespace BindTestbed
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    internal sealed partial class DetectLeaksPage : Page
    {
        public DetectLeaksPage()
        {
            this.InitializeComponent();
        }

        private static List<
           KeyValuePair<
               string,
               WeakReference<Object>
               >
           > objects = new List<KeyValuePair<string, WeakReference<Object>>>();

        public static void TrackObject(object obj, string name)
        {
            objects.Add(new KeyValuePair<string, WeakReference<object>>(name, new WeakReference<object>(obj)));
        }

        public static void TrackObject<T>(T obj)
        {
            TrackObject(obj, typeof(T).Name);
        }

        public static void TrackBindingObject<T>(object obj)
        {
            TrackObject(obj, typeof(T).Name + "_Bindings");

            foreach (var nestedType in typeof(T).GetNestedTypes(BindingFlags.NonPublic))
            {
                var bindingsTrackingField = nestedType.GetField("bindingsTracking", BindingFlags.NonPublic | BindingFlags.Instance);
                if (bindingsTrackingField != null)
                {
                    object bindingsTracking = null;
                    try
                    {
                        bindingsTracking = bindingsTrackingField.GetValue(obj);
                    }
                    catch (ArgumentException)
                    {
                        // it's expected to get AE in some cases, just let it go (F5)
                        continue;
                    }
                    TrackObject(bindingsTracking, typeof(T).Name + "_BindingTracking");
                    return;
                }
            }
            throw new ArgumentException("Can't find bindings class");
        }

        public static IEnumerable<string> GetLeakedNames()
        {
            return objects.Where(pair =>
            {
                object obj;
                pair.Value.TryGetTarget(out obj);
                return obj != null;
            }).Select(p => p.Key);
        }

        private void DetectLeakedObjects_Click(object sender, RoutedEventArgs e)
        {
            var leakedObjects = GetLeakedNames();
            if (leakedObjects.Count() > 0)
            {
                leakedObjectNames.Text = String.Join(", ", leakedObjects);
            }
            else
            {
                leakedObjectNames.Text = "No leaks";
            }
            DetectLeakedObjectsButton.IsEnabled = leakedObjects.Count() != 0;
        }

        private void BackButton_Click(object sender, RoutedEventArgs e)
        {
            Frame.Navigate(typeof(MainPage));
        }

        private void GC_Click(object sender, RoutedEventArgs e)
        {
            GC.Collect();
            GC.WaitForPendingFinalizers();
        }

        private void UpdateValues(object sender, RoutedEventArgs e)
        {
            App.Model.UpdateValues();
            App.DOModel.UpdateValues();
            App.LanguageModel.UpdateValues();
            App.ModelCX.UpdateValues();
        }
    }
}
