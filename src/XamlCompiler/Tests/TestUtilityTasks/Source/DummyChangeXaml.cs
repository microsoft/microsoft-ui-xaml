// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------------
// Copyright(c) 2014 Microsoft Corporation
//--------------------------------------------------------------------------------------------

namespace XamlCompilerTestsUtilityTasks
{
    using System;
    using System.IO;
    using Microsoft.Build.Framework;
    using Microsoft.Build.Utilities;
    using System.Text;

    public class MakeDummyChangeXaml : Task
    {
        [Required]
        public string XamlFilePath { get; set; }

        [Output]
        public bool ShouldUndoEdit { get; set; }

        public override bool Execute()
        {
            if (!Path.IsPathRooted(this.XamlFilePath)) return false;
            this.ShouldUndoEdit = true;
            Encoding e = null;
            using (StreamReader reader = new StreamReader(this.XamlFilePath))
            {
                e = reader.CurrentEncoding;
            }
            var text = File.ReadAllText(this.XamlFilePath);
            text = text.Replace(Constants.CSharpSimpleMainGridWhiteColor, Constants.CSharpSimpleMainGridBlackColor);
            using (StreamWriter writer = new StreamWriter(this.XamlFilePath, false, e))
            {
                writer.Write(text);
            }
            return true;
        }
    }

    public class UndoDummyChangeXaml : Task
    {
        [Required]
        public string XamlFilePath { get; set; }

        [Required]
        public bool ShouldUndoEdit { get; set; }

        public override bool Execute()
        {
            if (!Path.IsPathRooted(this.XamlFilePath)) return false;
            Encoding e = null;
            using (StreamReader reader = new StreamReader(this.XamlFilePath))
            {
                e = reader.CurrentEncoding;
            }

            var text = File.ReadAllText(this.XamlFilePath);
            if (text.IndexOf(Constants.CSharpSimpleMainGridBlackColor) != -1)
            {
                text = text.Replace(Constants.CSharpSimpleMainGridBlackColor, Constants.CSharpSimpleMainGridWhiteColor);
                using (StreamWriter writer = new StreamWriter(this.XamlFilePath, false, e))
                {
                    writer.Write(text);
                }
            }

            return true;
        }
    }
}
