#ifndef UNICODE
#define UNICODE
#endif
#define _UNICODE
#include <windows.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>

struct Note { std::wstring title, body; };
std::vector<Note> notes;
std::vector<int> visible;
HWND win, listbox, searchbox, titlebox, bodybox, statusbox;
int selected=-1;
bool changing=false, dirty=false;
std::wstring file;
std::wstring getText(HWND h) { int n=GetWindowTextLengthW(h); std::wstring s(n+1,L'\0'); GetWindowTextW(h,s.data(),n+1); s.resize(n); return s; }
void status(const wchar_t* s) { SetWindowTextW(statusbox,s); }
void error(const wchar_t* s) { MessageBoxW(win,s,L"备忘录",MB_OK|MB_ICONERROR); }
bool writeAll(HANDLE h,const void* p,DWORD n) { DWORD done=0; return WriteFile(h,p,n,&done,nullptr)&&done==n; }
bool readAll(HANDLE h,void* p,DWORD n) { DWORD done=0; return ReadFile(h,p,n,&done,nullptr)&&done==n; }
bool save() {
    if(!dirty) return true;
    uint64_t bytes=12;
    for(auto& n:notes) bytes+=8+2*(n.title.size()+n.body.size());
    if(bytes>128*1024*1024) { status(L"数据超过 128 MB，请缩减内容后保存"); return false; }
    std::wstring tmp=file+L".tmp";
    HANDLE h=CreateFileW(tmp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(h==INVALID_HANDLE_VALUE) { status(L"保存失败，请检查磁盘或权限"); return false; }
    uint32_t count=(uint32_t)notes.size();
    bool ok=writeAll(h,"MEMO0001",8)&&writeAll(h,&count,4);
    for(auto& n:notes) for(auto* s:{&n.title,&n.body}) {
        uint32_t len=(uint32_t)s->size();
        ok=ok&&writeAll(h,&len,4)&&writeAll(h,s->data(),len*2);
    }
    ok=ok&&FlushFileBuffers(h); CloseHandle(h);
    if(ok) ok=MoveFileExW(tmp.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
    if(!ok) { status(L"保存失败，内容仍在内存中，请勿关闭"); return false; }
    dirty=false; status(L"已自动保存 · 数据保存在当前用户的 AppData\\Local\\SimpleMemo"); return true;
}
bool load() {
    HANDLE h=CreateFileW(file.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(h==INVALID_HANDLE_VALUE) return GetLastError()==ERROR_FILE_NOT_FOUND;
    char magic[8]; uint32_t count=0; LARGE_INTEGER size;
    bool ok=GetFileSizeEx(h,&size)&&size.QuadPart<=128*1024*1024&&readAll(h,magic,8)&&std::string(magic,8)=="MEMO0001"&&readAll(h,&count,4)&&count<=10000;
    std::vector<Note> loaded;
    for(uint32_t i=0;ok&&i<count;i++) {
        Note n;
        for(auto* s:{&n.title,&n.body}) { uint32_t len=0; ok=ok&&readAll(h,&len,4)&&len<=1000000; if(ok) { s->resize(len); ok=readAll(h,s->data(),len*2); } }
        if(ok) loaded.push_back(n);
    }
    char extra; DWORD got=0; if(ok) ok=ReadFile(h,&extra,1,&got,nullptr)&&got==0;
    CloseHandle(h); if(ok) notes=std::move(loaded); return ok;
}
std::wstring lower(std::wstring s) { if(!s.empty()) CharLowerBuffW(s.data(),(DWORD)s.size()); return s; }
void refresh() {
    changing=true; SendMessageW(listbox,LB_RESETCONTENT,0,0); visible.clear();
    auto q=lower(getText(searchbox)); int row=-1;
    for(int i=0;i<(int)notes.size();i++) {
        if(!q.empty()&&lower(notes[i].title+L"\n"+notes[i].body).find(q)==std::wstring::npos) continue;
        if(i==selected) row=(int)visible.size();
        visible.push_back(i);
        std::wstring label=notes[i].title.empty()?L"（无标题）":notes[i].title;
        std::replace(label.begin(),label.end(),L'\n',L' ');
        SendMessageW(listbox,LB_ADDSTRING,0,(LPARAM)label.c_str());
    }
    SendMessageW(listbox,LB_SETCURSEL,row,0); changing=false;
}
void show(int i) {
    selected=i; changing=true;
    SetWindowTextW(titlebox,i>=0?notes[i].title.c_str():L""); SetWindowTextW(bodybox,i>=0?notes[i].body.c_str():L"");
    EnableWindow(titlebox,i>=0); EnableWindow(bodybox,i>=0); changing=false;
}
void layout() {
    RECT r; GetClientRect(win,&r); int w=r.right,h=r.bottom, left=230;
    MoveWindow(searchbox,16,16,left-32,30,TRUE);
    MoveWindow(listbox,16,56,left-32,h-126,TRUE);
    MoveWindow(GetDlgItem(win,101),16,h-60,92,30,TRUE); MoveWindow(GetDlgItem(win,102),116,h-60,98,30,TRUE);
    MoveWindow(titlebox,left+8,16,w-left-24,34,TRUE);
    MoveWindow(bodybox,left+8,62,w-left-24,h-100,TRUE);
    MoveWindow(statusbox,16,h-25,w-32,22,TRUE);
}
HWND control(const wchar_t* cls,const wchar_t* text,DWORD style,int id,DWORD ex=0) {
    HWND h=CreateWindowExW(ex,cls,text,WS_CHILD|WS_VISIBLE|style,0,0,0,0,win,(HMENU)(INT_PTR)id,GetModuleHandleW(nullptr),nullptr);
    SendMessageW(h,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE); return h;
}
LRESULT CALLBACK proc(HWND h,UINT m,WPARAM wp,LPARAM lp) {
    switch(m) {
    case WM_CREATE:
        win=h;
        searchbox=control(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,100,WS_EX_CLIENTEDGE);
        SendMessageW(searchbox,EM_SETCUEBANNER,TRUE,(LPARAM)L"搜索标题或内容…");
        listbox=control(L"LISTBOX",L"",WS_TABSTOP|LBS_NOTIFY|WS_VSCROLL,103,WS_EX_CLIENTEDGE);
        control(L"BUTTON",L"＋ 新增",WS_TABSTOP,101); control(L"BUTTON",L"删除",WS_TABSTOP,102);
        titlebox=control(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,104,WS_EX_CLIENTEDGE);
        SendMessageW(titlebox,EM_SETLIMITTEXT,1000,0); SendMessageW(titlebox,EM_SETCUEBANNER,TRUE,(LPARAM)L"备忘标题");
        bodybox=control(L"EDIT",L"",WS_TABSTOP|ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|WS_VSCROLL,105,WS_EX_CLIENTEDGE);
        SendMessageW(bodybox,EM_SETLIMITTEXT,1000000,0);
        statusbox=control(L"STATIC",L"点击“新增”开始记录 · 编辑后自动保存",0,106);
        show(notes.empty()?-1:0); refresh(); return 0;
    case WM_SIZE: layout(); return 0;
    case WM_GETMINMAXINFO: ((MINMAXINFO*)lp)->ptMinTrackSize={640,400}; return 0;
    case WM_COMMAND:
        if(changing) return 0;
        if(LOWORD(wp)==101) {
            if(notes.size()>=10000) { error(L"最多支持 10000 条备忘。"); return 0; }
            changing=true; SetWindowTextW(searchbox,L""); changing=false;
            notes.push_back({L"新备忘",L""}); show((int)notes.size()-1); refresh(); dirty=true; save(); SetFocus(titlebox); SendMessageW(titlebox,EM_SETSEL,0,-1);
        } else if(LOWORD(wp)==102&&selected>=0) {
            if(MessageBoxW(h,L"确定删除当前备忘？此操作无法撤销。",L"删除备忘",MB_YESNO|MB_ICONQUESTION)==IDYES) {
                notes.erase(notes.begin()+selected); show(-1); refresh(); dirty=true; save();
            }
        } else if(LOWORD(wp)==103&&HIWORD(wp)==LBN_SELCHANGE) {
            int row=(int)SendMessageW(listbox,LB_GETCURSEL,0,0); if(row>=0&&row<(int)visible.size()) show(visible[row]);
        } else if(LOWORD(wp)==100&&HIWORD(wp)==EN_CHANGE) refresh();
        else if((LOWORD(wp)==104||LOWORD(wp)==105)&&HIWORD(wp)==EN_CHANGE&&selected>=0) {
            notes[selected]={getText(titlebox),getText(bodybox)}; dirty=true; status(L"正在编辑…"); SetTimer(h,1,600,nullptr); refresh();
        }
        return 0;
    case WM_TIMER: KillTimer(h,1); save(); return 0;
    case WM_CLOSE:
        if(!save()) { error(L"保存失败。请检查磁盘空间和数据目录权限后重试，程序将保持打开。"); return 0; }
        DestroyWindow(h); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h,m,wp,lp);
}
int WINAPI wWinMain(HINSTANCE inst,HINSTANCE,LPWSTR,int showCmd) {
    // Prevent concurrent writers to the same per-user file.
    HANDLE mutex=CreateMutexW(nullptr,TRUE,L"Local\\SimpleMemo.SingleInstance");
    if(!mutex||GetLastError()==ERROR_ALREADY_EXISTS) { MessageBoxW(nullptr,L"备忘录已经运行，请使用已打开的窗口。",L"备忘录",MB_OK); if(mutex) CloseHandle(mutex); return 1; }
    wchar_t path[MAX_PATH];
    if(FAILED(SHGetFolderPathW(nullptr,CSIDL_LOCAL_APPDATA,nullptr,SHGFP_TYPE_CURRENT,path))) { error(L"无法获取用户数据目录。"); return 1; }
    std::wstring dir=std::wstring(path)+L"\\SimpleMemo";
    if(!CreateDirectoryW(dir.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS) { error(L"无法创建数据目录。"); return 1; }
    file=dir+L"\\notes.dat";
    if(!load()) { error(L"无法读取备忘数据，文件可能损坏或无读取权限。原文件未修改，请先备份并检查 AppData\\Local\\SimpleMemo\\notes.dat。"); return 1; }
    WNDCLASSW wc{}; wc.lpfnWndProc=proc; wc.hInstance=inst; wc.lpszClassName=L"SimpleMemoWindow"; wc.hCursor=LoadCursorW(nullptr,IDC_ARROW); wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
    RegisterClassW(&wc);
    HWND h=CreateWindowExW(0,wc.lpszClassName,L"备忘录",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,880,580,nullptr,nullptr,inst,nullptr);
    if(!h) return 1;
    ShowWindow(h,showCmd); UpdateWindow(h);
    MSG msg; while(GetMessageW(&msg,nullptr,0,0)>0) { if(!IsDialogMessageW(h,&msg)) { TranslateMessage(&msg); DispatchMessageW(&msg); } }
    ReleaseMutex(mutex); CloseHandle(mutex); return 0;
}
