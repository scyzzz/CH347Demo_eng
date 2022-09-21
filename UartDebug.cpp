/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2022                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  USB2.0 UART API Demo
Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2022 Nanjing Qinheng Microelectronics Co., Ltd.
Revision History:
  4/3/2022: TECH30
--*/

#include "Main.h"


#define WM_UartArrive WM_USER+10         //�豸����֪ͨ�¼�,������̽���
#define WM_UartRemove WM_USER+11         //�豸�γ�֪ͨ�¼�,������̽���

HWND UartDebugHwnd;     //��������
extern HINSTANCE AfxMainIns; //����ʵ��
extern HWND AfxActiveHwnd;

BOOL UartDevIsOpened;  //�豸�Ƿ��
ULONG UartIndex;
ULONG TotalTxCnt=0,TotalRxCnt=0,TxFileSize;
BOOL StopTxThread,StopRxThread;
mDeviceInforS UartDevInfor[16] = {0};
BOOL UartAutoRecvIsStart = FALSE;
BOOL UartAutoRecvToFile = FALSE,UartAutoRecvShow=FALSE;
HANDLE hRxFile = INVALID_HANDLE_VALUE;

// USB�豸��μ��֪ͨ����.��ص������Ժ������������ƣ�ͨ��������Ϣת�Ƶ���Ϣ�������ڽ��д���
VOID	 CALLBACK	 Uart_UsbDevPnpNotify (ULONG iEventStatus ) 
{
	if(iEventStatus==CH347_DEVICE_ARRIVAL)// �豸�����¼�,�Ѿ�����
		PostMessage(UartDebugHwnd,WM_UartArrive,0,0);
	else if(iEventStatus==CH347_DEVICE_REMOVE)// �豸�γ��¼�,�Ѿ��γ�
		PostMessage(UartDebugHwnd,WM_UartRemove,0,0);
	return;
}

//��ʾ�豸��Ϣ
BOOL Uart_ShowDevInfor()
{
	ULONG ObjSel;
	CHAR  FmtStr[128]="";

	ObjSel = SendDlgItemMessage(UartDebugHwnd,IDC_ObjList,CB_GETCURSEL,0,0);
	if(ObjSel!=CB_ERR)
	{
		//sprintf(FmtStr,"**Chip Mode:%d,DevID:%s",UartDevInfor[ObjSel].ChipMode,UartDevInfor[ObjSel].DeviceID);
		sprintf(FmtStr,"**ChipMode:%d,%s,DevID:%s",UartDevInfor[ObjSel].ChipMode,UartDevInfor[ObjSel].UsbSpeedType?"HS":"FS",UartDevInfor[ObjSel].DeviceID);
		SetDlgItemText(UartDebugHwnd,IDC_DevInfor,FmtStr);
	}
	return (ObjSel!=CB_ERR);
}

//enumerate devices
ULONG Uart_EnumDevice()
{
	ULONG i,oLen,DevCnt = 0;
	USB_DEVICE_DESCRIPTOR DevDesc = {0};
	CHAR tem[256] = "";
	mDeviceInforS DevInfor = {0};

	SendDlgItemMessage(UartDebugHwnd,IDC_ObjList,CB_RESETCONTENT,0,0);	
	for(i=0;i<16;i++)
	{
		if(CH347Uart_Open(i) != INVALID_HANDLE_VALUE)
		{
			oLen = sizeof(USB_DEVICE_DESCRIPTOR);
			CH347Uart_GetDeviceInfor(i,&DevInfor);			
			sprintf(tem,"%d# %s",i,DevInfor.FuncDescStr);
			SendDlgItemMessage(UartDebugHwnd,IDC_ObjList,CB_ADDSTRING,0,(LPARAM)(LPCTSTR)tem);		
			memcpy(&UartDevInfor[DevCnt],&DevInfor,sizeof(DevInfor));
			DevCnt++;
		}
		CH347Uart_Close(i);
	}
	if(DevCnt)
	{
		SendDlgItemMessage(UartDebugHwnd,IDC_ObjList,CB_SETCURSEL,DevCnt-1,0);
		SetFocus(GetDlgItem(UartDebugHwnd,IDC_ObjList));
	}
	return DevCnt;
}

