// WinAPI.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//
#include "framework.h"
#include "D3DX_Test1.h"

#define MAX_LOADSTRING 100

// 전역 변수:
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
HRESULT InitD3D(HWND);
HRESULT InitVertexBuffer();
HRESULT InitIndexBuffer();
void Cleanup();
void Render();
void SetupMatrices();
void Update();
void DrawMesh(const D3DXMATRIXA16& matrix);

LPDIRECT3D9 g_pD3D = NULL; // D3D Device를 생성할 D3D 객체 변수, 전역변수
LPDIRECT3DDEVICE9 g_pd3dDevice = NULL; // 렌더링에 사용될 D3D Device. 전역변수
LPDIRECT3DVERTEXBUFFER9 g_pVB = NULL; // 정점 버퍼, 전역변수
LPDIRECT3DINDEXBUFFER9 g_pIB = NULL; // 인덱스 버퍼 , 전역변수

D3DXMATRIXA16 g_matEarth, g_matEarth2, g_matMoon, g_matSun;

struct CUSTOMVERTEX
{
    float x, y, z; 
    DWORD color;
};

struct INDEX
{
    WORD _0, _1, _2;
};

#define D3DFVF_CUSTOM (D3DFVF_XYZ | D3DFVF_DIFFUSE)
#define Deg2Rad 0.017453293f

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 여기에 코드를 입력합니다.

    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_D3DXTEST1, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    HWND hWnd = CreateWindowW(szWindowClass, L"타이틀 이름!!"/*szTitle*/, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
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
                    ZeroMemory(&msg, sizeof(msg));
                    ULONGLONG frameTime, limitFrameTime = GetTickCount64();

                    while (WM_QUIT != msg.message)
                    {
                        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
                        {
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
    return 0;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDC_D3DXTEST1));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;//MAKEINTRESOURCEW(IDC_WINAPI); 메뉴 바를 사용하지 않음.
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = NULL;// LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

// Direct3D 초기화
HRESULT InitD3D(HWND hWnd)
{
    // Device를 생성하기 위한 D3D객체.
    if (NULL == (g_pD3D = Direct3DCreate9(D3D_SDK_VERSION))) return E_FAIL;

    // Device 생성을 위한 구조체
    // Present 는  백 버퍼의 내용을 스크린에 보여주는 작업
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp)); // 메모리의 쓰레기 값을 반드시 지워야함

    d3dpp.Windowed = TRUE; // 창 모드로 생성
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;  // 가장 효율적인 SWAP 효과. 백 <=> 프론트
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;  // 런타임에 현재 디스플레이 모드에 맞춰 백 버퍼 생성
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    // 멀티 샘플링
    DWORD level;
    for (auto type = (int)D3DMULTISAMPLE_16_SAMPLES; 0 < type; type--)
    {
        if (SUCCEEDED(g_pD3D->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_D16,
            FALSE, (D3DMULTISAMPLE_TYPE)type, &level)))
        {
            d3dpp.MultiSampleQuality = level - 1;
            d3dpp.MultiSampleType = (D3DMULTISAMPLE_TYPE)type;
        }
    }

    // Device 생성
    if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice)))
    {
        return E_FAIL;
    }

    g_pd3dDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, (0 < d3dpp.MultiSampleType)); // 멀티 샘플링
    g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW); // 컬링 모드 켜기
    g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE); // 광원 기능 끄기
    return S_OK;
}

HRESULT InitVertexBuffer()
{
    CUSTOMVERTEX vertices[] =
    {
        // 상자 그리기
        { -1, 1, 1, 0xffff0000 }, // v0
{ 1, 1, 1, 0xff00ff00 }, // v1
{ 1, 1, -1, 0xff0000ff }, // v2
{ -1, 1, -1, 0xffffff00 }, // v3
{ -1, -1, 1, 0xff00ffff }, // v4
{ 1, -1, 1, 0xffff00ff }, // v5
{ 1, -1, -1, 0xff000000 }, // v6
{ -1, -1, -1, 0xffffffff }, // v7
    };

    // 정점 버퍼 생성, 보관할 메모리 공간 할당
    if (FAILED(g_pd3dDevice->CreateVertexBuffer(sizeof(vertices), 0, D3DFVF_CUSTOM, D3DPOOL_DEFAULT,
        &g_pVB, NULL)))
    {
        return E_FAIL;
    }

    LPVOID pVertices;
    if (FAILED(g_pVB->Lock(0, sizeof(vertices), (void**)&pVertices, 0))) return E_FAIL;
    memcpy(pVertices, vertices, sizeof(vertices));
    g_pVB->Unlock();

    return S_OK;
}

