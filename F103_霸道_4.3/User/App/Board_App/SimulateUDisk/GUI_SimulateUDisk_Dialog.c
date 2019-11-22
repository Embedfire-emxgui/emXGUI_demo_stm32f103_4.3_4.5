#include <emXGUI.h>
#include <string.h>
#include <stdio.h>
#include "x_libc.h"
#include "GUI_AppDef.h"
#include "./SimulateUDisk/GUI_SimulateUDisk_Dialog.h"
#include "emXGUI_JPEG.h"
#include "emxgui_png.h"
#include 	"hw_config.h" 
#include 	"usb_lib.h"
#include 	"usb_pwr.h"     

static HDC hdc_bk;
static HDC hdc_btn_press;
static HDC hdc_btn;

//退出按钮重绘制
static void _ExitButton_OwnerDraw(DRAWITEM_HDR *ds)
{
  HDC hdc;
  RECT rc;
//  HWND hwnd;

	hdc = ds->hDC;   
	rc = ds->rc; 
//  hwnd = ds->hwnd;

//  GetClientRect(hwnd, &rc_tmp);//得到控件的位置
//  WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//坐标转换

//  BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);

  if (ds->State & BST_PUSHED)
	{ //按钮是按下状态
		SetPenColor(hdc, MapRGB(hdc, 1, 191, 255));
	}
	else
	{ //按钮是弹起状态

		SetPenColor(hdc, MapRGB(hdc, 250, 250, 250));      //设置画笔色
	}
  
  for(int i=0; i<4; i++)
  {
    HLine(hdc, rc.x, rc.y, rc.w);
    rc.y += 5;
  }

}

static void btn_owner_draw(DRAWITEM_HDR *ds) //绘制一个按钮外观
{
	HDC hdc;
	RECT rc, rc_tmp;
  WCHAR wbuf[128];
  HWND hwnd;
  
  hwnd = ds->hwnd;
	hdc = ds->hDC;   //button的绘图上下文句柄.
	rc = ds->rc;     //button的绘制矩形区.

  GetClientRect(hwnd, &rc_tmp);//得到控件的位置
  WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//坐标转换

  BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);

  if (ds->State & BST_PUSHED)
  { //按钮是按下状态
    BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_btn_press, 0, 0, SRCCOPY);
    SetTextColor(hdc, MapRGB(hdc, 200, 200, 200));
  }
  else
  { //按钮是弹起状态
    BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_btn, 0, 0, SRCCOPY);
    SetTextColor(hdc, MapRGB(hdc, 255, 255, 255));
  }
  
  GetWindowText(ds->hwnd, wbuf, 128); //获得按钮控件的文字
  
  /* 显示文本 */
	DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_CENTER);//绘制文字(居中对齐方式)
}

static void USB_GPIO_init(void)
{

 	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;	
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  
  GPIO_SetBits(GPIOD, GPIO_Pin_3);
}	

