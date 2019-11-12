using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Diagnostics;

namespace NeutronBuilder
{
    class Program
    {
        static string[] ASMfiles;
        static string[] FSfiles;
        static string[] BINfiles;
        static string IMAfile;

        static void Main(string[] args)
        {
            if (!File.Exists("neutron.nbuild"))
                Error("No neutron.nbuild file");

            string[] lines = File.ReadAllLines("neutron.nbuild");

            List<string> ASMfilesList = new List<string>();
            List<string> FSfileList = new List<string>();
            bool fsSection = false;
            for (int i = 0; i < lines.Length; i++)
            {
                string line = lines[i];
                if (!line.StartsWith("#"))
                {
                    if (!fsSection)
                    {
                        if (line == ".fs")
                            fsSection = true;
                        else
                            ASMfilesList.Add(line);
                    }
                    else
                        FSfileList.Add(line);
                }
            }
            ASMfiles = ASMfilesList.ToArray();
            FSfiles = FSfileList.ToArray();

            BINfiles = new string[ASMfiles.Length];
            for (int i = 0; i < ASMfiles.Length; i++)
                BINfiles[i] = "build\\" + Path.GetFileNameWithoutExtension(ASMfiles[i]) + ".bin";

            IMAfile = "build\\Neutron.ima";

            using (BinaryWriter bw = new BinaryWriter(File.OpenWrite(IMAfile)))
            {
                Console.WriteLine("--- ASSEMBLING ---");
                for (int i = 0; i < ASMfiles.Length; i++)
                {
                    Console.WriteLine("ASSEMBLING: " + ASMfiles[i].Split(' ')[0] + " >>> " + BINfiles[i]);
                    ProcessStartInfo yasm = new ProcessStartInfo();
                    yasm.FileName = "yasm.exe";
                    yasm.Arguments = "-f bin -o " + BINfiles[i] + " " + ASMfiles[i].Split(' ')[0];
                    yasm.RedirectStandardError = true;
                    yasm.RedirectStandardOutput = true;
                    yasm.UseShellExecute = false;
                    yasm.CreateNoWindow = true;
                    Process yasmProcess = Process.Start(yasm);
                    string yasmOutput = yasmProcess.StandardError.ReadToEnd();
                    yasmProcess.WaitForExit();
                    if (yasmProcess.ExitCode != 0)
                        Error("YASM ERROR:\n" + yasmOutput);

                    if (!ASMfiles[i].EndsWith("--no-append"))
                    {
                        Console.WriteLine("APPENDING: " + BINfiles[i] + " >>> " + IMAfile);
                        using (BinaryReader br = new BinaryReader(File.OpenRead(BINfiles[i])))
                            while (br.BaseStream.Position < br.BaseStream.Length)
                                bw.Write((byte)br.ReadByte());
                    }
                }

                Console.WriteLine("--- BUILDING FILE SYSTEM ---");
                Console.WriteLine("WRITING: FS HEADER");
                byte[] bytesToAdd = new byte[2560 - bw.BaseStream.Position];
                bw.Write(bytesToAdd);
                bw.Write(0xDEADF500);
                WriteString(bw, "NEUTRON TEST FS");
                bw.Write((byte)0);
                bw.Write((byte)1);
                bw.Write((byte)1);
                bytesToAdd = new byte[512 - 22];
                bw.Write(bytesToAdd);
                int[] FSfileClusters = new int[FSfileList.Count];
                int fileStartSector = 7;
                for (int i = 0; i < FSfileList.Count; i++)
                {
                    Console.WriteLine("CALCULATING: OFSET FOR " + FSfileList[i].Split('>')[0]);

                    string file = FSfileList[i];
                    string orig = file.Split('>')[0];
                    string dest = file.Split('>')[1];
                    long fileSize = new FileInfo(orig).Length;
                    int fileSizeSectors = (int)fileSize / 512;
                    if (fileSize % 512 > 0)
                        fileSizeSectors++;
                    FSfileClusters[i] = fileSizeSectors;

                    WriteString(bw, dest.PadRight(23, (char)0));
                    bw.Write((byte)0);
                    bw.Write((uint)fileSize);
                    bw.Write((uint)fileStartSector);
                    fileStartSector += fileSizeSectors;
                }

                bytesToAdd = new byte[512 - (FSfileList.Count * 32)];
                bw.Write(bytesToAdd);

                for (int i = 0; i < FSfileList.Count; i++)
                {
                    string file = FSfileList[i];
                    string orig = file.Split('>')[0];
                    string dest = file.Split('>')[1];
                    long fileSize = new FileInfo(orig).Length;

                    Console.WriteLine("WRITING: " + orig + " >>> " + dest + " (nFS IN " + IMAfile + ")");

                    using (BinaryReader br = new BinaryReader(File.OpenRead(orig)))
                        while (br.BaseStream.Position < br.BaseStream.Length)
                            bw.Write((byte)br.ReadByte());

                    bytesToAdd = new byte[(512 * FSfileClusters[i]) - fileSize];
                    bw.Write(bytesToAdd);
                }

                Console.WriteLine("--- PADDING ---");
                bytesToAdd = new byte[1474560 - bw.BaseStream.Position];
                bw.Write(bytesToAdd);
            }

            Console.WriteLine("\nCOMPLETED! PRESS ANY KEY TO EXIT");
            Console.ReadKey();
        }

        static void Error(string text)
        {
            Console.WriteLine(text);
            Console.WriteLine("PRESS ANY KEY TO EXIT");
            Console.ReadKey();
            Environment.Exit(-1);
        }

        static void WriteString(BinaryWriter bw, string str)
        {
            foreach(char c in str)
                bw.Write((byte)c);
        }
    }
}
