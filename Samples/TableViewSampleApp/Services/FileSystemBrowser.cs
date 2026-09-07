// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.IO;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Services;

public class FileSystemBrowser
{
    public List<FileSystemEntry> GetEntries(string directoryPath)
    {
        var entries = new List<FileSystemEntry>();

        try
        {
            foreach (var directory in Directory.EnumerateDirectories(directoryPath))
            {
                try
                {
                    var info = new DirectoryInfo(directory);
                    entries.Add(new FileSystemEntry
                    {
                        Name = info.Name,
                        IsFolder = true,
                        FullPath = info.FullName,
                        TypeDisplay = "File folder",
                        DateModified = info.LastWriteTime,
                        Size = 0,
                    });
                }
                catch
                {
                }
            }

            foreach (var file in Directory.EnumerateFiles(directoryPath))
            {
                try
                {
                    var info = new FileInfo(file);
                    entries.Add(new FileSystemEntry
                    {
                        Name = info.Name,
                        IsFolder = false,
                        FullPath = info.FullName,
                        TypeDisplay = FileSystemEntry.GetFileType(info.FullName),
                        DateModified = info.LastWriteTime,
                        Size = info.Length,
                    });
                }
                catch
                {
                }
            }
        }
        catch
        {
        }

        return entries;
    }

    public static string GetInitialDirectory()
        => Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
}
