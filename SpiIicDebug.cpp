/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2022                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  CH347 SPI/I2C½Ó¿ÚÊý¾ÝÁ÷²Ù×÷

Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2022 Nanjing Qinheng Microelectronics Co., Ltd.
Revision History:
  4/3/2022: TECH30
--*/

#include "Main.h"

#define WM_CH347DevArrive WM_USER+10         //Éè±¸²åÈëÍ¨ÖªÊÂ¼þ,´°Ìå½ø³Ì½ÓÊÕ
#define WM_CH347DevRemove WM_USER+11         //Éè±¸°Î³öÍ¨ÖªÊÂ¼þ,´°Ìå½ø³Ì½ÓÊÕ

#define CH347DevID "VID_1A86&PID_55D\0"  //¼àÊÓCH347 USB²å°Î¶¯×÷,²åÈëÊ±¿É×Ô¶¯´ò¿ªÉè±¸;¹Ø±ÕÊ±×Ô¶¯¹Ø±ÕÉè±¸

//ÈçÐè×¼È·¼à²â¸÷Ä£Ê½ÏÂ´®¿Ú²å°Î¶¯×÷£¬¿ÉÐ´ÈçÏÂÍêÕûUSBID¡£ÒòDEMO»ã×ÜÁËËùÓÐÄ£Ê½£¬ËùÒÔÖ»ÐèÈ¡ID¹²Í¬²¿·Ö
//ÉèÖÃÖ¸¶¨Éè±¸µÄUSB²å°Î¼à²â: CH347SetDeviceNotify(,USBID_Mode???,)
//È¡ÏûÖ¸¶¨Éè±¸µÄUSB²å°Î¼à²â: CH347SetDeviceNotify(,USBID_Mode???,)
//#define USBID_VCP_Mode0_UART0       "VID_1A86&PID_55DA&MI_00\0"  //MODE0 UART0
//#define USBID_VCP_Mode0_UART1       "VID_1A86&PID_55DA&MI_01\0"  //MODE0 UART1
//#define USBID_VCP_Mode0_UART        "VID_1A86&PID_55DA\0"        //MODE0 UART
//#define USBID_VEN_Mode1_UART1       "VID_1A86&PID_55DB&MI_00\0"  //MODE1 UART
//#define USBID_HID_Mode2_UART1       "VID_1A86&PID_55DB&MI_00\0"  //MODE2 UART
//#define USBID_VEN_Mode3_UART1       "VID_1A86&PID_55DB&MI_00\0"  //MODE3 UART

//ÈçÐè×¼È·¼à²â¸÷Ä£Ê½ÏÂ½Ó¿Ú²å°Î¶¯×÷£¬¿ÉÐ´ÈçÏÂÍêÕûUSBID¡£ÒòDEMO»ã×ÜÁËËùÓÐÄ£Ê½£¬ËùÒÔÖ»ÐèÈ¡ID¹²Í¬²¿·Ö
//ÉèÖÃÖ¸¶¨Éè±¸µÄUSB²å°Î¼à²â: CH347Uart_SetDeviceNotify(,USBID_Mode???,)
//È¡ÏûÖ¸¶¨Éè±¸µÄUSB²å°Î¼à²â: CH347Uart_SetDeviceNotify(,USBID,)
//#define USBID_VEN_Mode1_SPI_I2C     "VID_1A86&PID_55DA&MI_00\0"  //MODE1 SPI/I2C
//#define USBID_HID_Mode2_SPI_I2C     "VID_1A86&PID_55DA&MI_00\0"  //MODE2 SPI/I2C
//#define USBID_VEN_Mode3_JTAG_I2C    "VID_1A86&PID_55DA&MI_00\0"  //MODE3 JTAG/I2C

extern HINSTANCE AfxMainIns; //½ø³ÌÊµÀý 
extern HWND AfxActiveHwnd;
extern HWND JtagDlgHwnd;
extern HWND FlashEepromDbgHwnd;
extern HWND UartDebugHwnd;
extern BOOL EnablePnPAutoOpen_Jtag; //ÆôÓÃ²å°ÎºóÉè±¸×Ô¶¯´ò¿ª¹Ø±Õ¹¦ÄÜ
extern BOOL EnablePnPAutoOpen_Uart; //ÆôÓÃ²å°ÎºóÉè±¸×Ô¶¯´ò¿ª¹Ø±Õ¹¦ÄÜ
extern BOOL EnablePnPAutoOpen_Flash; //ÆôÓÃ²å°ÎºóÉè±¸×Ô¶¯´ò¿ª¹Ø±Õ¹¦ÄÜ


