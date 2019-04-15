#include "emXGUI.h"
#include "gui_drv.h"
#include "GUI_CAMERA_DIALOG.h"
#include "./camera/ov5640_AF.h"
#include "./camera/bsp_ov5640.h"
#include "GUI_AppDef.h"
extern BOOL g_dma2d_en ;

static HDC hdc_bk = NULL;//背景图层，绘制透明控件
Cam_DIALOG_Typedef CamDialog;
OV5640_IDTypeDef OV5640_Camera_ID;
static int b_close=FALSE;//窗口关闭标志位
static RECT win_rc;//二级菜单位置信息
/*
 * @brief  自定义参数设置按钮
 * @param  ds:	自定义绘制结构体
 * @retval NONE
*/

static void BtCam_owner_draw(DRAWITEM_HDR *ds) //绘制一个按钮外观
{
	HDC hdc;
	RECT rc;
	WCHAR wbuf[128];

	hdc = ds->hDC;   //button的绘图上下文句柄.
	rc = ds->rc;     //button的绘制矩形区.
  GetWindowText(ds->hwnd, wbuf, 128); //获得按钮控件的文字

	if(ds->State & BST_PUSHED)
	{ //按钮是按下状态
//    GUI_DEBUG("ds->ID=%d,BST_PUSHED",ds->ID);
		SetBrushColor(hdc,MapRGB(hdc,105,105,105)); //设置填充色(BrushColor用于所有Fill类型的绘图函数)
		SetPenColor(hdc,MapRGB(hdc,105,105,105));        //设置绘制色(PenColor用于所有Draw类型的绘图函数)
		SetTextColor(hdc, MapRGB(hdc, 250, 250, 250));      //设置文字色
    
    DrawRoundRect(hdc, &rc, MIN(rc.w,rc.h)>>1);
    InflateRect(&rc, -1, -1);
    DrawRoundRect(hdc, &rc, MIN(rc.w,rc.h)>>1); 
    
    FillRoundRect(hdc, &rc, MIN(rc.w,rc.h)>>1);  
    DrawText(hdc, wbuf, -1, &rc, DT_VCENTER | DT_CENTER);//绘制文字(居中对齐方式) 
    
	}
	else//按钮是弹起状态
	{ 

		SetPenColor(hdc,MapRGB(hdc,250,250,250));
		SetTextColor(hdc, MapRGB(hdc, 250,250,250)); 
    DrawRoundRect(hdc, &rc, MIN(rc.w,rc.h)>>1);
    InflateRect(&rc, -1, -1);
    DrawRoundRect(hdc, &rc, MIN(rc.w,rc.h)>>1);

    DrawText(hdc, wbuf, -1, &rc, DT_VCENTER | DT_CENTER);//绘制文字(居中对齐方式)    
	}
}
static void button_owner_draw(DRAWITEM_HDR *ds) 
{
	//	HWND hwnd;
	HDC hdc;
	RECT rc;
	WCHAR wbuf[128];

	//	hwnd =ds->hwnd; //button的窗口句柄.
	hdc = ds->hDC;   //button的绘图上下文句柄.
	rc = ds->rc;     //button的绘制矩形区.
	SetTextColor(hdc, MapRGB(hdc, 250, 250, 250));
	
  GetWindowText(ds->hwnd, wbuf, 128); //获得按钮控件的文字
  DrawText(hdc, wbuf, -1, &rc, DT_VCENTER | DT_CENTER);//绘制文字(居中对齐方式)

}
/*
 * @brief  自定义自动对焦开关
 * @param  ds:	自定义绘制结构体
 * @retval NONE
*/
static void Checkbox_owner_draw(DRAWITEM_HDR *ds) 
{
	HDC hdc;
	RECT rc;

	hdc = ds->hDC;   //button的绘图上下文句柄.
	rc = ds->rc;     //button的绘制矩形区.
  EnableAntiAlias(hdc, TRUE);
	if (CamDialog.focus_status==1)//按钮是按下状态
	{ 
		SetBrushColor(hdc, MapRGB(hdc, 119, 136, 153)); 
		FillRoundRect(hdc, &rc, rc.h / 2);
		InflateRect(&rc, -3, -3);

		SetBrushColor(hdc, MapRGB(hdc, 0, 250, 0)); 
		FillRoundRect(hdc, &rc, rc.h / 2);

		GetClientRect(ds->hwnd, &rc);
		SetBrushColor(hdc, MapRGB(hdc, 119, 136, 153)); 
		FillCircle(hdc, rc.w - 15, 15, 15);


		SetBrushColor(hdc, MapRGB(hdc, 250, 250, 250)); 
		FillCircle(hdc, rc.w - 15, 15, 12);
	}
	else//按钮是弹起状态
	{ 
		SetBrushColor(hdc, MapRGB(hdc, 119, 136, 153)); 
		FillRoundRect(hdc, &rc, rc.h/2);
		InflateRect(&rc, -3,  -3);

		SetBrushColor(hdc, MapRGB(hdc, 250, 250, 250)); 
		FillRoundRect(hdc, &rc, rc.h / 2); 

		GetClientRect(ds->hwnd, &rc);
		SetBrushColor(hdc, MapRGB(hdc, 119, 136, 153)); //设置填充色(BrushColor用于所有Fill类型的绘图函数)
		FillCircle(hdc, rc.x + 15, 15, 15);//用矩形填充背景
		
		SetBrushColor(hdc, MapRGB(hdc, 250, 250, 250)); //设置填充色(BrushColor用于所有Fill类型的绘图函数)
		FillCircle(hdc, rc.x + 15, 15, 12);

	}
  EnableAntiAlias(hdc, FALSE);
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
  /***************绘制透明控件背景*************************/
  
  GetClientRect(hwnd, &rc_tmp);//得到控件的位置
  GetClientRect(hwnd, &rc);//得到控件的位置
  ClientToScreen(hwnd, (POINT *)&rc_tmp, 1);//坐标转换
  ScreenToClient(CamDialog.SetWIN, (POINT *)&rc_tmp, 1);
  BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);
  
  rc_scrollbar.x = rc.x;
  rc_scrollbar.y = rc.h/2;
  rc_scrollbar.w = rc.w;
  rc_scrollbar.h = 2;
  EnableAntiAlias(hdc, TRUE);
  SetBrushColor(hdc, MapRGB888(hdc, Page_c));
  FillRect(hdc, &rc_scrollbar);

  /* 滑块 */
  SendMessage(hwnd, SBM_GETTRACKRECT, 0, (LPARAM)&rc);

  SetBrushColor(hdc, MapRGB(hdc, 169, 169, 169));
  FillCircle(hdc, rc.x + rc.w / 2, rc.y + rc.h / 2, rc.h / 2);
  InflateRect(&rc, -2, -2);

  SetBrushColor(hdc, MapRGB888(hdc, fore_c));
  FillCircle(hdc, rc.x + rc.w / 2, rc.y + rc.h / 2, rc.h / 2);
  EnableAntiAlias(hdc, FALSE);
}