VOID Uart_InitParam()
{
	{
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Baudrate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"9600");
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Baudrate,CB_SETITEMDATA,0,(LPARAM)9600);
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Baudrate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"19200");
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Baudrate,CB_SETITEMDATA,1,(LPARAM)19200);
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Baudrate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"115200");
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Baudrate,CB_SETITEMDATA,2,(LPARAM)115200);
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Baudrate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"4000000");
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Baudrate,CB_SETITEMDATA,2,(LPARAM)4000000);
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Baudrate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"9000000");
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Baudrate,CB_SETITEMDATA,2,(LPARAM)9000000);
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Baudrate,CB_SETCURSEL,2,0);
	}		

	// У��λ(0��None; 1��Odd; 2��Even; 3��Mark; 4��Space)
	{
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Parity:None");
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_SETITEMDATA,0,(LPARAM)0);
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Parity:Odd");
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_SETITEMDATA,1,(LPARAM)1);
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Parity:Even");
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_SETITEMDATA,2,(LPARAM)2);
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_SETCURSEL,0,0);
	}
	// ֹͣλ��(0��1ֹͣλ; 1��1.5ֹͣλ; 2��2ֹͣλ)��
	{
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_StopBits,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Stop bit:1");
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_StopBits,CB_SETITEMDATA,0,(LPARAM)0);
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_StopBits,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Stop bit:2");
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_StopBits,CB_SETITEMDATA,1,(LPARAM)2);
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_StopBits,CB_SETCURSEL,0,0);
	}
	// ����λ��(5,6,7,8,16)
	{
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_DataSize,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Data bits:8");
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_DataSize,CB_SETITEMDATA,0,(LPARAM)8);
		//SendDlgItemMessage(UartDebugHwnd,IDC_Uart_DataSize,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Data bits:16");
		//SendDlgItemMessage(UartDebugHwnd,IDC_Uart_DataSize,CB_SETITEMDATA,4,(LPARAM)16);
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_DataSize,CB_SETCURSEL,0,0);
	}
	//Timeout time, unit 100uS
	SetDlgItemInt(UartDebugHwnd,IDC_Uart_Timeout,0,FALSE);

	SendDlgItemMessage(UartDebugHwnd,IDC_RefreshObjList,BM_CLICK,0,0);	
	EnableButtonEnable_Uart();
}
//���豸
BOOL Uart_OpenDevice()
{
	//��ȡ�豸���
	UartIndex = SendDlgItemMessage(UartDebugHwnd,IDC_ObjList,CB_GETCURSEL,0,0);
	if(UartIndex==CB_ERR)
	{
		DbgPrint("Failed to open the device, please select the device first.");
		goto Exit; //�˳�
	}	
	UartDevIsOpened = (CH347Uart_Open(UartIndex) != INVALID_HANDLE_VALUE);
	CH347Uart_SetTimeout(UartIndex,500,500);
	StopTxThread = FALSE;
	StopRxThread = FALSE;

	DbgPrint(">>%d #Device open...%s",UartIndex,UartDevIsOpened?"success":"fail");

	EnableButtonEnable_Uart();
Exit:
	return UartDevIsOpened;
}

//�ر��豸
BOOL Uart_CloseDevice()
{
	StopTxThread = TRUE;
	StopRxThread = TRUE;
	Sleep(300);
	CH347Uart_Close(UartIndex);
	UartDevIsOpened = FALSE;
	DbgPrint(">>%d#�豸�ѹر�",UartIndex);

	UartIndex = CB_ERR;
	EnableButtonEnable_Uart();
	return TRUE;
}

