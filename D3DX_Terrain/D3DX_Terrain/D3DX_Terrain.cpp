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
#define PLANE_EPSILON 0.001f // 절두체에 정확하게 포함되지 않더라도, 약간의 여분을 주어서 포함시키기 위한 값


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

// Frustum Culling 관련
LPD3DXVECTOR3 g_pTerrainVertices = NULL;
D3DXVECTOR3 g_FrustumVertices[8];
D3DXPLANE g_FrustumPlanes[6];
void MakeFrustum(LPD3DXMATRIXA16 viewProj);
bool IsInFrustum(LPD3DXVECTOR3 v);
bool IsInFrustumBoundsSphere(LPD3DXVECTOR3 center, float radius);

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

class QuadTree
{
private:
    enum class Location {
        FRUSTUM_OUT = 0,            // 절두체의 완전한 외부
        FRUSTUM_PARTIALLY_IN = 1,   // 절두체의 부분적 내,외부
        FRUSTUM_COMPLETELY_IN = 2,  // 절두체의 완전한 내부
        UNKNOWN = -1                // 알 수 없는 상태
    };
    enum CornerType { CornerType_TL = 0, CornerType_TR, CornerType_BL, CornerType_BR };
    int center;
    int corners[4];
    bool isCulled;
    float radius; // 경계구의 반지름
    QuadTree* childs[4];
public:
    QuadTree(int cx, int cy);
    ~QuadTree();
private:
    QuadTree();
    VOID SetCorners(int cornerTL, int cornerTR, int cornerBL, int cornerBR);
    QuadTree* AddChild(int cornerTL, int cornerTR, int cornerBL, int cornerBR);
    BOOL SubDivided(); // 4개의 하위 노드로 분할
    BOOL IsVisible() { return (1 >= (corners[CornerType_TR] - corners[CornerType_TL])); }
    // pIndex : 폴리곤(triangle) 인덱스 정보 output
    // return : 그리게 될 폴리곤(trinagle)의 수
    INT GenerateIndices(LPVOID pIndices, int triangles = 0);
    Location FrustumLocation(LPD3DXVECTOR3 pVertices);
    VOID ProcessFrustumCull(LPD3DXVECTOR3 pVertices);
public:
    VOID Build(LPD3DXVECTOR3 pVertices); // 쿼드 트리 구축
    INT GetIndices(LPVOID pIndices, LPD3DXVECTOR3 pVertices);
};
QuadTree* q_pQuadTree = NULL;

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
    g_pTerrainVertices = new D3DXVECTOR3[g_vertexCount];

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
            vertex.postion.x = (-0.5f + xPos) * 10/*1 => 10*/;
            vertex.postion.z = (0.5f - zPos) * 10/*1 => 1-*/;
            if (pDIB) vertex.postion.y = (float)(*(DIB_DATAXY_INV(pDIB, x, z))) * 1 /*y size*/ * 0.003022f;
            else vertex.postion.y = 0;
            D3DXVec3Normalize(&vertex.normal, &vertex.postion);
            vertex.tex.x = xPos;
            vertex.tex.y = zPos;
            int index = x + z * g_cx;
            vertices[index] = vertex;
            
            g_pTerrainVertices[index] = vertex.postion;
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
            if (SUCCEEDED(g_pd3dDevice->CreateIndexBuffer((g_cx - 1) * (g_cz - 1) * 2 * sizeof(INDEX), 0, D3DFMT_INDEX32, D3DPOOL_DEFAULT, &g_pIB, NULL)))
            {
                q_pQuadTree = new QuadTree(g_cx, g_cz);
                q_pQuadTree->Build(g_pTerrainVertices);
            }
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
    if (NULL != q_pQuadTree) delete q_pQuadTree;
    if (NULL != g_pTerrainVertices) delete[] g_pTerrainVertices;
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
    D3DXVECTOR3 vEyePt(0.0f, 3.0f, -5.0f); // 월드 좌표의 카메라 위치
    D3DXVECTOR3 vLookAtPt(0.0f, 0.0f, 0.0f); // 월드 좌표의 카메라가 바라보는 위치
    D3DXVECTOR3 vUpVector(0.0f, 1.0f, 0.0f); // 월드 좌표의 하늘 방향을 알기 위한 업 벡터

    if (GetAsyncKeyState(VK_LEFT))
    {
        vEyePt.x -= 0.01f;
        vLookAtPt.x -= 0.01f;
    }
    else if (GetAsyncKeyState(VK_RIGHT))
    {
        vEyePt.x += 0.01f;
        vLookAtPt.x += 0.01f;
    }
    if (GetAsyncKeyState(VK_UP))
    {
        vEyePt.z += 0.01f;
        vLookAtPt.z += 0.01f;
    }
    else if (GetAsyncKeyState(VK_DOWN))
    {
        vEyePt.z -= 0.01f;
        vLookAtPt.z -= 0.01f;
    }

    D3DXMATRIXA16 matView;
    D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookAtPt, &vUpVector); // 뷰 변환 행렬 계산
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView); // 뷰 스페이스로 변환

    // 투영
    D3DXMATRIXA16 matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, 45 * Deg2Rad, 1.77f, 1.0f, 100.0f);
    g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);

    if (GetKeyState(VK_NUMPAD1))
    {
        D3DXMatrixPerspectiveFovLH(&matProj, 45 * Deg2Rad * 0.5f, 1.77f, 0.5f, 5);
        matView._43 -= 0.6f;
    }
    D3DXMATRIXA16 matViewProj = matView * matProj;
    MakeFrustum(&matViewProj);
}