//È«¾Ö±äÁ¿
HWND SpiI2cGpioDebugHwnd;     //´°Ìå¾ä±ú
BOOL DevIsOpened;   //Éè±¸ÊÇ·ñ´ò¿ª
BOOL SpiIsCfg;
ULONG SpiI2cGpioDevIndex;
mDeviceInforS SpiI2cDevInfor[16] = {0};
BOOL EnablePnPAutoOpen; //ÆôÓÃ²å°ÎºóÉè±¸×Ô¶¯´ò¿ª¹Ø±Õ¹¦ÄÜ
// CH347Éè±¸²å°Î¼ì²âÍ¨Öª³ÌÐò.Òò»Øµ÷º¯Êý¶Ôº¯Êý²Ù×÷ÓÐÏÞÖÆ£¬Í¨¹ý´°ÌåÏûÏ¢×ªÒÆµ½ÏûÏ¢´¦Àíº¯ÊýÄÚ½øÐÐ´¦Àí¡£
VOID	 CALLBACK	 UsbDevPnpNotify (ULONG iEventStatus ) 
{
	if(iEventStatus==CH347_DEVICE_ARRIVAL)// Éè±¸²åÈëÊÂ¼þ,ÒÑ¾­²åÈë
		PostMessage(SpiI2cGpioDebugHwnd,WM_CH347DevArrive,0,0);
	else if(iEventStatus==CH347_DEVICE_REMOVE)// Éè±¸°Î³öÊÂ¼þ,ÒÑ¾­°Î³ö
		PostMessage(SpiI2cGpioDebugHwnd,WM_CH347DevRemove,0,0);
	return;
}

//´ò¿ªÉè±¸
BOOL OpenDevice()
{
	//»ñÈ¡Éè±¸ÐòºÅ
	SpiI2cGpioDevIndex = SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_ObjList,CB_GETCURSEL,0,0);
	if(SpiI2cGpioDevIndex==CB_ERR)
	{
		DbgPrint("Failed to open the device, please select the device first");
		goto Exit; //ÍË³ö
	}	
	DevIsOpened = (CH347OpenDevice(SpiI2cGpioDevIndex) != INVALID_HANDLE_VALUE);
	CH347SetTimeout(SpiI2cGpioDevIndex,500,500);
	DbgPrint(">>device open...%s",DevIsOpened?"success":"fail");
Exit:
	return DevIsOpened;
}

//¹Ø±ÕÉè±¸
BOOL CloseDevice()
{
	CH347CloseDevice(SpiI2cGpioDevIndex);
	DevIsOpened = FALSE;
	DbgPrint(">>Device is turned off");

	return TRUE;
}

BOOL CH347InitSpi()
{	
	BOOL RetVal = FALSE;
	mSpiCfgS SpiCfg = {0};	
	UCHAR HwVer = 0;

	SpiCfg.iMode = (UCHAR)SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_SpiCfg_Mode,CB_GETCURSEL,0,0);
	SpiCfg.iClock = (UCHAR)SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_SpiCfg_Clock,CB_GETCURSEL,0,0);
	SpiCfg.iByteOrder = (UCHAR)SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_SpiCfg_ByteBitOrder,CB_GETCURSEL,0,0);
	SpiCfg.iSpiWriteReadInterval = GetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiCfg_OutInIntervalT,NULL,FALSE);
	SpiCfg.iSpiOutDefaultData = 0xFF;
	SpiCfg.iChipSelect = SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_SpiCfg_ChipIndex,CB_GETCURSEL,0,0);	
	SpiCfg.CS1Polarity = (UCHAR)SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_SpiCfg_CS1Polarity,CB_GETCURSEL,0,0);	
	SpiCfg.CS2Polarity = (UCHAR)SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_SpiCfg_CS2Polarity,CB_GETCURSEL,0,0);
	if(IsDlgButtonChecked(SpiI2cGpioDebugHwnd,IDC_EnableCS)==BST_CHECKED)
		SpiCfg.iChipSelect |= 0x80;
	SpiCfg.iIsAutoDeativeCS = (IsDlgButtonChecked(SpiI2cGpioDebugHwnd,IDC_AutoDeativeCS)==BST_CHECKED);
	SpiCfg.iActiveDelay = GetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiCfg_ActiveDelay,NULL,FALSE);;
	SpiCfg.iDelayDeactive = GetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiCfg_DelayDeactive,NULL,FALSE);;

	RetVal = CH347SPI_Init(SpiI2cGpioDevIndex,&SpiCfg);
	DbgPrint("CH347SPI_Init %s",RetVal?"succ":"failure");

	return RetVal;
}

