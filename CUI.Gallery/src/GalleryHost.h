#pragma once

#include <functional>
#include <string>

namespace Gallery {

class Host {
public:
    static Host& Instance();

    void SetNavigator(std::function<void(const std::string& tag)> navigator);
    void Navigate(const std::string& tag);

private:
    std::function<void(const std::string&)> m_navigator;
};

} // namespace Gallery
