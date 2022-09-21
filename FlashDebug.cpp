/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2022                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  ����CH347 SPI�ӿں�������FLASHӦ��ʾ����FLASH �ͺ�ʶ�𡢿������д���������FLASH���ݶ����ļ����ļ�
  д��FLASH���ٶȲ��ԵȲ���������SPI�����ٶȿɴ�2M�ֽ�/S

Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2022 Nanjing Qinheng Microelectronics Co., Ltd.
Revision History:
  4/3/2022: TECH30
--*/

#include "Main.h"

#define DevID_Mode1_2 "USB\\VID_1A86&PID_55D"

extern HINSTANCE AfxMainIns; //����ʵ��
extern ULONG Flash_Sector_Count; // FLASHоƬ������ 
extern USHORT Flash_Sector_Size; // FLASHоƬ������С
extern HWND AfxActiveHwnd;
//ȫ�ֱ���
HWND FlashEepromDbgHwnd; //������
BOOL FlashDevIsOpened;   //�豸�Ƿ��
BOOL IsSpiInit;     //SPIʹ��ǰ���SPI���г���
ULONG DevIndex;
mDeviceInforS FlashDevInfor[16] = {0};

//ʹ�ܲ�����ť�����ȴ򿪺�����,�����޷�����
VOID FlashDlg_EnableButtonEnable();

//FLASH�ͺ�ʶ��
BOOL FlashIdentify()
{
	double BT,UsedT;
	BOOL RetVal;

	if(!FlashDevIsOpened)
	{
		DbgPrint("���ȴ��豸");
		return FALSE;
	}
	DbgPrint(">>FLASHоƬ�ͺż��...");
	BT = GetCurrentTimerVal();
	RetVal = FLASH_IC_Check();
	UsedT = GetCurrentTimerVal()-BT;
	DbgPrint(">>FLASHоƬ�ͺż��...%s,��ʱ:%.3fS",RetVal?"�ɹ�":"ʧ��",UsedT);

	return RetVal;
}

//��ʼ��SPI��������Ƭѡ����
BOOL Flash_InitSpi()
{	
	BOOL RetVal = FALSE;
	mSpiCfgS SpiCfg = {0};

	CH347SPI_GetCfg(DevIndex,&SpiCfg); //��ȡӲ��SPI��ǰ����
	SpiCfg.iMode = (UCHAR)SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Mode,CB_GETCURSEL,0,0);
	SpiCfg.iClock = (UCHAR)SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Clock,CB_GETCURSEL,0,0);
	SpiCfg.iByteOrder = 1; //MSB
	SpiCfg.iSpiOutDefaultData = 0Xff;

	RetVal = CH347SPI_Init(DevIndex,&SpiCfg);    //����SPI
	CH347SPI_SetChipSelect(DevIndex,0x0001,0,0,0,0); //ʹ�ܲ�����CS1��ΪƬѡ�ź�
	FlashIdentify(); //ʶ���������ϵ�W24Q64оƬ�ͺ�

	DbgPrint("Flash_Init %s",RetVal?"succ":"failure");

	return RetVal;
}


//FLASH�ֽڶ�
BOOL FlashBlockRead()
{
	double BT,UseT;
	ULONG DataLen,FlashAddr=0,i;
	UCHAR DBuf[8192] = {0};
	CHAR FmtStr[512] = "",FmtStr1[8*1024*3+16]="";

	if(!FlashDevIsOpened)
	{
		DbgPrint("���ȴ��豸");
		return FALSE;
	}

	//��ȡFLASH������ʼ��ַ
	GetDlgItemText(FlashEepromDbgHwnd,IDC_FlashStartAddr,FmtStr,32);
	FlashAddr = mStrToHEX(FmtStr);
	//��ȡFLASH�����ֽ���,ʮ������
	GetDlgItemText(FlashEepromDbgHwnd,IDC_FlashDataSize,FmtStr,32);
	DataLen = mStrToHEX(FmtStr);
	if(DataLen<1)
	{
		DbgPrint("������Flash���ݲ�������");
		return FALSE;
	}
	else if(DataLen>(8*1024*3)) //��ʾ����
	{
		DbgPrint("������С��0x%X��Flash���ݲ�������",8*1024);
		return FALSE;
	}

	BT = GetCurrentTimerVal();
	DataLen = FLASH_RD_Block(FlashAddr,DBuf,DataLen);
	UseT = GetCurrentTimerVal()-BT;

	if(DataLen<1)
		DbgPrint(">>Flash��:��[%X]��ַ��ʼ����%d�ֽ�...ʧ��.",FlashAddr,DataLen);
	else
	{	
		DbgPrint(">>Flash��:��[%X]��ַ��ʼ����%d�ֽ�...�ɹ�.��ʱ%.3fS",FlashAddr,DataLen,UseT/1000);
		{//��ʾFLASH����,16������ʾ
			for(i=0;i<DataLen;i++)		
				sprintf(&FmtStr1[strlen(FmtStr1)],"%02X ",DBuf[i]);						
			SetDlgItemText(FlashEepromDbgHwnd,IDC_FlashData,FmtStr1);
		}
	}
	return TRUE;
}

//Flash������д
BOOL FlashBlockWrite()
{
	ULONG DataLen,FlashAddr=0,i,StrLen;
	UCHAR DBuf[8*1024+16] = {0};
	CHAR FmtStr[8*1024*3+16] = "",ValStr[16]="";
	double BT,UseT;

	//�Ȳ���
	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_FlashErase,BM_CLICK,0,0);

	//��ȡдFLASH����ʼ��ַ,ʮ������
	GetDlgItemText(FlashEepromDbgHwnd,IDC_FlashStartAddr,FmtStr,32);
	FlashAddr = mStrToHEX(FmtStr);				

	//��ȡдFLASH���ֽ���,ʮ������
	DataLen = 0;
	StrLen = GetDlgItemText(FlashEepromDbgHwnd,IDC_FlashData,FmtStr,sizeof(FmtStr));	
	for(i=0;i<StrLen;i+=3)
	{		
		memcpy(&ValStr[0],&FmtStr[i],2);

		DBuf[DataLen] = (UCHAR)mStrToHEX(ValStr);
		DataLen++;
	}
	
	BT = GetCurrentTimerVal();
	DataLen = FLASH_WR_Block(FlashAddr,DBuf,DataLen);
	UseT = GetCurrentTimerVal()-BT;
	if(DataLen<1)
		DbgPrint(">>Flashд:��[%X]��ַ��ʼд��%d�ֽ�...ʧ��",FlashAddr,DataLen);
	else
		DbgPrint(">>Flashд:��[%X]��ַ��ʼд��%d�ֽ�...�ɹ�.��ʱ%.3fS",FlashAddr,DataLen,UseT/1000);

	return TRUE;
}

