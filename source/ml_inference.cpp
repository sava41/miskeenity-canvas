#include "ml_inference.h"

#include <onnxruntime_cxx_api.h>

namespace mc
{
    struct OnnxData
    {
        Ort::SessionOptions sessionOptions[2];
        std::unique_ptr<Ort::Session> sessionPre;
        std::unique_ptr<Ort::Session> sessionSam;
        std::vector<int64_t> inputShapePre;
        std::vector<int64_t> outputShapePre;
        std::vector<int64_t> intermShapePre;
        Ort::MemoryInfo memoryInfo;
        std::vector<float> outputTensorValuesPre;
        std::vector<float> intermTensorValuesPre;

        std::array<char*, 6> inputNamesSam{ { "image_embeddings" }, { "point_coords" },   { "point_labels" },
                                            { "mask_input" },       { "has_mask_input" }, { "orig_im_size" } };

        OnnxData()
            : memoryInfo( Ort::MemoryInfo::CreateCpu( OrtArenaAllocator, OrtMemTypeDefault ) )
        {
        }
    };

    MlInference::MlInference( const std::string& preModelPath, const std::string& samModelPath, int threadsNumber )
        : m_numThreads( threadsNumber )
        , m_onnxData( new OnnxData )
    {
        
    }

    MlInference::~MlInference()
    {
        delete m_onnxData;
    }

    bool MlInference::getMask( void* imageData, int width, int height, const std::vector<glm::vec2>& points )
    {
    }

} // namespace mc