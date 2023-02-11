#include "HardwareInfo.h"

template <typename T>
inline bool queryWMI(const string& WMIClass, string field, vector<T>& value, const string& serverName = "ROOT\\CIMV2") {
    std::string query("SELECT " + field + " FROM " + WMIClass);

    HRESULT hres;
    hres = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        return false;
    }
    hres = CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr, EOAC_NONE, nullptr);
    if (FAILED(hres)) {
        CoUninitialize();
        return false;
    }
    IWbemLocator* pLoc = nullptr;
    hres = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hres)) {
        CoUninitialize();
        return false;
    }
    IWbemServices* pSvc = nullptr;
    hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, nullptr, 0, nullptr, nullptr, &pSvc);
    if (FAILED(hres)) {
        pLoc->Release();
        CoUninitialize();
        return false;
    }
    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return false;
    }
    IEnumWbemClassObject* pEnumerator = nullptr;
    hres = pSvc->ExecQuery(bstr_t(L"WQL"), bstr_t(std::wstring(query.begin(), query.end()).c_str()),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnumerator);
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return false;
    }
    IWbemClassObject* pclsObj = nullptr;
    ULONG uReturn = 0;
    while (pEnumerator) {
        pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

        if (!uReturn) {
            break;
        }

        VARIANT vtProp;
        pclsObj->Get(std::wstring(field.begin(), field.end()).c_str(), 0, &vtProp, nullptr, nullptr);

        if (std::is_same<T, long>::value || std::is_same<T, int>::value) {
            value.push_back((T)vtProp.intVal);
        }
        else if (std::is_same<T, bool>::value) {
            value.push_back((T)vtProp.boolVal);
        }
        else if (std::is_same<T, unsigned>::value) {
            value.push_back((T)vtProp.uintVal);
        }
        else if (std::is_same<T, unsigned short>::value) {
            value.push_back((T)vtProp.uiVal);
        }
        else if (std::is_same<T, long long>::value) {
            value.push_back((T)vtProp.llVal);
        }
        else if (std::is_same<T, unsigned long long>::value) {
            value.push_back((T)vtProp.ullVal);
        }
        else {
            value.push_back((T)((bstr_t)vtProp.bstrVal).copy());
        }

        VariantClear(&vtProp);
        pclsObj->Release();
    }

    if (value.empty()) {
        value.resize(1);
    }

    pSvc->Release();
    pLoc->Release();
    if (pEnumerator) pEnumerator->Release();
    CoUninitialize();
    return true;
}

string getIs64bit() {
    BOOL bWow64Process = FALSE;
    IsWow64Process(GetCurrentProcess(), &bWow64Process) && bWow64Process;
    if (bWow64Process)
        return "x64";
    else
        return "x86";
}

