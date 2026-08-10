#include "HyperlinkStore.h"

namespace CUI {
namespace Term {

int HyperlinkStore::Begin(const std::string& url) {
    if (url.empty()) {
        m_activeId = 0;
        return 0;
    }

    const int id = m_nextId++;
    m_idToUrl[id] = url;
    m_activeId = id;
    return id;
}

std::string HyperlinkStore::GetUrl(int id) const {
    if (id == 0) {
        return std::string();
    }
    auto it = m_idToUrl.find(id);
    return it != m_idToUrl.end() ? it->second : std::string();
}

bool HyperlinkStore::HasUrl(int id) const {
    return id != 0 && m_idToUrl.find(id) != m_idToUrl.end();
}

void HyperlinkStore::Clear() {
    m_idToUrl.clear();
    m_activeId = 0;
    m_nextId = 1;
}

} // namespace Term
} // namespace CUI
