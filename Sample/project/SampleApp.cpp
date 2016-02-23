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

} // namespace /* anonymous */


///////////////////////////////////////////////////////////////////////////////////////////////////
// SampleApp class
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//      コンストラクタです.
//-------------------------------------------------------------------------------------------------
SampleApp::SampleApp()
: asvk::App( L"SampleApp", 960, 540, nullptr, nullptr, nullptr )
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

        m_Gpus.resize(count);
        result = vkEnumeratePhysicalDevices(m_Instance, &count, m_Gpus.data());
        if ( result != VK_SUCCESS )
        {
            ELOG( "Error : vkEnumeratePhysicalDevices() Failed." );
            return false;
        }
    }

    // デバイスとキューの生成.
    {
        uint32_t propCount;
        vkGetPhysicalDeviceQueueFamilyProperties(m_Gpus[0], &propCount, nullptr);

        std::vector<VkQueueFamilyProperties> props;
        props.resize(propCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_Gpus[0], &propCount, props.data());

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

        auto result = vkCreateDevice(m_Gpus[0], &deviceInfo, nullptr, &m_Device);
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
    }

    // スワップチェインの生成.
    {
    }

    // 深度ステンシルバッファの生成.
    {
        VkFormat depthFormat = VK_FORMAT_D24_UNORM_S8_UINT;

        VkImageTiling tiling;
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_Gpus[0], depthFormat, &props);

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

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType             = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext             = nullptr;
        allocInfo.allocationSize    = requirements.size;
        allocInfo.memoryTypeIndex   = 0;

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
    }

    // ユニフォームバッファの生成.
    {
    }

    // ディスクリプタとパイプライのレイアウトの初期化.
    {
    }

    // レンダーパスの生成.
    {
    }

    // シェーダの生成.
    {
    }

    // フレームバッファの生成.
    {
    }

    // 頂点バッファの生成.
    {
    }

    // ディスクリプタプールの生成
    {
    }

    // ディスクリプタ―セットの生成.
    {
    }

    // パイプラインキャッシュの生成.
    {
    }

    // パイプラインの生成.
    {
    }

    return true;
}

//-------------------------------------------------------------------------------------------------
//      終了時の処理です.
//-------------------------------------------------------------------------------------------------
void SampleApp::OnTerm()
{
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

    if (m_Device != nullptr)
    { vkDestroyDevice(m_Device, nullptr); }

    if (m_Instance != nullptr)
    { vkDestroyInstance(m_Instance, &m_AllocatorCallbacks); }

    m_Gpus.clear();
    m_CommandBuffers.clear();

    m_GraphicsFamilyIndex = 0;

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
}
