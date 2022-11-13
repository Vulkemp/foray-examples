#include "foray_pbr_rtapp.hpp"
#include <gltf/foray_modelconverter.hpp>
#include <scene/globalcomponents/foray_lightmanager.hpp>

namespace pbr_raytracer {
    void PbrRaytracingStage::Init(foray::core::Context* context, foray::scene::Scene* scene)
    {
        mLightManager = scene->GetComponent<foray::scene::gcomp::LightManager>();
        foray::stages::ExtRaytracingStage::Init(context, scene);
    }

    void PbrRaytracingStage::CreateRtPipeline()
    {
        mRaygen.LoadFromSource(mContext, RAYGEN_FILE);
        mClosestHit.LoadFromSource(mContext, CLOSESTHIT_FILE);
        mMiss.LoadFromSource(mContext, MISS_FILE);
        mVisiMiss.LoadFromSource(mContext, VISI_MISS_FILE);

        mShaderSourcePaths.insert(mShaderSourcePaths.begin(), {RAYGEN_FILE, CLOSESTHIT_FILE, MISS_FILE, VISI_MISS_FILE});

        mPipeline.GetRaygenSbt().SetGroup(0, &mRaygen);
        mPipeline.GetHitSbt().SetGroup(0, &mClosestHit, nullptr, nullptr);
        mPipeline.GetMissSbt().SetGroup(0, &mMiss);
        mPipeline.GetMissSbt().SetGroup(1, &mVisiMiss);
        mPipeline.Build(mContext, mPipelineLayout);
    }

    void PbrRaytracingStage::DestroyRtPipeline()
    {
        mPipeline.Destroy();
        mRaygen.Destroy();
        mClosestHit.Destroy();
        mMiss.Destroy();
        mVisiMiss.Destroy();
    }

    void PbrRaytracingStage::CreateOrUpdateDescriptors()
    {
        const uint32_t bindpoint_lights = 11;

        mDescriptorSet.SetDescriptorAt(bindpoint_lights, mLightManager->GetBuffer().GetVkDescriptorInfo(), VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, RTSTAGEFLAGS);

        foray::stages::ExtRaytracingStage::CreateOrUpdateDescriptors();
    }

    void PbrRaytracerApp::ApiBeforeInit()
    {
        mInstance.SetEnableDebugReport(false);
    }
    void PbrRaytracerApp::ApiInit()
    {
        mWindowSwapchain.GetWindow().DisplayMode(foray::osi::EDisplayMode::WindowedResizable);

        mScene = std::make_unique<foray::scene::Scene>(&mContext);

        foray::gltf::ModelConverter converter(mScene.get());

        converter.LoadGltfModel(SCENE_FILE);

        mScene->UpdateTlasManager();
        mScene->UseDefaultCamera();
        mScene->UpdateLightManager();

        foray::scene::gcomp::LightManager* lightManager = mScene->GetComponent<foray::scene::gcomp::LightManager>();

        mRtStage.Init(&mContext, mScene.get());
        mSwapCopyStage.Init(&mContext, mRtStage.GetRtOutput());

        RegisterRenderStage(&mRtStage);
        RegisterRenderStage(&mSwapCopyStage);
    }

    void PbrRaytracerApp::ApiOnEvent(const foray::osi::Event* event)
    {
        mScene->InvokeOnEvent(event);
    }

    void PbrRaytracerApp::ApiOnResized(VkExtent2D size)
    {
        mScene->InvokeOnResized(size);
    }

    void PbrRaytracerApp::ApiRender(foray::base::FrameRenderInfo& renderInfo)
    {
        foray::core::DeviceCommandBuffer& cmdBuffer = renderInfo.GetPrimaryCommandBuffer();
        cmdBuffer.Begin();
        renderInfo.GetInFlightFrame()->ClearSwapchainImage(cmdBuffer, renderInfo.GetImageLayoutCache());
        mScene->Update(renderInfo, cmdBuffer);
        mRtStage.RecordFrame(cmdBuffer, renderInfo);
        mSwapCopyStage.RecordFrame(cmdBuffer, renderInfo);
        renderInfo.GetInFlightFrame()->PrepareSwapchainImageForPresent(cmdBuffer, renderInfo.GetImageLayoutCache());
        cmdBuffer.Submit();
    }

    void PbrRaytracerApp::ApiDestroy()
    {
        mRtStage.Destroy();
        mSwapCopyStage.Destroy();
        mScene = nullptr;
    }
}  // namespace pbr_raytracer
