#include "foray_complexrtapp.hpp"
#include <gltf/foray_modelconverter.hpp>
#include <scene/globalcomponents/foray_lightmanager.hpp>
#include <scene/components/foray_freecameracontroller.hpp>

namespace complex_raytracer {
    void ComplexRaytracingStage::Init(foray::core::Context* context, foray::scene::Scene* scene)
    {
        mLightManager = scene->GetComponent<foray::scene::gcomp::LightManager>();
        foray::stages::ExtRaytracingStage::Init(context, scene);
    }

    void ComplexRaytracingStage::CreateRtPipeline()
    {
        foray::core::ShaderCompilerConfig options{.IncludeDirs = {FORAY_SHADER_DIR}};

        mShaderKeys.push_back(mContext->ShaderMan->CompileShader(RAYGEN_FILE, mRaygen, options));
        mShaderKeys.push_back(mContext->ShaderMan->CompileShader(CLOSESTHIT_FILE, mClosestHit, options));
        mShaderKeys.push_back(mContext->ShaderMan->CompileShader(ANYHIT_FILE,mAnyHit, options));
        mShaderKeys.push_back(mContext->ShaderMan->CompileShader(MISS_FILE, mMiss, options));
        mShaderKeys.push_back(mContext->ShaderMan->CompileShader(VISI_MISS_FILE, mVisiMiss, options));
        mShaderKeys.push_back(mContext->ShaderMan->CompileShader(VISI_ANYHIT_FILE, mVisiAnyHit, options));

        mPipeline.GetRaygenSbt().SetGroup(0, &mRaygen);
        mPipeline.GetHitSbt().SetGroup(0, &mClosestHit, &mAnyHit, nullptr);
        mPipeline.GetHitSbt().SetGroup(1, nullptr, &mVisiAnyHit, nullptr);
        mPipeline.GetMissSbt().SetGroup(0, &mMiss);
        mPipeline.GetMissSbt().SetGroup(1, &mVisiMiss);
        mPipeline.Build(mContext, mPipelineLayout);
    }

    void ComplexRaytracingStage::DestroyRtPipeline()
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

        mDescriptorSet.SetDescriptorAt(bindpoint_lights, mLightManager->GetBuffer().GetVkDescriptorInfo(), VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, RTSTAGEFLAGS);

        foray::stages::ExtRaytracingStage::CreateOrUpdateDescriptors();
    }

    void ComplexRaytracerApp::ApiBeforeInit()
    {
        mInstance.SetEnableDebugReport(false);
    }
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

        foray::scene::gcomp::LightManager* lightManager = mScene->GetComponent<foray::scene::gcomp::LightManager>();

        mRtStage.Init(&mContext, mScene.get());
        mSwapCopyStage.Init(&mContext, mRtStage.GetRtOutput());

        if constexpr(INVERT_BLIT_INSTEAD)
        {
            mSwapCopyStage.SetFlipY(true);
        }

        RegisterRenderStage(&mRtStage);
        RegisterRenderStage(&mSwapCopyStage);
    }

    void ComplexRaytracerApp::ApiOnEvent(const foray::osi::Event* event)
    {
        mScene->InvokeOnEvent(event);
    }

    void ComplexRaytracerApp::ApiOnResized(VkExtent2D size)
    {
        mScene->InvokeOnResized(size);
    }

    void ComplexRaytracerApp::ApiRender(foray::base::FrameRenderInfo& renderInfo)
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

    void ComplexRaytracerApp::ApiDestroy()
    {
        mRtStage.Destroy();
        mSwapCopyStage.Destroy();
        mScene = nullptr;
    }
}  // namespace complex_raytracer
