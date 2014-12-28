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
// $Revision: 5 $
// $Author: brix8x $
// $Date: 2008-03-13 14:15:59 +0300 (Р§С‚, 13 РјР°СЂ 2008) $
//
/////////////////////////////////////////////////////////////////////////////

// V8File.cpp: implementation of the CV8File class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

//#include "View1CV8Data.h"
#include "V8File.h"



#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CV8File::CV8File()
{
	pBlocksAddr = NULL;
	pBlocks = NULL;
	IsDataPacked = true;

	if (!ObjectCount)
	{
		InitZLibEngine();
	}

	ObjectCount++;

}


CV8File::CV8File(BYTE *pFileData, bool boolUndeflate)
{
	if (!ObjectCount)
	{
		InitZLibEngine();
	}

	ObjectCount++;

	LoadFile(pFileData, boolUndeflate);
}

CV8File::~CV8File()
{
	if (pBlocks)
		delete[] pBlocks;

	if (pBlocksAddr)
		delete pBlocksAddr;

	ObjectCount--;

	if (!ObjectCount)
		ReleaseZLibEngine();

}

int CV8File::LoadFile(BYTE *pFileData, bool boolUndeflate, bool UnpackWhenNeed)
{
	if (!pFileData)
		return -1;

	//mb("", "LoadFile");

	//BYTE *cpp1251_string = (BYTE*)malloc(100000);
	//UINT cpp1251_string_size;

	UINT undeflate_size;
	UINT BlocksAddrSize;

	BYTE *DataOut = NULL;//(BYTE*)malloc(100000);

	stFileHeader *pFileHeader = (stFileHeader*) pFileData;
	if (pFileHeader->sig != 0x7fffffff || pFileHeader->sig2 != 0x00000200)
		return -1;

	memcpy(&FileHeader, pFileHeader, sizeof(stFileHeader));

	stItemHeader *pItemHeader = (stItemHeader*) &pFileHeader[1];

	ReadItem(pItemHeader, pFileData, (BYTE*&)pBlocksAddr, &BlocksAddrSize);

	//pBlocks = (stBlock*) malloc(sizeof(stBlock) * FileHeader.blocks_num);

	FileHeader.blocks_num = BlocksAddrSize / sizeof(stBlockAddr);

	pBlocks = new stBlock[FileHeader.blocks_num];

	for (UINT i = 0; i < FileHeader.blocks_num; i++)
	{

		if (pBlocksAddr[i].fffffff != 0x7fffffff)
		{
			FileHeader.blocks_num = i;
			break;
		}

		//_RPT1(_CRT_WARN, "i = %i ", i);

		pItemHeader = (stItemHeader*) &pFileData[pBlocksAddr[i].block_header_addr];

		if (pItemHeader->space1 != ' ' || pItemHeader->space2 != ' ' || pItemHeader->space3 != ' ')
		{
			AfxMessageBox("BlockAdrr not correct!");
			break;
		}


		ReadItem(pItemHeader, pFileData, pBlocks[i].pHeader, &pBlocks[i].HeaderSize);

		//v8.unicode_to_1251((wchar_t*)pBlocks[i].pHeader, &cpp1251_string, _a/2);
		//memcpy(pBlocks[i].pHeader, cpp1251_string + 8 + 2, _a/2 - 8 - 2);


		//080228 Блока данных может не быть, тогда адрес блока данных равен 0x7fffffff
		if (pBlocksAddr[i].block_data_addr != 0x7fffffff)
		{
			pItemHeader = (stItemHeader*) &pFileData[pBlocksAddr[i].block_data_addr];
			ReadItem(pItemHeader, pFileData, pBlocks[i].pData, &pBlocks[i].DataSize);
		}
		else
			ReadItem(NULL, pFileData, pBlocks[i].pData, &pBlocks[i].DataSize);

		//pBlocks[i].IsDataPacked = true;

		pBlocks[i].UnpakedData.IsDataPacked = false;

		if (boolUndeflate && IsDataPacked)
		{
			undeflate_size = Undeflate(pBlocks[i].pData, pBlocks[i].DataSize, &DataOut);

			if (!undeflate_size)
				IsDataPacked = false;
			else
			{

				if (UnpackWhenNeed)
				{

					delete[] pBlocks[i].pData;
					pBlocks[i].pData = new BYTE[undeflate_size];
					pBlocks[i].DataSize = undeflate_size;
					memcpy(pBlocks[i].pData, DataOut, undeflate_size);

					stFileHeader *pFileHeader = (stFileHeader*) DataOut;
					if (pFileHeader->sig != 0x7fffffff || pFileHeader->sig2 != 0x00000200)
						pBlocks[i].IsV8File = false;
					else
					{
						pBlocks[i].IsV8File = true;
						pBlocks[i].NeedUnpack = true;
					}
				}
				else
				{
					pBlocks[i].NeedUnpack = false;
					delete[] pBlocks[i].pData;
					if (!pBlocks[i].UnpakedData.LoadFile(DataOut, boolUndeflate))
					{
						pBlocks[i].pData = NULL;
						pBlocks[i].IsV8File = true;
					}
					else
					{
						pBlocks[i].pData = new BYTE[undeflate_size];
						pBlocks[i].DataSize = undeflate_size;
						memcpy(pBlocks[i].pData, DataOut, undeflate_size);
					}
				}
				
			}


		}
	}

	if (DataOut)
		free(DataOut);
	//free(cpp1251_string);
	return 0;
}

