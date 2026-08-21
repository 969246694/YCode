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
    if (MiniDumpReadDumpStream(base, ThreadListStream, NULL, (void**)&dir, &cnt)) {
        ULONG32 n = *(ULONG32*)dir;
        MINIDUMP_THREAD *thr = (MINIDUMP_THREAD*)((BYTE*)dir + 4);
        for (ULONG32 i = 0; i < n; i++) {
            if (thr[i].ThreadId == 13944) {
                BYTE *ctx = (BYTE*)base + thr[i].ThreadContext.Rva;
                fprintf(stderr, "context size=%u\n", thr[i].ThreadContext.DataSize);
                for (int r = 0; r < 0x100 && r < (int)thr[i].ThreadContext.DataSize; r += 16) {
                    fprintf(stderr, "%04X: ", r);
                    for (int c = 0; c < 16; c++) fprintf(stderr, "%02X ", ctx[r+c]);
                    fprintf(stderr, "\n");
                }
            }
        }
    }
    UnmapViewOfFile(base); CloseHandle(hm); return 0;
}
