/* ============================================================================

NAME: UPdirTest00.js
REVISION: $Revision: 19 $

AUTHOR: $Author: brix8x $ (����� ������� �������� PremierEx)
DATE  : $Date: 2008-03-31 16:46:17 +0400 (Пн, 31 мар 2008) $

COMMENT:

�����������:

��� ���������� ����� ���������� ������� ����� V8Unpack.exe, ������� ����� ����������
1. � �������� ���������� �����
2. � �������� ..\BIN (������������ �������� ���������� �����)
3. � �������� \BIN (������������ �������� ���������� �����)
4. � �������� � ������������ �������

���� ���������� � ������� CF �� ������, �� ����������� �� ��������� ���������
������� �������

������ ������ ��������� ������� ������� ������� CF ����� - UNPACK-PACK (level=0)

============================================================================ */

/* ============================================================================
�������� ���� ������� �����

1. ����������� CF-���� (TestCF) ������� ����������� �� (TestIB)
2. ���������� 1� ������������ �������� TestIB � ����������� ����� �������� (TestLog)
3. TestCF ��������������� � ������ UNPACK � ������� ���������� (UnpackDir)
4. �� UnpackDir ��� ������ ����� PACK ��������� ����� CF-���� (NewCF)
5. NewCF ������� ����� �� (NewIB)
6. ���������� 1� ������������ �������� NewIB � ����������� ����� �������� (NewLog)
7. ������ �������� �������� ��������� ��������� ������ TestCF vs NewCF � TestLog vs NewLog

============================================================================ */

/* ============================================================================

������� ������:
1. �������, � ������� ��������� ����������� ����� *.cf
2. ���� /TESTIB - ���������� ����������� ��, ����������� �� ������ CF
3. ���� /PARSE - ���������� ������������ ��� ���������� � �������� ������ ������ PARSE\BUILD

============================================================================ */

// ============================================================================
var oShell = new ActiveXObject("WScript.Shell");
var oFS = new ActiveXObject("Scripting.FileSystemObject");

var oError		= new Error;

var sTempDir	= new String();
var sCurrentDir	= new String();
var sCFDir	 	= new String();
var sV8Unpack	= new String();
var sV8Path		= new String();
var sLogName	= new String();

var oLog	 	= null;
var nWndStyle   = 10; //0 - hide

var bTESTIB		= false;
var bParseMode	= false;	//	����� ������������� ������ PARSE\BUILD ������ UNPACK\PACK

WScript.Quit(main())
// ============================================================================

// ============================================================================
function main()
{
	
	if(!Init())
	{
		e = oError; 
		WScript.Echo("������ ������������� �����. " + e.message);
		return e.number;
	}

	if(!Run())
	{
		e = oError; 
		WScript.Echo("������ ���������� �����. " + e.message);
		return e.number;
	}
	
	if(!Done())
	{
		e = oError; 
		WScript.Echo("������ ��������������� �����. " + e.message);
		return e.number;
	}
	
	return 0;
}	
// ============================================================================

