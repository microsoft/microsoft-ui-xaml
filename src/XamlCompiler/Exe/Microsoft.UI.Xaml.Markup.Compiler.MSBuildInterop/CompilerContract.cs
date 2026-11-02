// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop
{
    // The version of the JSON contract between the CompileXaml task and the XAML compiler (CompilerInputs, CompilerOutputs,
    // FileOverlay). The types are serialized by the task with DataContractJsonSerializer and by the compiler with the
    // System.Text.Json source generator, so they must only use members both handle the same way with their default settings:
    // public read/write properties of string, bool, int, long, enum (as numbers), arrays, List<T>/IList<T> and these types. No
    // dictionaries, DateTime, init-only or read-only members.
    public static class CompilerContract
    {
        // 0: the inputs of the tasks before design-time support (no version); 1: IsDesignTimeBuild, FileOverlaysPath
        public const int Version = 1;
    }

    // The state of a XAML file open in the IDE, read by the CompileXaml task from the IDE host object of a design-time build
    public sealed class FileOverlay
    {
        // Full path of the XAML file
        public string Path { get; set; }

        // The text of the document in the editor, or null if it is the same as the file on disk
        public string Contents { get; set; }

        // The last change time of the document (a FILETIME, as returned by the host object), or 0 to use the file's
        public long LastChangeFileTime { get; set; }
    }
}
