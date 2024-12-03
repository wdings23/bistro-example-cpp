#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <assert.h>

#include <Windows.h>

#include <render/DX12/RendererDX12.h>
#include <render/Vulkan/RendererVulkan.h>

#include <render-driver/Vulkan/ImageViewVulkan.h>

#include <skeletal-animation/AnimationManager.h>
#include <LogPrint.h>
#include <wtfassert.h>

#include <chrono>
#include <sstream>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx12.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <tinyexr.h>

//#define USE_IMGUI       1

#include <mutex>
std::mutex gMutex;

#define WINDOW_WIDTH    512
#define WINDOW_HEIGHT   512

#define PI 3.14159f

HWND _initWindow(
    HINSTANCE hInstance,
    WNDPROC windowProc,
    uint32_t iWidth,
    uint32_t iHeight);

LRESULT CALLBACK _windowProc(
    HWND windowHandle,
    UINT uMessage,
    WPARAM wParam,
    LPARAM lParam);


//static float3 sCameraPosition(6.8f, 2.3f, 4.8f);
//static float3 sCameraLookAt(0.0f, 0.0f, 0.0f);

//static float3 sCameraLookAt(-3.44f, 1.53f, 0.625f);
//static float3 sCameraPosition(-7.08f, 1.62f, 0.675f);
//static float3 sInitialCameraPosition(1.0f, 1.0f, 0.0f);
//static float3 sInitialCameraLookAt(0.0f, 1.0f, 0.0f);
static float3 sInitialCameraPosition(-6.623f, 1.35f, -0.1312f);
static float3 sInitialCameraLookAt(-5.695f, 1.35f, 0.2434f);
static float3 sCameraPosition(1.0f, 1.0f, 0.0f);
static float3 sCameraLookAt(0.0f, 0.0f, 0.0f);

static float3 sCameraUp(0.0f, 1.0f, 0.0f);
static float4 sBallPosition(0.0f, -0.2f, 0.0f, 1.0f);

float2 sCameraAngle(0.0f, 0.0f);
int32_t siLastX = -1;
int32_t siLastY = -1;

static float3 sUpperEyeTranslation(0.0f, 0.0f, 0.0f);
static float sfHeading = 0.0f;
static bool sbMoving = false;
static float3 sWalkDir(0.0f, 0.0f, 0.0f);

static POINT sMouseClickPos = { -1, -1 };
static float3 sMouseClickWorldPos(0.0f, 0.0f, 0.0f);

static float gfCameraNear = 0.8f;
static float gfCameraFar = 100.0f;
static float gfFOV = 3.14159f * 0.5f;

static POINT sMiddleClickPos;
static POINT sLastPt;
static POINT sLastRightMousePt;
static POINT sLastLeftMousePt;

static bool sbMiddleMouseDown = false;
static bool sbRightMouseDown = false;
static bool sbLeftMouseDown = false;

static float3 sSunColor = float3(15.0f, 15.0f, 15.0f);

static ImGuiIO* spImguiIO = nullptr;
void initImgui(
    HWND hWnd,
    ID3D12Device* pDevice,
    ID3D12DescriptorHeap* pShaderResourceHeap,
    DXGI_FORMAT outputFormat,
    uint32_t iNumFramesInFlight);

void imguiLayout(Render::Common::CRenderer* pRenderer);

/*
**
*/
extern "C" char const* getSaveDir()
{
    return "C:\\Users\\Dingwings\\Logs";
}

static Render::Common::RenderDriverType sRenderDriverType = Render::Common::RenderDriverType::DX12;


#include "quaternion.h"
#include "mat4.h"

/*
**
*/
int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR pCommandLine,
    _In_ int iCommandShow)
{
    if(strstr(pCommandLine, "--render=vulkan"))
    {
        sRenderDriverType = Render::Common::RenderDriverType::Vulkan;
        printf("Using Vulkan\n");
    }
    else
    {
        printf("Using DX12\n");
    }

    AllocConsole();
    FILE* fpstdin = stdin, * fpstdout = stdout, * fpstderr = stderr;
    freopen_s(&fpstdin, "CONIN$", "r", stdin);
    freopen_s(&fpstdout, "CONOUT$", "w", stdout);
    freopen_s(&fpstderr, "CONOUT$", "w", stderr);

    HWND window = _initWindow(
        hInstance,
        _windowProc,
        500,
        500);

    // for attaching debugger
    //Sleep(20000);

    std::unique_ptr<Render::Common::RendererDescriptor> pDesc = nullptr;
    if(strstr(pCommandLine, "--render=vulkan"))
    {
        pDesc = std::make_unique<Render::Vulkan::RendererDescriptor>();
        reinterpret_cast<Render::Vulkan::RendererDescriptor*>(pDesc.get())->mHWND = window;
    }
    else
    {
        pDesc = std::make_unique<Render::DX12::RendererDescriptor>();
        reinterpret_cast<Render::DX12::RendererDescriptor*>(pDesc.get())->mHWND = window;
    }

    pDesc->mFormat = RenderDriver::Common::Format::R10G10B10A2_UNORM;
    pDesc->miScreenWidth = WINDOW_WIDTH;
    pDesc->miScreenHeight = WINDOW_HEIGHT;
    pDesc->miSamples = 1;
    pDesc->miSwapChainFlags = RenderDriver::Common::SwapChainFlagBits::FIFO;
    pDesc->miNumBuffers = 3;
    pDesc->miViewportWidth = WINDOW_WIDTH;
    pDesc->miViewportHeight = WINDOW_HEIGHT;
    pDesc->mfViewportMaxDepth = 1.0f;

    std::unique_ptr<Render::DX12::CRenderer> dx12Renderer = nullptr;
    std::unique_ptr<Render::Vulkan::CRenderer> vulkanRenderer = nullptr;
    Render::Common::CRenderer* pRenderer = nullptr;
    if(strstr(pCommandLine, "--render=vulkan"))
    {
        pDesc->mFormat = RenderDriver::Common::Format::B8G8R8A8_UNORM;
        vulkanRenderer = std::make_unique<Render::Vulkan::CRenderer>();
        pRenderer = vulkanRenderer.get();
    }
    else
    {
        dx12Renderer = std::make_unique<Render::DX12::CRenderer>();
        pRenderer = dx12Renderer.get();
    }

    pDesc->mRenderJobsFilePath = "d:\\test\\bistro-example-cpp\\render-jobs\\bistro-example-render-jobs.json";
    pRenderer->setup(*pDesc);
    
    float3 diff = normalize(sInitialCameraLookAt - sInitialCameraPosition);
    sInitialCameraLookAt = diff * 0.25f + sInitialCameraPosition;

    sCameraPosition = sInitialCameraPosition;
    sCameraLookAt = sInitialCameraLookAt;

    if(strstr(pCommandLine, "--render=vulkan"))
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        static_cast<Render::Vulkan::CRenderer*>(pRenderer)->initImgui();
    }

    pRenderer->initData();
    pRenderer->loadRenderJobInfo(pDesc->mRenderJobsFilePath);
    pRenderer->prepareRenderJobData();

    ShowWindow(window, iCommandShow);

