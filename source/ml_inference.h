#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

namespace mc
{
    struct OnnxData;

    class MlInference
    {
      public:
        MlInference( const std::string& preModelPath, const std::string& samModelPath, int threadsNumber );
        ~MlInference();

        bool loadInput( const uint8_t* buffer, int len, int& width, int& height );

        bool getMask( void* imageData, int width, int height, const std::vector<glm::vec2>& points );

        bool pipelineValid() const;

      private:
        std::unique_ptr<OnnxData> m_onnxData;
        glm::vec2 m_imageSize;
        bool m_valid;
    };
} // namespace mc