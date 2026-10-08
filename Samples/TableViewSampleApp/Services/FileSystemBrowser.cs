// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.IO;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Services;

/// <summary>The result of reading one folder: its full path and entries, or why it could not be read.</summary>
public sealed record DirectoryListing(string FullPath, IReadOnlyList<FileSystemEntry> Entries, string? Error);

/// <summary>
/// Reads a folder for the File Explorer page. Pure file-system code with no UI types, so the
/// page can run it on a background thread and marshal only the result.
/// </summary>
public static class FileSystemBrowser
{
    /// <summary>
    /// The folder the page opens in: the sample's own install folder, so the gallery never lists
    /// the user's profile. Browse up from there with Up or the address bar.
    /// </summary>
    public static string InitialDirectory =>
        Path.TrimEndingDirectorySeparator(AppContext.BaseDirectory);

    public static DirectoryListing Read(string directoryPath)
    {
        string fullPath;
        try
        {
            // Keeps the separator of a drive root ("C:\").
            fullPath = Path.TrimEndingDirectorySeparator(Path.GetFullPath(Environment.ExpandEnvironmentVariables(directoryPath)));
        }
        catch (Exception ex) when (ex is ArgumentException or NotSupportedException or PathTooLongException)
        {
            return new DirectoryListing(directoryPath, Array.Empty<FileSystemEntry>(), "not a valid path");
        }

        if (!Directory.Exists(fullPath))
        {
            return new DirectoryListing(fullPath, Array.Empty<FileSystemEntry>(), "folder not found");
        }

        var entries = new List<FileSystemEntry>();
        try
        {
            var folder = new DirectoryInfo(fullPath);
            foreach (var info in folder.EnumerateFileSystemInfos())
            {
                // Skip hidden and system items, as File Explorer does by default.
                if ((info.Attributes & (FileAttributes.Hidden | FileAttributes.System)) != 0)
                {
                    continue;
                }

                var isFolder = info is DirectoryInfo;
                entries.Add(new FileSystemEntry
                {
                    Name = info.Name,
                    IsFolder = isFolder,
                    FullPath = info.FullName,
                    TypeDisplay = isFolder ? FileSystemEntry.FolderType : FileSystemEntry.GetFileType(info.FullName),
                    DateModified = info.LastWriteTime,
                    Size = info is FileInfo file ? file.Length : 0,
                });
            }
        }
        catch (UnauthorizedAccessException)
        {
            return new DirectoryListing(fullPath, Array.Empty<FileSystemEntry>(), "access denied");
        }
        catch (IOException ex)
        {
            return new DirectoryListing(fullPath, Array.Empty<FileSystemEntry>(), ex.Message);
        }

        return new DirectoryListing(fullPath, entries, null);
    }
}