#if defined(USE_IMGUI)
    if(strstr(pCommandLine, "--render=vulkan"))
    {
        ImGui_ImplWin32_Init(window);

        
        spImguiIO = &ImGui::GetIO();
    }
    else
    {
        initImgui(
            window,
            reinterpret_cast<ID3D12Device*>(pRenderer->getDevice()->getNativeDevice()),
            reinterpret_cast<ID3D12DescriptorHeap*>(pRenderer->getImguiDescriptorHeap()->getNativeDescriptorHeap()),
            SerializeUtils::DX12::convert(pDesc->mFormat),
            pDesc->miNumBuffers);
    }
#endif // USE_IMGUI

    
    auto lastTime = std::chrono::high_resolution_clock::now();
    uint64_t iCurrTimeMS = 0;
    
    MSG msg = {};
    while(msg.message != WM_QUIT)
    {
        if(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            continue;
        }

auto start = std::chrono::high_resolution_clock::now();
#if defined(USE_IMGUI)
        // imgui begin frame
        {
            if(sRenderDriverType == Render::Common::RenderDriverType::DX12)
            {
                ImGui_ImplDX12_NewFrame();
            }
            else if(sRenderDriverType == Render::Common::RenderDriverType::Vulkan)
            {
                ImGui_ImplVulkan_NewFrame();
            }

            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
        }
#endif // USE_IMGUI

#if defined(USE_IMGUI)
        // imgui layout
        pRenderer->beginDebugMarker("ImGUI Layout");
        imguiLayout(pRenderer);
        pRenderer->endDebugMarker();
#endif // USE_IMGUI
uint64_t iElapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();
        
        float3 up = float3(0.0f, 1.0f, 0.0f);
        float3 diff = normalize(sCameraLookAt - sCameraPosition);
        if(fabs(diff.y) > 0.98f)
        {
            up = float3(1.0f, 0.0f, 0.0f);
        }

        Render::Common::UpdateCameraDescriptor cameraDesc = {};
        cameraDesc.mfFar = 100.0f;
        cameraDesc.mfNear = 0.4f;
        cameraDesc.miCameraIndex = 0;
        cameraDesc.mUp = up;
        cameraDesc.mPosition = sCameraPosition;
        cameraDesc.mLookAt = sCameraLookAt;
        cameraDesc.mfFov = 3.14159f * 0.5f;

        pRenderer->updateCamera(cameraDesc);
        pRenderer->updateRenderJobData();
        pRenderer->draw();
    }

    return 0;
}

