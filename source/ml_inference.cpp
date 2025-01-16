#include "ml_inference.h"

#include "image.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_filesystem.h>
#define ORT_API_MANUAL_INIT
#include <codecvt>
#include <filesystem>
#include <locale>
#include <onnxruntime_cxx_api.h>

namespace mc
{
    struct OnnxData
    {
        Ort::Env env{ ORT_LOGGING_LEVEL_WARNING, "segment anything" };
        Ort::MemoryInfo memoryInfo{ Ort::MemoryInfo::CreateCpu( OrtArenaAllocator, OrtMemTypeDefault ) };
        Ort::RunOptions samRunOptions{};
        std::array<Ort::SessionOptions, 2> sessionOptions;
        std::unique_ptr<Ort::Session> sessionPre;
        std::unique_ptr<Ort::Session> sessionSam;
        std::vector<int64_t> inputShapePre;
        std::vector<int64_t> outputShapePre;
        std::vector<int64_t> outputShapeSam;
        std::vector<uint8_t> inputTensorValuesPre;
        std::vector<float> outputTensorValuesPre;
        std::vector<float> outputTensorValuesSam;
        std::unique_ptr<Ort::Value> inputTensorPre;
        std::unique_ptr<Ort::Value> outputTensorPre;

        OnnxData() {};
    };

    const std::array<const char*, 6> InputNamesSam{ "image_embeddings", "point_coords", "point_labels", "mask_input", "has_mask_input", "orig_im_size" };
    const std::array<const char*, 3> OutputNamesSam{ "masks", "iou_predictions", "low_res_masks" };
    const std::array<const char*, 1> InputNamesPre{ "input" };
    const std::array<const char*, 2> OutputNamesPre{ "output", "interm_embeddings" };

    const std::array<int64_t, 4> MaskInputShape    = { 1, 1, 256, 256 };
    const std::array<float, 1> HasMaskValues       = { 0 };
    const std::array<int64_t, 1> HasMaskInputShape = { 1 };
    const std::array<int64_t, 1> ImageSizeShape    = { 2 };

    // silly but idk an alternative
    const std::array<float, 65536> MaskInputValues;

