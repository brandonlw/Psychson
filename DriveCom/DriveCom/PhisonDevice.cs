using Microsoft.Win32.SafeHandles;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace DriveCom
{
    public class PhisonDevice
    {
        private char _driveLetter;
        private SafeFileHandle _handle;

        public enum RunMode
        {
            Unknown,
            BootMode,
            Burner,
            HardwareVerify,
            Firmware
        }

        [Flags]
        public enum EFileAttributes : uint
        {
            Readonly = 0x00000001,
            Hidden = 0x00000002,
            System = 0x00000004,
            Directory = 0x00000010,
            Archive = 0x00000020,
            Device = 0x00000040,
            Normal = 0x00000080,
            Temporary = 0x00000100,
            SparseFile = 0x00000200,
            ReparsePoint = 0x00000400,
            Compressed = 0x00000800,
            Offline = 0x00001000,
            NotContentIndexed = 0x00002000,
            Encrypted = 0x00004000,
            Write_Through = 0x80000000,
            Overlapped = 0x40000000,
            NoBuffering = 0x20000000,
            RandomAccess = 0x10000000,
            SequentialScan = 0x08000000,
            DeleteOnClose = 0x04000000,
            BackupSemantics = 0x02000000,
            PosixSemantics = 0x01000000,
            OpenReparsePoint = 0x00200000,
            OpenNoRecall = 0x00100000,
            FirstPipeInstance = 0x00080000
        }

        [DllImport("Kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        public static extern SafeFileHandle CreateFile(
          string fileName,
          [MarshalAs(UnmanagedType.U4)] FileAccess fileAccess,
          [MarshalAs(UnmanagedType.U4)] FileShare fileShare,
          IntPtr securityAttributes,
          [MarshalAs(UnmanagedType.U4)] FileMode creationDisposition,
          [MarshalAs(UnmanagedType.U4)] EFileAttributes flags,
          IntPtr template);

        [DllImport("kernel32.dll")]
        static public extern int CloseHandle(SafeFileHandle hObject);

        public const byte SCSI_IOCTL_DATA_OUT = 0;
        public const byte SCSI_IOCTL_DATA_IN = 1;

        [StructLayout(LayoutKind.Sequential)]
        class SCSI_PASS_THROUGH_DIRECT
        {
            private const int _CDB_LENGTH = 16;

            public short Length;
            public byte ScsiStatus;
            public byte PathId;
            public byte TargetId;
            public byte Lun;
            public byte CdbLength;
            public byte SenseInfoLength;
            public byte DataIn;
            public int DataTransferLength;
            public int TimeOutValue;
            public IntPtr DataBuffer;
            public uint SenseInfoOffset;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = _CDB_LENGTH)]
            public byte[] Cdb;

            public SCSI_PASS_THROUGH_DIRECT()
            {
                Cdb = new byte[_CDB_LENGTH];
            }
        };

        [StructLayout(LayoutKind.Sequential)]
        class SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER
        {
            private const int _SENSE_LENGTH = 32;
            internal SCSI_PASS_THROUGH_DIRECT sptd = new SCSI_PASS_THROUGH_DIRECT();

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = _SENSE_LENGTH)]
            internal byte[] sense;

            public SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER()
            {
                sense = new byte[_SENSE_LENGTH];
            }
        };

        [DllImport("kernel32.dll", ExactSpelling = true, SetLastError = true, CharSet = CharSet.Auto)]
        static extern bool DeviceIoControl(IntPtr hDevice, uint dwIoControlCode,
            IntPtr lpInBuffer, uint nInBufferSize,
            IntPtr lpOutBuffer, uint nOutBufferSize,
            out uint lpBytesReturned, IntPtr lpOverlapped);

        /// <summary>
        /// Creates a reference to a device with a Phison USB controller.
        /// </summary>
        /// <param name="driveLetter">The Windows drive letter representing the device.</param>
        public PhisonDevice(char driveLetter)
        {
            _driveLetter = driveLetter;
        }

        /// <summary>
        /// Opens a connection to the device.
        /// </summary>
        public void Open()
        {
            _handle = CreateFile(string.Format("\\\\.\\{0}:", _driveLetter), FileAccess.ReadWrite, FileShare.ReadWrite,
                IntPtr.Zero, FileMode.Open, EFileAttributes.NoBuffering, IntPtr.Zero);
        }

        public byte[] RequestVendorInfo()
        {
            var data = SendCommand(new byte[] { 0x06, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 },
                512 + 16);
            byte[] ret = null;

            if (data != null)
            {
                ret = data.Take(512).ToArray();
            }

            return ret;
        }

        public string GetChipID()
        {
            var response = SendCommand(new byte[] { 0x06, 0x56, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 512);

            return BitConverter.ToString(response, 0, 6);
        }

        public string GetFirmwareVersion()
        {
            var info = RequestVendorInfo();

            return info[0x94] + "." + info[0x95].ToString("X02") + "." + info[0x96].ToString("X02");
        }

        public ushort? GetChipType()
        {
            ushort? ret = null;
            var info = RequestVendorInfo();

            if (info != null)
            {
                if (info[0x17A] == (byte)'V' && info[0x17B] == (byte)'R')
                {
                    var data = info.Skip(0x17E).Take(2).ToArray();
                    ret = (ushort)((data[0] << 8) | data[1]);
                }
            }

            return ret;
        }

        public RunMode GetRunMode()
        {
            var ret = RunMode.Unknown;
            var info = RequestVendorInfo();

            if (info != null)
            {
                if (info[0x17A] == (byte)'V' && info[0x17B] == (byte)'R')
                {
                    //TODO: Fix this, this is a dumb way of detecting it
                    switch (ASCIIEncoding.ASCII.GetString(info.Skip(0xA0).Take(8).ToArray()))
                    {
                        case " PRAM   ":
                            ret = RunMode.BootMode;
                            break;
                        case " FW BURN":
                            ret = RunMode.Burner;
                            break;
                        case " HV TEST":
                            ret = RunMode.HardwareVerify;
                            break;
                        default:
                            ret = RunMode.Firmware;
                            break;
                    }
                }
            }

            return ret;
        }

        public ulong GetNumLBAs()
        {
            var response = SendCommand(new byte[] { 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 8);
            ulong ret = response[3];
            ret |= (ulong)((ulong)(response[2] << 8) & 0x0000FF00);
            ret |= (ulong)((ulong)(response[1] << 16) & 0x00FF0000);
            ret |= (ulong)((ulong)(response[0] << 24) & 0xFF000000);

            return ret + 1;
        }

        public void JumpToPRAM()
        {
            SendCommand(new byte[] { 0x06, 0xB3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
        }

        public void JumpToBootMode()
        {
            SendCommand(new byte[] { 0x06, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
        }

        public void TransferFile(byte[] data)
        {
            TransferFile(data, 0x03, 0x02);
        }

        public void TransferFile(byte[] data, byte header, byte body)
        {
            var size = data.Length - 1024;

            //Send header
            SendCommand(new byte[] { 0x06, 0xB1, header, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, data.Take(0x200).ToArray());
            
            //Get response
            var response = SendCommand(new byte[] { 0x06, 0xB0, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 8);
            if (response == null || response[0] != 0x55)
            {
                throw new InvalidOperationException("Header not accepted");
            }

            //Send body
            int address = 0;
            while (size > 0)
            {
                int chunkSize;
                if (size > 0x8000)
                {
                    chunkSize = 0x8000;
                }
                else
                {
                    chunkSize = size;
                }

                int cmdAddress = address >> 9;
                int cmdChunk = chunkSize >> 9;
                SendCommand(new byte[] { 0x06, 0xB1, body, (byte)((cmdAddress >> 8) & 0xFF), (byte)(cmdAddress & 0xFF),
                    0x00, 0x00, (byte)((cmdChunk >> 8) & 0xFF), (byte)(cmdChunk & 0xFF), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
                    data.Skip(address + 0x200).Take(chunkSize).ToArray());

                //Get response
                var r = SendCommand(new byte[] { 0x06, 0xB0, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 8);
                if (r == null || r[0] != 0xA5)
                {
                    throw new InvalidOperationException("Body not accepted");
                }

                address += chunkSize;
                size -= chunkSize;
            }
        }

        /// <summary>
        /// Sends command with no attached data and returns expected response.
        /// </summary>
        /// <param name="cmd"></param>
        /// <param name="bytesExpected"></param>
        /// <returns></returns>
        public byte[] SendCommand(byte[] cmd, int bytesExpected)
        {
            return _SendCommand(_handle, cmd, null, bytesExpected);
        }

        /// <summary>
        /// Sends command with no attached data and no response.
        /// </summary>
        /// <param name="cmd"></param>
        public void SendCommand(byte[] cmd)
        {
            SendCommand(cmd, null);
        }

        /// <summary>
        /// Sends command with attached data and no response.
        /// </summary>
        /// <param name="cmd"></param>
        /// <param name="data"></param>
        public void SendCommand(byte[] cmd, byte[] data)
        {
            _SendCommand(_handle, cmd, data, 0);
        }

        /// <summary>
        /// Closes the connection to the device.
        /// </summary>
        public void Close()
        {
            if (_handle != null && !_handle.IsClosed)
            {
                _handle.Close();
            }
        }

        private static byte[] _SendCommand(SafeFileHandle handle, byte[] cmd, byte[] data, int bytesExpected)
        {
            const int IOCTL_SCSI_PASS_THROUGH_DIRECT = 0x4D014;
            const int TIMEOUT_SECS = 30;
            SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER scsi = null;
            IntPtr inBuffer = IntPtr.Zero;
            byte[] ret = null;

            try
            {
                scsi = new SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER();
                scsi.sptd.Length = (short)Marshal.SizeOf(scsi.sptd);
                scsi.sptd.TimeOutValue = TIMEOUT_SECS;
                scsi.sptd.SenseInfoOffset = (uint)Marshal.OffsetOf(typeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER), "sense");
                scsi.sptd.SenseInfoLength = (byte)scsi.sense.Length;
                scsi.sptd.CdbLength = (byte)cmd.Length;
                Array.Copy(cmd, scsi.sptd.Cdb, cmd.Length);
                scsi.sptd.DataIn = data != null && data.Length > 0 ? SCSI_IOCTL_DATA_OUT : SCSI_IOCTL_DATA_IN;
                scsi.sptd.DataTransferLength = data != null && data.Length > 0 ? data.Length : bytesExpected;
                scsi.sptd.DataBuffer = Marshal.AllocHGlobal(scsi.sptd.DataTransferLength);
                if (data != null && data.Length > 0)
                {
                    Marshal.Copy(data, 0, scsi.sptd.DataBuffer, data.Length);
                }

                uint bytesReturned;
                inBuffer = Marshal.AllocHGlobal(Marshal.SizeOf(scsi));
                var size = (uint)Marshal.SizeOf(scsi);
                Marshal.StructureToPtr(scsi, inBuffer, false);
                if (!DeviceIoControl(handle.DangerousGetHandle(), IOCTL_SCSI_PASS_THROUGH_DIRECT,
                    inBuffer, size, inBuffer, size, out bytesReturned, IntPtr.Zero))
                {
                    //Whoops, do something with the error code
                    int last = Marshal.GetLastWin32Error();
                    throw new InvalidOperationException("DeviceIoControl failed: " + last.ToString("X04"));
                }
                else
                {
                    if (scsi.sptd.ScsiStatus != 0)
                    {
                        //Whoops, do something with the error code
                        throw new InvalidOperationException("SCSI command failed: " + scsi.sptd.ScsiStatus.ToString("X02"));
                    }
                    else
                    {
                        //Success, marshal back any data we received
                        if (scsi.sptd.DataTransferLength > 0)
                        {
                            ret = new byte[scsi.sptd.DataTransferLength];
                            Marshal.Copy(scsi.sptd.DataBuffer, ret, 0, ret.Length);
                        }
                    }
                }
            }
            finally
            {
                /* Free any unmanaged resources */

                if (scsi != null && scsi.sptd.DataBuffer != IntPtr.Zero)
                {
                    Marshal.FreeHGlobal(scsi.sptd.DataBuffer);
                }

                if (inBuffer != IntPtr.Zero)
                {
                    Marshal.FreeHGlobal(inBuffer);
                }
            }

            return ret;
        }
    }
}
