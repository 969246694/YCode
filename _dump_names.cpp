#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>
#pragma comment(lib, "dbghelp.lib")
int wmain() {
    HANDLE h = CreateFileA("F:\\YiyangzaiCode\\_dump.dmp", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    HANDLE hm = CreateFileMappingA(h, NULL, PAGE_READONLY, 0, 0, NULL);
    LPVOID base = MapViewOfFile(hm, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(h);
    PMINIDUMP_DIRECTORY dir = NULL; ULONG cnt = 0;
    if (MiniDumpReadDumpStream(base, ModuleListStream, NULL, (void**)&dir, &cnt)) {
        ULONG32 n = *(ULONG32*)dir;
        MINIDUMP_MODULE *m = (MINIDUMP_MODULE*)((BYTE*)dir + 4);
        for (ULONG32 i = 0; i < n; i++) {
            char name[512] = {0};
            DWORD rva = m[i].ModuleNameRva;
            if (rva < 0x1000000) {
                strncpy(name, (char*)base + rva, 511);
            } else {
                snprintf(name, 512, "(rva=%X)", (unsigned)rva);
            }
            fprintf(stderr, "0x%llX size=0x%llX %s\n", (unsigned long long)m[i].BaseOfImage,
                    (unsigned long long)m[i].SizeOfImage, name);
        }
    }
    UnmapViewOfFile(base); CloseHandle(hm); return 0;
}
