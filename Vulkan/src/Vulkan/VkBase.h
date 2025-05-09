#pragma once

#include "VKEasyHearder.h"

namespace VK
{
	constexpr VkExtent2D defaultWindowSize = { 1280, 720 };
	
	class VkBase
	{
		static VkBase VKSingleton;

		VkBase() = default;
		VkBase(VkBase&&) = delete;

		~VkBase();

	public:
		static VkBase& Base()
		{
			return VKSingleton;
		}

		void Terminate() {
			this->~VkBase();
			instance = VK_NULL_HANDLE;
			physicalDevice = VK_NULL_HANDLE;
			device = VK_NULL_HANDLE;
			surface = VK_NULL_HANDLE;
			swapchain = VK_NULL_HANDLE;
			swapchainImages.resize(0);
			swapchainImageViews.resize(0);
			swapchainCreateInfo = {};
			debugMessenger = VK_NULL_HANDLE;
		}


	/** Instance **/
	private:
		VkInstance instance;

		std::vector<const char*> instanceLayers;
		std::vector<const char*> instanceExtensions;

		VkDebugUtilsMessengerEXT debugMessenger;

		static void AddLayerOrExtension( std::vector<const char*>& container, const char* name)
		{
			for (auto& i : container)
				if (!strcmp(name, i))
					return;          
			container.push_back(name);
		}

		VkResult CreateDebugMessenger();

	public:
		VkInstance Instance() const {
			return instance;
		}
		const std::vector<const char*>& InstanceLayers() const {
			return instanceLayers;
		}
		const std::vector<const char*>& InstanceExtensions() const {
			return instanceExtensions;
		}

		// Function used before create instance
		void AddInstanceLayer(const char* layerName) {
			AddLayerOrExtension(instanceLayers, layerName);
		}
		void AddInstanceExtension(const char* extensionName) {
			AddLayerOrExtension(instanceExtensions, extensionName);
		}
		
		VkResult CreateInstance(VkInstanceCreateFlags flags = 0);
		
		// Check after failed to create instance
		VkResult CheckInstanceLayers(std::span<const char*> layersToCheck);
		
		void InstanceLayers(const std::vector<const char*>& layerNames) {
			instanceLayers = layerNames;
		}
		VkResult CheckInstanceExtensions(std::span<const char*> extensionsToCheck, const char* layerName = nullptr) const;
		
		void InstanceExtensions(const std::vector<const char*>& extensionNames) {
			instanceExtensions = extensionNames;
		}

		

		/**
		 * Surface
		 **/
	private:
		VkSurfaceKHR surface;
		
	public:
		VkSurfaceKHR Surface() const { return surface; }
		
		void Surface(VkSurfaceKHR newSurface) {																		
			if (!this->surface)
				this->surface = newSurface;
		}

		/**
		 * physicalDevice
		 **/
	private:
		VkPhysicalDevice physicalDevice;
		VkPhysicalDeviceProperties physicalDeviceProperties;
		VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
		std::vector<VkPhysicalDevice> availablePhysicalDevices;

		VkDevice device;
		//有效的索引从0开始，因此使用特殊值VK_QUEUE_FAMILY_IGNORED（为UINT32_MAX）为队列族索引的默认值
		uint32_t queueFamilyIndex_graphics = VK_QUEUE_FAMILY_IGNORED;
		uint32_t queueFamilyIndex_presentation = VK_QUEUE_FAMILY_IGNORED;
		uint32_t queueFamilyIndex_compute = VK_QUEUE_FAMILY_IGNORED;
		VkQueue queue_graphics;
		VkQueue queue_presentation;
		VkQueue queue_compute;

		std::vector<const char*> deviceExtensions;

		//该函数被DeterminePhysicalDevice(...)调用，用于检查物理设备是否满足所需的队列族类型，并将对应的队列族索引返回到queueFamilyIndices，执行成功时直接将索引写入相应成员变量
		VkResult GetQueueFamilyIndices(VkPhysicalDevice physicalDevice, bool enableGraphicsQueue, bool enableComputeQueue, uint32_t (&queueFamilyIndices)[3]);

	public:
		//Getter
		VkPhysicalDevice PhysicalDevice() const { return physicalDevice; }

		const VkPhysicalDeviceProperties& PhysicalDeviceProperties() const { return physicalDeviceProperties; }
		const VkPhysicalDeviceMemoryProperties& PhysicalDeviceMemoryProperties() const { return physicalDeviceMemoryProperties; }
		
