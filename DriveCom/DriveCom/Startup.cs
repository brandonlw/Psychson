using Microsoft.Win32.SafeHandles;
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace DriveCom
{
    class Startup
    {
        private const int _WAIT_TIME_MS = 2000;
        private static PhisonDevice _device = null;
        private static string _burner;
        private static string _firmware;
        private static string _password;

        public enum Action
        {
            None,
            GetInfo,
            SetPassword,
            DumpFirmware,
            SetBootMode,
            SendExecutable,
            SendFirmware,
            GetNumLBAs
        }

        public enum ExitCode
        {
            Success = 0,
            Failure = 1
        }

        static void Main(string[] args)
        {
            try
            {
                Environment.ExitCode = (int)ExitCode.Success;

                var action = Action.None;
                string drive = string.Empty;

                foreach (var arg in args)
                {
                    var parts = arg.TrimStart(new char[] { '/' }).Split(new char[] { '=' },
                        StringSplitOptions.RemoveEmptyEntries);
                    switch (parts[0].ToLower())
                    {
                        case "action":
                            {
                                action = (Action)Enum.Parse(typeof(Action), parts[1]);
                                break;
                            }
                        case "drive":
                            {
                                drive = parts[1];
                                break;
                            }
                        case "burner":
                            {
                                _burner = parts[1];
                                break;
                            }
                        case "firmware":
                            {
                                _firmware = parts[1];
                                break;
                            }
                        case "password":
                            {
                                _password = parts[1];
                                break;
                            }
                        default:
                            {
                                break;
                            }
                    }
                }

                if (!string.IsNullOrEmpty(drive))
                {
                    _OpenDrive(drive);
                }

                if (action != Action.None)
                {
                    Console.WriteLine("Action specified: " + action.ToString());

                    switch (action)
                    {
                        case Action.DumpFirmware:
                            {
                                _DumpFirmware(_firmware);
                                break;
                            }
                        case Action.GetInfo:
                            {
                                _GetInfo();
                                break;
                            }
                        case Action.SendExecutable:
                            {
                                _ExecuteImage(_burner);
                                break;
                            }
                        case Action.SendFirmware:
                            {
                                _SendFirmware();
                                break;
                            }
                        case Action.GetNumLBAs:
                            {
                                _DisplayLBAs();
                                break;
                            }
                        case Action.SetBootMode:
                            {
                                _device.JumpToBootMode();
                                Thread.Sleep(_WAIT_TIME_MS);
                                break;
                            }
                        case Action.SetPassword:
                            {
                                _SendPassword(_password);
                                break;
                            }
                        default:
                            {
                                throw new ArgumentException("No/invalid action specified");
                            }
                    }
                }
                else
                {
                    Console.WriteLine("No action specified, entering console.");

                    bool exiting = false;
                    while (!exiting)
                    {
                        Console.Write(">");
                        var line = Console.ReadLine();
                        var @params = line.Split(new char[] { ' ' });

                        try
                        {
                            switch (@params[0].ToLower())
                            {
                                case "open":
                                    {
                                        _OpenDrive(@params[1]);
                                        break;
                                    }
                                case "close":
                                    {
                                        _CloseDrive();
                                        break;
                                    }
                                case "mode":
                                    {
                                        _GetInfo();
                                        break;
                                    }
                                case "info":
                                    {
                                        var data = _device.RequestVendorInfo();
                                        Console.WriteLine(string.Format("Info: {0}...", BitConverter.ToString(data, 0, 16)));
                                        break;
                                    }
                                case "get_num_lbas":
                                    {
                                        _DisplayLBAs();
                                        break;
                                    }
                                case "password":
                                    {
                                        _SendPassword(@params[1]);
                                        break;
                                    }
                                case "dump_xram":
                                    {
                                        var address = 0;
                                        var data = new byte[0xF000];
                                        for (int i = 0; i < data.Length; i++)
                                        {
                                            var result = _device.SendCommand(new byte[] { 0x06, 0x06,
                                        (byte)((address >> 8) & 0xFF), (byte)(address & 0xFF), 0x00, 0x00, 0x00, 0x00 }, 1);
                                            data[address] = result[0];
                                            address++;
                                        }

                                        File.WriteAllBytes(@params[1], data);
                                        break;
                                    }
                                case "dump_firmware":
                                    {
                                        _DumpFirmware(@params[1]);
                                        break;
                                    }
                                case "nand_read":
                                    {
                                        var address = Convert.ToInt32(@params[1], 16);
                                        var size = Convert.ToInt32(@params[2], 16);
                                        var result = _device.SendCommand(new byte[] { 0x06, 0xB2, 0x10,
                                        (byte)((address >> 8) & 0xFF), (byte)(address & 0xFF), 0x00, 0x00,
                                        (byte)((size >> 8) & 0xFF), (byte)(size & 0xFF) }, size * 512);
                                        Console.WriteLine(string.Format("Data: {0}...", BitConverter.ToString(result, 0, 16)));
                                        break;
                                    }
                                case "boot":
                                    {
                                        _device.JumpToBootMode();
                                        Thread.Sleep(_WAIT_TIME_MS);
                                        break;
                                    }
                                case "set_burner":
                                    {
                                        _burner = @params[1];
                                        break;
                                    }
                                case "set_firmware":
                                    {
                                        _firmware = @params[1];
                                        break;
                                    }
                                case "burner":
                                    {
                                        _ExecuteImage(_burner);
                                        break;
                                    }
                                case "firmware":
                                    {
                                        _SendFirmware();
                                        break;
                                    }
                                case "peek":
                                    {
                                        var address = Convert.ToInt32(@params[1], 16);
                                        var result = _device.SendCommand(new byte[] { 0x06, 0x06,
                                        (byte)((address >> 8) & 0xFF), (byte)(address & 0xFF), 0x00, 0x00, 0x00, 0x00 }, 1);
                                        Console.WriteLine("Value: " + result[0].ToString("X02"));
                                        break;
                                    }
                                case "poke":
                                    {
                                        var address = Convert.ToInt32(@params[1], 16);
                                        var value = Convert.ToInt32(@params[2], 16);
                                        _device.SendCommand(new byte[] { 0x06, 0x07,
                                        (byte)((address >> 8) & 0xFF), (byte)(address & 0xFF), (byte)value, 0x00, 0x00 }, 1);
                                        break;
                                    }
                                case "ipeek":
                                    {
                                        var address = Convert.ToInt32(@params[1], 16);
                                        var result = _device.SendCommand(new byte[] { 0x06, 0x08,
                                        (byte)(address & 0xFF), 0x00, 0x00, 0x00, 0x00 }, 1);
                                        Console.WriteLine("Value: " + result[0].ToString("X02"));
                                        break;
                                    }
                                case "ipoke":
                                    {
                                        var address = Convert.ToInt32(@params[1], 16);
                                        var value = Convert.ToInt32(@params[2], 16);
                                        _device.SendCommand(new byte[] { 0x06, 0x09,
                                        (byte)(address & 0xFF), (byte)value, 0x00, 0x00 }, 1);
                                        break;
                                    }
                                case "quit":
                                case "exit":
                                    {
                                        exiting = true;
                                        break;
                                    }
                                default:
                                    Console.WriteLine("Invalid command: " + @params[0]);
                                    break;
                            }
                        }
                        catch (Exception ex)
                        {
                            Console.WriteLine("ERROR: " + ex.ToString());
                        }
                    }

                    Console.WriteLine("Done.");
                }
            }
            catch (Exception ex)
            {
                Environment.ExitCode = (int)ExitCode.Failure;

                Console.WriteLine("FATAL: " + ex.ToString());
            }
            finally
            {
                if (_device != null)
                {
                    _device.Close();
                }
            }
        }

        private static void _OpenDrive(string drive)
        {
            _CloseDrive();

            _device = new PhisonDevice(drive[0]);
            _device.Open();
        }

        private static void _CloseDrive()
        {
            if (_device != null)
            {
                _device.Close();
                _device = null;
            }
        }

        private static void _DisplayLBAs()
        {
            Console.WriteLine("Number of LBAs: 0x" + _device.GetNumLBAs().ToString("X08"));
        }

        private static void _DumpFirmware(string fileName)
        {
            var address = 0;
            var data = new byte[0x32400];
            var header = new byte[] { 0x42, 0x74, 0x50, 0x72, 0x61, 0x6D, 0x43, 0x64,
                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x10, 0x0B, 0x18 };
            Array.Copy(header, 0, data, 0, header.Length);
            while (address * 0x200 < data.Length)
            {
                var length = Math.Min(0x40 * 512, (data.Length - 0x400) - (address * 0x200));
                var temp = length / 512;
                var result = _device.SendCommand(new byte[] { 0x06, 0xB2, 0x10,
                                        (byte)((address >> 8) & 0xFF), (byte)(address & 0xFF), 0x00, 0x00,
                                        (byte)((temp >> 8) & 0xFF), (byte)(temp & 0xFF) }, length);
                Array.Copy(result.Take(length).ToArray(), 0, data, 0x200 + address * 512, length);
                address += 0x40;
            }

            var footer = new byte[] { 0x74, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x6D,
                                        0x70, 0x20, 0x6D, 0x61, 0x72, 0x6B, 0x00, 0x03, 0x01, 0x00, 0x10, 0x01, 0x04, 0x10, 0x42 };
            Array.Copy(footer, 0, data, data.Length - 0x200, footer.Length);
            File.WriteAllBytes(fileName, data);
        }

        private static void _SendPassword(string password)
        {
            var data = new byte[0x200];
            var pw = ASCIIEncoding.ASCII.GetBytes(password);
            Array.Copy(pw, 0, data, 0x10, pw.Length);
            _device.SendCommand(new byte[] { 0x0E, 0x00, 0x01, 0x55, 0xAA,
                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, data);
        }

        private static void _SendFirmware()
        {
            var mode = _GetInfo();
            if (mode != PhisonDevice.RunMode.Burner)
            {
                if (mode != PhisonDevice.RunMode.BootMode)
                {
                    Console.WriteLine("Switching to boot mode...");
                    _device.JumpToBootMode();
                    Thread.Sleep(_WAIT_TIME_MS);
                }

                _ExecuteImage(_burner);
            }

            _RunFirmware(_firmware);
        }

        private static PhisonDevice.RunMode _GetInfo()
        {
            Console.WriteLine("Gathering information...");
            Console.WriteLine("Reported chip type: " + _device.GetChipType().GetValueOrDefault().ToString("X04"));
            Console.WriteLine("Reported chip ID: " + _device.GetChipID());
            Console.WriteLine("Reported firmware version: " + _device.GetFirmwareVersion());

            var ret = _device.GetRunMode();
            Console.WriteLine("Mode: " + ret.ToString());

            return ret;
        }

        private static void _ExecuteImage(string fileName)
        {
            //Read image
            var file = new FileStream(fileName, FileMode.Open);
            var fileData = new byte[file.Length];
            file.Read(fileData, 0, fileData.Length);
            file.Close();

            //Load it
            _device.TransferFile(fileData);
            _device.JumpToPRAM();

            //Wait a little bit
            Thread.Sleep(_WAIT_TIME_MS);
        }

        private static void _RunFirmware(string fileName)
        {
            //Get file data
            var fw = new FileStream(fileName, FileMode.Open);
            var data = new byte[fw.Length];
            fw.Read(data, 0, data.Length);
            fw.Close();

            //TODO: Find out what this actually does...
            //Console.WriteLine("Sending scary B7 command (takes several seconds)...");
            //_device.SendCommand(new byte[] { 0x06, 0xB7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });

            Console.WriteLine("Rebooting...");
            _device.JumpToBootMode();
            Thread.Sleep(_WAIT_TIME_MS);

            Console.WriteLine("Sending firmware...");
            _device.TransferFile(data, 0x01, 0x00);
            var ret = _device.SendCommand(new byte[] { 0x06, 0xEE, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 64 + 8);
            Thread.Sleep(_WAIT_TIME_MS);
            _device.TransferFile(data, 0x03, 0x02);
            ret = _device.SendCommand(new byte[] { 0x06, 0xEE, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 }, 64 + 8);
            Thread.Sleep(_WAIT_TIME_MS);
            ret = _device.SendCommand(new byte[] { 0x06, 0xEE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 64 + 8);
            Thread.Sleep(_WAIT_TIME_MS);
            ret = _device.SendCommand(new byte[] { 0x06, 0xEE, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 }, 64 + 8);
            Thread.Sleep(_WAIT_TIME_MS);

            Console.WriteLine("Executing...");
            _device.JumpToPRAM();
            Thread.Sleep(_WAIT_TIME_MS);
            
            //Display new mode, if we can actually get it
            Console.WriteLine("Mode: " + _device.GetRunMode().ToString());
        }
    }
}
