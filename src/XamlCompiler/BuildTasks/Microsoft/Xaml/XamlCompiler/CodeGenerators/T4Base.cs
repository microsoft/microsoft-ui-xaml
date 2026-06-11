// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler.CodeGen
{
    internal enum Indent
    {
        None = 0,
        OneTab = 1,
        TwoTabs,
        ThreeTabs,
        FourTabs,
        FiveTabs,
        SixTabs,
        SevenTabs,
    }

    internal abstract class T4Base<TModel> : T4Base
    {
        public TModel Model { get; set; }

        public override void SetModel(XamlProjectInfo projectInfo, XamlSchemaCodeInfo schemaInfo, object model)
        {
            Model = (TModel)model;
            ProjectInfo = projectInfo;
            SchemaInfo = schemaInfo;
        }
    }

    internal abstract class T4Base
    {
        public XamlProjectInfo ProjectInfo { get; protected set; }
        public XamlSchemaCodeInfo SchemaInfo { get; protected set; }

        private static string [] Indents =  {
            "",
            "    ",
            "        ",
            "            ",
            "                ",
            "                    ",
            "                        ",
            "                            ",
            "                                ",
            };

        public virtual string TransformText() { return null; }

        /// <summary>
        /// Executes a template and appends the result to the GenerationEnvironment.
        /// </summary>
        /// <typeparam name="TTemplate"></typeparam>
        /// <returns></returns>
        protected string IncludeTemplate<TTemplate>() where TTemplate : T4Base, new()
        {
            return IncludeTemplate<TTemplate>(null);
        }

        /// <summary>
        /// Executes a template and appends the result to the GenerationEnvironment.
        /// </summary>
        /// <typeparam name="TTemplate"></typeparam>
        /// <param name="model"></param>
        /// <param name="args"></param>
        /// <returns></returns>
        protected string IncludeTemplate<TTemplate>(object model, params object[] args) where TTemplate : T4Base, new()
        {
            TTemplate template = new TTemplate();
            if (model != null)
            {
                template.SetModel(this.ProjectInfo, this.SchemaInfo, model);
            }
            template.Arguments = args;
            string result = template.TransformText().Trim('\r', '\n');
            if (!string.IsNullOrEmpty(result))
            {
                PushIndent(GetCurrentIndent());
                WriteLine(result);
                PopIndent();
            }

            // Return nothing... We only use a return type so we can use the <#= #> syntax in the templates.
            return string.Empty;
        }

        #region Fields
        private global::System.Text.StringBuilder generationEnvironmentField;
        private global::System.CodeDom.Compiler.CompilerErrorCollection errorsField;
        private global::System.Collections.Generic.List<int> indentLengthsField;
        private string currentIndentField = "";
        private bool endsWithNewline;
        #endregion
        #region Properties

        public object[] Arguments
        {
            get;
            set;
        }

        public abstract void SetModel(XamlProjectInfo projectInfo, XamlSchemaCodeInfo schemaInfo, object model);

        protected Indent GetCurrentIndent()
        {
            if (GenerationEnvironment.Length > 0)
            {
                const int SpacesInATab = 4;
                int i;
                for (i = GenerationEnvironment.Length - 1; i >= 0 && GenerationEnvironment[i] == ' '; i--) ;
                if (GenerationEnvironment[i] == '\n')
                {
                    return (Indent)(((GenerationEnvironment.Length - i) - 1) / SpacesInATab);
                }
                else if (i == 0)
                {
                    return (Indent)((GenerationEnvironment.Length - i) / SpacesInATab);
                }
            }
            return Indent.None;
        }

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

        #endregion
        #region Transform-time helpers
        /// <summary>
        /// Write text directly into the generated output
        /// </summary>
        public void Write(string textToAppend)
        {
            if (string.IsNullOrEmpty(textToAppend))
            {
                return;
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
        public void PushIndent(Indent tabs = Indent.OneTab)
        {
            string indent = Indents[(int)tabs];
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

        public abstract string ToStringWithCulture(ICodeGenOutput codegenOutput);
        public virtual string ToStringWithCulture(bool value)
        {
            return value ? "true" : "false";
        }
        public abstract string ToStringWithCulture(XamlType type);

        public string ToStringWithCulture(string objectToConvert)
        {
            return objectToConvert.ToString();
        }

        public string ToStringWithCulture(int objectToConvert)
        {
            return objectToConvert.ToString();
        }

        public T4Base ToStringHelper
        {
            get
            {
                return this;
            }
        }
    }
}
