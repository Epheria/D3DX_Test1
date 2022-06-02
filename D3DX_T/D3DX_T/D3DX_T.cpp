// D3DX_T.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

#include <d3d9.h>
#include <d3dx9.h>

#include "framework.h"
#include "D3DX_T.h"
#include "DIB.h"

#define MAX_LOADSTRING 100
#define Deg2Rad 0.017453293f


// 전역 변수:
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

LPDIRECT3D9 g_pD3D = NULL; // D3D Device를 생성할 D3D 객체 변수. 전역변수
LPDIRECT3DDEVICE9 g_pd3dDevice = NULL; // 렌더링에 사용될 D3D Device 전역 변수
LPDIRECT3DINDEXBUFFER9 g_pIB = NULL; // 인덱스 버퍼
LPDIRECT3DVERTEXBUFFER9 g_pVB = NULL;
LPDIRECT3DTEXTURE9 g_pEarthTexture = NULL;
LPDIRECT3DTEXTURE9 g_pMoonTexture = NULL;
LPDIRECT3DTEXTURE9 g_pSunTexture = NULL;

D3DXMATRIXA16 g_matEarth, g_matMoon, g_matSun;

struct INDEX
{
    DWORD _0, _1, _2;
};
#define D3DFVF_CUSTOM (D3DFVF_XYZ | D3DFVF_NORMAL)

struct CUSTOMVERTEX
{
    float x, y, z; // 정점의 좌표
    D3DXVECTOR3 normal;
    D3DXVECTOR2 tex;
};
#define D3DFVF_CUSTOM (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)

HRESULT InitD3D(HWND);
HRESULT InitIndexBuffer();
HRESULT InitVertexBuffer();
void SetupMatrices();
void Cleanup();
void Render();
void Update();
void DrawMesh(const D3DXMATRIXA16& matrix);

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

    HWND hWnd = CreateWindowW(szWindowClass, L"D3D", WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    if (SUCCEEDED(InitD3D(hWnd)))
    {
        if (SUCCEEDED(InitVertexBuffer()))
        {
            if (SUCCEEDED(InitIndexBuffer()))
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
                        Update();
                        Render();
                    }
                }
            }
        }
    }
    Cleanup();

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
    d3dpp.EnableAutoDepthStencil = TRUE; // D3D에서 프로그램의 깊이 버퍼(Z-Buffer)를 관리하게 한다.
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16; // 16비트의 깊이 버퍼 사용

    DWORD level;
    for (auto type = (int)D3DMULTISAMPLE_16_SAMPLES; 0 < type; type--)
    {
        if (SUCCEEDED(g_pD3D->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_D16, FALSE, (D3DMULTISAMPLE_TYPE)type, &level)))
        {
            d3dpp.MultiSampleQuality = level - 1;
            d3dpp.MultiSampleType = (D3DMULTISAMPLE_TYPE)type;
            break;
        }
    }

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
    g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE); // 깊이 버퍼 기능을 켠다.
    g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE); // 광원 기능을 끈다.
    g_pd3dDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, (0 < d3dpp.MultiSampleType));

    D3DLIGHT9 light; // Direct 3D 9 조명 구조체 변수
    ZeroMemory(&light, sizeof(light));
    light.Type = D3DLIGHTTYPE::D3DLIGHT_DIRECTIONAL;
    light.Diffuse = { 1, 1, 1, 0.5f }; 
    D3DXVECTOR3 dir = { 1, -1, 1 };
    D3DXVec3Normalize((D3DXVECTOR3*)&light.Direction, &dir);
    g_pd3dDevice->SetLight(0, &light);
    g_pd3dDevice->LightEnable(0, TRUE);
    g_pd3dDevice->SetRenderState(D3DRS_AMBIENT, 0x00040404);
    D3DXCreateTextureFromFile(g_pd3dDevice, L"earth.png", &g_pEarthTexture);
    D3DXCreateTextureFromFile(g_pd3dDevice, L"moon.jpg", &g_pMoonTexture);
    D3DXCreateTextureFromFile(g_pd3dDevice, L"sun.png", &g_pSunTexture);

    D3DMATERIAL9 mtrl;
    ZeroMemory(&mtrl, sizeof(mtrl));
    mtrl.Diffuse = mtrl.Ambient = { 1, 1, 1, 1 };
    g_pd3dDevice->SetMaterial(&mtrl);
    return S_OK;
}

