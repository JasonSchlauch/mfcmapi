// PropertyTagEditor.cpp : implementation file
//

#include "stdafx.h"
#include "PropertyTagEditor.h"
#include "InterpretProp.h"
#include "InterpretProp2.h"
#include "MAPIFunctions.h"
#include "NamedPropCache.h"

static TCHAR* CLASS = _T("CPropertyTagEditor");

enum __PropTagFields
{
	PROPTAG_TAG,
	PROPTAG_ID,
	PROPTAG_TYPE,
	PROPTAG_NAME,
	PROPTAG_TYPESTRING,
	PROPTAG_NAMEPROPKIND,
	PROPTAG_NAMEPROPNAME,
	PROPTAG_NAMEPROPGUID,
};

CPropertyTagEditor::CPropertyTagEditor(
	UINT uidTitle,
	UINT uidPrompt,
	ULONG ulPropTag,
	bool bIsAB,
	_In_opt_ LPMAPIPROP lpMAPIProp,
	_In_ CWnd* pParentWnd):
CEditor(pParentWnd,
		uidTitle?uidTitle:IDS_PROPTAGEDITOR,
		uidPrompt,
		0,
		CEDITOR_BUTTON_OK|CEDITOR_BUTTON_CANCEL|CEDITOR_BUTTON_ACTION1|(lpMAPIProp?CEDITOR_BUTTON_ACTION2:0),
		IDS_ACTIONSELECTPTAG,
		IDS_ACTIONCREATENAMEDPROP,
		NULL)
{
	TRACE_CONSTRUCTOR(CLASS);
	m_ulPropTag = ulPropTag;
	m_bIsAB = bIsAB;
	m_lpMAPIProp = lpMAPIProp;

	if (m_lpMAPIProp) m_lpMAPIProp->AddRef();

	CreateControls(m_lpMAPIProp?8:5);
	InitPane(PROPTAG_TAG, CreateSingleLinePane(IDS_PROPTAG, NULL, false));
	InitPane(PROPTAG_ID,  CreateSingleLinePane(IDS_PROPID, NULL, false));
	InitPane(PROPTAG_TYPE, CreateDropDownPane(IDS_PROPTYPE, 0, NULL, false));
	InitPane(PROPTAG_NAME, CreateSingleLinePane(IDS_PROPNAME, NULL, true));
	InitPane(PROPTAG_TYPESTRING, CreateSingleLinePane(IDS_PROPTYPE, NULL, true));

	// Map named properties if we can, but not for Address Books
	if (m_lpMAPIProp && !m_bIsAB)
	{
		InitPane(PROPTAG_NAMEPROPKIND, CreateDropDownPane(IDS_NAMEPROPKIND, 0, NULL, true));
		InitPane(PROPTAG_NAMEPROPNAME, CreateSingleLinePane(IDS_NAMEPROPNAME, NULL, false));
		InitPane(PROPTAG_NAMEPROPGUID, CreateDropDownGuidPane(IDS_NAMEPROPGUID, false));
	}
} // CPropertyTagEditor::CPropertyTagEditor

CPropertyTagEditor::~CPropertyTagEditor()
{
	TRACE_DESTRUCTOR(CLASS);
	if (m_lpMAPIProp) m_lpMAPIProp->Release();
} // CPropertyTagEditor::~CPropertyTagEditor

BOOL CPropertyTagEditor::OnInitDialog()
{
	BOOL bRet = CEditor::OnInitDialog();

	// initialize our dropdowns here
	ULONG ulDropNum = 0;

	// prop types
	for (ulDropNum=0 ; ulDropNum < ulPropTypeArray ; ulDropNum++)
	{
#ifdef UNICODE
		InsertDropString(PROPTAG_TYPE,ulDropNum,PropTypeArray[ulDropNum].lpszName);
#else
		HRESULT hRes = S_OK;
		LPSTR szAnsiName = NULL;
		EC_H(UnicodeToAnsi(PropTypeArray[ulDropNum].lpszName,&szAnsiName));
		if (SUCCEEDED(hRes))
		{
			InsertDropString(PROPTAG_TYPE,ulDropNum,szAnsiName);
		}
		delete[] szAnsiName;
#endif
	}

	if (m_lpMAPIProp)
	{
		InsertDropString(PROPTAG_NAMEPROPKIND,0,_T("MNID_STRING")); // STRING_OK
		InsertDropString(PROPTAG_NAMEPROPKIND,1,_T("MNID_ID")); // STRING_OK
	}

	PopulateFields(NOSKIPFIELD);

	return bRet;
} // CPropertyTagEditor::OnInitDialog

