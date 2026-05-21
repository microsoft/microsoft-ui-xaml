// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Linq;
using System.Threading.Tasks;
using WEX.Logging;
using WEX.Logging.Interop;
using WEX.TestExecution;
using Microsoft.UI.Xaml.Tests.Common;
using System.Runtime.InteropServices;

namespace Windows.Graphics.Imaging
{
    public static class ImagingExtensions
    {
        private static string GetTestName(string id)
        {
            string testName = XamlTestsBase.Context.TestName;
            testName = testName.Replace("Microsoft::UI::Xaml::Tests::", "");
            testName = testName.Replace("Microsoft.UI.Xaml.Tests.", "");
            testName = testName.Replace("::", "_");
            testName = testName.Replace(".", "_");
            if (!string.IsNullOrEmpty(id))
            {
                testName += "_" + id;
            }
            const int maxLength = 15;
            if (testName.Length > maxLength)
            {
                testName = testName.Substring(testName.Length - maxLength, maxLength);
            }
            return testName;
        }

        public static Task LogAsync(this SoftwareBitmap @this)
        {
            return @this.LogAsync(string.Empty);
        }

        public static async Task LogAsync(this SoftwareBitmap @this, string Id)
        {
            Verify.IsNotNull(XamlTestsBase.Context);
            Log.Comment($"Capturing {Id}: PixelHeight={@this.PixelHeight}, PixelWidth={@this.PixelWidth}");
            var folder = Windows.Storage.ApplicationData.Current.LocalFolder;
            var name = GetTestName(Id) + ".png";
            var fileName = folder.Path + @"\" + name;
            Log.Comment($"Saving to {fileName}");
            try
            {
                var file = await folder.CreateFileAsync(name, Windows.Storage.CreationCollisionOption.ReplaceExisting);
                using (var stream = await file.OpenAsync(Windows.Storage.FileAccessMode.ReadWrite))
                {
                    var encoder = await BitmapEncoder.CreateAsync(BitmapEncoder.PngEncoderId, stream);
                    encoder.SetSoftwareBitmap(@this);
                    await encoder.FlushAsync();
                }
                Log.File(file.Path);
                Log.Comment($"Done");
            }
            catch(Exception e)
            {
                Log.Comment($"Exception saving file: {e.Message}");
            }
            finally
            {
                // Awaiting DeleteAsync causes an InvalidCastException from WindowHelper to IWindowHelper
                //await file.DeleteAsync(Windows.Storage.StorageDeleteOption.PermanentDelete);
            }
        }
    }
}