		VkPhysicalDevice AvailablePhysicalDevice(uint32_t index) const { return availablePhysicalDevices[index]; }
		uint32_t AvailablePhysicalDeviceCount() const { return uint32_t(availablePhysicalDevices.size()); }

		VkDevice Device() const { return device; }
		
		uint32_t QueueFamilyIndex_Graphics() const {
		    return queueFamilyIndex_graphics;
		}
		uint32_t QueueFamilyIndex_Presentation() const {
		    return queueFamilyIndex_presentation;
		}
		uint32_t QueueFamilyIndex_Compute() const {
		    return queueFamilyIndex_compute;
		}
		VkQueue Queue_Graphics() const {
		    return queue_graphics;
		}
		VkQueue Queue_Presentation() const {
		    return queue_presentation;
		}
		VkQueue Queue_Compute() const {
		    return queue_compute;
		}

		const std::vector<const char*>& DeviceExtensions() const {
		    return deviceExtensions;
		}

		//该函数用于创建逻辑设备前
		void AddDeviceExtension(const char* extensionName) {
		    AddLayerOrExtension(deviceExtensions, extensionName);
		}
		
		//该函数用于获取物理设备
		VkResult GetPhysicalDevices();
		
		//该函数用于指定所用物理设备并调用GetQueueFamilyIndices(...)取得队列族索引
		VkResult DeterminePhysicalDevice(uint32_t deviceIndex = 0, bool enableGraphicsQueue = true, bool enableComputeQueue = true);
		
		//该函数用于创建逻辑设备，并取得队列
		VkResult CreateDevice(VkDeviceCreateFlags flags = 0);
		
		//以下函数用于创建逻辑设备失败后
		VkResult CheckDeviceExtensions(std::span<const char*> extensionsToCheck, const char* layerName = nullptr) const;
		
		void DeviceExtensions(const std::vector<const char*>& extensionNames) {
		    deviceExtensions = extensionNames;
		}

		/**
		 * Swap Chain
		 **/
	private:
		std::vector <VkSurfaceFormatKHR> availableSurfaceFormats;

		VkSwapchainKHR swapchain;
		std::vector <VkImage> swapchainImages;
		std::vector <VkImageView> swapchainImageViews;
		// In order to rebuild the swapchain conveniently, save the createInfo of swapchain 
		VkSwapchainCreateInfoKHR swapchainCreateInfo = {};

		//该函数被CreateSwapchain(...)和RecreateSwapchain()调用
		VkResult CreateSwapchain_Internal();

		// Callback functions container
		std::vector<void(*)()> callbacks_createSwapchain;
		std::vector<void(*)()> callbacks_destroySwapchain;
		
		std::vector<void(*)()> callbacks_createDevice;
		std::vector<void(*)()> callbacks_destroyDevice;

	public:
		//Getter
		const VkFormat& AvailableSurfaceFormat(uint32_t index) const {
			return availableSurfaceFormats[index].format;
		}
		const VkColorSpaceKHR& AvailableSurfaceColorSpace(uint32_t index) const {
			return availableSurfaceFormats[index].colorSpace;
		}
		uint32_t AvailableSurfaceFormatCount() const {
			return uint32_t(availableSurfaceFormats.size());
		}

		VkSwapchainKHR Swapchain() const {
			return swapchain;
		}
		VkImage SwapchainImage(uint32_t index) const {
			return swapchainImages[index];
		}
		VkImageView SwapchainImageView(uint32_t index) const {
			return swapchainImageViews[index];
		}
		uint32_t SwapchainImageCount() const {
			return uint32_t(swapchainImages.size());
		}
		const VkSwapchainCreateInfoKHR& SwapchainCreateInfo() const {
			return swapchainCreateInfo;
		}

		VkResult GetSurfaceFormats();

		VkResult SetSurfaceFormat(VkSurfaceFormatKHR surfaceFormat);
		
		//该函数用于创建交换链
		VkResult CreateSwapchain(bool limitFrameRate = true, VkSwapchainCreateFlagsKHR flags = 0);
			
		//该函数用于重建交换链
		VkResult RecreateSwapchain();

		VkResult RecreateDevice(VkDeviceCreateFlags flags = 0);

		VkResult WaitIdle() const;

