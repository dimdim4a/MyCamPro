#include "gui/window.h"
#include "settings.h"

#include <wx/settings.h>
#include <wx/dcbuffer.h>
#include <algorithm>
#include <memory>
#include <vector>

namespace {
struct Palette {
    wxColour bg{248,249,253};
    wxColour surface{255,255,255};
    wxColour surface2{244,246,250};
    wxColour border{226,230,240};
    wxColour text{25,30,45};
    wxColour muted{103,112,132};
    wxColour primary{91,82,245};
    wxColour primarySoft{238,236,255};
    wxColour success{24,180,110};
    wxColour danger{239,76,91};
};

Palette LightPalette() { return Palette{}; }

Palette DarkPalette() {
    Palette p;
    p.bg = wxColour(15,17,23);
    p.surface = wxColour(23,26,34);
    p.surface2 = wxColour(30,34,44);
    p.border = wxColour(51,56,68);
    p.text = wxColour(242,244,249);
    p.muted = wxColour(158,166,182);
    p.primary = wxColour(118,106,255);
    p.primarySoft = wxColour(54,48,88);
    p.success = wxColour(43,202,133);
    p.danger = wxColour(255,91,107);
    return p;
}

wxBitmap MakeButtonBitmap(const wxSize& size, const wxString& label,
                          const Palette& p, bool primary, bool compact = false)
{
    wxBitmap bmp(size.x, size.y, 32);
    wxMemoryDC dc(bmp);
    const wxColour fill = primary ? p.primary : p.surface2;
    const wxColour fg = primary ? *wxWHITE : p.text;
    dc.SetBackground(wxBrush(fill));
    dc.Clear();
    dc.SetPen(wxPen(primary ? p.primary : p.border, 1));
    dc.SetBrush(wxBrush(fill));
    dc.DrawRoundedRectangle(wxRect(0, 0, size.x - 1, size.y - 1), 9);

    wxFont font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    font.SetPointSize(std::max(8, font.GetPointSize() + (compact ? 0 : 1)));
    font.SetWeight(primary ? wxFONTWEIGHT_SEMIBOLD : wxFONTWEIGHT_NORMAL);
    dc.SetFont(font);
    dc.SetTextForeground(fg);
    dc.DrawLabel(label, wxRect(8, 0, size.x - 16, size.y),
                 wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);
    dc.SelectObject(wxNullBitmap);
    return bmp;
}

wxButton* MakeButton(wxWindow* parent, const wxString& label, const Palette& p,
                     bool primary = false, const wxSize& logical = wxSize(120, 40))
{
    auto* b = new wxBitmapButton(parent, wxID_ANY,
        MakeButtonBitmap(parent->FromDIP(logical), label, p, primary),
        wxDefaultPosition, parent->FromDIP(logical), wxBORDER_NONE);
    b->SetBackgroundColour(parent->GetBackgroundColour());
    b->SetToolTip(label);
    return b;
}

wxStaticText* Label(wxWindow* parent, const wxString& text, const Palette& p,
                    int size = 10, bool bold = false)
{
    auto* t = new wxStaticText(parent, wxID_ANY, text);
    wxFont f = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    f.SetPointSize(size);
    f.SetWeight(bold ? wxFONTWEIGHT_SEMIBOLD : wxFONTWEIGHT_NORMAL);
    t->SetFont(f);
    t->SetForegroundColour(p.text);
    return t;
}

void SendMenu(wxWindow* host, int id, const wxString& value = wxEmptyString)
{
    wxCommandEvent e(wxEVT_MENU, id);
    e.SetEventObject(host);
    e.SetString(value);
    host->ProcessWindowEvent(e);
}

void TriggerButton(wxButton* button)
{
    if (!button) return;
    wxCommandEvent e(wxEVT_BUTTON, button->GetId());
    e.SetEventObject(button);
    button->Command(e);
}

void StyleChoice(wxChoice* c, const Palette& p)
{
    c->SetBackgroundColour(p.surface);
    c->SetForegroundColour(p.text);
    c->SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
}

void StyleCheck(wxCheckBox* c, const Palette& p)
{
    c->SetBackgroundColour(p.surface);
    c->SetForegroundColour(p.text);
}

void StylePanelTree(wxWindow* root, const Palette& p)
{
    if (!root) return;
    root->SetBackgroundColour(p.surface);
    if (auto* st = wxDynamicCast(root, wxStaticText))
        st->SetForegroundColour(p.text);
    if (auto* ch = wxDynamicCast(root, wxChoice))
        StyleChoice(ch, p);
    if (auto* cb = wxDynamicCast(root, wxCheckBox))
        StyleCheck(cb, p);
    for (auto* child : root->GetChildren())
        StylePanelTree(child, p);
}
}

