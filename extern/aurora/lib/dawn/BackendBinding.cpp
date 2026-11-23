#include "BackendBinding.hpp"

#include <aurora/webgpu.hpp>

#include <memory>

#include "../window.hpp"

#if !defined(SDL_PLATFORM_MACOS) && !defined(SDL_PLATFORM_IOS) && !defined(SDL_PLATFORM_TVOS)
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#endif

namespace aurora::webgpu {
namespace utils {

std::shared_ptr<wgpu::ChainedStruct> SetupWindowAndGetSurfaceDescriptorCocoa(SDL_Window* window);

#if defined(SDL_PLATFORM_LINUX)
WGPUBool create_kmsdrm_surface(void* userdata, void* instance, void** surface) {
  static bool vulkanLoaderInitialized = false;
  if (!vulkanLoaderInitialized) {
    if (!SDL_Vulkan_LoadLibrary(nullptr)) {
      SDL_Log("KMSDRM: SDL_Vulkan_LoadLibrary failed: %s", SDL_GetError());
      return WGPU_FALSE;
    }
    vulkanLoaderInitialized = true;
  }

  VkSurfaceKHR nativeSurface{};
  if (!SDL_Vulkan_CreateSurface(static_cast<SDL_Window*>(userdata), reinterpret_cast<VkInstance>(instance),
                                nullptr, &nativeSurface)) {
    SDL_Log("KMSDRM: SDL_Vulkan_CreateSurface failed: %s", SDL_GetError());
    return WGPU_FALSE;
  }

  *surface = reinterpret_cast<void*>(nativeSurface);
  return WGPU_TRUE;
}

#if defined(WEBGPU_DAWN)
WGPUBool get_kmsdrm_egl_surface(void* userdata, void** surface) {
  auto* window = static_cast<SDL_Window*>(userdata);
  void* eglSurface = window::get_kmsdrm_egl_surface(window);
  if (eglSurface == nullptr) {
    SDL_Log("KMSDRM: no SDL-created EGL draw surface is available for Dawn");
    return WGPU_FALSE;
  }
  *surface = eglSurface;
  return WGPU_TRUE;
}

WGPUBool present_kmsdrm_egl_surface(void* userdata) {
  if (!window::make_kmsdrm_gl_context_current(true)) {
    SDL_Log("KMSDRM: failed to make SDL's GL context current for presentation: %s", SDL_GetError());
    return WGPU_FALSE;
  }
  const bool swapped = SDL_GL_SwapWindow(static_cast<SDL_Window*>(userdata));
  const bool released = window::make_kmsdrm_gl_context_current(false);
  if (!swapped) {
    SDL_Log("KMSDRM: SDL_GL_SwapWindow failed: %s", SDL_GetError());
    return WGPU_FALSE;
  }
  if (!released) {
    SDL_Log("KMSDRM: failed to release SDL's GL context after presentation: %s", SDL_GetError());
    return WGPU_FALSE;
  }
  return WGPU_TRUE;
}
#endif
#endif

std::shared_ptr<wgpu::ChainedStruct> SetupWindowAndGetSurfaceDescriptor(SDL_Window* window) {
#if defined(SDL_PLATFORM_MACOS) || defined(SDL_PLATFORM_IOS) || defined(SDL_PLATFORM_TVOS)
  return SetupWindowAndGetSurfaceDescriptorCocoa(window);
#else
  const auto props = SDL_GetWindowProperties(window);
#if defined(SDL_PLATFORM_ANDROID)
  std::shared_ptr<wgpu::SurfaceSourceAndroidNativeWindow> desc =
      std::make_shared<wgpu::SurfaceSourceAndroidNativeWindow>();
  desc->window = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, nullptr);
  if (desc->window == nullptr) {
    return nullptr; // Port: no surface yet (or already destroyed); CreateAndroidSurfaceKHR would crash
  }
  return std::move(desc);
#elif defined(SDL_PLATFORM_WIN32)
  std::shared_ptr<wgpu::SurfaceSourceWindowsHWND> desc = std::make_shared<wgpu::SurfaceSourceWindowsHWND>();
  desc->hwnd = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
  desc->hinstance = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, nullptr);
  return std::move(desc);
#elif defined(SDL_PLATFORM_LINUX)
  const char* driver = SDL_GetCurrentVideoDriver();
  if (SDL_strcmp(driver, "kmsdrm") == 0) {
    auto desc = std::make_shared<wgpu::SurfaceSourceKmsdrm>();
    desc->userdata = window;
    desc->createSurface = create_kmsdrm_surface;
  #if defined(WEBGPU_DAWN)
    desc->getEGLSurface = get_kmsdrm_egl_surface;
    desc->present = present_kmsdrm_egl_surface;
  #endif
    return desc;
  }
  if (SDL_strcmp(driver, "wayland") == 0) {
    std::shared_ptr<wgpu::SurfaceSourceWaylandSurface> desc = std::make_shared<wgpu::SurfaceSourceWaylandSurface>();
    desc->display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
    desc->surface = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
    return std::move(desc);
  }
  if (SDL_strcmp(driver, "x11") == 0) {
    std::shared_ptr<wgpu::SurfaceSourceXlibWindow> desc = std::make_shared<wgpu::SurfaceSourceXlibWindow>();
    desc->display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
    desc->window = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    return std::move(desc);
  }
#endif
  return nullptr;
#endif
}

} // namespace utils

wgpu::Surface create_window_surface(const wgpu::Instance& instance, SDL_Window* window, const char* label) {
  if (!instance || window == nullptr) {
    return nullptr;
  }
  const auto chainedDescriptor = utils::SetupWindowAndGetSurfaceDescriptor(window);
  if (!chainedDescriptor) {
    return nullptr;
  }
  const wgpu::SurfaceDescriptor descriptor{
      .nextInChain = chainedDescriptor.get(),
      .label = label,
  };
  return instance.CreateSurface(&descriptor);
}

} // namespace aurora::webgpu
