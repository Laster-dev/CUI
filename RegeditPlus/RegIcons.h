#pragma once
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>

namespace RegeditPlus {

// Loads the small-icon set embedded in %WinDir%\regedit.exe.
// Indices (Win10/11): 3=computer, 4=folder/key, 5=string(ab), 6=binary(010).
class RegIcons {
public:
    RegIcons() = default;
    ~RegIcons() { Release(); }

    RegIcons(const RegIcons&) = delete;
    RegIcons& operator=(const RegIcons&) = delete;

    void EnsureLoaded();
    void Release();

    HICON Computer() const { return At(3); }
    HICON Folder() const { return At(4); }
    HICON StringValue() const { return At(5); }
    HICON BinaryValue() const { return At(6); }

    HICON ForValueType(DWORD type) const {
        switch (type) {
            case REG_SZ:
            case REG_EXPAND_SZ:
            case REG_MULTI_SZ:
                return StringValue();
            default:
                return BinaryValue();
        }
    }

private:
    HICON At(size_t index) const {
        return (index < m_small.size()) ? m_small[index] : nullptr;
    }

    std::vector<HICON> m_small;
    bool m_loaded = false;
};

} // namespace RegeditPlus