HRESULT InitIndexBuffer()
{
    INDEX indices[] = 
    {
        { 0, 1, 2 }, { 0, 2, 3 }, // 윗면.
{ 4, 6, 5 }, { 4, 7, 6 }, // 아랫면.
{ 0, 3, 7 }, { 0, 7, 4 }, // 왼쪽면.
{ 1, 5, 6 }, { 1, 6, 2 }, // 오른쪽면.
{ 3, 2, 6 }, { 3, 6, 7 }, // 앞면.
{ 0, 4, 5 }, { 0, 5, 1 }, // 뒷면.
    };

    if (FAILED(g_pd3dDevice->CreateIndexBuffer(sizeof(indices), 0, D3DFMT_INDEX16,
        D3DPOOL_DEFAULT, &g_pIB, NULL))) return E_FAIL;

    LPVOID pIndices;
    if (FAILED(g_pIB->Lock(0, sizeof(indices), (void**)&pIndices, 0))) return E_FAIL;
    memcpy(pIndices, indices, sizeof(indices));
    g_pIB->Unlock();

    return S_OK;
}

// 초기화된 객체를 해제
void Cleanup()
{
    // 반드시 생성의 역순으로 해제
    if (NULL != g_pVB) g_pVB->Release();
    if (NULL != g_pd3dDevice) g_pd3dDevice->Release();
    if (NULL != g_pD3D) g_pD3D->Release();
    if (NULL != g_pIB) g_pIB->Release();
}

// 화면에 그리기
void Render()
{
    if (NULL == g_pd3dDevice) return;

    if (SUCCEEDED(g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0)))
    {
        SetupMatrices();
        // 렌더링 시작, 폴리곤 그리겠다고 D3D 에 알림 (BeginScene)
        if (SUCCEEDED(g_pd3dDevice->BeginScene()))
        {
            // 정점의 정보가 담긴 정점 버퍼를 출력 스트림으로 할당
            //g_pd3dDevice->SetStreamSource(0, g_pVB, 0, sizeof(CUSTOMVERTEX));

            //// D3D에 쉐이더 정보를 지정
            //g_pd3dDevice->SetFVF(D3DFVF_CUSTOM);

            //g_pd3dDevice->SetIndices(g_pIB);

            //g_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 8, 0, 12);

            // 계층구조
            DrawMesh(g_matSun);
            DrawMesh(g_matEarth);
            DrawMesh(g_matMoon);

            // 렌더링 종료
            g_pd3dDevice->EndScene();
        }

        // 백 버퍼를 보이는 화면으로 전환
        g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
    }
}

void Update()
{
    // 회전(자전) * 이동    
    // 크기 변경 * 이동 * 회전(공전) * 부모의 정보 추가.

    auto angle = GetTickCount64() * 0.001f;
    D3DXMATRIXA16 matSunTr, matSunRo, matSunSc;
    D3DXMATRIXA16 matEarthTr, matEarthRo, matEarthRo2, matEarthSc;
    D3DXMATRIXA16 matMoonTr, matMoonRo, matMoonSc;

    D3DXMatrixTranslation(&matSunTr, 0, 0, 0);
    D3DXMatrixScaling(&matSunSc, 1.0f, 1.0f, 1.0f);
    D3DXMatrixRotationY(&matSunRo, angle);
    g_matSun = matSunSc * matSunRo * matSunTr;


    D3DXMatrixRotationY(&matEarthRo, angle * 1.2f);
    D3DXMatrixRotationY(&matEarthRo2, angle * 0.5f);
    D3DXMatrixScaling(&matEarthSc, 0.5f, 0.5f, 0.5f);
    D3DXMatrixTranslation(&matEarthTr, 6, 0, 0);
    
    // 자전 공전 같이됨
    g_matEarth = matEarthSc * matEarthRo2 * matEarthTr * matEarthRo * g_matSun;

    D3DXMatrixRotationY(&matMoonRo, angle * 0.5f);
    D3DXMatrixScaling(&matMoonSc, 0.35f, 0.35f, 0.35f);
    D3DXMatrixTranslation(&matMoonTr, 5, 0, 0);
    g_matMoon = matMoonSc * matMoonTr * matMoonRo * g_matEarth;
}

void SetupMatrices()
{
    // 월드 스페이스
    D3DXMATRIXA16 matWorld;  // 월드 행렬
    D3DXMatrixIdentity(&matWorld); // 단위 행렬로 변경, 좌표 (0,0,0)

    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld); // 월드 스페이스로 변환

    // 뷰 스페이스
    D3DXVECTOR3 vEyePt(0.0f, 10.0f, -15.0f);  // 월드 좌표의 카메라 위치
    D3DXVECTOR3 vLookAtPt(0.0f, 0.0f, 0.0f);  // 월드 좌표의 카메라가 바라보는 위치
    D3DXVECTOR3 vUpVector(0.0f, 0.1f, 0.0f);  // 월드 좌표의 하늘 방향을 알기 위한 up vector

    D3DXMATRIXA16 matView;
    D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookAtPt, &vUpVector); // 뷰 변환 행렬 계산
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);  // 뷰 스페이스로 변환

    // 투영
    D3DXMATRIXA16 matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, 45 * Deg2Rad, 1.77f, 1.0f, 100.0f);
    g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);
}

void DrawMesh(const D3DXMATRIXA16& matrix)
{
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matrix); g_pd3dDevice->SetStreamSource(0, g_pVB, 0, sizeof(CUSTOMVERTEX));
    g_pd3dDevice->SetFVF(D3DFVF_CUSTOM);
    g_pd3dDevice->SetIndices(g_pIB);
    g_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 8, 0, 12);
}

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
