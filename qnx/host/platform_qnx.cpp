
#include <stdlib.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

// without this the build fails to properly link to the qnx EGL and GLESv2 libs:
//   SharedLibrary.cpp:266] SharedLibrary::open for [libGLESv2.so] failed (posix). dlerror: [Unresolved symbols]
//   EglOsApi_egl.cpp:208] EglOsGlLibrary: Could not open GL library libGLESv2.so [Unresolved symbols]
void link_EGL_GLES2(void) {
	eglGetDisplay(EGL_DEFAULT_DISPLAY);
	glViewport(0,0,0,0);
}
