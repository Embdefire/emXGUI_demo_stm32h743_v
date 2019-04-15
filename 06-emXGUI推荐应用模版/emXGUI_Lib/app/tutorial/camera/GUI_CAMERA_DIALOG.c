#include "emXGUI.h"
#include "gui_drv.h"
#include "GUI_CAMERA_DIALOG.h"
extern BOOL g_dma2d_en ;


Cam_DIALOG_Typedef CamDialog;


static LRESULT CAM_win_procproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch(msg)
  {
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
	wcex.lpfnWndProc = CAM_win_procproc; //������������Ϣ����Ļص�����.
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = NULL;//hInst;
	wcex.hIcon = NULL;//LoadIcon(hInstance, (LPCTSTR)IDI_WIN32_APP_TEST);
	wcex.hCursor = NULL;//LoadCursor(NULL, IDC_ARROW);

	//����������
	CamDialog.Cam_Hwnd = CreateWindowEx(WS_EX_NOFOCUS|WS_EX_FRAMEBUFFER,
                                    &wcex,
                                    L"GUI_Camera_Dialog",
                                    WS_VISIBLE|WS_CLIPCHILDREN|WS_OVERLAPPED,
                                    0, 0, GUI_XSIZE, GUI_YSIZE,
									NULL, NULL, NULL, NULL);

	//��ʾ������
	ShowWindow(CamDialog.Cam_Hwnd, SW_SHOW);

	//��ʼ������Ϣѭ��(���ڹرղ�����ʱ,GetMessage������FALSE,�˳�����Ϣѭ��)��
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


