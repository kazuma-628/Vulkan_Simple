//-------------------------------------------------------------------------------------------------
// File : SampleApp.cpp
// Desc : Sample Application.
// Copyright(c) Project Asura. All right reserved.
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------------------------------
#include "SampleApp.h"
#include <asvkLogger.h>
#include <array>


namespace /* anonymous */ {

//-------------------------------------------------------------------------------------------------
//      メモリ確保処理.
//-------------------------------------------------------------------------------------------------
VKAPI_ATTR
void* VKAPI_CALL Alloc
(
    void* pUserData,
    size_t size,
    size_t alignment,
    VkSystemAllocationScope scope
)
{
    ASVK_UNUSED(pUserData);
    ASVK_UNUSED(scope);
    return _aligned_malloc(size, alignment);
}

//-------------------------------------------------------------------------------------------------
//      メモリ再確保処理.
//-------------------------------------------------------------------------------------------------
VKAPI_ATTR 
void* VKAPI_CALL Realloc
(
    void* pUserData,
    void* pOriginal,
    size_t size,
    size_t alignment,
    VkSystemAllocationScope scope
)
{
    ASVK_UNUSED(pUserData);
    ASVK_UNUSED(scope);
    return _aligned_realloc(pOriginal, size, alignment);
}

//-------------------------------------------------------------------------------------------------
//      メモリ解放処理.
//-------------------------------------------------------------------------------------------------
VKAPI_ATTR
void VKAPI_CALL Free(void* pUserData, void* pMemory)
{
    ASVK_UNUSED(pUserData);
    _aligned_free( pMemory );
}

//-------------------------------------------------------------------------------------------------
//      イメージレイアウトを設定します.
//-------------------------------------------------------------------------------------------------
void SetImageLayout
(
    VkDevice            device,
    VkCommandBuffer     commandBuffer,
    VkImage             image, 
    VkImageAspectFlags  aspectFlags, 
    VkImageLayout       oldLayout,
    VkImageLayout       newLayout
)
{
    assert(device != nullptr);
    assert(commandBuffer != nullptr);

    VkImageMemoryBarrier barrier = {};
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext               = nullptr;
    barrier.srcAccessMask       = 0;
    barrier.dstAccessMask       = 0;
    barrier.oldLayout           = oldLayout;
    barrier.newLayout           = newLayout;
    barrier.image               = image;
    barrier.subresourceRange    = {aspectFlags, 0, 1, 0, 1};

    if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    { barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT; }

    if (newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    { barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; }

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    { barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; }

    if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    { barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT; }

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);
}

} // namespace /* anonymous */


///////////////////////////////////////////////////////////////////////////////////////////////////
// SampleApp class
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//      コンストラクタです.
//-------------------------------------------------------------------------------------------------
SampleApp::SampleApp()
: asvk::App( L"SampleApp", 960, 540, nullptr, nullptr, nullptr )
, m_Instance            (nullptr)
, m_Device              (nullptr)
, m_GraphicsQueue       (nullptr)
, m_GraphicsFence       (nullptr)
, m_GraphicsSemaphore   (nullptr)
, m_GraphicsFamilyIndex (0)
, m_Surface             (nullptr)
, m_SwapChain           (nullptr)
, m_BufferIndex         (0)
, m_CommandPool         (nullptr)
{ /* DO_NOTHING */ }

//-------------------------------------------------------------------------------------------------
//      デストラクタです.
//-------------------------------------------------------------------------------------------------
SampleApp::~SampleApp()
{ /* DO_NOTHING */ }

