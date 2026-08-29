// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.CodeGen
{
    public class LanguageSpecificString : ICodeGenOutput
    {
        public CodeGenDelegate CppCXName { get; }
        public CodeGenDelegate CppWinRTName { get; }
        public CodeGenDelegate CSharpName { get; }
        public CodeGenDelegate VBName { get; }

        public LanguageSpecificString(
            CodeGenDelegate cppCX,
            CodeGenDelegate cppWinRT,
            CodeGenDelegate cs,
            CodeGenDelegate vb)
        {
            this.CSharpName = cs;
            this.CppCXName = cppCX;
            this.CppWinRTName = cppWinRT;
            this.VBName = vb;
        }

        public LanguageSpecificString(CodeGenDelegate all)
            : this(all, all, all, all)
        {
        }

        public override int GetHashCode()
        {
            return (CppCXName() + CppWinRTName() + CSharpName() + VBName()).GetHashCode();
        }

        public override bool Equals(object other) 
        {
            if (other is LanguageSpecificString && other != null)
            {
                return this.GetHashCode() == other.GetHashCode();
            }
            return false;
        }

        public static bool operator ==(LanguageSpecificString left, LanguageSpecificString right)
        {
            if (left is null)
            {
                return right is null;
            }
            return left.Equals(right);
        }

        public static bool operator !=(LanguageSpecificString left, LanguageSpecificString right)
        {
            return !(left == right);
        }

        public static LanguageSpecificString Null = new LanguageSpecificString(() => "nullptr", () => "nullptr", () => "null", () => "Nothing");
    }
}
