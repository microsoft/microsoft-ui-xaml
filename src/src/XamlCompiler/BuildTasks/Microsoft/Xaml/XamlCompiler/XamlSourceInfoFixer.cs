// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using System;

    internal static class XamlSourceInfoFixer
    {
        public static string ReadMarkup(SourcePos start, SourcePos end, string[] xamlLines)
        {
            int startLine = start.Row - 1;
            int endLine = end.Row - 1;

            System.Text.StringBuilder sb = new System.Text.StringBuilder();

            for (int curLine = startLine; curLine <= endLine; curLine++)
            {
                int startCol = 0;
                int endCol = xamlLines[curLine].Length - 1;
                bool needSubstring = false;
                if (curLine == startLine)
                {
                    needSubstring = true;
                    startCol = start.Col - 1;
                }

                if (curLine == endLine)
                {
                    needSubstring = true;
                    endCol = end.Col - 1;
                }

                string appendString = null;
                if (!needSubstring)
                {
                    appendString = xamlLines[curLine];
                }
                else
                {
                    appendString = xamlLines[curLine].Substring(startCol, endCol - startCol + 1);
                }

                sb.AppendLine(appendString);
            }

            return sb.ToString();
        }

        public static FixedSourceInfo GetFixedSourceInfo(ILineNumberAndErrorInfo element, string[] xamlLines)
        {
            var lineNumberInfo = element.LineNumberInfo;

            int startLine = lineNumberInfo.StartLineNumber - 1;
            int startColumn = lineNumberInfo.StartLinePosition - 2; // Take an additional one off the column info to include the opening '<' in what we strip out

            int endLine = lineNumberInfo.EndLineNumber - 1;
            int endColumn = lineNumberInfo.EndLinePosition - 1;

            // HACK: our end source info is inconsistent and differs on whether the object uses an opening and closing tag, or just one self-closing tag.
            // For a self-closing tag, the end source info is actually for the start of the NEXT object/tag.  So we need to determine whether the current
            // object has an opening and closing tag or just a self-closing tag to determine what we need to strip out.

            bool determinedSelfClosing = false;
            bool selfClosing = false;

            FixedSourceInfo retInfo = new FixedSourceInfo();

            // The original source info always has the start line number/position as the first character of the element, which is always immediately next to the opening tag.
            // So subtract 1 from the line position to get the position of the opening start tag
            retInfo.StartOpeningTag.Row = lineNumberInfo.StartLineNumber;
            retInfo.StartOpeningTag.Col = lineNumberInfo.StartLinePosition - 1;

            bool foundStartClosingTag = false;

            for (int curLine = startLine; curLine <= endLine; curLine++)
            {
                int curStartCol = 0;
                int curEndCol = xamlLines[curLine].Length - 1;
                string line = xamlLines[curLine];

                if (curLine == startLine)
                {
                    curStartCol = startColumn;
                }

                if (curLine == endLine)
                {
                    curEndCol = endColumn;
                }

                // Look for the first instance of a '>' character, which indicates the closing of the opening tag
                if (!foundStartClosingTag)
                {
                    int startClosingTagPos = line.IndexOf('>', curStartCol);
                    if (startClosingTagPos != -1)
                    {
                        foundStartClosingTag = true;

                        // Add 1 to account for the different indexing between our 0-indexed array-lookups and the line info
                        retInfo.StartClosingTag.Row = curLine + 1;
                        retInfo.StartClosingTag.Col = startClosingTagPos + 1;
                    }
                }

                if (!determinedSelfClosing)
                {
                    // If we see an opening '<' before we see a '/>', we know the original object didn't have a self-closing tag (since we found the start of a new element or the start of the closing tag)
                    // Also when searching the first start line, the column will be at the original object's opening '<' tag, so we want to increment the column by 1
                    // to avoid finding it

                    int openTagSearchStart = curStartCol;
                    if (curLine == startLine)
                    {
                        openTagSearchStart++;
                    }

                    int foundOpeningTag = line.IndexOf('<', openTagSearchStart);

                    int foundClosingTag = line.IndexOf("/>", StringComparison.OrdinalIgnoreCase);

                    if (foundOpeningTag != -1)
                    {
                        // If we found an opening tag on this line (signaling a new object definition), whether or not the original object was closed depends on
                        // whether the original object had a closing tag before the new opening tag
                        determinedSelfClosing = true;
                        selfClosing = (foundClosingTag != -1 && foundClosingTag < foundOpeningTag);
                        break;
                    }
                    else
                    {
                        if (foundClosingTag != -1)
                        {
                            determinedSelfClosing = true;
                            selfClosing = true;
                            break;
                        }
                    }
                }
            }

            System.Diagnostics.Debug.Assert(determinedSelfClosing);
            retInfo.SelfClosing = selfClosing;

            if (selfClosing)
            {
                retInfo.EndOpeningTag = retInfo.StartOpeningTag;
                retInfo.EndClosingTag = retInfo.StartClosingTag;
            }
            else
            {
                retInfo.EndOpeningTag.Row = lineNumberInfo.EndLineNumber;
                retInfo.EndOpeningTag.Col = lineNumberInfo.EndLinePosition - 2;

                retInfo.EndClosingTag.Row = lineNumberInfo.EndLineNumber;
                retInfo.EndClosingTag.Col = xamlLines[retInfo.EndClosingTag.Row - 1].IndexOf('>', retInfo.EndOpeningTag.Col - 1) + 1;
            }
            return retInfo;
        }
    }

    internal class SourcePos
    {
        public int Row;
        public int Col;
    }

    // Getting more detailed info about how a XamlDomObject is specified in markup.  Row and column start from 1, like the original source info
    internal class FixedSourceInfo
    {
        public SourcePos StartOpeningTag = new SourcePos();
        public SourcePos StartClosingTag = new SourcePos();
        public SourcePos EndOpeningTag = new SourcePos();
        public SourcePos EndClosingTag = new SourcePos();

        // Whether the element has an opening and closing tag, or a self-closing tag.  E.g. "<Apple>hi</Apple>" is not self closing, while "<Apple Prop='hi' />" is self-closing.
        public bool SelfClosing;

        // The element's type given in markup, including its unprocessed namespace prefix.  Used to modify the markup with a property set for the default value.
        public string UnprocessedType;
    }
}
