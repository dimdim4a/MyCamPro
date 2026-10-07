#include "gui/window.h"
#include "settings.h"
#include <wx/settings.h>
#include <vector>
#include <string>

namespace {
const wxColour BG(6,11,19), SURFACE(11,18,29), CARD(16,25,39), CARD2(21,32,49);
const wxColour BLUE(47,126,255), GREEN(45,220,132), TEXT(242,246,252), MUTED(151,165,187);
wxString U(const char* s){return wxString::FromUTF8(s);}
wxStaticText* T(wxWindow* p,const wxString& s,int n=10,bool b=false,wxColour c=TEXT){
 auto* x=new wxStaticText(p,wxID_ANY,s); auto f=x->GetFont(); f.SetPointSize(n); f.SetWeight(b?wxFONTWEIGHT_BOLD:wxFONTWEIGHT_NORMAL); x->SetFont(f); x->SetForegroundColour(c); return x;
}
wxPanel* C(wxWindow* p,wxColour c=CARD){auto* x=new wxPanel(p,wxID_ANY);x->SetBackgroundColour(c);return x;}
wxButton* B(wxWindow* p,const wxString& s,int id=wxID_ANY,bool primary=false){
 auto* x=new wxButton(p,id,s,wxDefaultPosition,wxDefaultSize,wxBORDER_NONE);x->SetBackgroundColour(primary?BLUE:CARD2);x->SetForegroundColour(TEXT);x->SetMinSize(p->FromDIP(wxSize(primary?112:96,36)));return x;
}
wxButton* I(wxWindow* p,const wxString& f,const wxString& tip){
 wxImage im(wxString("res/")+f,wxBITMAP_TYPE_PNG); if(im.IsOk()){if(!im.HasAlpha())im.InitAlpha();auto*d=im.GetData();size_t n=(size_t)im.GetWidth()*im.GetHeight()*3;for(size_t i=0;i<n;i++)d[i]=255;}
 auto*x=new wxBitmapButton(p,wxID_ANY,im.IsOk()?wxBitmap(im):wxNullBitmap,wxDefaultPosition,p->FromDIP(wxSize(48,48)),wxBORDER_NONE);x->SetBackgroundColour(CARD2);x->SetToolTip(tip);return x;
}
}

Window::Window(Server::HostInfo hostinfo):wxFrame(nullptr,wxID_ANY,"MyCam Pro",wxDefaultPosition,wxDefaultSize,wxDEFAULT_FRAME_STYLE){
 wxIcon icon("res/nexora.ico",wxBITMAP_TYPE_ICO);taskbarIcon=new wxTaskBarIcon();taskbarIcon->SetIcon(icon,"MyCam Pro");SetIcon(icon);
 taskbarIcon->Bind(wxEVT_TASKBAR_LEFT_DCLICK,&Window::MaximizeFromTaskbar,this);Bind(wxEVT_ICONIZE,&Window::MinimizeToTaskbar,this);
 auto* root=new wxPanel(this,wxID_ANY);root->SetBackgroundColour(BG);auto* top=new wxBoxSizer(wxVERTICAL);InitializeMenu(hostinfo);InitializeHeader(root,top);root->SetSizer(top);
 SetSizer(new wxBoxSizer(wxVERTICAL));GetSizer()->Add(root,1,wxEXPAND);SetMinClientSize(FromDIP(wxSize(1120,720)));SetClientSize(FromDIP(wxSize(1500,900)));Layout();Center();
}
Window::~Window(){delete taskbarIcon;}

void Window::InitializeMenu(Server::HostInfo hostinfo){
 auto* mb=new wxMenuBar();auto* m=new wxMenu();m->AppendCheckItem(MenuIDs::HIDE2TRAY,"Minimize to tray");m->AppendCheckItem(MenuIDs::SHOWSTATS,"Show stream statistics");m->AppendCheckItem(MenuIDs::SAVESTATE,"Remember device settings");
 auto* r=new wxMenu();r->AppendRadioItem(MenuIDs::DS_SD,"640 x 480");r->AppendRadioItem(MenuIDs::DS_HD,"1280 x 720");r->AppendRadioItem(MenuIDs::DS_FHD,"1920 x 1080");r->AppendRadioItem(MenuIDs::DS_QHD,"3840 x 2160");m->AppendSubMenu(r,"Virtual camera resolution");m->Append(wxID_EXIT,"Exit");mb->Append(m,"MyCam Pro");
 auto* c=new wxMenu();c->Append(MenuIDs::QR,"Show QR code");c->Append(MenuIDs::DEVICES,"Connected devices");c->AppendSeparator();c->Append(wxID_ANY,"Address: "+std::get<1>(hostinfo));c->Append(wxID_ANY,"Port: "+std::get<2>(hostinfo));mb->Append(c,"Connection");SetMenuBar(mb);
}

