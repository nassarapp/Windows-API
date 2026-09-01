// Heaven's Gate — 32-bit (WoW64) -> 64-bit user-mode transition
//
// Build as a 32-bit binary on 64-bit Windows, MSVC x86 toolset:
//   vcvarsall.bat x86
//   cl /nologo /W4 HeavensGate.cpp
//
// This technique only exists in a 32-bit process running on 64-bit Windows
// (WoW64). A native 64-bit build, or a 32-bit OS, has nothing to "gate" into.

#ifndef _MSC_VER
#error This file uses MSVC 32-bit inline assembly (_emit / __asm).
#endif
#ifdef _WIN64
#error Heaven's Gate is a WoW64 trick. Compile as 32-bit (/DWIN32, x86 cl).
#endif

#include <windows.h>
#include <winternl.h>
#include <intrin.h>
#include <stdio.h>

// ---------------------------------------------------------------------------
// What "Heaven's Gate" is
//
// On x64 Windows, a 32-bit process does not run on a 32-bit CPU. The processor
// is in long mode the whole time. WoW64 runs your 32-bit code in
// compatibility mode (CS = 0x23): addresses are 32-bit, the instruction
// decoder treats opcodes as i386, and FS points at the 32-bit TEB.
//
// The same process also has a 64-bit half: a 64-bit ntdll.dll, a 64-bit TEB
// (GS), and a 64-bit PEB. The CPU switches into that world when CS becomes
// 0x33 — the 64-bit user-mode code segment. That far jump/return through
// selector 0x33 is what researchers nicknamed Heaven's Gate (you "go up"
// from the 32-bit WoW64 world into native 64-bit user mode).
//
// Selectors (x64 Windows user mode, GDT):
//   0x23  — 32-bit CS  (WoW64 compatibility mode)
//   0x33  — 64-bit CS  (long mode, 64-bit user code)
//
// Transition is a far return (retf), which loads CS from the stack:
//
//   32-bit code                    64-bit code
//   ----------                    -----------
//   push 0x33                     ... 64-bit ops ...
//   push <addr of x64 stub>       push 0x23   (via the [rsp+4] overlay)
//   retf  ------------------>     retf  ------------------>  back to 32-bit
//         CS := 0x33                     CS := 0x23
//
// Why anyone bothers: the 64-bit ntdll syscall stubs are a different path
// from the 32-bit ones. This file does not issue syscalls. It only enters
// 64-bit mode, reads GS-based thread state, and returns — enough to prove
// the gate works and to inspect the 64-bit TEB/PEB from a 32-bit process.
// ---------------------------------------------------------------------------

// Values written from 64-bit mode. The 32-bit compiler cannot emit x64
// stores into locals reliably, so the gate stub writes a global.
struct Wow64NativeView {
    UINT64 teb64;   // GS:[0x30] = NT_TIB.Self of the 64-bit TEB
    UINT64 peb64;   // 64-bit TEB + 0x60
    UINT64 pid64;   // 64-bit TEB + 0x40 = CLIENT_ID.UniqueProcess
};

static volatile Wow64NativeView g_native;

// MSVC will otherwise feel free to shuffle the surrounding function; the
// opcode stream after retf is position-sensitive.
#pragma optimize("", off)

