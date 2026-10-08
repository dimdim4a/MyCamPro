#include "gui/window.h"
#include "settings.h"
#include <wx/settings.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/dcbuffer.h>
#include <functional>
#include <memory>
#include <vector>
#include <string>

namespace {
const wxColour BG(6,11,19), SURFACE(11,18,29), CARD(16,25,39), CARD2(21,32,49);
const wxColour BLUE(47,126,255), GREEN(45,220,132), TEXTC(242,246,252), MUTED(151,165,187);
wxString U(const char* s){return wxString::FromUTF8(s);}
wxStaticText* T(wxWindow* p,const wxString& s,int n=10,bool b=false,wxColour c=TEXTC){
 auto* x=new wxStaticText(p,wxID_ANY,s); auto f=x->GetFont(); f.SetPointSize(n); f.SetWeight(b?wxFONTWEIGHT_BOLD:wxFONTWEIGHT_NORMAL); x->SetFont(f); x->SetForegroundColour(c); return x;
}
wxPanel* C(wxWindow* p,wxColour c=CARD){auto* x=new wxPanel(p,wxID_ANY);x->SetBackgroundColour(c);return x;}
wxButton* B(wxWindow* p,const wxString& s,int id=wxID_ANY,bool primary=false){
 auto* x=new wxButton(p,id,s,wxDefaultPosition,wxDefaultSize,wxBORDER_NONE);x->SetBackgroundColour(primary?BLUE:CARD2);x->SetForegroundColour(TEXTC);x->SetMinSize(p->FromDIP(wxSize(primary?112:96,36)));return x;
}
class PreviewGrid : public wxPanel { public: PreviewGrid(wxWindow* p):wxPanel(p,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxTRANSPARENT_WINDOW|wxNO_BORDER){SetBackgroundStyle(wxBG_STYLE_PAINT);Bind(wxEVT_PAINT,&PreviewGrid::Paint,this);Hide();} void SetMode(int m){mode=m;Refresh();} private: int mode=0; void Paint(wxPaintEvent&){wxAutoBufferedPaintDC dc(this);dc.Clear();if(mode==0)return;int w=GetClientSize().x,h=GetClientSize().y;dc.SetPen(wxPen(wxColour(255,255,255,70),1));if(mode==1||mode==2){dc.DrawLine(w/3,0,w/3,h);dc.DrawLine(2*w/3,0,2*w/3,h);dc.DrawLine(0,h/3,w,h/3);dc.DrawLine(0,2*h/3,w,2*h/3);}if(mode==2){dc.SetPen(wxPen(wxColour(255,255,255,120),1));dc.DrawLine(w/2,0,w/2,h);dc.DrawLine(0,h/2,w,h/2);}if(mode==3){dc.DrawLine(0,0,w,h);dc.DrawLine(w,0,0,h);}if(mode==4){for(int i=1;i<4;i++){dc.DrawLine(i*w/4,0,i*w/4,h);dc.DrawLine(0,i*h/4,w,i*h/4);}}if(mode==5)dc.DrawRectangle(w/12,h/12,5*w/6,5*h/6);}};
wxButton* I(wxWindow* p,const wxString& f,const wxString& tip){
 wxImage im(wxString("res/")+f,wxBITMAP_TYPE_PNG); if(im.IsOk()){if(!im.HasAlpha())im.InitAlpha();auto*d=im.GetData();size_t n=(size_t)im.GetWidth()*im.GetHeight()*3;for(size_t i=0;i<n;i++)d[i]=255;}
 auto*x=new wxBitmapButton(p,wxID_ANY,im.IsOk()?wxBitmap(im):wxNullBitmap,wxDefaultPosition,p->FromDIP(wxSize(48,48)),wxBORDER_NONE);x->SetBackgroundColour(CARD2);x->SetToolTip(tip);return x;
}
}

