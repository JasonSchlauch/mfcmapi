#pragma once
// MMFolder.h : Folder related utilities for MrMAPI

void DoChildFolders(_In_ MYOPTIONS ProgOpts);
void DoFolderProps(_In_ MYOPTIONS ProgOpts);
void DoFolderSize(_In_ MYOPTIONS ProgOpts);

HRESULT HrMAPIOpenFolderExW(
	_In_ LPMDB lpMdb,                        // Open message store
	_In_z_ LPCWSTR lpszFolderPath,           // folder path
	_Deref_out_opt_ LPMAPIFOLDER* lppFolder);// pointer to folder opened