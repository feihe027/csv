#include "StdAfx.h"
#include "cSVFile.h"
#include "SGFile.h"

#ifdef DEBUG
#define ENABLE_DEBUG
#endif

#ifdef ENABLE_DEBUG
#define MY_TRACE TRACE
#else
#define MY_TRACE //
#endif

const UINT INVALID_FLAG = 0xFFFFFFFF;
CCSVFile::CCSVFile(LPCTSTR _lpcFilaPath)
	: m_strFilePath(_lpcFilaPath)
	, m_pBuff(NULL)
	, m_uBuffSize(1024*1024)
	, m_uCurOffset(0)
{
	Parse();
}

CCSVFile::~CCSVFile(void)
{
	if (m_file.m_hFile != INVALID_HANDLE_VALUE)
	{
		m_file.Close();
	}
	if (NULL != m_pBuff)
	{
		delete[] m_pBuff;
		m_pBuff = NULL;
	}
}

BOOL CCSVFile::Parse()
{
	if (!m_file.Open(m_strFilePath, CSGFile::modeReadWrite))
	{
		return FALSE;
	}
	const UINT uSize = static_cast<UINT>(m_file.GetLength());
	m_uCurOffset = uSize;
	if (2*uSize >= m_uBuffSize)
	{
		m_uBuffSize = 2*uSize;
	}

	m_pBuff = new char[m_uBuffSize]();
	m_file.Read(m_pBuff, uSize);

	BYTE bQuotedFlag = 0;
	FILED_INFO filedInfo = {1, 1, 0, 0};
	for (UINT uIdx = 0; uIdx < uSize; uIdx++)
	{
		if (('"' == m_pBuff[uIdx]) && (0 == bQuotedFlag))
		{
			bQuotedFlag = 1;
			continue;
		}

		if (('"' == m_pBuff[uIdx]) && ('"' == m_pBuff[uIdx+1]))
		{
			uIdx++;
			continue;
		}

		if (('"' == m_pBuff[uIdx]) && (1 == bQuotedFlag))
		{
			bQuotedFlag = 2;
		}
		
		if (1 == bQuotedFlag)
		{
			continue;
		}

		BOOL bNewField = FALSE;
		if (',' == m_pBuff[uIdx])
		{
			filedInfo.uBuffEnd = uIdx;
			m_vFiled.push_back(filedInfo);
			char szBuff[1024] = {0};
			memcpy_s(szBuff, 1024, m_pBuff+filedInfo.uBuffStart, filedInfo.uBuffEnd-filedInfo.uBuffStart);
			MY_TRACE(szBuff);
			MY_TRACE("\r\n");
			filedInfo.uColNo++;
			filedInfo.uBuffStart = filedInfo.uBuffEnd+1;
			bNewField = TRUE;
		}
		else if ((0xA == m_pBuff[uIdx]) || (0xD == m_pBuff[uIdx]))
		{
			filedInfo.uBuffEnd = uIdx;
			m_vFiled.push_back(filedInfo);

			if (((0xA == m_pBuff[uIdx]) && (0xD == m_pBuff[uIdx+1]))
				|| ((0xD == m_pBuff[uIdx]) && (0xA == m_pBuff[uIdx+1])))
			{
				uIdx++;
			}
			
			char szBuff[1024] = {0};
			memcpy_s(szBuff, 1024, m_pBuff+filedInfo.uBuffStart, filedInfo.uBuffEnd-filedInfo.uBuffStart);
			MY_TRACE(szBuff);
			MY_TRACE("\r\n");

			filedInfo.uRowNo++;
			filedInfo.uColNo = 1;
			filedInfo.uBuffStart = uIdx+1;
			bNewField = TRUE;
		}
		else if (uSize == uIdx+1)
		{
			filedInfo.uBuffEnd = uIdx+1;
			m_vFiled.push_back(filedInfo);
			bNewField = TRUE;
			char szBuff[1024] = {0};
			memcpy_s(szBuff, 1024, m_pBuff+filedInfo.uBuffStart, filedInfo.uBuffEnd-filedInfo.uBuffStart);
			MY_TRACE(szBuff);
			MY_TRACE("\r\n");
		}

		// 如果接下来的新字段不是",则后面的"不作特殊处理
		if ((uIdx+1 < uSize) && bNewField && ('"' != m_pBuff[uIdx+1]))
		{
			bQuotedFlag = 2;
		}
		else if ((uIdx+1 < uSize) && bNewField && ('"' == m_pBuff[uIdx+1]))
		{
			bQuotedFlag = 0;
		}
	}

	return TRUE;
}

void CCSVFile::ReDefrag()
{

}

UINT CCSVFile::GetRowCnt()
{
	UINT uRowCnt = 0;
	for (auto it = m_vFiled.begin(); it != m_vFiled.end(); ++it)
	{
		if ((it->uRowNo > uRowCnt) && (it->uBuffStart != INVALID_FLAG))
		{
			uRowCnt = it->uRowNo;
		}
	}

	return uRowCnt;
}