BOOL Uart_Set()
{	
	BOOL RetVal = FALSE;
	ULONG Baudrate,Sel;
	UCHAR StopBits,Parity,DataSize,Timeout;
	CHAR FmtStr[64]="";

	GetDlgItemText(UartDebugHwnd,IDC_Uart_Baudrate,FmtStr,sizeof(FmtStr));
	Baudrate = atol(FmtStr);

	Sel = SendDlgItemMessage(UartDebugHwnd,IDC_Uart_StopBits,CB_GETCURSEL,0,0);
	StopBits = (UCHAR)SendDlgItemMessage(UartDebugHwnd,IDC_Uart_StopBits,CB_GETITEMDATA,Sel,0);

	Sel = SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_GETCURSEL,0,0);
	Parity = (UCHAR)SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_GETITEMDATA,Sel,0);

	Sel = SendDlgItemMessage(UartDebugHwnd,IDC_Uart_DataSize,CB_GETCURSEL,0,0);
	DataSize = (UCHAR)SendDlgItemMessage(UartDebugHwnd,IDC_Uart_DataSize,CB_GETITEMDATA,Sel,0);

	Timeout = (UCHAR)GetDlgItemInt(UartDebugHwnd,IDC_Uart_Timeout,NULL,FALSE);

	RetVal = CH347Uart_Init(UartIndex,Baudrate,DataSize,Parity,StopBits,Timeout);
	DbgPrint("Uart_Set %s,Baudrate:%d,DataSize:%d,Parity:%d,StopBits:%d,Timeout:%",RetVal?"succ":"failure",
		Baudrate,DataSize,Parity,StopBits,Timeout);

	return RetVal;
}

VOID UpdateTRxCountShow()
{
	CHAR FmtStr[256]="";

	if(TxFileSize)
		sprintf(FmtStr,"Tx:%d,Rx:%d,%.1f%%",TotalTxCnt,TotalRxCnt,(float)TotalTxCnt*100/TxFileSize);
	else
		sprintf(FmtStr,"Tx:%d,Rx:%d",TotalTxCnt,TotalRxCnt);
	SetDlgItemText(UartDebugHwnd,IDC_TotalCntTRx,FmtStr);
	return;
}

BOOL Uart_Write()
{
	ULONG OutLen,i,StrLen;
	UCHAR OutBuf[4096] = "";
	CHAR FmtStr[4096*3*6] = "",ValStr[16]="";
	double BT,UseT;
	BOOL RetVal = FALSE;

	StrLen = GetDlgItemText(UartDebugHwnd,IDC_Uart_WriteData,FmtStr,sizeof(FmtStr));
	if(StrLen > 4096*3)
		StrLen = 4096*3;
	OutLen = 0;
	for(i=0;i<StrLen;i+=3)
	{		
		memcpy(&ValStr[0],&FmtStr[i],2);
		OutBuf[OutLen] = (UCHAR)mStrToHEX(ValStr);
		OutLen++;
	}
	SetDlgItemInt(UartDebugHwnd,IDC_Uart_WriteLen,OutLen,FALSE);	
	
	if(OutLen<1)
	{
		DbgPrint("length not specified");
		return FALSE;
	}		
	BT = GetCurrentTimerVal();		
	RetVal = CH347Uart_Write(UartIndex,OutBuf,&OutLen);
	UseT = GetCurrentTimerVal()-BT;
	if(RetVal)
	{
		TotalTxCnt += OutLen;
		UpdateTRxCountShow();
	}

	DbgPrint("Uart_Write %dB %s time cost%.3fS",OutLen,RetVal?"succ":"failure",UseT/1000);
	
	return RetVal;
}

