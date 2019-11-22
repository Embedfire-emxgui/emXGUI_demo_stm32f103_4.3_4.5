#include "emXGUI.h"
#include "GUI_MUSICPLAYER_DIALOG.h"
#include "x_libc.h"
#include <stdlib.h>
#include "string.h"
#include "ff.h"
#include "./mp3_player/Backend_mp3Player.h"
#include "GUI_AppDef.h"
#include "emXGUI_JPEG.h"
/******************按钮控件ID值***********************/
#define ID_BUTTON_Power      0x1000   //音量 
#define ID_BUTTON_List       0x1001   //音乐List
#define ID_BUTTON_Equa       0x1002   //均衡器
#define ID_BUTTON_Folder     0x1003   //文件夹
#define ID_BUTTON_BACK       0x1004   //上一首
#define ID_BUTTON_START      0x1005   //暂停键
#define ID_BUTTON_NEXT       0x1006   //下一首
#define ID_BUTTON_MINISTOP   0x1007   //迷你版暂停键
#define ID_BUTTON_Horn       0x1008   //喇叭
/*****************滑动条控件ID值*********************/
#define ID_SCROLLBAR_POWER   0x1104   //音量条
#define ID_SCROLLBAR_TIMER   0x1105   //进度条
/*****************文本框控件ID值*********************/
//本例程显示五行歌词
#define ID_TEXTBOX_LRC1      0x1201   //歌词第一行
#define ID_TEXTBOX_LRC2      0x1202   //歌词第二行
#define ID_TEXTBOX_LRC3      0x1203   //歌词第三行（当前行）
#define ID_TEXTBOX_LRC4      0x1204   //歌词第四行
#define ID_TEXTBOX_LRC5      0x1205   //歌词第五行

#define ID_EXIT        0x3000

/* 外部资源名 */
#define ROTATE_DISK_NAME "rotate_disk_ARGB8888.bmp"


//图标管理数组
icon_S music_icon[8] = {
   {"yinliang",         {10,200,32,32},       FALSE},//音量0
   {"yinyueliebiao",    {285,209,24,24},      FALSE},//音乐列表1
   {"geci",             {258,209,24,24},      FALSE},//歌词栏2
   {"laba",             {258,209,24,24},      FALSE},//喇叭3  231
   {"NULL",             {0,0,0,0},            FALSE},//无4
   {"shangyishou",      {112, 209, 24, 24},   FALSE},//上一首5
   {"zanting/bofang",   {144, 205, 34, 34},   FALSE},//播放6
   {"xiayishou",        {185, 209, 24, 24},   FALSE},//下一首7
  
};
static char path[50]="0:";//文件根目录
static int power = 220;//音量值
s32 old_scrollbar_value;//上一个音量值
TaskHandle_t h_music;//音乐播放进程
int enter_flag = 0;//切换标志位
int IsCreateList = 0;
int time2exit = 0;
static COLORREF color_bg;//透明控件的背景颜色
uint8_t chgsch=0; //调整进度条标志位
char music_name[FILE_NAME_LEN] __EXRAM;//歌曲名数组
//文件系统相关变量
FRESULT f_result; 
FIL f_file __EXRAM;
//FIL     f_file;
UINT    f_num;
//歌词数组--存放歌词数据
uint8_t ReadBuffer1[1024*5] __EXRAM;
//MINI播放键、上一首、下一首控件句柄句柄
//static HWND mini_next,mini_start,mini_back;
//歌词结构体
LYRIC lrc  __EXRAM;
static HDC hdc_bk;
static HWND wnd;//音量滑动条窗口句柄 
static HWND wnd_power;//音量icon句柄
extern const unsigned char gImage_0[];
GUI_SEM *exit_sem = NULL;
/*============================================================================*/

SCROLLINFO sif_power;
//HFONT Music_Player_hFont48=NULL;
//HFONT Music_Player_hFont64  =NULL;
//HFONT Music_Player_hFont72  =NULL;

/***********************外部声明*************************/
extern void	GUI_MusicList_DIALOG(void);

/******************读取歌词文件*************************/

static uint16_t getonelinelrc(uint8_t *buff,uint8_t *str,int16_t len)
{
	uint16_t i;
	for(i=0;i<len;i++)
	{
		*(str+i)=*(buff+i);
		if((*(buff+i)==0x0A)||(*(buff+i)==0x00))
		{
			*(buff+i)='\0';
			*(str+i)='\0';
			break;
		}
	}
	return (i+1);
}
/**
  * @brief  插入字符串
  * @param  name：  数据数组
  * @param  sfx：   带插入的数据字符串
  * @retval 无
  * @notes  本程序调用该函数为歌词文件插入.lrc后缀
  */
static void lrc_chg_suffix(uint8_t *name,const char *sfx)
{		    	     
	while(*name!='\0')name++;
	while(*name!='.')name--;
	*(++name)=sfx[0];
	*(++name)=sfx[1];
	*(++name)=sfx[2];
	*(++name)='\0';
}
/**
  * @brief  歌词文件排序
  * @param  lyric：  歌词结构体
  * @retval 无
  * @notes  无
  */
static void lrc_sequence(LYRIC	*lyric)
{
	uint16_t i=0,j=0;
	uint16_t temp=0;
	if (lyric->indexsize == 0)return;
	
	for(i = 0; i < lyric->indexsize - 1; i++)
	{
		for(j = i+1; j < lyric->indexsize; j++)
		{
			if(lyric->time_tbl[i] > lyric->time_tbl[j])
			{
				temp = lyric->time_tbl[i];
				lyric->time_tbl[i] = lyric->time_tbl[j];
				lyric->time_tbl[j] = temp;

				temp = lyric->addr_tbl[i];
				lyric->addr_tbl[i] = lyric->addr_tbl[j];
				lyric->addr_tbl[j] = temp;
			}
		}
	}	
}
/**
  * @brief  歌词文件解析
  * @param  lyric：  歌词结构体
  * @param  strbuf： 存放歌词的数组
  * @retval 无
  * @notes  
  */
