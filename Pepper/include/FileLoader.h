/****************************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/										*
* This software is available under the "MIT License modified with The Commons Clause".				*
* https://github.com/jovibor/Pepper/blob/master/LICENSE												*
* Pepper - PE (x86) and PE+ (x64) files viewer, based on libpe: https://github.com/jovibor/Pepper	*
* libpe - Windows library for reading PE (x86) and PE+ (x64) files inner structure information.		*
* https://github.com/jovibor/libpe																	*
****************************************************************************************************/
/****************************************************************
* This is a helper class for Pepper and HexCtrl routines of		*
* showing files' offsets. It loads (maps) files into memory		*
* by demand and unloads then. Works with big files - files that *
* can't	be loaded fully at ones - as well.						*
*																*
****************************************************************/
#pragma once
#include "HexCtrl.h"

using namespace HEXCTRL;

class CFileLoader : public CWnd
{
public:
	CFileLoader() {};
	~CFileLoader() {};

	//First function to call.
	HRESULT LoadFile(LPCWSTR lpszFileName);

	//Shows arbitrary offset in already loaded file (LoadFile)
	//If pHexCtrl == nullptr inner CHexCtrl object is used.
	HRESULT ShowOffset(ULONGLONG ullOffset, ULONGLONG ullSelectionSize, CHexCtrl* pHexCtrl = nullptr);

	//Shows only a piece of the whole loaded file.
	//If pHexCtrl == nullptr inner CHexCtrl object is used.
	HRESULT ShowFilePiece(ULONGLONG ullOffsetFrom, ULONGLONG ullSize, CHexCtrl* pHexCtrl = nullptr);

	//Unloads loaded file and all pieces, if present.
	HRESULT UnloadFile();
private:
	/****************************************************************************
	* This is a helper structure for query information.							*
	* Every window (hWnd) can have its own set of mapped data.					*
	* So we can map different parts, of the big file,							*
	* for different windows (CHexCtrl instances) simultaneously.				*
	****************************************************************************/
	struct QUERYDATA {
		HWND		hWnd { };
		ULONGLONG	ullStartOffsetMapped { };
		ULONGLONG	ullEndOffsetMapped { };
		ULONGLONG	ullOffsetDelta { };
		DWORD		dwDeltaFileOffsetMapped { };
		LPVOID		lpData { };
	};
	CHexCtrl m_stHex;
//	CHexCtrl* m_pHexCur { }; //Current
	HEXCREATESTRUCT m_stHC { };
	//Size of the loaded PE file.
	LARGE_INTEGER m_stFileSize { };
	HANDLE m_hFile { };
	//Returned by CreateFileMappingW.
	HANDLE m_hMapObject { };
	//Pointer to file mapping beginning,
	//no matter if mapped completely or section by section.
	LPVOID m_lpBase { };
	//Is file loaded (mapped) completely, or section by section?
	bool m_fMapViewOfFileWhole { };
	//System information getting from GetSystemInfo().
	//Needed for dwAllocationGranularity.
	SYSTEM_INFO m_stSysInfo { };
	bool m_fCreated { false };
	std::vector<QUERYDATA> m_vecQuery;
	const int IDC_HEX_CTRL = 0xFF; //Id of inner CHexCtrl.
private:
	unsigned char GetByte(HWND hWnd, ULONGLONG ullOffset); //For Virtual HexCtrl retrives next byte on demand.
	HRESULT MapFileOffset(QUERYDATA& rData, ULONGLONG ullOffset, DWORD dwSize = 0); //Main routine for mapping big file's parts.
	HRESULT UnmapFileOffset(QUERYDATA& rData);
	bool IsCreated();
	bool IsLoaded();
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
};