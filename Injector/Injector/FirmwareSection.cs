using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Injector
{
    public enum FirmwareSection
    {
        None = -2,
        Base = -1,
        Section0 = 0x00,
        Section1 = 0x01,
        Section2 = 0x02,
        Section3 = 0x03,
        Section4 = 0x04,
        Section5 = 0x05,
        Section6 = 0x06,
        Section7 = 0x07,
        Section8 = 0x08,
        Section9 = 0x09,
        SectionA = 0x0A,
        SectionB = 0x0B,
        SectionC = 0x0C,
        SectionD = 0x0D,
        SectionE = 0x0E,
        SectionF = 0x0F
    }
}
