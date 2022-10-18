#include "foray_minimalrtapp.hpp"
#include <core/foray_shadermanager.hpp>
#include <gltf/foray_modelconverter.hpp>
#include <scene/components/foray_camera.hpp>
#include <scene/components/foray_freecameracontroller.hpp>
#include <scene/globalcomponents/foray_cameramanager.hpp>
#include <scene/globalcomponents/foray_tlasmanager.hpp>

namespace foray::minimal_raytracer {
    void MinimalRaytracingStage::Init(const foray::core::VkContext* context, foray::scene::Scene* scene)
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

    void MinimalRaytracerApp::Init()
    {
        mScene = std::make_unique<foray::scene::Scene>(&mContext);

        gltf::ModelConverter converter(mScene.get());

        converter.LoadGltfModel(SCENE_FILE);

        mScene->MakeComponent<foray::scene::TlasManager>(&mContext)->CreateOrUpdate();

        auto cameraNode = mScene->MakeNode();

        cameraNode->MakeComponent<foray::scene::Camera>()->InitDefault();
        cameraNode->MakeComponent<foray::scene::FreeCameraController>();
        mScene->GetComponent<foray::scene::CameraManager>()->RefreshCameraList();

        mRtStage.Init(&mContext, mScene.get());
        mSwapCopyStage.Init(&mContext, mRtStage.GetColorAttachmentByName(stages::RaytracingStage::RaytracingRenderTargetName),
                            foray::stages::ImageToSwapchainStage::PostCopy{.AccessFlags      = (VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT),
                                                                           .ImageLayout      = (VkImageLayout::VK_IMAGE_LAYOUT_GENERAL),
                                                                           .QueueFamilyIndex = (mContext.QueueGraphics)});
    }

    void MinimalRaytracerApp::OnEvent(const foray::Event* event)
    {
        mScene->InvokeOnEvent(event);
    }

    void MinimalRaytracerApp::RecordCommandBuffer(foray::base::FrameRenderInfo& renderInfo)
    {
        core::DeviceCommandBuffer& cmdBuffer = renderInfo.GetPrimaryCommandBuffer();
        cmdBuffer.Begin();
        mScene->Update(renderInfo, cmdBuffer);
        mRtStage.RecordFrame(cmdBuffer, renderInfo);
        mSwapCopyStage.RecordFrame(cmdBuffer, renderInfo);
        renderInfo.GetInFlightFrame()->PrepareSwapchainImageForPresent();
        cmdBuffer.Submit(mContext.QueueGraphics);
    }

    void MinimalRaytracerApp::OnResized(VkExtent2D size)
    {
        mRtStage.OnResized(size);
        mSwapCopyStage.OnResized(size, mRtStage.GetColorAttachmentByName(stages::RaytracingStage::RaytracingRenderTargetName));
    }

    void MinimalRaytracerApp::OnShadersRecompiled()
    {
        mRtStage.OnShadersRecompiled();
    }

    void MinimalRaytracerApp::Destroy()
    {
        vkDeviceWaitIdle(mContext.Device);
        mRtStage.Destroy();
        mSwapCopyStage.Destroy();
        mScene = nullptr;
        DefaultAppBase::Destroy();
    }
}  // namespace foray::minimal_raytracer