Window::Window(Server::HostInfo hostinfo)
    : wxFrame(nullptr, wxID_ANY, "MyCam Pro", wxDefaultPosition, wxDefaultSize,
              wxDEFAULT_FRAME_STYLE)
{
    auto* root = new wxPanel(this, wxID_ANY);
    root->SetBackgroundColour(LightPalette().bg);
    auto* main = new wxBoxSizer(wxVERTICAL);

    wxIcon icon("res/nexora.ico", wxBITMAP_TYPE_ICO);
    taskbarIcon = new wxTaskBarIcon();
    taskbarIcon->SetIcon(icon, "MyCam Pro");
    taskbarIcon->Bind(wxEVT_TASKBAR_LEFT_DCLICK, &Window::MaximizeFromTaskbar, this);
    Bind(wxEVT_ICONIZE, &Window::MinimizeToTaskbar, this);
    SetIcon(icon);
    SetTitle("MyCam Pro");

    InitializeMenu(hostinfo);

    // Top app bar
    auto* top = new wxPanel(root, wxID_ANY);
    top->SetBackgroundColour(LightPalette().surface);
    auto* tr = new wxBoxSizer(wxHORIZONTAL);
    auto* brand = new wxBoxSizer(wxVERTICAL);
    auto* title = Label(top, "MYCAM PRO", LightPalette(), 15, true);
    auto* subtitle = Label(top, "Professional camera studio", LightPalette(), 9, false);
    subtitle->SetForegroundColour(LightPalette().muted);
    brand->Add(title, 0, wxLEFT, FromDIP(4));
    brand->Add(subtitle, 0, wxLEFT | wxTOP, FromDIP(4));
    tr->Add(brand, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(18));
    tr->AddStretchSpacer();

    statusText = Label(top, "●  Не подключено", LightPalette(), 10, true);
    statusText->SetForegroundColour(LightPalette().muted);
    tr->Add(statusText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(18));

    auto* recordTop = MakeButton(top, "●  REC", LightPalette(), true, wxSize(88, 38));
    recordTop->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { SendMenu(this, 200); });
    tr->Add(recordTop, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));

    auto* devicesTop = MakeButton(top, "Устройства", LightPalette(), false, wxSize(112, 38));
    devicesTop->SetId(MenuIDs::DEVICES);
    tr->Add(devicesTop, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(18));
    top->SetSizer(tr);
    tr->SetMinSize(-1, FromDIP(68));
    main->Add(top, 0, wxEXPAND);

    // Body
    auto* body = new wxBoxSizer(wxHORIZONTAL);
    auto* sidebar = new wxPanel(root, wxID_ANY);
    sidebar->SetBackgroundColour(LightPalette().surface);
    sidebar->SetMinSize(FromDIP(wxSize(222, -1)));
    auto* sr = new wxBoxSizer(wxVERTICAL);
    sr->AddSpacer(FromDIP(20));

    auto* sideCaption = Label(sidebar, "СТУДИЯ", LightPalette(), 9, true);
    sideCaption->SetForegroundColour(LightPalette().muted);
    sr->Add(sideCaption, 0, wxLEFT | wxBOTTOM, FromDIP(18));

    auto* book = new wxSimplebook(root, wxID_ANY);
    book->SetBackgroundColour(LightPalette().bg);

    struct NavItem { wxString title; wxString hint; int page; };
    const std::vector<NavItem> nav = {
        {"Камера", "Live preview", 0},
        {"Сцены", "Presets", 1},
        {"Изображение", "Correction", 2},
        {"Эффекты", "Filters", 3},
        {"Запись", "Video", 4},
        {"Устройства", "Phones", 5},
        {"Настройки", "System", 6}
    };

    std::vector<wxButton*> navButtons;
    for (size_t i = 0; i < nav.size(); ++i) {
        auto* b = MakeButton(sidebar, nav[i].title, LightPalette(),
                             i == 0, wxSize(190, 42));
        b->SetToolTip(nav[i].hint);
        const int page = nav[i].page;
        b->Bind(wxEVT_BUTTON, [book, page, navButtons](wxCommandEvent&) {
            book->ChangeSelection(page);
            wxUnusedVar(navButtons);
        });
        navButtons.push_back(b);
        sr->Add(b, 0, wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(10));
    }

    sr->AddStretchSpacer();
    auto* sideInfo = new wxPanel(sidebar, wxID_ANY);
    sideInfo->SetBackgroundColour(LightPalette().surface2);
    auto* sir = new wxBoxSizer(wxVERTICAL);
    sir->Add(Label(sideInfo, "Виртуальная камера", LightPalette(), 9, true),
             0, wxLEFT | wxTOP, FromDIP(12));
    auto* vc = Label(sideInfo, "MyCam Pro Camera", LightPalette(), 10, true);
    sir->Add(vc, 0, wxLEFT | wxTOP, FromDIP(12));
    auto* vcState = Label(sideInfo, "● Готова", LightPalette(), 9, false);
    vcState->SetForegroundColour(LightPalette().success);
    sir->Add(vcState, 0, wxLEFT | wxTOP | wxBOTTOM, FromDIP(12));
    sideInfo->SetSizer(sir);
    sr->Add(sideInfo, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(14));
    sidebar->SetSizer(sr);
    body->Add(sidebar, 0, wxEXPAND);

    // Camera page
    auto* cameraPage = new wxPanel(book, wxID_ANY);
    cameraPage->SetBackgroundColour(LightPalette().bg);
    auto* cameraRoot = new wxBoxSizer(wxHORIZONTAL);

    auto* center = new wxPanel(cameraPage, wxID_ANY);
    center->SetBackgroundColour(LightPalette().bg);
    auto* cr = new wxBoxSizer(wxVERTICAL);

    auto* deviceCard = new wxPanel(center, wxID_ANY);
    deviceCard->SetBackgroundColour(LightPalette().surface);
    auto* dcr = new wxBoxSizer(wxHORIZONTAL);
    dcr->Add(Label(deviceCard, "КАМЕРА", LightPalette(), 9, true),
             0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(12));
    sourceChoice = new wxChoice(deviceCard, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                1, new wxString[1]{ "Поиск устройств…" });
    sourceChoice->SetMinSize(FromDIP(wxSize(260, 38)));
    sourceChoice->SetSelection(0);
    StyleChoice(sourceChoice, LightPalette());
    dcr->Add(sourceChoice, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(14));

    statsText = Label(deviceCard, "1080p  •  30 fps  •  -- Mbps", LightPalette(), 9, false);
    statsText->SetForegroundColour(LightPalette().muted);
    statsText->Show(Settings::Get("SHOW_STATS") == 1);
    dcr->AddStretchSpacer();
    dcr->Add(statsText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(12));

    streamOptionsButton = MakeButton(deviceCard, "⚙", LightPalette(), false, wxSize(42, 38));
    streamOptionsButton->SetToolTip("Параметры потока");
    dcr->Add(streamOptionsButton, 0, wxALIGN_CENTER_VERTICAL);
    deviceCard->SetSizer(dcr);
    cr->Add(deviceCard, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    auto* previewCard = new wxPanel(center, wxID_ANY);
    previewCard->SetBackgroundColour(wxColour(10,12,18));
    auto* pr = new wxBoxSizer(wxVERTICAL);
    canvas = new Canvas(previewCard, wxDefaultPosition, FromDIP(wxSize(720, 440)));
    canvas->SetBackgroundColour(wxColour(10,12,18));
    pr->Add(canvas, 1, wxEXPAND | wxALL, FromDIP(1));
    previewCard->SetSizer(pr);
    cr->Add(previewCard, 1, wxEXPAND | wxBOTTOM, FromDIP(12));

    auto* quick = new wxPanel(center, wxID_ANY);
    quick->SetBackgroundColour(LightPalette().surface);
    auto* qr = new wxBoxSizer(wxHORIZONTAL);
    rotateLeftButton = MakeButton(quick, "↶", LightPalette(), false, wxSize(48, 42));
    rotateRightButton = MakeButton(quick, "↷", LightPalette(), false, wxSize(48, 42));
    flipButton = MakeButton(quick, "↔", LightPalette(), false, wxSize(48, 42));
    flipVerticalButton = MakeButton(quick, "↕", LightPalette(), false, wxSize(48, 42));
    zoomOutButton = MakeButton(quick, "−", LightPalette(), false, wxSize(48, 42));
    zoomLevelLabel = Label(quick, "1.0×", LightPalette(), 10, true);
    zoomLevelLabel->SetMinSize(FromDIP(wxSize(52, 42)));
    zoomInButton = MakeButton(quick, "+", LightPalette(), false, wxSize(48, 42));
    torchButton = MakeButton(quick, "☼", LightPalette(), false, wxSize(48, 42));
    swapButton = MakeButton(quick, "⇄", LightPalette(), false, wxSize(48, 42));
    snapshotButton = MakeButton(quick, "Фото", LightPalette(), true, wxSize(72, 42));
    adjustmentsButton = MakeButton(quick, "Изображение", LightPalette(), false, wxSize(112, 42));

    for (auto* b : {rotateLeftButton, rotateRightButton, flipButton, flipVerticalButton}) {
        qr->Add(b, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(5));
    }
    qr->AddSpacer(FromDIP(8));
    qr->Add(zoomOutButton, 0, wxALIGN_CENTER_VERTICAL);
    qr->Add(zoomLevelLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, FromDIP(2));
    qr->Add(zoomInButton, 0, wxALIGN_CENTER_VERTICAL);
    qr->AddStretchSpacer();
    qr->Add(torchButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(5));
    qr->Add(swapButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(5));
    qr->Add(snapshotButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(5));
    qr->Add(adjustmentsButton, 0, wxALIGN_CENTER_VERTICAL);
    quick->SetSizer(qr);
    cr->Add(quick, 0, wxEXPAND);

    auto* streamInfo = new wxPanel(center, wxID_ANY);
    streamInfo->SetBackgroundColour(LightPalette().surface);
    auto* sir2 = new wxBoxSizer(wxHORIZONTAL);
    auto* s1 = Label(streamInfo, "Wi‑Fi", LightPalette(), 9, true);
    auto* s2 = Label(streamInfo, " 1080p60", LightPalette(), 9, false);
    auto* s3 = Label(streamInfo, "  •  8 Mbps", LightPalette(), 9, false);
    auto* s4 = Label(streamInfo, "  •  28 ms", LightPalette(), 9, false);
    s1->SetForegroundColour(LightPalette().success);
    s2->SetForegroundColour(LightPalette().muted);
    s3->SetForegroundColour(LightPalette().muted);
    s4->SetForegroundColour(LightPalette().muted);
    sir2->Add(s1, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(12));
    sir2->Add(s2, 0, wxALIGN_CENTER_VERTICAL);
    sir2->Add(s3, 0, wxALIGN_CENTER_VERTICAL);
    sir2->Add(s4, 0, wxALIGN_CENTER_VERTICAL);
    sir2->AddStretchSpacer();
    sir2->Add(Label(streamInfo, "Виртуальная камера  •  1920×1080", LightPalette(), 9, false),
              0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(12));
    streamInfo->SetSizer(sir2);
    cr->Add(streamInfo, 0, wxEXPAND | wxTOP, FromDIP(12));

    center->SetSizer(cr);
    cameraRoot->Add(center, 1, wxEXPAND | wxRIGHT, FromDIP(12));

    // Right settings rail
    auto* right = new wxPanel(cameraPage, wxID_ANY);
    right->SetBackgroundColour(LightPalette().surface);
    right->SetMinSize(FromDIP(wxSize(310, -1)));
    auto* rr = new wxBoxSizer(wxVERTICAL);
    rr->Add(Label(right, "Настройки камеры", LightPalette(), 14, true),
            0, wxALL, FromDIP(18));

    auto addChoice = [&](const wxString& caption, const wxArrayString& values) {
        rr->Add(Label(right, caption, LightPalette(), 9, true),
                0, wxLEFT | wxRIGHT | wxTOP, FromDIP(18));
        auto* c = new wxChoice(right, wxID_ANY, wxDefaultPosition, FromDIP(wxSize(-1, 38)), values);
        c->SetSelection(0);
        StyleChoice(c, LightPalette());
        rr->Add(c, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(8));
        return c;
    };

    wxArrayString resolutions;
    resolutions.Add("1920 × 1080");
    resolutions.Add("1280 × 720");
    resolutions.Add("640 × 480");
    auto* resolutionChoice = addChoice("Разрешение", resolutions);
    resolutionChoice->Bind(wxEVT_CHOICE, [this](wxCommandEvent& e) {
        SendMenu(this, 201, e.GetString());
    });

    wxArrayString fpsValues;
    fpsValues.Add("60 FPS"); fpsValues.Add("30 FPS"); fpsValues.Add("24 FPS");
    auto* fpsChoice = addChoice("Частота кадров", fpsValues);
    fpsChoice->Bind(wxEVT_CHOICE, [this](wxCommandEvent& e) {
        SendMenu(this, 202, e.GetString());
    });

    wxArrayString bitrateValues;
    bitrateValues.Add("8 Mbps"); bitrateValues.Add("6 Mbps"); bitrateValues.Add("4 Mbps"); bitrateValues.Add("2 Mbps");
    auto* bitrateChoice = addChoice("Битрейт", bitrateValues);
    bitrateChoice->Bind(wxEVT_CHOICE, [this](wxCommandEvent& e) {
        SendMenu(this, 203, e.GetString());
    });

    rr->Add(Label(right, "Фокус", LightPalette(), 9, true),
            0, wxLEFT | wxRIGHT | wxTOP, FromDIP(18));
    wxArrayString focusValues;
    focusValues.Add("Авто"); focusValues.Add("Непрерывный"); focusValues.Add("Фиксированный");
    auto* focusChoice = new wxChoice(right, wxID_ANY, wxDefaultPosition, FromDIP(wxSize(-1, 38)), focusValues);
    focusChoice->SetSelection(0);
    StyleChoice(focusChoice, LightPalette());
    rr->Add(focusChoice, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(8));
    focusChoice->Bind(wxEVT_CHOICE, [this](wxCommandEvent& e) {
        SendMenu(this, 204, e.GetString());
    });

    auto* stabilization = new wxCheckBox(right, wxID_ANY, "Стабилизация");
    StyleCheck(stabilization, LightPalette());
    rr->Add(stabilization, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(18));
    stabilization->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& e) {
        SendMenu(this, 205, e.IsChecked() ? "1" : "0");
    });

    auto* h265 = new wxCheckBox(right, wxID_ANY, "HEVC / H.265");
    StyleCheck(h265, LightPalette());
    rr->Add(h265, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(12));
    h265->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& e) {
        SendMenu(this, 206, e.IsChecked() ? "1" : "0");
    });

    auto* advanced = MakeButton(right, "Расширенные параметры потока", LightPalette(), false, wxSize(260, 42));
    advanced->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { TriggerButton(streamOptionsButton); });
    rr->Add(advanced, 0, wxEXPAND | wxALL, FromDIP(18));
    rr->AddStretchSpacer();

    auto* note = Label(right, "Все изменения применяются к подключённому телефону сразу.", LightPalette(), 8, false);
    note->SetForegroundColour(LightPalette().muted);
    rr->Add(note, 0, wxALL, FromDIP(18));
    right->SetSizer(rr);
    cameraRoot->Add(right, 0, wxEXPAND);
    cameraPage->SetSizer(cameraRoot);

    // Scenes page
    auto* scenesPage = new wxPanel(book, wxID_ANY);
    scenesPage->SetBackgroundColour(LightPalette().bg);
    auto* scenesSizer = new wxBoxSizer(wxVERTICAL);
    scenesSizer->Add(Label(scenesPage, "Сцены", LightPalette(), 20, true), 0, wxALL, FromDIP(24));
    scenesSizer->Add(Label(scenesPage, "Быстрые пресеты для съёмки. Применяются одним нажатием.", LightPalette(), 10),
                     0, wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(24));
    for (const auto& scene : std::vector<std::pair<wxString,int>>{
        {"Студия • 1080p / 60 FPS", 210},
        {"Портрет • 1080p / 30 FPS", 211},
        {"Ночь • 720p / 30 FPS", 212},
        {"Презентация • 720p / 24 FPS", 213}})
    {
        auto* b = MakeButton(scenesPage, scene.first, LightPalette(), false, wxSize(520, 52));
        b->Bind(wxEVT_BUTTON, [this, id=scene.second](wxCommandEvent&) { SendMenu(this, id); });
        scenesSizer->Add(b, 0, wxLEFT | wxBOTTOM, FromDIP(12));
    }
    scenesSizer->AddStretchSpacer();
    scenesPage->SetSizer(scenesSizer);

    // Image page
    auto* imagePage = new wxPanel(book, wxID_ANY);
    imagePage->SetBackgroundColour(LightPalette().bg);
    auto* imageSizer = new wxBoxSizer(wxVERTICAL);
    imageSizer->Add(Label(imagePage, "Изображение", LightPalette(), 20, true), 0, wxALL, FromDIP(24));
    imageSizer->Add(Label(imagePage, "Коррекция цвета, экспозиции и резкости", LightPalette(), 10),
                    0, wxLEFT | wxBOTTOM, FromDIP(24));
    auto* openAdjust = MakeButton(imagePage, "Открыть редактор изображения", LightPalette(), true, wxSize(300, 48));
    openAdjust->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { TriggerButton(adjustmentsButton); });
    imageSizer->Add(openAdjust, 0, wxLEFT | wxBOTTOM, FromDIP(20));
    for (const auto& row : std::vector<wxString>{"Экспозиция", "Контраст", "Насыщенность", "Резкость"}) {
        imageSizer->Add(Label(imagePage, row, LightPalette(), 9, true), 0, wxLEFT | wxTOP, FromDIP(20));
        auto* slider = new wxSlider(imagePage, wxID_ANY, 50, 0, 100, wxDefaultPosition,
                                    FromDIP(wxSize(520, 34)), wxSL_HORIZONTAL);
        imageSizer->Add(slider, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(12));
    }
    imageSizer->AddStretchSpacer();
    imagePage->SetSizer(imageSizer);

    // Effects page
    auto* effectsPage = new wxPanel(book, wxID_ANY);
    effectsPage->SetBackgroundColour(LightPalette().bg);
    auto* effectsSizer = new wxBoxSizer(wxVERTICAL);
    effectsSizer->Add(Label(effectsPage, "Эффекты", LightPalette(), 20, true), 0, wxALL, FromDIP(24));
    effectsSizer->Add(Label(effectsPage, "Фильтры устройства доступны в реальном редакторе.", LightPalette(), 10),
                      0, wxLEFT | wxBOTTOM, FromDIP(20));
    for (const auto& effect : std::vector<wxString>{"Без фильтра", "Кино", "Ч/Б", "Винтаж"}) {
        auto* b = MakeButton(effectsPage, effect, LightPalette(), false, wxSize(300, 46));
        b->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { adjustmentsButton->Command(wxEVT_BUTTON); });
        effectsSizer->Add(b, 0, wxLEFT | wxBOTTOM, FromDIP(10));
    }
    effectsSizer->AddStretchSpacer();
    effectsPage->SetSizer(effectsSizer);

    // Recording page
    auto* recordingPage = new wxPanel(book, wxID_ANY);
    recordingPage->SetBackgroundColour(LightPalette().bg);
    auto* recordingSizer = new wxBoxSizer(wxVERTICAL);
    recordingSizer->Add(Label(recordingPage, "Запись", LightPalette(), 20, true), 0, wxALL, FromDIP(24));
    recordingSizer->Add(Label(recordingPage, "Видео сохраняется в Документы → MyCam Pro → Recordings.", LightPalette(), 10),
                        0, wxLEFT | wxBOTTOM, FromDIP(24));
    auto* recButton = MakeButton(recordingPage, "●  Начать запись", LightPalette(), true, wxSize(260, 54));
    recButton->Bind(wxEVT_BUTTON, [this, recButton](wxCommandEvent&) {
        SendMenu(this, 200);
        recButton->SetBitmapLabel(MakeButtonBitmap(recButton->GetSize(), "●  REC", LightPalette(), true));
    });
    recordingSizer->Add(recButton, 0, wxLEFT | wxBOTTOM, FromDIP(18));
    auto* snapshot = MakeButton(recordingPage, "Сделать снимок", LightPalette(), false, wxSize(260, 48));
    snapshot->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { TriggerButton(snapshotButton); });
    recordingSizer->Add(snapshot, 0, wxLEFT | wxBOTTOM, FromDIP(12));
    recordingSizer->AddStretchSpacer();
    recordingPage->SetSizer(recordingSizer);

    // Devices page
    auto* devicesPage = new wxPanel(book, wxID_ANY);
    devicesPage->SetBackgroundColour(LightPalette().bg);
    auto* devicesSizer = new wxBoxSizer(wxVERTICAL);
    devicesSizer->Add(Label(devicesPage, "Устройства", LightPalette(), 20, true), 0, wxALL, FromDIP(24));
    devicesSizer->Add(Label(devicesPage, "Подключённые телефоны и Wi‑Fi соединение.", LightPalette(), 10),
                      0, wxLEFT | wxBOTTOM, FromDIP(24));
    auto* showDevices = MakeButton(devicesPage, "Управление устройствами", LightPalette(), true, wxSize(300, 48));
    showDevices->SetId(MenuIDs::DEVICES);
    devicesSizer->Add(showDevices, 0, wxLEFT | wxBOTTOM, FromDIP(18));
    auto* showQr = MakeButton(devicesPage, "Показать QR для подключения", LightPalette(), false, wxSize(300, 48));
    showQr->SetId(MenuIDs::QR);
    devicesSizer->Add(showQr, 0, wxLEFT | wxBOTTOM, FromDIP(12));
    devicesSizer->AddStretchSpacer();
    devicesPage->SetSizer(devicesSizer);

    // Settings page
    auto* settingsPage = new wxPanel(book, wxID_ANY);
    settingsPage->SetBackgroundColour(LightPalette().bg);
    auto* settingsSizer = new wxBoxSizer(wxVERTICAL);
    settingsSizer->Add(Label(settingsPage, "Настройки", LightPalette(), 20, true), 0, wxALL, FromDIP(24));
    settingsSizer->Add(Label(settingsPage, "Внешний вид и поведение MyCam Pro", LightPalette(), 10),
                       0, wxLEFT | wxBOTTOM, FromDIP(24));
    settingsSizer->Add(Label(settingsPage, "Тема", LightPalette(), 9, true), 0, wxLEFT | wxTOP, FromDIP(20));
    wxArrayString themes; themes.Add("Светлая"); themes.Add("Тёмная"); themes.Add("Системная");
    auto* themeChoice = new wxChoice(settingsPage, wxID_ANY, wxDefaultPosition, FromDIP(wxSize(300, 40)), themes);
    themeChoice->SetSelection(0);
    StyleChoice(themeChoice, LightPalette());
    settingsSizer->Add(themeChoice, 0, wxLEFT | wxTOP, FromDIP(10));
    auto* showStats = new wxCheckBox(settingsPage, wxID_ANY, "Показывать статистику потока");
    showStats->SetValue(Settings::Get("SHOW_STATS") == 1);
    StyleCheck(showStats, LightPalette());
    settingsSizer->Add(showStats, 0, wxLEFT | wxTOP, FromDIP(22));
    showStats->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& e) {
        Settings::Set("SHOW_STATS", e.IsChecked() ? 1 : 0);
        statsText->Show(e.IsChecked());
        Layout();
    });
    auto* tray = new wxCheckBox(settingsPage, wxID_ANY, "Сворачивать в трей");
    tray->SetValue(Settings::Get("MINIMIZE_TASKBAR") == 1);
    StyleCheck(tray, LightPalette());
    settingsSizer->Add(tray, 0, wxLEFT | wxTOP, FromDIP(12));
    tray->Bind(wxEVT_CHECKBOX, [](wxCommandEvent& e) {
        Settings::Set("MINIMIZE_TASKBAR", e.IsChecked() ? 1 : 0);
    });
    settingsSizer->AddStretchSpacer();
    settingsPage->SetSizer(settingsSizer);

    book->AddPage(cameraPage, "Камера", true);
    book->AddPage(scenesPage, "Сцены");
    book->AddPage(imagePage, "Изображение");
    book->AddPage(effectsPage, "Эффекты");
    book->AddPage(recordingPage, "Запись");
    book->AddPage(devicesPage, "Устройства");
    book->AddPage(settingsPage, "Настройки");

    // Keep navigation and simplebook together.
    body->Add(book, 1, wxEXPAND | wxALL, FromDIP(14));
    root->SetSizer(main);
    main->Add(body, 1, wxEXPAND);

    // Theme is a real UI preference, not a fake dropdown. It immediately restyles
    // all native controls; the video preview intentionally remains dark.
    themeChoice->Bind(wxEVT_CHOICE, [root, book](wxCommandEvent& e) {
        Palette p = e.GetSelection() == 1 ? DarkPalette() : LightPalette();
        root->SetBackgroundColour(p.bg);
        book->SetBackgroundColour(p.bg);
        StylePanelTree(root, p);
        root->Refresh(true);
        root->Layout();
    });

    // Buttons with existing engine bindings must keep their original wx IDs/events.
    devicesTop->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        wxCommandEvent e(wxEVT_BUTTON, MenuIDs::DEVICES);
        e.SetEventObject(this);
        ProcessWindowEvent(e);
    });

    Layout();
    SetMinClientSize(FromDIP(wxSize(1120, 700)));
    SetClientSize(FromDIP(wxSize(1440, 900)));
    Center();
}

