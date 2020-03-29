#include "property.h"

namespace celestia
{
namespace engine
{

template <typename T>
void Property<T>::copy(const Property<T> &other)
{
    m_config = other.m_config;
    m_name = other.m_name;
    m_default = other.m_default;
    m_validate = other.m_validate;
    m_config->addProperty(this);
}

template <typename T>
void Property<T>::move(Property<T> &&other)
{
    m_config = other.m_config;
    m_name = std::move(other.m_name);
    m_default = std::move(other.m_default);
    m_validate = other.m_validate;
    m_config->addProperty(this);
    m_config->removeProperty(&other);
    other.m_config = nullptr;
    other.m_validate = nullptr;
}

#define PROPERTY_UPDATE(ValueType) \
    auto v = m_config->find(m_name); \
    m_hasValue = v->getType() == Value:: ValueType ## Type; \
    if (m_hasValue) \
    { \
        if (m_validate != nullptr) \
            m_value = m_validate(v->get ## ValueType ()); \
        else \
            m_value = v->get ## ValueType (); \
    }

template<>
void Property<double>::update()
{
    PROPERTY_UPDATE(Number);
}

template<>
void Property<std::string>::update()
{
    PROPERTY_UPDATE(String);
}

template<>
void Property<bool>::update()
{
    PROPERTY_UPDATE(Boolean);
}

#undef PROPERTY_UPDATE

}
} // namespace;
