using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;

namespace Injector
{
    class Startup
    {
        private static string _firmwareImage;
        private static string _outputFile;
        private static FirmwareSection _section = FirmwareSection.None;
        private static Dictionary<FirmwareSection, string> _codeFiles;
        private static Dictionary<FirmwareSection, string> _rstFiles;

        internal enum Action
        {
            None,
            GenerateHFile,
            FindFreeBlock,
            ApplyPatches
        }

        internal enum ExitCode
        {
            Success = 0,
            Failure = 1
        }

        static void Main(string[] args)
        {
            try
            {
                _codeFiles = new Dictionary<FirmwareSection, string>();
                _rstFiles = new Dictionary<FirmwareSection, string>();

                //Assume success to start with
                Environment.ExitCode = (int)ExitCode.Success;

                var action = Action.None;

                //Parse command line arguments
                foreach (var arg in args)
                {
                    var parts = arg.TrimStart(new char[] { '/' }).Split(new char[] { '=' },
                        StringSplitOptions.RemoveEmptyEntries);
                    switch (parts[0].ToLower())
                    {
                        case "action":
                            {
                                action = (Action)Enum.Parse(typeof(Action), parts[1]);
                                Console.WriteLine("Action: " + action.ToString());
                                break;
                            }
                        case "section":
                            {
                                _section = (FirmwareSection)Enum.Parse(typeof(FirmwareSection), parts[1]);
                                Console.WriteLine("Section: " + _section.ToString());
                                break;
                            }
                        case "firmware":
                            {
                                _firmwareImage = parts[1];
                                Console.WriteLine("Firmware image: " + _firmwareImage);
                                _CheckFirmwareImage();
                                break;
                            }
                        case "output":
                            {
                                _outputFile = parts[1];
                                Console.WriteLine("Output file: " + _outputFile);
                                break;
                            }
                        default:
                            {
                                _ParseFileNames(ref _codeFiles, "code", parts[0], parts[1]);
                                _ParseFileNames(ref _rstFiles, "rst", parts[0], parts[1]);
                                break;
                            }
                    }
                }

                //Firmware image file name is always required
                if (string.IsNullOrEmpty(_firmwareImage))
                {
                    throw new ArgumentException("No/Invalid firmware image file name specified");
                }

                switch (action)
                {
                    case Action.GenerateHFile:
                        {
                            if (string.IsNullOrEmpty(_outputFile))
                            {
                                throw new ArgumentException("No/Invalid output file name specified");
                            }

                            Console.WriteLine("Generating .h file...");

                            _GenerateHFile();
                            break;
                        }
                    case Action.ApplyPatches:
                        {
                            //Check required arguments for this action

                            if (string.IsNullOrEmpty(_outputFile))
                            {
                                throw new ArgumentException("No/Invalid output file name specified");
                            }

                            if (_codeFiles.Count == 0)
                            {
                                throw new ArgumentException("No code file name(s) specified");
                            }

                            if (_rstFiles.Count == 0)
                            {
                                throw new ArgumentException("No/Invalid RST file name specified");
                            }

                            Console.WriteLine("Applying patches...");
                            _ApplyPatches();
                            break;
                        }
                    case Action.FindFreeBlock:
                        {
                            //Check required arguments for this action
                            if (_section == FirmwareSection.None)
                            {
                                throw new ArgumentException("No/Invalid section specified");
                            }

                            Console.WriteLine("Retriving free space...");
                            _GetFreeSpaceToFile();
                            break;
                        }
                    default:
                        throw new ArgumentException("No/Invalid action specified");
                }

                Console.WriteLine("Done.");
            }
            catch (Exception ex)
            {
                //Uh-oh...
                Environment.ExitCode = (int)ExitCode.Failure;

                var asm = System.Reflection.Assembly.GetExecutingAssembly();
                var asmName = asm.GetName();
                Console.WriteLine(asmName.Name + " v" + asmName.Version.ToString(3));
                Console.WriteLine("Actions:");
                Console.WriteLine("\tGenerateHFile\tGenerates C .h file of common XRAM & function equates.");
                Console.WriteLine("\tFindFreeBlock\tWrites amount of free space for a section to file.");
                Console.WriteLine("\tApplyPatches\tApplies available patches from code into firmware image.");
                Console.WriteLine();
                Console.WriteLine("FATAL: " + ex.ToString());
            }
        }