BOOL CH347SpiCsCtrl()
{
	USHORT          iEnableSelect;      // µÍ°ËÎ»ÎªCS1£¬¸ß°ËÎ»ÎªCS2; ×Ö½ÚÖµÎª1=ÉèÖÃCS,Îª0=ºöÂÔ´ËCSÉèÖÃ
	USHORT          iChipSelect;		// µÍ°ËÎ»ÎªCS1£¬¸ß°ËÎ»ÎªCS2;Æ¬Ñ¡Êä³ö,0=L,1=H
	USHORT          iIsAutoDeativeCS;   // µÍ°ËÎ»ÎªCS1£¬¸ß°ËÎ»ÎªCS2;²Ù×÷Íê³ÉºóÊÇ·ñ×Ô¶¯³·ÏûÆ¬Ñ¡
    ULONG           iActiveDelay;		// µÍ°ËÎ»ÎªCS1£¬¸ß°ËÎ»ÎªCS2;ÉèÖÃÆ¬Ñ¡ºóÖ´ÐÐ¶ÁÐ´²Ù×÷µÄÑÓÊ±Ê±¼ä
	ULONG           iDelayDeactive;		// µÍ°ËÎ»ÎªCS1£¬¸ß°ËÎ»ÎªCS2;³·ÏûÆ¬Ñ¡ºóÖ´ÐÐ¶ÁÐ´²Ù×÷µÄÑÓÊ±Ê±¼ä
	UCHAR           CsSel;
	BOOL RetVal;
	mSpiCfgS SpiCfg = {0};

	CsSel = (UCHAR)SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_SpiCfg_ChipIndex,CB_GETCURSEL,0,0);	
	iEnableSelect = ( IsDlgButtonChecked(SpiI2cGpioDebugHwnd,IDC_EnableCS)==BST_CHECKED );
	iChipSelect = (UCHAR)SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_SpiCsStatus,CB_GETCURSEL,0,0);	
	iIsAutoDeativeCS = (IsDlgButtonChecked(SpiI2cGpioDebugHwnd,IDC_AutoDeativeCS)==BST_CHECKED);
	iActiveDelay = GetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiCfg_ActiveDelay,NULL,FALSE)&0xFF;
	iDelayDeactive = GetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiCfg_DelayDeactive,NULL,FALSE)&0xFF;	
	if(iEnableSelect)
	{
		if( CsSel )
		{
			iEnableSelect = 0x0100;		
			iChipSelect = (iChipSelect<<8)&0xFF00;
			iIsAutoDeativeCS = (iIsAutoDeativeCS<<8)&0xFF00;
			iActiveDelay = (iActiveDelay<<16)&0xFFFF0000;
			iDelayDeactive = (iDelayDeactive<<16)&0xFFFF0000;

		}
		else 
		{
			iEnableSelect = 0x01;
		}
	}
	else
		iEnableSelect = 0;

	RetVal = CH347SPI_SetChipSelect(SpiI2cGpioDevIndex,iEnableSelect,iChipSelect,iIsAutoDeativeCS,iActiveDelay,iDelayDeactive);
	
	DbgPrint("CH347SPI_ConfigCS %s",RetVal?"succ":"failure");

	return RetVal;
}