/*
**
*/
HWND _initWindow(
    HINSTANCE hInstance,
    WNDPROC windowProc,
    uint32_t iWidth,
    uint32_t iHeight)
{
    HWND windowHandle = 0;
    WNDCLASSEX windowClass = { 0 };

    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = hInstance;
    windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = (sRenderDriverType == Render::Common::RenderDriverType::DX12) ? L"RenderWithMe (DX12)" : L"RenderWithMe (Vulkan)";

    auto result = RegisterClassEx(&windowClass);
    assert(result);

    int iScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    int iScreenHeight = GetSystemMetrics(SM_CYSCREEN);

    DWORD dwExStyle, dwStyle;
    dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

    RECT windowRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    windowHandle = CreateWindow(
        windowClass.lpszClassName,
        windowClass.lpszClassName,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    assert(windowHandle);

    return windowHandle;
}

/*
**
*/
void handleCameraMousePan(
    int32_t iX,
    int32_t iY,
    int32_t iLastX,
    int32_t iLastY)
{
    float const fSpeed = 0.01f;

    float fDiffX = float(iX - iLastX);
    float fDiffY = float(iY - iLastY);

    float3 viewDir = sCameraLookAt - sCameraPosition;
    float3 normalizedViewDir = normalize(viewDir);

    float3 tangent = cross(sCameraUp, normalizedViewDir);
    float3 binormal = cross(tangent, normalizedViewDir);

    sCameraPosition = sCameraPosition + binormal * -fDiffY * fSpeed + tangent * -fDiffX * fSpeed;
    sCameraLookAt = sCameraLookAt + binormal * -fDiffY * fSpeed + tangent * -fDiffX * fSpeed;
}

/*
**
*/
void handleCameraMouseRotate(
    int32_t iX,
    int32_t iY,
    int32_t iLastX,
    int32_t iLastY)
{
    if(siLastX < 0)
    {
        siLastX = iX;
    }

    if(siLastY < 0)
    {
        siLastY = iY;
    }

    float fDiffX = float(iX - siLastX) * -1.0f;
    float fDiffY = float(iY - siLastY);

    float fRotationSpeed = 0.3f;
    float fDeltaX = (2.0f * 3.14159f) / 512.0f;
    float fDeltaY = (2.0f * 3.14159f) / 512.0f;

    sCameraAngle.y += fDiffX * fRotationSpeed * fDeltaY;
    sCameraAngle.x += fDiffY * fRotationSpeed * fDeltaX;

    if(sCameraAngle.y < 0.0f)
    {
        sCameraAngle.y = 2.0f * 3.14159f + sCameraAngle.y;
    }
    if(sCameraAngle.y > 2.0f * 3.14159f)
    {
        sCameraAngle.y = sCameraAngle.y - 2.0f * 3.14159f;
    }

    if(sCameraAngle.x < -PI * 0.5f)
    {
        sCameraAngle.x = -PI * 0.5f;
    }
    if(sCameraAngle.x > PI * 0.5f)
    {
        sCameraAngle.x = PI * 0.5f;
    }


    float4x4 rotateX = rotateMatrixX(sCameraAngle.x);
    float4x4 rotateY = rotateMatrixY(sCameraAngle.y);
    float4x4 totalMatrix = rotateY * rotateX;

    float3 diff = sInitialCameraPosition - sInitialCameraLookAt;
    
    float4 xformEyePosition = totalMatrix * float4(diff, 1.0f);
    xformEyePosition.x += sCameraLookAt.x;
    xformEyePosition.y += sCameraLookAt.y;
    xformEyePosition.z += sCameraLookAt.z;
    sCameraPosition = xformEyePosition;

    siLastX = iX;
    siLastY = iY;
}

static float3 sSunDirection(0.0f, 1.0f, 0.0f);
static float3 sSunRotation(0.0f, 0.0f, 0.0f);

/*
**
*/
void handleRotateSunDirection(
    int32_t iX,
    int32_t iY,
    int32_t iLastX,
    int32_t iLastY)
{
    float const fSpeed = 0.01f;
    
    float fDiffX = float(iX - iLastX);
    float fDiffY = float(iY - iLastY);

    sSunRotation.x += fDiffY * fSpeed;
    sSunRotation.y += fDiffX * fSpeed;

    //sSunRotation.y = 0.0f;

    sSunRotation.x = clamp(sSunRotation.x, -3.14159f * 0.5f, 3.14159f * 0.5f);

    float4x4 rotateX = rotateMatrixX(-sSunRotation.x);
    float4x4 rotateY = rotateMatrixY(-sSunRotation.y);
    float4x4 rotationMatrix = rotateY * rotateX;
    sSunDirection = mul(float4(0.0f, 1.0f, 0.0f, 1.0f), rotationMatrix);
}

#if defined(USE_IMGUI)
// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif // USE_IMGUI

/*
**
*/
LRESULT CALLBACK _windowProc(
    HWND windowHandle,
    UINT uMessage,
    WPARAM wParam,
    LPARAM lParam)
{
#if defined(USE_IMGUI)
    if(ImGui_ImplWin32_WndProcHandler(windowHandle, uMessage, wParam, lParam))
    {
        return true;
    }
#endif // USE_IMGUI

    switch(uMessage)
    {

    case WM_CLOSE:
        DestroyWindow(windowHandle);
        PostQuitMessage(0);
        break;
    case WM_PAINT:
        break;
    };

#if defined(USE_IMGUI)
    if(spImguiIO)
    {
        if(spImguiIO->WantCaptureKeyboard || spImguiIO->WantCaptureMouse)
        {
            return true;
        }
    }
#endif // USE_IMGUI


    switch(uMessage)
    {
    case WM_MBUTTONDOWN:
        GetCursorPos(&sMiddleClickPos);
        ScreenToClient(windowHandle, &sMiddleClickPos);
        sLastPt = sMiddleClickPos;
        sbMiddleMouseDown = true;
        break;

    case WM_LBUTTONUP:
        sbLeftMouseDown = false;
        siLastX = -1;
        siLastY = -1;
        break;

    case WM_MBUTTONUP:
        sbMiddleMouseDown = false;
        break;

    case WM_RBUTTONUP:
        sbRightMouseDown = false;
        break;

    case WM_LBUTTONDOWN:
    {
        //GetCursorPos(&sMouseClickPos);
        //ScreenToClient(windowHandle, &sMouseClickPos);
        //DEBUG_PRINTF("mouse (%d, %d)\n", sMouseClickPos.x, sMouseClickPos.y);
        sbLeftMouseDown = true;
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(windowHandle, &pt);

        sLastLeftMousePt = pt;
        
        break;
    }
    case WM_RBUTTONDOWN:
    {
        sbRightMouseDown = true;
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(windowHandle, &pt);

        sLastRightMousePt = pt;
        break;
    }
    case WM_MOUSEMOVE:
    {
        if(sbMiddleMouseDown)
        {
            SetCapture(windowHandle);

            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(windowHandle, &pt);
            
            sLastPt = pt;
        }
        else if(sbRightMouseDown)
        {
            SetCapture(windowHandle);

            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(windowHandle, &pt);
            handleCameraMousePan(pt.x, pt.y, sLastRightMousePt.x, sLastRightMousePt.y);
            sLastRightMousePt = pt;
        }
        else if(sbLeftMouseDown)
        {
            SetCapture(windowHandle);

            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(windowHandle, &pt);
            //handleRotateSunDirection(pt.x, pt.y, sLastLeftMousePt.x, sLastLeftMousePt.y);
            handleCameraMouseRotate(pt.x, pt.y, sLastPt.x, sLastPt.y);
            sLastLeftMousePt = pt;
        }
        break;
    }
    case WM_MOUSEWHEEL:
    {
        int32_t iDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        if(iDelta < 0)
        {
            float const fSpeed = 0.1f;
            float3 viewDir = normalize(sCameraLookAt - sCameraPosition);
            sCameraPosition += viewDir * -fSpeed;
            sCameraLookAt += viewDir * -fSpeed;

            DEBUG_PRINTF("delta forward: %d\n", iDelta);
        }
        else if(iDelta > 0)
        {
            float const fSpeed = 0.1f;
            float3 viewDir = normalize(sCameraLookAt - sCameraPosition);
            sCameraPosition += viewDir * fSpeed;
            sCameraLookAt += viewDir * fSpeed;
            
            DEBUG_PRINTF("delta backward: %d\n", iDelta);
        }

        break;
    }
    case WM_KEYDOWN:
        switch(wParam)
        {
            case VK_F1:
            {
                float4x4 const& viewMatrix = Render::Common::gaCameras[0].getViewMatrix();
                float4x4 const& projectionMatrix = Render::Common::gaCameras[0].getProjectionMatrix();

                std::vector<char> acBuffer0(1 << 16);
                memset(acBuffer0.data(), 0, acBuffer0.size());
                sprintf(acBuffer0.data(), "float viewMatrixData[] =\n{\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n};\n",
                    viewMatrix._m00, viewMatrix._m01, viewMatrix._m02, viewMatrix._m03,
                    viewMatrix._m10, viewMatrix._m11, viewMatrix._m12, viewMatrix._m13,
                    viewMatrix._m20, viewMatrix._m21, viewMatrix._m22, viewMatrix._m23,
                    viewMatrix._m30, viewMatrix._m31, viewMatrix._m32, viewMatrix._m33);

                std::vector<char> acBuffer1(1 << 16);
                memset(acBuffer1.data(), 0, acBuffer1.size());
                sprintf(acBuffer1.data(), "float projectionMatrixData[] =\n{\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n};",
                    projectionMatrix._m00, projectionMatrix._m01, projectionMatrix._m02, projectionMatrix._m03,
                    projectionMatrix._m10, projectionMatrix._m11, projectionMatrix._m12, projectionMatrix._m13,
                    projectionMatrix._m20, projectionMatrix._m21, projectionMatrix._m22, projectionMatrix._m23,
                    projectionMatrix._m30, projectionMatrix._m31, projectionMatrix._m32, projectionMatrix._m33);

                std::string totalContent = std::string(acBuffer0.data()) + std::string(acBuffer1.data());
                FILE* fp = fopen("c:\\Users\\Dingwings\\deferred-screenshots-3\\matrices.txt", "w");
                fwrite(totalContent.c_str(), sizeof(char), totalContent.length(), fp);
                fclose(fp);

                DEBUG_PRINTF("float viewMatrixData[] =\n{\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n};\n",
                    viewMatrix._m00, viewMatrix._m01, viewMatrix._m02, viewMatrix._m03,
                    viewMatrix._m10, viewMatrix._m11, viewMatrix._m12, viewMatrix._m13,
                    viewMatrix._m20, viewMatrix._m21, viewMatrix._m22, viewMatrix._m23,
                    viewMatrix._m30, viewMatrix._m31, viewMatrix._m32, viewMatrix._m33);

                DEBUG_PRINTF("float projectionMatrixData[] =\n{\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n\t%.4ff, %.4ff, %.4ff, %.4ff,\n};",
                    projectionMatrix._m00, projectionMatrix._m01, projectionMatrix._m02, projectionMatrix._m03,
                    projectionMatrix._m10, projectionMatrix._m11, projectionMatrix._m12, projectionMatrix._m13,
                    projectionMatrix._m20, projectionMatrix._m21, projectionMatrix._m22, projectionMatrix._m23,
                    projectionMatrix._m30, projectionMatrix._m31, projectionMatrix._m32, projectionMatrix._m33);

                break;
            }

            case VK_F2:
            {
                break;
            }

            case VK_UP:
            {
                break;
            }

            case VK_DOWN:
            {
                break;
            }

            case VK_LEFT:
            {
                break;
            }

            case VK_RIGHT:
            {
                
                break;
            }

            case VK_F11:
            {
                break;
            }
        }
    case WM_CHAR:
        switch(wParam)
        {
            case 'w':
            {
                float const fSpeed = 0.1f;
                float3 viewDir = normalize(sCameraLookAt - sCameraPosition);
                sCameraPosition += viewDir * fSpeed;
                sCameraLookAt += viewDir * fSpeed;

                break;
            }

            case 's':
            {
                float const fSpeed = 0.1f;
                float3 viewDir = normalize(sCameraLookAt - sCameraPosition);
                sCameraPosition += viewDir * -fSpeed;
                sCameraLookAt += viewDir * -fSpeed;

                break;
            }

            case 'a':
            {
                float const fSpeed = 0.1f;
                float3 viewDir = normalize(sCameraLookAt - sCameraPosition);
                float3 tangent = cross(sCameraUp, viewDir);
                float3 binormal = cross(viewDir, tangent);

                sCameraPosition += tangent * -fSpeed;
                sCameraLookAt += tangent * -fSpeed;

                break;
            }

            case 'd':
            {
                float const fSpeed = 0.1f;
                float3 viewDir = normalize(sCameraLookAt - sCameraPosition);
                float3 tangent = cross(sCameraUp, viewDir);
                float3 binormal = cross(viewDir, tangent);

                sCameraPosition += tangent * fSpeed;
                sCameraLookAt += tangent * fSpeed;

                break;
            }

            case '-':
            {
                float const fSpeed = 0.1f;
                float3 viewDir = normalize(sCameraLookAt - sCameraPosition);
                float3 tangent = cross(sCameraUp, viewDir);
                float3 binormal = cross(viewDir, tangent);

                sCameraPosition += tangent * fSpeed;
                sCameraLookAt += tangent * fSpeed;

                gfCameraNear -= 0.05f;

                break;
            }

            case '=':
            {
                float const fSpeed = 0.1f;
                float3 viewDir = normalize(sCameraLookAt - sCameraPosition);
                float3 tangent = cross(sCameraUp, viewDir);
                float3 binormal = cross(viewDir, tangent);

                sCameraPosition += tangent * fSpeed;
                sCameraLookAt += tangent * fSpeed;

                gfCameraNear += 0.05f;

                break;
            }

            case '9':
            {
                float const fSpeed = 0.1f;
                float3 viewDir = normalize(sCameraLookAt - sCameraPosition);
                float3 tangent = cross(sCameraUp, viewDir);
                float3 binormal = cross(viewDir, tangent);

                sCameraPosition += tangent * fSpeed;
                sCameraLookAt += tangent * fSpeed;

                gfFOV -= 0.01f;

                break;
            }

            case '0':
            {
                float const fSpeed = 0.1f;
                float3 viewDir = normalize(sCameraLookAt - sCameraPosition);
                float3 tangent = cross(sCameraUp, viewDir);
                float3 binormal = cross(viewDir, tangent);

                sCameraPosition += tangent * fSpeed;
                sCameraLookAt += tangent * fSpeed;

                gfFOV += 0.01f;

                break;
            }

            case '/':
            {
                sSunColor += float3(-0.1f, -0.1f, -0.1f);
                break;
            }

            case '*':
            {
                sSunColor += float3(0.1f, 0.1f, 0.1f);
                break;
            }

            case '1':
            {
                break;
            }

            case '2':
            {
                break;
            }

            case '3':
            {
                break;
            }
        }
    }

    return DefWindowProc(windowHandle, uMessage, wParam, lParam);
}

#if defined(USE_IMGUI)
/*
**
*/
void initImgui(
    HWND hWnd,
    ID3D12Device* pDevice,
    ID3D12DescriptorHeap* pShaderResourceHeap,
    DXGI_FORMAT outputFormat,
    uint32_t iNumFramesInFlight)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    spImguiIO = &ImGui::GetIO();

    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX12_Init(
        pDevice,
        iNumFramesInFlight,
        outputFormat,
        pShaderResourceHeap,
        pShaderResourceHeap->GetCPUDescriptorHandleForHeapStart(),
        pShaderResourceHeap->GetGPUDescriptorHandleForHeapStart());
}

