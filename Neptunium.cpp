#include "Neptunium.h"
#include <iostream>
#include <rttidata.h>

Neptunium::Neptunium(const char* moduleName)
{
    HMODULE handle = GetModuleHandleA(moduleName);
    if (!handle) return;

    auto* dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(handle);
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return;

    auto* ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(
        reinterpret_cast<uint8_t*>(handle) + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return;

    m_moduleBase = reinterpret_cast<uintptr_t>(handle);
    m_moduleSize = ntHeaders->OptionalHeader.SizeOfImage;

}


bool Neptunium::ValidRegion(const MEMORY_BASIC_INFORMATION& mbi, unsigned int page) const
{
    if (mbi.State != MEM_COMMIT) return false;
    if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return false;

    if ((mbi.Protect & page) == false) return false;

    return true;
}


void Neptunium::ClipRegion(uintptr_t regionStart, size_t regionSize,uintptr_t searchStart, uintptr_t searchEnd,uintptr_t& outStart, size_t& outSize) const
{
    outStart = (regionStart < searchStart) ? searchStart : regionStart;

    uintptr_t end = regionStart + regionSize;
    if (end > searchEnd) end = searchEnd;

    if (outStart >= end)
    {
        outSize = 0;
    }
    else
    {
        outSize = static_cast<size_t>(end - outStart);
    }

}


void Neptunium::ScanRegion(const char* regionStart, size_t regionSize,const void* pattern, size_t valueSize,bool aligned, std::vector<uintptr_t>& results) const
{
    if (regionSize < valueSize) return;

    const char* bytes = static_cast<const char*>(pattern);
    const char* start = regionStart;
    const char* end = regionStart + regionSize - valueSize + 0x1;
    size_t step = aligned ? valueSize : 0x1;

    for (const char* pos = start; pos < end; pos += step)
    {
        if (std::memcmp(pos, bytes, valueSize) == 0) results.push_back(reinterpret_cast<uintptr_t>(pos));
    }

}


std::vector<uintptr_t> Neptunium::Memory(uintptr_t start, uintptr_t end, const void* pattern, size_t valueSize, unsigned int page, bool aligned) const
{
    std::vector<uintptr_t> results;
    if (valueSize == 0 || start >= end) return results;

    MEMORY_BASIC_INFORMATION mbi = {};
    uintptr_t current = start;

    while (current < end && VirtualQuery(reinterpret_cast<LPCVOID>(current), &mbi, sizeof(mbi)))
    {
        size_t regionSize = mbi.RegionSize;
        uintptr_t regionStart = reinterpret_cast<uintptr_t>(mbi.BaseAddress);

        if (!ValidRegion(mbi, page))
        {
            current = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
            continue;
        }

        size_t clippedSize;
        uintptr_t clippedStart;
        ClipRegion(regionStart, regionSize, start, end, clippedStart, clippedSize);

        if (clippedSize > 0)
        {
            ScanRegion(reinterpret_cast<const char*>(clippedStart), clippedSize,pattern, valueSize, aligned, results);
        }

        current = regionStart + regionSize;
    }

    return results;
}


std::vector<uintptr_t> Neptunium::Base(const void* pattern, size_t valueSize, unsigned int page, bool aligned) const
{
    if (!m_moduleBase || valueSize == 0) return {};
    uintptr_t moduleEnd = m_moduleBase + m_moduleSize;
    return Memory(m_moduleBase, moduleEnd, pattern, valueSize, page, aligned);
}
std::vector<uintptr_t> Neptunium::Long(long long value, unsigned int page, bool aligned) const
{
    return Base(&value, sizeof(value), page, aligned);
}
std::vector<uintptr_t> Neptunium::Int(int value, unsigned int page, bool aligned) const
{
    return Base(&value, sizeof(value), page, aligned);
}
std::vector<uintptr_t> Neptunium::Float(float value, unsigned int page, bool aligned) const
{
    return Base(&value, sizeof(value), page, aligned);
}
std::vector<uintptr_t> Neptunium::String(const std::string& value, unsigned int page, bool aligned) const
{
    if (value.empty()) return {};
    return Base(value.data(), value.size(), page, aligned);
}


std::vector<uintptr_t> Neptunium::BaseEx(const void* pattern, size_t valueSize, unsigned int page, bool aligned) const
{
    if (valueSize == 0) return {};
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    uintptr_t start = reinterpret_cast<uintptr_t>(si.lpMinimumApplicationAddress);
    uintptr_t end = reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress);
    return Memory(start, end, pattern, valueSize, page, aligned);
}
std::vector<uintptr_t> Neptunium::LongEx(long long value, unsigned int page, bool aligned) const
{
    return BaseEx(&value, sizeof(value), page, aligned);
}
std::vector<uintptr_t> Neptunium::IntEx(int value, unsigned int page, bool aligned) const
{
    return BaseEx(&value, sizeof(value), page, aligned);
}
std::vector<uintptr_t> Neptunium::FloatEx(float value, unsigned int page, bool aligned) const
{
    return BaseEx(&value, sizeof(value), page, aligned);
}
std::vector<uintptr_t> Neptunium::StringEx(const std::string& value, unsigned int page, bool aligned) const
{
    if (value.empty()) return {};
    return BaseEx(value.data(), value.size(), page, aligned);
}


