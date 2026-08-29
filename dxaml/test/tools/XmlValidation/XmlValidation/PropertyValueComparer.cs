// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;

namespace XmlValidation
{
    // Interface for comparing property values read from XML.
    public interface IStringEqualityComparer : IEqualityComparer<string> { }

    public class DefaultStringEqualityComparer : DefaultInstance<DefaultStringEqualityComparer>,
                                                 IStringEqualityComparer
    {
        public bool Equals(string x, string y)
        {
            return string.CompareOrdinal(x, y) == 0;
        }

        public int GetHashCode(string obj)
        {
            return obj.GetHashCode();
        }
    }

    // Parses and compares doubles with optional tolerance.
    public class DoubleEqualityComparer : IStringEqualityComparer
    {
        public DoubleEqualityComparer()
        {
            UseTolerance = false;
        }

        public bool Equals(string x, string y)
        {
            double xAsDouble = Double.Parse(x);
            double yAsDouble = Double.Parse(y);
            return Math.Abs(xAsDouble - yAsDouble) <= ((UseTolerance) ? DefaultTolerance : 0.0);
        }

        public int GetHashCode(string obj)
        {
            return obj.GetHashCode();
        }

        public bool UseTolerance { get; set; }
        public const double DefaultTolerance = 0.1;
    }
}