#endif // USE_IMGUI

static int32_t siSelectedOutputAttachment = 0;
static uint32_t siRestirType = 1;
static float sfWorldSpaceBlurRadius = 0.5f;
static float sfLastWorldSpaceBlurRadius = 0.5f;
static int32_t siWorldSpaceBlurSamples = 16;
static int32_t siLastWorldSpaceBlurSamples = 16;
static float sfSunIntensity = 15.0f;
static float sfLastSunIntensity = 15.0f;
static int32_t siSelectedOutputAttachmentRenderJob = 0;
static int32_t siSelectedInputAttachmentRenderJob = 0;
static float sfSpatialRestirRadius0 = 32.0f;
static float sfSpatialRestirRadius1 = 16.0f;
static float sfLastSpatialRestirRadius0 = 2.0f;
static float sfLastSpatialRestirRadius1 = 1.0f;
static int32_t siSpatialRestirSampleCount0 = 8;
static int32_t siSpatialRestirSampleCount1 = 6;
static int32_t siLastSpatialRestirSampleCount0 = 8;
static int32_t siLastSpatialRestirSampleCount1 = 6;
static uint64_t siSelectedOutputAttachmentTexturePtr = UINT64_MAX;
static uint64_t siSelectedInputAttachmentTexturePtr = UINT64_MAX;
static bool sbShowDebugOptionsUI = false;
static bool sbShowOutputAttachmentUI = false;
static bool sbShowInputAttachmentUI = false;
static bool sbShowRenderJobStatsUI = false;
static float sfBilateralBlurRadius = 2.0f;
static float sfBilateralBlurScale = 2.0f;
static float sfBilateralBlurColorSensitivty = 30.0f;
static float sfLastBilateralBlurRadius = 2.0f;
static float sfLastBilateralBlurScale = 2.0f;
static float sfLastBilateralBlurColorSensitivty = 30.0f;
static float sfAmbientOcclusionBlurRadius = 1.0f;
static float sfLastAmbientOcclusionBlurRadius = 1.0f;
static float sfAmbientOcclusionMaxDistance = 1.5f;
static float sfLastAmbientOcclusionMaxDistance = 1.5f;
static float sfMaterialRoughness = 0.5f;
static float sfLastMaterialRoughness = 0.5f;
static float sfMaterialMetalness = 0.0f;
static float sfLastMaterialMetalness = 0.0f;
static float sfUpscaleRadius = 1.0f;
static float sfLastUpscaleRadius = 1.0f;
static float3 sLocalLightColor = float3(5.0f, 2.5f, 0.3f);
static float sfLocalLightIntensity = 0.0f;
static float sfLastLocalLightIntensity = 5.0f;

