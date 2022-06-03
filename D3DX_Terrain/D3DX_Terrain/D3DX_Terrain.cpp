// D3DX_T.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

#include <d3d9.h>
#include <d3dx9.h>

#include "framework.h"
#include "D3DX_Terrain.h"
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
LPDIRECT3DTEXTURE9 g_pTexture = NULL;
int g_vertexCount, g_indexCount;
int g_cx = 3, g_cz = 3; // 가로, 세로 정점의 수

struct INDEX
{
    DWORD _0, _1, _2;
};
#define D3DFVF_CUSTOM (D3DFVF_XYZ | D3DFVF_NORMAL)

struct CUSTOMVERTEX
{
    D3DXVECTOR3 postion;
    D3DXVECTOR3 normal;
    D3DXVECTOR2 tex;
};
#define D3DFVF_CUSTOM (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)

HRESULT InitD3D(HWND);
HRESULT CreateVIB();
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
    LoadStringW(hInstance, IDC_D3DXTERRAIN, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    HWND hWnd = CreateWindowW(szWindowClass, L"D3D", WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    if (SUCCEEDED(InitD3D(hWnd)))
    {
        if (SUCCEEDED(CreateVIB()))
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

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDC_D3DXTERRAIN));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;//MAKEINTRESOURCEW(IDC_D3DXT);
    wcex.lpszClassName = szWindowClass;
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
    D3DXCreateTextureFromFile(g_pd3dDevice, L"tile.tga", &g_pTexture);
    
    D3DMATERIAL9 mtrl;
    ZeroMemory(&mtrl, sizeof(mtrl));
    mtrl.Diffuse = mtrl.Ambient = { 1, 1, 1, 1 };
    g_pd3dDevice->SetMaterial(&mtrl);
    return S_OK;
}

HRESULT CreateVIB()
{
    LPBYTE pDIB = NULL;
    if (pDIB = DIBLoadHandle("heightMap129.bmp")) // 높이 맵의 정보를 가져오기
    {
        g_cx = DIB_CX(pDIB); // 비트맵 이미지의 픽셀 사이즈를 구한다
        g_cz = DIB_CY(pDIB);
    }

    g_vertexCount = g_cx * g_cz;
    CUSTOMVERTEX* vertices = new CUSTOMVERTEX[g_vertexCount];
    if (!vertices) return E_FAIL;

    CUSTOMVERTEX vertex;
    const int row = (g_cx - 1);
    const int column = (g_cz - 1);
    const float unitRow = 1.0f / row;
    const float unitColumn = 1.0f / column;

    float xPos, zPos;
    for (int z = 0; g_cz > z; z++)
    {
        for (int x = 0; g_cx > x; x++)
        {
            xPos = x * unitRow;
            zPos = z * unitColumn;
            vertex.postion.x = (-0.5f + xPos) * 1/*x size*/;
            vertex.postion.z = (0.5f - zPos) * 1/*z size*/;
            if (pDIB) vertex.postion.y = (float)(*(DIB_DATAXY_INV(pDIB, x, z))) * 1 /*y size*/;
            else vertex.postion.y = 0;
            D3DXVec3Normalize(&vertex.normal, &vertex.postion);
            vertex.tex.x = xPos;
            vertex.tex.y = zPos;
            int index = x + z * g_cx;
            vertices[index] = vertex;
        }
    }
    if (pDIB) DIBDeleteHandle(pDIB);

    if (SUCCEEDED(g_pd3dDevice->CreateVertexBuffer(g_vertexCount * sizeof(CUSTOMVERTEX), 0, D3DFVF_CUSTOM, D3DPOOL_DEFAULT, &g_pVB, NULL)))
    {
        VOID* pVertices;
        if (SUCCEEDED(g_pVB->Lock(0, g_vertexCount * sizeof(CUSTOMVERTEX), (void**)&pVertices, 0)))
        {
            memcpy(pVertices, vertices, g_vertexCount * sizeof(CUSTOMVERTEX));
            g_pVB->Unlock();

            //IB 생성
            g_pd3dDevice->CreateIndexBuffer(row * column * 2 * sizeof(INDEX), 0, D3DFMT_INDEX32, D3DPOOL_DEFAULT, &g_pIB, NULL);
        } // g_pVB->Lock
    } // CreateVertexBuffer

    DELS(vertices);

    return S_OK;
}

void Cleanup()
{
    // 반드시 생성의 역순으로 해제
    if (NULL != g_pd3dDevice) g_pd3dDevice->Release();
    if (NULL != g_pD3D) g_pD3D->Release();
    if (NULL != g_pIB) g_pIB->Release();
    if (NULL != g_pVB) g_pVB->Release();
    if (NULL != g_pTexture) g_pTexture->Release();
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
            D3DXMATRIXA16 world;
            D3DXMatrixIdentity(&world);
            DrawMesh(world);
            // 렌더링 종료.
            g_pd3dDevice->EndScene();
        }

        // 백 버퍼를 보이는 화면으로 전환.
        g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
    }
}

void SetupMatrices()
{
    // 월드 스페이스
    D3DXMATRIXA16 matWorld; // 월드 행렬
    D3DXMatrixIdentity(&matWorld); // 단위 행렬로 변경, 좌표를 (0,0,0)변경
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);

    // 뷰 스페이스
    D3DXVECTOR3 vEyePt(0.0f, 10.0f, -10.0f); // 월드 좌표의 카메라 위치
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
    if (NULL == g_pIB) return;
    LPDWORD pl;
    const int row = (g_cx - 1), column = (g_cz - 1);
    if (SUCCEEDED(g_pIB->Lock(0, row * column * 2 * sizeof(INDEX), (void**)&pl, 0)))
    {
        g_indexCount = 0;
        DWORD index[4];
        for (int z = 0; column > z; z++)
        {
            for (int x = 0; row > x; x++)
            {
                // 하나의 정점당 두 개의 폴리곤 index를 가진다.
                index[0] = x + z * g_cx;
                index[1] = index[0] + 1;
                index[2] = index[1] + g_cx;
                index[3] = index[0] + g_cx;
                (*pl++) = index[0];
                (*pl++) = index[1];
                (*pl++) = index[2];
                g_indexCount++;
                (*pl++) = index[0];
                (*pl++) = index[2];
                (*pl++) = index[3];
                g_indexCount++;
            }
        }
        g_pIB->Unlock();
    }
}

void DrawMesh(const D3DXMATRIXA16& matrix)
{
    g_pd3dDevice->SetTexture(0, g_pTexture);
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matrix);
    g_pd3dDevice->SetStreamSource(0, g_pVB, 0, sizeof(CUSTOMVERTEX));
    g_pd3dDevice->SetFVF(D3DFVF_CUSTOM);
    g_pd3dDevice->SetIndices(g_pIB);
    g_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, g_vertexCount, 0, g_indexCount);
}
