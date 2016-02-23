﻿//-------------------------------------------------------------------------------------------------
// File : SampleApp.h
// Desc : Sample Application.
// Copyright(c) Project Asura. All right reserved.
//-------------------------------------------------------------------------------------------------
#pragma once


//-------------------------------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------------------------------
#include <asvkApp.h>
#include <vulkan/vulkan.h>
#include <vector>


///////////////////////////////////////////////////////////////////////////////////////////////////
// Texture structure
///////////////////////////////////////////////////////////////////////////////////////////////////
struct Texture
{
    VkImage     Image;
    VkImageView View;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// DepthStencilBuffer structure
///////////////////////////////////////////////////////////////////////////////////////////////////
struct DepthStencilBuffer
{
    VkImage         Image;
    VkImageView     View;
    VkDeviceMemory  Memory;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Gpu structure
///////////////////////////////////////////////////////////////////////////////////////////////////
struct Gpu
{
    VkPhysicalDevice                    Device;
    VkPhysicalDeviceMemoryProperties    MemoryProps;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// SampleApp class
///////////////////////////////////////////////////////////////////////////////////////////////////
class SampleApp : public asvk::App
{
    //=============================================================================================
    // list of friend classes and methods.
    //=============================================================================================
    /* NOTHING */

public:
    //=============================================================================================
    // public variables.
    //=============================================================================================
    /* NOTHING */

    //=============================================================================================
    // public methods.
    //=============================================================================================

    //---------------------------------------------------------------------------------------------
    //! @brief      コンストラクタです.
    //---------------------------------------------------------------------------------------------
    SampleApp();

    //---------------------------------------------------------------------------------------------
    //! @brief      デストラクタです.
    //---------------------------------------------------------------------------------------------
    virtual ~SampleApp();

private:
    //=============================================================================================
    // private variables.
    //=============================================================================================
    static constexpr uint32_t SwapChainCount = 2;
    static constexpr uint64_t TimeOutNanoSec = 100000000;  // ナノ秒単位.
    VkInstance                                      m_Instance;
    VkDevice                                        m_Device;
    std::vector<Gpu>                                m_Gpus;
    VkQueue                                         m_GraphicsQueue;
    VkFence                                         m_GraphicsFence;
    VkSemaphore                                     m_GraphicsSemaphore;
    uint32_t                                        m_GraphicsFamilyIndex;
    VkAllocationCallbacks                           m_AllocatorCallbacks;
    VkSurfaceKHR                                    m_Surface;
    VkSwapchainKHR                                  m_SwapChain;
    std::vector<Texture>                            m_BackBuffers;
    uint32_t                                        m_BufferIndex;
    VkCommandPool                                   m_CommandPool;
    std::vector<VkCommandBuffer>                    m_CommandBuffers;
    VkRenderPass                                    m_RenderPass;
    DepthStencilBuffer                              m_Depth;
    std::vector<VkFramebuffer>                      m_FrameBuffers;

    //=============================================================================================
    // private methods.
    //=============================================================================================

    //---------------------------------------------------------------------------------------------
    //! @brief      初期化時の処理です.
    //!
    //! @retval true    初期化に成功.
    //! @retval false   初期化に失敗.
    //---------------------------------------------------------------------------------------------
    bool OnInit() override;

    //---------------------------------------------------------------------------------------------
    //! @brief      終了時の処理です.
    //---------------------------------------------------------------------------------------------
    void OnTerm() override;

    //---------------------------------------------------------------------------------------------
    //! @brief      描画時の処理です.
    //---------------------------------------------------------------------------------------------
    void OnFrameRender(const asvk::FrameEventArgs& args) override;
};
