// TestCSV.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "TestCSV.h"
#include "CSVFile.h"
#include <vector>
#include <string>
#include "CSVFileEx.h"
using namespace std;
#ifdef _DEBUG
#define new DEBUG_NEW
#endif



// 获取指定目录下的所有文件
BOOL FindAllFile(LPCTSTR _lpcPath, vector<string>& _vstrAllFile)
{
	CFileFind finder; 
	CString strPath(_lpcPath);

	// start working for files
	strPath += _T("\\*.*"); 
	BOOL bWorking = finder.FindFile(strPath);

	while (bWorking)
	{
		bWorking = finder.FindNextFile();

		// skip . and .. files; otherwise, we'd
		// recur infinitely!
		if (finder.IsDots())
			continue;

		// if it's a directory, recursively search it
		CString str = finder.GetFilePath();
		if (finder.IsDirectory())
		{
			FindAllFile(str, _vstrAllFile);
		}
		else
		{
			_vstrAllFile.push_back(LPCTSTR(str));
		}
	}

	return TRUE;
}

// The one and only application object

CWinApp theApp;

using namespace std;

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	HMODULE hModule = ::GetModuleHandle(NULL);

	if (hModule != NULL)
	{
		// initialize MFC and print and error on failure
		if (!AfxWinInit(hModule, NULL, ::GetCommandLine(), 0))
		{
			// TODO: change error code to suit your needs
			_tprintf(_T("Fatal Error: MFC initialization failed\n"));
			nRetCode = 1;
		}
	}
	else
	{
		// TODO: change error code to suit your needs
		_tprintf(_T("Fatal Error: GetModuleHandle failed\n"));
		nRetCode = 1;
	}

	vector<string> vstrFile;
	FindAllFile("I:\\a\\inputs", vstrFile);

	CFile file;
	file.Open("I:\\a\\a2.txt", CFile::modeReadWrite|CFile::modeCreate);

	char* szBuff = new char[1024*1024*10]();
	UINT uOffset = 0;
	for (auto it = vstrFile.begin(); it != vstrFile.end(); ++it)
	{
		CString strPath = it->c_str();

		CCSVFileEx csv;
		csv.Init(strPath);

		strPath.Replace("inputs", "inputsa");
		csv.SaveAs(strPath);

		//CCSVFile csv(strPath);
		UINT uRowCnt = csv.GetRowCnt();
		UINT uColCnt = csv.GetColCnt();



		if ((uRowCnt == 0) && (uColCnt == 0))
			continue;

		for (UINT uRowNo = 1; uRowNo <= uRowCnt; uRowNo++)
		{
			for (UINT uColNo = 1; uColNo <= uColCnt; uColNo++)
			{
				CString strText = csv.GetContent(uRowNo, uColNo).c_str();
				// TRACE(strText+"\r\n");
				memcpy_s(szBuff+uOffset, 1024, strText.GetString(), strText.GetLength());
				uOffset += strText.GetLength();
				memcpy_s(szBuff+uOffset, 1024, ",", 1);
				uOffset++;
			}
			memcpy_s(szBuff+uOffset, 1024, "\r\n", 2);
			uOffset += 2;
		}
	}
	
	file.Write(szBuff, uOffset);
	file.Close();
	delete szBuff;
	std::cout<<"a2 is completed."<<std::endl;

	CCSVFileEx csv;
	csv.Init("i:\\a\\ssd.csv");
	CString str = csv.GetContent(1, 1).c_str();
	csv.SetContent(1, 1, "abc,dd");
	csv.SetContent(2, 1, "ab\"c,d\"d");
	csv.SetContent(1, 2, "dd\"a\"d,ee");
	csv.SetContent(3, 1, "武汉人");
	csv.SetContent(3, 2, "武汉\"人");
	csv.SetContent(3, 2, "武\"aaa\"汉\"人aaaaaa\r\nabbb");
 	csv.InsertRow(3);
 	csv.InsertRow(3);
	csv.InsertCol(2);
	csv.InsertCol(2);
	UINT uRowNo = csv.AddRow();
	csv.SetContent(uRowNo, 1, "acdfe");
	csv.SetContent(uRowNo, 2, "eeeee,badd");
	uRowNo = csv.AddRow();
	csv.SetContent(uRowNo, 1, "acd\rfe");
	csv.SetContent(uRowNo, 2, "eee\r\nee,badd");
	csv.SetContent(uRowNo, 3, "eee\r\nee,b\"add");
	csv.SetContent(uRowNo, 4, "eee\r\nee,b\"a\"d\"d");
	UINT uColNo = csv.AddCol();
	csv.SetContent(1, uColNo, "abc");
	csv.SetContent(3, uColNo, "a,bc");
	csv.SetContent(5, uColNo, "a\"bc");
	uColNo = csv.AddCol();
	csv.SetContent(1, uColNo, "abc");
	csv.SetContent(3, uColNo, "a,bc");
	csv.SetContent(7, uColNo, "a\"b11111c,");

	csv.SaveAs("i:\\a\\ssdModify.csv");

	for (UINT uIdx = 0; uIdx < 2; uIdx++)
	{	
		csv.DeleteCol(csv.GetColCnt());
		csv.DeleteRow(csv.GetRowCnt());
	}

	csv.SaveAs("i:\\a\\ssdDelete.csv");

	return nRetCode;
}
