#pragma once
#include <string>
#include <atltime.h>

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
#define _MAX_PATH   260 /* max. length of full pathname */

class CSGFile
{
public:
	// Flag values
	enum OpenFlags {
		modeRead =         (int) 0x00000,
		modeWrite =        (int) 0x00001,
		modeReadWrite =    (int) 0x00002,
		shareCompat =      (int) 0x00000,
		shareExclusive =   (int) 0x00010,
		shareDenyWrite =   (int) 0x00020,
		shareDenyRead =    (int) 0x00030,
		shareDenyNone =    (int) 0x00040,
		modeNoInherit =    (int) 0x00080,
		modeCreate =       (int) 0x01000,
		modeNoTruncate =   (int) 0x02000,
		typeText =         (int) 0x04000, // typeText and typeBinary are
		typeBinary =       (int) 0x08000, // used in derived classes only
		osNoBuffer =       (int) 0x10000,
		osWriteThrough =   (int) 0x20000,
		osRandomAccess =   (int) 0x40000,
		osSequentialScan = (int) 0x80000,
	};

	enum Attribute {
		normal =    0x00,
		readOnly =  0x01,
		hidden =    0x02,
		system =    0x04,
		volume =    0x08,
		directory = 0x10,
		archive =   0x20
	};

	typedef void * HANDLE;

	enum SeekPosition { begin = 0x0, current = 0x1, end = 0x2 };

	static const HANDLE hFileNull;

	// Constructors
	CSGFile();

	CSGFile(HANDLE hFile);
	CSGFile(LPCTSTR lpszFileName, UINT nOpenFlags);

	// Attributes
	HANDLE m_hFile;
	operator HANDLE() const;

	virtual ULONGLONG GetPosition() const;

	// Operations
	virtual bool Open(LPCTSTR lpszFileName, UINT nOpenFlags);

	virtual ULONGLONG Seek(LONGLONG lOff, UINT nFrom);
	virtual void SetLength(ULONGLONG dwNewLen);
	virtual ULONGLONG GetLength() const;

	virtual UINT Read(void* lpBuf, UINT nCount);
	virtual void Write(const void* lpBuf, UINT nCount);

	virtual void Flush();
	virtual void Close();

	// Implementation
	virtual ~CSGFile();

private:
	std::string m_strFileName;
};

