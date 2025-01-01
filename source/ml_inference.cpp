#include "ml_inference.h"

#include "image.h"

#include <SDL3/SDL.h>
#define ORT_API_MANUAL_INIT
#include <codecvt>
#include <filesystem>
#include <locale>
#include <onnxruntime_cxx_api.h>

namespace mc
{
    struct OnnxData
    {
        std::unique_ptr<Ort::Session> sessionPre;
        std::unique_ptr<Ort::Session> sessionSam;
        std::vector<int64_t> inputShapePre;
        std::vector<int64_t> outputShapePre;
        std::vector<float> outputTensorValuesPre;

        OnnxData() {};
    };

    const std::array<const char*, 6> InputNamesSam{ "image_embeddings", "point_coords", "point_labels", "mask_input", "has_mask_input", "orig_im_size" };
    const std::array<const char*, 3> OutputNamesSam{ "masks", "iou_predictions", "low_res_masks" };
    const std::array<const char*, 1> InputNamesPre{ "input" };
    const std::array<const char*, 2> OutputNamesPre{ "output", "interm_embeddings" };

    const std::array<float, 1> HasMaskValues       = { 0 };
    const std::array<int64_t, 1> HasMaskInputShape = { 1 };
    const std::array<int64_t, 1> ImageSizeShape    = { 2 };

    MlInference::MlInference( const std::string& preModelPath, const std::string& samModelPath, int threadsNumber )
        : m_valid( true )
    {
        if( !std::filesystem::exists( preModelPath ) || std::filesystem::exists( samModelPath ) )
        {
            m_valid = false;
            return;
        }

        Ort::InitApi();

        Ort::Env env( ORT_LOGGING_LEVEL_WARNING, "test" );

        Ort::SessionOptions sessionOptions[2];

        sessionOptions[0].SetIntraOpNumThreads( threadsNumber );
        sessionOptions[0].SetGraphOptimizationLevel( GraphOptimizationLevel::ORT_ENABLE_ALL );

        sessionOptions[1].SetIntraOpNumThreads( threadsNumber );
        sessionOptions[1].SetGraphOptimizationLevel( GraphOptimizationLevel::ORT_ENABLE_ALL );

        m_onnxData = std::make_unique<OnnxData>();


#if defined( SDL_PLATFORM_WINDOWS )
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        auto wpreModelPath = converter.from_bytes( preModelPath );
        auto wsamModelPath = converter.from_bytes( samModelPath );
#else
        auto wpreModelPath = preModelPath;
        auto wsamModelPath = samModelPath;
#endif

        m_onnxData->sessionPre = std::make_unique<Ort::Session>( env, wpreModelPath.c_str(), sessionOptions[0] );
        m_onnxData->sessionSam = std::make_unique<Ort::Session>( env, wsamModelPath.c_str(), sessionOptions[1] );

        if( m_onnxData->sessionPre->GetInputCount() != 1 && m_onnxData->sessionPre->GetOutputCount() != 1 && m_onnxData->sessionSam->GetInputCount() != 6 &&
            m_onnxData->sessionSam->GetOutputCount() != 3 )
        {
            m_valid = false;
            return;
        }

        m_onnxData->inputShapePre  = m_onnxData->sessionPre->GetInputTypeInfo( 0 ).GetTensorTypeAndShapeInfo().GetShape();
        m_onnxData->outputShapePre = m_onnxData->sessionPre->GetOutputTypeInfo( 0 ).GetTensorTypeAndShapeInfo().GetShape();

        if( m_onnxData->inputShapePre.size() != 4 || m_onnxData->outputShapePre.size() != 4 )
        {
            m_valid = false;
            return;
        }
    }

    bool MlInference::pipelineValid() const
    {
        return m_valid;
    }