UINT CCSVFile::GetColCnt()
{
	UINT uColCnt = 0;
	for (auto it = m_vFiled.begin(); it != m_vFiled.end(); ++it)
	{
		if ((it->uColNo > uColCnt) && (it->uBuffStart != INVALID_FLAG))
		{
			uColCnt = it->uColNo;
		}
	}

	return uColCnt;
}

CString CCSVFile::GetContent(UINT _uRow, UINT _uCol)
{
	FILED_INFO filedInfo = {0};
	for (auto it = m_vFiled.begin(); it != m_vFiled.end(); ++it)
	{
		if ((it->uRowNo == _uRow) && (it->uColNo == _uCol) && (it->uBuffStart != INVALID_FLAG))
		{
			filedInfo = *it;
		}
	}

	if ((0 == filedInfo.uRowNo) || (filedInfo.uBuffEnd == filedInfo.uBuffStart))
	{
		return CString("");
	}

	char szBuff[1024] = {0};

	// 如果有双引号，去掉
	BOOL bQuotedFlag = FALSE;
	if (('"' == m_pBuff[filedInfo.uBuffStart]) && (filedInfo.uBuffEnd > 1) && ('"' == m_pBuff[filedInfo.uBuffEnd-1]))
	{
		bQuotedFlag = TRUE;
		memcpy_s(szBuff, sizeof(szBuff), m_pBuff+filedInfo.uBuffStart+1, filedInfo.uBuffEnd-filedInfo.uBuffStart-2);
	}
	else
	{
		memcpy_s(szBuff, sizeof(szBuff), m_pBuff+filedInfo.uBuffStart, filedInfo.uBuffEnd-filedInfo.uBuffStart);
	}

	// 相邻双引号去掉前面的
	// 如果没有双引号,则去掉最后的0xA,0xD
	CString strText;
	UINT uSize = strlen(szBuff);
	for (UINT uIdx = 0; uIdx < uSize; uIdx++)
	{
		if (('"' == szBuff[uIdx]) && (uIdx+1 < uSize) && ('"' == szBuff[uIdx+1]))
		{
			continue;
		}

		if ((uIdx+2 == uSize) || (uIdx+1 == uSize))
		{
			if (!bQuotedFlag 
				&& ((0xA == szBuff[uIdx]) || (0xD == szBuff[uIdx])))
			{
				continue;
			}
		}

		strText += szBuff[uIdx];
	}

	return strText;
}

