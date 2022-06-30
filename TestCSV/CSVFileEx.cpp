#include "StdAfx.h"
#include "CSVFileEx.h"
#include <assert.h>
#include <map>
#include <iostream>
#include <fstream>
using std::map;



typedef struct _CSV_POS
{	
	UINT uRow;
	UINT uCol;
	bool operator < (const _CSV_POS& _other) const
	{
		if (uRow < _other.uRow) 
		{
			return true ;
		}
		else if (uRow == _other.uRow)  
		{
			return uCol < _other.uCol;
		}

		return   false ;
	}
}CSV_POS;

enum FieldState
{
	FS_BEGIN = 0,
	FS_END,
};

typedef struct _FIELD_SIZE
{
	UINT uBegin;
	UINT uEnd;
	UINT uSize;
	int nQuoteFlag;
}FIELD_SIZE, *PFIELD_SIZE;

const static UINT MAX_FIELD_SIZE = 2048;

class CCSVFileEx::CPImpl
{
	friend CCSVFileEx;

private:
	CPImpl(char _delimiter)
		: m_pBuff(nullptr)
		, m_delimiter(_delimiter)
		, m_uBuffSize(0)
		, m_uOffset(0)
		, m_uFileSize(0)
		, m_uRowCnt(0)
		, m_uColCnt(0)
	{}

public:
	~CPImpl()
	{
		delete[] m_pBuff;
		m_pBuff = nullptr;
	}

public:
	BOOL Init(LPCTSTR _lpcPath)
	{
		m_strPath = _lpcPath;

		if (!OpenFile())
		{
			return FALSE;
		}

		if (!Parse())
		{
			return FALSE;
		}

		UINT uRow = GetRowCnt();

		return TRUE;
	}

	BOOL Init(char* _pBuff, UINT _uSize)
	{
		m_uFileSize = _uSize;
		m_uOffset = m_uFileSize;
		m_uBuffSize = m_uFileSize + m_uFileSize/2;
		m_pBuff = _pBuff;

		if (!Parse())
		{
			return FALSE;
		}

		UINT uRow = GetRowCnt();

		return TRUE;
	}

	UINT GetRowCnt() const
	{
		if (m_mapPosField.size() == 0)
		{
			return 0;
		}

		return m_uRowCnt;
	}

	UINT GetColCnt() const 
	{
		return m_uColCnt;
	}

	CString GetContent(UINT _uRow, UINT _uCol)
	{
		CString strContent;
		if ((0 == _uRow) || (0 == _uCol) || (_uRow > m_uRowCnt) || (_uCol > m_uColCnt))
		{
			assert(0);
			return strContent;
		}

		CSV_POS pos = {_uRow-1, _uCol-1};
		FIELD_SIZE field = m_mapPosField[pos];

		if (field.uSize > 0)
		{
			memcpy_s(strContent.GetBuffer(field.uSize), field.uSize, m_pBuff+field.uBegin, field.uSize);
			strContent.ReleaseBuffer(field.uSize);
			ReEscape(strContent);
		}

		return strContent;
	}

	BOOL SetContent(UINT _uRow, UINT _uCol, LPCTSTR _lpcContent)
	{
		if ((0 == _uRow) || (0 == _uCol) || (_uRow > m_uRowCnt) || (_uCol > m_uColCnt))
		{
			assert(0);
			return FALSE;
		}

		CSV_POS pos = {_uRow-1, _uCol-1};
		FIELD_SIZE field = m_mapPosField[pos];

		CString strContent = _lpcContent;
		for (int uIdx = 0; uIdx < strContent.GetLength(); uIdx++)
		{
			if ((strContent.GetAt(uIdx) == m_delimiter) || (strContent.GetAt(uIdx) == '\r') || (strContent.GetAt(uIdx) == '\n') || (strContent.GetAt(uIdx) == '\"'))
			{
				field.nQuoteFlag = 1;
				break;
			}
		}

		Escape(strContent);

		size_t size = strContent.GetLength();
		if (size <= field.uEnd-field.uBegin)
		{
			memcpy_s(m_pBuff+field.uBegin, size, strContent, size);
		}
		else
		{
			if (m_uOffset+size > m_uBuffSize)
			{
				UINT uPreBuffSize = m_uBuffSize;
				m_uBuffSize = m_uBuffSize + m_uBuffSize/2;
				char* pBuff = new char[m_uBuffSize]();
				memcpy_s(pBuff, m_uBuffSize, m_pBuff, uPreBuffSize);
				delete[] m_pBuff;
				m_pBuff = pBuff;
			}

			memcpy_s(m_pBuff+m_uOffset, size, strContent, size);
			field.uBegin = m_uOffset;
			field.uSize = size;
			field.uEnd = field.uBegin + size;
			m_uOffset += size;
		}

		m_mapPosField[pos] = field;

		return TRUE;
	}

