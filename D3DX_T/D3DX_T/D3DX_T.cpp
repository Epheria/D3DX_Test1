// D3DX_T.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "framework.h"
#include "D3DX_T.h"

#define MAX_LOADSTRING 100


// 전역 변수:
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

LPDIRECT3D9 g_pD3D = NULL; // D3D Device를 생성할 D3D 객체 변수. 전역변수
LPDIRECT3DDEVICE9 g_pd3dDevice = NULL; // 렌더링에 사용될 D3D Device 전역 변수
LPDIRECT3DVERTEXBUFFER9 g_pVB = NULL; // 정점 버퍼

struct CUSTOMVERTEX
{
    float x, y, z; // 정점의 좌표
    DWORD color; // 정점의 색깔
};
#define D3DFVF_CUSTOM (D3DFVF_XYZ | D3DFVF_DIFFUSE)

HRESULT InitD3D(HWND);
void Cleanup();
void Render();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 여기에 코드를 입력합니다.

    // 전역 문자열을 초기화합니다.
    //LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_D3DXT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    HWND hWnd = CreateWindowW(szWindowClass, L"타이틀 이름", WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    if (SUCCEEDED(InitD3D(hWnd)))
    {
        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);

        MSG msg;
        // 기본 메시지 루프입니다:
        while (true)
        {
            if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
            {
                if (WM_QUIT == msg.message) break;

                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                Render();
            }
        }
    }

    return 0;
}



//
//  함수: MyRegisterClass()
//
//  용도: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_D3DXT));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName = NULL;//MAKEINTRESOURCEW(IDC_D3DXT);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 애플리케이션 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        Cleanup();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

//Direct3D 초기화.
HRESULT InitD3D(HWND hWnd)
{
    // Device를 생성하기 위한 D3D 객체.
    if (NULL == (g_pD3D = Direct3DCreate9(D3D_SDK_VERSION))) return E_FAIL;

    // Device 생성을 위한 구조체.
    // Present는 백 버퍼의 내용을 스크린에 보여주는 작업을 한다.
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp)); // 메모리의 쓰레기 값을 반드시 지워야 한다.

    d3dpp.Windowed = TRUE; // 창모드로 생성
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD; // 가장 효율적인 SWAP 효과. 백 버퍼의 내용을 프론트 버퍼로 Swap하는 방식.
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // 런타임에, 현재 디스플레이 모드에 맞춰 백 버퍼를 생성.

    // Device 생성.
    //D3DADAPTER_DEFAULT : 기본 그래픽 카드를 사용
    //D3DDEVTYPE_HAL : 하드웨어 가속장치를 지원한다(그래픽카드 제조사에서 DirectX가 세부적인 부분을 제어할 필요가 없도록 지원하는 기능)
    //D3DCREATE_SOFTWARE_VERTEXPROCESSING : 정점(Vertex) 쉐이더 처리를 소프트웨어에서 한다(하드웨어에서 할 경우 더욱 높은 성능을 발휘 : D3DCREATE_HARDWARE_VERTEXPROCESSING)
    if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice)))
    {
        return E_FAIL;
    }

    // TODO: 여기에서 Device 상태 정보 처리를 처리한다.
    g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW); // 컬링 모드를 켠다
    g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE); // 광원 기능을 끈다.
    return S_OK;
}

void Cleanup()
{
    // 반드시 생성의 역순으로 해제
    if (NULL != g_pd3dDevice) g_pd3dDevice->Release();
    if (NULL != g_pD3D) g_pD3D->Release();
}

// 화면에 그리기
void Render()
{
    if (NULL == g_pd3dDevice) return;

    // pRects(두 번째 인자)가 NULL 이라면 Count(첫 번째 인자)는 0.
    // D3DCLEAR_TARGET:surface를 대상으로 한다.
    // D3DCLEAR_ZBUFFER:Z(깊이) 버퍼를 대상으로 한다.
    // 버퍼를 파란색[RGB(0,0,255)]으로 지운다.
    // Z(깊이) 버퍼를 1로 지운다. (0~1 사이의값을 사용)
    // 스텐실(Stencil) 버퍼를 0으로 지운다. (0 ~ 2^n-1 사이의 값을 사용)
    // 다음과 같은 경우 IDirect3DDevice9 :: Clear() 함수가 실패.
    // - 깊이 버퍼가 연결되지 않은 렌더링 대상의 깊이 버퍼 또는 스텐실 버퍼를 지운다.
    // - 깊이 버퍼에 스텐실 데이터가 포함되지 않은 경우 스텐실 버퍼를 지운다.
    if (SUCCEEDED(g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0)))
    {
        // 렌더링 시작, 폴리곤을 그리겠다고 D3D에 알림(BeginScene).
        if (SUCCEEDED(g_pd3dDevice->BeginScene()))
        {
            // 렌더링 종료.
            g_pd3dDevice->EndScene();
        }

        // 백 버퍼를 보이는 화면으로 전환.
        g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
    }
}