Window::~Window() { delete taskbarIcon; }

void Window::InitializeMenu(Server::HostInfo hostinfo)
{
    auto* menuBar = new wxMenuBar();
    auto* app = new wxMenu();
    auto* tray = app->AppendCheckItem(MenuIDs::HIDE2TRAY, "Сворачивать в трей");
    tray->Check(Settings::Get("MINIMIZE_TASKBAR") == 1);
    auto* stats = app->AppendCheckItem(MenuIDs::SHOWSTATS, "Показывать статистику");
    stats->Check(Settings::Get("SHOW_STATS") == 1);
    auto* save = app->AppendCheckItem(MenuIDs::SAVESTATE, "Сохранять настройки устройства");
    save->Check(Settings::Get("SAVE_DEVICE_STATE") == 1);

    auto* resolutions = new wxMenu();
    resolutions->AppendRadioItem(MenuIDs::DS_SD, "640 × 480");
    resolutions->AppendRadioItem(MenuIDs::DS_HD, "1280 × 720");
    resolutions->AppendRadioItem(MenuIDs::DS_FHD, "1920 × 1080");
    resolutions->AppendRadioItem(MenuIDs::DS_QHD, "3840 × 2160");
    const auto selected = Settings::Get("DIRECTSHOW_RESOLUTION");
    resolutions->Check((selected != -1 ? selected : 0) + MenuIDs::DS_SD, true);
    app->AppendSubMenu(resolutions, "Разрешение виртуальной камеры");
    app->AppendSeparator();
    app->Append(wxID_EXIT, "Выход");
    menuBar->Append(app, "MyCam Pro");

    auto* connection = new wxMenu();
    connection->Append(MenuIDs::QR, "QR-код подключения");
    connection->Append(MenuIDs::DEVICES, "Устройства");
    connection->AppendSeparator();
    connection->Append(wxID_ANY, "Адрес: " + std::get<1>(hostinfo));
    connection->Append(wxID_ANY, "Порт: " + std::get<2>(hostinfo));
    menuBar->Append(connection, "Подключение");
    SetMenuBar(menuBar);
}