static std::map<uint64_t, VkDescriptorSet>  saImageDescriptorSets;

/*
**
*/
void outputAttachmentLayout(Render::Common::CRenderer* pRenderer)
{
#if 0
    Render::Common::Serializer* pSerializer = pRenderer->getSerializer();
    auto const& aRenderJobs = pSerializer->getRenderJobs();

    ImGui::Begin("Render Job Output Attachments");
    {
        if(ImGui::ListBoxHeader("Render Jobs"))
        {
            std::vector<char[256]> aRenderJobNames(aRenderJobs.size());
            for(uint32_t i = 0; i < static_cast<uint32_t>(aRenderJobs.size()); i++)
            {
                strcpy(aRenderJobNames[i], aRenderJobs[i].mName.c_str());
            }

            for(uint32_t i = 0; i < static_cast<uint32_t>(aRenderJobs.size()); i++)
            {
                bool bSelected = (i == siSelectedOutputAttachmentRenderJob) ? true : false;
                if(ImGui::Selectable(aRenderJobNames.data()[i], &bSelected))
                {
                    siSelectedOutputAttachmentRenderJob = i;
                }
            }
            ImGui::ListBoxFooter();
        }
    }
    ImGui::End();

    // output attachment images
    std::string outputAttachmentName = "";
    std::string outputAttachmentTitle = "Output Attachments (" + aRenderJobs[siSelectedOutputAttachmentRenderJob].mName + ")";
    PLATFORM_OBJECT_HANDLE renderTargetHandle = 0;
    ImGui::Begin(outputAttachmentTitle.c_str());
    {
        ImGui::Text(aRenderJobs[siSelectedOutputAttachmentRenderJob].mName.c_str());

        auto& renderJob = pSerializer->getRenderJob(aRenderJobs[siSelectedOutputAttachmentRenderJob].mName);
        renderTargetHandle = 0;
        uint32_t iNumAttachments = min(static_cast<uint32_t>(renderJob.maOutputRenderTargetAttachments.size()), 32);
        for(uint32_t iOutputAttachment = 0; iOutputAttachment < iNumAttachments; iOutputAttachment++)
        {
            if(renderJob.maOutputRenderTargetAttachments[iOutputAttachment] <= 0)
            {
                continue;
            }

            RenderDriver::Common::CImage* pImage = nullptr;
            if(renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[iOutputAttachment]].mType == Render::Common::AttachmentType::TextureOut ||
                renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[iOutputAttachment]].mType == Render::Common::AttachmentType::TextureInOut)
            {
                bool bIsImage = pSerializer->isImage(renderJob.maOutputRenderTargetAttachments[iOutputAttachment]);
                if(bIsImage)
                {
                    pImage = pSerializer->getImage(renderJob.maOutputRenderTargetAttachments[iOutputAttachment]).get();
                }
                else
                {
                    auto& renderTarget = pSerializer->getRenderTarget(renderJob.maOutputRenderTargetAttachments[iOutputAttachment]);
                    pImage = renderTarget->getImage().get();
                }
            }

            if(pImage)
            {
                uint64_t iTexturePtr = 0;
                if(pRenderer->getRenderDriverType() == Render::Common::RenderDriverType::Vulkan)
                {
                    // descriptor_set = ImGui_ImplVulkan_AddTexture(...), need image view for image
                    // 
                    // equivalent to:
                    // fill out VkWriteDescriptorSet descriptorWrite = {};
                    // fill out VkDescriptorImageInfo -> sampler,  imageView, imageLayout
                    // call vkUpdateDescriptorSet
                
                    VkImage& nativeImage = *(static_cast<VkImage*>(pImage->getNativeImage()));
                    uint64_t iID = reinterpret_cast<uint64_t>(nativeImage);
                    if(saImageDescriptorSets.find(iID) == saImageDescriptorSets.end())
                    {
                        // first time creation of the image view and descriptor set used as ImTextureID 

                        Render::Vulkan::CRenderer* pRendererVulkan = static_cast<Render::Vulkan::CRenderer*>(pRenderer);

                        // create image view for AddTexture()
                        std::unique_ptr<RenderDriver::Vulkan::CImageView> imageView = std::make_unique<RenderDriver::Vulkan::CImageView>();
                        RenderDriver::Common::ImageViewDescriptor imageViewDesc = {};
                        imageViewDesc.mpImage = pImage;
                        imageViewDesc.mFormat = pImage->getDescriptor().mFormat;
                        imageViewDesc.miNumImages = 1;
                        PLATFORM_OBJECT_HANDLE handle = imageView->create(imageViewDesc, *(pRenderer->getDevice()));

                        //Render::Vulkan::Serializer* pSerializer = static_cast<Render::Vulkan::Serializer*>(pRenderer->getSerializer());
                        //RenderDriver::Vulkan::CImageView* pImageView = static_cast<RenderDriver::Vulkan::CImageView*>(pSerializer->registerObject(imageView, handle));

                        // descriptor set for the image, use as input for texture id used in ImageButton
                        // pass descriptor_set to , ie.  ImGui::Image((ImTextureID)descriptor_set, ...)
                        //VkDescriptorSet descriptorSet = ImGui_ImplVulkan_AddTexture(
                        //    pRendererVulkan->getNativeLinearSampler(),
                        //    pImageView->getNativeImageView(),
                        //    VK_IMAGE_LAYOUT_GENERAL);

                        // save descriptor set
                        //iTexturePtr = reinterpret_cast<uint64_t>(descriptorSet);
                        //saImageDescriptorSets[iID] = std::move(descriptorSet);
                    }
                    else
                    {
                        iTexturePtr = reinterpret_cast<uint64_t>(saImageDescriptorSets[iID]);
                    }
                }
                else
                {
                    // existing texture
                    pRenderer->getImguiDescriptorHeap()->setImageView(pImage, iOutputAttachment + 1, *pRenderer->getDevice());
                    iTexturePtr = pRenderer->getImguiDescriptorHeap()->getGPUHandle(iOutputAttachment + 1, *pRenderer->getDevice());
                }

                renderTargetHandle = renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[iOutputAttachment]].mAttachmentHandle;
                std::string const& attachmentName = renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[iOutputAttachment]].mName.c_str();

                std::ostringstream oss;
                auto const& imageDesc = pImage->getDescriptor();
                oss << attachmentName.c_str() << " (" << imageDesc.miWidth << " x " << imageDesc.miHeight << ")";
                ImGui::Text(oss.str().c_str());

                if(ImGui::ImageButton((ImTextureID)iTexturePtr, ImVec2(64, 64)))
                {
                    siSelectedOutputAttachment = iOutputAttachment;
                    outputAttachmentName = attachmentName;
                    siSelectedOutputAttachmentTexturePtr = iTexturePtr;
                }
            }
            //else
            //{
            //    std::string const& attachmentName = renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[iOutputAttachment]].mName.c_str();
            //    if(ImGui::Button(attachmentName.c_str()))
            //    {
            //        siSelectedOutputAttachment = iOutputAttachment;
            //        outputAttachmentName = attachmentName;
            //        siSelectedOutputAttachmentTexturePtr = UINT64_MAX;
            //    }
            //}
        }
    }
    ImGui::End();

    if(outputAttachmentName.length() <= 0)
    {
        outputAttachmentName = "Output Attachment";
    }

    // Selected Attachment
    ImGui::Begin(outputAttachmentName.c_str());
    {
        if(siSelectedOutputAttachmentTexturePtr != UINT64_MAX)
        {
            ImGui::Image((ImTextureID)siSelectedOutputAttachmentTexturePtr, ImVec2(768, 768));
        }
    }
    ImGui::End();
