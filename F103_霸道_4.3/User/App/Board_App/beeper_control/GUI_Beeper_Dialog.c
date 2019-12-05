#include <emXGUI.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "ff.h"
#include "x_libc.h"
#include "GUI_AppDef.h"
#include "./beep/bsp_beep.h"   

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

  SetPenSize(hdc, 2);

  InflateRect(&rc, 0, -1);
  
  for(int i=0; i<4; i++)
  {
    HLine(hdc, rc.x, rc.y, rc.w);
    rc.y += 9;
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
  RECT rc;
	WCHAR wbuf[128];

  hwnd = ds->hwnd; //button的窗口句柄.
	hdc = ds->hDC;   //button的绘图上下文句柄.
  rc = ds->rc;

	/* 背景 */
  SetBrushColor(hdc, MapRGB(hdc, 240, 240, 240));
  FillRect(hdc, &rc);
  
  EnableAntiAlias(hdc, TRUE);
  
  SetBrushColor(hdc, MapRGB(hdc, 50, 240, 240));
  FillRoundRect(hdc, &rc, MIN(rc.w, rc.h)/2);

  GetWindowText(hwnd, wbuf, 128); //获得按钮控件的文字
  
  if (ds->ID == ID_BEEPER_SW)
  {
    if (sw_flag == 0)
    {
      SetBrushColor(hdc, MapRGB(hdc, 230, 230, 230));
      InflateRect(&rc, -5, -5);
      FillRoundRect(hdc, &rc, MIN(rc.w, rc.h)/2);
      
      SetBrushColor(hdc, MapRGB(hdc, 255, 255, 255));
      FillCircle(hdc, rc.w/2 + 5, rc.w/2 + 5, MIN(rc.w, rc.h)/2);

      rc.h = rc.w;
      DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_CENTER);    // 绘制文字(居中对齐方式)
    }
    else 
    {
      SetBrushColor(hdc, MapRGB(hdc, 50, 50, 240));
      InflateRect(&rc, -5, -5);
      FillRoundRect(hdc, &rc, MIN(rc.w, rc.h)/2);
      
      SetBrushColor(hdc, MapRGB(hdc, 240, 240, 240));
      FillCircle(hdc, rc.w/2 + 5, rc.h - rc.w/2 + 5, MIN(rc.w, rc.h)/2);
      
      rc.y = rc.h - rc.w;
      rc.h = rc.w;
      DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_CENTER);    // 绘制文字(居中对齐方式)
    }
  }
  else if (ds->ID == ID_BEEPER_TICK)
  {
    if (tick_flag == 0)
    {
      SetBrushColor(hdc, MapRGB(hdc, 230, 230, 230));
      InflateRect(&rc, -5, -5);
      FillRoundRect(hdc, &rc, MIN(rc.w, rc.h)/2);
      
      SetBrushColor(hdc, MapRGB(hdc, 255, 255, 255));
      FillCircle(hdc, rc.w/2 + 5, rc.w/2 + 5, MIN(rc.w, rc.h)/2);
      
      rc.h = rc.w;
      DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_CENTER);    // 绘制文字(居中对齐方式)
    }
    else 
    {
      SetBrushColor(hdc, MapRGB(hdc, 50, 50, 240));
      InflateRect(&rc, -5, -5);
      FillRoundRect(hdc, &rc, MIN(rc.w, rc.h)/2);
      
      SetBrushColor(hdc, MapRGB(hdc, 240, 240, 240));
      FillCircle(hdc, rc.w/2 + 5, rc.h - rc.w/2 + 5, MIN(rc.w, rc.h)/2);
      
      rc.y = rc.h - rc.w;
      rc.h = rc.w;
      DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_CENTER);    // 绘制文字(居中对齐方式)
    }
  }
  
  EnableAntiAlias(hdc, FALSE);
}

/*
 * @brief  中间按住按钮
 * @param  ds:	自定义绘制结构体
 * @retval NONE
*/
static void press_button_OwnerDraw(DRAWITEM_HDR *ds)
{
	HDC hdc;
  RECT rc;

	hdc = ds->hDC;   //button的绘图上下文句柄.
  rc = ds->rc;

	/* 背景 */
  SetBrushColor(hdc, MapRGB(hdc, 240, 240, 240));
  FillRect(hdc, &rc);
  
  EnableAntiAlias(hdc, TRUE);
  
  SetBrushColor(hdc, MapRGB(hdc, 50, 240, 240));
  FillCircle(hdc, rc.w/2, rc.h/2, MIN(rc.w, rc.h)/2);
  
  SetBrushColor(hdc, MapRGB(hdc, 50, 50, 50));
  
  switch (press_flag)
  {
    case 3:
      FillArc(hdc, rc.w/2, rc.h/2, 100, 120, 0, 360);
    
    case 2:
      FillArc(hdc, rc.w/2, rc.h/2, 60, 80, 0, 360);

    case 1:
      FillArc(hdc, rc.w/2, rc.h/2, 20, 40, 0, 360);
  
  default:
    break;
  }
  
  EnableAntiAlias(hdc, FALSE);
}

static LRESULT win_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch(msg)
  {
    case WM_CREATE:
    {
      RECT rc;
      GetClientRect(hwnd, &rc); 
            
      CreateWindow(BUTTON, L"O", WS_TRANSPARENT|BS_FLAT | BS_NOTIFY | WS_OWNERDRAW | WS_VISIBLE,
                  740, 10, 36, 36, hwnd, ID_BEEPER_EXIT, NULL, NULL); 

      CreateWindow(BUTTON, L"滴答", WS_TRANSPARENT|BS_FLAT | BS_NOTIFY | WS_OWNERDRAW | WS_VISIBLE,
                  71, 177, 84, 166, hwnd, ID_BEEPER_TICK, NULL, NULL); 

      CreateWindow(BUTTON, L"按住叫", WS_TRANSPARENT|BS_FLAT | BS_NOTIFY | WS_OWNERDRAW | WS_VISIBLE,
                  228, 82, 344, 344, hwnd, ID_BEEPER_PRESS, NULL, NULL); 

      CreateWindow(BUTTON, L"长鸣", WS_TRANSPARENT|BS_FLAT | BS_NOTIFY | WS_OWNERDRAW | WS_VISIBLE,
                  650, 177, 84, 166, hwnd, ID_BEEPER_SW, NULL, NULL); 
      
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
        
    }break;

    case WM_PAINT:
    {
      HDC hdc;
      PAINTSTRUCT ps;
      hdc = BeginPaint(hwnd, &ps);
      
      RECT rc = {0, 0, GUI_XSIZE, GUI_YSIZE};
      RECT rc_title_grad = {0, 50, GUI_XSIZE, 5};
      
      SetBrushColor(hdc, MapRGB(hdc, 240, 240, 240));
      FillRect(hdc, &rc);

      rc.h = 50;
      SetBrushColor(hdc, MapRGB(hdc, 1, 218, 254));
      FillRect(hdc, &rc);//, MapRGB(hdc, 1, 218, 254), MapRGB(hdc, 1, 168, 255), FALSE);
      
      GradientFillRect(hdc, &rc_title_grad, MapRGB(hdc, 150, 150, 150), MapRGB(hdc, 240, 240, 240), TRUE);
      
      SetTextColor(hdc, MapRGB(hdc, 50, 50, 50));
      DrawText(hdc, L"蜂鸣器控制", -1, &rc, DT_VCENTER|DT_CENTER);

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


