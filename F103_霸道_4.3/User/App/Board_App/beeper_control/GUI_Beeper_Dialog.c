#include <emXGUI.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "ff.h"
#include "x_libc.h"
#include "GUI_AppDef.h"
#include "emXGUI_JPEG.h"
#include "emxgui_png.h"

#define GUI_BEEPER_BACKGROUNG_PIC   "0:/beeper_desktop.jpg"
#define GUI_BEEPER_HORN_PIC         "0:/beeper_horn.png"
#define GUI_BEEPER_HORN1_PIC        "0:/beeper_horn1.png"
#define GUI_BEEPER_HORN2_PIC        "0:/beeper_horn2.png"
#define GUI_BEEPER_HORN3_PIC        "0:/beeper_horn3.png"
#define GUI_BEEPER_BUTTON_PIC       "0:/beeper_button.png"

typedef enum
{
  hdc_beeper_horn = 0,
	hdc_beeper_horn1,
	hdc_beeper_horn2,
	hdc_beeper_horn3,
	hdc_beeper_button,
  
	hdc_beeper_end,
}hdc_beep_png_t;

typedef struct{
	char *pic_name;      // 图片名
	int w;               // 图片宽
	int h;               // 图片高
	hdc_beep_png_t id;   // hdc 编号
}beeper_hdc_t;

static HDC hdc_beeper_bk;
static HDC hdc_beeper_png[hdc_beeper_end];

const beeper_hdc_t beeper_png_info[hdc_beeper_end] = 
{
  {GUI_BEEPER_HORN_PIC,     59, 101,   hdc_beeper_horn},
  {GUI_BEEPER_HORN1_PIC,    13,  40,   hdc_beeper_horn1},
  {GUI_BEEPER_HORN2_PIC,    21,  79,   hdc_beeper_horn2},
  {GUI_BEEPER_HORN3_PIC,    29, 116,   hdc_beeper_horn3},
  {GUI_BEEPER_BUTTON_PIC,   30,  30,   hdc_beeper_button},
};

/* 按钮 ID */
enum 
{
  ID_BEEPER_TICK = 0x1000,    // 蜂鸣器滴答按钮
  ID_BEEPER_SW,               // 蜂鸣器开关长鸣按钮
  ID_BEEPER_PRESS,            // 蜂鸣器按住响按钮
  ID_BEEPER_EXIT,             // 蜂鸣器退出按钮
};

static uint8_t sw_flag    = 0;
static uint8_t tick_flag  = 0;
static uint8_t press_flag = 3;

//退出按钮重绘制
static void ExitButton_OwnerDraw(DRAWITEM_HDR *ds)
{
  HDC hdc;
  RECT rc;

  hdc = ds->hDC;   
  rc = ds->rc; 

  if (ds->State & BST_PUSHED)
  { //按钮是按下状态
    SetPenColor(hdc, MapRGB(hdc, 1, 191, 255));      //设置文字色
  }
  else
  { //按钮是弹起状态
    SetPenColor(hdc, MapRGB(hdc, 250, 250, 250));
  }

  for(int i=0; i<4; i++)
  {
    HLine(hdc, rc.x, rc.y, rc.w);
    rc.y += 5;
  }
}

/*
 * @brief  选择按钮
 * @param  ds:	自定义绘制结构体
 * @retval NONE
*/
static void sw_button_OwnerDraw(DRAWITEM_HDR *ds)
{
  HWND hwnd;
	HDC hdc;
  RECT rc, rc_tmp;
	WCHAR wbuf[128];

  hwnd = ds->hwnd; //button的窗口句柄.
	hdc = ds->hDC;   //button的绘图上下文句柄.
  rc = ds->rc;

	/* 背景 */
  GetClientRect(hwnd, &rc_tmp);//得到控件的位置
  GetClientRect(hwnd, &rc);//得到控件的位置
  WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//坐标转换

  BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_beeper_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);

  GetWindowText(hwnd, wbuf, 128); //获得按钮控件的文字
  rc.h = 83;
  if (ds->ID == ID_BEEPER_SW)
  {
    if (sw_flag == 0)
    {
      BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_beeper_png[hdc_beeper_button], 0, 0, SRCCOPY);
      InflateRect(&rc, 0, -6);
      DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_CENTER);    // 绘制文字(居中对齐方式)
    }
    else 
    {
      rc.y = ds->rc.h - rc.h;
      BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_beeper_png[hdc_beeper_button], 0, 0, SRCCOPY);
      InflateRect(&rc, 0, -6);
      DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_CENTER);    // 绘制文字(居中对齐方式)
    }
  }
  else if (ds->ID == ID_BEEPER_TICK)
  {
    if (tick_flag == 0)
    {
      BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_beeper_png[hdc_beeper_button], 0, 0, SRCCOPY);
      InflateRect(&rc, 0, -6);
      DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_CENTER);    // 绘制文字(居中对齐方式)
    }
    else 
    {
      rc.y = ds->rc.h - rc.h;
      BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_beeper_png[hdc_beeper_button], 0, 0, SRCCOPY);
      InflateRect(&rc, 0, -6);
      DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_CENTER);    // 绘制文字(居中对齐方式)
    }
  }
}

