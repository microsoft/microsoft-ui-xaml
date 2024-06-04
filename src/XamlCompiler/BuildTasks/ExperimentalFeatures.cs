// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Win32;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    internal sealed class ExperimentalFeatures
    {
        private const string RegistryKey_ExperimentalFeatures = @"SOFTWARE\Microsoft\WinUI\XAML\XamlCompiler";

        private static int GetIntValue(string key, int defaultValue)
        {
            int intValue = defaultValue;
            object value = GetValue(key);
            if (value != null)
            {
                int.TryParse(value.ToString(), out intValue);
            }
            return intValue;
        }

        private static object GetValue(string keyName)
        {
            using (RegistryKey key = Registry.CurrentUser.OpenSubKey(RegistryKey_ExperimentalFeatures, false))
            {
                return key?.GetValue(keyName);
            }
        }
    }
}
