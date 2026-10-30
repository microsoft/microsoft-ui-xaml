// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Native AOT (net10.0) only: exposes the XAML input/XBF output streams to GenXbf.dll as COM IStream objects through
// source-generated COM (System.Runtime.InteropServices.ComTypes.IStream relies on built-in COM interop instead).
// Not implemented members throw NotImplementedException, which is returned to native code as E_NOTIMPL.

namespace Microsoft.UI.Xaml.Markup.Compiler.FileIO
{
    using System;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Runtime.InteropServices.Marshalling;

    // ISequentialStream (objidl.h)
    [GeneratedComInterface]
    [Guid("0c733a30-2a1c-11ce-ade5-00aa0044077d")]
    internal unsafe partial interface ISequentialStreamNative
    {
        void Read(byte* pv, uint cb, uint* pcbRead);

        void Write(byte* pv, uint cb, uint* pcbWritten);
    }

    // IStream (objidl.h). LARGE_INTEGER/ULARGE_INTEGER by value are passed as 64-bit integers.
    [GeneratedComInterface]
    [Guid("0000000c-0000-0000-C000-000000000046")]
    internal unsafe partial interface IStreamNative : ISequentialStreamNative
    {
        void Seek(long dlibMove, uint dwOrigin, ulong* plibNewPosition);

        void SetSize(ulong libNewSize);

        void CopyTo(void* pstm, ulong cb, ulong* pcbRead, ulong* pcbWritten);

        void Commit(uint grfCommitFlags);

        void Revert();

        void LockRegion(ulong libOffset, ulong cb, uint dwLockType);

        void UnlockRegion(ulong libOffset, ulong cb, uint dwLockType);

        void Stat(void* pstatstg, uint grfStatFlag);

        void Clone(void** ppstm);
    }

    [GeneratedComClass]
    internal unsafe partial class StreamImpl : IStreamNative
    {
        void ISequentialStreamNative.Read(byte* pv, uint cb, uint* pcbRead)
        {
            if (!_underlyingStream.CanRead)
            {
                throw new InvalidOperationException();
            }
            int cntRead = _underlyingStream.Read(new Span<byte>(pv, checked((int)cb)));
            if (pcbRead != null)
            {
                *pcbRead = (uint)cntRead;
            }
        }

        void ISequentialStreamNative.Write(byte* pv, uint cb, uint* pcbWritten)
        {
            if (!_underlyingStream.CanWrite)
            {
                throw new InvalidOperationException();
            }
            _underlyingStream.Write(new ReadOnlySpan<byte>(pv, checked((int)cb)));
            if (pcbWritten != null)
            {
                *pcbWritten = cb;
            }
        }

        void IStreamNative.Seek(long dlibMove, uint dwOrigin, ulong* plibNewPosition)
        {
            if (!_underlyingStream.CanSeek)
            {
                throw new InvalidOperationException();
            }
            SeekOrigin origin;
            switch (dwOrigin)
            {
                case 0:
                    origin = SeekOrigin.Begin;
                    break;
                case 1:
                    origin = SeekOrigin.Current;
                    break;
                case 2:
                    origin = SeekOrigin.End;
                    break;
                default:
                    throw new ArgumentException("dwOrigin");
            }
            long newPos = _underlyingStream.Seek(dlibMove, origin);
            if (plibNewPosition != null)
            {
                *plibNewPosition = (ulong)newPos;
            }
        }

        void IStreamNative.SetSize(ulong libNewSize)
        {
            _underlyingStream.SetLength(checked((long)libNewSize));
        }

        void IStreamNative.CopyTo(void* pstm, ulong cb, ulong* pcbRead, ulong* pcbWritten) => throw new NotImplementedException();

        void IStreamNative.Commit(uint grfCommitFlags) => throw new NotImplementedException();

        void IStreamNative.Revert() => throw new NotImplementedException();

        void IStreamNative.LockRegion(ulong libOffset, ulong cb, uint dwLockType) => throw new NotImplementedException();

        void IStreamNative.UnlockRegion(ulong libOffset, ulong cb, uint dwLockType) => throw new NotImplementedException();

        void IStreamNative.Stat(void* pstatstg, uint grfStatFlag) => throw new NotImplementedException();

        void IStreamNative.Clone(void** ppstm) => throw new NotImplementedException();
    }

    // The COM class information is looked up on the exact runtime type, so derived types need to be marked as well
    [GeneratedComClass]
    internal partial class StreamXamlInput
    {
    }

    [GeneratedComClass]
    internal partial class StreamXbfOutput
    {
    }
}
