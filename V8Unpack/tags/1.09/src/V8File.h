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
// $Revision: 7 $
// $Author: disa_da $
// $Date: 2008-03-14 00:10:52 +0300 (Пт, 14 мар 2008) $
//
/////////////////////////////////////////////////////////////////////////////

// V8File.h: interface for the CV8File class.
//
//////////////////////////////////////////////////////////////////////

#define mb(A,B) MessageBox(0, A, B, 0)

//#define AfxMessageBox(A) mb(A,"")


#if !defined(AFX_V8FILE_H__935D5C2B_70FA_45F2_BDF2_A0274A8FD60C__INCLUDED_)
#define AFX_V8FILE_H__935D5C2B_70FA_45F2_BDF2_A0274A8FD60C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//#include "V8.h"

#pragma warning(disable:4192)
#import "zlibeng.dll" no_namespace


struct stFileHeader
{
	//DWORDLONG
	DWORD sig;
	DWORD sig2;
	DWORD blocks_num;
	DWORD reserved;
};

struct stBlockAddr
{
	DWORD block_header_addr;
	DWORD block_data_addr;
	DWORD fffffff;
};

struct stItemHeader
{
	char EOL[2];
	char data_len[8];
	char space1;
	char page_len[8];
	char space2;
	char next_item_addr[8];
	char space3;
	char EOL_2[2];
	//DWORD res1, res2, res3, res4;
};


class stBlock;

class CV8File  
{
public:
	void Unpack(stBlock *pBlock, BYTE *&DataOut);
	static void ReleaseZLibEngine();
	static IzlibEnginePtr iLibEngine;
	static UINT ObjectCount;
	static void InitZLibEngine();
	void Deflate(char *filename_in, char *filename_out);
	void Undeflate(char *filename_in, char *filename_out);
	void PackFromFolder(char *dirname, char *filename);
	int UnpackToFolder(char *filename, char *dirname, char *block_name = NULL, bool print_progress = false);
	UINT utf8_to_ansi(unsigned char *utf8_string, char **cp1251_string, UINT size);
	stBlock* GetBlockByName(char *BlockName);
	int SaveBlockToTextFile(FILE *file_out, stBlock *pBlock, UINT mode = 0);
	int SaveToTextFile(FILE *file_out, UINT mode = 0);
	
	ULONG Undeflate(unsigned char *DataIn, ULONG DataInSize, unsigned char **DataOut);

	DWORD _httoi(char *value);

	UINT unicode_to_ansi(wchar_t *unicode_string, char **cp1251_string, UINT size);

	UINT ansi_to_unicode(char *cp1251_string, wchar_t **unicode_string, UINT size);

	BYTE* ReadItem(stItemHeader *pItemHeader, BYTE *pFileData, BYTE *&pItem, UINT *ItemSize = NULL);
	int LoadFile(BYTE *pFileData, bool boolUndeflate = true, bool UnpackWhenNeed = false);

	int SaveFile(FILE *file_out);
	BYTE* SaveItem(FILE *file_out, BYTE *pitem_data, UINT item_data_len, UINT item_page_len = 512);
	ULONG Deflate(unsigned char *DataIn, ULONG DataInSize, unsigned char **DataOut);

	CV8File();
	CV8File(BYTE *pFileData, bool boolUndeflate = true);
	virtual ~CV8File();
	stFileHeader	FileHeader;
	stBlockAddr		*pBlocksAddr;
	stBlock			*pBlocks;
	bool			IsDataPacked;
};


class stBlock
{
public:
	BYTE	*pHeader;
	UINT	HeaderSize;
	BYTE	*pData;
	UINT	DataSize;
	CV8File UnpakedData;
	bool	IsV8File;
	bool	NeedUnpack;

	stBlock()
	{
		pHeader = NULL;
		pData = NULL;
		IsV8File = false;
	}

	~stBlock()
	{
		if (pHeader)
			delete[] pHeader;
		if (pData)
			delete[] pData;
	}
};

class stHeaderAttr
{
public:
	time_t Created;
	time_t Modificated;

};


#endif // !defined(AFX_V8FILE_H__935D5C2B_70FA_45F2_BDF2_A0274A8FD60C__INCLUDED_)