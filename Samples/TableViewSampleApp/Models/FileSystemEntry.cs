// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Globalization;
using System.IO;

namespace TableViewSampleApp.Models;

public class FileSystemEntry
{
    public string Name { get; set; } = string.Empty;

    public bool IsFolder { get; set; }

    public string FullPath { get; set; } = string.Empty;

    public string TypeDisplay { get; set; } = string.Empty;

    public DateTime DateModified { get; set; }

    public string DateModifiedDisplay => DateModified.ToString("g", CultureInfo.CurrentCulture);

    public long Size { get; set; }

    public string SizeDisplay => IsFolder ? string.Empty : FormatSize(Size);

    public string IconGlyph => IsFolder ? "\uE8B7" : GetFileGlyph(FullPath);

    public static string GetFileType(string path)
    {
        var extension = Path.GetExtension(path).ToLowerInvariant();
        return extension switch
        {
            ".txt" => "Text Document",
            ".rtf" => "Rich Text Document",
            ".doc" or ".docx" => "Microsoft Word Document",
            ".xls" or ".xlsx" => "Microsoft Excel Worksheet",
            ".ppt" or ".pptx" => "Microsoft PowerPoint Presentation",
            ".pdf" => "PDF Document",
            ".xaml" => "XAML File",
            ".xml" => "XML Document",
            ".html" or ".htm" => "Microsoft Edge HTML Document",
            ".json" => "JSON File",
            ".cs" => "C# Source File",
            ".md" => "Markdown Source File",
            ".jpg" or ".jpeg" => "JPEG Image",
            ".png" => "PNG Image",
            ".gif" => "GIF Image",
            ".bmp" => "Bitmap Image",
            ".svg" => "SVG Image",
            ".mp3" or ".wav" or ".wma" or ".aac" or ".flac" => "Audio File",
            ".mp4" or ".mov" or ".avi" or ".mkv" or ".wmv" => "Video File",
            ".zip" => "Compressed (zipped) Folder",
            ".7z" or ".rar" or ".tar" or ".gz" => "Archive File",
            ".exe" => "Application",
            "" => "File",
            _ => $"{extension.TrimStart('.').ToUpperInvariant()} File",
        };
    }

    public static string FormatSize(long bytes)
    {
        if (bytes < 0)
        {
            bytes = 0;
        }

        if (bytes < 1024)
        {
            return string.Format(CultureInfo.CurrentCulture, "{0:N0} bytes", bytes);
        }

        double value = bytes / 1024d;
        if (value < 1024)
        {
            return string.Format(CultureInfo.CurrentCulture, "{0:N0} KB", value);
        }

        value /= 1024d;
        if (value < 1024)
        {
            return string.Format(CultureInfo.CurrentCulture, "{0:N1} MB", value);
        }

        value /= 1024d;
        return string.Format(CultureInfo.CurrentCulture, "{0:N1} GB", value);
    }

    private static string GetFileGlyph(string path)
    {
        var extension = Path.GetExtension(path).ToLowerInvariant();
        return extension switch
        {
            ".txt" or ".doc" or ".docx" or ".pdf" or ".xaml" or ".xml" => "\uE8A5",
            ".xls" or ".xlsx" => "\uE9E9",
            ".jpg" or ".jpeg" or ".png" or ".gif" or ".bmp" or ".svg" => "\uEB9F",
            ".mp3" or ".wav" or ".wma" or ".aac" or ".flac" => "\uE8D6",
            ".mp4" or ".mov" or ".avi" or ".mkv" or ".wmv" => "\uE8B2",
            ".zip" or ".7z" or ".rar" or ".tar" or ".gz" => "\uE8B5",
            ".exe" => "\uE756",
            ".cs" => "\uE943",
            _ => "\uE7C3",
        };
    }
}
