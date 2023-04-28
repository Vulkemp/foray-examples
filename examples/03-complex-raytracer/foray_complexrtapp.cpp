#include "foray_complexrtapp.hpp"

namespace complex_raytracer {
    ComplexRaytracingStage::ComplexRaytracingStage(foray::core::Context* context, foray::scene::Scene* scene, foray::stages::RenderDomain* domain, int32_t resizeOrder)
        : foray::stages::DefaultRaytracingStageBase(context, domain, resizeOrder)
    {
        mLightManager = scene->GetComponent<foray::scene::gcomp::LightManager>();
        foray::stages::DefaultRaytracingStageBase::Init(scene);
    }

    void ComplexRaytracingStage::ApiCreateRtPipeline()
    {
        foray::core::ShaderCompilerConfig options{.IncludeDirs = {FORAY_SHADER_DIR}};

        mRaygen.New();
        mClosestHit.New();
        mAnyHit.New();
        mMiss.New();
        mVisiMiss.New();
        mVisiAnyHit.New();
        mShaderKeys.push_back(mRaygen->CompileFromSource(mContext, RAYGEN_FILE, options));
        mShaderKeys.push_back(mClosestHit->CompileFromSource(mContext, CLOSESTHIT_FILE, options));
        mShaderKeys.push_back(mAnyHit->CompileFromSource(mContext, ANYHIT_FILE, options));
        mShaderKeys.push_back(mMiss->CompileFromSource(mContext, MISS_FILE, options));
        mShaderKeys.push_back(mVisiMiss->CompileFromSource(mContext, VISI_MISS_FILE, options));
        mShaderKeys.push_back(mVisiAnyHit->CompileFromSource(mContext, VISI_ANYHIT_FILE, options));

        foray::rtpipe::RtPipeline::Builder builder;
        builder.GetRaygenSbtBuilder().SetEntryModule(0, mRaygen.Get());
        builder.GetHitSbtBuilder().SetEntryModules(0, mClosestHit.Get(), mAnyHit.Get(), nullptr);
        builder.GetHitSbtBuilder().SetEntryModules(1, nullptr, mVisiAnyHit.Get(), nullptr);
        builder.GetMissSbtBuilder().SetEntryModule(0, mMiss.Get());
        builder.GetMissSbtBuilder().SetEntryModule(1, mVisiMiss.Get());
        builder.SetPipelineLayout(mPipelineLayout.GetPipelineLayout());
        mPipeline.New(mContext, builder);
    }

    void ComplexRaytracingStage::ApiDestroyRtPipeline()
    {
        mPipeline.Delete();
        mRaygen.Delete();
        mClosestHit.Delete();
        mAnyHit.Delete();
        mMiss.Delete();
        mVisiMiss.Delete();
        mVisiAnyHit.Delete();
    }

    void ComplexRaytracingStage::CreateOrUpdateDescriptors()
    {
        const uint32_t bindpoint_lights = 11;

        mDescriptorSet.SetDescriptorAt(bindpoint_lights, mLightManager->GetBuffer()->GetVkDescriptorInfo(), VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                       foray::stages::RTSTAGEFLAGS);

        foray::stages::DefaultRaytracingStageBase::CreateOrUpdateDescriptors();
    }

    // void ComplexRaytracerApp::ApiBeforeInit()
    // {
    //     mInstance.SetEnableDebugReport(false);
    // }
    void ComplexRaytracerApp::ApiInit()
    {
        mWindowSwapchain->GetWindow().DisplayMode(foray::osi::EDisplayMode::WindowedResizable);

        mScene.New(&mContext);

        foray::gltf::ModelConverter converter(mScene.Get());

        foray::gltf::ModelConverterOptions options{.FlipY = !INVERT_BLIT_INSTEAD};

        converter.LoadGltfModel(SCENE_FILE, nullptr, options);

        mScene->UpdateTlasManager();
        mScene->UseDefaultCamera(INVERT_BLIT_INSTEAD);
        mScene->UpdateLightManager();

        mRtStage.New(&mContext, mScene.Get(), mWindowSwapchain.Get());
        mRtStage->SetResizeOrder(1);
        mSwapCopyStage.New(&mContext, mRtStage->GetRtOutput());

        if constexpr(INVERT_BLIT_INSTEAD)
        {
            mSwapCopyStage->SetFlipY(true);
        }
    }

    void ComplexRaytracerApp::ApiRender(foray::base::FrameRenderInfo& renderInfo)
    {
        foray::core::DeviceSyncCommandBuffer& cmdBuffer = renderInfo.GetPrimaryCommandBuffer();
        cmdBuffer.Begin();
        renderInfo.GetInFlightFrame()->ClearSwapchainImage(cmdBuffer, renderInfo.GetImageLayoutCache());
        mScene->Update(cmdBuffer, renderInfo, mWindowSwapchain.Get());
        mRtStage->RecordFrame(cmdBuffer, renderInfo);
        mSwapCopyStage->RecordFrame(cmdBuffer, renderInfo);
        renderInfo.GetInFlightFrame()->PrepareSwapchainImageForPresent(cmdBuffer, renderInfo.GetImageLayoutCache());
        cmdBuffer.Submit();
    }

    void ComplexRaytracerApp::ApiDestroy()
    {
        mRtStage.Delete();
        mSwapCopyStage.Delete();
        mScene = nullptr;
    }
}  // namespace complex_raytracer
