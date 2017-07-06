/*
  * libfpk -- using FPK archive on P/ECE
  *   By Yui N., 2003 - 2005.
  * http://www.autch.net/
*/

/*
 * Merged by Yui N. 2005.03.21
 * pva �W�J�R�[�h��g�ݍ��݂܂����D
 > * Modified by �d��Chu at 2004/11/08
 > * pva�`���W�J�ɑΉ����܂����B
 > * �W�J�o�b�t�@�̖�����'\0'��ǉ����܂����iP/VNScripter�d�l�j�B
 * �f�[�^�̉t�����m�ۂ��邽�߁Cpva �`���ȊO�Ńo�b�t�@������ \0 ��t������̂�
 * �����ł���Ă��������i��
 *
 * Modified by �d��Chu at 2005/04/09
 * pva�ȊO�ɂ��W�J�o�b�t�@�̖�����'\0'��ǉ����܂����iP/VNScripter�d�l�j�B
 * P/VNScripter�̕��̃\�[�X������Ƃ��������Ȃ����̂ł������ɂ���񂹁i��
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
        // fpk ���A���Z�N�^��ɗL��Ƃ͌���Ȃ��̂ŁC�������񒆊ԃo�b�t�@�ɃR�s�[
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