        private static void _CheckFirmwareImage()
        {
            var md5 = new MD5CryptoServiceProvider();
            var verified = new List<string>();
            verified.Add("4C4C0001EC83102C4627D271FF8362A2");

            var hash = BitConverter.ToString(md5.ComputeHash(File.ReadAllBytes(_firmwareImage)))
                .Replace("-", string.Empty);
            if (!verified.Contains(hash))
            {
                Console.WriteLine("WARNING! This firmware version has not been " +
                    "verified to work with these patches.");
            }
        }

        private static void _ParseFileNames(ref Dictionary<FirmwareSection, string> files,
            string suffix, string name, string value)
        {
            if (name.ToLower().EndsWith(suffix))
            {
                var section = FirmwareSection.Base;
                int s;

                if (int.TryParse(name.Substring(0, name.Length - suffix.Length), out s))
                {
                    section = (FirmwareSection)s;
                }

                files.Add(section, value);
                Console.WriteLine(suffix + " " + section.ToString() + " file: " + value);
            }
        }

        private static Dictionary<string, int> _GetAddressMap(string fileName)
        {
            //Read in RST file and its label<->address map
            var addressMap = new Dictionary<string, int>();
            var ret = new Dictionary<string, int>();
            var rst = new StreamReader(fileName, ASCIIEncoding.ASCII);

            while (true)
            {
                var line = rst.ReadLine();
                if (line == null)
                {
                    break;
                }

                if (line.EndsWith(":"))
                {
                    var parts = line.Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
                    var label = parts[parts.Length - 1].TrimEnd(':');
                    var address = parts[0];

                    if (label.StartsWith("_"))
                    {
                        ret.Add(label, Convert.ToInt32(address, 16));
                    }
                }
            }

            rst.Close();

            return ret;
        }

        private static void _GenerateHFile()
        {
            var stream = new StreamWriter(_outputFile);

            //Read in firmware image
            var image = new FirmwareImage(_firmwareImage);
            image.Open();

            var pattern = new byte?[] { 0x90, 0xF0, 0xB8, 0xE0, //mov DPTR, #0xF0B8 \ movx a, @DPTR
                                        0x90, null, null, 0xF0, //mov DPTR, #0x???? \ movx @DPTR, a
                                        0x90, 0xF0, 0xB9, 0xE0 }; //mov DPTR, #0xF0B9 \ movx a, @DPTR \ movx DPTR, #0x????
            FirmwareSection section;
            int address;
            if (image.FindPattern(pattern, out section, out address))
            {
                var a = image.GetSection(section)[address + 5] << 8;
                a |= image.GetSection(section)[address + 6];

                stream.WriteLine(string.Format("__xdata __at 0x{0} BYTE {1};", a.ToString("X04"), "bmRequestType"));
                stream.WriteLine(string.Format("__xdata __at 0x{0} BYTE {1};", (a + 1).ToString("X04"), "bRequest"));
            }

            pattern = new byte?[] { 0x90, null, null, 0xE0, //mov DPTR, #0x???? \ movx a, @DPTR
                                    0xB4, 0x28 }; //cjne A, #0x28, ????
            if (image.FindPattern(pattern, out section, out address))
            {
                var a = image.GetSection(section)[address + 1] << 8;
                a |= image.GetSection(section)[address + 2];
                stream.WriteLine(string.Format("__xdata __at 0x{0} BYTE {1}[16];", a.ToString("X04"), "scsi_cdb"));

                stream.WriteLine(string.Format("#define {0} 0x{1}", "DEFAULT_READ_SECTOR_HANDLER", (address + 7).ToString("X04")));
                pattern = new byte?[] { 0x90, (byte)((a >> 8) & 0xFF), (byte)(a & 0xFF), //mov DPTR, #scsi_tag
                                        0xE0, 0x12 }; //mvox A, @DPTR \ lcall 0x????
                if (image.FindPattern(pattern, address, out section, out address))
                {
                    stream.WriteLine(string.Format("#define {0} 0x{1}", "DEFAULT_CDB_HANDLER", address.ToString("X04")));
                }
            }

            pattern = new byte?[] { 0x90, 0xF2, 0x1C, //mov DPTR, #0xF21C
                                    0x74, 0x55, 0xF0, //mov A, #0x55 \ movx @DPTR, A
                                    0x74, 0x53, 0xF0, //mov A, #0x53 \ movx @DPTR, A
                                    0x74, 0x42, 0xF0, //mov A, #0x42 \ movx @DPTR, A
                                    0x74, 0x53, 0xF0, //mov A, #0x53 \ movx @DPTR, A
                                    0x90 }; //mov DPTR, #0x????
            if (image.FindPattern(pattern, out section, out address))
            {
                var a = image.GetSection(section)[address + pattern.Length] << 8;
                a |= image.GetSection(section)[address + pattern.Length + 1];

                stream.WriteLine(string.Format("__xdata __at 0x{0} BYTE {1}[4];", (a - 3).ToString("X04"), "scsi_tag"));
            }

            pattern = new byte?[] { 0xC0, 0xE0, 0xC0, 0x83, 0xC0, 0x82, //push ACC \ push DPH \ push DPL
                                    0x90, 0xF0, 0x20, 0xE0, //mov DPTR, #0xF020 \ movx A, @DPTR
                                    0x30, 0xE1, null, //jnb ACC.1, ????
                                    0x12, null, null, 0x90 }; //lcall ???? \ mov DPTR, #0x????
            if (image.FindPattern(pattern, out section, out address))
            {
                var a = image.GetSection(section)[address + 17] << 8;
                a |= image.GetSection(section)[address + 18];

                stream.WriteLine(string.Format("__xdata __at 0x{0} BYTE {1};", a.ToString("X04"), "FW_EPIRQ"));
            }

            stream.WriteLine(string.Format("__xdata __at 0x{0} BYTE {1}[1024];", "B000", "EPBUF"));

            stream.Close();
        }