_Check_return_ ULONG CPropertyTagEditor::GetPropertyTag()
{
	return m_ulPropTag;
} // CPropertyTagEditor::GetPropertyTag

// Select a property tag
void CPropertyTagEditor::OnEditAction1()
{
	HRESULT hRes = S_OK;

	CPropertySelector MyData(
		m_bIsAB,
		m_lpMAPIProp,
		this);

	WC_H(MyData.DisplayDialog());
	if (S_OK != hRes) return;

	m_ulPropTag = MyData.GetPropertyTag();
	PopulateFields(NOSKIPFIELD);
} // CPropertyTagEditor::OnEditAction1

// GetNamesFromIDs - always with MAPI_CREATE
void CPropertyTagEditor::OnEditAction2()
{
	if (!m_lpMAPIProp) return;

	LookupNamedProp(NOSKIPFIELD, true);

	PopulateFields(NOSKIPFIELD);
} // CPropertyTagEditor::OnEditAction2

void CPropertyTagEditor::LookupNamedProp(ULONG ulSkipField, bool bCreate)
{
	HRESULT hRes = S_OK;

	ULONG ulPropType = GetSelectedPropType();

	GUID guid = {0};
	MAPINAMEID NamedID = {0};
	LPMAPINAMEID lpNamedID = NULL;
	lpNamedID = &NamedID;

	// Assume an ID to help with the dispid case
	NamedID.ulKind = MNID_ID;

	int iCurSel = 0;
	iCurSel = GetDropDownSelection(PROPTAG_NAMEPROPKIND);
	if (iCurSel != CB_ERR)
	{
		if (0 == iCurSel) NamedID.ulKind = MNID_STRING;
		if (1 == iCurSel) NamedID.ulKind = MNID_ID;
	}
	CString szName;
	LPWSTR	szWideName = NULL;
	szName = GetStringUseControl(PROPTAG_NAMEPROPNAME);

	// Convert our prop tag name to a wide character string
#ifdef UNICODE
	szWideName = (LPWSTR) (LPCWSTR) szName;
#else
	EC_H(AnsiToUnicode(szName,&szWideName));
#endif

	if (GetSelectedGUID(PROPTAG_NAMEPROPGUID, false, &guid))
	{
		NamedID.lpguid = &guid;
	}

	// Now check if that string is a known dispid
	LPNAMEID_ARRAY_ENTRY lpNameIDEntry = NULL;

	if (MNID_ID == NamedID.ulKind)
	{
		lpNameIDEntry = GetDispIDFromName(szWideName);

		// If we matched on a dispid name, use that for our lookup
		// Note that we should only ever reach this case if the user typed a dispid name
		if (lpNameIDEntry)
		{
			NamedID.Kind.lID = lpNameIDEntry->lValue;
			NamedID.lpguid = (LPGUID) lpNameIDEntry->lpGuid;
			ulPropType = lpNameIDEntry->ulType;
			lpNamedID = &NamedID;

			// We found something in our lookup, but later GetIDsFromNames call may fail
			// Make sure we write what we found back to the dialog
			// However, don't overwrite the field the user changed
			if (PROPTAG_NAMEPROPKIND != ulSkipField) SetDropDownSelection(PROPTAG_NAMEPROPKIND,_T("MNID_ID")); // STRING_OK

			if (PROPTAG_NAMEPROPGUID != ulSkipField)
			{
				LPTSTR szGUID = GUIDToString(lpNameIDEntry->lpGuid);
				SetDropDownSelection(PROPTAG_NAMEPROPGUID,szGUID);
				delete[] szGUID;
			}

			// This will accomplish setting the type field
			// If the stored type was PT_UNSPECIFIED, we'll just keep the user selected type
			if (PT_UNSPECIFIED != lpNameIDEntry->ulType)
			{
				m_ulPropTag = CHANGE_PROP_TYPE(m_ulPropTag,lpNameIDEntry->ulType);
			}
		}
		else
		{
			NamedID.Kind.lID = _tcstoul((LPCTSTR)szName,NULL,16);
		}
	}
	else if (MNID_STRING == NamedID.ulKind)
	{
		NamedID.Kind.lpwstrName = szWideName;
	}

	if (NamedID.lpguid &&
		((MNID_ID == NamedID.ulKind && NamedID.Kind.lID) || (MNID_STRING == NamedID.ulKind && NamedID.Kind.lpwstrName)))
	{
		LPSPropTagArray lpNamedPropTags = NULL;

		WC_H_GETPROPS(GetIDsFromNames(m_lpMAPIProp,
			1,
			&lpNamedID,
			bCreate?MAPI_CREATE:0,
			&lpNamedPropTags));

		if (lpNamedPropTags)
		{
			m_ulPropTag = CHANGE_PROP_TYPE(lpNamedPropTags->aulPropTag[0],ulPropType);
			MAPIFreeBuffer(lpNamedPropTags);
		}
	}

	if (MNID_STRING == NamedID.ulKind)
	{
		MAPIFreeBuffer(NamedID.Kind.lpwstrName);
	}
#ifndef UNICODE
	delete[] szWideName;
#endif
} // CPropertyTagEditor::LookupNamedProp