		void AddCallback_CreateSwapchain(void(*function)()) { callbacks_createSwapchain.push_back(function); }
		void AddCallback_DestroySwapchain(void(*function)()) { callbacks_destroySwapchain.push_back(function); }
		void AddCallback_CreateDevice(void(*function)()) { callbacks_createDevice.push_back(function); }
		void AddCallback_DestroyDevice(void(*function)()) { callbacks_destroyDevice.push_back(function); }

	private:
		uint32_t apiVersion = VK_API_VERSION_1_0;

	public:
		// Getter
		uint32_t ApiVersion() const { return apiVersion; }

		VkResult UseLatestApiVersion();

	private:
		uint32_t currentImageIndex = 0;
		
	public:
		//Getter
		uint32_t CurrentImageIndex() const { return currentImageIndex; }

		result_t SwapImage(VkSemaphore semaphore_imageIsAvailable);

		// 该函数用于将命令缓冲区提交到用于图形的队列
		result_t SubmitCommandBuffer_Graphics(VkSubmitInfo& submitInfo, VkFence fence = VK_NULL_HANDLE) const;

		// 该函数用于在渲染循环中将命令缓冲区提交到图形队列的常见情形
		result_t SubmitCommandBuffer_Graphics(VkCommandBuffer commandBuffer,
			VkSemaphore semaphore_imageIsAvailable = VK_NULL_HANDLE, VkSemaphore semaphore_renderingIsOver = VK_NULL_HANDLE, VkFence fence = VK_NULL_HANDLE,
			VkPipelineStageFlags waitDstStage_imageIsAvailable = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT) const;

		// 该函数用于将命令缓冲区提交到用于图形的队列，且只使用栅栏的常见情形
		result_t SubmitCommandBuffer_Graphics(VkCommandBuffer commandBuffer, VkFence fence = VK_NULL_HANDLE) const;
		
		//该函数用于将命令缓冲区提交到用于计算的队列
		result_t SubmitCommandBuffer_Compute(VkSubmitInfo& submitInfo, VkFence fence = VK_NULL_HANDLE) const;

		//该函数用于将命令缓冲区提交到用于计算的队列，且只使用栅栏的常见情形
		result_t SubmitCommandBuffer_Compute(VkCommandBuffer commandBuffer, VkFence fence = VK_NULL_HANDLE) const;

		result_t PresentImage(VkPresentInfoKHR& presentInfo);

		result_t PresentImage(VkSemaphore semaphore_renderingIsOver = VK_NULL_HANDLE);

		// VKBase+
	private:
		class VkBasePlus* pPlus = nullptr;

	public:
		static VkBasePlus& Plus() { return *VKSingleton.pPlus; }
		static void Plus(VkBasePlus& plus) { if (!VKSingleton.pPlus) VKSingleton.pPlus = &plus; }
	};

	inline VkBase VkBase::VKSingleton;

	



	class DeviceMemory
	{
		VkDeviceMemory handle = VK_NULL_HANDLE;
		// Actual allocated memory size 
		VkDeviceSize allocationSize = 0;
		// Memory attribute
		VkMemoryPropertyFlags memoryProperties = 0;

		// Modify the range of Non-host coherent Memory, when try to map the memory
		VkDeviceSize AdjustNonCoherentMemoryRange(VkDeviceSize& size, VkDeviceSize& offset) const
		{
			const VkDeviceSize& nonCoherentAtomSize = VkBase::Base().PhysicalDeviceProperties().limits.nonCoherentAtomSize;
			VkDeviceSize _offset = offset;
			offset = offset / nonCoherentAtomSize * nonCoherentAtomSize;
			size = std::min((size + _offset + nonCoherentAtomSize - 1) / nonCoherentAtomSize * nonCoherentAtomSize, allocationSize) - offset;
			return _offset - offset;
		}

	protected:
		class
		{
			friend class BufferMemory;
			friend class ImageMemory;
			bool value = false;
			operator bool() const { return value; }
			auto& operator = (bool value) { this->value = value; return *this;}
		} areBound;

	public:
		DeviceMemory() = default;
		DeviceMemory(VkMemoryAllocateInfo& allocateInfo) { Allocate(allocateInfo); }
		DeviceMemory(DeviceMemory&& other) noexcept
		{
			MoveHandle;
			allocationSize = other.allocationSize;
			memoryProperties = other.memoryProperties;
			other.allocationSize = 0;
			other.memoryProperties = 0;
		}
		~DeviceMemory() { DestroyHandleBy(vkFreeMemory); allocationSize = 0; memoryProperties = 0; }

		// Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		VkDeviceSize AllocationSize() const { return allocationSize; }
		VkMemoryPropertyFlags MemoryProperties() const { return memoryProperties; }

		// Const function
		// 映射host visible的内存区
		result_t MapMemory(void*& pData, VkDeviceSize size, VkDeviceSize offset = 0) const
		{
			VkDeviceSize inverseDeltaOffset;
			if (!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
			{
				inverseDeltaOffset = AdjustNonCoherentMemoryRange(size, offset);
			}
			if (VkResult result = vkMapMemory(VkBase::Base().Device(), handle, offset, size, 0, &pData)) {
				outStream << std::format("[ deviceMemory ] ERROR\nFailed to map the memory!\nError code: {}\n", int32_t(result));
				return result;
			}
			if (!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
				pData = static_cast<uint8_t*>(pData) + inverseDeltaOffset;
				VkMappedMemoryRange mappedMemoryRange = {
					.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
					.memory = handle,
					.offset = offset,
					.size = size
				};
				if (VkResult result = vkInvalidateMappedMemoryRanges(VkBase::Base().Device(), 1, &mappedMemoryRange)) {
					outStream << std::format("[ deviceMemory ] ERROR\nFailed to flush the memory!\nError code: {}\n", int32_t(result));
					return result;
				}
			}
			return VK_SUCCESS;
		}

		// 取消映射host visible的内存区
		result_t UnmapMemory(VkDeviceSize size, VkDeviceSize offset = 0) const
		{
			if (!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
			{
				AdjustNonCoherentMemoryRange(size, offset);
				VkMappedMemoryRange mappedMemoryRange = {
					.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
					.memory = handle,
					.offset = offset,
					.size = size
				};
				if (VkResult result = vkFlushMappedMemoryRanges(VkBase::Base().Device(), 1, &mappedMemoryRange))
				{
					outStream << std::format("[ deviceMemory ] ERROR\nFailed to flush the memory!\nError code: {}\n", int32_t(result));
					return result;
				}
			}
			vkUnmapMemory(VkBase::Base().Device(), handle);
			return VK_SUCCESS;
		}

		// BufferData(...)用于方便地更新设备内存区，适用于用memcpy(...)向内存区写入数据后立刻取消映射的情况
		result_t BufferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset = 0) const
		{
			void* pData_dst;
			if (VkResult result = MapMemory(pData_dst, size, offset))
			{
				return result;
			}
			memcpy(pData_dst, pData_src, size_t(size));
			return UnmapMemory(size, offset);
		}

		result_t BufferData(const auto& data_src) const
		{
			return BufferData(&data_src, sizeof data_src);
		}

		// RetrieveData(...)用于方便地从设备内存区取回数据，适用于用memcpy(...)从内存区取得数据后立刻取消映射的情况
		result_t RetrieveData(void* pData_dst, VkDeviceSize size, VkDeviceSize offset = 0) const
		{
			void* pData_src;
			if (VkResult result = MapMemory(pData_src, size, offset))
				return result;
			memcpy(pData_dst, pData_src, size_t(size));
			return UnmapMemory(size, offset);
		}

		//Non-const Function
		result_t Allocate(VkMemoryAllocateInfo& allocateInfo)
		{
			if (allocateInfo.memoryTypeIndex >= VkBase::Base().PhysicalDeviceMemoryProperties().memoryTypeCount)
			{
				outStream << std::format("[ deviceMemory ] ERROR\nInvalid memory type index!\n");
				return VK_RESULT_MAX_ENUM; //没有合适的错误代码，别用VK_ERROR_UNKNOWN
			}
			
			allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			if (VkResult result = vkAllocateMemory(VkBase::Base().Device(), &allocateInfo, nullptr, &handle))
			{
				outStream << std::format("[ deviceMemory ] ERROR\nFailed to allocate memory!\nError code: {}\n", int32_t(result));
				return result;
			}
			//记录实际分配的内存大小
			allocationSize = allocateInfo.allocationSize;
			//取得内存属性
			memoryProperties = VkBase::Base().PhysicalDeviceMemoryProperties().memoryTypes[allocateInfo.memoryTypeIndex].propertyFlags;
			return VK_SUCCESS;
		}
	};

	class  Buffer
	{
		VkBuffer handle = VK_NULL_HANDLE;
		