#endif // #if 0
}

/*
**
*/
void inputAttachmentLayout(Render::Common::CRenderer* pRenderer)
{
#if 0
    Render::Common::Serializer* pSerializer = pRenderer->getSerializer();
    auto const& aRenderJobs = pSerializer->getRenderJobs();

    auto getImageTexturePtrVulkan = [&](RenderDriver::Common::CImage* pImage)
    {
        uint64_t iTexturePtr = 0;

        // descriptor_set = ImGui_ImplVulkan_AddTexture(...), need image view for image
        // 
        // equivalent to:
        // fill out VkWriteDescriptorSet descriptorWrite = {};
        // fill out VkDescriptorImageInfo -> sampler,  imageView, imageLayout
        // call vkUpdateDescriptorSet

        VkImage& nativeImage = *(static_cast<VkImage*>(pImage->getNativeImage()));
        uint64_t iID = reinterpret_cast<uint64_t>(nativeImage);
        if(saImageDescriptorSets.find(iID) == saImageDescriptorSets.end())
        {
            // first time creation of the image view and descriptor set used as ImTextureID 

            Render::Vulkan::CRenderer* pRendererVulkan = static_cast<Render::Vulkan::CRenderer*>(pRenderer);

            // create image view for AddTexture()
            std::unique_ptr<RenderDriver::Vulkan::CImageView> imageView = std::make_unique<RenderDriver::Vulkan::CImageView>();
            RenderDriver::Common::ImageViewDescriptor imageViewDesc = {};
            imageViewDesc.mpImage = pImage;
            imageViewDesc.mFormat = pImage->getDescriptor().mFormat;
            imageViewDesc.miNumImages = 1;
            PLATFORM_OBJECT_HANDLE handle = imageView->create(imageViewDesc, *(pRenderer->getDevice()));

            //Render::Vulkan::Serializer* pSerializer = static_cast<Render::Vulkan::Serializer*>(pRenderer->getSerializer());
            //RenderDriver::Vulkan::CImageView* pImageView = static_cast<RenderDriver::Vulkan::CImageView*>(pSerializer->registerObject(imageView, handle));
            //
            //// descriptor set for the image, use as input for texture id used in ImageButton
            //// pass descriptor_set to , ie.  ImGui::Image((ImTextureID)descriptor_set, ...)
            //VkDescriptorSet descriptorSet = ImGui_ImplVulkan_AddTexture(
            //    pRendererVulkan->getNativeLinearSampler(),
            //    pImageView->getNativeImageView(),
            //    VK_IMAGE_LAYOUT_GENERAL);
            //
            //// save descriptor set
            //iTexturePtr = reinterpret_cast<uint64_t>(descriptorSet);
            //saImageDescriptorSets[iID] = std::move(descriptorSet);
        }
        else
        {
            iTexturePtr = reinterpret_cast<uint64_t>(saImageDescriptorSets[iID]);
        }

        return iTexturePtr;
    };

    ImGui::Begin("Render Job Input Attachments");
    {
        if(ImGui::ListBoxHeader("Render Jobs"))
        {
            std::vector<char[256]> aRenderJobNames(aRenderJobs.size());
            for(uint32_t i = 0; i < static_cast<uint32_t>(aRenderJobs.size()); i++)
            {
                strcpy(aRenderJobNames[i], aRenderJobs[i].mName.c_str());
            }

            for(uint32_t i = 0; i < static_cast<uint32_t>(aRenderJobs.size()); i++)
            {
                bool bSelected = (i == siSelectedInputAttachmentRenderJob) ? true : false;
                if(ImGui::Selectable(aRenderJobNames.data()[i], &bSelected))
                {
                    siSelectedInputAttachmentRenderJob = i;
                }
            }
            ImGui::ListBoxFooter();
        }
    }
    ImGui::End();

    // output attachment images
    std::string inputAttachmentName = "";
    std::string inputAttachmentTitle = "Input Attachments (" + aRenderJobs[siSelectedInputAttachmentRenderJob].mName + ")";
    ImGui::Begin(inputAttachmentTitle.c_str());
    {
        ImGui::Text(aRenderJobs[siSelectedInputAttachmentRenderJob].mName.c_str());

        auto& renderJob = pSerializer->getRenderJob(aRenderJobs[siSelectedInputAttachmentRenderJob].mName);
        uint32_t iNumInputAttachments = min(static_cast<uint32_t>(renderJob.maInputRenderTargetAttachments.size()), 32);
        for(uint32_t iInputAttachment = 0; iInputAttachment < iNumInputAttachments; iInputAttachment++)
        {
            if(renderJob.maInputRenderTargetAttachments[iInputAttachment] <= 0)
            {
                continue;
            }

            bool bSeparateImage = false;
            for(uint32_t i = 0; i < static_cast<uint32_t>(renderJob.maInputRenderTargetAttachmentBufferAsImages.size()); i++)
            {
                if(renderJob.maInputRenderTargetAttachmentBufferAsImages[i] == renderJob.maInputRenderTargetAttachments[iInputAttachment])
                {
                    bSeparateImage = true;
                    break;
                }
            }

            std::ostringstream oss;
            uint64_t iTexturePtr = 0x0;
            std::string attachmentName = "";
            if(bSeparateImage)
            {
                RenderDriver::Common::CImage* pImage = pSerializer->getImage(renderJob.maInputRenderTargetAttachments[iInputAttachment]).get();
                
                if(pRenderer->getRenderDriverType() == Render::Common::RenderDriverType::Vulkan)
                {
                    iTexturePtr = getImageTexturePtrVulkan(pImage);
                }
                else
                {
                    // + 32 to offset the descriptor heap index from output's maximum number of textures
                    pRenderer->getImguiDescriptorHeap()->setImageView(pImage, iInputAttachment + 32 + 1, *pRenderer->getDevice());
                    iTexturePtr = pRenderer->getImguiDescriptorHeap()->getGPUHandle(iInputAttachment + 32 + 1, *pRenderer->getDevice());
                }

                auto const& attachmentInfo = renderJob.maAttachmentInfo[renderJob.maiInputAttachmentMapping[iInputAttachment]];
                attachmentName = attachmentInfo.mName;
                std::string attachmentOutput = attachmentName + " (" + attachmentInfo.mParentJobName + ")";

                auto const& imageDesc = pImage->getDescriptor();
                oss << attachmentOutput.c_str() << " (" << imageDesc.miWidth << " x " << imageDesc.miHeight << ")";

            }
            else
            {
                bool bIsImage = pSerializer->isImage(renderJob.maInputRenderTargetAttachments[iInputAttachment]);
                RenderDriver::Common::CImage* pImage = nullptr;
                if(bIsImage)
                {
                    pImage = pSerializer->getImage(renderJob.maInputRenderTargetAttachments[iInputAttachment]).get();
                }
                else
                {
                    if(pSerializer->hasRenderTarget(renderJob.maInputRenderTargetAttachments[iInputAttachment]))
                    {
                        auto& renderTarget = pSerializer->getRenderTarget(renderJob.maInputRenderTargetAttachments[iInputAttachment]);
                        if(renderTarget.get())
                        {
                            pImage = renderTarget->getImage().get();
                        }
                    }
                }
                if(pImage)
                {
                    if(pRenderer->getRenderDriverType() == Render::Common::RenderDriverType::Vulkan)
                    {
                        iTexturePtr = getImageTexturePtrVulkan(pImage);
                    }
                    else
                    {
                        // + 32 to offset the descriptor heap index from output's maximum number of textures
                        pRenderer->getImguiDescriptorHeap()->setImageView(pImage, iInputAttachment + 32 + 1, *pRenderer->getDevice());
                        iTexturePtr = pRenderer->getImguiDescriptorHeap()->getGPUHandle(iInputAttachment + 32 + 1, *pRenderer->getDevice());
                    }

                    auto const& attachmentInfo = renderJob.maAttachmentInfo[renderJob.maiInputAttachmentMapping[iInputAttachment]];
                    attachmentName = attachmentInfo.mName;
                    std::string attachmentOutput = attachmentName + " (" + attachmentInfo.mParentJobName + ")";

                    auto const& imageDesc = pImage->getDescriptor();
                    oss << attachmentOutput.c_str() << " (" << imageDesc.miWidth << " x " << imageDesc.miHeight << ")";
                }
                else
                {
                    auto const& attachmentInfo = renderJob.maAttachmentInfo[renderJob.maiInputAttachmentMapping[iInputAttachment]];
                    attachmentName = attachmentInfo.mName;
                    std::string attachmentOutput = attachmentName + " (" + attachmentInfo.mParentJobName + ")";
                    oss << attachmentOutput.c_str() << " buffer";
                }
            }
            
            ImGui::Text(oss.str().c_str());

            if(iTexturePtr > 0x0)
            {
                if(ImGui::ImageButton((ImTextureID)iTexturePtr, ImVec2(64, 64)))
                {
                    siSelectedOutputAttachment = iInputAttachment;
                    inputAttachmentName = attachmentName;
                    siSelectedInputAttachmentTexturePtr = iTexturePtr;
                }
            }
            else
            {
                if(ImGui::Button(oss.str().c_str()))
                {
                    siSelectedOutputAttachment = iInputAttachment;
                    inputAttachmentName = attachmentName;
                    siSelectedInputAttachmentTexturePtr = iTexturePtr;
                }
            }
        }
    }
    ImGui::End();

    if(inputAttachmentName.length() <= 0)
    {
        inputAttachmentName = "Input Attachment";
    }

    // Selected Attachment
    ImGui::Begin(inputAttachmentName.c_str());
    {
        if(siSelectedInputAttachmentTexturePtr != UINT64_MAX)
        {
            ImGui::Image((ImTextureID)siSelectedInputAttachmentTexturePtr, ImVec2(768, 768));
        }
    }
    ImGui::End();
#endif // #if 0
}