BYTE* CV8File::ReadItem(stItemHeader *pItemHeader, BYTE *pFileData, BYTE *&pitem_data, UINT *ItemSize)
{

	stItemHeader *pCurItemHeader = pItemHeader;

	UINT item_data_len, item_page_len;
	DWORD next_item_addr;
	//BYTE *pitem_data;
	UINT read_in_bytes, bytes_to_read;


	//080228 Если pItemHeader = NULL, то читаем блок данных, которого нет
	if (pItemHeader != NULL)
	{
		item_data_len = _httoi(pCurItemHeader->data_len);//, NULL, 16);
		//080312 При предыдущей доработке пропала строка с выделением памяти
		pitem_data = new BYTE[item_data_len];
	}
	else
		item_data_len = 0;

	read_in_bytes = 0;
	while (read_in_bytes < item_data_len)
	{

		item_page_len = _httoi(pCurItemHeader->page_len);//, NULL, 16);
		next_item_addr = _httoi(pCurItemHeader->next_item_addr);//, NULL, 16);

		bytes_to_read = min(item_page_len, item_data_len - read_in_bytes);

		memcpy(&pitem_data[read_in_bytes], (BYTE*)(&pCurItemHeader[1]), bytes_to_read);
		read_in_bytes += bytes_to_read;
		if (next_item_addr != 0x7fffffff) // есть следующая страница
			pCurItemHeader = (stItemHeader*) &pFileData[next_item_addr];
		else
			break;
	}

	if (ItemSize)
		*ItemSize = item_data_len;

	return NULL;//pitem_data;
}

int CV8File::SaveToTextFile(FILE *file_out, UINT mode)
{

	bool DontShow;

	for (UINT i = 0; i < FileHeader.blocks_num; i++)
	{
		DontShow = false;
		if (mode == 1) 
		{
			if ((*(pBlocks[i].pHeader+12+8) != 'm' || 
				*(pBlocks[i].pHeader+12+10) != 'o' || 
				*(pBlocks[i].pHeader+12+12) != 'd' || 
				*(pBlocks[i].pHeader+12+14) != 'u' ||
				*(pBlocks[i].pHeader+12+16) != 'l' ||
				*(pBlocks[i].pHeader+12+18) != 'e')
				&&
				(*(pBlocks[i].pHeader+12+8) != 't' || 
				*(pBlocks[i].pHeader+12+10) != 'e' || 
				*(pBlocks[i].pHeader+12+12) != 'x' || 
				*(pBlocks[i].pHeader+12+14) != 't'))
				DontShow = true;
		}

		if (!DontShow)
		{
			if (mode != 1)
			{
				fprintf(file_out, "BLOCK HEADER: %li\r\n", pBlocks[i].HeaderSize);
				fwrite(pBlocks[i].pHeader, 1, pBlocks[i].HeaderSize, file_out);
				fprintf(file_out, "\r\n");
				fprintf(file_out, "BLOCK DATA: %li\r\n", pBlocks[i].DataSize);
			}

		}
		if (!pBlocks[i].IsV8File)
		{
			if (!DontShow)
			{
				fwrite(pBlocks[i].pData, 1, pBlocks[i].DataSize, file_out);
				fprintf(file_out, "\r\n");
				fprintf(file_out, "_____________________________________________________________________________________\r\n");
			}
		}
		else
		{
			if (!DontShow)
			{
				fprintf(file_out, "BLOCK INSIDE DATA: %li\r\n", pBlocks[i].DataSize);
			}
			pBlocks[i].UnpakedData.SaveToTextFile(file_out, mode);
		}
	}


	return 0;
}

int CV8File::SaveBlockToTextFile(FILE *file_out, stBlock *pBlock, UINT mode)
{

	char *ansi_string = NULL;
	UINT ansi_len;

	ansi_len = utf8_to_ansi(pBlock->pData, &ansi_string, pBlock->DataSize);

	/*
	if (!ansi_len)
		return 0;
	char *cur_pos = ansi_string;

	//char spaces[] = "                                                 ";

	int tabs;

	if (0 && *cur_pos == '{') // другой вывод для удобочитаемости
	{
		int num_chars = 0;
		int open_brackets = 0;
		bool print_tabs = false;
		do
		{

			if (*cur_pos == '{')
			{
				fprintf(file_out, "{");
				open_brackets++;
			}
			else if (*cur_pos == '}')
			{
				fprintf(file_out, "\r\n");
				open_brackets--;
				for(tabs = 0; tabs < open_brackets; tabs++)
					fprintf(file_out, "|\t");
				fprintf(file_out, "}");
			}
			else if (*cur_pos == '\r')
			{
			}
			else if (*cur_pos == '\n')
			{
			}
			else if (*cur_pos == ',')
			{
				fprintf(file_out, ",\r\n");
				print_tabs = true;
			}
			else
				fprintf(file_out, "%c", *cur_pos);

			if (print_tabs)
			{
				for(tabs = 0; tabs < open_brackets; tabs++)
					fprintf(file_out, "|\t");
				print_tabs = false;
			}

			cur_pos++;

		}
		while (open_brackets != 0);

	}
	else
	*/

	if (ansi_len)
		fwrite(ansi_string, 1, ansi_len, file_out);
	else
		fwrite(pBlock->pData, 1, pBlock->DataSize, file_out);

	if (ansi_string)
		free(ansi_string);

	return 0;

}