    bool MlInference::loadInput( const uint8_t* buffer, int len, int& width, int& height )
    {
        if( !m_valid || width > m_onnxData->inputShapePre[3] || height > m_onnxData->inputShapePre[2] )
        {
            return false;
        }

        Ort::MemoryInfo memoryInfo{ Ort::MemoryInfo::CreateCpu( OrtArenaAllocator, OrtMemTypeDefault ) };

        std::vector<uint8_t> inputTensorValues( m_onnxData->inputShapePre[0] * m_onnxData->inputShapePre[1] * m_onnxData->inputShapePre[2] *
                                                m_onnxData->inputShapePre[3] );

        for( int i = 0; i < m_onnxData->inputShapePre[2]; ++i )
        {
            for( int j = 0; j < m_onnxData->inputShapePre[3]; ++j )
            {
                uint8_t r = 0;
                uint8_t g = 0;
                uint8_t b = 0;

                if( i < width && j < height )
                {
                    r = buffer[i * width * 4 + j * 4 + 0];
                    g = buffer[i * width * 4 + j * 4 + 1];
                    b = buffer[i * width * 4 + j * 4 + 2];
                }

                inputTensorValues[i * m_onnxData->inputShapePre[3] + j]                                                                   = r;
                inputTensorValues[m_onnxData->inputShapePre[2] * m_onnxData->inputShapePre[3] + i * m_onnxData->inputShapePre[3] + j]     = g;
                inputTensorValues[2 * m_onnxData->inputShapePre[2] * m_onnxData->inputShapePre[3] + i * m_onnxData->inputShapePre[3] + j] = b;
            }
        }

        Ort::Value inputTensor( Ort::Value::CreateTensor<uint8_t>( memoryInfo, inputTensorValues.data(), inputTensorValues.size(),
                                                                   m_onnxData->inputShapePre.data(), m_onnxData->inputShapePre.size() ) );

        m_onnxData->outputTensorValuesPre.resize( m_onnxData->outputShapePre[0] * m_onnxData->outputShapePre[1] * m_onnxData->outputShapePre[2] *
                                                  m_onnxData->outputShapePre[3] );

        Ort::Value outputTensor( Ort::Value::CreateTensor<float>( memoryInfo, m_onnxData->outputTensorValuesPre.data(),
                                                                  m_onnxData->outputTensorValuesPre.size(), m_onnxData->outputShapePre.data(),
                                                                  m_onnxData->outputShapePre.size() ) );

        Ort::RunOptions runOptions;

        m_onnxData->sessionPre->Run( runOptions, InputNamesPre.data(), &inputTensor, 1, OutputNamesPre.data(), &outputTensor, 1 );

        m_imageSize = glm::vec2( width, height );

        return true;
    }

    MlInference::~MlInference()
    {
    }

    bool MlInference::getMask( void* imageData, int width, int height, const std::vector<glm::vec2>& points )
    {
        if( points.size() == 0 )
        {
            return false;
        }

        Ort::MemoryInfo memoryInfo{ Ort::MemoryInfo::CreateCpu( OrtArenaAllocator, OrtMemTypeDefault ) };

        std::vector<float> inputPointValues( points.size() * 2 );
        std::vector<float> inputLabelValues( points.size(), 1 );
        for( int i = 0; i < points.size(); i += 2 )
        {
            inputPointValues[i]     = points[i].x;
            inputPointValues[i + 1] = points[i + 1].y;
        }

        std::vector<int64_t> inputPointShape  = { 1, static_cast<int64_t>( points.size() ), 2 };
        std::vector<int64_t> pointLabelsShape = { 1, static_cast<int64_t>( points.size() ) };

        std::vector<Ort::Value> inputTensorsSam;

        // std::array<Ort::Value, 6> inputTensorsSam{};

        inputTensorsSam.push_back( Ort::Value::CreateTensor<float>( memoryInfo, m_onnxData->outputTensorValuesPre.data(),
                                                                    m_onnxData->outputTensorValuesPre.size(), m_onnxData->outputShapePre.data(),
                                                                    m_onnxData->outputShapePre.size() ) );

        inputTensorsSam.push_back(
            Ort::Value::CreateTensor<float>( memoryInfo, inputPointValues.data(), inputPointValues.size(), inputPointShape.data(), inputPointShape.size() ) );
        inputTensorsSam.push_back(
            Ort::Value::CreateTensor<float>( memoryInfo, inputLabelValues.data(), inputLabelValues.size(), pointLabelsShape.data(), pointLabelsShape.size() ) );

        inputTensorsSam.push_back( Ort::Value::CreateTensor<float>( memoryInfo, nullptr, 0, nullptr, 0 ) );
        inputTensorsSam.push_back( Ort::Value::CreateTensor<float>( memoryInfo, const_cast<float*>( HasMaskValues.data() ), HasMaskValues.size(),
                                                                    HasMaskInputShape.data(), HasMaskInputShape.size() ) );

        inputTensorsSam.push_back( Ort::Value::CreateTensor<float>( memoryInfo, &m_imageSize.x, 2, ImageSizeShape.data(), ImageSizeShape.size() ) );

        // Ort::Value outputTensor( Ort::Value::CreateTensor<float>( memoryInfo, m_onnxData->outputTensorValuesPre.data(),
        //                                                           m_onnxData->outputTensorValuesPre.size(), m_onnxData->outputShapePre.data(),
        //                                                           m_onnxData->outputShapePre.size() ) );

        Ort::RunOptions runOptionsSam;
        auto outputTensors = m_onnxData->sessionSam->Run( runOptionsSam, InputNamesSam.data(), inputTensorsSam.data(), inputTensorsSam.size(),
                                                          OutputNamesSam.data(), OutputNamesSam.size() );

        auto& outputMask = outputTensors[0];
        auto maskShape   = outputMask.GetTensorTypeAndShapeInfo().GetShape();

        float iouValue = outputTensors[1].GetTensorMutableData<float>()[0];

        return true;
    }

} // namespace mc