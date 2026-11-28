// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using Microsoft.UI.Windowing;
using Windows.Graphics;
using Windows.Storage;

namespace WindowPlacementApp
{
    // The app's own placement storage for spec example 5 (Setup AppStorage). It stores every field
    // of AppWindowPlacementDetails, separate from WinUI's private record.
    public static class MyStore
    {
        private const string Container = "WindowPlacementApp.MyStore";

        public static void Save(string id, AppWindowPlacementDetails details)
        {
            var value = new ApplicationDataCompositeValue
            {
                ["Dpi"] = details.Dpi,
                ["ShowCmd"] = details.ShowCmd,
                ["Flags"] = Convert.ToInt64(details.Flags),
                ["DeviceName"] = details.DeviceName ?? "",
            };
            PutRect(value, "Normal", details.NormalRect);
            PutRect(value, "Work", details.WorkArea);
            PutRect(value, "Arrange", details.ArrangeRect);
            ApplicationData.Current.LocalSettings.CreateContainer(Container, ApplicationDataCreateDisposition.Always).Values[id] = value;
        }

        public static AppWindowPlacementDetails Load(string id)
        {
            if (!ApplicationData.Current.LocalSettings.Containers.TryGetValue(Container, out ApplicationDataContainer container) ||
                !container.Values.TryGetValue(id, out object stored) ||
                stored is not ApplicationDataCompositeValue value)
            {
                return null;
            }
            return AppWindowPlacementDetails.Create(
                GetRect(value, "Normal"), GetRect(value, "Work"), (int)value["Dpi"], (int)value["ShowCmd"],
                GetRect(value, "Arrange"), (PlacementInfo)Enum.ToObject(typeof(PlacementInfo), (long)value["Flags"]),
                (string)value["DeviceName"]);
        }

        private static void PutRect(ApplicationDataCompositeValue value, string name, RectInt32 r)
        {
            value[name + "X"] = r.X;
            value[name + "Y"] = r.Y;
            value[name + "W"] = r.Width;
            value[name + "H"] = r.Height;
        }

        private static RectInt32 GetRect(ApplicationDataCompositeValue value, string name) =>
            new RectInt32((int)value[name + "X"], (int)value[name + "Y"], (int)value[name + "W"], (int)value[name + "H"]);
    }
}