UINT CV8File::unicode_to_ansi(wchar_t *unicode_string, char **cp1251_string, UINT size)
{
	int err;
	
	int res_len = WideCharToMultiByte(
		1251,				// Code page
		0,					// Default replacement of illegal chars
		unicode_string,		// Multibyte characters string
		size,				// Number of unicode chars is not known
		NULL,				// No buffer yet, allocate it later
		0,					// No buffer
		NULL,				// Use system default
		NULL				// We are not interested whether the default char was used
		);
	
	if (res_len == 0) 
	{
		mb("Failed to obtain required cp1251 string length", "unicode_to_1251");
		return NULL;
	}
		
	*cp1251_string = (char*) realloc(*cp1251_string, sizeof(char) * res_len);
	
	if (*cp1251_string == NULL) 
	{
		mb("Failed to allocate cp1251 string", "unicode_to_1251");
		return NULL;
	}

	err = WideCharToMultiByte(
		1251,				// Code page
		0,					// Default replacement of illegal chars
		unicode_string,		// Multibyte characters string
		size,				// Number of unicode chars is not known
		(char*)*cp1251_string,		// Output buffer
		res_len,			// buffer size
		NULL,				// Use system default
		NULL				// We are not interested whether the default char was used
		);
	if (err == 0)
	{
		mb("Failed to convert from unicode", "unicode_to_1251");
		free(*cp1251_string);
		return NULL;
	}
	return res_len;
}


UINT CV8File::ansi_to_unicode(char *cp1251_string, wchar_t **unicode_string, UINT size)
{
		int err;
		//wchar_t * res;
		int res_len = MultiByteToWideChar(
			CP_ACP,				// Code page
			0,					// No flags
			cp1251_string,		// Multibyte characters string
			size,				// The string is NULL terminated
			NULL,				// No buffer yet, allocate it later
			0					// No buffer
			);
		if (res_len == 0) 
		{
			mb("Failed to obtain utf8 string length",  "ansi_to_unicode");
			return NULL;
		}
		*unicode_string = (wchar_t*) realloc(*unicode_string, sizeof(wchar_t) * res_len);


		if (*unicode_string == NULL) 
		{
			mb("Failed to allocate unicode string", "ansi_to_unicode");
			return NULL;
		}
		err = MultiByteToWideChar(
			CP_ACP,				// Code page
			0,					// No flags
			cp1251_string,		// Multibyte characters string
			size,				// The string is NULL terminated
			*unicode_string,	// Output buffer
			res_len				// buffer size
			);
		if (err == 0)
		{
			printf("Failed to convert to unicode\n");
			free(*unicode_string);
			return NULL;
		}
		
		return res_len;
}



DWORD CV8File::_httoi(char *value)
{

	DWORD result = 0;

	char *s = value;
	BYTE lower_s;
	while (*s != '\0')
	{
		lower_s = tolower(*s);
		if (lower_s >= '0' && lower_s <= '9')
		{
			result <<= 4;
			result += lower_s - '0';
		}
		else if (lower_s >= 'a' && lower_s <= 'f')
		{
			result <<= 4;
			result += lower_s - 'a' + 10;
		}
		else
			break;
		s++;
	}
	return result;
}


ULONG CV8File::Undeflate(unsigned char *DataIn, ULONG DataInSize, unsigned char **DataOut)
{
	// создаем новый поток из string
    HRESULT hr;
    //USES_CONVERSION;

	/*
	IzlibEnginePtr iLibEngine;
	CoInitialize(NULL);
	hr = iLibEngine.CreateInstance(L"V75.zlibEngine");

	if (FAILED(hr))
	{
		//pBkEndUI->DoMessageLine("error iLibEngine->pkDecompress", mmBlackErr);          
		AfxMessageBox("err");
		return 0;
	}
	*/
	
    IStreamPtr pStreamNew;
    IStream *pIStrNew;
    if (S_OK == CreateStreamOnHGlobal(NULL, TRUE, &pIStrNew))
	{
		pStreamNew.Attach(pIStrNew, false);
		
		
		ULONG pcbWritten = 0;
		if (pIStrNew->Write(DataIn, DataInSize, &pcbWritten) != S_OK)
		{
			AfxMessageBox("err");
			//pBkEndUI->DoMessageLine("pStreamOut->Read(&lpStr[0], cbStreamSizeOut, &pcbRead) != S_OK", mmBlackErr);	                            
			return 0;
		}              
		
	}
	
    LARGE_INTEGER __zero;
	__zero.QuadPart = __int64(0);
    if (pStreamNew->Seek(__zero, 0, NULL) == S_OK)
    {
	}
	
	
	ULONG cbStreamSizeOut;
    IStreamPtr pStreamOut;
    IStream *pIStrOut;
    if (S_OK == CreateStreamOnHGlobal(NULL, TRUE, &pIStrOut))
    {     
		pStreamOut.Attach(pIStrOut, false);
		try 
		{
			//CoInitialize(NULL);
			//IzlibEnginePtr iLibEngine;
			//hr = iLibEngine.CreateInstance(L"V75.zlibEngine");
			
			//if (FAILED(hr))
			//{
			//	//pBkEndUI->DoMessageLine("error iLibEngine->pkDecompress", mmBlackErr);          
			//	AfxMessageBox("err");
			//	return 0;
			//}
			
			hr = iLibEngine->pkDecompress(pStreamNew, pStreamOut);            
			
			if (FAILED(hr))
			{
				//pBkEndUI->DoMessageLine("error iLibEngine->pkDecompress", mmBlackErr);          
				AfxMessageBox("err");
				return 0;
			}          
			else
			{          
				STATSTG St;
				pStreamOut->Stat(&St, STATFLAG_NONAME);
				cbStreamSizeOut = St.cbSize.LowPart;

				LARGE_INTEGER __zero;
				__zero.QuadPart = __int64(0);
				if (pStreamOut->Seek(__zero, 0, NULL) == S_OK)
				{
					
					*DataOut = (unsigned char*)realloc(*DataOut, cbStreamSizeOut);
					ULONG pcbRead = 0;
					if (pStreamOut->Read(*DataOut, cbStreamSizeOut, &pcbRead) != S_OK)
					{
						AfxMessageBox("err");
						//pBkEndUI->DoMessageLine("pStreamOut->Read(&lpStr[0], cbStreamSizeOut, &pcbRead) != S_OK", mmBlackErr);	                            
						return 0;
					}
				}
				else
				{
					AfxMessageBox("err");
					//pBkEndUI->DoMessageLine("pStreamOut->Seek(__zero, 0, NULL) != S_OK", mmBlackErr);	                            
					return 0;
				}
			}
			
		} 
		catch (_com_error& err) 
		{
			//CString str;
			//str.Format("V75.zlibEngine Error:%s", err.ErrorMessage());
			AfxMessageBox(err.ErrorMessage());
			//pBkEndUI->DoMessageLine(str, mmBlackErr);
			return 0;
		}                  
    }
    else
    {
		//pBkEndUI->DoMessageLine("error in CreateStreamOnHGlobal", mmBlackErr);    
		AfxMessageBox("err");
		return 0; 
    }        
    return cbStreamSizeOut;    

}

