#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>
#include <map>
#include <string>
#pragma comment(lib, "dbghelp.lib")
struct Mod { ULONG64 base, size; std::wstring name; };
int wmain() {
    HANDLE h = CreateFileA("F:\\YiyangzaiCode\\_dump.dmp", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    HANDLE hm = CreateFileMappingA(h, NULL, PAGE_READONLY, 0, 0, NULL);
    LPVOID base = MapViewOfFile(hm, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(h);
    std::map<ULONG64, Mod> mods;
    PMINIDUMP_DIRECTORY dir = NULL; ULONG cnt = 0;
    if (MiniDumpReadDumpStream(base, ModuleListStream, NULL, (void**)&dir, &cnt)) {
        ULONG32 n = *(ULONG32*)dir;
        MINIDUMP_MODULE *m = (MINIDUMP_MODULE*)((BYTE*)dir + 4);
        for (ULONG32 i = 0; i < n; i++) {
            Mod mod; mod.base = m[i].BaseOfImage; mod.size = m[i].SizeOfImage;
            DWORD rva = m[i].ModuleNameRva;
            unsigned short len = *(unsigned short*)((BYTE*)base + rva);
            mod.name.assign((wchar_t*)((BYTE*)base + rva + 2), len / 2);
            mods[mod.base] = mod;
        }
    }
    // print modules whose size matches known DLLs and print all with name != empty
    for (auto &kv : mods) {
        const Mod &mod = kv.second;
        char mb[1024];
        int n = WideCharToMultiByte(CP_UTF8, 0, mod.name.c_str(), (int)mod.name.size(), mb, 1023, NULL, NULL);
        mb[n<0?0:n] = 0;
        fprintf(stderr, "0x%llX size=0x%llX %s\n", (unsigned long long)mod.base, (unsigned long long)mod.size, mb);
    }
    UnmapViewOfFile(base); CloseHandle(hm); return 0;
}