//FLASH�����
BOOL FlashBlockErase()
{
	ULONG DataLen,FlashAddr=0;
	CHAR FmtStr[128] = "";
	double BT,UseT;
	BOOL RetVal;

	//��ȡ����FLASH����ʼ��ַ,ʮ������
	GetDlgItemText(FlashEepromDbgHwnd,IDC_FlashStartAddr,FmtStr,32);
	FlashAddr = mStrToHEX(FmtStr);
	//��ȡ����FLASH���ֽ���,ʮ������
	GetDlgItemText(FlashEepromDbgHwnd,IDC_FlashDataSize,FmtStr,32);
	DataLen = mStrToHEX(FmtStr);

	BT = GetCurrentTimerVal();
	RetVal = FLASH_Erase_Sector(FlashAddr);
	UseT = GetCurrentTimerVal()-BT;
	if( !RetVal )
		DbgPrint(">>FLASH����:[%X]...ʧ��",FlashAddr);
	else
		DbgPrint(">>FLASH����:[%X]...�ɹ�,��ʱ%.3fS",FlashAddr,UseT/1000);

	return TRUE;
}

//��ʾ�������ݳ�������
VOID DumpDataBuf(ULONG Addr,PUCHAR Buf1,PUCHAR SampBuf2,ULONG DataLen1,ULONG ErrLoca)
{
	CHAR FmtStr1[8192*3] = "",FmtStr2[8192*3] = "";
	ULONG i;

	memset(FmtStr1,0,sizeof(FmtStr1));	
	memset(FmtStr2,0,sizeof(FmtStr2));	
	//16������ʾ
	for(i=0;i<DataLen1;i++)
	{
		if( ((i%16)==0) && (i>0) )
		{
			AddStrToEdit(FlashEepromDbgHwnd,IDC_InforShow,"Data[%08X]:%s\n",i-16+Addr,FmtStr1);
			AddStrToEdit(FlashEepromDbgHwnd,IDC_InforShow,"Samp[%08X]:%s\n",i-16+Addr,FmtStr2);
			memset(FmtStr1,0,16*4);
			memset(FmtStr2,0,16*4);
			if(ErrLoca==i)
			{
				sprintf(&FmtStr1[strlen(FmtStr1)],"[%02X] ",Buf1[i]);						
				sprintf(&FmtStr2[strlen(FmtStr2)],"[%02X] ",SampBuf2[i]);						
			}
			else
			{
				sprintf(&FmtStr1[strlen(FmtStr1)],"%02X ",Buf1[i]);						
				sprintf(&FmtStr2[strlen(FmtStr2)],"%02X ",SampBuf2[i]);						
			}
		}
		else
		{
			if(ErrLoca==i)
			{
				sprintf(&FmtStr1[strlen(FmtStr1)],"[%02X] ",Buf1[i]);						
				sprintf(&FmtStr2[strlen(FmtStr2)],"[%02X] ",SampBuf2[i]);						
			}
			else
			{
				sprintf(&FmtStr1[strlen(FmtStr1)],"%02X ",Buf1[i]);						
				sprintf(&FmtStr2[strlen(FmtStr2)],"%02X ",SampBuf2[i]);						
			}

		}
	}
	AddStrToEdit(FlashEepromDbgHwnd,IDC_InforShow,"Data[%08X]:%s\r\n",(i%16)?(i-i%16+Addr):(i-16+Addr),FmtStr1);
	AddStrToEdit(FlashEepromDbgHwnd,IDC_InforShow,"Samp[%08X]:%s\r\n",(i%16)?(i-i%16+Addr):(i-16+Addr),FmtStr2);
}

