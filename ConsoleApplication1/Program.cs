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
    };
    struct BlockHead {
        public string sig; // сигнатура “1CDBOBV8”
        public int length;
        public int version1;
        public int version2;
        public UInt32 version;
        public List<int> directoryBlocks; // 1018
    };
    struct RootHead {
        public string lang;
        public int numblocks;
        public List<int> directoryBlocks; // 1018
    };

    struct PlainBlockDirectory {
        public int numblocks;
        public List<int> directoryBlocks; // 1018
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
                        directoryBlocks = new List<int>()
                    };

                    for( int i = 0; i < 1018; i++ ) {
                        var r = reader.ReadInt32();
                        if( r == 0 )
                            break;
                        sig.directoryBlocks.Add(r);
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
                    sig.directoryBlocks = new List<int>();

                    for( int i = 0; i < sig.numblocks; i++ ) {
                        sig.directoryBlocks.Add(reader.ReadInt32());
                    }

                    return sig;
                }
            }
        }

        static int[] readPlainBlockDirectory( List<byte[]> blocks, List<int> directoryBlocks ) {
            var r = new List<int>();
            foreach( var b in directoryBlocks ) {
                using (var mms = new MemoryStream( blocks[b] )) {
                    using (var reader = new BinaryReader( mms )) {
                        var sig = new PlainBlockDirectory
                        {
                            numblocks = reader.ReadInt32()
                        };
                        sig.directoryBlocks = new List<int>();

                        for( int i = 0; i < sig.numblocks; i++ ) {
                            r.Add( reader.ReadInt32() );
                        }
                    }
                }
            }
            return r.ToArray();
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
        static byte[] concatBlocks( List<byte[]> blocks, int[] directory ) {
            var block = new byte[directory.Length * 4096];
            for( int i = 0; i < directory.Length; i++ ) {
                Array.Copy( blocks[directory[i]], 0, block, i * 4096, 4096 );
            }

            return block;
        }
        static IEnumerable<string> readStructures( List<byte[]> blocks ) {
            var blockHead = readBlockHead( blocks[2] );
            var plainPD = readPlainBlockDirectory( blocks, blockHead.directoryBlocks );
            var root = readRootHead( concatBlocks( blocks, plainPD ) );

            for( int i = 0; i < root.directoryBlocks.Count; i++ ) {
                var blocksa = readBlockHead( blocks[root.directoryBlocks[i]] );
                var blocksaPD = readPlainBlockDirectory( blocks, blocksa.directoryBlocks );

                yield return readStructureBlock( concatBlocks( blocks, blocksaPD ), blocksa.length );
            }
        }


        public abstract class BasicField {
            public string Name;
            public bool Nullable;

            public BasicField( string Name, bool Nullable ) {
                this.Nullable = Nullable;
                this.Name = Name;
            }

            public abstract int Size { get; }
            public string AsJson( BinaryReader r ) {
                return string.Format( "\"{0}\": {1}", Name, AsString( r ) );
            }
            public abstract string AsString( BinaryReader r );
        }

        public abstract class BasicFieldWithLength: BasicField {
            public int Length;

            public BasicFieldWithLength( string Name, bool Nullable, int Length ) : base( Name, Nullable ) {
                this.Length = Length;
            }
        }
        public class BinaryField: BasicFieldWithLength {
            public BinaryField( string Name, bool Nullable, int Length ) : base( Name, Nullable, Length ) { }

            public override int Size {
                get {
                    return Length;
                }
            }

            public override string AsString( BinaryReader r ) {
                r.ReadBytes( Size );
                return "\"<binary>\"";
            }
        }
        public class NumericField: BasicFieldWithLength {
            public int Precision;
            public NumericField( string Name, bool Nullable, int Length, int Precision ) : base( Name, Nullable, Length ) { this.Precision = Precision; }

            public override int Size {
                get {
                    return ( ( Length + 2 ) / 2 );
                }
            }

            public override string AsString( BinaryReader r ) {
                r.ReadBytes( Size );
                return "\"<numeric>\"";
            }
        }
        public class BooleanField: BasicField {
            public BooleanField( string Name, bool Nullable ) : base( Name, Nullable ) { }
            public override int Size {
                get {
                    return 1;
                }
            }

            public override string AsString( BinaryReader r ) {
                return r.ReadBoolean().ToString();
            }
        }
        public class VersionField: BasicField {
            public VersionField( string Name, bool Nullable ) : base( Name, Nullable ) { }
            public override int Size {
                get {
                    return 16;
                }
            }

            public override string AsString( BinaryReader r ) {
                r.ReadBytes( Size );
                return "\"<version>\"";
            }
        }
        public class HiddenShortVersionField: BasicField {
            public HiddenShortVersionField() : base( "HiddenVersion", false ) { }
            public override int Size {
                get {
                    return 8;
                }
            }

            public override string AsString( BinaryReader r ) {
                r.ReadBytes( Size );
                return "\"<version>\"";
            }
        }
        public class StringField: BasicFieldWithLength {
            public StringField( string Name, bool Nullable, int Length ) : base( Name, Nullable, Length ) { }
            public override int Size {
                get {
                    return Length * 2;
                }
            }

            public override string AsString( BinaryReader r ) {
                return "\"" + Encoding.Unicode.GetString( r.ReadBytes( Size ) ) + "\"";
            }
        }
        public class VarStringField: BasicFieldWithLength {
            public VarStringField( string Name, bool Nullable, int Length ) : base( Name, Nullable, Length ) { }
            public override int Size {
                get {
                    return Length * 2 + 2;
                }
            }

            public override string AsString( BinaryReader r ) {
                var l = Size - 2;
                var length = (int)r.ReadUInt16();
                if( length > Length )
                    length = Length;
                length *= 2;
                l -= length;
                var res = Encoding.Unicode.GetString( r.ReadBytes( length ) );
                r.ReadBytes( l );
                return "\"" + res + "\"";
            }
        }
        public class TextField: BasicField {
            public TextField( string Name, bool Nullable ) : base( Name, Nullable ) { }
            public override int Size {
                get {
                    return 8;
                }
            }

            public override string AsString( BinaryReader r ) {
                r.ReadBytes( Size );
                return "\"<large text>\"";
            }
        }
        public class BlobField: BasicField {
            public BlobField( string Name, bool Nullable ) : base( Name, Nullable ) { }
            public override int Size {
                get {
                    return 8;
                }
            }

            public override string AsString( BinaryReader r ) {
                r.ReadBytes( Size );
                return "\"<blob>\"";
            }
        }
        public class DateTimeField: BasicField {
            public DateTimeField( string Name, bool Nullable ) : base( Name, Nullable ) { }
            public override int Size {
                get {
                    return 7;
                }
            }

            public override string AsString( BinaryReader r ) {
                r.ReadBytes( Size );
                return "\"<dateTime>\"";
            }
        }



        class TableInfo {
            public string Name;
            public List<BasicField> Fields = new List<BasicField>();
            public int DataDirectoryBlock;
            public int BlobDirectoryBlock;

            public int Size { get { var s = Fields.Sum( vx => vx.Size ) + Fields.Count( vx => vx.Nullable ); return s < 4 ? 4 : s; } }
        }

        static TableInfo exportStructure( string structure ) {
            var reName = new Regex( "^{\"(?<name>[^\"]+)\",\\d+,$" );
            var reFieldsStart = new Regex( "^{\"Fields\",$" );
            var reField = new Regex( "^{\"(?<FieldName>[^\"]+)\",\"(?<FieldType>[^\"]+)\",(?<NullExists>\\d+),(?<FieldLength>\\d+),(?<FieldPrecision>\\d+),\"(?<FieldCaseSensitive>[^\"]+)\"}" );
            var reRecordlock = new Regex( "^{\"Recordlock\"," );
            var reFiles = new Regex( "^{\"Files\",(?<data>\\d+),(?<blobs>\\d+),(\\d+)}$" );
            var rows = structure.Split( new string[] { "\r\n", "\n" }, StringSplitOptions.RemoveEmptyEntries );

            var tableInfo = new TableInfo();
            tableInfo.Name = reName.Match( rows[0] ).Groups["name"].Value;

            if( !reFieldsStart.Match( rows[1] ).Success )
                throw new InvalidDataException();

            int i = 2;
            for( var match = reField.Match( rows[i] ); match.Success; match = reField.Match( rows[i] ) ) {
                var name = match.Groups["FieldName"].Value;
                var nullable = match.Groups["NullExists"].Value == "1";
                var length = int.Parse( match.Groups["FieldLength"].Value );
                var fieldPrecision = int.Parse( match.Groups["FieldPrecision"].Value );

                switch( match.Groups["FieldType"].Value ) {
                    case "B": tableInfo.Fields.Add( new BinaryField( name, nullable, length ) ); break;
                    case "L": tableInfo.Fields.Add( new BooleanField( name, nullable ) ); break;
                    case "N": tableInfo.Fields.Add( new NumericField( name, nullable, length, fieldPrecision ) ); break;
                    case "NC": tableInfo.Fields.Add( new StringField( name, nullable, length ) ); break;
                    case "NVC": tableInfo.Fields.Add( new VarStringField( name, nullable, length ) ); break;
                    case "RV": tableInfo.Fields.Insert( 0, new VersionField( name, nullable ) ); break;
                    case "NT": tableInfo.Fields.Add( new TextField( name, nullable ) ); break;
                    case "I": tableInfo.Fields.Add( new BlobField( name, nullable ) ); break;
                    case "DT": tableInfo.Fields.Add( new DateTimeField( name, nullable ) ); break;
                    default:
                        throw new InvalidDataException();
                }

                i++;
            }

            for( ; i < rows.Length; i++ ) {
                var matchA = reFiles.Match( rows[i] );
                var matchB = reRecordlock.Match( rows[i] );

                if( matchA.Success ) {
                    tableInfo.BlobDirectoryBlock = int.Parse( matchA.Groups["blobs"].Value );
                    tableInfo.DataDirectoryBlock = int.Parse( matchA.Groups["data"].Value );
                }

                if( matchB.Success ) {
                    var rl = rows[i];
                    if( rl == "{\"Recordlock\",\"0\"}," )
                        continue;
                    tableInfo.Fields.Insert( 0, new HiddenShortVersionField() );
                }
            }

            return tableInfo;
        }

        static IEnumerable<Tuple<bool, byte[]>> loadTableData( int dataObjectBlock, int Size, List<byte[]> blocks ) {
            var result = new List<Tuple<bool, byte[]>>();

            if( dataObjectBlock == 0 )
                return result;

            var blockHead = readBlockHead( blocks[dataObjectBlock] );
            if( blockHead.length == 0 )
                return result;

            var plainPD = readPlainBlockDirectory( blocks, blockHead.directoryBlocks );

            using (var ms = new MemoryStream( concatBlocks( blocks, plainPD ) )) {
                using (var reader = new BinaryReader( ms )) {
                    for( var remain = blockHead.length; remain > 0; remain -= Size + 1 ) {
                        var free = reader.ReadBoolean();
                        result.Add( Tuple.Create( free, reader.ReadBytes( Size ) ) );

                        if( remain < Size + 1 )
                            throw new InvalidOperationException();
                    }
                }
            }

            return result;
        }

        static byte[] loadBlobsData( int blobObjectBlock, List<byte[]> blocks ) {
            if( blobObjectBlock == 0 )
                return null;

            return null;
        }

        static string readValue( BasicField fi, BinaryReader reader ) {
            if( fi.Nullable ) {
                if( reader.ReadByte() == 0 ) {
                    reader.ReadBytes( fi.Size );
                    return string.Format( "\"{0}\": {1}", fi.Name, "null" );
                }
            }

            return fi.AsJson( reader );
        }


        static IEnumerable<string> loadTable( TableInfo ti, List<byte[]> blocks ) {
            var ddd = loadTableData( ti.DataDirectoryBlock, ti.Size, blocks ).ToArray();
            foreach( var data in ddd ) {
                if( data.Item1 )
                    continue;

                using (var ms = new MemoryStream( data.Item2 )) {
                    using (var reader = new BinaryReader( ms )) {
                        var result = string.Join( ",", ti.Fields.Select( vx => readValue( vx, reader ) ) );

                        yield return result;
                    }
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

            var structures = readStructures( blocks ).Select( vx => exportStructure( vx ) ).ToArray();
            foreach( var s in structures ) {
                var tableData = loadTable( s, blocks ).ToArray();
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
