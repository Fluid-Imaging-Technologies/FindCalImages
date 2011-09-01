

#include "stdafx.h"
#include "resource.h"
#include <io.h>
#include <time.h>
#include <stdio.h>

char default_oldestFileDate[] = "01/01/2011";
char default_SearchFolder[] = "C:\\TEMP\\TESTFOLDER";
char default_SaveFolder[] = "C:\\FlowCAM\\Experiments\\Calibration Images";

time_t BeginTime;
char SearchFolder[MAX_PATH];
char SaveFolder[MAX_PATH];

HINSTANCE hInst;

INT_PTR CALLBACK	MainDialog(HWND, UINT, WPARAM, LPARAM);
bool directory_exists(const char *dirname);
void copyfiles(char *dir, char* dirname);
HWND mdialog;
int NFilesCopied = 0;

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;

	DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG), NULL, MainDialog);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int) msg.wParam;
}


INT_PTR CALLBACK MainDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	mdialog = hDlg;

	switch (message)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hDlg, IDC_START_DATE, default_oldestFileDate);
		SetDlgItemText(hDlg, IDC_SEARCH_FOLDER, default_SearchFolder);
		SetDlgItemText(hDlg, IDC_SAVE_FOLDER, default_SaveFolder);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			char datestr[100];
			char month[3], day[3], year[5];
			int imonth, iday, iyear;
			int shift;

			memset(datestr, 0,20);
			GetDlgItemText(hDlg, IDC_START_DATE, datestr, 100);

			imonth = 0;
			iday = 0;
			iyear = 0;
			strncpy_s (month, 3, datestr, 2);
			imonth = atoi(month);
			shift = 3;
			if (month[1] == '/') shift = 2; 
			strncpy_s (day, 3, datestr + shift, 2);
			iday = atoi(day);
			shift = shift + 3;
			if (day[1] == '/') --shift;
			strncpy_s (year, 5, datestr + shift, 4);
			iyear = atoi(year);
			if (iyear < 50) iyear = iyear + 2000;

			if (imonth < 1 || imonth > 12 || iday < 1 || iday > 31 || iyear < 2000) 
			{
				MessageBox (0, "Invalid Oldest File Date", "INVALID DATE", 0);
				return (INT_PTR) FALSE;
			}

			struct tm fromuser;
			memset (&fromuser, 0, sizeof(fromuser));
			fromuser.tm_year = iyear - 1900;
			fromuser.tm_mday = iday;
			fromuser.tm_mon = imonth - 1;
			BeginTime = mktime(&fromuser);

			time_t now;
			time(&now);
			if (BeginTime > now) 
			{
				MessageBox (0, "Oldest File Date cannot be a future date", "FUTURE DATE", 0);
				return (INT_PTR) FALSE;
			}

			memset(SearchFolder, 0, sizeof(SearchFolder));
			GetDlgItemText(hDlg, IDC_SEARCH_FOLDER, SearchFolder, sizeof(SearchFolder));
			if (!directory_exists(SearchFolder)) 
			{
				MessageBox (0, "Search Folder not Found", "FOLDER NOT FOUND", 0);
				return (INT_PTR) FALSE;
			}

			memset(SaveFolder, 0, sizeof(SaveFolder));
			GetDlgItemText(hDlg, IDC_SAVE_FOLDER, SaveFolder, sizeof(SaveFolder));
			if (!directory_exists(SaveFolder)) 
			{
				MessageBox (0, "Save Folder not Found", "FOLDER NOT FOUND", 0);
				return (INT_PTR) FALSE;
			}

			SetDlgItemText(mdialog, IDC_STATUS, "RUNNING");
			SetDlgItemText(mdialog, IDC_COPY_COUNT, "0");
			copyfiles(SearchFolder, "basename");
			SetDlgItemText(mdialog, IDC_STATUS, "COMPLETE");
			Sleep(3000);

			EndDialog(hDlg, LOWORD(wParam));
			exit(0);			
		}

		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, 0);
			exit(0);			
		}
	}
	return (INT_PTR)FALSE;
}

bool directory_exists(const char *dirname)
{
	WIN32_FILE_ATTRIBUTE_DATA fileInfo;

	if (dirname && *dirname) 
	{
		if (!GetFileAttributesEx(dirname, GetFileExInfoStandard, &fileInfo)) 
		{
			return false;
		}

		if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
		{
			return true;
		}
	}

	return false;
}

void copyfiles(char *dir, char *dirname) 
{
	char searchdir[MAX_PATH];
	char subdir[MAX_PATH];
	_finddata_t finddata;
	intptr_t handle;
	bool first;

	sprintf_s(searchdir, sizeof(searchdir), "%s\\*.*", dir);

	handle = _findfirst(searchdir, &finddata);
	if (handle == -1)
		return;

	first = true;
	while (true) 
	{
		if (!first) 
		{
			if (_findnext(handle, &finddata) != 0)
				break;
		}
	    first = false;
		if (strcmp(finddata.name, ".") == 0 || strcmp(finddata.name, "..") == 0)
			continue;
		if (finddata.attrib == _A_SUBDIR)
		{
			sprintf_s(subdir, sizeof(subdir), "%s\\%s", dir, finddata.name);
			copyfiles(subdir, finddata.name);
		}
		else if (strncmp(finddata.name, "cal_image", 8) == 0) 
		{
			if (finddata.time_write >= BeginTime) 
			{
				int image_number = 1;
				if (strlen(finddata.name) >= 10)
					image_number = atoi(finddata.name + 10);

				struct tm filetime;
				if (localtime_s(&filetime, &finddata.time_write) == 0)
				{
					char oldname[MAX_PATH], newname[MAX_PATH];

					sprintf_s(oldname, sizeof(oldname), "%s\\%s", dir, finddata.name);

					sprintf_s(newname, sizeof(newname), 
						"%s\\%04d_%02d_%02d_%02d_%02d__%s_%03d.tif", 
						SaveFolder, 
						filetime.tm_year + 1900,
						filetime.tm_mon + 1,
						filetime.tm_mday,
						filetime.tm_hour,
						filetime.tm_min,
						dirname,
						image_number);
					
					CopyFile(oldname, newname, true);

					char buf[10];
					sprintf_s(buf, "%d", ++NFilesCopied);
					SetDlgItemText(mdialog, IDC_COPY_COUNT, buf);
				}
			}
		}
	}
}