stBlock* CV8File::GetBlockByName(char *BlockName)
{

	wchar_t *unicode_string = NULL, *pCurBlockName;

	ansi_to_unicode(BlockName, &unicode_string, -1);

	UINT i;

	for (i=0; i < FileHeader.blocks_num; i++)
	{
		pCurBlockName = (wchar_t*)(pBlocks[i].pHeader + 12 + 8);
		if (!wcscmp(unicode_string, pCurBlockName))
		{
			if (unicode_string)
				free(unicode_string);

			if (pBlocks[i].NeedUnpack)
			{
				BYTE *DataOut = NULL;
				Unpack(pBlocks + i, DataOut);
				if (DataOut)
					free(DataOut);
			}


			return &pBlocks[i];
		}
	}

	if (unicode_string)
		free(unicode_string);
	return NULL;
}



UINT CV8File::utf8_to_ansi(unsigned char *utf8_string, char **ansi_string, UINT size)
{
	int err;

	if (utf8_string[0] == 0xef && utf8_string[1] == 0xbb && utf8_string[2] == 0xbf)
	{
		utf8_string += 3;
		size = size - 3;
	}
	else
	{
		return 0;
	}


	if (!size)
		return 0;

	int unicode_len = MultiByteToWideChar(
		CP_UTF8,			// Code page
		0,					// No flags
		(char*)utf8_string,	// Multibyte characters string
		size,				// The string is NULL terminated
		NULL,				// No buffer yet, allocate it later
		0					// No buffer
		);
	if (unicode_len == 0) 
	{
		mb("Failed to obtain utf8 string length", "utf8_to_ansi");
		return NULL;
	}
	
	wchar_t *m_unicode_string = NULL;
	m_unicode_string = (wchar_t*) realloc(m_unicode_string, sizeof(wchar_t) * unicode_len);

	if (m_unicode_string == NULL) 
	{
		mb("Failed to allocate unicode string", "utf8_to_ansi");
		return NULL;
	}
	err = MultiByteToWideChar(
		CP_UTF8,			// Code page
		0,					// No flags
		(char*)utf8_string,	// Multibyte characters string
		size,				// The string is NULL terminated
		m_unicode_string,	// Output buffer
		unicode_len			// buffer size
		);

	if (err == 0)
	{
		mb("Failed to convert to unicode", "utf8_to_ansi");
		//free(m_unicode_string);
		return NULL;
	}
	
	int ansi_len = unicode_len;
	/*ansi_len = WideCharToMultiByte(
		1251,				// Code page
		0,					// Default replacement of illegal chars
		m_unicode_string,	// Multibyte characters string
		unicode_len,		// Number of unicode chars is not known
		NULL,				// No buffer yet, allocate it later
		0,					// No buffer
		NULL,				// Use system default
		NULL				// We are not interested whether the default char was used
		);
	*/



	*ansi_string = (char*) realloc(*ansi_string, sizeof(unsigned char)*ansi_len);

	if (*ansi_string == NULL) 
	{
		mb("Failed to allocate cp1251 string", "utf8_to_ansi");
		return NULL;
	}
	err = WideCharToMultiByte(
		1251,				// Code page
		0,					// Default replacement of illegal chars
		m_unicode_string,	// Multibyte characters string
		unicode_len,		// Number of unicode chars is not known
		*ansi_string,		// Output buffer
		ansi_len,		// buffer size
		NULL,				// Use system default
		NULL				// We are not interested whether the default char was used
		);
	if (err == 0)
	{
		mb("Failed to convert from unicode", "utf8_to_ansi");
		//free(*cp1251_string);
		return NULL;
	}

	free(m_unicode_string);

	return ansi_len;
}


