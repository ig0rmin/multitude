#include "GPUAssociation.hpp"

#include <Radiant/Platform.hpp>

#include <cassert>

#if defined(RADIANT_LINUX)
# include <GL/glx.h>
#elif defined(RADIANT_WINDOWS)
# define WIN32_LEAN_AND_MEAN
# include <Windows.h>
# include "Luminous.hpp"
#endif

namespace Luminous
{

  bool GPUAssociation::isSupported()
  {
    bool ok = false;

#if defined(RADIANT_WINDOWS)
    ok = (wglGetProcAddress("wglGetGPUIDsAMD") != nullptr);
#elif defined(RADIANT_LINUX)
    ok = (glXGetProcAddress((GLubyte*)("glXGetGPUIDsAMD")) != nullptr);
#endif

    return ok;
  }

  unsigned int GPUAssociation::numGPUs()
  {
    unsigned int count = 0;

#if defined(RADIANT_WINDOWS)
    using functionPtr = UINT (*)(UINT maxCount, UINT *ids);

    auto wglGetGPUIDsAMD = (functionPtr)wglGetProcAddress("wglGetGPUIDsAMD");

    count = wglGetGPUIDsAMD(0, nullptr);
#elif defined(RADIANT_LINUX)
    using functionPtr = unsigned int (*)(unsigned int maxCount, unsigned int *ids);

    auto glXGetGPUIDsAMD = (functionPtr)glXGetProcAddress((GLubyte*)"glXGetGPUIDsAMD");

    count = glXGetGPUIDsAMD(0, nullptr);
#endif

    return count;
  }

  unsigned int GPUAssociation::gpuId(glbinding::ContextHandle context)
  {
    unsigned int id = 0;

#if defined(RADIANT_WINDOWS)
    using functionPtr = UINT (*)(HGLRC hglrc);

    auto platformHandle = reinterpret_cast<HGLRC>(context);
    auto wglGetContextGPUIDAMD = (functionPtr)wglGetProcAddress("wglGetContextGPUIDAMD");

    id = wglGetContextGPUIDAMD(platformHandle);

#elif defined(RADIANT_LINUX)
    using functionPtr = unsigned int (*)(GLXContext ctx);

    auto platformHandle = reinterpret_cast<GLXContext>(context);
    auto glXGetContextGPUIDAMD = (functionPtr)glXGetProcAddress((GLubyte*)"glXGetContextGPUIDAMD");

    id = glXGetContextGPUIDAMD(platformHandle);

#endif

    return id;
  }

  unsigned int GPUAssociation::gpuRam(unsigned int gpuId)
  {
    unsigned int totalMemoryInMB = 0;
#if defined(RADIANT_WINDOWS)
    using functionPtr = int (*)(unsigned int id, int property, GLenum dataType, unsigned int size, void *data);
    const int WGL_GPU_RAM_AMD = 0x21A3;

    auto wglGetGPUInfoAMD = (functionPtr)wglGetProcAddress("wglGetGPUInfoAMD");

    wglGetGPUInfoAMD(gpuId, WGL_GPU_RAM_AMD, GL_UNSIGNED_INT, 1, &totalMemoryInMB);

#elif defined(RADIANT_LINUX)
    using functionPtr = int (*)(unsigned int id, int property, GLenum dataType, unsigned int size, void *data);

    auto glXGetGPUInfoAMD = (functionPtr)glXGetProcAddress((GLubyte*)"glXGetGPUInfoAMD");

    glXGetGPUInfoAMD(gpuId, GLX_GPU_RAM_AMD, GL_UNSIGNED_INT, 1, &totalMemoryInMB);
#endif

    return totalMemoryInMB;
  }

}
