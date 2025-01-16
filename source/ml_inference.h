#pragma once

#include <atomic>
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

        bool loadInput( const uint8_t* buffer, int len, int width, int height );
        bool genMask();
        const uint8_t* getMask() const;
        int getMaskWidth() const;
        int getMaskHeight() const;
        // void clearModel();

        void setPoints( const std::vector<glm::vec2>& points );
        void addPoint( const glm::vec2& point );
        void resetPoints();
        const std::vector<glm::vec2>& getPoints() const;

        int getMaxWidth() const;
        int getMaxHeight() const;

        bool inferenceReady() const;
        bool pipelineValid() const;

      private:
        std::vector<glm::vec2> m_points;
        std::vector<uint8_t> m_maskData;
        std::unique_ptr<OnnxData> m_onnxData;
        glm::vec2 m_imageSize;
        bool m_valid;
        std::atomic<bool> m_imageReady;
    };
} // namespace mc