void Cleanup()
{
    // 반드시 생성의 역순으로 해제
    if (NULL != g_pd3dDevice) g_pd3dDevice->Release();
    if (NULL != g_pD3D) g_pD3D->Release();
    if (NULL != g_pIB) g_pIB->Release();
    if (NULL != g_pVB) g_pVB->Release();
    if (NULL != g_pEarthTexture) g_pEarthTexture->Release();
    if (NULL != g_pMoonTexture) g_pMoonTexture->Release();
    if (NULL != g_pSunTexture) g_pSunTexture->Release();
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
    if (SUCCEEDED(g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0)))
    {
        SetupMatrices();
        // 렌더링 시작, 폴리곤을 그리겠다고 D3D에 알림(BeginScene).
        if (SUCCEEDED(g_pd3dDevice->BeginScene()))
        {
            g_pd3dDevice->SetTexture(0, g_pSunTexture);
            DrawMesh(g_matSun);
            g_pd3dDevice->SetTexture(0, g_pEarthTexture);
            DrawMesh(g_matEarth);
            g_pd3dDevice->SetTexture(0, g_pMoonTexture);
            DrawMesh(g_matMoon);
            // 렌더링 종료.
            g_pd3dDevice->EndScene();
        }

        // 백 버퍼를 보이는 화면으로 전환.
        g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
    }
}

HRESULT InitVertexBuffer()
{
    CUSTOMVERTEX vertices[24];

    for (int i = 0; 2 > i; i++)
    {
        int index = i * 4;
        vertices[index] = { -0.5f, 0.5f - i, 0.5f, D3DXVECTOR3(-0.5f, 0.5f - i, 0.5f), D3DXVECTOR2(0.25f, i) };
        vertices[index + 1] = { -0.5f, 0.5f - i, -0.5f, D3DXVECTOR3(-0.5f, 0.5f - i, -0.5f), D3DXVECTOR2(0.25f, i * 0.333333f + 0.333333f) };
        vertices[index + 2] = { 0.5f, 0.5f - i, -0.5f, D3DXVECTOR3(0.5f, 0.5f - i, -0.5f), D3DXVECTOR2(0.5f, i * 0.333333f + 0.333333f) };
        vertices[index + 3] = { 0.5f, 0.5f - i, 0.5f, D3DXVECTOR3(0.5f, 0.5f - i, 0.5f), D3DXVECTOR2(0.5f, i) };
    }
    for (int i = 0; 4 > i; i++)
    {
        
        int index = (i + 2) * 4;
        vertices[index] = vertices[Repeat(i, 3)]; // left, top
        vertices[index].tex = D3DXVECTOR2(i * 0.25f, 0.333333f);
        vertices[index + 1] = vertices[Repeat(i, 3) + 4]; // left, bottom
        vertices[index + 1].tex = D3DXVECTOR2(i * 0.25f, 0.666666f);
        vertices[index + 2] = vertices[Repeat(i + 1, 3) + 4]; // right, bottom
        vertices[index + 2].tex = D3DXVECTOR2((i + 1) * 0.25f, 0.666666f);
        vertices[index + 3] = vertices[Repeat(i + 1, 3)]; // right, top
        vertices[index + 3].tex = D3DXVECTOR2((i + 1) * 0.25f, 0.333333f);
    }

    if (FAILED(g_pd3dDevice->CreateVertexBuffer(sizeof(vertices), 0, D3DFVF_CUSTOM, D3DPOOL_DEFAULT, &g_pVB, NULL)))
    {
        return E_FAIL;
    }
    // 정점

    LPVOID pVertices;
    if (FAILED(g_pVB->Lock(0, sizeof(vertices), (void**)&pVertices, 0))) return E_FAIL;
    memcpy(pVertices, vertices, sizeof(vertices));
    g_pVB->Unlock();
    return S_OK;
}