	BOOL InsertRow(UINT _uRowNo)
	{
		if ((_uRowNo > m_uRowCnt) || (0 == _uRowNo))
		{
			assert(0);
			return FALSE;
		}

		m_uRowCnt++;

		for (UINT uRow = m_uRowCnt-1; uRow > _uRowNo-1; uRow--)
		{
			for (UINT uCol = 0; uCol < m_uColCnt; uCol++)
			{
				CSV_POS pos = {uRow, uCol};
				CSV_POS nextLinePos = {uRow+1, uCol};
				std::swap(m_mapPosField[pos], m_mapPosField[nextLinePos]);
			}
		}

		return TRUE;
	}

	UINT AddRow()
	{
		m_uRowCnt++;
		return m_uRowCnt;
	}

	BOOL InserCol(UINT _uCol)
	{
		if ((_uCol > m_uColCnt) || (0 == _uCol))
		{
			assert(0);
			return FALSE;
		}

		m_uColCnt++;

		for (UINT uRow = 0; uRow < m_uRowCnt; uRow++)
		{
			for (UINT uCol = m_uColCnt-1; uCol > _uCol-1; uCol--)
			{
				CSV_POS pos = {uRow, uCol};
				CSV_POS nextLinePos = {uRow, uCol+1};
				std::swap(m_mapPosField[pos], m_mapPosField[nextLinePos]);
			}
		}

		return TRUE;
	}

	UINT AddCol()
	{
		m_uColCnt++;
		return m_uColCnt;
	}

	void AddRow(const std::vector<std::string>& _vstrField)
	{
		m_uRowCnt++;
		for (UINT uCol = 0; uCol < _vstrField.size(); uCol++)
		{
			SetContent(m_uRowCnt-1, uCol, _vstrField[uCol].c_str());
		}
	}

	void AddCol(const std::vector<std::string>& _vstrField)
	{

	}

	BOOL DeleteRow(UINT _uRowNo)
	{
		if ((_uRowNo > m_uRowCnt) || (0 == _uRowNo))
		{
			assert(0);
			return FALSE;
		}

		m_uRowCnt--;

		for (UINT uRow = _uRowNo-1; uRow <= m_uRowCnt; uRow++)
		{
			for (UINT uCol = 0; uCol < m_uColCnt; uCol++)
			{
				CSV_POS pos = {uRow, uCol};
				CSV_POS nextLinePos = {uRow+1, uCol};
				std::swap(m_mapPosField[pos], m_mapPosField[nextLinePos]);
			}
		}

		return TRUE;
	}

	BOOL DeleteCol(UINT _uCol)
	{
		if ((_uCol > m_uColCnt) || (0 == _uCol))
		{
			assert(0);
			return FALSE;
		}

		m_uColCnt--;

		for (UINT uRow = 0; uRow < m_uRowCnt; uRow++)
		{
			for (UINT uCol = _uCol-1; uCol <= m_uColCnt; uCol++)
			{
				CSV_POS pos = {uRow, uCol};
				CSV_POS nextLinePos = {uRow, uCol+1};
				std::swap(m_mapPosField[pos], m_mapPosField[nextLinePos]);
			}
		}

		return TRUE;
	}

	BOOL Save()
	{
		if (m_strPath.IsEmpty())
		{
			return FALSE;
		}

		return SaveAs(m_strPath);
	}

