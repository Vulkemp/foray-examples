#include "foray_complexrtapp.hpp"

namespace complex_raytracer {
    void ComplexRaytracingStage::Init(foray::core::Context* context, foray::scene::Scene* scene, foray::stages::RenderDomain* domain, int32_t resizeOrder)
    {
        mLightManager = scene->GetComponent<foray::scene::gcomp::LightManager>();
        foray::stages::DefaultRaytracingStageBase::Init(context, scene, domain, resizeOrder);
    }

    void ComplexRaytracingStage::ApiCreateRtPipeline()
    {
        foray::core::ShaderCompilerConfig options{.IncludeDirs = {FORAY_SHADER_DIR}};

        mShaderKeys.push_back(mRaygen.CompileFromSource(mContext, RAYGEN_FILE, options));
        mShaderKeys.push_back(mClosestHit.CompileFromSource(mContext, CLOSESTHIT_FILE, options));
        mShaderKeys.push_back(mAnyHit.CompileFromSource(mContext, ANYHIT_FILE, options));
        mShaderKeys.push_back(mMiss.CompileFromSource(mContext, MISS_FILE, options));
        mShaderKeys.push_back(mVisiMiss.CompileFromSource(mContext, VISI_MISS_FILE, options));
        mShaderKeys.push_back(mVisiAnyHit.CompileFromSource(mContext, VISI_ANYHIT_FILE, options));

        mPipeline.GetRaygenSbt().SetGroup(0, &mRaygen);
        mPipeline.GetHitSbt().SetGroup(0, &mClosestHit, &mAnyHit, nullptr);
        mPipeline.GetHitSbt().SetGroup(1, nullptr, &mVisiAnyHit, nullptr);
        mPipeline.GetMissSbt().SetGroup(0, &mMiss);
        mPipeline.GetMissSbt().SetGroup(1, &mVisiMiss);
        mPipeline.Build(mContext, mPipelineLayout);
    }

    void ComplexRaytracingStage::ApiDestroyRtPipeline()
    {
        mPipeline.Destroy();
        mRaygen.Destroy();
        mClosestHit.Destroy();
        mAnyHit.Destroy();
        mMiss.Destroy();
        mVisiMiss.Destroy();
        mVisiAnyHit.Destroy();
    }

    void ComplexRaytracingStage::CreateOrUpdateDescriptors()
    {
        const uint32_t bindpoint_lights = 11;

        mDescriptorSet.SetDescriptorAt(bindpoint_lights, mLightManager->GetBuffer().GetVkDescriptorInfo(), VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, foray::stages::RTSTAGEFLAGS);

        foray::stages::DefaultRaytracingStageBase::CreateOrUpdateDescriptors();
    }

    // void ComplexRaytracerApp::ApiBeforeInit()
    // {
    //     mInstance.SetEnableDebugReport(false);
    // }
    void ComplexRaytracerApp::ApiInit()
    {
        mWindowSwapchain.GetWindow().DisplayMode(foray::osi::EDisplayMode::WindowedResizable);

        mScene = std::make_unique<foray::scene::Scene>(&mContext);

        foray::gltf::ModelConverter converter(mScene.get());

        foray::gltf::ModelConverterOptions options{.FlipY = !INVERT_BLIT_INSTEAD};

        converter.LoadGltfModel(SCENE_FILE, nullptr, options);

        mScene->UpdateTlasManager();
        mScene->UseDefaultCamera(INVERT_BLIT_INSTEAD);
        mScene->UpdateLightManager();

        mRtStage.Init(&mContext, mScene.get(), &mWindowSwapchain);
        mRtStage.SetResizeOrder(1);
        mSwapCopyStage.Init(&mContext, mRtStage.GetRtOutput());

        if constexpr(INVERT_BLIT_INSTEAD)
        {
            mSwapCopyStage.SetFlipY(true);
        }
    }

    void ComplexRaytracerApp::ApiRender(foray::base::FrameRenderInfo& renderInfo)
    {
        foray::core::DeviceSyncCommandBuffer& cmdBuffer = renderInfo.GetPrimaryCommandBuffer();
        cmdBuffer.Begin();
        renderInfo.GetInFlightFrame()->ClearSwapchainImage(cmdBuffer, renderInfo.GetImageLayoutCache());
        mScene->Update(cmdBuffer, renderInfo, &mWindowSwapchain);
        mRtStage.RecordFrame(cmdBuffer, renderInfo);
        mSwapCopyStage.RecordFrame(cmdBuffer, renderInfo);
        renderInfo.GetInFlightFrame()->PrepareSwapchainImageForPresent(cmdBuffer, renderInfo.GetImageLayoutCache());
        cmdBuffer.Submit();
    }

    void ComplexRaytracerApp::ApiDestroy()
    {
        mRtStage.Destroy();
        mSwapCopyStage.Destroy();
        mScene = nullptr;
    }
}  // namespace complex_raytracer
