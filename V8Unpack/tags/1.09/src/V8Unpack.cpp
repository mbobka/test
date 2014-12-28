/////////////////////////////////////////////////////////////////////////////
//
//
//	Author:			disa_da
//	E-mail:			disa_da2@mail.ru
//
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// $Revision: 10 $
// $Author: brix8x $
// $Date: 2008-03-14 17:58:47 +0300 (Пт, 14 мар 2008) $
//
/////////////////////////////////////////////////////////////////////////////


// V8Unpack.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "V8Unpack.h"
#include "V8File.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

CWinApp theApp;

using namespace std;

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		cerr << _T("Fatal Error: MFC initialization failed") << endl;
		nRetCode = 1;
		return nRetCode;
	}


	CString CurrMode(argv[1]);
	CurrMode.MakeLower();

	int err = 0;

	if(CurrMode == "-unpack" || CurrMode == "-unp")
	{

		CV8File V8File;

		err = V8File.UnpackToFolder(argv[2], argv[3], argv[4], true);

		if (err)
			cout << "Error!";

		return err;
	}

	if(CurrMode == "-pack" || CurrMode == "-p")
	{

		CV8File V8File;

		V8File.PackFromFolder(argv[2], argv[3]);


		return nRetCode;
	}

	if(CurrMode == "-undeflate" || CurrMode == "-und")
	{

		CV8File V8File;

		V8File.Undeflate(argv[2], argv[3]);


		return nRetCode;
	}

	if(CurrMode == "-deflate" || CurrMode == "-d")
	{

		CV8File V8File;

		V8File.Deflate(argv[2], argv[3]);

		return nRetCode;
	}

	if(CurrMode == "-bat")
	{
		
		cout << "if %1 == P GOTO PACK" << endl;
		cout << "if %1 == p GOTO PACK" << endl;
		cout << endl;
		cout << endl;
		cout << ":UNPACK" << endl;
		cout << "V8Unpack.exe -unpack      %2                              %2.unp" << endl;
		cout << "V8Unpack.exe -undeflate   %2.unp\\metadata.data            %2.unp\\metadata.data.und" << endl;
		cout << "V8Unpack.exe -unpack      %2.unp\\metadata.data.und        %2.unp\\metadata.unp" << endl;
		cout << "GOTO END" << endl;
		cout << endl;
		cout << endl;
		cout << ":PACK" << endl;
		cout << "V8Unpack.exe -pack        %2.unp\\metadata.unp            %2.unp\\metadata_new.data.und" << endl;
		cout << "V8Unpack.exe -deflate     %2.unp\\metadata_new.data.und   %2.unp\\metadata.data" << endl;
		cout << "V8Unpack.exe -pack        %2.unp                         %2.new.cf" << endl;
		cout << endl;
		cout << endl;
		cout << ":END" << endl;

		return nRetCode;
	}

	if(CurrMode == "-example" || CurrMode == "-ex")
	{
		
		cout << endl;
		cout << endl;
		cout << "UNPACK" << endl;
		cout << "V8Unpack.exe -unpack      1Cv8.cf                         1Cv8.unp" << endl;
		cout << "V8Unpack.exe -undeflate   1Cv8.unp\\metadata.data          1Cv8.unp\\metadata.data.und" << endl;
		cout << "V8Unpack.exe -unpack      1Cv8.unp\\metadata.data.und      1Cv8.unp\\metadata.unp" << endl;
		cout << endl;
		cout << endl;
		cout << "PACK" << endl;
		cout << "V8Unpack.exe -pack        1Cv8.unp\\metadata.unp           1Cv8.unp\\metadata_new.data.und" << endl;
		cout << "V8Unpack.exe -deflate     1Cv8.unp\\metadata_new.data.und  1Cv8.unp\\metadata.data" << endl;
		cout << "V8Unpack.exe -pack        1Cv8.und                        1Cv8_new.cf" << endl;
		cout << endl;
		cout << endl;

		return nRetCode;
	}
	

	cout << endl;
	cout << "V8Upack Version 1.09 $ Copyright (c) 2008 Denis Demidov 2008-03-14" << endl;
	cout << endl;
	cout << "Unpack, pack, undeflate and deflate 1C v8 file (*.cf)" << endl;
	cout << endl;
	cout << "V8UNPACK" << endl;
	cout << "  -UNP[ACK]     in_filename.cf     out_dirname" << endl;
	cout << "  -P[ACK]       in_dirname         out_filename.cf" << endl;
	cout << "  -UND[EFLATE]  in_filename.data   out_filename" << endl;
	cout << "  -D[EFLATE]    in_filename        filename.data" << endl;
	cout << "  -EX[AMPLE]" << endl;
	cout << "  -BAT" << endl;

	return nRetCode;
}