	BOOL SaveAs(LPCTSTR _lpcPath)
	{
		std::ofstream outFile(_lpcPath, std::ios::binary);
		CString strContent = GetAllContent();
		outFile.write(strContent.GetString(), strContent.GetLength());
		outFile.close();

		return TRUE;
	}

	CString GetAllContent()
	{
		CString strContent;
		for (UINT uRow = 0; uRow < m_uRowCnt; uRow++)
		{
			CString strLine;
			for (UINT uCol = 0; uCol < m_uColCnt; uCol++)
			{
				CSV_POS pos = {uRow, uCol};
				FIELD_SIZE field = m_mapPosField[pos];
				char szBuff[MAX_FIELD_SIZE] = {0};
				memcpy_s(szBuff, MAX_FIELD_SIZE, m_pBuff+field.uBegin, field.uSize);
				CString strField = szBuff;
				if (field.nQuoteFlag)
				{
					strField = "\"" + strField;
					strField += "\"";
				}

				strField += m_delimiter;
				strLine += strField;
			}
			strLine.Delete(strLine.GetLength()-1, 1);
			strLine += "\r\n";
			strContent += strLine;
		}

		return strContent;
	}

private:
	BOOL OpenFile()
	{
		std::ifstream file(m_strPath, std::ios::in|std::ios::binary|std::ios::ate);
		if (!file)
		{
			return FALSE;
		}

		m_uFileSize = static_cast<UINT>(file.tellg());
		m_uOffset = m_uFileSize;
		m_uBuffSize = m_uFileSize + m_uFileSize/2;
		m_pBuff = new char[m_uBuffSize]();
		file.seekg(0, std::ios::beg);
		file.read(m_pBuff, m_uFileSize);
		file.close();

		return TRUE;
	}

	BOOL Parse()
	{
		CSV_POS csvPos = {0};
		for (UINT uIdx = 0; uIdx < m_uFileSize; uIdx++)
		{
			FIELD_SIZE field = FindFieldSize(m_pBuff+uIdx, min(MAX_FIELD_SIZE, m_uFileSize-uIdx));
			field.uBegin += uIdx;
			uIdx += field.uEnd;
			Debug(field);
			m_mapPosField[csvPos] = field;
			csvPos.uCol++;

			m_uRowCnt = csvPos.uRow+1;
			if ((0xA == m_pBuff[uIdx]) || (0xD == m_pBuff[uIdx]) || (uIdx+1 == m_uFileSize))
			{
				if (((0xA == m_pBuff[uIdx]) && (0xD == m_pBuff[uIdx+1]))
					|| ((0xD == m_pBuff[uIdx]) && (0xA == m_pBuff[uIdx+1])))
				{
					uIdx++;
				}

				csvPos.uRow++;
				csvPos.uCol = 0;
			}
		}

		for (auto it = m_mapPosField.begin(); it != m_mapPosField.end(); ++it)
		{
			m_uColCnt = max(m_uColCnt, it->first.uCol);
		}
		m_uColCnt++;

		return TRUE;
	}

	FIELD_SIZE FindFieldSize(const char* _pBuff, UINT _uSize)
	{
		int nQuoteFlag = 0;
		FIELD_SIZE fieldInfo = {0};
		for (UINT uIdx = 0; uIdx < _uSize; uIdx++)
		{
			if (('"' == _pBuff[uIdx]) && (0 == nQuoteFlag))
			{
				nQuoteFlag = 1;
				continue;
			}

			if ((1 == nQuoteFlag) && ('"' == _pBuff[uIdx]))
			{
				if ((m_delimiter == _pBuff[uIdx+1]) || (0xA == _pBuff[uIdx+1]) || (0xD == _pBuff[uIdx+1]) || (uIdx+1 == _uSize))
				{
					fieldInfo.uBegin = 1;
					fieldInfo.uEnd = uIdx+1;
					fieldInfo.uSize = uIdx-1;
					fieldInfo.nQuoteFlag = 1;
					return fieldInfo;
				}
			}

			// if don't search the end of ",ignore the ".
			if ((MAX_FIELD_SIZE == uIdx) && (1 == nQuoteFlag))
			{
				nQuoteFlag = 3;
				uIdx = 0;
				continue;
			}

			if (1 == nQuoteFlag)
			{
				continue;
			}

			if ((m_delimiter == _pBuff[uIdx]) || (0xA == _pBuff[uIdx]) || (0xD == _pBuff[uIdx]))
			{
				fieldInfo.uSize = uIdx;
				fieldInfo.uEnd = uIdx;
				return fieldInfo;
			}

			if (uIdx+1 == _uSize)
			{
				fieldInfo.uSize = uIdx + 1;
				fieldInfo.uEnd = uIdx + 1;
				return fieldInfo;
			}
		}

		return fieldInfo;
	}

