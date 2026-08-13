#include "DataPackage.h"

namespace CUI {

void DataPackage::SetText(std::string text) {
    m_text = std::move(text);
    m_hasText = true;
}

void DataPackage::SetFiles(std::vector<std::string> paths) {
    m_files = std::move(paths);
}

void DataPackage::SetFormat(const std::string& mime, std::string payload) {
    m_formats[mime] = std::move(payload);
}

bool DataPackage::HasFormat(const std::string& mime) const {
    return m_formats.find(mime) != m_formats.end();
}

std::string DataPackage::GetFormat(const std::string& mime) const {
    auto it = m_formats.find(mime);
    return it == m_formats.end() ? std::string() : it->second;
}

std::string DataPackage::PreviewLabel() const {
    if (m_hasText && !m_text.empty()) {
        const auto nl = m_text.find('\n');
        std::string line = (nl == std::string::npos) ? m_text : m_text.substr(0, nl);
        if (line.size() > 28) {
            line = line.substr(0, 27) + "…";
        }
        return line;
    }
    if (!m_files.empty()) {
        if (m_files.size() == 1) {
            const auto slash = m_files[0].find_last_of("\\/");
            return slash == std::string::npos ? m_files[0] : m_files[0].substr(slash + 1);
        }
        return std::to_string(m_files.size()) + " 个文件";
    }
    return "拖放";
}

} // namespace CUI
