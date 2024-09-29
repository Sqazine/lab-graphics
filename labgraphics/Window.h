#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <SDL.h>
#include <SDL_vulkan.h>
#include "Math/Vector2.h"
#include "Logger.h"

#define INIT_SDL_AND_LOAD_VULKAN()                                                    \
    do                                                                                \
    {                                                                                 \
        if (SDL_Init(SDL_INIT_VIDEO) < 0)                                             \
            LOG_ERROR("Failed to init sdl.");                                         \
        int flag = SDL_Vulkan_LoadLibrary(nullptr);                                   \
        if (flag != 0)                                                                \
        {                                                                             \
            mIsSupportVulkan = false;                                                 \
            LOG_ERROR("Not Support vulkan:{}\n", SDL_GetError());                     \
        }                                                                             \
        void *instanceProcAddr = SDL_Vulkan_GetVkGetInstanceProcAddr();               \
        if (!instanceProcAddr)                                                        \
        {                                                                             \
            mIsSupportVulkan = false;                                                 \
            LOG_ERROR("SDL_Vulkan_GetVkGetInstanceProcAddr(): {}\n", SDL_GetError()); \
            exit(1);                                                                  \
        }                                                                             \
    } while (false);

class Window
{
#define CREATE_WINDOW(x, y) mHandle = SDL_CreateWindow(mTitle.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, x, y, SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE)
public:
    Window(const Vector2f &extent)
        : mHandle(nullptr), mIsVisible(false)
    {
        INIT_SDL_AND_LOAD_VULKAN()

        CREATE_WINDOW(extent.x, extent.y);
    }

    Window()
        : mHandle(nullptr), mIsVisible(false)
    {
        INIT_SDL_AND_LOAD_VULKAN()
        SDL_Rect rect;
        SDL_GetDisplayBounds(0, &rect);

        auto aspect = 4.0f / 3.0f;

        int32_t actualWidth;
        int32_t actualHeight;

        if (rect.w > rect.h)
        {
            actualHeight = rect.h * 0.75f;
            actualWidth = actualHeight * aspect;
        }
        else
        {
            actualWidth = rect.w * 0.75f;
            actualHeight = actualWidth / aspect;
        }

        CREATE_WINDOW(actualWidth, actualHeight);
    }

    ~Window()
    {
        if (mHandle)
            SDL_DestroyWindow(mHandle);

        if (mIsSupportVulkan)
            SDL_Vulkan_UnloadLibrary();

        SDL_Quit();
    }

    void SetTitle(std::string_view str)
    {
        mTitle = str;
        SDL_SetWindowTitle(mHandle, mTitle.c_str());
    }

    void Resize(const Vector2u32 &extent)
    {
        Resize(extent.x, extent.y);
    }

    void Resize(uint32_t w, uint32_t h)
    {
        SDL_SetWindowSize(mHandle, w, h);
        SDL_SetWindowPosition(mHandle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }

    void Show()
    {
        SDL_ShowWindow(mHandle);
    }

    void Hide()
    {
        SDL_HideWindow(mHandle);
    }

    bool IsVisible()
    {
        return mIsVisible;
    }

    Vector2i32 GetExtent() const
    {
        Vector2i32 result;
        SDL_GetWindowSize(mHandle, (int *)&result.x, (int *)&result.y);
        return result;
    }

    SDL_Window *GetHandle() const
    {
        return mHandle;
    }

    std::vector<const char *> GetRequiredVulkanExtensions() const
    {
        uint32_t sdlRequiredExtensionCount = 0;
        SDL_Vulkan_GetInstanceExtensions(mHandle, &sdlRequiredExtensionCount, nullptr);
        std::vector<const char *> sdlRequiredExtensions(sdlRequiredExtensionCount);
        SDL_Vulkan_GetInstanceExtensions(mHandle, &sdlRequiredExtensionCount, sdlRequiredExtensions.data());
        return sdlRequiredExtensions;
    }

    VkSurfaceKHR GetSurface(VkInstance instance) const
    {
        VkSurfaceKHR result;
        SDL_Vulkan_CreateSurface(mHandle, instance, &result);
        return result;
    }

private:
    SDL_Window *mHandle;
    std::string mTitle = "lab-graphics";
    bool mIsSupportVulkan = true;
    bool mIsVisible = false;
};