void Window::InitializeHeader(wxPanel* parent, wxBoxSizer* topsizer)
{
    wxUnusedVar(parent);
    wxUnusedVar(topsizer);
}

void Window::InitializeTopBar(wxPanel* parent, wxBoxSizer* topsizer)
{
    wxUnusedVar(parent);
    wxUnusedVar(topsizer);
}

void Window::InitializeCanvasPanel(wxPanel* parent, wxBoxSizer* topsizer)
{
    wxUnusedVar(parent);
    wxUnusedVar(topsizer);
}

void Window::InitializeBottomBar(wxPanel* parent, wxBoxSizer* topsizer)
{
    wxUnusedVar(parent);
    wxUnusedVar(topsizer);
}

void Window::SetConnectionStatus(bool connected, const wxString& deviceName)
{
    if (!statusText) return;
    statusText->SetLabel(connected ? wxString("●  ") + deviceName
                                   : wxString("●  Не подключено"));
    statusText->SetForegroundColour(connected ? LightPalette().success
                                               : LightPalette().muted);
    statusText->GetParent()->Layout();
}

void Window::MinimizeToTaskbar(wxIconizeEvent& evt)
{
    if (Settings::Get("MINIMIZE_TASKBAR") == 1) {
        Hide();
        evt.Skip();
        return;
    }
    evt.Skip();
}