_Check_return_ ULONG CPropertyTagEditor::GetSelectedPropType()
{
	if (!IsValidDropDown(PROPTAG_TYPE)) return PT_NULL;

	CString szType;
	int iCurSel = 0;
	iCurSel = GetDropDownSelection(PROPTAG_TYPE);
	if (iCurSel != CB_ERR)
	{
		szType = PropTypeArray[iCurSel].lpszName;
	}
	else
	{
		szType = GetDropStringUseControl(PROPTAG_TYPE);
	}

	ULONG ulType = PropTypeNameToPropType(szType);

	return ulType;
} // CPropertyTagEditor::GetSelectedPropType

_Check_return_ ULONG CPropertyTagEditor::HandleChange(UINT nID)
{
	ULONG i = CEditor::HandleChange(nID);

	if ((ULONG) -1 == i) return (ULONG) -1;

	switch (i)
	{
	case(PROPTAG_TAG): // Prop tag changed
		{
			m_ulPropTag = GetPropTagUseControl(PROPTAG_TAG);
		}
		break;
	case(PROPTAG_ID): // Prop ID changed
		{
			CString szID;
			szID = GetStringUseControl(PROPTAG_ID);
			ULONG ulID = _tcstoul((LPCTSTR) szID,NULL,16);

			m_ulPropTag = PROP_TAG(PROP_TYPE(m_ulPropTag),ulID);
		}
		break;
	case(PROPTAG_TYPE): // Prop Type changed
		{
			ULONG ulType = GetSelectedPropType();

			m_ulPropTag = CHANGE_PROP_TYPE(m_ulPropTag,ulType);
		}
		break;
	case(PROPTAG_NAMEPROPKIND):
	case(PROPTAG_NAMEPROPNAME):
	case(PROPTAG_NAMEPROPGUID):
		{
			LookupNamedProp(i, false);
		}
		break;
	default:
		return i;
		break;
	}

	PopulateFields(i);

	return i;
} // CPropertyTagEditor::HandleChange