//FLASH���ԣ��Ȳ�����д����������
DWORD WINAPI FlashRWSpeedTest(LPVOID lpParameter)
{
	ULONG FlashAddr = 0,TC,TestLen,i,k,BlockSize=0;
	double BT = 0;
	BOOL RetVal = FALSE;
	CHAR FileName[MAX_PATH]="";
	HANDLE hFile=INVALID_HANDLE_VALUE;
	PUCHAR FileBuf=NULL,RBuf=NULL;
	ULONG FileSize,RLen,UnitSize,Addr,WLen;
	double UsedT = 0;
	ULONG Timeout,PrintLen,ErrLoca;
	UCHAR temp;
	OPENFILENAME mOpenFile={0};
	
	EnableWindow(FlashEepromDbgHwnd,FALSE);

	// Fill in the OPENFILENAME structure to support a template and hook.
	mOpenFile.lStructSize = sizeof(OPENFILENAME);
	mOpenFile.hwndOwner         = FlashEepromDbgHwnd;
	mOpenFile.hInstance         = AfxMainIns;		
	mOpenFile.lpstrFilter       = "*.*\x0\x0";		
	mOpenFile.lpstrCustomFilter = NULL;
	mOpenFile.nMaxCustFilter    = 0;
	mOpenFile.nFilterIndex      = 0;
	mOpenFile.lpstrFile         = FileName;
	mOpenFile.nMaxFile          = sizeof(FileName);
	mOpenFile.lpstrFileTitle    = NULL;
	mOpenFile.nMaxFileTitle     = 0;
	mOpenFile.lpstrInitialDir   = NULL;
	mOpenFile.lpstrTitle        = "ѡ���д��Flash�������ļ�";

	mOpenFile.nFileOffset       = 0;
	mOpenFile.nFileExtension    = 0;
	mOpenFile.lpstrDefExt       = NULL;
	mOpenFile.lCustData         = 0;
	mOpenFile.lpfnHook 		   = NULL;
	mOpenFile.lpTemplateName    = NULL;
	mOpenFile.Flags             = OFN_SHOWHELP | OFN_EXPLORER | OFN_READONLY | OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&mOpenFile))
		DbgPrint("Write data to flash from:%s",FileName);
	else 
		goto Exit;

	if(strlen(FileName) < 1)
	{
		DbgPrint("�����ļ���Ч��������ѡ��");
		goto Exit;
	}	
	hFile = CreateFile(FileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);//���ļ�
	if(hFile == INVALID_HANDLE_VALUE)
	{
		ShowLastError("CreateFile");
		goto Exit;
	}
	FileSize = GetFileSize(hFile,NULL);
	if(FileSize<128)
	{
		DbgPrint("�����ļ�%d�ֽ�̫С��������ѡ��",FileSize);
		goto Exit;
	}
	if( FileSize > (Flash_Sector_Size * Flash_Sector_Count) )
		TestLen = Flash_Sector_Size * Flash_Sector_Count;
	else
		TestLen = FileSize;
	
	FileBuf = (PUCHAR)malloc(FileSize);
	if(FileBuf==NULL)
	{
		DbgPrint("��������ļ��ڴ�%dBʧ��",FileSize);
		goto Exit;
	}
	RBuf = (PUCHAR)malloc(FileSize+128);
	if(RBuf==NULL)
	{
		DbgPrint("������Զ��ڴ�%dBʧ��",FileSize);
		goto Exit;
	}
	RLen = FileSize;
	if( !ReadFile(hFile,FileBuf,RLen,&RLen,NULL) )
	{
		DbgPrint("���ļ��ж�ȡ��������ʧ��",FileSize);
		goto Exit;
	}
	DbgPrint("\r\n");
	DbgPrint("****��ʼ�ٶȲ���,�������ݳ���Ϊ%dB",FileSize);

	DbgPrint("*>>*1.Flash�����ٶȲ���");
	TestLen = FileSize;
	BT = GetCurrentTimerVal();
	/*
	for(i=0;i<TC;i++)
	{
		RetVal = FLASH_Erase_Sector(FlashAddr+i*Flash_Sector_Size);
		//RetVal = FLASH_Erase_Block(FlashAddr+i*32768);		
		if(!RetVal )
		{
			DbgPrint("  FLASH_Erase_Sector[%X] failure",i);
			break;
		}
	}
	*/
	RetVal = FLASH_Erase_Full();
	UsedT = (GetCurrentTimerVal()-BT)/1000;
	DbgPrint("*<<*����ȫ��%s,ƽ���ٶ�:%.2fKB/S,�ۼ���ʱ%.3fS",RetVal?"�ɹ�":"ʧ��",TestLen/UsedT/1000,UsedT);	
	
	DbgPrint("*>>*2.Flash��д�ٶȲ���");
	BlockSize = 0x100;
	TestLen = FileSize;	
	TC = (FileSize+BlockSize-1)/BlockSize;
	BT = GetCurrentTimerVal();
	for(i=0;(i<TC)&(RetVal);i++)
	{
		Addr = i*BlockSize;
		if( (i+1)==TC )
			RLen = FileSize-i*BlockSize;
		else
			RLen = BlockSize;		
	
		RetVal = FLASH_WriteEnable();		
		if(!RetVal)		
		{
			DbgPrint("FLASH_WriteEnable failure.");
			break;
		}
		RBuf[0] = CMD_FLASH_BYTE_PROG;
		RBuf[1] = (UINT8)( Addr >> 16 );
		RBuf[2] = (UINT8)( Addr >> 8 );
		RBuf[3] = (UINT8)Addr&0xFF;
		RLen += 4;
		memcpy(&RBuf[4],FileBuf+Addr,RLen);
		RetVal = CH347SPI_Write(DevIndex,0x80,RLen,BlockSize+4,RBuf); //SPI����д
		if(!RetVal)	
		{
			DbgPrint("дFLASH[0x%X][0x%X]ʧ��",Addr,RLen);
			break;
		}

		Timeout = 0;
		do//�ȴ�д����
		{
			temp = FLASH_ReadStatusReg();
			if( (temp & 0x01)<1)
				break;
			Sleep(0);
			Timeout += 1;
			if(Timeout > FLASH_OP_TIMEOUT*20)
			{
				DbgPrint("    [0x%X][0x%X]>FLASH_Read timeout",Addr,RLen);
				RetVal= FALSE; 
				break; //�˳�д
			}
		}while( temp & 0x01 );		
	}
	UsedT = (GetCurrentTimerVal()-BT)/1000;
	DbgPrint("*<<*��д%d�ֽ� %s.ƽ���ٶ�:%.3fKB/S,�ۼ���ʱ%.3fS",TestLen,RetVal?"�ɹ�":"ʧ��",TestLen/UsedT/1000,UsedT);
	
	DbgPrint("*>>*3.Flash����ٶȲ���");
	TestLen = FileSize;

	//�ȷ��Ͷ���ַ
	Addr = 0;		
	RBuf[0] = CMD_FLASH_READ;
	RBuf[1] = (UINT8)( Addr >> 16 );
	RBuf[2] = (UINT8)( Addr >> 8 );
	RBuf[3] = (UINT8)( Addr );		
	WLen = 4;
	
	//���ٶ�ȡSPI FLASH
	RLen = TestLen;//һ�ζ���		

	BT = GetCurrentTimerVal();
	RetVal = CH347SPI_Read(DevIndex,0x80,WLen,&RLen,RBuf);//SPI������,�ȷ�����
	UsedT = (GetCurrentTimerVal()-BT)/1000;	
	if( !RetVal )
	{
		DbgPrint("��FLASH��ʼ��ַ��%dB����ʧ��",TestLen);
	}
	if(RLen != TestLen)
	{
		DbgPrint("��ȡ���ݲ�����(0x%X-0x%X)",RLen,TestLen);
	}	
	DbgPrint("*<<*���%d�ֽ� %s.ƽ���ٶ�:%.3fKB/S,�ۼ���ʱ%.3fS",TestLen,RetVal?"�ɹ�":"ʧ��",TestLen/UsedT/1000,UsedT);	

	if( !RetVal )
		goto Exit;

	DbgPrint("*>>*4.Flashд�����ݼ��");
	TestLen = FileSize;
	TC = (TestLen+8192-1)/8192;
	for(i=0;i<TC;i++)
	{
		Addr = i*8192;
		if( (i+1)==TC)
			UnitSize = FileSize-i*8192;
		else
			UnitSize = 8192;
		for(k=0;k<UnitSize;k++)
		{
			if(FileBuf[Addr+k]!=RBuf[Addr+k])
			{	
				if(((Addr+k)&0xFFF0+16)>FileSize)
					PrintLen = FileSize - ((Addr+k)&0xFFF0+16);
				else 
					PrintLen = 16;
				ErrLoca = (Addr+k)%16;
				DbgPrint("[%04X]:%02X-%02X:д��Ͷ��������ݲ�ƥ��",k+Addr,FileBuf[Addr+k],RBuf[Addr+k]);				
				DumpDataBuf((Addr+k)&0xFFF0,RBuf+((Addr+k)&0xFFF0),FileBuf+((Addr+k)&0xFFF0),PrintLen,ErrLoca);
				goto Exit;
			}
		}
	}
	DbgPrint("*<<*���Flashд�Ͷ�����%dB,ȫ��ƥ��",TestLen);
	DbgPrint("Test Result: P A S S");