static void lyric_analyze(LYRIC	*lyric,uint8_t *strbuf)
{
	uint8_t strtemp[MAX_LINE_LEN]={0};
	uint8_t *pos=NULL;
	uint8_t sta=0,strtemplen=0;
	uint16_t lrcoffset=0;
	uint16_t str_len=0,i=0;
	
	pos=strbuf;
	str_len=strlen((const char *)strbuf);
	if(str_len==0)return;
	i=str_len;
   //此处的while循环用于判断歌词文件的标准
	while(--i)
	{
		if(*pos=='[')
			sta=1;
		else if((*pos==']')&&(sta==1))
			sta=2;
	  else if((sta==2)&&(*pos!=' '))
		{
			sta=3;
			break;
		}
		pos++; 
	}
	if(sta!=3)return;	
	lrcoffset=0;
	lyric->indexsize=0;
	while(lrcoffset<=str_len)
	{
		i=getonelinelrc(strbuf+lrcoffset,strtemp,MAX_LINE_LEN);
		lrcoffset+=i;
//		printf("lrcoffset:%d,i:%d\n",lrcoffset,i);
		strtemplen=strlen((const char *)strtemp);
		pos=strtemp;
		while(*pos!='[')
			pos++;
		pos++;
      
		if((*pos<='9')&&(*pos>='0'))
		{
         //记录时间标签
			lyric->time_tbl[lyric->indexsize]=(((*pos-'0')*10+(*(pos + 1)-'0'))*60+((*(pos+3)-'0')*10+(*(pos+4)-'0')))*100+((*(pos+6)-'0')*10+(*(pos+7)-'0'));
			//记录歌词内容
         lyric->addr_tbl[lyric->indexsize]=(uint16_t)(lrcoffset-strtemplen+10); 
         //记录歌词长度
			lyric->length_tbl[lyric->indexsize]=strtemplen-10;
			lyric->indexsize++;
		}		
//		else
//				continue;		
	}
}
static void Music_Button_OwnerDraw(DRAWITEM_HDR *ds) //绘制一个按钮外观
{
	HWND hwnd;
	HDC hdc;
	RECT rc, rc_tmp;
	WCHAR wbuf[128];

	hwnd = ds->hwnd; //button的窗口句柄.
	hdc = ds->hDC;   //button的绘图上下文句柄.
   GetClientRect(hwnd, &rc_tmp);//得到控件的位置
   GetClientRect(hwnd, &rc);//得到控件的位置
   WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//坐标转换
	SetBrushColor(hdc, MapRGB(hdc, 0, 0, 0));
   
   FillRect(hdc, &rc);  
   BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);
  SetTextColor(hdc, MapRGB(hdc, 255, 255, 255));


	GetWindowText(hwnd, wbuf, 128); //获得按钮控件的文字

	DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_CENTER);//绘制文字(居中对齐方式)

}
static void exit_owner_draw(DRAWITEM_HDR *ds) //绘制一个按钮外观
{
  HDC hdc;
  RECT rc, rc_tmp;
  HWND hwnd;

	hdc = ds->hDC;   
	rc = ds->rc; 
  rc_tmp = rc;
  hwnd = ds->hwnd;

  WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//坐标转换
  BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);

  if (ds->State & BST_PUSHED)
	{ //按钮是按下状态
		SetPenColor(hdc, MapRGB(hdc, 120, 120, 120));      //设置文字色
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



/**
  * @brief  创建音乐列表进程
  * @param  无
  * @retval 无
  * @notes  
  */
static TaskHandle_t h1;

static void App_MusicList()
{
	static int thread=0;
	static int app=0;
   
	if(thread==0)
	{  
//      h1=rt_thread_create("App_MusicList",(void(*)(void*))App_MusicList,NULL,4*1024,5,1);
     xTaskCreate((TaskFunction_t )(void(*)(void*))App_MusicList,    /* 任务入口函数 */
                            (const char*    )"App_MusicList",       /* 任务名字 */
                            (uint16_t       )2*1024/4,              /* 任务栈大小FreeRTOS的任务栈以字为单位 */
                            (void*          )NULL,                  /* 任务入口函数参数 */
                            (UBaseType_t    )5,                     /* 任务的优先级 */
                            (TaskHandle_t  )&h1);                   /* 任务控制块指针 */
	
      thread =1;
      return;
	}

   vTaskSuspend(h_music);    // 进入列表挂起音乐播放

	while(thread) //线程已创建了
	{
    if(thread == 1)
    {
      if(app==0)
      {
        app=1;
        GUI_MusicList_DIALOG();
        app=0;
        thread=0;
      }
    }
    GUI_msleep(10);
	}

   vTaskResume(h_music);    // 退出选择列表恢复音乐播放
    
  GUI_Thread_Delete(GUI_GetCurThreadHandle()); 
}
/**
  * @brief  播放音乐列表进程
  * @param  hwnd：屏幕窗口的句柄
  * @retval 无
  * @notes  
  */

int stop_flag = 0;
static int thread=0;
static void App_PlayMusic(HWND hwnd)
{
//	BaseType_t xReturn = pdPASS;/* 定义一个创建信息返回值，默认为pdPASS */
	int app=0;
   HDC hdc = NULL;
//   SCROLLINFO sif;
	if(thread==0)
	{  
   xTaskCreate((TaskFunction_t )(void(*)(void*))App_PlayMusic,  /* 任务入口函数 */
                            (const char*    )"App_PlayMusic",   /* 任务名字 */
                            (uint16_t       )3*1024/4,          /* 任务栈大小FreeRTOS的任务栈以字为单位 */
                            (void*          )hwnd,              /* 任务入口函数参数 */
                            (UBaseType_t    )5,                 /* 任务的优先级 */
                            (TaskHandle_t  )&h_music);          /* 任务控制块指针 */
                                                
      thread =1;

      return;
	}
	while(thread) //线程已创建了
	{     
		if(app==0)
		{
			app=1;
//         hdc = GetDC(hwnd);   
         int i = 0;      
#if 1
         //读取歌词文件
         while(music_playlist[play_index][i]!='\0')
         {
           music_name[i]=music_playlist[play_index][i];
           i++;
         }			         
         music_name[i]='\0';
         //为歌词文件添加.lrc后缀
         lrc_chg_suffix((uint8_t *)music_name,"lrc");
         i=0;
         //初始化数组内容
         while(i<LYRIC_MAX_SIZE)
         {
           lrc.addr_tbl[i]=0;
           lrc.length_tbl[i]=0;
           lrc.time_tbl[i]=0;
           i++;
         }
         lrc.indexsize=0;
         lrc.oldtime=0;
         lrc.curtime=0;
         //打开歌词文件
         f_result=f_open(&f_file, music_name,FA_OPEN_EXISTING | FA_READ);
         //打开成功，读取歌词文件，分析歌词文件，同时将flag置1，表示文件读取成功
         if((f_result==FR_OK)&&(f_file.fsize<COMDATA_SIZE))
         {					
           f_result=f_read(&f_file,ReadBuffer1, sizeof(ReadBuffer1),&f_num);		
           if(f_result==FR_OK) 
           {  
              lyric_analyze(&lrc,ReadBuffer1);
              lrc_sequence(&lrc);
              lrc.flag = 1;      
           }
         }
         else    // 打开失败（未找到该歌词文件），则将flag清零，表示没有读取到该歌词文件
         {
            lrc.flag = 0;
            printf("读取歌词失败\n");
         }
         //关闭文件
			   f_close(&f_file);	 
#endif
         i = 0;
         //得到播放曲目的文件名
        while(music_playlist[play_index][i]!='\0')
        {
          music_name[i]=music_playlist[play_index][i];
          i++;
        }
        music_name[i]='\0';

        vs1053_player((uint8_t *)music_name, power, hdc);//

        printf("播放结束\n");
         
        app=0;
        //使用 GETDC之后需要释放掉HDC
//        ReleaseDC(hwnd, hdc);
        //进行任务调度
        GUI_msleep(300);
        
        if (time2exit == 1)
        {
           GUI_SemPost(exit_sem);
        }
 		}
   }
//  
//  GUI_Thread_Delete(GUI_GetCurThreadHandle()); 
}
/**
  * @brief  scan_files 递归扫描sd卡内的歌曲文件
  * @param  path:初始扫描路径
  * @retval result:文件系统的返回值
  */

static FRESULT scan_files (char* path) 
{ 
  FRESULT res; 		//部分在递归过程被修改的变量，不用全局变量	
  FILINFO fno; 
  DIR dir ;
  int i; 
  char *fn; 
  char file_name[FILE_NAME_LEN];	

#if _USE_LFN 
  static char lfn[_MAX_LFN * (_DF1S ? 2 : 1) + 1]; 	//长文件名支持
  fno.lfname = lfn; 
  fno.lfsize = sizeof(lfn); 
#endif  
  res = f_opendir(&dir, path); //打开目录
  if (res == FR_OK) 
  { 
    i = strlen(path); 
    for (;;) 
    { 
      res = f_readdir(&dir, &fno); 										//读取目录下的内容
     if (res != FR_OK || fno.fname[0] == 0)
     {
       f_closedir(&dir);
       break; 	//为空时表示所有项目读取完毕，跳出
     }
#if _USE_LFN 
      fn = *fno.lfname ? fno.lfname : fno.fname; 
#else 
      fn = fno.fname; 
#endif 
      if(strstr(path,"recorder")!=NULL)continue;       //逃过录音文件
      if (*fn == '.') continue; 											//点表示当前目录，跳过			
      if (fno.fattrib & AM_DIR) 
			{ 																							//目录，递归读取
        sprintf(&path[i], "/%s", fn); 							//合成完整目录名
        res = scan_files(path);											//递归遍历 
        if (res != FR_OK) 
        {
          f_closedir(&dir);
					break; 																	     	// 打开失败，跳出循环
        }																	     	// 打开失败，跳出循环
        path[i] = 0; 
      } 
      else 
		{ 
//				printf("%s%s\r\n", path, fn);								//输出文件名
				if(strstr(fn,".mp3")||strstr(fn,".MP3"))//判断是否mp3、wav或flac文件||strstr(fn,".flac")||strstr(fn,".FLAC")strstr(fn,".wav")||strstr(fn,".WAV")||
				{
					if ((strlen(path)+strlen(fn)<FILE_NAME_LEN)&&(music_file_num<MUSIC_MAX_NUM))
					{
						sprintf(file_name, "%s/%s", path, fn);						
						memcpy(music_playlist[music_file_num],file_name,strlen(file_name)+1);
                  printf("%s\r\n",music_playlist[music_file_num]);
						memcpy(music_lcdlist[music_file_num],fn,strlen(fn)+1);						
						music_file_num++;//记录文件个数
					}
				}//if mp3||wav
      }//else
     } //for
  } 
  return res; 
}
/***********************控件重绘函数********************************/
/**
  * @brief  button_owner_draw 按钮控件的重绘制
  * @param  ds:DRAWITEM_HDR结构体
  * @retval NULL
  */

static void button_owner_draw(DRAWITEM_HDR *ds)
{
   HDC hdc; //控件窗口HDC
   HDC hdc_mem;//内存HDC，作为缓冲区
   HWND hwnd; //控件句柄 
   RECT rc_cli, rc_tmp;//控件的位置大小矩形
   WCHAR wbuf[128];
	hwnd = ds->hwnd;
	hdc = ds->hDC; 
//   if(ds->ID ==  ID_BUTTON_START && show_lrc == 1)
//      return;
   //获取控件的位置大小信息
   GetClientRect(hwnd, &rc_cli);
   //创建缓冲层，格式为SURF_ARGB4444
   hdc_mem = CreateMemoryDC(SURF_SCREEN, rc_cli.w, rc_cli.h);
   
	GetWindowText(ds->hwnd,wbuf,128); //获得按钮控件的文字  
//   if(ds->ID == ID_BUTTON_Power || ds->ID == ID_BUTTON_MINISTOP){
//      SetBrushColor(hdc, color_bg);
//      FillRect(hdc, &rc_cli);
//   }
//   
//   SetBrushColor(hdc_mem,MapARGB(hdc_mem, 0, 255, 250, 250));
//   FillRect(hdc_mem, &rc_cli);

   GetClientRect(hwnd, &rc_tmp);//得到控件的位置
   GetClientRect(hwnd, &rc_cli);//得到控件的位置
   WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//坐标转换
   
   BitBlt(hdc_mem, rc_cli.x, rc_cli.y, rc_cli.w, rc_cli.h, hdc_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);

   //播放键使用100*100的字体
   if(ds->ID == ID_BUTTON_START)
      SetFont(hdc_mem, controlFont_32);
   else if(ds->ID == ID_BUTTON_NEXT || ds->ID == ID_BUTTON_BACK)
      SetFont(hdc_mem, controlFont_24);
   else
      SetFont(hdc_mem, controlFont_24);
   //设置按键的颜色
   SetTextColor(hdc_mem, MapARGB(hdc_mem, 250,250,250,250));
   //NEXT键、BACK键和LIST键按下时，改变颜色
	if((ds->State & BST_PUSHED) )
	{ //按钮是按下状态
		SetTextColor(hdc_mem, MapARGB(hdc_mem, 250,105,105,105));      //设置文字色     
	}
 
   DrawText(hdc_mem, wbuf,-1,&rc_cli,DT_VCENTER);//绘制文字(居中对齐方式)
   
   BitBlt(hdc, rc_cli.x, rc_cli.y, rc_cli.w, rc_cli.h, hdc_mem, 0, 0, SRCCOPY);
   
   DeleteDC(hdc_mem);  
}
//透明文本
static void _music_textbox_OwnerDraw(DRAWITEM_HDR *ds) //绘制一个按钮外观
{
	HWND hwnd;
	HDC hdc;
	RECT rc, rc_tmp;
	WCHAR wbuf[128];

	hwnd = ds->hwnd; //button的窗口句柄.
	hdc = ds->hDC;   //button的绘图上下文句柄.
  GetClientRect(hwnd, &rc_tmp);//得到控件的位置
  GetClientRect(hwnd, &rc);//得到控件的位置
  WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//坐标转换

  BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);
  SetTextColor(hdc, MapRGB(hdc, 255, 255, 255));


  GetWindowText(hwnd, wbuf, 128); //获得按钮控件的文字
  if(ds->ID == ID_TEXTBOX_LRC3)
    SetTextColor(hdc, MapRGB(hdc, 255, 0, 0));
  DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_CENTER);//绘制文字(居中对齐方式)
}

