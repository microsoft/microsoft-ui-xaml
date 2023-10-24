// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

namespace XamlGen
{
    class CSVHelper
    {
        internal static List<string[]> Read(string csv, string separator = ",")
        {
            List<string[]> result = new List<string[]>();
            foreach (string line in Regex.Split(csv, System.Environment.NewLine).ToList().Where(s => !string.IsNullOrEmpty(s)))
            {
                string[] values = Regex.Split(line, separator);
                for (int i = 0; i < values.Length; ++i)
                {
                    values[i] = values[i].Trim('\"');
                }
                result.Add(values);
            }
            return result;
        }
    }
}
