// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DumpXBF
{
    using System;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Runtime.InteropServices.ComTypes;

    internal class StreamImpl : IStream, IDisposable
    {
        private bool disposedValue = false; // To detect redundant calls
        protected Stream _underlyingStream;

        public void Close()
        {
            _underlyingStream.Close();
        }

        // ----- IStream Methods -----
        public void Read(byte[] pv, int cb, IntPtr pcbRead)
        {
            if (!_underlyingStream.CanRead)
            {
                throw new InvalidOperationException();
            }
            int cntRead = _underlyingStream.Read(pv, 0, cb);
            Marshal.WriteInt32(pcbRead, cntRead);
        }

        public void Write(byte[] pv, int cb, IntPtr pcbWritten)
        {
            if (!_underlyingStream.CanWrite)
            {
                throw new InvalidOperationException();
            }
            _underlyingStream.Write(pv, 0, cb);
            Marshal.WriteInt32(pcbWritten, cb);
        }

        public void Seek(long dlibMove, int dwOrigin, IntPtr plibNewPosition)
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
            Marshal.WriteInt64(plibNewPosition, newPos);
        }

        public void SetSize(long libNewSize)
        {
            _underlyingStream.SetLength(libNewSize);
        }

        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        #region NonImplemented IStream Methods
        public void CopyTo(IStream pstm, long cb, IntPtr pcbRead, IntPtr pcbWritten) { throw new NotImplementedException(); }
        public void Clone(out IStream ppstm) { throw new NotImplementedException(); }
        public void Commit(int grfCommitFlags) { throw new NotImplementedException(); }
        public void LockRegion(long libOffset, long cb, int dwLockType) { throw new NotImplementedException(); }
        public void Revert() { throw new NotImplementedException(); }
        public void UnlockRegion(long libOffset, long cb, int dwLockType) { throw new NotImplementedException(); }
        public void Stat(out System.Runtime.InteropServices.ComTypes.STATSTG pstatstg, int grfStatFlag) { throw new NotImplementedException(); }
        #endregion

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    _underlyingStream.Dispose();
                    _underlyingStream = null;
                }
                disposedValue = true;
            }
        }
    }
}