BOOL CH347SpiStream(ULONG CmdCode)
{
	ULONG SpiOutLen,SpiInLen,FlashAddr=0,i,StrLen;
	UCHAR InBuf[4096] = "",OutBuf[4096] = "";
	CHAR FmtStr[4096*3*6] = "",ValStr[16]="",FmtStr2[4096*3];
	double BT,UseT;
	UCHAR ChipSelect;
	BOOL RetVal = FALSE;

	if(IsDlgButtonChecked(SpiI2cGpioDebugHwnd,IDC_EnableCS)==BST_CHECKED )
		ChipSelect = 0x80;
	else
		ChipSelect = 0x00;

	StrLen = GetDlgItemText(SpiI2cGpioDebugHwnd,IDC_SpiOut,FmtStr,sizeof(FmtStr));	
	if(StrLen > 4096*3)
		StrLen = 4096*3;
	SpiOutLen = 0;
	for(i=0;i<StrLen;i+=3)
	{		
		memcpy(&ValStr[0],&FmtStr[i],2);
		OutBuf[SpiOutLen] = (UCHAR)mStrToHEX(ValStr);
		SpiOutLen++;
	}
	SetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiOutLen,SpiOutLen,FALSE);
	SpiInLen = GetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiInLen,NULL,FALSE);	
	if(SpiInLen>4096)
		SpiInLen = 4096;	
	SetDlgItemText(SpiI2cGpioDebugHwnd,IDC_SpiIn,"");
	memset(FmtStr,0,sizeof(FmtStr));

	BT = GetCurrentTimerVal();
	if(CmdCode == 0xC2)
	{
		SpiInLen = SpiOutLen; //Êä³ö×Ö½ÚÊýÓëÊäÈë×Ö½ÚÊýÏàµÈ
		if(SpiOutLen<1)
		{
			DbgPrint("SPI read length not specified");
			return FALSE;
		}
		memcpy(InBuf,OutBuf,SpiOutLen);
		RetVal = CH347StreamSPI4(SpiI2cGpioDevIndex,ChipSelect,SpiOutLen,InBuf);
		sprintf(FmtStr,"Cmd%X(StreamSpi) %s.",CmdCode,RetVal?"succ":"failure");
	}
	else if(CmdCode == 0xC3)
	{
		if(SpiInLen<1)
		{
			DbgPrint("SPI read length not specified");
			return FALSE;
		}
		SpiOutLen = 0;
		RetVal = CH347SPI_Read(SpiI2cGpioDevIndex,ChipSelect,4,&SpiInLen,InBuf);
		sprintf(FmtStr,"Cmd%X(StreamSpiBulkRead) %s.",CmdCode,RetVal?"succ":"failure");
	}
	else if(CmdCode == 0xC4)
	{
		if(SpiOutLen<1)
		{
			DbgPrint("SPI read length not specified");
			return FALSE;
		}
		SpiInLen = 0;
		RetVal = CH347SPI_Write(SpiI2cGpioDevIndex,ChipSelect,SpiOutLen,512,OutBuf);
		sprintf(FmtStr,"Cmd%X(StreamSpiBulkWrite) %s.",CmdCode,RetVal?"succ":"failure");
	}
	else
		return FALSE;
	UseT = GetCurrentTimerVal()-BT;
	sprintf(&FmtStr[strlen(FmtStr)],",ÓÃÊ±%.3fS.",UseT/1000);
	
	if(RetVal)
	{
		if(SpiOutLen)
		{//´òÓ¡
			sprintf(&FmtStr[strlen(FmtStr)],"\r\n                    OutData(%d):",SpiOutLen);
			//16½øÖÆÏÔÊ¾
			for(i=0;i<SpiOutLen;i++)
			{			
				sprintf(&FmtStr[strlen(FmtStr)],"%02X ",OutBuf[i]);						
			}		
		}
		if(SpiInLen)
		{//´òÓ¡
			//memset(FmtStr,0,sizeof(FmtStr));
			sprintf(&FmtStr[strlen(FmtStr)],"\r\n                    InData (%d):",SpiInLen);
			//16½øÖÆÏÔÊ¾
			FmtStr2[0]=0;
			for(i=0;i<SpiInLen;i++)
			{
				sprintf(&FmtStr[strlen(FmtStr)],"%02X ",InBuf[i]);
				sprintf(&FmtStr2[strlen(FmtStr2)],"%02X ",InBuf[i]);
			}
			SetDlgItemText(SpiI2cGpioDebugHwnd,IDC_SpiIn,FmtStr2);		
			SetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiInLen,SpiInLen,FALSE);		
		}
	}
	else
	{
		SetDlgItemText(SpiI2cGpioDebugHwnd,IDC_SpiIn,"");		
		SetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiInLen,0,FALSE);
	}
	DbgPrint("%s",FmtStr);
	return RetVal;
}

