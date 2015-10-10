#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<windows.h>
#include<mmsystem.h>

#include "resource.h"

#pragma comment(lib, "winmm.lib")

#define random(n) (rand()%n) //랜덤 사용자 정의 함수... 0~n-1까지의 랜덤한 수를 만듬~
#define randomize() srand((unsigned)time(NULL))

// 시간게이지 33으로 우선 정함...
char* ProgNameKor="지뢰찾기 세트메뉴"; // 프로그램 타이틀 한글제목~
char* ProgNameEng="Mine Sweeper Set Menu"; // 프로그램 타이틀 영어제목~

bool korean=true,replay=false,gameover=true,clear=false,pause=false; // korean 언어변수, replay 리플레이 보기상태, gameover 게임오버상태, clear 게임깼나 안깻나, pause 게임정지상태
bool act_DC=false,act_Bit=false,act_VBit=false;

int x=8,y=8,type=1,level=1,mine=10,user_mine,tile_num=0,Play_Sec=0;
/* x,y 지뢰 가로세로 크기, type 지뢰찾기 형태, level 지뢰찾기 난이도, mine 지뢰갯수...
user_mine 사용자가 설치할 지뢰갯수, tile_num 클리어조건 계산위한 변수 Play_Sec 게임 플레이 시간(초단위)*/
float best[4]={0.0,0.0,0.0,0.0}; // 단계별 최고기록...
char bestname[4][21]; // 단계별 최고기록 이름...
char** span; // 실제 지뢰찾기 보드가 저장될 공간~

char info[188];
char debug[256];
unsigned long Play_Time=0;
MMRESULT timer1;

HINSTANCE g_inst;
HMENU hMenu;
HDC hDC,hMemDC,hBitDC,hPauseDC;
HBITMAP hBitmap, hTL, hBoard, hIcons, hOldBit, hOld, hPause;
HWND hWnd;

BOOL CALLBACK MineDlgProc(HWND hDlg,UINT iMessage,WPARAM wParam, LPARAM lParam); // 사용자 정의(지뢰찾기 난이도) 다이얼로그
BOOL CALLBACK CreditDlgProc(HWND hDlg,UINT iMessage,WPARAM wParam,LPARAM lParam); // 제작자 정보 다이얼로그
BOOL CALLBACK FirstDlgProc(HWND hDlg,UINT iMessage,WPARAM wParam,LPARAM lParam); // 최고기록 물어보기 다이얼로그
BOOL CALLBACK RecordDlgProc(HWND hDlg,UINT iMessage,WPARAM wParam,LPARAM lParam); // 최고기록 다이얼로그
void CALLBACK PlayTimeProc(UINT uID,UINT uMsg,DWORD dwUser,DWORD dw1,DWORD dw2); // 1/100초 단위로 플레이 시간을 표시... 리플레이용+1초마다 시간표시용으로도 쓴다...
LRESULT CALLBACK WndProc(HWND hWnd,UINT iMessage,WPARAM wParam,LPARAM lParam);
void Control_File(char* filename,char contents[],DWORD mode,int bytes);
void ReadData();
void WriteData();
void Menu(); // 메뉴 한영 전환을 위한 함수...
void Check(bool type_menu,int number); // 메뉴 체크 상태를 정의~
void Make_mine(); // 지뢰찾기 보드를 만듬...
void Kill_mine();
void draw_right(); // 지뢰찾기 시간, 남은 갯수 표시...
void draw_all(bool board); // 지뢰찾기 보드를 그림...
void draw_board(int cx,int cy,int num); // 지뢰찾기 보드 안에 숫자, 지뢰등을 표시~
void cacul_board(int bx,int by,bool left,bool right); // 지뢰찾기 보드의 숫자, 지뢰 등을 마우스 버튼에 대한 계산(반응)
void make_DCs(bool vir);
void kill_DCs();
void Select_Bitmap(HDC dc,HGDIOBJ ob,bool act);
void Pause_Screen();
void Unpause();
//void Debug_Print(char *where,char *what,HGDIOBJ ob);

void ReadData(){
	char imsi[5];
	Control_File("window.dat",info,GENERIC_READ,188);
	if(atoi(strncpy(imsi,info+24,1))!=0) level = atoi(strncpy(imsi,info+24,1));
	if(atoi(strncpy(imsi,info+33,1))!=0) type = atoi(strncpy(imsi,info+33,1));
	if(atoi(strncpy(imsi,info+4,3))!=0) x = atoi(strncpy(imsi,info+4,3));
	if(atoi(strncpy(imsi,info+12,3))!=0) y = atoi(strncpy(imsi,info+12,3));
	if(atoi(strncpy(imsi,info+42,5))!=0) mine = atoi(strncpy(imsi,info+42,5));
	for(int i=0;i<4;i++){
		strncpy(bestname[i],info+52+35*i,20);
		bestname[i][20]='\0';
		best[i] = (float)atof(strncpy(imsi,info+74+35*i,8));
	}
}