std::vector<uintptr_t> Neptunium::GetObjectInst(const char* object, const char* moduleName)
{
    using namespace std;

    Neptunium target(moduleName);
    if (!target.m_moduleBase)
    {
        cout << "Invalid Module" << endl;
        return {};
    }

    string objectName = ".?AV" + string(object) + "@@";
    vector<uintptr_t> name_List = target.String(objectName.c_str(), PAGE_READWRITE | PAGE_WRITECOPY);
    if (name_List.empty())
    {
        cout << "Name Not Found" << endl;
        return {};
    }

    TypeDescriptor* pTypeDescriptor = reinterpret_cast<TypeDescriptor*>(name_List[0] - offsetof(TypeDescriptor, name));
    cout << "pTypeDescriptor" << ": 0x" << hex << uppercase << pTypeDescriptor << nouppercase << dec << endl;
    if (pTypeDescriptor == nullptr) return {};

    unsigned int typeDescriptor_ = static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pTypeDescriptor) - target.m_moduleBase);

    vector<uintptr_t> typeDescriptor_List = target.Int(typeDescriptor_, PAGE_EXECUTE_READWRITE | PAGE_READONLY);
    if (typeDescriptor_List.empty())
    {
        cout << "TypeDescriptor Not Found" << endl;
        return {};
    }

    uintptr_t col{};
    for (const auto& address : typeDescriptor_List)
    {
        cout << "TypeDescriptor" << ": 0x" << hex << uppercase << address << nouppercase << dec << endl;

        _s_RTTICompleteObjectLocator* pCOL = reinterpret_cast<_s_RTTICompleteObjectLocator*>(address - offsetof(_s_RTTICompleteObjectLocator, pTypeDescriptor));
        if (pCOL->signature != _RTTI_RELATIVE_TYPEINFO) continue;
        if (pCOL->offset != 0) continue;
        if (pCOL->cdOffset != 0) continue;

        _s_RTTIBaseClassDescriptor* pBCD = reinterpret_cast<_s_RTTIBaseClassDescriptor*>(address - offsetof(_s_RTTIBaseClassDescriptor, pTypeDescriptor));
        if (pBCD->where.mdisp == 0 && pBCD->where.pdisp == -1 && pBCD->where.vdisp == 0) continue;

        col = address - offsetof(_s_RTTICompleteObjectLocator, pTypeDescriptor);
        cout << "pCompleteObjectLocator" << ": 0x" << hex << uppercase << col << nouppercase << dec << endl;
        break;
    }

    if (col == NULL) return{};

    uintptr_t vTable = target.Long(col, PAGE_EXECUTE_READWRITE)[0] + sizeof(void*);
    cout << "vfTable" << ": 0x" << hex << uppercase << vTable << nouppercase << dec << endl;

    return target.LongEx(vTable, PAGE_READWRITE | PAGE_WRITECOPY);


}


const char* Neptunium::GetObjectName(void* object, uintptr_t base)
{
    uintptr_t* vftable = *reinterpret_cast<uintptr_t**>(object);
    uintptr_t* meta = *reinterpret_cast<uintptr_t**>(reinterpret_cast<uintptr_t>(vftable) - sizeof(void*));
    _s_RTTICompleteObjectLocator* col = reinterpret_cast<_s_RTTICompleteObjectLocator*>(meta);
    TypeDescriptor* type = reinterpret_cast<TypeDescriptor*>(col->pTypeDescriptor + base);
    return type->name;

}

void* Neptunium::GetObjectVTable(const char* object) const
{
    using namespace std;

    if (!this->m_moduleBase) return {};

    string prefix = ".?AV";

    string objectName = prefix + string(object) + "@@";
    if (string(object).find(".?AU") != string::npos)
    {
        objectName = string(object) + "@@";
    }

    vector<uintptr_t> name_List = this->String(objectName.c_str(), PAGE_READWRITE | PAGE_WRITECOPY);
    if (name_List.empty()) return {};

    TypeDescriptor* pTypeDescriptor = reinterpret_cast<TypeDescriptor*>(name_List[0] - offsetof(TypeDescriptor, name));
    if (pTypeDescriptor == nullptr) return {};

    unsigned int typeDescriptor_ = static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pTypeDescriptor) - this->m_moduleBase);

    vector<uintptr_t> typeDescriptor_List = this->Int(typeDescriptor_, PAGE_EXECUTE_READWRITE | PAGE_READONLY);
    if (typeDescriptor_List.empty()) return {};

    uintptr_t col{};
    for (const auto& address : typeDescriptor_List)
    {
        _s_RTTICompleteObjectLocator* pCOL = reinterpret_cast<_s_RTTICompleteObjectLocator*>(address - offsetof(_s_RTTICompleteObjectLocator, pTypeDescriptor));
        if (pCOL->signature != _RTTI_RELATIVE_TYPEINFO) continue;
        if (pCOL->offset != 0) continue;
        if (pCOL->cdOffset != 0) continue;

        _s_RTTIBaseClassDescriptor* pBCD = reinterpret_cast<_s_RTTIBaseClassDescriptor*>(address - offsetof(_s_RTTIBaseClassDescriptor, pTypeDescriptor));
        if (pBCD->where.mdisp == 0 && pBCD->where.pdisp == -1 && pBCD->where.vdisp == 0) continue;

        col = address - offsetof(_s_RTTICompleteObjectLocator, pTypeDescriptor);
        break;
    }

    if (col == NULL) return{};

    return reinterpret_cast<void*>(this->Long(col, PAGE_EXECUTE_READWRITE | PAGE_READONLY)[0] + sizeof(void*));

}