static LRESULT	win_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch(msg)
  {
    case WM_CREATE:
    {
      RECT rc;
      
      USB_GPIO_init();
      
      GetClientRect(hwnd, &rc);
                      
      CreateWindow(BUTTON, L"O", WS_TRANSPARENT|BS_FLAT | BS_NOTIFY |WS_OWNERDRAW|WS_VISIBLE,
                  286, 10, 23, 23, hwnd, eID_SUD_EXIT, NULL, NULL);

      CreateWindow(BUTTON, L"连接", WS_TRANSPARENT| BS_NOTIFY | WS_VISIBLE | BS_3D|WS_OWNERDRAW,
                  123, 197,  71,  30, hwnd, eID_SUD_LINK, NULL, NULL);    // 使用时钟的按钮背景
      
      BOOL res;
      u8 *jpeg_buf;
      u32 jpeg_size;
      JPG_DEC *dec;
      res = RES_Load_Content(GUI_UDISK_BACKGROUNG_PIC, (char**)&jpeg_buf, &jpeg_size);
//      res = FS_Load_Content(GUI_UDISK_BACKGROUNG_PIC, (char**)&jpeg_buf, &jpeg_size);
      hdc_bk = CreateMemoryDC(SURF_SCREEN, GUI_XSIZE, GUI_YSIZE);
      if(res)
      {
       /* 根据图片数据创建JPG_DEC句柄 */
       dec = JPG_Open(jpeg_buf, jpeg_size);

       /* 绘制至内存对象 */
       JPG_Draw(hdc_bk, 0, 0, dec);

       /* 关闭JPG_DEC句柄 */
       JPG_Close(dec);
      }
      /* 释放图片内容空间 */
      RES_Release_Content((char **)&jpeg_buf);
      
      u8 *pic_buf;
      u32 pic_size;
      PNG_DEC *png_dec;
      BITMAP png_bm;
      

      /* 创建 HDC */
      hdc_btn = CreateMemoryDC((SURF_FORMAT)COLOR_FORMAT_ARGB8888, 71, 30);
      ClrDisplay(hdc_btn, NULL, 0);
      res = RES_Load_Content(GUI_UDISK_BTN_PIC, (char**)&pic_buf, &pic_size);
//            res = FS_Load_Content(GUI_UDISK_BTN_PIC, (char**)&pic_buf, &pic_size);
      if(res)
      {
        png_dec = PNG_Open(pic_buf, pic_size);
        PNG_GetBitmap(png_dec, &png_bm);
        DrawBitmap(hdc_btn, 0, 0, &png_bm, NULL);
        PNG_Close(png_dec);
      }
      /* 释放图片内容空间 */
      RES_Release_Content((char **)&pic_buf);
      
      /* 创建 HDC */
      hdc_btn_press = CreateMemoryDC((SURF_FORMAT)COLOR_FORMAT_ARGB8888, 71, 30);
      ClrDisplay(hdc_btn_press, NULL, 0);
      res = RES_Load_Content(GUI_UDISK_BTN_PRESS_PIC, (char**)&pic_buf, &pic_size);
//      res = FS_Load_Content(GUI_UDISK_BTN_PRESS_PIC, (char**)&pic_buf, &pic_size);
      if(res)
      {
        png_dec = PNG_Open(pic_buf, pic_size);
        PNG_GetBitmap(png_dec, &png_bm);
        DrawBitmap(hdc_btn_press, 0, 0, &png_bm, NULL);
        PNG_Close(png_dec);
      }
      /* 释放图片内容空间 */
      RES_Release_Content((char **)&pic_buf);


      break;
    } 
    case WM_TIMER:
    {
      int tmr_id;

      tmr_id = wParam;    // 定时器 ID

      if (tmr_id == 10)    
      {
        /*初始化*/
        Set_System();
        /*设置USB时钟为48M*/
        Set_USBClock();
        /*配置USB中断(包括SDIO中断)*/
        USB_Interrupts_Config();
        /*USB初始化*/
        USB_Init();
//        while (bDeviceState != CONFIGURED)
//        {}	 //等待配置完成
      }
      
      break;
    }

    case WM_ERASEBKGND:
    {
      HDC hdc = (HDC)wParam;
      RECT rc =*(RECT*)lParam;

      // GetClientRect(hwnd, &rc);
      // SetBrushColor(hdc, MapRGB(hdc, 0, 0, 0));
      // FillRect(hdc, &rc);
      
      BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_bk, rc.x, rc.y, SRCCOPY);    // 使用与时钟APP相同的背景

      return TRUE;
    }

    case WM_PAINT:
    {
      HDC hdc;
      PAINTSTRUCT ps;
      RECT rc  = {0, 45, GUI_XSIZE, 150};
      RECT rc1 = {50, 0, 220, 40};

      hdc = BeginPaint(hwnd, &ps);
      
      SetFont(hdc, defaultFont); 
      SetTextColor(hdc, MapRGB(hdc, 250, 250, 250));
      DrawText(hdc, L"外部FLASH模拟U盘", -1, &rc1, DT_VCENTER|DT_CENTER);//绘制文字(居中对齐方式)
      SetTextInterval(hdc, -1, 16);
      DrawText(hdc, L"本应用使用外部FLASH的后10M模拟U盘\r\n请在点击连接前使用Micro USB\r\n数据线连接开发板的J24到电脑！", -1, &rc, DT_VCENTER|DT_CENTER);//绘制文字(居中对齐方式)
   
      EndPaint(hwnd, &ps);

      break;
    }

    case WM_DRAWITEM:
    {
       DRAWITEM_HDR *ds;
       ds = (DRAWITEM_HDR*)lParam;
       switch(ds->ID)
       {
          case eID_SUD_EXIT:
          {
            _ExitButton_OwnerDraw(ds);
            return TRUE;              
          }  

          case eID_SUD_LINK:
          {
            btn_owner_draw(ds);
            return TRUE;             
          }  
       }

       break;
    }
    case WM_NOTIFY:
    {
      u16 code, id;
      id  =LOWORD(wParam);//获取消息的ID码
      code=HIWORD(wParam);//获取消息的类型    

      switch(id)
       {
        /* 退出按钮按下 */
          case eID_SUD_EXIT:
          {
               switch(code)
                {
                    case BN_CLICKED:
                    {
                        PostCloseMessage(hwnd);    // 发送关闭窗口的消息
                    }  
                    break;
                }
          }
          break;

          /* 连接按钮按下 */
          case eID_SUD_LINK:
          {
               switch(code)
                {
                    case BN_CLICKED:
                    {
                      SetTimer(hwnd, 10, 10, TMR_START | TMR_SINGLE, NULL);

                      SetWindowText(GetDlgItem(hwnd, eID_SUD_LINK), L"已连接");
                      EnableWindow(GetDlgItem(hwnd, eID_SUD_LINK), FALSE);
                    }  
                    break;
                }
          }
          break;
       }
      
      break;
    } 

    case WM_DESTROY:
    { 
      DeleteDC(hdc_bk);
      DeleteDC(hdc_btn);
      DeleteDC(hdc_btn_press);
      
      USB_Cable_Config(DISABLE);
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, DISABLE);

      return PostQuitMessage(hwnd);	
    } 

    default:
      return	DefWindowProc(hwnd, msg, wParam, lParam);   
  }
  
  return WM_NULL;
  
}

void GUI_SimulateUDisk_Dialog(void)
{
	HWND SUD_Main_Handle;
	WNDCLASS	wcex;
	MSG msg;
  
	wcex.Tag = WNDCLASS_TAG;
	wcex.Style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = win_proc; //设置主窗口消息处理的回调函数.
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = NULL;//hInst;
	wcex.hIcon = NULL;//LoadIcon(hInstance, (LPCTSTR)IDI_WIN32_APP_TEST);
	wcex.hCursor = NULL;//LoadCursor(NULL, IDC_ARROW);
   
	//创建主窗口
	SUD_Main_Handle = CreateWindowEx(WS_EX_NOFOCUS|WS_EX_FRAMEBUFFER,
                              &wcex,
                              L"GUI Simulate U Disk Dialog",
                              WS_VISIBLE|WS_CLIPCHILDREN,
                              0, 0, GUI_XSIZE, GUI_YSIZE,
                              NULL, NULL, NULL, NULL);

   //显示主窗口
	ShowWindow(SUD_Main_Handle, SW_SHOW);

	//开始窗口消息循环(窗口关闭并销毁时,GetMessage将返回FALSE,退出本消息循环)。
	while (GetMessage(&msg, SUD_Main_Handle))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}


