#include "layers.h"

namespace mc {

Layers::Layers(size_t maxLayers)
    : m_curLength(0), m_maxLength(maxLayers), m_array(std::make_unique<Layer[]>(maxLayers))
{
}

bool Layers::add(const Layer &layer)
{
    if (m_curLength == m_maxLength) {
        return false;
    }

    m_array[m_curLength] = layer;
    m_curLength += 1;

    return true;
}

bool Layers::move(int to, int from)
{
    if (to < 0 || from < 0 || to > m_curLength || from > m_curLength || to == from) {
        return false;
    }

    Layer temp = m_array[from];

    if (to > from) {
        std::memmove(
            m_array.get() + from, m_array.get() + from + 1, (to - from) * sizeof(Layer));
    } else {
        std::memmove(m_array.get() + to + 1, m_array.get() + to, (from - to) * sizeof(Layer));
    }

    m_array[to] = temp;

    return true;
}

bool Layers::remove(int index)
{
    if (index < 0 || index >= m_curLength) {
        return false;
    }

    if (index != m_curLength - 1) {
        std::memmove(m_array.get() + index,
                     m_array.get() + index + 1,
                     (m_curLength - index - 1) * sizeof(Layer));
    }

    m_curLength -= 1;

    return true;
}

size_t Layers::length() const
{
    return m_curLength;
}

Layer *Layers::data() const
{
    return m_array.get();
}

}