//-------------------------------------------------------------------------------------------------
//      初期化時の処理です.
//-------------------------------------------------------------------------------------------------
bool SampleApp::OnInit()
{
    // インスタンスの生成.
    {
        std::array<const char*, 2> extensions;
        extensions[0] = VK_KHR_SURFACE_EXTENSION_NAME;
        extensions[1] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
 
        VkApplicationInfo appInfo = {};
        appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pNext              = nullptr;
        appInfo.pApplicationName   = "SampleApp";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName        = "asvk";
        appInfo.engineVersion      = ASVK_CURRENT_VERSION_NUMBER;
        appInfo.apiVersion         = VK_API_VERSION;

        VkInstanceCreateInfo instanceInfo = {};
        instanceInfo.sType                      = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceInfo.pNext                      = nullptr;
        instanceInfo.flags                      = 0;
        instanceInfo.pApplicationInfo           = &appInfo;
        instanceInfo.enabledLayerCount          = 0;
        instanceInfo.ppEnabledLayerNames        = nullptr;
        instanceInfo.enabledExtensionCount      = static_cast<uint32_t>(extensions.size());
        instanceInfo.ppEnabledExtensionNames    = extensions.data();

        m_AllocatorCallbacks.pfnAllocation         = Alloc;
        m_AllocatorCallbacks.pfnFree               = Free;
        m_AllocatorCallbacks.pfnReallocation       = Realloc;
        m_AllocatorCallbacks.pfnInternalAllocation = nullptr;
        m_AllocatorCallbacks.pfnInternalFree       = nullptr;

        auto result = vkCreateInstance(&instanceInfo, &m_AllocatorCallbacks, &m_Instance);
        if ( result != VK_SUCCESS )
        {
            ELOG( "Error : vkCreatInstance() Failed." );
            return false;
        }
    }

    // 物理デバイスの取得.
    {
        uint32_t count = 0;
        auto result = vkEnumeratePhysicalDevices(m_Instance, &count, nullptr);
        if ( result != VK_SUCCESS || count < 1 )
        {
            ELOG( "Error : vkEnumeratePhysicalDevices() Failed." );
            return false;
        }

        std::vector<VkPhysicalDevice> physicalDevices;
        physicalDevices.resize(count);

        result = vkEnumeratePhysicalDevices(m_Instance, &count, physicalDevices.data());
        if ( result != VK_SUCCESS )
        {
            ELOG( "Error : vkEnumeratePhysicalDevices() Failed." );
            return false;
        }

        m_Gpus.resize(count);
        for(auto i=0u; i<count; ++i)
        {
            m_Gpus[i].Device = physicalDevices[i];
            vkGetPhysicalDeviceMemoryProperties(m_Gpus[i].Device, &m_Gpus[i].MemoryProps);
        }

        physicalDevices.clear();
    }

    // デバイスとキューの生成.
    {
        uint32_t propCount;
        vkGetPhysicalDeviceQueueFamilyProperties(m_Gpus[0].Device, &propCount, nullptr);

        std::vector<VkQueueFamilyProperties> props;
        props.resize(propCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_Gpus[0].Device, &propCount, props.data());

        for(auto i=0u; i<propCount; ++i)
        {
            if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            { m_GraphicsFamilyIndex = i; }
        }

        VkDeviceQueueCreateInfo queueInfo;
        float queuePriorities[] = { 0.0f, 0.0f };
        queueInfo.sType              = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.pNext              = nullptr;
        queueInfo.queueCount         = 1;
        queueInfo.queueFamilyIndex   = m_GraphicsFamilyIndex;
        queueInfo.pQueuePriorities   = queuePriorities;

        VkDeviceCreateInfo deviceInfo = {};
        deviceInfo.sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.pNext                    = nullptr;
        deviceInfo.queueCreateInfoCount     = 1;
        deviceInfo.pQueueCreateInfos        = &queueInfo;
        deviceInfo.enabledLayerCount        = 0;
        deviceInfo.ppEnabledLayerNames      = nullptr;
        deviceInfo.enabledExtensionCount    = 0;
        deviceInfo.ppEnabledExtensionNames  = nullptr;
        deviceInfo.pEnabledFeatures         = nullptr;

        auto result = vkCreateDevice(m_Gpus[0].Device, &deviceInfo, nullptr, &m_Device);
        if ( result != VK_SUCCESS )
        {
            ELOG( "Error : vkCreateDevice() Failed." );
            return false;
        }

        vkGetDeviceQueue(m_Device, m_GraphicsFamilyIndex, 0, &m_GraphicsQueue);

        props.clear();
    }

    // フェンスの生成.
    {
        VkFenceCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;
        auto result = vkCreateFence(m_Device, &info, nullptr, &m_GraphicsFence);
        if ( result != VK_SUCCESS )
        {
            ELOG( "Error : vkCreateFence() Failed." );
            return false;
        }
    }

    // セマフォの生成.
    {
        VkSemaphoreCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;

        auto result = vkCreateSemaphore(m_Device, &info, nullptr, &m_GraphicsSemaphore);
        if ( result != VK_SUCCESS )
        {
            ELOG( "Error : vkCreatSemaphore() Failed." );
            return false;
        }
    }

    // コマンドプールの生成.
    {
        VkCommandPoolCreateInfo info = {};
        info.sType              = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.pNext              = nullptr;
        info.queueFamilyIndex   = m_GraphicsFamilyIndex;
        info.flags              = 0;

        auto result = vkCreateCommandPool(m_Device, &info, nullptr, &m_CommandPool);
        if ( result != VK_SUCCESS )
        {
            ELOG( "Error : vkCreateCommandPool() Failed." );
            return false;
        }
    }

    // コマンドバッファの生成.
    {
        // メモリを確保.
        m_CommandBuffers.resize(SwapChainCount);

        VkCommandBufferAllocateInfo info = {};
        info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.pNext              = nullptr;
        info.commandPool        = m_CommandPool;
        info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandBufferCount = SwapChainCount;
       
        auto result = vkAllocateCommandBuffers(m_Device, &info, m_CommandBuffers.data());
        if ( result != VK_SUCCESS )
        {
            ELOG( "Error : vkAllocateCommandBuffers() Failed." );
            return false;
        }

        VkCommandBufferInheritanceInfo inheritanceInfo = {};
        inheritanceInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritanceInfo.pNext                   = nullptr;
        inheritanceInfo.renderPass              = nullptr;
        inheritanceInfo.subpass                 = 0;
        inheritanceInfo.framebuffer             = nullptr;
        inheritanceInfo.occlusionQueryEnable    = VK_FALSE;
        inheritanceInfo.queryFlags              = 0;
        inheritanceInfo.pipelineStatistics      = 0;

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType             = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.pNext             = nullptr;
        beginInfo.flags             = 0;
        beginInfo.pInheritanceInfo  = &inheritanceInfo;

        result = vkBeginCommandBuffer(m_CommandBuffers[m_BufferIndex], &beginInfo);
        if (result != VK_SUCCESS)
        {
            ELOG( "Error : vkBeginCommandBuffer() Failed." );
            return false;
        }
    }

    // サーフェイスの生成.
    {
        VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
        surfaceInfo.sType       = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceInfo.pNext       = nullptr;
        surfaceInfo.flags       = 0;
        surfaceInfo.hinstance   = m_hInst;
        surfaceInfo.hwnd        = m_hWnd;

        auto result = vkCreateWin32SurfaceKHR(m_Instance, &surfaceInfo, nullptr, &m_Surface);
        if ( result != VK_SUCCESS )
        {
            ELOG( "Error : vkCreateWin32SurfaceKHR() Failed." );
            return false;
        }
    }

    // スワップチェインの生成.
    {
        uint32_t count  = 0;
        auto result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_Gpus[0].Device, m_Surface, &count, nullptr);
        if ( result != VK_SUCCESS )
        {
            ELOG( "Error : vkGetPhysicalDeviceSurfaceFormatKHR() Failed." );
            return false;
        }

        std::vector<VkSurfaceFormatKHR> formats;
        formats.resize(count);
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_Gpus[0].Device, m_Surface, &count, formats.data());
        if ( result != VK_SUCCESS )
        {
            ELOG( "Error : vkGetPhysicalDeviceSUrfaceFormatsKHR() Failed." );
            return false;
        }

        VkFormat        imageFormat     = VK_FORMAT_R8G8B8A8_UNORM;
        VkColorSpaceKHR imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

        bool isFind = false;
        for(size_t i=0; i<formats.size(); ++i)
        {
            if (imageFormat     == formats[i].format &&
                imageColorSpace == formats[i].colorSpace)
            { 
                isFind = true;
                break;
            }
        }

        if (!isFind)
        {
            imageFormat     = formats[0].format;
            imageColorSpace = formats[0].colorSpace;
        }

        VkSurfaceCapabilitiesKHR capabilities;
        VkSurfaceTransformFlagBitsKHR preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        {
            auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                m_Gpus[0].Device,
                m_Surface,
                &capabilities);
            if ( result != VK_SUCCESS )
            {
                ELOG( "Error : vkGetPhysicalDeviceSurfaceCapabilitiesKHR() Failed.");
                return false;
            }

            if (!(capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR))
            { preTransform = capabilities.currentTransform; }
        }

        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
        {
            uint32_t presentModeCount;
            result = vkGetPhysicalDeviceSurfacePresentModesKHR(
                m_Gpus[0].Device,
                m_Surface,
                &presentModeCount,
                nullptr);
            if ( result != VK_SUCCESS )
            {
                ELOG( "Error : vkGetPhysicalDeviceSurfacePresentModesKHR() Failed." );
                return false;
            }

            std::vector<VkPresentModeKHR> presentModes;
            presentModes.resize(presentModeCount);
            result = vkGetPhysicalDeviceSurfacePresentModesKHR(
                m_Gpus[0].Device,
                m_Surface,
                &presentModeCount,
                presentModes.data());
            if ( result != VK_SUCCESS )
            {
                ELOG( "Error : vkGetPhysicalDeviceSurfacePresentModesKHR() Failed." );
                return false;
            }

            for(size_t i=0; i<presentModes.size(); ++i)
            {
                if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                    break;
                }
                if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
                {
                    presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                }
            }

            presentModes.clear();
        }

        uint32_t desiredSwapChainImageCount = capabilities.minImageCount + 1;
        if ((capabilities.maxImageCount > 0) && (desiredSwapChainImageCount > capabilities.maxImageCount))
        { desiredSwapChainImageCount = capabilities.maxImageCount; }

        {
            VkSwapchainCreateInfoKHR createInfo = {};
            createInfo.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.pNext                    = nullptr;
            createInfo.flags                    = 0;
            createInfo.surface                  = m_Surface;
            createInfo.minImageCount            = desiredSwapChainImageCount;
            createInfo.imageFormat              = imageFormat;
            createInfo.imageColorSpace          = imageColorSpace;
            createInfo.imageExtent              = { m_Width, m_Height };
            createInfo.imageArrayLayers         = 1;
            createInfo.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            createInfo.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount    = 0;
            createInfo.pQueueFamilyIndices      = nullptr;
            createInfo.preTransform             = preTransform;
            createInfo.compositeAlpha           = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            createInfo.presentMode              = presentMode;
            createInfo.clipped                  = VK_TRUE;
            createInfo.oldSwapchain             = nullptr;

            auto result = vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_SwapChain);
            if ( result != VK_SUCCESS )
            {
                ELOG( "Error : vkCreateSwapChainKHR() Failed." );
                return false;
            }
        }
    }

    // イメージの作成
    {
        uint32_t swapChainCount = 0;
        auto result = vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &swapChainCount, nullptr);
        if ( result != VK_SUCCESS )
        {
            ELOG( "Error : vkGetSwapChainImagesKHR() Failed." );
            return false;
        }

        m_BackBuffers.resize(swapChainCount);

        std::vector<VkImage> images;
        images.resize(swapChainCount);
        result = vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &swapChainCount, images.data());
        if ( result != VK_SUCCESS )
        {
            ELOG( "Error : vkGetSwapChainImagesKHR() Failed." );
            return false;
        }

        for(auto i=0u; i<swapChainCount; ++i)
        { m_BackBuffers[i].Image = images[i]; }

        images.clear();
    }

    // イメージビューの生成.
    {
        for(size_t i=0; i<m_BackBuffers.size(); ++i)
        {
            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.pNext            = nullptr;
            viewInfo.format           = VK_FORMAT_R8G8B8A8_UNORM;
            viewInfo.components.r     = VK_COMPONENT_SWIZZLE_R;
            viewInfo.components.g     = VK_COMPONENT_SWIZZLE_G;
            viewInfo.components.b     = VK_COMPONENT_SWIZZLE_B;
            viewInfo.components.a     = VK_COMPONENT_SWIZZLE_A;
            viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
            viewInfo.flags            = 0;
            viewInfo.image            = m_BackBuffers[i].Image;

            auto result = vkCreateImageView(m_Device, &viewInfo, nullptr, &m_BackBuffers[i].View);
            if ( result != VK_SUCCESS )
            {
                ELOG( "Error : vkCreateImageView() Failed." );
                return false;
            }

            SetImageLayout(
                m_Device,
                m_CommandBuffers[m_BufferIndex],
                m_BackBuffers[i].Image,
                VK_IMAGE_ASPECT_COLOR_BIT, 
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        }
    }

    // 深度ステンシルバッファの生成.
    {
        VkFormat depthFormat = VK_FORMAT_D24_UNORM_S8_UINT;

        VkImageTiling tiling;
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_Gpus[0].Device, depthFormat, &props);

        if (props.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        { tiling = VK_IMAGE_TILING_LINEAR; }
        else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        { tiling = VK_IMAGE_TILING_OPTIMAL; }
        else
        {
            ELOG( "Error : Unsupported Format." );
            return false;
        }

        VkImageCreateInfo info = {};
        info.sType                  = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.pNext                  = nullptr;
        info.imageType              = VK_IMAGE_TYPE_2D;
        info.format                 = depthFormat;
        info.extent.width           = m_Width;
        info.extent.height          = m_Height;
        info.extent.depth           = 1;
        info.mipLevels              = 1;
        info.arrayLayers            = 1;
        info.samples                = VK_SAMPLE_COUNT_1_BIT;
        info.tiling                 = tiling;
        info.initialLayout          = VK_IMAGE_LAYOUT_UNDEFINED;
        info.queueFamilyIndexCount  = 0;
        info.pQueueFamilyIndices    = nullptr;
        info.sharingMode            = VK_SHARING_MODE_EXCLUSIVE;
        info.flags                  = 0;

        auto result = vkCreateImage(m_Device, &info, nullptr, &m_Depth.Image);
        if (result != VK_SUCCESS)
        {
            ELOG( "Error : vkCreateImage() Failed." );
            return false;
        }

        VkMemoryRequirements requirements;
        vkGetImageMemoryRequirements(m_Device, m_Depth.Image, &requirements);

        VkFlags requirementsMask = 0;
        uint32_t typeBits  = requirements.memoryTypeBits;
        uint32_t typeIndex = 0;
        for(auto i=0u; i<32; ++i)
        {
            if ((typeBits & 0x1) == 1)
            {
                if ((m_Gpus[0].MemoryProps.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask)
                {
                    typeIndex = i;
                    break;
                }
            }
            typeBits >>= 1;
        }

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType             = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext             = nullptr;
        allocInfo.allocationSize    = requirements.size;
        allocInfo.memoryTypeIndex   = typeIndex;

        result = vkAllocateMemory(m_Device, &allocInfo, nullptr, &m_Depth.Memory);
        if (result != VK_SUCCESS)
        {
            ELOG( "Error : vkAllocateMemory() Failed." );
            return false;
        }

        result = vkBindImageMemory(m_Device, m_Depth.Image, m_Depth.Memory, 0);
        if (result != VK_SUCCESS)
        {
            ELOG( "Error : vkBindImageMemory() Failed." );
            return false;
        }

        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.pNext                           = nullptr;
        viewInfo.image                           = m_Depth.Image;
        viewInfo.format                          = depthFormat;
        viewInfo.components.r                    = VK_COMPONENT_SWIZZLE_R;
        viewInfo.components.g                    = VK_COMPONENT_SWIZZLE_G;
        viewInfo.components.b                    = VK_COMPONENT_SWIZZLE_B;
        viewInfo.components.a                    = VK_COMPONENT_SWIZZLE_A;
        viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.flags                           = 0;

        result = vkCreateImageView(m_Device, &viewInfo, nullptr, &m_Depth.View);
        if (result != VK_SUCCESS)
        {
            ELOG( "Error : vkCreateImageView() Failed." );
            return false;
        }

        SetImageLayout(
            m_Device,
            m_CommandBuffers[m_BufferIndex],
            m_Depth.Image,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }

    // フレームバッファの生成.
    {
        VkImageView attachments[2];

        VkFramebufferCreateInfo info = {};
        info.sType              = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.pNext              = nullptr;
        info.renderPass         = nullptr;
        info.attachmentCount    = 2;
        info.pAttachments       = attachments;
        info.width              = m_Width;
        info.height             = m_Height;
        info.layers             = 1;

        m_FrameBuffers.resize(SwapChainCount);
        for(auto i=0u; i<SwapChainCount; ++i)
        {
            attachments[0] = m_BackBuffers[i].View;
            attachments[1] = m_Depth.View;
            auto result = vkCreateFramebuffer(m_Device, &info, nullptr, &m_FrameBuffers[i]);
            if ( result != VK_SUCCESS )
            {
                ELOG( "Error : vkCreateFramebuffer() Failed." );
                return false;
            }
        }
    }

    // コマンドを実行しておく.
    {
        auto result = vkEndCommandBuffer(m_CommandBuffers[m_BufferIndex]);
        if ( result != VK_SUCCESS )
        {
            ELOG( "Error : vkEndCommandBuffer() Failed." );
            return false;
        }

        VkPipelineStageFlags pipeStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

        VkSubmitInfo info = {};
        info.sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.pNext                  = nullptr;
        info.waitSemaphoreCount     = 0;
        info.pWaitSemaphores        = nullptr;
        info.pWaitDstStageMask      = &pipeStageFlags;
        info.commandBufferCount     = 1;
        info.pCommandBuffers        = &m_CommandBuffers[m_BufferIndex];
        info.signalSemaphoreCount   = 0;
        info.pSignalSemaphores      = nullptr;

        result = vkQueueSubmit(m_GraphicsQueue, 1, &info, m_GraphicsFence);
        if ( result != VK_SUCCESS )
        {
            ELOG( "Error : vkQueueSubmit() Failed." );
            return false;
        }

        do {
            result = vkWaitForFences(m_Device, 1, &m_GraphicsFence, VK_TRUE, TimeOutNanoSec);
        } while( result == VK_TIMEOUT );

        result = vkQueueWaitIdle(m_GraphicsQueue);
        if ( result != VK_SUCCESS )
        {
            ELOG( "Error : vkQueueWaitIdle() Failed." );
            return false;
        }
    }

    // フレームを用意.
    {
        auto result = vkAcquireNextImageKHR(
            m_Device,
            m_SwapChain,
            UINT64_MAX,
            m_GraphicsSemaphore,
            nullptr, 
            &m_BufferIndex);
        if ( result != VK_SUCCESS )
        {
            ELOG( "Error : vkAcquireNextImageKHR() Failed." );
        }
    }

    return true;
}

//-------------------------------------------------------------------------------------------------
//      終了時の処理です.
//-------------------------------------------------------------------------------------------------
void SampleApp::OnTerm()
{
    for(auto i=0u; i<SwapChainCount; ++i)
    { vkDestroyFramebuffer(m_Device, m_FrameBuffers[i], nullptr); }

    if (m_Depth.View != nullptr)
    { vkDestroyImageView(m_Device, m_Depth.View, nullptr); }

    if (m_Depth.Image != nullptr)
    { vkDestroyImage(m_Device, m_Depth.Image, nullptr); }

    if (m_Depth.Memory != nullptr)
    { vkFreeMemory(m_Device, m_Depth.Memory, nullptr); }

    if (!m_CommandBuffers.empty())
    { vkFreeCommandBuffers(m_Device, m_CommandPool, SwapChainCount, m_CommandBuffers.data()); }

    if (m_CommandPool != nullptr)
    { vkDestroyCommandPool(m_Device, m_CommandPool, nullptr); }

    if (m_GraphicsSemaphore != nullptr)
    { vkDestroySemaphore(m_Device, m_GraphicsSemaphore, nullptr); }

    if (m_GraphicsFence != nullptr)
    { vkDestroyFence(m_Device, m_GraphicsFence, nullptr); }

    if (m_SwapChain != nullptr)
    { vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr); }

    if (m_Surface != nullptr)
    { vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr); }

    if (m_Device != nullptr)
    { vkDestroyDevice(m_Device, nullptr); }

    if (m_Instance != nullptr)
    { vkDestroyInstance(m_Instance, &m_AllocatorCallbacks); }

    m_FrameBuffers  .clear();
    m_Gpus          .clear();
    m_CommandBuffers.clear();

    m_GraphicsFamilyIndex = 0;

    m_Surface           = nullptr;
    m_SwapChain         = nullptr;
    m_CommandPool       = nullptr;
    m_GraphicsSemaphore = nullptr;
    m_GraphicsFence     = nullptr;
    m_GraphicsQueue     = nullptr;
    m_Device            = nullptr;
    m_Instance          = nullptr;
}