std::string getFullName() {
    static NTSTATUS(__stdcall * RtlGetVersion)(OUT PRTL_OSVERSIONINFOEXW lpVersionInformation) =
        (NTSTATUS(__stdcall*)(PRTL_OSVERSIONINFOEXW))GetProcAddress(GetModuleHandle(L"ntdll.dll"), "RtlGetVersion");
    static void(__stdcall * GetNativeSystemInfo)(OUT LPSYSTEM_INFO lpSystemInfo) =
        (void(__stdcall*)(LPSYSTEM_INFO))GetProcAddress(GetModuleHandle(L"kernel32.dll"), "GetNativeSystemInfo");
    static BOOL(__stdcall * GetProductInfo)(IN DWORD dwOSMajorVersion, IN DWORD dwOSMinorVersion,
        IN DWORD dwSpMajorVersion, IN DWORD dwSpMinorVersion,
        OUT PDWORD pdwReturnedProductType) =
        (BOOL(__stdcall*)(DWORD, DWORD, DWORD, DWORD, PDWORD))GetProcAddress(GetModuleHandle(L"kernel32.dll"),
            "GetProductInfo");

    OSVERSIONINFOEXW osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXW));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

    if (RtlGetVersion != nullptr) {
        NTSTATUS ntRtlGetVersionStatus = RtlGetVersion(&osvi);
        if (ntRtlGetVersionStatus != 0x00000000) {
            return "<windows>";
        }
    }
    else {
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
        BOOL bOsVersionInfoEx = GetVersionExW((OSVERSIONINFOW*)&osvi);
        if (bOsVersionInfoEx == 0) {
            return "<windows>";
        }
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    }

    SYSTEM_INFO si;
    ZeroMemory(&si, sizeof(SYSTEM_INFO));

    if (GetNativeSystemInfo != nullptr) {
        GetNativeSystemInfo(&si);
    }
    else {
        GetSystemInfo(&si);
    }

    if ((osvi.dwPlatformId != VER_PLATFORM_WIN32_NT) || (osvi.dwMajorVersion <= 4)) {
        return "Windows <unknown version>";
    }

    std::stringstream os;
    os << "Microsoft ";
    if (osvi.dwMajorVersion >= 6) {
        if (osvi.dwMajorVersion == 10) {
            if (osvi.dwMinorVersion == 0) {
                if (osvi.wProductType == VER_NT_WORKSTATION) {
                    if (osvi.dwBuildNumber >= 22000) {
                        os << "Windows 11 ";
                    }
                    else {
                        os << "Windows 10 ";
                    }
                }
                else {
                    if (osvi.dwBuildNumber >= 20348) {
                        os << "Windows Server 2022 ";
                    }
                    else if (osvi.dwBuildNumber >= 17763) {
                        os << "Windows Server 2019 ";
                    }
                    else {
                        os << "Windows Server 2016 ";
                    }
                }
            }
        }
        else if (osvi.dwMajorVersion == 6) {
            if (osvi.dwMinorVersion == 3) {
                if (osvi.wProductType == VER_NT_WORKSTATION) {
                    os << "Windows 8.1 ";
                }
                else {
                    os << "Windows Server 2012 R2 ";
                }
            }
            else if (osvi.dwMinorVersion == 2) {
                if (osvi.wProductType == VER_NT_WORKSTATION) {
                    os << "Windows 8 ";
                }
                else {
                    os << "Windows Server 2012 ";
                }
            }
            else if (osvi.dwMinorVersion == 1) {
                if (osvi.wProductType == VER_NT_WORKSTATION) {
                    os << "Windows 7 ";
                }
                else {
                    os << "Windows Server 2008 R2 ";
                }
            }
            else if (osvi.dwMinorVersion == 0) {
                if (osvi.wProductType == VER_NT_WORKSTATION) {
                    os << "Windows Vista ";
                }
                else {
                    os << "Windows Server 2008 ";
                }
            }
        }

        DWORD dwType;
        if ((GetProductInfo != nullptr) && GetProductInfo(osvi.dwMajorVersion, osvi.dwMinorVersion, 0, 0, &dwType)) {
            switch (dwType) {
            case PRODUCT_ULTIMATE:
                os << "Ultimate Edition";
                break;
            case PRODUCT_PROFESSIONAL:
                os << "Professional";
                break;
            case PRODUCT_HOME_PREMIUM:
                os << "Home Premium Edition";
                break;
            case PRODUCT_HOME_BASIC:
                os << "Home Basic Edition";
                break;
            case PRODUCT_ENTERPRISE:
                os << "Enterprise Edition";
                break;
            case PRODUCT_BUSINESS:
                os << "Business Edition";
                break;
            case PRODUCT_STARTER:
                os << "Starter Edition";
                break;
            case PRODUCT_CLUSTER_SERVER:
                os << "Cluster Server Edition";
                break;
            case PRODUCT_DATACENTER_SERVER:
                os << "Datacenter Edition";
                break;
            case PRODUCT_DATACENTER_SERVER_CORE:
                os << "Datacenter Edition (core installation)";
                break;
            case PRODUCT_ENTERPRISE_SERVER:
                os << "Enterprise Edition";
                break;
            case PRODUCT_ENTERPRISE_SERVER_CORE:
                os << "Enterprise Edition (core installation)";
                break;
            case PRODUCT_ENTERPRISE_SERVER_IA64:
                os << "Enterprise Edition for Itanium-based Systems";
                break;
            case PRODUCT_SMALLBUSINESS_SERVER:
                os << "Small Business Server";
                break;
            case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
                os << "Small Business Server Premium Edition";
                break;
            case PRODUCT_STANDARD_SERVER:
                os << "Standard Edition";
                break;
            case PRODUCT_STANDARD_SERVER_CORE:
                os << "Standard Edition (core installation)";
                break;
            case PRODUCT_WEB_SERVER:
                os << "Web Server Edition";
                break;
            default:
                break;
            }
        }
    }
    else if ((osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion == 2)) {
        if (GetSystemMetrics(SM_SERVERR2)) {
            os << "Windows Server 2003 R2, ";
        }
        else if (osvi.wSuiteMask & VER_SUITE_STORAGE_SERVER) {
            os << "Windows Storage Server 2003";
        }
        else if (osvi.wSuiteMask & VER_SUITE_WH_SERVER) {
            os << "Windows Home Server";
        }
        else if ((osvi.wProductType == VER_NT_WORKSTATION) &&
            (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)) {
            os << "Windows XP Professional x64 Edition";
        }
        else {
            os << "Windows Server 2003, ";
        }
        if (osvi.wProductType != VER_NT_WORKSTATION) {
            if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) {
                if (osvi.wSuiteMask & VER_SUITE_DATACENTER) {
                    os << "Datacenter Edition for Itanium-based Systems";
                }
                else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE) {
                    os << "Enterprise Edition for Itanium-based Systems";
                }
            }
            else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
                if (osvi.wSuiteMask & VER_SUITE_DATACENTER) {
                    os << "Datacenter x64 Edition";
                }
                else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE) {
                    os << "Enterprise x64 Edition";
                }
                else {
                    os << "Standard x64 Edition";
                }
            }
            else {
                if (osvi.wSuiteMask & VER_SUITE_COMPUTE_SERVER) {
                    os << "Compute Cluster Edition";
                }
                else if (osvi.wSuiteMask & VER_SUITE_DATACENTER) {
                    os << "Datacenter Edition";
                }
                else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE) {
                    os << "Enterprise Edition";
                }
                else if (osvi.wSuiteMask & VER_SUITE_BLADE) {
                    os << "Web Edition";
                }
                else {
                    os << "Standard Edition";
                }
            }
        }
    }
    else if ((osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion == 1)) {
        os << "Windows XP ";
        if (osvi.wSuiteMask & VER_SUITE_PERSONAL) {
            os << "Home Edition";
        }
        else {
            os << "Professional";
        }
    }
    else if ((osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion == 0)) {
        os << "Windows 2000 ";
        if (osvi.wProductType == VER_NT_WORKSTATION) {
            os << "Professional";
        }
        else {
            if (osvi.wSuiteMask & VER_SUITE_DATACENTER) {
                os << "Datacenter Server";
            }
            else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE) {
                os << "Advanced Server";
            }
            else {
                os << "Server";
            }
        }
    }
    os << " (build " << osvi.dwBuildNumber << ")";
    return os.str();
}