/*
 * @brief  绘制滚动条
 * @param  hwnd:   滚动条的句柄值
 * @param  hdc:    绘图上下文
 * @param  back_c：背景颜色
 * @param  Page_c: 滚动条Page处的颜色
 * @param  fore_c：滚动条滑块的颜色
 * @retval NONE
*/
static void draw_scrollbar(HWND hwnd, HDC hdc, COLOR_RGB32 back_c, COLOR_RGB32 Page_c, COLOR_RGB32 fore_c)
{
	 RECT rc,rc_tmp;
   RECT rc_scrollbar;

	/* 背景 */
   GetClientRect(hwnd, &rc_tmp);//得到控件的位置
   GetClientRect(hwnd, &rc);//得到控件的位置
   WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//坐标转换
   
   BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);

   rc_scrollbar.x = rc.x;
   rc_scrollbar.y = rc.h/2-1;
   rc_scrollbar.w = rc.w;
   rc_scrollbar.h = 2;
   
	SetBrushColor(hdc, MapRGB888(hdc, Page_c));
	FillRect(hdc, &rc_scrollbar);

	/* 滑块 */
	SendMessage(hwnd, SBM_GETTRACKRECT, 0, (LPARAM)&rc);

	SetBrushColor(hdc, MapRGB(hdc, 169, 169, 169));
	//rc.y += (rc.h >> 2) >> 1;
	//rc.h -= (rc.h >> 2);
	/* 边框 */
	//FillRoundRect(hdc, &rc, MIN(rc.w, rc.h) >> 2);
	FillCircle(hdc, rc.x + rc.w / 2, rc.y + rc.h / 2, rc.h / 2 - 1);
  InflateRect(&rc, -2, -2);

	SetBrushColor(hdc, MapRGB888(hdc, fore_c));
	FillCircle(hdc, rc.x + rc.w / 2, rc.y + rc.h / 2, rc.h / 2 - 1);
   //FillRoundRect(hdc, &rc, MIN(rc.w, rc.h) >> 2);
}
/*
 * @brief  自定义滑动条绘制函数
 * @param  ds:	自定义绘制结构体
 * @retval NONE
*/
static void scrollbar_owner_draw(DRAWITEM_HDR *ds)
{
	HWND hwnd;
	HDC hdc;
	HDC hdc_mem;
	HDC hdc_mem1;
	RECT rc;
	RECT rc_cli;
	//	int i;

	hwnd = ds->hwnd;
	hdc = ds->hDC;
	GetClientRect(hwnd, &rc_cli);

	hdc_mem = CreateMemoryDC(SURF_SCREEN, rc_cli.w, rc_cli.h);
	hdc_mem1 = CreateMemoryDC(SURF_SCREEN, rc_cli.w, rc_cli.h);   
         
   	
	//绘制白色类型的滚动条
	draw_scrollbar(hwnd, hdc_mem1, color_bg, RGB888( 250, 250, 250), RGB888( 255, 255, 255));
	//绘制绿色类型的滚动条
	draw_scrollbar(hwnd, hdc_mem, color_bg, RGB888(	50, 205, 50), RGB888(50, 205, 50));
   SendMessage(hwnd, SBM_GETTRACKRECT, 0, (LPARAM)&rc);   

	//左
	BitBlt(hdc, rc_cli.x, rc_cli.y, rc.x, rc_cli.h, hdc_mem, 0, 0, SRCCOPY);
	//右
	BitBlt(hdc, rc.x + rc.w, 0, rc_cli.w - (rc.x + rc.w) , rc_cli.h, hdc_mem1, rc.x + rc.w, 0, SRCCOPY);

	//绘制滑块
	if (ds->State & SST_THUMBTRACK)//按下
	{
      BitBlt(hdc, rc.x, 0, rc.w, rc_cli.h, hdc_mem1, rc.x, 0, SRCCOPY);
		
	}
	else//未选中
	{
		BitBlt(hdc, rc.x, 0, rc.w, rc_cli.h, hdc_mem, rc.x, 0, SRCCOPY);
	}
	//释放内存MemoryDC
	DeleteDC(hdc_mem1);
	DeleteDC(hdc_mem);
}