//-------------------------------------------------------------------------------------------------
//      描画時の処理です.
//-------------------------------------------------------------------------------------------------
void SampleApp::OnFrameRender(const asvk::FrameEventArgs& args)
{
    ASVK_UNUSED(args);
    auto cmd = m_CommandBuffers[m_BufferIndex];

    // コマンド記録開始.
    {
        VkCommandBufferInheritanceInfo inheritanceInfo = {};
        inheritanceInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritanceInfo.pNext                   = nullptr;
        inheritanceInfo.renderPass              = nullptr;
        inheritanceInfo.subpass                 = 0;
        inheritanceInfo.framebuffer             = m_FrameBuffers[m_BufferIndex];
        inheritanceInfo.occlusionQueryEnable    = VK_FALSE;
        inheritanceInfo.queryFlags              = 0;
        inheritanceInfo.pipelineStatistics      = 0;

        VkCommandBufferBeginInfo cmdBeginInfo = {};
        cmdBeginInfo.sType             = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBeginInfo.pNext             = nullptr;
        cmdBeginInfo.flags             = 0;
        cmdBeginInfo.pInheritanceInfo  = &inheritanceInfo;

        vkBeginCommandBuffer(cmd, &cmdBeginInfo);
    }

    // カラーバッファをクリア.
    {
        VkClearColorValue clearColor;
        clearColor.float32[0] = 0.392156899f;
        clearColor.float32[1] = 0.584313750f;
        clearColor.float32[2] = 0.929411829f;
        clearColor.float32[3] = 1.0f;

        VkImageSubresourceRange range;
        range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel   = 0;
        range.levelCount     = 1;
        range.baseArrayLayer = 0;
        range.layerCount     = 1;

        vkCmdClearColorImage(
            cmd,
            m_BackBuffers[m_BufferIndex].Image,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            &clearColor,
            1,
            &range);
    }

    // 深度バッファをクリア.
    {
        VkClearDepthStencilValue clearDepthStencil;
        clearDepthStencil.depth   = 1.0f;
        clearDepthStencil.stencil = 0;

        VkImageSubresourceRange range;
        range.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        range.baseMipLevel   = 0;
        range.levelCount     = 1;
        range.baseArrayLayer = 0;
        range.layerCount     = 1;

        vkCmdClearDepthStencilImage(
            cmd,
            m_Depth.Image,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            &clearDepthStencil,
            1,
            &range);
    }

    // リソースバリアの設定.
    {
        VkImageMemoryBarrier barrier = {};
        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext                           = nullptr;
        barrier.srcAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask                   = VK_ACCESS_MEMORY_READ_BIT;
        barrier.oldLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.newLayout                       = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;
        barrier.image                           = m_BackBuffers[m_BufferIndex].Image;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);
    }

    // コマンドの記録を終了.
    vkEndCommandBuffer(cmd);

    // コマンドを実行し，表示する.
    {
        VkPipelineStageFlags pipeStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

        VkSubmitInfo info = {};
        info.sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.pNext                  = nullptr;
        info.waitSemaphoreCount     = 0;
        info.pWaitSemaphores        = nullptr;
        info.pWaitDstStageMask      = &pipeStageFlags;
        info.commandBufferCount     = 1;
        info.pCommandBuffers        = &m_CommandBuffers[m_BufferIndex];
        info.signalSemaphoreCount   = 0;
        info.pSignalSemaphores      = nullptr;

        // コマンドを実行.
        auto result = vkQueueSubmit(m_GraphicsQueue, 1, &info, m_GraphicsFence);
        if ( result != VK_SUCCESS )
        {
            ELOG( "Error : vkQueueSubmit() Failed." );
        }

        // 完了を待機.
        do {
            result = vkWaitForFences(m_Device, 1, &m_GraphicsFence, VK_TRUE, TimeOutNanoSec);
        } while( result == VK_TIMEOUT );

        VkPresentInfoKHR present = {};
        present.sType               = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present.pNext               = nullptr;
        present.swapchainCount      = 1;
        present.pSwapchains         = &m_SwapChain;
        present.pImageIndices       = &m_BufferIndex;
        present.pWaitSemaphores     = nullptr;
        present.waitSemaphoreCount  = 0;
        present.pResults            = nullptr;

        // 表示.
        result = vkQueuePresentKHR(m_GraphicsQueue, &present);
        if ( result != VK_SUCCESS )
        { ELOG( "Error : vkQueuePresentKHR() Failed." ); }

        // 次の画像を取得.
        result = vkAcquireNextImageKHR(
            m_Device,
            m_SwapChain,
            UINT64_MAX,
            m_GraphicsSemaphore,
            nullptr, 
            &m_BufferIndex);
        if ( result != VK_SUCCESS )
        { ELOG( "Error : vkAcquireNextImageKHR() Failed." ); }
    }
}