void Window::InitializeHeader(wxPanel* parent,wxBoxSizer* topsizer){
 auto* shell=new wxPanel(parent,wxID_ANY);shell->SetBackgroundColour(BG);auto* hs=new wxBoxSizer(wxHORIZONTAL);
 auto* side=new wxPanel(shell,wxID_ANY);side->SetBackgroundColour(SURFACE);side->SetMinSize(FromDIP(wxSize(228,-1)));auto* ss=new wxBoxSizer(wxVERTICAL);
 auto* brand=new wxBoxSizer(wxHORIZONTAL);brand->Add(T(side,"◈",18,true,BLUE),0,wxALIGN_CENTER_VERTICAL|wxRIGHT,8);brand->Add(T(side,"MyCam Pro",15,true),0,wxALIGN_CENTER_VERTICAL);ss->Add(brand,0,wxALL,18);
 const char* names[]={"Камера","Сцены","Изображение","Эффекты","Запись","Аудио","Устройства","Настройки"};const char* icons[]={"▣","▤","◉","✦","●","♬","▣","⚙"};std::vector<wxButton*> nav;
 auto* ns=new wxBoxSizer(wxVERTICAL);
 for(int i=0;i<8;i++){auto*x=B(side,U((std::string(icons[i])+"   "+names[i]).c_str()),i==6?MenuIDs::DEVICES:wxID_ANY,i==0);x->SetMinSize(FromDIP(wxSize(208,42)));nav.push_back(x);ns->Add(x,0,wxEXPAND|wxBOTTOM,5);}ss->Add(ns,0,wxEXPAND|wxLEFT|wxRIGHT,10);ss->AddStretchSpacer();
 auto* vc=C(side,CARD2);auto*vcs=new wxBoxSizer(wxVERTICAL);vcs->Add(T(vc,U("●  MyCam Pro Camera"),10,true,GREEN),0,wxALL,12);vcs->Add(T(vc,U("Виртуальная камера"),9,false,MUTED),0,wxLEFT|wxRIGHT|wxBOTTOM,12);vcs->Add(B(vc,U("Открыть в OBS"),wxID_ANY,true),0,wxEXPAND|wxALL,10);vc->SetSizer(vcs);ss->Add(vc,0,wxEXPAND|wxALL,12);side->SetSizer(ss);hs->Add(side,0,wxEXPAND);
 auto* content=new wxPanel(shell,wxID_ANY);content->SetBackgroundColour(BG);auto* cs=new wxBoxSizer(wxVERTICAL);
 auto* head=C(content);auto* hr=new wxBoxSizer(wxHORIZONTAL);hr->Add(T(head,U("MYCAM PRO"),14,true),0,wxALIGN_CENTER_VERTICAL|wxLEFT,16);hr->AddStretchSpacer();
 statusText=new wxStaticText(head,wxID_ANY,U("●  Готово   Ожидание телефона   ·   Wi‑Fi"),wxDefaultPosition,FromDIP(wxSize(410,36)),wxALIGN_CENTER);statusText->SetBackgroundColour(CARD2);statusText->SetForegroundColour(MUTED);hr->Add(statusText,0,wxALIGN_CENTER_VERTICAL|wxRIGHT,8);
 hr->Add(B(head,U("QR-код"),MenuIDs::QR,true),0,wxALIGN_CENTER_VERTICAL|wxRIGHT,6);hr->Add(B(head,U("Устройства"),MenuIDs::DEVICES),0,wxALIGN_CENTER_VERTICAL|wxRIGHT,10);head->SetSizer(hr);cs->Add(head,0,wxEXPAND|wxBOTTOM,10);
 auto* pages=new wxPanel(content,wxID_ANY);pages->SetBackgroundColour(BG);auto* pgs=new wxBoxSizer(wxVERTICAL);pages->SetSizer(pgs);cs->Add(pages,1,wxEXPAND);content->SetSizer(cs);hs->Add(content,1,wxEXPAND|wxLEFT,1);shell->SetSizer(hs);topsizer->Add(shell,1,wxEXPAND);

 auto* camera=new wxPanel(pages,wxID_ANY);camera->SetBackgroundColour(BG);auto* cam=new wxBoxSizer(wxVERTICAL);camera->SetSizer(cam);
 auto* row=new wxBoxSizer(wxHORIZONTAL);auto* pv=C(camera);auto*pvs=new wxBoxSizer(wxVERTICAL);auto* pm=new wxBoxSizer(wxHORIZONTAL);pm->Add(T(pv,U("LIVE PREVIEW"),9,true,MUTED),0,wxALIGN_CENTER_VERTICAL|wxRIGHT,12);pm->Add(T(pv,U("720p/1080p  •  H.264  •  Hardware"),9,false,MUTED),0,wxALIGN_CENTER_VERTICAL);pvs->Add(pm,0,wxALL,12);
 canvas=new Canvas(pv,wxDefaultPosition,FromDIP(wxSize(780,450)));canvas->SetMinSize(FromDIP(wxSize(520,300)));canvas->SetBackgroundColour(wxColour(2,5,10));pvs->Add(canvas,1,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,10);
 auto* q=new wxBoxSizer(wxHORIZONTAL);snapshotButton=I(pv,"photo.png",U("Снимок"));torchButton=I(pv,"flash.png",U("Вспышка"));swapButton=I(pv,"swap.png",U("Камера"));adjustmentsButton=I(pv,"settings.png",U("Изображение"));rotateLeftButton=I(pv,"rotate-left.png",U("Влево"));rotateRightButton=I(pv,"rotate-right.png",U("Вправо"));flipButton=I(pv,"flip.png",U("Зеркало"));flipVerticalButton=I(pv,"flip-v.png",U("Переворот"));zoomOutButton=I(pv,"zoom-out.png",U("Уменьшить"));zoomInButton=I(pv,"zoom-in.png",U("Увеличить"));
 for(auto*x:{snapshotButton,torchButton,swapButton,adjustmentsButton})q->Add(x,0,wxRIGHT,7);q->AddStretchSpacer();for(auto*x:{rotateLeftButton,rotateRightButton,flipButton,flipVerticalButton,zoomOutButton})q->Add(x,0,wxRIGHT,5);zoomLevelLabel=T(pv,"1.0x",10,true);q->Add(zoomLevelLabel,0,wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT,5);q->Add(zoomInButton);pvs->Add(q,0,wxLEFT|wxRIGHT|wxBOTTOM,12);pv->SetSizer(pvs);row->Add(pv,1,wxEXPAND|wxRIGHT,10);
 auto* rp=C(camera);auto* rs=new wxBoxSizer(wxVERTICAL);rs->Add(T(rp,U("Камера"),15,true),0,wxALL,14);rs->Add(T(rp,U("ИСТОЧНИК"),9,true,MUTED),0,wxLEFT|wxRIGHT,14);
 wxArrayString ch;ch.Add(U("Устройства не найдены"));sourceChoice=new wxChoice(rp,wxID_ANY,wxDefaultPosition,wxDefaultSize,ch);sourceChoice->SetMinSize(FromDIP(wxSize(210,36)));sourceChoice->SetSelection(0);rs->Add(sourceChoice,0,wxEXPAND|wxALL,10);
 auto add=[&](const char*a,const char*b){rs->Add(T(rp,U(a),9,false,MUTED),0,wxLEFT|wxRIGHT|wxTOP,12);auto*x=new wxChoice(rp,wxID_ANY);x->Append(U(b));x->SetSelection(0);x->SetMinSize(FromDIP(wxSize(210,36)));rs->Add(x,0,wxEXPAND|wxLEFT|wxRIGHT|wxTOP,6);};add("Разрешение","Auto / 720p / 1080p / 4K");add("FPS","Auto / 30 / 60");add("Битрейт","Auto / 2–40 Mbps");add("Профиль","Сбалансированный");
 streamOptionsButton=B(rp,U("⚙  Настройки потока"));rs->Add(streamOptionsButton,0,wxEXPAND|wxALL,14);auto* info=C(rp,CARD2);auto*is=new wxBoxSizer(wxVERTICAL);is->Add(T(info,U("ПРИОРИТЕТ"),9,true,MUTED),0,wxALL,10);is->Add(T(info,U("Стабильность → FPS → задержка → качество"),9,false),0,wxLEFT|wxRIGHT|wxBOTTOM,10);info->SetSizer(is);rs->Add(info,0,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,12);rs->AddStretchSpacer();rp->SetSizer(rs);row->Add(rp,0,wxEXPAND);cam->Add(row,1,wxEXPAND);
 auto* st=C(camera,CARD);auto* sr=new wxBoxSizer(wxHORIZONTAL);sr->Add(T(st,U("СТАТИСТИКА"),9,true,MUTED),0,wxALIGN_CENTER_VERTICAL|wxLEFT,12);statsText=new wxStaticText(st,wxID_ANY,U("— × —   |   — fps   |   — Mbps"));statsText->SetForegroundColour(GREEN);sr->Add(statsText,0,wxALIGN_CENTER_VERTICAL|wxLEFT,16);sr->AddStretchSpacer();sr->Add(T(st,U("MyCam Pro Camera  •  Ready"),9,false,MUTED),0,wxALIGN_CENTER_VERTICAL|wxRIGHT,12);st->SetSizer(sr);cam->Add(st,0,wxEXPAND|wxTOP,10);

 auto page=[&](const wxString& title,const wxString& desc){auto*p=new wxPanel(pages,wxID_ANY);p->SetBackgroundColour(BG);auto*s=new wxBoxSizer(wxVERTICAL);auto*c=C(p);auto*cs2=new wxBoxSizer(wxVERTICAL);cs2->Add(T(c,title,20,true),0,wxALL,20);cs2->Add(T(c,desc,10,false,MUTED),0,wxLEFT|wxRIGHT|wxBOTTOM,20);auto*b=B(c,U("Открыть настройки"),wxID_ANY,true);cs2->Add(b,0,wxLEFT|wxRIGHT|wxBOTTOM,20);c->SetSizer(cs2);s->Add(c,0,wxEXPAND);p->SetSizer(s);return p;};
 auto* scenes=page(U("Сцены"),U("Профили камеры, качества, эффектов, микрофона и fallback."));
 auto* image=page(U("Изображение"),U("Яркость • Контраст • Насыщенность • Тени • Света • Резкость • WB • HDR • Стабилизация."));
 auto* effects=page(U("Эффекты"),U("Natural • Warm • Cool • Vivid • Cinematic • B&W • LUT .cube • Blur • Vignette • Grain."));
 auto* recording=page(U("Запись"),U("MP4 • аппаратное кодирование • Start / Pause / Resume / Stop • папка записей."));
 auto* audio=page(U("Аудио"),U("MyCam Pro Microphone • PC / Phone / Auto • Gain • AGC • Noise Reduction • A/V Sync."));
 auto* devicesPage=page(U("Устройства"),U("Ручное подключение. Wi‑Fi / USB / QR. Trusted devices. Автоматического переключения нет."));
 auto* settings=page(U("Настройки"),U("Тема • Производительность • Горячие клавиши • Fallback • Хранилище • Диагностика."));
 std::vector<wxPanel*> pp={camera,scenes,image,effects,recording,audio,devicesPage,settings};for(auto*x:pp)pgs->Add(x,1,wxEXPAND);for(size_t i=1;i<pp.size();i++)pp[i]->Hide();
 for(size_t i=0;i<nav.size();i++){nav[i]->Bind(wxEVT_BUTTON,[pages,pp,nav,i](wxCommandEvent&){for(size_t j=0;j<pp.size();j++){pp[j]->Show(i==j);nav[j]->SetBackgroundColour(i==j?wxColour(26,91,190):SURFACE);}pages->Layout();});}
}
void Window::InitializeTopBar(wxPanel*,wxBoxSizer*){}
void Window::InitializeCanvasPanel(wxPanel*,wxBoxSizer*){}
void Window::InitializeBottomBar(wxPanel*,wxBoxSizer*){}
void Window::SetConnectionStatus(bool connected,const wxString& deviceName){if(connected){statusText->SetLabel(U("●  Отлично   ")+deviceName+U("   ·   1080p60 · 8 Mbps · Wi‑Fi"));statusText->SetForegroundColour(GREEN);}else{statusText->SetLabel(U("●  Готово   Ожидание телефона   ·   Wi‑Fi"));statusText->SetForegroundColour(MUTED);}statusText->GetParent()->Layout();}
void Window::MinimizeToTaskbar(wxIconizeEvent&e){if(Settings::Get("MINIMIZE_TASKBAR")==1){Hide();e.Skip();}}
void Window::MaximizeFromTaskbar(wxTaskBarIconEvent&){Iconize(false);Show();Raise();SetFocus();}
Canvas* Window::GetCanvas(){return canvas;} wxChoice* Window::GetSourceChoice(){return sourceChoice;} wxButton* Window::GetStreamOptionsButton(){return streamOptionsButton;}
wxButton* Window::GetRotateLeftButton(){return rotateLeftButton;} wxButton* Window::GetRotateRightButton(){return rotateRightButton;} wxButton* Window::GetFlipButton(){return flipButton;} wxButton* Window::GetFlipVerticalButton(){return flipVerticalButton;}
wxButton* Window::GetZoomInButton(){return zoomInButton;} wxButton* Window::GetZoomOutButton(){return zoomOutButton;} wxStaticText* Window::GetZoomLevelLabel(){return zoomLevelLabel;}
wxButton* Window::GetTorchButton(){return torchButton;} wxButton* Window::GetSwapButton(){return swapButton;} wxButton* Window::GetAdjustmentsButton(){return adjustmentsButton;} wxButton* Window::GetSnapshotButton(){return snapshotButton;} wxStaticText* Window::GetStatsText(){return statsText;} wxTaskBarIcon* Window::GetTaskbarIcon(){return taskbarIcon;}
