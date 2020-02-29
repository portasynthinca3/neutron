using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Drawing;
using System.IO;

namespace NeutronFontConv
{
    static class Program
    {
        const string inFile = "neutral.png";
        const string outFile = "neutral.nfnt";
        const int charWidth = 6;
        const int charHeight = 8;

        static void Main(string[] args)
        {
            Bitmap input = (Bitmap)Image.FromFile(inFile);
            BinaryWriter bw = new BinaryWriter(File.OpenWrite(outFile));
            StreamWriter sw = new StreamWriter(File.OpenWrite(outFile + ".asm"));

            string prefixedName = "font_" + Path.GetFileNameWithoutExtension(inFile);
            sw.WriteLine(prefixedName + ":");

            for(int cy = 0; cy < input.Height / charHeight; cy++)
            {
                for(int cx = 0; cx < input.Width / charWidth; cx++)
                {
                    sw.Write("font_char_" + ((cy * (input.Width / charWidth)) + cx) + ": db ");

                    for(int x = 0; x < charWidth; x++)
                    {
                        byte curByte = 0;
                        for (int y = 0; y < charHeight; y++)
                        {
                            Color pixel = input.GetPixel(cx * charWidth + x, cy * charHeight + y);
                            int pixSum = pixel.R + pixel.G + pixel.B;
                            curByte |= (byte)(((pixSum >= 255) ? 1 : 0) << y);
                        }
                        bw.Write(curByte);

                        sw.Write(curByte.ToString());
                        if (x != charWidth - 1){
                            sw.Write(", ");
                        } else {
                            sw.WriteLine();
                        }
                    }
                }
            }

            sw.WriteLine(prefixedName + "_end:");
            sw.WriteLine("%define " + prefixedName + "_size " + prefixedName + "_size - " + prefixedName);

            bw.Close();
            sw.Close();
        }
    }
}