string WMICI::GetName(string win32, string name) {
    vector<const wchar_t*> vendor{};
    queryWMI(win32, name, vendor);
    if (vendor.empty())
        return "<unknown>";

    wstring tmp(vendor[0]);
    return { tmp.begin(), tmp.end() };
}


int WMICI::GetNumber(string win32, string name) {
    std::vector<int64_t> vendor{};
    queryWMI(win32, name, vendor);
    if (vendor.empty()) {
        return 0;
    }
    return vendor[0];
}

void WMICI::createtext(string path)
{
	ofstream out;
	out.open(path);
	if (out.is_open())
	{
        out << "*** OS ***" << endl;
        out << "   System: " << getFullName() << endl;
        out << "   User: " << GetName("Win32_ComputerSystem", "UserName") << endl;
        out << "   Architect: " << getIs64bit() << endl;

        out << endl;

		out << "*** CPU ***" << endl;
        out << "   Manafacture: " << GetName("Win32_Processor", "Manufacturer") << endl;
        out << "   Name: " << GetName("Win32_Processor", "Name") << endl;
        out << "   PhysicalCores: " << GetNumber("Win32_Processor", "NumberOfCores") << endl;
        out << "   LogicalCores: " << GetNumber("Win32_Processor", "NumberOfLogicalProcessors") << endl;
        out << "   CacheSize_Bytes: " << GetNumber("Win32_Processor", "L3CacheSize") << endl;

        out << endl;

        out << "*** GPU ***" << endl;
        out << "   Manafacture: " << GetName("WIN32_VideoController", "AdapterCompatibility") << endl;
        out << "   Name: " << GetName("WIN32_VideoController", "Name") << endl;
        out << "   DriverVersion: " << GetName("WIN32_VideoController", "DriverVersion") << endl;
        out << "   AdapterRam: " << GetNumber("WIN32_VideoController", "AdapterRam") << endl;

        out << endl;

        out << "*** RAM ***" << endl;
        out << "   Manafacture: " << GetName("WIN32_PhysicalMemory", "Manufacturer") << endl;
        out << "   Name: " << GetName("WIN32_PhysicalMemory", "Name") << endl;
        out << "   PartNumber: " << GetName("WIN32_PhysicalMemory", "PartNumber") << endl;
        out << "   SerialNumber: " << GetName("WIN32_PhysicalMemory", "SerialNumber") << endl;

        out << endl;

        out << "*** DISK ***" << endl;
        out << "   Manafacture: " << GetName("Win32_DiskDrive", "Manufacturer") << endl;
        out << "   Model: " << GetName("Win32_DiskDrive", "Model") << endl;
        out << "   Size_Bytes: " << GetNumber("Win32_DiskDrive", "PartNumber") << endl;
        out << "   SerialNumber: " << GetName("Win32_DiskDrive", "Size") << endl;

        out << endl;

        out << "*** MAIN BOARD ***" << endl;
        out << "   Manafacture: " << GetName("Win32_BaseBoard", "Manufacturer") << endl;
        out << "   Product: " << GetName("Win32_BaseBoard", "Product") << endl;
        out << "   Version: " << GetName("Win32_BaseBoard", "Version") << endl;
        out << "   SerialNumber: " << GetName("Win32_BaseBoard", "SerialNumber") << endl;

        out << endl;

        out << "*** BIOS ***" << endl;
        out << "   Manafacture: " << GetName("Win32_BIOS", "Manufacturer") << endl;
        out << "   Version: " << GetName("Win32_BIOS", "Version") << endl;
        out << "   Name: " << GetName("Win32_BIOS", "Name") << endl;
        out << "   SerialNumber: " << GetName("Win32_BIOS", "SerialNumber") << endl;
	}
	out.close();
}