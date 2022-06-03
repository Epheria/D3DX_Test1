#pragma once

#ifndef _DIB_H_
#define _DIB_H_

#define ALIGN_4B( a )					((a) % 4 ? (a) + 4 - ((a) % 4) : (a)) // 4byte로 맞춘다.
#define DIB_HEADER( a )				((LPBITMAPINFOHEADER)(a))
#define DIB_CX( a )						(DIB_HEADER(a)->biWidth)
#define DIB_CY( a )						(DIB_HEADER(a)->biHeight)
#define DIB_BPP( a )						(DIB_HEADER(a)->biBitCount) // BPP : 픽셀당 비트 수.
#define DIB_PALSIZE( a )				(DIB_BPP(a) == 24 ? 0 : sizeof(RGBQUAD) * (1 << DIB_BPP(a))) // PAL : 컬러 인코딩 방식.
#define DIB_DATA( a )					((LPBYTE)(a) + sizeof(BITMAPINFOHEADER) + DIB_PALSIZE(a))
#define DIB_DATAXY_INV( a, x, y )	(DIB_DATA(a) + (x) * DIB_BPP(a) / 8 + (DIB_CY(a) - (y) - 1) * ALIGN_4B( DIB_CX(a) * DIB_BPP(a) / 8 )) // 0~255 사이 값을 반환. INV : 아마 Inverter


#ifndef DEL
#define DEL(p)		{ if(p) { delete (p); (p) = NULL; } }
#endif

#ifndef DELS
#define DELS(p)		{ if(p) { delete[] (p); (p) = NULL; } }
#endif

#ifndef REL
#define REL(p)		{ if(p) { (p)->Release(); (p) = NULL; } }
#endif

template<typename T>
T Repeat(T value, const T& max) { return (value > max) ? Repeat(value - (max + 1), max) : value; }

//DIB Device Independent Bitmap(디바이스 독립적 비트맵)
extern LPBYTE	DIBLoadHandle(const char* lpFileName);
extern VOID		DIBDeleteHandle(LPBYTE lpSrc);

BOOL IsPowerOf2Plus1(UINT n);

#define IS_IN_RANGE(value, r0, r1) ((((r0) <= (value)) && ((value) <= (r1))) ? 1 : 0)
BOOL IsInRect(LPRECT rc, POINT pt);

#endif