BOOL I2C_WriteRead()
{
	ULONG OutLen,InLen,i,StrLen;
	UCHAR OutBuf[4096] = "",InBuf[4096] = "";
	CHAR FmtStr[4096*3*6] = "",ValStr[16]="";
	double BT,UseT;
	BOOL RetVal = FALSE;

	StrLen = GetDlgItemText(SpiI2cGpioDebugHwnd,IDC_I2COut,FmtStr,sizeof(FmtStr));
	if(StrLen > 4096*3)
		StrLen = 4096*3;
	OutLen = 0;
	for(i=0;i<StrLen;i+=3)
	{		
		memcpy(&ValStr[0],&FmtStr[i],2);
		OutBuf[OutLen] = (UCHAR)mStrToHEX(ValStr);
		OutLen++;
	}
	SetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_I2COutLen,OutLen,FALSE);	

	InLen = GetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_I2CInLen,NULL,FALSE);
	if(InLen>4096)
		InLen = 4096;

	if((OutLen+InLen)<1)
	{
		DbgPrint("Unspecified read length");
		return FALSE;
	}		
	BT = GetCurrentTimerVal();		
	RetVal = CH347StreamI2C(SpiI2cGpioDevIndex,OutLen,OutBuf,InLen,InBuf);
	UseT = GetCurrentTimerVal()-BT;
	if(RetVal)
	{		
		if(InLen)
		{//´òÓ¡
			memset(FmtStr,0,sizeof(FmtStr));			
			for(i=0;i<InLen;i++)
			{
				sprintf(&FmtStr[strlen(FmtStr)],"%02X ",InBuf[i]);
			}
			SetDlgItemText(SpiI2cGpioDebugHwnd,IDC_I2CIn,FmtStr);		
			SetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_I2CInLen,InLen,FALSE);		
		}
	}

	DbgPrint("I2C_WriteRead %s.Write:%dB,Read:%dB ,time cost %.3fS",RetVal?"succ":"failure",OutLen,InLen,UseT/1000);

	return RetVal;
}

//Ê¹ÄÜ²Ù×÷°´Å¥£¬ÐèÏÈ´ò¿ªºÍÅäÖÃJTAG£¬·ñÔòÎÞ·¨²Ù×÷
VOID EnableButtonEnable()
{
	if(!DevIsOpened)
		SpiIsCfg = FALSE;

	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_CMD_InitSPI),DevIsOpened);

	//¸üÐÂ´ò¿ª/¹Ø±ÕÉè±¸°´Å¥×´Ì¬
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_OpenDevice),!DevIsOpened);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_CloseDevice),DevIsOpened);

	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_ObjList),!DevIsOpened);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_RefreshObjList),!DevIsOpened);

	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_CMD_SPICsCtrl),SpiIsCfg);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_FlashIdentify),SpiIsCfg);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_FlashRead),SpiIsCfg);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_FlashWrite),SpiIsCfg);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_FlashErase),SpiIsCfg);

	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_ReadToFile),SpiIsCfg);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_WriteFormFile),SpiIsCfg);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_FlashVerify),SpiIsCfg);
	

	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_CMD_SPIStream),SpiIsCfg);	
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_CMD_BulkSpiIn),SpiIsCfg);	
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_CMD_BulkSpiOut),SpiIsCfg);	
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_CMD_I2C_RW),SpiIsCfg);

	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_SetGpio),DevIsOpened);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_GetGpio),DevIsOpened);
}

//ÏÔÊ¾Éè±¸ÐÅÏ¢
BOOL ShowDevInfor()
{
	ULONG ObjSel;
	CHAR  FmtStr[128]="";

	ObjSel = SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_ObjList,CB_GETCURSEL,0,0);
	if(ObjSel!=CB_ERR)
	{
		sprintf(FmtStr,"**ChipMode:%d,%s,Ver:%02X,DevID:%s",SpiI2cDevInfor[ObjSel].ChipMode,SpiI2cDevInfor[ObjSel].UsbSpeedType?"HS":"FS",SpiI2cDevInfor[ObjSel].FirewareVer,SpiI2cDevInfor[ObjSel].DeviceID);
		SetDlgItemText(SpiI2cGpioDebugHwnd,IDC_DevInfor,FmtStr);
	}	

	return (ObjSel!=CB_ERR);
}

