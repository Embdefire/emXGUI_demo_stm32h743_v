#include "emXGUI.h"
#include "GUI_MUSICPLAYER_DIALOG.h"
#include "Widget.h"
#include "./OwnerDraw/Widget.h"
#include "GUI_AppDef.h"
#include "emXGUI_JPEG.h"
#include <string.h>
#include	"CListMenu.h"
#include "x_libc.h"
#include "./wm8978/bsp_wm8978.h" 
#include "./mp3Player/mp3Player.h"
//歌词结构体
LYRIC lrc;
SCROLLINFO g_sif_power;//音量滑动条
SCROLLINFO g_sif_time;//歌曲进度
//旋转图标
static HDC rotate_disk_hdc;
static SURFACE *pSurf;
static HDC hdc_rotate=NULL;//旋转图层
static BITMAP bm_rotate;
static char path[100]="0:";//文件根目录
MUSIC_DIALOG_Typedef MusicDialog = 
{
  .filename = "bg.jpg",
  .power = 20,
  .playindex = 0,
};
#if 1
char music_playlist[MUSIC_MAX_NUM][100];//播放List
char music_lcdlist[MUSIC_MAX_NUM][100];//显示list
//歌词数组--存放歌词数据
uint8_t ReadBuffer1[1024*5]={0};
#else
char **music_playlist;
char **music_lcdlist;
#endif
//图标管理数组
ICON_Typedef music_icon[12] = {
   {"yinliang",         {20,400,48,48},       FALSE},//音量
   {"yinyueliebiao",    {668,404,48,48},      FALSE},//音乐列表
   {"geci",             {728,404,72,72},      FALSE},//歌词栏
   {"NULL",             {0,0,0,0},            FALSE},//无
   {"NULL",             {0,0,0,0},            FALSE},//无
   {"shangyishou",      {294, 404, 72, 72},   FALSE},//上一首
   {"zanting/bofang",   {364, 406, 72, 72},   FALSE},//播放
   {"xiayishou",        {448, 404, 72, 72},   FALSE},//下一首
  
};
/*
 * @brief  绘制滚动条
 * @param  hwnd:   滚动条的句柄值
 * @param  hdc:    绘图上下文
 * @param  back_c：背景颜色
 * @param  Page_c: 滚动条Page处的颜色
 * @param  fore_c：滚动条滑块的颜色
 * @retval NONE
*/
static void draw_scrollbar(HWND hwnd, HDC hdc, COLOR_RGB32 Page_c, COLOR_RGB32 fore_c)
{
	RECT rc,rc_tmp;
   RECT rc_scrollbar;
	GetClientRect(hwnd, &rc);
	/* 背景 */
   GetClientRect(hwnd, &rc_tmp);//得到控件的位置
   GetClientRect(hwnd, &rc);//得到控件的位置
   WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//坐标转换
   
   BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, MusicDialog.hdc_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);

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
static void _music_scrollbar_ownerdraw(DRAWITEM_HDR *ds)
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
	draw_scrollbar(hwnd, hdc_mem1,  RGB888( 250, 250, 250), RGB888( 255, 255, 255));
	//绘制绿色类型的滚动条
	draw_scrollbar(hwnd, hdc_mem,  RGB888(	50, 205, 50), RGB888(50, 205, 50));
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

//绘制POWER，LIST，LRC等按钮
static void button_owner_draw(DRAWITEM_HDR *ds)
{
  HDC hdc; //控件窗口HDC
  HWND hwnd; //控件句柄 
  RECT rc_cli, rc_tmp;//控件的位置大小矩形
  WCHAR wbuf[128];
  hwnd = ds->hwnd;
  hdc = ds->hDC; 
//   if(ds->ID ==  ID_BUTTON_START && show_lrc == 1)
//      return;
   //获取控件的位置大小信息
  GetClientRect(hwnd, &rc_cli);
  GetClientRect(hwnd, &rc_tmp);//得到控件的位置
  WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//坐标转换

  BitBlt(hdc, rc_cli.x, rc_cli.y, rc_cli.w, rc_cli.h, MusicDialog.hdc_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);
   
	GetWindowText(ds->hwnd,wbuf,128); //获得按钮控件的文字  
//   if(ds->ID == ID_BUTTON_Power || ds->ID == ID_BUTTON_MINISTOP){
//      SetBrushColor(hdc, color_bg);
//      FillRect(hdc, &rc_cli);
//   }
   
//   SetBrushColor(hdc_mem,MapARGB(hdc_mem, 0, 255, 250, 250));
//   FillRect(hdc_mem, &rc_cli);
   //播放键使用100*100的字体
   if(ds->ID == eID_BUTTON_START)
      SetFont(hdc, ctrlFont72);
   else if(ds->ID == eID_BUTTON_NEXT || ds->ID == eID_BUTTON_BACK)
      SetFont(hdc, ctrlFont64);
   else
      SetFont(hdc, ctrlFont48);
   //设置按键的颜色
   SetTextColor(hdc, MapRGB(hdc,250,250,250));
   //NEXT键、BACK键和LIST键按下时，改变颜色
	if((ds->State & BST_PUSHED) )
	{ //按钮是按下状态
		SetTextColor(hdc, MapRGB(hdc, 105,105,105));      //设置文字色     
	}
 
   DrawText(hdc, wbuf,-1,&rc_cli,DT_VCENTER);//绘制文字(居中对齐方式)
   
//   BitBlt(hdc, rc_cli.x, rc_cli.y, rc_cli.w, rc_cli.h, hdc_mem, 0, 0, SRCCOPY);
//   
//   DeleteDC(hdc_mem);  
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

  BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, MusicDialog.hdc_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);
  SetTextColor(hdc, MapRGB(hdc, 255, 255, 255));


  GetWindowText(hwnd, wbuf, 128); //获得按钮控件的文字

  DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_CENTER);//绘制文字(居中对齐方式)

}
static void _music_ExitButton_OwnerDraw(DRAWITEM_HDR *ds) //绘制一个按钮外观
{
	HWND hwnd;
	HDC hdc;
	RECT rc,rc_tmp;
	WCHAR wbuf[128];

	hwnd = ds->hwnd; //button的窗口句柄.
	hdc = ds->hDC;   //button的绘图上下文句柄.
	rc = ds->rc;     //button的绘制矩形区.
  GetClientRect(hwnd, &rc_tmp);//得到控件的位置
  WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//坐标转换
  BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, MusicDialog.hdc_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);
	SetBrushColor(hdc, MapRGB(hdc, COLOR_DESKTOP_BACK_GROUND));
   
   FillCircle(hdc, rc.x+rc.w, rc.y, rc.w);
	//FillRect(hdc, &rc); //用矩形填充背景

   if (ds->State & BST_PUSHED)
	{ //按钮是按下状态
//    GUI_DEBUG("ds->ID=%d,BST_PUSHED",ds->ID);
//		SetBrushColor(hdc,MapRGB(hdc,150,200,250)); //设置填充色(BrushColor用于所有Fill类型的绘图函数)
//		SetPenColor(hdc,MapRGB(hdc,250,0,0));        //设置绘制色(PenColor用于所有Draw类型的绘图函数)
		SetTextColor(hdc, MapRGB(hdc, 105, 105, 105));      //设置文字色
	}
	else
	{ //按钮是弹起状态
//		SetBrushColor(hdc,MapRGB(hdc,255,255,255));
//		SetPenColor(hdc,MapRGB(hdc,0,250,0));
		SetTextColor(hdc, MapRGB(hdc, 255, 255, 255));
	}

	  /* 使用控制图标字体 */
	SetFont(hdc, ctrlFont64);
	//  SetTextColor(hdc,MapRGB(hdc,255,255,255));

	GetWindowText(hwnd, wbuf, 128); //获得按钮控件的文字
   rc.y = -10;
   rc.x = 16;
	DrawText(hdc, wbuf, -1, &rc, NULL);//绘制文字(居中对齐方式)


  /* 恢复默认字体 */
	SetFont(hdc, defaultFont);

}


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


