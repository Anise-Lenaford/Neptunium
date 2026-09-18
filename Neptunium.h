#pragma once
#include <vector>
#include <string>
#include <Windows.h>

#define PAGE_ALL (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)

class Neptunium
{
public:
    explicit Neptunium(const char* moduleName = nullptr);

    Neptunium(const Neptunium&) = delete;
    Neptunium& operator=(const Neptunium&) = delete;

    std::vector<uintptr_t> Long(long long value, unsigned int page, bool aligned = true) const;
    std::vector<uintptr_t> Int(int value, unsigned int page, bool aligned = true) const;
    std::vector<uintptr_t> Float(float value, unsigned int page, bool aligned = true) const;
    std::vector<uintptr_t> String(const std::string& value, unsigned int page, bool aligned = false) const;

    std::vector<uintptr_t> LongEx(long long value, unsigned int page, bool aligned = true) const;
    std::vector<uintptr_t> IntEx(int value, unsigned int page, bool aligned = true) const;
    std::vector<uintptr_t> FloatEx(float value, unsigned int page, bool aligned = true) const;
    std::vector<uintptr_t> StringEx(const std::string& value, unsigned int page, bool aligned = false) const;

    static std::vector<uintptr_t> GetObjectInst(const char* object, const char* moduleName = nullptr);
    static const char* GetObjectName(void* object , uintptr_t base);

    void* GetObjectVTable(const char* object) const;

private:
    void ClipRegion(uintptr_t regionStart, size_t regionSize,uintptr_t searchStart, uintptr_t searchEnd,uintptr_t& outStart, size_t& outSize) const;
    void ScanRegion(const char* regionStart, size_t regionSize,const void* pattern, size_t valueSize,bool aligned, std::vector<uintptr_t>& results) const;
    bool ValidRegion(const MEMORY_BASIC_INFORMATION& mbi, unsigned int page) const;

    std::vector<uintptr_t> Memory(uintptr_t start, uintptr_t end,const void* pattern, size_t valueSize,unsigned int page, bool aligned) const;
    std::vector<uintptr_t> Base(const void* pattern, size_t valueSize, unsigned int page, bool aligned) const;
    std::vector<uintptr_t> BaseEx(const void* pattern, size_t valueSize, unsigned int page, bool aligned) const;

    size_t m_moduleSize {};
    uintptr_t m_moduleBase {};


};