/*
**
*/
void debugOptionsLayout(Render::Common::CRenderer* pRenderer)
{
    char const* szTitle = "Restir Type";
    ImGui::Begin("Debug Options");
    {
        char const* aszItems[] = { "Temporal", "Spatial 0", "Spatial 1" };
        ImGui::Text(aszItems[siRestirType]);

        static char const* pItem = aszItems[0];
        if(ImGui::ListBoxHeader("Restir Pass"))
        {
            for(uint32_t i = 0; i < 3; i++)
            {
                bool bSelected = (i == siRestirType) ? true : false;
                if(ImGui::Selectable(aszItems[i], &bSelected))
                {
                    
                    siRestirType = i;
                }
            }
            ImGui::ListBoxFooter();
        }

        ImGui::SliderFloat("World Space Blur Radius", &sfWorldSpaceBlurRadius, 0.0f, 5.0f);
        if(sfWorldSpaceBlurRadius != sfLastWorldSpaceBlurRadius)
        {
            sfLastWorldSpaceBlurRadius = sfWorldSpaceBlurRadius;
        }

        ImGui::SliderInt("World Space Samples", &siWorldSpaceBlurSamples, 4, 64);
        if(siLastWorldSpaceBlurSamples != siWorldSpaceBlurSamples)
        {
            siLastWorldSpaceBlurSamples = siWorldSpaceBlurSamples;
        }

        ImGui::SliderFloat("Sky Light Intensity", &sfSunIntensity, 0.0f, 300.0f);
        if(sfLastSunIntensity != sfSunIntensity)
        {
            sfLastSunIntensity = sfSunIntensity;
        }

        ImGui::SliderFloat("Local Light Intensity", &sfLocalLightIntensity, 0.0f, 300.0f);
        if(sfLastLocalLightIntensity != sfLocalLightIntensity)
        {
            sfLastLocalLightIntensity = sfLocalLightIntensity;
        }

        ImGui::SliderFloat("Spatial Restir Radius 0", &sfSpatialRestirRadius0, 0.1f, 50.0f);
        if(sfLastSpatialRestirRadius0 != sfSpatialRestirRadius0)
        {
            sfLastSpatialRestirRadius0 = sfSpatialRestirRadius0;
        }

        ImGui::SliderFloat("Spatial Restir Radius 1", &sfSpatialRestirRadius1, 0.1f, 50.0f);
        if(sfLastSpatialRestirRadius1 != sfSpatialRestirRadius1)
        {
            sfLastSpatialRestirRadius1 = sfSpatialRestirRadius1;
        }

        ImGui::SliderInt("Spatial Restir Sample Count 0", &siSpatialRestirSampleCount0, 1, 64);
        if(siLastSpatialRestirSampleCount0 != siSpatialRestirSampleCount0)
        {
            siLastSpatialRestirSampleCount0 = siSpatialRestirSampleCount0;
        }

        ImGui::SliderInt("Spatial Restir Sample Count 1", &siSpatialRestirSampleCount1, 1, 64);
        if(siLastSpatialRestirSampleCount1 != siSpatialRestirSampleCount1)
        {
            siLastSpatialRestirSampleCount1 = siSpatialRestirSampleCount1;
        }

        ImGui::SliderFloat("Upscale Radius", &sfUpscaleRadius, 0.1f, 5.0f);
        if(sfLastUpscaleRadius != sfUpscaleRadius)
        {
            sfLastUpscaleRadius = sfUpscaleRadius;
        }

        ImGui::SliderFloat("Bilateral Blur Radius", &sfBilateralBlurRadius, 1.0f, 16.0f);
        if(sfLastBilateralBlurRadius != sfBilateralBlurRadius)
        {
           sfLastBilateralBlurRadius = sfBilateralBlurRadius;
        }

        ImGui::SliderFloat("Bilateral Blur Scale", &sfBilateralBlurScale, 1.0f, 5.0f);
        if(sfLastBilateralBlurScale != sfLastBilateralBlurScale)
        {
            sfLastBilateralBlurScale = sfLastBilateralBlurScale;
        }

        ImGui::SliderFloat("Bilateral Blur Color Sensitivity", &sfBilateralBlurColorSensitivty, 1.0f, 64.0f);
        if(sfLastBilateralBlurColorSensitivty != sfBilateralBlurColorSensitivty)
        {
            sfLastBilateralBlurColorSensitivty = sfBilateralBlurColorSensitivty;
        }

        ImGui::SliderFloat("Ambient Occlusion Blur Radius", &sfAmbientOcclusionBlurRadius, 1.0f, 5.0f);
        if(sfLastAmbientOcclusionBlurRadius != sfAmbientOcclusionBlurRadius)
        {
            sfLastAmbientOcclusionBlurRadius = sfAmbientOcclusionBlurRadius;
        }

        ImGui::SliderFloat("Ambient Occlusion Maximum Distance", &sfAmbientOcclusionMaxDistance, 0.01f, 5.0f);
        if(sfLastAmbientOcclusionMaxDistance != sfAmbientOcclusionMaxDistance)
        {
            sfLastAmbientOcclusionMaxDistance = sfAmbientOcclusionMaxDistance;
        }

        ImGui::SliderFloat("Material Roughness", &sfMaterialRoughness, 0.01f, 1.0f);
        if(sfLastMaterialRoughness != sfMaterialRoughness)
        {
            sfLastMaterialRoughness = sfMaterialRoughness;
        }


        ImGui::SliderFloat("Material Metalness", &sfMaterialMetalness, 0.0f, 1.0f);
        if(sfLastMaterialMetalness != sfMaterialMetalness)
        {
            sfLastMaterialMetalness = sfMaterialMetalness;
        }
    }
    ImGui::End();
}