Window::Window(Server::HostInfo hostinfo):wxFrame(nullptr,wxID_ANY,"MyCam Pro",wxDefaultPosition,wxDefaultSize,wxDEFAULT_FRAME_STYLE){
 wxString ep=wxStandardPaths::Get().GetExecutablePath(); wxFileName ef(ep); wxIcon icon(ef.GetPathWithSep()+wxString("res/nexora.ico"),wxBITMAP_TYPE_ICO);taskbarIcon=new wxTaskBarIcon();taskbarIcon->SetIcon(icon,"MyCam Pro");SetIcon(icon);
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
    auto* shell=new wxPanel(parent,wxID_ANY);
    shell->SetBackgroundColour(BG);
    auto* hs=new wxBoxSizer(wxHORIZONTAL);

    // Premium navigation rail
    auto* side=new wxPanel(shell,wxID_ANY);
    side->SetBackgroundColour(SURFACE);
    side->SetMinSize(FromDIP(wxSize(220,-1)));
    auto* ss=new wxBoxSizer(wxVERTICAL);

    auto* brand=new wxBoxSizer(wxHORIZONTAL);
    brand->Add(T(side,U("MY"),10,true,BLUE),0,wxALIGN_CENTER_VERTICAL|wxRIGHT,7);
    brand->Add(T(side,U("MyCam Pro"),16,true,TEXTC),0,wxALIGN_CENTER_VERTICAL);
    ss->Add(brand,0,wxALL,20);

    auto* sub=T(side,U("КАМЕРА • СТУДИЯ"),8,true,MUTED);
    ss->Add(sub,0,wxLEFT|wxRIGHT|wxBOTTOM,20);

    const char* names[]={"Камера","Сцены","Изображение","Эффекты","Запись","Аудио","Устройства","Настройки"};
    const char* icons[]={"◉","◇","◈","✦","●","♫","▣","⚙"};
    std::vector<wxButton*> nav;
    auto* ns=new wxBoxSizer(wxVERTICAL);
    for(int i=0;i<8;i++){
        auto* x=B(side,U((std::string(icons[i])+"   "+names[i]).c_str()),i==6?MenuIDs::DEVICES:wxID_ANY,i==0);
        x->SetMinSize(FromDIP(wxSize(190,44)));
        x->SetFont(wxFontInfo(10).FaceName("Segoe UI"));
        nav.push_back(x);
        ns->Add(x,0,wxEXPAND|wxBOTTOM,6);
    }
    ss->Add(ns,0,wxEXPAND|wxLEFT|wxRIGHT,14);
    ss->AddStretchSpacer();

    auto* vc=C(side,CARD2);
    auto* vcs=new wxBoxSizer(wxVERTICAL);
    vcs->Add(T(vc,U("●  MyCam Pro Camera"),10,true,GREEN),0,wxALL,13);
    vcs->Add(T(vc,U("Виртуальная камера"),9,false,MUTED),0,wxLEFT|wxRIGHT|wxBOTTOM,13);
    auto* obs=B(vc,U("Открыть в OBS"),wxID_ANY,true);
    obs->SetMinSize(FromDIP(wxSize(-1,38)));
    vcs->Add(obs,0,wxEXPAND|wxALL,10);
    vc->SetSizer(vcs);
    ss->Add(vc,0,wxEXPAND|wxALL,12);
    side->SetSizer(ss);
    hs->Add(side,0,wxEXPAND);

    // Main workspace
    auto* content=new wxPanel(shell,wxID_ANY);
    content->SetBackgroundColour(BG);
    auto* cs=new wxBoxSizer(wxVERTICAL);

    auto* head=new wxPanel(content,wxID_ANY);
    head->SetBackgroundColour(BG);
    auto* hr=new wxBoxSizer(wxHORIZONTAL);
    hr->Add(T(head,U("КАМЕРА"),18,true,TEXTC),0,wxALIGN_CENTER_VERTICAL|wxLEFT,18);
    hr->AddStretchSpacer();

    statusText=new wxStaticText(head,wxID_ANY,U("●  Готово   •   Ожидание телефона   •   Wi‑Fi"),
                                wxDefaultPosition,FromDIP(wxSize(340,34)),wxALIGN_CENTER);
    statusText->SetBackgroundColour(CARD);
    statusText->SetForegroundColour(MUTED);
    statusText->SetFont(wxFontInfo(9).FaceName("Segoe UI"));
    hr->Add(statusText,0,wxALIGN_CENTER_VERTICAL|wxRIGHT,8);
    auto* qr=B(head,U("QR  ПОДКЛЮЧЕНИЕ"),MenuIDs::QR,true);
    qr->SetMinSize(FromDIP(wxSize(145,36)));
    hr->Add(qr,0,wxALIGN_CENTER_VERTICAL|wxRIGHT,7);
    auto* devices=B(head,U("УСТРОЙСТВА"),MenuIDs::DEVICES);
    devices->SetMinSize(FromDIP(wxSize(118,36)));
    hr->Add(devices,0,wxALIGN_CENTER_VERTICAL|wxRIGHT,12);
    head->SetSizer(hr);
    cs->Add(head,0,wxEXPAND|wxTOP|wxBOTTOM,10);

    auto* pages=new wxPanel(content,wxID_ANY);
    pages->SetBackgroundColour(BG);
    auto* pgs=new wxBoxSizer(wxVERTICAL);
    pages->SetSizer(pgs);
    cs->Add(pages,1,wxEXPAND);
    content->SetSizer(cs);
    hs->Add(content,1,wxEXPAND);

    auto* camera=new wxPanel(pages,wxID_ANY);
    camera->SetBackgroundColour(BG);
    auto* cam=new wxBoxSizer(wxVERTICAL);
    camera->SetSizer(cam);

    auto* row=new wxBoxSizer(wxHORIZONTAL);
    auto* pv=C(camera,SURFACE);
    auto* pvs=new wxBoxSizer(wxVERTICAL);

    auto* pm=new wxBoxSizer(wxHORIZONTAL);
    pm->Add(T(pv,U("ПРЯМОЙ ЭФИР"),9,true,TEXTC),0,wxALIGN_CENTER_VERTICAL|wxRIGHT,10);
    pm->Add(T(pv,U("1920 × 1080  •  60 FPS  •  H.264"),9,false,MUTED),0,wxALIGN_CENTER_VERTICAL);
    auto* live=T(pv,U("● LIVE"),9,true,GREEN);
    pm->AddStretchSpacer();
    pm->Add(live,0,wxALIGN_CENTER_VERTICAL|wxRIGHT,12);
    auto* gridChoice=new wxChoice(pv,wxID_ANY);
    for(const auto& v:{U("Сетка: выкл."),U("Третьи"),U("Центр"),U("Диагонали"),U("Золотое сечение"),U("4 × 4"),U("Безопасная зона")}) gridChoice->Append(v);
    gridChoice->SetSelection(0);
    gridChoice->SetMinSize(FromDIP(wxSize(150,30)));
    pm->Add(gridChoice,0,wxALIGN_CENTER_VERTICAL);
    pvs->Add(pm,0,wxEXPAND|wxALL,13);

    canvas=new Canvas(pv,wxDefaultPosition,FromDIP(wxSize(800,500)));
    canvas->SetMinSize(FromDIP(wxSize(520,330)));
    canvas->SetBackgroundColour(wxColour(2,5,10));
    auto* grid=new PreviewGrid(canvas);
    grid->SetPosition(wxPoint(0,0));
    grid->SetSize(canvas->GetClientSize());
    canvas->Bind(wxEVT_SIZE,[grid](wxSizeEvent& e){grid->SetSize(e.GetSize());e.Skip();});
    gridChoice->Bind(wxEVT_CHOICE,[grid](wxCommandEvent& e){grid->SetMode(e.GetSelection());grid->Show(e.GetSelection()!=0);grid->Raise();});
    pvs->Add(canvas,1,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,13);

    auto* q=new wxBoxSizer(wxHORIZONTAL);
    snapshotButton=I(pv,"photo.png",U("Снимок"));
    torchButton=I(pv,"flash.png",U("Вспышка"));
    swapButton=I(pv,"swap.png",U("Сменить камеру"));
    adjustmentsButton=I(pv,"settings.png",U("Изображение"));
    rotateLeftButton=I(pv,"rotate-left.png",U("Повернуть влево"));
    rotateRightButton=I(pv,"rotate-right.png",U("Повернуть вправо"));
    flipButton=I(pv,"flip.png",U("Зеркально"));
    flipVerticalButton=I(pv,"flip-v.png",U("Перевернуть"));
    zoomOutButton=I(pv,"zoom-out.png",U("Уменьшить"));
    zoomInButton=I(pv,"zoom-in.png",U("Увеличить"));
    for(auto*x:{snapshotButton,torchButton,swapButton,adjustmentsButton}) q->Add(x,0,wxRIGHT,6);
    q->AddStretchSpacer();
    for(auto*x:{rotateLeftButton,rotateRightButton,flipButton,flipVerticalButton,zoomOutButton}) q->Add(x,0,wxRIGHT,5);
    zoomLevelLabel=T(pv,U("1.0×"),10,true,TEXTC);
    q->Add(zoomLevelLabel,0,wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT,6);
    q->Add(zoomInButton);
    pvs->Add(q,0,wxLEFT|wxRIGHT|wxBOTTOM,13);
    pv->SetSizer(pvs);
    row->Add(pv,1,wxEXPAND|wxRIGHT,10);

    // Contextual right panel
    auto* rp=C(camera,CARD);
    auto* rs=new wxBoxSizer(wxVERTICAL);
    auto* rt=new wxBoxSizer(wxHORIZONTAL);
    rt->Add(T(rp,U("Камера"),17,true,TEXTC),0,wxALIGN_CENTER_VERTICAL);
    rt->AddStretchSpacer();
    auto* hideRight=B(rp,U("‹"));
    hideRight->SetToolTip(U("Скрыть панель"));
    hideRight->SetMinSize(FromDIP(wxSize(38,34)));
    rt->Add(hideRight);
    rs->Add(rt,0,wxEXPAND|wxALL,14);

    rs->Add(T(rp,U("ИСТОЧНИК"),8,true,MUTED),0,wxLEFT|wxRIGHT,14);
    wxArrayString ch; ch.Add(U("Устройства не найдены"));
    sourceChoice=new wxChoice(rp,wxID_ANY,wxDefaultPosition,wxDefaultSize,ch);
    sourceChoice->SetMinSize(FromDIP(wxSize(250,36)));
    sourceChoice->SetSelection(0);
    rs->Add(sourceChoice,0,wxEXPAND|wxALL,9);

    auto add=[&](const char*a,const char*b){
        rs->Add(T(rp,U(a),8,true,MUTED),0,wxLEFT|wxRIGHT|wxTOP,12);
        auto*x=new wxChoice(rp,wxID_ANY);
        x->Append(U(b)); x->SetSelection(0);
        x->SetMinSize(FromDIP(wxSize(250,36)));
        rs->Add(x,0,wxEXPAND|wxLEFT|wxRIGHT|wxTOP,6);
    };
    add("РАЗРЕШЕНИЕ","1920 × 1080");
    add("ЧАСТОТА КАДРОВ","60 FPS");
    add("БИТРЕЙТ","8 Mbps");
    add("ПРОФИЛЬ","Сбалансированный");

    streamOptionsButton=B(rp,U("⚙  Настройки потока"));
    streamOptionsButton->SetMinSize(FromDIP(wxSize(-1,38)));
    rs->Add(streamOptionsButton,0,wxEXPAND|wxALL,14);

    auto* modes=C(rp,CARD2);
    auto* ms=new wxBoxSizer(wxVERTICAL);
    ms->Add(T(modes,U("РЕЖИМ"),8,true,MUTED),0,wxALL,11);
    ms->Add(T(modes,U("Стабильный"),10,true,TEXTC),0,wxLEFT|wxRIGHT,11);
    ms->Add(T(modes,U("FPS  →  задержка  →  качество"),8,false,MUTED),0,wxALL,11);
    modes->SetSizer(ms);
    rs->Add(modes,0,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,12);
    rs->AddStretchSpacer();
    rp->SetSizer(rs);
    row->Add(rp,0,wxEXPAND);

    auto* restoreRight=B(camera,U("›"));
    restoreRight->SetToolTip(U("Показать панель"));
    restoreRight->Hide();

    cam->Add(row,1,wxEXPAND);

    auto* st=C(camera,CARD);
    auto* sr=new wxBoxSizer(wxHORIZONTAL);
    sr->Add(T(st,U("СТАТИСТИКА"),8,true,MUTED),0,wxALIGN_CENTER_VERTICAL|wxLEFT,13);
    statsText=new wxStaticText(st,wxID_ANY,U("— × —    •    — FPS    •    — Mbps    •    — ms"));
    statsText->SetForegroundColour(GREEN);
    statsText->SetFont(wxFontInfo(9).FaceName("Segoe UI"));
    sr->Add(statsText,0,wxALIGN_CENTER_VERTICAL|wxLEFT,16);
    sr->AddStretchSpacer();
    sr->Add(T(st,U("MyCam Pro Camera  •  Готово"),9,false,MUTED),0,wxALIGN_CENTER_VERTICAL|wxRIGHT,13);
    st->SetSizer(sr);
    cam->Add(st,0,wxEXPAND|wxTOP,9);

    auto page=[&](const wxString& title,const wxString& desc){
        auto*p=new wxPanel(pages,wxID_ANY); p->SetBackgroundColour(BG);
        auto*s=new wxBoxSizer(wxVERTICAL);
        auto*c=C(p,CARD); auto*cs2=new wxBoxSizer(wxVERTICAL);
        cs2->Add(T(c,title,22,true,TEXTC),0,wxALL,22);
        cs2->Add(T(c,desc,10,false,MUTED),0,wxLEFT|wxRIGHT|wxBOTTOM,22);
        auto*b=B(c,U("Открыть настройки"),wxID_ANY,true);
        cs2->Add(b,0,wxLEFT|wxRIGHT|wxBOTTOM,22);
        c->SetSizer(cs2); s->Add(c,0,wxEXPAND); p->SetSizer(s); return p;
    };

    auto* scenes=page(U("Сцены"),U("Профили камеры, качества, эффектов, микрофона и fallback."));
    auto* image=page(U("Изображение"),U("Яркость • Контраст • Насыщенность • Тени • Света • Резкость • Баланс белого • HDR."));
    auto* effects=page(U("Эффекты"),U("Natural • Warm • Cool • Vivid • Cinematic • B&W • LUT • Blur • Vignette • Grain."));
    auto* recording=page(U("Запись"),U("MP4 • аппаратное кодирование • запуск • пауза • продолжение • остановка."));
    auto* audio=page(U("Аудио"),U("MyCam Pro Microphone • ПК / Телефон / Авто • усиление • AGC • шумоподавление • A/V Sync."));
    auto* devicesPage=page(U("Устройства"),U("Ручное подключение • Wi‑Fi • USB • QR • доверенные устройства."));
    auto* settings=page(U("Настройки"),U("Тема • производительность • горячие клавиши • fallback • хранилище • диагностика."));

    std::vector<wxPanel*> pp={camera,scenes,image,effects,recording,audio,devicesPage,settings};
    for(auto*x:pp) pgs->Add(x,1,wxEXPAND);
    for(size_t i=1;i<pp.size();i++) pp[i]->Hide();

    for(size_t i=0;i<nav.size();i++){
        nav[i]->Bind(wxEVT_BUTTON,[pages,pp,nav,i](wxCommandEvent&){
            for(size_t j=0;j<pp.size();j++){
                pp[j]->Show(i==j);
                nav[j]->SetBackgroundColour(i==j?wxColour(42,83,165):SURFACE);
                nav[j]->SetForegroundColour(TEXTC);
            }
            pages->Layout();
        });
    }

    hideRight->Bind(wxEVT_BUTTON,[rp,restoreRight](wxCommandEvent&){rp->Hide();restoreRight->Show();rp->GetParent()->Layout();});
    restoreRight->Bind(wxEVT_BUTTON,[rp,restoreRight](wxCommandEvent&){restoreRight->Hide();rp->Show();rp->GetParent()->Layout();});
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