Exit:
	if(hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	if(FileBuf!=NULL)
		free(FileBuf);

	EnableWindow(FlashEepromDbgHwnd,TRUE);
	return RetVal;
}

//��FLASH�������ļ�
DWORD WINAPI WriteFlashFromFile(LPVOID lpParameter)
{
	CHAR FileName[MAX_PATH] = "",FmtStr[64]="";
	OPENFILENAME mOpenFile={0};
	ULONG TestLen,RLen,Addr,TC,i,Timeout,temp,BlockSize=0;
	PUCHAR FileBuf=NULL;
	double BT,UsedT;
	BOOL RetVal = FALSE;
	HANDLE hFile=INVALID_HANDLE_VALUE;
	UCHAR RBuf[4096] = "";
	
	// Fill in the OPENFILENAME structure to support a template and hook.
	mOpenFile.lStructSize = sizeof(OPENFILENAME);
	mOpenFile.hwndOwner         = FlashEepromDbgHwnd;
	mOpenFile.hInstance         = AfxMainIns;		
	mOpenFile.lpstrFilter       = "*.*\x0\x0";		
	mOpenFile.lpstrCustomFilter = NULL;
	mOpenFile.nMaxCustFilter    = 0;
	mOpenFile.nFilterIndex      = 0;
	mOpenFile.lpstrFile         = FileName;
	mOpenFile.nMaxFile          = sizeof(FileName);
	mOpenFile.lpstrFileTitle    = NULL;
	mOpenFile.nMaxFileTitle     = 0;
	mOpenFile.lpstrInitialDir   = NULL;
	mOpenFile.lpstrTitle        = "ѡ���д��Flash�������ļ�";

	mOpenFile.nFileOffset       = 0;
	mOpenFile.nFileExtension    = 0;
	mOpenFile.lpstrDefExt       = NULL;
	mOpenFile.lCustData         = 0;
	mOpenFile.lpfnHook 		   = NULL;
	mOpenFile.lpTemplateName    = NULL;
	mOpenFile.Flags             = OFN_SHOWHELP | OFN_EXPLORER | OFN_READONLY | OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&mOpenFile))
		DbgPrint("Write data to flash from:%s",FileName);
	else
		goto Exit;

	hFile = CreateFile(FileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_WRITE|FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_ARCHIVE,NULL);
	if(hFile==INVALID_HANDLE_VALUE)
	{
		ShowLastError("WriteFlashFromFile.�򿪱����ļ�");
		goto Exit;
	}
	TestLen = GetFileSize(hFile,NULL);
	if(TestLen<1)
	{
		DbgPrint("WriteFlashFromFile.д��flash�����ļ�Ϊ�գ�����ѡ");
		goto Exit;
	}
	if( TestLen > (Flash_Sector_Size * Flash_Sector_Count) ) //Flash����
		TestLen = Flash_Sector_Size * Flash_Sector_Count;

	DbgPrint("*>>*WriteFlashFromFile.Flashд%d�ֽ�",TestLen);
	FileBuf = (PUCHAR)malloc(TestLen+64);
	if(FileBuf==NULL)
	{
		DbgPrint("WriteFlashFromFile.�����ڴ�ʧ��");
		goto Exit;
	}
	memset(FileBuf,0,TestLen+64);
	RLen = TestLen;
	if( !ReadFile(hFile,FileBuf,RLen,&RLen,NULL) )
	{
		ShowLastError("WriteFlashFromFile.Read file");
		goto Exit;
	}
	if(RLen!=TestLen)
	{
		DbgPrint("WriteFlashFromFile.ReadFlashFile len err(%d-%d)",RLen,TestLen);
		goto Exit;
	}
	DbgPrint("*>>*1.WriteFlashFromFile.����");
	Addr=0;
	TC = (TestLen+Flash_Sector_Size-1)/Flash_Sector_Size; //page	
	BT = GetCurrentTimerVal();
	for(i=0;i<TC;i++)
	{
		RetVal = FLASH_Erase_Sector(Addr+i*Flash_Sector_Size);
		//RetVal = FLASH_Erase_Block(FlashAddr+i*32768);		
		if(!RetVal )
		{
			DbgPrint("  FLASH_Erase_Sector[%X] failure",i);
			break;
		}
	}
	UsedT = (GetCurrentTimerVal()-BT)/1000;
	DbgPrint("*<<*WriteFlashFromFile.����%d��(%dB) %s,ƽ���ٶ�:%.2fKB/S,�ۼ���ʱ%.3fS",
		TC,TC*32768,RetVal?"�ɹ�":"ʧ��",
		TestLen/UsedT/1000,UsedT);
	
	DbgPrint("*>>*2.WriteFlashFromFile.Flash��д");
	BlockSize = 0x100;
	TC = (TestLen+BlockSize-1)/BlockSize;
	BT = GetCurrentTimerVal();
	for(i=0;(i<TC)&(RetVal);i++)
	{
		Addr = i*BlockSize;
		if( (i+1)==TC )
			RLen = TestLen-i*BlockSize;
		else
			RLen = BlockSize;		
	
		RetVal = FLASH_WriteEnable();		
		if(RetVal)		
		{			
			RBuf[0] = CMD_FLASH_BYTE_PROG;
			RBuf[1] = (UINT8)( Addr >> 16 );
			RBuf[2] = (UINT8)( Addr >> 8 );
			RBuf[3] = (UINT8)Addr;
			RLen += 4;			
			memcpy(&RBuf[4],FileBuf+Addr,RLen);
			RetVal = CH347SPI_Write(DevIndex,0x80,RLen,BlockSize+4,RBuf);
			if(!RetVal)	DbgPrint("WriteFlashFromFile.дFLASH[0x%X][0x%X]ʧ��",Addr,RLen);
		}
		else break;
		Timeout = 0;
		do
		{
			temp = FLASH_ReadStatusReg();
			if( (temp & 0x01)<1)
				break;
			Sleep(0);
			Timeout += 1;
			if(Timeout > FLASH_OP_TIMEOUT)
			{
				DbgPrint("    [0x%X][0x%X]>FLASH_Read timeout",Addr,RLen);
				RetVal= FALSE; break; //�˳�д
			}
		}while( temp & 0x01 );		
	}
	UsedT = (GetCurrentTimerVal()-BT)/1000;
	DbgPrint("*<<*WriteFlashFromFile.��д%d�ֽ� %s.ƽ���ٶ�:%.3fKB/S,�ۼ���ʱ%.3fS",TestLen,RetVal?"�ɹ�":"ʧ��",TestLen/UsedT/1000,UsedT);