BOOL Uart_Read()
{
	ULONG InLen,i;
	UCHAR InBuf[4096] = "";
	CHAR FmtStr[4096*3*6] = "",ValStr[16]="";
	double BT,UseT;
	BOOL RetVal = FALSE;	

	InLen = GetDlgItemInt(UartDebugHwnd,IDC_Uart_ReadLen,NULL,FALSE);	
	if(InLen>4096)
		InLen = 4096;	
	SetDlgItemText(UartDebugHwnd,IDC_Uart_ReadData,"");
	memset(FmtStr,0,sizeof(FmtStr));
	
	if(InLen<1)
	{
		DbgPrint("length not specified");
		return FALSE;
	}
	BT = GetCurrentTimerVal();
	RetVal = CH347Uart_Read(UartIndex,InBuf,&InLen);
	UseT = GetCurrentTimerVal()-BT;
	
	DbgPrint("CH347Uart_Read %dB %s time cost%.3fS.",InLen,RetVal?"succ":"failure",UseT/1000);
	
	if(RetVal)
	{		
		if(InLen)
		{//��ӡ
			memset(FmtStr,0,sizeof(FmtStr));			
			for(i=0;i<InLen;i++)
			{
				sprintf(&FmtStr[strlen(FmtStr)],"%02X ",InBuf[i]);
			}
			SetDlgItemText(UartDebugHwnd,IDC_Uart_ReadData,FmtStr);		
			TotalRxCnt += InLen;
			UpdateTRxCountShow();
		}
		else
		{			
			SetDlgItemText(UartDebugHwnd,IDC_Uart_ReadData,"");
		}
		SetDlgItemInt(UartDebugHwnd,IDC_Uart_ReadLen,InLen,FALSE);		
	}
	return RetVal;
}

DWORD WINAPI UartAutoRecvToFileThread(LPVOID lpParameter)
{
	CHAR FileName[MAX_PATH] = "",FmtStr[4096]="";
	OPENFILENAME mOpenFile={0};
	ULONG RLen,i;
    UCHAR RBuf[4096]="";
	BOOL RetVal = FALSE;

	DbgPrint(">>**>>The serial port data receiving thread starts.");

	EnableWindow(GetDlgItem(UartDebugHwnd,IDC_Uart_StopRxThread),TRUE);
	UartAutoRecvIsStart = TRUE;
	
	while( 1 )
	{
		if(StopRxThread)
		{
			DbgPrint("Abort serial read, exit");
			break;
		}
		if(!UartDevIsOpened)
		{
			DbgPrint("Serial port is closed, exit");
			break;
		}		

		RLen = 4096;
		RetVal = CH347Uart_Read(UartIndex,RBuf,&RLen);
		if(!RetVal)
		{
			DbgPrint("UartAutoRecvToFIle.CH347Uart_Read err,break");
			break;
		}
		if(RLen)
		{
			if(UartAutoRecvToFile)
			{
				if( !WriteFile(hRxFile,RBuf,RLen,&RLen,NULL) )
				{
					ShowLastError("UartAutoRecvToFIle.WriteFile");
					break;
				}
			}
			if(UartAutoRecvShow)
			{
				FmtStr[0]=0;
				for(i=0;i<RLen;i++)
					sprintf(&FmtStr[strlen(FmtStr)],"%02X ",RBuf[i]);
				SendDlgItemMessage(UartDebugHwnd,IDC_Uart_ReadData,EM_SETSEL,0xFFFFFFFE,0xFFFFFFFE);
				SendDlgItemMessage(UartDebugHwnd,IDC_Uart_ReadData,EM_REPLACESEL,0,(LPARAM)FmtStr);
				SendDlgItemMessage(UartDebugHwnd,IDC_Uart_ReadData,EM_SETSEL,0xFFFFFFFE,0xFFFFFFFE);
			}
			SetDlgItemInt(UartDebugHwnd,IDC_Uart_ReadLen,RLen,FALSE);
			TotalRxCnt += RLen;
			UpdateTRxCountShow();
		}
		Sleep(0);
	}
//Exit:
	if(hRxFile!=INVALID_HANDLE_VALUE)
	{
		CloseHandle(hRxFile);
		hRxFile = INVALID_HANDLE_VALUE;
	}
	CheckDlgButton(UartDebugHwnd,IDC_EnAutoRecvToFile,BST_UNCHECKED);
	CheckDlgButton(UartDebugHwnd,IDC_EnAutoRecvShow,BST_UNCHECKED);	
	UartAutoRecvIsStart = FALSE;
	UartAutoRecvToFile = FALSE;
	DbgPrint("<<**<<The serial port data receiving thread exits, the cumulative receiving: %dB",TotalRxCnt);
	EnableWindow(GetDlgItem(UartDebugHwnd,IDC_Uart_StopRxThread),FALSE);

	return 0;
}