/**
  * @brief  播放音乐列表进程
  * @param  hwnd：屏幕窗口的句柄
  * @retval 无
  * @notes  
  */

int stop_flag = 0;
static int thread=0;
char music_name[100]={0};//歌曲名数组
FRESULT f_result; 
FIL     f_file;
UINT    f_num;
static void App_PlayMusic(HWND hwnd)
{
	int app=0;
  HDC hdc;
  SCROLLINFO sif;
	if(thread==0)
	{  
      //h_music=rt_thread_create("App_PlayMusic",(void(*)(void*))App_PlayMusic,NULL,5*1024,5,1);
    GUI_Thread_Create(App_PlayMusic,"MUSIC_DIALOG",5*1024,NULL,5,5);
    thread =1;
    return;
	}
	while(thread) //线程已创建了
	{     
		if(app==0)
		{
			app=1;   
      int i = 0;      
      //读取歌词文件
      while(music_playlist[MusicDialog.playindex][i]!='\0')
      {
        music_name[i]=music_playlist[MusicDialog.playindex][i];
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
      //打开失败（未找到该歌词文件），则将flag清零，表示没有读取到该歌词文件
      else
      {
        lrc.flag = 0;
        printf("读取失败\n");
      }
      //关闭文件
      f_close(&f_file);	 

      mp3PlayerDemo(music_playlist[MusicDialog.playindex]);
      app = 0;

	 
//         
//         app=0;
//         //使用 GETDC之后需要释放掉HDC
//         ReleaseDC(hwnd, hdc);
//         //进行任务调度
//         GUI_msleep(20);
		}
	   
   }
   

}
/**
* @brief  scan_files 递归扫描sd卡内的歌曲文件
  * @param  path:初始扫描路径
  * @retval result:文件系统的返回值
  */
static FRESULT scan_Musicfiles(char* path) 
{ 
  FRESULT res; 		//部分在递归过程被修改的变量，不用全局变量	
  FILINFO fno; 
  DIR dir; 
  int i; 
  char *fn; 
  char file_name[MUSICFILE_NAME_LEN];	
	
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
     if (res != FR_OK || fno.fname[0] == 0) break; 	//为空时表示所有项目读取完毕，跳出
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
        res = scan_Musicfiles(path);											//递归遍历 
        if (res != FR_OK) 
					break; 																		//打开失败，跳出循环
        path[i] = 0; 
      } 
      else 
		{ 
				//printf("%s%s\r\n", path, fn);								//输出文件名
				if(strstr(fn,".wav")||strstr(fn,".WAV")||strstr(fn,".mp3")||strstr(fn,".MP3"))//判断是否mp3或wav文件
				{
					if ((strlen(path)+strlen(fn)<MUSICFILE_NAME_LEN)&&(MusicDialog.music_file_num<MUSIC_MAX_NUM))
					{
						sprintf(file_name, "%s/%s", path, fn);						
						memcpy(music_playlist[MusicDialog.music_file_num],file_name,strlen(file_name));
                  //printf("%s\r\n", music_playlist[music_file_num]);
						memcpy(music_lcdlist[MusicDialog.music_file_num],fn,strlen(fn));						
						MusicDialog.music_file_num++;//记录文件个数
					}
				}//if mp3||wav
      }//else
     } //for
  } 
  return res; 
}

FIL file1;											/* file objects */

