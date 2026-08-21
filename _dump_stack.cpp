#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>
#include <map>
#pragma comment(lib, "dbghelp.lib")
int wmain() {
    HANDLE h = CreateFileA("F:\\YiyangzaiCode\\_dump.dmp", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    HANDLE hm = CreateFileMappingA(h, NULL, PAGE_READONLY, 0, 0, NULL);
    LPVOID base = MapViewOfFile(hm, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(h);
    PMINIDUMP_DIRECTORY dir = NULL; ULONG cnt = 0;
    // memory map
    std::map<ULONG64, ULONG64> mem;
    if (MiniDumpReadDumpStream(base, MemoryListStream, NULL, (void**)&dir, &cnt)) {
        ULONG32 n = *(ULONG32*)dir;
        MINIDUMP_MEMORY_DESCRIPTOR *md = (MINIDUMP_MEMORY_DESCRIPTOR*)((BYTE*)dir + 4);
        for (ULONG32 i = 0; i < n; i++) mem[md[i].StartOfMemoryRange] = md[i].Memory.Rva;
        fprintf(stderr, "ranges=%u\n", (unsigned)n);
    }
    // find stack range containing 0xF243CFD678
    ULONG64 rsp = 0xF243CFD678ULL;
    for (auto &kv : mem) {
        ULONG64 start = kv.first, off = kv.second;
        ULONG64 end = 0;
        for (auto &kv2 : mem) if (kv2.first > start && (end == 0 || kv2.first < end)) end = kv2.first;
        ULONG64 size = (end ? end - start : 0x100000);
        if (rsp >= start && rsp < start + size) {
            fprintf(stderr, "stack range start=0x%llX size=0x%llX\n", (unsigned long long)start, (unsigned long long)size);
            // hexdump 256 bytes from rsp
            ULONG64 base_addr = rsp & ~0xFULL;
            for (int row = 0; row < 64; row++) {
                ULONG64 a = base_addr + row * 16;
                if (a < start || a + 16 > start + size) continue;
                fprintf(stderr, "%08llX: ", (unsigned long long)a);
                for (int c = 0; c < 16; c++) fprintf(stderr, "%02X ", *(BYTE*)((BYTE*)base + off + (a - start) + c));
                fprintf(stderr, "\n");
            }
            break;
        }
    }
    UnmapViewOfFile(base); CloseHandle(hm); return 0;
}
