#include "gbufferdemo.hpp"
#include <imgui/imgui.h>
#include <nameof/nameof.hpp>
#include <scene/globalcomponents/foray_drawmanager.hpp>
#include <scene/globalcomponents/foray_materialmanager.hpp>
#include <scene/components/foray_freecameracontroller.hpp>

namespace gbuffer {
    void GBufferDemoApp::ApiBeforeInstanceCreate(vkb::InstanceBuilder& instanceBuilder)
    {
    }
    void GBufferDemoApp::ApiInit()
    {
        {  // Load Scene
            mScene = std::make_unique<foray::scene::Scene>(&mContext);

            foray::gltf::ModelConverter converter(mScene.get());

            converter.LoadGltfModel(SCENE_FILE);

            mScene->UseDefaultCamera(true);
        }

        {  // Init and register render stages
            int32_t resizeOrder = 0;

            mGBufferStage.EnableBuiltInFeature(foray::stages::ConfigurableRasterStage::BuiltInFeaturesFlagBits::ALPHATEST);
            mGBufferStage.AddOutput(NAMEOF_ENUM(EOutput::Position), foray::stages::ConfigurableRasterStage::Templates::WorldPos);
            mGBufferStage.AddOutput(NAMEOF_ENUM(EOutput::Normal), foray::stages::ConfigurableRasterStage::Templates::WorldNormal);
            mGBufferStage.AddOutput(NAMEOF_ENUM(EOutput::Albedo), foray::stages::ConfigurableRasterStage::Templates::Albedo);
            mGBufferStage.AddOutput(NAMEOF_ENUM(EOutput::Motion), foray::stages::ConfigurableRasterStage::Templates::ScreenMotion);
            mGBufferStage.AddOutput(NAMEOF_ENUM(EOutput::MaterialIdx), foray::stages::ConfigurableRasterStage::Templates::MaterialId);
            mGBufferStage.AddOutput(NAMEOF_ENUM(EOutput::MeshInstanceIdx), foray::stages::ConfigurableRasterStage::Templates::MeshInstanceId);
            mGBufferStage.AddOutput(NAMEOF_ENUM(EOutput::LinearZ), foray::stages::ConfigurableRasterStage::Templates::DepthAndDerivative);

            mGBufferStage.Build(&mContext, mScene.get(), &mWindowSwapchain, resizeOrder++, "GBuffer Stage");

            mComparerStage.Init(&mContext, &mWindowSwapchain, true, resizeOrder++);
            SetView(0, EOutput::Albedo);
            SetView(1, EOutput::Position);
            mComparerStage.SetMixValue(0.75);

            mSwapCopyStage.Init(&mContext, mComparerStage.GetImageOutput(mComparerStage.OutputName));
            mSwapCopyStage.SetFlipY(true);
            mImguiStage.InitForSwapchain(&mContext, resizeOrder++);
            mImguiStage.AddWindowDraw([this]() { this->HandleImGui(); });
            mImguiStage.AddWindowDraw(&foray::scene::ncomp::FreeCameraController::RenderImguiHelpWindow);
        }
    }
    void GBufferDemoApp::ApiRender(foray::base::FrameRenderInfo& renderInfo)
    {
        // Set the comparer stage inputs if they have changed
        for(int32_t i = 0; i < 2; i++)
        {
            if(mViewChanged[i])
            {
                SetView(i, mView[i]);
            }
        }

        foray::core::DeviceSyncCommandBuffer& cmdBuffer = renderInfo.GetPrimaryCommandBuffer();
        cmdBuffer.Begin();

        // Update the scene. This updates the scenegraph for animated objects, updates camera matrices, etc.
        mScene->Update(cmdBuffer, renderInfo, &mWindowSwapchain);

        int32_t updateOrder = 1;

        // Record the rasterized GBuffer stage
        mGBufferStage.RecordFrame(cmdBuffer, renderInfo);
        mGBufferStage.SetResizeOrder(updateOrder++);

        // Record the comparer stage (displays any format to the screen)
        mComparerStage.RecordFrame(cmdBuffer, renderInfo);
        mComparerStage.SetResizeOrder(updateOrder++);

        // Copy comparer stage output to the swapchain
        mSwapCopyStage.RecordFrame(cmdBuffer, renderInfo);
        // SwapCopyStage doesn't handle any events, order doesn't matter

        // Record ImGui
        mImguiStage.RecordFrame(cmdBuffer, renderInfo);
        mImguiStage.SetResizeOrder(updateOrder++);

        // Prepare for present
        renderInfo.PrepareSwapchainImageForPresent(cmdBuffer);

        cmdBuffer.Submit();
    }