void Update()
{
    if (NULL == g_pIB || NULL == q_pQuadTree || NULL == g_pTerrainVertices) return;
    LPDWORD pl;
    const int row = (g_cx - 1), column = (g_cz - 1);
    if (SUCCEEDED(g_pIB->Lock(0, (g_cx - 1) * (g_cz - 1) * 2 * sizeof(INDEX), (void**)&pl, 0)))
    {
        g_indexCount = q_pQuadTree->GetIndices(pl, g_pTerrainVertices);
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

QuadTree::QuadTree(int cx, int cy) : center(0), isCulled(false), radius(0) {
    for (int i = 0; 4 > i; i++)
    {
        corners[i] = 0;
        childs[i] = NULL;
    }
    
    if (!(IsPowerOf2Plus1(cx) && IsPowerOf2Plus1(cy))) return; // cx, cy가 (2^n)+1 인지 확인
    // [n = 1] cx = 3, cy = 3
    // TL          TR // n = 2, cx = 5, cy = 5
    // 0 --- 1 --- 2 // 00-01-02-03-04
    // |     |     | // 05-06-07-08-09
    // 3 --- 4 --- 5 // 10-11-12-13-14
    // |     |     | // 15-16-17-18-19
    // 6 --- 7 --- 8 // 20-21-22-23-24
    // BL          BR
    // center = 4 = ( 0 + 2 + 6 + 8) / 4
    SetCorners(0, (cx - 1), (cx * (cy - 1)), (cx * cy - 1));
}

QuadTree::QuadTree() : center(0), isCulled(false), radius(0) {
    for (int i = 0; 4 > i; i++)
    {
        corners[i] = 0;
        childs[i] = NULL;
    }
}
QuadTree::~QuadTree() { for (int i = 0; 4 > i; i++) DEL(childs[i]); }

VOID QuadTree::Build(LPD3DXVECTOR3 pVertices)
{
    if (SubDivided())
    {
        // 경계구의 반지름을 구하기 위해, 영역의 대각선 길이(경계구의 지름)를 구한다.
        D3DXVECTOR3 v = (pVertices[corners[CornerType_TL]] - pVertices[corners[CornerType_BR]]);
        radius = D3DXVec3Length(&v) * 0.5f;
    }
}

INT QuadTree::GetIndices(LPVOID pIndices, LPD3DXVECTOR3 pVertices)
{
    ProcessFrustumCull(pVertices);
    return GenerateIndices(pIndices);
}

QuadTree* QuadTree::AddChild(int cornerTL, int cornerTR, int cornerBL, int cornerBR)
{
    QuadTree* child = new QuadTree();
    child->SetCorners(cornerTL, cornerTR, cornerBL, cornerBR);
    return child;
}

BOOL QuadTree::SubDivided()
{
    // 간격이 1보다 작거나 같을 때까지 분할 한다.
    if (1 >= (corners[CornerType_TR] - corners[CornerType_TL])) return FALSE;
    int topCenter = (corners[CornerType_TL] + corners[CornerType_TR]) * 0.5f;
    int bottomCenter = (corners[CornerType_BL] + corners[CornerType_BR]) * 0.5f;
    int leftCenter = (corners[CornerType_TL] + corners[CornerType_BL]) * 0.5f;
    int rightCenter = (corners[CornerType_TR] + corners[CornerType_BR]) * 0.5f;
    childs[CornerType_TL] = AddChild(corners[CornerType_TL], topCenter, leftCenter, center);
    childs[CornerType_TR] = AddChild(topCenter, corners[CornerType_TR], center, rightCenter);
    childs[CornerType_BL] = AddChild(leftCenter, center, corners[CornerType_BL], bottomCenter);
    childs[CornerType_BR] = AddChild(center, rightCenter, bottomCenter, corners[CornerType_BR]);
    return TRUE;
}


VOID QuadTree::SetCorners(int cornerTL, int cornerTR, int cornerBL, int cornerBR)
{
    corners[CornerType_TL] = cornerTL;
    corners[CornerType_TR] = cornerTR;
    corners[CornerType_BL] = cornerBL;
    corners[CornerType_BR] = cornerBR;
    center = (corners[CornerType_TL] + corners[CornerType_TR] + corners[CornerType_BL] + corners[CornerType_BR]) * 0.25f;
}

VOID QuadTree::ProcessFrustumCull(LPD3DXVECTOR3 pVertices)
{
    switch (FrustumLocation(pVertices))
    {
    case Location::FRUSTUM_OUT: isCulled = true; return;
    case Location::FRUSTUM_COMPLETELY_IN: isCulled = false; return;
    case Location::FRUSTUM_PARTIALLY_IN: isCulled = false; break;
    default: return;
    }

    if (childs[CornerType_TL]) childs[CornerType_TL]->ProcessFrustumCull(pVertices);
    if (childs[CornerType_TR]) childs[CornerType_TR]->ProcessFrustumCull(pVertices);
    if (childs[CornerType_BL]) childs[CornerType_BL]->ProcessFrustumCull(pVertices);
    if (childs[CornerType_BR]) childs[CornerType_BR]->ProcessFrustumCull(pVertices);
}

QuadTree::Location QuadTree::FrustumLocation(LPD3DXVECTOR3 pVertices)
{
    // 경계구를 이용하여 해당 영역이 절두체 밖에 있는지 확인(완전한 외부)
    if (!IsInFrustumBoundsSphere(&pVertices[center], radius)) return Location::FRUSTUM_OUT;

    bool b[4]; // 영역 전체가 절두체 안에 있는지 확인 (완전한 내부)
    b[0] = IsInFrustum(&pVertices[corners[CornerType_TL]]);
    b[1] = IsInFrustum(&pVertices[corners[CornerType_TR]]);
    b[2] = IsInFrustum(&pVertices[corners[CornerType_BL]]);
    b[3] = IsInFrustum(&pVertices[corners[CornerType_BR]]);
    if (b[0] & b[1] & b[2] & b[3]) return Location::FRUSTUM_COMPLETELY_IN;

    return Location::FRUSTUM_PARTIALLY_IN; // 부분적 내 외부
}

INT QuadTree::GenerateIndices(LPVOID pIndices, int triangles)
{
    if (isCulled)
    {
        isCulled = FALSE;
        return triangles;
    }

    if (IsVisible())
    {
        LPDWORD pIdx = ((LPDWORD)pIndices) + triangles * 3;
        (*pIdx++) = corners[CornerType_TL];
        (*pIdx++) = corners[CornerType_TR];
        (*pIdx++) = corners[CornerType_BL];
        triangles++;
        (*pIdx++) = corners[CornerType_BL];
        (*pIdx++) = corners[CornerType_TR];
        (*pIdx++) = corners[CornerType_BR];
        triangles++;
        return triangles;
    }

    if (childs[CornerType_TL]) triangles = childs[CornerType_TL]->GenerateIndices(pIndices, triangles);
    if (childs[CornerType_TR]) triangles = childs[CornerType_TR]->GenerateIndices(pIndices, triangles);
    if (childs[CornerType_BL]) triangles = childs[CornerType_BL]->GenerateIndices(pIndices, triangles);
    if (childs[CornerType_BR]) triangles = childs[CornerType_BR]->GenerateIndices(pIndices, triangles);

    return triangles;

}

void MakeFrustum(LPD3DXMATRIXA16 viewProj)
{
    if (NULL == viewProj) return;
    // 투영행렬까지 거치면 모든 3차원 월드좌표의 점은 (-1,-1,0) ~ (1,1,1)사이의 값으로 바뀐다.
    g_FrustumVertices[0] = { -1.0f, -1.0f, 0.0f };// near left bottom
    g_FrustumVertices[1] = { 1.0f, -1.0f, 0.0f };// near right bottom
    g_FrustumVertices[2] = { 1.0f, -1.0f, 1.0f };// far right bottom
    g_FrustumVertices[3] = { -1.0f, -1.0f, 1.0f };// far left bottom
    g_FrustumVertices[4] = { -1.0f, 1.0f, 0.0f };// near left top
    g_FrustumVertices[5] = { 1.0f, 1.0f, 0.0f }; // near right top
    g_FrustumVertices[6] = { 1.0f, 1.0f, 1.0f }; // far right top
    g_FrustumVertices[7] = { -1.0f, 1.0f, 1.0f };// far left top
    // view * Proj의 역행렬을 구한다.
    D3DXMATRIXA16 inv;
    D3DXMatrixInverse(&inv, NULL, viewProj);
    // 최종Vertex = WorldVertex * ViewMatrix * ProjectionMatrix 에서,
    // 역행렬[(ViewMatrix * ProjectionMatrix)^-1]을 양변에 곱하면,
    // 최종Vertex * 역행렬 = Vertex_World 가 된다.
    // 그러므로, frustumVerties * inv = WorldVertex가 되어,
    // 월드 좌표계의 절두체 좌표를 얻을 수 있다.
    for (int i = 0; 8 > i; i++) D3DXVec3TransformCoord(&g_FrustumVertices[i], &g_FrustumVertices[i], &inv);
    // 월드 좌표를 이용하여 법선 벡터가 절두체 안쪽에서 바깥쪽으로 나가는 평면을 만든다. 상, 하면은 제외.
    //D3DXPlaneFromPoints(&g_FrustumPlanes[0], &g_FrustumVerties[4], &g_FrustumVerties[7], &g_FrustumVerties[6]);// 상 평면(top)
    //D3DXPlaneFromPoints(&g_FrustumPlanes[1], &g_FrustumVerties[0], &g_FrustumVerties[1], &g_FrustumVerties[2]);// 하 평면(bottom)
    D3DXPlaneFromPoints(&g_FrustumPlanes[2], &g_FrustumVertices[0], &g_FrustumVertices[4], &g_FrustumVertices[5]);// 근 평면(near)
    D3DXPlaneFromPoints(&g_FrustumPlanes[3], &g_FrustumVertices[2], &g_FrustumVertices[6], &g_FrustumVertices[7]);// 원 평면(far)
    D3DXPlaneFromPoints(&g_FrustumPlanes[4], &g_FrustumVertices[0], &g_FrustumVertices[3], &g_FrustumVertices[7]);// 좌 평면(left)
    D3DXPlaneFromPoints(&g_FrustumPlanes[5], &g_FrustumVertices[1], &g_FrustumVertices[5], &g_FrustumVertices[6]);// 우 평면(right)
}

bool IsInFrustum(LPD3DXVECTOR3 v)
{
    if (NULL == v) return FALSE;
    float dist;
    for (int i = 2; i < 6; i++) // 현재는 left, right, near, far plane만 적용
    {
        // 평면과 벡터(v)의 내적을 구한다.
        // // p⋅v == 0 직교.
        // // p⋅v > 0 내각이 90보다 작다(절두체 밖에 위치)
        // p⋅v < 0 내각이 90보다 크다(절두체 안에 위치)
        dist = D3DXPlaneDotCoord(&g_FrustumPlanes[i], v);
        if (PLANE_EPSILON < dist) return FALSE;
    }
    return TRUE;
}

bool IsInFrustumBoundsSphere(LPD3DXVECTOR3 center, float radius)
{
    if (NULL == center || 0 >= radius) return FALSE;
    float dist;
    for (int i = 2; i < 6; i++) // 현재는 left, right, near, far plane만 적용
    {
        dist = D3DXPlaneDotCoord(&g_FrustumPlanes[i], center);
        if ((radius + PLANE_EPSILON) < dist) return FALSE; // 평면과 중심점의 거리가 반지름보다 크면 절두체 안에 없음.
    }
    return TRUE;
}