int CV8File::SaveFile(FILE *file_out)
{

	UINT block_num;
	BYTE *DataOut = NULL;
	//UINT deflate_size;

	//file_out = fopen(filename, "wb");


	fwrite(&FileHeader, 1, sizeof(FileHeader), file_out);

	stBlockAddr *pNewBlocksAddr = new stBlockAddr[FileHeader.blocks_num];

	DWORD cur_block_addr = sizeof(stFileHeader) + sizeof(stItemHeader);
	if (sizeof(stBlockAddr) * FileHeader.blocks_num < 512)
		cur_block_addr += 512;
	else
		cur_block_addr += sizeof(stBlockAddr) * FileHeader.blocks_num;

	for (block_num = 0; block_num < FileHeader.blocks_num; block_num++)
	{
		pNewBlocksAddr[block_num].block_header_addr = cur_block_addr;
		cur_block_addr += sizeof(stItemHeader) + pBlocks[block_num].HeaderSize;
		pNewBlocksAddr[block_num].block_data_addr = cur_block_addr;
		cur_block_addr += sizeof(stItemHeader);
		if (pBlocks[block_num].DataSize > 512)
			cur_block_addr += pBlocks[block_num].DataSize;
		else
			cur_block_addr += 512;

		pNewBlocksAddr[block_num].fffffff = 0x7fffffff;
		
	}


	SaveItem(file_out, (BYTE*) pNewBlocksAddr, sizeof(stBlockAddr) * FileHeader.blocks_num);


	for (block_num= 0; block_num < FileHeader.blocks_num; block_num++)
	{
		SaveItem(file_out, pBlocks[block_num].pHeader, pBlocks[block_num].HeaderSize, pBlocks[block_num].HeaderSize);
		SaveItem(file_out, pBlocks[block_num].pData, pBlocks[block_num].DataSize);
		/*
		if (pBlocks[block_num].IsV8File)
		{
		}
		else 
		{
			if (IsDataPacked && pBlocks[block_num].pData)
			{
				deflate_size = Deflate(pBlocks[block_num].pData, pBlocks[block_num].DataSize, &DataOut);
				SaveItem(file_out, DataOut, deflate_size);
			}
		}
		*/
	}


	//fclose(file_out);


	delete[] pNewBlocksAddr;
	free(DataOut);

	return false;
}


BYTE* CV8File::SaveItem(FILE *file_out, BYTE *pitem_data, UINT item_data_len, UINT item_page_len)
{
	//stItemHeader *pItemHeader, BYTE *pFileData, 

	if (item_page_len < item_data_len)
		item_page_len = item_data_len;
	
	//stItemHeader *pCurItemHeader = pItemHeader;

	//UINT item_page_len = 512;
	//DWORD next_item_addr;
	//BYTE *pitem_data;
	//UINT write_out_bytes, bytes_to_write;

	stItemHeader CurItemHeader;

	memcpy(&CurItemHeader.EOL, "\r\n", 2);
	memcpy(&CurItemHeader.EOL_2, "\r\n", 2);
	CurItemHeader.space1 = 0;
	CurItemHeader.space2 = 0;
	CurItemHeader.space3 = 0;

	sprintf(CurItemHeader.page_len, "%08x", item_page_len);
	//CurItemHeader.
	sprintf(CurItemHeader.data_len, "%08x", item_data_len);
	sprintf(CurItemHeader.next_item_addr, "%08x", 0x7fffffff);

	CurItemHeader.space1 = ' ';
	CurItemHeader.space2 = ' ';
	CurItemHeader.space3 = ' ';

	fwrite((void*)&CurItemHeader, sizeof(CurItemHeader), 1, file_out);

	fwrite((void*)pitem_data, 1, item_data_len, file_out);

	for(UINT i = 0; i < item_page_len - item_data_len; i++)
	{
		fwrite("\0", 1, 1, file_out);
	}

	/*item_data_len = _httoi(pCurItemHeader->data_len);//, NULL, 16);
	pitem_data = new BYTE[item_data_len];

	read_in_bytes = 0;
	while (read_in_bytes < item_data_len)
	{

		item_page_len = _httoi(pCurItemHeader->page_len);//, NULL, 16);
		next_item_addr = _httoi(pCurItemHeader->next_item_addr);//, NULL, 16);

		bytes_to_read = min(item_page_len, item_data_len - read_in_bytes);

		memcpy(&pitem_data[read_in_bytes], (BYTE*)(&pCurItemHeader[1]), bytes_to_read);
		read_in_bytes += bytes_to_read;
		if (next_item_addr != 0x7fffffff) // есть следующая страница
			pCurItemHeader = (stItemHeader*) &pFileData[next_item_addr];
		else
			break;
	}

	if (ItemSize)
		*ItemSize = item_data_len;

	*/


	return NULL;//pitem_data;
}