    void GBufferDemoApp::SetView(int32_t index, EOutput view)
    {
        // since this changes descriptorsets, we need to make sure any GPU work is finished and halted
        foray::AssertVkResult(mContext.DispatchTable().deviceWaitIdle());

        mView[index]        = view;
        mViewChanged[index] = false;

        foray::core::ManagedImage* image = nullptr;

        if(view != EOutput::Depth)
        {
            image = mGBufferStage.GetImageOutput(NAMEOF_ENUM(view));
        }
        else
        {
            image = mGBufferStage.GetDepthImage();
        }

        foray::stages::ComparerStage::InputInfo input{.Image        = image,
                                                      .ChannelCount = 4,
                                                      .Scale        = glm::vec4(1.f),
                                                      .Aspect       = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                                                      .Type         = foray::stages::ComparerStage::EInputType::Float};

        // Set channel count, type and scale properly
        switch(view)
        {
            case EOutput::Motion:
                input.ChannelCount = 2;  // UV X,Y
                break;
            case EOutput::LinearZ:
                input.ChannelCount = 2;  // UV X,Y
                break;
            case EOutput::MaterialIdx: {
                foray::scene::gcomp::MaterialManager* matBuffer = mScene->GetComponent<foray::scene::gcomp::MaterialManager>();
                input.ChannelCount                              = 1;
                float scale                                     = 1 / std::max<float>(1.f, (float)matBuffer->GetVector().size());
                input.Scale                                     = glm::vec4(scale, 0.f, 0.f, 1.f);
                input.Type                                      = foray::stages::ComparerStage::EInputType::Int;
                break;
            }
            case EOutput::MeshInstanceIdx: {
                foray::scene::gcomp::DrawDirector* drawDirector = mScene->GetComponent<foray::scene::gcomp::DrawDirector>();
                input.ChannelCount                              = 1;
                float scale                                     = 1 / std::max<float>(1.f, (float)drawDirector->GetTotalCount());
                input.Scale                                     = glm::vec4(scale, 0.f, 0.f, 1.f);
                input.Type                                      = foray::stages::ComparerStage::EInputType::Int;
                break;
            }
            case EOutput::Depth:
                input.ChannelCount = 1;
                input.Aspect       = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
                break;
            default:
                break;
        }

        mComparerStage.SetInput(index, input);
    }

    void GBufferDemoApp::HandleImGui()
    {
        using namespace foray::stages;
        const EOutput OUTPUTS[] = {EOutput::Position,    EOutput::Normal,          EOutput::Albedo,  EOutput::Motion,
                                   EOutput::MaterialIdx, EOutput::MeshInstanceIdx, EOutput::LinearZ, EOutput::Depth};

        if(ImGui::Begin("Controls", nullptr))
        {
            if(ImGui::CollapsingHeader("About the GBuffer"))
            {
                ImGui::Indent(3.f);
                ImGui::TextWrapped("%s", GBUFFER_ABOUT.c_str());
                ImGui::Unindent(3.f);
            }
            float mix = mComparerStage.GetMixValue();
            if(ImGui::DragFloat("Mix", &mix, 0.01f, 0.f, 1.f, nullptr, ImGuiSliderFlags_AlwaysClamp))
            {
                mComparerStage.SetMixValue(mix);
            }


            const char* Sides[] = {"Left", "Right"};
            for(int32_t i = 0; i < 2; i++)
            {
                ImGui::Spacing();
                std::string labelName(fmt::format("{} Output", Sides[i]));
                std::string outputName(NAMEOF_ENUM(mView[i]));
                if(ImGui::BeginCombo(labelName.c_str(), outputName.c_str()))
                {
                    EOutput newOutput = mView[i];
                    for(EOutput output : OUTPUTS)
                    {
                        bool        selected = output == mView[i];
                        std::string outputName(NAMEOF_ENUM(output));
                        if(ImGui::Selectable(outputName.c_str(), selected))
                        {
                            newOutput = output;
                        }
                    }
                    if(newOutput != mView[i])
                    {
                        mView[i]        = newOutput;
                        mViewChanged[i] = true;
                    }
                    ImGui::EndCombo();
                }
                std::string labelAbout(fmt::format("About {}", outputName));
                if(ImGui::CollapsingHeader(labelAbout.c_str()))
                {
                    ImGui::Indent(3.f);
                    ImGui::TextWrapped("%s", OUTPUT_ABOUT_TEXTS[(size_t)mView[i]].c_str());
                    ImGui::Unindent(3.f);
                }
            }

            ImGui::Spacing();
            if(ImGui::CollapsingHeader("Pipette"))
            {
                ComparerStage::PipetteValue pipette = mComparerStage.GetPipetteValue();
                ImGui::LabelText("Value", "( %f, %f, %f, %f )", pipette.Value.x, pipette.Value.y, pipette.Value.z, pipette.Value.w);
                ImGui::LabelText("Texture UV", "(%f, %f )", pipette.UvPos.x, pipette.UvPos.y);
                ImGui::LabelText("Mouse Pos", "( %i, %i )", pipette.TexelPos.x, pipette.TexelPos.y);
            }

            ImGui::End();
        }
    }

    void GBufferDemoApp::ApiDestroy()
    {
        mScene = nullptr;  // The unique ptr will call destructor upon assigning a nullptr value
        mGBufferStage.Destroy();
        mComparerStage.Destroy();
        mImguiStage.Destroy();
        mSwapCopyStage.Destroy();
    }
}  // namespace gbuffer
