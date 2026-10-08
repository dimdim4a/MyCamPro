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
const wxColour BG(248,249,253), SURFACE(255,255,255), CARD(255,255,255), CARD2(246,247,251), BORDER(226,230,240);
const wxColour BLUE(91,82,245), GREEN(26,184,112), RED(255,72,91), TEXTC(24,29,45), MUTED(105,114,135);
wxString U(const char* s){return wxString::FromUTF8(s);}
wxStaticText* T(wxWindow* p,const wxString& s,int n=10,bool b=false,wxColour c=TEXTC){
 auto* x=new wxStaticText(p,wxID_ANY,s); auto f=x->GetFont(); f.SetPointSize(n); f.SetWeight(b?wxFONTWEIGHT_BOLD:wxFONTWEIGHT_NORMAL); x->SetFont(f); x->SetForegroundColour(c); return x;
}
wxPanel* C(wxWindow* p,wxColour c=CARD){auto* x=new wxPanel(p,wxID_ANY);x->SetBackgroundColour(c);return x;}
wxBitmap MakeButtonBitmap(const wxString& label, const wxSize& size, bool primary){
 wxBitmap bmp(size.x,size.y,32); wxMemoryDC dc(bmp);
 dc.SetBackground(wxBrush(primary?BLUE:SURFACE)); dc.Clear();
 dc.SetPen(wxPen(primary?BLUE:BORDER,1)); if(!primary) dc.DrawRoundedRectangle(0,0,size.x-1,size.y-1,10);
 dc.SetTextForeground(primary?*wxWHITE:TEXTC);
 wxFont f=wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT); f.SetPointSize(9); f.SetWeight(wxFONTWEIGHT_BOLD); dc.SetFont(f);
 int tw=0,th=0; dc.GetTextExtent(label,&tw,&th); dc.DrawText(label,(size.x-tw)/2,(size.y-th)/2);
 dc.SelectObject(wxNullBitmap); return bmp;
}
wxButton* B(wxWindow* p,const wxString& s,int id=wxID_ANY,bool primary=false){
 auto sz=p->FromDIP(wxSize(primary?124:112,38));
 auto* x=new wxBitmapButton(p,id,MakeButtonBitmap(s,sz,primary),wxDefaultPosition,sz,wxBORDER_NONE);
 x->SetBackgroundColour(primary?BLUE:SURFACE); x->SetMinSize(sz); x->SetToolTip(s); return x;
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
 SetSizer(new wxBoxSizer(wxVERTICAL));GetSizer()->Add(root,1,wxEXPAND);SetMinClientSize(FromDIP(wxSize(1120,720)));SetClientSize(FromDIP(wxSize(1500,900)));Layout();root->Layout();CallAfter([this,root](){Layout();root->Layout();root->Refresh();});Center();
}
Window::~Window(){delete taskbarIcon;}

void Window::InitializeMenu(Server::HostInfo hostinfo){
 auto* mb=new wxMenuBar();auto* m=new wxMenu();m->AppendCheckItem(MenuIDs::HIDE2TRAY,"Minimize to tray");m->AppendCheckItem(MenuIDs::SHOWSTATS,"Show stream statistics");m->AppendCheckItem(MenuIDs::SAVESTATE,"Remember device settings");
 auto* r=new wxMenu();r->AppendRadioItem(MenuIDs::DS_SD,"640 x 480");r->AppendRadioItem(MenuIDs::DS_HD,"1280 x 720");r->AppendRadioItem(MenuIDs::DS_FHD,"1920 x 1080");r->AppendRadioItem(MenuIDs::DS_QHD,"3840 x 2160");m->AppendSubMenu(r,"Virtual camera resolution");m->Append(wxID_EXIT,"Exit");mb->Append(m,"MyCam Pro");
 auto* c=new wxMenu();c->Append(MenuIDs::QR,"Show QR code");c->Append(MenuIDs::DEVICES,"Connected devices");c->AppendSeparator();c->Append(wxID_ANY,"Address: "+std::get<1>(hostinfo));c->Append(wxID_ANY,"Port: "+std::get<2>(hostinfo));mb->Append(c,"Connection");SetMenuBar(mb);
}