/*
 * @brief  中间按住按钮
 * @param  ds:	自定义绘制结构体
 * @retval NONE
*/
static void press_button_OwnerDraw(DRAWITEM_HDR *ds)
{
  HWND hwnd;
	HDC hdc;
  RECT rc, rc_tmp;

  hwnd = ds->hwnd; //button的窗口句柄.
	hdc = ds->hDC;   //button的绘图上下文句柄.
  rc = ds->rc;

	/* 背景 */
  GetClientRect(hwnd, &rc_tmp);//得到控件的位置
  GetClientRect(hwnd, &rc);//得到控件的位置
  WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//坐标转换

  BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_beeper_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);

  BitBlt(hdc, 54, 77, 103, 190, hdc_beeper_png[hdc_beeper_horn], 0, 0, SRCCOPY);
  
  switch (press_flag)
  {
    case 3:
      BitBlt(hdc, 200, 62, 74, 220, hdc_beeper_png[hdc_beeper_horn3], 0, 0, SRCCOPY);
    
    case 2:
      BitBlt(hdc, 187, 98, 53, 148, hdc_beeper_png[hdc_beeper_horn2], 0, 0, SRCCOPY);

    case 1:
      BitBlt(hdc, 174, 135, 33, 74, hdc_beeper_png[hdc_beeper_horn1], 0, 0, SRCCOPY);
  
  default:
    break;
  }
}

