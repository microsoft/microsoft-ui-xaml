// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Buffers;
using System.IO;
using System.Reflection;
using System.Reflection.Metadata;
using System.Reflection.Metadata.Ecma335;
using System.Reflection.PortableExecutable;
using System.Security.Cryptography;
using System.Text;

namespace Microsoft.UI.Xaml.Markup.Compiler.Utilities
{
    internal static class VCMetaManaged
    {
        public static Guid HashForWinMD(string path)
        {
            using (var md5 = MD5.Create())
            using (var pe = new PEReader(File.OpenRead(path)))
            {
                var reader = pe.GetMetadataReader();

                // All types seem to be in the hash
                foreach (var handle in reader.TypeDefinitions)
                {
                    var type = reader.GetTypeDefinition(handle);
                    Hash(md5, (int)type.Attributes);
                    Hash(md5, reader.GetString(type.Name));
                    Hash(md5, reader.GetString(type.Namespace));
                    Hash(md5, MetadataTokens.GetToken(type.BaseType));
                }

                // public & internal fields
                foreach (var handle in reader.FieldDefinitions)
                {
                    var field = reader.GetFieldDefinition(handle);
                    var access = field.Attributes & FieldAttributes.FieldAccessMask;
                    if (access != FieldAttributes.Public && access != FieldAttributes.Family)
                        continue;
                    Hash(md5, (int)field.Attributes);
                    Hash(md5, reader.GetString(field.Name));
                    Hash(md5, reader.GetBlobBytes(field.Signature));
                }

                // public & internal methods
                foreach (var handle in reader.MethodDefinitions)
                {
                    var method = reader.GetMethodDefinition(handle);
                    var access = method.Attributes & MethodAttributes.MemberAccessMask;
                    if (access != MethodAttributes.Public && access != MethodAttributes.Family)
                        continue;
                    Hash(md5, (int)method.ImplAttributes);
                    Hash(md5, (int)method.Attributes);
                    Hash(md5, reader.GetString(method.Name));
                    Hash(md5, reader.GetBlobBytes(method.Signature));
                }

                // interface implementations, are best found by iterating over types
                // I found that System.Reflection.Metadata is missing a InterfaceImplementation.Class API
                // But I can see this column plainly in ILSpy!
                foreach (var typeHandle in reader.TypeDefinitions)
                {
                    var type = reader.GetTypeDefinition(typeHandle);
                    foreach (var interfaceHandle in type.GetInterfaceImplementations())
                    {
                        var implementation = reader.GetInterfaceImplementation(interfaceHandle);
                        Hash(md5, MetadataTokens.GetToken(typeHandle));
                        Hash(md5, MetadataTokens.GetToken(implementation.Interface));
                    }
                }

                // MemberReferences (from original vcmeta.dll)
                foreach (var handle in reader.MemberReferences)
                {
                    var member = reader.GetMemberReference(handle);
                    Hash(md5, reader.GetString(member.Name));
                    Hash(md5, reader.GetBlobBytes(member.Signature));
                }

                // events
                foreach (var handle in reader.EventDefinitions)
                {
                    var @event = reader.GetEventDefinition(handle);
                    Hash(md5, reader.GetString(@event.Name));
                    Hash(md5, MetadataTokens.GetToken(@event.Type));
                }

                // properties
                foreach (var handle in reader.PropertyDefinitions)
                {
                    var property = reader.GetPropertyDefinition(handle);
                    Hash(md5, reader.GetString(property.Name));
                    Hash(md5, reader.GetBlobBytes(property.Signature));
                }

                // TypeSpecifications (from original vcmeta.dll)
                // These are related to generic types
                var table = TableIndex.TypeSpec;
                var offset = reader.GetTableMetadataOffset(table);
                var rowSize = reader.GetTableRowSize(table);
                var rowCount = reader.GetTableRowCount(table);
                for (int i = 0; i < rowCount; i++)
                {
                    var handle = MetadataTokens.TypeSpecificationHandle(i + 1);
                    var type = reader.GetTypeSpecification(handle);
                    Hash(md5, reader.GetBlobBytes(type.Signature));
                }

                md5.TransformFinalBlock(Array.Empty<byte>(), 0, 0);
                return new Guid(md5.Hash);
            }
        }

        private static void Hash(MD5 md5, int value)
        {
            var bytes = BitConverter.GetBytes(value);
            Hash(md5, bytes);
        }

        private static void Hash(MD5 md5, string value)
        {
            var encoding = Encoding.UTF8;
            int length = encoding.GetByteCount(value);
            var bytes = ArrayPool<byte>.Shared.Rent(length);
            try
            {
                encoding.GetBytes(value, 0, value.Length, bytes, 0);
                Hash(md5, bytes, length);
            }
            finally
            {
                ArrayPool<byte>.Shared.Return(bytes);
            }
        }

        private static void Hash(MD5 md5, byte[] bytes) => md5.TransformBlock(bytes, 0, bytes.Length, null, 0);

        private static void Hash(MD5 md5, byte[] bytes, int length) => md5.TransformBlock(bytes, 0, length, null, 0);
    }
}