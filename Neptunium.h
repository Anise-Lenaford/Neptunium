#pragma once
#include <vector>
#include <string>

#define PAGE_ALL (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)

class Neptunium
{
public:
    explicit Neptunium(const char* moduleName = nullptr);

    Neptunium(const Neptunium&) = delete;
    Neptunium& operator=(const Neptunium&) = delete;

    std::vector<uintptr_t> Long(long long value,unsigned int page) const;
    std::vector<uintptr_t> Int(int value, unsigned int page) const;
    std::vector<uintptr_t> String(const std::string& value, unsigned int page) const;

    std::vector<uintptr_t> LongEx(long long value, unsigned int page) const;
    std::vector<uintptr_t> IntEx(int value, unsigned int page) const;
    std::vector<uintptr_t> StringEx(const std::string& value, unsigned int page) const;

    static std::vector<uintptr_t> RTTI(const char* object, const char* moduleName = nullptr);

private:
    std::vector<uintptr_t> Base(const void* pattern, size_t valueSize, unsigned int page) const;
    std::vector<uintptr_t> BaseEx(const void* pattern, size_t valueSize, unsigned int page) const;

    size_t mModuleSize {};
    uintptr_t mModuleBase {};


};