/*
 * @brief  自定义滑动条绘制函数
 * @param  ds:	自定义绘制结构体
 * @retval NONE
*/
static void Cam_scrollbar_ownerdraw(DRAWITEM_HDR *ds)
{
	HWND hwnd;
	HDC hdc;
	HDC hdc_mem;
	HDC hdc_mem1;
	RECT rc;
	RECT rc_cli;

	hwnd = ds->hwnd;
	hdc = ds->hDC;
	GetClientRect(hwnd, &rc_cli);

	hdc_mem = CreateMemoryDC(SURF_SCREEN, rc_cli.w, rc_cli.h);
	hdc_mem1 = CreateMemoryDC(SURF_SCREEN, rc_cli.w, rc_cli.h);   	
	//绘制白色类型的滚动条
	draw_scrollbar(hwnd, hdc_mem1, RGB888( 250, 250, 250), RGB888( 250, 250, 250), RGB888( 255, 255, 255));
	//绘制绿色类型的滚动条
	draw_scrollbar(hwnd, hdc_mem,RGB888( 250, 250, 250), RGB888( 0, 250, 0), RGB888( 0, 250, 0));
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
		BitBlt(hdc, rc.x, 0, rc.w+1, rc_cli.h, hdc_mem, rc.x, 0, SRCCOPY);
	}
	//释放内存MemoryDC
	DeleteDC(hdc_mem1);
	DeleteDC(hdc_mem);
}
static void home_owner_draw(DRAWITEM_HDR *ds) //绘制一个按钮外观
{
	HWND hwnd;
	HDC hdc;
	RECT rc;
	WCHAR wbuf[128];

	hwnd = ds->hwnd; //button的窗口句柄.
	hdc = ds->hDC;   //button的绘图上下文句柄.
	rc = ds->rc;     //button的绘制矩形区.

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

static void camera_return_ownerdraw(DRAWITEM_HDR *ds) //绘制一个按钮外观
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
/*
 * @brief  自定义按钮（分辨率，光线模式，特殊效果）
 * @param  ds:	自定义绘制结构体
 * @retval NONE
*/
static void Button_owner_draw(DRAWITEM_HDR *ds) //绘制一个按钮外观
{
	HDC hdc;
	RECT rc;
	WCHAR wbuf[128];
  HFONT font_old;

	hdc = ds->hDC;   //button的绘图上下文句柄.
	rc = ds->rc;     //button的绘制矩形区.

  GetWindowText(ds->hwnd, wbuf, 128); //获得按钮控件的文字

  if(ds->ID >= eID_TB1 && ds->ID <= eID_TB3)
  {
    font_old = SetFont(hdc, ctrlFont32);
    rc.x = 160;
    rc.w = 40;
    if(ds->State & BST_PUSHED)//按键被按下变颜色
      SetTextColor(hdc, MapRGB(hdc, 192,192,192));
    else
      SetTextColor(hdc, MapRGB(hdc, 250, 250, 250));
    DrawText(hdc, L"C", -1, &rc, DT_VCENTER);
    SetFont(hdc, font_old);
    rc.x = 5;
    rc.w = 160;
    DrawText(hdc, wbuf, -1, &rc, DT_VCENTER | DT_RIGHT);

  }
  else
  {
    SetTextColor(hdc, MapRGB(hdc, 250, 250, 250));
    DrawText(hdc, wbuf, -1, &rc, DT_VCENTER | DT_LEFT);
  }

}
/*
 *参数：y0--以y0为纵坐标对齐，h---要对齐的控件高度
 *发挥：控件的纵坐标
 */
int Set_VCENTER(int y0, int h)
{
  return y0-h/2;
}

static LRESULT dlg_set_WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  RECT rc;
  static SCROLLINFO sif, sif1, sif2;
  RECT rc_first[2];
  RECT rc_second[2];
  RECT rc_third[2];
  switch(msg)
  {
    case WM_CREATE:
    {
      b_close =FALSE;
			rc.x =5;
			rc.y =50;
			rc.w =400;
			rc.h =50;    


      MakeMatrixRect(rc_first, &rc, 5, 0, 2, 1);
      CreateWindow(BUTTON,L"自动对焦",WS_OWNERDRAW|WS_TRANSPARENT|WS_VISIBLE,
                   rc_first[0].x,rc_first[0].y,rc_first[0].w,rc_first[0].h,hwnd,eID_SET1,NULL,NULL); 
      rc_first[1].y = Set_VCENTER(rc_first[0].y+rc_first[0].h/2,30);
      CreateWindow(BUTTON,L" ",WS_OWNERDRAW|WS_TRANSPARENT|WS_VISIBLE,
                   rc.w-70,rc_first[1].y,60,30,hwnd,eID_switch,NULL,NULL);   

			OffsetRect(&rc,0,rc.h);
      MakeMatrixRect(rc_second, &rc, 5, 0, 2, 1);
      CreateWindow(BUTTON,L"亮度",WS_OWNERDRAW|WS_TRANSPARENT|WS_VISIBLE,
                   rc_second[0].x,rc_second[0].y,rc_second[0].w,rc_second[0].h,hwnd,eID_SET2,NULL,NULL);
      sif.cbSize = sizeof(sif);
      sif.fMask = SIF_ALL;
      sif.nMin = -2;
      sif.nMax = 2;
      sif.nValue = 0;//cam_mode.brightness;
      sif.TrackSize = 31;//滑块值
      sif.ArrowSize = 0;//两端宽度为0（水平滑动条）
      rc_second[1].y = Set_VCENTER(rc_second[0].y+rc_second[0].h/2,31);
      CreateWindow(SCROLLBAR, L"SCROLLBAR_liangdu", WS_OWNERDRAW|WS_VISIBLE, 
                   rc_second[1].x,rc_second[1].y,180,31, hwnd, eID_SCROLLBAR, NULL, NULL);                 
      SendMessage(GetDlgItem(hwnd, eID_SCROLLBAR), SBM_SETSCROLLINFO, TRUE, (LPARAM)&sif);       

			OffsetRect(&rc,0,rc.h);
      CreateWindow(BUTTON,L"饱和度",WS_OWNERDRAW|WS_TRANSPARENT|WS_VISIBLE,rc.x,rc.y,rc.w,rc.h,hwnd,eID_SET3,NULL,NULL);
      MakeMatrixRect(rc_third, &rc, 5, 0, 2, 1);

      sif1.cbSize = sizeof(sif1);
      sif1.fMask = SIF_ALL;
      sif1.nMin = -3;
      sif1.nMax = 3;
      sif1.nValue = 0;//cam_mode.saturation;
      sif1.TrackSize = 31;//滑块值
      sif1.ArrowSize = 0;//两端宽度为0（水平滑动条）
      rc_third[1].y = Set_VCENTER(rc_third[0].y+rc_third[0].h/2,31);
      CreateWindow(SCROLLBAR, L"SCROLLBAR_R", WS_OWNERDRAW|WS_VISIBLE, 
                   rc_third[1].x,rc_third[1].y,180,31, hwnd, eID_SCROLLBAR1, NULL, NULL);
      SendMessage(GetDlgItem(hwnd, eID_SCROLLBAR1), SBM_SETSCROLLINFO, TRUE, (LPARAM)&sif1);

      OffsetRect(&rc,0,rc.h);
      CreateWindow(BUTTON,L"对比度",WS_OWNERDRAW|WS_TRANSPARENT|WS_VISIBLE,rc.x,rc.y,rc.w,rc.h,hwnd,eID_SET4,NULL,NULL);

      MakeMatrixRect(rc_third, &rc, 5, 0, 2, 1);
      sif2.cbSize = sizeof(sif2);
      sif2.fMask = SIF_ALL;
      sif2.nMin = -3;
      sif2.nMax = 3;
      sif2.nValue = 0;//cam_mode.contrast;
      sif2.TrackSize = 31;//滑块值
      sif2.ArrowSize = 0;//两端宽度为0（水平滑动条）
      rc_third[1].y = Set_VCENTER(rc_third[0].y+rc_third[0].h/2,31);
      CreateWindow(SCROLLBAR, L"SCROLLBAR_R", WS_OWNERDRAW|WS_VISIBLE, 
                   rc_third[1].x,rc_third[1].y, 180, 31, hwnd, eID_SCROLLBAR2, NULL, NULL);
      SendMessage(GetDlgItem(hwnd, eID_SCROLLBAR2), SBM_SETSCROLLINFO, TRUE, (LPARAM)&sif2);



      OffsetRect(&rc,0,rc.h);
      CreateWindow(BUTTON,L"分辨率",WS_OWNERDRAW|WS_TRANSPARENT|WS_VISIBLE,rc.x,rc.y,rc.w,rc.h,hwnd,eID_SET5,NULL,NULL);
      MakeMatrixRect(rc_third, &rc, 0, 0, 2, 1);
      rc_third[1].y = Set_VCENTER(rc_third[0].y+rc_third[0].h/2,30);
      CreateWindow(BUTTON,L"800*480(默认)",WS_OWNERDRAW|WS_TRANSPARENT|WS_VISIBLE,
                   rc_third[1].x,rc_third[1].y,200,30,hwnd,eID_TB1,NULL,NULL); 

      switch(CamDialog.cur_Resolution)
      {
        case eID_RB1:
          SetWindowText(GetDlgItem(hwnd, eID_TB1), L"320*240");break;                 
        case eID_RB2:
          SetWindowText(GetDlgItem(hwnd, eID_TB1), L"480*272");break;
        case eID_RB3:
          SetWindowText(GetDlgItem(hwnd, eID_TB1), L"800*480(默认)");break;
      }   
      OffsetRect(&rc,0,rc.h);
      CreateWindow(BUTTON,L"光线模式",WS_OWNERDRAW|WS_TRANSPARENT|WS_VISIBLE,rc.x,rc.y,rc.w,rc.h,hwnd,eID_SET6,NULL,NULL);
      MakeMatrixRect(rc_third, &rc, 0, 0, 2, 1);
      rc_third[1].y = Set_VCENTER(rc_third[0].y+rc_third[0].h/2,40);
      CreateWindow(BUTTON,L"自动(默认)",WS_OWNERDRAW|WS_TRANSPARENT|WS_VISIBLE,
      rc_third[1].x,rc_third[1].y,200,40,hwnd,eID_TB2,NULL,NULL);                      

      switch(CamDialog.cur_LightMode)
      {
        case eID_RB4:
          SetWindowText(GetDlgItem(hwnd, eID_TB2), L"自动(默认)");break;                 
        case eID_RB5:
          SetWindowText(GetDlgItem(hwnd, eID_TB2), L"光照");break;
        case eID_RB6:
          SetWindowText(GetDlgItem(hwnd, eID_TB2), L"阴天");break;
        case eID_RB7:
          SetWindowText(GetDlgItem(hwnd, eID_TB2), L"办公室");break;
        case eID_RB8:
          SetWindowText(GetDlgItem(hwnd, eID_TB2), L"家里");break;
      }  
      OffsetRect(&rc,0,rc.h);
      CreateWindow(BUTTON,L"特殊效果",WS_OWNERDRAW|WS_TRANSPARENT|WS_VISIBLE,rc.x,rc.y,rc.w,rc.h,hwnd,eID_SET7,NULL,NULL);

      MakeMatrixRect(rc_third, &rc, 0, 0, 2, 1);
      rc_third[1].y = Set_VCENTER(rc_third[0].y+rc_third[0].h/2,30);         
      CreateWindow(BUTTON,L"正常(默认)",WS_OWNERDRAW|WS_TRANSPARENT|WS_VISIBLE,
      rc_third[1].x,rc_third[1].y,200,30,hwnd,eID_TB3,NULL,NULL);                      
      switch(CamDialog.cur_SpecialEffects)
      {
        case eID_RB9:
          SetWindowText(GetDlgItem(hwnd, eID_TB3), L"冷色");break;                 
        case eID_RB10:
          SetWindowText(GetDlgItem(hwnd, eID_TB3), L"暖色");break;
        case eID_RB11:
          SetWindowText(GetDlgItem(hwnd, eID_TB3), L"黑白");break;
        case eID_RB12:
          SetWindowText(GetDlgItem(hwnd, eID_TB3), L"泛黄");break;
        case eID_RB13:
          SetWindowText(GetDlgItem(hwnd, eID_TB3), L"反色");break;
        case eID_RB14:
          SetWindowText(GetDlgItem(hwnd, eID_TB3), L"偏绿");break;
        case eID_RB15:
          SetWindowText(GetDlgItem(hwnd, eID_TB3), L"过曝");break;
        case eID_RB16:
          SetWindowText(GetDlgItem(hwnd, eID_TB3), L"正常(默认)");break;            
      }        
      CreateWindow(BUTTON, L"F", BS_FLAT | BS_NOTIFY|WS_TRANSPARENT|WS_OWNERDRAW |WS_VISIBLE,
                    0, 0, 90, 50, hwnd, eID_RETURN, NULL, NULL);       
      SetTimer(hwnd,2,20,TMR_START,NULL);
      GetClientRect(hwnd, &rc);
      hdc_bk = CreateMemoryDC(SURF_SCREEN, rc.w, rc.h);
      break;
    }
		case WM_TIMER://实现窗口下落的效果
		{
			switch(wParam)
			{
				case 1:
				{
          break;
				}
				case 2:
				{
					if(GetKeyState(VK_LBUTTON)!=0)
					{
						break;
					}
					GetWindowRect(hwnd,&rc);

					if(b_close==FALSE)
					{
						if(rc.y < win_rc.y )
						{
							if((win_rc.y-rc.y)>50)
							{
								rc.y +=30;
							}
							if((win_rc.y-rc.y)>30)
							{
								rc.y +=20;
							}
							else
							{
								rc.y +=4;
							}
							ScreenToClient(GetParent(hwnd),(POINT*)&rc.x,1);
                     
							MoveWindow(hwnd,rc.x,rc.y,rc.w,rc.h,TRUE);
						}
					}
					else
					{
						if(rc.y > -(rc.h))
						{
							rc.y -= 40;

							ScreenToClient(GetParent(hwnd),(POINT*)&rc.x,1);
                     
							MoveWindow(hwnd,rc.x,rc.y,rc.w,rc.h,TRUE);
						}
						else
						{
							PostCloseMessage(hwnd);
						}
					}


				}
				break;
			}
      break;
		}
		case	WM_DRAWITEM:
		{
      DRAWITEM_HDR *ds;
      ds = (DRAWITEM_HDR*)lParam;
      if (ds->ID == eID_switch)
      {
        Checkbox_owner_draw(ds); //执行自绘制按钮
      }
      if (ds->ID == eID_SCROLLBAR || ds->ID == eID_SCROLLBAR1 || ds->ID == eID_SCROLLBAR2)
      {
        Cam_scrollbar_ownerdraw(ds);
        return TRUE;
      }         
      if ((ds->ID >= eID_SET1 && ds->ID <= eID_SET7) || (ds->ID >= eID_TB1 && ds->ID <= eID_TB3)||
          (ds->ID >= eID_Setting1 && ds->ID <= eID_Setting3))
      {
        Button_owner_draw(ds); //执行自绘制按钮
      }
      if(ds->ID == eID_RETURN)  
      {
        camera_return_ownerdraw(ds);
      }

      return TRUE;
		}    
		case WM_PAINT: //窗口需要绘制时，会自动产生该消息.
		{
			PAINTSTRUCT ps;
			HDC hdc;
			RECT rc;
			hdc =BeginPaint(hwnd,&ps); //开始绘图
      GetClientRect(hwnd, &rc);

      rc.h = 50;
      SetBrushColor(hdc_bk,MapRGB(hdc_bk,0,0,0));
      FillRect(hdc_bk, &rc);
      GetClientRect(hwnd, &rc);
      SetBrushColor(hdc_bk,MapRGB(hdc_bk,105,105,105));
      rc.y = 50;
      rc.h = rc.h-50;
      FillRect(hdc_bk, &rc);         
      SetTextColor(hdc_bk, MapRGB(hdc_bk,250,250,250));

      rc.x =100;
      rc.y =0;
      rc.w =200; 
      rc.h =50;

      DrawText(hdc_bk,L"参数设置",-1,&rc,DT_CENTER|DT_VCENTER); 
      SetPenColor(hdc_bk, MapRGB(hdc_bk, 245,245,245));
      GetClientRect(hwnd, &rc);
      //间隔线
      HLine(hdc_bk, 0, 50, 400);
      HLine(hdc_bk, 0, 100, 400);
      HLine(hdc_bk, 0, 150, 400);
      HLine(hdc_bk, 0, 200, 400);
      HLine(hdc_bk, 0, 250, 400);
      HLine(hdc_bk, 0, 300, 400);
      HLine(hdc_bk, 0, 350, 400);
      BitBlt(hdc, 0,0,rc.w, rc.h, hdc_bk,0,0,SRCCOPY);        
      EndPaint(hwnd,&ps); //结束绘图
      break;
		}    
    default:
      return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return WM_NULL;
}

static LRESULT Cam_win_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch(msg)
  {
    case WM_CREATE:
    {
      RECT rc;
      /* 初始化摄像头GPIO及IIC */
//      OV5640_HW_Init();  
//      /* 读取摄像头芯片ID，确定摄像头正常连接 */
//      OV5640_ReadID(&OV5640_Camera_ID);

//      if(OV5640_Camera_ID.PIDH  == 0x56)
//      {
//        GUI_DEBUG("OV5640 ID:%x %x",OV5640_Camera_ID.PIDH ,OV5640_Camera_ID.PIDL);
//      }
//      else
//      {
//        MSGBOX_OPTIONS ops;
//        //const WCHAR *btn[]={L"确定"};
//        int x,y,w,h;

//        ops.Flag =MB_ICONERROR;
//        //ops.pButtonText =btn;
//        ops.ButtonCount =0;
//        w =500;
//        h =200;
//        x =(GUI_XSIZE-w)>>1;
//        y =(GUI_YSIZE-h)>>1;
//        MessageBox(hwnd,x,y,w,h,L"没有检测到OV5640摄像头，\n请重新检查连接。",L"消息",&ops); 
//        break;  
//      }
      GetClientRect(hwnd, &rc);
      //设置按键
      CreateWindow(BUTTON,L"参数设置",WS_OWNERDRAW|WS_TRANSPARENT|WS_VISIBLE,rc.w-135,419,120,40,hwnd,eID_SET,NULL,NULL);
      //退出按键
      CreateWindow(BUTTON, L"O",WS_OWNERDRAW|WS_TRANSPARENT|WS_VISIBLE,730, 0, 70, 70, hwnd, eID_EXIT, NULL, NULL); 
      //帧率
      CreateWindow(BUTTON,L" ",WS_OWNERDRAW|WS_TRANSPARENT|WS_VISIBLE,rc.w-600,400,400,72,hwnd,eID_FPS,NULL,NULL);
    } 
    case	WM_DRAWITEM:
    {
      DRAWITEM_HDR *ds;
      ds = (DRAWITEM_HDR*)lParam;
      switch(ds->ID)
      {
        case eID_SET:
        {
          BtCam_owner_draw(ds); //执行自绘制按钮
          return TRUE;          
        }
        case eID_EXIT:
        {
          home_owner_draw(ds); 
          return TRUE;          
        }
        case eID_FPS:
        {
          button_owner_draw(ds);
          return TRUE;          
        }
        
      }
    }
     case WM_NOTIFY: //WM_NOTIFY消息:wParam低16位为发送该消息的控件ID,高16位为通知码;lParam指向了一个NMHDR结构体.
    {
      u16 code,id;
      static int flag = 0;//设置窗口是否弹出
      code =HIWORD(wParam); //获得通知码类型.
      id   =LOWORD(wParam); //获得产生该消息的控件ID.
      if(flag == 0)//设置窗口未存在，则创建
      {
        if(id==eID_SET && code==BN_CLICKED)
        {
          flag = 1;
          WNDCLASS wcex;

          GUI_DEBUG("C");
          wcex.Tag	 		= WNDCLASS_TAG;
          wcex.Style			= CS_HREDRAW | CS_VREDRAW;
          wcex.lpfnWndProc	= (WNDPROC)dlg_set_WinProc;
          wcex.cbClsExtra		= 0;
          wcex.cbWndExtra		= 0;
          wcex.hInstance		= NULL;
          wcex.hIcon			= NULL;
          wcex.hCursor		= NULL;

          if(1)
          {
            RECT rc;

            GetClientRect(hwnd,&rc);
           

            win_rc.w =400;
            win_rc.h =400;

            win_rc.x = rc.x+(rc.w-win_rc.w)/2;
            win_rc.y = rc.y;//rc.y+(rc.h>>2);

            CamDialog.SetWIN = CreateWindowEx(
                                              NULL,
                                              &wcex,L"Set",
                                              WS_OVERLAPPED|WS_CLIPCHILDREN|WS_VISIBLE,

                                              win_rc.x,-win_rc.h-4,win_rc.w,win_rc.h,
                                              hwnd,0,NULL,NULL);
          }

        }
      }
      else//设置窗口已经存在，再次点击设置按钮，则关闭窗口
      {
        flag = 0;
        PostCloseMessage(CamDialog.SetWIN);

      }

      if(id==eID_EXIT && code==BN_CLICKED)//退出窗口
      {
        PostCloseMessage(hwnd);
      }
      break;  
    }   
    default:
      return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return WM_NULL;
}

void	GUI_Camera_DIALOG(void)
{	
	WNDCLASS	wcex;
	MSG msg;

  g_dma2d_en = FALSE;
	wcex.Tag = WNDCLASS_TAG;

	wcex.Style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = Cam_win_proc; //设置主窗口消息处理的回调函数.
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = NULL;//hInst;
	wcex.hIcon = NULL;//LoadIcon(hInstance, (LPCTSTR)IDI_WIN32_APP_TEST);
	wcex.hCursor = NULL;//LoadCursor(NULL, IDC_ARROW);

	//创建主窗口
	CamDialog.Cam_Hwnd = CreateWindowEx(WS_EX_NOFOCUS|WS_EX_FRAMEBUFFER,
                                    &wcex,
                                    L"GUI_Camera_Dialog",
                                    WS_VISIBLE|WS_CLIPCHILDREN|WS_OVERLAPPED,
                                    0, 0, GUI_XSIZE, GUI_YSIZE,
									NULL, NULL, NULL, NULL);

	//显示主窗口
	ShowWindow(CamDialog.Cam_Hwnd, SW_SHOW);

	//开始窗口消息循环(窗口关闭并销毁时,GetMessage将返回FALSE,退出本消息循环)。
	while (GetMessage(&msg, CamDialog.Cam_Hwnd))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
  }
}

void GUI_Camera_DIALOGTest(void *param)
{
  static int thread = 0;
  int app = 0;
  
  if(thread == 0)
  {
     GUI_Thread_Create(GUI_Camera_DIALOGTest,"Camera_DIALOG",5*1024,NULL,5,3);
     thread = 1;
     return;
  }
  if(thread == 1)
  {
		if(app==0)
		{
			app=1;
			GUI_Camera_DIALOG();
      
      
			app=0;
			thread=0;
      GUI_Thread_Delete(GUI_GetCurThreadHandle());
		}    
  }
}


