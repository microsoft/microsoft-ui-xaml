// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Text.Json.Serialization;
using Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop;

namespace Microsoft.UI.Xaml.Markup.Compiler.Executable
{
    // Source-generated serialization for the compiler's input and output files, so that no reflection is needed (required
    // by Native AOT). The default options are used on purpose: the files are produced and consumed by InputSerializer and
    // OutputDeserializer, which use the reflection-based serializer with the same (default) options.
    [JsonSerializable(typeof(CompilerInputs))]
    [JsonSerializable(typeof(CompilerOutputs))]
    internal sealed partial class CompilerJsonContext : JsonSerializerContext
    {
    }
}
