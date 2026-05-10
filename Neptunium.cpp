#include "Neptunium.h"
#include <Windows.h>
#include <iostream>

Neptunium::Neptunium(const char* moduleName)
{
    HMODULE handle = GetModuleHandleA(moduleName);
    if (!handle) return;

    auto* dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(handle);
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return;

    auto* ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(
        reinterpret_cast<uint8_t*>(handle) + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return;

    mModuleBase = reinterpret_cast<uintptr_t>(handle);
    mModuleSize = ntHeaders->OptionalHeader.SizeOfImage;

}


std::vector<uintptr_t> Neptunium::Base(const void* pattern, size_t valueSize, unsigned int page) const
{
    std::vector<uintptr_t> results;
    if (!mModuleBase || valueSize == 0) return results;

    const uintptr_t moduleEnd = mModuleBase + mModuleSize;
    const char* bytes = static_cast<const char*>(pattern);

    uintptr_t base = mModuleBase;
    MEMORY_BASIC_INFORMATION mbi = {};

    while (base < moduleEnd && VirtualQuery(reinterpret_cast<LPCVOID>(base), &mbi, sizeof(mbi)))
    {
        if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) ||
            !(mbi.Protect & (page)))
        {
            base = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
            continue;
        }

        uintptr_t regionStart = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        size_t regionSize = mbi.RegionSize;

        if (regionStart >= moduleEnd) break;
        if (regionStart + regionSize > moduleEnd) regionSize = moduleEnd - regionStart;

        if (regionSize < valueSize)
        {
            base = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
            continue;
        }

        const char* start = reinterpret_cast<const char*>(regionStart);
        const char* end = start + regionSize - valueSize + 0x1;

        for (const char* index = start; index < end; ++index)
        {
            if (std::memcmp(index, bytes, valueSize) == 0) results.push_back(reinterpret_cast<uintptr_t>(index));
        }

        base = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
    }

    return results;

}
std::vector<uintptr_t> Neptunium::Long(long long value, unsigned int page) const
{
    return Base(&value, sizeof(value), page);
}
std::vector<uintptr_t> Neptunium::Int(int value, unsigned int page) const
{
    return Base(&value, sizeof(value), page);
}
std::vector<uintptr_t> Neptunium::String(const std::string& value, unsigned int page) const
{
    if (value.empty()) return {};
    return Base(value.data(), value.size(), page);
}

std::vector<uintptr_t> Neptunium::BaseEx(const void* pattern, size_t valueSize, unsigned int page) const
{
    std::vector<uintptr_t> results;
    if (!mModuleBase || valueSize == 0) return results;

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    const char* bytes = static_cast<const char*>(pattern);

    uintptr_t max = reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress);
    uintptr_t base = reinterpret_cast<uintptr_t>(si.lpMinimumApplicationAddress);

    MEMORY_BASIC_INFORMATION mbi = {};
    while (base < max && VirtualQuery(reinterpret_cast<LPCVOID>(base), &mbi, sizeof(mbi)))
    {
        if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) ||
            !(mbi.Protect & (page)))
        {
            base = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
            continue;
        }

        uintptr_t regionStart = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        size_t regionSize = mbi.RegionSize;

        if (regionStart >= max) break;
        if (regionStart + regionSize > max) regionSize = max - regionStart;

        if (regionSize < valueSize)
        {
            base = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
            continue;
        }

        const char* start = reinterpret_cast<const char*>(regionStart);
        const char* end = start + regionSize - valueSize + 1;

        for (const char* index = start; index < end; ++index)
        {
            if (std::memcmp(index, bytes, valueSize) == 0)
                results.push_back(reinterpret_cast<uintptr_t>(index));
        }

        base = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
    }

    return results;

}
std::vector<uintptr_t> Neptunium::LongEx(long long value, unsigned int page) const
{
    return BaseEx(&value, sizeof(value), page);
}
std::vector<uintptr_t> Neptunium::IntEx(int value, unsigned int page) const
{
    return BaseEx(&value, sizeof(value), page);
}
std::vector<uintptr_t> Neptunium::StringEx(const std::string& value, unsigned int page) const
{
    if (value.empty()) return {};
    return BaseEx(value.data(), value.size(), page);
}