BOOL CCSVFile::IsSpecial(LPCTSTR _lpcContent)
{
	UINT uQuotationCnt = 0;
	for (UINT uIdx = 0; uIdx < strlen(_lpcContent); uIdx++)
	{
		if ((',' == _lpcContent[uIdx]) || (0xA == _lpcContent[uIdx]) || (0xD == _lpcContent[uIdx]) || ('"' == _lpcContent[uIdx]))
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CCSVFile::SetContent(UINT _uRow, UINT _uCol, LPCTSTR _lpcContent)
{
	UINT uRowCnt = GetRowCnt();
	UINT uColCnt = GetColCnt();
	if ((_uRow > uRowCnt) || (_uCol > uColCnt))
	{
		return FALSE;
	}

	if (m_uCurOffset*2 > m_uBuffSize)
	{
		char* pNewBuff = new char[m_uCurOffset*2]();
		memcpy_s(pNewBuff, m_uBuffSize, m_pBuff, m_uBuffSize);
		delete[] m_pBuff;
		m_pBuff = pNewBuff;
		m_uBuffSize = m_uCurOffset*2;
	}

	if (NULL == _lpcContent)
	{
		return FALSE;
	}

	CString strText = _lpcContent;
	if (IsSpecial(_lpcContent))
	{
		strText = "\"" + strText + "\"";
	}

	CString strNewText;
	if (strText.GetLength() > 1)
	{
		strNewText = strText.Left(1);
		for (UINT uIdx = 1; uIdx < strText.GetLength()-1; uIdx++)
		{
			if ((uIdx+2 < strText.GetLength()) && ('"' == strText[uIdx])  && ('"' != strText[uIdx+1]))
			{
				strNewText += "\"";
			}
			strNewText += strText[uIdx];
		}
		strNewText += strText.Right(1);
	}
	else
	{
		strNewText = strText;
	}
	
	FILED_INFO filedInfo = {_uRow, _uCol, m_uCurOffset, 0};
	filedInfo.uBuffEnd = filedInfo.uBuffStart + strNewText.GetLength();
	m_vFiled.push_back(filedInfo);

	memcpy_s(m_pBuff+filedInfo.uBuffStart, 1024, strNewText.GetString(), strNewText.GetLength());
	m_uCurOffset += strNewText.GetLength();

	return TRUE;
}

BOOL CCSVFile::InsertRow(UINT _uRowNo)
{
	UINT uRowCnt = GetRowCnt();
	if (_uRowNo > uRowCnt)
	{
		ASSERT(0);
		return FALSE;
	}

	for (auto it = m_vFiled.begin(); it != m_vFiled.end(); ++it)
	{
		if (it->uRowNo >= _uRowNo)
		{
			it->uRowNo++;
		}
	}

	UINT uColCnt = GetColCnt();
	for (UINT uIdx = 1; uIdx <= uColCnt; uIdx++)
	{
		SetContent(_uRowNo, uIdx, "");
	}

	return TRUE;
}

BOOL CCSVFile::InsertCol(UINT _uColNo)
{
	UINT uColCnt = GetColCnt();
	if (_uColNo > uColCnt)
	{
		ASSERT(0);
		return FALSE;
	}

	for (auto it = m_vFiled.begin(); it != m_vFiled.end(); ++it)
	{
		if (it->uColNo >= _uColNo)
		{
			it->uColNo++;
		}
	}

	UINT uRowCnt = GetRowCnt();
	for (UINT uIdx = 1; uIdx <= uRowCnt; uIdx++)
	{
		SetContent(uIdx, _uColNo, "");
	}

	return TRUE;
}

UINT CCSVFile::AddRow()
{
	UINT uColCnt = GetColCnt();
	UINT uRowCnt = GetRowCnt();
	for (UINT uIdx = 1; uIdx <= uColCnt; uIdx++)
	{
		FILED_INFO filedInfo = {uRowCnt+1, uIdx, m_uCurOffset, m_uCurOffset};
		m_vFiled.push_back(filedInfo);
	}

	return uRowCnt+1;
}

UINT CCSVFile::AddCol()
{
	UINT uColCnt = GetColCnt();
	UINT uRowCnt = GetRowCnt();
	for (UINT uIdx = 1; uIdx <= uRowCnt; uIdx++)
	{
		FILED_INFO filedInfo = {uIdx, uColCnt+1, m_uCurOffset, m_uCurOffset};
		m_vFiled.push_back(filedInfo);
	}

	return uColCnt+1;
}


BOOL CCSVFile::DeleteRow(UINT _uRowNo)
{
	UINT uColCnt = GetColCnt();
	UINT uRowCnt = GetRowCnt();
	if (_uRowNo > uRowCnt)
	{
		ASSERT(0);
		return FALSE;
	}

	for (auto it = m_vFiled.begin(); it != m_vFiled.end(); ++it)
	{
		if (it->uRowNo == _uRowNo)
		{
			it->uBuffStart = INVALID_FLAG;
			it->uBuffEnd = INVALID_FLAG;
		}
		else if (it->uRowNo > _uRowNo)
		{
			it->uRowNo--;
		}
	}

	return TRUE;
}

BOOL CCSVFile::DeleteCol(UINT _uColNo)
{
	UINT uColCnt = GetColCnt();
	UINT uRowCnt = GetRowCnt();
	if (_uColNo > uColCnt)
	{
		ASSERT(0);
		return FALSE;
	}

	for (auto it = m_vFiled.begin(); it != m_vFiled.end(); ++it)
	{
		if (it->uColNo == _uColNo)
		{
			it->uBuffStart = INVALID_FLAG;
			it->uBuffEnd = INVALID_FLAG;
		}
		else if (it->uColNo > _uColNo)
		{
			it->uColNo--;
		}
	}

	return TRUE;
}

BOOL CCSVFile::Flush()
{	
	m_file.Close();
	if (!m_file.Open(m_strFilePath, CSGFile::modeReadWrite|CSGFile::modeCreate))
	{
		return FALSE;
	}

	UINT uRowCnt = GetRowCnt();
	UINT uColCnt = GetColCnt();
	for (UINT uRowNo = 1; uRowNo <= uRowCnt; uRowNo++)
	{
		for (UINT uColNo = 1; uColNo <= uColCnt; uColNo++)
		{
			FILED_INFO filedInfo = {uRowNo, uColNo, INVALID_FLAG, INVALID_FLAG};

			for (auto itDest = m_vFiled.begin(); itDest != m_vFiled.end(); ++itDest)
			{
				if ((filedInfo.uRowNo == itDest->uRowNo) && (filedInfo.uColNo == itDest->uColNo))
				{
					filedInfo = *itDest;
				}
			}
			if (INVALID_FLAG != filedInfo.uBuffStart)
			{
				m_file.Write(m_pBuff+filedInfo.uBuffStart, filedInfo.uBuffEnd-filedInfo.uBuffStart);
				char szBuff[1024] = {0};
				memcpy_s(szBuff, 1024, m_pBuff+filedInfo.uBuffStart, filedInfo.uBuffEnd-filedInfo.uBuffStart);
				MY_TRACE(szBuff);
				MY_TRACE("\r\n");
			}

			if (uColNo < uColCnt)
			{
				m_file.Write(",",1 );
			}
			else
			{
				m_file.Write("\n",1 );
			}
		}
	}
	
	return TRUE;
}

void CCSVFile::Close()
{
	m_file.Close();
}