using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace ConsoleApplication1 {
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
        public int length;
        public int version1;
        public int version2;
        public UInt32 version;
        public int[] directoryBlocks; // 1018
    };
    struct RootHead {
        public string lang;
        public int numblocks;
        public int[] directoryBlocks; // 1018
    };

    struct PlainBlockDirectory {
        public int numblocks;
        public int[] directoryBlocks; // 1023
    };

    class Program {
        static Signature readSignature( byte[] block ) {
            using (var mms = new MemoryStream( block )) {
                using (var reader = new BinaryReader( mms )) {
                    var sig = new Signature
                    {

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
            using (var mms = new MemoryStream( block )) {
                using (var reader = new BinaryReader( mms )) {
                    var sig = new BlockHead
                    {

                        sig = Encoding.ASCII.GetString( reader.ReadBytes( 8 ) ),
                        length = reader.ReadInt32(),
                        version1 = reader.ReadInt32(),
                        version2 = reader.ReadInt32(),
                        version = reader.ReadUInt32(),
                        directoryBlocks = new int[1018]
                    };

                    for( int i = 0; i < 1018; i++ ) {
                        sig.directoryBlocks[i] = reader.ReadInt32();
                    }

                    return sig;
                }
            }
        }
        static RootHead readRootHead( params byte[] data ) {
            using (var mms = new MemoryStream( data )) {
                using (var reader = new BinaryReader( mms )) {
                    var sig = new RootHead
                    {
                        lang = Encoding.ASCII.GetString( reader.ReadBytes( 32 ) ),
                        numblocks = reader.ReadInt32()
                    };
                    sig.directoryBlocks = new int[sig.numblocks];

                    for( int i = 0; i < sig.numblocks; i++ ) {
                        sig.directoryBlocks[i] = reader.ReadInt32();
                    }

                    return sig;
                }
            }
        }

        static PlainBlockDirectory readPlainBlockDirectory( byte[] block ) {
            using (var mms = new MemoryStream( block )) {
                using (var reader = new BinaryReader( mms )) {
                    var sig = new PlainBlockDirectory
                    {
                        numblocks = reader.ReadInt32()
                    };
                    sig.directoryBlocks = new int[sig.numblocks];

                    for( int i = 0; i < sig.numblocks; i++ ) {
                        sig.directoryBlocks[i] = reader.ReadInt32();
                    }

                    return sig;
                }
            }
        }

        static string readStructureBlock( byte[] block, int Size ) {
            using (var mms = new MemoryStream( block )) {
                using (var reader = new BinaryReader( mms )) {
                    return Encoding.Unicode.GetString( reader.ReadBytes( Size ) );
                }
            }
        }

        static byte[][] joinBlocks( List<byte[]> blocks, PlainBlockDirectory directory ) {
            return directory.directoryBlocks.Select( vx => blocks[vx] ).ToArray();
        }
        static byte[] concatBlocks( List<byte[]> blocks, PlainBlockDirectory directory ) {
            var block = new byte[directory.directoryBlocks.Length * 4096];
            for( int i = 0; i < directory.directoryBlocks.Length; i++ ) {
                Array.Copy( blocks[directory.directoryBlocks[i]], 0, block, i * 4096, 4096 );
            }

            return block;
        }
        static IEnumerable<string> readStructures( List<byte[]> blocks ) {
            var blockHead = readBlockHead( blocks[2] );
            var plainPD = readPlainBlockDirectory( blocks[blockHead.directoryBlocks[0]] );
            var root = readRootHead( concatBlocks( blocks, plainPD ) );

            for( int i = 0; i < root.directoryBlocks.Length; i++ ) {
                var blocksa = readBlockHead( blocks[root.directoryBlocks[i]] );
                var blocksaPD = readPlainBlockDirectory( blocks[blocksa.directoryBlocks[0]] );

                yield return readStructureBlock( concatBlocks( blocks, blocksaPD ), blocksa.length );
            }
        }


        /*
         * {"IBVERSION",0,
         * {"Fields",
         * {"IBVERSION","N",0,10,0,"CS"},
         * {"PLATFORMVERSIONREQ","N",0,10,0,"CS"}
         * },
         * {"Indexes"},
         * {"Recordlock","0"},
         * {"Files",6,0,0}
         * }
        */


        /// <summary>
        /// Далее, размер и формат поля зависит от типа поля. Типы поля бывают такими:
        /// </summary>
        /// <param name="B">двоичные данные. Длина поля равна FieldLength байт.</param>
        /// <param name="L">булево. Длина поля 1 байт. Нулевое значение байта означает Ложь, иначе Истина.</param>
        /// <param name="N">число. Длина поля в байтах равна Цел((FieldLength + 2) / 2). Числа хранятся в двоично-десятичном виде. Первый полубайт означает знак числа. 0 – число отрицательное, 1 – положительное. Каждый следующий полубайт соответствует одной десятичной цифре. Всего цифр FieldLength. Десятичная точка находится в FieldPrecision цифрах справа. Например, FieldLength = 5, FieldPrecision = 3. Байты 0x18, 0x47, 0x23 означают число 84.723, а байты 0x00, 0x00, 0x91 представляют число -0.091.</param>
        /// <param name="NC">строка фиксированной длины. Длина поля равна FieldLength * 2 байт. Представляет собой строку в формате Unicode (каждый символ занимает 2 байта).</param>
        /// <param name="NVC">строка переменной длины. Длина поля равна FieldLength * 2 + 2 байт. Первые 2 байта содержат длину строки (максимум FieldLength). Оставшиеся байты представляет собой строку в формате Unicode (каждый символ занимает 2 байта).</param>
        /// <param name="RV">версия. Длина поля 16 байт. Предположительно содержит четыре числа int.</param>
        /// <param name="NT">строка неограниченной длины. Длина поля 8 байт. Первые четыре байта содержат начальный индекс блока в объекте Blob таблицы, вторые четыре – длину данных в объекте Blob. В объекте Blob содержится строка в формате Unicode.</param>
        /// <param name="I">двоичные данные неограниченной длины. Длина поля 8 байт. Первые четыре байта содержат начальный индекс блока в объекте Blob таблицы, вторые четыре – длину данных в объекте Blob.</param>
        /// <param name="DT">дата-время. Длина поля 7 байт. Содержит данные в двоично-десятичном виде. Первые 2 байта содержат четыре цифры года, третий байт – две цифры месяца, четвертый байт – день, пятый – часы, шестой – минуты и седьмой – секунды, все также по 2 цифры.</param>
        /// 

        public enum FieldTypes {
            BinaryData,
            Boolean,
            Numeric,
            String,
            VarString,
            Version,
            Text,
            Blob,
            DateTime
        }
        public class FieldInfo {
            public string Name;
            public FieldTypes Type;
            public bool Nullable;
            public int Length;
            public int FieldPrecision;
            public string CaseSensitive;
        }

        class TableInfo {
            public string Name;
            public List<FieldInfo> Fields = new List<FieldInfo>();
            public int DataDirectoryBlock;
            public int BlobDirectoryBlock;
        }

        static TableInfo exportStructure( string structure ) {
            var reName = new Regex( "^{\"(?<name>[^\"]+)\",\\d+,$" );
            var reFieldsStart = new Regex( "^{\"Fields\",$" );
            var reField = new Regex( "^{\"(?<FieldName>[^\"]+)\",\"(?<FieldType>[^\"]+)\",(?<NullExists>\\d+),(?<FieldLength>\\d+),(?<FieldPrecision>\\d+),\"(?<FieldCaseSensitive>[^\"]+)\"}" );
            var reFiles = new Regex( "^{\"Files\",(?<data>\\d+),(?<blobs>\\d+),(\\d+)}$" );
            var rows = structure.Split( new string[] { "\r\n", "\n" }, StringSplitOptions.RemoveEmptyEntries );

            var tableInfo = new TableInfo();
            tableInfo.Name = reName.Match( rows[0] ).Groups["name"].Value;

            if( !reFieldsStart.Match( rows[1] ).Success )
                throw new InvalidDataException();

            int i = 2;
            for( var match = reField.Match( rows[i] ); match.Success; match = reField.Match( rows[i] ) ) {
                var fi = new FieldInfo();
                fi.Name = match.Groups["FieldName"].Value;
                switch( match.Groups["FieldType"].Value ) {
                    case "B": fi.Type =FieldTypes.BinaryData ; break;
                    case "L": fi.Type = FieldTypes.Boolean; break;
                    case "N": fi.Type = FieldTypes.Numeric; break;
                    case "NC": fi.Type = FieldTypes.String; break;
                    case "NVC": fi.Type = FieldTypes.VarString; break;
                    case "RV": fi.Type = FieldTypes.Version; break;
                    case "NT": fi.Type = FieldTypes.Text; break;
                    case "I": fi.Type = FieldTypes.Blob; break;
                    case "DT": fi.Type = FieldTypes.DateTime; break;
                    default:
                        throw new InvalidDataException();
                }

                fi.Nullable = match.Groups["NullExists"].Value == "1";
                fi.Length = int.Parse( match.Groups["FieldLength"].Value );
                fi.FieldPrecision = int.Parse( match.Groups["FieldPrecision"].Value );
                fi.CaseSensitive = match.Groups["FieldCaseSensitive"].Value;

                tableInfo.Fields.Add( fi );
                i++;
            }

            for( ; i < rows.Length; i++ ) {
                var match = reFiles.Match( rows[i] );

                if( match.Success ) {
                    tableInfo.BlobDirectoryBlock = int.Parse( match.Groups["blobs"].Value );
                    tableInfo.DataDirectoryBlock = int.Parse( match.Groups["data"].Value );
                }
            }

            return tableInfo;
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

            var structures = readStructures( blocks ).Select( vx => exportStructure( vx ) ).ToArray();
            foreach( var s in structures ) {
                //                exportStructure( s );
            }

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

            using (var ms = new MemoryStream()) {
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
