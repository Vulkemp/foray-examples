#include "foray_minimalrtapp.hpp"
#include <gltf/foray_modelconverter.hpp>

namespace minimal_raytracer {
    void MinimalRaytracingStage::Init(foray::core::Context* context, foray::scene::Scene* scene)
    {
        mContext = context;
        mScene   = scene;
        RaytracingStage::Init();
    }
    void MinimalRaytracingStage::CreateRaytraycingPipeline()
    {
        mRaygen.LoadFromSource(mContext, RAYGEN_FILE);
        mClosestHit.LoadFromSource(mContext, CLOSESTHIT_FILE);
        mMiss.LoadFromSource(mContext, MISS_FILE);

        mPipeline.GetRaygenSbt().SetGroup(0, &mRaygen);
        mPipeline.GetHitSbt().SetGroup(0, &mClosestHit, nullptr, nullptr);
        mPipeline.GetMissSbt().SetGroup(0, &mMiss);
        RaytracingStage::CreateRaytraycingPipeline();
    }
    void MinimalRaytracingStage::OnShadersRecompiled()
    {
        foray::core::ShaderManager& shaderCompiler = foray::core::ShaderManager::Instance();

        bool rebuild = shaderCompiler.HasShaderBeenRecompiled(RAYGEN_FILE) | shaderCompiler.HasShaderBeenRecompiled(CLOSESTHIT_FILE) | shaderCompiler.HasShaderBeenRecompiled(MISS_FILE);
        if(rebuild)
        {
            ReloadShaders();
        }
    }
    void MinimalRaytracingStage::DestroyShaders()
    {
        mRaygen.Destroy();
        mClosestHit.Destroy();
        mMiss.Destroy();
    }

    void MinimalRaytracerApp::ApiInit()
    {
        mWindowSwapchain.GetWindow().DisplayMode(foray::osi::EDisplayMode::WindowedResizable);

        mScene = std::make_unique<foray::scene::Scene>(&mContext);

        foray::gltf::ModelConverter converter(mScene.get());

        converter.LoadGltfModel(SCENE_FILE);

        mScene->UpdateTlasManager();
        mScene->UseDefaultCamera();

        mRtStage.Init(&mContext, mScene.get());
        mSwapCopyStage.Init(&mContext, mRtStage.GetImageOutput(foray::stages::RaytracingStage::RaytracingRenderTargetName));

        RegisterRenderStage(&mRtStage);
        RegisterRenderStage(&mSwapCopyStage);
    }

    void MinimalRaytracerApp::ApiOnEvent(const foray::osi::Event* event)
    {
        mScene->InvokeOnEvent(event);
    }

    void MinimalRaytracerApp::ApiRender(foray::base::FrameRenderInfo& renderInfo)
    {
        foray::core::DeviceCommandBuffer& cmdBuffer = renderInfo.GetPrimaryCommandBuffer();
        cmdBuffer.Begin();
        renderInfo.GetInFlightFrame()->ClearSwapchainImage(cmdBuffer, renderInfo.GetImageLayoutCache());
        mScene->Update(renderInfo, cmdBuffer);
        mRtStage.RecordFrame(cmdBuffer, renderInfo);
        mSwapCopyStage.RecordFrame(cmdBuffer, renderInfo);
        renderInfo.GetInFlightFrame()->PrepareSwapchainImageForPresent(cmdBuffer, renderInfo.GetImageLayoutCache());
        cmdBuffer.Submit();
        // logger()->info("Frame #{}", renderInfo.GetFrameNumber());
    }

    void MinimalRaytracerApp::ApiDestroy()
    {
        mRtStage.Destroy();
        mSwapCopyStage.Destroy();
        mScene = nullptr;
    }
}  // namespace foray::minimal_raytracer