void Window::InitializeHeader(wxPanel* parent,wxBoxSizer* topsizer){
 auto* shell=new wxPanel(parent,wxID_ANY);shell->SetBackgroundColour(BG);auto* hs=new wxBoxSizer(wxHORIZONTAL);
 auto* side=new wxPanel(shell,wxID_ANY);side->SetBackgroundColour(SURFACE);side->SetMinSize(FromDIP(wxSize(250,-1)));auto* ss=new wxBoxSizer(wxVERTICAL);
 auto* brand=new wxBoxSizer(wxHORIZONTAL);brand->Add(T(side,U("◉"),17,true,BLUE),0,wxALIGN_CENTER_VERTICAL|wxRIGHT,8);brand->Add(T(side,U("MyCam"),18,true,TEXTC),0,wxALIGN_CENTER_VERTICAL);brand->Add(T(side,U(" Pro"),18,true,BLUE),0,wxALIGN_CENTER_VERTICAL);ss->Add(brand,0,wxLEFT|wxTOP,22);ss->Add(T(side,U("ПРОФЕССИОНАЛЬНАЯ КАМЕРА"),8,true,MUTED),0,wxLEFT|wxTOP|wxBOTTOM,7);
 const char* labels[]={"Камера","Сцены","Эффекты","Запись","Виртуальная камера","Устройства","Настройки"};const char* icons[]={"●","▣","✦","▶","▣","▤","⚙"};std::vector<wxButton*> nav;
 for(int i=0;i<7;i++){auto*n=B(side,U((std::string(icons[i])+"   "+labels[i]).c_str()),i==5?MenuIDs::DEVICES:wxID_ANY,i==0);n->SetMinSize(FromDIP(wxSize(220,48)));n->SetFont(wxFontInfo(10).FaceName("Segoe UI"));nav.push_back(n);ss->Add(n,0,wxEXPAND|wxLEFT|wxRIGHT|wxTOP,12);}
 ss->AddStretchSpacer();
 auto* device=C(side,wxColour(248,249,253));auto* ds=new wxBoxSizer(wxHORIZONTAL);ds->Add(T(device,U("▯"),25,true,TEXTC),0,wxALIGN_CENTER_VERTICAL|wxALL,10);auto* dt=new wxBoxSizer(wxVERTICAL);dt->Add(T(device,U("Xiaomi 14"),11,true,TEXTC),0);dt->Add(T(device,U("●  Подключено"),9,true,GREEN),0);dt->Add(T(device,U("🔋 87%  •  Wi‑Fi"),8,false,MUTED),0);ds->Add(dt,1,wxALIGN_CENTER_VERTICAL);ds->Add(T(device,U("›"),20,false,MUTED),0,wxALIGN_CENTER_VERTICAL|wxRIGHT,8);device->SetSizer(ds);ss->Add(device,0,wxEXPAND|wxALL,12);
 auto* pro=C(side,wxColour(245,243,255));auto* ps=new wxBoxSizer(wxVERTICAL);ps->Add(T(pro,U("♛  MyCam Pro"),11,true,TEXTC),0,wxALL,12);ps->Add(T(pro,U("Больше возможностей для контента"),8,false,MUTED),0,wxLEFT|wxRIGHT|wxBOTTOM,12);auto* upgrade=B(pro,U("Обновить до Pro"),wxID_ANY,true);upgrade->SetMinSize(FromDIP(wxSize(-1,36)));ps->Add(upgrade,0,wxEXPAND|wxALL,9);pro->SetSizer(ps);ss->Add(pro,0,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,12);side->SetSizer(ss);hs->Add(side,0,wxEXPAND);

 auto* content=new wxPanel(shell,wxID_ANY);content->SetBackgroundColour(BG);auto* cs=new wxBoxSizer(wxVERTICAL);auto* head=new wxPanel(content,wxID_ANY);head->SetBackgroundColour(BG);auto* hr=new wxBoxSizer(wxHORIZONTAL);
 auto* dev=C(head,SURFACE);auto*d=new wxBoxSizer(wxHORIZONTAL);d->Add(T(dev,U("▯"),19,true,BLUE),0,wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT,12);auto*dtx=new wxBoxSizer(wxVERTICAL);dtx->Add(T(dev,U("Xiaomi 14"),10,true,TEXTC),0);dtx->Add(T(dev,U("●  Подключено"),8,true,GREEN),0);d->Add(dtx,0,wxALIGN_CENTER_VERTICAL);d->Add(T(dev,U("⌄"),13,false,MUTED),0,wxALIGN_CENTER_VERTICAL|wxLEFT,18);dev->SetSizer(d);dev->SetMinSize(FromDIP(wxSize(205,52)));hr->Add(dev,0,wxALIGN_CENTER_VERTICAL|wxLEFT,12);
 auto metric=[&](const wxString&a,const wxString&b){auto*p=C(head,SURFACE);auto*z=new wxBoxSizer(wxVERTICAL);z->Add(T(p,a,10,true,TEXTC),0,wxALIGN_CENTER|wxTOP,6);z->Add(T(p,b,7,false,MUTED),0,wxALIGN_CENTER|wxBOTTOM,6);p->SetSizer(z);p->SetMinSize(FromDIP(wxSize(95,52)));return p;};hr->Add(metric(U("▣ 1080p"),U("Full HD")),0,wxLEFT,7);hr->Add(metric(U("◉ 60 FPS"),U("Частота")),0,wxLEFT,5);hr->Add(metric(U("◌ 8.4 Mbps"),U("Битрейт")),0,wxLEFT,5);hr->Add(metric(U("⌁ Wi‑Fi"),U("28 ms")),0,wxLEFT,5);hr->AddStretchSpacer();auto*gear=B(head,U("⚙"),wxID_ANY);gear->SetMinSize(FromDIP(wxSize(42,42)));hr->Add(gear,0,wxALIGN_CENTER_VERTICAL|wxRIGHT,12);head->SetSizer(hr);cs->Add(head,0,wxEXPAND|wxTOP|wxBOTTOM,8);
 auto* pages=new wxPanel(content,wxID_ANY);pages->SetBackgroundColour(BG);auto*pgs=new wxBoxSizer(wxVERTICAL);pages->SetSizer(pgs);cs->Add(pages,1,wxEXPAND);content->SetSizer(cs);hs->Add(content,1,wxEXPAND);

 auto* camera=new wxPanel(pages,wxID_ANY);camera->SetBackgroundColour(BG);auto*cam=new wxBoxSizer(wxVERTICAL);camera->SetSizer(cam);auto*row=new wxBoxSizer(wxHORIZONTAL);auto*center=new wxPanel(camera,wxID_ANY);center->SetBackgroundColour(BG);auto*cz=new wxBoxSizer(wxVERTICAL);
 auto*pv=C(center,SURFACE);auto*pvs=new wxBoxSizer(wxVERTICAL);auto*ph=new wxBoxSizer(wxHORIZONTAL);statusText=new wxStaticText(pv,wxID_ANY,U("●  LIVE  •  Xiaomi 14  •  1080p  •  60 FPS"));statusText->SetForegroundColour(GREEN);statusText->SetFont(wxFontInfo(9).FaceName("Segoe UI"));ph->Add(statusText,0,wxALIGN_CENTER_VERTICAL|wxLEFT,12);ph->AddStretchSpacer();auto*expand=B(pv,U("⛶"),wxID_ANY);expand->SetMinSize(FromDIP(wxSize(38,30)));ph->Add(expand,0,wxRIGHT,8);pvs->Add(ph,0,wxEXPAND|wxTOP|wxBOTTOM,7);
 canvas=new Canvas(pv,wxDefaultPosition,FromDIP(wxSize(800,470)));canvas->SetMinSize(FromDIP(wxSize(520,320)));canvas->SetBackgroundColour(wxColour(230,233,240));auto*grid=new PreviewGrid(canvas);grid->SetPosition(wxPoint(0,0));grid->SetSize(canvas->GetClientSize());canvas->Bind(wxEVT_SIZE,[grid](wxSizeEvent&e){grid->SetSize(e.GetSize());e.Skip();});pvs->Add(canvas,1,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,10);
 auto*quick=new wxBoxSizer(wxHORIZONTAL);auto q=[&](const wxString&a,const wxString&b){auto*p=C(pv,wxColour(248,249,253));auto*z=new wxBoxSizer(wxVERTICAL);z->Add(T(p,a,8,false,MUTED),0,wxALIGN_CENTER|wxTOP,6);z->Add(T(p,b,9,true,TEXTC),0,wxALIGN_CENTER|wxBOTTOM,6);p->SetSizer(z);p->SetMinSize(FromDIP(wxSize(84,55)));return p;};quick->Add(q(U("↻ Поворот"),U("0°")),0,wxRIGHT,6);quick->Add(q(U("▣ Зеркало"),U("Вкл")),0,wxRIGHT,6);quick->Add(q(U("✦ Стабилизация"),U("Вкл")),0,wxRIGHT,6);quick->Add(q(U("HDR"),U("Выкл")),0,wxRIGHT,6);quick->Add(q(U("□ Сетка"),U("Выкл")),0,wxRIGHT,6);quick->Add(q(U("ϟ Фонарик"),U("Выкл")),0,wxRIGHT,6);pvs->Add(quick,0,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,9);pv->SetSizer(pvs);cz->Add(pv,1,wxEXPAND);
 auto*capture=C(center,SURFACE);auto*cr=new wxBoxSizer(wxHORIZONTAL);snapshotButton=B(capture,U("▣  Сделать снимок"),wxID_ANY,false);snapshotButton->SetMinSize(FromDIP(wxSize(170,46)));cr->Add(snapshotButton,0,wxRIGHT,7);cr->AddStretchSpacer();auto*record=B(capture,U("●  Начать запись"),wxID_ANY,true);record->SetMinSize(FromDIP(wxSize(210,46)));cr->Add(record,0,wxRIGHT,7);auto*mic=B(capture,U("♩  Микрофон  ⌄"),wxID_ANY,false);mic->SetMinSize(FromDIP(wxSize(165,46)));cr->Add(mic,0,wxRIGHT,9);capture->SetSizer(cr);cz->Add(capture,0,wxEXPAND|wxTOP,8);
 auto*scene=C(center,SURFACE);auto*scs=new wxBoxSizer(wxVERTICAL);auto*sch=new wxBoxSizer(wxHORIZONTAL);sch->Add(T(scene,U("Сцены"),13,true,TEXTC),0,wxALIGN_CENTER_VERTICAL|wxLEFT,12);auto*plus=B(scene,U("+"),wxID_ANY,false);plus->SetMinSize(FromDIP(wxSize(36,32)));sch->Add(plus,0,wxLEFT,9);sch->AddStretchSpacer();scs->Add(sch,0,wxEXPAND|wxTOP,7);auto*sr=new wxBoxSizer(wxHORIZONTAL);const wxString sn[]={U("Основная"),U("Стрим"),U("Рабочий стол"),U("Игра"),U("Интервью")};for(int i=0;i<5;i++){auto*p=C(scene,i==0?wxColour(239,240,255):wxColour(248,249,253));auto*z=new wxBoxSizer(wxVERTICAL);auto*t=C(p,i==0?wxColour(224,228,255):wxColour(231,234,241));t->SetMinSize(FromDIP(wxSize(130,58)));t->SetSizer(new wxBoxSizer(wxVERTICAL));t->GetSizer()->Add(T(t,i==0?U("● LIVE"):U("▣"),14,true,i==0?BLUE:MUTED),1,wxALIGN_CENTER);z->Add(t,0,wxEXPAND);z->Add(T(p,sn[i],8,true,TEXTC),0,wxALL,6);p->SetSizer(z);p->SetMinSize(FromDIP(wxSize(130,82)));sr->Add(p,1,wxRIGHT,6);}auto*ns=C(scene,wxColour(249,250,253));auto*nz=new wxBoxSizer(wxVERTICAL);nz->Add(T(ns,U("+"),20,true,BLUE),1,wxALIGN_CENTER);nz->Add(T(ns,U("Новая сцена"),8,false,MUTED),0,wxALIGN_CENTER|wxBOTTOM,7);ns->SetSizer(nz);ns->SetMinSize(FromDIP(wxSize(105,82)));sr->Add(ns,0,wxRIGHT,9);scs->Add(sr,0,wxEXPAND|wxALL,9);scene->SetSizer(scs);cz->Add(scene,0,wxEXPAND|wxTOP,8);center->SetSizer(cz);row->Add(center,1,wxEXPAND|wxRIGHT,9);

 auto*rp=C(camera,SURFACE);rp->SetMinSize(FromDIP(wxSize(335,-1)));auto*rs=new wxBoxSizer(wxVERTICAL);auto*rt=new wxBoxSizer(wxHORIZONTAL);rt->Add(T(rp,U("Настройки камеры"),14,true,TEXTC),0,wxALIGN_CENTER_VERTICAL|wxLEFT,13);rt->AddStretchSpacer();auto*reset=B(rp,U("Сбросить"),wxID_ANY,false);reset->SetMinSize(FromDIP(wxSize(82,30)));rt->Add(reset,0,wxRIGHT,8);rs->Add(rt,0,wxEXPAND|wxTOP|wxBOTTOM,10);
 auto addChoice=[&](const wxString&l,const wxString&v){rs->Add(T(rp,l,8,false,MUTED),0,wxLEFT|wxRIGHT|wxTOP,7);auto*x=new wxChoice(rp,wxID_ANY);x->Append(v);x->SetSelection(0);x->SetMinSize(FromDIP(wxSize(-1,36)));rs->Add(x,0,wxEXPAND|wxLEFT|wxRIGHT|wxTOP,5);};
 rs->Add(T(rp,U("Источник"),8,false,MUTED),0,wxLEFT|wxRIGHT,13);wxArrayString ch;ch.Add(U("Xiaomi 14 (Wi‑Fi)"));ch.Add(U("Камера ПК"));sourceChoice=new wxChoice(rp,wxID_ANY,wxDefaultPosition,wxDefaultSize,ch);sourceChoice->SetSelection(0);sourceChoice->SetMinSize(FromDIP(wxSize(-1,36)));rs->Add(sourceChoice,0,wxEXPAND|wxALL,7);addChoice(U("Разрешение"),U("1920 × 1080 (Full HD)"));addChoice(U("FPS"),U("60"));
 rs->Add(T(rp,U("Битрейт (Mbps)"),8,false,MUTED),0,wxLEFT|wxRIGHT|wxTOP,7);auto*bitrate=new wxSlider(rp,wxID_ANY,84,10,120);rs->Add(bitrate,0,wxEXPAND|wxLEFT|wxRIGHT,9);rs->Add(T(rp,U("8.4 Mbps"),8,true,TEXTC),0,wxALIGN_RIGHT|wxRIGHT,10);rs->Add(T(rp,U("Экспозиция"),8,false,MUTED),0,wxLEFT|wxRIGHT|wxTOP,7);auto*exposure=new wxSlider(rp,wxID_ANY,50,0,100);rs->Add(exposure,0,wxEXPAND|wxLEFT|wxRIGHT,9);rs->Add(T(rp,U("0"),8,true,TEXTC),0,wxALIGN_RIGHT|wxRIGHT,10);addChoice(U("Баланс белого"),U("Авто"));addChoice(U("Фокус"),U("Авто"));
 streamOptionsButton=B(rp,U("Дополнительно  ⌄"),wxID_ANY,false);streamOptionsButton->SetMinSize(FromDIP(wxSize(-1,38)));rs->Add(streamOptionsButton,0,wxEXPAND|wxALL,9);
 auto*vc=C(rp,wxColour(245,247,253));auto*vcs=new wxBoxSizer(wxVERTICAL);vcs->Add(T(vc,U("●  Виртуальная камера"),10,true,TEXTC),0,wxALL,10);vcs->Add(T(vc,U("▯  MyCam Pro Virtual Camera"),9,false,TEXTC),0,wxLEFT|wxRIGHT|wxBOTTOM,8);vcs->Add(T(vc,U("●  Готово к использованию"),8,true,GREEN),0,wxLEFT|wxRIGHT|wxBOTTOM,10);vc->SetSizer(vcs);rs->Add(vc,0,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,9);
 auto*audio=C(rp,wxColour(248,249,253));auto*aus=new wxBoxSizer(wxVERTICAL);aus->Add(T(audio,U("♩  Аудио"),10,true,TEXTC),0,wxALL,9);auto*micC=new wxChoice(audio,wxID_ANY);micC->Append(U("Микрофон (Xiaomi 14)"));micC->SetSelection(0);aus->Add(micC,0,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,7);auto*vol=new wxSlider(audio,wxID_ANY,88,0,100);aus->Add(vol,0,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,7);audio->SetSizer(aus);rs->Add(audio,0,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,9);rp->SetSizer(rs);row->Add(rp,0,wxEXPAND);cam->Add(row,1,wxEXPAND);
 auto*bottom=C(camera,SURFACE);auto*bs=new wxBoxSizer(wxHORIZONTAL);bs->Add(T(bottom,U("CPU 12%"),8,false,MUTED),0,wxALIGN_CENTER_VERTICAL|wxLEFT,12);bs->Add(T(bottom,U("RAM 846 MB"),8,false,MUTED),0,wxALIGN_CENTER_VERTICAL|wxLEFT,18);bs->Add(T(bottom,U("FPS 60"),8,false,MUTED),0,wxALIGN_CENTER_VERTICAL|wxLEFT,18);bs->Add(T(bottom,U("Задержка 28 ms"),8,false,MUTED),0,wxALIGN_CENTER_VERTICAL|wxLEFT,18);bs->Add(T(bottom,U("Поток 8.4 Mbps"),8,false,MUTED),0,wxALIGN_CENTER_VERTICAL|wxLEFT,18);bs->AddStretchSpacer();bs->Add(T(bottom,U("● Xiaomi 14 подключено"),8,true,GREEN),0,wxALIGN_CENTER_VERTICAL|wxRIGHT,14);bs->Add(T(bottom,U("v1.0.0"),8,false,MUTED),0,wxALIGN_CENTER_VERTICAL|wxRIGHT,10);bottom->SetSizer(bs);cam->Add(bottom,0,wxEXPAND|wxTOP,7);

 auto page=[&](const wxString&t,const wxString&d){auto*p=new wxPanel(pages,wxID_ANY);p->SetBackgroundColour(BG);auto*s=new wxBoxSizer(wxVERTICAL);auto*c=C(p,SURFACE);auto*z=new wxBoxSizer(wxVERTICAL);z->Add(T(c,t,22,true,TEXTC),0,wxALL,22);z->Add(T(c,d,10,false,MUTED),0,wxLEFT|wxRIGHT|wxBOTTOM,22);auto*x=B(c,U("Открыть настройки"),wxID_ANY,true);z->Add(x,0,wxLEFT|wxRIGHT|wxBOTTOM,22);c->SetSizer(z);s->Add(c,0,wxEXPAND);p->SetSizer(s);return p;};
 auto*scenes=page(U("Сцены"),U("Управление готовыми сценами и быстрым переключением."));auto*effects=page(U("Эффекты"),U("Фильтры, цветокоррекция, HDR, резкость и размытие."));auto*recording=page(U("Запись"),U("Качество, кодек, путь сохранения и горячие клавиши."));auto*virtualCamera=page(U("Виртуальная камера"),U("MyCam Pro Virtual Camera для OBS, Zoom, Discord и других приложений."));auto*devices=page(U("Устройства"),U("Wi‑Fi / USB / QR, подключенные телефоны и доверенные устройства."));auto*settings=page(U("Настройки"),U("Тема, производительность, хранилище, уведомления и диагностика."));
    {
        auto* themeCard=C(settings,SURFACE); auto* ts=new wxBoxSizer(wxHORIZONTAL);
        ts->Add(T(themeCard,U("Тема"),11,true,TEXTC),0,wxALIGN_CENTER_VERTICAL|wxLEFT,16);
        ts->Add(T(themeCard,U("Светлая / Тёмная / Системная"),9,false,MUTED),0,wxALIGN_CENTER_VERTICAL|wxLEFT,12);
        ts->AddStretchSpacer();
        auto* theme=new wxChoice(themeCard,wxID_ANY); theme->Append(U("Светлая")); theme->Append(U("Тёмная")); theme->Append(U("Системная")); theme->SetSelection(0);
        theme->SetMinSize(FromDIP(wxSize(180,38))); ts->Add(theme,0,wxALIGN_CENTER_VERTICAL|wxRIGHT,16);
        themeCard->SetSizer(ts); themeCard->SetMinSize(FromDIP(wxSize(-1,58)));
        theme->Bind(wxEVT_CHOICE,[settings](wxCommandEvent& e){
            const bool dark=e.GetSelection()==1;
            const wxColour bg=dark?wxColour(20,24,34):BG, fg=dark?wxColour(242,245,251):TEXTC, card=dark?wxColour(30,36,50):SURFACE;
            std::function<void(wxWindow*)> paint=[&](wxWindow* w){ w->SetBackgroundColour(card); w->SetForegroundColour(fg); for(auto* child:w->GetChildren()) paint(child); };
            paint(settings); settings->SetBackgroundColour(bg); settings->Refresh(); settings->Layout();
        });
        settings->GetSizer()->Add(themeCard,0,wxEXPAND|wxBOTTOM,10);
    }
 std::vector<wxPanel*>pp={camera,scenes,effects,recording,virtualCamera,devices,settings};for(auto*x:pp)pgs->Add(x,1,wxEXPAND);for(size_t i=1;i<pp.size();i++)pp[i]->Hide();for(size_t i=0;i<nav.size();i++)nav[i]->Bind(wxEVT_BUTTON,[pages,pp,nav,i](wxCommandEvent&){for(size_t j=0;j<pp.size();j++){pp[j]->Show(i==j);nav[j]->SetBackgroundColour(i==j?wxColour(238,239,255):CARD2);}pages->Layout();});
 statsText=new wxStaticText(bottom,wxID_ANY,U("60 FPS • 8.4 Mbps • 28 ms"));statsText->SetForegroundColour(MUTED);
 // Real engine-connected controls remain visible. No mock controls are hidden over the runtime.
 rotateLeftButton=B(pv,U("↶  Поворот"),wxID_ANY);
 rotateRightButton=B(pv,U("↷"),wxID_ANY);
 flipButton=B(pv,U("Зеркало"),wxID_ANY);
 flipVerticalButton=B(pv,U("⇵"),wxID_ANY);
 zoomOutButton=B(pv,U("−"),wxID_ANY);
 zoomInButton=B(pv,U("+"),wxID_ANY);
 torchButton=B(pv,U("ϟ  Фонарик"),wxID_ANY);
 swapButton=B(pv,U("⇄  Камера"),wxID_ANY);
 adjustmentsButton=B(pv,U("✦  Изображение"),wxID_ANY);
 zoomLevelLabel=T(pv,U("1.0×"),9,true,TEXTC);
 shell->SetSizer(hs);topsizer->Add(shell,1,wxEXPAND);
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