Exit:
	if(FileBuf!=NULL)
		free(FileBuf);
	if(hFile!=INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	return RetVal;
}

//����FLASH����
DWORD WINAPI FlashVerifyWithFile(LPVOID lpParameter)
{
	ULONG FlashAddr = 0,TC,TestLen,i,k;
	double BT = 0;
	BOOL RetVal = FALSE;
	CHAR FileName[MAX_PATH]="";
	HANDLE hFile=INVALID_HANDLE_VALUE;
	PUCHAR FileBuf=NULL,RBuf=NULL;
	ULONG FileSize,RLen,UnitSize,Addr,WLen;
	double UsedT = 0;
	ULONG PrintLen,ErrLoca;
	OPENFILENAME mOpenFile={0};

	DbgPrint("*>>*��ʼ��֤FLASH����");
	// Fill in the OPENFILENAME structure to support a template and hook.
	mOpenFile.lStructSize = sizeof(OPENFILENAME);
	mOpenFile.hwndOwner         = FlashEepromDbgHwnd;
	mOpenFile.hInstance         = AfxMainIns;		
	mOpenFile.lpstrFilter       = "*.*\x0\x0";		
	mOpenFile.lpstrCustomFilter = NULL;
	mOpenFile.nMaxCustFilter    = 0;
	mOpenFile.nFilterIndex      = 0;
	mOpenFile.lpstrFile         = FileName;
	mOpenFile.nMaxFile          = sizeof(FileName);
	mOpenFile.lpstrFileTitle    = NULL;
	mOpenFile.nMaxFileTitle     = 0;
	mOpenFile.lpstrInitialDir   = NULL;
	mOpenFile.lpstrTitle        = "ѡ���д��Flash�������ļ�";

	mOpenFile.nFileOffset       = 0;
	mOpenFile.nFileExtension    = 0;
	mOpenFile.lpstrDefExt       = NULL;
	mOpenFile.lCustData         = 0;
	mOpenFile.lpfnHook 		   = NULL;
	mOpenFile.lpTemplateName    = NULL;
	mOpenFile.Flags             = OFN_SHOWHELP | OFN_EXPLORER | OFN_READONLY | OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&mOpenFile))
	{ 	
		DbgPrint("Verify flash with file:%s",FileName);
	}
	else
		goto Exit;

	if(strlen(FileName) < 1)
	{
		DbgPrint("�ļ���Ч��������ѡ��");
		goto Exit;
	}	
	hFile = CreateFile(FileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);//���ļ�
	if(hFile == INVALID_HANDLE_VALUE)
	{
		ShowLastError("CreateFile");
		goto Exit;
	}
	FileSize = GetFileSize(hFile,NULL);
	if(FileSize<128)
	{
		DbgPrint("���ڱȶԵ��ļ�%d�ֽ�̫С��������ѡ��",FileSize);
		goto Exit;
	}
	if( FileSize > (Flash_Sector_Size * Flash_Sector_Count) )
	{
		TestLen = Flash_Sector_Size * Flash_Sector_Count;		
	}
	else
		TestLen = FileSize;
	
	FileBuf = (PUCHAR)malloc(FileSize);
	if(FileBuf==NULL)
	{
		DbgPrint("�����ļ��ڴ�%dBʧ��",FileSize);
		goto Exit;
	}
	RBuf = (PUCHAR)malloc(FileSize+128);
	if(RBuf==NULL)
	{
		DbgPrint("��������ڴ�%dBʧ��",FileSize);
		goto Exit;
	}
	RLen = TestLen;
	if( !ReadFile(hFile,FileBuf,RLen,&RLen,NULL) )
	{
		DbgPrint("���ļ��ж�ȡ����ʧ��",FileSize);
		goto Exit;
	}

	//�ȷ��Ͷ���ַ
	Addr = 0;		
	RBuf[0] = CMD_FLASH_READ;
	RBuf[1] = (UINT8)( Addr >> 16 );
	RBuf[2] = (UINT8)( Addr >> 8 );
	RBuf[3] = (UINT8)( Addr );		
	WLen = 4;
	
	//���ٶ�ȡSPI FLASH
	RLen = TestLen;//һ�ζ���		

	BT = GetCurrentTimerVal();
	RetVal = CH347SPI_Read(DevIndex,0x80,WLen,&RLen,RBuf);//��	
	if( !RetVal )
	{
		DbgPrint("��FLASH��ʼ��ַ��%dB����ʧ��",TestLen);
	}
	if(RLen != TestLen)
	{
		DbgPrint("��ȡ���ݲ�����(0x%X-0x%X)",RLen,TestLen);
	}	

	if( !RetVal )
		goto Exit;

	TestLen = FileSize;
	TC = (TestLen+8192-1)/8192;
	for(i=0;i<TC;i++)
	{
		Addr = i*8192;
		if( (i+1)==TC)
			UnitSize = FileSize-i*8192;
		else
			UnitSize = 8192;
		for(k=0;k<UnitSize;k++)
		{
			if(FileBuf[Addr+k]!=RBuf[Addr+k])
			{	
				if(((Addr+k)&0xFFF0+16)>FileSize)
					PrintLen = FileSize - ((Addr+k)&0xFFF0+16);
				else 
					PrintLen = 16;
				ErrLoca = (Addr+k)%16;
				DbgPrint("[%04X]:%02X-%02X:д��Ͷ��������ݲ�ƥ��",k+Addr,FileBuf[Addr+k],RBuf[Addr+k]);				
				DumpDataBuf((Addr+k)&0xFFF0,RBuf+((Addr+k)&0xFFF0),FileBuf+((Addr+k)&0xFFF0),PrintLen,ErrLoca);
				goto Exit;
			}
		}
	}
	RetVal = TRUE;
	UsedT = (GetCurrentTimerVal()-BT)/1000;	
	DbgPrint("*<<*��֤Flash����%dB,���ļ�����ƥ��,ƽ���ٶ�:%.3fKB/S,�ۼ���ʱ%.3fS",TestLen,TestLen/UsedT/1000,UsedT);
