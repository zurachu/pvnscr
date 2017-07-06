/*
  * libfpk -- using FPK archive on P/ECE
  *   By Yui N., 2003 - 2005.
  * http://www.autch.net/
*/

/*
 * Merged by Yui N. 2005.03.21
 * pva 展開コードを組み込みました．
 > * Modified by ヅラChu at 2004/11/08
 > * pva形式展開に対応しました。
 > * 展開バッファの末尾に'\0'を追加しました（P/VNScripter仕様）。
 * データの可逆性を確保するため，pva 形式以外でバッファ末尾に \0 を付加するのは
 * 自分でやってください（ぉ
 *
 * Modified by ヅラChu at 2005/04/09
 * pva以外にも展開バッファの末尾に'\0'を追加しました（P/VNScripter仕様）。
 * P/VNScripterの方のソースいじるとおかしくなったのでこっちにしわ寄せ（ぉ
 */

#include "libfpk.h"
#include "hitodeLZSSDecoder.h"
#include "zlibDecode.h"

BYTE *fpkExtractToBuffer(HFPK hFpk, FPKENTRY *fpkEntry)
{
  if(hFpk == NULL || fpkEntry == NULL)
    return NULL;

  switch((BYTE)fpkEntry->szFileName[15])
  {
    case FPK_LZSS_COMPRESSION:
    case FPK_ZLIB_COMPRESSION:
    {
      BYTE *pBuffer = pceHeapAlloc(fpkEntry->dwSize);
      if(pBuffer == NULL)
        return NULL;

      pceFileReadPos(&hFpk->pfa, pBuffer, fpkEntry->dwOffset, fpkEntry->dwSize);

      {
        // fpk が連続セクタ上に有るとは限らないので，いったん中間バッファにコピー
        DWORD dwOriginalSize = *((DWORD *)pBuffer);
//        BYTE *pOutput = pceHeapAlloc(dwOriginalSize);
        BYTE *pOutput = pceHeapAlloc(dwOriginalSize + 1);
        if(pOutput == NULL)
        {
          pceHeapFree(pBuffer);
          return NULL;
        }
        if((BYTE)fpkEntry->szFileName[15] == FPK_LZSS_COMPRESSION)
          hitodeLZSSDecode(pOutput, pBuffer + 4, dwOriginalSize - 4);
        else
          zlibDecode(pBuffer + 4, pOutput);
        pceHeapFree(pBuffer);
        *(pOutput + dwOriginalSize) = '\0';
        return pOutput;
      }
    }
    case FPK_NO_COMPRESSION:
    default:
    {
//      BYTE *pBuffer = pceHeapAlloc(fpkEntry->dwSize);
      BYTE *pBuffer = pceHeapAlloc(fpkEntry->dwSize + 1);
      if(pBuffer == NULL)
        return NULL;

      pceFileReadPos(&hFpk->pfa, pBuffer, fpkEntry->dwOffset, fpkEntry->dwSize);

      if(*((DWORD *)pBuffer) == PVNSPACK_HEADER)  // pvnspack format
      {
        DWORD dwOriginalSize = *((DWORD *)(pBuffer + 28));
        BYTE *pOutput = pceHeapAlloc(dwOriginalSize + 1);
        if(pOutput == NULL)
        {
          pceHeapFree(pBuffer);
          return NULL;
        }
        zlibDecode(pBuffer + 36, pOutput);
        pceHeapFree(pBuffer);
        *(pOutput + dwOriginalSize) = '\0';
        return pOutput;
      }
      else
        *(pBuffer + fpkEntry->dwSize) = '\0';
        return pBuffer;
    }
  }
}