//Ã¶¾ÙÉè±¸
ULONG EnumDevice()
{
	ULONG i,oLen,DevCnt = 0;
	USB_DEVICE_DESCRIPTOR DevDesc = {0};
	CHAR tem[256] = "";
	mDeviceInforS DevInfor = {0};

	SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_ObjList,CB_RESETCONTENT,0,0);
	for(i=0;i<16;i++)
	{
		if(CH347OpenDevice(i) != INVALID_HANDLE_VALUE)
		{
			oLen = sizeof(USB_DEVICE_DESCRIPTOR);
			CH347GetDeviceInfor(i,&DevInfor);

			if(DevInfor.ChipMode == 3) //Ä£Ê½3´Ë½Ó¿ÚÎªJTAG/I2C
				continue;

			sprintf(tem,"%d# %s",i,DevInfor.FuncDescStr);
			SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_ObjList,CB_ADDSTRING,0,(LPARAM)(LPCTSTR)tem);		
			memcpy(&SpiI2cDevInfor[DevCnt],&DevInfor,sizeof(DevInfor));
			DevCnt++;
		}
		CH347CloseDevice(i);
	}
	if(DevCnt)
	{
		SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_ObjList,CB_SETCURSEL,0,0);
		SetFocus(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_ObjList));
	}
	return DevCnt;
}

//Initialize the form
VOID InitWindows()
{	
	//Find and display a list of devices
	SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_RefreshObjList,BM_CLICK,0,0);	
	
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_OpenDevice),!DevIsOpened);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_CloseDevice),DevIsOpened);
	//Flash address bin initial value
	SetDlgItemText(SpiI2cGpioDebugHwnd,IDC_FlashStartAddr,"0");
	//Flash operand bin initial value
	SetDlgItemText(SpiI2cGpioDebugHwnd,IDC_FlashDataSize,"100");
	//Clear Flash Data Mode½
	SetDlgItemText(SpiI2cGpioDebugHwnd,IDC_FlashData,"");
	//The output box sets the maximum number of characters displayed
	SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_InforShow,EM_LIMITTEXT,0xFFFFFFFF,0);

	return;
}

