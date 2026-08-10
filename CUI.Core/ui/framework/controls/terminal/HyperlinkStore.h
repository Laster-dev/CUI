#pragma once
#include <string>
#include <unordered_map>

namespace CUI {
namespace Term {

class HyperlinkStore {
public:
    int ActiveId() const { return m_activeId; }

    int Begin(const std::string& url);
    void End() { m_activeId = 0; }

    // Returns an empty string when the id is unknown or zero.
    std::string GetUrl(int id) const;
    bool HasUrl(int id) const;

    void Clear();

private:
    std::unordered_map<int, std::string> m_idToUrl;
    int m_nextId = 1;
    int m_activeId = 0;
};

} // namespace Term
} // namespace CUI
