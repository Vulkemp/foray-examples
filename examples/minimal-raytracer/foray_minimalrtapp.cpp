#include "foray_minimalrtapp.hpp"
#include <gltf/foray_modelconverter.hpp>

namespace minimal_raytracer {
    void MinimalRaytracingStage::CreateRtPipeline()
    {
        // Compiling shaders with 'repo-root/foray/src/shaders' additional include directory
        foray::core::ShaderCompilerConfig options{.IncludeDirs = {FORAY_SHADER_DIR}};

        // Compiles shaders and loads them into mRaygen, mClosestHit, mMiss ShaderModules.
        // Store compilation keys so that the Rt pipeline is recreated whenever the shaders change
        mShaderKeys.push_back(mRaygen.CompileFromSource(mContext, RAYGEN_FILE, options));
        mShaderKeys.push_back(mClosestHit.CompileFromSource(mContext, CLOSESTHIT_FILE, options));
        mShaderKeys.push_back(mMiss.CompileFromSource(mContext, MISS_FILE, options));

        // Configure shader binding table
        mPipeline.GetRaygenSbt().SetGroup(0, &mRaygen);
        mPipeline.GetHitSbt().SetGroup(0, &mClosestHit, nullptr, nullptr);
        mPipeline.GetMissSbt().SetGroup(0, &mMiss);

        // Build Binding Table and Pipeline
        mPipeline.Build(mContext, mPipelineLayout);
    }

    void MinimalRaytracingStage::DestroyRtPipeline()
    {
        // Destroy pipeline and shader modules
        mPipeline.Destroy();
        mRaygen.Destroy();
        mClosestHit.Destroy();
        mMiss.Destroy();
    }

    void MinimalRaytracerApp::ApiInit()
    {
        mWindowSwapchain.GetWindow().DisplayMode(foray::osi::EDisplayMode::WindowedResizable);

        // Load scene
        mScene = std::make_unique<foray::scene::Scene>(&mContext);
        foray::gltf::ModelConverter converter(mScene.get());
        converter.LoadGltfModel(SCENE_FILE);

        // Initialize TLAS
        mScene->UpdateTlasManager();
        // Adds a node with camera + freeflight controls
        mScene->UseDefaultCamera(true);

        // Initialize and configure stages
        mRtStage.Init(&mContext, mScene.get());
        mSwapCopyStage.Init(&mContext, mRtStage.GetRtOutput());
        mSwapCopyStage.SetFlipY(true);

        RegisterRenderStage(&mRtStage);
        RegisterRenderStage(&mSwapCopyStage);
    }

    void MinimalRaytracerApp::ApiOnEvent(const foray::osi::Event* event)
    {
        mScene->InvokeOnEvent(event);
    }

    void MinimalRaytracerApp::ApiOnResized(VkExtent2D size)
    {
        mScene->InvokeOnResized(size);
    }

    void MinimalRaytracerApp::ApiRender(foray::base::FrameRenderInfo& renderInfo)
    {
        // Get and begin command buffer
        foray::core::DeviceCommandBuffer& cmdBuffer = renderInfo.GetPrimaryCommandBuffer();
        cmdBuffer.Begin();

        // Update scene (uploads scene specific dynamic data such as node transformations, camera matrices, ...)
        mScene->Update(renderInfo, cmdBuffer);

        // Call ray tracer
        mRtStage.RecordFrame(cmdBuffer, renderInfo);

        // Copy ray tracing output to swapchain
        mSwapCopyStage.RecordFrame(cmdBuffer, renderInfo);
        
        // Prepare swapchain image for present and submit command buffer
        renderInfo.GetInFlightFrame()->PrepareSwapchainImageForPresent(cmdBuffer, renderInfo.GetImageLayoutCache());
        cmdBuffer.Submit();
    }

    void MinimalRaytracerApp::ApiDestroy()
    {
        mRtStage.Destroy();
        mSwapCopyStage.Destroy();
        mScene = nullptr;
    }
}  // namespace minimal_raytracer