ULONG CV8File::Deflate(unsigned char *DataIn, ULONG DataInSize, unsigned char **DataOut)
{
	// создаем новый поток из string
    HRESULT hr;
    //USES_CONVERSION;

	/*
	IzlibEnginePtr iLibEngine;
	CoInitialize(NULL);
	hr = iLibEngine.CreateInstance(L"V75.zlibEngine");

	if (FAILED(hr))
	{
		AfxMessageBox("err");
		return 0;
	}
	*/
	
    IStreamPtr pStreamNew;
    IStream *pIStrNew;
    if (S_OK == CreateStreamOnHGlobal(NULL, TRUE, &pIStrNew))
	{
		pStreamNew.Attach(pIStrNew, false);
		
		
		ULONG pcbWritten = 0;
		if (pIStrNew->Write(DataIn, DataInSize, &pcbWritten) != S_OK)
		{
			AfxMessageBox("err");
			//pBkEndUI->DoMessageLine("pStreamOut->Read(&lpStr[0], cbStreamSizeOut, &pcbRead) != S_OK", mmBlackErr);	                            
			return 0;
		}              
		
	}
	
    LARGE_INTEGER __zero;
	__zero.QuadPart = __int64(0);
    if (pStreamNew->Seek(__zero, 0, NULL) == S_OK)
    {
	}
	
	
	ULONG cbStreamSizeOut;
    IStreamPtr pStreamOut;
    IStream *pIStrOut;
    if (S_OK == CreateStreamOnHGlobal(NULL, TRUE, &pIStrOut))
    {     
		pStreamOut.Attach(pIStrOut, false);
		try 
		{
			//CoInitialize(NULL);
			//IzlibEnginePtr iLibEngine;
			//hr = iLibEngine.CreateInstance(L"V75.zlibEngine");
			
			//if (FAILED(hr))
			//{
			//	//pBkEndUI->DoMessageLine("error iLibEngine->pkDecompress", mmBlackErr);          
			//	AfxMessageBox("err");
			//	return 0;
			//}
			
			hr = iLibEngine->pkCompress(pStreamNew, pStreamOut);            
			
			if (FAILED(hr))
			{
				//pBkEndUI->DoMessageLine("error iLibEngine->pkDecompress", mmBlackErr);          
				AfxMessageBox("err");
				return 0;
			}          
			else
			{          
				STATSTG St;
				pStreamOut->Stat(&St, STATFLAG_NONAME);
				cbStreamSizeOut = St.cbSize.LowPart;

				LARGE_INTEGER __zero;
				__zero.QuadPart = __int64(0);
				if (pStreamOut->Seek(__zero, 0, NULL) == S_OK)
				{
					
					*DataOut = (unsigned char*)realloc(*DataOut, cbStreamSizeOut);
					ULONG pcbRead = 0;
					if (pStreamOut->Read(*DataOut, cbStreamSizeOut, &pcbRead) != S_OK)
					{
						AfxMessageBox("err");
						//pBkEndUI->DoMessageLine("pStreamOut->Read(&lpStr[0], cbStreamSizeOut, &pcbRead) != S_OK", mmBlackErr);	                            
						return 0;
					}
				}
				else
				{
					AfxMessageBox("err");
					//pBkEndUI->DoMessageLine("pStreamOut->Seek(__zero, 0, NULL) != S_OK", mmBlackErr);	                            
					return 0;
				}
			}
			
		} 
		catch (_com_error& err) 
		{
			//CString str;
			//str.Format("V75.zlibEngine Error:%s", err.ErrorMessage());
			AfxMessageBox(err.ErrorMessage());
			//pBkEndUI->DoMessageLine(str, mmBlackErr);
			return 0;
		}                  
    }
    else
    {
		//pBkEndUI->DoMessageLine("error in CreateStreamOnHGlobal", mmBlackErr);    
		AfxMessageBox("err");
		return 0; 
    }        
    return cbStreamSizeOut;    

}

