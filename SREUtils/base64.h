//
//  base64 encoding and decoding with C++.
//  Version: 1.01.00
//

#ifndef BASE64_H_C0CE2A47_D10E_42C9_A27C_C883944E704A
#define BASE64_H_C0CE2A47_D10E_42C9_A27C_C883944E704A

#include <string>
#include <array>
#include <vector>
#if 0
#define CONSTEXPR constexpr
#else
#define CONSTEXPR
#endif
namespace BASE64
{
    CONSTEXPR const char szBase64Chars[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    CONSTEXPR const char szPaddingChar='=';

    CONSTEXPR size_t GetOutStrLen(size_t DataLen)
    {
        return (((4ll*DataLen/3ll)+3ll)&~3ll)+1; /*Including '/0';*/ }
    /*CONSTEXPR size_t GetStrLen(const char *Str){size_t j = 0;while (Str&&*Str++) { j++; }return j;}*/

    CONSTEXPR size_t GetDecodeLen(const char *Str)
    {
        size_t StrLen=0;
        size_t DataLen=0;
        size_t nPad=0;
        while(Str&&Str[StrLen])
        {
            StrLen++;
        }
        if(StrLen>0)
        {
            DataLen=(StrLen/4ll*3);
            while(Str[StrLen-1-nPad]==szPaddingChar)
            {
                nPad++;
            }
            DataLen-=nPad;
        }
        return DataLen;
    }

    CONSTEXPR bool IsBase64Char(unsigned char c)
    {
        if((c>='0'&&c<='9')||(c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c=='+')||(c=='/'))
        {
            return true;
        }
        return false;
    }

    CONSTEXPR unsigned char GetCharPos(unsigned char c)
    {
        if(IsBase64Char(c))
        {
            for(int i=0; i<sizeof(szBase64Chars); i++)
            {
                if(c==szBase64Chars[i])
                {
                    return i;
                }
            }
        }
        return 0xFF;
    }

    inline std::string Encode(const void *pDataBuf, size_t nBuffSize)
    {

        std::string ret;
        size_t i=0, j=0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];
        ret.resize(GetOutStrLen(nBuffSize));

        size_t nNumBlocks=nBuffSize/3;
        size_t nRemBytes=nBuffSize%3;

        for(int64_t nBlock3=0; nBlock3<nNumBlocks; nBlock3++)
        {
            unsigned char i_char_array_3[3];
            i_char_array_3[0]=*(const unsigned char *) ((size_t) (pDataBuf)+nBlock3*3+0);
            i_char_array_3[1]=*(const unsigned char *) ((size_t) (pDataBuf)+nBlock3*3+1);
            i_char_array_3[2]=*(const unsigned char *) ((size_t) (pDataBuf)+nBlock3*3+2);
            ret[nBlock3*4+0]=szBase64Chars[(i_char_array_3[0]&0xfc)>>2];
            ret[nBlock3*4+1]=szBase64Chars[((i_char_array_3[0]&0x03)<<4)+((i_char_array_3[1]&0xf0)>>4)];
            ret[nBlock3*4+2]=szBase64Chars[((i_char_array_3[1]&0x0f)<<2)+((i_char_array_3[2]&0xc0)>>6)];
            ret[nBlock3*4+3]=szBase64Chars[i_char_array_3[2]&0x3f];
        }

        if(nRemBytes)
        {
            for(i=0; i<nRemBytes; i++)
            {
                char_array_3[i]=reinterpret_cast<const unsigned char *>(pDataBuf)[nNumBlocks*3+i];
            }
            for(j=i; j<3; j++)
            {
                char_array_3[j]='\0';
            }
            char_array_4[0]=(char_array_3[0]&0xfc)>>2;
            char_array_4[1]=((char_array_3[0]&0x03)<<4)+((char_array_3[1]&0xf0)>>4);
            char_array_4[2]=((char_array_3[1]&0x0f)<<2)+((char_array_3[2]&0xc0)>>6);

            for(j=0; (j<i+1); j++)
            {
                ret[nNumBlocks*4+j]=szBase64Chars[char_array_4[j]];
            }

            while((i++<3))
            {
                ret[nNumBlocks*4+i]=szPaddingChar;
            }
        }
        ret[nNumBlocks*4+i]=0;
        return ret;
    }

    inline std::vector<uint8_t> Decode(const char *szEncodedString)
    {
        size_t vOutSize=GetDecodeLen(szEncodedString);
        size_t i=0, j=0;
        unsigned char char_array_4[4], char_array_3[3];
        std::vector<uint8_t> vOut(vOutSize);
        size_t nNumBlocks=vOutSize/3;
        size_t nRemBytes=vOutSize%3;

        for(int64_t nBlock4=0; nBlock4<nNumBlocks; nBlock4++)
        {
            char_array_4[0]=GetCharPos(szEncodedString[nBlock4*4+0]);
            char_array_4[1]=GetCharPos(szEncodedString[nBlock4*4+1]);
            char_array_4[2]=GetCharPos(szEncodedString[nBlock4*4+2]);
            char_array_4[3]=GetCharPos(szEncodedString[nBlock4*4+3]);
            vOut[nBlock4*3+0]=(char_array_4[0]<<2)+((char_array_4[1]&0x30)>>4);
            vOut[nBlock4*3+1]=((char_array_4[1]&0xf)<<4)+((char_array_4[2]&0x3c)>>2);
            vOut[nBlock4*3+2]=((char_array_4[2]&0x3)<<6)+char_array_4[3];
        }

        if(nRemBytes)
        {
            for(j=0; j<nRemBytes+1; j++)
            {
                char_array_4[j]=GetCharPos(szEncodedString[nNumBlocks*4+j]);
            }
            char_array_3[0]=(char_array_4[0]<<2)+((char_array_4[1]&0x30)>>4);
            char_array_3[1]=((char_array_4[1]&0xf)<<4)+((char_array_4[2]&0x3c)>>2);

            for(j=0; j<nRemBytes; j++)
            {
                vOut[nNumBlocks*3+j]=char_array_3[j];
            }
        }
        return vOut;
    }
}

#endif /* BASE64_H_C0CE2A47_D10E_42C9_A27C_C883944E704A */
