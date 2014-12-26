using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace ConsoleApplication1 {
    [StructLayout( LayoutKind.Sequential, CharSet = CharSet.Ansi )]

    struct Signature {
        public string sig; // сигнатура “1CDBMSV8”
        public char ver1;
        public char ver2;
        public char ver3;
        public char ver4;
        public UInt32 length;
        public int unknown;
    };
    struct BlockHead {
        public string sig; // сигнатура “1CDBOBV8”
        public UInt32 length;
        public int version1;
        public int version2;
        public UInt32 version;
        public UInt32[] blocks; // 1018
    };
    struct RootHead {
        public string lang;
        public int numblocks;
        public UInt32[] blocks; // 1018
    };

    struct PlainBlockDirectory {
        public int numblocks;
        public UInt32[] blocks; // 1023
    };

    class Program {
        static Signature readSignature( byte[] block ) {
            using( var mms = new MemoryStream( block ) ) {
                using( var reader = new BinaryReader( mms ) ) {
                    var sig = new Signature {

                        sig = Encoding.ASCII.GetString( reader.ReadBytes( 8 ) ),
                        ver1 = reader.ReadChar(),
                        ver2 = reader.ReadChar(),
                        ver3 = reader.ReadChar(),
                        ver4 = reader.ReadChar(),
                        length = reader.ReadUInt32()
                    };

                    return sig;
                }
            }
        }
        static BlockHead readBlockHead( byte[] block ) {
            using( var mms = new MemoryStream( block ) ) {
                using( var reader = new BinaryReader( mms ) ) {
                    var sig = new BlockHead {

                        sig = Encoding.ASCII.GetString( reader.ReadBytes( 8 ) ),
                        length = reader.ReadUInt32(),
                        version1 = reader.ReadInt32(),
                        version2 = reader.ReadInt32(),
                        version = reader.ReadUInt32(),
                        blocks = new UInt32[1018]
                    };

                    for( int i = 0; i < 1018; i++ ) {
                        sig.blocks[i] = reader.ReadUInt32();
                    }

                    return sig;
                }
            }
        }
        static RootHead readRootHead( byte[] block ) {
            using( var mms = new MemoryStream( block ) ) {
                using( var reader = new BinaryReader( mms ) ) {
                    var sig = new RootHead {

                        lang = Encoding.ASCII.GetString( reader.ReadBytes( 32 ) ),
                        numblocks = reader.ReadInt32()
                    };
                    sig.blocks = new UInt32[sig.numblocks];

                    for( int i = 0; i < sig.numblocks; i++ ) {
                        sig.blocks[i] = reader.ReadUInt32();
                    }

                    return sig;
                }
            }
        }

        static PlainBlockDirectory readPlainBlockDirectory( byte[] block ) {
            using( var mms = new MemoryStream( block ) ) {
                using( var reader = new BinaryReader( mms ) ) {
                    var sig = new PlainBlockDirectory {
                        numblocks = reader.ReadInt32()
                    };
                    sig.blocks = new UInt32[sig.numblocks];

                    for( int i = 0; i < sig.numblocks; i++ ) {
                        sig.blocks[i] = reader.ReadUInt32();
                    }

                    return sig;
                }
            }
        }
        static void Main( string[] args ) {

            var data1 = File.ReadAllBytes( @"..\..\..\1cv8.1CD" );
            var blocks = new List<byte[]>();
            for( int i = 0; i < data1.Length; i += 4096 ) {
                var na = new byte[4096];
                Array.Copy( data1, i, na, 0, 4096 );
                blocks.Add( na );
            }

            var sig = readSignature( blocks[0] );
            var freesHead = readBlockHead( blocks[1] );
            var blockHead = readBlockHead( blocks[2] );
            var plainPD = readPlainBlockDirectory( blocks[3] );
            var root = readRootHead( blocks[4] );

            var block = new byte[]{
0x1E,0x2D,0x78,0xBF,0x20,0xEB,0x57,0x34,0x6E,0x9F,0x4C,0x79,0x0F,0xD0,0x26,0x8A,
0x12,0xCF,0x05,0x97,0x65,0xA6,0x42,0xB6,0x7A,0x7F,0x57,0x28,0x1A,0x4F,0x4F,0xC2,
0xC3,0x00,0x5B,0xD9,0x66,0x04,0x5A,0xAF,0x2E,0x1D,0x38,0xFD,0x43,0xE8,0x25,0xAC,
0x28,0xA3,0x06,0x93,0x7B,0x9B,0x18,0x48,0x64,0x4C,0x37,0x2D,0x7B,0x1A,0x19,0xDD,
0x43,0x8D,0x6F,0x07,0x5A,0xFB,0x2E,0x55,0x2D,0x85,0x55,0xEF,0x60,0xFC,0x27,0xBB,
0x47,0x49,0xFD,0x0B,0x58,0x53,0x75,0x7D,0x69,0x2A,0x3D,0x1E,0x5A,0x93,0x10,0xDB,
0x67,0x04,0x5E,0xAF,0x7C,0x49,0x22,0xE0,0x16,0xBA,0x22,0xE2,0x35,0xA7,0x55,0x96,
0x6F,0x86,0x4A,0x4F,0x67,0x05,0x2A,0x7F,0x7F,0x1D,0x48,0x8F,0x10,0xDB,0x67,0x04,
0x5E,0xAF,0x60,0x74,0x05,0xAB,0x16,0xF7,0x3E,0xFF,0x35,0xA7,0x55,0x96,0x72,0x86,
0x4A,0x52,0x67,0x18,0x2A,0x7F,0x62,0x1D,0x48,0x8F,0x10,0xC6,0x67,0x04,0x5E,0xAF,
0x61,0x49,0x3F,0xE0,0x16,0xBA,0x22,0xFF,0x35,0xA7,0x55,0x96,0x72,0x9A,0x4A,0x53,
0x66,0x04,0x36,0x7F,0x63,0x1D,0x54,0x9D,0x71,0xA7,0x67,0x75,0x28,0xC8,0x01,0x30,
0x57,0xE8,0x68,0xD8,0x48,0x9B,0x4E,0xF2,0x0A,0xC0,0x7B,0xD5,0x22,0x0C,0x21,0x4A,
0x6C,0x3A,0x77,0x10,0x5A,0x93,0x02,0xBA,0x1B,0x04,0x2F,0xD9,0x1B,0x34,0x46,0x88,
0x1E,0xC4,0x40,0x95,0x51,0xDC,0x00,0xC9,0x24,0x8F,0x19,0x27,0x24,0x5E,0x78,0x39,
0x3A,0x15,0x45,0x9D,0x0C,0xD9,0x7B,0x07,0x42,0xAD,0x7C,0x48,0x3C,0xE0,0x17,0xBA,
0x21,0xFE,0x30,0xA2,0x56,0x95,0x76,0x9A,0x4A,0x02};

            using( var ms = new MemoryStream() ) {
                for( int i = block[0] + 1, j = 1; i < block.Length; i++ ) {
                    if( j > block[0] )
                        j = 1;

                    var r = block[i] ^ block[j];
                    ms.WriteByte( (byte)r );
                    j++;
                }
                var data = ms.ToArray();
                var row = Encoding.UTF8.GetString( data );
                Console.WriteLine( row );
            }
        }
    }
}