// Enter CS=0x33, read 64-bit TEB/PEB/PID, return to CS=0x23.
//
// The bytes after the first retf are 64-bit instructions. The 32-bit
// assembler does not understand them, so they are emitted by hand.
static void HeavensGateReadNativeState(void)
{
    __asm {
        pushad
        pushfd

        // RBX will still hold this address after the mode switch: entering
        // long mode zero-extends the 32-bit GPRs. The struct lives in the
        // low 4 GB, so a 32-bit pointer is a valid 64-bit pointer here.
        mov     ebx, offset g_native

        // ================================================================
        // 32-bit -> 64-bit  (Heaven's Gate)
        //
        // 32-bit CALL pushes a 4-byte return EIP. retf then pops EIP and
        // CS. We point EIP at the first _emit below and CS at 0x33.
        //
        //   call $+5          pushes address of the ADD
        //   add [esp], 5      ADD is 4 bytes, retf is 1 byte -> skip both
        //   retf              pops patched EIP + 0x33
        // ================================================================
        push    0x33
        call    $+5
        add     dword ptr [esp], 5
        retf

        // ================================================================
        // 64-bit user code  (CPU is in long mode, CS = 0x33)
        //
        // GS base is now the 64-bit TEB. On x64, NT_TIB.Self is at 0x30,
        // same field the 64-bit TEB-example reads with __readgsqword.
        //
        //   mov rax, qword ptr gs:[0x30]
        // ================================================================
        _emit 0x65 _emit 0x48 _emit 0x8B _emit 0x04 _emit 0x25
        _emit 0x30 _emit 0x00 _emit 0x00 _emit 0x00

        //   mov qword ptr [rbx], rax          ; g_native.teb64
        _emit 0x48 _emit 0x89 _emit 0x03

        //   mov rcx, qword ptr [rax+0x40]     ; ClientId.UniqueProcess
        _emit 0x48 _emit 0x8B _emit 0x48 _emit 0x40

        //   mov qword ptr [rbx+0x10], rcx     ; g_native.pid64
        _emit 0x48 _emit 0x89 _emit 0x4B _emit 0x10

        //   mov rax, qword ptr [rax+0x60]     ; PEB*
        _emit 0x48 _emit 0x8B _emit 0x40 _emit 0x60

        //   mov qword ptr [rbx+0x08], rax     ; g_native.peb64
        _emit 0x48 _emit 0x89 _emit 0x43 _emit 0x08

        // ================================================================
        // 64-bit -> 32-bit  (close the gate)
        //
        // A 64-bit CALL pushes an 8-byte return RIP. A 32-bit-operand retf
        // (opcode CB, no REX.W) pops 4-byte EIP then 4-byte CS. So the high
        // half of that 8-byte slot is exactly where CS goes:
        //
        //   [rsp+0]  EIP  = this trampoline + 0x0D  (land in 32-bit code)
        //   [rsp+4]  CS   = 0x23
        //
        // Length of the trampoline after the CALL:
        //   C7 44 24 04 23 00 00 00   mov dword [rsp+4], 0x23     (8)
        //   83 04 24 0D               add dword [rsp], 0x0D       (4)
        //   CB                        retf                        (1)
        //                                             total  = 0x0D
        // ================================================================
        _emit 0xE8 _emit 0x00 _emit 0x00 _emit 0x00 _emit 0x00
        _emit 0xC7 _emit 0x44 _emit 0x24 _emit 0x04
        _emit 0x23 _emit 0x00 _emit 0x00 _emit 0x00
        _emit 0x83 _emit 0x04 _emit 0x24 _emit 0x0D
        _emit 0xCB

        // 32-bit again (CS = 0x23). Stack is back where it was before
        // the 0x33 push; pushad/pushfd can come off in reverse order.
        popfd
        popad
    }
}

#pragma optimize("", on)

int main(void)
{
    BOOL wow64 = FALSE;
    WORD csBefore = 0;
    WORD csAfter = 0;

    // WoW64 is "32-bit process, 64-bit OS". Native 32-bit Windows has no
    // 0x33 64-bit CS; jumping there would #GP.
    if (!IsWow64Process(GetCurrentProcess(), &wow64) || !wow64)
    {
        printf("Not WoW64. Heaven's Gate needs a 32-bit process on 64-bit Windows.\n");
        return 1;
    }

    __asm { mov ax, cs; mov csBefore, ax }

    printf("WoW64 process:               yes\n");
    printf("CS before gate:              0x%04X  (expect 0x0023)\n", csBefore);
    printf("32-bit TEB (FS:[0x18]):      %p\n", NtCurrentTeb());
    printf("32-bit PEB:                  %p\n", NtCurrentTeb()->ProcessEnvironmentBlock);
    printf("GetCurrentProcessId():       %lu\n", GetCurrentProcessId());
    printf("\nEntering CS=0x33 ...\n\n");

    HeavensGateReadNativeState();

    __asm { mov ax, cs; mov csAfter, ax }

    printf("CS after gate:               0x%04X  (expect 0x0023 again)\n", csAfter);
    printf("64-bit TEB (GS:[0x30]):      0x%I64X\n", g_native.teb64);
    printf("64-bit PEB (TEB+0x60):       0x%I64X\n", g_native.peb64);
    printf("64-bit ClientId.Process:     0x%I64X  (PID %lu)\n",
           g_native.pid64, (unsigned long)g_native.pid64);

    // On WoW64 both TEBs usually sit in the same 32-bit address range, with
    // the 64-bit TEB just below the 32-bit TEB. The exact gap is a Windows
    // implementation detail; do not hard-code it.
    printf("\nTEB64 vs TEB32 gap:          %Id bytes\n",
           (INT_PTR)NtCurrentTeb() - (INT_PTR)(ULONG_PTR)g_native.teb64);

    return 0;
}
