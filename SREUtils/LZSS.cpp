#include "LZSS.h"
#include <string.h>

typedef struct lzss_header_s
{
    unsigned int id;
    unsigned int actualSize; // always little endian
} lzss_header_t;

#define LZSS_ID 0x53535a4c/*((uint32_t)'LZSS')*/
#define LZSS_LOOKSHIFT 4
#define LZSS_LOOKAHEAD (1 << LZSS_LOOKSHIFT)

uint32_t LZSS_VGetActualSize(const std::vector<uint8_t> &vInput)
{
    lzss_header_t *pHeader=(lzss_header_t *) (&vInput[0]);
    if(pHeader&&vInput.size()>sizeof(lzss_header_t)&&pHeader->id==LZSS_ID)
    {
        return pHeader->actualSize;
    }
    return 0;
}

//-----------------------------------------------------------------------------
// Uncompress a buffer
//-----------------------------------------------------------------------------
std::vector<uint8_t> LZSS_VUncompress(const std::vector<uint8_t> &vInput)
{
    std::vector<uint8_t> vOut;

#define GetInput(v)                                       \
   if(nInputPos >= vInput.size())                         \
   { /*printf("LZSS_U:IN %p>=%p\n",_pInput,_pInputMax);*/ \
      vOut.clear();                                       \
      return vOut;                                        \
   }                                                      \
   else                                                   \
   {                                                      \
      v = vInput[nInputPos++];                            \
   }
    size_t nInputPos=sizeof(lzss_header_t); // skip header;
    int cmdByte=0;
    int getCmdByte=0;
    int i=0;
    int count;

    int position;
    uint8_t v;
    size_t nSource;
    uint32_t actualSize=LZSS_VGetActualSize(vInput);
    if(!actualSize)
    {
        vOut.clear();
        return vOut;
    }

    for(;;)
    {
        if(!getCmdByte)
        {
            GetInput(cmdByte);
        }
        getCmdByte=(getCmdByte+1)&0x07;

        if(cmdByte&0x01)
        {
            GetInput(v);
            position=v<<LZSS_LOOKSHIFT;
            GetInput(v);
            position|=(v>>LZSS_LOOKSHIFT);
            count=(v&0x0F)+1;
            if(count==1)
            {
                break;
            }
            nSource=vOut.size()-position-1;
            for(i=0; i<count; i++)
            {
                if(nSource>=0&&nSource<vOut.size())
                {
                    vOut.push_back(vOut[nSource++]);
                }
                else
                {
                    vOut.clear();
                    return vOut;;
                }
            }
        }
        else
        {
            GetInput(v);
            vOut.push_back(v);
        }
        cmdByte=cmdByte>>1;
    }

    if(vOut.size()!=actualSize)
    {
        // unexpected failure
        vOut.clear();
        return vOut;;
    }

    return vOut;
}

typedef struct lzss_node_s
{
    const uint8_t *pData;
    struct lzss_node_s *pPrev;
    struct lzss_node_s *pNext;
    // int8_t			empty[4];
} lzss_node_t;

typedef struct lzss_list_s
{
    lzss_node_t *pStart;
    lzss_node_t *pEnd;
} lzss_list_t;

#define DEFAULT_LZSS_WINDOW_SIZE (1 << 12)
static lzss_list_t g_pHashTable[256];
static lzss_node_t g_pHashTarget[DEFAULT_LZSS_WINDOW_SIZE];

void LZSS_BuildHash(const uint8_t *pData)
{
    lzss_list_t *pList;
    lzss_node_t *pTarget;

    int targetindex=(uint32_t) ((size_t) pData&(DEFAULT_LZSS_WINDOW_SIZE-1));
    pTarget=&g_pHashTarget[targetindex];
    if(pTarget->pData)
    {
        pList=&g_pHashTable[*pTarget->pData];
        if(pTarget->pPrev)
        {
            pList->pEnd=pTarget->pPrev;
            pTarget->pPrev->pNext=0;
        }
        else
        {
            pList->pEnd=0;
            pList->pStart=0;
        }
    }

    pList=&g_pHashTable[*pData];
    pTarget->pData=pData;
    pTarget->pPrev=0;
    pTarget->pNext=pList->pStart;
    if(pList->pStart)
    {
        pList->pStart->pPrev=pTarget;
    }
    else
    {
        pList->pEnd=pTarget;
    }
    pList->pStart=pTarget;
}