// ============================================================================
function Init()
{
	// � ���� ������ ������������ ������������� �����, ��������, �������� ��������� ���������
	var bResult = true;
	
	try
	{
		//{ ��������� �������� �������
		sCurrentDir = oFS.GetParentFolderName(WScript.ScriptFullName);
		//}
		
		//{ ���������� �������� � CF-�������
		if (WScript.Arguments.Count())
		{ 
			sCFDir = WScript.Arguments.Item(0);
			
			if(!oFS.FolderExists(sCFDir))
				throw new Error(-1, "������� '" + sCFDir + "' �� ������.");
				
			for (var i=1; i<WScript.Arguments.Count(); i++) {
				if ("/TESTIB" == WScript.Arguments.Item(i)) {
					bTESTIB = true;	
				}
				if ("/PARSE" == WScript.Arguments.Item(i)) {
					bParseMode = true;	
				}
			}	
		}	
		else
		{
				sCFDir = sCurrentDir;
		}
		//}
		
		//{ �������� ������� ����� V8Unpack.exe
		arrPaths = new Array(sCurrentDir, oFS.GetParentFolderName(sCurrentDir) + "\\bin", sCurrentDir + "\\bin", sCFDir);
		for(nIndex = 0; nIndex < arrPaths.length; nIndex++)
		{
			if(oFS.FileExists(arrPaths[nIndex] + "\\V8Unpack.exe"))
			{
				sV8Unpack = arrPaths[nIndex] + "\\V8Unpack.exe";
				break;
			}
		}
		
		if(!sV8Unpack.length)
			throw new Error(-3, "���� V8Unpack.exe �� ������.");
		//}
		
		//{ ���������� ���� � ����� 1cv8.exe
		sV8Path = Get1CPath();
		if(!sV8Path.length)
			throw oError;
		//}
		
		//{ �������� ���������� ��������
		sTempDir = sCurrentDir + "\\" + oFS.GetTempName();
		if(!oFS.FolderExists(sTempDir))
			if("object" != typeof(oFS.CreateFolder(sTempDir)))	
				throw new Error(-2, "��� �������� ����������� '" + sTempDir + "' ��������� ������.");
		//}
		
		//{ �������� ����� ������
		sLogName = WScript.ScriptFullName.replace(".js", ".txt");
		oLog = oFS.CreateTextFile(sLogName, true);
			if("object" != typeof(oLog))
				throw new Error(-4, "��������� ������� �������� ����� ������.");
		//}
	}
	catch(e)
	{
		// �������� ������ ����� � ��������� ������
		// � ���������� false
		oError = e;
		bResult = false;		
	}
	return bResult;
}

function Done()
{	
	var bResult = true;
	try
	{
		oLog.Close();
		oShell.Run(sLogName, nWndStyle, false);

//		if(oFS.FolderExists(sTempDir))
//			oFS.DeleteFolder(sTempDir, true);
	}
	catch(e)
	{
		// �������� ������ ����� � ��������� ������
		// � ���������� false
		oError = e;
		bResult = false;		
	}
	return bResult;
}

function Run()
{

	var bResult = true;

	try
	{
		//{ ������������ ����� ����
		oLog.WriteLine("====================================================");
		oLog.WriteLine("������: "+Date());
		oLog.WriteLine("");
		oLog.WriteLine("���������� �������: '"+sCurrentDir+"'");
		oLog.WriteLine("���������� CF     : '"+sCFDir+"'");
		oLog.WriteLine("���� � V8Unpack   : '"+sV8Unpack+"'");
		oLog.WriteLine("���� � 1�         : '"+sV8Path+"'");
		oLog.WriteLine("");
		oLog.WriteLine("����������� ��    : '"+bTESTIB+"'");
		oLog.WriteLine("���. PARSE-BUILD  : '"+bParseMode+"'");
		oLog.WriteLine("====================================================");
		//}
		
		//{ �������� ��� ������� ����� � �������� cCFDir ������� ������������
		var arResults = ForEachFileInFolder(sCFDir, "*.cf", Test);	
		//}
		
		//{ ������������ ������� ����
		oLog.WriteLine("");
		oLog.WriteLine("====================================================");
		oLog.WriteLine("����������: "+Date());
		oLog.WriteLine("");

		for (var i = 0; i < arResults[1].length; i++)
		{
			oLog.WriteLine(arResults[0][i] + " : " + arResults[1][i]);
		}					
		
		oLog.WriteLine("====================================================");
		//}
	}
	catch(e)
	{
		// �������� ������ ����� � ��������� ������
		// � ���������� false
		oError = e;
		bResult = false;		
	}

	return bResult;
}
// ============================================================================

