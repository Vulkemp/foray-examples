#include <chrono>
#include <foray_api.hpp>
#include <fstream>
#include <vector>

namespace inflighttest {

    namespace chr = std::chrono;

    inline const std::string SCENE_FILE = DATA_DIR "/gltf/outdoorbox/scene.gltf";

    class InFlightTestApp : public foray::base::DefaultAppBase
    {
      protected:
        virtual void ApiInit() override;
        virtual void ApiBeforeSwapchainBuilding(vkb::SwapchainBuilder& swapchainBuilder) override;
        virtual void ApiRender(foray::base::FrameRenderInfo& renderInfo) override;
        virtual void ApiFrameFinishedExecuting(uint64_t frameIndex) override;
        virtual void ApiOnEvent(const foray::osi::Event* event) override;
        virtual void ApiDestroy() override;

        chr::high_resolution_clock clock;

        using timepoint = chr::time_point<chr::high_resolution_clock, chr::duration<double, std::milli>>;

        timepoint startup                = clock.now();
        timepoint frameBeginTimestamp[2] = {timepoint::min(), timepoint::min()};

        /// @brief The GBuffer stage renders scene information using rasterization
        foray::stages::GBufferStage mGBufferStage;
        /// @brief The swap copy stage blits the output image onto the swapchain
        foray::stages::ImageToSwapchainStage mSwapCopyStage;

        /// @brief Scene
        std::unique_ptr<foray::scene::Scene> mScene;

        foray::osi::Utf8Path savePath;
        std::fstream         out;
        uint64_t             recordCount = 0;
    };

    void InFlightTestApp::ApiInit()
    {
        mRenderLoop.GetFrameTiming().DisableFpsLimit();

        {  // Load Scene
            mScene = std::make_unique<foray::scene::Scene>(&mContext);

            foray::gltf::ModelConverter converter(mScene.get());

            converter.LoadGltfModel(SCENE_FILE);

            mScene->UseDefaultCamera(true);
        }

        {  // Init and register render stages

            mGBufferStage.Init(&mContext, mScene.get());

            mSwapCopyStage.Init(&mContext, mGBufferStage.GetImageEOutput(foray::stages::GBufferStage::EOutput::Albedo));
            mSwapCopyStage.SetFlipY(true);

            RegisterRenderStage(&mGBufferStage);
            RegisterRenderStage(&mSwapCopyStage);
        }

        {
            savePath = foray::osi::Utf8Path(fmt::format("BenchFrames_{}_Frames.csv", foray::INFLIGHT_FRAME_COUNT)).MakeAbsolute();
            out.open((std::filesystem::path)savePath, std::ios_base::out);
            out << "Since Start" << ";" << "Frame Delta" << std::endl;
        }
    }

    void InFlightTestApp::ApiBeforeSwapchainBuilding(vkb::SwapchainBuilder& swapchainBuilder)
    {
        swapchainBuilder.set_desired_present_mode(VkPresentModeKHR::VK_PRESENT_MODE_IMMEDIATE_KHR);
    }

    void InFlightTestApp::ApiRender(foray::base::FrameRenderInfo& renderInfo)
    {

        foray::core::DeviceSyncCommandBuffer& cmdBuffer = renderInfo.GetPrimaryCommandBuffer();
        cmdBuffer.Begin();

        // Update the scene. This updates the scenegraph for animated objects, updates camera matrices, etc.
        mScene->Update(renderInfo, cmdBuffer);

        // Record the rasterized GBuffer stage
        mGBufferStage.RecordFrame(cmdBuffer, renderInfo);

        // Copy comparer stage output to the swapchain
        mSwapCopyStage.RecordFrame(cmdBuffer, renderInfo);

        // Prepare for present
        renderInfo.PrepareSwapchainImageForPresent(cmdBuffer);

        cmdBuffer.Submit();
    }
    void InFlightTestApp::ApiFrameFinishedExecuting(uint64_t frameIndex)
    {
        namespace fs = std::filesystem;

        if(frameIndex % 1000 == 0)
        {
            foray::logger()->info("Frame # {} finished", frameIndex);
        }

        timepoint& framebegin = frameBeginTimestamp[frameIndex % foray::INFLIGHT_FRAME_COUNT];
        if(framebegin > timepoint::min())
        {
            out << (timepoint(clock.now()) - startup).count() << ";" << (timepoint(clock.now()) - framebegin).count() << std::endl;
            recordCount++;
        }
        framebegin = clock.now();

        if(recordCount >= uint64_t{50000U})
        {
            out.flush();
            out.close();
            mRenderLoop.RequestStop();
        }
    }
    void InFlightTestApp::ApiOnEvent(const foray::osi::Event* event)
    {
        // Scene event handling includes moving the camera around
        mScene->HandleEvent(event);
    }

    void InFlightTestApp::ApiDestroy()
    {
        mScene = nullptr;  // The unique ptr will call destructor upon assigning a nullptr value
        mGBufferStage.Destroy();
        mSwapCopyStage.Destroy();
    }

    int example(std::vector<std::string>& args)
    {
        foray::osi::OverrideCurrentWorkingDirectory(CWD_OVERRIDE);
        InFlightTestApp app;
        return app.Run();
    }
}  // namespace inflighttest

int main(int argv, char** args)
{
    std::vector<std::string> argvec(argv);
    for(int i = 0; i < argv; i++)
    {
        argvec[i] = args[i];
    }
    return inflighttest::example(argvec);
}