static void Dialog_Init(HWND hwnd)
{
  //Step1:初始化背景
  u8 *jpeg_buf;
  u32 jpeg_size;
  JPG_DEC *dec;
  MusicDialog.Load_File = RES_Load_Content(MusicDialog.filename, (char**)&jpeg_buf, &jpeg_size);
  MusicDialog.hdc_bk = CreateMemoryDC(SURF_SCREEN, 800, 480);
  if(MusicDialog.Load_File)
  {
    /* 根据图片数据创建JPG_DEC句柄 */
    dec = JPG_Open(jpeg_buf, jpeg_size);

    /* 绘制至内存对象 */
    JPG_Draw(MusicDialog.hdc_bk, 0, 0, dec);

    /* 关闭JPG_DEC句柄 */
    JPG_Close(dec);
  }
  /* 释放图片内容空间 */
  RES_Release_Content((char **)&jpeg_buf);   
  //Step2:旋转图标
  rotate_disk_hdc = CreateMemoryDC(SURF_ARGB8888,240,240);
  /* 清空背景为透明 */
  ClrDisplay(rotate_disk_hdc,NULL,0);
  /* 绘制bmp到hdc */
  RECT rc = {0,0,240,240};
  SetTextColor(rotate_disk_hdc, MapARGB(rotate_disk_hdc, 255, 50, 205, 50));
  SetFont(rotate_disk_hdc, logoFont252);
  DrawTextEx(rotate_disk_hdc,L"a",-1,&rc,DT_SINGLELINE|DT_VCENTER|DT_CENTER,NULL);
  /* 转换成bitmap */
  DCtoBitmap(rotate_disk_hdc,&bm_rotate); 
  pSurf =CreateSurface(SURF_RGB565,240,240,-1,NULL);  
  SetTimer(hwnd, 1, 200, TMR_START,NULL);

  rc.x =0;
  rc.y =0;
  rc.w =240;
  rc.h =240;
  hdc_rotate =CreateDC(pSurf,&rc);
  DeleteDC(rotate_disk_hdc);
  //Step3：分配内存
    
  //Step4：读取音乐列表
  scan_Musicfiles(path);//出现hardfault时，注意线程栈的大小
  //Step5:initialize Hardware
	if (wm8978_Init()==0)
	{
		GUI_DEBUG("检测不到WM8978芯片!!!\n");
		while (1);	/* 停机 */
	}  
  mp3PlayerDemo("0:/mp3/张国荣-玻璃之情.mp3");
  
}

/**
* @brief  scan_files 递归扫描sd卡内的歌曲文件
  * @param  path:初始扫描路径
  * @retval result:文件系统的返回值
  */

static void MusicList_ReturnButton_OwnerDraw(DRAWITEM_HDR *ds) //绘制一个按钮外观
{
  HWND hwnd;
  HDC hdc;
  RECT rc;
  WCHAR wbuf[128];

  hwnd = ds->hwnd; //button的窗口句柄.
  hdc = ds->hDC;   //button的绘图上下文句柄.
  rc = ds->rc;     //button的绘制矩形区.




  SetBrushColor(hdc, MapRGB(hdc, 0,0,0));

  FillCircle(hdc, rc.x, rc.y, rc.w);
  SetBrushColor(hdc, MapRGB(hdc, 0,0,0));
  FillRect(hdc, &rc); //用矩形填充背景

  if (IsWindowEnabled(hwnd) == FALSE)
  {
    SetTextColor(hdc, MapRGB(hdc, COLOR_INVALID));
  }
  else if (ds->State & BST_PUSHED)
  { //按钮是按下状态
  //    GUI_DEBUG("ds->ID=%d,BST_PUSHED",ds->ID);
  //		SetBrushColor(hdc,MapRGB(hdc,150,200,250)); //设置填充色(BrushColor用于所有Fill类型的绘图函数)
  //		SetPenColor(hdc,MapRGB(hdc,250,0,0));        //设置绘制色(PenColor用于所有Draw类型的绘图函数)
    SetTextColor(hdc, MapRGB(hdc, 105, 105, 105));      //设置文字色
  }
  else
  { //按钮是弹起状态
  //		SetBrushColor(hdc,MapRGB(hdc,255,255,255));
  //		SetPenColor(hdc,MapRGB(hdc,0,250,0));
    SetTextColor(hdc, MapRGB(hdc, 255, 255, 255));
  }


  //	SetBrushColor(hdc,COLOR_BACK_GROUND);

  //	FillRect(hdc,&rc); //用矩形填充背景
  //	DrawRect(hdc,&rc); //画矩形外框
  //  
  //  FillCircle(hdc,rc.x+rc.w/2,rc.x+rc.w/2,rc.w/2); //用矩形填充背景FillCircle
  //	DrawCircle(hdc,rc.x+rc.w/2,rc.x+rc.w/2,rc.w/2); //画矩形外框

  /* 使用控制图标字体 */
  SetFont(hdc, ctrlFont48);
  //  SetTextColor(hdc,MapRGB(hdc,255,255,255));

  GetWindowText(ds->hwnd, wbuf, 128); //获得按钮控件的文字

  DrawText(hdc, wbuf, -1, &rc, DT_VCENTER);//绘制文字(居中对齐方式)
  rc.x = 35; 
  //   rc.y = 20;
  /* 恢复默认字体 */
  SetFont(hdc, defaultFont);
  DrawText(hdc, L"返回", -1, &rc, DT_VCENTER);
}