void WriteData(){
	sprintf(info,"x = %3d\ny = %3d\nlevel = %1d\ntype = %1d\nmine = %5d\nb = %20s, %8.2f\nm = %20s, %8.2f\np = %20s, %8.2f\nf = %20s, %8.2f",
		x,y,level,type,mine,bestname[0],best[0],bestname[1],best[1],bestname[2],best[2],bestname[3],best[3]);
	Control_File("window.dat",info,GENERIC_WRITE,188);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst,LPSTR lpszCmdParam,int nCmdShow)
{
	WNDCLASSEX WndClass;
	MSG msg; 
	
	g_inst=hInstance;
	int msize=GetSystemMetrics(SM_CYCAPTION)+GetSystemMetrics(SM_CYMENU);
	int ysize;
		
	WndClass.cbSize=sizeof(WNDCLASSEX);
	WndClass.cbClsExtra=0;
	WndClass.cbWndExtra=0;
	WndClass.hbrBackground=(HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.hCursor=LoadCursor(NULL,IDC_ARROW);
	WndClass.hIcon=LoadIcon(hInstance,MAKEINTRESOURCE(IDI_ICON1));
	WndClass.hInstance=hInstance;
	WndClass.lpfnWndProc=(WNDPROC)WndProc;
	WndClass.lpszClassName=ProgNameKor;
	WndClass.lpszMenuName=NULL;
	WndClass.style=CS_HREDRAW|CS_VREDRAW;
	WndClass.hIconSm=(HICON)LoadImage(hInstance,MAKEINTRESOURCE(IDI_ICON1),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_DEFAULTCOLOR);
	
	RegisterClassEx(&WndClass);
	
	ReadData();

	if(korean && x<10) ysize = y*16+msize+90; else ysize = y*16+msize+74;
	
	hWnd = CreateWindow(ProgNameKor,ProgNameKor,WS_CAPTION|WS_MINIMIZEBOX|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,x*16+29,ysize,NULL,(HMENU)LoadMenu(g_inst,MAKEINTRESOURCE((korean)?KOREAN:ENGLISH)),hInstance,NULL);
	ShowWindow(hWnd,nCmdShow);
	
	hTL = LoadBitmap(g_inst,MAKEINTRESOURCE(Tl));
	hBoard = LoadBitmap(g_inst,MAKEINTRESOURCE(Mine_Board));
	hIcons = LoadBitmap(g_inst,MAKEINTRESOURCE(Icons));
	
	CreateMutex(NULL, TRUE, ProgNameKor);
	if(GetLastError() == ERROR_ALREADY_EXISTS)
	{
		// MessageBox("프로그램이 이미 실행중입니다.", MB_ICONSTOP);
		return FALSE;
	}
	
	draw_all(true);
	Menu();
	
	while(GetMessage(&msg,NULL,NULL,NULL)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	return msg.wParam;
}

BOOL CALLBACK MineDlgProc(HWND hDlg,UINT iMessage,WPARAM wParam, LPARAM lParam){
	int minex=GetSystemMetrics(SM_CXSCREEN)-28;
	int miney=GetSystemMetrics(SM_CYSCREEN)-74;
	int Dialogx,Dialogy,Dialogmine;
	
	switch(iMessage){
	case WM_INITDIALOG:
		SetDlgItemInt(hDlg,WIDTH,x,false);
		SetDlgItemInt(hDlg,HEIGHT,y,false);
		SetDlgItemInt(hDlg,MINES,mine,false);
		return TRUE;
	case WM_COMMAND:
		switch(wParam){
		case IDOK:
			Kill_mine();
			minex=(minex/16);
			miney=miney-GetSystemMetrics(SM_CYCAPTION)-GetSystemMetrics(SM_CYMENU);
			miney=(miney/16);
			
			Dialogx=GetDlgItemInt(hDlg,WIDTH,NULL,FALSE);
			Dialogy=GetDlgItemInt(hDlg,HEIGHT,NULL,FALSE);
			Dialogmine=GetDlgItemInt(hDlg,MINES,NULL,FALSE);
			if(Dialogx >= minex) x=minex; else x=GetDlgItemInt(hDlg,WIDTH,NULL,false);
			if(Dialogy >= miney) y=miney; else y=GetDlgItemInt(hDlg,HEIGHT,NULL,false);
			if(Dialogx <= 8) x=8;
			if(Dialogy <= 8) y=8;
			if(Dialogmine >= (int)(x*y*0.9)) mine=(int)(x*y*0.9); 
			else if(Dialogmine <= (int)(x*y*0.1)) mine=(int)(x*y*0.1);
			else mine=Dialogmine;
			if(Dialogmine > 999) mine=999;
			EndDialog(hDlg,IDOK);
			return TRUE;
		case IDCANCEL: EndDialog(hDlg,0); return TRUE;
		}
		break;
	}
	return FALSE;
}

BOOL CALLBACK CreditDlgProc(HWND hDlg,UINT iMessage,WPARAM wParam,LPARAM lParam){
	switch(iMessage){
	case WM_INITDIALOG: return TRUE;
	case WM_COMMAND:
		switch(wParam){
		case IDOK: case IDCANCEL: EndDialog(hDlg,IDOK); return TRUE; 
		}break;
	} return FALSE;
}

BOOL CALLBACK FirstDlgProc(HWND hDlg,UINT iMessage,WPARAM wParam,LPARAM lParam){
	char text[128],lv[10],form[10];
	switch(iMessage){
	case WM_INITDIALOG:
		if(korean){
			switch(level){
			case 1: strcpy(lv,"초급"); break;
			case 2: strcpy(lv,"중급"); break;
			case 3: strcpy(lv,"고급"); break;
			case 4: strcpy(lv,"전체화면"); break;
			}
			switch(type){
			case 1: strcpy(form,"사각형"); break;
			case 2: strcpy(form,"육각형"); break;
			}
		}else{
			switch(level){
			case 1: strcpy(lv,"Beginner"); break;
			case 2: strcpy(lv,"Medium"); break;
			case 3: strcpy(lv,"Professional"); break;
			case 4: strcpy(lv,"Full Mode"); break;
			}
			switch(type){
			case 1: strcpy(form,"Rectangles"); break;
			case 2: strcpy(form,"Hexagons"); break;
			}
		}
		if(korean) sprintf(text,"%s, %s에서",lv,form);
		else sprintf(text,"In %s and On %s,",lv,form);
		SetDlgItemText(hDlg,MODE,text);
		if(korean) sprintf(text,"최고기록:%5.2f",Play_Time/100.0);
		else sprintf(text,"Best Record:%5.2f",Play_Time/100.0);
		SetDlgItemText(hDlg,RECORD,text);
		return TRUE;
	case WM_COMMAND:
		switch(wParam){
		case IDOK:
			GetDlgItemText(hDlg,REG,bestname[level-1],20);
			best[level-1] = Play_Time/100.0f;
			WriteData();
			EndDialog(hDlg,IDOK); return TRUE; 
		case IDCANCEL: EndDialog(hDlg,IDCANCEL); return TRUE; 
		}
	} return FALSE;
}

BOOL CALLBACK RecordDlgProc(HWND hDlg,UINT iMessage,WPARAM wParam,LPARAM lParam){
	char temp[10];
	switch(iMessage){
	case WM_INITDIALOG:
		SetDlgItemText(hDlg,BEGIN_NAME,bestname[0]);
		SetDlgItemText(hDlg,MEDIUM_NAME,bestname[1]);
		SetDlgItemText(hDlg,PRO_NAME,bestname[2]);
		SetDlgItemText(hDlg,FULL_NAME,bestname[3]);
		sprintf(temp,"%5.2f",best[0]);		SetDlgItemText(hDlg,BEGIN_TIMES,temp);
		sprintf(temp,"%5.2f",best[1]);		SetDlgItemText(hDlg,MEDIUM_TIMES,temp);
		sprintf(temp,"%5.2f",best[2]);		SetDlgItemText(hDlg,PRO_TIMES,temp);
		sprintf(temp,"%5.2f",best[3]);		SetDlgItemText(hDlg,FULL_TIMES,temp);
		return TRUE;
	case WM_COMMAND:
		switch(wParam){
		case IDOK: case IDCANCEL: EndDialog(hDlg,IDOK); return TRUE; 
		}break;
	} return FALSE;
}

void CALLBACK PlayTimeProc(UINT uID,UINT uMsg,DWORD dwUser,DWORD dw1,DWORD dw2){
	if(!pause) Play_Time++;
	Play_Sec = Play_Time/100;
	if(x>10){ if(Play_Sec>=999) Play_Sec=999; }
	else{ if(Play_Sec>=99) Play_Sec=99; }

	if(Play_Time%100==0){
		if(!act_VBit) draw_right();
		InvalidateRect(hWnd,false,NULL);
	} 
}

LRESULT CALLBACK WndProc(HWND hWnd,UINT iMessage,WPARAM wParam,LPARAM lParam)
{
	static int mousex,mousey; // 마우스 좌표
	static bool left=false,right=false; // 마우스 왼쪽 오른쪽 버튼
	PAINTSTRUCT ps;
	int msize=GetSystemMetrics(SM_CYCAPTION)+GetSystemMetrics(SM_CYMENU),ysize;
	
	switch(iMessage)
	{
	case WM_CREATE:
		Make_mine();
		return 0;
	case WM_PAINT:
		hDC = BeginPaint(hWnd,&ps);
		if(!pause){
			make_DCs(false);
		
			if(korean && x<16 || !korean && x<13) ysize = y*16+msize+94; else ysize = y*16+msize+80;
			if(hBitmap==NULL) hBitmap = CreateCompatibleBitmap(hDC,x*16+32,ysize);
		
			if(!act_VBit){ hOldBit = (HBITMAP)SelectObject(hBitDC,hBitmap); act_VBit=true; }
			//Debug_Print("WM_PAINT","hOldBit",hOldBit);
			BitBlt(hDC,0,0,x*16+28,ysize,hBitDC,0,0,SRCCOPY);
			if(act_VBit){ SelectObject(hBitDC,hOldBit); act_VBit=false; }
		
			kill_DCs();
		}
		if(pause) Pause_Screen();
		EndPaint(hWnd,&ps);
		return 0;
	case WM_SIZE:
		if(wParam==SIZE_MINIMIZED) pause=true;
		else{ pause=false; InvalidateRect(hWnd,NULL,true); }
		return 0;
	case WM_RBUTTONDOWN: right=true; return 0;
	case WM_RBUTTONUP:
		mousex=LOWORD(lParam); mousey=HIWORD(lParam);
		
		if(mousex>=11 && mousey>=55){
			if(korean && x<16 || !korean && x<13) ysize = y*16+msize+94; else ysize = y*16+msize+80;
			cacul_board((mousex-11)/16,(mousey-55)/16,left,right);
		}
		
		right=false; draw_right(); InvalidateRect(hWnd,NULL,false); return 0;
	case WM_LBUTTONDOWN: left=true; return 0;
	case WM_LBUTTONUP:
		mousex=LOWORD(lParam); mousey=HIWORD(lParam);
		if(mousex>=(x*16/2-14) && mousex<=(x*16/2+12) && mousey>=15 && mousey<=41) { goto begin; break; }
		if(mousex>=11 && mousey>=55) {
			if(korean && x<16 || !korean && x<13) ysize = y*16+msize+94; else ysize = y*16+msize+80;
			cacul_board((mousex-11)/16,(mousey-55)/16,left,right);
		}
		if(clear==true && level<5) if(best[level-1]==0.0 || (Play_Time/100.0) < best[level-1])
			if(DialogBox(g_inst,MAKEINTRESOURCE((korean)?ASK_KOR:ASK_ENG),NULL,FirstDlgProc)==IDOK) 
				{	DialogBox(g_inst,MAKEINTRESOURCE((korean)?RECORD_KOR:RECORD_ENG),NULL,RecordDlgProc); // 윈도우 핸들을 NULL로 했더니 됨...
					clear=false; 
				}
		left=false; InvalidateRect(hWnd,NULL,false);
		return 0;
	case WM_MOUSEMOVE:
		mousex=LOWORD(lParam); mousey=HIWORD(lParam);
		return 0;
	case WM_KEYDOWN:
		switch(wParam){
		case VK_F2: goto begin; break;
		}
		return 0;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case START: begin: timeKillEvent(timer1); Kill_mine(); Make_mine(); draw_all(true); InvalidateRect(hWnd,NULL,false); break;
			
		case RECTANGLE: Check(true,1); break;
		case HIVE: Check(true,2); break;
				
		case BEGINNER: Check(false,1); break;
		case MEDIUM: Check(false,2); break;
		case PRO: Check(false,3); break;
		case FULL: Check(false,4); break;
		case CUSTOM: Check(false,5); break;
				
		case ONLINE: break;
		case SCORE: Pause_Screen(); InvalidateRect(hWnd,NULL,true); DialogBox(g_inst,MAKEINTRESOURCE((korean)?RECORD_KOR:RECORD_ENG),NULL,RecordDlgProc); Unpause(); break;
		case OUT_EXIT: goto exit; break;
				
		case OPEN: break;
		case SAVE: break;
		case ANALYSIS: break;
				
		case KOREAN: korean=true; Menu(); break;
		case ENGLISH: korean=false; Menu(); break;
				
		case HELP: break;
		case CREDIT: Pause_Screen(); InvalidateRect(hWnd,NULL,true); DialogBox(g_inst,MAKEINTRESOURCE((korean)?CREDIT_KOR:CREDIT_ENG),NULL,CreditDlgProc); Unpause(); break;
		default: break;
		}
		return 0;
exit:
		case WM_DESTROY:
			WriteData();
			Kill_mine();
			DeleteObject(hTL);
			DeleteObject(hMenu);
			DeleteObject(hIcons);
			DeleteObject(hBitmap);
			DeleteObject(hPause);
			timeKillEvent(timer1);
			kill_DCs();
			PostQuitMessage(0);
			return 0;
	}
	return(DefWindowProc(hWnd,iMessage,wParam,lParam));
}

void Control_File(char *filename,char contents[],DWORD mode,int bytes){
	HANDLE hFile;
	DWORD dwRW;
	
	hFile = CreateFile(filename,mode,0,NULL,(mode==GENERIC_READ)?OPEN_EXISTING:CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if(hFile != INVALID_HANDLE_VALUE){
		if(mode==GENERIC_READ) ReadFile(hFile,contents,bytes,&dwRW,0);
		else WriteFile(hFile,contents,bytes,&dwRW,0); 
	}
	CloseHandle(hFile);
}

void Menu(){ // 영어 한글 메뉴 바꿈~
	int msize=GetSystemMetrics(SM_CYCAPTION)+GetSystemMetrics(SM_CYMENU);
	DestroyMenu(hMenu);
	hMenu=LoadMenu(g_inst,MAKEINTRESOURCE((korean)?KOREAN:ENGLISH));
	SetMenu(hWnd,hMenu);
	CheckMenuItem(hMenu,(korean)?KOREAN:ENGLISH,MF_CHECKED);
	CheckMenuItem(hMenu,(!korean)?KOREAN:ENGLISH,MF_UNCHECKED);
	SetWindowText(hWnd,(korean)?ProgNameKor:ProgNameEng);
	if(replay==false) EnableMenuItem(hMenu,ANALYSIS,MF_GRAYED|MF_DISABLED);
	else EnableMenuItem(hMenu,ANALYSIS,MF_ENABLED|MF_BYCOMMAND);
	if(gameover==false) EnableMenuItem(hMenu,SAVE,MF_GRAYED|MF_DISABLED);
	else EnableMenuItem(hMenu,SAVE,MF_ENABLED|MF_BYCOMMAND);
	
	switch(type){
	case 1: type=1; CheckMenuItem(hMenu,RECTANGLE,MF_CHECKED); break;
	case 2: type=2; CheckMenuItem(hMenu,HIVE,MF_CHECKED); break;
	case 3: type=3; CheckMenuItem(hMenu,CIRCLE,MF_CHECKED); break;
	}
	switch(level){
	case 1: level=1; x=8; y=8; mine=10; CheckMenuItem(hMenu,BEGINNER,MF_CHECKED); break;
	case 2: level=2; x=16; y=16; mine=40; CheckMenuItem(hMenu,MEDIUM,MF_CHECKED); break;
	case 3: level=3; x=30; y=16; mine=99; CheckMenuItem(hMenu,PRO,MF_CHECKED); break;
	case 4: level=4; CheckMenuItem(hMenu,FULL,MF_CHECKED); break;
	case 5: level=5; CheckMenuItem(hMenu,CUSTOM,MF_CHECKED); break;
	}
	if(korean && x<10) SetWindowPos(hWnd,NULL,0,0,x*16+29,y*16+90+msize,(level>=4)?0:SWP_NOMOVE);
		else SetWindowPos(hWnd,NULL,0,0,x*16+29,y*16+74+msize,(level>=4)?0:SWP_NOMOVE);
}

void Check(bool type_menu,int number){ // true : 형태 바꿈, false : 레벨 바꿈... (메뉴 체크표시)
	int msize=GetSystemMetrics(SM_CYCAPTION)+GetSystemMetrics(SM_CYMENU);
	int minex=GetSystemMetrics(SM_CXSCREEN)-28;
	int miney=GetSystemMetrics(SM_CYSCREEN)-74-msize;
	int Dialog=IDOK;
	
	if(type_menu){
		CheckMenuItem(hMenu,RECTANGLE,MF_UNCHECKED);
		CheckMenuItem(hMenu,HIVE,MF_UNCHECKED);
		CheckMenuItem(hMenu,CIRCLE,MF_UNCHECKED);
		switch(number){
		case 1: type=1; CheckMenuItem(hMenu,RECTANGLE,MF_CHECKED); break;
		case 2: type=2; CheckMenuItem(hMenu,HIVE,MF_CHECKED); break;
		case 3: type=3; CheckMenuItem(hMenu,CIRCLE,MF_CHECKED); break;
		}
	}else{
		CheckMenuItem(hMenu,BEGINNER,MF_UNCHECKED);
		CheckMenuItem(hMenu,MEDIUM,MF_UNCHECKED);
		CheckMenuItem(hMenu,PRO,MF_UNCHECKED);
		CheckMenuItem(hMenu,FULL,MF_UNCHECKED);
		CheckMenuItem(hMenu,CUSTOM,MF_UNCHECKED);
		switch(number){
		case 1: Kill_mine(); level=1; x=8; y=8; mine=10; CheckMenuItem(hMenu,BEGINNER,MF_CHECKED); break;
		case 2: Kill_mine(); level=2; x=16; y=16; mine=40; CheckMenuItem(hMenu,MEDIUM,MF_CHECKED); break;
		case 3: Kill_mine(); level=3; x=30; y=16; mine=99; CheckMenuItem(hMenu,PRO,MF_CHECKED); break;
		case 4: Kill_mine(); level=4; x=minex/16; y=miney/16; mine=x*y/5; CheckMenuItem(hMenu,FULL,MF_CHECKED); break;
		case 5:
			Pause_Screen();
			InvalidateRect(hWnd,NULL,true);
			Dialog = DialogBox(g_inst,MAKEINTRESOURCE((korean)?DIALOG_KOR:DIALOG_ENG),hWnd,MineDlgProc); 
			Unpause();
			if(Dialog==IDOK){
				level=5; CheckMenuItem(hMenu,CUSTOM,MF_CHECKED);
				timeKillEvent(timer1); 
				Make_mine(); 
			}
			if(level==1) CheckMenuItem(hMenu,BEGINNER,MF_CHECKED);
			if(level==2) CheckMenuItem(hMenu,MEDIUM,MF_CHECKED);
			if(level==3) CheckMenuItem(hMenu,PRO,MF_CHECKED);
			if(level==4) CheckMenuItem(hMenu,FULL,MF_CHECKED);
			if(level==5) CheckMenuItem(hMenu,CUSTOM,MF_CHECKED);
			break;
		}
		if(number>=1 && number<=4) { timeKillEvent(timer1); Make_mine(); }
		if(korean && x<10) SetWindowPos(hWnd,NULL,0,0,x*16+29,y*16+90+msize,(level>=4)?0:SWP_NOMOVE);
		else SetWindowPos(hWnd,NULL,0,0,x*16+29,y*16+74+msize,(level>=4)?0:SWP_NOMOVE);
		if(Dialog==IDOK) { DeleteObject(hBitmap); hBitmap = CreateCompatibleBitmap(hDC,x*16+29,y*16+90+msize); draw_all(true); }
		InvalidateRect(hWnd,NULL,false);
	}
}

void Make_mine(){
	int i,ix,iy;
	tile_num=0; Play_Sec=0; Play_Time=0;
	gameover=false; clear=false; user_mine=mine;
	
	if(gameover==false) EnableMenuItem(hMenu,SAVE,MF_GRAYED|MF_DISABLED);
	
	randomize();
	if(type==1){
		span=(char **)malloc(sizeof(char *)*x);
		for(i=0;i<x;i++) span[i]=(char *)malloc(sizeof(char)*y);
		for(ix=0;ix<x;ix++)
			for(iy=0;iy<y;iy++)
				span[ix][iy]=-2; // 지뢰 아님...
			for(i=0;i<mine;i++){
				ix=random(x); iy=random(y);
				while(span[ix][iy]==-1){ ix=random(x); iy=random(y); }
				span[ix][iy]=-1; // 지뢰
			}
			tile_num=mine;
	}
}

void Kill_mine(){
	int i;
	if(type==1) for(i=0;i<x;i++) free(span[i]);
	free(span);
}

void draw_right(){
	make_DCs(false);
	//Debug_Print("draw_right()","hOldBit",hOldBit);
	
	Select_Bitmap(hMemDC,hTL,true);
	//Debug_Print("draw_right()","hOld",hOld);
	
	BitBlt(hBitDC,x*16-17,17,10,21,hMemDC,34+11*(user_mine%100/10),25,SRCCOPY);
	BitBlt(hBitDC,x*16-4,17,10,21,hMemDC,34+11*(user_mine%10),25,SRCCOPY);
	if(x>10) BitBlt(hBitDC,x*16-30,17,10,21,hMemDC,34+11*(user_mine/100),25,SRCCOPY);
	
	BitBlt(hBitDC,18,17,10,21,hMemDC,(x<11)?(34+11*(Play_Sec/10)):(34+11*(Play_Sec/100)),25,SRCCOPY); // 시간재기...
	BitBlt(hBitDC,31,17,10,21,hMemDC,(x<11)?(34+11*(Play_Sec%10)):(34+11*(Play_Sec%100/10)),25,SRCCOPY);
	if(x>10) BitBlt(hBitDC,44,17,10,21,hMemDC,34+11*(Play_Sec%10),25,SRCCOPY);
	
	Select_Bitmap(hMemDC,hTL,false);
	
	kill_DCs();
}

void draw_all(bool board){
	int ix,iy;
	make_DCs(true);
	//Debug_Print("draw_all()","hOldBit",hOldBit);
	
	Select_Bitmap(hMemDC,hTL,true);
	//Debug_Print("draw_all(1)","hOld",hOld);
	
	BitBlt(hBitDC,0,0,11,11,hMemDC,0,0,SRCCOPY);
	StretchBlt(hBitDC,11,0,x*16,11,hMemDC,11,0,11,11,SRCCOPY); 
	BitBlt(hBitDC,x*16+11,0,11,11,hMemDC,22,0,SRCCOPY);
	StretchBlt(hBitDC,0,11,11,33,hMemDC,0,11,11,11,SRCCOPY); 
	StretchBlt(hBitDC,x*16+11,11,11,33,hMemDC,22,11,11,11,SRCCOPY); 
	StretchBlt(hBitDC,11,44,x*16,11,hMemDC,11,0,11,11,SRCCOPY); 
	BitBlt(hBitDC,0,44,11,11,hMemDC,0,22,SRCCOPY); 
	BitBlt(hBitDC,x*16+11,44,11,11,hMemDC,22,22,SRCCOPY); 
	StretchBlt(hBitDC,0,55,11,y*16,hMemDC,0,11,11,11,SRCCOPY);
	StretchBlt(hBitDC,x*16+11,55,11,y*16,hMemDC,22,11,11,11,SRCCOPY);
	StretchBlt(hBitDC,11,55+y*16,x*16,11,hMemDC,11,0,11,11,SRCCOPY);
	StretchBlt(hBitDC,11,11,x*16,33,hMemDC,12,12,10,10,SRCCOPY);
	BitBlt(hBitDC,0,55+y*16,11,11,hMemDC,0,33,SRCCOPY);
	BitBlt(hBitDC,x*16+11,55+y*16,11,11,hMemDC,22,33,SRCCOPY);
	
	if(x<11){ StretchBlt(hBitDC,15,15,28,25,hMemDC,34,0,42,25,SRCCOPY); StretchBlt(hBitDC,x*16-20,15,28,25,hMemDC,34,0,42,25,SRCCOPY); }
	else { BitBlt(hBitDC,15,15,42,25,hMemDC,34,0,SRCCOPY); BitBlt(hBitDC,x*16-34,15,42,25,hMemDC,34,0,SRCCOPY); }
	
	BitBlt(hBitDC,x*16-17,17,10,21,hMemDC,34+11*(user_mine%100/10),25,SRCCOPY);
	BitBlt(hBitDC,x*16-4,17,10,21,hMemDC,34+11*(user_mine%10),25,SRCCOPY);
	if(x>10) BitBlt(hBitDC,x*16-30,17,10,21,hMemDC,34+11*(user_mine/100),25,SRCCOPY);
	
	BitBlt(hBitDC,18,17,10,21,hMemDC,(x<11)?(34+11*(Play_Sec/10)):(34+11*(Play_Sec/100)),25,SRCCOPY); // 시간재기...
	BitBlt(hBitDC,31,17,10,21,hMemDC,(x<11)?(34+11*(Play_Sec%10)):(34+11*(Play_Sec%100/10)),25,SRCCOPY);
	if(x>10) BitBlt(hBitDC,44,17,10,21,hMemDC,34+11*(Play_Sec%10),25,SRCCOPY);
	
	Select_Bitmap(hMemDC,hTL,false);
	
	Select_Bitmap(hMemDC,hBoard,true);
	//Debug_Print("draw_all(2)","hOld",hOld);
	
	for(iy=0;iy<y;iy++)
		for(ix=0;ix<x;ix++) 
			if(board && !gameover && !clear)
				BitBlt(hBitDC,11+16*ix,55+16*iy,16,16,hMemDC,0,0,SRCCOPY); 
			
	Select_Bitmap(hMemDC,hBoard,false);
			
	Select_Bitmap(hMemDC,hIcons,true);
	//Debug_Print("draw_all(3)","hOld",hOld);
			
	BitBlt(hBitDC,x*16/2+13,15,26,26,hMemDC,78,0,SRCCOPY);
	if(gameover) BitBlt(hBitDC,x*16/2-14,15,26,26,hMemDC,52,0,SRCCOPY);
	else if(clear) BitBlt(hBitDC,x*16/2-14,15,26,26,hMemDC,0,0,SRCCOPY);
	else { 
		BitBlt(hBitDC,x*16/2-14,15,26,26,hMemDC,26,0,SRCCOPY); 
		BitBlt(hBitDC,x*16/2+13,15,26,26,hMemDC,104,0,SRCCOPY); 
	}
			
	Select_Bitmap(hMemDC,hIcons,false);
			
	kill_DCs();
}

void draw_board(int cx,int cy,int num){
	make_DCs(false);
	//Debug_Print("draw_board()","hOldBit",hOldBit);
	
	Select_Bitmap(hMemDC,hBoard,true);
	//Debug_Print("draw_board()","hOld",hOld);
	
	if(!gameover){
		if(span[cx][cy]>=0) BitBlt(hBitDC,11+(16*cx),55+(16*cy),16,16,hMemDC,(num==10)?144:16*num,128,SRCCOPY);
		else BitBlt(hBitDC,11+(16*cx),55+(16*cy),16,16,hMemDC,0,0,SRCCOPY);
	}
	if(gameover){
		if(num==-2) BitBlt(hBitDC,11+(16*cx),55+(16*cy),16,16,hMemDC,0,0,SRCCOPY);
		if(num==-1) BitBlt(hBitDC,11+(16*cx),55+(16*cy),16,16,hMemDC,16,0,SRCCOPY);
		if(span[cx][cy]>=0) BitBlt(hBitDC,11+(16*cx),55+(16*cy),16,16,hMemDC,16*num,128,SRCCOPY);
		if(num==11) BitBlt(hBitDC,11+(16*cx),55+(16*cy),16,16,hMemDC,32,0,SRCCOPY);
	}
	Select_Bitmap(hMemDC,hBoard,false);
	
	kill_DCs();
}

void draw_gameover(){
	int ix,iy;
	make_DCs(false);
	
	Select_Bitmap(hMemDC,hBoard,true);
	
	for(ix=0;ix<x;ix++) for(iy=0;iy<y;iy++){
		if(span[ix][iy]==-1) BitBlt(hBitDC,11+(16*ix),55+(16*iy),16,16,hMemDC,16,0,SRCCOPY);
		else if(span[ix][iy]==11) BitBlt(hBitDC,11+(16*ix),55+(16*iy),16,16,hMemDC,32,0,SRCCOPY);
		else BitBlt(hBitDC,11+(16*ix),55+(16*iy),16,16,hMemDC,16*span[ix][iy],128,SRCCOPY);
	}
	
	Select_Bitmap(hMemDC,hBoard,false);
	if(gameover) EnableMenuItem(hMenu,SAVE,MF_ENABLED|MF_BYCOMMAND);
	kill_DCs();
}

void cacul_board(int bx,int by,bool left,bool right){
	int mine_num=0,right_mine=0,ix,iy;
	//char times[128];
	if(bx>=0 && bx<x && by>=0 && by<y && !gameover && !clear){
		if(!(left && right) && left){ //마우스 왼쪽 버튼을 눌렀을 때...
			if(span[bx][by]==-2){ // 지뢰가 아니고 눌린 적이 없으면 주변에 지뢰를 탐색...
				if(tile_num==mine) { Play_Sec=0; timer1 = timeSetEvent(10,1,PlayTimeProc,0,TIME_PERIODIC); }
				tile_num++;
				if(bx>=0 && bx<x-1) if(span[bx+1][by]==-1 || span[bx+1][by]==9) ++mine_num;
				if(bx>0 && bx<=x-1) if(span[bx-1][by]==-1 || span[bx-1][by]==9) ++mine_num;
				if(by>=0 && by<y-1) if(span[bx][by+1]==-1 || span[bx][by+1]==9) ++mine_num;
				if(by>0 && by<=y-1) if(span[bx][by-1]==-1 || span[bx][by-1]==9) ++mine_num;
				if(bx>=0 && by>=0 && bx<x-1 && by<y-1) if(span[bx+1][by+1]==-1 || span[bx+1][by+1]==9) ++mine_num;
				if(bx>0 && by>=0 && bx<=x-1 && by<y-1) if(span[bx-1][by+1]==-1 || span[bx-1][by+1]==9) ++mine_num;
				if(bx>=0 && by>0 && bx<x-1 && by<=y-1) if(span[bx+1][by-1]==-1 || span[bx+1][by-1]==9) ++mine_num;
				if(bx>0 && by>0 && bx<=x-1 && by<=y-1) if(span[bx-1][by-1]==-1 || span[bx-1][by-1]==9) ++mine_num;
				span[bx][by]=mine_num; // mine_num이 주변에 지뢰갯수를 뜻함...
				if(mine_num==0){ //주변에 지뢰가 없으면 재귀함수로 계속탐색...
					if(bx>=0 && bx<x-1) cacul_board(bx+1,by,left,right);
					if(bx>0 && bx<=x-1) cacul_board(bx-1,by,left,right);
					if(by>=0 && by<y-1) cacul_board(bx,by+1,left,right);
					if(by>0 && by<=y-1) cacul_board(bx,by-1,left,right);
					if(bx>=0 && by>=0 && bx<x-1 && by<y-1) cacul_board(bx+1,by+1,left,right);
					if(bx>0 && by>=0 && bx<=x-1 && by<y-1) cacul_board(bx-1,by+1,left,right);
					if(bx>=0 && by>0 && bx<x-1 && by<=y-1) cacul_board(bx+1,by-1,left,right);
					if(bx>0 && by>0 && bx<=x-1 && by<=y-1) cacul_board(bx-1,by-1,left,right);
				}
				draw_board(bx,by,mine_num);
			}
			if(bx>=0 && bx<x && by>=0 && by<y && span[bx][by]==-1){ // 지뢰가 눌렸을 때...
				gameover=true; timeKillEvent(timer1); span[bx][by]=11; draw_all(false);
				draw_board(bx,by,11);
				draw_gameover();
				InvalidateRect(hWnd,NULL,false);
			}
			if(tile_num==x*y && !clear){ // 다 깼다면 :)
				clear=true; timeKillEvent(timer1); user_mine=0; draw_all(false);
				for(ix=0;ix<x;ix++) for(iy=0;iy<y;iy++) if(span[ix][iy]==-1) { span[ix][iy]=9; draw_board(ix,iy,span[ix][iy]); }
				InvalidateRect(hWnd,NULL,false);
				//sprintf(times,"걸린 시간 : %10.2f 초",(float)Play_Time/100); MessageBox(NULL,times,"걸린 시간",MB_OK);
			}
		}
		if(right && !(left && right)){
			if(bx>=0 && bx<x && by>=0 && by<y){
				if(span[bx][by]<0 && user_mine>0) { span[bx][by]=8-span[bx][by]; user_mine--; draw_board(bx,by,span[bx][by]); }
				else if(span[bx][by]>8) { span[bx][by]=8-span[bx][by]; user_mine++; draw_board(bx,by,0); }
			}
		}
		if(left && right && span[bx][by]!=-2) { //마우스 왼쪽 오른쪽 버튼 동시에 눌렀을 때...
			if(bx>=0 && bx<x-1) if(span[bx+1][by]>8) ++right_mine;
			if(bx>0 && bx<=x-1) if(span[bx-1][by]>8) ++right_mine;
			if(by>=0 && by<y-1) if(span[bx][by+1]>8) ++right_mine;
			if(by>0 && by<=y-1) if(span[bx][by-1]>8) ++right_mine;
			if(bx>=0 && by>=0 && bx<x-1 && by<y-1) if(span[bx+1][by+1]>8) ++right_mine;
			if(bx>0 && by>=0 && bx<=x-1 && by<y-1) if(span[bx-1][by+1]>8) ++right_mine;
			if(bx>=0 && by>0 && bx<x-1 && by<=y-1) if(span[bx+1][by-1]>8) ++right_mine;
			if(bx>0 && by>0 && bx<=x-1 && by<=y-1) if(span[bx-1][by-1]>8) ++right_mine;
			if(right_mine>0 && span[bx][by]==right_mine){
				if(bx>=0 && bx<x-1) if(span[bx+1][by]==-1){ gameover=true; timeKillEvent(timer1); span[bx+1][by]=11; } else cacul_board(bx+1,by,true,false); 
				if(bx>0 && bx<=x-1) if(span[bx-1][by]==-1){ gameover=true; timeKillEvent(timer1); span[bx-1][by]=11; } else cacul_board(bx-1,by,true,false); 
				if(by>=0 && by<y-1) if(span[bx][by+1]==-1){ gameover=true; timeKillEvent(timer1); span[bx][by+1]=11; } else cacul_board(bx,by+1,true,false); 
				if(by>0 && by<=y-1) if(span[bx][by-1]==-1){ gameover=true; timeKillEvent(timer1); span[bx][by-1]=11; } else cacul_board(bx,by-1,true,false); 
				if(bx>=0 && by>=0 && bx<x-1 && by<y-1) if(span[bx+1][by+1]==-1){ gameover=true; timeKillEvent(timer1); span[bx+1][by+1]=11; } else cacul_board(bx+1,by+1,true,false); 
				if(bx>0 && by>=0 && bx<=x-1 && by<y-1) if(span[bx-1][by+1]==-1){ gameover=true; timeKillEvent(timer1); span[bx-1][by+1]=11; } else cacul_board(bx-1,by+1,true,false); 
				if(bx>=0 && by>0 && bx<x-1 && by<=y-1) if(span[bx+1][by-1]==-1){ gameover=true; timeKillEvent(timer1); span[bx+1][by-1]=11; } else cacul_board(bx+1,by-1,true,false); 
				if(bx>0 && by>0 && bx<=x-1 && by<=y-1) if(span[bx-1][by-1]==-1){ gameover=true; timeKillEvent(timer1); span[bx-1][by-1]=11; } else cacul_board(bx-1,by-1,true,false); 
				if(gameover) { draw_gameover(); InvalidateRect(hWnd,NULL,false); }
			}
		}
	} 
}

void make_DCs(bool vir){
	int msize=GetSystemMetrics(SM_CYCAPTION)+GetSystemMetrics(SM_CYMENU),ysize;
	
	if(!act_DC){
		hDC = GetDC(hWnd);
		hMemDC = CreateCompatibleDC(hDC);
		hBitDC = CreateCompatibleDC(hDC);
		hPauseDC = CreateCompatibleDC(hDC);
		act_DC=true;
	}
	
	if(korean && x<16 || !korean && x<13) ysize = y*16+msize+94; else ysize = y*16+msize+80;
	if(vir) if(hBitmap==NULL) hBitmap = CreateCompatibleBitmap(hDC,x*16+32,ysize);
	
	//if(act_VBit) { sprintf(debug,"Act_VBit = true\n"); MessageBox(NULL,"true","act_VBit",MB_OK); }else sprintf(debug,"Act_VBit = false\n");
	//OutputDebugString(debug);
	
	if(!act_VBit) { hOldBit=(HBITMAP)SelectObject(hBitDC,hBitmap); act_VBit=true; }
}

void kill_DCs(){
	if(act_VBit) { SelectObject(hBitDC,hOldBit); act_VBit=false; }
	if(act_DC){ DeleteDC(hMemDC); DeleteDC(hBitDC); DeleteDC(hPauseDC); ReleaseDC(hWnd,hDC); act_DC=false;}	
} 

void Select_Bitmap(HDC dc,HGDIOBJ ob,bool act){
	if(act){
		if(act_Bit) { SelectObject(dc,hOld); hOld = (HBITMAP)SelectObject(dc,ob); }
		if(!act_Bit) { hOld = (HBITMAP)SelectObject(dc,ob); act_Bit=true; }
	} else{
		if(act_Bit) { SelectObject(dc,hOld); act_Bit=false; }
	}
}

void Pause_Screen(){
	int ysize;
	int msize=GetSystemMetrics(SM_CYCAPTION)+GetSystemMetrics(SM_CYMENU);
	HBITMAP hOldBit,hOld;
	if(!gameover && tile_num>mine){
	pause=true;
	make_DCs(false);
	if(korean && x<16 || !korean && x<13) ysize = y*16+msize+94; else ysize = y*16+msize+80;
	if(hPause==NULL) 
		hPause = CreateCompatibleBitmap(hDC,x*16+32,ysize);
	
	hOldBit = (HBITMAP)SelectObject(hPauseDC,hPause); 
	
	Rectangle(hPauseDC,0,0,x*16+23,ysize);
	hOld = (HBITMAP)SelectObject(hMemDC,hTL);
	
	StretchBlt(hPauseDC,1,1,x*16+21,54,hMemDC,12,12,10,10,SRCCOPY);
	if(x<11){ StretchBlt(hPauseDC,15,15,28,25,hMemDC,34,0,42,25,SRCCOPY); StretchBlt(hPauseDC,x*16-20,15,28,25,hMemDC,34,0,42,25,SRCCOPY); }
	else { BitBlt(hPauseDC,15,15,42,25,hMemDC,34,0,SRCCOPY); BitBlt(hPauseDC,x*16-34,15,42,25,hMemDC,34,0,SRCCOPY); }
	
	BitBlt(hPauseDC,x*16-17,17,10,21,hMemDC,34+11*(user_mine%100/10),25,SRCCOPY);
	BitBlt(hPauseDC,x*16-4,17,10,21,hMemDC,34+11*(user_mine%10),25,SRCCOPY);
	if(x>10) BitBlt(hPauseDC,x*16-30,17,10,21,hMemDC,34+11*(user_mine/100),25,SRCCOPY);
	
	BitBlt(hPauseDC,18,17,10,21,hMemDC,(x<11)?(34+11*(Play_Sec/10)):(34+11*(Play_Sec/100)),25,SRCCOPY); // 시간재기...
	BitBlt(hPauseDC,31,17,10,21,hMemDC,(x<11)?(34+11*(Play_Sec%10)):(34+11*(Play_Sec%100/10)),25,SRCCOPY);
	if(x>10) BitBlt(hPauseDC,44,17,10,21,hMemDC,34+11*(Play_Sec%10),25,SRCCOPY);
	
	SelectObject(hMemDC,hOld);

	BitBlt(hDC,0,0,x*16+28,ysize,hPauseDC,0,0,SRCCOPY);

	SelectObject(hPauseDC,hOldBit); 
	kill_DCs();	
	}
}

void Unpause(){
	pause=false;
	InvalidateRect(hWnd,NULL,false);
}


