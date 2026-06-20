// 사용법: RoundTripTest <input.dat>
// Load → Save(out1) → Load(out1) → Save(out2). out1과 out2가 바이트 동일해야 통과(멱등성).
#include "../../RunBetterRun/EditorModel.h"
#include "../../RunBetterRun/EditorSerializer.h"
#include "../../RunBetterRun/DataManager.h"
#include <windows.h>
#include <cstdio>

// config.h의 extern 전역변수 정의 (GUI 없는 콘솔 테스트용)
HWND g_hWnd = nullptr;
HINSTANCE g_hInstance = nullptr;
POINT g_ptMouse = {0, 0};

static bool SameBytes(const wchar_t* a, const wchar_t* b) {
    HANDLE ha = CreateFileW(a, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    HANDLE hb = CreateFileW(b, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (ha == INVALID_HANDLE_VALUE || hb == INVALID_HANDLE_VALUE) return false;
    DWORD sa = GetFileSize(ha, 0), sb = GetFileSize(hb, 0);
    bool ok = (sa == sb);
    if (ok && sa > 0) {
        BYTE* ba = new BYTE[sa]; BYTE* bb = new BYTE[sb]; DWORD r;
        ReadFile(ha, ba, sa, &r, 0); ReadFile(hb, bb, sb, &r, 0);
        ok = (memcmp(ba, bb, sa) == 0);
        delete[] ba; delete[] bb;
    }
    CloseHandle(ha); CloseHandle(hb);
    return ok;
}

int wmain(int argc, wchar_t** argv) {
    if (argc < 2) { wprintf(L"usage: RoundTripTest <input.dat>\n"); return 2; }
    DataManager::GetInstance()->Init();
    EditorModel m1;
    if (!EditorSerializer::Load(m1, argv[1])) { wprintf(L"FAIL: load %s\n", argv[1]); return 1; }
    if (!EditorSerializer::Save(m1, L"out1.dat")) { wprintf(L"FAIL: save out1\n"); return 1; }
    EditorModel m2;
    if (!EditorSerializer::Load(m2, L"out1.dat")) { wprintf(L"FAIL: reload out1\n"); return 1; }
    if (!EditorSerializer::Save(m2, L"out2.dat")) { wprintf(L"FAIL: save out2\n"); return 1; }
    if (!SameBytes(L"out1.dat", L"out2.dat")) { wprintf(L"FAIL: out1 != out2 (not idempotent)\n"); return 1; }
    wprintf(L"PASS: round-trip idempotent for %s\n", argv[1]);
    return 0;
}