	void ReEscape(CString& _strContent)
	{
		int nPos = _strContent.Find("\"\"");
		if (nPos < 0)
		{
			return;
		}

		_strContent.Replace("\"\"", "\"");
	}

	void Escape(CString& _strContent)
	{
		int nPos = _strContent.Find("\"");
		if (nPos < 0)
		{
			return;
		}

		_strContent.Replace("\"", "\"\"");
	}

	void Debug(const FIELD_SIZE& _field)
	{
#ifdef _DEBUG1
		char szBuff[1024] = {0};
		memcpy_s(szBuff, 1024, m_pBuff+_field.uBegin, _field.uSize);
		OutputDebugString(szBuff);
		OutputDebugString("\r\n");
#endif
	}

private:
	char m_delimiter;
	CString m_strPath;
	char* m_pBuff;
	UINT m_uBuffSize;
	UINT m_uOffset;
	UINT m_uFileSize;
	map<CSV_POS, FIELD_SIZE> m_mapPosField;
	UINT m_uRowCnt;
	UINT m_uColCnt;
};

CCSVFileEx::CCSVFileEx(char _delimiter/* = ','*/)
	: m_pImpl(new CCSVFileEx::CPImpl(_delimiter))
{
}

CCSVFileEx::~CCSVFileEx(void)
{
}

BOOL CCSVFileEx::Init(LPCTSTR _lpcPath)
{
	assert(NULL != _lpcPath);
	return m_pImpl->Init(_lpcPath);
}

BOOL CCSVFileEx::Init(char* _pBuff, UINT _uSize)
{
	assert((NULL != _pBuff) && (_uSize > 0));
	return m_pImpl->Init(_pBuff, _uSize);
}

std::string CCSVFileEx::GetContent(UINT _uRow, UINT _uCol)
{
	return m_pImpl->GetContent(_uRow, _uCol).GetString();
}

BOOL CCSVFileEx::SetContent(UINT _uRow, UINT _uCol, LPCTSTR _lpcContent)
{
	return m_pImpl->SetContent(_uRow, _uCol, _lpcContent);
}

UINT CCSVFileEx::GetRowCnt() const
{
	return m_pImpl->GetRowCnt();
}

UINT CCSVFileEx::GetColCnt() const 
{
	return m_pImpl->GetColCnt();
}

BOOL CCSVFileEx::InsertRow(UINT _uRowNo)
{
	return m_pImpl->InsertRow(_uRowNo);
}

BOOL CCSVFileEx::InsertCol(UINT _uColNo)
{
	return m_pImpl->InserCol(_uColNo);
}

void CCSVFileEx::AddRow(const std::vector<std::string>& _vstrField)
{

}

void CCSVFileEx::AddCol(const std::vector<std::string>& _vstrField)
{

}

UINT CCSVFileEx::AddRow()
{
	return m_pImpl->AddRow();
}

UINT CCSVFileEx::AddCol()
{
	return m_pImpl->AddCol();
}

BOOL CCSVFileEx::DeleteRow(UINT _uRowNo)
{
	return m_pImpl->DeleteRow(_uRowNo);
}

BOOL CCSVFileEx::DeleteCol(UINT _uColNo)
{
	return m_pImpl->DeleteCol(_uColNo);
}

BOOL CCSVFileEx::Save()
{
	return m_pImpl->Save();
}

BOOL CCSVFileEx::SaveAs(LPCTSTR _lpcPath)
{
	return m_pImpl->SaveAs(_lpcPath);
}

std::string CCSVFileEx::GetAllContent()
{
	return m_pImpl->GetAllContent().GetString();
}