	public:
		Buffer() = default;
		Buffer(VkBufferCreateInfo& createInfo) { Create(createInfo); }
		Buffer(Buffer&& other) noexcept { MoveHandle; }
		~Buffer() { DestroyHandleBy(vkDestroyBuffer); }
		
		// Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;

		// Const Function
		VkMemoryAllocateInfo MemoryAllocateInfo(VkMemoryPropertyFlags desiredMemoryProperties) const
		{
			VkMemoryAllocateInfo memoryAllocateInfo = {
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
			};
			
			VkMemoryRequirements memoryRequirements;
			vkGetBufferMemoryRequirements(VkBase::Base().Device(), handle, &memoryRequirements);
			
			memoryAllocateInfo.allocationSize = memoryRequirements.size;
			memoryAllocateInfo.memoryTypeIndex = UINT32_MAX;
			auto& physicalDeviceMemoryProperties = VkBase::Base().PhysicalDeviceMemoryProperties();
			for (size_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
			{
				if (memoryRequirements.memoryTypeBits & 1 << i &&
					(physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & desiredMemoryProperties) == desiredMemoryProperties)
				{
					memoryAllocateInfo.memoryTypeIndex = i;
					break;
				}
			}
				
			//不在此检查是否成功取得内存类型索引，因为会把memoryAllocateInfo返回出去，交由外部检查
			//if (memoryAllocateInfo.memoryTypeIndex == UINT32_MAX)
			//    outStream << std::format("[ buffer ] ERROR\nFailed to find any memory type satisfies all desired memory properties!\n");
			return memoryAllocateInfo;
		}

		result_t BindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0) const
		{
			VkResult result = vkBindBufferMemory(VkBase::Base().Device(), handle, deviceMemory, memoryOffset);
			if (result)
			{
				outStream << std::format("[ buffer ] ERROR\nFailed to attach the memory!\nError code: {}\n", int32_t(result));
			}
			return result;
		}

		// Non-const Function
		result_t Create(VkBufferCreateInfo& createInfo)
		{
			createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			VkResult result = vkCreateBuffer(VkBase::Base().Device(), &createInfo, nullptr, &handle);
			if (result)
			{
				outStream << std::format("[ buffer ] ERROR\nFailed to create a buffer!\nError code: {}\n", int32_t(result));
			}
			return result;
		}
	};

	class BufferMemory : Buffer, DeviceMemory
	{
	public:
		BufferMemory() = default;
		BufferMemory(VkBufferCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties) { Create(createInfo, desiredMemoryProperties); }
		BufferMemory(BufferMemory&& other) noexcept : Buffer(std::move(other)), DeviceMemory(std::move(other))
		{
			areBound = other.areBound;
			other.areBound = false;
		}
		~BufferMemory() { areBound = false; }

		// Getter
		// 不定义到VkBuffer和VkDeviceMemory的转换函数，因为32位下这俩类型都是uint64_t的别名，会造成冲突（虽然，谁他妈还用32位PC！）
		VkBuffer BufferRef() const { return static_cast<const Buffer&>(*this); }
		const VkBuffer* AddressOfBuffer() const { return Buffer::Address(); }
		VkDeviceMemory MemoryRef() const { return static_cast<const DeviceMemory&>(*this); }
		const VkDeviceMemory* AddressOfMemory() const { return DeviceMemory::Address(); }
		//若areBond为true，则成功分配了设备内存、创建了缓冲区，且成功绑定在一起
		bool AreBound() const { return areBound; }
		using DeviceMemory::AllocationSize;
		using DeviceMemory::MemoryProperties;

		// Const Function
		using DeviceMemory::MapMemory;
		using DeviceMemory::UnmapMemory;
		using DeviceMemory::BufferData;
		using DeviceMemory::RetrieveData;

		// Non-const Function
		// 以下三个函数仅用于Create(...)可能执行失败的情况
		result_t CreateBuffer(VkBufferCreateInfo& createInfo)
		{
			return Buffer::Create(createInfo);
		}

		result_t AllocateMemory(VkMemoryPropertyFlags desiredMemoryProperties)
		{
			VkMemoryAllocateInfo allocateInfo = MemoryAllocateInfo(desiredMemoryProperties);
			if (allocateInfo.memoryTypeIndex >= VkBase::Base().PhysicalDeviceMemoryProperties().memoryTypeCount)
			{
				return VK_RESULT_MAX_ENUM; //没有合适的错误代码，别用VK_ERROR_UNKNOWN
			}
			return Allocate(allocateInfo);
		}