HRESULT InitIndexBuffer()
{
    INDEX indices[12];
    for (int i = 0; 6 > i; i++)
    {
        int index = i * 2;
        DWORD value = i * 4;
        indices[index] = { value, value + 3, value + 2 };
        indices[index + 1] = { value, value + 2, value + 1 };
    }

    indices[2] = { 5, 6, 7 };
    indices[3] = { 5, 7, 4 };

    if (FAILED(g_pd3dDevice->CreateIndexBuffer(sizeof(indices), 0, D3DFMT_INDEX32, D3DPOOL_DEFAULT, &g_pIB, NULL))) return E_FAIL;

    LPVOID pIndices;
    if (FAILED(g_pIB->Lock(0, sizeof(indices), (void**)&pIndices, 0))) return E_FAIL;
    memcpy(pIndices, indices, sizeof(indices));
    g_pIB->Unlock();

    return S_OK;
}

void SetupMatrices()
{
    // 월드 스페이스
    D3DXMATRIXA16 matWorld; // 월드 행렬
    D3DXMatrixIdentity(&matWorld); // 단위 행렬로 변경, 좌표를 (0,0,0)변경
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);

    // 뷰 스페이스
    D3DXVECTOR3 vEyePt(0.0f, 3.0f, -10.0f); // 월드 좌표의 카메라 위치
    D3DXVECTOR3 vLookAtPt(0.0f, 0.0f, 0.0f); // 월드 좌표의 카메라가 바라보는 위치
    D3DXVECTOR3 vUpVector(0.0f, 1.0f, 0.0f); // 월드 좌표의 하늘 방향을 알기 위한 업 벡터

    D3DXMATRIXA16 matView;
    D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookAtPt, &vUpVector); // 뷰 변환 행렬 계산
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView); // 뷰 스페이스로 변환

    // 투영
    D3DXMATRIXA16 matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, 45 * Deg2Rad, 1.77f, 1.0f, 100.0f);
    g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);
}

void Update()
{
    auto angle = GetTickCount64() * 0.001f;
    D3DXMATRIXA16 matSunTr, matSunRo;
    D3DXMATRIXA16 matEarthTr, matEarthRo, matEarthSc;
    D3DXMATRIXA16 matMoonTr, matMoonRo, matMoonSc;

    D3DXMatrixTranslation(&matSunTr, 0, 0, 0);
    D3DXMatrixRotationY(&matSunRo, angle);
    g_matSun = matSunRo * matSunTr;

    D3DXMatrixTranslation(&matEarthTr, 5, 0, 0);
    D3DXMatrixRotationY(&matEarthRo, angle * 0.1f);
    //D3DXMatrixScaling(&matEarthSc, 0.8f, 0.8f, 0.8f);
    // 회전(자전) * 이동
    g_matEarth = matEarthRo * matEarthTr * matEarthRo * g_matSun;

    D3DXMatrixRotationY(&matMoonRo, angle);
    D3DXMatrixScaling(&matMoonSc, 0.5f, 0.5f, 0.5f);
    D3DXMatrixTranslation(&matMoonTr, 3, 0, 0);
    // 크기 변경 * 이동 * 회전(공전) * 부모의 정보 추가
    g_matMoon = matMoonRo * matMoonSc * matMoonTr * matMoonRo * g_matEarth;
}

void DrawMesh(const D3DXMATRIXA16& matrix)
{
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matrix);
    g_pd3dDevice->SetStreamSource(0, g_pVB, 0, sizeof(CUSTOMVERTEX));
    g_pd3dDevice->SetFVF(D3DFVF_CUSTOM);
    g_pd3dDevice->SetIndices(g_pIB);
    g_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 24, 0, 12);
}