Exit:
	if(hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	if(FileBuf!=NULL)
		free(FileBuf);

	DbgPrint("*>>*FLASH��֤%s",RetVal?"���":"ʧ��");
	DbgPrint("\r\n");	

	return RetVal;
}

//��FLASH�������ļ�
DWORD WINAPI ReadFlashToFile(LPVOID lpParameter)
{
	// ��ȡ��Ҫ���͵��ļ���
	CHAR FileName[MAX_PATH] = "",FmtStr[64]="";
	OPENFILENAME mOpenFile={0};
	ULONG TestLen,RLen,Addr;
	PUCHAR RBuf=NULL;
	double BT,UsedT;
	BOOL RetVal = FALSE;
	HANDLE hFile=INVALID_HANDLE_VALUE;

	// Fill in the OPENFILENAME structure to support a template and hook.
	mOpenFile.lStructSize = sizeof(OPENFILENAME);
	mOpenFile.hwndOwner         = FlashEepromDbgHwnd;
	mOpenFile.hInstance         = AfxMainIns;		
	mOpenFile.lpstrFilter       = "*.*\x0\x0";		
	mOpenFile.lpstrCustomFilter = NULL;
	mOpenFile.nMaxCustFilter    = 0;
	mOpenFile.nFilterIndex      = 0;
	mOpenFile.lpstrFile         = FileName;
	mOpenFile.nMaxFile          = sizeof(FileName);
	mOpenFile.lpstrFileTitle    = NULL;
	mOpenFile.nMaxFileTitle     = 0;
	mOpenFile.lpstrInitialDir   = NULL;
	mOpenFile.lpstrTitle        = "ѡ��Flash���ݱ����ļ�";

	mOpenFile.nFileOffset       = 0;
	mOpenFile.nFileExtension    = 0;
	mOpenFile.lpstrDefExt       = NULL;
	mOpenFile.lCustData         = 0;
	mOpenFile.lpfnHook 		   = NULL;
	mOpenFile.lpTemplateName    = NULL;
	mOpenFile.Flags             = OFN_SHOWHELP | OFN_EXPLORER | OFN_READONLY | OFN_FILEMUSTEXIST;
	if (GetSaveFileName(&mOpenFile))
		DbgPrint("FlashData will save to:%s",FileName);
	else
		goto Exit;

	TestLen = Flash_Sector_Size * Flash_Sector_Count; //Flash����
	DbgPrint("*>>*ReadFlashToFile.Flash��%d�ֽ����ļ�",TestLen);

	hFile = CreateFile(FileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_WRITE|FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_ARCHIVE,NULL);
	if(hFile==INVALID_HANDLE_VALUE)
	{
		ShowLastError("ReadFlashToFile.�򿪱����ļ�");
		goto Exit;
	}
	RBuf = (PUCHAR)malloc(TestLen+64);
	if(RBuf==NULL)
	{
		DbgPrint("ReadFlashToFile.�����ڴ�ʧ��");
		goto Exit;
	}	
	//���Ͷ���ַ
	Addr = 0;		
	RBuf[0] = CMD_FLASH_READ;
	RBuf[1] = (UINT8)( Addr >> 16 );
	RBuf[2] = (UINT8)( Addr >> 8 );
	RBuf[3] = (UINT8)( Addr );

	BT = GetCurrentTimerVal();
	//���ٶ�ȡSPI FLASH
	RLen = TestLen;//��������
	RetVal = CH347SPI_Read(DevIndex,0x80,4,&RLen,RBuf);
	if( !RetVal )
	{
		DbgPrint("ReadFlashToFile.��FLASH��ʼ��ַ��%dB����ʧ��",RLen);
	}
	if(RLen != TestLen)
	{
		DbgPrint("ReadFlashToFile.��ȡ���ݲ�����(0x%X-0x%X)",RLen,TestLen);
	}
	UsedT = (GetCurrentTimerVal()-BT)/1000;
	DbgPrint("*<<*ReadFlashToFile.���%d�ֽ� %s.ƽ���ٶ�:%.3fKB/S,�ۼ���ʱ%.3fS",TestLen,RetVal?"�ɹ�":"ʧ��",TestLen/UsedT/1000,UsedT);			
	if( !WriteFile(hFile,RBuf,RLen,&RLen,NULL) )
	{
		ShowLastError("ReadFlashToFile.����д���ļ�:");
		goto Exit;
	}
	if(RLen!=TestLen)
	{
		DbgPrint("ReadFlashToFileд�벻����");
		goto Exit;
	}		
	RetVal = TRUE;
Exit:
	if(RBuf!=NULL)
		free(RBuf);
	if(hFile!=INVALID_HANDLE_VALUE)
		CloseHandle(hFile);

	return RetVal;
}