DWORD WINAPI UartTxFileThread(LPVOID lpParameter)
{
	ULONG FlashAddr = 0;
	double BT = 0;
	BOOL RetVal = FALSE;
	CHAR FileName[MAX_PATH]="";
	HANDLE hFile=INVALID_HANDLE_VALUE;
	PUCHAR FileBuf=NULL,RBuf=NULL;
	ULONG FileSize,RLen,UnitSize,RI,WLen;
	double UsedT = 0;
	OPENFILENAME mOpenFile={0};

	DbgPrint("*>>*Serial port sending file thread starts...");
	// Fill in the OPENFILENAME structure to support a template and hook.
	mOpenFile.lStructSize = sizeof(OPENFILENAME);
	mOpenFile.hwndOwner         = UartDebugHwnd;
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
	mOpenFile.lpstrTitle        = "Select the data file to be written";

	mOpenFile.nFileOffset       = 0;
	mOpenFile.nFileExtension    = 0;
	mOpenFile.lpstrDefExt       = NULL;
	mOpenFile.lCustData         = 0;
	mOpenFile.lpfnHook 		   = NULL;
	mOpenFile.lpTemplateName    = NULL;
	mOpenFile.Flags             = OFN_SHOWHELP | OFN_EXPLORER | OFN_READONLY | OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&mOpenFile))
	{ 	
		DbgPrint("Uart send data from file:%s",FileName);
	}
	else
		goto Exit;

	if(strlen(FileName) < 1)
	{
		DbgPrint("Invalid file, please select again");
		goto Exit;
	}	
	hFile = CreateFile(FileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);//���ļ�
	if(hFile == INVALID_HANDLE_VALUE)
	{
		ShowLastError("UartTxFileThread.CreateFile");
		goto Exit;
	}
	FileSize = GetFileSize(hFile,NULL);
	DbgPrint("UartTxFileThread filesize:%d",FileSize);
	TxFileSize = FileSize;
	
	FileBuf = (PUCHAR)malloc(4096);
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

	BT = GetCurrentTimerVal();
	UnitSize = 512;
	RI = 0;
	while(RI<FileSize)
	{		
		if(StopTxThread)
		{
			DbgPrint("��ֹ����д,�˳�");
			break;
		}
		if(!UartDevIsOpened)
		{
			DbgPrint("�����ѹرգ��˳�");
			break;
		}

		if( (FileSize-RI) > UnitSize )
			RLen = UnitSize;
		else
			RLen = FileSize-RI;
		
		SetFilePointer(hFile,RI,NULL,FILE_BEGIN);
		if( !ReadFile(hFile,FileBuf,RLen,&RLen,NULL) )
		{
			ShowLastError("UartTxFileThread.ReadFile");
			goto Exit;
		}
		if(RLen<1)
		{
			DbgPrint("UartTxFileThread.�ѵ��ļ�ĩβ,�˳�");
			break;
		}
		WLen = RLen;
		RetVal = CH347Uart_Write(UartIndex,FileBuf,&WLen);		
		if( !RetVal )
		{
			DbgPrint("UartTxFileThread.��%X��д%dB����ʧ��",RI,WLen);
			break;
		}
		TotalTxCnt += WLen;
		RI += WLen;
		if( ( RLen != WLen ) && (WLen) )
		{
			DbgPrint("UartTxFileThread.д���ݲ�����(0x%X-0x%X)",RLen,WLen);
			//break;
		}
		UpdateTRxCountShow();
		Sleep(0);
	}
	RetVal = TRUE;
	UsedT = (GetCurrentTimerVal()-BT)/1000;	
	DbgPrint("*<<*�ۼƷ���%dB,ƽ���ٶ�:%.3fKB/S,��ʱ%.3fS",RI,RI/UsedT/1000,UsedT);