int CV8File::UnpackToFolder(char *Name, char *dirname, char *block_name, bool print_progress)
{


	unsigned char *pFileData;

	//CV8File V8File;

	//char *Name = "ПустаяСправочник.cf";
	//char *Name = "KC_20060626_1636.cf";
	//char *Name = "Тест1.cf";
	//char *Name = "Тест_Языки.cf";

	int err = 0;

	HANDLE hFile=CreateFile(Name,GENERIC_READ,FILE_SHARE_READ,NULL,
		OPEN_EXISTING,0,NULL);
	
	if(hFile != INVALID_HANDLE_VALUE)
	{
		DWORD FileSizeLow=GetFileSize(hFile,NULL);
		HANDLE hFileMapping=CreateFileMapping(hFile,NULL,PAGE_READONLY,0,0,NULL);
		CloseHandle(hFile);
		
		if(hFileMapping != NULL)
		{
			pFileData = (unsigned char *)MapViewOfFile(hFileMapping,FILE_MAP_READ,0,0,FileSizeLow);
			

			err = LoadFile(pFileData, false);

			UnmapViewOfFile(pFileData);

			CloseHandle(hFileMapping);

			if (err)
				return err;
		}
	}
	else
	{
		AfxMessageBox("File not found!");	
		return -1;
	}


	FILE *file_out;
	char *cur_dir = dirname;

	_mkdir(cur_dir);

	char filename[MAX_PATH];

	sprintf(filename, "%s\\%s", cur_dir, "FileHeader");
	file_out = fopen(filename, "wb");
	fwrite(&FileHeader,  sizeof(stFileHeader), 1, file_out);
	fclose(file_out);

	char *cp1251_string = NULL;

	// Вместо декодирования unicode преобразовываем сами
	cp1251_string = (char*) realloc(cp1251_string, 512);

	UINT cp1251_len, unicode_len;

	UINT one_percent = FileHeader.blocks_num / 50;
	if (print_progress && one_percent)
	{
		std::cout << "Progress (50 points): ";
	}

	UINT block_num;
	for(block_num = 0; block_num < FileHeader.blocks_num; block_num++)
	{

		if (print_progress && block_num && one_percent && block_num%one_percent == 0)
		{
			if (block_num % (one_percent*10) == 0)
				std::cout << "|";
			else
				std::cout << ".";
		}

		// Вместо декодирования unicode преобразовываем сами
		//cp1251_len = unicode_to_ansi((wchar_t*) (pBlocks[block_num].pHeader+12+8), &cp1251_string, pBlocks[block_num].HeaderSize-12-8);
		unicode_len = pBlocks[block_num].HeaderSize - 12 - 8;
		for (UINT j = 0; j < unicode_len; j+=2)
			cp1251_string[j/2] = pBlocks[block_num].pHeader[12+8+j];
		cp1251_len = unicode_len / 2;

		/*
		time_t ltime;
		time( &ltime );
 
		tm *ptm;
		ptm = localtime(&ltime);

		
		stHeaderAttr Attr;
		memcpy(&Attr, pBlocks[block_num].pHeader, sizeof(Attr));
		int sz = sizeof(Attr);

		ptm = gmtime((const time_t*) &Attr.Created);
		//buf_0612 = ctime((const time_t*) Attr->Created);
		*/

		if (block_name && strcmp(block_name, cp1251_string))
			continue;

		sprintf(filename, "%s\\%s.%s", cur_dir, cp1251_string, "header");
		file_out = fopen(filename, "wb");
		fwrite(pBlocks[block_num].pHeader,  1, pBlocks[block_num].HeaderSize, file_out);
		fclose(file_out);

		sprintf(filename, "%s\\%s.%s", cur_dir, cp1251_string, "data");
		file_out = fopen(filename, "wb");
		//std::cout << pBlocks[block_num].DataSize << std::endl;
		fwrite(pBlocks[block_num].pData,  1, pBlocks[block_num].DataSize, file_out);
		fclose(file_out);
	}

	if (print_progress && one_percent)
	{
		std::cout << std::endl;
	}

	free(cp1251_string);


	return 0;
}

void CV8File::PackFromFolder(char *dirname, char *filename_out)
{

	//CV8File V8File;

	char cur_dir[MAX_PATH];
	strcpy(cur_dir, dirname);

	struct _finddata_t find_data;
	long hFind;

	char filename[MAX_PATH];//, filename_out[MAX_PATH];

	struct _stat stat;

	FILE *file_in, *file_out;

	char *point_pos;

	//sprintf(filename_out, "%s\\pack.cf", cur_dir);
	file_out = fopen(filename_out, "wb");


	sprintf(filename, "%s\\FileHeader", cur_dir);

	_stat(filename, &stat);


	stFileHeader FileHeader;

	file_in = fopen(filename, "rb");
	fread(&FileHeader, 1, stat.st_size, file_in);
	fclose(file_in);

	sprintf(filename, "%s\\*.header", cur_dir);
	hFind = _findfirst(filename, &find_data);
	FileHeader.blocks_num = 0;

	if( hFind != -1 )
	{
		do
		{
			FileHeader.blocks_num ++;

		} while( _findnext(hFind, &find_data) == 0 );
		_findclose(hFind);
	}

	
	fwrite(&FileHeader, 1, sizeof(stFileHeader), file_out);

	stBlockAddr *pNewBlocksAddr = new stBlockAddr[FileHeader.blocks_num];
	stBlock *pNewBlocks = new stBlock[FileHeader.blocks_num];

	DWORD cur_block_addr = sizeof(stFileHeader) + sizeof(stItemHeader);
	if (sizeof(stBlockAddr) * FileHeader.blocks_num < 512)
		cur_block_addr += 512;
	else
		cur_block_addr += sizeof(stBlockAddr) * FileHeader.blocks_num;

	hFind = _findfirst(filename, &find_data);
	UINT block_num = 0;

	if( hFind != -1 )
	{
		do
		{

			sprintf(filename, "%s\\%s", cur_dir, find_data.name);

			_stat(filename, &stat);
			pNewBlocks[block_num].HeaderSize = stat.st_size;
			pNewBlocks[block_num].pHeader = new BYTE[pNewBlocks[block_num].HeaderSize];
			file_in = fopen(filename, "rb");
			fread(pNewBlocks[block_num].pHeader, 1, pNewBlocks[block_num].HeaderSize, file_in);
			fclose(file_in);

			point_pos = strrchr(filename, '.');
			filename[point_pos - filename] = 0;
			strcat(filename, ".data");
			_stat(filename, &stat);
			pNewBlocks[block_num].DataSize = stat.st_size;
			pNewBlocks[block_num].pData = new BYTE[pNewBlocks[block_num].DataSize];
			file_in = fopen(filename, "rb");
			fread(pNewBlocks[block_num].pData, 1, pNewBlocks[block_num].DataSize, file_in);
			fclose(file_in);

			
			pNewBlocksAddr[block_num].block_header_addr = cur_block_addr;
			cur_block_addr += sizeof(stItemHeader) + pNewBlocks[block_num].HeaderSize;
			pNewBlocksAddr[block_num].block_data_addr = cur_block_addr;
			cur_block_addr += sizeof(stItemHeader);
			if (pNewBlocks[block_num].DataSize > 512)
				cur_block_addr += pNewBlocks[block_num].DataSize;  
			else
				cur_block_addr += 512;

			pNewBlocksAddr[block_num].fffffff = 0x7fffffff;

			block_num++;

		} while( _findnext(hFind, &find_data) == 0 );
		_findclose(hFind);
	}


	//FileHeader.blocks_num = block_num;

	SaveItem(file_out, (BYTE*) pNewBlocksAddr, sizeof(stBlockAddr) * FileHeader.blocks_num);


	for(block_num = 0; block_num < FileHeader.blocks_num; block_num++)
	{
		SaveItem(file_out, pNewBlocks[block_num].pHeader, pNewBlocks[block_num].HeaderSize, pNewBlocks[block_num].HeaderSize);
		SaveItem(file_out, pNewBlocks[block_num].pData, pNewBlocks[block_num].DataSize);

		//delete[] pNewBlocks[block_num].pHeader;
		//delete[] pNewBlocks[block_num].pData;
	}

	delete[] pNewBlocksAddr;
	delete[] pNewBlocks;

	//fseek(file_out, SEEK_SET, 0);
	//fwrite(&FileHeader, 1, sizeof(stFileHeader), file_out);

	fclose(file_out);


}

