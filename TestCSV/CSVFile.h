#pragma once
#include "SGFile.h"
#include <atlstr.h>
#include <vector>
using std::vector;



class CCSVFile
{
	typedef struct _FILED_INFO
	{
		UINT uRowNo;
		UINT uColNo;
		UINT uBuffStart;
		UINT uBuffEnd;
	}FILED_INFO, *PFILED_INFO;

public:
	CCSVFile(LPCTSTR _lpcFilaPath);
	~CCSVFile(void);

public:
	UINT GetRowCnt();
	UINT GetColCnt();
	CString GetContent(UINT _uRow, UINT _uCol);
	BOOL SetContent(UINT _uRow, UINT _uCol, LPCTSTR _lpcContent);
	BOOL InsertRow(UINT _uRowNo);
	BOOL InsertCol(UINT _uColNo);
	UINT AddRow();
	UINT AddCol();
	BOOL DeleteRow(UINT _uRowNo);
	BOOL DeleteCol(UINT _uColNo);
	BOOL Flush();
	void Close();

private:
	BOOL Parse();
	void ReDefrag();
	BOOL IsSpecial(LPCTSTR _lpcContent);

private:
	CString m_strFilePath;
	char* m_pBuff;
	vector<FILED_INFO> m_vFiled;
	UINT m_uBuffSize;
	UINT m_uCurOffset;
	CSGFile m_file;
};

