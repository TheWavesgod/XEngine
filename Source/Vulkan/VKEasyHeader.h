#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <stack>
#include <map>
#include <unordered_map>
#include <span>
#include <memory>
#include <functional>
#include <concepts>
#include <format>
#include <chrono>
#include <numeric>
#include <numbers>

#include <thread>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

// Vulkan
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX  
#pragma comment(lib, "vulkan-1.lib")
#endif

#include <vulkan/vulkan.h>

#define VK_RESULT_THROW

#ifdef VK_RESULT_THROW
class result_t
{
    VkResult result;
public:
    static void(*callback_throw)(VkResult result);
    result_t(VkResult result) : result(result) {}
    result_t(result_t&& other) noexcept : result(other.result) { other.result = VK_SUCCESS; }
    ~result_t() noexcept(false)
    {
        if (static_cast<uint32_t>(result) < VK_RESULT_MAX_ENUM)
        {
            return;
        }
        if (callback_throw)
        {
            callback_throw(result);
        }
        throw result;
    }

    operator VkResult()
    {
        VkResult result = this->result;
        this->result = VK_SUCCESS;
        return result;
    }
};

inline void(*result_t::callback_throw)(VkResult);

#elif  defined VK_RESULT_NODISCARD
struct [[nodiscard]] result_t {
    VkResult result;
    result_t(VkResult result) :result(result) {}
    operator VkResult() const { return result; }
};
#pragma warning(disable:4834)
#pragma warning(disable:6031)
#else
using result_t = VkResult;
#endif

template<typename T>
class arrayRef {
    T* const pArray = nullptr;
    size_t count = 0;
    
public:
    //从空参数构造，count为0
    arrayRef() = default;
    
    //从单个对象构造，count为1
    arrayRef(T& data) :pArray(&data), count(1) {}
    
    //从顶级数组构造
    template<size_t elementCount>
    arrayRef(T(&data)[elementCount]) : pArray(data), count(elementCount) {}
    
    //从指针和元素个数构造
    arrayRef(T* pData, size_t elementCount) :pArray(pData), count(elementCount) {}
    
    //复制构造，若T带const修饰，兼容从对应的无const修饰版本的arrayRef构造
    //24.01.07 修正因复制粘贴产生的typo：从pArray(&other)改为pArray(other.Pointer())
    arrayRef(const arrayRef<std::remove_const_t<T>>& other) :pArray(other.Pointer()), count(other.Count()) {}
    
    //Getter
    T* Pointer() const { return pArray; }
    size_t Count() const { return count; }
    
    //Const Function
    T& operator[](size_t index) const { return pArray[index]; }
    T* begin() const { return pArray; }
    T* end() const { return pArray + count; }
    
    //Non-const Function
    //禁止复制/移动赋值
    arrayRef& operator=(const arrayRef&) = delete;
};

static void AddLayerOrExtension(std::vector<const char*>& container, const char* name)
{
    for (auto& i : container)
    {
        if (!strcmp(name, i)) return;
    }

    container.push_back(name);
}

#ifndef NDEBUG
#define ENABLE_DEBUG_MESSENGER true
#else
#define ENABLE_DEBUG_MESSENGER false
#endif

#define DestroyHandleBy(Func) if (handle) { Func(VkBase::Base().Device(), handle, nullptr); handle = VK_NULL_HANDLE; }
#define MoveHandle handle = other.handle; other.handle = VK_NULL_HANDLE
#define DefineMoveAssignmentOperator(type) type& operator=(type&& other) { this->~type(); MoveHandle; return *this; }
#define DefineHandleTypeOperator operator decltype(handle)() const { return handle; }
#define DefineAddressFunction const decltype(handle)* Address() const { return &handle; }

#define ExecuteOnce(...) { static bool executed = false; if (executed) return __VA_ARGS__; executed = true; }

inline auto& outStream = std::cout; //不是constexpr，因为std::cout具有外部链接

constexpr VkExtent2D defaultWindowSize = { 1280, 720 };