BOOL APIENTRY DlgProc_SpiUartI2cDbg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	ULONG ThreadID;

	switch (message)
	{
	case WM_INITDIALOG:
		SpiI2cGpioDebugHwnd = hWnd;
		AfxActiveHwnd = hWnd;	
		// Seed the random-number generator with current time so that
		// the numbers will be different every time we run.		
		{//Ìí¼Óalt+tabÇÐ»»Ê±ÏÔÊ¾µÄÍ¼±ê
			HICON hicon;
			hicon = (HICON)LoadIcon(AfxMainIns,(LPCTSTR)IDI_Main);
			PostMessage(SpiI2cGpioDebugHwnd,WM_SETICON,ICON_BIG,(LPARAM)(HICON)hicon);
			PostMessage(SpiI2cGpioDebugHwnd,WM_SETICON,ICON_SMALL,(LPARAM)(HICON)hicon);
		}
		DevIsOpened = FALSE;		
		InitWindows(); //³õÊ¼»¯´°Ìå
		{
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode0");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode1");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode2");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode3");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Mode,CB_SETCURSEL,3,0);
		}
		{//0=60MHz, 1=30MHz, 2=15MHz, 3=7.5MHz, 4=3.75MHz, 5=1.875MHz, 6=937.5KHz£¬7=468.75KHz
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"60MHz");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"30MHz");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"15MHz");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"7.5MHz");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"3.75MHz");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"1.875MHz");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"937.5KHz");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"468.75KHz");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_SETCURSEL,1,0);
		}
		{//0=µÍÎ»ÔÚÇ°(LSB), 1=¸ßÎ»ÔÚÇ°(MSB)
			SendDlgItemMessage(hWnd,IDC_SpiCfg_ByteBitOrder,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"LSB");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_ByteBitOrder,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"MSB");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_ByteBitOrder,CB_SETCURSEL,1,0);
		}
		{
			SendDlgItemMessage(hWnd,IDC_SpiCfg_ChipIndex,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"CS1");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_ChipIndex,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"CS2");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_ChipIndex,CB_SETCURSEL,0,0);
		}
		{
			SendDlgItemMessage(hWnd,IDC_SpiCsStatus,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"CS Active");
			SendDlgItemMessage(hWnd,IDC_SpiCsStatus,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"CS Deactive");
			SendDlgItemMessage(hWnd,IDC_SpiCsStatus,CB_SETCURSEL,0,0);
		}
		{
			SendDlgItemMessage(hWnd,IDC_SpiCfg_CS1Polarity,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"CS1 POLA_LOW");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_CS1Polarity,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"CS1 POLA_HIGH");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_CS1Polarity,CB_SETCURSEL,0,0);
		}
		{
			SendDlgItemMessage(hWnd,IDC_SpiCfg_CS2Polarity,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"CS2 POLA_LOW");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_CS2Polarity,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"CS2 POLA_HIGH");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_CS2Polarity,CB_SETCURSEL,0,0);
		}
		SetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiCfg_OutInIntervalT,0,FALSE);
		SetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiCfg_ActiveDelay,0,FALSE);
		SetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiCfg_DelayDeactive,0,FALSE);
		CheckDlgButton(SpiI2cGpioDebugHwnd,IDC_EnableCS,BST_CHECKED);	
		CheckDlgButton(SpiI2cGpioDebugHwnd,IDC_EnablePnPAutoOpen,BST_CHECKED);	

		GpioEnableAll();
	
		SetEditlInputMode(hWnd,IDC_SpiOut,1);		
		SetEditlInputMode(hWnd,IDC_SpiIn,1);	
		SetEditlInputMode(hWnd,IDC_I2COut,1);		
		SetEditlInputMode(hWnd,IDC_I2CIn,1);

		EnableButtonEnable();	
		//ÎªUSB2.0JTAGÉè±¸ÉèÖÃ²åÈëºÍ°Î³öµÄÍ¨Öª.²åÈëºó×Ô¶¯´ò¿ªÉè±¸,°Î³öºó¹Ø±ÕÉè±¸
		if(CH347SetDeviceNotify(SpiI2cGpioDevIndex,CH347DevID, UsbDevPnpNotify) )       //Éè±¸²å°ÎÍ¨Öª»Øµ÷º¯Êý
			DbgPrint("USB device plugging and unplugging monitoring is enabled");
		break;	
	case WM_CH347DevArrive:
		{
			DbgPrint("****It is found that the CH347 device is inserted into the USB port, and the device is turned on");			
			Sleep(100);	//¼ì²âµ½Éè±¸²åÈë×´Ì¬Ö®ºóµ½Éè±¸Ã¶¾Ù»áÓÐÒ»¶¨µÄÖÍºó£¬²»Í¬ÏµÍ³»·¾³´æÔÚ²îÒì£¬´Ë´¦ÑÓÊ±100ms
			//SPI/I2C Debug´°Ìå		
			if(AfxActiveHwnd==SpiI2cGpioDebugHwnd)
			{				
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //ÏÈÃ¶¾ÙUSBÉè±¸		
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_OpenDevice,BM_CLICK,0,0); //´ò¿ªÉè±¸
				SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus,"device plugged in");
			}
			//Flash/Eeprom Debug´°Ìå			
			if(AfxActiveHwnd==FlashEepromDbgHwnd)
			{				
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //ÏÈÃ¶¾ÙUSBÉè±¸		
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen_Flash)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_OpenDevice,BM_CLICK,0,0); //´ò¿ªÉè±¸
				SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Flash,"device plugged in");
			}
			//Jtag Debug´°Ìå
			if(AfxActiveHwnd==JtagDlgHwnd)
			{				
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //ÏÈÃ¶¾ÙUSBÉè±¸		
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen_Jtag)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_OpenDevice,BM_CLICK,0,0); //´ò¿ªÉè±¸
				SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Jtag,"device plugged in");
			}
			//Uart Debug´°Ìå
			if(AfxActiveHwnd==UartDebugHwnd)
			{				
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //ÏÈÃ¶¾ÙUSBÉè±¸		
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen_Uart)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_OpenDevice,BM_CLICK,0,0); //´ò¿ªÉè±¸
				SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Uart,"device plugged in");
			}
		}
		break;
	case WM_CH347DevRemove:	
		{
			//¹Ø±ÕÉè±¸
			DbgPrint("****Found that CH347 has been removed from the USB port, turn off the device¸");			
			//SPI/I2C Debug´°Ìå		
			if(AfxActiveHwnd==SpiI2cGpioDebugHwnd)
			{
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_CloseDevice,BM_CLICK,0,0); //´ò¿ªÉè±¸
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //ÏÈÃ¶¾ÙUSBÉè±¸		
				SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus,"Device removal");
			}
			//Flash/Eeprom Debug´°Ìå		
			if(AfxActiveHwnd==FlashEepromDbgHwnd)
			{
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen_Flash)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_CloseDevice,BM_CLICK,0,0); //´ò¿ªÉè±¸
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //ÏÈÃ¶¾ÙUSBÉè±¸		
				SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Flash,"Device removal");
			}
			//Jtag Debug´°Ìå
			if(AfxActiveHwnd==JtagDlgHwnd)
			{
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen_Jtag)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_CloseDevice,BM_CLICK,0,0); //´ò¿ªÉè±¸
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //ÏÈÃ¶¾ÙUSBÉè±¸
				SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Jtag,"Device removal");
			}
			//Uart Debug´°Ìå
			if(AfxActiveHwnd==UartDebugHwnd)
			{
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen_Uart)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_CloseDevice,BM_CLICK,0,0); //´ò¿ªÉè±¸
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //ÏÈÃ¶¾ÙUSBÉè±¸
				SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Uart,"Device removal");
			}
		}
		break;
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
			EnumDevice();   //enumerate and display devices¸
			break;
		case IDC_ObjList:
			ShowDevInfor();
			break;
		case IDC_EnablePnPAutoOpen:
			EnablePnPAutoOpen = (IsDlgButtonChecked(hWnd,IDC_EnablePnPAutoOpen)==BST_CHECKED);
			break;
		case IDC_OpenDevice://Turn on the device¸
			OpenDevice();
			EnableButtonEnable();	//Update button state		
			break;
		case IDC_CloseDevice:			
			CloseDevice();				
			EnableButtonEnable();	//Update button state			
			break;	
		case IDC_FlashIdentify:
			FlashIdentify();
			break;
		case IDC_FlashRead:
			FlashBlockRead();
			break;
		case IDC_FlashWrite://Write the data in the IDC_FLASHDATA box to FLASH
			FlashBlockWrite();
			break;
		case IDC_FlashErase:
			FlashBlockErase();
			break;	
		case IDC_CMD_InitSPI:			
			SpiIsCfg = CH347InitSpi();
			CH347I2C_Set(SpiI2cGpioDevIndex,3); //Configure I2C speed as fast 750K
			EnableButtonEnable();
			break;
		case IDC_CMD_SPICsCtrl:
			CH347SpiCsCtrl();
			EnableButtonEnable();
			break;
		case IDC_CMD_SPIStream:
			CH347SpiStream(0xC2);
			break;
		case IDC_CMD_BulkSpiIn:
			CH347SpiStream(0xC3);
			break;
		case IDC_CMD_BulkSpiOut:
			CH347SpiStream(0xC4);
			break;
		case IDC_FlashVerify:
			CloseHandle(CreateThread(NULL,0,FlashVerifyWithFile,NULL,0,&ThreadID)); //Start USB download
			break;
		case IDC_WriteFormFile:
			CloseHandle(CreateThread(NULL,0,WriteFlashFromFile,NULL,0,&ThreadID)); //Start USB download
			break;
		case IDC_ReadToFile:
			CloseHandle(CreateThread(NULL,0,ReadFlashToFile,NULL,0,&ThreadID)); //Start USB downloadØ
			break;
		case IDC_FlashRWSpeedTest://Read and write speed
			CloseHandle(CreateThread(NULL,0,FlashRWSpeedTest,NULL,0,&ThreadID)); //Start USB download
			break;		
		case IDC_CMD_I2C_RW:
			I2C_WriteRead();
			break;
		case IDC_SetGpio://GPIO settings
			Gpio_Set();
			break;
		case IDC_GetGpio://GPIO state acquisition
			Gpio_Get();
			break;
		case IDC_GpioSetDataAll://Select all GPIO levels
			GpioSetDataAll();
			break;
		case IDC_GpioEnableAll://Enable all GPIOs
			GpioEnableAll();
			break;
		case IDC_GpioSetDirAll://Select all GPIO directions
			GpioSetDirAll();
			break;
		case IDC_ClearInfor:
			SetDlgItemText(hWnd,IDC_InforShow,"");
			break;
		case WM_DESTROY:			
			SendDlgItemMessage(hWnd,IDC_CloseDevice,BM_CLICK,0,0);
			CH347SetDeviceNotify(SpiI2cGpioDevIndex,CH347DevID,NULL);
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

