#pragma once

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

    //glm::hvec2 uvTop;
    //glm::hvec2 uvBottom;

    uint16_t texture;
    uint16_t mask;

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

}
