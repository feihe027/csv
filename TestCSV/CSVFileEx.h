#pragma once
#include <Windows.h>
#include <memory>
#include <vector>
#include <string>


class CCSVFileEx
{
public:
	explicit CCSVFileEx(char _delimiter = ',');
	~CCSVFileEx(void);

public:
	BOOL Init(LPCTSTR _lpcPath);
	BOOL Init(char* _pBuff, UINT _uSize);
	std::string GetContent(UINT _uRow, UINT _uCol);
	BOOL SetContent(UINT _uRow, UINT _uCol, LPCTSTR _lpcContent);
	UINT GetRowCnt() const;
	UINT GetColCnt() const ;
	BOOL InsertRow(UINT _uRowNo);
	BOOL InsertCol(UINT _uColNo);
	void AddRow(const std::vector<std::string>& _vstrField);
	void AddCol(const std::vector<std::string>& _vstrField);
	UINT AddRow();
	UINT AddCol();
	BOOL DeleteRow(UINT _uRowNo);
	BOOL DeleteCol(UINT _uColNo);
	BOOL Save();
	BOOL SaveAs(LPCTSTR _lpcPath);
	std::string GetAllContent();

private:
	class CPImpl;
	std::unique_ptr<CPImpl> m_pImpl;
};