// Fill out the fields in the form
// Don't touch the field passed in ulSkipField
// Pass NOSKIPFIELD to fill out all fields
void CPropertyTagEditor::PopulateFields(ULONG ulSkipField)
{
	HRESULT hRes = S_OK;

	CString PropType;
	LPTSTR szExactMatch = NULL;
	LPTSTR szPartialMatch = NULL;
	LPTSTR szNamedPropName = NULL;

	InterpretProp(
		NULL,
		m_ulPropTag,
		m_lpMAPIProp,
		NULL,
		NULL,
		m_bIsAB,
		&szExactMatch, // Built from ulPropTag & bIsAB
		&szPartialMatch, // Built from ulPropTag & bIsAB
		&PropType,
		NULL,
		NULL,
		NULL,
		&szNamedPropName,
		NULL,
		NULL);

	if (PROPTAG_TAG != ulSkipField) SetHex(PROPTAG_TAG,m_ulPropTag);
	if (PROPTAG_ID != ulSkipField) SetStringf(PROPTAG_ID,_T("0x%04X"),PROP_ID(m_ulPropTag)); // STRING_OK
	if (PROPTAG_TYPE != ulSkipField) SetDropDownSelection(PROPTAG_TYPE,PropType);
	if (PROPTAG_NAME != ulSkipField)
	{
		if (PROP_ID(m_ulPropTag) && (szExactMatch || szPartialMatch))
			SetStringf(PROPTAG_NAME,_T("%s (%s)"),szExactMatch?szExactMatch:_T(""),szPartialMatch?szPartialMatch:_T("")); // STRING_OK
		else if (szNamedPropName)
			SetStringf(PROPTAG_NAME,_T("%s"),szNamedPropName); // STRING_OK
		else
			LoadString(PROPTAG_NAME,IDS_UNKNOWNPROPERTY);
	}
	if (PROPTAG_TYPESTRING != ulSkipField) SetString(PROPTAG_TYPESTRING,(LPCTSTR) TypeToString(m_ulPropTag));

	// Do a named property lookup and fill out fields
	// But only if PROPTAG_TAG or PROPTAG_ID is what the user changed
	// And never for Address Books
	if (m_lpMAPIProp && !m_bIsAB &&
		(PROPTAG_TAG == ulSkipField || PROPTAG_ID == ulSkipField ))
	{
		ULONG			ulPropNames = 0;
		SPropTagArray	sTagArray = {0};
		LPSPropTagArray lpTagArray = &sTagArray;
		LPMAPINAMEID*	lppPropNames = NULL;

		lpTagArray->cValues = 1;
		lpTagArray->aulPropTag[0] = m_ulPropTag;

		WC_H_GETPROPS(GetNamesFromIDs(m_lpMAPIProp,
			&lpTagArray,
			NULL,
			NULL,
			&ulPropNames,
			&lppPropNames));
		if (SUCCEEDED(hRes) && ulPropNames == lpTagArray->cValues && lppPropNames && lppPropNames[0])
		{
			if (MNID_STRING == lppPropNames[0]->ulKind)
			{
				if (PROPTAG_NAMEPROPKIND != ulSkipField) SetDropDownSelection(PROPTAG_NAMEPROPKIND,_T("MNID_STRING")); // STRING_OK
				if (PROPTAG_NAMEPROPNAME != ulSkipField) SetStringW(PROPTAG_NAMEPROPNAME,lppPropNames[0]->Kind.lpwstrName);
			}
			else if (MNID_ID == lppPropNames[0]->ulKind)
			{
				if (PROPTAG_NAMEPROPKIND != ulSkipField) SetDropDownSelection(PROPTAG_NAMEPROPKIND,_T("MNID_ID")); // STRING_OK
				if (PROPTAG_NAMEPROPNAME != ulSkipField) SetHex(PROPTAG_NAMEPROPNAME,lppPropNames[0]->Kind.lID);
			}
			else
			{
				if (PROPTAG_NAMEPROPNAME != ulSkipField) SetString(PROPTAG_NAMEPROPNAME,NULL);
			}
			if (PROPTAG_NAMEPROPGUID != ulSkipField)
			{
				LPTSTR szGUID = GUIDToString(lppPropNames[0]->lpguid);
				SetDropDownSelection(PROPTAG_NAMEPROPGUID,szGUID);
				delete[] szGUID;
			}
		}
		else
		{
			if (PROPTAG_NAMEPROPKIND != ulSkipField &&
				PROPTAG_NAMEPROPNAME != ulSkipField &&
				PROPTAG_NAMEPROPGUID != ulSkipField)
			{
				SetDropDownSelection(PROPTAG_NAMEPROPKIND,NULL);
				SetString(PROPTAG_NAMEPROPNAME,NULL);
				SetDropDownSelection(PROPTAG_NAMEPROPGUID,NULL);
			}
		}
		MAPIFreeBuffer(lppPropNames);
	}

	FreeNameIDStrings(szNamedPropName, NULL, NULL);
	delete[] szPartialMatch;
	delete[] szExactMatch;
} // CPropertyTagEditor::PopulateFields

void CPropertyTagEditor::SetDropDownSelection(ULONG i, _In_opt_z_ LPCTSTR szText)
{
	if (IsValidDropDown(i))
	{
		DropDownPane* lpPane = (DropDownPane*) GetControl(i);
		if (lpPane)
		{
			return lpPane->SetDropDownSelection(szText);
		}
	}
}

_Check_return_ CString CPropertyTagEditor::GetDropStringUseControl(ULONG iControl)
{
	if (IsValidDropDown(iControl))
	{
		DropDownPane* lpPane = (DropDownPane*) GetControl(iControl);
		if (lpPane)
		{
			return lpPane->GetDropStringUseControl();
		}
	}
	return _T("");
}

_Check_return_ int CPropertyTagEditor::GetDropDownSelection(ULONG iControl)
{
	if (IsValidDropDown(iControl))
	{
		DropDownPane* lpPane = (DropDownPane*) GetControl(iControl);
		if (lpPane)
		{
			return lpPane->GetDropDownSelection();
		}
	}
	return CB_ERR;
}