static LRESULT win_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  
  static uint8_t Load_Flag = 1;
  switch(msg)
  {
    case WM_CREATE:
    {
      RECT rc;
      GetClientRect(hwnd, &rc); 
            
      CreateWindow(BUTTON, L"O", WS_TRANSPARENT|BS_FLAT | BS_NOTIFY | WS_OWNERDRAW | WS_VISIBLE,
                  286, 2, 23, 23, hwnd, ID_BEEPER_EXIT, NULL, NULL); 

      CreateWindow(BUTTON, L"滴答", WS_TRANSPARENT|BS_FLAT | BS_NOTIFY | WS_OWNERDRAW | WS_VISIBLE,
                  15, 95, 32, 77, hwnd, ID_BEEPER_TICK, NULL, NULL); 

      CreateWindow(BUTTON, L"按住叫", WS_TRANSPARENT|BS_FLAT | BS_NOTIFY | WS_OWNERDRAW | WS_VISIBLE,
                  69, 42, 182, 182, hwnd, ID_BEEPER_PRESS, NULL, NULL); 

      CreateWindow(BUTTON, L"长鸣", WS_TRANSPARENT|BS_FLAT | BS_NOTIFY | WS_OWNERDRAW | WS_VISIBLE,
                  269, 95, 32, 77, hwnd, ID_BEEPER_SW, NULL, NULL); 
      
      u8 *jpeg_buf;
      u32 jpeg_size;
      JPG_DEC *dec;
      BOOL res = NULL;

//      res = RES_Load_Content(GUI_BEEPER_BACKGROUNG_PIC, (char**)&jpeg_buf, &jpeg_size);
      res = FS_Load_Content(GUI_BEEPER_BACKGROUNG_PIC, (char**)&jpeg_buf, &jpeg_size);
      hdc_beeper_bk = CreateMemoryDC(SURF_SCREEN, GUI_XSIZE, GUI_YSIZE);
      if(res)
      {
        /* 根据图片数据创建JPG_DEC句柄 */
        dec = JPG_Open(jpeg_buf, jpeg_size);

        /* 绘制至内存对象 */
        JPG_Draw(hdc_beeper_bk, 0, 0, dec);

        /* 关闭JPG_DEC句柄 */
        JPG_Close(dec);
      }
      /* 释放图片内容空间 */
      RES_Release_Content((char **)&jpeg_buf);

      u8 *pic_buf;
      u32 pic_size;
      PNG_DEC *png_dec;
      BITMAP png_bm;
      
      for (uint8_t xC=0; xC<hdc_beeper_end; xC++)
      {
        /* 创建 HDC */
        hdc_beeper_png[beeper_png_info[xC].id] = CreateMemoryDC((SURF_FORMAT)COLOR_FORMAT_ARGB8888, beeper_png_info[xC].w, beeper_png_info[xC].h);
        ClrDisplay(hdc_beeper_png[beeper_png_info[xC].id], NULL, 0);
//          res = RES_Load_Content(clock_png_info[xC].pic_name, (char**)&pic_buf, &pic_size);
        res = FS_Load_Content(beeper_png_info[xC].pic_name, (char**)&pic_buf, &pic_size);
        if(res)
        {
          png_dec = PNG_Open(pic_buf, pic_size);
          PNG_GetBitmap(png_dec, &png_bm);
          DrawBitmap(hdc_beeper_png[beeper_png_info[xC].id], 0, 0, &png_bm, NULL);
          PNG_Close(png_dec);
        }
        /* 释放图片内容空间 */
        RES_Release_Content((char **)&pic_buf);
      }
      
//      SetTimer(hwnd, 2, 10, TMR_START | TMR_SINGLE, NULL);
      SetTimer(hwnd, 5, 10, TMR_START | TMR_SINGLE, NULL);
      SetTimer(hwnd, 6, 1000, TMR_START | TMR_SINGLE, NULL);
      break;
    } 
    case WM_TIMER:
    {
      static uint8_t beep_flag = 0;

      uint16_t timer_id;
      timer_id = wParam;
      if(timer_id == 5)
      {
        BEEP_ON;
      }
      else if(timer_id == 6)
      {
        BEEP_OFF;
        KillTimer(hwnd,55);
        KillTimer(hwnd,66);
      }
      else if (timer_id == 1)
      {
        beep_flag = !beep_flag;

        if (beep_flag)
        {
          BEEP_ON;
        }
        else
        {
          BEEP_OFF;
        }

        press_flag++;
        press_flag %= 4;

        RedrawWindow(GetDlgItem(hwnd, ID_BEEPER_PRESS), NULL, TRUE);
			} 
      else if (timer_id == 2)
      {
        u8 *jpeg_buf;
        u32 jpeg_size;
        JPG_DEC *dec;
        BOOL res = NULL;

        res = RES_Load_Content(GUI_BEEPER_BACKGROUNG_PIC, (char**)&jpeg_buf, &jpeg_size);
//        res = FS_Load_Content(GUI_BEEPER_BACKGROUNG_PIC, (char**)&jpeg_buf, &jpeg_size);
        hdc_beeper_bk = CreateMemoryDC(SURF_SCREEN, GUI_XSIZE, GUI_YSIZE);
        if(res)
        {
          /* 根据图片数据创建JPG_DEC句柄 */
          dec = JPG_Open(jpeg_buf, jpeg_size);

          /* 绘制至内存对象 */
          JPG_Draw(hdc_beeper_bk, 0, 0, dec);

          /* 关闭JPG_DEC句柄 */
          JPG_Close(dec);
        }
        /* 释放图片内容空间 */
        RES_Release_Content((char **)&jpeg_buf);

        u8 *pic_buf;
        u32 pic_size;
        PNG_DEC *png_dec;
        BITMAP png_bm;
        
        for (uint8_t xC=0; xC<hdc_beeper_end; xC++)
        {
          /* 创建 HDC */
          hdc_beeper_png[beeper_png_info[xC].id] = CreateMemoryDC((SURF_FORMAT)COLOR_FORMAT_ARGB8888, beeper_png_info[xC].w, beeper_png_info[xC].h);
          ClrDisplay(hdc_beeper_png[beeper_png_info[xC].id], NULL, 0);
//          res = RES_Load_Content(clock_png_info[xC].pic_name, (char**)&pic_buf, &pic_size);
          res = FS_Load_Content(beeper_png_info[xC].pic_name, (char**)&pic_buf, &pic_size);
          if(res)
          {
            png_dec = PNG_Open(pic_buf, pic_size);
            PNG_GetBitmap(png_dec, &png_bm);
            DrawBitmap(hdc_beeper_png[beeper_png_info[xC].id], 0, 0, &png_bm, NULL);
            PNG_Close(png_dec);
          }
          /* 释放图片内容空间 */
          RES_Release_Content((char **)&pic_buf);
        }

        Load_Flag = 1;    // 标志资源加载完成
        for (uint32_t xC=ID_BEEPER_TICK; xC<=ID_BEEPER_EXIT; xC++)
        {
          ShowWindow(GetDlgItem(hwnd, xC), SW_SHOW);    // 资源加载完成，显示主页的全部控件
        }
        InvalidateRect(hwnd, NULL, TRUE);    // 重绘窗口
      }
        
    }break;

    case WM_PAINT:
    {
      HDC hdc;
      PAINTSTRUCT ps;
      hdc = BeginPaint(hwnd, &ps);
      
      RECT rc2 = {0, 0, GUI_XSIZE, GUI_YSIZE};
        
      if (Load_Flag)     // 资源加载完成
      {
        BitBlt(hdc, 0, 0, GUI_XSIZE, GUI_YSIZE, hdc_beeper_bk, 0, 0, SRCCOPY);
      }
      else
      {    /* 资源加载未完成 */ 
        SetBrushColor(hdc, MapRGB(hdc, 10, 10, 10));
        FillRect(hdc, &rc2);
        SetTextColor(hdc, MapRGB(hdc, 250, 250, 250));
        DrawText(hdc, L"资源加载中，请稍等...", -1, &rc2, DT_VCENTER|DT_CENTER);//绘制文字(居中对齐方式)
      }

      EndPaint(hwnd, &ps);
      break;
    }

    case WM_DRAWITEM:
    {
       DRAWITEM_HDR *ds;
       ds = (DRAWITEM_HDR*)lParam;
       switch(ds->ID)
       {
          case ID_BEEPER_EXIT:
          {
            ExitButton_OwnerDraw(ds);
            return TRUE;             
          }  

          case ID_BEEPER_SW:
          case ID_BEEPER_TICK:
          {
            sw_button_OwnerDraw(ds);
            return TRUE;   
          }

          case ID_BEEPER_PRESS:
          {
            press_button_OwnerDraw(ds);
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
      if(code == BN_CLICKED)
      {
        if (id == ID_BEEPER_EXIT)
        {
          PostCloseMessage(hwnd);
        }
        else if (id == ID_BEEPER_TICK)
        {
          tick_flag = !tick_flag;    // 反正标志

          if (tick_flag)
          {
            SetTimer(hwnd, 1, 500, TMR_START, NULL);    // 开启滴答定时器
          }
          else
          {
            KillTimer(hwnd, 1);    // 关闭滴答定时器
            BEEP_OFF;              // 关闭蜂鸣器
            press_flag =3;
          }
          
          sw_flag = 0;    // 复位标志
          RedrawWindow(GetDlgItem(hwnd, ID_BEEPER_SW), NULL, TRUE);
          RedrawWindow(GetDlgItem(hwnd, ID_BEEPER_PRESS), NULL, TRUE);
        }
        else if (id == ID_BEEPER_SW)
        {
          sw_flag = !sw_flag;    // 反正标志

          if (sw_flag)
          {
            BEEP_ON;    // 开蜂鸣器
          }
          else
          { 
            BEEP_OFF;    // 关蜂鸣器 
          }

          KillTimer(hwnd, 1);
          tick_flag = 0;
          press_flag = 3;

          RedrawWindow(GetDlgItem(hwnd, ID_BEEPER_TICK), NULL, TRUE);
          RedrawWindow(GetDlgItem(hwnd, ID_BEEPER_PRESS), NULL, TRUE);
        }
        else if (id == ID_BEEPER_PRESS)
        {
          KillTimer(hwnd, 1);
          BEEP_OFF;    // 关蜂鸣器 
          press_flag = 3;
          RedrawWindow(GetDlgItem(hwnd, ID_BEEPER_PRESS), NULL, TRUE);
        }
      }
      
      if(code == BN_PUSHED && id == ID_BEEPER_PRESS)    // 按下
      {
        tick_flag = 0;
        sw_flag = 0;

        RedrawWindow(GetDlgItem(hwnd, ID_BEEPER_TICK), NULL, TRUE);
        RedrawWindow(GetDlgItem(hwnd, ID_BEEPER_SW), NULL, TRUE);
        SetTimer(hwnd, 1, 500, TMR_START, NULL);
      }

      break;
    } 

    case WM_DESTROY:
    {
      tick_flag = 0;
      sw_flag = 0;
      press_flag = 3;
      
      BEEP_OFF;
      
      return PostQuitMessage(hwnd);	
    } 

    default:
      return	DefWindowProc(hwnd, msg, wParam, lParam);   
  }
  
  return WM_NULL;
  
}

void GUI_Beeper_Dialog(void)
{
	
	WNDCLASS	wcex;
	MSG msg;
  HWND MAIN_Handle;
	wcex.Tag = WNDCLASS_TAG;

	wcex.Style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = win_proc; //设置主窗口消息处理的回调函数.
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = NULL;//hInst;
	wcex.hIcon = NULL;//LoadIcon(hInstance, (LPCTSTR)IDI_WIN32_APP_TEST);
	wcex.hCursor = NULL;//LoadCursor(NULL, IDC_ARROW);
   
	//创建主窗口
	MAIN_Handle = CreateWindowEx(WS_EX_NOFOCUS|WS_EX_FRAMEBUFFER,
                              &wcex,
                              L"GUI_Beeper_Dialog",
                              WS_VISIBLE|WS_CLIPCHILDREN,
                              0, 0, GUI_XSIZE, GUI_YSIZE,
                              NULL, NULL, NULL, NULL);
   //显示主窗口
	ShowWindow(MAIN_Handle, SW_SHOW);
	//开始窗口消息循环(窗口关闭并销毁时,GetMessage将返回FALSE,退出本消息循环)。
	while (GetMessage(&msg, MAIN_Handle))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}  
}


