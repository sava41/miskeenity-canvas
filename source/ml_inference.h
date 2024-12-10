#pragma once

#include <glm/glm.hpp>
#include <string>

namespace mc
{
    struct OnnxData;

    class MlInference
    {
      public:
        MlInference( const std::string& preModelPath, const std::string& samModelPath, int threadsNumber );

        bool getMask( void* imageData, int width, int height, const std::vector<glm::vec2>& points );


      private:
        int m_numThreads = 8;
        OnnxData* m_onnxData;
    };
} // namespace mc