    MlInference::MlInference( const std::string& preModelPath, const std::string& samModelPath, int threadsNumber )
        : m_valid( true )
    {
        const std::string fullPreModelPath = std::string( SDL_GetBasePath() ) + preModelPath;
        const std::string fullSamModelPath = std::string( SDL_GetBasePath() ) + samModelPath;


        if( !SDL_GetPathInfo( fullPreModelPath.c_str(), nullptr ) || !SDL_GetPathInfo( fullSamModelPath.c_str(), nullptr ) )
        {
            m_valid = false;
            return;
        }

        Ort::InitApi();
        m_onnxData = std::make_unique<OnnxData>();

        m_onnxData->sessionOptions[0].SetIntraOpNumThreads( threadsNumber );
        m_onnxData->sessionOptions[0].SetGraphOptimizationLevel( GraphOptimizationLevel::ORT_ENABLE_ALL );

        m_onnxData->sessionOptions[1].SetIntraOpNumThreads( threadsNumber );
        m_onnxData->sessionOptions[1].SetGraphOptimizationLevel( GraphOptimizationLevel::ORT_ENABLE_ALL );


#if defined( SDL_PLATFORM_WINDOWS )
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        auto wpreModelPath = converter.from_bytes( fullPreModelPath );
        auto wsamModelPath = converter.from_bytes( fullSamModelPath );
#else
        auto wpreModelPath = fullPreModelPath;
        auto wsamModelPath = fullSamModelPath;
#endif

        m_onnxData->sessionPre = std::make_unique<Ort::Session>( m_onnxData->env, wpreModelPath.c_str(), m_onnxData->sessionOptions[0] );
        m_onnxData->sessionSam = std::make_unique<Ort::Session>( m_onnxData->env, wsamModelPath.c_str(), m_onnxData->sessionOptions[1] );

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

    MlInference::~MlInference()
    {
    }

    bool MlInference::loadInput( const uint8_t* buffer, int len, int width, int height, int stride )
    {
        if( !m_valid || width > getMaxWidth() || height > getMaxHeight() )
        {
            return false;
        }

        m_imageReady = false;

        m_onnxData->inputTensorValuesPre.resize( m_onnxData->inputShapePre[0] * m_onnxData->inputShapePre[1] * m_onnxData->inputShapePre[2] *
                                                 m_onnxData->inputShapePre[3] );

        m_onnxData->outputTensorValuesPre.resize( m_onnxData->outputShapePre[0] * m_onnxData->outputShapePre[1] * m_onnxData->outputShapePre[2] *
                                                  m_onnxData->outputShapePre[3] );

        for( int i = 0; i < getMaxHeight(); ++i )
        {
            for( int j = 0; j < getMaxWidth(); ++j )
            {
                uint8_t r = i < height && j < width ? buffer[i * stride * 4 + j * 4 + 0] : 0;
                uint8_t g = i < height && j < width ? buffer[i * stride * 4 + j * 4 + 1] : 0;
                uint8_t b = i < height && j < width ? buffer[i * stride * 4 + j * 4 + 2] : 0;

                m_onnxData->inputTensorValuesPre[0 * getMaxHeight() * getMaxWidth() + i * getMaxWidth() + j] = r;
                m_onnxData->inputTensorValuesPre[1 * getMaxHeight() * getMaxWidth() + i * getMaxWidth() + j] = g;
                m_onnxData->inputTensorValuesPre[2 * getMaxHeight() * getMaxWidth() + i * getMaxWidth() + j] = b;
            }
        }

        m_onnxData->inputTensorPre = std::make_unique<Ort::Value>(
            Ort::Value::CreateTensor<uint8_t>( m_onnxData->memoryInfo, m_onnxData->inputTensorValuesPre.data(), m_onnxData->inputTensorValuesPre.size(),
                                               m_onnxData->inputShapePre.data(), m_onnxData->inputShapePre.size() ) );

        m_onnxData->outputTensorPre = std::make_unique<Ort::Value>(
            Ort::Value::CreateTensor<float>( m_onnxData->memoryInfo, m_onnxData->outputTensorValuesPre.data(), m_onnxData->outputTensorValuesPre.size(),
                                             m_onnxData->outputShapePre.data(), m_onnxData->outputShapePre.size() ) );

        m_onnxData->samRunOptions.UnsetTerminate();
        m_onnxData->sessionPre->RunAsync(
            m_onnxData->samRunOptions, InputNamesPre.data(), m_onnxData->inputTensorPre.get(), 1, OutputNamesPre.data(), m_onnxData->outputTensorPre.get(), 1,
            []( void* user_data, OrtValue** outputs, size_t num_outputs, OrtStatusPtr status )
            {
                MlInference* mlinference  = reinterpret_cast<MlInference*>( user_data );
                mlinference->m_imageReady = true;
            },
            this );


        m_imageSize = glm::vec2( width, height );

        return true;
    }

    void MlInference::setPoints( const std::vector<glm::vec2>& points )
    {
        m_points = points;
    }

    void MlInference::addPoint( const glm::vec2& point )
    {
        m_points.push_back( point );
    }

    void MlInference::resetPoints()
    {
        m_points.clear();
    }

    const std::vector<glm::vec2>& MlInference::getPoints() const
    {
        return m_points;
    }

    bool MlInference::genMask()
    {
        if( m_points.size() == 0 || !m_valid || !m_imageReady.load() )
        {
            return false;
        }

        m_onnxData->samRunOptions.SetTerminate();

        std::vector<float> inputPointValues( m_points.size() * 2 );
        std::vector<float> inputLabelValues( m_points.size(), 1 );
        for( int i = 0; i < m_points.size(); ++i )
        {
            inputPointValues[i * 2]     = m_points[i].x * m_imageSize.x;
            inputPointValues[i * 2 + 1] = m_points[i].y * m_imageSize.y;
            inputLabelValues[i]         = 1;
        }

        std::array<int64_t, 3> inputPointShape  = { 1, static_cast<int64_t>( m_points.size() ), 2 };
        std::array<int64_t, 2> pointLabelsShape = { 1, static_cast<int64_t>( m_points.size() ) };

        std::array<float, 2> inputImageSize{ getMaxHeight(), getMaxWidth() };

        std::array<Ort::Value, 6> inputTensorsSam{ Ort::Value::CreateTensor<float>( m_onnxData->memoryInfo, m_onnxData->outputTensorValuesPre.data(),
                                                                                    m_onnxData->outputTensorValuesPre.size(), m_onnxData->outputShapePre.data(),
                                                                                    m_onnxData->outputShapePre.size() ),
                                                   Ort::Value::CreateTensor<float>( m_onnxData->memoryInfo, inputPointValues.data(), inputPointValues.size(),
                                                                                    inputPointShape.data(), inputPointShape.size() ),
                                                   Ort::Value::CreateTensor<float>( m_onnxData->memoryInfo, inputLabelValues.data(), inputLabelValues.size(),
                                                                                    pointLabelsShape.data(), pointLabelsShape.size() ),
                                                   Ort::Value::CreateTensor<float>( m_onnxData->memoryInfo, const_cast<float*>( MaskInputValues.data() ),
                                                                                    MaskInputValues.size(), MaskInputShape.data(), MaskInputShape.size() ),
                                                   Ort::Value::CreateTensor<float>( m_onnxData->memoryInfo, const_cast<float*>( HasMaskValues.data() ),
                                                                                    HasMaskValues.size(), HasMaskInputShape.data(), HasMaskInputShape.size() ),
                                                   Ort::Value::CreateTensor<float>( m_onnxData->memoryInfo, inputImageSize.data(), inputImageSize.size(),
                                                                                    ImageSizeShape.data(), ImageSizeShape.size() ) };

        m_onnxData->outputShapeSam = { 1, 4, getMaxHeight(), getMaxWidth() };
        m_onnxData->outputTensorValuesSam.resize( m_onnxData->outputShapeSam[0] * m_onnxData->outputShapeSam[1] * m_onnxData->outputShapeSam[2] *
                                                  m_onnxData->outputShapeSam[3] );
        Ort::Value outputTensor( Ort::Value::CreateTensor<float>( m_onnxData->memoryInfo, m_onnxData->outputTensorValuesSam.data(),
                                                                  m_onnxData->outputTensorValuesSam.size(), m_onnxData->outputShapeSam.data(),
                                                                  m_onnxData->outputShapeSam.size() ) );

        m_onnxData->samRunOptions.UnsetTerminate();
        m_onnxData->sessionSam->Run( m_onnxData->samRunOptions, InputNamesSam.data(), inputTensorsSam.data(), inputTensorsSam.size(), OutputNamesSam.data(),
                                     &outputTensor, 1 );


        m_maskData.resize( getInputWidth() * getInputHeight() * 4 );

        for( int i = 0; i < getInputHeight(); ++i )
        {
            for( int j = 0; j < getInputWidth(); ++j )
            {
                uint8_t maskValue = std::clamp<float>( m_onnxData->outputTensorValuesSam[i * getMaxWidth() + j] * 255, 0.0, 255.0 );
                m_maskData[i * getInputWidth() * 4 + j * 4 + 0] = maskValue;
                m_maskData[i * getInputWidth() * 4 + j * 4 + 1] = maskValue;
                m_maskData[i * getInputWidth() * 4 + j * 4 + 2] = maskValue;
                m_maskData[i * getInputWidth() * 4 + j * 4 + 3] = 255;
            }
        }

        return true;
    }

    const uint8_t* MlInference::getMask() const
    {
        return m_maskData.data();
    }

    int MlInference::getInputWidth() const
    {
        return m_imageSize.x;
    }

    int MlInference::getInputHeight() const
    {
        return m_imageSize.y;
    }

    bool MlInference::inferenceReady() const
    {
        return m_imageReady.load();
    }

    bool MlInference::pipelineValid() const
    {
        return m_valid;
    }

    int MlInference::getMaxWidth() const
    {
        if( m_valid )
        {
            return m_onnxData->inputShapePre[3];
        }

        return 0;
    }
    int MlInference::getMaxHeight() const
    {
        if( m_valid )
        {
            return m_onnxData->inputShapePre[2];
        }

        return 0;
    }

} // namespace mc