Exit:
	if(hFile != INVALID_HANDLE_VALUE) 
	{
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}
	if(FileBuf!=NULL)
		free(FileBuf);

	DbgPrint("*<<*���ڷ����ļ��߳��˳�");
	DbgPrint("\r\n");	

	return RetVal;
}

//ʹ�ܲ�����ť�����ȴ򿪺�����JTAG�������޷�����
VOID EnableButtonEnable_Uart()
{
	//���´�/�ر��豸��ť״̬
	EnableWindow(GetDlgItem(UartDebugHwnd,IDC_CloseDevice),UartDevIsOpened);
	EnableWindow(GetDlgItem(UartDebugHwnd,IDC_OpenDevice),!UartDevIsOpened);

	EnableWindow(GetDlgItem(UartDebugHwnd,IDC_ObjList),!UartDevIsOpened);
	EnableWindow(GetDlgItem(UartDebugHwnd,IDC_RefreshObjList),!UartDevIsOpened);

	EnableWindow(GetDlgItem(UartDebugHwnd,IDC_Uart_Set),UartDevIsOpened);
	EnableWindow(GetDlgItem(UartDebugHwnd,IDC_Uart_Write),UartDevIsOpened);
	EnableWindow(GetDlgItem(UartDebugHwnd,IDC_Uart_Read),UartDevIsOpened);
}


