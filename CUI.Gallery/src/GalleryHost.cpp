#include "GalleryHost.h"

namespace Gallery {

Host& Host::Instance() {
    static Host instance;
    return instance;
}

void Host::SetNavigator(std::function<void(const std::string& tag)> navigator) {
    m_navigator = std::move(navigator);
}

void Host::Navigate(const std::string& tag) {
    if (m_navigator) {
        m_navigator(tag);
    }
}

} // namespace Gallery
