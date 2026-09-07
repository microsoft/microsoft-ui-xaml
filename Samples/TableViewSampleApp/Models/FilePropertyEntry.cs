// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace TableViewSampleApp.Models;

public class FilePropertyEntry
{
    public FilePropertyEntry(string section, string property, string value = "")
    {
        Section = section;
        Property = property;
        Value = value;
    }

    public string Section { get; set; }

    public string Property { get; set; }

    public string Value { get; set; }
}