std::vector<uint8_t> LZSS_VCompress(const std::vector<uint8_t> vInput)
{
    std::vector<uint8_t> vOut;
    //#define AddOutput(v) if (pOutput >= pEnd){return 0;}else{*pOutput++ =v;}
    size_t nCmdBytePos=0;

    const void *pInput=&vInput[0];
    uint32_t inputLength=vInput.size();
    const uint8_t *pLookAhead, *pWindow, *pEncodedPosition;

    int putCmdByte, encodedLength, lookAheadLength, matchLength, length, i;
    lzss_node_t *pHash;

    if(inputLength<=sizeof(lzss_header_t)+8)
    {
        vOut.clear();
        return vOut;
    }

    memset(g_pHashTable, 0, sizeof(g_pHashTable));
    memset(g_pHashTarget, 0, sizeof(g_pHashTarget));
    lzss_header_t Header;
    Header.id=LZSS_ID;
    Header.actualSize=inputLength;
    vOut.insert(vOut.end(), reinterpret_cast<uint8_t *>(&Header), reinterpret_cast<uint8_t *>(&Header)+sizeof(Header));
    pLookAhead=(const uint8_t *) pInput;
    pWindow=(const uint8_t *) pInput;
    pEncodedPosition=0;

    putCmdByte=0;

    while(inputLength>0)
    {
        pWindow=pLookAhead-DEFAULT_LZSS_WINDOW_SIZE;
        if(pWindow<(const uint8_t *) pInput)
        {
            pWindow=(const uint8_t *) pInput;
        }

        if(!putCmdByte)
        {
            vOut.push_back(0);
            nCmdBytePos=vOut.size()-1;
        }
        putCmdByte=(putCmdByte+1)&0x07;

        encodedLength=0;
        lookAheadLength=inputLength<LZSS_LOOKAHEAD?inputLength:LZSS_LOOKAHEAD;

        pHash=g_pHashTable[pLookAhead[0]].pStart;
        while(pHash)
        {
            matchLength=0;
            length=lookAheadLength;
            while(length--&&pHash->pData[matchLength]==pLookAhead[matchLength])
            {
                matchLength++;
            }
            if(matchLength>encodedLength)
            {
                encodedLength=matchLength;
                pEncodedPosition=pHash->pData;
            }
            if(matchLength==lookAheadLength)
            {
                break;
            }
            pHash=pHash->pNext;
        }

        if(encodedLength>=3)
        {

            vOut[nCmdBytePos]=(vOut[nCmdBytePos]>>1)|0x80;
            vOut.push_back((pLookAhead-pEncodedPosition-1)>>LZSS_LOOKSHIFT);
            vOut.push_back(((pLookAhead-pEncodedPosition-1)<<LZSS_LOOKSHIFT)|(encodedLength-1));
        }
        else
        {
            encodedLength=1;
            vOut[nCmdBytePos]=(vOut[nCmdBytePos]>>1);
            vOut.push_back(*pLookAhead);
        }

        for(i=0; i<encodedLength; i++)
        {
            LZSS_BuildHash(pLookAhead++);
        }

        inputLength-=encodedLength;

        if(vOut.size()>=vInput.size())
        {
            // compression is worse, abandon
            vOut.clear();
            return vOut;
        }
    }

    if(inputLength!=0)
    {
        // unexpected failure
        // Assert(0);
        vOut.clear();
        return vOut;
    }

    if(!putCmdByte)
    {
        vOut.push_back(0x01);
        // pCmdByte = pOutput-1;
        nCmdBytePos=vOut.size()-1;
    }
    else
    {
        vOut[nCmdBytePos]=((vOut[nCmdBytePos]>>1)|0x80)>>(7-putCmdByte);
    }
    vOut.push_back(0);
    vOut.push_back(0);
    return vOut;
}