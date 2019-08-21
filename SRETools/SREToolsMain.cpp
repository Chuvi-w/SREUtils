
#include <stdlib.h>

#include <stdio.h>
#include "CMemory.h"
#include <time.h>




void RTest(const CMemory &Mem)
{
    auto vRange=Mem.GetDataRange();
    printf("RANGE %lu\n",vRange.size());
	for(const auto &R:vRange)
	{
		printf("%08x:%08x\n", R.first, R.second);
	}
    return;
}


#define SRE_FILE_NAME "TestSre.sre"
void GenRandSREFile()
{
	srand(time(0));
	CMemory SRETool;
	uint32_t nMaxSections=rand()%5+5;
	uint32_t nCurSection=0;
	uint32_t nCurAddr=0, nCurSize=0;

	printf("Generating SRE file. Num sections=%u\n", nMaxSections);
	do 
	{
		nCurAddr=(nCurAddr+nCurSize)+(2+rand()%10)*0x10000;

		nCurSize=(2+rand()%4)*0x1000;
		uint8_t *pData=new uint8_t[nCurSize];

		for(int i=0; i<nCurSize/4; i++)
		{
			((uint32_t*)pData)[i]=((rand())<<16)^(~(rand()));
		}
		SRETool.SetData(nCurAddr, pData, nCurSize);
		delete[] pData;
		printf("Sec[%u]: %08x:%08x\n", nCurSection,nCurAddr,nCurAddr+nCurSize);

		nCurSection++;
	} while (nCurSection<nMaxSections);

	if(SRETool.WriteS19File("TestSre.sre", false, 8))
	{
		printf("File saved to %s\n\n\n", SRE_FILE_NAME);
	}
	else
	{
		printf("Unable to save %s\n\n\n", SRE_FILE_NAME);
	}
}

int main(int argc, const char *argv[])
{
	
	
	printf("Hi.\n");

	GenRandSREFile();

	CMemory SRETool;
	
    if(SRETool.ReadS19File(SRE_FILE_NAME))
    {
		printf("Load OK\n");
        RTest(SRETool);
    }
	else
	{
		printf("Load error\n");
	}


    return 0;

}