//���豸
BOOL FlashSpi_OpenDevice()
{
	//��ȡ�豸���
	DevIndex = SendDlgItemMessage(FlashEepromDbgHwnd,IDC_ObjList,CB_GETCURSEL,0,0);
	if(DevIndex==CB_ERR)
	{
		DbgPrint("����ѡ���豸");
		goto Exit; //�˳�
	}	
	FlashDevIsOpened = (CH347OpenDevice(DevIndex) != INVALID_HANDLE_VALUE);
	DbgPrint(">>%d#�豸��...%s",DevIndex,FlashDevIsOpened?"�ɹ�":"ʧ��");
Exit:
	FlashDlg_EnableButtonEnable();
	return FlashDevIsOpened;
}

//�ر��豸
BOOL FlashSpi_CloseDevice()
{
	CH347CloseDevice(DevIndex);
	FlashDevIsOpened = FALSE;
	DbgPrint(">>�豸�ѹر�");
	DevIndex = CB_ERR;

	FlashDlg_EnableButtonEnable();

	return TRUE;
}


//��ʾ�豸��Ϣ
BOOL  FlashDlg_ShowDevInfor()
{
	ULONG ObjSel;
	CHAR  FmtStr[128]="";

	ObjSel = SendDlgItemMessage(FlashEepromDbgHwnd,IDC_ObjList,CB_GETCURSEL,0,0);
	if(ObjSel!=CB_ERR)
	{
		sprintf(FmtStr,"**ChipMode:%d,%s,Ver:%02X,DevID:%s",FlashDevInfor[ObjSel].ChipMode,FlashDevInfor[ObjSel].UsbSpeedType?"HS":"FS",FlashDevInfor[ObjSel].FirewareVer,FlashDevInfor[ObjSel].DeviceID);
		SetDlgItemText(FlashEepromDbgHwnd,IDC_DevInfor,FmtStr);
	}
	return (ObjSel!=CB_ERR);
}

//ö���豸
ULONG  FlashDlg_EnumDevice()
{
	ULONG i,oLen,DevCnt = 0;
	USB_DEVICE_DESCRIPTOR DevDesc = {0};
	CHAR tem[256] = "";
	mDeviceInforS DevInfor = {0};

	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_ObjList,CB_RESETCONTENT,0,0);	
	for(i=0;i<16;i++)
	{
		if(CH347OpenDevice(i) != INVALID_HANDLE_VALUE)
		{
			oLen = sizeof(USB_DEVICE_DESCRIPTOR);
			CH347GetDeviceInfor(i,&DevInfor);
			if(DevInfor.ChipMode == 3) //ģʽ3�˽ӿ�ΪJTAG/I2C
				continue;
			sprintf(tem,"%d# %s",i,DevInfor.FuncDescStr);
			SendDlgItemMessage(FlashEepromDbgHwnd,IDC_ObjList,CB_ADDSTRING,0,(LPARAM)(LPCTSTR)tem);		
			memcpy(&FlashDevInfor[DevCnt],&DevInfor,sizeof(DevInfor));
			DevCnt++;
		}
		CH347CloseDevice(i);
	}
	if(DevCnt)
	{
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_ObjList,CB_SETCURSEL,0,0);
		SetFocus(GetDlgItem(FlashEepromDbgHwnd,IDC_ObjList));
	}
	return DevCnt;
}


//ʹ�ܲ�����ť�����ȴ򿪺�����,�����޷�����
VOID FlashDlg_EnableButtonEnable()
{
	if(!FlashDevIsOpened)
		IsSpiInit = FALSE;

	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_CMD_InitSPI),FlashDevIsOpened);

	//���´�/�ر��豸��ť״̬
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_OpenDevice),!FlashDevIsOpened);
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_CloseDevice),FlashDevIsOpened);	

	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_ObjList),!FlashDevIsOpened);
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_RefreshObjList),!FlashDevIsOpened);

	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_FlashVerify),IsSpiInit);
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_FlashRWSpeedTest),IsSpiInit);	
	
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_FlashIdentify),IsSpiInit);
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_FlashRead),IsSpiInit);
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_FlashWrite),IsSpiInit);
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_FlashErase),IsSpiInit);

	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_ReadToFile),IsSpiInit);
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_WriteFormFile),IsSpiInit);
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_FlashVerify),IsSpiInit);
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_FlashRWSpeedTest),IsSpiInit);	
}

