#include "Binding.h"

namespace CUI {

Binding::Binding(std::shared_ptr<Object> target, std::string targetProperty,
                 std::shared_ptr<Object> source, std::string sourceProperty,
                 BindingMode mode)
    : m_target(target), m_targetProperty(targetProperty),
      m_source(source), m_sourceProperty(sourceProperty), m_mode(mode)
{
    UpdateTarget();

    if (mode == BindingMode::OneWay || mode == BindingMode::TwoWay) {
        if (auto src = m_source.lock()) {
            m_sourceConnId = src->OnPropertyChanged().Connect(
                [this](const std::string& propName, const Value& val) {
                    if (!m_isUpdating && propName == m_sourceProperty) {
                        UpdateTarget();
                    }
                });
        }
    }

    if (mode == BindingMode::TwoWay) {
        if (auto tgt = m_target.lock()) {
            m_targetConnId = tgt->OnPropertyChanged().Connect(
                [this](const std::string& propName, const Value& val) {
                    if (!m_isUpdating && propName == m_targetProperty) {
                        UpdateSource();
                    }
                });
        }
    }
}

Binding::~Binding() {
    if (m_sourceConnId != 0) {
        if (auto src = m_source.lock()) {
            src->OnPropertyChanged().Disconnect(m_sourceConnId);
        }
    }
    if (m_targetConnId != 0) {
        if (auto tgt = m_target.lock()) {
            tgt->OnPropertyChanged().Disconnect(m_targetConnId);
        }
    }
}

void Binding::UpdateTarget() {
    auto tgt = m_target.lock();
    auto src = m_source.lock();
    if (tgt && src) {
        m_isUpdating = true;
        Value val = src->GetProperty(m_sourceProperty);
        tgt->SetProperty(m_targetProperty, val);
        m_isUpdating = false;
    }
}

void Binding::UpdateSource() {
    auto tgt = m_target.lock();
    auto src = m_source.lock();
    if (tgt && src) {
        m_isUpdating = true;
        Value val = tgt->GetProperty(m_targetProperty);
        src->SetProperty(m_sourceProperty, val);
        m_isUpdating = false;
    }
}

} // namespace CUI