		result_t BindMemory()
		{
			if (VkResult result = Buffer::BindMemory(MemoryRef()))
				return result;
			areBound = true;
			return VK_SUCCESS;
		}

		//分配设备内存、创建缓冲、绑定
		result_t Create(VkBufferCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties)
		{
			VkResult result;
			false || //这行用来应对Visual Studio中代码的对齐
				(result = CreateBuffer(createInfo)) || //用||短路执行
				(result = AllocateMemory(desiredMemoryProperties)) ||
				(result = BindMemory());
			return result;
		}
	};

	class BufferView
	{
		VkBufferView handle = VK_NULL_HANDLE;
		
	public:
		BufferView() = default;
		BufferView(VkBufferViewCreateInfo& createInfo) { Create(createInfo); }
		BufferView(VkBuffer buffer, VkFormat format, VkDeviceSize offset = 0, VkDeviceSize range = 0 /*VkBufferViewCreateFlags flags*/) { Create(buffer, format, offset, range); }
		BufferView(BufferView&& other) noexcept { MoveHandle; }
		~BufferView() { DestroyHandleBy(vkDestroyBufferView); }
		
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		
		//Non-const Function
		result_t Create(VkBufferViewCreateInfo& createInfo)
		{
			createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
			VkResult result = vkCreateBufferView(VkBase::Base().Device(), &createInfo, nullptr, &handle);
			if (result)
			{
				outStream << std::format("[ bufferView ] ERROR\nFailed to create a buffer view!\nError code: {}\n", int32_t(result));
			}
			return result;
		}
		
		result_t Create(VkBuffer buffer, VkFormat format, VkDeviceSize offset = 0, VkDeviceSize range = 0 /*VkBufferViewCreateFlags flags*/)
		{
			VkBufferViewCreateInfo createInfo = {
				.buffer = buffer,
				.format = format,
				.offset = offset,
				.range = range
			};
			return Create(createInfo);
		}
	};

	class Image
	{
		VkImage handle = VK_NULL_HANDLE;

	public:
		Image() = default;
		Image(VkImageCreateInfo& createInfo) { Create(createInfo); }
	    Image(Image&& other) noexcept { MoveHandle; }
	    ~Image() { DestroyHandleBy(vkDestroyImage); }
		
	    // Getter
	    DefineHandleTypeOperator;
	    DefineAddressFunction;
		