//��ʼ������
VOID FlashDlg_InitWindows()
{	
	//���Ҳ���ʾ�豸�б� 
	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_RefreshObjList,BM_CLICK,0,0);	
	
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_OpenDevice),!FlashDevIsOpened);
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_CloseDevice),FlashDevIsOpened);
	//Flash��ַ���ֵ
	SetDlgItemText(FlashEepromDbgHwnd,IDC_FlashStartAddr,"0");
	//Flash���������ֵ
	SetDlgItemText(FlashEepromDbgHwnd,IDC_FlashDataSize,"100");
	//���Flash����ģʽ
	SetDlgItemText(FlashEepromDbgHwnd,IDC_FlashData,"");
	//�����������ʾ������ַ���
	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_InforShow,EM_LIMITTEXT,0xFFFFFFFF,0);

	{
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode0");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode1");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode2");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode3");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Mode,CB_SETCURSEL,3,0);
	}
	{//0=60MHz, 1=30MHz, 2=15MHz, 3=7.5MHz, 4=3.75MHz, 5=1.875MHz, 6=937.5KHz��7=468.75KHz
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"60MHz");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"30MHz");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"15MHz");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"7.5MHz");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"3.75MHz");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"1.875MHz");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"937.5KHz");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"468.75KHz");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Clock,CB_SETCURSEL,1,0);
	}
	SetDlgItemInt(FlashEepromDbgHwnd,IDC_TestLen,100,FALSE);
	SetEditlInputMode(FlashEepromDbgHwnd,IDC_FlashData,1); //����editΪʮ����������ģʽ

	return;
}

//���������
BOOL APIENTRY DlgProc_FlashEepromDbg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	ULONG ThreadID;

	switch (message)
	{
	case WM_INITDIALOG:
		FlashEepromDbgHwnd = hWnd;
		AfxActiveHwnd = hWnd;	
		FlashDevIsOpened = FALSE;		
		CheckDlgButton(hWnd,IDC_EnablePnPAutoOpen_Flash,BST_CHECKED);
		FlashDlg_InitWindows(); //��ʼ������		
		FlashDlg_EnableButtonEnable();

		InitWindows_Eeprom();
		Eeprom_InitI2C();
		//Uart_InitParam();		
		//ΪUSB2.0JTAG�豸���ò���Ͱγ���֪ͨ.������Զ����豸,�γ���ر��豸
	//	if(CH341SetDeviceNotify(0,DevID_Mode1_2, UsbDevPnpNotify) )       //�豸���֪ͨ�ص�����
		//	DbgPrint("�ѿ���USB�豸��μ���");
	//	break;	
	//case WM_USB20SpiDevArrive: //��⵽�豸����
	//	DbgPrint("****����CH347�豸����USB��,���豸");
		//��ö��USB�豸
	//	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_RefreshObjList,BM_CLICK,0,0);
		//���豸
	//	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_OpenDevice,BM_CLICK,0,0);
	//	break;
	//case WM_USB20SpiDevRemove: //��⵽�豸�γ�
	//	DbgPrint("****����CH347�Ѵ�USB���Ƴ�,�ر��豸");
		//�ر��豸
	//	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_CloseDevice,BM_CLICK,0,0);
	//	break;
	case WM_NOTIFY:
		{
			if(((LPNMHDR)lParam)->code == PSN_SETACTIVE)
				AfxActiveHwnd = hWnd;
		}
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		// Parse the menu selections:
		switch (wmId)
		{				
		case IDC_RefreshObjList:
			FlashDlg_EnumDevice();   //ö�ٲ���ʾ�豸
			break;
		case IDC_ObjList:
			FlashDlg_ShowDevInfor();
			break;
		case IDC_OpenDevice://���豸
			FlashSpi_OpenDevice();
			FlashDlg_EnableButtonEnable();	//���°�ť״̬
			CH347I2C_Set(DevIndex,3); //����I2C�ٶ�Ϊ����750K
			break;
		case IDC_CloseDevice:			
			FlashSpi_CloseDevice();				
			FlashDlg_EnableButtonEnable();	//���°�ť״̬			
			break;	
		case IDC_CMD_InitSPI:
			IsSpiInit = Flash_InitSpi();
			FlashDlg_EnableButtonEnable();
			break;
		case IDC_FlashIdentify:
			FlashIdentify();
			break;
		case IDC_FlashRead:
			FlashBlockRead();
			break;
		case IDC_FlashWrite://��IDC_FLASHDATA��������д��FLASH
			FlashBlockWrite();
			break;
		case IDC_FlashErase:
			FlashBlockErase();
			break;	
		case IDC_FlashVerify:
			CloseHandle(CreateThread(NULL,0,FlashVerifyWithFile,NULL,0,&ThreadID));
			break;
		case IDC_WriteFormFile:
			CloseHandle(CreateThread(NULL,0,WriteFlashFromFile,NULL,0,&ThreadID));
			break;
		case IDC_ReadToFile:
			CloseHandle(CreateThread(NULL,0,ReadFlashToFile,NULL,0,&ThreadID));
			break;
		case IDC_FlashRWSpeedTest://��д����
			CloseHandle(CreateThread(NULL,0,FlashRWSpeedTest,NULL,0,&ThreadID));
			break;
		case IDC_EepromRead:
			EerpromRead();
			break;
		case IDC_EepromWrite:
			EepromWrite();
			break;
		case IDC_EepromVerify:
			CloseHandle(CreateThread(NULL,0,EepromVerifyWithFile,NULL,0,&ThreadID));
			break;
		case IDC_WriteEepromFormFile:
			CloseHandle(CreateThread(NULL,0,WriteEepromFromFile,NULL,0,&ThreadID));
			break;
		case IDC_ReadEepromToFile:
			CloseHandle(CreateThread(NULL,0,ReadEepromToFile,NULL,0,&ThreadID));
			break;
		case IDC_EepromType:
			Eeprom_InitI2C();
			break;
		case IDC_ClearInfor:
			SetDlgItemText(hWnd,IDC_InforShow,"");
			break;
		case WM_DESTROY:			
			SendDlgItemMessage(hWnd,IDC_CloseDevice,BM_CLICK,0,0);
			//CH347SetDeviceNotify(DevIndex,DevID_Mode1_2,NULL);
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;		
		case WM_DESTROY:
			PostQuitMessage(0);
			break;		
	}
	return 0;
}