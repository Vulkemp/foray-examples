#include "gbufferdemo.hpp"
#include <imgui/imgui.h>
#include <nameof/nameof.hpp>
#include <scene/globalcomponents/foray_drawmanager.hpp>
#include <scene/globalcomponents/foray_materialmanager.hpp>

namespace gbuffer {
    void GBufferDemoApp::ApiInit()
    {
        {  // Load Scene
            mScene = std::make_unique<foray::scene::Scene>(&mContext);

            foray::gltf::ModelConverter converter(mScene.get());

            converter.LoadGltfModel(SCENE_FILE);

            mScene->UseDefaultCamera();
        }

        {  // Init and register render stages

            mGBufferStage.Init(&mContext, mScene.get());

            mComparerStage.Init(&mContext);
            SetView(0, foray::stages::GBufferStage::EOutput::Albedo);
            SetView(1, foray::stages::GBufferStage::EOutput::Position);
            mComparerStage.SetMixValue(0.75);

            mImguiStage.AddWindowDraw([this]() { this->HandleImGui(); });
            mImguiStage.Init(&mContext, mComparerStage.GetImageOutput(mComparerStage.OutputName));
            mSwapCopyStage.Init(&mContext, mComparerStage.GetImageOutput(mComparerStage.OutputName));

            RegisterRenderStage(&mGBufferStage);
            RegisterRenderStage(&mComparerStage);
            RegisterRenderStage(&mImguiStage);
            RegisterRenderStage(&mSwapCopyStage);
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

        foray::core::DeviceCommandBuffer& cmdBuffer = renderInfo.GetPrimaryCommandBuffer();
        cmdBuffer.Begin();

        // Update the scene. This updates the scenegraph for animated objects, updates camera matrices, etc.
        mScene->Update(renderInfo, cmdBuffer);

        // Record the rasterized GBuffer stage
        mGBufferStage.RecordFrame(cmdBuffer, renderInfo);

        // Record the comparer stage (displays any format to the screen)
        mComparerStage.RecordFrame(cmdBuffer, renderInfo);

        // Record ImGui
        mImguiStage.RecordFrame(cmdBuffer, renderInfo);

        // Copy comparer stage output to the swapchain
        mSwapCopyStage.RecordFrame(cmdBuffer, renderInfo);

        // Prepare for present
        renderInfo.PrepareSwapchainImageForPresent(cmdBuffer);

        cmdBuffer.Submit();
    }
    void GBufferDemoApp::ApiOnEvent(const foray::osi::Event* event)
    {
        // Scene event handling includes moving the camera around
        mScene->HandleEvent(event);
        // The comparer stage handles events to capture current mouse position
        mComparerStage.HandleEvent(event);
        // The imgui stage handles events to animate the UI
        mImguiStage.ProcessSdlEvent(&(event->RawSdlEventData));
    }

    void GBufferDemoApp::SetView(int32_t index, foray::stages::GBufferStage::EOutput view)
    {
        // since this changes descriptorsets, we need to make sure any GPU work is finished and halted
        foray::AssertVkResult(mContext.VkbDispatchTable->deviceWaitIdle());

        mView[index]        = view;
        mViewChanged[index] = false;

        foray::stages::ComparerStage::InputInfo input{.Image        = mGBufferStage.GetImageEOutput(view),
                                                      .ChannelCount = 4,
                                                      .Scale        = glm::vec4(1.f),
                                                      .Aspect       = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                                                      .Type         = foray::stages::ComparerStage::EInputType::Float};

        // Set channel count, type and scale properly
        switch(view)
        {
            case foray::stages::GBufferStage::EOutput::Motion:
                input.ChannelCount = 2;  // UV X,Y
                break;
            case foray::stages::GBufferStage::EOutput::LinearZ:
                input.ChannelCount = 2;  // UV X,Y
                break;
            case foray::stages::GBufferStage::EOutput::MaterialIdx: {
                foray::scene::gcomp::MaterialManager* matBuffer = mScene->GetComponent<foray::scene::gcomp::MaterialManager>();
                input.ChannelCount                              = 1;
                float scale                                     = 1 / std::max<float>(1.f, (float)matBuffer->GetVector().size());
                input.Scale                                     = glm::vec4(scale, 0.f, 0.f, 1.f);
                input.Type                                      = foray::stages::ComparerStage::EInputType::Int;
                break;
            }
            case foray::stages::GBufferStage::EOutput::MeshInstanceIdx: {
                foray::scene::gcomp::DrawDirector* drawDirector = mScene->GetComponent<foray::scene::gcomp::DrawDirector>();
                input.ChannelCount                              = 1;
                float scale                                     = 1 / std::max<float>(1.f, (float)drawDirector->GetTotalCount());
                input.Scale                                     = glm::vec4(scale, 0.f, 0.f, 1.f);
                input.Type                                      = foray::stages::ComparerStage::EInputType::Uint;
                break;
            }
            case foray::stages::GBufferStage::EOutput::Depth:
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
        const GBufferStage::EOutput OUTPUTS[] = {GBufferStage::EOutput::Position, GBufferStage::EOutput::Normal,      GBufferStage::EOutput::Albedo,
                                                 GBufferStage::EOutput::Motion,   GBufferStage::EOutput::MaterialIdx, GBufferStage::EOutput::MeshInstanceIdx,
                                                 GBufferStage::EOutput::LinearZ,  GBufferStage::EOutput::Depth};

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
                    GBufferStage::EOutput newOutput = mView[i];
                    for(GBufferStage::EOutput output : OUTPUTS)
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
