#include "app.hpp"

namespace example{
    void App::ApiInit() {

	}

    void App::ApiRender(foray::base::FrameRenderInfo& renderInfo)
    {
        foray::core::DeviceSyncCommandBuffer& cmdBuffer = renderInfo.GetPrimaryCommandBuffer();
        cmdBuffer.Begin();
	
		// render..
		//	...	instead of clearing, write your rendercode here
		renderInfo.ClearSwapchainImage(cmdBuffer);

        // wait for all writes to swapchain image to finish
        renderInfo.PrepareSwapchainImageForPresent(cmdBuffer);

        // Submit() ends and submits the command buffer
        cmdBuffer.Submit();
	}

	App::~App() {
        mDevice->GetDispatchTable().deviceWaitIdle();
	}
};