static void MusicList_Button_OwnerDraw(DRAWITEM_HDR *ds) //绘制一个按钮外观
{
	HWND hwnd;
	HDC hdc;
	RECT rc;
	WCHAR wbuf[128];

	hwnd = ds->hwnd; //button的窗口句柄.
	hdc = ds->hDC;   //button的绘图上下文句柄.
	rc = ds->rc;     //button的绘制矩形区.

	SetBrushColor(hdc, MapRGB(hdc, 0,0,0));
	FillRect(hdc, &rc); //用矩形填充背景

	if (IsWindowEnabled(hwnd) == FALSE)
	{
		SetTextColor(hdc, MapRGB(hdc, COLOR_INVALID));
	}
	else if (ds->State & BST_PUSHED)
	{ //按钮是按下状态
//    GUI_DEBUG("ds->ID=%d,BST_PUSHED",ds->ID);
//		SetBrushColor(hdc,MapRGB(hdc,150,200,250)); //设置填充色(BrushColor用于所有Fill类型的绘图函数)
//		SetPenColor(hdc,MapRGB(hdc,250,0,0));        //设置绘制色(PenColor用于所有Draw类型的绘图函数)
		SetTextColor(hdc, MapRGB(hdc, 105, 105, 105));      //设置文字色
	}
	else
	{ //按钮是弹起状态
//		SetBrushColor(hdc,MapRGB(hdc,255,255,255));
//		SetPenColor(hdc,MapRGB(hdc,0,250,0));
		SetTextColor(hdc, MapRGB(hdc, 255, 255, 255));
	}


	//	SetBrushColor(hdc,COLOR_BACK_GROUND);

	//	FillRect(hdc,&rc); //用矩形填充背景
	//	DrawRect(hdc,&rc); //画矩形外框
	//  
	//  FillCircle(hdc,rc.x+rc.w/2,rc.x+rc.w/2,rc.w/2); //用矩形填充背景FillCircle
	//	DrawCircle(hdc,rc.x+rc.w/2,rc.x+rc.w/2,rc.w/2); //画矩形外框

	  /* 使用控制图标字体 */
	SetFont(hdc, ctrlFont64);
	//  SetTextColor(hdc,MapRGB(hdc,255,255,255));

	GetWindowText(ds->hwnd, wbuf, 128); //获得按钮控件的文字

	DrawText(hdc, wbuf, -1, &rc, DT_VCENTER | DT_CENTER);//绘制文字(居中对齐方式)


  /* 恢复默认字体 */
	SetFont(hdc, defaultFont);

}
static LRESULT Dlg_List_WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static struct __obj_list *menu_list = NULL;
  static WCHAR (*wbuf)[128];  
  switch(msg)
  {
    case WM_CREATE:
    {
      int i = 0;
      list_menu_cfg_t cfg;
      RECT rc;
      GetClientRect(hwnd, &rc);   
      /* 需要分配N+1项，最后一项为空 */
      menu_list = (struct __obj_list *)GUI_VMEM_Alloc(sizeof(struct __obj_list)*(MusicDialog.music_file_num+1));
      wbuf = (WCHAR (*)[128])GUI_VMEM_Alloc(sizeof(WCHAR *)*MusicDialog.music_file_num);  
      if(menu_list == NULL) return 0;
      
      for(;i < MusicDialog.music_file_num; i++)
      {
        char p[128] ;
        strcpy(p, music_lcdlist[i]);
        int t, L;
        L = (int)strlen(p);
        if (L > 13)
        {
          for (t = L; t > 13; t --)
          {
          p[t] = p[t - 1];
          }
          p[13] = '\0';
          p[L + 1] = '\0';
        }            
        x_mbstowcs_cp936(wbuf[i], p, MUSICFILE_NAME_LEN);
        menu_list[i].pName = wbuf[i];
        menu_list[i].cbStartup = NULL;
        menu_list[i].icon = L"a";
        menu_list[i].bmp = NULL;
        menu_list[i].color = RGB_WHITE;
      } 
      /* 最后一项为空 */
      menu_list[i].pName = NULL;
      menu_list[i].cbStartup = NULL;
      menu_list[i].icon = NULL;
      menu_list[i].bmp = NULL;
      menu_list[i].color = NULL;         

      cfg.list_objs = menu_list; 
      cfg.x_num = 3;
      cfg.y_num = 2; 
      cfg.bg_color = 0;
      CreateWindow(&wcex_ListMenu,
                          L"ListMenu1",
                          WS_VISIBLE | LMS_ICONFRAME|LMS_PAGEMOVE,
                          rc.x + 100, rc.y + 80, rc.w - 200, rc.h-80,
                          hwnd,
                          eID_MUSICLIST,
                          NULL,
                          &cfg); 
      SendMessage(GetDlgItem(hwnd,eID_MUSICLIST), MSG_SET_SEL, 0, 0);
      CreateWindow(BUTTON, L"L", BS_FLAT | BS_NOTIFY | WS_OWNERDRAW |WS_VISIBLE,
                   0, rc.h * 1 / 2, 70, 70, hwnd, eMUSIC_VIEWER_ID_PREV, NULL, NULL);
      SetWindowFont(GetDlgItem(hwnd,eMUSIC_VIEWER_ID_PREV), ctrlFont48); 
      CreateWindow(BUTTON, L"K", BS_FLAT | BS_NOTIFY | WS_OWNERDRAW | WS_VISIBLE,
                   rc.w - 65, rc.h * 1 / 2, 70, 70, hwnd, eMUSIC_VIEWER_ID_NEXT, NULL, NULL);
      SetWindowFont(GetDlgItem(hwnd,eMUSIC_VIEWER_ID_NEXT), ctrlFont48);   
      CreateWindow(BUTTON, L"F", BS_FLAT | BS_NOTIFY|WS_OWNERDRAW |WS_VISIBLE,
                   0, 0, 240, 80, hwnd, eID_MUSIC_RETURN, NULL, NULL);  



      break;
    }
    
    case WM_DRAWITEM:
    {

      DRAWITEM_HDR *ds;

      ds = (DRAWITEM_HDR*)lParam;

      if(ds->ID == eID_MUSIC_RETURN)
        MusicList_ReturnButton_OwnerDraw(ds);
      else
        MusicList_Button_OwnerDraw(ds); //执行自绘制按钮
      return TRUE;

    } 
    case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hdc;
      RECT rc;//窗口大小
      GetClientRect(hwnd, &rc); //获得客户区矩形.
      hdc = BeginPaint(hwnd, &ps);
      //背景
      SetBrushColor(hdc, MapRGB(hdc, 0,0,0));
      FillRect(hdc, &rc);  
      //DrawBitmap(hdc,0,0,&bm_0,NULL);   
      rc.x = 0;
      rc.y = 0;
      rc.w = 800;
      rc.h = 80;
      SetTextColor(hdc, MapRGB(hdc, 250, 250, 250));
      DrawText(hdc, L"播放列表", -1, &rc, DT_VCENTER|DT_CENTER);
      EndPaint(hwnd, &ps);
      break;
    }
    case WM_NOTIFY:
    {
      u16 code, id;	
      LM_NMHDR *nm;
      code = HIWORD(wParam);
      id = LOWORD(wParam); 

      nm = (LM_NMHDR*)lParam;

      if (code == LMN_CLICKED)
      {
        switch (id)
        {
          case eID_MUSICLIST:
          {
            MusicDialog.playindex = nm->idx;//切换至下一首
            mp3player.ucStatus = STA_SWITCH;	
            GUI_DEBUG("%d", MusicDialog.playindex);
          }

        break;
        }

      }


      if (code == BN_CLICKED && id == eMUSIC_VIEWER_ID_PREV)
      {
        SendMessage(GetDlgItem(hwnd, eID_MUSICLIST), MSG_MOVE_PREV, TRUE, 0);
      }

      if (code == BN_CLICKED && id == eMUSIC_VIEWER_ID_NEXT)
      {
        SendMessage(GetDlgItem(hwnd, eID_MUSICLIST), MSG_MOVE_NEXT, TRUE, 0);
      }         
      if (code == BN_CLICKED && id == eID_MUSIC_RETURN)
      {
        PostCloseMessage(hwnd);
      }   
      break;
    }  
    case WM_DESTROY:
    {
      MusicDialog.mList_State = 0;
      return PostQuitMessage(hwnd);	
    }
    default: 
      return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return WM_NULL;
}
static LRESULT Dlg_LRC_WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch(msg)
  {
    case WM_CREATE:
    {
      //以下控件为TEXTBOX的创建
      CreateWindow(TEXTBOX, L"1", WS_VISIBLE, 
                      0, 0, 800, 60, hwnd, eID_TEXTBOX_LRC1, NULL, NULL);  
      SendMessage(GetDlgItem(hwnd, eID_TEXTBOX_LRC1),TBM_SET_TEXTFLAG,0,DT_VCENTER|DT_CENTER|DT_BKGND);                                
      CreateWindow(TEXTBOX, L"2", WS_VISIBLE, 
                      0, 60, 800, 60, hwnd, eID_TEXTBOX_LRC2, NULL, NULL); 
      SendMessage(GetDlgItem(hwnd, eID_TEXTBOX_LRC2),TBM_SET_TEXTFLAG,0,DT_VCENTER|DT_CENTER|DT_BKGND);
      CreateWindow(TEXTBOX, L"3", WS_VISIBLE, 
                      0, 120, 800, 60, hwnd, eID_TEXTBOX_LRC3, NULL, NULL);  
      SendMessage(GetDlgItem(hwnd, eID_TEXTBOX_LRC3),TBM_SET_TEXTFLAG,0,DT_VCENTER|DT_CENTER|DT_BKGND);     
      CreateWindow(TEXTBOX, L"4", WS_VISIBLE, 
                      0, 180, 800, 60, hwnd, eID_TEXTBOX_LRC4, NULL, NULL);  
      SendMessage(GetDlgItem(hwnd, eID_TEXTBOX_LRC4),TBM_SET_TEXTFLAG,0,DT_VCENTER|DT_CENTER|DT_BKGND); 
      CreateWindow(TEXTBOX, L"5", WS_VISIBLE, 
                      0, 240, 800, 50, hwnd, eID_TEXTBOX_LRC5, NULL, NULL);  
      SendMessage(GetDlgItem(hwnd, eID_TEXTBOX_LRC5),TBM_SET_TEXTFLAG,0,DT_VCENTER|DT_CENTER|DT_BKGND);      
      break;
    }
    //设置TEXTBOX的背景颜色以及文字颜色
    case	WM_CTLCOLOR:
    {
      u16 id;
      id =LOWORD(wParam);
         //第三个TEXTBOX为当前的歌词行
      if(id== eID_TEXTBOX_LRC3)
      {
        CTLCOLOR *cr;
        cr =(CTLCOLOR*)lParam;
        cr->TextColor =RGB888(255,255,255);//文字颜色（RGB888颜色格式)
        cr->BackColor =RGB888(0,0,0);//背景颜色（RGB888颜色格式)
        //cr->BorderColor =RGB888(255,10,10);//边框颜色（RGB888颜色格式)
        return TRUE;
      }
      else if(id == eID_TEXTBOX_LRC1||id == eID_TEXTBOX_LRC2||
              id == eID_TEXTBOX_LRC5||id == eID_TEXTBOX_LRC4)
      {
        CTLCOLOR *cr;
        cr =(CTLCOLOR*)lParam;
        cr->TextColor =RGB888(250,0,0);//文字颜色（RGB888颜色格式)
        cr->BackColor =RGB888(0,0,0);//背景颜色（RGB888颜色格式)
        //cr->BorderColor =RGB888(255,10,10);//边框颜色（RGB888颜色格式)
        return TRUE;				
      }
      return FALSE;
    }    
    default:
      return DefWindowProc(hwnd, msg, wParam, lParam);  
  }
  return WM_NULL;
}
static LRESULT music_win_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static int angle=0;
	switch (msg)
	{
      
    case WM_CREATE:
    {
      
      Dialog_Init(hwnd);
      
      music_icon[0].rc.y = 440-music_icon[0].rc.h/2;//以纵坐标440居中对齐
      CreateWindow(BUTTON,L"A",WS_OWNERDRAW |WS_VISIBLE,//按钮控件，属性为自绘制和可视
                   music_icon[0].rc.x,music_icon[0].rc.y,//位置坐标和控件大小
                   music_icon[0].rc.w,music_icon[0].rc.h,//由music_icon[0]决定
                   hwnd,eID_BUTTON_Power,NULL,NULL);//父窗口hwnd,ID为ID_BUTTON_Power，附加参数为： NULL     
      music_icon[1].rc.y = 440-music_icon[1].rc.h/2;//以纵坐标440居中对齐      
      //音乐列表icon
      CreateWindow(BUTTON,L"G",WS_OWNERDRAW |WS_VISIBLE, //按钮控件，属性为自绘制和可视
                   music_icon[1].rc.x,music_icon[1].rc.y,//位置坐标
                   music_icon[1].rc.w,music_icon[1].rc.h,//控件大小
                   hwnd,eID_BUTTON_List,NULL,NULL);//父窗口hwnd,ID为ID_BUTTON_List，附加参数为： NULL
      music_icon[2].rc.y = 440-music_icon[2].rc.h/2;//以纵坐标440居中对齐
      //歌词icon
      CreateWindow(BUTTON,L"W",WS_OWNERDRAW |WS_VISIBLE,
                   music_icon[2].rc.x,music_icon[2].rc.y,
                   music_icon[2].rc.w,music_icon[2].rc.h,
                   hwnd,eID_BUTTON_LRC,NULL,NULL);
      //music_icon[5].rc.y = 440-music_icon[5].rc.h/2;//以纵坐标440居中对齐
      //上一首
      CreateWindow(BUTTON,L"S",WS_OWNERDRAW |WS_VISIBLE,
                   music_icon[5].rc.x,music_icon[5].rc.y,
                   music_icon[5].rc.w,music_icon[5].rc.h,
                   hwnd,eID_BUTTON_BACK,NULL,NULL);
      //music_icon[7].rc.y = 440-music_icon[7].rc.h/2;//以纵坐标440居中对齐
      //下一首 
      CreateWindow(BUTTON,L"V",WS_OWNERDRAW |WS_VISIBLE,
                   music_icon[7].rc.x,music_icon[7].rc.y,
                   music_icon[7].rc.w,music_icon[7].rc.h,
                   hwnd,eID_BUTTON_NEXT,NULL,NULL);
                   
      //music_icon[6].rc.y = 440-music_icon[6].rc.h/2;//以纵坐标440居中对齐
      //播放键
      CreateWindow(BUTTON,L"U",WS_OWNERDRAW |WS_VISIBLE,
                   music_icon[6].rc.x,music_icon[6].rc.y,
                   music_icon[6].rc.w,music_icon[6].rc.h,
                   hwnd,eID_BUTTON_START,NULL,NULL);       
      /*********************歌曲进度条******************/
      g_sif_time.cbSize = sizeof(g_sif_time);
      g_sif_time.fMask = SIF_ALL;
      g_sif_time.nMin = 0;
      g_sif_time.nMax = 255;
      g_sif_time.nValue = 0;//初始值
      g_sif_time.TrackSize = 30;//滑块值
      g_sif_time.ArrowSize = 0;//两端宽度为0（水平滑动条）          
      MusicDialog.TIME_Hwnd = CreateWindow(SCROLLBAR, L"SCROLLBAR_Time",  WS_OWNERDRAW| WS_VISIBLE, 
                                    80, 370, 640, 35, 
                                    hwnd, eID_SCROLLBAR_TIMER, NULL, NULL);
      SendMessage(GetDlgItem(hwnd, eID_SCROLLBAR_TIMER), SBM_SETSCROLLINFO, TRUE, (LPARAM)&g_sif_time);         

      /*********************音量值滑动条******************/
      g_sif_power.cbSize = sizeof(g_sif_power);
      g_sif_power.fMask = SIF_ALL;
      g_sif_power.nMin = 0;
      g_sif_power.nMax = 63;//音量最大值为63
      g_sif_power.nValue = MusicDialog.power;//初始音量值
      g_sif_power.TrackSize = 30;//滑块值
      g_sif_power.ArrowSize = 0;//两端宽度为0（水平滑动条）

      CreateWindow(SCROLLBAR, L"SCROLLBAR_R", WS_OWNERDRAW, 
                         70, 440-31/2, 150, 31, 
                         hwnd, eID_SCROLLBAR_POWER, NULL, NULL);
      SendMessage(GetDlgItem(hwnd, eID_SCROLLBAR_POWER), SBM_SETSCROLLINFO, TRUE, (LPARAM)&g_sif_power);


      CreateWindow(BUTTON,L"歌曲文件名",WS_OWNERDRAW|WS_VISIBLE,
                  100,0,600,80,hwnd,eID_MUSIC_ITEM,NULL,NULL);


      CreateWindow(BUTTON,L"00:00",WS_OWNERDRAW|WS_VISIBLE,
                  720,387-15,80,30,hwnd,eID_ALL_TIME,NULL,NULL);


      CreateWindow(BUTTON,L"00:00",WS_OWNERDRAW|WS_VISIBLE,
                   0,387-15,80,30,hwnd,eID_CUR_TIME,NULL,NULL);
                   
      CreateWindow(BUTTON, L"O", BS_FLAT | BS_NOTIFY |WS_OWNERDRAW|WS_VISIBLE,
                   730, 0, 70, 70, hwnd, eID_MUSIC_EXIT, NULL, NULL);     
      App_PlayMusic(hwnd);                   
      break;
    } 
    case WM_TIMER:
    {
      
      RECT rc;
      {
        if(1)
        {

          angle+=5;
          angle%=360;
          //ClrDisplay(hdc_mem11,NULL,MapRGB(hdc_mem11,0,0,0));
          BitBlt(hdc_rotate, 0, 0, 240, 240, MusicDialog.hdc_bk, 280, 120, SRCCOPY);
          RotateBitmap(hdc_rotate,120,120,&bm_rotate,angle);
        }
        rc.x=280;
        rc.y=120;
        rc.w=240;
        rc.h=240;

        InvalidateRect(hwnd,&rc,FALSE);
      }
        break;
    } 

    case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hdc;//屏幕hdc
      RECT rc;

      //开始绘制
      hdc = BeginPaint(hwnd, &ps); 
      //初始化旋转图标
      if(MusicDialog.Init_State == 0)
      {
        MusicDialog.Init_State = 1;
        BitBlt(hdc_rotate, 0, 0, 240, 240, MusicDialog.hdc_bk, 280, 120, SRCCOPY);
        RotateBitmap(hdc_rotate,120,120,&bm_rotate,0);
      }            


      rc.x=280;
      rc.y=120;
      rc.w=240;
      rc.h=240;

      BitBlt(hdc,rc.x,rc.y,rc.w,rc.h,hdc_rotate,0,0,SRCCOPY);
      EndPaint(hwnd, &ps);
      break;
    }    
    case WM_DRAWITEM:
    {
      DRAWITEM_HDR *ds;
      ds = (DRAWITEM_HDR*)lParam;        
      if(ds->ID == eID_MUSIC_EXIT)
      {
        _music_ExitButton_OwnerDraw(ds);
        return TRUE;
      }
      if (ds->ID == eID_SCROLLBAR_TIMER || ds->ID == eID_SCROLLBAR_POWER)
      {
        _music_scrollbar_ownerdraw(ds);
        return TRUE;
      }
      if (ds->ID >= eID_BUTTON_Power && ds->ID<= eID_BUTTON_START)
      {
        button_owner_draw(ds);
        return TRUE;
      }
      if(ds->ID == eID_ALL_TIME || ds->ID == eID_CUR_TIME || ds->ID == eID_MUSIC_ITEM)
      {
        _music_textbox_OwnerDraw(ds);
        return TRUE;
      } 
//       if(ds->ID == ID_TB1 || ds->ID == ID_TB2 || ds->ID == ID_TB5)
//       {
//          Music_Button_OwnerDraw(ds);
//         return TRUE;
//       }

    }
    
    case WM_NOTIFY:
    {
      u16 code,  id, ctr_id;
      NMHDR *nr;
      id  =LOWORD(wParam);//获取消息的ID码
      code=HIWORD(wParam);//获取消息的类型
      ctr_id = LOWORD(wParam); //wParam低16位是发送该消息的控件ID.  

      if(code == BN_CLICKED)
      { 
        switch(id)
        {
          case eID_BUTTON_Power:
          {
            music_icon[0].state = ~music_icon[0].state;
            //当音量icon未被按下时
            if(music_icon[0].state == FALSE)
            {
              RedrawWindow(hwnd, NULL, RDW_ALLCHILDREN|RDW_INVALIDATE);
              ShowWindow(GetDlgItem(hwnd, eID_SCROLLBAR_POWER), SW_HIDE); //窗口隐藏
            }
            //当音量icon被按下时，设置为静音模式
            else
            {                
              ShowWindow(GetDlgItem(hwnd, eID_SCROLLBAR_POWER), SW_SHOW); //窗口显示
            }
            break;
          }
          case eID_BUTTON_NEXT:
          {     
            WCHAR wbuf[128];
            COLORREF color;
            MusicDialog.playindex++;
            if(MusicDialog.playindex >= MusicDialog.music_file_num) MusicDialog.playindex = 0;
            if(MusicDialog.playindex < 0) MusicDialog.playindex = MusicDialog.music_file_num - 1;
            mp3player.ucStatus = STA_SWITCH;
//            hdc = GetDC(hwnd);
//                
//            color = GetPixel(hdc, 385, 404);  
//            x_mbstowcs_cp936(wbuf, music_lcdlist[play_index], FILE_NAME_LEN);
//            SetWindowText(GetDlgItem(hwnd, ID_TB5), wbuf);
//                 
//            SendMessage(music_wnd_time, SBM_SETVALUE, TRUE, 0); //设置进度值
//            SetWindowText(GetDlgItem(MusicPlayer_hwnd, ID_TB1), L"00:00"); 
//            SetWindowText(GetDlgItem(MusicPlayer_hwnd, ID_TB2), L"00:00"); 
//            //                  DrawText(hdc, wbuf, -1, &rc_musicname, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
//            //                  ClrDisplay(hdc, &rc_MusicTimes, color);
//            //                  DrawText(hdc, L"00:00", -1, &rc_MusicTimes, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
//            ReleaseDC(hwnd, hdc);

            break;
          }
          //上一首icon处理case
          case eID_BUTTON_BACK:
          {

            COLORREF color;
            MusicDialog.playindex--;
            if(MusicDialog.playindex > MusicDialog.music_file_num) MusicDialog.playindex = 0;
            if(MusicDialog.playindex < 0) MusicDialog.playindex = MusicDialog.music_file_num - 1;
            mp3player.ucStatus = STA_SWITCH;   
//            hdc = GetDC(hwnd);
//            color = GetPixel(hdc, 385, 404);
//            //                  x_mbstowcs_cp936(wbuf, music_lcdlist[play_index], FILE_NAME_LEN);
//            //                  SetWindowText(GetDlgItem(hwnd, ID_TB5), wbuf);
//            //                  DrawText(hdc, wbuf, -1, &rc_musicname, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
//            ReleaseDC(hwnd, hdc);            
            break;
          }  
        
        }

      }//end of if(code == BN_CLICKED)   
      if(id==eID_BUTTON_List && code==BN_CLICKED)
      {
        
        WNDCLASS wcex;

        MusicDialog.mList_State = 1;
        wcex.Tag	 		= WNDCLASS_TAG;
        wcex.Style			= CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc	= (WNDPROC)Dlg_List_WinProc;
        wcex.cbClsExtra		= 0;
        wcex.cbWndExtra		= 0;
        wcex.hInstance		= NULL;
        wcex.hIcon			= NULL;
        wcex.hCursor		= NULL;
        if(1)
        {
          RECT rc;

          GetClientRect(hwnd,&rc);

          MusicDialog.LIST_Hwnd = CreateWindowEx(NULL,
                                                    &wcex,L"MusicList",
                                                    WS_OVERLAPPED|WS_VISIBLE,
                                                    0,0,800,480,
                                                    hwnd,0,NULL,NULL);
          
        }
      }
      if(id==eID_BUTTON_LRC && code==BN_CLICKED)
      {
        if(!MusicDialog.mLRC_State)
        {
          MusicDialog.mLRC_State = 1;
          WNDCLASS wcex;


          wcex.Tag	 		= WNDCLASS_TAG;
          wcex.Style			= CS_HREDRAW | CS_VREDRAW;
          wcex.lpfnWndProc	= (WNDPROC)Dlg_LRC_WinProc;
          wcex.cbClsExtra		= 0;
          wcex.cbWndExtra		= 0;
          wcex.hInstance		= NULL;
          wcex.hIcon			= NULL;
          wcex.hCursor		= NULL;
          if(1)
          {
            RECT rc;

            GetClientRect(hwnd,&rc);

            MusicDialog.LRC_Hwnd = CreateWindowEx(NULL,
                                                      &wcex,L"MusicList",
                                                      WS_OVERLAPPED|WS_CLIPCHILDREN|WS_VISIBLE,
                                                      0,80,800,290,
                                                      hwnd,0,NULL,NULL);
          }
        }
        else
        {
          MusicDialog.mLRC_State = 0;
          PostCloseMessage(MusicDialog.LRC_Hwnd);
        }
      }     
      
      
      nr = (NMHDR*)lParam; //lParam参数，是以NMHDR结构体开头.
      if (ctr_id == eID_SCROLLBAR_TIMER)
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
            MusicDialog.chgsch=1;
            break;
          }
            
        }
      }
      nr = (NMHDR*)lParam; //lParam参数，是以NMHDR结构体开头.
      //音量条处理case
      if (ctr_id == eID_SCROLLBAR_POWER)
      {
        NM_SCROLLBAR *sb_nr;
        sb_nr = (NM_SCROLLBAR*)nr; //Scrollbar的通知消息实际为 NM_SCROLLBAR扩展结构,里面附带了更多的信息.
        static int NoVol_flag = 0;
        switch (nr->code)
        {
          case SBN_THUMBTRACK: //R滑块移动
          {
            MusicDialog.power= sb_nr->nTrackValue; //得到当前的音量值
            if(MusicDialog.power == 0) 
            {
              wm8978_OutMute(1);//静音
              SetWindowText(GetDlgItem(hwnd, eID_BUTTON_Power), L"J");
              NoVol_flag = 1;
            }
            else
            {
              if(NoVol_flag == 1)
              {
                NoVol_flag = 0;
                SetWindowText(GetDlgItem(hwnd, eID_BUTTON_Power), L"A");
              }
              wm8978_OutMute(0);
              wm8978_SetOUT1Volume(MusicDialog.power);//设置WM8978的音量值
            } 
            SendMessage(nr->hwndFrom, SBM_SETVALUE, TRUE, MusicDialog.power); //发送SBM_SETVALUE，设置音量值
          }
          break;
        }
      }
      
      break;
    }
    case WM_ERASEBKGND:
    {
      HDC hdc =(HDC)wParam;
      RECT rc =*(RECT*)lParam;
   
      if(MusicDialog.Load_File!=FALSE)
        BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, MusicDialog.hdc_bk, rc.x, rc.y, SRCCOPY);         

      return TRUE;
    }       
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
  return WM_NULL;
}



