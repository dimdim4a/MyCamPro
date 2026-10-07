#include "gui/window.h"
#include "settings.h"

#include <wx/settings.h>
#include <wx/statline.h>
#include <algorithm>

namespace {
const wxColour kBg(7, 11, 18);
const wxColour kPanel(15, 22, 34);
const wxColour kPanel2(20, 29, 44);
const wxColour kPanel3(25, 36, 54);
const wxColour kPrimary(64, 130, 255);
const wxColour kPrimaryHover(84, 148, 255);
const wxColour kAccent(43, 214, 176);
const wxColour kText(244, 247, 252);
const wxColour kMuted(158, 171, 194);
const wxColour kBorder(43, 57, 79);

wxBitmap LoadWhiteIcon(const wxString& icon)
{
    wxImage image(wxString("res/") + icon, wxBITMAP_TYPE_PNG);
    if (!image.IsOk()) return wxNullBitmap;
    if (!image.HasAlpha()) image.InitAlpha();
    auto* pixels = image.GetData();
    const auto count = static_cast<size_t>(image.GetWidth()) * image.GetHeight() * 3;
    for (size_t i = 0; i < count; ++i) pixels[i] = 255;
    return wxBitmap(image);
}

wxButton* MakeIconButton(wxWindow* parent, const wxString& icon, const wxString& tooltip)
{
    auto* b = new wxBitmapButton(parent, wxID_ANY, LoadWhiteIcon(icon),
        wxDefaultPosition, parent->FromDIP(wxSize(42, 42)), wxBORDER_NONE);
    b->SetBackgroundColour(kPanel3);
    b->SetForegroundColour(kText);
    b->SetMinSize(parent->FromDIP(wxSize(42, 42)));
    b->SetToolTip(tooltip);
    return b;
}

wxButton* MakeActionButton(wxWindow* parent, const wxString& label, int id, bool primary = false)
{
    auto* b = new wxButton(parent, id, label, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    b->SetMinSize(parent->FromDIP(wxSize(primary ? 108 : 100, 38)));
    b->SetBackgroundColour(primary ? kPrimary : kPanel3);
    b->SetForegroundColour(kText);
    return b;
}

wxStaticText* MakeLabel(wxWindow* parent, const wxString& text, int size, bool bold = false)
{
    auto* l = new wxStaticText(parent, wxID_ANY, text);
    auto f = l->GetFont();
    f.SetPointSize(size);
    f.SetWeight(bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
    l->SetFont(f);
    l->SetForegroundColour(kText);
    return l;
}
}

Window::Window(Server::HostInfo hostinfo)
    : wxFrame(nullptr, wxID_ANY, "MyCam Pro Studio", wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE)
{
    SetBackgroundColour(kBg);

    auto* panel = new wxPanel(this, wxID_ANY);
    panel->SetBackgroundColour(kBg);
    panel->SetForegroundColour(kText);

    auto* topsizer = new wxBoxSizer(wxVERTICAL);

    wxIcon icon("res/nexora.ico", wxBITMAP_TYPE_ICO);
    taskbarIcon = new wxTaskBarIcon();
    taskbarIcon->SetIcon(icon, "MyCam Pro Studio");
    taskbarIcon->Bind(wxEVT_TASKBAR_LEFT_DCLICK, &Window::MaximizeFromTaskbar, this);
    Bind(wxEVT_ICONIZE, &Window::MinimizeToTaskbar, this);
    SetIcon(icon);

    InitializeMenu(hostinfo);
    InitializeHeader(panel, topsizer);
    InitializeTopBar(panel, topsizer);
    InitializeCanvasPanel(panel, topsizer);
    InitializeBottomBar(panel, topsizer);

    panel->SetSizer(topsizer);
    auto* frameSizer = new wxBoxSizer(wxVERTICAL);
    frameSizer->Add(panel, 1, wxEXPAND);
    SetSizer(frameSizer);

    SetMinClientSize(FromDIP(wxSize(900, 600)));
    SetClientSize(FromDIP(wxSize(1180, 760)));
    Layout();
    panel->Layout();
    CallAfter([this, panel]() {
        Layout();
        panel->Layout();
        panel->Refresh();
    });
    Center();
}

Window::~Window() { delete taskbarIcon; }

void Window::InitializeMenu(Server::HostInfo hostinfo)
{
    auto* menuBar = new wxMenuBar();
    auto* file = new wxMenu();

    auto* c1 = file->AppendCheckItem(MenuIDs::HIDE2TRAY, "Minimize to tray");
    c1->Check(Settings::Get("MINIMIZE_TASKBAR") == 1);
    auto* c2 = file->AppendCheckItem(MenuIDs::SHOWSTATS, "Show stream statistics");
    c2->Check(Settings::Get("SHOW_STATS") == 1);
    auto* c3 = file->AppendCheckItem(MenuIDs::SAVESTATE, "Remember device settings");
    c3->Check(Settings::Get("SAVE_DEVICE_STATE") == 1);

    auto* resolutions = new wxMenu();
    resolutions->AppendRadioItem(MenuIDs::DS_SD, "640 x 480", "Standard 4:3");
    resolutions->AppendRadioItem(MenuIDs::DS_HD, "1280 x 720", "HD");
    resolutions->AppendRadioItem(MenuIDs::DS_FHD, "1920 x 1080", "Full HD");
    resolutions->AppendRadioItem(MenuIDs::DS_QHD, "3840 x 2160", "4K UHD");
    auto selected = Settings::Get("DIRECTSHOW_RESOLUTION");
    resolutions->Check((selected != -1 ? selected : 0) + MenuIDs::DS_SD, true);
    file->AppendSubMenu(resolutions, "Virtual camera resolution", "Requires restart");
    file->AppendSeparator();
    file->Append(wxID_EXIT, "Exit");
    menuBar->Append(file, "MyCam Pro");

    auto* connect = new wxMenu();
    connect->Append(MenuIDs::QR, "Show QR code");
    connect->Append(MenuIDs::DEVICES, "Connected devices");
    connect->AppendSeparator();
    connect->Append(wxID_ANY, "Address: " + std::get<1>(hostinfo));
    connect->Append(wxID_ANY, "Port: " + std::get<2>(hostinfo));
    menuBar->Append(connect, "Connection");
    SetMenuBar(menuBar);
}

void Window::InitializeHeader(wxPanel* parent, wxBoxSizer* topsizer)
{
    auto* header = new wxPanel(parent, wxID_ANY);
    header->SetBackgroundColour(kPanel);

    auto* row = new wxBoxSizer(wxHORIZONTAL);
    auto* brand = new wxBoxSizer(wxVERTICAL);

    auto* title = MakeLabel(header, "MYCAM PRO", 18, true);
    title->SetForegroundColour(kText);

    auto* tagline = MakeLabel(header,
        wxString::FromUTF8("Телефон как веб-камера  •  Wi-Fi / USB  •  OBS"),
        9, false);
    tagline->SetForegroundColour(kMuted);

    brand->Add(title, 0, wxBOTTOM, FromDIP(2));
    brand->Add(tagline, 0);

    row->Add(brand, 0, wxALIGN_CENTER_VERTICAL);
    row->AddStretchSpacer();

    statusText = new wxStaticText(header, wxID_ANY,
        wxString::FromUTF8("●  ОФЛАЙН  •  Ожидание телефона"),
        wxDefaultPosition, FromDIP(wxSize(270, 34)),
        wxALIGN_CENTER);
    statusText->SetBackgroundColour(kPanel3);
    statusText->SetForegroundColour(kMuted);
    auto sf = statusText->GetFont();
    sf.SetPointSize(9);
    sf.SetWeight(wxFONTWEIGHT_BOLD);
    statusText->SetFont(sf);
    row->Add(statusText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(10));

    auto* qrButton = MakeActionButton(header, wxString::FromUTF8("QR-КОД"), MenuIDs::QR, true);
    qrButton->SetMinSize(FromDIP(wxSize(108, 38)));
    qrButton->SetToolTip(wxString::FromUTF8("Показать QR-код для подключения"));
    row->Add(qrButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(6));

    auto* devicesButton = MakeActionButton(header, wxString::FromUTF8("УСТРОЙСТВА"), MenuIDs::DEVICES);
    devicesButton->SetMinSize(FromDIP(wxSize(118, 38)));
    row->Add(devicesButton, 0, wxALIGN_CENTER_VERTICAL);

    row->InsertSpacer(0, FromDIP(22));
    row->AddSpacer(FromDIP(22));
    row->SetMinSize(-1, FromDIP(78));

    header->SetSizer(row);
    topsizer->Add(header, 0, wxEXPAND);
}

void Window::InitializeTopBar(wxPanel* parent, wxBoxSizer* topsizer)
{
    auto* card = new wxPanel(parent, wxID_ANY);
    card->SetBackgroundColour(kPanel);
    auto* row = new wxBoxSizer(wxHORIZONTAL);

    auto* label = MakeLabel(card, wxString::FromUTF8("ИСТОЧНИК"), 9, true);
    label->SetForegroundColour(kMuted);
    row->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(14));

    wxString choices[1] = { wxString::FromUTF8("Устройства не найдены") };
    sourceChoice = new wxChoice(card, wxID_ANY, wxDefaultPosition, wxDefaultSize, 1, choices);
    sourceChoice->SetMinSize(FromDIP(wxSize(240, 38)));
    sourceChoice->SetSelection(0);
    row->Add(sourceChoice, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(12));

    statsText = new wxStaticText(card, wxID_ANY, "-- x --  |  -- fps  |  -- Mbps",
        wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
    statsText->SetForegroundColour(kAccent);
    statsText->Show(Settings::Get("SHOW_STATS") == 1);
    row->Add(statsText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(12));

    streamOptionsButton = MakeIconButton(card, "setting.png",
        wxString::FromUTF8("Настройки потока"));
    row->Add(streamOptionsButton, 0, wxALIGN_CENTER_VERTICAL);

    row->InsertSpacer(0, FromDIP(16));
    row->AddSpacer(FromDIP(16));
    row->SetMinSize(-1, FromDIP(62));

    card->SetSizer(row);
    topsizer->Add(card, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(14));
}

void Window::InitializeCanvasPanel(wxPanel* parent, wxBoxSizer* topsizer)
{
    auto* frame = new wxPanel(parent, wxID_ANY);
    frame->SetBackgroundColour(kPanel);
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    canvas = new Canvas(frame, wxDefaultPosition, FromDIP(wxSize(640, 360)));
    canvas->SetMinSize(FromDIP(wxSize(320, 180)));
    canvas->SetBackgroundColour(wxColour(2, 5, 10));

    sizer->Add(canvas, 1, wxEXPAND | wxALL, FromDIP(8));
    frame->SetSizer(sizer);

    topsizer->Add(frame, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(14));
}

void Window::InitializeBottomBar(wxPanel* parent, wxBoxSizer* topsizer)
{
    auto* bar = new wxPanel(parent, wxID_ANY);
    bar->SetBackgroundColour(kPanel);
    auto* row = new wxBoxSizer(wxHORIZONTAL);

    rotateLeftButton = MakeIconButton(bar, "rotate-left.png", wxString::FromUTF8("Повернуть влево"));
    rotateRightButton = MakeIconButton(bar, "rotate-right.png", wxString::FromUTF8("Повернуть вправо"));
    flipButton = MakeIconButton(bar, "flip.png", wxString::FromUTF8("Отразить по горизонтали"));
    flipVerticalButton = MakeIconButton(bar, "flip-v.png", wxString::FromUTF8("Отразить по вертикали"));

    zoomOutButton = MakeIconButton(bar, "zoom-out.png", wxString::FromUTF8("Уменьшить"));
    zoomLevelLabel = new wxStaticText(bar, wxID_ANY, "1.0x",
        wxDefaultPosition, FromDIP(wxSize(50, 42)), wxALIGN_CENTER);
    zoomLevelLabel->SetBackgroundColour(kPanel3);
    zoomLevelLabel->SetForegroundColour(kText);
    zoomInButton = MakeIconButton(bar, "zoom-in.png", wxString::FromUTF8("Увеличить"));

    torchButton = MakeIconButton(bar, "flash.png", wxString::FromUTF8("Вспышка"));
    swapButton = MakeIconButton(bar, "swap.png", wxString::FromUTF8("Переключить камеру"));
    snapshotButton = MakeIconButton(bar, "photo.png", wxString::FromUTF8("Снимок"));
    adjustmentsButton = MakeIconButton(bar, "settings.png", wxString::FromUTF8("Настройки изображения"));

    for (auto* b : { rotateLeftButton, rotateRightButton, flipButton, flipVerticalButton })
        row->Add(b, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(5));

    row->AddStretchSpacer();

    row->Add(zoomOutButton, 0, wxALIGN_CENTER_VERTICAL);
    row->Add(zoomLevelLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, FromDIP(4));
    row->Add(zoomInButton, 0, wxALIGN_CENTER_VERTICAL);

    row->AddStretchSpacer();

    for (auto* b : { torchButton, swapButton, snapshotButton, adjustmentsButton })
        row->Add(b, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(5));

    row->InsertSpacer(0, FromDIP(10));
    row->AddSpacer(FromDIP(10));
    row->SetMinSize(-1, FromDIP(58));

    bar->SetSizer(row);
    topsizer->Add(bar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, FromDIP(14));
}

void Window::SetConnectionStatus(bool connected, const wxString& deviceName)
{
    if (connected)
    {
        statusText->SetLabel(wxString::FromUTF8("●  ОНЛАЙН  •  ") + deviceName);
        statusText->SetForegroundColour(kAccent);
    }
    else
    {
        statusText->SetLabel(wxString::FromUTF8("●  ОФЛАЙН  •  Ожидание телефона"));
        statusText->SetForegroundColour(kMuted);
    }
    statusText->GetParent()->Layout();
}

void Window::MinimizeToTaskbar(wxIconizeEvent& evt)
{
    if (Settings::Get("MINIMIZE_TASKBAR") == 1) { Hide(); evt.Skip(); }
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