	    // Const Function
	    VkMemoryAllocateInfo MemoryAllocateInfo(VkMemoryPropertyFlags desiredMemoryProperties) const
		{
	        VkMemoryAllocateInfo memoryAllocateInfo = {
	            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
	        };
	    	
	        VkMemoryRequirements memoryRequirements;
	        vkGetImageMemoryRequirements(VkBase::Base().Device(), handle, &memoryRequirements);
	        memoryAllocateInfo.allocationSize = memoryRequirements.size;
	    	
	        auto GetMemoryTypeIndex = [](uint32_t memoryTypeBits, VkMemoryPropertyFlags desiredMemoryProperties)->size_t
	    	{
	            auto& physicalDeviceMemoryProperties = VkBase::Base().PhysicalDeviceMemoryProperties();
	            for (size_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
	                if (memoryTypeBits & 1 << i &&
	                    (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & desiredMemoryProperties) == desiredMemoryProperties)
	                    return i;
	            return UINT32_MAX;
	        };
	        memoryAllocateInfo.memoryTypeIndex = GetMemoryTypeIndex(memoryRequirements.memoryTypeBits, desiredMemoryProperties);

	    	if (memoryAllocateInfo.memoryTypeIndex == UINT32_MAX &&
	            desiredMemoryProperties & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
	    	{
	            memoryAllocateInfo.memoryTypeIndex = GetMemoryTypeIndex(memoryRequirements.memoryTypeBits, desiredMemoryProperties & ~VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
	    	}
	        //不在此检查是否成功取得内存类型索引，因为会把memoryAllocateInfo返回出去，交由外部检查
	        //if (memoryAllocateInfo.memoryTypeIndex == -1)
	        //    outStream << std::format("[ image ] ERROR\nFailed to find any memory type satisfies all desired memory properties!\n");
	        return memoryAllocateInfo;
	    }
		
	    result_t BindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0) const
		{
	        VkResult result = vkBindImageMemory(VkBase::Base().Device(), handle, deviceMemory, memoryOffset);
	        if (result)
	        {
	            outStream << std::format("[ image ] ERROR\nFailed to attach the memory!\nError code: {}\n", int32_t(result));
	        }
	        return result;
	    }
		
	    // Non-const Function
	    result_t Create(VkImageCreateInfo& createInfo)
		{
	        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	        VkResult result = vkCreateImage(VkBase::Base().Device(), &createInfo, nullptr, &handle);
	        if (result)
	        {
	            outStream << std::format("[ image ] ERROR\nFailed to create an image!\nError code: {}\n", int32_t(result));
	        }
	        return result;
	    }
	};

	class ImageMemory :Image, DeviceMemory
	{
	public:
	    ImageMemory() = default;
	    ImageMemory(VkImageCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties) { Create(createInfo, desiredMemoryProperties); }
	    ImageMemory(ImageMemory&& other) noexcept : Image(std::move(other)), DeviceMemory(std::move(other))
		{
	        areBound = other.areBound;
	        other.areBound = false;
	    }
	    ~ImageMemory() { areBound = false; }
		
	    //Getter
	    VkImage ImageRef() const { return static_cast<const Image&>(*this); }
	    const VkImage* AddressOfImage() const { return Image::Address(); }
	    VkDeviceMemory MemoryRef() const { return static_cast<const DeviceMemory&>(*this); }
	    const VkDeviceMemory* AddressOfMemory() const { return DeviceMemory::Address(); }
	    bool AreBound() const { return areBound; }
	    using DeviceMemory::AllocationSize;
	    using DeviceMemory::MemoryProperties;
		
	    //Non-const Function
	    //以下三个函数仅用于Create(...)可能执行失败的情况
	    result_t CreateImage(VkImageCreateInfo& createInfo)
		{
	        return Image::Create(createInfo);
	    }
		
	    result_t AllocateMemory(VkMemoryPropertyFlags desiredMemoryProperties)
		{
	        VkMemoryAllocateInfo allocateInfo = MemoryAllocateInfo(desiredMemoryProperties);
	        if (allocateInfo.memoryTypeIndex >= VkBase::Base().PhysicalDeviceMemoryProperties().memoryTypeCount)
	        {
	            return VK_RESULT_MAX_ENUM; //没有合适的错误代码，别用VK_ERROR_UNKNOWN
	        }
	        return Allocate(allocateInfo);
	    }
		
	    result_t BindMemory()
		{
	        if (VkResult result = Image::BindMemory(MemoryRef()))
	        {
	            return result;
	        }
	        areBound = true;
	        return VK_SUCCESS;
	    }
		
	    //分配设备内存、创建图像、绑定
	    result_t Create(VkImageCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties)
		{
	        VkResult result;
	        false || //这行用来应对Visual Studio中代码的对齐
	            (result = CreateImage(createInfo)) || //用||短路执行
	            (result = AllocateMemory(desiredMemoryProperties)) ||
	            (result = BindMemory());
	        return result;
	    }
	};

	class ImageView
	{
		VkImageView handle = VK_NULL_HANDLE;
		
	public:
		ImageView() = default;
		ImageView(VkImageViewCreateInfo& createInfo) { Create(createInfo); }
		ImageView(VkImage image, VkImageViewType viewType, VkFormat format, const VkImageSubresourceRange& subresourceRange, VkImageViewCreateFlags flags = 0)
		{
			Create(image, viewType, format, subresourceRange, flags);
		}
		ImageView(ImageView&& other) noexcept { MoveHandle; }
		~ImageView() { DestroyHandleBy(vkDestroyImageView); }
		
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		
		//Non-const Function
		result_t Create(VkImageViewCreateInfo& createInfo)
		{
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			VkResult result = vkCreateImageView(VkBase::Base().Device(), &createInfo, nullptr, &handle);
			if (result)
			{
				outStream << std::format("[ imageView ] ERROR\nFailed to create an image view!\nError code: {}\n", int32_t(result));
			}
			return result;
		}
		
		result_t Create(VkImage image, VkImageViewType viewType, VkFormat format, const VkImageSubresourceRange& subresourceRange, VkImageViewCreateFlags flags = 0)
		{
			VkImageViewCreateInfo createInfo = {
				.flags = flags,
				.image = image,
				.viewType = viewType,
				.format = format,
				.subresourceRange = subresourceRange
			};
			return Create(createInfo);
		}
	};
}