/*
**
*/
void renderJobStatsLayout(Render::Common::CRenderer* pRenderer)
{
#if 0
    Render::Common::Serializer* pSerializer = pRenderer->getSerializer();
    auto const& aRenderJobs = pSerializer->getRenderJobs();

    ImGui::Begin("Render Job Stats");
    {
        auto const& aRenderJobExecTimes = pRenderer->getRenderJobExecTimes();
        auto const& aRenderJobs = pRenderer->getSerializer()->getRenderJobs();
        for(auto const& renderJob : aRenderJobs)
        {
            uint64_t iTimeElapsed = 0;
            for(auto const& keyValue : aRenderJobExecTimes)
            {
                if(keyValue.first == renderJob.mName)
                {
                    iTimeElapsed = keyValue.second.miTimeDuration;
                    break;
                }
            }

            std::ostringstream renderJobTimeOutput;
            renderJobTimeOutput << 
                renderJob.mName << 
                " " << 
                (float(iTimeElapsed) * 0.001f) << 
                " milliseconds " << 
                ((float(iTimeElapsed) / float(pRenderer->getTotalRenderJobsTime())) * 100.0f) <<
                " %%";
            ImGui::LabelText("", renderJobTimeOutput.str().c_str());
        }

        std::ostringstream renderJobTime;
        renderJobTime << "Total Render Jobs Time " << (float(pRenderer->getTotalRenderJobsTime()) * 0.001f) << " msec";
        ImGui::LabelText("", renderJobTime.str().c_str());
    }
    ImGui::End();
#endif // #if 0
}

/*
**
*/
void imguiLayout(Render::Common::CRenderer* pRenderer)
{
    std::ostringstream oss;
    
    pRenderer->beginDebugMarker("Top Window UI");
    ImGui::Begin("Toggle UI");
    {
        if(ImGui::Button("Toggle Debug Options UI"))
        {
            sbShowDebugOptionsUI = !sbShowDebugOptionsUI;
        }

        if(ImGui::Button("Toggle Render Job Input Attachments UI"))
        {
            sbShowInputAttachmentUI = !sbShowInputAttachmentUI;
        }

        if(ImGui::Button("Toggle Render Job Output Attachments UI"))
        {
            sbShowOutputAttachmentUI = !sbShowOutputAttachmentUI;
        }

        if(ImGui::Button("Toggle Render Job Stats"))
        {
            sbShowRenderJobStatsUI = !sbShowRenderJobStatsUI;
        }
    }
    ImGui::End();
    pRenderer->endDebugMarker();

    if(sbShowDebugOptionsUI)
    {
        pRenderer->beginDebugMarker("Debug Options UI");
        debugOptionsLayout(pRenderer);
        pRenderer->endDebugMarker();
    }

    if(sbShowOutputAttachmentUI)
    {
        pRenderer->beginDebugMarker("Output Attachment UI");
        outputAttachmentLayout(pRenderer);
        pRenderer->endDebugMarker();
    }

    if(sbShowInputAttachmentUI)
    {
        pRenderer->beginDebugMarker("Input Attachment UI");
        inputAttachmentLayout(pRenderer);
        pRenderer->endDebugMarker();
    }
    
    if(sbShowRenderJobStatsUI)
    {
        pRenderer->beginDebugMarker("Stats UI");
        renderJobStatsLayout(pRenderer);
        pRenderer->endDebugMarker();
    }
}