        private static void _GetFreeSpaceToFile()
        {
            //Read in firmware image
            var image = new FirmwareImage(_firmwareImage);
            image.Open();

            File.WriteAllText(_outputFile, "0x" + image.FindLastFreeChunk(_section).ToString("X04"));
        }

        private static void _ApplyPatches()
        {
            //Read in firmware image
            var image = new FirmwareImage(_firmwareImage);
            image.Open();

            //Read in the RST files
            var maps = new Dictionary<FirmwareSection, Dictionary<string, int>>();
            foreach (var file in _rstFiles)
            {
                maps.Add(file.Key, _GetAddressMap(file.Value));
            }

            //Find how much free space is left on each page
            var emptyStart = new Dictionary<FirmwareSection, int>();
            for (FirmwareSection i = FirmwareSection.Base; i < FirmwareSection.SectionF; i++)
            {
                emptyStart.Add(i, image.FindLastFreeChunk(i));
            }

            //Embed our code files into the firmware image
            foreach (var file in _codeFiles)
            {
                var code = File.ReadAllBytes(file.Value);
                Array.Copy(code, 0, image.GetSection(file.Key), emptyStart[file.Key], code.Length);
                emptyStart[file.Key] += code.Length;
            }

            //Find the off-page call stubs
            var stubs = new Dictionary<FirmwareSection, int>();
            int saddr = 0;
            var spattern = new byte?[] { 0xC0, 0x5B, 0x74, 0x08, //push RAM_5B \ mov A, #8
                                         0xC0, 0xE0, 0xC0, 0x82, 0xC0, 0x83, //push ACC \ push DPL \ push DPH
                                         0x75, 0x5B }; //mov RAM_5B, #0x??
            FirmwareSection fs;
            for (FirmwareSection i = FirmwareSection.Section0; i <= FirmwareSection.SectionF; i++)
            {
                if (image.FindPattern(spattern, saddr, out fs, out saddr))
                {
                    stubs.Add(i, saddr);
                    saddr += spattern.Length; //move ahead so we can find the next stub
                }
            }

            //Hook into control request handling
            foreach (var map in maps)
            {
                if (map.Value.ContainsKey("_HandleControlRequest"))
                {
                    var address = map.Value["_HandleControlRequest"];
                    var pattern = new byte?[] { 0x12, null, null, //lcall #0x????
                                                0x90, 0xFE, 0x82, 0xE0, //mov DPTR, #0xFE82 \ movx A, @DPTR
                                                0x54, 0xEF, 0xF0 }; //anl A, #0xEF \ movx @DPTR, A
                    FirmwareSection s;
                    int a;
                    if (image.FindPattern(pattern, out s, out a))
                    {
                        a = (image.GetSection(s)[a + 1] << 8) | image.GetSection(s)[a + 2];

                        image.GetSection(s)[a + 1] = (byte)((address >> 8) & 0xFF);
                        image.GetSection(s)[a + 2] = (byte)(address & 0xFF);
                        if (map.Key != FirmwareSection.Base)
                        {
                            image.GetSection(s)[a + 4] = (byte)((stubs[map.Key] >> 8) & 0xFF);
                            image.GetSection(s)[a + 5] = (byte)(stubs[map.Key] & 0xFF);
                        }
                    }
                    break;
                }
            }

            //Replace the EP interrupt vector, handling all incoming and outgoing non-control data
            foreach (var map in maps)
            {
                //This part must be on the base page
                if (map.Value.ContainsKey("_EndpointInterrupt"))
                {
                    var address = map.Value["_EndpointInterrupt"];
                    var s = image.GetSection(FirmwareSection.Base);
                    s[0x0014] = (byte)((address >> 8) & 0xFF);
                    s[0x0015] = (byte)(address & 0xFF);
                }

                if (map.Value.ContainsKey("_HandleEndpointInterrupt"))
                {
                    //Find the base page location to patch
                    var pattern = new byte?[] { 0x30, 0xE1, null, //jnb ACC.1, #0x????
                                                0x12, null, null, //lcall #0x????
                                                0x90, 0xFE, 0x82, 0xE0, //mov DPTR, #0xFE82 \ movx A, @DPTR
                                                0x54, 0xEF, 0xF0 }; //anl A, #0xEF \ movx @DPTR, A
                    FirmwareSection ps;
                    int pa;
                    if (image.FindPattern(pattern, out ps, out pa))
                    {
                        //Create off-page stub for this if necessary
                        var address = map.Value["_HandleEndpointInterrupt"];
                        var stubAddress = address;
                        if (map.Key != FirmwareSection.Base)
                        {
                            stubAddress = emptyStart[FirmwareSection.Base];
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = 0x90;
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = (byte)((address >> 8) & 0xFF);
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = (byte)(address & 0xFF);
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = 0x02;
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = (byte)((stubs[map.Key] >> 8) & 0xFF);
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = (byte)(stubs[map.Key] & 0xFF);
                        }

                        //Apply the patch
                        var s = image.GetSection(ps);
                        s[pa + 0] = 0x60;
                        s[pa + 1] = 0x0B;
                        s[pa + 2] = 0x00;
                        s[pa + 4] = (byte)((stubAddress >> 8) & 0xFF);
                        s[pa + 5] = (byte)(stubAddress & 0xFF);
                        for (int i = 0; i < 7; i++)
                        {
                            s[pa + 6 + i] = 0x00;
                        }
                    }
                }
            }

            //Apply CDB-handling code
            foreach (var map in maps)
            {
                if (map.Value.ContainsKey("_HandleCDB"))
                {
                    var pattern = new byte?[] { 0x90, null, null, 0xE0, //mov DPTR, #0x???? \ movx a, @DPTR
                                    0xB4, 0x28 }; //cjne A, #0x28, ????
                    FirmwareSection ps;
                    int pa;
                    if (image.FindPattern(pattern, out ps, out pa))
                    {
                        //Create off-page stub for this if necessary
                        var address = map.Value["_HandleCDB"];
                        var stubAddress = address;
                        if (map.Key != FirmwareSection.Base)
                        {
                            stubAddress = emptyStart[FirmwareSection.Base];
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = 0x90;
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = (byte)((address >> 8) & 0xFF);
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = (byte)(address & 0xFF);
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = 0x02;
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = (byte)((stubs[map.Key] >> 8) & 0xFF);
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = (byte)(stubs[map.Key] & 0xFF);
                        }

                        //Apply the patch
                        var s = image.GetSection(FirmwareSection.Base);
                        s[pa + 0] = 0x02;
                        s[pa + 1] = (byte)((stubAddress >> 8) & 0xFF);
                        s[pa + 2] = (byte)(stubAddress & 0xFF);
                    }
                }
            }

            //Add our own code to the infinite loop
            foreach (var map in maps)
            {
                if (map.Value.ContainsKey("_LoopDo"))
                {
                    var pattern = new byte?[] { 0x90, null, null, 0xE0, //mov DPTR, #0x???? \ movx A, @DPTR
                                                0xB4, 0x01, null, //cjne A, #1, #0x????
                                                0x90, 0xF0, 0x79 }; //mov DPTR, #0xF079
                    FirmwareSection ps;
                    int pa;
                    if (image.FindPattern(pattern, out ps, out pa))
                    {
                        //Create off-page stub for this if necessary
                        var address = map.Value["_LoopDo"];
                        var stubAddress = address;
                        if (map.Key != FirmwareSection.Base)
                        {
                            stubAddress = emptyStart[FirmwareSection.Base];
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = 0x90;
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = (byte)((address >> 8) & 0xFF);
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = (byte)(address & 0xFF);
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = 0x02;
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = (byte)((stubs[map.Key] >> 8) & 0xFF);
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = (byte)(stubs[map.Key] & 0xFF);
                        }

                        var s = image.GetSection(ps);
                        var loopDoStart = emptyStart[FirmwareSection.Base];
                        s[emptyStart[FirmwareSection.Base]++] = 0x12;
                        s[emptyStart[FirmwareSection.Base]++] = (byte)((stubAddress >> 8) & 0xFF);
                        s[emptyStart[FirmwareSection.Base]++] = (byte)(stubAddress & 0xFF);
                        s[emptyStart[FirmwareSection.Base]++] = 0x90;
                        s[emptyStart[FirmwareSection.Base]++] = image.GetSection(ps)[pa + 1];
                        s[emptyStart[FirmwareSection.Base]++] = image.GetSection(ps)[pa + 2];
                        s[emptyStart[FirmwareSection.Base]++] = 0x22;
                        s[pa + 0] = 0x12;
                        s[pa + 1] = (byte)((loopDoStart >> 8) & 0xFF);
                        s[pa + 2] = (byte)(loopDoStart & 0xFF);
                    }
                }
            }

            //Apply password patch code
            foreach (var map in maps)
            {
                if (map.Value.ContainsKey("_PasswordReceived"))
                {
                    var pattern = new byte?[] { 0x90, 0xF2, 0x4C, 0xF0, 0xA3, //mov DPTR, #0xF24C \ movx @DPTR, A \ inc DPTR
                                                0xC0, 0x83, 0xC0, 0x82, 0x12, //push DPH \ push DPL
                                                null, null, 0xD0, 0x82, 0xD0, 0x83, 0xF0, //lcall #0x???? \ pop DPL \ pop DPH \ movx @DPTR, A
                                                0x90, 0xF2, 0x53, 0x74, 0x80, 0xF0, //mov DPTR, #0xF253 \ mov A, #0x80 \ movx @DPTR, A
                                                0x90, 0xF2, 0x53, 0xE0, //mov DPTR, #0xF253 \ movx A, @DPTR
                                                0x30, 0xE7, null, //jnb ACC.7, #0x????
                                                0x12, null, null, 0x40, null, //lcall #0x???? \ jc #0x????
                                                0x12, null, null, 0x7F, 0x00, 0x22 }; //lcall #0x???? \ mov R7, #0 \ ret
                    FirmwareSection ps;
                    int pa;
                    if (image.FindPattern(pattern, out ps, out pa))
                    {
                        //Create off-page stub for this if necessary
                        var address = map.Value["_PasswordReceived"];
                        var stubAddress = address;
                        if (map.Key != FirmwareSection.Base)
                        {
                            stubAddress = emptyStart[FirmwareSection.Base];
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = 0x90;
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = (byte)((address >> 8) & 0xFF);
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = (byte)(address & 0xFF);
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = 0x02;
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = (byte)((stubs[map.Key] >> 8) & 0xFF);
                            image.GetSection(FirmwareSection.Base)[emptyStart[FirmwareSection.Base]++] = (byte)(stubs[map.Key] & 0xFF);
                        }

                        //Apply the patch
                        pa += 0x24;
                        var passRecvdStart = emptyStart[ps] + (ps == FirmwareSection.Base ? 0x0000 : 0x5000);
                        image.GetSection(ps)[emptyStart[ps]++] = 0x12;
                        image.GetSection(ps)[emptyStart[ps]++] = image.GetSection(ps)[pa + 0];
                        image.GetSection(ps)[emptyStart[ps]++] = image.GetSection(ps)[pa + 1];
                        image.GetSection(ps)[emptyStart[ps]++] = 0x02;
                        image.GetSection(ps)[emptyStart[ps]++] = (byte)((stubAddress >> 8) & 0xFF);
                        image.GetSection(ps)[emptyStart[ps]++] = (byte)(stubAddress & 0xFF);
                        image.GetSection(ps)[pa + 0] = (byte)((passRecvdStart >> 8) & 0xFF);
                        image.GetSection(ps)[pa + 1] = (byte)(passRecvdStart & 0xFF);
                    }
                }
            }

            //Write the resulting file out
            image.Save(_outputFile);
        }
    }
}