void	GUI_MUSIC_DIALOG(void)
{
  
	WNDCLASS	wcex;
	MSG msg;

	wcex.Tag = WNDCLASS_TAG;

	wcex.Style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = music_win_proc; //设置主窗口消息处理的回调函数.
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = NULL;//hInst;
	wcex.hIcon = NULL;//LoadIcon(hInstance, (LPCTSTR)IDI_WIN32_APP_TEST);
	wcex.hCursor = NULL;//LoadCursor(NULL, IDC_ARROW);

 
   
	//创建主窗口
	MusicDialog.MUSIC_Hwnd = CreateWindowEx(WS_EX_NOFOCUS|WS_EX_FRAMEBUFFER,
                                            &wcex,
                                            L"MUSIC_DIALOG",
                                            WS_VISIBLE|WS_CLIPCHILDREN|WS_OVERLAPPED,
                                            
                                            0, 0, GUI_XSIZE, GUI_YSIZE,
                                            NULL, NULL, NULL, NULL);
   //显示主窗口
	ShowWindow(MusicDialog.MUSIC_Hwnd, SW_SHOW);
	//开始窗口消息循环(窗口关闭并销毁时,GetMessage将返回FALSE,退出本消息循环)。
	while (GetMessage(&msg, MusicDialog.MUSIC_Hwnd))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

}


void GUI_MUSIC_DIALOGTest(void *param)
{
  static int thread = 0;
  int app = 0;
  
  if(thread == 0)
  {
     GUI_Thread_Create(GUI_MUSIC_DIALOGTest,"MUSIC_DIALOG",12*1024,NULL,5,5);
     thread = 1;
     return;
  }
  if(thread == 1)
  {
		if(app==0)
		{
			app=1;
			GUI_MUSIC_DIALOG();
      
      
			app=0;
			thread=0;
      GUI_Thread_Delete(GUI_GetCurThreadHandle());
		}    
  }
}