BOOL APIENTRY DlgProc_UartDbg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case WM_INITDIALOG:
		UartDebugHwnd = hWnd;
		AfxActiveHwnd = hWnd;
		CheckDlgButton(hWnd,IDC_EnablePnPAutoOpen_Uart,BST_CHECKED);
		EnableWindow(GetDlgItem(UartDebugHwnd,IDC_Uart_StopRxThread),FALSE);
		SetEditlInputMode(UartDebugHwnd,IDC_Uart_WriteData,1);
		SetEditlInputMode(UartDebugHwnd,IDC_Uart_ReadData,1);
		Uart_InitParam();		
		//ΪUSB2.0�����豸���ò���Ͱγ���֪ͨ.������Զ����豸,�γ���ر��豸
		//if(CH347Uart_SetDeviceNotify(UartIndex,UartUsbID, Uart_UsbDevPnpNotify) )       //�豸���֪ͨ�ص�����
		//	DbgPrint("�ѿ���USB�豸��μ���");
		break;	
	//case WM_UartArrive:
	//	DbgPrint("****����CH347�豸����U��,���豸");
	//	//��ö��USB�豸
	//	SendDlgItemMessage(UartDebugHwnd,IDC_RefreshObjList,BM_CLICK,0,0);
	//	//���豸
	//	SendDlgItemMessage(UartDebugHwnd,IDC_OpenDevice,BM_CLICK,0,0);
	//	break;
	//case WM_UartRemove:
	//	DbgPrint("****����CH347�Ѵ�USB���Ƴ�,�ر��豸");
	//	//�ر��豸
	//	SendDlgItemMessage(UartDebugHwnd,IDC_CloseDevice,BM_CLICK,0,0);
	//	SendDlgItemMessage(UartDebugHwnd,IDC_RefreshObjList,BM_CLICK,0,0);
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
		case IDC_ObjList:
			Uart_ShowDevInfor();
			break;
		case IDC_UartSendFile:
			if(!UartDevIsOpened)
			{
				CheckDlgButton(UartDebugHwnd,IDC_EnAutoRecvToFile,BST_UNCHECKED);
				CheckDlgButton(UartDebugHwnd,IDC_EnAutoRecvShow,BST_UNCHECKED);				
				DbgPrint("�ȴ��豸");
				break;
			}
			TotalTxCnt = TotalRxCnt = 0;
			TxFileSize = 0;
			StopTxThread = FALSE;			
			CloseHandle(CreateThread(NULL,0,UartTxFileThread,(PVOID)0,0,NULL));
			break;
		case IDC_Uart_StopTxThread:
			StopTxThread = TRUE;
			break;
		case IDC_EnAutoRecvToFile:
		case IDC_EnAutoRecvShow:
			if(wmEvent==BN_CLICKED)
			{
				BOOL UartAutoRecvToFile2;
				if(!UartDevIsOpened)
				{
					CheckDlgButton(UartDebugHwnd,IDC_EnAutoRecvToFile,BST_UNCHECKED);
					CheckDlgButton(UartDebugHwnd,IDC_EnAutoRecvShow,BST_UNCHECKED);					
					DbgPrint("�ȴ��豸");
					break;
				}			
				UartAutoRecvShow = (IsDlgButtonChecked(UartDebugHwnd,IDC_EnAutoRecvShow)==BST_CHECKED);
				UartAutoRecvToFile2 = (IsDlgButtonChecked(UartDebugHwnd,IDC_EnAutoRecvToFile)==BST_CHECKED);
				if( UartAutoRecvShow | UartAutoRecvToFile2 )
				{
					if(UartAutoRecvToFile2 && (UartAutoRecvToFile2!=UartAutoRecvToFile))
					{
						CHAR FileName[MAX_PATH] = "";
						OPENFILENAME mOpenFile={0};

						UartAutoRecvToFile = UartAutoRecvToFile2;
						// Fill in the OPENFILENAME structure to support a template and hook.
						mOpenFile.lStructSize = sizeof(OPENFILENAME);
						mOpenFile.hwndOwner         = UartDebugHwnd;
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
						mOpenFile.lpstrTitle        = "ѡ������ļ�";

						mOpenFile.nFileOffset       = 0;
						mOpenFile.nFileExtension    = 0;
						mOpenFile.lpstrDefExt       = NULL;
						mOpenFile.lCustData         = 0;
						mOpenFile.lpfnHook 		   = NULL;
						mOpenFile.lpTemplateName    = NULL;
						mOpenFile.Flags             = OFN_SHOWHELP | OFN_EXPLORER | OFN_READONLY | OFN_FILEMUSTEXIST;
						if (GetSaveFileName(&mOpenFile))
						{
							DbgPrint("*>>*UartData will save to %s",FileName);

							hRxFile = CreateFile(FileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_WRITE|FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_ARCHIVE,NULL);
							if(hRxFile==INVALID_HANDLE_VALUE)
							{
								ShowLastError("UartAutoRecvToFIle.CreateFile");
								//goto Exit;
							}
						}
						else
						{
							if( !UartAutoRecvShow )
								break;
						}
					}
					if(!UartAutoRecvIsStart)
					{
						StopRxThread = FALSE;
						CloseHandle(CreateThread(NULL,0,UartAutoRecvToFileThread,(PVOID)0,0,NULL));
					}
				}
				else
					StopRxThread = TRUE;
			}
			break;
		case IDC_Uart_StopRxThread:
			StopRxThread = TRUE;
			break;
		case IDC_RefreshObjList:
			Uart_EnumDevice();
			break;
		case IDC_CloseDevice:
			Uart_CloseDevice();
			break;
		case IDC_OpenDevice:
			Uart_OpenDevice();
			break;
		case IDC_Uart_Set:			
			Uart_Set();
			break;
		case IDC_Uart_Write:
			Uart_Write();
			break;
		case IDC_Uart_Read:
			Uart_Read();
			break;
		case IDC_ClearInfor:
			SetDlgItemText(hWnd,IDC_InforShow,"");
			break;
		case IDC_ResetCnt:
			TotalTxCnt=TotalRxCnt=0;
			SetDlgItemText(hWnd,IDC_TotalCntTRx,"Tx:0,Rx:0");
			break;
		case WM_DESTROY:
			SendDlgItemMessage(hWnd,IDC_CloseDevice,BM_CLICK,0,0);
			//CH347Uart_SetDeviceNotify(UartIndex,UartUsbID,NULL);
			DestroyWindow(hWnd);
			break;
		default:
			DefWindowProc(hWnd, message, wParam, lParam);
			break;
		}
		break;		
		case WM_DESTROY:
			PostQuitMessage(0);
			break;		
	}
	return 0;
}

