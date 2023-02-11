#include "HardwareInfo.h"
using namespace std;
WMICI wmi;

BOOL ProgStartedWithAdmin()
{
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID SecurityIdentifier;
    if (!AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &SecurityIdentifier))
        return 0;

    BOOL IsAdminMember;
    if (!CheckTokenMembership(NULL, SecurityIdentifier, &IsAdminMember))
        IsAdminMember = FALSE;

    FreeSid(SecurityIdentifier);

    return IsAdminMember;
}

int main()
{
    if (ProgStartedWithAdmin())
    {
        wmi.createtext("Info.txt");
        cout << "Press smth...";
        _getch();
    }
}
