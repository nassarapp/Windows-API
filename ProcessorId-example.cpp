#include <windows.h>
#include <intrin.h>
#include <stdio.h>
#include <string.h>

static const char* ArchitectureName(WORD architecture)
{
    switch (architecture)
    {
    case PROCESSOR_ARCHITECTURE_AMD64: return "x64";
    case PROCESSOR_ARCHITECTURE_INTEL: return "x86";
    case PROCESSOR_ARCHITECTURE_ARM64: return "ARM64";
    case PROCESSOR_ARCHITECTURE_ARM:   return "ARM";
    case PROCESSOR_ARCHITECTURE_IA64:  return "IA64";
    default:                           return "Unknown";
    }
}

// CPUID leaf 0 returns the vendor string in EBX, EDX, ECX (in that order).
static void GetCpuVendor(char vendor[13])
{
    int regs[4] = { 0 };

    __cpuid(regs, 0);
    memcpy(vendor + 0, &regs[1], 4);  // EBX
    memcpy(vendor + 4, &regs[3], 4);  // EDX
    memcpy(vendor + 8, &regs[2], 4);  // ECX
    vendor[12] = '\0';
}

// CPUID leaf 1 EAX is the processor signature (family / model / stepping).
static void DecodeSignature(int eax, int* family, int* model, int* stepping)
{
    int baseFamily = (eax >> 8) & 0xF;
    int baseModel = (eax >> 4) & 0xF;

    *stepping = eax & 0xF;
    *family = baseFamily;
    *model = baseModel;

    if (baseFamily == 0xF)
    {
        *family += (eax >> 20) & 0xFF;
    }
    if (baseFamily == 0x6 || baseFamily == 0xF)
    {
        *model += ((eax >> 16) & 0xF) << 4;
    }
}

// CPUID leaves 0x80000002..0x80000004 hold the brand string, 16 bytes each.
static void GetCpuBrand(char brand[49])
{
    int regs[4] = { 0 };

    __cpuid(regs, 0x80000000);
    if ((unsigned)regs[0] < 0x80000004)
    {
        brand[0] = '\0';
        return;
    }

    __cpuid(regs, 0x80000002);
    memcpy(brand + 0, regs, 16);
    __cpuid(regs, 0x80000003);
    memcpy(brand + 16, regs, 16);
    __cpuid(regs, 0x80000004);
    memcpy(brand + 32, regs, 16);
    brand[48] = '\0';
}

int main()
{
    char vendor[13] = { 0 };
    char brand[49] = { 0 };
    char processorId[17] = { 0 };
    int regs[4] = { 0 };
    int family = 0;
    int model = 0;
    int stepping = 0;
    SYSTEM_INFO sysInfo = { 0 };

    GetCpuVendor(vendor);
    GetCpuBrand(brand);

    // Same ProcessorId that WMI Win32_Processor reports: EDX then EAX from leaf 1.
    __cpuid(regs, 1);
    sprintf_s(processorId, sizeof(processorId), "%08X%08X", regs[3], regs[0]);
    DecodeSignature(regs[0], &family, &model, &stepping);

    GetSystemInfo(&sysInfo);

    printf("Vendor:            %s\n", vendor);
    if (brand[0] != '\0')
    {
        printf("Brand:             %s\n", brand);
    }
    printf("Processor ID:      %s\n", processorId);
    printf("Signature:         Family %d, Model %d, Stepping %d (EAX=%08X)\n",
           family, model, stepping, regs[0]);
    printf("Feature flags:     EDX=%08X\n", regs[3]);
    printf("Architecture:      %s\n", ArchitectureName(sysInfo.wProcessorArchitecture));
    printf("Logical processors:%lu\n", sysInfo.dwNumberOfProcessors);
    printf("Processor level:   %u\n", sysInfo.wProcessorLevel);
    printf("Processor revision:%04X\n", sysInfo.wProcessorRevision);

    return 0;
}
