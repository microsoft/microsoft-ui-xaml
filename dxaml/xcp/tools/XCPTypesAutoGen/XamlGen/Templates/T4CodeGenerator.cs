// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using OM;
using System;
using System.Collections.Generic;
using System.Text;
namespace XamlGen.Templates
{
    /// <summary>
    /// Base class for all T4 code generators.
    /// </summary>
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("Microsoft.VisualStudio.TextTemplating", "11.0.0.0")]
    public abstract class T4CodeGenerator
    {
        public object[] Arguments
        {
            get;
            set;
        }

        public Encoding OutputEncoding
        {
            get;
            set;
        }

        public string OutputPath
        {
            get;
            set;
        }

        protected T4CodeGenerator()
        {
            OutputEncoding = Encoding.Default;
        }

        public abstract void SetModel(object model);

        public abstract string TransformText();

        protected string GetCurrentIndent()
        {
            if (GenerationEnvironment.Length > 0)
            {
                int i;
                for (i = GenerationEnvironment.Length - 1; i >= 0 && GenerationEnvironment[i] == ' '; i--) ;
                if (GenerationEnvironment[i] == '\n')
                {
                    return new string(' ', (GenerationEnvironment.Length - i) - 1);
                }
                else if (i == 0)
                {
                    return new string(' ', GenerationEnvironment.Length - i);
                }
            }
            return string.Empty;
        }

        // Return true if the end of the current string is the start of a block
        // (I.e., trimmed of whitespace and crlf, it ends with a "{", or it's the top of the string.)
        private bool AtStartOfScopeBlock
        {
            get
            {
                // Check for empty string case
                int i;
                if (GenerationEnvironment.Length == 0)
                {
                    return true;
                }

                // Walk back from the end of the current string until we see:
                // non whitespace, the start of the string, or two CRLF
                bool crlfSeen = false;
                for (i = GenerationEnvironment.Length - 1; i >= 0; i--)
                {
                    char character = GenerationEnvironment[i];
                    if (!char.IsWhiteSpace(character))
                    {
                        break;
                    }

                    if (character == '\r')
                    {
                        if (crlfSeen)
                        {
                            // We've seen two blank lines, we're at the start of a block
                            return true;
                        }

                        // This is the first CRLF we've seen in this backwards walk.
                        crlfSeen = true;
                    }
                }

                // Is this either all whitespace, or the last non-whitespace character is an '{'?
                return i >= 0 && GenerationEnvironment[i] == '{';
            }
        }

        #region Fields
        private global::System.Text.StringBuilder generationEnvironmentField;
        private global::System.CodeDom.Compiler.CompilerErrorCollection errorsField;
        private global::System.Collections.Generic.List<int> indentLengthsField;
        private string currentIndentField = "";
        private bool endsWithNewline;
        private global::System.Collections.Generic.IDictionary<string, object> sessionField;
        #endregion
        #region Properties
        /// <summary>
        /// The string builder that generation-time code is using to assemble generated output
        /// </summary>
        protected System.Text.StringBuilder GenerationEnvironment
        {
            get
            {
                if ((this.generationEnvironmentField == null))
                {
                    this.generationEnvironmentField = new global::System.Text.StringBuilder();
                }
                return this.generationEnvironmentField;
            }
            set
            {
                this.generationEnvironmentField = value;
            }
        }
        /// <summary>
        /// The error collection for the generation process
        /// </summary>
        public System.CodeDom.Compiler.CompilerErrorCollection Errors
        {
            get
            {
                if ((this.errorsField == null))
                {
                    this.errorsField = new global::System.CodeDom.Compiler.CompilerErrorCollection();
                }
                return this.errorsField;
            }
        }
        /// <summary>
        /// A list of the lengths of each indent that was added with PushIndent
        /// </summary>
        private System.Collections.Generic.List<int> indentLengths
        {
            get
            {
                if ((this.indentLengthsField == null))
                {
                    this.indentLengthsField = new global::System.Collections.Generic.List<int>();
                }
                return this.indentLengthsField;
            }
        }
        /// <summary>
        /// Gets the current indent we use when adding lines to the output
        /// </summary>
        public string CurrentIndent
        {
            get
            {
                return this.currentIndentField;
            }
        }
        /// <summary>
        /// Current transformation session
        /// </summary>
        public virtual global::System.Collections.Generic.IDictionary<string, object> Session
        {
            get
            {
                return this.sessionField;
            }
            set
            {
                this.sessionField = value;
            }
        }
        #endregion
        #region Transform-time helpers

        // If we're in the middle of a logical chunk of source lines, insert a blank line
        public void EnsureScopeWhitespace()
        {
            // We don't need a blank line at the top of a scope
            if (!AtStartOfScopeBlock)
            {
                this.Write($"{Environment.NewLine}");
            }
        }

        // We're about to start generating a logical group of source code, so insert white
        // space (unless we've already inserted some).
        // The need for this is so that you can call it twice and you'll only get one blank line.
        public void StartLogicalGroupOfLines()
        {
            _logicalGroupStarted = true;
        }
        bool _logicalGroupStarted = false;

        // Write text with a newline, or nothing at all if the text is empty.
        public void WriteWithNewline(string textToAppend)
        {
            if (string.IsNullOrEmpty(textToAppend))
                return;

            this.Write(textToAppend);
            this.Write(Environment.NewLine);
        }

        /// <summary>
        /// Write text directly into the generated output
        /// </summary>
        public virtual void Write(string textToAppend)
        {
            if (string.IsNullOrEmpty(textToAppend))
            {
                return;
            }

            // Ensure there's a blank line if this is the first line in a logical group
            if (_logicalGroupStarted)
            {
                _logicalGroupStarted = false;

                // We don't need to add whitespace before a closing brace
                if (!textToAppend.Trim().StartsWith("}"))
                {
                    this.EnsureScopeWhitespace();
                }
            }

            // If we're starting off, or if the previous text ended with a newline,
            // we have to append the current indent first.
            if (((this.GenerationEnvironment.Length == 0)
                        || this.endsWithNewline))
            {
                this.GenerationEnvironment.Append(this.currentIndentField);
                this.endsWithNewline = false;
            }
            // Check if the current text ends with a newline
            if (textToAppend.EndsWith(global::System.Environment.NewLine, global::System.StringComparison.CurrentCulture))
            {
                this.endsWithNewline = true;
            }
            // This is an optimization. If the current indent is "", then we don't have to do any
            // of the more complex stuff further down.
            if ((this.currentIndentField.Length == 0))
            {
                this.GenerationEnvironment.Append(textToAppend);
                return;
            }
            // Everywhere there is a newline in the text, add an indent after it
            textToAppend = textToAppend.Replace(global::System.Environment.NewLine, (global::System.Environment.NewLine + this.currentIndentField));
            // If the text ends with a newline, then we should strip off the indent added at the very end
            // because the appropriate indent will be added when the next time Write() is called
            if (this.endsWithNewline)
            {
                this.GenerationEnvironment.Append(textToAppend, 0, (textToAppend.Length - this.currentIndentField.Length));
            }
            else
            {
                this.GenerationEnvironment.Append(textToAppend);
            }
        }

        /// <summary>
        /// Write text directly into the generated output
        /// </summary>
        public void WriteLine(string textToAppend)
        {
            this.Write(textToAppend);
            this.GenerationEnvironment.AppendLine();
            this.endsWithNewline = true;
        }
        /// <summary>
        /// Write formatted text directly into the generated output
        /// </summary>
        public void Write(string format, params object[] args)
        {
            this.Write(string.Format(global::System.Globalization.CultureInfo.CurrentCulture, format, args));
        }
        /// <summary>
        /// Write formatted text directly into the generated output
        /// </summary>
        public void WriteLine(string format, params object[] args)
        {
            this.WriteLine(string.Format(global::System.Globalization.CultureInfo.CurrentCulture, format, args));
        }
        /// <summary>
        /// Raise an error
        /// </summary>
        public void Error(string message)
        {
            System.CodeDom.Compiler.CompilerError error = new global::System.CodeDom.Compiler.CompilerError();
            error.ErrorText = message;
            this.Errors.Add(error);
        }
        /// <summary>
        /// Raise a warning
        /// </summary>
        public void Warning(string message)
        {
            System.CodeDom.Compiler.CompilerError error = new global::System.CodeDom.Compiler.CompilerError();
            error.ErrorText = message;
            error.IsWarning = true;
            this.Errors.Add(error);
        }
        /// <summary>
        /// Increase the indent
        /// </summary>
        public void PushIndent(string indent)
        {
            if ((indent == null))
            {
                throw new global::System.ArgumentNullException("indent");
            }
            this.currentIndentField = (this.currentIndentField + indent);
            this.indentLengths.Add(indent.Length);
        }
        /// <summary>
        /// Remove the last indent that was added with PushIndent
        /// </summary>
        public string PopIndent()
        {
            string returnValue = "";
            if ((this.indentLengths.Count > 0))
            {
                int indentLength = this.indentLengths[(this.indentLengths.Count - 1)];
                this.indentLengths.RemoveAt((this.indentLengths.Count - 1));
                if ((indentLength > 0))
                {
                    returnValue = this.currentIndentField.Substring((this.currentIndentField.Length - indentLength));
                    this.currentIndentField = this.currentIndentField.Remove((this.currentIndentField.Length - indentLength));
                }
            }
            return returnValue;
        }
        /// <summary>
        /// Remove any indentation
        /// </summary>
        public void ClearIndent()
        {
            this.indentLengths.Clear();
            this.currentIndentField = "";
        }
        #endregion
        #region ToString Helpers
        /// <summary>
        /// Utility class to produce culture-oriented representation of an object as a string.
        /// </summary>
        public class ToStringInstanceHelper
        {
            private System.IFormatProvider formatProviderField = global::System.Globalization.CultureInfo.InvariantCulture;
            /// <summary>
            /// Gets or sets format provider to be used by ToStringWithCulture method.
            /// </summary>
            public System.IFormatProvider FormatProvider
            {
                get
                {
                    return this.formatProviderField;
                }
                set
                {
                    if ((value != null))
                    {
                        this.formatProviderField = value;
                    }
                }
            }
            /// <summary>
            /// This is called from the compile/run appdomain to convert objects within an expression block to a string
            /// </summary>
            public string ToStringWithCulture(object objectToConvert)
            {
                if ((objectToConvert == null))
                {
                    throw new global::System.ArgumentNullException("objectToConvert");
                }
                System.Type t = objectToConvert.GetType();
                System.Reflection.MethodInfo method = t.GetMethod("ToString", new System.Type[] {
                            typeof(System.IFormatProvider)});
                if ((method == null))
                {
                    return objectToConvert.ToString();
                }
                else
                {
                    return ((string)(method.Invoke(objectToConvert, new object[] {
                                this.formatProviderField })));
                }
            }
        }
        private ToStringInstanceHelper toStringHelperField = new ToStringInstanceHelper();
        /// <summary>
        /// Helper to produce culture-oriented representation of an object as a string
        /// </summary>
        public ToStringInstanceHelper ToStringHelper
        {
            get
            {
                return this.toStringHelperField;
            }
        }
        #endregion

        /// <summary>
        /// Helper for loops in T4 files where the first iteration requires slightly different code.
        /// Example: a loop which generates an if, else-if statement inside a QueryInterface implementation.
        /// </summary>
        protected class HeaderGenerator : IDisposable
        {
            private bool _isFirst = true;

            public bool IsFirst
            {
                get
                {
                    if (_isFirst)
                    {
                        _isFirst = false;
                        return true;
                    }
                    return _isFirst;
                }
            }

            void IDisposable.Dispose()
            {
            }
        }
    }
}
