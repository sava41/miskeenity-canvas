#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_precision.hpp>

namespace mc {

//enum LayerType : uint8_t { Textured, Paint, Mask, Text };

#pragma pack(push, 4) 
struct Layer{
    glm::vec2 offset;
    glm::vec2 basisA;
    glm::vec2 basisB;

    glm::u16vec2 uvTop;
    glm::u16vec2 uvBottom;

    uint16_t texture;
    uint16_t mask;

    glm::u8vec3 color;

    uint8_t type;
};
#pragma pack(pop)

const std::vector<float> VertexData {
    // Triangle #0
    -0.5, -0.5, 0.0 , 1.0,
    +0.5, -0.5, 1.0 , 1.0,
    +0.5, +0.5, 0.0 , 0.0,

    // Triangle #1
    -0.5, -0.5, 0.0 , 1.0,
    +0.5, +0.5, 0.0 , 0.0,
    -0.5, +0.5, 1.0 , 0.0,
};

class Layers {
public:
    Layers(size_t maxLayers);
    ~Layers() = default;

    bool add(const Layer &layer);
    bool move(int to, int from);
    bool remove(int index);

    size_t length() const;
    Layer *data() const;

private:
    size_t m_maxLength;
    size_t m_curLength;

    std::unique_ptr<Layer[]> m_array;
};
}