void Window::MaximizeFromTaskbar(wxTaskBarIconEvent&)
{
    Iconize(false);
    Show();
    Raise();
    SetFocus();
}

Canvas* Window::GetCanvas() { return canvas; }
wxChoice* Window::GetSourceChoice() { return sourceChoice; }
wxButton* Window::GetStreamOptionsButton() { return streamOptionsButton; }
wxButton* Window::GetRotateLeftButton() { return rotateLeftButton; }
wxButton* Window::GetRotateRightButton() { return rotateRightButton; }
wxButton* Window::GetFlipButton() { return flipButton; }
wxButton* Window::GetFlipVerticalButton() { return flipVerticalButton; }
wxButton* Window::GetZoomInButton() { return zoomInButton; }
wxButton* Window::GetZoomOutButton() { return zoomOutButton; }
wxStaticText* Window::GetZoomLevelLabel() { return zoomLevelLabel; }
wxButton* Window::GetTorchButton() { return torchButton; }
wxButton* Window::GetSwapButton() { return swapButton; }
wxButton* Window::GetAdjustmentsButton() { return adjustmentsButton; }
wxButton* Window::GetSnapshotButton() { return snapshotButton; }
wxStaticText* Window::GetStatsText() { return statsText; }
wxTaskBarIcon* Window::GetTaskbarIcon() { return taskbarIcon; }