void CPropertyTagEditor::InsertDropString(ULONG iControl, int iRow, _In_z_ LPCTSTR szText)
{
	if (IsValidDropDown(iControl))
	{
		DropDownPane* lpPane = (DropDownPane*) GetControl(iControl);
		if (lpPane)
		{
			return lpPane->InsertDropString(iRow, szText);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////
// CPropertySelector
// Property selection dialog
// Displays a list of known property tags - no add or delete
//////////////////////////////////////////////////////////////////////////////////////

CPropertySelector::CPropertySelector(
	bool bIncludeABProps,
	_In_ LPMAPIPROP lpMAPIProp,
	_In_ CWnd* pParentWnd):
CEditor(pParentWnd,IDS_PROPSELECTOR,0,0,CEDITOR_BUTTON_OK|CEDITOR_BUTTON_CANCEL)
{
	TRACE_CONSTRUCTOR(CLASS);
	m_ulPropTag = PR_NULL;
	m_bIncludeABProps = bIncludeABProps;
	m_lpMAPIProp = lpMAPIProp;

	if (m_lpMAPIProp) m_lpMAPIProp->AddRef();

	CreateControls(1);
	InitPane(0, CreateListPane(IDS_KNOWNPROPTAGS, true, true, this));
} // CPropertySelector::CPropertySelector

CPropertySelector::~CPropertySelector()
{
	TRACE_DESTRUCTOR(CLASS);
	if (m_lpMAPIProp) m_lpMAPIProp->Release();
} // CPropertySelector::~CPropertySelector

BOOL CPropertySelector::OnInitDialog()
{
	BOOL bRet = CEditor::OnInitDialog();

	if (IsValidList(0))
	{
		CString szTmp;
		SortListData* lpData = NULL;
		ULONG i = 0;
		InsertColumn(0,1,IDS_PROPERTYNAMES);
		InsertColumn(0,2,IDS_TAG);
		InsertColumn(0,3,IDS_TYPE);

		ULONG ulCurRow = 0;
		for (i = 0;i< ulPropTagArray;i++)
		{
			if (!m_bIncludeABProps && (PropTagArray[i].ulValue & 0x80000000)) continue;
#ifdef UNICODE
			lpData = InsertListRow(0,ulCurRow,PropTagArray[i].lpszName);
#else
			HRESULT hRes = S_OK;
			LPSTR szAnsiName = NULL;
			EC_H(UnicodeToAnsi(PropTagArray[i].lpszName,&szAnsiName));
			if (SUCCEEDED(hRes))
			{
				lpData = InsertListRow(0,ulCurRow,szAnsiName);
			}
			delete[] szAnsiName;
#endif

			if (lpData)
			{
				lpData->ulSortDataType = SORTLIST_PROP;
				lpData->data.Prop.ulPropTag = PropTagArray[i].ulValue;
				lpData->bItemFullyLoaded = true;
			}

			szTmp.Format(_T("0x%08X"),PropTagArray[i].ulValue); // STRING_OK
			SetListString(0,ulCurRow,1,szTmp);
			SetListString(0,ulCurRow,2,TypeToString(PropTagArray[i].ulValue));
			ulCurRow++;
		}

		// Initial sort is by property tag
		ResizeList(0,true);
	}

	return bRet;
} // CPropertySelector::OnInitDialog

void CPropertySelector::OnOK()
{
	SortListData* lpListData = GetSelectedListRowData(0);
	if (lpListData)
		m_ulPropTag = lpListData->data.Prop.ulPropTag;

	CEditor::OnOK();
} // CPropertySelector::OnOK

// We're not actually editing the list here - just overriding this to allow double-click
// So it's OK to return false
_Check_return_ bool CPropertySelector::DoListEdit(ULONG /*ulListNum*/, int /*iItem*/, _In_ SortListData* /*lpData*/)
{
	OnOK();
	return false;
} // CPropertySelector::DoListEdit

_Check_return_ ULONG CPropertySelector::GetPropertyTag()
{
	return m_ulPropTag;
} // CPropertySelector::GetPropertyTag

_Check_return_ SortListData* CPropertySelector::GetSelectedListRowData(ULONG iControl)
{
	if (IsValidList(iControl))
	{
		ListPane* lpPane = (ListPane*) GetControl(iControl);
		if (lpPane)
		{
			return lpPane->GetSelectedListRowData();
		}
	}
	return NULL;
}