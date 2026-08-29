// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    internal class FileNameAndChecksumPair
    {
        public string FileName { get; private set; }
        public string Checksum { get; private set; }

        public FileNameAndChecksumPair(string fileName, string contents)
        {
            FileName = fileName;
            Checksum = contents;
        }
    }

    internal class FileNameAndContentPair
    {
        public string FileName { get; private set; }
        public string Contents { get; private set; }

        public FileNameAndContentPair(string fileName, string contents)
        {
            FileName = fileName;
            Contents = contents;
        }
    }

    internal class CodeInfoAndTaskItemPair
    {
        public XamlClassCodeInfo ClassCodeInfo { get; private set; }
        public TaskItemFilename TaskItem { get; private set; }

        public CodeInfoAndTaskItemPair(XamlClassCodeInfo classCodeInfo, TaskItemFilename taskItem)
        {
            ClassCodeInfo = classCodeInfo;
            TaskItem = taskItem;
        }
    }

}
