// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Text;
    using CodeGen;

    internal class XamlConnectionIdRewriter
    {
        private string[] xamlLines;
        private List<XamlCompileError> errors = new List<XamlCompileError>();

        private static char[] Whitespace = { ' ', '\r', '\n', '\t' };
        private static char[] QuoteCharacters = { '"', '\'' };

        public List<XamlCompileError> Errors
        {
            get { return this.errors; }
        }

        public string Parse(string xamlText, IXamlClassCodeInfo classCodeInfo, IXamlFileCodeInfo fileCodeInfo)
        {
            if (classCodeInfo == null)
            {
                throw new ArgumentNullException("classCodeInfo");
            }

            if (fileCodeInfo == null)
            {
                throw new ArgumentNullException("fileCodeInfo");
            }

            this.xamlLines = XamlConnectionIdRewriter.ReadAllLinesOfString(xamlText);
            return this.ProcessLines(classCodeInfo, fileCodeInfo);
        }

        public string Edit(string xamlFileName, IXamlClassCodeInfo classCodeInfo, IXamlFileCodeInfo fileCodeInfo)
        {
            if (classCodeInfo == null)
            {
                throw new ArgumentNullException("classCodeInfo");
            }

            if (fileCodeInfo == null)
            {
                throw new ArgumentNullException("fileCodeInfo");
            }

            if (classCodeInfo.IsApplication)
            {
                return this.ReadAllTextFromFile(xamlFileName);
            }

            this.xamlLines = this.ReadAllLinesFromFile(xamlFileName);
            return this.ProcessLines(classCodeInfo, fileCodeInfo);
        }

        internal virtual string[] ReadAllLinesFromFile(string xamlFileName)
        {
            try
            {
                return File.ReadAllLines(xamlFileName);
            }
            catch (Exception ex)
            {
                this.Errors.Add(new XamlRewriterErrorFileOpenFailure(xamlFileName, ex.Message));
                return null;
            }
        }

        internal virtual string ReadAllTextFromFile(string xamlFileName)
        {
            try
            {
                return File.ReadAllText(xamlFileName);
            }
            catch (Exception ex)
            {
                this.Errors.Add(new XamlRewriterErrorFileOpenFailure(xamlFileName, ex.Message));
                return null;
            }
        }

        internal static int ConnectionIdElementComparer(ConnectionIdElement a, ConnectionIdElement b)
        {
            if (a.LineNumberInfo.StartLineNumber < b.LineNumberInfo.StartLineNumber)
            {
                return -1;
            }
            else if (a.LineNumberInfo.StartLineNumber > b.LineNumberInfo.StartLineNumber)
            {
                return 1;
            }
            else if (a.LineNumberInfo.StartLinePosition < b.LineNumberInfo.StartLinePosition)
            {
                return -1;
            }
            else if (a.LineNumberInfo.StartLinePosition > b.LineNumberInfo.StartLinePosition)
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }

        private static string[] ReadAllLinesOfString(string xamlText)
        {
            return xamlText.Split(new string[] { "\r\n", "\n" }, StringSplitOptions.None);
        }

        private static string GetSpaces(int start, int end)
        {
            return new string(' ', (end - start + 1));
        }

        private string ProcessLines(IXamlClassCodeInfo classCodeInfo, IXamlFileCodeInfo fileCodeInfo)
        {
            List<ConnectionIdElement> connectionIdElementList = fileCodeInfo.ConnectionIdElements.Where(
                c =>
                c.HasRewritableAttributes)
                .ToList<ConnectionIdElement>();

            foreach (ConnectionIdElement connectionIdElement in connectionIdElementList)
            {
                // Erase the Event Hookups (leaving space where they were)
                foreach (var evt in connectionIdElement.EventAssignments)
                {
                    this.AttributeProcessing(evt);
                }

                // Erase the x:Bind Hookups (leaving space where they were)
                foreach (BindAssignment bind in connectionIdElement.BindAssignments)
                {
                    this.AttributeProcessing(bind, true);
                    bind.ParsePath();
                }

                // Erase the x:Phase Hookup (leaving space where it was)
                if (connectionIdElement.HasPhase)
                {
                    PhaseAssignment phaseAssignment = connectionIdElement.PhaseAssignment;
                    this.AttributeProcessing(phaseAssignment);
                }

                // Erase the x:Bind Event Hookups (leaving space where they were)
                //
                foreach (BoundEventAssignment bndEvt in connectionIdElement.BoundEventAssignments)
                {
                    this.AttributeProcessing(bndEvt, true);
                    bndEvt.ParsePath();
                }
            }

            // Erase the x:DataType Hookups (leaving space where they were)
            foreach (DataTypeAssignment dataTypeAssignment in fileCodeInfo.DataTypeAssignments)
            {
                this.AttributeProcessing(dataTypeAssignment);
            }

            // Erase any other compiler directives GenXBF shouldn't get, like x:DefaultBindMode
            foreach (StrippableMember strippableMember in fileCodeInfo.StrippableMembers)
            {
                this.AttributeProcessing(strippableMember);
            }

            // Erase platform-conditional statements from the namespace
            foreach (StrippableNamespace strippableNamespace in fileCodeInfo.StrippableNamespaces)
            {
                this.AttributeProcessingNamespace(strippableNamespace);
            }

            // Erase any strippable objects, e.g. objects behind a platform conditional that shouldn't be present for the targeted platform
            foreach (StrippableObject strippableObject in fileCodeInfo.StrippableObjects)
            {
                this.StripObject(strippableObject);
            }

            if (Errors.Count > 0)
            {
                return null;
            }

            connectionIdElementList.Sort(XamlConnectionIdRewriter.ConnectionIdElementComparer);
            connectionIdElementList.Reverse();

            // Add the Connection Ids
            foreach (ConnectionIdElement connectionIdElement in connectionIdElementList)
            {
                int startLine = connectionIdElement.LineNumberInfo.StartLineNumber - 1;
                int startColumn = connectionIdElement.LineNumberInfo.StartLinePosition - 1;

                string line = this.xamlLines[startLine];
                int insertionPoint = line.IndexOfAny(XamlConnectionIdRewriter.Whitespace, startColumn);
                string connectionIdString = string.Format(CultureInfo.InvariantCulture, " {0}:ConnectionId='{1}'", "x", connectionIdElement.ConnectionId);
                string newLine = null;
                if (insertionPoint == -1)
                {
                    insertionPoint = line.IndexOf(">");
                    if (insertionPoint != -1 && line[insertionPoint - 1] == '\\')
                    {
                        insertionPoint -= 1;
                    }
                }
                if (insertionPoint != -1)
                {
                    newLine = line.Substring(0, insertionPoint) + connectionIdString + line.Substring(insertionPoint);
                }
                else
                {
                    //Assumption is that the tag has no attributes and is not closed on this line
                    newLine = line + connectionIdString;
                }
                this.xamlLines[startLine] = newLine;
            }

            StringBuilder sb = new StringBuilder();
            foreach (string line in xamlLines)
            {
                sb.AppendLine(line);
            }

            return sb.ToString();
        }

        // Replaces the characters between the start and ending source positions with spaces.  Can span multiple lines
        private void ReplaceWithSpaces(SourcePos start, SourcePos end)
        {
            for (int curLine = start.Row - 1; curLine < end.Row; curLine++)
            {
                int curStartCol = 0;
                int curEndCol = this.xamlLines[curLine].Length - 1;
                string line = this.xamlLines[curLine];

                if (curLine == start.Row - 1)
                {
                    curStartCol = start.Col - 1;
                }

                if (curLine == end.Row - 1)
                {
                    curEndCol = end.Col - 1;
                }

                string spaces = XamlConnectionIdRewriter.GetSpaces(curStartCol, curEndCol);
                string newLine = line.Substring(0, curStartCol) + spaces + line.Substring(curEndCol + 1);
                this.xamlLines[curLine] = newLine;
            }
        }

        private void StripObject(ILineNumberAndErrorInfo element)
        {
            var lineNumberInfo = element.LineNumberInfo;

            FixedSourceInfo srcInfo = XamlSourceInfoFixer.GetFixedSourceInfo(element, this.xamlLines);
            ReplaceWithSpaces(srcInfo.StartOpeningTag, srcInfo.EndClosingTag);
        }

        private void AttributeProcessingNamespace(StrippableNamespace element)
        {
            const string TargetPlatformString = "TargetPlatform";
            int startLine = element.LineNumberInfo.StartLineNumber - 1;
            int startCol = element.LineNumberInfo.StartLinePosition - 1;
            string line = this.xamlLines[startLine];

            int quote1 = -1;
            int quote2 = -1;

            char quoteChar = '"';

            quote1 = line.IndexOfAny(XamlConnectionIdRewriter.QuoteCharacters, startCol);
            if (quote1 != -1)
            {
                quoteChar = line[quote1];
                if (line.Length > quote1)
                {
                    quote2 = line.IndexOf(quoteChar, quote1 + 1);
                }
            }

            if (quote1 == -1 || quote2 == -1)
            {
                // If it spans multiple lines
                this.Errors.Add(new XamlRewriterErrorLineBreak(element.LineNumberInfo.StartLineNumber, element.LineNumberInfo.StartLinePosition));
                return;
            }

            if (element.StripWholeNamespace)
            {
                // Unfortunately, the end line/position information is inaccurate and is always equal to the start line/position.
                // We'll need to figure out the end of the namespace to strip it out by searching for the second quote character.

                string spaces = XamlConnectionIdRewriter.GetSpaces(startCol, quote2);

                string newLine = line.Substring(0, startCol) + spaces + line.Substring(quote2 + 1);
                this.xamlLines[startLine] = newLine;
            }
            else
            {
                // We need to strip out the TargetPlatform declaration in the namespace.  There are several cases to cover:
                // 1. The TargetPlatform declaration is the only modifier to the namespace. (e.g. uri?TargetPlatform(UWP)).  In that case we need to strip out the '?' as well
                // 2. The TargetPlatform declaration is the last modifier to the namespace when there are more than 1 (e.g. uri?RuntimeConditional;TargetPlatform(UWP)).  Then we need to remove the TargetPlatform and previous semicolon
                // 3. The TargetPlatform declaration is not the last modifier to the namespace when there are more than 1 (e.g. uri?TargetPlatform(UWP);RuntimeConditional).  We need to remove TargetPlatform and the semicolon after it

                // Get the namespace, including the quotes surrounding it.
                string nameSpace = line.Substring(quote1, quote2 - quote1 + 1);

                // The beginning and end of the TargetPlatform declaration, which are also used as the indices to strip out.  Will change depending on the position
                // of the TargetPlatform declaration.
                int platformLocStart = nameSpace.IndexOf(TargetPlatformString, StringComparison.OrdinalIgnoreCase);
                int platformLocEnd = nameSpace.IndexOf(')', platformLocStart);

                // Determine the positioning of the TargetPlatform declaration in the namespace.
                bool isLast = nameSpace[platformLocEnd + 1] != ';';

                if (isLast)
                {
                    // Case 1 or 2, move the starting index back to also remove the '?' or ';'
                    platformLocStart--;
                }
                else
                {
                    // Case 3, move the ending index forward 1 to also remove the ';' for the TargetPlatform declaration
                    platformLocEnd++;
                }

                // Now readjust the indices from the substring back to the full line.
                platformLocStart += quote1;
                platformLocEnd += quote1;

                // Strip out the TargetPlatform declaration and append the missing spaces after the namespace.
                string spaces = XamlConnectionIdRewriter.GetSpaces(platformLocStart, platformLocEnd);

                string newLine = line.Substring(0, platformLocStart) + line.Substring(platformLocEnd + 1, quote2 - platformLocEnd) + spaces + line.Substring(quote2 + 1);
                this.xamlLines[startLine] = newLine;
            }
        }

        private void AttributeProcessing(ILineNumberAndErrorInfo element, bool allowMultiline = false)
        {
            var lineNumberInfo = element.LineNumberInfo;
            if (lineNumberInfo.StartLineNumber != lineNumberInfo.EndLineNumber ||
                lineNumberInfo.StartLinePosition != lineNumberInfo.EndLinePosition)
            {
                this.Errors.Add(element.GetAttributeProcessingError());
                return;
            }

            int startLine = lineNumberInfo.StartLineNumber - 1;
            int startColumn = lineNumberInfo.StartLinePosition - 1;

            string line = this.xamlLines[startLine];

            int quote1 = -1;
            int quote2 = -1;
            char quoteChar = 'x';

            quote1 = line.IndexOfAny(XamlConnectionIdRewriter.QuoteCharacters, startColumn);
            if (quote1 != -1)
            {
                quoteChar = line[quote1];
                if (line.Length > quote1)
                {
                    quote2 = line.IndexOf(quoteChar, quote1 + 1);
                }
            }

            // if attribute is all on one line
            if (quote1 != -1 && quote2 != -1)
            {
                string spaces = XamlConnectionIdRewriter.GetSpaces(startColumn, quote2);
                if (element is BoundLoadAssignment)
                {
                    spaces = OverwriteText(spaces, "x:Load=\"False\"");
                }
                string newLine = line.Substring(0, startColumn) + spaces + line.Substring(quote2 + 1);
                this.xamlLines[startLine] = newLine;
            }
            else if (quote1 != -1 && allowMultiline)
            {
                // Fill first line.
                string spaces = XamlConnectionIdRewriter.GetSpaces(startColumn, line.Length - 1);
                if (element is BoundLoadAssignment)
                {
                    spaces = OverwriteText(spaces, "x:Load=\"False\"");
                }
                this.xamlLines[startLine] = line.Substring(0, startColumn) + spaces;

                // Fill subsequent lines.
                for (int i = startLine + 1; i < this.xamlLines.Length; i++)
                {
                    line = this.xamlLines[i];
                    quote2 = line.IndexOf(quoteChar);
                    if (quote2 == -1)
                    {
                        // Space the entire line.
                        spaces = XamlConnectionIdRewriter.GetSpaces(0, line.Length - 1);
                        this.xamlLines[i] = spaces;
                    }
                    else
                    {
                        // Space up to quote2 and exit the loop.
                        spaces = XamlConnectionIdRewriter.GetSpaces(0, quote2);
                        this.xamlLines[i] = spaces + line.Substring(quote2 + 1);
                        break;
                    }
                }
            }
            else
            {
                // If it spans multiple lines
                this.Errors.Add(new XamlRewriterErrorLineBreak(lineNumberInfo.StartLineNumber, lineNumberInfo.StartLinePosition));
            }
        }

        string OverwriteText(string text, string replacement)
        {
            if (text.Length > replacement.Length)
            {
                return replacement + text.Substring(replacement.Length);
            }
            else
            {
                return replacement;
            }
        }
    }
}
