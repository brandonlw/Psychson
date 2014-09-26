using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EmbedPayload
{
    class Startup
    {
        public enum ExitCode
        {
            Success = 0,
            Failure = 1
        }

        static void Main(string[] args)
        {
            if (args.Length != 2)
            {
                Console.WriteLine("Usage: [payload BIN] [firmware image]");
                return;
            }

            try
            {
                //Assume success at first
                Environment.ExitCode = (int)ExitCode.Success;

                //Read all bytes from input file
                var payload = File.ReadAllBytes(args[0]);

                //Read all bytes from output file:
                var stream = new FileStream(args[1], FileMode.Open, FileAccess.ReadWrite);
                var header = new byte[0x200];
                stream.Read(header, 0, header.Length);
                var data = new byte[0x6000];
                stream.Read(data, 0, data.Length);

                //  Look for 0x12345678
                var signature = new byte[] { 0x12, 0x34, 0x56, 0x78 };
                int? address = null;
                for (int i = 0; i < data.Length; i++)
                {
                    bool match = true;
                    for (int j = 0; j < signature.Length; j++)
                    {
                        if (data[i + j] != signature[j])
                        {
                            match = false;
                            break;
                        }
                    }

                    if (match)
                    {
                        address = i;
                        break;
                    }
                }

                //  When found, overwrite with input data
                if (address.HasValue)
                {
                    if ((0x200 + address.Value) >= 0x6000)
                    {
                        throw new InvalidOperationException("Insufficient memory to inject file!");
                    }

                    stream.Seek(0x200 + address.Value, SeekOrigin.Begin);
                    stream.Write(payload, 0, payload.Length);

                    //Save output file back out
                    stream.Close();
                    Console.WriteLine("File updated.");
                }
                else
                {
                    Console.WriteLine("Signature not found!");
                }
            }
            catch (Exception ex)
            {
                //Uh-oh
                Environment.ExitCode = (int)ExitCode.Failure;

                Console.WriteLine("FATAL: " + ex.ToString());
            }
        }
    }
}