HWND music_wnd_time;//歌曲进度条窗口句柄
SCROLLINFO sif;/*设置滑动条的参数*/

HWND wnd_lrc1;//歌词窗口句柄
HWND wnd_lrc2;//歌词窗口句柄
HWND wnd_lrc3;//歌词窗口句柄
HWND wnd_lrc4;//歌词窗口句柄
HWND wnd_lrc5;//歌词窗口句柄
HWND sub11_wnd; //播放键句柄
HWND horn_wnd; //播放键句柄
static LRESULT win_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){


   RECT rc;
   static BOOL res;
   switch(msg){
      case WM_CREATE:
      {
         u8 *jpeg_buf;
         u32 jpeg_size;
         JPG_DEC *dec;
        
        VS_Init();
        printf("vs1053:%4X\n",VS_Ram_Test());
        GUI_msleep(100);
//        VS_Sine_Test();	
        VS_HD_Reset();
        VS_Soft_Reset();
         res = RES_Load_Content(GUI_RGB_BACKGROUNG_PIC, (char**)&jpeg_buf, &jpeg_size);

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
         exit_sem = GUI_SemCreate(0,1);//创建一个信号量        
         music_icon[0].rc.y = 220-music_icon[0].rc.h/2;//居中
         //音量icon（切换静音模式），返回控件句柄值
         wnd_power = CreateWindow(BUTTON,L"A",WS_OWNERDRAW |WS_VISIBLE,//按钮控件，属性为自绘制和可视
                                  music_icon[0].rc.x,music_icon[0].rc.y,//位置坐标和控件大小
                                  music_icon[0].rc.w,music_icon[0].rc.h,//由music_icon[0]决定
                                  hwnd,ID_BUTTON_Power,NULL,NULL);//父窗口hwnd,ID为ID_BUTTON_Power，附加参数为： NULL
         //音乐列表icon
         CreateWindow(BUTTON,L"G",WS_OWNERDRAW |WS_VISIBLE, //按钮控件，属性为自绘制和可视
                      music_icon[1].rc.x,music_icon[1].rc.y,//位置坐标
                      music_icon[1].rc.w,music_icon[1].rc.h,//控件大小
                      hwnd,ID_BUTTON_List,NULL,NULL);//父窗口hwnd,ID为ID_BUTTON_List，附加参数为： NULL
         //歌词icon
//         CreateWindow(BUTTON,L"W",WS_OWNERDRAW |WS_VISIBLE,
//                      music_icon[2].rc.x,music_icon[2].rc.y,
//                      music_icon[2].rc.w,music_icon[2].rc.h,
//                      hwnd,ID_BUTTON_Equa,NULL,NULL);
         
         //喇叭icon
         horn_wnd = CreateWindow(BUTTON,L"Q",WS_OWNERDRAW |WS_VISIBLE,
                      music_icon[3].rc.x,music_icon[3].rc.y,
                      music_icon[3].rc.w,music_icon[3].rc.h,
                      hwnd,ID_BUTTON_Horn,NULL,NULL);

         //上一首
         CreateWindow(BUTTON,L"S",WS_OWNERDRAW |WS_VISIBLE,
                      music_icon[5].rc.x,music_icon[5].rc.y,
                      music_icon[5].rc.w,music_icon[5].rc.h,
                      hwnd,ID_BUTTON_BACK,NULL,NULL);
         //下一首
         CreateWindow(BUTTON,L"V",WS_OWNERDRAW |WS_VISIBLE,
                      music_icon[7].rc.x,music_icon[7].rc.y,
                      music_icon[7].rc.w,music_icon[7].rc.h,
                      hwnd,ID_BUTTON_NEXT,NULL,NULL);
         //播放键
         sub11_wnd = CreateWindow(BUTTON,L"U",WS_OWNERDRAW |WS_VISIBLE,
                      music_icon[6].rc.x,music_icon[6].rc.y,
                      music_icon[6].rc.w,music_icon[6].rc.h,
                      hwnd,ID_BUTTON_START,NULL,NULL); 

//         CreateWindow(BUTTON, L"N", BS_FLAT | BS_NOTIFY|WS_OWNERDRAW |WS_VISIBLE,
//                        0, 0, 80, 80, hwnd, ID_EXIT, NULL, NULL); 
         /*********************歌曲进度条******************/
         sif.cbSize = sizeof(sif);
         sif.fMask = SIF_ALL;
         sif.nMin = 0;
         sif.nMax = 255;
         sif.nValue = 0;//初始值
         sif.TrackSize = 18;//滑块值
         sif.ArrowSize = 0;//两端宽度为0（水平滑动条）          
         music_wnd_time = CreateWindow(SCROLLBAR, L"SCROLLBAR_Time",  WS_OWNERDRAW| WS_VISIBLE, 
                         45, 183, 230, 20, hwnd, ID_SCROLLBAR_TIMER, NULL, NULL);
         SendMessage(music_wnd_time, SBM_SETSCROLLINFO, TRUE, (LPARAM)&sif);         

         /*********************音量值滑动条******************/
         sif_power.cbSize = sizeof(sif_power);
         sif_power.fMask = SIF_ALL;
         sif_power.nMin = 80;
         sif_power.nMax = 254;        //音量最大值为254
         sif_power.nValue = 220;      //初始音量值
         sif_power.TrackSize = 18;    //滑块值
         sif_power.ArrowSize = 0;     //两端宽度为0（水平滑动条）
         
         wnd = CreateWindow(SCROLLBAR, L"SCROLLBAR_R", WS_OWNERDRAW|WS_TRANSPARENT, 
                            40, 220-31/2+5, 60, 20, hwnd, ID_SCROLLBAR_POWER, NULL, NULL);
         SendMessage(wnd, SBM_SETSCROLLINFO, TRUE, (LPARAM)&sif_power);
         
         //以下控件为TEXTBOX的创建
         wnd_lrc1 = CreateWindow(TEXTBOX, L" ", WS_OWNERDRAW | WS_VISIBLE, 
                                0, 40, 300, 20, hwnd, ID_TEXTBOX_LRC1, NULL, NULL);  
         SendMessage(wnd_lrc1,TBM_SET_TEXTFLAG,0,DT_VCENTER|DT_CENTER|DT_BKGND);                                
         wnd_lrc2 = CreateWindow(TEXTBOX, L" ", WS_OWNERDRAW | WS_VISIBLE, 
                                0, 70, 300, 20, hwnd, ID_TEXTBOX_LRC2, NULL, NULL); 
         SendMessage(wnd_lrc2,TBM_SET_TEXTFLAG,0,DT_VCENTER|DT_CENTER|DT_BKGND);
         wnd_lrc3 = CreateWindow(TEXTBOX, L" ", WS_OWNERDRAW | WS_VISIBLE, 
                                0, 100, 300, 20, hwnd, ID_TEXTBOX_LRC3, NULL, NULL);  
         SendMessage(wnd_lrc3,TBM_SET_TEXTFLAG,0,DT_VCENTER|DT_CENTER|DT_BKGND);     
         wnd_lrc4 = CreateWindow(TEXTBOX, L" ", WS_OWNERDRAW | WS_VISIBLE, 
                                0, 130, 300, 20, hwnd, ID_TEXTBOX_LRC4, NULL, NULL);  
         SendMessage(wnd_lrc4,TBM_SET_TEXTFLAG,0,DT_VCENTER|DT_CENTER|DT_BKGND); 
         wnd_lrc5 = CreateWindow(TEXTBOX, L" ", WS_OWNERDRAW | WS_VISIBLE, 
                                0, 150, 300, 20, hwnd, ID_TEXTBOX_LRC5, NULL, NULL);  
         SendMessage(wnd_lrc5,TBM_SET_TEXTFLAG,0,DT_VCENTER|DT_CENTER|DT_BKGND);  			
         CreateWindow(BUTTON,L"歌曲文件名",WS_OWNERDRAW|WS_TRANSPARENT|WS_VISIBLE,
                      30,3,260,20,hwnd,ID_TB5,NULL,NULL);


         CreateWindow(BUTTON,L"00:00",WS_TRANSPARENT|WS_OWNERDRAW|WS_VISIBLE,
                      280,200-25,40,30,hwnd,ID_TB1,NULL,NULL);
     

         CreateWindow(BUTTON,L"00:00",WS_TRANSPARENT|WS_OWNERDRAW|WS_VISIBLE,
                      0,200-25,40,30,hwnd,ID_TB2,NULL,NULL);
    
         //获取音乐列表
         memset(music_playlist, 0, sizeof(music_playlist));
         memset(music_lcdlist, 0, sizeof(music_lcdlist));
         scan_files(path);
         //创建音乐播放线程
         App_PlayMusic(hwnd);
        
         CreateWindow(BUTTON, L"O", BS_FLAT | BS_NOTIFY |WS_OWNERDRAW|WS_VISIBLE|WS_TRANSPARENT,
                        286, 3, 23, 23, hwnd, ID_EXIT, NULL, NULL); 


         GetClientRect(hwnd,&rc); //获得窗口的客户区矩形
         break;
      }

      case WM_NOTIFY:
      {
         u16 code,  id, ctr_id;;
         id  =LOWORD(wParam);//获取消息的ID码
         code=HIWORD(wParam);//获取消息的类型
         ctr_id = LOWORD(wParam); //wParam低16位是发送该消息的控件ID. 
         
         NMHDR *nr;        
         HDC hdc;
         //发送单击
         if(code == BN_CLICKED)
         { 
            switch(id)
            {
               //音乐ICON处理case
               case ID_EXIT:
               {
//                  vTaskDelete(h_music);
                  PostCloseMessage(hwnd);
                  break;
               }
               case ID_BUTTON_List:
               {
                  enter_flag = 1;
                  IsCreateList = 1;
                  App_MusicList();
                            
                  break;
               }
               //音量icon处理case
               case ID_BUTTON_Power:
               {
//                  RECT rc_cli = {80, 431, 150, 30};
                  music_icon[0].state = ~music_icon[0].state;
                  //InvalidateRect(hwnd, &music_icon[0].rc, TRUE);
                  //当音量icon未被按下时
                  if(music_icon[0].state == FALSE)
                  {
                       RedrawWindow(hwnd, NULL, RDW_ALLCHILDREN|RDW_INVALIDATE);
                       ShowWindow(wnd, SW_HIDE); //窗口隐藏
                  }
                  //当音量icon被按下时，设置为静音模式
                  else
                  {      
                       ShowWindow(wnd, SW_SHOW); //窗口显示
                  }

                  break;
               }                  

               //播放icon处理case
               case ID_BUTTON_START:
               {
                     music_icon[6].state = ~music_icon[6].state;
                     //擦除icon的背景
                     //

                     if(music_icon[6].state == FALSE)
                     {

                        vTaskResume(h_music);
                        SetWindowText(sub11_wnd, L"U");
                        ResetTimer(hwnd, 1, 200, TMR_START,NULL);
                        
                     }
                     else if(music_icon[6].state != FALSE)
                     {
                        vTaskSuspend(h_music);             
                        SetWindowText(sub11_wnd, L"T");
                        ResetTimer(hwnd, 1, 200, NULL,NULL);                       

                        
                     }  
                     InvalidateRect(hwnd, &music_icon[6].rc, TRUE);                     
                  break;                  
               }
               //下一首icon处理case
               case ID_BUTTON_NEXT:
               {     
                  WCHAR wbuf[128];
//                  COLORREF color;
                  play_index++;
                  if(play_index >= music_file_num) play_index = 0;
                  if(play_index < 0) play_index = music_file_num - 1;
                  Music_State = STA_SWITCH;
                  hdc = GetDC(hwnd);
                                
//                  color = GetPixel(hdc, 385, 404);  
                  x_mbstowcs(wbuf, music_lcdlist[play_index], FILE_NAME_LEN);
                  SetWindowText(GetDlgItem(hwnd, ID_TB5), wbuf);
                                 
                  SendMessage(music_wnd_time, SBM_SETVALUE, TRUE, 0); //设置进度值
                  SetWindowText(GetDlgItem(MusicPlayer_hwnd, ID_TB1), L"00:00"); 
                  SetWindowText(GetDlgItem(MusicPlayer_hwnd, ID_TB2), L"00:00"); 
                  ReleaseDC(hwnd, hdc);
                  
                  break;
               }
               //上一首icon处理case
               case ID_BUTTON_BACK:
               {
                 
//                  COLORREF color;
                  play_index--;
                  if(play_index > music_file_num) play_index = 0;
                  if(play_index < 0) play_index = music_file_num - 1;
                  Music_State = STA_SWITCH;   
                  hdc = GetDC(hwnd);

                  ReleaseDC(hwnd, hdc);            
                  break;
               }            
            
               //喇叭icon处理case
               case ID_BUTTON_Horn:
               {
//                     WCHAR wbuf[128];
                     music_icon[3].state = ~music_icon[3].state;
                     //擦除icon的背景
                     //

                     if(music_icon[3].state == FALSE)
                     {
                        SetWindowText(horn_wnd, L"Q");
                        VS_Horn_Set(DISABLE);
                     }
                     else if(music_icon[3].state != FALSE)
                     {          
                        SetWindowText(horn_wnd, L"P");
                        VS_Horn_Set(ENABLE);
                     }  
                     InvalidateRect(hwnd, &music_icon[3].rc, TRUE);                     
                  break;                  
               }  
               
               //喇叭icon处理case
               case ID_SCROLLBAR_TIMER:
               {
                  //置位进度条变更位置
                  chgsch = 1;                    
                  break;                  
               } 
            }
         }
         
      	nr = (NMHDR*)lParam; //lParam参数，是以NMHDR结构体开头.
         //音量条处理case
         if (ctr_id == ID_SCROLLBAR_POWER)
         {
            NM_SCROLLBAR *sb_nr;
            sb_nr = (NM_SCROLLBAR*)nr; //Scrollbar的通知消息实际为 NM_SCROLLBAR扩展结构,里面附带了更多的信息.
            static int ttt = 0;
            switch (nr->code)
            {
               case SBN_THUMBTRACK: //R滑块移动
               {
                  power= sb_nr->nTrackValue; //得到当前的音量值
                  if(power == 0) 
                  {
                     SetWindowText(wnd_power, L"J");
                     ttt = 1;
                  }
                  else
                  {
                     if(ttt == 1)
                     {
                        ttt = 0;
                        SetWindowText(wnd_power, L"A");
                     }
                  } 
                  
                  SendMessage(nr->hwndFrom, SBM_SETVALUE, TRUE, power); //发送SBM_SETVALUE，设置音量值
               } 
               break;
               
               case SBN_CLICKED:
               {
                 VS_Set_Vol(power);     // 设置vs1053的音量
               }
               break;
            }
         }
         
         //进度条处理case
         if (ctr_id == ID_SCROLLBAR_TIMER)
         {
            NM_SCROLLBAR *sb_nr;
            int i = 0;
            sb_nr = (NM_SCROLLBAR*)nr; //Scrollbar的通知消息实际为 NM_SCROLLBAR扩展结构,里面附带了更多的信息.
            switch (nr->code)
            {
               case SBN_THUMBTRACK: //R滑块移动
               {
                  i = sb_nr->nTrackValue; //获得滑块当前位置值                
                  SendMessage(nr->hwndFrom, SBM_SETVALUE, TRUE, i); //设置进度值
//                  //置位进度条变更位置
                  chgsch = 2;     // 正在改变进度条
               }
               break;
               
               case SBN_CLICKED:
               {
                 chgsch = 1;    //调整滑动条结束，更新进度条
               }
               break;
            }
         }         
         
         break;
      } 
      //重绘制函数消息
      case WM_DRAWITEM:
      {
         DRAWITEM_HDR *ds;
         ds = (DRAWITEM_HDR*)lParam;        
         if(ds->ID == ID_EXIT)
         {
            exit_owner_draw(ds);
            return TRUE;
         }
         if (ds->ID == ID_SCROLLBAR_POWER || ds->ID == ID_SCROLLBAR_TIMER)
         {
            scrollbar_owner_draw(ds);
            return TRUE;
         }
         if (ds->ID >= ID_BUTTON_Power && ds->ID<= ID_BUTTON_Horn)
         {
            button_owner_draw(ds);
            return TRUE;
         }
         if(ds->ID == ID_EXIT)
         {
            exit_owner_draw(ds);
            return TRUE;
         }
         if(ds->ID == ID_TB1 || ds->ID == ID_TB2 || ds->ID == ID_TB5)
         {
            Music_Button_OwnerDraw(ds);
           return TRUE;
         }
         if(ds->ID >= ID_TEXTBOX_LRC1 && ds->ID <= ID_TEXTBOX_LRC5)
         {
            _music_textbox_OwnerDraw(ds);
            return TRUE;
         }


      }     
      //绘制窗口界面消息
      case WM_PAINT:
      {
         PAINTSTRUCT ps;
         HDC hdc;//屏幕hdc
         
         //开始绘制
         hdc = BeginPaint(hwnd, &ps); 
////         if(tt == 0)
//         {
////            tt = 1;
////            BitBlt(hdc_mem11, 0, 0, 50, 50, hdc_bk, 135, 95, SRCCOPY);
////           if (show_lrc == 0)
//            RotateBitmap(hdc_mem11,25,25,&bm_0,0);
//         }            
//         
//        rc.x=135;
//        rc.y=60;
//        rc.w=50;
//        rc.h=50;
//         
//         SetTextColor(hdc, MapARGB(rotate_disk_hdc, 255, 50, 205, 50));
//         SetFont(hdc, iconFont_50);
//         DrawTextEx(hdc,L"a",-1,&rc,DT_SINGLELINE|DT_VCENTER|DT_CENTER,NULL);

//        BitBlt(hdc,rc.x,rc.y,rc.w,rc.h,rotate_disk_hdc,0,0,SRCCOPY);
             
         //获取屏幕点（385，404）的颜色，作为透明控件的背景颜色
        color_bg = GetPixel(hdc, 300, 200);
        EndPaint(hwnd, &ps);
        break;
      }
      case WM_ERASEBKGND:
      {
         HDC hdc =(HDC)wParam;
         RECT rc =*(RECT*)lParam;
         //GetClientRect(hwnd, &rc_cli);//获取客户区位置信息
         SetBrushColor(hdc, MapRGB(hdc, 250,0,0));
         FillRect(hdc, &rc); 
         if(res!=FALSE)
            BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_bk, rc.x, rc.y, SRCCOPY);         
 
         return TRUE;
      }
      //设置TEXTBOX的背景颜色以及文字颜色
		case	WM_CTLCOLOR:
		{
			/* 控件在绘制前，会发送 WM_CTLCOLOR到父窗口.
			 * wParam参数指明了发送该消息的控件ID;lParam参数指向一个CTLCOLOR的结构体指针.
			 * 用户可以通过这个结构体改变控件的颜色值.用户修改颜色参数后，需返回TRUE，否则，系统
			 * 将忽略本次操作，继续使用默认的颜色进行绘制.
			 *
			 */
			u16 id;
			id =LOWORD(wParam);
         //第三个TEXTBOX为当前的歌词行
			if(id== ID_TEXTBOX_LRC3)
			{
				CTLCOLOR *cr;
				cr =(CTLCOLOR*)lParam;
				cr->TextColor =RGB888(255,255,255);//文字颜色（RGB888颜色格式)
				cr->BackColor =RGB888(0,0,0);//背景颜色（RGB888颜色格式)
				//cr->BorderColor =RGB888(255,10,10);//边框颜色（RGB888颜色格式)
				return TRUE;
			}
			else if(id == ID_TEXTBOX_LRC1||id == ID_TEXTBOX_LRC2||id == ID_TEXTBOX_LRC5||id == ID_TEXTBOX_LRC4)
			{
				CTLCOLOR *cr;
				cr =(CTLCOLOR*)lParam;
				cr->TextColor =RGB888(250,0,0);//文字颜色（RGB888颜色格式)
				cr->BackColor =RGB888(0,0,0);//背景颜色（RGB888颜色格式)
				//cr->BorderColor =RGB888(255,10,10);//边框颜色（RGB888颜色格式)
				return TRUE;				
			}
         if(id== ID_TB1 || id== ID_TB2 || id== ID_TB5 )
			{
				CTLCOLOR *cr;
				cr =(CTLCOLOR*)lParam;
				cr->TextColor =RGB888(255,255,255);//文字颜色（RGB888颜色格式)
				cr->BackColor =RGB888(0,0,0);//背景颜色（RGB888颜色格式)
				cr->BorderColor =RGB888(255,0,0);//边框颜色（RGB888颜色格式)
				return TRUE;
			}
         return FALSE;
		}     
    
    //关闭窗口消息处理case
      case WM_CLOSE:
      { 
        DestroyWindow(hwnd);
        return TRUE;	
      }

      //关闭窗口消息处理case
      case WM_DESTROY:
      {        
        Music_State = STA_IDLE;		/* 待机状态 */
        time2exit = 1;
        GUI_SemWait(exit_sem, 0xFFFFFFFF);
        vTaskDelete(h_music);    // 删除播放线程
        GUI_SemDelete(exit_sem);
        DeleteDC(hdc_bk);
        thread = 0;
        time2exit = 0;
        play_index = 0;
        res = FALSE;
        music_file_num = 0;
        power = 220;
        PostQuitMessage(hwnd);	        
        return TRUE;	
      }
      
      default:
         return DefWindowProc(hwnd, msg, wParam, lParam);
   }
     
   return WM_NULL;
}


//音乐播放器句柄
HWND	MusicPlayer_hwnd;
void	GUI_MUSICPLAYER_DIALOG(void)
{
	
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
	MusicPlayer_hwnd = CreateWindowEx(WS_EX_NOFOCUS|WS_EX_FRAMEBUFFER,
                                    &wcex,
                                    L"GUI_MUSICPLAYER_DIALOG",
                                    WS_VISIBLE|WS_CLIPCHILDREN|WS_OVERLAPPED,
                                    0, 0, GUI_XSIZE, GUI_YSIZE,
                                    NULL, NULL, NULL, NULL);

	//显示主窗口
	ShowWindow(MusicPlayer_hwnd, SW_SHOW);

	//开始窗口消息循环(窗口关闭并销毁时,GetMessage将返回FALSE,退出本消息循环)。
	while (GetMessage(&msg, MusicPlayer_hwnd))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
//  UpdateWindow( GetDesktopWindow());
}


