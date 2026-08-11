#pragma once

#include <string>

namespace CUI {

class DockManager;

class DockLayoutSerializer {
public:
    static bool Save(const DockManager& manager, const std::wstring& path);
    static bool Load(DockManager& manager, const std::wstring& path);
};

} // namespace CUI
