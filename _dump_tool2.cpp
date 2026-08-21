// minidump_stack.cpp - exception info + stack walk from a minidump
#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>
#include <vector>
#include <map>
#include <string>
#pragma comment(lib, "dbghelp.lib")

struct Module { ULONG64 base, size; std::string name; };

int wmain()
{
    const char *path = "F:\\YiyangzaiCode\\_dump.dmp";
    HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) { fprintf(stderr, "cannot open\n"); return 1; }
    HANDLE hDump = CreateFileMappingA(h, NULL, PAGE_READONLY, 0, 0, NULL);
    LPVOID base = MapViewOfFile(hDump, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(h);
    if (!hDump || !base) { fprintf(stderr, "cannot map\n"); return 1; }

    std::vector<Module> mods;
    ULONG64 exAddr = 0;
    DWORD exThread = 0;
    ULONG64 ctxRva = 0, ctxSize = 0;

    PMINIDUMP_DIRECTORY dir = NULL;
    ULONG cnt = 0;

    if (MiniDumpReadDumpStream(base, ModuleListStream, NULL, (void**)&dir, &cnt)) {
        ULONG32 n = *(ULONG32*)dir;
        MINIDUMP_MODULE *m = (MINIDUMP_MODULE*)((BYTE*)dir + 4);
        for (ULONG32 i = 0; i < n; i++) {
            Module mod;
            mod.base = m[i].BaseOfImage;
            mod.size = m[i].SizeOfImage;
            char *namePtr = (char*)base + m[i].ModuleNameRva;
            mod.name = namePtr;
            mods.push_back(mod);
        }
    }

    if (MiniDumpReadDumpStream(base, ExceptionStream, NULL, (void**)&dir, &cnt)) {
        MINIDUMP_EXCEPTION_STREAM *es = (MINIDUMP_EXCEPTION_STREAM*)dir;
        exThread = es->ThreadId;
        exAddr = (ULONG64)es->ExceptionRecord.ExceptionAddress;
        fprintf(stderr, "EXCEPTION thread=%u code=0x%08X addr=0x%llX read-addr=0x%llX\n",
                (unsigned)exThread, (unsigned)es->ExceptionRecord.ExceptionCode,
                (unsigned long long)exAddr,
                (unsigned long long)es->ExceptionRecord.ExceptionInformation[0]);
    }

    if (MiniDumpReadDumpStream(base, ThreadListStream, NULL, (void**)&dir, &cnt)) {
        ULONG32 n = *(ULONG32*)dir;
        MINIDUMP_THREAD *thr = (MINIDUMP_THREAD*)((BYTE*)dir + 4);
        for (ULONG32 i = 0; i < n; i++) {
            if (thr[i].ThreadId == exThread) {
                ctxRva = thr[i].ThreadContext.Rva;
                ctxSize = thr[i].ThreadContext.DataSize;
                fprintf(stderr, "exception thread ctx rva=0x%llX size=%llu\n",
                        (unsigned long long)ctxRva, (unsigned long long)ctxSize);
            }
        }
    }

    std::map<ULONG64, ULONG64> memRanges;
    if (MiniDumpReadDumpStream(base, MemoryListStream, NULL, (void**)&dir, &cnt)) {
        ULONG32 n = *(ULONG32*)dir;
        MINIDUMP_MEMORY_DESCRIPTOR *md = (MINIDUMP_MEMORY_DESCRIPTOR*)((BYTE*)dir + 4);
        for (ULONG32 i = 0; i < n; i++)
            memRanges[md[i].StartOfMemoryRange] = md[i].Memory.Rva;
        fprintf(stderr, "memory ranges: %u\n", (unsigned)n);
    }

    auto readQword = [&](ULONG64 addr, ULONG64 *out) -> bool {
        for (auto &kv : memRanges) {
            ULONG64 start = kv.first;
            ULONG64 dumpOff = kv.second;
            ULONG64 end = 0;
            for (auto &kv2 : memRanges) if (kv2.first > start && (end == 0 || kv2.first < end)) end = kv2.first;
            ULONG64 size = (end ? end - start : 0x100000);
            if (addr >= start && addr + 8 <= start + size) {
                memcpy(out, (BYTE*)base + dumpOff + (addr - start), 8);
                return true;
            }
        }
        return false;
    };

    auto fmtMod = [&](ULONG64 addr, char *buf, size_t buflen) {
        for (auto &m : mods) {
            if (addr >= m.base && addr < m.base + m.size) {
                snprintf(buf, buflen, "%s+0x%llX", m.name.c_str(), (unsigned long long)(addr - m.base));
                return;
            }
        }
        snprintf(buf, buflen, "0x%llX", (unsigned long long)addr);
    };

    if (ctxRva && ctxSize >= 0xE8) {
        BYTE *ctx = (BYTE*)base + ctxRva;
        ULONG64 rip, rsp, rbp;
        memcpy(&rip, ctx + 0xE8, 8);
        memcpy(&rsp, ctx + 0x88, 8);
        memcpy(&rbp, ctx + 0x90, 8);
        fprintf(stderr, "RIP=0x%llX RSP=0x%llX RBP=0x%llX\n", (unsigned long long)rip,
                (unsigned long long)rsp, (unsigned long long)rbp);

        fprintf(stderr, "\n-- RBP chain --\n");
        ULONG64 fp = rbp;
        for (int frame = 0; frame < 40 && fp; frame++) {
            ULONG64 retAddr = 0, nextFp = 0;
            if (!readQword(fp, &retAddr)) { fprintf(stderr, "  stop read fp=0x%llX\n", (unsigned long long)fp); break; }
            if (!readQword(fp + 8, &nextFp)) break;
            if (retAddr < 0x10000) { fprintf(stderr, "  stop small ret\n"); break; }
            char buf[512];
            fmtMod(retAddr, buf, sizeof(buf));
            fprintf(stderr, "  frame %d: %s\n", frame, buf);
            if (nextFp <= fp || nextFp - fp > 0x100000) { fprintf(stderr, "  stop fp chain\n"); break; }
            fp = nextFp;
        }

        fprintf(stderr, "\n-- stack linear scan --\n");
        ULONG64 addr = rsp;
        for (int i = 0; i < 300; i++) {
            ULONG64 q = 0;
            if (!readQword(addr, &q)) break;
            addr += 8;
            char buf[512];
            fmtMod(q, buf, sizeof(buf));
            if (strstr(buf, "+0x") && strstr(buf, ".dll"))
                fprintf(stderr, "  [%d] %s\n", i, buf);
        }
    }

    UnmapViewOfFile(base);
    CloseHandle(hDump);
    return 0;
}

