/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include <stdio.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "NativeSubWindow.h"

namespace {

constexpr char kExternalDisplayParentEnv[] = "CROSVM_DISPLAY_PARENT_HWND";

// crosvm's normal top-level display contains gfxstream's Vulkan window as one child. When HD
// embeds that outer display, the Vulkan window becomes a second-level cross-process child. Intel's
// Windows Vulkan WSI accepts the resulting surface and presents successfully, but DWM composes it
// as black. Create the actual render child directly below HD's viewport when the Worker supplied a
// short-lived launch handle. The crosvm input window remains a sibling and can keep owning input.
HWND getRenderParent(HWND fallback) {
    char value[32] = {};
    const DWORD length = GetEnvironmentVariableA(kExternalDisplayParentEnv, value, sizeof(value));
    if (length == 0 || length >= sizeof(value)) {
        return fallback;
    }
    char* end = nullptr;
    const unsigned long long raw = std::strtoull(value, &end, 10);
    if (end == value || *end != '\0') {
        return fallback;
    }
    HWND parent = reinterpret_cast<HWND>(static_cast<std::uintptr_t>(raw));
    return parent != nullptr && IsWindow(parent) ? parent : fallback;
}

HWND getCrosvmInputSibling(HWND renderWindow) {
    const HWND parent = GetParent(renderWindow);
    if (parent == nullptr) {
        return nullptr;
    }
    for (HWND child = GetWindow(parent, GW_CHILD); child != nullptr;
         child = GetWindow(child, GW_HWNDNEXT)) {
        if (child == renderWindow || !IsWindowVisible(child) || !IsWindowEnabled(child)) {
            continue;
        }
        char className[64] = {};
        const int length = GetClassNameA(child, className, sizeof(className));
        if (length >= 6 && std::strncmp(className, "CROSVM", 6) == 0) {
            return child;
        }
    }
    return nullptr;
}

bool isPointerMessage(UINT message) {
    switch (message) {
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
            return true;
        default:
            return false;
    }
}

}  // namespace

struct SubWindowUserData {
    SubWindowRepaintCallback repaint_callback;
    void* repaint_callback_param;
    bool external_presentation;
};

static LRESULT CALLBACK subWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (isPointerMessage(uMsg)) {
        const HWND input = getCrosvmInputSibling(hwnd);
        if (input != nullptr) {
            // The render and crosvm input HWNDs occupy the same Host viewport. Client-coordinate
            // pointer messages and screen-coordinate wheel messages therefore keep their native
            // Win32 meaning. Asynchronous forwarding avoids a cross-window re-entrancy path while
            // preserving the normal crosvm EventDevice and capture behavior.
            PostMessage(input, uMsg, wParam, lParam);
        }
        return 0;
    } else if (uMsg == WM_PAINT) {
        auto user_data =
            (SubWindowUserData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (user_data && user_data->external_presentation) {
            // HD's Vulkan swapchain is the authoritative retained presentation. A resize already
            // queues the last complete ColorBuffer from FrameBuffer::setupSubWindow; invoking the
            // synchronous repaint callback here races that post and can serialize two swapchain
            // recreations through the Win32 window thread. Validate the GDI update region without
            // touching the Vulkan/PostWorker pipeline.
            PAINTSTRUCT paint = {};
            BeginPaint(hwnd, &paint);
            EndPaint(hwnd, &paint);
            return 0;
        } else if (user_data && user_data->repaint_callback) {
            user_data->repaint_callback(user_data->repaint_callback_param);
        }
    } else if (uMsg == WM_NCDESTROY) {
        SubWindowUserData* user_data =
            (SubWindowUserData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        delete user_data;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

EGLNativeWindowType createSubWindow(FBNativeWindowType p_window,
                                    int x, int y,int width, int height, float dpr,
                                    SubWindowRepaintCallback repaint_callback,
                                    void* repaint_callback_param, int hideWindow){
    static const char className[] = "subWin";
    const HWND renderParent = getRenderParent(p_window);

    WNDCLASSA wc = {};
    if (!GetClassInfoA(GetModuleHandle(NULL), className, &wc)) {
        wc.style = CS_OWNDC;
        if (renderParent == p_window) {
            // Preserve the legacy GDI invalidation contract for ordinary gfxstream windows. HD's
            // direct Vulkan sibling must retain its last DWM image until the replacement
            // swapchain presents; CS_HREDRAW/CS_VREDRAW would explicitly invalidate it first.
            wc.style |= CS_HREDRAW | CS_VREDRAW;
        }
        wc.lpfnWndProc = &subWindowProc;               // points to window procedure
        wc.cbWndExtra = sizeof(void*) ;                // save extra window memory
        wc.lpszClassName = className;                  // name of window class
        RegisterClassA(&wc);
    }

    // We assume size/pos are passed in as logical size/coordinates. Convert it to pixel
    // coordinates.
    x *= dpr;
    y *= dpr;
    width *= dpr;
    height *= dpr;
    EGLNativeWindowType ret =
        CreateWindowExA(WS_EX_NOPARENTNOTIFY,  // do not bother our parent window
                        className, "sub", WS_CHILD, x, y, width, height, renderParent,
                        NULL, NULL, NULL);

    auto user_data = new SubWindowUserData();
    user_data->repaint_callback = repaint_callback;
    user_data->repaint_callback_param = repaint_callback_param;
    user_data->external_presentation = renderParent != p_window;

    SetWindowLongPtr(ret, GWLP_USERDATA, (LONG_PTR)user_data);
    // Vulkan presentation needs an enabled child on Windows. The WndProc above remains a thin
    // bridge: it forwards ordinary pointer messages to crosvm and never owns guest input state.
    EnableWindow(ret, TRUE);
    if (!hideWindow)
        ShowWindow(ret, SW_SHOW);
    return ret;
}

void destroySubWindow(EGLNativeWindowType win){
    PostMessage(win, WM_CLOSE, 0, 0);
}

int moveSubWindow(FBNativeWindowType p_parent_window,
                  EGLNativeWindowType p_sub_window,
                  int x,
                  int y,
                  int width,
                  int height,
                  float dpr) {
    // A normal gfxstream child keeps the legacy GDI repaint request. HD presents through a direct
    // sibling Vulkan surface and setupSubWindow explicitly queues its retained ColorBuffer, so an
    // additional WM_PAINT only adds a competing synchronous repost during swapchain recreation.
    const BOOL repaint = GetParent(p_sub_window) == p_parent_window ? TRUE : FALSE;
    BOOL ret = MoveWindow(p_sub_window,
                          x * dpr,
                          y * dpr,
                          width * dpr,
                          height * dpr,
                          repaint);
    return ret;
}