// ============================================================================
function Test(sFileName)
{
	var sResult = "";
	var sTestCFDir = new String();
	var sUnpackDir = new String();

	var sCF 	 = sFileName;
	
	try
	{
		oLog.WriteLine("");
		oLog.WriteLine("### ������������ ����� ### " + sFileName);
		
		//{ �������� �������� ����� ����������� ����� CF
		sTestCFDir = sTempDir + "\\" + oFS.GetBaseName(sFileName);
		if(!oFS.FolderExists(sTestCFDir))
			if("object" != typeof(oFS.CreateFolder(sTestCFDir)))	
				return("��������� ������� �������� �������� ����� ��� CF-�����.");
		//}
		
		//{ �������� � �������� �� TestIB
		if (bTESTIB)
		{
			var sTestIBDir = sTestCFDir + "\\TestIBDir";
			var sTestLog = sTestCFDir + "\\TestIBDir.txt";
		
			var res = CreateIB(sV8Path, sCF, sTestIBDir, sTestLog);
			if (0 != res) {
				oLog.WriteLine(sCmdLine);
				oLog.WriteLine("res = "+res);
				return("TestIB: ������ �������� � �������� ��: "+res);
			}	
		}	
		//}
		
		//{ ����������� ������� ���������� � ��������
		if (bParseMode) {
			sUnpackMode = "-PARSE";
			sPackMode = "-BUILD";
		}	
		else {
			sUnpackMode = "-UNPACK";
			sPackMode = "-PACK";
		}			
		//}
		
		//{ ���������� CF ����� � ������� ����������
		sUnpackDir = sTestCFDir + "\\udir";
		
		var sCmdLine = "\"" + sV8Unpack + "\" "+ sUnpackMode +" \"" + sCF + "\" \"" + sUnpackDir + "\"";
		
		var res = oShell.Run(sCmdLine, 10, true);
		if (0 != res) {
			oLog.WriteLine(sCmdLine);
			oLog.WriteLine("res = "+res);
			return("UNPACK error number "+res);
		}	
		//}
		
		//{ �������� ������ CF ����� �� �������� ����������
		var sNewCF 	 = sTestCFDir + "\\2" + oFS.GetFileName(sFileName);
		sCmdLine = "\"" + sV8Unpack + "\"" + sPackMode + " \"" + sUnpackDir + "\" \"" + sNewCF + "\"";
		oShell.Run(sCmdLine, 10, true);
		if (0 != res) {
			oLog.WriteLine(sCmdLine);
			oLog.WriteLine("res = "+res);
			return("PACK error number "+res);
		}	
		
		if(!oFS.FileExists(sNewCF))
			return ("����� CF-���� �� ��� ������ �� ����: '" + sNewCF + "'");
		//}
		
		//{ �������� � �������� �� NewIB
		if (bTESTIB)
		{
			var sNewIBDir = sTestCFDir + "\\NewIBDir";
			var sNewLog = sTestCFDir + "\\NewIBDir.txt";
		
			var res = CreateIB(sV8Path, sNewCF, sNewIBDir, sNewLog);
			if (0 != res) {
				oLog.WriteLine(sCmdLine);
				oLog.WriteLine("res = "+res);
				return("NewIB : ������ �������� � �������� ��: "+res);
			}	
		}	
		//}
		
		//{ ��������� ������ CF � ������ ��������
		if (oFS.GetFile(sCF).Size == oFS.GetFile(sNewCF).Size)
		{
			sResult = "[CF - OK]"
		}	
		else
		{
			sResult = "[TestCF="+oFS.GetFile(sCF).Size+" "+"NewCF="+oFS.GetFile(sNewCF).Size+"]"
		}
			
		if (bTESTIB)
		{
			if (oFS.GetFile(sTestLog).Size == oFS.GetFile(sNewLog).Size)
			{
				sResult = sResult+" [IB - OK]"
			}	
			else
			{
				sResult = sResult+" [sTestLog="+oFS.GetFile(sTestLog).Size+" "+"sNewLog="+oFS.GetFile(sNewLog).Size+"]"
			}
		}	
		//}

		oLog.WriteLine("##########################");
		oLog.WriteLine("");
	}
	catch(e)
	{
		oError = e;
		sResult = "Test: �������������� ������: "+e.Description;
	}

	return sResult;	
}

