#include <Windows.h>
#include <stdio.h>
#include <math.h>

#include "DIB.h"

//#define  _CRT_SECURE_NO_WARNINGS


LPBYTE DIBLoadHandle(const char* lpFileName)
{
	LPBYTE	lpDib = NULL;
	FILE*		fp;
	// rb : read binary(바이너리 형식으로 읽음)
	// 파일(이미지)를 연다.
	if (!fopen_s(&fp, lpFileName, "rb")) //성공하면 0을 반환.
	{
		BITMAPFILEHEADER	bmfh;
		// 반환 값이 count(1)보다 작으면 오류 또는 파일 끝.
		// 입력 stream에서 size 길이의 count 항목까지 읽고, 지정된 buffer에 저장.
		if (1 <= fread(&bmfh, sizeof(BITMAPFILEHEADER), 1, fp))
		{
			int nSize = bmfh.bfSize - sizeof(BITMAPFILEHEADER);
			// Allocation : 동적 할당.
			// GMEM_FIXED : 고정된 메모리 블럭을 할당.
			// 파일의 크기로 메모리 공간을 할당.
			if (HGLOBAL hDib = (HGLOBAL)GlobalAlloc(GMEM_FIXED, nSize))
			{
				// 메모리 락.
				if (lpDib = (LPBYTE)GlobalLock(hDib)) fread(lpDib, nSize, 1, fp);
			}
		}
		fclose(fp);
	}
	return lpDib;
}

VOID	DIBDeleteHandle(LPBYTE lpSrc)
{
	if (lpSrc)
	{
		GlobalUnlock(lpSrc);
		GlobalFree(lpSrc);
	}
}

BOOL IsPowerOf2Plus1(UINT n)
{
	if (0U == n) return FALSE;

	--n;
	return (0 == (n & (n - 1)));
}

BOOL IsInRect(LPRECT rc, POINT pt)
{
	return ((rc->left <= pt.x) && (rc->top >= pt.y) && (rc->right >= pt.x) && (rc->bottom <= pt.y));
}