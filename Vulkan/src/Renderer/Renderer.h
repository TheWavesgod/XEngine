#pragma once

class OpenGLRendererImpl;
class VulkanRendererImpl;
class DirectX12RendererImpl;

class RendererImpl
{
public:
    virtual void Initialize() = 0;
    virtual void RenderFrame() = 0;
    virtual ~RendererImpl() = default;
    
};

class Renderer
{
public:
    enum class API { OpenGL, Vulkan, DirectX12 };

    RendererImpl* Impl;
};

