#include "stdafx.h"
#include "SGFile.h"
#include <assert.h>



CSGFile::CSGFile()
{
	m_hFile = INVALID_HANDLE_VALUE;
}

CSGFile::CSGFile(HANDLE hFile)
{
	m_hFile = hFile;
}

CSGFile::CSGFile(LPCTSTR lpszFileName, UINT nOpenFlags)
{
	Open(lpszFileName, nOpenFlags);
}

CSGFile::~CSGFile()
{
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		Close();
	}
}

bool CSGFile::Open(LPCTSTR lpszFileName, UINT nOpenFlags)
{
	assert((nOpenFlags & typeText) == 0);   // text mode not supported

	// shouldn't open an already open file (it will leak)
	assert(m_hFile == INVALID_HANDLE_VALUE);

	// CSGFile objects are always binary and CreateFile does not need flag
	nOpenFlags &= ~(UINT)typeBinary;

	m_hFile = INVALID_HANDLE_VALUE;

	assert(strlen(lpszFileName) <= _MAX_PATH);

	m_strFileName = lpszFileName;

	// map read/write mode
	assert((modeRead|modeWrite|modeReadWrite) == 3);
	DWORD dwAccess = 0;
	switch (nOpenFlags & 3)
	{
	case modeRead:
		dwAccess = GENERIC_READ;
		break;
	case modeWrite:
		dwAccess = GENERIC_WRITE;
		break;
	case modeReadWrite:
		dwAccess = GENERIC_READ | GENERIC_WRITE;
		break;
	default:
		assert(false);  // invalid share mode
	}

	// map share mode
	DWORD dwShareMode = 0;
	switch (nOpenFlags & 0x70)    // map compatibility mode to exclusive
	{
	default:
		assert(false);  // invalid share mode?
	case shareCompat:
	case shareExclusive:
		dwShareMode = 0;
		break;
	case shareDenyWrite:
		dwShareMode = FILE_SHARE_READ;
		break;
	case shareDenyRead:
		dwShareMode = FILE_SHARE_WRITE;
		break;
	case shareDenyNone:
		dwShareMode = FILE_SHARE_WRITE | FILE_SHARE_READ;
		break;
	}

	// Note: typeText and typeBinary are used in derived classes only.

	// map modeNoInherit flag
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = (nOpenFlags & modeNoInherit) == 0;

	// map creation flags
	DWORD dwCreateFlag;
	if (nOpenFlags & modeCreate)
	{
		if (nOpenFlags & modeNoTruncate)
			dwCreateFlag = OPEN_ALWAYS;
		else
			dwCreateFlag = CREATE_ALWAYS;
	}
	else
		dwCreateFlag = OPEN_EXISTING;

	// special system-level access flags

	// Random access and sequential scan should be mutually exclusive
	assert((nOpenFlags&(osRandomAccess|osSequentialScan)) != (osRandomAccess|osSequentialScan) );

	DWORD dwFlags = FILE_ATTRIBUTE_NORMAL;
	if (nOpenFlags & osNoBuffer)
		dwFlags |= FILE_FLAG_NO_BUFFERING;
	if (nOpenFlags & osWriteThrough)
		dwFlags |= FILE_FLAG_WRITE_THROUGH;
	if (nOpenFlags & osRandomAccess)
		dwFlags |= FILE_FLAG_RANDOM_ACCESS;
	if (nOpenFlags & osSequentialScan)
		dwFlags |= FILE_FLAG_SEQUENTIAL_SCAN;

	// attempt file creation
	HANDLE hFile = ::CreateFile(lpszFileName, dwAccess, dwShareMode, &sa, dwCreateFlag, dwFlags, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		assert(false);
		return false;
	}
	m_hFile = hFile;

	return true;
}

UINT CSGFile::Read(void* lpBuf, UINT nCount)
{
	assert(m_hFile != INVALID_HANDLE_VALUE);

	if (nCount == 0)
		return 0;   // avoid Win32 "null-read"

	assert(lpBuf != NULL);

	DWORD dwRead;
	if (!::ReadFile(m_hFile, lpBuf, nCount, &dwRead, NULL))
		assert(false);

	return (UINT)dwRead;
}

void CSGFile::Write(const void* lpBuf, UINT nCount)
{
	assert(m_hFile != INVALID_HANDLE_VALUE);

	if (nCount == 0)
		return;     // avoid Win32 "null-write" option

	assert(lpBuf != NULL);

	DWORD nWritten;
	if (!::WriteFile(m_hFile, lpBuf, nCount, &nWritten, NULL))
		assert(false);

	if (nWritten != nCount)
		assert(false);
}

ULONGLONG CSGFile::Seek(LONGLONG lOff, UINT nFrom)
{
	assert(m_hFile != INVALID_HANDLE_VALUE);
	assert(nFrom == begin || nFrom == end || nFrom == current);
	assert(begin == FILE_BEGIN && end == FILE_END && current == FILE_CURRENT);

	LARGE_INTEGER liOff;

	liOff.QuadPart = lOff;
	liOff.LowPart = ::SetFilePointer(m_hFile, liOff.LowPart, &liOff.HighPart,
		(DWORD)nFrom);
	if (liOff.LowPart  == (DWORD)-1)
		if (::GetLastError() != NO_ERROR)
			assert(false);

	return liOff.QuadPart;
}

ULONGLONG CSGFile::GetPosition() const
{
	assert(m_hFile != INVALID_HANDLE_VALUE);

	LARGE_INTEGER liPos;
	liPos.QuadPart = 0;
	liPos.LowPart = ::SetFilePointer(m_hFile, liPos.LowPart, &liPos.HighPart , FILE_CURRENT);
	if (liPos.LowPart == (DWORD)-1)
		if (::GetLastError() != NO_ERROR)
			assert(false);

	return liPos.QuadPart;
}

void CSGFile::Flush()
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return;

	if (!::FlushFileBuffers(m_hFile))
		assert(false);
}

void CSGFile::Close()
{
	assert(m_hFile != INVALID_HANDLE_VALUE);
	bool bError = false;
	if (m_hFile != INVALID_HANDLE_VALUE)
		bError = !::CloseHandle(m_hFile);

	m_hFile = INVALID_HANDLE_VALUE;
	m_strFileName.empty();

	if (bError)
		assert(false);
}

void CSGFile::SetLength(ULONGLONG dwNewLen)
{
	assert(m_hFile != INVALID_HANDLE_VALUE);

	Seek(dwNewLen, (UINT)begin);

	if (!::SetEndOfFile(m_hFile))
		assert(false);
}

ULONGLONG CSGFile::GetLength() const
{
	ULARGE_INTEGER liSize;
	liSize.LowPart = ::GetFileSize(m_hFile, &liSize.HighPart);
	if (liSize.LowPart == INVALID_FILE_SIZE)
		if (::GetLastError() != NO_ERROR)
			assert(false);

	return liSize.QuadPart;
}