std::vector<uintptr_t> Neptunium::RTTI(const char* object, const char* moduleName)
{
    using namespace std;

    Neptunium target(moduleName);
    if (!target.mModuleBase)
    {
        cout << "Invalid Module" << endl;
        return {};
    }

    vector<uintptr_t> padName = target.String(object, PAGE_READWRITE | PAGE_WRITECOPY);

    uintptr_t type = {};
    for (const auto& address : padName)
    {
        cout << "PadName" << ": 0x" << hex << uppercase << address << nouppercase << dec << endl;

        constexpr uintptr_t offset = sizeof(void*) + sizeof(uintptr_t) + 0x4;  //0x4 Padding string
        uintptr_t vTable = address - offset;
        uintptr_t spare = vTable + sizeof(uintptr_t);

#ifdef _WIN64
        long long spareValue = *reinterpret_cast<long long*>(spare);
#else
        int spareValue = *reinterpret_cast<int*>(spare);
#endif

        if (spareValue == 0)
        {
            type = vTable;
        }

    }
    cout << "Type" << ": 0x" << hex << uppercase << type << nouppercase << dec << endl;
    if (!type) return {};

#ifdef _WIN64
    uintptr_t typeOffset = type - target.mModuleBase;
    vector<uintptr_t> typeDescriptor = target.Int(static_cast<int>(typeOffset), PAGE_READONLY);
#else
    vector<uintptr_t> typeDescriptor = target.Int(type, PAGE_READONLY);
#endif

    uintptr_t signature = {};
    for (const auto& address : typeDescriptor)
    {
        cout << "TypeDescriptor" << ": 0x" << hex << uppercase << address << nouppercase << dec << endl;

        uintptr_t col_signature = address - 0xC;
        int col_signatureValue = *reinterpret_cast<int*>(col_signature);

#ifdef _WIN64
        if (col_signatureValue != 1) continue;
#else
        if (col_signatureValue != 0) continue;
#endif

        uintptr_t col_offset = address - 0x8;
        int col_offsetValue = *reinterpret_cast<int*>(col_offset);
        if (col_offsetValue != 0) continue;

        uintptr_t pmd_pad_0 = address + 0x8;
        int pmdValue = *reinterpret_cast<int*>(pmd_pad_0);
        if (pmdValue == 0)
        {
            uintptr_t pmd_pad_4 = address + 0xC;
            int pmdValue = *reinterpret_cast<int*>(pmd_pad_4);
            if (pmdValue == -1)
            {
                uintptr_t pmd_pad_8 = address + 0x10;
                int pmdValue = *reinterpret_cast<int*>(pmd_pad_8);
                if (pmdValue == 0)
                {
                    continue;
                }
            }

        }

        signature = address;
        cout << "Signature" << ": 0x" << hex << uppercase << signature << nouppercase << dec << endl;

    }


    uintptr_t col = signature - 0xC;
    cout << "COL" << ": 0x" << hex << uppercase << col << nouppercase << dec << endl;

#ifdef _WIN64
    uintptr_t meta = target.Long(col, PAGE_READONLY)[0];
    uintptr_t vTable = meta + sizeof(uintptr_t);
    return target.LongEx(vTable, PAGE_READWRITE | PAGE_WRITECOPY);  //Stack Pollution
#else
    uintptr_t meta = target.Int(col, PAGE_WRITECOPY)[0];
    uintptr_t vTable = meta + sizeof(uintptr_t);
    return target.IntEx(vTable, PAGE_READWRITE | PAGE_WRITECOPY);  //Stack Pollution
#endif


}
