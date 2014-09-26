using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Injector
{
    public class FirmwareImage
    {
        private string _fileName;
        private byte[] _header;
        private Dictionary<FirmwareSection, byte[]> _sections;
        private byte[] _footer;

        public FirmwareImage(string fileName)
        {
            _fileName = fileName;
            _header = new byte[0x200];
            _sections = new Dictionary<FirmwareSection, byte[]>();
            _sections.Add(FirmwareSection.Base, new byte[0x6000]);
        }

        public byte[] GetSection(FirmwareSection section)
        {
            byte[] ret = null;

            if (_sections.ContainsKey(section))
            {
                ret = _sections[section];
            }

            return ret;
        }

        public void Open()
        {
            FirmwareSection i = 0;

            //Get the header and base page
            var stream = new FileStream(_fileName, FileMode.Open);
            var @base = GetSection(FirmwareSection.Base);
            stream.Read(_header, 0, _header.Length);
            stream.Read(@base, 0, @base.Length);

            //Read in all the sections
            while ((stream.Length - stream.Position) > 0x200)
            {
                var data = new byte[0x4000];
                stream.Read(data, 0, data.Length);
                _sections.Add(i++, data);
            }

            //If we have a footer, read it in
            if ((stream.Length - stream.Position) == 0x200)
            {
                _footer = new byte[0x200];
                stream.Read(_footer, 0, _footer.Length);
            }

            //All done
            stream.Close();
        }

        public bool FindPattern(byte?[] pattern, out FirmwareSection section, out int address)
        {
            return FindPattern(pattern, 0, out section, out address);
        }

        public bool FindPattern(byte?[] pattern, int startingOffset, out FirmwareSection section, out int address)
        {
            bool ret = false;
            section = FirmwareSection.Base;
            address = 0;

            foreach (var s in _sections)
            {
                for (int i = startingOffset; i < s.Value.Length; i++)
                {
                    bool match = true;
                    for (int j = 0; j < pattern.Length; j++)
                    {
                        if (((i + j) >= s.Value.Length) ||
                            ((s.Value[i + j] != pattern[j]) && (pattern[j].HasValue)))
                        {
                            match = false;
                            break;
                        }
                    }

                    if (match)
                    {
                        section = s.Key;
                        address = i;
                        ret = true;
                        break;
                    }
                }

                if (ret)
                {
                    break;
                }
            }

            return ret;
        }

        public int FindLastFreeChunk(FirmwareSection section)
        {
            int ret = -1;

            if (_sections.ContainsKey(section))
            {
                var data = _sections[section];
                var repeating = data[data.Length - 1];
                ret = data.Length - 2;

                while (data[ret] == repeating)
                {
                    ret--;
                    if (ret < 0)
                    {
                        break;
                    }
                }
            }
            
            return ++ret;
        }

        public void Save(string fileName)
        {
            var output = new FileStream(fileName, FileMode.Create);
            output.Write(_header, 0, _header.Length);
            foreach (var section in _sections.OrderBy(t => t.Key))
            {
                output.Write(section.Value, 0, section.Value.Length);
            }

            if (_footer != null)
            {
                output.Write(_footer, 0, _footer.Length);
            }

            output.Close();
        }
    }
}
