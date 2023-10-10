#pragma once
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <iostream>
#include <functional>
#include <cstring>
#include <source_location>
#include <glm/glm.hpp>

struct swapChainFrame
{
    VkFence         fence;
    VkCommandBuffer cmdBuffer;
};
class boilerPlate{

    private:
        bool                 VALIDATION_ENABLED = false;
        uint32_t             frameNumber        = 0;
        static constexpr int s_FRAME_OVERLAP    = 2;

        VkFence                                     uploadFence;
        uint32_t                                    imgCount;
        VkFormat                                    colorFormat;
        VkPipeline                                  defaultPipeline;
        VkSurfaceKHR                                surface;
        VkCommandPool                               cmdPool;
        VkSwapchainKHR                              swapChain = VK_NULL_HANDLE;
        VkCommandBuffer                             uploadBuffer;
        VkColorSpaceKHR                             colorSpace;
        VkPipelineLayout                            defaultLayout;
        std::vector<VkImage>                        images;
        VkDescriptorSetLayout                       globalSetLayout;
        VkDescriptorSetLayout                       textureSetLayout;
        VkDebugUtilsMessengerEXT                    debugger;
        std::vector<VkFramebuffer>                  framebuffers;
        std::vector<VkImageView>                    imageViews;
        VkPhysicalDeviceMemoryProperties            memoryProps;
        std::array<VkFence,s_FRAME_OVERLAP>         fences;
        PFN_vkDestroyDebugUtilsMessengerEXT         debuggerDestroyer;
        std::array<VkCommandBuffer,s_FRAME_OVERLAP> cmdBuffers;
        
        struct 
        {
            uint32_t                        formatCount;
            //uint32_t                        pModesCount;
            VkSurfaceCapabilitiesKHR        capabilities;
            //std::vector<VkPresentModeKHR>   presentModes;
            std::vector<VkSurfaceFormatKHR> surfaceFormats;
        } swapchainDetails;
        void    vulkanBoilerplate  ();

        void    fenceSetup         ();
        void    swapchainSetup     ();
        void    handleResizing     (VkResult res);
        void    cmdBufferSetup     ();
        void    framebufferSetup   ();

        VkPipelineShaderStageCreateInfo loadShader(const char* fileName,VkShaderStageFlagBits stage);

        void    glfwInitialization ();
        

        int32_t          getQueueFamilyIndex(std::vector<VkQueueFamilyProperties>& props);
        VkFence&         current(int);
        VkCommandBuffer& current(double); /** @attention replace this ugly thing later*/

    protected:
        GLFWwindow*      window;

        VkQueue          graphicsQueue;
        VkDevice         logicalDevice;
        VkInstance       vulkInstance;
        VkRenderPass     renderPass;
        VkDescriptorPool imguiPool;
        VkDescriptorPool descPool;
        VkPhysicalDevice physicalDevice;

        struct 
        {
            VkSemaphore sRender;
            VkSemaphore sPresent;
        } sync;
        struct 
        {
            bool        setupSuccess = false;
            uint16_t    width        = 600;
            uint16_t    height       = 500;
            const char* appName;
        } generalInfo;
        
    
        

        /** @warning */
        void LOG(VkResult res, const std::source_location loc = std::source_location::current());
        void         fastSubmit  ( std::function<void(VkCommandBuffer cmd)>&& function );
        VkResult     setupDebugger();
        virtual void drawUI      ( VkCommandBuffer* cmd );
        virtual void imguiInit   ();
        virtual void imguiDestroy();

    public:
        void run    ();
        void setup  ( bool validation , const char* appName);
        void cleanup();
};
struct Vertex 
{
    glm::vec2 position;
    glm::vec3 color;
	glm::vec2 uv;
};


namespace info
{
    VkSubmitInfo                            subInfo               ( VkCommandBuffer*          cmd   );
    VkFenceCreateInfo                       fence                 ( VkFenceCreateFlags        flags );
    VkCommandBufferBeginInfo                bufferBeginInfo       ( VkCommandBufferUsageFlags flags );
    VkCommandBufferAllocateInfo             commandBufferAllocate ( VkCommandPool pool, uint32_t count , VkCommandBufferLevel level );
    VkDescriptorSetLayoutBinding            descriptorSetLayoutBinding(VkDescriptorType type,VkShaderStageFlags flags,uint32_t binding,uint32_t descriptorCount = 1);
    VkGraphicsPipelineCreateInfo            graphicsPipeline(VkPipelineLayout layout,VkRenderPass renderPass,VkPipelineCreateFlags flags=0);
    VkDescriptorSetLayoutCreateInfo         descriptorSetLayout(const VkDescriptorSetLayoutBinding* bindings,uint32_t bindingCount,VkDescriptorSetLayoutCreateFlags flags=0);
    VkVertexInputBindingDescription         vertex_input_bind(uint32_t stride,uint32_t binding,VkVertexInputRate inputRate);
    VkPipelineDynamicStateCreateInfo        dynamic_state(const VkDynamicState * pDynamicStates,uint32_t dynamicStateCount,VkPipelineDynamicStateCreateFlags flags = 0);
    VkPipelineViewportStateCreateInfo       viewport_state(const VkViewport* viewport,const VkRect2D* scissor,uint32_t viewportCount,uint32_t scissorCount,VkPipelineViewportStateCreateFlags flags=0);
    VkVertexInputAttributeDescription       vertex_input_attr(VkFormat format,uint32_t binding,uint32_t location,uint32_t offset);
    VkPipelineViewportStateCreateInfo       viewport_state(const VkViewport* viewport,const VkRect2D* scissor,uint32_t viewportCount,uint32_t scissorCount,VkPipelineViewportStateCreateFlags flags);
    VkPipelineColorBlendStateCreateInfo     color_blend_state(int32_t attachmentCount,const VkPipelineColorBlendAttachmentState * pAttachments);
    VkPipelineColorBlendAttachmentState     color_blend_attachment_state(VkColorComponentFlags colorWriteMask,VkBool32 blendEnable) ;
    VkPipelineColorBlendStateCreateInfo     color_blend_state(int32_t attachmentCount,const VkPipelineColorBlendAttachmentState * pAttachments);
    VkPipelineMultisampleStateCreateInfo    multisampling_state(VkSampleCountFlagBits rasterizationSamples,VkPipelineMultisampleStateCreateFlags flags = 0);
    VkPipelineVertexInputStateCreateInfo    vertex_input_state(uint32_t vACount , uint32_t vBCount ,const VkVertexInputAttributeDescription *vAttrDesk,const VkVertexInputBindingDescription *vBindDesk,VkPipelineVertexInputStateCreateFlags flags=0);
    VkPipelineDepthStencilStateCreateInfo   depth_stencil(VkBool32 depthTestEnable,VkBool32 depthWriteEnable,VkCompareOp depthCompareOp);
    VkPipelineInputAssemblyStateCreateInfo  input_assembly_state( VkPrimitiveTopology topology, VkPipelineInputAssemblyStateCreateFlags flags,VkBool32 primitiveRestartEnable);
    VkPipelineRasterizationStateCreateInfo  rasterization_state( VkPolygonMode polygonMode,VkCullModeFlags cullMode,VkFrontFace frontFace,VkPipelineRasterizationStateCreateFlags flags = 0 ); 
};