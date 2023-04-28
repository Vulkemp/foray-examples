#include "foray_minimalrtapp.hpp"

namespace minimal_raytracer {
    MinimalRaytracingStage::MinimalRaytracingStage(foray::core::Context* context, foray::scene::Scene* scene, foray::stages::RenderDomain* domain)
     : foray::stages::DefaultRaytracingStageBase(context, domain, 0)
    {
        foray::stages::DefaultRaytracingStageBase::Init(scene);
    }

    void MinimalRaytracingStage::ApiCreateRtPipeline()
    {
        // Compiling shaders with 'repo-root/foray/src/shaders' additional include directory
        foray::core::ShaderCompilerConfig options{.IncludeDirs = {FORAY_SHADER_DIR}};

        // Compiles shaders and loads them into mRaygen, mClosestHit, mMiss ShaderModules.
        // Store compilation keys so that the Rt pipeline is recreated whenever the shaders change
        mRaygen.New();
        mClosestHit.New();
        mMiss.New();
        mShaderKeys.push_back(mRaygen->CompileFromSource(mContext, RAYGEN_FILE, options));
        mShaderKeys.push_back(mClosestHit->CompileFromSource(mContext, CLOSESTHIT_FILE, options));
        mShaderKeys.push_back(mMiss->CompileFromSource(mContext, MISS_FILE, options));

        // Configure shader binding table
        foray::rtpipe::RtPipeline::Builder builder;
        builder.GetRaygenSbtBuilder().SetEntryModule(0, mRaygen.Get());
        builder.GetRaygenSbtBuilder().SetEntryModule(0, mRaygen.Get());
        builder.GetHitSbtBuilder().SetEntryModules(0, mClosestHit.Get(), nullptr, nullptr);
        builder.GetMissSbtBuilder().SetEntryModule(0, mMiss.Get());
        builder.SetPipelineLayout(mPipelineLayout.GetPipelineLayout());

        // Build Binding Table and Pipeline
        mPipeline.New(mContext, builder);
    }

    void MinimalRaytracingStage::ApiDestroyRtPipeline()
    {
        // Destroy pipeline and shader modules
        mPipeline.Delete();
        mRaygen.Delete();
        mClosestHit.Delete();
        mMiss.Delete();
    }

    void MinimalRaytracerApp::ApiInit()
    {
        mWindowSwapchain->GetWindow().DisplayMode(foray::osi::EDisplayMode::WindowedResizable);

        // Load scene
        mScene.New(&mContext);
        foray::gltf::ModelConverter converter(mScene.Get());
        converter.LoadGltfModel(SCENE_FILE);

        // Initialize TLAS
        mScene->UpdateTlasManager();
        // Adds a node with camera + freeflight controls
        mScene->UseDefaultCamera(true);

        // Initialize and configure stages
        mRtStage.New(&mContext, mScene.Get(), mWindowSwapchain.Get());
        mRtStage->SetResizeOrder(1);
        mSwapCopyStage.New(&mContext, mRtStage->GetRtOutput());
        mSwapCopyStage->SetFlipY(true);
    }

    void MinimalRaytracerApp::ApiRender(foray::base::FrameRenderInfo& renderInfo)
    {
        // Get and begin command buffer
        foray::core::DeviceSyncCommandBuffer& cmdBuffer = renderInfo.GetPrimaryCommandBuffer();
        cmdBuffer.Begin();

        // Update scene (uploads scene specific dynamic data such as node transformations, camera matrices, ...)
        mScene->Update(cmdBuffer, renderInfo, mWindowSwapchain.Get());

        // Call ray tracer
        mRtStage->RecordFrame(cmdBuffer, renderInfo);

        // Copy ray tracing output to swapchain
        mSwapCopyStage->RecordFrame(cmdBuffer, renderInfo);

        // Prepare swapchain image for present and submit command buffer
        renderInfo.GetInFlightFrame()->PrepareSwapchainImageForPresent(cmdBuffer, renderInfo.GetImageLayoutCache());
        cmdBuffer.Submit();
    }

    void MinimalRaytracerApp::ApiDestroy()
    {
        mRtStage       = nullptr;
        mSwapCopyStage = nullptr;
        mScene         = nullptr;
    }
}  // namespace minimal_raytracer