void CV8File::Undeflate(char *filename_in, char *filename_out)
{

	HANDLE hFile=CreateFile(filename_in,GENERIC_READ,FILE_SHARE_READ,NULL,
		OPEN_EXISTING,0,NULL);
	
	if(hFile != INVALID_HANDLE_VALUE)
	{
		DWORD FileSizeLow=GetFileSize(hFile,NULL);
		HANDLE hFileMapping=CreateFileMapping(hFile,NULL,PAGE_READONLY,0,0,NULL);
		CloseHandle(hFile);
		
		if(hFileMapping != NULL)
		{
			unsigned char *pFileData = (unsigned char *)MapViewOfFile(hFileMapping,FILE_MAP_READ,0,0,FileSizeLow);

			BYTE *DataOut = NULL;

			CV8File V8File;
			UINT unpacked_size = Undeflate(pFileData, FileSizeLow, &DataOut);

			FILE *file_metadata;
			
			file_metadata = fopen(filename_out, "wb");
			fwrite(DataOut, 1, unpacked_size, file_metadata);


			fclose(file_metadata);

			free(DataOut);

			UnmapViewOfFile(pFileData);

			CloseHandle(hFileMapping);

		}
	}

}

void CV8File::Deflate(char *filename_in, char *filename_out)
{

	HANDLE hFile=CreateFile(filename_in,GENERIC_READ,FILE_SHARE_READ,NULL,
		OPEN_EXISTING,0,NULL);
	
	if(hFile != INVALID_HANDLE_VALUE)
	{
		DWORD FileSizeLow=GetFileSize(hFile,NULL);
		HANDLE hFileMapping=CreateFileMapping(hFile,NULL,PAGE_READONLY,0,0,NULL);
		CloseHandle(hFile);
		
		if(hFileMapping != NULL)
		{
			unsigned char *pFileData = (unsigned char *)MapViewOfFile(hFileMapping,FILE_MAP_READ,0,0,FileSizeLow);

			BYTE *DataOut = NULL;

			CV8File V8File;
			UINT packed_size = Deflate(pFileData, FileSizeLow, &DataOut);

			FILE *file_metadata;
			
			file_metadata = fopen(filename_out, "wb");
			fwrite(DataOut, 1, packed_size, file_metadata);


			fclose(file_metadata);

			free(DataOut);

			UnmapViewOfFile(pFileData);

			CloseHandle(hFileMapping);

		}
	}

}


IzlibEnginePtr CV8File::iLibEngine;
UINT CV8File::ObjectCount;

void CV8File::InitZLibEngine()
{
	CoInitialize(NULL);
	if( iLibEngine.CreateInstance(L"V77.zlibEngine") != S_OK )
	{
		//Msg(0, "Error CreateInstance V77.zlibEngine");		
		if( iLibEngine.CreateInstance(L"V75.zlibEngine") != S_OK )
		{
			//Msg(0, "Error CreateInstance V75.zlibEngine");
			throw NULL;
		}
	}

}

void CV8File::ReleaseZLibEngine()
{
	iLibEngine.Release();
	CoUninitialize();
}

void CV8File::Unpack(stBlock *pBlock, BYTE *&DataOut)
{

	pBlock->UnpakedData.LoadFile(pBlock->pData);
	delete[] pBlock->pData;
	pBlock->pData = NULL;
	pBlock->NeedUnpack = false;

	/*
	UINT undeflate_size = Undeflate(pBlock->pData, pBlock->DataSize, &DataOut);

	pBlock->NeedUnpack = false;

	if (!undeflate_size)
		IsDataPacked = false;
	else
	{
		delete[] pBlock->pData;
		if (!pBlock->UnpakedData.LoadFile(DataOut))
		{
			pBlock->pData = NULL;
			pBlock->IsV8File = true;
		}
		else
		{
			pBlock->IsV8File = false;
			pBlock->pData = new BYTE[undeflate_size];
			pBlock->DataSize = undeflate_size;
			memcpy(pBlock->pData, DataOut, undeflate_size);
		}
	}
	*/
}
