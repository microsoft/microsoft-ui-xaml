// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace TableViewSampleApp.Models;

/// <summary>One row of the File properties page: a named value, the section it is listed under and where it was read from.</summary>
public sealed class FilePropertyEntry
{
    public const string VersionResource = "Version resource";
    public const string FileSystem = "File system";

    public FilePropertyEntry(string section, string property, string value, string source, bool isDate = false)
    {
        Section = section;
        Property = property;
        Value = value;
        Source = source;
        IsDate = isDate;
    }

    // Settable: the page's "Move selected to next section" action rewrites it.
    public string Section { get; set; }

    public string Property { get; }

    public string Value { get; }

    // VersionResource (FileVersionInfo) or FileSystem (FileInfo).
    public string Source { get; }

    public bool IsDate { get; }
}