function CreateIB(sV8Path, sCfFile, sIBDir, sOutLog)
{
	var sResult = "";
	
	try
	{
		//{ �������� ������ ������������
		var sCmdLine = "\"" + sV8Path + "\" CREATEINFOBASE \"File=" + sIBDir + "\" /Out\"" + sOutLog + "\"";
		var res = oShell.Run(sCmdLine, nWndStyle, true);
		if (0 != res) {
			oLog.WriteLine(sCmdLine);
			oLog.WriteLine("res = "+res);
			oLog.WriteLine(readLog(sOutLog));
			return("CREATE EMPTY IB error "+res);
		}	
		//}
		
		//{ �������� ������������
		var sCmdLine = "\"" + sV8Path + "\" CONFIG /F \"" + sIBDir + "\" /Out\"" + sOutLog + "\" /LoadCfg\"" + sCfFile + "\"";
		var res = oShell.Run(sCmdLine, nWndStyle, true);
		if (0 != res) {
			oLog.WriteLine(sCmdLine);
			oLog.WriteLine("res = "+res);
			oLog.WriteLine(readLog(sOutLog));
			return("LOADCFG IB error "+res);
		}	
		//}
		
		//{ �������� �� ���������� 1�
		var sCmdLine = "\"" + sV8Path + "\" CONFIG /F \"" + sIBDir + "\" /Out\"" + sOutLog + "\" -NoTruncate /CheckConfig";
		var res = oShell.Run(sCmdLine, nWndStyle, true);
		if (0 != res) {
			oLog.WriteLine(sCmdLine);
			oLog.WriteLine("res = "+res);
			oLog.WriteLine(readLog(sOutLog));
			return("CHECKCONFIG IB error "+res);
		}	
	}
	catch(e)
	{
		oError = e;
		sResult = "CreateIB error: "+e.Description;
	}
	
	return sResult;	
}

function readLog(sLogFileName)
{
	if (!oFS.FileExists(sLogFileName))
		return "�� ������ ���� ���� '" + sLogFileName + "'";
		
	var oOutLog = oFS.OpenTextFile(sLogFileName, 1, false);
	if("object" != typeof(oOutLog))
		return "������ ������ ����� ����";
	
	return oOutLog.ReadAll();
}
// ============================================================================

// ============================================================================
function ForEachFileInFolder(sDir, sMask, sFunction)
{
	var arResults = new Array(new Array(), new Array());
	
	try
	{
		var oFolder = oFS.GetFolder(sDir);
		var cFiles  = new Enumerator(oFolder.Files);
		
		var nNamedArgsCount = Math.min(ForEachFileInFolder.length, arguments.length);
		var nUnnamedArgsCountCount = arguments.length - nNamedArgsCount;
		
		// transform mask to RegExp
		var sRegExp = "^"+sMask+"$";
		sRegExp = sRegExp.replace("\.", "\\.");
		sRegExp = sRegExp.replace("\*", ".*");
		// transform mask to RegExp
	
		// ������������ ������ ������������ ���������� ��� �������
		// ������ �������� - ������ sFullname, � ������������ ����� �����
		var sArgsList = "sFullName";
		for (i = 0; i < nUnnamedArgsCountCount; i++)
		{
			sArgsList += (sArgsList == "") ? "" : ",";
			sArgsList +=  "arguments[" + (nNamedArgsCount + i) + "]";
		}
		// ������������ ������ ������������ ���������� ��� �������
		
		var oRegExp = new RegExp(sRegExp,"i");
		
		for (; !cFiles.atEnd(); cFiles.moveNext())
		{
			var sName = cFiles.item().Name;

			if (oRegExp.test(sName)) 
			{
				var sFullName = sDir + "\\" + cFiles.item().Name;
				eval("var bResult = sFunction(" + sArgsList + ")");
				arResults[0][arResults[0].length] = cFiles.item().Path;
				arResults[1][arResults[1].length] = bResult;
			}

		}

	}
	catch(e)
	{
		oError = e;
	}	
	
	return arResults;
}

function Get1CPath()
{
	var s1CPath = new String();
	
	try
	{
		var sCLSID = oShell.RegRead("HKCR\\V81.Application\\CLSID\\");
		if("undefined" == typeof(sCLSID))
			throw new Error(-1, "��� ��������� ������ ������� 'V81.Application' �� ���������� ������� ��������� ������.");

		var sValue = oShell.RegRead("HKCR\\CLSID\\" + sCLSID + "\\LocalServer32\\");
		if(!oFS.FileExists(sValue))
			throw new Error(-2, "���� '" + sValue + "' �� ������.");

		s1CPath = sValue;	
	}
	catch(e)
	{
		oError = e;
	}
	
	return s1CPath;
}
// ============================================================================
